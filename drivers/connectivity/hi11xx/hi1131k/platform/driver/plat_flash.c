

#include "plat_flash.h"
#include "oam_ext_if.h"
#ifndef WIN32
#include    <linux/mtd/mtd.h>
#endif

#ifdef LOSCFG_DRIVERS_MTD_SPI_NOR
#ifndef WIN32

flash_iface_to_user g_flash_info = {0};

inline flash_iface_to_user *get_flash_info(oal_void)
{
    return &g_flash_info;
}

oal_int32 spiflash_erase_proc(oal_ulong ul_start, oal_ulong ul_size)
{
    flash_iface_to_user *pst_flash_ops_info = get_flash_info();
    if (pst_flash_ops_info->fcb_flash_erase == NULL) {
        dprintf("pst_flash_ops_info->fcb_flash_erase is NULL!\n");
        return -1;
    }
    return pst_flash_ops_info->fcb_flash_erase(ul_start, ul_size);
}

oal_int32 spiflash_write_proc(oal_void            *p_memaddr,
                              oal_ulong            ul_start,
                              oal_ulong            ul_size)
{
    flash_iface_to_user *pst_flash_ops_info = get_flash_info();
    if (pst_flash_ops_info->fcb_flash_write == NULL) {
        dprintf("pst_flash_ops_info->fcb_flash_write is NULL!\n");
        return -1;
    }
    return pst_flash_ops_info->fcb_flash_write(p_memaddr, ul_start, ul_size);
}

oal_int32 spiflash_read_proc(oal_void            *p_memaddr,
                             oal_ulong            ul_start,
                             oal_ulong            ul_size)
{
    flash_iface_to_user *pst_flash_ops_info = get_flash_info();
    if (pst_flash_ops_info->fcb_flash_read == NULL) {
        dprintf("pst_flash_ops_info->fcb_flash_read is NULL!\n");
        return -1;
    }
    return pst_flash_ops_info->fcb_flash_read(p_memaddr, ul_start, ul_size);
}
#else
oal_int32 spiflash_erase_proc(oal_ulong ul_start,  oal_ulong ul_size)
{
}

oal_int32 spiflash_write_proc(oal_void              *p_memaddr,
                              oal_ulong              ul_start,
                              oal_ulong              ul_size)
{
    return HISI_SUCC;
}

oal_int32 spiflash_read_proc(oal_void            *p_memaddr,
                             oal_ulong            ul_start,
                             oal_ulong            ul_size)
{
    return HISI_SUCC;
}

#endif
#endif

