/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * erofs_utils/include/erofs/config.h
 *
 * Copyright (C) 2018-2019 HUAWEI, Inc.
 *             http://www.huawei.com/
 * Created by Li Guifu <bluce.liguifu@huawei.com>
 */
#ifndef __EROFS_CONFIG_H
#define __EROFS_CONFIG_H

#include "defs.h"
#include <stdio.h>

#ifdef ANDROID
#include <private/android_filesystem_config.h>
#include <private/canned_fs_config.h>
#endif

#include <selinux/selinux.h>
#include <selinux/label.h>

#if defined(ANDROID) && !defined(CONFIG_EROFS_READ_XATTR_FROM_DIR)
#include <selinux/android.h>
#endif

#include "xattr.h"

#define EROFS_IMAGE_DEFAULT_TIMETAMP (1230739200) /* 2009-1-1 00:00:00 */
/*
 * algo version use for diff saved in map.bin, range form 0x1 max is 0xF
 * it will left shift 4 (vers << 4), so it's 10 ->a0->b0   f0
 * 0x01 LZ4 default (v1.7.3) + 1.8.2
 * 0x02 LZ4 default (v1.8.3)
 */
#define EROFS_COMPR_LOCAL_VERSION    0x3

struct erofs_configure {
	const char *c_version;
	bool c_dry_run;
	bool c_legacy_compress;
	int c_dbg_lvl;
	int c_cust_xattr;
	int c_crc;
	int c_real_uuid;
	uint32_t c_shared_xattr_threshold;
	uint32_t c_multi_blks_thr;
	uint64_t c_img_len;
	int64_t c_timestamp;

	/* related arguments for mkfs.erofs */
	char *c_img_path;
	char *c_src_path;
	char *c_compr_alg_master;
	int c_compr_level_master;
	int c_compr_ver_master;

	const char *c_mountpoint;
	const char *c_target_out_dir;
	const char *c_fs_config_file;
	const char *c_map_file;
	const char *c_label;
	const char *c_patch_path;
	const char **c_file_uncompr_list;
	const char **c_file_noinline_list;

	struct selabel_handle *sehnd;
	fs_config_func_t fs_config_func;
};

extern struct erofs_configure cfg;

void erofs_init_configure(void);
void erofs_show_config(void);
void erofs_exit_configure(void);

#endif

