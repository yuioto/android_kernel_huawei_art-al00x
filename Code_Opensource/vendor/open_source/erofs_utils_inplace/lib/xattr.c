/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * erofs_utils/lib/xattr.c
 *
 * Copyright (C) 2018-2019 HUAWEI, Inc.
 *             http://www.huawei.com/
 * Created by Li Guifu <bluce.liguifu@huawei.com>
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "erofs/xattr.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include <inttypes.h>
#include <libgen.h>
#include <limits.h>
#include <selinux/selinux.h>
#include <selinux/label.h>

#if defined(ANDROID) && !defined(CONFIG_EROFS_READ_XATTR_FROM_DIR)
#include <private/android_filesystem_capability.h>
#endif

#include "erofs/print.h"
#include "erofs/inode.h"

LIST_HEAD(h_xattr_def);
struct list_head *h_xattr = &h_xattr_def;

static struct erofs_xattr_info *alloc_erofs_xattr(void)
{
	struct erofs_xattr_info *xi;

	xi = (struct erofs_xattr_info *)calloc(sizeof(struct erofs_xattr_info), 1);
	if (!xi) {
		erofs_err("calloc failed");
		return NULL;
	}

	init_list_head(&xi->list);
	return xi;
}

static struct erofs_xattr_ref *alloc_erofs_xattr_ref(void)
{
	struct erofs_xattr_ref *ref;

	ref = (struct erofs_xattr_ref *)calloc(sizeof(struct erofs_xattr_ref), 1);
	if (!ref) {
		return NULL;
	}

	init_list_head(&ref->list);
	return ref;
}

struct erofs_xattr_info *xattr_lookup(int index, char *name, void *value, ssize_t vallen)
{
	struct erofs_xattr_info *pxi;
	int namelen = strlen(name);

	list_for_each_entry(pxi, h_xattr, list) {
		if (index == pxi->e_name_index &&
		    vallen == pxi->e_value_size &&
		    namelen   == pxi->e_name_len &&
		    memcmp(name, pxi->name, namelen) == 0 &&
		    memcmp(value, pxi->value, vallen) == 0) {
			return pxi;
		}
	}

	return NULL;
}

struct erofs_xattr_info *new_xattr_info(int e_name_index, char *name,
					void *value, ssize_t vallen)
{
	struct erofs_xattr_info *xi;

	xi = alloc_erofs_xattr();
	if (!xi)
		return NULL;

	xi->e_name_len   = strlen(name);
	xi->e_name_index = e_name_index;
	xi->e_value_size = vallen;

	xi->name = strdup(name);
	if (!xi->name) {
		erofs_err("strdup failed error[%s]", strerror(errno));
		goto EXIT_NAME;
	}

	xi->value = malloc(vallen);
	if (!xi->value) {
		erofs_err("malloc failed error[%s]", strerror(errno));
		goto EXIT_VALUE;
	}
	memcpy(xi->value, value, vallen);
	xi->refcount = 1;
	return xi;

EXIT_VALUE:
	free(xi->name);
EXIT_NAME:
	free(xi);
	return NULL;
}

int __add_xattr(struct list_head *head, struct erofs_xattr_info *xattr)
{
	struct erofs_xattr_ref *ref = alloc_erofs_xattr_ref();
	if (!ref)
		return -EINVAL;

	ref->xattr = xattr;
	list_add_tail(&ref->list, head);
	return 0;
}

int xattr_find_or_add(int index, char *name, void *value, size_t size,
		      struct list_head *xattr_head, bool update)
{
	int ret = 0;
	struct erofs_xattr_info *xi;

	xi = xattr_lookup(index, name, value, size);
	if (!xi) {
		if (!update) {
			erofs_err("xattr_lookup() not found update[%d] name[%s] value[%s] size[%u]",
				  update, name, (char *)value, (unsigned int)size);
			ret = -EINVAL;
			goto exit;
		}

		xi = new_xattr_info(index, name, value, size);
		if (!xi) {
			ret = -ENOMEM;
			goto exit;
		}

		list_add_tail(&xi->list, h_xattr);
	} else {
		if (update)
			xi->refcount++;
	}

