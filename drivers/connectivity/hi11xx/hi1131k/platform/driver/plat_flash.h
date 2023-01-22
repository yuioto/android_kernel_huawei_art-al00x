

#ifndef __PLAT_FLASH_H__
#define __PLAT_FLASH_H__
/*****************************************************************************
  1 Include other Head file
*****************************************************************************/
#include "oal_types.h"

/*****************************************************************************
  2 Define macro
*****************************************************************************/
#define MAX_DATA_NAME_LEN        22
#define VALUE_ZERO               0
#define VALUE_ONE                1

/*****************************************************************************
  3 STRUCT DEFINE
*****************************************************************************/
typedef struct {
    int (*fcb_flash_erase)(unsigned long  ul_start, unsigned long  ul_size);
    int (*fcb_flash_write)(void *p_memaddr, unsigned long   ul_start, unsigned long  ul_size);
    int (*fcb_flash_read)(void *p_memaddr, unsigned long  ul_start, unsigned long  ul_size);
}flash_iface_to_user;
/*****************************************************************************
  4 EXTERN VARIABLE
*****************************************************************************/
/*****************************************************************************
  5 EXTERN FUNCTION
*****************************************************************************/
extern flash_iface_to_user *get_flash_info(oal_void);

extern oal_int32 spiflash_erase_proc(oal_ulong ul_start, oal_ulong ul_size);
extern oal_int32 spiflash_write_proc(oal_void            *p_memaddr,
                                     oal_ulong            ul_start,
                                     oal_ulong            ul_size);
extern oal_int32 spiflash_read_proc(oal_void            *p_memaddr,
                                    oal_ulong            ul_start,
                                    oal_ulong            ul_size);
#endif

