

#ifndef __HMAC_P2P_H__
#define __HMAC_P2P_H__

#include "hmac_fsm.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_P2P_H

// 1 枚举定义
/* p2p 状态码 */
typedef enum {
    P2P_STATUS_DISCOVERY_ON = 0,
    P2P_STATUS_SEARCH_ENABLED,
    P2P_STATUS_IF_ADD,
    P2P_STATUS_IF_DEL,
    P2P_STATUS_IF_DELETING,
    P2P_STATUS_IF_CHANGING,
    P2P_STATUS_IF_CHANGED,
    P2P_STATUS_LISTEN_EXPIRED,
    P2P_STATUS_ACTION_TX_COMPLETED,
    P2P_STATUS_ACTION_TX_NOACK,
    P2P_STATUS_SCANNING,
    P2P_STATUS_GO_NEG_PHASE,
    P2P_STATUS_DISC_IN_PROGRESS
} hmac_cfgp2p_status_enum;

// 2 函数声明
#ifdef _PRE_WLAN_FEATURE_P2P
extern uint32_t hmac_check_p2p_vap_num(mac_device_stru *pst_mac_device, wlan_p2p_mode_enum_uint8 en_p2p_mode);
extern uint32_t hmac_p2p_send_listen_expired_to_host(hmac_vap_stru *pst_hmac_vap);
extern uint32_t hmac_p2p_send_listen_expired_to_device(hmac_vap_stru *pst_hmac_vap);
extern uint32_t hmac_add_p2p_cl_vap(mac_vap_stru *pst_vap, uint16_t us_len, const uint8_t *puc_param);
extern uint32_t hmac_del_p2p_cl_vap(mac_vap_stru *pst_vap, uint16_t us_len, const uint8_t *puc_param);
extern uint32_t hmac_p2p_check_can_enter_state(mac_vap_stru *pst_mac_vap, hmac_fsm_input_type_enum_uint8 en_input_req);
extern uint32_t hmac_p2p_get_home_channel(mac_vap_stru *pst_mac_vap, uint32_t *pul_home_channel,
                                          wlan_channel_bandwidth_enum_uint8 *pen_home_channel_type);
extern void hmac_disable_p2p_pm(hmac_vap_stru *pst_hmac_vap);
extern oal_bool_enum_uint8 hmac_is_p2p_go_neg_req_frame(OAL_CONST uint8_t* puc_data);
extern uint8_t hmac_get_p2p_status(uint32_t ul_p2p_status, uint32_t en_status);
extern void hmac_set_p2p_status(uint32_t *pul_p2p_status, uint32_t en_status);
extern void hmac_clr_p2p_status(uint32_t *pul_p2p_status, uint32_t en_status);

#else   /* _PRE_WLAN_FEATURE_P2P */
OAL_STATIC OAL_INLINE uint32_t hmac_check_p2p_vap_num(mac_device_stru *pst_mac_device,
                                                      wlan_p2p_mode_enum_uint8 en_p2p_mode)
{
    return OAL_TRUE;
}

OAL_STATIC OAL_INLINE uint32_t hmac_p2p_send_listen_expired_to_host(hmac_vap_stru *pst_hmac_vap)
{
    return OAL_TRUE;
}

OAL_STATIC OAL_INLINE uint32_t hmac_p2p_send_listen_expired_to_device(hmac_vap_stru *pst_hmac_vap)
{
    return OAL_TRUE;
}

OAL_STATIC OAL_INLINE uint32_t hmac_p2p_check_can_enter_state(mac_vap_stru *pst_mac_vap,
                                                              hmac_fsm_input_type_enum_uint8 en_input_req)
{
    return OAL_SUCC;
}

OAL_STATIC OAL_INLINE uint32_t hmac_p2p_get_home_channel(mac_vap_stru *pst_mac_vap, uint32_t *pul_home_channel,
                                                         wlan_channel_bandwidth_enum_uint8 *pen_home_channel_type)
{
    return OAL_SUCC;
}

#endif  /* _PRE_WLAN_FEATURE_P2P */


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of hmac_p2p.h */
