

#include "oal_no_fs_adapt.h"
#include "plat_flash.h"
#include "plat_debug.h"
#include "stdlib.h"
#include "string.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

struct st_file_store_info g_apst_file_store_info[] = {
    {
        .ul_store_addr      = 0x0,
        .ul_store_length    = 0x30000,
        .ul_store_used      = 0x30000,
        .pus_store_path= "/jffs0/etc/hisi_wifi/firmware/dump_mem_ram.bin",
    },
    {
        .ul_store_addr      = 0x30000,
        .ul_store_length    = 0x30000,
        .ul_store_used      = 0x30000,
        .pus_store_path= "/jffs0/etc/hisi_wifi/firmware/dump_mem_pkt_b_ram.bin",
    },
    {
        .ul_store_addr      = 0x60000,
        .ul_store_length    = 0x10000,
        .ul_store_used      = 0x10000,
        .pus_store_path= "/jffs0/etc/hisi_wifi/firmware/dump_mem_pkt_h_ram.bin",
    },
};

struct st_file_store_cfg g_st_file_store_cfg = {
    .base_addr = 0x750000,
    .total_length = 0x0,
};


/*****************************************************************************
  3 函数实现
*****************************************************************************/
oal_void oal_no_fs_config(oal_ulong ul_base_addr, oal_uint32 u_length)
{
    g_st_file_store_cfg.base_addr = ul_base_addr;
    g_st_file_store_cfg.total_length = u_length;
}

oal_uint32 oal_get_no_fs_file_count(oal_void)
{
    oal_uint32 ul_len;

    ul_len = g_st_file_store_cfg.total_length;
    if ((ul_len >= FILE_STORE_MIN_SIZE) && (ul_len < FILE_STORE_MID_SIZE)) {
        return FILE_STORE_MIN_CNT;
    } else if ((ul_len >= FILE_STORE_MID_SIZE) && (ul_len < FILE_STORE_MAX_SIZE)) {
        return FILE_STORE_MID_CNT;
    } else if (ul_len >= FILE_STORE_MAX_SIZE) {
        return FILE_STORE_MAX_CNT;
    }
}


oal_file_stru* oal_no_fs_file_open(const oal_int8 *pc_path)
{
    oal_uint32 ul_index;
    oal_uint32 ul_total_file_count;
    oal_file_stru   *pst_file = NULL;
    pst_file = (oal_file_stru *)calloc(1, sizeof(oal_file_stru));
    if (pst_file == NULL)
        return NULL;
    PS_PRINT_DBG("open path:%s\n", pc_path);

    ul_total_file_count = OAL_MIN(oal_get_no_fs_file_count(),\
                                  sizeof(g_apst_file_store_info) / sizeof(struct st_file_store_info));
    for (ul_index = 0; ul_index < ul_total_file_count; ul_index++) {
        if (strcmp(g_apst_file_store_info[ul_index].pus_store_path, pc_path) == 0) {
            pst_file->ul_store_index = ul_index;
            pst_file->ul_pos = 0;
            return pst_file;
        }
    }

    PS_PRINT_WARNING("cannot find %s !\n", pc_path);
    free(pst_file);
    return NULL;
}


