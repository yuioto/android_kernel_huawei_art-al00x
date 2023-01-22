// SPDX-License-Identifier: GPL-2.0+
/*
 * mkfs/main.c
 *
 * Copyright (C) 2018-2019 HUAWEI, Inc.
 *             http://www.huawei.com/
 * Created by Li Guifu <bluce.liguifu@huawei.com>
 */
#define _GNU_SOURCE
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <limits.h>
#include <libgen.h>
#include "erofs/config.h"
#include "erofs/print.h"
#include "erofs/cache.h"
#include "erofs/inode.h"
#include "erofs/io.h"
#include "erofs/compress.h"
#include "erofs/map.h"
#include "erofs/patch.h"

#define EROFS_SUPER_END (EROFS_SUPER_OFFSET + sizeof(struct erofs_super_block))

static void usage(char *execpath)
{
	erofs_dump("%s %s", basename(execpath), cfg.c_version);
	erofs_dump("usage: [options] FILE DIRECTORY\n");
	erofs_dump("Generate erofs image from DIRECTORY to FILE, and [options] are:");
	erofs_dump("    -zX[,Y]  X=compressor (Y=compression level, optional)");
	erofs_dump("    -d#      set output message level to # (maximum 9)");

#ifdef ANDROID
	erofs_dump("    [-a <mountpoint>  ]");
	erofs_dump("    [-z <compr algin> ]");
	erofs_dump("    [-x <custom xattr>]");
	erofs_dump("    [-B <block_list_file>]");
	erofs_dump("    [-h <shared xattr threhold>]");
	erofs_dump("    [-T <timetamp> ] ");
	erofs_dump("    [-l <img size> ]");
	erofs_dump("    [-N <patch dir>]");
	erofs_dump("    [-u <UUID>  ]");
	erofs_dump("    [-c <CRC> ]");
	erofs_dump("    [-S <file_contexts>]");
	erofs_dump("    [-C <fs_config> ]");
	erofs_dump("    [-L <label>   ]");
	erofs_dump("    <image name> <directory> <target_out_directory>]");
#else
	erofs_dump("    [-a <mountpoint>  ]");
	erofs_dump("    [-z <compr algin> ]");
	erofs_dump("    [-x <custom xattr>]");
	erofs_dump("    [-B <block_list_file>]");
	erofs_dump("    [-h <shared xattr threhold>]");
	erofs_dump("    [-T <timetamp> ]");
	erofs_dump("    [-l <img size> ]");
	erofs_dump("    [-N <patch dir>]");

	erofs_dump("    <image name> <directory>");

#endif
	exit(1);
}

u64 parse_num_from_str(const char *str)
{
	u64 num      = 0;
	char *endptr = NULL;

	num = strtoull(str, &endptr, 10);
	DBG_BUGON(num == ULLONG_MAX);
	return num;
}

s64 str2s64(const char *str)
{
	char *endptr;

	s64 num = strtoll(str, &endptr, 10);
	return num;
}

static char *path_absolute(const char *str)
{
	const char *pstart = NULL;
	const char *pend = NULL;
	char *pstr = NULL;
	char *pret = NULL;
	int len = strlen(str);
	int pathlen = len;

	if (len == 0) {
		pret = strdup("/");
		goto End;
	}

	pstart = str;
	pend   = str + len - 1;
	while (*pstart == '/' && pstart != pend) {
		pstart++;
		pathlen--;
	}

	while (*pend == '/' && pend != pstart) {
		pend--;
		pathlen--;
	}

	if (pstart == pend && *pstart == '/') {
		pret = strdup("/");
		goto End;
	}

	pathlen += 3;
	pstr = pret = malloc(pathlen);
	if (!pret) {
		erofs_err("malloc failed len=[%d]", pathlen);
		return NULL;
	}
	memset(pstr, 0, pathlen);
	*pstr++ = '/';
	strncpy(pstr, pstart, pend - pstart + 1);
	pstr += pend - pstart + 1;
	*pstr++ = '/';
	*pstr = '\0';

End:
	return pret;
}

