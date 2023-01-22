/*

	Hisilicon exFAT implementation.
	Copyright (C) HUAWEI

*/

#include "uct_repair.h"
#include "uctc_repair.h"

static off_t uct_alignment_repair(void)
{
	return get_cluster_size_rp();
}

static off_t uct_size_repair(void)
{
	return sizeof(upcase_table_r);
}

static int uct_write_repair(_UNUSED struct exfat_dev* dev)
{
	/*TODO in next repair step*/
	return 0;
}

const struct fs_object_r uct_r =
{
	.get_alignment = uct_alignment_repair,
	.get_size = uct_size_repair,
	.write = uct_write_repair,
};
