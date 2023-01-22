// SPDX-License-Identifier: GPL-2.0+
/*
 * erofs_utils/lib/config.c
 *
 * Copyright (C) 2018-2019 HUAWEI, Inc.
 *             http://www.huawei.com/
 * Created by Li Guifu <bluce.liguifu@huawei.com>
 */
#include <string.h>
#include <stdlib.h>
#include "erofs/print.h"
#include "limits.h"
#include "erofs/internal.h"

/* diff multi blocks 150MB = 4KB * 38400 */
#define EROFS_MULTI_BLKS_THRESHOLD    38400
#define EROFS_MULTI_BLKS_POSTFIX      "erofs_postfix"

#if !defined(PACKAGE_VERSION)
#define PACKAGE_VERSION          "version: v4.0.190917.2"
#endif

struct erofs_configure cfg;

static const char *file_uncompr_list[] = {
	"/system/priv-app/HwCamera2/HwCamera2.apk",
	"/system/priv-app/HwSystemManager/HwSystemManager.apk",
	"/system/priv-app/HwStartupGuide/HwStartupGuide.apk",
	"/system/framework/boot-framework.vdex",
	"/system/framework/arm/boot-framework.vdex",
	"/system/framework/arm/boot-framework.art",
	"/system/framework/arm/boot-framework.oat",
	"/system/framework/arm64/boot-framework.vdex",
	"/system/framework/arm64/boot-framework.art",
	"/system/framework/arm64/boot-framework.oat",
	"/system/lib/libc.so",
	"/system/lib64/libc.so",
	"/system/lib/libc++.so",
	"/system/lib64/libc++.so",
	"/system/lib/libEGL.so",
	"/system/lib64/libEGL.so",
	"/system/lib/libart.so",
	"/system/lib64/libart.so",
	NULL,
};

static const char *file_noinline_list[] = {
	"zip",
	"apk",
	"jar",
	NULL
};

void erofs_init_configure(void)
{
	memset(&cfg, 0, sizeof(cfg));

	cfg.c_dbg_lvl  = 0;
	cfg.c_version  = PACKAGE_VERSION;
	cfg.c_dry_run  = false;
	cfg.c_legacy_compress = false;
	cfg.c_compr_level_master = -1;
	cfg.c_compr_ver_master = EROFS_COMPR_LOCAL_VERSION;
	cfg.c_mountpoint = "/";

	cfg.c_cust_xattr = 0;
	cfg.c_shared_xattr_threshold = MKFS_XATTR_MAX_SHARED_REFCOUNT;
	cfg.c_img_len    = ULONG_MAX;
	cfg.c_timestamp  = EROFS_IMAGE_DEFAULT_TIMETAMP;
	cfg.c_multi_blks_thr = EROFS_MULTI_BLKS_THRESHOLD;
	sbi.requirements = EROFS_REQUIREMENT_LZ4_0PADDING;
	cfg.c_img_path = NULL;
	cfg.c_src_path = NULL;
	cfg.c_patch_path = NULL;
	cfg.c_compr_alg_master = "lz4hc";
	cfg.c_file_uncompr_list = file_uncompr_list;
	cfg.c_file_noinline_list = file_noinline_list;
}

void erofs_show_config(void)
{
	const struct erofs_configure *c = &cfg;

	erofs_dump("\tc_version:             [%8s]", c->c_version);
	erofs_dump("\tc_dbg_lvl:             [%8d]", c->c_dbg_lvl);
	erofs_dump("\tc_dry_run:             [%8d]", c->c_dry_run);
	erofs_dump("\tc_img_path:            [%8s]", c->c_img_path);
	erofs_dump("\tc_src_path:            [%8s]", c->c_src_path);
	erofs_dump("\tc_mountpoint:          [%8s]", c->c_mountpoint);
	erofs_dump("\tc_compr version:       [%8d]", c->c_compr_ver_master);
	erofs_dump("\tc_compr_alg_master:    [%8s]", c->c_compr_alg_master);
#ifdef ANDROID
	erofs_dump("\tc_target_out_dir:      [%8s]", c->c_target_out_dir);
	erofs_dump("\tc_fs_config_file:      [%8s]", c->c_fs_config_file);
#endif
	erofs_dump("\tc_cust_xattr:          [%8d]", c->c_cust_xattr);
	erofs_dump("\tc_shared_xattr_thresh: [%8d]", c->c_shared_xattr_threshold);
	erofs_dump("\tc_img_len:             [%8" PRIu64 "]", c->c_img_len);
	erofs_dump("\tc_timestamp:           [%8" PRIi64 "]", c->c_timestamp);
	erofs_dump("\tc_multi_blks_thr:      [%8u]", c->c_multi_blks_thr);
	erofs_dump("\tc_patch_path:          [%8s]", c->c_patch_path);
}

void erofs_exit_configure(void)
{
	if (cfg.c_img_path) {
		free((void *)cfg.c_img_path);
		cfg.c_img_path = NULL;
	}
	if (cfg.c_src_path) {
		free((void *)cfg.c_src_path);
		cfg.c_src_path = NULL;
	}
	if (cfg.c_patch_path) {
		free((void *)cfg.c_patch_path);
		cfg.c_patch_path = NULL;
	}
#ifdef ANDROID
	if (cfg.c_target_out_dir) {
		free((void *)cfg.c_target_out_dir);
		cfg.c_target_out_dir = NULL;
	}
#endif
}