	/* storage selinux xattr to inode list */
	if (xattr_head)
		__add_xattr(xattr_head, xi);

exit:
	return ret;

}

#ifdef CONFIG_EROFS_READ_XATTR_FROM_DIR
struct XATTR_IDX{
	const char *name;
	unsigned int index;
} xattr_table[] = {
	{EROFS_XATTR_PREFIX_USER,             EROFS_XATTR_INDEX_USER},
	{EROFS_XATTR_SYSTEM_ACL_ACCESS_NAME,  EROFS_XATTR_INDEX_POSIX_ACL_ACCESS},
	{EROFS_XATTR_SYSTEM_ACL_DEFAULT_NAME, EROFS_XATTR_INDEX_POSIX_ACL_DEFAULT},
	{EROFS_XATTR_PREFIX_TRUSTED,          EROFS_XATTR_INDEX_TRUSTED},
	{EROFS_XATTR_PREFIX_LUSTRE,           EROFS_XATTR_INDEX_LUSTRE},
	{EROFS_XATTR_PREFIX_SECURITY,         EROFS_XATTR_INDEX_SECURITY},
	{EROFS_XATTR_SYSTEM_ADVISE_NAME,      EROFS_XATTR_INDEX_ADVISE},
	{NULL, 0},
};

static int get_xattr_type(const char *name, unsigned *prefix_len)
{
	struct XATTR_IDX *idx = &xattr_table[0];
	unsigned int len = strlen(name);

	while (idx->name != NULL) {
		unsigned int idx_len = strlen(idx->name);

		if ((len >= idx_len) && !strncmp(name, idx->name, idx_len)) {
			*prefix_len = idx_len;
			return idx->index;
		}
		idx++;
	}

	erofs_err("need add support name[%s]", name);
	*prefix_len = 0;
	return -EINVAL;
}

int erofs_lookup_xattr_by_path(struct erofs_inode *inode,
			       struct list_head *xattr_head, bool update)
{
	int ret = 0;
	ssize_t buflen, keylen, vallen;
	char *buf = NULL;
	char *key = NULL;
	const char *path = inode->i_srcpath;

	/*
	 * On success, a non-negative number is returned indicating the size of
	 * the extended attribute name list. On failure, -1 is returned and
	 * errno is set appropriately
	 */
	buflen = llistxattr(path, NULL, 0);
	if (buflen < 0) {
		erofs_err("buflen is negtive when get xattr lenght path=%s errno:%s",
			  path, strerror(errno));
		return -errno;
	} else if (buflen == 0) {
		return 0;
	}

	buf = malloc(buflen);
	if (!buf) {
		erofs_err("buf is NULL");
		return -ENOMEM;
	}

	buflen = llistxattr(path, buf, buflen);
	if (buflen < 0) {
		erofs_err("buflen is negtive when loop xattr key path=%s", path);
		ret = -errno;
		goto err_freebuf;
	}

	key = buf;
	while (buflen > 0) {
		/*
		 * lgetxattr
		 * On success, a positive number is returned indicating the size of
		 * the extended attribute value. On failure, -1 is returned and
		 * errno is set appropriately
		 */
		vallen = lgetxattr(path, key, NULL, 0);
		if (vallen < 0) {
			erofs_err("lgetxattr() failed [%s]", path);
			goto err_freebuf;
		} else if (vallen == 0) {
			erofs_info("inode->i_srcpath[%s] <no value>", path);
			goto next_value;
		}

		uint32_t prefix_len = 0;
		char *val = malloc(vallen + 1);
		if (!val) {
			erofs_err("malloc() failed vallen=%lu", (long)vallen);
			ret = -errno;
			goto err_freebuf;
		}
		vallen = lgetxattr(path, key, val, vallen);
		if (vallen < 0) {
			erofs_err("lgetxattr() failed [%s]", path);
			free(val);
			ret = -errno;
			goto err_freebuf;
		} else {
			val[vallen] = 0;
		}

		int name_index = get_xattr_type(key, &prefix_len);
		/* share xattr reference count */
		if (name_index < 0) {
			ret = -EINVAL;
			free(val);
			goto err_freebuf;
		}

		char *key_name = key + prefix_len;
		ret = xattr_find_or_add(name_index, key_name, val, vallen,
					xattr_head, update);
		if (ret < 0) {
			free(val);
			goto err_freebuf;
		}

		free(val);
		val = NULL;

next_value:
		keylen = strlen(key) + 1;
		buflen -= keylen;
		key += keylen;
	}

