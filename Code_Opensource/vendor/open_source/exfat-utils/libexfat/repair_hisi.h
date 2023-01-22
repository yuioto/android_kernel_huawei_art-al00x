/*

	Hisilicon exFAT implementation.
	Copyright (C) HUAWEI

*/

#ifndef REPAIR_H_INCLUDED
#define REPAIR_H_INCLUDED

#include "exfat.h"

struct fs_object_r
{
	off_t (*get_alignment)(void);
	off_t (*get_size)(void);
	int (*write)(struct exfat_dev* dev);
};

extern const struct fs_object_r* objects_r[];

int get_sector_bits_rp(void);
int get_spc_bits_rp(void);
off_t get_volume_size_rp(void);
const le16_t* get_volume_label_rp(void);
uint32_t get_volume_serial_rp(void);
uint64_t get_first_sector_rp(void);
int get_sector_size_rp(void);
int get_cluster_size_rp(void);

off_t get_position_rp(const struct fs_object_r* object_r);
int repair_boot_sector(const char* spec);
#endif /* ifndef REPAIR_H_INCLUDED */
