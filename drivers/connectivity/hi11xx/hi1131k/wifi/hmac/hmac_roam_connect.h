

#ifndef __HMAC_ROAM_CONNECT_H__
#define __HMAC_ROAM_CONNECT_H__

#ifdef _PRE_WLAN_FEATURE_ROAM

#include "oam_wdk.h"
#include "hmac_vap.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_ROAM_CONNECT_H
// 1 宏定义
#define ROAM_JOIN_TIME_MAX        (1 * 1000)       /* JOIN超时时间 单位ms */
#define ROAM_AUTH_TIME_MAX        (3 * 1000)       /* AUTH超时时间 单位ms */
#define ROAM_ASSOC_TIME_MAX       (3 * 1000)       /* ASSOC超时时间 单位ms */
#define ROAM_HANDSHAKE_TIME_MAX   (7 * 1000)       /* 握手超时时间 单位ms */

#define ROAM_MAC_ADDR(_puc_mac)   ((oal_uint32)(((oal_uint32)_puc_mac[2] << 24) |\
                                              ((oal_uint32)_puc_mac[3] << 16) |\
                                              ((oal_uint32)_puc_mac[4] << 8) |\
                                              ((oal_uint32)_puc_mac[5])))

// 2 枚举定义
/* 漫游connect状态机状态 */
typedef enum {
    ROAM_CONNECT_STATE_INIT            = 0,
    ROAM_CONNECT_STATE_FAIL            = ROAM_CONNECT_STATE_INIT,
    ROAM_CONNECT_STATE_WAIT_JOIN       = 1,
    ROAM_CONNECT_STATE_JOIN_COMP       = 2,
    ROAM_CONNECT_STATE_WAIT_FT_COMP    = 3,
    ROAM_CONNECT_STATE_WAIT_AUTH_COMP  = 4,
    ROAM_CONNECT_STATE_WAIT_ASSOC_COMP = 5,
    ROAM_CONNECT_STATE_HANDSHAKING     = 6,
    ROAM_CONNECT_STATE_UP              = 7,

    ROAM_CONNECT_STATE_BUTT
} roam_connect_state_enum;
typedef oal_uint8  roam_connect_state_enum_uint8;

/* 漫游connect状态机事件类型 */
typedef enum {
    ROAM_CONNECT_FSM_EVENT_START        = 0,
    ROAM_CONNECT_FSM_EVENT_TBTT         = 1,
    ROAM_CONNECT_FSM_EVENT_MGMT_RX      = 2,
    ROAM_CONNECT_FSM_EVENT_MGMT_FAIL_RX = 3,
    ROAM_CONNECT_FSM_EVENT_KEY_DONE     = 4,
    ROAM_CONNECT_FSM_EVENT_TIMEOUT      = 5,
    ROAM_CONNECT_FSM_EVENT_FT_OVER_DS   = 6,
    ROAM_CONNECT_FSM_EVENT_TYPE_BUTT
} roam_connect_fsm_event_type_enum;

// 3 函数声明
oal_uint32 hmac_roam_connect_set_join_reg(mac_vap_stru *pst_mac_vap);
oal_uint32 hmac_roam_connect_set_dtim_param(mac_vap_stru *pst_mac_vap, oal_uint32 ul_dtim_cnt,
    oal_uint32 ul_dtim_period, oal_uint16 us_tsf_bit0);
oal_uint32 hmac_roam_connect_timeout(oal_void *p_arg);
oal_uint32 hmac_roam_connect_start(hmac_vap_stru *pst_hmac_vap, mac_bss_dscr_stru *pst_bss_dscr);
oal_uint32 hmac_roam_connect_stop(hmac_vap_stru *pst_hmac_vap);
oal_void hmac_roam_connect_rx_tbtt(hmac_vap_stru *pst_hmac_vap);
oal_void hmac_roam_connect_rx_mgmt(hmac_vap_stru *pst_hmac_vap, dmac_wlan_crx_event_stru *pst_crx_event);
oal_void hmac_roam_connect_fsm_init(oal_void);
oal_void hmac_roam_connect_key_done(hmac_vap_stru *pst_hmac_vap);
#ifdef _PRE_WLAN_FEATURE_11R
oal_uint32 hmac_roam_connect_ft_reassoc(hmac_vap_stru *pst_hmac_vap);
#endif // _PRE_WLAN_FEATURE_11R

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif // _PRE_WLAN_FEATURE_ROAM

#endif /* end of hmac_roam_connect.h */
