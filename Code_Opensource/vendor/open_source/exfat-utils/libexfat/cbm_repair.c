/*

	Hisilicon exFAT implementation.
	Copyright (C) HUAWEI

*/

#include "cbm_repair.h"
#include "fat_repair.h"
#include "uct_repair.h"
#include "rootdir_repair.h"
#include <limits.h>
#include <string.h>

static off_t cbm_alignment_repair(void)
{
	return get_cluster_size_rp();
}

static off_t cbm_size_repair(void)
{
	return DIV_ROUND_UP(
			(get_volume_size_rp() - get_position_rp(&cbm_r)) / get_cluster_size_rp(),
			CHAR_BIT);
}

static int cbm_write_repair(_UNUSED struct exfat_dev* dev)
{
	/*TODO in next repair step*/
	return 0;
}

const struct fs_object_r cbm_r =
{
	.get_alignment = cbm_alignment_repair,
	.get_size = cbm_size_repair,
	.write = cbm_write_repair,
};