	free(buf);
	return 0;

err_freebuf:
	free(buf);
	return ret;

}
#else //CONFIG_EROFS_READ_XATTR_FROM_DIR

static int inode_set_capabilities(uint64_t capabilities,
				  struct list_head *xattr_head, bool update)
{
	struct vfs_cap_data cap_data;

	if (capabilities == 0)
		return 0;

	memset(&cap_data, 0, sizeof(cap_data));

	cap_data.magic_etc = VFS_CAP_REVISION | VFS_CAP_FLAGS_EFFECTIVE;
	cap_data.data[0].permitted = (uint32_t)(capabilities & 0xffffffff);
	cap_data.data[0].inheritable = 0;
	cap_data.data[1].permitted = (uint32_t)(capabilities >> 32);
	cap_data.data[1].inheritable = 0;

	return xattr_find_or_add(EROFS_XATTR_INDEX_SECURITY, XATTR_CAPS_SUFFIX,
				 &cap_data, sizeof(cap_data), xattr_head, update);

}

static int adr_get_xattr_by_path(const char *mnt_path, int mode,
				 struct list_head *xattr_head, bool update)
{
	int ret = 0;
	char *secontext = NULL;
	struct selabel_handle *sehnd = cfg.sehnd;

	if (!sehnd) {
		erofs_err("sehnd is NULL");
		return -EINVAL;
	}

	if (selabel_lookup(sehnd, &secontext, mnt_path, mode) < 0) {
		erofs_err("cannot find selinux xattr for %s", mnt_path);
		return -EINVAL;
	}

	if (!secontext)
		goto exit;

	ssize_t vallen = strlen(secontext);

	ret = xattr_find_or_add(EROFS_XATTR_INDEX_SECURITY, XATTR_SELINUX_SUFFIX,
				secontext, vallen, xattr_head, update);

exit:
	freecon(secontext);
	return ret;

}

int erofs_lookup_xattr_by_path(struct erofs_inode *inode,
			       struct list_head *xattr_head, bool update)
{
	int ret;
	unsigned int mode = 0, uid = 0, gid = 0;
	uint64_t capabilities = 0;
	char *mnt_path = NULL;
	char *file_path = inode->i_srcpath + strlen(cfg.c_src_path) + 1;
	fs_config_func_t fs_config_func = cfg.fs_config_func;
	const char *tgt_out_path = cfg.c_target_out_dir;

	if (!fs_config_func) {
		erofs_err("fs_config_func is NULL");
		return -EINVAL;
	}

	if (asprintf(&mnt_path, "%s%s", cfg.c_mountpoint, file_path) <= 0) {
		erofs_err("cannot allocate security path for %s%s\n",
			  cfg.c_mountpoint, file_path);
		return -ENOMEM;
	}

	/* XATTR_CAPS_SUFFIX */
	int is_dir = S_ISDIR(inode->i_mode);
	fs_config_func(mnt_path, is_dir, tgt_out_path, &uid, &gid, &mode, &capabilities);

	erofs_info("inode[%s] update[%d] is_dir[%d] capabilities[%"PRIX64"] mnt_path[%s] c_target_out_dir[%s]",
		   basename(inode->i_srcpath), update, is_dir, capabilities,
		   mnt_path, tgt_out_path);

	inode->i_mode = (inode->i_mode & S_IFMT) | mode;
	inode->i_uid  = uid;
	inode->i_gid  = gid;
	ret = inode_set_capabilities(capabilities, xattr_head, update);
	if (ret)
		goto exit;

	/* get XATTR_SELINUX_SUFFIX  */
	ret = adr_get_xattr_by_path(mnt_path, inode->i_mode, xattr_head, update);
	if (ret)
		goto exit;
exit:
	free(mnt_path);
	return ret;
}

