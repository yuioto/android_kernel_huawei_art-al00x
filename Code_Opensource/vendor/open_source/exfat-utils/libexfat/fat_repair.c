/*

	Hisilicon exFAT implementation.
	Copyright (C) HUAWEI

*/

#include "fat_repair.h"
#include "cbm_repair.h"
#include "uct_repair.h"
#include "rootdir_repair.h"
#include <unistd.h>

static off_t fat_alignment_repair(void)
{
	return (off_t) 128 * get_sector_size_rp();
}

static off_t fat_size_repair(void)
{
	return get_volume_size_rp() / get_cluster_size_rp() * sizeof(cluster_t);
}

static int fat_write_repair(_UNUSED struct exfat_dev* dev)
{
	/*TODO in next repair step*/
	return 0;
}

const struct fs_object_r fat_r =
{
	.get_alignment = fat_alignment_repair,
	.get_size = fat_size_repair,
	.write = fat_write_repair,
};
