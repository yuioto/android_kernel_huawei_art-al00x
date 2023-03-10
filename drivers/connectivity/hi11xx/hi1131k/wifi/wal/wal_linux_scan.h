

#ifndef __WAL_LINUX_SCAN_H__
#define __WAL_LINUX_SCAN_H__

#include "oal_ext_if.h"
#include "frw_ext_if.h"
#include "hmac_device.h"
#include "wal_linux_rx_rsp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_LINUX_SCAN_H

extern oal_void wal_inform_all_bss(oal_wiphy_stru *pst_wiphy, hmac_bss_mgmt_stru *pst_bss_mgmt, oal_uint8 uc_vap_id);
extern oal_uint32 wal_scan_work_func(hmac_scan_stru *pst_scan_mgmt, oal_net_device_stru *pst_netdev,
    oal_cfg80211_scan_request_stru *pst_request);
extern oal_int32 wal_force_scan_complete(oal_net_device_stru *pst_net_dev, oal_bool_enum en_is_aborted);
extern oal_int32 wal_stop_sched_scan(oal_net_device_stru *pst_netdev);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
extern void wal_update_bss(oal_wiphy_stru *wiphy, hmac_bss_mgmt_stru *bss_mgmt, uint8_t *bssid);
#endif
#define IS_P2P_SCAN_REQ(pst_request) ((pst_request->n_ssids > 0) && (NULL != pst_request->ssids) && \
        (pst_request->ssids[0].ssid_len == OAL_STRLEN("DIRECT-")) && \
        (0 == oal_memcmp(pst_request->ssids[0].ssid, "DIRECT-", OAL_STRLEN("DIRECT-"))))

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of wal_linux_scan.h */