#endif //CONFIG_EROFS_READ_XATTR_FROM_DIR

static unsigned int compute_xattr_size(struct erofs_xattr_info *xi)
{
	unsigned int size = 0;

	size = sizeof(struct erofs_xattr_entry) + strlen(xi->name) + xi->e_value_size;
	return EROFS_XATTR_ALIGN(size);
}

static int xattr_list_add_sort(struct erofs_xattr_info *xattr, struct list_head *head)
{
	struct erofs_xattr_info *xi;

	if (list_empty(head)) {
		list_add(&xattr->list, head);
		return 0;
	}

	list_for_each_entry(xi, head, list) {
		if (xi->refcount >= xattr->refcount)
			continue;

		list_add_tail(&xattr->list, &xi->list);
		return 0;
	}

	list_add_tail(&xattr->list, head);
	return 0;

}

void erofs_xattr_resort_by_refs(void)
{
	struct erofs_xattr_info *xi, *next;
	LIST_HEAD(h_xattr_sort);

	list_add(&h_xattr_sort, h_xattr);
	list_del(h_xattr);
	init_list_head(h_xattr);

	list_for_each_entry_safe(xi, next, &h_xattr_sort, list) {
		list_del(&xi->list);
		xattr_list_add_sort(xi, h_xattr);
	}

	list_del(&h_xattr_sort);
}

void erofs_upadte_node_xattr_count(struct erofs_inode *inode)
{
	u32 xattr_size = 0;
	struct erofs_xattr_ref *ref;

	list_for_each_entry(ref, &inode->xattr_head, list) {
		if (ref->xattr->refcount >= cfg.c_shared_xattr_threshold)
			inode->i_shared_count++;
		else
			xattr_size += compute_xattr_size(ref->xattr);
	}

	if (xattr_size > 0 || inode->i_shared_count > 0) {
		inode->i_xattr_scnt += inode->i_shared_count * sizeof(u32);
		inode->i_xattr_scnt += xattr_size;
		inode->i_xattr_scnt = inode->i_xattr_scnt / sizeof(u32);

		/* add a header */
		inode->i_xattr_scnt += 1;
	}
}

static unsigned int fill_xattr_buffer(struct erofs_xattr_info *xi, void *buf)
{
	struct erofs_xattr_entry entry;
	unsigned int size = 0;

	memset(&entry, 0, sizeof(entry));
	entry.e_name_len = strlen(xi->name);
	entry.e_name_index = xi->e_name_index;
	entry.e_value_size = cpu_to_le16(xi->e_value_size);
	memcpy((char *)buf + size, &entry, sizeof(entry));
	size += sizeof(entry);

	memcpy((char *)buf + size, xi->name, entry.e_name_len);
	size += entry.e_name_len;

	memcpy((char *)buf + size, xi->value, entry.e_value_size);
	size += entry.e_value_size;
	size = EROFS_XATTR_ALIGN(size);

	return size;
}

void erofs_compute_shared_xattr_id(unsigned int base_id)
{
	unsigned int start_id = base_id;
	struct erofs_xattr_info *xi;

	list_for_each_entry(xi, h_xattr, list) {
		if (xi->refcount < cfg.c_shared_xattr_threshold)
			break;

		xi->xattr_id = cpu_to_le32(start_id);
		start_id +=  compute_xattr_size(xi) / EROFS_XATTR_ENTRY_ALIGN_SIZE;
	}
}

void erofs_fill_shared_xattrs(char *__buf)
{
	char *buf = __buf;
	struct erofs_xattr_info *xi;

	list_for_each_entry(xi, h_xattr, list) {
		if (xi->refcount < cfg.c_shared_xattr_threshold)
			break;

		buf += fill_xattr_buffer(xi, buf);
	}
}

unsigned int erofs_get_shared_xattrs_size(void)
{
	unsigned int size = 0;
	struct erofs_xattr_info *xi;

	list_for_each_entry(xi, h_xattr, list) {
		if (xi->refcount < cfg.c_shared_xattr_threshold)
			break;

		size += compute_xattr_size(xi);
	}

	return size;
}

