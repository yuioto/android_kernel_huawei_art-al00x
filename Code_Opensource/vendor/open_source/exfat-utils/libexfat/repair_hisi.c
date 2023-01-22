/*

	Hisilicon exFAT implementation.
	Copyright (C) HUAWEI

*/

#include "repair_hisi.h"
#include "vbr_repair.h"
#include "fat_repair.h"
#include "cbm_repair.h"
#include "uct_repair.h"
#include "rootdir_repair.h"
#include "exfat.h"
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "securectype.h"
#include "securec.h"

const struct fs_object_r* objects_r[] =
{
	&vbr_r,
	&vbr_r,
	&fat_r,
	/* clusters heap */
	&cbm_r,
	&uct_r,
	&rootdir_r,
	NULL,
};

static struct
{
	int sector_bits;
	int spc_bits;
	off_t volume_size;
	le16_t volume_label[EXFAT_ENAME_MAX + 1];
	uint32_t volume_serial;
	uint64_t first_sector;
}
param_r;

int get_sector_bits_rp(void)
{
	return param_r.sector_bits;
}

int get_spc_bits_rp(void)
{
	return param_r.spc_bits;
}

off_t get_volume_size_rp(void)
{
	return param_r.volume_size;
}

const le16_t* get_volume_label_rp(void)
{
	return param_r.volume_label;
}

uint32_t get_volume_serial_rp(void)
{
	return param_r.volume_serial;
}

uint64_t get_first_sector_rp(void)
{
	return param_r.first_sector;
}

int get_sector_size_rp(void)
{
	return 1 << get_sector_bits_rp();
}

int get_cluster_size_rp(void)
{
	return get_sector_size_rp() << get_spc_bits_rp();
}

off_t get_position_rp(const struct fs_object_r* object_r)
{
	const struct fs_object_r** pp;
	off_t position = 0;

	for (pp = objects_r; *pp; pp++)
	{
		position = ROUND_UP(position, (*pp)->get_alignment());
		if (*pp == object_r)
			return position;
		position += (*pp)->get_size();
	}
	exfat_bug("unknown object");
}

static int write_repair(struct exfat_dev* dev, const struct fs_object_r* object_r)
{
	const struct fs_object_r** pp;
	off_t position = 0;


	for (pp = objects_r; *pp; pp++)
	{
		position = ROUND_UP(position, (*pp)->get_alignment());
		if (*pp == object_r)
		{
			if (exfat_seek(dev, position, SEEK_SET) == (off_t) -1)
			{
				exfat_error("seek to 0x%"PRIx64" failed", position);
				return 1;
			}
			if ((*pp)->write(dev) != 0)
				return 1;
		}
		position += (*pp)->get_size();
	}
	return 0;
}

static int setup_spc_bits(int sector_bits, int user_defined, off_t volume_size)
{
	int i;

	if (user_defined != -1)
	{
		off_t cluster_size = 1 << sector_bits << user_defined;
		if (volume_size / cluster_size > EXFAT_LAST_DATA_CLUSTER)
		{
			struct exfat_human_bytes chb, vhb;

			exfat_humanize_bytes(cluster_size, &chb);
			exfat_humanize_bytes(volume_size, &vhb);
			exfat_error("cluster size %"PRIu64" %s is too small for "
					"%"PRIu64" %s volume, try -s %d",
					chb.value, chb.unit,
					vhb.value, vhb.unit,
					1 << setup_spc_bits(sector_bits, -1, volume_size));
			return -1;
		}
		return user_defined;
	}

	if (volume_size < 256ll * 1024 * 1024)
		return MAX(0, 12 - sector_bits);	/* 4 KB */
	if (volume_size < 32ll * 1024 * 1024 * 1024)
		return MAX(0, 15 - sector_bits);	/* 32 KB */

	for (i = 17; ; i++)						/* 128 KB or more */
		if (DIV_ROUND_UP(volume_size, 1 << i) <= EXFAT_LAST_DATA_CLUSTER)
			return MAX(0, i - sector_bits);
}

static int setup_volume_label(le16_t label[EXFAT_ENAME_MAX + 1], const char* s)
{
	memset_s(label, (EXFAT_ENAME_MAX + 1) * sizeof(le16_t), 0,
		 (EXFAT_ENAME_MAX + 1) * sizeof(le16_t));
	if (s == NULL)
		return 0;
	return utf8_to_utf16(label, s, EXFAT_ENAME_MAX, strlen(s));
}

static uint32_t setup_volume_serial(uint32_t user_defined)
{
	struct timeval now;

	if (user_defined != 0)
		return user_defined;

	if (gettimeofday(&now, NULL) != 0)
	{
		exfat_error("failed to form volume id");
		return 0;
	}
	return (now.tv_sec << 20) | now.tv_usec;
}

static int setup(struct exfat_dev* dev, int sector_bits, int spc_bits,
		const char* volume_label, uint32_t volume_serial,
		uint64_t first_sector)
{
	param_r.sector_bits = sector_bits;
	param_r.first_sector = first_sector;
	param_r.volume_size = exfat_get_size(dev);

	param_r.spc_bits = setup_spc_bits(sector_bits, spc_bits, param_r.volume_size);
	if (param_r.spc_bits == -1)
		return 1;

	if (setup_volume_label(param_r.volume_label, volume_label) != 0)
		return 1;

	param_r.volume_serial = setup_volume_serial(volume_serial);
	if (param_r.volume_serial == 0)
		return 1;

	return 0;
}

int repair_boot_sector(const char* spec)
{
	int spc_bits = -1;
	const char* volume_label = NULL;
	uint32_t volume_serial = 0;
	uint64_t first_sector = 0;
	struct exfat_dev* dev;

	printf("exfat boot sector repair exfat %s\n", VERSION);

	dev = exfat_open(spec, EXFAT_MODE_RW);
	if (dev == NULL)
		return 1;

	if (setup(dev, 9, spc_bits, volume_label, volume_serial,
				first_sector) != 0)
	{
		printf("exfat setup param failed for boot sector repair.\n");
		exfat_close(dev);
		return 1;
	}

	if (write_repair(dev, &vbr_r))
	{
		printf("exfat repair write failed for boot sector repair.\n");
		exfat_close(dev);
		return 1;
	}

	if (exfat_fsync(dev))
	{
		printf("exfat fsync failed for boot sector repair.\n");
		exfat_close(dev);
		return 1;
	}

	if (exfat_close(dev) != 0)
		return 1;

	printf("exfat boot sector repaired successfully.\n");
	return 0;
}
