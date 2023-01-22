/*

	Hisilicon exFAT implementation.
	Copyright (C) HUAWEI

*/

#include "vbr_repair.h"
#include "fat_repair.h"
#include "cbm_repair.h"
#include "uct_repair.h"
#include "rootdir_repair.h"
#include <string.h>
#include "securec.h"
#include "securectype.h"

static off_t vbr_alignment_repair(void)
{
	return get_sector_size_rp();
}

static off_t vbr_size_repair(void)
{
	return 12 * get_sector_size_rp();
}

static void init_sb(struct exfat_super_block* sb)
{
	uint32_t clusters_max;
	uint32_t fat_sectors;

	clusters_max = get_volume_size_rp() / get_cluster_size_rp();
	fat_sectors = DIV_ROUND_UP((off_t) clusters_max * sizeof(cluster_t),
			get_sector_size_rp());

	memset_s(sb, sizeof(struct exfat_super_block), 0, sizeof(struct exfat_super_block));
	sb->jump[0] = 0xeb;
	sb->jump[1] = 0x76;
	sb->jump[2] = 0x90;
	memcpy_s(sb->oem_name, sizeof(sb->oem_name), "EXFAT   ", sizeof(sb->oem_name));
	sb->sector_start = cpu_to_le64(get_first_sector_rp());
	sb->sector_count = cpu_to_le64(get_volume_size_rp() / get_sector_size_rp());
	sb->fat_sector_start = cpu_to_le32(
			fat_r.get_alignment() / get_sector_size_rp());
	sb->fat_sector_count = cpu_to_le32(ROUND_UP(
			le32_to_cpu(sb->fat_sector_start) + fat_sectors,
				1 << get_spc_bits_rp()) -
			le32_to_cpu(sb->fat_sector_start));
	sb->cluster_sector_start = cpu_to_le32(
			get_position_rp(&cbm_r) / get_sector_size_rp());
	sb->cluster_count = cpu_to_le32(clusters_max -
			((le32_to_cpu(sb->fat_sector_start) +
			  le32_to_cpu(sb->fat_sector_count)) >> get_spc_bits_rp()));
	sb->rootdir_cluster = cpu_to_le32(
			(get_position_rp(&rootdir_r) - get_position_rp(&cbm_r)) / get_cluster_size_rp()
			+ EXFAT_FIRST_DATA_CLUSTER);
	sb->volume_serial = cpu_to_le32(get_volume_serial_rp());
	sb->version.major = 1;
	sb->version.minor = 0;
	sb->volume_state = cpu_to_le16(0);
	sb->sector_bits = get_sector_bits_rp();
	sb->spc_bits = get_spc_bits_rp();
	sb->fat_count = 1;
	sb->drive_no = 0x80;
	sb->allocated_percent = 0;
	sb->boot_signature = cpu_to_le16(0xaa55);
}

static int vbr_write_repair(struct exfat_dev* dev)
{
	struct exfat_super_block sb;
	uint32_t checksum;
	le32_t* sector = malloc(get_sector_size_rp());
	size_t i;

	if (sector == NULL)
	{
		exfat_error("failed to allocate sector-sized block of memory");
		return 1;
	}

	init_sb(&sb);

	if (exfat_write(dev, &sb, sizeof(struct exfat_super_block)) < 0)
	{
		free(sector);
		exfat_error("failed to write super block sector");
		return 1;
	}
	checksum = exfat_vbr_start_checksum(&sb, sizeof(struct exfat_super_block));

	memset_s(sector, get_sector_size_rp(), 0, get_sector_size_rp());
	sector[get_sector_size_rp() / sizeof(sector[0]) - 1] =
			cpu_to_le32(0xaa550000);
	for (i = 0; i < 8; i++)
	{
		if (exfat_write(dev, sector, get_sector_size_rp()) < 0)
		{
			free(sector);
			exfat_error("failed to write a sector with boot signature");
			return 1;
		}
		checksum = exfat_vbr_add_checksum(sector, get_sector_size_rp(), checksum);
	}

	memset_s(sector, get_sector_size_rp(), 0, get_sector_size_rp());
	for (i = 0; i < 2; i++)
	{
		if (exfat_write(dev, sector, get_sector_size_rp()) < 0)
		{
			free(sector);
			exfat_error("failed to write an empty sector");
			return 1;
		}
		checksum = exfat_vbr_add_checksum(sector, get_sector_size_rp(), checksum);
	}

	for (i = 0; i < get_sector_size_rp() / sizeof(sector[0]); i++)
		sector[i] = cpu_to_le32(checksum);
	if (exfat_write(dev, sector, get_sector_size_rp()) < 0)
	{
		free(sector);
		exfat_error("failed to write checksum sector");
		return 1;
	}

	free(sector);
	return 0;
}

const struct fs_object_r vbr_r =
{
	.get_alignment = vbr_alignment_repair,
	.get_size = vbr_size_repair,
	.write = vbr_write_repair,
};
