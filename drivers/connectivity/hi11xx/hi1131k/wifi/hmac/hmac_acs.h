
#ifndef __HMAC_ACS_H__
#define __HMAC_ACS_H__


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "frw_ext_if.h"
#include "hmac_vap.h"
#include "mac_device.h"
#include "hmac_device.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_ACS_H
/* 2 宏定义 */
#define HMAC_ACS_RECHECK_INTERVAL   (15*60*1000) // 15 min

/* 10 函数声明 */
extern oal_uint32  hmac_acs_init_scan_hook(hmac_scan_record_stru   *pst_scan_record,
                                           hmac_device_stru        *pst_dev);
extern oal_uint32  hmac_acs_got_init_rank(hmac_device_stru *pst_hmac_device, mac_vap_stru *pst_mac_vap,
                                          mac_acs_cmd_stru *pst_cmd);

extern oal_uint32  hmac_acs_init(oal_void);
extern oal_uint32  hmac_acs_exit(oal_void);
#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif

#endif