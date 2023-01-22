/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * erofs_utils/include/erofs/xattr.h
 *
 * Copyright (C) 2018-2019 HUAWEI, Inc.
 *             http://www.huawei.com/
 * Created by Li Guifu <bluce.liguifu@huawei.com>
 */

#ifndef __EROFS_XATTR_H
#define __EROFS_XATTR_H

#include "defs.h"
#include "internal.h"

#define XATTR_SELINUX_SUFFIX        "selinux"
#define XATTR_CAPS_SUFFIX           "capability"
#define EROFS_XATTR_PREFIX_USER             "user."
#define EROFS_XATTR_SYSTEM_ACL_ACCESS_NAME  "system.posix_acl_access"
#define EROFS_XATTR_SYSTEM_ACL_DEFAULT_NAME "system.posix_acl_default"
#define EROFS_XATTR_PREFIX_TRUSTED          "trusted."
#define EROFS_XATTR_PREFIX_LUSTRE           "lustre."
#define EROFS_XATTR_PREFIX_SECURITY         "security."
#define EROFS_XATTR_SYSTEM_ADVISE_NAME      "system.advise"
#define MKFS_XATTR_MAX_SHARED_REFCOUNT  (8)
#define EROFS_MAX_SHARED_XATTRS         (128)
/* h_shared_count between 129 ... 255 are special # */
#define EROFS_SHARED_XATTR_EXTENT       (255)
#define EROFS_XATTR_ENTRY_ALIGN_SIZE    (4)
/* Name indexes */
#define EROFS_XATTR_INDEX_USER              1
#define EROFS_XATTR_INDEX_POSIX_ACL_ACCESS  2
#define EROFS_XATTR_INDEX_POSIX_ACL_DEFAULT 3
#define EROFS_XATTR_INDEX_TRUSTED           4
#define EROFS_XATTR_INDEX_LUSTRE            5
#define EROFS_XATTR_INDEX_SECURITY          6
#define EROFS_XATTR_INDEX_ADVISE            7

struct erofs_xattr_info {
	struct list_head list;

	uint8_t e_name_len;
	uint8_t e_name_index;
	uint16_t e_value_size;

	uint32_t refcount;
	uint32_t xattr_id;
	uint32_t ondisk_size;

	char *name;
	char *value;
};

struct erofs_xattr_ref {
	struct list_head list;
	struct erofs_xattr_info *xattr;
};

typedef void (*fs_config_func_t)(const char *path, int dir,
				 const char *target_out_path, unsigned int *uid,
				 unsigned int *gid, unsigned int *mode,
				 uint64_t *capabilities);

int erofs_xattr_init(void);
int erofs_xattr_build_tree(const char *path);
void erofs_xattr_resort_by_refs(void);
int mkfs_add_xattr(struct erofs_inode *node);
void erofs_compute_shared_xattr_id(unsigned int base_id);
void erofs_fill_shared_xattrs(char *__buf);
unsigned int erofs_get_shared_xattrs_size(void);
void erofs_upadte_node_xattr_count(struct erofs_inode *node);
unsigned int erofs_get_node_xattr_total_size(struct erofs_inode *node);
unsigned int erofs_fill_node_xattrs(struct erofs_inode *node, char *__buf);
void mkfs_add_xattr_custom(struct list_head *inode_head);
int erofs_lookup_xattr_by_path(struct erofs_inode *inode,
			       struct list_head *xattr_head, bool update);


#endif //__EROFS_XATTR_H

