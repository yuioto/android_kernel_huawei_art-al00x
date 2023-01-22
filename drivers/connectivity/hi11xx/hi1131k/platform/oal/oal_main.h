
#ifndef __OAL_MAIN_H__
#define __OAL_MAIN_H__

#include "oal_ext_if.h"
#include "oal_types.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

extern oal_int32  oal_main_init(oal_void);
extern oal_void  oal_main_exit(oal_void);
extern oal_uint32 oal_chip_get_version(oal_void);
extern oal_uint8 oal_chip_get_device_num(oal_uint32 ul_chip_ver);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_main */
