
#ifndef __HMAC_RX_FILTER_H__
#define __HMAC_RX_FILTER_H__

// 1 其他头文件包含
#include "oal_types.h"
#include "mac_vap.h"
#include "mac_device.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_RX_FILTER_H

// 2 函数声明
extern oal_void hmac_rx_filter_init(oal_void);
extern oal_uint32 hmac_set_rx_filter_value(mac_vap_stru *pst_mac_vap);
extern oal_void hmac_rx_filter_exit(oal_void);
extern oal_uint32 hmac_calc_up_ap_num(mac_device_stru *pst_mac_device);

#ifdef _PRE_DEBUG_MODE
extern oal_void hmac_set_rx_filter_en(oal_uint8 uc_rx_filter_en);
extern oal_void hmac_get_rx_filter_en(oal_void);
extern oal_uint32 hmac_set_status_rx_filter_val(oal_ulong uc_dev_mode, oal_ulong uc_vap_mode, oal_ulong uc_vap_status,
    oal_uint32 uc_val);
extern oal_uint32 hmac_get_status_rx_filter_val(oal_ulong uc_dev_mode, oal_ulong uc_vap_mode, oal_ulong uc_vap_status);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of hmac_rx_filter.h */