unsigned int erofs_fill_node_xattrs(struct erofs_inode *inode, char *__buf)
{
	char *pbuf = __buf;
	unsigned int size = 0;
	struct erofs_xattr_ref *ref;
	struct erofs_xattr_ibody_header header;

	memset(&header, 0, sizeof(struct erofs_xattr_ibody_header));
	header.h_shared_count = inode->i_shared_count;
	header.h_checksum = 0;

	memcpy(pbuf + size, &header, sizeof(struct erofs_xattr_ibody_header));
	size += sizeof(struct erofs_xattr_ibody_header);

	list_for_each_entry(ref, &inode->xattr_head, list) {
		if (ref->xattr->refcount >= cfg.c_shared_xattr_threshold) {
			*(u32 *)(pbuf + size) = cpu_to_le32(ref->xattr->xattr_id);
			size += sizeof(ref->xattr->xattr_id);
		}
	}

	list_for_each_entry(ref, &inode->xattr_head, list) {
		if (ref->xattr->refcount < cfg.c_shared_xattr_threshold)
			size += fill_xattr_buffer(ref->xattr, pbuf + size);
	}
	return size;
}

unsigned int erofs_get_node_xattr_total_size(struct erofs_inode *inode)
{
	unsigned int size = 0;
	struct erofs_xattr_ref *ref;

	list_for_each_entry(ref, &inode->xattr_head, list) {
		if (ref->xattr->refcount >= cfg.c_shared_xattr_threshold)
			size += sizeof(u32);
		 else
			size += compute_xattr_size(ref->xattr);
	}

	if (size == 0) {
		return 0;
	} else {
		size += sizeof(struct erofs_xattr_ibody_header);
		size = EROFS_XATTR_ALIGN(size);
		return size;
	}
}

static int lookup_xattr_by_path(const char *path, unsigned int path_len)
{
	int ret = -EINVAL;
	unsigned int len;
	struct erofs_inode *inode = erofs_new_inode();

	if (!inode)
		return -ENOMEM;

	len = sizeof(inode->i_srcpath) - 1;
	if (path_len > len)
		goto exit_err;

	strncpy(inode->i_srcpath, path, path_len);
	inode->i_srcpath[path_len] = '\0';

	ret = erofs_lookup_xattr_by_path(inode, NULL, TRUE);

exit_err:
	free(inode);
	inode = NULL;
	return ret;
}

static int is_directory(const char *path)
{
	struct stat64 st;
	return (!lstat64(path, &st) && S_ISDIR(st.st_mode));
}

int erofs_xattr_build_tree(const char *path)
{
	int ret;
	char file_path[PATH_MAX + 1];
	struct dirent *dp;

	ret = lookup_xattr_by_path(path, strlen(path));
	if (ret) {
		erofs_err("lookup_xattr_by_path() failed ret=%d path[%s]",
			  ret, path);
		return ret;
	}

	if (!is_directory(path))
		return 0;

	DIR *dirp = opendir(path);
	if (!dirp) {
		erofs_info("dirp is NULL dir=%s, errno:%d-%s",
			   path, errno, strerror(errno));
		return -errno;
	}

	errno = 0;
	while ((dp = readdir(dirp)) != NULL) {
		if (is_dot_dotdot(dp->d_name) ||
		    !strncmp(dp->d_name, "lost+found", strlen("lost+found")))
			continue;
		memset(file_path, 0, sizeof(file_path));
		ret = snprintf(file_path, PATH_MAX, "%s/%s", path, dp->d_name);
		if (ret < 0 || ret >= PATH_MAX) {
			erofs_err("snprintf errorly ret[%d]", ret);
			closedir(dirp);
			return -EINVAL;
		}

		ret = erofs_xattr_build_tree(file_path);
		if (ret) {
			closedir(dirp);
			return ret;
		}

		/* this errno only to present the status of readdir */
		errno = 0;
	}

	if (errno != 0) {
		erofs_err("dir[%s] error[%d][%s]", path, errno, strerror(errno));
		closedir(dirp);
		return -errno;
	}

	closedir(dirp);
	return 0;
}

