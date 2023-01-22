

#ifndef __WAL_CONFIG_ACS_H__
#define __WAL_CONFIG_ACS_H__

// 1 其他头文件包含
#include "oal_ext_if.h"
#include "wlan_types.h"
#include "wlan_spec.h"
#include "mac_vap.h"
#include "hmac_ext_if.h"
#include "wal_ext_if.h"
#include "wal_config.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_CONFIG_ACS_H

// 10 函数声明
extern oal_uint32 wal_acs_response_event_handler(frw_event_mem_stru *pst_event_mem);
extern oal_uint32 wal_acs_exit(oal_void);
extern oal_uint32 wal_acs_init(oal_void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of wal_config_acs.h */