oal_int32 oal_no_fs_file_write(oal_file_stru *pst_file, oal_int8 *pc_string, oal_uint32 ul_length)
{
    oal_uint32 ul_index;
    oal_uint32 ul_free_szie;
    if (pst_file == OAL_PTR_NULL) {
        PS_PRINT_WARNING("fp is null!\n");
        return -OAL_EFAIL;
    }
    ul_index = pst_file->ul_store_index;

    if (g_apst_file_store_info[ul_index].ul_store_used >= g_apst_file_store_info[ul_index].ul_store_length) {
        if (spiflash_erase_proc(g_st_file_store_cfg.base_addr + \
                                g_apst_file_store_info[ul_index].ul_store_addr, \
                                g_apst_file_store_info[ul_index].ul_store_length) != 0) {
            PS_PRINT_WARNING("flash erase failed!\n");
            return -OAL_EFAIL;
        }
        g_apst_file_store_info[ul_index].ul_store_used = 0x0;
    }

    ul_free_szie = g_apst_file_store_info[ul_index].ul_store_length \
                   - g_apst_file_store_info[ul_index].ul_store_used;
    if (ul_length > ul_free_szie) {
        PS_PRINT_WARNING("space is not enough,free size is 0x%x, need size 0x%x!\n", ul_free_szie, ul_length);
        return -OAL_EFAIL;
    }
    PS_PRINT_DBG("str:%s, len: %x\n", pc_string, ul_length);

    if (spiflash_write_proc(pc_string, g_st_file_store_cfg.base_addr + \
                                 g_apst_file_store_info[ul_index].ul_store_addr + pst_file->ul_pos, ul_length) != 0) {
        PS_PRINT_WARNING("flash write failed!\n");
        return -OAL_EFAIL;
    }

    g_apst_file_store_info[ul_index].ul_store_used += ul_length;
    pst_file->ul_pos += ul_length;
    return ul_length;
}


oal_int32  oal_no_fs_file_read(oal_file_stru *pst_file, oal_int8 *pc_buf, oal_uint32 ul_length)
{
    oal_uint32 ul_index;
    if (pst_file == OAL_PTR_NULL) {
        PS_PRINT_WARNING("fp is null!\n");
        return -OAL_EFAIL;
    }
    ul_index = pst_file->ul_store_index;
    if (pst_file->ul_pos >= g_apst_file_store_info[ul_index].ul_store_used) {
        return 0; // read length is zero.
    }

    if ((pst_file->ul_pos + ul_length) > g_apst_file_store_info[ul_index].ul_store_used) {
        ul_length = g_apst_file_store_info[ul_index].ul_store_used - pst_file->ul_pos;
    }

    if (spiflash_read_proc(pc_buf, g_st_file_store_cfg.base_addr + \
                                g_apst_file_store_info[ul_index].ul_store_addr + pst_file->ul_pos, ul_length) != 0) {
        PS_PRINT_WARNING("flash read failed!\n");
        return -OAL_EFAIL;
    }
    pst_file->ul_pos += ul_length;
    return ul_length;
}


oal_void oal_no_fs_file_close(oal_file_stru *pst_file)
{
    free(pst_file);
    pst_file = OAL_PTR_NULL;
}


oal_int64  oal_no_fs_file_lseek(oal_file_stru *pst_file, oal_int64 offset, oal_int32 whence)
{
    oal_uint32 ul_index;
    oal_int32 ul_ret = OAL_SUCC;
    if (pst_file == OAL_PTR_NULL) {
        PS_PRINT_WARNING("fp is null!\n");
        return -OAL_EFAIL;
    }

    ul_index = pst_file->ul_store_index;

    switch (whence) {
        case OAL_SEEK_CUR:
        {
            // offset cur下继续执行set分支
            offset += pst_file->ul_pos;
        }
        case OAL_SEEK_SET:
        {
            if (offset >= 0) {
                pst_file->ul_pos = offset; /* Might be beyond the end-of-file */
                break;
            } else {
                ul_ret = -OAL_EFAIL;
                goto errout;
            }
            break;
        }
        case OAL_SEEK_END:
        {
            pst_file->ul_pos = (g_apst_file_store_info[ul_index].ul_store_used + offset);
            break;
        }

        default:
            ul_ret = -OAL_EFAIL;
            goto errout;
    }

    return (oal_int64)pst_file->ul_pos;

errout:
    return (oal_int64)ul_ret;
}

oal_int32  oal_no_fs_get_file_pos(oal_file_stru *pst_file)
{
    if (pst_file == OAL_PTR_NULL) {
        PS_PRINT_WARNING("fp is null!\n");
        return -OAL_EFAIL;
    }

    return pst_file->ul_pos;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