static int mkfs_parse_options_cfg(int argc, char *argv[])
{
	int opt;
	s64 timestamp;

#if defined(ANDROID) && !defined(CONFIG_EROFS_READ_XATTR_FROM_DIR)
	struct selinux_opt seopts[] = { { SELABEL_OPT_PATH, "" } };
#endif
	while ((opt = getopt(argc, argv, "m:j:b:g:i:I:e:o:T:l:L:a:S:C:B:d:h:N:z::fwJsctvux")) != -1) {
		switch (opt) {
		case 'j':
		case 'b':
		case 'g':
		case 'i':
		case 'I':
		case 'e':
		case 'o':
		case 'f':
		case 'w':
		case 's':
		case 't':
		case 'v':
		case 'J':
		// compatible to mkfs_ext4 param not use
		break;
		case 'm':
			cfg.c_multi_blks_thr = parse_num_from_str(optarg);
			break;

		case 'T':
			timestamp = str2s64(optarg);
			if (timestamp != -1)
				cfg.c_timestamp = timestamp;
			break;

		case 'l':
			cfg.c_img_len = parse_num_from_str(optarg);
			break;

		case 'L':
			cfg.c_label = optarg;
			break;

		case 'a':
			cfg.c_mountpoint = path_absolute(optarg);
			break;

		case 'u':
			cfg.c_real_uuid = 1;
			break;

		case 'z':
			if (!optarg) {
				cfg.c_compr_alg_master = "lz4hc";
				break;
			}
			/* get specified compression level */
			int i;
			for (i = 0; optarg[i] != '\0'; ++i) {
				if (optarg[i] == ',') {
					cfg.c_compr_level_master =
						atoi(optarg + i + 1);
					optarg[i] = '\0';
					break;
				}
			}
			cfg.c_compr_alg_master = strndup(optarg, i);
			break;

		case 'c':
			cfg.c_crc = 1;
			break;
		case 'x':
			cfg.c_cust_xattr = 1;
			break;

		case 'S':
#if defined(ANDROID) && !defined(CONFIG_EROFS_READ_XATTR_FROM_DIR)
		seopts[0].value = optarg;
		cfg.sehnd  = selabel_open(SELABEL_CTX_FILE, seopts, 1);

		if (!cfg.sehnd) {
			perror(optarg);
			return -EINVAL;
		}
#endif
		break;

		case 'C':
			cfg.c_fs_config_file = optarg;
			break;

		case 'B':
			cfg.c_map_file = optarg;
			break;

		case 'd':
			cfg.c_dbg_lvl = parse_num_from_str(optarg);
			break;
		case 'h':
			cfg.c_shared_xattr_threshold = parse_num_from_str(optarg);
			break;
		case 'N':
			cfg.c_patch_path = realpath(optarg, NULL);
			break;

		default: /* '?' */
			return -EINVAL;
		}
	}

#if !defined(HOST) && !defined(CONFIG_EROFS_READ_XATTR_FROM_DIR)
	if (!cfg.sehnd && cfg.c_mountpoint) {
		cfg.sehnd  = selinux_android_file_context_handle();
		if (!cfg.sehnd) {
			perror(optarg);
			return -EINVAL;
		}
	}
#endif

#if defined(ANDROID) && !defined(CONFIG_EROFS_READ_XATTR_FROM_DIR)
	if (cfg.c_fs_config_file) {
		if (load_canned_fs_config(cfg.c_fs_config_file) < 0) {
			erofs_err("failed to load %s\n", cfg.c_fs_config_file);
			return -EINVAL;
		}
		cfg.fs_config_func = canned_fs_config;
	} else if (cfg.c_mountpoint) {
		cfg.fs_config_func = fs_config;
	}
#endif

	if (optind >= argc) {
		erofs_err("Expected image filename after options\n");
		return -EINVAL;
	}

	cfg.c_img_path = strdup(argv[optind++]);
	if (!cfg.c_img_path) {
		erofs_err("c_img_path is null %s", strerror(errno));
		return -EINVAL;
	}

	if (optind >= argc) {
		erofs_err("Expected source filename after options\n");
		return -EINVAL;
	}

	cfg.c_src_path = argv[optind++];
	cfg.c_src_path = realpath(cfg.c_src_path, NULL);
	if (!cfg.c_src_path) {
		erofs_err("cfg.c_src_path is NULL errno:%s", strerror(errno));
		return -EINVAL;
	}

	if (optind < argc) {
#if defined(ANDROID)
		cfg.c_target_out_dir = strdup(argv[optind++]);
#else
		erofs_err("can't set android c_target_out_dir - built without android support\n");
		return -EINVAL;
#endif
	}

	if (optind < argc) {
		erofs_err("Unexpected argument: %s\n", argv[optind]);
		return -EINVAL;
	}

	return 0;
}

int erofs_mkfs_update_shared_xattr(struct erofs_buffer_head *bh)
{
	unsigned int size = erofs_get_shared_xattrs_size();
	char *buf;

	buf = calloc(size, 1);
	if (!buf)
		return -ENOMEM;

	erofs_fill_shared_xattrs(buf);
	bh->fsprivate = buf;
	bh->op = &erofs_buf_write_bhops;
	return 0;
}

