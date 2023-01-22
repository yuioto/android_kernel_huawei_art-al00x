/*

	Hisilicon exFAT implementation.
	Copyright (C) HUAWEI

*/

#include "rootdir_repair.h"
#include "uct_repair.h"
#include "cbm_repair.h"
#include "uctc_repair.h"
#include <string.h>

static off_t rootdir_alignment_repair(void)
{
	return get_cluster_size_rp();
}

static off_t rootdir_size_repair(void)
{
	return get_cluster_size_rp();
}

static int rootdir_write_repair(_UNUSED struct exfat_dev* dev)
{
	/*TODO in next repair step*/
	return 0;
}

const struct fs_object_r rootdir_r =
{
	.get_alignment = rootdir_alignment_repair,
	.get_size = rootdir_size_repair,
	.write = rootdir_write_repair,
};
