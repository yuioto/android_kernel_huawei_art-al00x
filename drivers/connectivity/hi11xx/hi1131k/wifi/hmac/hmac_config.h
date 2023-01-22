
#ifndef __HMAC_CONFIG_H__
#define __HMAC_CONFIG_H__


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "mac_vap.h"
#include "mac_device.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CONFIG_H


/*****************************************************************************
  3 枚举定义
*****************************************************************************/
typedef enum {
    HMAC_MONITOR_SWITCH_OFF,
    HMAC_MONITOR_SWITCH_MCAST_DATA, // 上报组播(广播)数据包
    HMAC_MONITOR_SWITCH_UCAST_DATA, // 上报单播数据包
    HMAC_MONITOR_SWITCH_MCAST_MANAGEMENT, // 上报组播(广播)管理包
    HMAC_MONITOR_SWITCH_UCAST_MANAGEMENT, // 上报单播管理包
    HMAC_MONITOR_SWITCH_BUTT
}hmac_monitor_switch_enum;

#define REG_INFO_MAX_NUM    20
typedef struct {
    oal_uint32    ul_reg_info_num;
    oal_uint32    ul_flag;
    oal_uint32    ul_val[REG_INFO_MAX_NUM];
}hmac_reg_info_receive_event;


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/
/* hmac_vap结构中，一部分成员的大小，这些成员在linux和windows下的定义可能不同 */
typedef struct {
    oal_uint32      ul_hmac_vap_cfg_priv_stru_size;
    oal_uint32      ul_frw_timeout_stru_size;
    oal_uint32      ul_oal_spin_lock_stru_size;
    oal_uint32      ul_mac_key_mgmt_stru_size;
    oal_uint32      ul_mac_pmkid_cache_stru_size;
    oal_uint32      ul_mac_curr_rateset_stru_size;
    oal_uint32      ul_hmac_vap_stru_size;
}hmac_vap_member_size_stru;


/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint32 hmac_config_start_vap_event(mac_vap_stru  *pst_mac_vap, oal_bool_enum_uint8 en_mgmt_rate_init_flag);
extern oal_uint32 hmac_set_mode_event(mac_vap_stru *pst_mac_vap);
extern oal_uint32 hmac_config_update_opmode_event(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user,
    oal_uint8 uc_mgmt_frm_type);
extern oal_uint32  hmac_config_sta_update_rates(mac_vap_stru *pst_mac_vap, mac_cfg_mode_param_stru *pst_cfg_mode,
    mac_bss_dscr_stru *pst_bss_dscr);
extern oal_uint32  hmac_event_config_syn(frw_event_mem_stru *pst_event_mem);
extern oal_uint32  hmac_event_log_syn(frw_event_mem_stru *pst_event_mem);

extern oal_uint32  hmac_config_set_protection(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
extern oal_uint32 hmac_get_thruput_info(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_set_obss_scan_param(mac_vap_stru *pst_mac_vap);
extern oal_void hcc_msg_slave_thruput_bypass(oal_void);
#ifdef _PRE_WLAN_FEATURE_STA_PM
extern oal_uint32  hmac_config_sta_pm_on_syn(mac_vap_stru *pst_mac_vap);
extern oal_uint32  hmac_set_ipaddr_timeout(void   *puc_para);
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
extern oal_uint32 hmac_config_enable_arp_offload(mac_vap_stru *pst_mac_vap, oal_uint16 us_len,
    const oal_uint8 *puc_param);
extern oal_uint32 hmac_config_set_ip_addr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
extern oal_uint32 hmac_config_show_arpoffload_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len,
    const oal_uint8 *puc_param);
#endif
extern oal_uint32 hmac_config_send_sub_event(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_ROAM
oal_uint32 hmac_config_roam_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32 hmac_config_roam_org(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32 hmac_config_roam_band(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32 hmac_config_roam_start(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32 hmac_config_roam_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif // _PRE_WLAN_FEATURE_ROAM

#ifdef _PRE_WLAN_FEATURE_11R
oal_uint32 hmac_config_set_ft_ies(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif // _PRE_WLAN_FEATURE_11R

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
oal_uint32 hmac_config_enable_2040bss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
oal_uint32 hmac_config_set_auto_freq_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_STA_PM
oal_uint32  hmac_config_set_sta_pm_on(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
oal_uint32 hmac_config_set_sta_pm_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
oal_uint32 hmac_config_set_linkloss_threshold(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
oal_uint32 hmac_config_set_all_log_level(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
oal_uint32 hmac_config_set_d2h_hcc_assemble_cnt(mac_vap_stru *pst_mac_vap, oal_uint16 us_len,
    const oal_uint8 *puc_param);
oal_uint32 hmac_config_set_chn_est_ctrl(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
oal_uint32 hmac_config_set_power_ref(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
oal_uint32 hmac_config_set_pm_cfg_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
oal_uint32 hmac_config_set_cus_rf(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
oal_uint32  hmac_config_set_cus_dts_cali(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
oal_uint32  hmac_config_set_cus_nvram_params(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
oal_uint32 hmac_config_dev_customize_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

oal_uint32 hmac_config_mcs_set_check_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_APF
extern uint32_t hmac_config_apf_filter_cmd(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param);
#endif
extern uint32_t hmac_config_set_auth_rsp_time(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param);
extern uint32_t hmac_config_forbit_open_auth(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param);

#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif

#endif /* end of hmac_main */
