

#ifndef __WAL_LINUX_EVENT_H__
#define __WAL_LINUX_EVENT_H__

// 1 其他头文件包含
#include "oal_ext_if.h"
#include "frw_ext_if.h"
#include "mac_vap.h"
#include "mac_device.h"
#include "hmac_ext_if.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_LINUX_EVENT_H

// 10 函数声明
extern oal_uint32 wal_cfg80211_start_scan(oal_net_device_stru *pst_net_dev,
                                          mac_cfg80211_scan_param_stru *pst_scan_param);
extern oal_uint32 wal_cfg80211_start_sched_scan(oal_net_device_stru *pst_net_dev,
                                                mac_pno_scan_stru *pst_pno_scan_info);
extern oal_int32 wal_cfg80211_start_connect(oal_net_device_stru *pst_net_dev,
                                            mac_cfg80211_connect_param_stru *pst_connect_param);
extern oal_int32 wal_cfg80211_start_disconnect(oal_net_device_stru *pst_net_dev,
                                               mac_cfg_kick_user_param_stru *pst_disconnect_param);
#ifdef _PRE_WLAN_FEATURE_SAE
extern int32_t wal_cfg80211_do_external_auth(oal_net_device_stru *netdev,
    hmac_external_auth_req_stru *ext_auth);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of wal_linux_event.h */