int erofs_mkfs_update_super_block(struct erofs_buffer_head *bh,
				  erofs_nid_t root_nid)
{
	struct erofs_super_block sb = {
		.magic     = cpu_to_le32(EROFS_SUPER_MAGIC_V1),
		.blkszbits = LOG_BLOCK_SIZE,
		.inos   = 0,
		.blocks = 0,
		.meta_blkaddr  = sbi.meta_blkaddr,
		.xattr_blkaddr = 0,
		.requirements = cpu_to_le32(sbi.requirements),
	};
	const unsigned int sb_blksize =
		round_up(EROFS_SUPER_END, EROFS_BLKSIZ);
	u32 blocks  = erofs_mapbh(NULL, true);

	if (dev_truncate(blknr_to_addr(blocks)) < 0)
		return -errno;
	sb.build_time      = cpu_to_le64(cfg.c_timestamp);;
	sb.build_time_nsec = cpu_to_le32(0);
	sb.root_nid = cpu_to_le16(root_nid);
	sb.blocks   = cpu_to_le32(blocks);

	if (blknr_to_addr(blocks) > cfg.c_img_len) {
		erofs_err("Image[%" PRIu64 "] is too bigger than LIMIT size:%" PRIu64 "",
			  blknr_to_addr(blocks), cfg.c_img_len);
		return -ENOSPC;
	}

	erofs_dump("Image[%u blocks][%" PRIu64 "] <= LIMIT size:%" PRIu64 "",
		   blocks, blknr_to_addr(blocks), cfg.c_img_len);

	char *buf = calloc(sb_blksize, 1);

	if (!buf) {
		erofs_err("Failed to allocate memory for sb: %s",
			  erofs_strerror(-errno));
		return -ENOMEM;
	}
	memcpy(buf + EROFS_SUPER_OFFSET, &sb, sizeof(sb));

	bh->fsprivate = buf;
	bh->op = &erofs_buf_write_bhops;
	return 0;
}

int main(int argc, char **argv)
{
	int err = 0;
	struct erofs_buffer_head *sb_bh;
	struct erofs_inode *root_inode;
	erofs_nid_t root_nid;

	erofs_init_configure();

	if (argc < 2)
		usage(argv[0]);

	err = mkfs_parse_options_cfg(argc, argv);
	if (err) {
		if (err == -EINVAL)
			usage(argv[0]);
		goto exit_err;
	}

	err = dev_open(cfg.c_img_path);
	if (err)
		goto exit_cfg;

	erofs_dump("%s %s", basename(argv[0]), cfg.c_version);
	erofs_show_config();

	err = erofs_xattr_build_tree(cfg.c_src_path);
	if (err)
		goto exit_dev;

	erofs_xattr_resort_by_refs();

	sb_bh = erofs_buffer_init();
	err = erofs_bh_balloon(sb_bh, EROFS_SUPER_END);
	if (err < 0) {
		erofs_err("Failed to balloon erofs_super_block: %s",
			  erofs_strerror(err));
		goto exit_dev;
	}

	unsigned int size = erofs_get_shared_xattrs_size();
	struct erofs_buffer_head *bh = erofs_balloc(META, size, 0, 0);

	bh->op = &erofs_skip_write_bhops;
	erofs_mapbh(bh->block, true);

	const erofs_off_t start = erofs_btell(bh, false);
	unsigned int base_id = start / EROFS_XATTR_ENTRY_ALIGN_SIZE;

	erofs_compute_shared_xattr_id(base_id);
	erofs_mkfs_update_shared_xattr(bh);

	err = z_erofs_compress_init();
	if (err)
		goto exit_dev;

	if (cfg.c_map_file) {
		err = erofs_map_file_init(cfg.c_map_file);
		if (err)
			goto exit_compr;
		err = erofs_map_save_compr_algo_ver(cfg.c_compr_ver_master,
						    MAP_BIN_VERSION_BLK_ADDR);
		if (err < 0) {
			erofs_err("save compress algorithm version failed");
			goto exit_map;
		}
	}

	erofs_inode_manager_init();

	root_inode = erofs_mkfs_build_tree_from_path(NULL, cfg.c_src_path);
	if (IS_ERR(root_inode)) {
		err = PTR_ERR(root_inode);
		goto exit_map;
	}

#if PATCH_ENABLED
	/* for fwk cold patch */
	if (cfg.c_patch_path) {
		erofs_info("patch build start");
		err = erofs_mkfs_build_tree_patch(cfg.c_patch_path, root_inode);
		if (err)
			goto exit_map;
	}
#endif

	root_nid = erofs_lookupnid(root_inode);
	erofs_iput(root_inode);

	err = erofs_mkfs_update_super_block(sb_bh, root_nid);
	if (err)
		goto exit_map;

	/* flush all remaining buffers */
	if (!erofs_bflush(NULL))
		err = -EIO;

	erofs_dump("%s: [ %s ] Done", basename(argv[0]), cfg.c_img_path);
exit_map:
	erofs_map_file_close();
exit_compr:
	z_erofs_compress_exit();
exit_dev:
	dev_close();
exit_cfg:
	erofs_exit_configure();
exit_err:
	if (err) {
		erofs_err("\tCould not format the device : %s\n",
			  erofs_strerror(err));
		return 1;
	}
	return err;
}
