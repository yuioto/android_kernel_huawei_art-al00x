
#ifndef __HMAC_SME_STA_H__
#define __HMAC_SME_STA_H__


// 1 其他头文件包含

#include "oal_types.h"
#include "hmac_vap.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_SME_STA_H

// 2 函数指针定义
typedef oal_void (*hmac_sme_handle_rsp_func)(hmac_vap_stru *pst_hmac_vap, const oal_uint8 *puc_msg);

// 3 枚举定义
/* 上报给SME结果 类型定义 */
typedef enum {
    HMAC_SME_SCAN_RSP,
    HMAC_SME_JOIN_RSP,
    HMAC_SME_AUTH_RSP,
    HMAC_SME_ASOC_RSP,

    HMAC_SME_RSP_BUTT
}hmac_sme_rsp_enum;
typedef oal_uint8 hmac_sme_rsp_enum_uint8;

typedef enum {
    HMAC_AP_SME_START_RSP = 0,

    HMAC_AP_SME_RSP_BUTT
}hmac_ap_sme_rsp_enum;
typedef oal_uint8 hmac_ap_sme_rsp_enum_uint8;

// 4 宏定义
#define     MAX_AUTH_CNT        3
#define     MAX_ASOC_CNT        5

// 5 函数声明
extern oal_void hmac_send_rsp_to_sme_sta(hmac_vap_stru *pst_hmac_vap, hmac_sme_rsp_enum_uint8 en_type,
    const oal_uint8 *puc_msg);
extern oal_void hmac_send_rsp_to_sme_ap(hmac_vap_stru *pst_hmac_vap, hmac_ap_sme_rsp_enum_uint8 en_type,
    oal_uint8 *puc_msg);
oal_void hmac_handle_scan_rsp_sta(hmac_vap_stru *pst_hmac_vap, const oal_uint8 *puc_msg);
oal_void hmac_handle_join_rsp_sta(hmac_vap_stru *pst_hmac_vap, const oal_uint8 *puc_msg);
oal_void hmac_handle_auth_rsp_sta(hmac_vap_stru *pst_hmac_vap, const oal_uint8 *puc_msg);
oal_void hmac_handle_asoc_rsp_sta(hmac_vap_stru *pst_hmac_vap, const oal_uint8 *puc_msg);
oal_uint32 hmac_send_connect_result_to_dmac_sta(hmac_vap_stru *pst_hmac_vap, oal_uint32 ul_result);
#ifdef _PRE_WLAN_FEATURE_SAE
extern uint32_t hmac_report_external_auth_req(hmac_vap_stru *hmac_vap, enum nl80211_external_auth_action action);
extern oal_void hmac_report_connect_failed_result(hmac_vap_stru *pst_hmac_vap, mac_status_code_enum_uint16 reason_code);
#endif /* _PRE_WLAN_FEATURE_SAE */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of hmac_sme_sta.h */
