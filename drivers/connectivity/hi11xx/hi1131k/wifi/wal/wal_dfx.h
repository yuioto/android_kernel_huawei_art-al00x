

#ifndef __WAL_DFX_H__
#define __WAL_DFX_H__

// 1 其他头文件包含
#include "oal_ext_if.h"
#include "oal_types.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_DFX_H

// 10 函数声明
extern oal_uint32 wal_dfr_excp_rx(oal_uint8 uc_device_id, oal_uint32 ul_exception_type);
#ifdef _PRE_WLAN_FEATURE_DFR
extern oal_void wal_dfr_init_param(oal_void);
#endif
extern oal_uint32 wal_dfx_init(void);
extern oal_void wal_dfx_exit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of wal_dfx.h */
