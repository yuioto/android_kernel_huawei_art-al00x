

// 1 头文件包含
#include "oal_types.h"
#include "oal_ext_if.h"
#include "oam_ext_if.h"
#include "oal_util.h"
#include "wlan_types.h"
#include "securec.h"

#include "mac_device.h"
#include "mac_vap.h"
#include "mac_resource.h"

#include "hmac_resource.h"
#include "hmac_device.h"
#include "hmac_scan.h"
#include "hmac_ext_if.h"
#include "hmac_config.h"
#include "wal_ext_if.h"
#include "wal_main.h"
#include "wal_config.h"
#include "wal_linux_bridge.h"
#include "oam_ext_if.h"

#if(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
#include "hmac_cali_mgmt.h"
#endif

#ifdef _PRE_WLAN_CHIP_TEST
#include "hmac_test_main.h"
#endif

#ifdef _PRE_WLAN_DFT_REG
#include "hal_witp_debug.h"
#endif

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
#include "wal_proxysta.h"
#endif

#ifdef _PRE_WLAN_FEATURE_MCAST
#include "hmac_m2u.h"
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
#include "hmac_proxy_arp.h"
#endif
#ifdef _PRE_WLAN_FEATURE_WAPI
#include "hmac_wapi.h"
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || \
    (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION))
#include "plat_pm_wlan.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_CONFIG_C

// 2 全局变量定义
/* 静态函数声明 */
#ifdef _PRE_WLAN_FEATURE_PM
OAL_STATIC oal_uint32 wal_config_wifi_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_pm_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_pm_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif

OAL_STATIC oal_uint32 wal_config_add_vap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_del_vap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_down_vap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_start_vap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_bandwidth(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_fem_pa_status(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);

OAL_STATIC oal_uint32 wal_config_set_mac_addr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_concurrent(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_concurrent(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_bss_type(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_bss_type(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_ssid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_ssid(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_shortgi20(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_shortgi20(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_shortgi40(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_shortgi40(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_shortgi80(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_shortgi80(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_shpreamble(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_shpreamble(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_MONITOR
OAL_STATIC oal_uint32 wal_config_get_addr_filter(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_addr_filter(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
#endif
OAL_STATIC oal_uint32 wal_config_get_prot_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_prot_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_auth_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_auth_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_bintval(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_bintval(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_nobeacon(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_nobeacon(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_txchain(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_txchain(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_rxchain(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_rxchain(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_txpower(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_txpower(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_freq(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_freq(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_wmm_params(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_vap_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_event_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_eth_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_80211_ucast_switch(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);

#ifdef _PRE_DEBUG_MODE_USER_TRACK
OAL_STATIC oal_uint32 wal_config_report_thrput_stat(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_TXOPPS
OAL_STATIC oal_uint32 wal_config_set_txop_ps_machw(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_BTCOEX
OAL_STATIC oal_uint32 wal_config_print_btcoex_status(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_LTECOEX
OAL_STATIC uint32_t wal_config_ltecoex_mode_set(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
#endif

OAL_STATIC oal_uint32 wal_config_80211_mcast_switch(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_probe_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_rssi_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
OAL_STATIC uint32_t wal_config_lte_gpio_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_report_vap_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
OAL_STATIC uint32_t wal_config_wfa_cfg_aifsn(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_wfa_cfg_cw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);

#endif

OAL_STATIC oal_uint32 wal_config_get_mpdu_num(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);

#ifdef _PRE_WLAN_CHIP_TEST
OAL_STATIC oal_uint32 wal_config_beacon_offload_test(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32 wal_config_ota_beacon_switch(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_ota_rx_dscr_switch(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_all_ota(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_dhcp_arp_switch(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_random_mac_addr_scan(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_oam_output(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_add_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_del_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_ampdu_start(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_amsdu_start(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_ampdu_end(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_auto_ba_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_profiling_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
OAL_STATIC oal_uint32 wal_config_addba_req(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_delba_req(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_list_ap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_list_sta(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_sta_list(mac_vap_stru *pst_mac_vap, oal_uint16 *us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_list_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_rd_pwr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_reduce_sar(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_start_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_start_join(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_start_deauth(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_dump_timer(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_kick_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_ampdu_tx_on(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_pause_tid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_user_vip(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_vap_host(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_send_bar(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_dump_ba_bitmap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_vap_pkt_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_dtimperiod(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_dtimperiod(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_amsdu_ampdu_switch(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_amsdu_tx_on(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_dump_all_rx_dscr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
OAL_STATIC uint32_t wal_config_get_hipkt_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_flowctl_param(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_get_flowctl_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
#endif

#ifdef _PRE_WLAN_RF_110X_CALI_DPD
OAL_STATIC oal_uint32 wal_config_start_dpd(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_SMPS
OAL_STATIC uint32_t wal_config_set_smps_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_smps_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_smps_en(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
#endif


#ifdef _PRE_WLAN_FEATURE_UAPSD
OAL_STATIC oal_uint32 wal_config_set_uapsd_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_uapsd_en(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);

#endif

OAL_STATIC oal_uint32 wal_config_set_country(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_DFS    // 1131_debug
OAL_STATIC oal_uint32 wal_config_set_country_for_dfs(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32 wal_config_get_country(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_11D
OAL_STATIC oal_uint32 wal_config_set_rd_by_ie_switch(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32 wal_config_get_tid(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
oal_uint32 wal_config_set_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_beacon(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_reset_hw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_reset_operate(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);

#ifdef _PRE_WLAN_FEATURE_UAPSD
OAL_STATIC oal_uint32 wal_config_uapsd_debug(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_DFT_STAT
OAL_STATIC uint32_t wal_config_set_phy_stat_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
OAL_STATIC uint32_t wal_config_dbb_env_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_usr_queue_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_report_vap_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
OAL_STATIC uint32_t wal_config_report_all_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);

#endif
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32 wal_config_show_device_memleak(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_show_device_meminfo(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32 wal_config_add_key(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_key(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_remove_key(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_default_key(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
OAL_STATIC oal_uint32 wal_config_timer_start(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_scan_abort(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_cfg80211_start_sched_scan(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_cfg80211_stop_sched_scan(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_cfg80211_start_scan(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_cfg80211_start_join(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_alg_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_beacon_chain_switch(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);

OAL_STATIC oal_uint32 wal_config_2040_channel_switch_prohibited(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_FortyMHzIntolerant(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_2040_coext_support(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_rx_fcs_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);

#ifdef _PRE_WLAN_FEATURE_DFS
OAL_STATIC uint32_t wal_config_dfs_radartool(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
#ifdef _PRE_SUPPORT_ACS
OAL_STATIC oal_uint32 wal_config_acs(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif

OAL_STATIC uint32_t wal_config_show_profiling(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_dump_rx_dscr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_dump_tx_dscr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_dump_memory(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_txbf_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_frag_threshold(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_rts_threshold(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_user_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_dscr_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);

OAL_STATIC uint32_t wal_config_set_log_level(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_feature_log(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
OAL_STATIC uint32_t wal_config_set_log_lowpower(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
OAL_STATIC uint32_t wal_config_set_pm_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32 wal_config_set_rate(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_mcs(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_mcsac(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_nss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_rfch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_bw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#if defined (_PRE_WLAN_CHIP_TEST) || defined (_PRE_WLAN_FEATURE_ALWAYS_TX)
OAL_STATIC oal_uint32 wal_config_always_tx(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_bcast_pkt(mac_vap_stru *pst_mac_vap, oal_uint32 ul_payload_len);
#endif  /* _PRE_WLAN_CHIP_TEST */
#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
OAL_STATIC uint32_t wal_config_always_tx_1102(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
#endif
OAL_STATIC oal_uint32 wal_config_always_rx(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32 wal_config_dync_txpower(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32 wal_config_get_thruput(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_freq_skew(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_adjust_ppm(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_reg_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_dbb_scaling_amend(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);

#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST))
OAL_STATIC uint32_t wal_config_sdio_flowctrl(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_HILINK
OAL_STATIC oal_uint32 wal_config_set_monitor_switch(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32 wal_config_mips_cycle_switch(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_reg_write(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32 wal_config_resume_rx_intr_fifo(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
#endif

OAL_STATIC oal_uint32 wal_config_alg(mac_vap_stru *pst_mac_vap, oal_uint16 pus_len, const oal_uint8 *puc_param);

#ifdef _PRE_WLAN_PERFORM_STAT
OAL_STATIC oal_uint32 wal_config_pfm_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_pfm_display(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
OAL_STATIC oal_uint32 wal_config_set_edca_opt_weight_sta(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_edca_opt_switch_sta(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_edca_opt_switch_ap(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_edca_opt_cycle_ap(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_CHIP_TEST
OAL_STATIC oal_uint32 wal_config_lpm_tx_data(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_lpm_tx_probe_request(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_lpm_chip_state(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_lpm_soc_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
OAL_STATIC oal_uint32 wal_config_set_lpm_psm_param(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_lpm_smps_mode(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_lpm_smps_stub(mac_vap_stru *pst_mac_vap,
    al_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_lpm_txop_ps(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
OAL_STATIC oal_uint32 wal_config_set_lpm_txop_tx_stub(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_lpm_wow_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_remove_user_lut(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
OAL_STATIC oal_uint32 wal_config_send_frame(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_rx_pn(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_soft_retry(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_open_addr4(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_open_wmm_test(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_chip_test_open(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_coex(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_dfx(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_send_pspoll(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_send_nulldata(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_send_action(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_clear_all_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
OAL_STATIC oal_uint32 wal_config_enable_pmf(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */

OAL_STATIC oal_uint32 wal_config_open_wmm(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_hide_ssid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_mib(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_mib(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_thruput_bypass(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_auto_protection(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_send_2040_coext(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
OAL_STATIC uint32_t wal_config_2040_coext_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
OAL_STATIC oal_uint32 wal_config_get_version(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32 wal_config_get_all_reg_value(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_DAQ
OAL_STATIC oal_uint32 wal_config_data_acq(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_SMPS
#ifdef _PRE_DEBUG_MODE
OAL_STATIC uint32_t wal_config_get_smps_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
OAL_STATIC oal_uint32 wal_config_set_oma(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_proxysta_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
#endif

#ifdef _PRE_WLAN_DFT_REG
oal_uint32  wal_config_dump_reg(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
OAL_STATIC uint32_t wal_config_set_wps_p2p_ie(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_wps_ie(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);

#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
OAL_STATIC uint32_t wal_config_blacklist_add(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_blacklist_add_only(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_blacklist_del(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_blacklist_mode(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_blacklist_show(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_autoblacklist_enable(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_autoblacklist_aging(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_autoblacklist_threshold(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_autoblacklist_reset_time(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_isolation_mode(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_isolation_type(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_isolation_forward(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_isolation_clear(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_isolation_show(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
OAL_STATIC oal_uint32 wal_config_set_opmode_notify(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_get_user_rssbw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);

#endif

OAL_STATIC oal_uint32 wal_config_set_vap_nss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_MCAST
OAL_STATIC oal_uint32 wal_config_m2u_snoop_on(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_add_m2u_deny_table(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_m2u_deny_table(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_show_m2u_snoop_table(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_igmp_packet_xmit(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
#endif

#ifdef _PRE_DEBUG_MODE
OAL_STATIC uint32_t wal_config_rx_filter_val(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_rx_filter_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
OAL_STATIC uint32_t wal_config_get_rx_filter_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
OAL_STATIC oal_uint32 wal_config_report_ampdu_stat(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
#endif

OAL_STATIC oal_uint32 wal_config_set_ampdu_aggr_num(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32 wal_config_freq_adjust(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif

OAL_STATIC oal_uint32 wal_config_set_stbc_cap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_ldpc_cap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
OAL_STATIC oal_uint32 wal_config_proxyarp_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);

#ifdef _PRE_DEBUG_MODE
OAL_STATIC uint32_t wal_config_proxyarp_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif

#endif

#ifdef _PRE_WLAN_FEATURE_WAPI
#ifdef _PRE_WAPI_DEBUG
OAL_STATIC oal_uint32 wal_config_wapi_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
#endif /* #ifdef _PRE_WLAN_FEATURE_WAPI */

OAL_STATIC oal_uint32 wal_config_remain_on_channel(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_cancel_remain_on_channel(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_mgmt_tx(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);

OAL_STATIC uint32_t wal_config_vap_classify_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
OAL_STATIC uint32_t wal_config_vap_classify_tid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32 wal_config_scan_test(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
OAL_STATIC uint32_t wal_config_bgscan_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
OAL_STATIC oal_uint32 wal_config_query_station_stats(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_HS20
OAL_STATIC oal_uint32  wal_config_set_qos_map(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_P2P
OAL_STATIC uint32_t wal_config_set_p2p_ps_ops(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_p2p_ps_noa(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_p2p_ps_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_STA_PM
OAL_STATIC uint32_t wal_config_set_sta_pm_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
#ifdef _PRE_PSM_DEBUG_MODE
OAL_STATIC oal_uint32 wal_config_show_pm_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32 wal_config_set_pm_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_sta_pm_on(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_STA_UAPSD
OAL_STATIC uint32_t wal_config_set_uapsd_para(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
#endif

#ifdef _PRE_WLAN_PROFLING_MIPS
OAL_STATIC oal_uint32 wal_config_set_mips(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_show_mips(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32 wal_config_set_max_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);

#if ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 35))) || \
    (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#ifdef _PRE_WLAN_DFT_STAT
OAL_STATIC oal_uint32 wal_config_set_performance_log_switch(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
#endif
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32 wal_config_set_ampdu_mmss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
OAL_STATIC oal_uint32 wal_config_enable_arp_offload(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_show_arpoffload_info(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32 wal_config_cfg_vap_h2d(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_host_dev_init(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
OAL_STATIC uint32_t wal_config_host_dev_exit(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);

oal_uint32 wal_send_cali_data(oal_net_device_stru *pst_net_dev);

#endif
#ifdef _PRE_WLAN_TCP_OPT
OAL_STATIC oal_uint32  wal_config_get_tcp_ack_stream_info(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_tx_tcp_ack_opt_enable(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_rx_tcp_ack_opt_enable(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_tx_tcp_ack_limit(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
OAL_STATIC uint32_t wal_config_rx_tcp_ack_limit(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_ROAM
OAL_STATIC oal_uint32 wal_config_roam_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_roam_band(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_roam_org(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_roam_start(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_roam_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif // _PRE_WLAN_FEATURE_ROAM
#ifdef _PRE_WLAN_FEATURE_11R
OAL_STATIC oal_uint32 wal_config_set_ft_ies(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#endif // _PRE_WLAN_FEATURE_11R

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
OAL_STATIC uint32_t wal_config_enable_2040bss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
OAL_STATIC oal_uint32 wal_config_set_auto_freq_enable(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
#endif
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
OAL_STATIC oal_uint32 wal_config_get_lauch_cap(mac_vap_stru *pst_mac_vap, oal_uint16 *us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_linkloss_threshold(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_all_log_level(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const uint8_t *puc_param);
OAL_STATIC oal_uint32 wal_config_set_d2h_hcc_assemble_cnt(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_chn_est_ctrl(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
OAL_STATIC uint32_t wal_config_set_power_ref(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
OAL_STATIC uint32_t wal_config_set_pm_cfg_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
OAL_STATIC oal_uint32 wal_config_set_cus_rf(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC uint32_t wal_config_set_cus_dts_cali(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
OAL_STATIC oal_uint32 wal_config_set_cus_nvram_params(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_dev_customize_info(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */
OAL_STATIC oal_uint32  wal_config_vap_destroy(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);

#ifdef _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN
OAL_STATIC oal_uint32  wal_config_set_tx_classify_switch(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param);
#endif  /* _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifndef CONFIG_HAS_EARLYSUSPEND
OAL_STATIC uint32_t wal_config_set_suspend_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param);
#endif
#endif
#ifdef _PRE_WLAN_FEATURE_IP_FILTER
OAL_STATIC uint32_t wal_config_update_ip_filter(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *puc_param);
static uint32_t wal_config_assigned_filter(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param);
#endif
OAL_STATIC uint32_t wal_config_set_high_power_switch(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param);
#ifdef _PRE_WLAN_FEATURE_1131K_BTCOEX
OAL_STATIC uint32_t wal_config_set_btcoex_channel_switch(mac_vap_stru *mac_vap,
                                                         uint16_t len, const uint8_t *param);
#endif
OAL_STATIC uint32_t wal_rx_filter_frag(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param);
OAL_STATIC uint32_t wal_get_psm_info(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param);
OAL_STATIC uint32_t wal_get_psm_bcn_info(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_atomic g_wal_config_seq_num = ATOMIC_INIT(0);
oal_module_symbol(g_wal_config_seq_num);
#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
oal_atomic g_wal_config_seq_num = OAL_ATOMIC_INIT(0);
#else
oal_atomic g_wal_config_seq_num = 0;
#endif

#ifdef _PRE_WLAN_CFGID_DEBUG
OAL_STATIC oal_uint32 wal_config_mcs_set_check_enable(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
#endif

#ifdef _PRE_WLAN_FEATURE_APF
OAL_STATIC uint32_t wal_config_apf_filter_cmd(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_SAE
OAL_STATIC uint32_t wal_config_external_auth(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param);
#endif /* _PRE_WLAN_FEATURE_SAE */
OAL_STATIC uint32_t wal_config_set_auth_rsp_time(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param);
OAL_STATIC uint32_t wal_config_forbit_open_auth(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param);

/* cfgid操作全局变量 */
OAL_STATIC OAL_CONST wal_wid_op_stru g_ast_board_wid_op[] = {
     /* cfgid                   是否复位mac  保留一字节   get函数              set函数 */
    { WLAN_CFGID_BSS_TYPE,          OAL_TRUE,   {0},   wal_config_get_bss_type,   wal_config_set_bss_type },
    { WLAN_CFGID_ADD_VAP,           OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_add_vap },
    { WLAN_CFGID_START_VAP,         OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_start_vap },
    { WLAN_CFGID_DEL_VAP,           OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_del_vap },
    { WLAN_CFGID_DOWN_VAP,          OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_down_vap },
    { WLAN_CFGID_MODE,              OAL_FALSE,  {0},   wal_config_get_mode,       wal_config_set_mode },
    { WLAN_CFGID_BANDWIDTH,         OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_set_bandwidth },
    { WLAN_CFGID_CHECK_FEM_PA,      OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_get_fem_pa_status },

    { WLAN_CFGID_CURRENT_CHANEL,    OAL_FALSE,  {0},   wal_config_get_freq,       wal_config_set_freq },
    { WLAN_CFGID_STATION_ID,        OAL_TRUE,   {0},   OAL_PTR_NULL,              wal_config_set_mac_addr },
    { WLAN_CFGID_CONCURRENT,        OAL_TRUE,   {0},   wal_config_get_concurrent, wal_config_set_concurrent },
    { WLAN_CFGID_SSID,              OAL_FALSE,  {0},   wal_config_get_ssid,       wal_config_set_ssid },
    { WLAN_CFGID_SHORTGI,           OAL_FALSE,  {0},   wal_config_get_shortgi20,  wal_config_set_shortgi20 },
    { WLAN_CFGID_SHORTGI_FORTY,     OAL_FALSE,  {0},   wal_config_get_shortgi40,  wal_config_set_shortgi40 },
    { WLAN_CFGID_SHORTGI_EIGHTY,    OAL_FALSE,  {0},   wal_config_get_shortgi80,  wal_config_set_shortgi80 },

    { WLAN_CFGID_SHORT_PREAMBLE,    OAL_FALSE,  {0},   wal_config_get_shpreamble, wal_config_set_shpreamble },
#ifdef _PRE_WLAN_FEATURE_MONITOR
    { WLAN_CFGID_ADDR_FILTER,       OAL_FALSE,  {0},   wal_config_get_addr_filter, wal_config_set_addr_filter },
#endif
    { WLAN_CFGID_PROT_MODE,         OAL_FALSE,  {0},   wal_config_get_prot_mode,  wal_config_set_prot_mode },
    { WLAN_CFGID_AUTH_MODE,         OAL_FALSE,  {0},   wal_config_get_auth_mode,  wal_config_set_auth_mode },
    { WLAN_CFGID_BEACON_INTERVAL,   OAL_FALSE,  {0},   wal_config_get_bintval,    wal_config_set_bintval },
    { WLAN_CFGID_NO_BEACON,         OAL_FALSE,  {0},   wal_config_get_nobeacon,   wal_config_set_nobeacon },
    { WLAN_CFGID_TX_CHAIN,          OAL_FALSE,  {0},   wal_config_get_txchain,    wal_config_set_txchain },
    { WLAN_CFGID_RX_CHAIN,          OAL_FALSE,  {0},   wal_config_get_rxchain,    wal_config_set_rxchain },
    { WLAN_CFGID_TX_POWER,          OAL_FALSE,  {0},   wal_config_get_txpower,    wal_config_set_txpower },
#ifdef _PRE_WLAN_FEATURE_SMPS
    { WLAN_CFGID_SMPS_MODE,         OAL_FALSE,  {0},   wal_config_get_smps_mode,  wal_config_set_smps_mode },
    { WLAN_CFGID_SMPS_EN,           OAL_FALSE,  {0},   wal_config_get_smps_en,    OAL_PTR_NULL },
#endif
#ifdef _PRE_WLAN_FEATURE_UAPSD
    { WLAN_CFGID_UAPSD_EN,          OAL_FALSE,  {0},   wal_config_get_uapsd_en,   wal_config_set_uapsd_en },
#endif
    { WLAN_CFGID_DTIM_PERIOD,       OAL_FALSE,  {0},   wal_config_get_dtimperiod, wal_config_set_dtimperiod },

    { WLAN_CFGID_EDCA_TABLE_CWMIN,          OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params },
    { WLAN_CFGID_EDCA_TABLE_CWMAX,          OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params },
    { WLAN_CFGID_EDCA_TABLE_AIFSN,          OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params },
    { WLAN_CFGID_EDCA_TABLE_TXOP_LIMIT,     OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params },
    { WLAN_CFGID_EDCA_TABLE_MSDU_LIFETIME,  OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params },
    { WLAN_CFGID_EDCA_TABLE_MANDATORY,      OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params },
    { WLAN_CFGID_QEDCA_TABLE_CWMIN,         OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params },
    { WLAN_CFGID_QEDCA_TABLE_CWMAX,         OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params },
    { WLAN_CFGID_QEDCA_TABLE_AIFSN,         OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params },
    { WLAN_CFGID_QEDCA_TABLE_TXOP_LIMIT,    OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params },
    { WLAN_CFGID_QEDCA_TABLE_MSDU_LIFETIME, OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params },
    { WLAN_CFGID_QEDCA_TABLE_MANDATORY,     OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params },

    { WLAN_CFGID_VAP_INFO,               OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_vap_info },
    { WLAN_CFGID_EVENT_SWITCH,           OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_event_switch },
    { WLAN_CFGID_ETH_SWITCH,             OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_eth_switch },
    { WLAN_CFGID_80211_UCAST_SWITCH,     OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_80211_ucast_switch },

#ifdef _PRE_DEBUG_MODE_USER_TRACK
    { WLAN_CFGID_USR_THRPUT_STAT,        OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_report_thrput_stat },
#endif
#ifdef _PRE_WLAN_FEATURE_TXOPPS
    { WLAN_CFGID_TXOP_PS_MACHW,          OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_set_txop_ps_machw },
#endif
#ifdef _PRE_WLAN_FEATURE_BTCOEX
    { WLAN_CFGID_BTCOEX_STATUS_PRINT,    OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_print_btcoex_status },
#endif
#ifdef _PRE_WLAN_FEATURE_LTECOEX
    { WLAN_CFGID_LTECOEX_MODE_SET,       OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_ltecoex_mode_set },
#endif
    { WLAN_CFGID_80211_MCAST_SWITCH,     OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_80211_mcast_switch },
    { WLAN_CFGID_PROBE_SWITCH,           OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_probe_switch },
    { WLAN_CFGID_RSSI_SWITCH,            OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_rssi_switch },
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    { WLAN_CFGID_REPORT_VAP_INFO,        OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_report_vap_info },
    { WLAN_CFGID_WFA_CFG_AIFSN,          OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_wfa_cfg_aifsn },
    { WLAN_CFGID_WFA_CFG_CW,             OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_wfa_cfg_cw },
    { WLAN_CFGID_CHECK_LTE_GPIO,         OAL_FALSE,  {0},   OAL_PTR_NULL,        wal_config_lte_gpio_mode },
#endif
    { WLAN_CFGID_GET_MPDU_NUM,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_get_mpdu_num },
#ifdef _PRE_WLAN_CHIP_TEST
    { WLAN_CFGID_SET_BEACON_OFFLOAD_TEST, OAL_FALSE, {0},    OAL_PTR_NULL, wal_config_beacon_offload_test },
#endif
    { WLAN_CFGID_OTA_BEACON_SWITCH,      OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_ota_beacon_switch },
    { WLAN_CFGID_OTA_RX_DSCR_SWITCH,     OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_ota_rx_dscr_switch },
    { WLAN_CFGID_SET_ALL_OTA,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_all_ota },
    { WLAN_CFGID_SET_DHCP_ARP,           OAL_FALSE,  {0},    OAL_PTR_NULL, wal_config_set_dhcp_arp_switch },
    { WLAN_CFGID_SET_RANDOM_MAC_ADDR_SCAN, OAL_FALSE,  {0},    OAL_PTR_NULL, wal_config_set_random_mac_addr_scan },

#ifdef _PRE_WLAN_RF_110X_CALI_DPD
    { WLAN_CFGID_START_DPD,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_start_dpd },
#endif
    { WLAN_CFGID_OAM_OUTPUT_TYPE,        OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_oam_output },
    { WLAN_CFGID_ADD_USER,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_add_user },
    { WLAN_CFGID_DEL_USER,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_del_user },
    { WLAN_CFGID_AMPDU_START,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_ampdu_start },
    { WLAN_CFGID_AMSDU_START,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_amsdu_start },
    { WLAN_CFGID_AMPDU_END,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_ampdu_end },
    { WLAN_CFGID_AUTO_BA_SWITCH,         OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_auto_ba_switch },
    { WLAN_CFGID_PROFILING_SWITCH,       OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_profiling_switch },
    { WLAN_CFGID_ADDBA_REQ,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_addba_req },
    { WLAN_CFGID_DELBA_REQ,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_delba_req },
    { WLAN_CFGID_SET_LOG_LEVEL,          OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_log_level },
    { WLAN_CFGID_SET_FEATURE_LOG,        OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_feature_log },
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    { WLAN_CFGID_SET_LOG_PM,             OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_log_lowpower },
    { WLAN_CFGID_SET_PM_SWITCH,          OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_pm_switch },
#endif
    { WLAN_CFGID_LIST_AP,                OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_list_ap },
    { WLAN_CFGID_LIST_STA,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_list_sta },
    { WLAN_CFGID_DUMP_ALL_RX_DSCR,       OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_dump_all_rx_dscr },
    { WLAN_CFGID_START_SCAN,             OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_start_scan },
    { WLAN_CFGID_START_JOIN,             OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_start_join },
    { WLAN_CFGID_START_DEAUTH,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_start_deauth },
    { WLAN_CFGID_DUMP_TIEMR,             OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_dump_timer },
    { WLAN_CFGID_KICK_USER,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_kick_user },
    { WLAN_CFGID_PAUSE_TID,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_pause_tid },
    { WLAN_CFGID_SET_USER_VIP,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_user_vip },
    { WLAN_CFGID_SET_VAP_HOST,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_vap_host },
    { WLAN_CFGID_AMPDU_TX_ON,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_ampdu_tx_on },
    { WLAN_CFGID_AMSDU_TX_ON,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_amsdu_tx_on },
    { WLAN_CFGID_SEND_BAR,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_send_bar },
    { WLAN_CFGID_DUMP_BA_BITMAP,         OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_dump_ba_bitmap },
    { WLAN_CFGID_VAP_PKT_STAT,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_vap_pkt_stat },
    { WLAN_CFGID_TIMER_START,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_timer_start },
    { WLAN_CFGID_SHOW_PROFILING,         OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_show_profiling },
    { WLAN_CFGID_AMSDU_AMPDU_SWITCH,     OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_amsdu_ampdu_switch },
    { WLAN_CFGID_COUNTRY,                OAL_FALSE,  {0},    wal_config_get_country,  wal_config_set_country },
#ifdef _PRE_WLAN_FEATURE_DFS    // 1131_debug
    { WLAN_CFGID_COUNTRY_FOR_DFS,        OAL_FALSE,  {0},    OAL_PTR_NULL, wal_config_set_country_for_dfs },
#endif
    { WLAN_CFGID_TID,                    OAL_FALSE,  {0},    wal_config_get_tid,      OAL_PTR_NULL },
    { WLAN_CFGID_RESET_HW,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_reset_hw },
    { WLAN_CFGID_RESET_HW_OPERATE,       OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_reset_operate },

#ifdef _PRE_WLAN_FEATURE_UAPSD
    { WLAN_CFGID_UAPSD_DEBUG,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_uapsd_debug },
#endif
#ifdef _PRE_WLAN_DFT_STAT
    { WLAN_CFGID_PHY_STAT_EN,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_phy_stat_en },
    { WLAN_CFGID_DBB_ENV_PARAM,          OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_dbb_env_param },
    { WLAN_CFGID_USR_QUEUE_STAT,         OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_usr_queue_stat },
    { WLAN_CFGID_VAP_STAT,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_report_vap_stat },
    { WLAN_CFGID_ALL_STAT,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_report_all_stat },
#endif

    { WLAN_CFGID_DUMP_RX_DSCR,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_dump_rx_dscr },
    { WLAN_CFGID_DUMP_TX_DSCR,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_dump_tx_dscr },
    { WLAN_CFGID_DUMP_MEMORY,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_dump_memory },
    { WLAN_CFGID_ALG,                    OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_alg },
    { WLAN_CFGID_BEACON_CHAIN_SWITCH,    OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_beacon_chain_switch },
    { WLAN_CFGID_2040_CHASWI_PROHI,      OAL_FALSE,  {0},    OAL_PTR_NULL, wal_config_2040_channel_switch_prohibited },
    { WLAN_CFGID_2040_INTOLERANT,        OAL_FALSE,  {0},    OAL_PTR_NULL, wal_config_set_FortyMHzIntolerant },
    { WLAN_CFGID_2040_COEXISTENCE,       OAL_FALSE,  {0},    OAL_PTR_NULL, wal_config_set_2040_coext_support },
    { WLAN_CFGID_FRAG_THRESHOLD_REG,     OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_frag_threshold },
    { WLAN_CFGID_RX_FCS_INFO,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_rx_fcs_info },

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    { WLAN_CFGID_RESUME_RX_INTR_FIFO,    OAL_FALSE,  {0},    OAL_PTR_NULL, wal_config_resume_rx_intr_fifo },
#endif

#ifdef _PRE_WLAN_FEATURE_DFS
    { WLAN_CFGID_RADARTOOL,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_dfs_radartool },
#endif
#ifdef _PRE_SUPPORT_ACS
    { WLAN_CFGID_ACS_CONFIG,             OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_acs },
#endif

    { WLAN_CFGID_LIST_CHAN,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_list_channel },
    { WLAN_CFGID_REGDOMAIN_PWR,          OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_rd_pwr },
    { WLAN_CFGID_USER_INFO,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_user_info },
    { WLAN_CFGID_SET_DSCR,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_dscr_param },

    { WLAN_CFGID_SET_RATE,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_rate },
    { WLAN_CFGID_SET_MCS,                OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_mcs },
    { WLAN_CFGID_SET_MCSAC,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_mcsac },
    { WLAN_CFGID_SET_NSS,                OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_nss },
    { WLAN_CFGID_SET_RFCH,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_rfch },
    { WLAN_CFGID_SET_BW,                 OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_bw },
#if defined (_PRE_WLAN_CHIP_TEST) || defined (_PRE_WLAN_FEATURE_ALWAYS_TX)
    { WLAN_CFGID_SET_ALWAYS_TX,          OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_always_tx },
#endif
#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
    { WLAN_CFGID_SET_ALWAYS_TX_1102,     OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_always_tx_1102 },
#endif
    { WLAN_CFGID_SET_ALWAYS_RX,          OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_always_rx },
#ifdef _PRE_DEBUG_MODE
    { WLAN_CFGID_DYNC_TXPOWER,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_dync_txpower },
#endif
    { WLAN_CFGID_GET_THRUPUT,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_get_thruput },
    { WLAN_CFGID_SET_FREQ_SKEW,          OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_freq_skew },
    { WLAN_CFGID_ADJUST_PPM,             OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_adjust_ppm },

    { WLAN_CFGID_REG_INFO,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_reg_info },
    { WLAN_CFGID_DBB_SCALING_AMEND,      OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_dbb_scaling_amend },
#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST))
    { WLAN_CFGID_SDIO_FLOWCTRL,          OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_sdio_flowctrl },
#endif

#ifdef _PRE_WLAN_FEATURE_HILINK
    { WLAN_CFGID_MONITOR_EN,            OAL_FALSE,  {0},    OAL_PTR_NULL, wal_config_set_monitor_switch },
#endif
    { WLAN_MIPS_CYCLE_SWITCH,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_mips_cycle_switch },
    { WLAN_CFGID_REG_WRITE,               OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_reg_write },
    { WLAN_CFGID_TXBF_SWITCH,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_txbf_switch },

    { WLAN_CFGID_SCAN_ABORT,               OAL_FALSE,  {0},    OAL_PTR_NULL,      wal_config_scan_abort },
    /* 以下为内核cfg80211配置的命令 */
    { WLAN_CFGID_CFG80211_START_SCHED_SCAN, OAL_FALSE,  {0}, OAL_PTR_NULL, wal_config_cfg80211_start_sched_scan },
    { WLAN_CFGID_CFG80211_STOP_SCHED_SCAN, OAL_FALSE,  {0},    OAL_PTR_NULL, wal_config_cfg80211_stop_sched_scan },
    { WLAN_CFGID_CFG80211_START_SCAN,      OAL_FALSE,  {0},    OAL_PTR_NULL,      wal_config_cfg80211_start_scan },
    { WLAN_CFGID_CFG80211_START_CONNECT,   OAL_FALSE,  {0},    OAL_PTR_NULL,      wal_config_cfg80211_start_join },
    { WLAN_CFGID_CFG80211_SET_CHANNEL,     OAL_FALSE,  {0},    OAL_PTR_NULL,      wal_config_set_channel },
    { WLAN_CFGID_CFG80211_CONFIG_BEACON,   OAL_FALSE,  {0},    OAL_PTR_NULL,      wal_config_set_beacon },

    { WLAN_CFGID_ADD_KEY,           OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_add_key },
    { WLAN_CFGID_GET_KEY,           OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_get_key },
    { WLAN_CFGID_REMOVE_KEY,        OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_remove_key },

    { WLAN_CFGID_ALG_PARAM,         OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_alg_param },

#ifdef _PRE_WLAN_PERFORM_STAT
    { WLAN_CFGID_PFM_STAT,          OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_pfm_stat },
    { WLAN_CFGID_PFM_DISPLAY,       OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_pfm_display },
#endif

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
    { WLAN_CFGID_EDCA_OPT_SWITCH_STA,  OAL_FALSE,  {0},   OAL_PTR_NULL,       wal_config_set_edca_opt_switch_sta },
    { WLAN_CFGID_EDCA_OPT_WEIGHT_STA,  OAL_FALSE,  {0},   OAL_PTR_NULL,       wal_config_set_edca_opt_weight_sta },
    { WLAN_CFGID_EDCA_OPT_SWITCH_AP,   OAL_FALSE,  {0},   OAL_PTR_NULL,       wal_config_set_edca_opt_switch_ap },
    { WLAN_CFGID_EDCA_OPT_CYCLE_AP,    OAL_FALSE,  {0},   OAL_PTR_NULL,       wal_config_set_edca_opt_cycle_ap },
#endif

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
    { WLAN_CFGID_GET_HIPKT_STAT,        OAL_FALSE,  {0},   OAL_PTR_NULL,           wal_config_get_hipkt_stat },
    { WLAN_CFGID_SET_FLOWCTL_PARAM,     OAL_FALSE,  {0},   OAL_PTR_NULL,           wal_config_set_flowctl_param },
    { WLAN_CFGID_GET_FLOWCTL_STAT,      OAL_FALSE,  {0},   OAL_PTR_NULL,           wal_config_get_flowctl_stat },
#endif

    /* START:开源APP 程序下发的私有命令 */
    { WLAN_CFGID_SET_WPS_IE,        OAL_FALSE,  {0},   OAL_PTR_NULL,             wal_config_set_wps_ie },
    { WLAN_CFGID_SET_RTS_THRESHHOLD, OAL_FALSE,  {0},   OAL_PTR_NULL,             wal_config_rts_threshold },
    /* END:开源APP 程序下发的私有命令 */
#ifdef _PRE_WLAN_CHIP_TEST
    { WLAN_CFGID_LPM_TX_DATA,        OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_lpm_tx_data },
    { WLAN_CFGID_LPM_TX_PROBE_REQUEST,        OAL_FALSE,  {0},   OAL_PTR_NULL,   wal_config_lpm_tx_probe_request },
    { WLAN_CFGID_LPM_CHIP_STATE,     OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_lpm_chip_state },
    { WLAN_CFGID_LPM_SOC_MODE,      OAL_FALSE,  {0},   OAL_PTR_NULL,             wal_config_set_lpm_soc_mode },
    { WLAN_CFGID_LPM_PSM_PARAM,      OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_lpm_psm_param },
    { WLAN_CFGID_LPM_SMPS_MODE,      OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_lpm_smps_mode },
    { WLAN_CFGID_LPM_SMPS_STUB,      OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_lpm_smps_stub },
    { WLAN_CFGID_LPM_TXOP_PS_SET,    OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_lpm_txop_ps },
    { WLAN_CFGID_LPM_TXOP_TX_STUB,   OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_lpm_txop_tx_stub },
    { WLAN_CFGID_LPM_WOW_EN,         OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_lpm_wow_en },
    { WLAN_CFGID_REMOVE_LUT,         OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_remove_user_lut },
    { WLAN_CFGID_SEND_FRAME,         OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_send_frame },
    { WLAN_CFGID_SET_RX_PN_REG,      OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_rx_pn },
    { WLAN_CFGID_SET_SOFT_RETRY,     OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_soft_retry },
    { WLAN_CFGID_OPEN_ADDR4,         OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_open_addr4 },
    { WLAN_CFGID_OPEN_WMM_TEST,      OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_open_wmm_test },

    { WLAN_CFGID_CHIP_TEST_OPEN,     OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_chip_test_open },
    { WLAN_CFGID_SET_COEX,           OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_coex },
    { WLAN_CFGID_DFX_SWITCH,         OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_dfx },
#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
    { WLAN_CFGID_PMF_ENABLE,         OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_enable_pmf },
#endif
    { WLAN_CFGID_SEND_ACTION,        OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_send_action },
    { WLAN_CFGID_CLEAR_ALL_STAT,         OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_clear_all_stat },
    { WLAN_CFGID_SEND_PSPOLL,        OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_send_pspoll },
    { WLAN_CFGID_SEND_NULLDATA,      OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_send_nulldata },
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */
    { WLAN_CFGID_DEFAULT_KEY,        OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_default_key },

    { WLAN_CFGID_WMM_SWITCH,         OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_open_wmm },
    { WLAN_CFGID_HIDE_SSID,          OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_hide_ssid },

    { WLAN_CFGID_SET_MIB,            OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_mib },
    { WLAN_CFGID_GET_MIB,            OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_get_mib },
    { WLAN_CFGID_SET_THRUPUT_BYPASS, OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_thruput_bypass },
    { WLAN_CFGID_SET_AUTO_PROTECTION, OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_auto_protection },

    { WLAN_CFGID_SEND_2040_COEXT,    OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_send_2040_coext },
    { WLAN_CFGID_2040_COEXT_INFO,    OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_2040_coext_info },
    { WLAN_CFGID_GET_VERSION,        OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_get_version },
#ifdef _PRE_DEBUG_MODE
    { WLAN_CFGID_GET_ALL_REG_VALUE,  OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_get_all_reg_value },
#endif
#ifdef _PRE_WLAN_FEATURE_DAQ
    { WLAN_CFGID_DATA_ACQ,           OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_data_acq },
#endif

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    { WLAN_CFGID_SET_OMA,            OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_oma },
    { WLAN_CFGID_PROXYSTA_SWITCH,    OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_proxysta_switch },
#endif

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
    { WLAN_CFGID_SET_OPMODE_NOTIFY,  OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_opmode_notify },
    { WLAN_CFGID_GET_USER_RSSBW,     OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_get_user_rssbw },
#endif

    { WLAN_CFGID_SET_VAP_NSS,        OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_vap_nss },

#ifdef _PRE_WLAN_DFT_REG
    { WLAN_CFGID_DUMP_REG,       OAL_FALSE,  {0},   OAL_PTR_NULL,                wal_config_dump_reg },
#endif

#ifdef _PRE_DEBUG_MODE
    { WLAN_CFGID_RX_FILTER_VAL,      OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_rx_filter_val },
    { WLAN_CFGID_SET_RX_FILTER_EN,   OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_rx_filter_en },
    { WLAN_CFGID_GET_RX_FILTER_EN,   OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_get_rx_filter_en },
    { WLAN_CFGID_REPORT_AMPDU_STAT,  OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_report_ampdu_stat },
#endif

    { WLAN_CFGID_SET_AGGR_NUM,       OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_ampdu_aggr_num },

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    { WLAN_CFGID_FREQ_ADJUST,        OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_freq_adjust },
#endif

    { WLAN_CFGID_SET_STBC_CAP,       OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_stbc_cap },
    { WLAN_CFGID_SET_LDPC_CAP,       OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_ldpc_cap },

#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY

    /* 黑名单配置 */
    { WLAN_CFGID_ADD_BLACK_LIST, OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_blacklist_add },
    { WLAN_CFGID_DEL_BLACK_LIST, OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_blacklist_del },
    { WLAN_CFGID_BLACKLIST_MODE, OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_blacklist_mode },
    { WLAN_CFGID_BLACKLIST_SHOW, OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_blacklist_show },

    /* 自动黑名单参数配置 */
    { WLAN_CFGID_AUTOBLACKLIST_ON, OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_autoblacklist_enable },
    { WLAN_CFGID_AUTOBLACKLIST_AGING, OAL_FALSE, {0}, OAL_PTR_NULL, wal_config_set_autoblacklist_aging },
    { WLAN_CFGID_AUTOBLACKLIST_THRESHOLD, OAL_FALSE, {0}, OAL_PTR_NULL, wal_config_set_autoblacklist_threshold },
    { WLAN_CFGID_AUTOBLACKLIST_RESET, OAL_FALSE, {0}, OAL_PTR_NULL, wal_config_set_autoblacklist_reset_time },

    /* 用户隔离参数配置 */
    { WLAN_CFGID_ISOLATION_MODE, OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_isolation_mode },
    { WLAN_CFGID_ISOLATION_TYPE, OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_isolation_type },
    { WLAN_CFGID_ISOLATION_FORWARD, OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_isolation_forward },
    { WLAN_CFGID_ISOLATION_CLEAR, OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_isolation_clear },
    { WLAN_CFGID_ISOLATION_SHOW, OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_isolation_show },

#endif
#ifdef _PRE_WLAN_FEATURE_MCAST
    { WLAN_CFGID_M2U_SNOOP_ON,      OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_m2u_snoop_on },
    { WLAN_ADD_M2U_DENY_TABLE,      OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_add_m2u_deny_table },
    { WLAN_CFGID_M2U_DENY_TABLE,    OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_m2u_deny_table },
    { WLAN_SHOW_M2U_SNOOP_TABLE,    OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_show_m2u_snoop_table },
    { WLAN_CFGID_IGMP_PACKET_XMIT,  OAL_FALSE,  {0},   OAL_PTR_NULL,          wal_config_igmp_packet_xmit },
#endif

#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
    { WLAN_CFGID_ADD_BLACK_LIST_ONLY, OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_blacklist_add_only },
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
    { WLAN_CFGID_PROXYARP_EN, OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_proxyarp_en },
#ifdef _PRE_DEBUG_MODE
    { WLAN_CFGID_PROXYARP_INFO, OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_proxyarp_info },
#endif /* #ifdef _PRE_DEBUG_MODE */
#endif

#ifdef _PRE_WLAN_FEATURE_WAPI
#ifdef _PRE_WAPI_DEBUG
    { WLAN_CFGID_WAPI_INFO, OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_wapi_info },
#endif /* #ifdef _PRE_DEBUG_MODE */
#endif

#ifdef _PRE_WLAN_FEATURE_SMPS
#ifdef _PRE_DEBUG_MODE
    { WLAN_CFGID_GET_SMPS_INFO,           OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_get_smps_info },
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_PM
    { WLAN_CFGID_WIFI_EN,            OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_wifi_enable },
    { WLAN_CFGID_PM_INFO,            OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_pm_info },
    { WLAN_CFGID_PM_EN,              OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_pm_enable },
#endif

    { WLAN_CFGID_SET_WPS_P2P_IE,                    OAL_FALSE,  {0},    OAL_PTR_NULL,     wal_config_set_wps_p2p_ie },
    { WLAN_CFGID_CFG80211_REMAIN_ON_CHANNEL,        OAL_FALSE,  {0},    OAL_PTR_NULL, wal_config_remain_on_channel },
    { WLAN_CFGID_CFG80211_CANCEL_REMAIN_ON_CHANNEL, OAL_FALSE, {0}, OAL_PTR_NULL, wal_config_cancel_remain_on_channel },
    { WLAN_CFGID_CFG80211_MGMT_TX,   OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_mgmt_tx },

    { WLAN_CFGID_VAP_CLASSIFY_EN,    OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_vap_classify_en },
    { WLAN_CFGID_VAP_CLASSIFY_TID,   OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_vap_classify_tid },
#ifdef _PRE_DEBUG_MODE
    { WLAN_CFGID_SCAN_TEST,          OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_scan_test },
#endif
    { WLAN_CFIGD_BGSCAN_ENABLE,      OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_bgscan_enable },
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
    { WLAN_CFGID_QUERY_STATION_STATS,   OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_query_station_stats },
#endif
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    { WLAN_CFGID_DEVICE_MEM_LEAK,       OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_show_device_memleak },
    { WLAN_CFGID_DEVICE_MEM_INFO,       OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_show_device_meminfo },
#endif
#ifdef _PRE_WLAN_FEATURE_STA_PM
    { WLAN_CFGID_SET_PS_MODE,           OAL_FALSE,  {0},   OAL_PTR_NULL,         wal_config_set_sta_pm_mode },
#ifdef _PRE_PSM_DEBUG_MODE
    { WLAN_CFGID_SHOW_PS_INFO,          OAL_FALSE,  {0},   OAL_PTR_NULL,         wal_config_show_pm_info },
#endif
    { WLAN_CFGID_SET_PSM_PARAM,         OAL_FALSE,  {0},   OAL_PTR_NULL,          wal_config_set_pm_param },
    { WLAN_CFGID_SET_STA_PM_ON,          OAL_FALSE,  {0},   OAL_PTR_NULL,         wal_config_set_sta_pm_on },
#endif

#ifdef _PRE_WLAN_FEATURE_STA_UAPSD
    { WLAN_CFGID_SET_UAPSD_PARA,        OAL_FALSE, {0},      OAL_PTR_NULL,       wal_config_set_uapsd_para },
#endif
#ifdef _PRE_WLAN_FEATURE_11D
    { WLAN_CFGID_SET_RD_IE_SWITCH,      OAL_FALSE, {0},      OAL_PTR_NULL,       wal_config_set_rd_by_ie_switch },
#endif
#ifdef _PRE_WLAN_FEATURE_P2P
    { WLAN_CFGID_SET_P2P_PS_OPS,     OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_p2p_ps_ops },
    { WLAN_CFGID_SET_P2P_PS_NOA,     OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_p2p_ps_noa },
    { WLAN_CFGID_SET_P2P_PS_STAT,    OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_p2p_ps_stat },
#endif
#ifdef _PRE_WLAN_FEATURE_HS20
    { WLAN_CFGID_SET_QOS_MAP,        OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_qos_map },
#endif
#ifdef _PRE_WLAN_PROFLING_MIPS
    { WLAN_CFGID_SET_MIPS,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_mips },
    { WLAN_CFGID_SHOW_MIPS,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_show_mips },
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    { WLAN_CFGID_AMPDU_MMSS,          OAL_FALSE,  {0},    OAL_PTR_NULL,             wal_config_set_ampdu_mmss },
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
    { WLAN_CFGID_ENABLE_ARP_OFFLOAD,             OAL_FALSE,  {0},    OAL_PTR_NULL, wal_config_enable_arp_offload },
    { WLAN_CFGID_SHOW_ARPOFFLOAD_INFO,           OAL_FALSE,  {0},    OAL_PTR_NULL, wal_config_show_arpoffload_info },
#endif

#ifdef _PRE_WLAN_TCP_OPT
    { WLAN_CFGID_GET_TCP_ACK_STREAM_INFO, OAL_FALSE, {0}, OAL_PTR_NULL, wal_config_get_tcp_ack_stream_info },
    { WLAN_CFGID_TX_TCP_ACK_OPT_ENALBE,         OAL_FALSE,  {0},  OAL_PTR_NULL, wal_config_tx_tcp_ack_opt_enable },
    { WLAN_CFGID_RX_TCP_ACK_OPT_ENALBE,         OAL_FALSE,  {0},  OAL_PTR_NULL, wal_config_rx_tcp_ack_opt_enable },
    { WLAN_CFGID_TX_TCP_ACK_OPT_LIMIT,         OAL_FALSE,  {0},  OAL_PTR_NULL,          wal_config_tx_tcp_ack_limit },
    { WLAN_CFGID_RX_TCP_ACK_OPT_LIMIT,         OAL_FALSE,  {0},  OAL_PTR_NULL,          wal_config_rx_tcp_ack_limit },
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    { WLAN_CFGID_CFG_VAP_H2D,  OAL_FALSE,    {0},    OAL_PTR_NULL,            wal_config_cfg_vap_h2d },
    { WLAN_CFGID_HOST_DEV_INIT,  OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_host_dev_init },
    { WLAN_CFGID_HOST_DEV_EXIT,  OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_host_dev_exit },
#endif
    { WLAN_CFGID_SET_MAX_USER,        OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_max_user },
    { WLAN_CFGID_GET_STA_LIST,        OAL_FALSE,  {0},    wal_config_get_sta_list, OAL_PTR_NULL },
#if ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 35))) || \
    (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#ifdef _PRE_WLAN_DFT_STAT
    { WLAN_CFGID_SET_PERFORMANCE_LOG_SWITCH, OAL_FALSE,  {0},  OAL_PTR_NULL, wal_config_set_performance_log_switch },
#endif
#endif
#ifdef _PRE_WLAN_FEATURE_ROAM
    { WLAN_CFGID_ROAM_ENABLE,  OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_roam_enable },
    { WLAN_CFGID_ROAM_ORG,     OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_roam_org },
    { WLAN_CFGID_ROAM_BAND,    OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_roam_band },
    { WLAN_CFGID_ROAM_START,   OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_roam_start },
    { WLAN_CFGID_ROAM_INFO,    OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_roam_info },
#endif  // _PRE_WLAN_FEATURE_ROAM
#ifdef _PRE_WLAN_FEATURE_SAE
    {WLAN_CFGID_CFG80211_EXTERNAL_AUTH, OAL_FALSE, {0}, OAL_PTR_NULL,      wal_config_external_auth},
#endif /* _PRE_WLAN_FEATURE_SAE */
#ifdef _PRE_WLAN_FEATURE_11R
    { WLAN_CFGID_SET_FT_IES,  OAL_FALSE,  {0},    OAL_PTR_NULL,             wal_config_set_ft_ies },
#endif // _PRE_WLAN_FEATURE_11R
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    { WLAN_CFGID_2040BSS_ENABLE,  OAL_FALSE,  {0},    OAL_PTR_NULL,         wal_config_enable_2040bss },
#endif

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
    { WLAN_CFGID_SET_AUTO_FREQ_ENABLE, OAL_FALSE,  {0},  OAL_PTR_NULL, wal_config_set_auto_freq_enable },
#endif

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    { WLAN_CFGID_LAUCH_CAP,                 OAL_FALSE,  {0},  wal_config_get_lauch_cap, OAL_PTR_NULL },
    { WLAN_CFGID_SET_LINKLOSS_THRESHOLD,    OAL_FALSE,  {0},  OAL_PTR_NULL, wal_config_set_linkloss_threshold },
    { WLAN_CFGID_SET_ALL_LOG_LEVEL,         OAL_FALSE,  {0},  OAL_PTR_NULL,        wal_config_set_all_log_level },
    { WLAN_CFGID_SET_D2H_HCC_ASSEMBLE_CNT,  OAL_FALSE,  {0},  OAL_PTR_NULL, wal_config_set_d2h_hcc_assemble_cnt },
    { WLAN_CFGID_SET_CHN_EST_CTRL,          OAL_FALSE,  {0},  OAL_PTR_NULL,        wal_config_set_chn_est_ctrl },
    { WLAN_CFGID_SET_POWER_REF,             OAL_FALSE,  {0},  OAL_PTR_NULL,        wal_config_set_power_ref },
    { WLAN_CFGID_SET_PM_CFG_PARAM,          OAL_FALSE,  {0},  OAL_PTR_NULL,        wal_config_set_pm_cfg_param },
    { WLAN_CFGID_SET_CUS_RF,                OAL_FALSE,  {0},  OAL_PTR_NULL,        wal_config_set_cus_rf },
    { WLAN_CFGID_SET_CUS_DTS_CALI,          OAL_FALSE,  {0},  OAL_PTR_NULL,        wal_config_set_cus_dts_cali },
    { WLAN_CFGID_SET_CUS_NVRAM_PARAM,       OAL_FALSE,  {0},  OAL_PTR_NULL,        wal_config_set_cus_nvram_params },
    /* SHOW DEIVCE CUSTOMIZE INFOS */
    { WLAN_CFGID_SHOW_DEV_CUSTOMIZE_INFOS,  OAL_FALSE,  {0},  OAL_PTR_NULL,        wal_config_dev_customize_info },
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

    { WLAN_CFGID_DESTROY_VAP,     OAL_FALSE,  {0},    OAL_PTR_NULL,         wal_config_vap_destroy },

#ifdef _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN
    { WLAN_CFGID_TX_CLASSIFY_LAN_TO_WLAN_SWITCH, OAL_FALSE, {0}, OAL_PTR_NULL, wal_config_set_tx_classify_switch },
#endif
    { WLAN_CFGID_REDUCE_SAR,                OAL_FALSE,  {0},    OAL_PTR_NULL,       wal_config_reduce_sar },
#ifdef _PRE_WLAN_CFGID_DEBUG
    { WLAN_CFIGD_MCS_SET_CHECK_ENABLE,      OAL_FALSE,  {0},    OAL_PTR_NULL,       wal_config_mcs_set_check_enable },
#endif
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifndef CONFIG_HAS_EARLYSUSPEND
    { WLAN_CFGID_SET_SUSPEND_MODE,         OAL_FALSE, {0}, OAL_PTR_NULL,          wal_config_set_suspend_mode  },
#endif
#endif
#ifdef _PRE_WLAN_FEATURE_IP_FILTER
    { WLAN_CFGID_IP_FILTER,                 OAL_FALSE,  {0},    OAL_PTR_NULL,       wal_config_update_ip_filter },
    { WLAN_CFGID_ASSIGNED_FILTER,           OAL_FALSE,  {0},    NULL,               wal_config_assigned_filter },
#endif

#ifdef _PRE_WLAN_FEATURE_APF
    { WLAN_CFGID_SET_APF_FILTER,            OAL_FALSE,  {0},  OAL_PTR_NULL,          wal_config_apf_filter_cmd },
#endif
    { WLAN_CFGID_HIGH_POWER_SWITCH,         OAL_FALSE,  {0},  OAL_PTR_NULL,          wal_config_set_high_power_switch },
    { WLAN_CFGID_SET_AUTH_RSP_TIME,         OAL_FALSE,  {0},  OAL_PTR_NULL,          wal_config_set_auth_rsp_time },
    { WLAN_CFGID_FORBIT_OPEN_AUTH,          OAL_FALSE,  {0},  OAL_PTR_NULL,          wal_config_forbit_open_auth },
#ifdef _PRE_WLAN_FEATURE_1131K_BTCOEX
    { WLAN_CFGID_BTCOEX_CHANNEL_SWITCH,     OAL_FALSE,  {0},  OAL_PTR_NULL,      wal_config_set_btcoex_channel_switch },
#endif
    { WLAN_CFGID_RX_FILTER_FRAG,            OAL_FALSE,  {0},  OAL_PTR_NULL,      wal_rx_filter_frag },
    { WLAN_CFGID_PSM_GET_TXRX_INFO,         OAL_FALSE,  {0},  OAL_PTR_NULL,      wal_get_psm_info },
    { WLAN_CFGID_PSM_GET_BEACON_INFO,       OAL_FALSE,  {0},  OAL_PTR_NULL,      wal_get_psm_bcn_info },
    { WLAN_CFGID_BUTT,                      OAL_FALSE,  {0},    0,                       0 },
};

// 3 函数实现

OAL_STATIC oal_uint32 wal_config_add_vap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32 ul_ret;

    if (OAL_UNLIKELY((pst_mac_vap == OAL_PTR_NULL) || (puc_param == OAL_PTR_NULL))) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_add_vap(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_config_del_vap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32         ul_ret;
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    mac_device_stru   *pst_mac_device = OAL_PTR_NULL;
#endif

    if (OAL_UNLIKELY((pst_mac_vap == OAL_PTR_NULL) || (puc_param == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_del_vap::pst_mac_vap or puc_param null ptr error}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA, "{wal_config_del_vap::mac_res_get_dev is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_mac_device->st_cap_flag.bit_proxysta == OAL_TRUE) {
        /* 如果是删除的sta是Proxy STA，则需要从hash表移除，main Proxy STA不需要移除 */
        if ((pst_mac_vap->st_vap_proxysta.en_is_proxysta == OAL_TRUE) &&
            (pst_mac_vap->st_vap_proxysta.en_is_main_proxysta == OAL_FALSE)) {
            OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                "{wal_config_del_vap::Proxy STA is removed from hash table}");
            wal_proxysta_remove_vap(pst_mac_vap);
        }
    }

#endif
    ul_ret = hmac_config_del_vap(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_del_vap:: return error code [%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_config_start_vap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;

    if (OAL_UNLIKELY((pst_mac_vap == OAL_PTR_NULL) || (puc_param == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_start_vap::pst_mac_vap or puc_param null ptr error}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_start_vap(pst_mac_vap, us_len, puc_param);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_start_vap:: return error code %d.}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_config_down_vap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32    ul_ret;

    if (OAL_UNLIKELY((pst_mac_vap == OAL_PTR_NULL) || (puc_param == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_down_vap::pst_mac_vap or puc_param null ptr error}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_down_vap(pst_mac_vap, us_len, puc_param);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_down_vap::return error code [%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL

OAL_STATIC uint32_t wal_config_get_hipkt_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_get_hipkt_stat(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_flowctl_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_set_flowctl_param(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_get_flowctl_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_get_flowctl_stat(pst_mac_vap, us_len, puc_param);
}
#endif


OAL_STATIC oal_uint32 wal_config_set_bss_type(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_bss_type(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_bss_type(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_bss_type(pst_mac_vap, pus_len,  puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_mode(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_mode(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_bandwidth(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_mode_param_stru    *pst_prot_param = OAL_PTR_NULL;

    pst_prot_param = (mac_cfg_mode_param_stru *)puc_param;

    pst_prot_param->en_protocol  = pst_mac_vap->en_protocol;
    pst_prot_param->en_band      = pst_mac_vap->st_channel.en_band;

    return hmac_config_set_mode(pst_mac_vap, us_len, (oal_uint8 *)pst_prot_param);
}


OAL_STATIC uint32_t wal_config_get_fem_pa_status(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_get_fem_pa_status(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_mac_addr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                     ul_ret;

    if (OAL_UNLIKELY((pst_mac_vap == OAL_PTR_NULL) || (puc_param == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_set_mac_addr::pst_mac_vap or puc_param null ptr error}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_set_mac_addr(pst_mac_vap, us_len, puc_param);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_config_set_mac_addr:: return error code %d.}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC uint32_t wal_config_set_concurrent(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    uint32_t                      ul_ret;

    if (OAL_UNLIKELY((pst_mac_vap == OAL_PTR_NULL) || (puc_param == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_set_concurrent::pst_mac_vap or puc_param null ptr error}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_set_concurrent(pst_mac_vap, us_len, puc_param);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_config_set_concurrent:: return error code %d.}\r\n", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_config_get_concurrent(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_concurrent(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_ssid(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_ssid(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_ssid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_ssid(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_shpreamble(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_set_shpreamble(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_shpreamble(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_shpreamble(pst_mac_vap, pus_len, puc_param);
}
#ifdef _PRE_WLAN_FEATURE_MONITOR

OAL_STATIC uint32_t wal_config_set_addr_filter(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_set_addr_filter(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_addr_filter(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_addr_filter(pst_mac_vap, pus_len, puc_param);
}
#endif


OAL_STATIC oal_uint32 wal_config_set_shortgi20(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_shortgi20(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_shortgi40(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_shortgi40(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_shortgi80(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_shortgi80(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_shortgi20(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_shortgi20(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_shortgi40(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_shortgi40(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_shortgi80(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_shortgi80(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_prot_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_prot_mode(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_prot_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_prot_mode(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_auth_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_auth_mode(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_auth_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_auth_mode(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_bintval(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_bintval(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_bintval(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_bintval(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_dtimperiod(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_set_dtimperiod(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_dtimperiod(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_dtimperiod(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_nobeacon(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_nobeacon(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_nobeacon(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_nobeacon(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_txchain(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_txchain(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_txchain(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_txchain(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_rxchain(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_rxchain(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_rxchain(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_rxchain(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_txpower(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_txpower(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_txpower(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_txpower(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_freq(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_freq(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_freq(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_freq(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_wmm_params(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_set_wmm_params(pst_mac_vap, us_len, puc_param);
}


oal_uint32 wal_config_get_wmm_params(oal_net_device_stru *pst_net_dev, oal_uint8 *puc_param)
{
    mac_vap_stru               *pst_vap;

    pst_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(pst_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_get_wmm_params::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr.}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    return hmac_config_get_wmm_params(pst_vap, puc_param);
}

#ifdef _PRE_WLAN_FEATURE_SMPS

OAL_STATIC oal_uint32 wal_config_set_smps_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_smps_mode(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_smps_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_smps_mode(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_smps_en(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_smps_en(pst_mac_vap, pus_len, puc_param);
}

#endif

#ifdef _PRE_WLAN_FEATURE_UAPSD

OAL_STATIC oal_uint32 wal_config_set_uapsd_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_uapsden(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_uapsd_en(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_uapsden(pst_mac_vap, pus_len, puc_param);
}
#endif


oal_uint32 wal_config_set_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_channel(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_beacon(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                          ul_ret;

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL) || OAL_UNLIKELY(puc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_set_beacon::pst_mac_vap or puc_param is null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_set_beacon(pst_mac_vap, us_len, puc_param);

    return ul_ret;
}


OAL_STATIC oal_uint32 wal_config_vap_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_vap_info(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_event_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_event_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_eth_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_eth_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_80211_ucast_switch(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_80211_ucast_switch(pst_mac_vap, us_len, puc_param);
}


#ifdef _PRE_DEBUG_MODE_USER_TRACK

OAL_STATIC oal_uint32 wal_config_report_thrput_stat(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_report_thrput_stat(pst_mac_vap, us_len, puc_param);
}

#endif


#ifdef _PRE_WLAN_FEATURE_TXOPPS


OAL_STATIC uint32_t wal_config_set_txop_ps_machw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_set_txop_ps_machw(pst_mac_vap, us_len, puc_param);
}

#endif

#ifdef _PRE_WLAN_FEATURE_BTCOEX

OAL_STATIC oal_uint32 wal_config_print_btcoex_status(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_print_btcoex_status(pst_mac_vap, us_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_FEATURE_LTECOEX

OAL_STATIC uint32_t wal_config_ltecoex_mode_set(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_ltecoex_mode_set(pst_mac_vap, us_len, puc_param);
}
#endif


OAL_STATIC uint32_t wal_config_80211_mcast_switch(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_80211_mcast_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_probe_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_probe_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_rssi_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_rssi_switch(pst_mac_vap, us_len, puc_param);
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

OAL_STATIC uint32_t wal_config_report_vap_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_report_vap_info(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_wfa_cfg_aifsn(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_wfa_cfg_aifsn(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_wfa_cfg_cw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_wfa_cfg_cw(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_lte_gpio_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_lte_gpio_mode(pst_mac_vap, us_len, puc_param);
}
#endif


OAL_STATIC oal_uint32 wal_config_get_mpdu_num(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_get_mpdu_num(pst_mac_vap, us_len, puc_param);
}

#ifdef _PRE_WLAN_RF_110X_CALI_DPD
OAL_STATIC oal_uint32 wal_config_start_dpd(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_start_dpd(pst_mac_vap, us_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_CHIP_TEST

OAL_STATIC oal_uint32 wal_config_beacon_offload_test(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_beacon_offload_test(pst_mac_vap, us_len, puc_param);
}
#endif


OAL_STATIC uint32_t wal_config_ota_beacon_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_ota_beacon_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_ota_rx_dscr_switch(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_ota_rx_dscr_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_all_ota(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_all_ota(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_dhcp_arp_switch(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_dhcp_arp_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_random_mac_addr_scan(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_random_mac_addr_scan(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_oam_output(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_oam_output(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_add_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_add_user(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_del_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_del_user(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_ampdu_start(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_ampdu_start(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_amsdu_start(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_amsdu_start(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_ampdu_end(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_ampdu_end(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_auto_ba_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_auto_ba_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_amsdu_ampdu_switch(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_amsdu_ampdu_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_profiling_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_profiling_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_addba_req(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_addba_req(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_delba_req(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_delba_req(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_list_ap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_list_ap(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_list_sta(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_list_sta(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_sta_list(mac_vap_stru *pst_mac_vap, oal_uint16 *us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_sta_list(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_list_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_list_channel(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_rd_pwr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_regdomain_pwr(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_reduce_sar(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_reduce_sar(pst_mac_vap, us_len, puc_param);
}


oal_uint32 wal_config_dump_all_rx_dscr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_dump_all_rx_dscr(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_start_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_sta_initiate_scan(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_cfg80211_start_sched_scan(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_cfg80211_start_sched_scan(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_cfg80211_stop_sched_scan(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_cfg80211_stop_sched_scan(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_scan_abort(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_scan_abort(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_cfg80211_start_scan(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_cfg80211_start_scan_sta(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_start_join(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_bss_dscr_stru   *pst_bss_dscr = OAL_PTR_NULL;
    oal_uint32           ul_bss_idx;

    ul_bss_idx = (oal_uint8)oal_atoi((oal_int8 *)puc_param);

    pst_bss_dscr = hmac_scan_find_scanned_bss_dscr_by_index(pst_mac_vap->uc_device_id, ul_bss_idx);
    if (pst_bss_dscr == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_start_join::find bss failed by index!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    return hmac_sta_initiate_join(pst_mac_vap, pst_bss_dscr);
}


OAL_STATIC oal_uint32 wal_config_cfg80211_start_join(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_connect(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_start_deauth(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    hmac_config_send_deauth(pst_mac_vap, pst_mac_vap->auc_bssid);
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_config_dump_timer(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_dump_timer(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_pause_tid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_pause_tid(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_user_vip(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_user_vip(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_vap_host(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_vap_host(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_send_bar(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_send_bar(pst_mac_vap, us_len, puc_param);
}


oal_netbuf_stru* wal_config_create_al_tx_packet(oal_uint32 ul_size,
                                                oal_uint8 uc_tid,
                                                mac_rf_payload_enum_uint8 en_payload_flag,
                                                oal_bool_enum_uint8 en_init_flag)
{
    static oal_netbuf_stru         *pst_buf = OAL_PTR_NULL;
    oal_uint32               ul_loop;
    oal_uint32               l_reserve = 256;
    oal_uint32               l_ret = EOK;

    if (en_init_flag == OAL_TRUE) {
        pst_buf = oal_netbuf_alloc(ul_size + l_reserve, (oal_int32)l_reserve, WLAN_MEM_NETBUF_ALIGN);
        if (OAL_UNLIKELY(pst_buf == OAL_PTR_NULL)) {
            OAM_ERROR_LOG0(0, OAM_SF_TX, "wal_config_create_al_tx_packet::alloc Fail");
            return OAL_PTR_NULL;
        }
        oal_netbuf_put(pst_buf, ul_size);
    }

    if (pst_buf == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_TX, "wal_config_create_al_tx_packet::pst_buf is not initiate");
        return OAL_PTR_NULL;
    }

    switch (en_payload_flag) {
        case RF_PAYLOAD_ALL_ZERO:
            l_ret += memset_s(pst_buf->data, ul_size, 0, ul_size);
            break;
        case RF_PAYLOAD_ALL_ONE:
            l_ret += memset_s(pst_buf->data, ul_size, 0xFF, ul_size);
            break;
        case RF_PAYLOAD_RAND:
            pst_buf->data[0] = oal_gen_random(18, 1);           // 18：非安全类随机种子
            for (ul_loop = 1; ul_loop < ul_size; ul_loop++) {
                pst_buf->data[ul_loop] = oal_gen_random(18, 0);
            }
            break;
        default:
            break;
    }

    pst_buf->next = OAL_PTR_NULL;
    pst_buf->prev = OAL_PTR_NULL;

    l_ret += memset_s(oal_netbuf_cb(pst_buf), OAL_NETBUF_CB_SIZE(), 0, OAL_NETBUF_CB_SIZE());
    if (l_ret != EOK) {
        OAM_ERROR_LOG1(0, OAM_SF_ACS, "wal_config_create_al_tx_packet::memset fail![%d]", l_ret);
        oal_netbuf_free(pst_buf);
        return OAL_PTR_NULL;
    }
    return pst_buf;
}


oal_netbuf_stru* wal_config_create_igmp_packet(oal_uint32 ul_size,
                                               oal_uint8 uc_tid,
                                               const oal_uint8 *puc_mac_ra,
                                               const oal_uint8 *puc_mac_ta)
{
    oal_netbuf_stru         *pst_buf = OAL_PTR_NULL;
    mac_ether_header_stru   *pst_ether_header = OAL_PTR_NULL;
    mac_ip_header_stru      *pst_ip = OAL_PTR_NULL;
    oal_uint32               ul_loop;
    oal_uint32               l_reserve = 256;
    mac_igmp_header_stru     *pst_igmp_hdr = OAL_PTR_NULL;                         /* igmp header for v1 v2 */
    /* IGMP报文偏移量50，若传入长度小于50，可能造成data的越界 */
    if (ul_size < 50) {
        OAM_ERROR_LOG1(0, 0, "wal_config_create_igmp_packet::ul_size[%d] < 50", ul_size);
        return OAL_PTR_NULL;
    }

    pst_buf = oal_netbuf_alloc(ul_size + l_reserve, (oal_int32)l_reserve, WLAN_MEM_NETBUF_ALIGN);
    if (OAL_UNLIKELY(pst_buf == OAL_PTR_NULL)) {
        return OAL_PTR_NULL;
    }

    oal_netbuf_put(pst_buf, ul_size);
    oal_set_mac_addr(&pst_buf->data[0], puc_mac_ra);
    oal_set_mac_addr(&pst_buf->data[6], puc_mac_ta);

    /* 帧体内容 最后6个字节保持为0x00 */
    for (ul_loop = 0; ul_loop < ul_size - 50; ul_loop++) {
        pst_buf->data[14 + ul_loop] = (oal_uint8)ul_loop; //lint !e679
    }

    pst_ether_header = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);

    /*lint -e778*/
    pst_ether_header->us_ether_type = OAL_HOST2NET_SHORT(ETHER_TYPE_IP);
    /*lint +e778*/
    pst_ip = (mac_ip_header_stru *)(pst_ether_header + 1);      /* 偏移一个以太网头，取ip头 */
    pst_ip->uc_version_ihl = 0x45;
    pst_ip->uc_protocol = IPPROTO_IGMP;

    pst_ip->uc_tos = (oal_uint8)(uc_tid << WLAN_IP_PRI_SHIFT);
    /* 指向igmp头指针 */
    pst_igmp_hdr = (mac_igmp_header_stru *)(pst_ip + 1);
    pst_igmp_hdr->uc_type = MAC_IGMPV2_REPORT_TYPE;
    pst_igmp_hdr->ul_group = oal_byteorder_host_to_net_uint32(0xe0804020);

    pst_buf->next = OAL_PTR_NULL;
    pst_buf->prev = OAL_PTR_NULL;

    memset_s(oal_netbuf_cb(pst_buf), OAL_NETBUF_CB_SIZE(), 0, OAL_NETBUF_CB_SIZE());

    return pst_buf;
}

OAL_STATIC oal_uint32 wal_config_alg(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_alg(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_dump_ba_bitmap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_dump_ba_bitmap(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_timer_start(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return OAL_SUCC;
}


OAL_STATIC uint32_t wal_config_show_profiling(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
#ifdef _PRE_PROFILING_MODE

    oal_uint32          ul_value;

    ul_value = *((oal_uint32 *)puc_param);

    if (ul_value == OAM_PROFILING_RX) {
        oam_profiling_rx_show_offset();
    } else if (ul_value == OAM_PROFILING_TX) {
        oam_profiling_tx_show_offset();
    } else if (ul_value == OAM_PROFILING_ALG) {
        oam_profiling_alg_show_offset();
    } else if (ul_value == OAM_PROFILING_CHIPSTART) {
        oam_profiling_starttime_show_offset();
    } else if (ul_value == OAM_PROFILING_CHSWTICH) {
        oam_profiling_chswitch_show_offset();
    } else {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_config_show_profiling:: invalide profiling type %d!}\r\n", ul_value);
    }
#endif

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_config_amsdu_tx_on(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_amsdu_tx_on(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_ampdu_tx_on(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_ampdu_tx_on(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_txbf_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_ampdu_tx_on_param_stru *pst_txbf_on_param = OAL_PTR_NULL;

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_txbf_switch::null ptr}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_txbf_on_param = (mac_cfg_ampdu_tx_on_param_stru *)puc_param;

    if (pst_txbf_on_param->uc_aggr_tx_on == 1 || pst_txbf_on_param->uc_aggr_tx_on == 0) {
    } else {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_config_txbf_switch:: pst_ampdu_tx_on_param->uc_aggr_tx_on %d!}", pst_txbf_on_param->uc_aggr_tx_on);
        return OAL_FAIL;
    }

    return OAL_SUCC;
}


OAL_STATIC uint32_t wal_config_frag_threshold(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    mac_cfg_frag_threshold_stru *pst_frag_threshold = OAL_PTR_NULL;

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_frag_threshold::NULL ptr}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_frag_threshold = (mac_cfg_frag_threshold_stru *)puc_param;

    if (pst_mac_vap->pst_mib_info == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_frag_threshold:pst_mib_info param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    mac_mib_set_frag_threshold(pst_mac_vap, pst_frag_threshold->ul_frag_threshold);
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_config_rts_threshold(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_rts_threshold_stru *pst_rts_threshold = OAL_PTR_NULL;

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{wal_config_rts_threshold::pst_mac_vap/puc_param is null ptr}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_rts_threshold = (mac_cfg_rts_threshold_stru *)puc_param;

    if (pst_mac_vap->pst_mib_info == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_config_rts_threshold::pst_mib_info null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    mac_mib_set_rts_threshold(pst_mac_vap, pst_rts_threshold->ul_rts_threshold);

    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_config_rts_threshold: mib rts %d!}\r\n",
        pst_mac_vap->pst_mib_info->st_wlan_mib_operation.ul_dot11RTSThreshold);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_config_kick_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_kick_user(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_country(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_country(pst_mac_vap, us_len, puc_param);
}

#ifdef _PRE_WLAN_FEATURE_DFS    // 1131_debug

OAL_STATIC oal_uint32 wal_config_set_country_for_dfs(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_country_for_dfs(pst_mac_vap, us_len, puc_param);
}
#endif


OAL_STATIC oal_uint32 wal_config_reset_hw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_reset_hw(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_reset_operate(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_reset_operate(pst_mac_vap, us_len, puc_param);
}


#ifdef _PRE_WLAN_FEATURE_UAPSD
OAL_STATIC oal_uint32 wal_config_uapsd_debug(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_UAPSD_DEBUG, us_len, puc_param);
}

#endif

#ifdef _PRE_WLAN_DFT_STAT


OAL_STATIC uint32_t wal_config_set_phy_stat_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_phy_stat_en(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_dbb_env_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_dbb_env_param(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_usr_queue_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_usr_queue_stat(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_report_vap_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_report_vap_stat(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_report_all_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_report_all_stat(pst_mac_vap, us_len, puc_param);
}
#endif


OAL_STATIC oal_uint32 wal_config_dump_rx_dscr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_dump_rx_dscr(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_dump_tx_dscr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_dump_tx_dscr(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_dump_memory(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_dump_memory_stru *pst_param = OAL_PTR_NULL;
    oal_uint8                *puc_addr = OAL_PTR_NULL;
    oal_uint32                ul_i;

    pst_param = (mac_cfg_dump_memory_stru *)puc_param;
    puc_addr  = (oal_uint8 *)(uintptr_t)(pst_param->ul_addr);

    for (ul_i = 0; ul_i < pst_param->ul_len; ul_i++) {
        OAL_IO_PRINT("%02x ", puc_addr[ul_i]);
        if (ul_i && (ul_i % 20 == 0)) {   // 打印20个char换行一次
            OAL_IO_PRINT("\n");
        }
    }

    OAL_IO_PRINT("\n");

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_config_get_country(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_country(pst_mac_vap, pus_len, puc_param);
}
#ifdef _PRE_WLAN_FEATURE_11D

OAL_STATIC oal_uint32 wal_config_set_rd_by_ie_switch (mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_rd_by_ie_switch(pst_mac_vap, us_len, puc_param);
}
#endif

OAL_STATIC oal_uint32 wal_config_get_tid(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_tid(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_vap_pkt_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_vap_pkt_stat(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_user_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_user_info(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_dscr_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_dscr_param(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_log_level(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_log_level(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_feature_log(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    // 抛事件到DMAC层, 同步DMAC数据
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_FEATURE_LOG, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{wal_config_set_feature_log::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

OAL_STATIC uint32_t wal_config_set_log_lowpower(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    oal_uint32          ul_ret;

    // 抛事件到DMAC层, 同步DMAC数据
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_LOG_PM, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{wal_config_set_feature_log::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

OAL_STATIC oal_uint32 wal_config_set_pm_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;
    oal_uint8           uc_en;

    uc_en = (oal_uint8)(*puc_param);

    OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_config_set_pm_switch:[%d]}", uc_en);

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    /* DTIM倍数INI配置复用PM_SWITCH传参，PM开关只判断BIT0即可 */
    if (uc_en & 0x1) {
        g_wlan_pm_switch = OAL_TRUE;
        wlan_pm_enable();
    } else {
        wlan_pm_disable();
        g_wlan_pm_switch = OAL_FALSE;
    }
#endif
    // 抛事件到DMAC层, 同步DMAC数据
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_PM_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_config_set_feature_log::send fail[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif


OAL_STATIC oal_uint32 wal_config_set_rate(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;
    mac_cfg_tx_comp_stru            st_event_set_bcast;

    memset_s(&st_event_set_bcast, OAL_SIZEOF(mac_cfg_tx_comp_stru), 0, OAL_SIZEOF(mac_cfg_tx_comp_stru));
    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_set_rate::pst_mac_vap/puc_param is null ptr}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置参数 */
    ul_ret = hmac_config_set_rate(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_set_rate::hmac_config_set_rate error!}\r\n");
        return ul_ret;
    }
    return ul_ret;
}


OAL_STATIC oal_uint32 wal_config_set_mcs(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_set_mcs::pst_mac_vap/puc_param is null ptr}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置参数 */
    ul_ret = hmac_config_set_mcs(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_set_mcs::hmac_config_set_mcs error.}\r\n");
        return ul_ret;
    }

    return ul_ret;
}


OAL_STATIC oal_uint32 wal_config_set_mcsac(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_set_mcsac::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置参数 */
    ul_ret = hmac_config_set_mcsac(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_set_mcsac::hmac_config_set_mcsac error!}");
        return ul_ret;
    }

    return ul_ret;
}


OAL_STATIC oal_uint32 wal_config_set_nss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_set_nss::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置参数 */
    ul_ret = hmac_config_set_nss(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_set_nss::hmac_config_set_nss error!}\r\n");
        return ul_ret;
    }

    return ul_ret;
}


OAL_STATIC oal_uint32 wal_config_set_rfch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_set_rfch::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置参数 */
    ul_ret = hmac_config_set_rfch(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_set_rfch::hmac_config_set_rfch error!}\r\n");
        return ul_ret;
    }

    return ul_ret;
}


OAL_STATIC oal_uint32 wal_config_set_bw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_set_bw::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置参数 */
    ul_ret = hmac_config_set_bw(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_set_bw::hmac_config_set_bw error!}\r\n");
        return ul_ret;
    }

    return ul_ret;
}

#if defined (_PRE_WLAN_CHIP_TEST) || defined (_PRE_WLAN_FEATURE_ALWAYS_TX)

OAL_STATIC oal_uint32 wal_config_always_tx(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;
    mac_cfg_tx_comp_stru            *pst_event_set_bcast = OAL_PTR_NULL;

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_always_tx::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event_set_bcast = (mac_cfg_tx_comp_stru *)puc_param;

    ul_ret = hmac_config_always_tx(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_always_tx::hmac_config_always_tx failed}");
        return ul_ret;
    }

    if (pst_event_set_bcast->uc_param == OAL_SWITCH_ON) {
        ul_ret = wal_config_bcast_pkt(pst_mac_vap, pst_event_set_bcast->ul_payload_len);
        if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_always_tx::wal_config_bcast_pkt fail}");
            return ul_ret;
        }
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_config_bcast_pkt(mac_vap_stru *pst_mac_vap, oal_uint32 ul_payload_len)
{
    oal_netbuf_stru                *pst_netbuf = OAL_PTR_NULL;
    hmac_vap_stru                  *pst_hmac_vap = OAL_PTR_NULL;
    oal_uint8                       uc_tid;
    oal_uint32                      ul_ret;

    /* 入参检查 */
    if (pst_mac_vap == OAL_PTR_NULL || pst_mac_vap->pst_mib_info == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_bcast_pkt::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_config_bcast_pkt::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_tid = 0;

    /* 组包 */
    pst_netbuf = wal_config_create_al_tx_packet(ul_payload_len, uc_tid,
        (oal_uint8)pst_mac_vap->bit_payload_flag, (oal_uint8)pst_hmac_vap->bit_init_flag);
    if (pst_netbuf == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_bcast_pkt::return null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    memset_s(OAL_NETBUF_CB(pst_netbuf), OAL_NETBUF_CB_SIZE(), 0, OAL_NETBUF_CB_SIZE());

    ul_ret = hmac_tx_lan_to_wlan(pst_mac_vap, pst_netbuf);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_config_bcast_pkt::hmac_tx_lan_to_wlan fail %d!}", ul_ret);
    }
    return ul_ret;
}

#endif  /* _PRE_WLAN_CHIP_TEST */

#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX

OAL_STATIC uint32_t wal_config_always_tx_1102(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_always_tx::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_always_tx_1102(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_always_tx::hmac_config_always_tx failed!}");
        return ul_ret;
    }

    return OAL_SUCC;
}
#endif /* #ifdef _PRE_WLAN_FEATURE_ALWAYS_TX */


OAL_STATIC oal_uint32 wal_config_always_rx(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_always_rx::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_always_rx(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_always_rx::hmac_config_always_rx failed}");
        return ul_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32 wal_config_dync_txpower(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_dync_txpower::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_dync_txpower(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_dync_txpower:hmac_config_dync_txpower fail}");
        return ul_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32 wal_config_get_thruput(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_get_thruput::pst_mac_vap/puc_param is null ptr}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_get_thruput(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_get_thruput::hmac_config_get_thruput fail}");
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_config_set_freq_skew(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_set_freq_skew::pst_mac_vap/puc_param is null ptr!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_set_freq_skew(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_config_set_freq_skew::hmac_config_set_freq_skew failed!}");
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_config_adjust_ppm(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_adjust_ppm::pst_mac_vap/puc_param is null ptr!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_adjust_ppm(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_adjust_ppm::hmac_config_adjust_ppm failed!}");
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_config_reg_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_reg_info(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_dbb_scaling_amend(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_dbb_scaling_amend(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_reg_write(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_reg_write(pst_mac_vap, us_len, puc_param);
}

#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST))

OAL_STATIC oal_uint32 wal_config_sdio_flowctrl(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_sdio_flowctrl(pst_mac_vap, us_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_FEATURE_HILINK

OAL_STATIC uint32_t wal_config_set_monitor_switch(mac_vap_stru *pst_mac_vap, uint16_t us_len, const uint8_t *puc_param)
{
    OAM_WARNING_LOG0(0, OAM_SF_ANY, "wal_config_set_monitor_switch:: in");
    return hmac_config_set_monitor_switch(pst_mac_vap, us_len, puc_param);
}
#endif


OAL_STATIC uint32_t wal_config_mips_cycle_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_mips_cycle_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_alg_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_alg_param(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_beacon_chain_switch(mac_vap_stru *pst_mac_vap, uint16_t us_len, const uint8_t *puc_param)
{
    return hmac_config_beacon_chain_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_2040_channel_switch_prohibited(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_2040_channel_switch_prohibited(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_FortyMHzIntolerant(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_FortyMHzIntolerant(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_2040_coext_support(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_2040_coext_support(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_rx_fcs_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_rx_fcs_info(pst_mac_vap, us_len, puc_param);
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32 wal_config_resume_rx_intr_fifo(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_resume_rx_intr_fifo(pst_mac_vap, us_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_FEATURE_DFS

OAL_STATIC oal_uint32 wal_config_dfs_radartool(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_dfs_radartool(pst_mac_vap, us_len, puc_param);
}
#endif

#ifdef _PRE_SUPPORT_ACS

OAL_STATIC oal_uint32 wal_config_acs(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_acs(pst_mac_vap, us_len, puc_param);
}
#endif


OAL_STATIC oal_uint32 wal_config_process_query(mac_vap_stru *pst_mac_vap,
                                               oal_uint8 *puc_req_msg,
                                               oal_uint16 us_req_msg_len,
                                               oal_uint8 *puc_rsp_msg,
                                               oal_uint8 *puc_rsp_msg_len)
{
    oal_uint16          us_req_idx = 0;      /* 请求消息索引 */
    oal_uint16          us_rsp_idx = 0;      /* 返回消息索引 */
    oal_uint16          us_len     = 0;      /* WID对应返回值的长度 */
    wal_msg_query_stru *pst_query_msg = OAL_PTR_NULL;
    wal_msg_write_stru *pst_rsp_msg = OAL_PTR_NULL;
    oal_uint32          ul_ret;
    oal_uint16          us_cfgid;

    if (OAL_UNLIKELY((pst_mac_vap == OAL_PTR_NULL) ||
        (puc_req_msg == OAL_PTR_NULL) ||
        (puc_rsp_msg == OAL_PTR_NULL) ||
        (puc_rsp_msg_len == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_process_query::param null error}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*
     * 查询消息格式如下:
     * +-------------------------------------------------------------------+
     * | WID0          | WID1         | WID2         | ................... |
     * +-------------------------------------------------------------------+
     * |     2 Bytes   |    2 Bytes   |    2 Bytes   | ................... |
     * +-------------------------------------------------------------------+
     * 返回消息格式如下:
     * +-------------------------------------------------------------------+
     * | WID0      | WID0 Length | WID0 Value  | ......................... |
     * +-------------------------------------------------------------------+
     * | 2 Bytes   | 2 Byte      | WID Length  | ......................... |
     * +-------------------------------------------------------------------+
     */
    while (us_req_idx < us_req_msg_len) {
        /* 从查询消息中得到一个WID值 */
        pst_query_msg = (wal_msg_query_stru *)(&puc_req_msg[us_req_idx]);
        us_req_idx   += WAL_MSG_WID_LENGTH;                       /* 指向下一个WID */

        /* 获取返回消息内存 */
        pst_rsp_msg = (wal_msg_write_stru *)(&puc_rsp_msg[us_rsp_idx]);

        /* 寻找cfgid 对应的get函数 */
        for (us_cfgid = 0; g_ast_board_wid_op[us_cfgid].en_cfgid != WLAN_CFGID_BUTT; us_cfgid++) {
            if (g_ast_board_wid_op[us_cfgid].en_cfgid == pst_query_msg->en_wid) {
                break;
            }
        }

        /* 异常情况，cfgid不在操作函数表中 */
        if (g_ast_board_wid_op[us_cfgid].en_cfgid == WLAN_CFGID_BUTT) {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_process_query::cfgid not invalid %d!}",
                pst_query_msg->en_wid);
            continue;
        }

        /* 异常情况，cfgid对应的get函数为空 */
        if (g_ast_board_wid_op[us_cfgid].p_get_func == OAL_PTR_NULL) {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                "{wal_config_process_query::g_ast_board_wid_op.p_get_func null, wid is %d!}", pst_query_msg->en_wid);
            continue;
        }

        ul_ret = g_ast_board_wid_op[us_cfgid].p_get_func(pst_mac_vap, &us_len, pst_rsp_msg->auc_value);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                "{wal_config_process_query::g_ast_board_wid_op fail wid:%d, ret:%d!}", pst_query_msg->en_wid, ul_ret);
            continue;
        }

        pst_rsp_msg->en_wid = pst_query_msg->en_wid;            /* 设置返回消息的WID */
        pst_rsp_msg->us_len = us_len;

        us_rsp_idx += us_len + WAL_MSG_WRITE_MSG_HDR_LENGTH;    /* 消息体的长度 再加上消息头的长度 */

        /* 消息Response 接口容易让调用者使用超过消息数组空间长度，这里需要加判断，检查长度和狗牌，后续需要整改 */
        if (OAL_UNLIKELY(us_rsp_idx + OAL_SIZEOF(wal_msg_hdr_stru) > HMAC_RSP_MSG_MAX_LEN)) {
            OAM_ERROR_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                "{wal_config_process_query::us_cfgid:%d reponse msg len:%u over limit:%u}",
                us_cfgid, us_rsp_idx + OAL_SIZEOF(wal_msg_hdr_stru), HMAC_RSP_MSG_MAX_LEN);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
            oal_print_hex_dump((oal_uint8*)puc_rsp_msg, HMAC_RSP_MSG_MAX_LEN, 32, "puc_rsp_msg: ");
            OAL_BUG_ON(1);
#endif
        }
    }

    *puc_rsp_msg_len = (oal_uint8)us_rsp_idx;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_config_process_write(mac_vap_stru *pst_mac_vap,
                                               oal_uint8 *puc_req_msg,
                                               oal_uint16 us_msg_len,
                                               oal_uint8 *puc_rsp_msg,
                                               oal_uint8 *puc_rsp_msg_len)
{
    oal_uint16              us_req_idx = 0;
    oal_uint16              us_rsp_idx = 0;
    oal_uint16              us_cfgid;
    wal_msg_write_stru     *pst_write_msg = OAL_PTR_NULL;
    wal_msg_write_rsp_stru *pst_rsp_msg = OAL_PTR_NULL;
    oal_uint32              ul_ret;

    if (OAL_UNLIKELY((pst_mac_vap == OAL_PTR_NULL) || (puc_req_msg == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_process_write::pst_mac_vap/puc_req_msg null ptr error}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*
     * 设置消息的格式如下:
     * +-------------------------------------------------------------------+
     * | WID0      | WID0 Length | WID0 Value  | ......................... |
     * +-------------------------------------------------------------------+
     * | 2 Bytes   | 2 Byte      | WID Length  | ......................... |
     * +-------------------------------------------------------------------+
     * 返回消息的格式如下:
     * +-------------------------------------------------------------------+
     * | WID0     | resv    | WID0 错误码 |  WID1   | resv | WID1错误码 |  |
     * +-------------------------------------------------------------------+
     * | 2 Bytes  | 2 Bytes | 4 Byte      | 2 Bytes | 2 B  |  4 Bytes   |  |
     * +-------------------------------------------------------------------+
     */
    while (us_req_idx < us_msg_len) {
        /* 获取一个设置WID消息 */
        pst_write_msg = (wal_msg_write_stru *)(&puc_req_msg[us_req_idx]);

        /* 获取返回消息内存 */
        pst_rsp_msg = (wal_msg_write_rsp_stru *)(&puc_rsp_msg[us_rsp_idx]);

        us_req_idx += pst_write_msg->us_len + WAL_MSG_WRITE_MSG_HDR_LENGTH;   /* 指向下一个WID设置消息 */

        /* 寻找cfgid 对应的write函数 */
        for (us_cfgid = 0; g_ast_board_wid_op[us_cfgid].en_cfgid != WLAN_CFGID_BUTT; us_cfgid++) {
            if (g_ast_board_wid_op[us_cfgid].en_cfgid == pst_write_msg->en_wid) {
                break;
            }
        }

        /* 异常情况，cfgid不在操作函数表中 */
        if (g_ast_board_wid_op[us_cfgid].en_cfgid == WLAN_CFGID_BUTT) {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                "{wal_config_process_write::cfgid not invalid %d!}\r\n", pst_write_msg->en_wid);
            continue;
        }

        ul_ret = g_ast_board_wid_op[us_cfgid].p_set_func(pst_mac_vap, pst_write_msg->us_len, pst_write_msg->auc_value);

        /* 将返回错误码设置到rsp消息中 */
        pst_rsp_msg->en_wid = pst_write_msg->en_wid;
        pst_rsp_msg->ul_err_code = ul_ret;
        us_rsp_idx += OAL_SIZEOF(wal_msg_write_rsp_stru);
        /* 消息Response 接口容易让调用者使用超过消息数组空间长度，这里需要加判断，检查长度和狗牌，后续需要整改 */
        if (OAL_UNLIKELY(us_rsp_idx + OAL_SIZEOF(wal_msg_hdr_stru) > HMAC_RSP_MSG_MAX_LEN)) {
            OAM_ERROR_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                "{wal_config_process_write::us_cfgid:%d reponse msg len:%u over limit:%u}",
                us_cfgid, us_rsp_idx + OAL_SIZEOF(wal_msg_hdr_stru), HMAC_RSP_MSG_MAX_LEN);
            OAL_BUG_ON(1);
        }

        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                "{wal_config_process_write::g_ast_board_wid_op func return no OAL_SUCC. wid and ret value is %d, %d!}",
                pst_write_msg->en_wid, ul_ret);
        }
    }

    *puc_rsp_msg_len = (oal_uint8)us_rsp_idx;

    return OAL_SUCC;
}

/* 功能描述 : 对超长函数wal_config_process_pkt的拆分 */
OAL_STATIC uint32_t wal_config_process_pkt_rsp(mac_vap_stru *pst_mac_vap,
                                               wal_msg_stru *pst_msg,
                                               wal_msg_stru *pst_rsp_msg,
                                               oal_uint8 uc_rsp_len,
                                               oal_ulong req_addr)
{
    oal_uint8               uc_rsp_toal_len;

    /* response 长度要包含头长 */
    uc_rsp_toal_len = uc_rsp_len + OAL_SIZEOF(wal_msg_hdr_stru);
    if (OAL_UNLIKELY(uc_rsp_toal_len > HMAC_RSP_MSG_MAX_LEN)) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_config_process_pkt::invaild response len %u!}", uc_rsp_toal_len);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        OAL_BUG_ON(1);
#endif
    }

    /* 填充返回消息头 */
    pst_rsp_msg->st_msg_hdr.en_msg_type = WAL_MSG_TYPE_RESPONSE;
    pst_rsp_msg->st_msg_hdr.uc_msg_sn   = pst_msg->st_msg_hdr.uc_msg_sn;
    pst_rsp_msg->st_msg_hdr.us_msg_len  = uc_rsp_len;

    if (req_addr) {
        /* need response */
        oal_uint8* pst_rsp_msg_tmp = oal_memalloc(uc_rsp_toal_len);
        if (pst_rsp_msg_tmp == NULL) {
            /* no mem */
            OAM_ERROR_LOG1(0, OAM_SF_ANY,
                "{wal_config_process_pkt::wal_config_process_pkt msg alloc %u failed!", uc_rsp_toal_len);
            wal_set_msg_response_by_addr(req_addr, NULL, OAL_ERR_CODE_PTR_NULL, uc_rsp_toal_len);
        } else {
            if (memcpy_s(pst_rsp_msg_tmp, uc_rsp_toal_len, pst_rsp_msg, uc_rsp_toal_len) != EOK) {
                OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_config_process_pkt::memcpy_s failed!");
                oal_free(pst_rsp_msg_tmp);
                return OAL_FAIL;
            }
            if (wal_set_msg_response_by_addr(req_addr, pst_rsp_msg_tmp, OAL_SUCC, uc_rsp_toal_len) != OAL_SUCC) {
                OAL_IO_PRINT("wal_config_process_pkt did't found the request msg, addr:0x%lx\n", req_addr);
                OAM_ERROR_LOG0(0, OAM_SF_ANY,
                    "{wal_config_process_pkt::wal_set_msg_response_by_addr did't found the request msg!");
                oal_free(pst_rsp_msg_tmp);
            }
        }
    }

    return OAL_SUCC;
}


oal_uint32 wal_config_process_pkt(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru         *pst_event = OAL_PTR_NULL;
    wal_msg_stru           *pst_msg = OAL_PTR_NULL;
    wal_msg_stru           *pst_rsp_msg = OAL_PTR_NULL;
    frw_event_hdr_stru     *pst_event_hdr = OAL_PTR_NULL;
    mac_vap_stru           *pst_mac_vap = OAL_PTR_NULL;
    oal_uint16              us_msg_len;
    oal_uint8               uc_rsp_len = 0;
    oal_uint32              ul_ret;
    oal_ulong               ul_request_address;
    oal_uint8               ac_rsp_msg[HMAC_RSP_MSG_MAX_LEN] = {0};

    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_process_pkt::pst_event_mem null ptr error!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event     = (frw_event_stru *)pst_event_mem->puc_data;
    pst_event_hdr = &(pst_event->st_event_hdr);
    ul_request_address = ((wal_msg_rep_hdr*)pst_event->auc_event_data)->ul_request_address;
    pst_msg       = (wal_msg_stru *)(frw_get_event_payload(pst_event_mem) + OAL_SIZEOF(wal_msg_rep_hdr)); //lint !e416
    pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_event_hdr->uc_vap_id);
    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_ANY, "{wal_config_process_pkt::return err code!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 取返回消息 */
    pst_rsp_msg  = (wal_msg_stru *)ac_rsp_msg;
    /* 取配置消息的长度 */
    us_msg_len = pst_msg->st_msg_hdr.us_msg_len;
    OAM_INFO_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_ANY, "{wal_config_process_pkt::a config event occur!}\r\n");

    switch (pst_msg->st_msg_hdr.en_msg_type) {
        case WAL_MSG_TYPE_QUERY:
            ul_ret = wal_config_process_query(pst_mac_vap, pst_msg->auc_msg_data,
                us_msg_len, pst_rsp_msg->auc_msg_data, &uc_rsp_len);
            if (ul_ret != OAL_SUCC) {
                OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                    "{wal_config_process_pkt::wal_config_process_query return error code %d!}\r\n", ul_ret);
                return ul_ret;
            }

            break;

        case WAL_MSG_TYPE_WRITE:
            ul_ret = wal_config_process_write(pst_mac_vap, pst_msg->auc_msg_data,
                us_msg_len, pst_rsp_msg->auc_msg_data, &uc_rsp_len);
            if (ul_ret != OAL_SUCC) {
                OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                    "{wal_config_process_pkt::wal_config_process_write return error code %d!}\r\n", ul_ret);
                return ul_ret;
            }

            break;

        default:
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                "{wal_config_process_pkt::error en_msg_type : %d!}\r\n", pst_msg->st_msg_hdr.en_msg_type);

            return OAL_ERR_CODE_INVALID_CONFIG;
    }

    ul_ret = wal_config_process_pkt_rsp(pst_mac_vap, pst_msg, pst_rsp_msg, uc_rsp_len, ul_request_address);
    if (ul_ret != OAL_SUCC) {
        return ul_ret;
    }

    /* 唤醒WAL等待的进程 */
    wal_cfg_msg_task_sched();

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_config_add_key(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_11i_add_key(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_key(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    /* 通过函数调用，hmac具体实现 */
    return (hmac_config_11i_get_key(pst_mac_vap, us_len, puc_param));
}


OAL_STATIC oal_uint32 wal_config_remove_key(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    /* 通过函数调用，hmac具体实现 */
    return (hmac_config_11i_remove_key(pst_mac_vap, us_len, puc_param));
}


OAL_STATIC uint32_t wal_config_set_default_key(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    /* 通过函数调用，hmac具体实现 */
    return (hmac_config_11i_set_default_key(pst_mac_vap, us_len, puc_param));
}


oal_uint32 wal_config_set_wps_ie(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_wps_ie(pst_mac_vap, us_len, puc_param);
}


oal_uint32 wal_config_set_wps_p2p_ie(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_wps_p2p_ie(pst_mac_vap, us_len, puc_param);
}

#ifdef _PRE_WLAN_PERFORM_STAT

OAL_STATIC oal_uint32 wal_config_pfm_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_pfm_stat(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_pfm_display(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_pfm_display(pst_mac_vap, us_len, puc_param);
}

#endif

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP

OAL_STATIC oal_uint32 wal_config_set_edca_opt_switch_sta(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_edca_opt_switch_sta(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_edca_opt_weight_sta(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_edca_opt_weight_sta(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_edca_opt_switch_ap(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_edca_opt_switch_ap(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_edca_opt_cycle_ap(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_edca_opt_cycle_ap(pst_mac_vap, us_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_CHIP_TEST

OAL_STATIC oal_uint32 wal_config_lpm_tx_data(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_lpm_tx_data(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_lpm_tx_probe_request(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return  hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_LPM_TX_PROBE_REQUEST, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_lpm_chip_state(mac_vap_stru *pst_mac_vap, uint16_t us_len, const uint8_t *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_LPM_CHIP_STATE, us_len, puc_param);
}

OAL_STATIC uint32_t wal_config_set_lpm_soc_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_LPM_SOC_MODE, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_lpm_psm_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_LPM_PSM_PARAM, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_lpm_smps_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_LPM_SMPS_MODE, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_lpm_smps_stub(mac_vap_stru *pst_mac_vap, uint16_t us_len, const uint8_t *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_LPM_SMPS_STUB, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_lpm_txop_ps(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_LPM_TXOP_PS_SET, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_lpm_txop_tx_stub(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_LPM_TXOP_TX_STUB, us_len, puc_param);
}



OAL_STATIC uint32_t wal_config_set_lpm_wow_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_LPM_WOW_EN, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_remove_user_lut(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_REMOVE_LUT, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_send_frame(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_SEND_FRAME, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_rx_pn(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_SET_RX_PN_REG, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_soft_retry(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_SET_SOFT_RETRY, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_open_addr4(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_OPEN_ADDR4, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_open_wmm_test(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_OPEN_WMM_TEST, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_chip_test_open(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
#ifdef _PRE_WLAN_CHIP_TEST
        /* 设置一下hmac的芯片验证开关 */
        hmac_test_set_chip_test(*(oal_uint8*)puc_param);
#endif

    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_CHIP_TEST_OPEN, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_coex(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
/* 通过函数调用，hmac具体实现 */
#ifdef _PRE_WLAN_CHIP_TEST
    return (hmac_config_set_coex(pst_mac_vap, us_len, puc_param));
#else
    return OAL_SUCC;
#endif
}


OAL_STATIC oal_uint32 wal_config_set_dfx(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
#ifdef _PRE_WLAN_CHIP_TEST
    return hmac_config_set_dfx(pst_mac_vap, us_len, puc_param);
#else
    return OAL_SUCC;
#endif
}

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)

OAL_STATIC oal_uint32 wal_config_enable_pmf(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    /* 设置一下hmac的芯片验证开关 */
    hmac_enable_pmf(pst_mac_vap, puc_param);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_PMF_ENABLE, us_len, puc_param);
#else
    return OAL_SUCC;
#endif
}
#endif


OAL_STATIC oal_uint32 wal_config_send_action(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_test_send_action(pst_mac_vap, puc_param);
}


OAL_STATIC oal_uint32 wal_config_send_pspoll(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_send_pspoll(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_send_nulldata(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_send_nulldata(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_clear_all_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_clear_all_stat(pst_mac_vap, us_len, puc_param);
}
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */


OAL_STATIC oal_uint32 wal_config_open_wmm(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_open_wmm::pst_mac_vap/puc_param is null ptr}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 针对配置vap做保护 */
    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_CONFIG) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{wal_config_open_wmm::this is config vap! can't get info.}");
        return OAL_FAIL;
    }

    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_WMM_SWITCH, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_hide_ssid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_hide_ssid(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_mib(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_mib(pst_mac_vap, WLAN_CFGID_SET_MIB, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_get_mib(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_get_mib(pst_mac_vap, WLAN_CFGID_GET_MIB, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_thruput_bypass(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_set_thruput_bypass(pst_mac_vap, WLAN_CFGID_SET_THRUPUT_BYPASS, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_auto_protection(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const uint8_t *puc_param)
{
    oal_uint8 uc_auto_protection_flag;

    if (pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_auto_protection_flag = (oal_uint8) * ((oal_uint32 *)puc_param);

    return hmac_config_set_auto_protection(pst_mac_vap, uc_auto_protection_flag);
}


OAL_STATIC uint32_t wal_config_send_2040_coext(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_send_2040_coext(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_2040_coext_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_2040_coext_info(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_version(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_get_version(pst_mac_vap, us_len, puc_param);
}



#ifdef _PRE_DEBUG_MODE
OAL_STATIC uint32_t wal_config_get_all_reg_value(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_get_all_reg_value(pst_mac_vap, us_len, puc_param);
}
#endif
#ifdef _PRE_WLAN_FEATURE_DAQ

OAL_STATIC oal_uint32 wal_config_data_acq(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_data_acq(pst_mac_vap, us_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_FEATURE_SMPS
#ifdef _PRE_DEBUG_MODE

OAL_STATIC uint32_t wal_config_get_smps_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_get_smps_info(pst_mac_vap, us_len, puc_param);
}
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_PROXYSTA

oal_uint32  wal_config_set_oma(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_oma(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_proxysta_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_proxysta_switch(pst_mac_vap, us_len, puc_param);
}

#endif

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY

OAL_STATIC uint32_t wal_config_set_opmode_notify(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_set_opmode_notify(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_get_user_rssbw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_get_user_rssbw(pst_mac_vap, us_len, puc_param);
}
#endif


OAL_STATIC oal_uint32  wal_config_set_vap_nss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_vap_nss(pst_mac_vap, us_len, puc_param);
}

#ifdef _PRE_WLAN_DFT_REG

oal_uint32  wal_config_dump_reg(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_device_stru            *pst_mac_dev = OAL_PTR_NULL;
    hal_to_dmac_device_stru    *pst_hal_dmac_dev = OAL_PTR_NULL;

    pst_mac_dev = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(pst_mac_dev == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_config_dump_reg::pst_device[id:%d] is NULL!}",
            pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_hal_dmac_dev = pst_mac_dev->pst_device_stru;
    if (hi1151_debug_refresh_reg(pst_hal_dmac_dev, OAM_REG_EVT_USR) != OAL_SUCC) {
        return OAL_SUCC;
    }
    hi1151_debug_frw_evt(pst_hal_dmac_dev);
    return OAL_SUCC;
}
#endif


#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY

OAL_STATIC oal_uint32 wal_config_blacklist_add(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_blacklist_add(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_blacklist_add_only(mac_vap_stru *pst_mac_vap, uint16_t us_len, const uint8_t *puc_param)
{
    return hmac_config_blacklist_add_only(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_blacklist_del(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_blacklist_del(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_blacklist_mode(mac_vap_stru *pst_mac_vap, uint16_t us_len, const uint8_t *puc_param)
{
    return hmac_config_set_blacklist_mode(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_blacklist_show(mac_vap_stru *pst_mac_vap, uint16_t us_len, const uint8_t *puc_param)
{
    return hmac_config_show_blacklist(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_isolation_show(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_show_isolation(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_autoblacklist_enable(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_autoblacklist_enable(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_autoblacklist_aging(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_autoblacklist_aging(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_autoblacklist_threshold(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_autoblacklist_threshold(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_autoblacklist_reset_time(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_autoblacklist_reset_time(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_isolation_mode(mac_vap_stru *pst_mac_vap, uint16_t us_len, const uint8_t *puc_param)
{
    return hmac_config_set_isolation_mode(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_isolation_type(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_isolation_type(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_isolation_forward(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_isolation_forword(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_isolation_clear(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_isolation_clear(pst_mac_vap, us_len, puc_param);
}
#endif  /* #ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY */

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP

OAL_STATIC oal_uint32 wal_config_proxyarp_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_proxyarp_en_stru *pst_proxyarp_en_param = OAL_PTR_NULL;

    pst_proxyarp_en_param = (mac_proxyarp_en_stru *)puc_param;

    hmac_proxyarp_on(pst_mac_vap, pst_proxyarp_en_param->en_proxyarp);

    return OAL_SUCC;
}


#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32 wal_config_proxyarp_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    hmac_proxy_display_info(pst_mac_vap);
    return OAL_SUCC;
}
#endif /* #ifdef _PRE_DEBUG_MODE */
#endif

#ifdef _PRE_WLAN_FEATURE_WAPI

#ifdef _PRE_WAPI_DEBUG
OAL_STATIC oal_uint32 wal_config_wapi_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_user_info_param_stru *pst_user_info = OAL_PTR_NULL;
    pst_user_info = (mac_cfg_user_info_param_stru *)puc_param;

    hmac_wapi_display_info(pst_mac_vap, pst_user_info->us_user_idx);
    return OAL_SUCC;
}
#endif /* #ifdef _PRE_DEBUG_MODE */
#endif /* _PRE_WLAN_FEATURE_WAPI */


oal_int32 wal_recv_config_cmd(oal_uint8 *puc_buf, oal_uint16 us_len)
{
    oal_int8                ac_vap_name[OAL_IF_NAME_SIZE];
    oal_net_device_stru    *pst_net_dev = OAL_PTR_NULL;
    mac_vap_stru           *pst_mac_vap = OAL_PTR_NULL;
    frw_event_mem_stru     *pst_event_mem = OAL_PTR_NULL;
    frw_event_stru         *pst_event = OAL_PTR_NULL;
    oal_uint32              ul_ret;
    wal_msg_stru           *pst_msg = OAL_PTR_NULL;
    oal_netbuf_stru        *pst_netbuf = OAL_PTR_NULL;
    oal_uint16              us_netbuf_len; /* 传给sdt的skb数据区不包括头尾空间的长度 */
    wal_msg_stru           *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_rep_hdr        *pst_rep_hdr = OAL_PTR_NULL;
    oal_uint16              us_msg_size = us_len;
    oal_uint16              us_need_response = OAL_FALSE;
    oal_uint32              l_ret = EOK;

    DECLARE_WAL_MSG_REQ_STRU(st_msg_request);

    WAL_MSG_REQ_STRU_INIT(st_msg_request);
    if (us_msg_size < OAL_IF_NAME_SIZE) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_recv_config_cmd_etc::msg_size[%d] overrun!}", us_msg_size);
        return -OAL_EINVAL;
    }

    l_ret += memcpy_s(ac_vap_name, OAL_IF_NAME_SIZE, puc_buf, OAL_IF_NAME_SIZE);
    ac_vap_name[OAL_IF_NAME_SIZE - 1] = '\0';   /* 防止字符串异常 */

    /* 根据dev_name找到dev */
    pst_net_dev = oal_dev_get_by_name(ac_vap_name);
    if (pst_net_dev == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_recv_config_cmd::oal_dev_get_by_name return null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_dev_put(pst_net_dev);   /* 调用oal_dev_get_by_name后，必须调用oal_dev_put使net_dev的引用计数减一 */

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);    /* 获取mac vap */
    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_recv_config_cmd::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr.}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    us_msg_size -= OAL_IF_NAME_SIZE;

    /* 申请内存 */
    pst_event_mem = FRW_EVENT_ALLOC(us_msg_size + OAL_SIZEOF(wal_msg_rep_hdr));
    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_recv_config_cmd::request %d mem failed}\r\n",
                       us_msg_size);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    /* 填写事件头 */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_CRX,
                       WAL_HOST_CRX_SUBTYPE_CFG,
                       (oal_uint16)(us_msg_size),
                       FRW_EVENT_PIPELINE_STAGE_0,
                       pst_mac_vap->uc_chip_id,
                       pst_mac_vap->uc_device_id,
                       pst_mac_vap->uc_vap_id);

    /* 填写事件payload */
    l_ret += memcpy_s(frw_get_event_payload(pst_event_mem) + OAL_SIZEOF(wal_msg_rep_hdr),
        us_msg_size, puc_buf + OAL_IF_NAME_SIZE, us_msg_size); //lint !e416
    pst_msg = (wal_msg_stru *)(puc_buf + OAL_IF_NAME_SIZE);
    pst_rep_hdr = (wal_msg_rep_hdr*)pst_event->auc_event_data;

    if (pst_msg->st_msg_hdr.en_msg_type == WAL_MSG_TYPE_QUERY) {
        /* need response */
        us_need_response = OAL_TRUE;
    }
    pst_rep_hdr->ul_request_address = 0;
    if (us_need_response == OAL_TRUE) {
        pst_rep_hdr->ul_request_address = (oal_ulong)(uintptr_t)&st_msg_request;
        wal_msg_request_add_queue(&st_msg_request);
    }

    ul_ret = wal_config_process_pkt(pst_event_mem);

    if (us_need_response == OAL_TRUE) {
        wal_msg_request_remove_queue(&st_msg_request);
    }

    if (ul_ret != OAL_SUCC) {
        FRW_EVENT_FREE(pst_event_mem);
        if (st_msg_request.pst_resp_mem != NULL) {
            /* 异常时内存需要释放 */
            oal_free(st_msg_request.pst_resp_mem);
        }
        return (oal_int32)ul_ret;
    }

    /* 释放内存 */
    FRW_EVENT_FREE(pst_event_mem);

    /* 如果是查询消息类型，结果上报 */
    if (us_need_response == OAL_TRUE) {
        if (OAL_UNLIKELY(g_st_oam_sdt_func_hook.p_sdt_report_data_func == OAL_PTR_NULL)) {
            if (st_msg_request.pst_resp_mem != NULL) {
                /* 异常时内存需要释放 */
                oal_free(st_msg_request.pst_resp_mem);
            }
            return OAL_ERR_CODE_PTR_NULL;
        }

        if (st_msg_request.pst_resp_mem == NULL) {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_recv_config_cmd::get response ptr failed!}");
            return (oal_int32)ul_ret;
        }

        pst_rsp_msg  = (wal_msg_stru *)st_msg_request.pst_resp_mem;

        us_netbuf_len = pst_rsp_msg->st_msg_hdr.us_msg_len + 1; /* +1是sdt工具的需要 */

        us_netbuf_len = (us_netbuf_len > WLAN_SDT_NETBUF_MAX_PAYLOAD) ? WLAN_SDT_NETBUF_MAX_PAYLOAD : us_netbuf_len;

        pst_netbuf = oam_alloc_data2sdt(us_netbuf_len);
        if (pst_netbuf == OAL_PTR_NULL) {
            if (st_msg_request.pst_resp_mem != NULL) {
                /* 异常时内存需要释放 */
                oal_free(st_msg_request.pst_resp_mem);
            }
            return OAL_ERR_CODE_PTR_NULL;
        }

        oal_netbuf_data(pst_netbuf)[0] = 'M';     /* sdt需要 */
        l_ret += memcpy_s(oal_netbuf_data(pst_netbuf) + 1, us_netbuf_len - 1,
            (oal_uint8 *)pst_rsp_msg->auc_msg_data, us_netbuf_len - 1);
        if (l_ret != EOK) {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_judgment_memcpy_result::memcpy_s fail!");
            oal_free(st_msg_request.pst_resp_mem);
            st_msg_request.pst_resp_mem = NULL;
            return OAL_FAIL;
        }
        oal_free(st_msg_request.pst_resp_mem);
        st_msg_request.pst_resp_mem = NULL;

        oam_report_data2sdt(pst_netbuf, OAM_DATA_TYPE_CFG, OAM_PRIMID_TYPE_DEV_ACK);
    }

    return OAL_SUCC;
}


oal_int32  wal_recv_memory_cmd(oal_uint8 *puc_buf, oal_uint16 us_len)
{
    oal_netbuf_stru            *pst_netbuf = OAL_PTR_NULL;
    wal_sdt_mem_frame_stru     *pst_mem_frame = OAL_PTR_NULL;
    oal_ulong                    ul_mem_addr;       /* 读取内存地址 */
    oal_uint16                  us_mem_len;         /* 需要读取的长度 */
    oal_uint8                   uc_offload_core_mode; /* offload下，表示哪一个核 */
    oal_uint32                   l_ret = EOK;

    pst_mem_frame        = (wal_sdt_mem_frame_stru *)puc_buf;
    ul_mem_addr          = pst_mem_frame->ul_addr;
    us_mem_len           = pst_mem_frame->us_len;
    uc_offload_core_mode = pst_mem_frame->en_offload_core_mode;

    if (uc_offload_core_mode == WAL_OFFLOAD_CORE_MODE_DMAC) {
        /* 如果是offload情形，并且要读取的内存是wifi芯片侧，需要抛事件，后续开发 */
        return OAL_SUCC;
    }

    if (ul_mem_addr == OAL_PTR_NULL) { /* 读写地址不合理 */
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (us_mem_len > WAL_SDT_MEM_MAX_LEN) {
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    if (pst_mem_frame->uc_mode == MAC_SDT_MODE_READ) {
        if (OAL_UNLIKELY(g_st_oam_sdt_func_hook.p_sdt_report_data_func == OAL_PTR_NULL)) {
            return OAL_ERR_CODE_PTR_NULL;
        }

        pst_netbuf = oam_alloc_data2sdt(us_mem_len);
        if (pst_netbuf == OAL_PTR_NULL) {
            return OAL_ERR_CODE_PTR_NULL;
        }

        l_ret += memcpy_s(oal_netbuf_data(pst_netbuf), us_mem_len + 1, (oal_void *)(uintptr_t)ul_mem_addr, us_mem_len);

        oam_report_data2sdt(pst_netbuf, OAM_DATA_TYPE_MEM_RW, OAM_PRIMID_TYPE_DEV_ACK);
    } else if (pst_mem_frame->uc_mode == MAC_SDT_MODE_WRITE) {
        l_ret += memcpy_s((oal_void *)(uintptr_t)ul_mem_addr, us_mem_len + 1, pst_mem_frame->auc_data, us_mem_len);
    }
    if (l_ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_recv_memory_cmd::memcpy_s fail!");
        return OAL_FAIL;
    }
    return OAL_SUCC;
}


oal_int32 wal_parse_global_var_cmd(wal_sdt_global_var_stru *pst_global_frame, oal_ulong ul_global_var_addr)
{
    oal_netbuf_stru            *pst_netbuf = OAL_PTR_NULL;
    oal_uint16                  us_skb_len;

    if (pst_global_frame->uc_mode == MAC_SDT_MODE_WRITE) {
        if (memcpy_s((void *)(uintptr_t)ul_global_var_addr, WAL_GLB_VAR_NAME_LEN + 1,
            (void *)(pst_global_frame->auc_global_value), pst_global_frame->us_len) != EOK) {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_parse_global_var_cmd::memcpy_s failed!");
        }
    } else if (pst_global_frame->uc_mode == MAC_SDT_MODE_READ) {
        if (OAL_UNLIKELY(g_st_oam_sdt_func_hook.p_sdt_report_data_func == OAL_PTR_NULL)) {
            return OAL_ERR_CODE_PTR_NULL;
        }

        us_skb_len = pst_global_frame->us_len;
        us_skb_len = (us_skb_len > WLAN_SDT_NETBUF_MAX_PAYLOAD) ? WLAN_SDT_NETBUF_MAX_PAYLOAD : us_skb_len;
        pst_netbuf = oam_alloc_data2sdt(us_skb_len);
        if (pst_netbuf == OAL_PTR_NULL) {
            return OAL_ERR_CODE_PTR_NULL;
        }

        if (memcpy_s(oal_netbuf_data(pst_netbuf), oal_netbuf_get_len(pst_netbuf), (void *)(uintptr_t)ul_global_var_addr,
            us_skb_len) != EOK) {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_parse_global_var_cmd::memcpy_s failed!");
        }

        oam_report_data2sdt(pst_netbuf, OAM_DATA_TYPE_MEM_RW, OAM_PRIMID_TYPE_DEV_ACK);
    }

    return OAL_SUCC;
}


oal_int32 wal_recv_global_var_cmd(oal_uint8 *puc_buf, oal_uint16 us_len)
{
    wal_sdt_global_var_stru        *pst_global_frame = OAL_PTR_NULL;
    oal_ulong                       ul_global_var_addr;

    if (puc_buf == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (us_len < sizeof(wal_sdt_global_var_stru)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_recv_global_var_cmd::input us_len err!}\r\n");
        return OAL_FAIL;
    }
    pst_global_frame = (wal_sdt_global_var_stru *)puc_buf;

    if (pst_global_frame->en_offload_core_mode == WAL_OFFLOAD_CORE_MODE_DMAC) {
        /* offload情形，并且要读取的全局变量在wifi芯片侧，需要抛事件，后续开发 */
        return OAL_SUCC;
    }

    ul_global_var_addr = oal_kallsyms_lookup_name(pst_global_frame->auc_global_value_name);
    if (ul_global_var_addr == 0) { /* not found */
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_recv_global_var_cmd::kernel lookup global var address returns 0!}\r\n");
        return OAL_FAIL;
    }

    return wal_parse_global_var_cmd(pst_global_frame, ul_global_var_addr);
}


oal_int32 wal_recv_reg_cmd(oal_uint8 *puc_buf, oal_uint16 us_len)
{
    oal_int8                     ac_vap_name[OAL_IF_NAME_SIZE];
    oal_net_device_stru         *pst_net_dev = OAL_PTR_NULL;
    mac_vap_stru                *pst_mac_vap = OAL_PTR_NULL;
    wal_sdt_reg_frame_stru      *pst_reg_frame = OAL_PTR_NULL;
    oal_int32                    l_ret;
    hmac_vap_cfg_priv_stru      *pst_cfg_priv = OAL_PTR_NULL;
    oal_netbuf_stru             *pst_net_buf = OAL_PTR_NULL;
    oal_uint32                   ul_ret;

    if (us_len < OAL_IF_NAME_SIZE) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_recv_reg_cmd::us_len[%d] overrun!}", us_len);
        return -OAL_EINVAL;
    }

    if (memcpy_s(ac_vap_name, OAL_IF_NAME_SIZE, puc_buf, OAL_IF_NAME_SIZE) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_recv_reg_cmd::memcpy_s failed!");
        return -OAL_EINVAL;
    }
    ac_vap_name[OAL_IF_NAME_SIZE - 1] = '\0';   /* 防止字符串异常 */

    /* 根据dev_name找到dev */
    pst_net_dev = oal_dev_get_by_name(ac_vap_name);
    if (pst_net_dev == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_recv_reg_cmd::oal_dev_get_by_name return null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_dev_put(pst_net_dev);   /* 调用oal_dev_get_by_name后，必须调用oal_dev_put使net_dev的引用计数减一 */

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);    /* 获取mac vap */

    ul_ret = hmac_vap_get_priv_cfg(pst_mac_vap, &pst_cfg_priv);      /* 取配置私有结构体 */
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_recv_reg_cmd::hmac_vap_get_priv_cfg fail!}\r\n");
        return (oal_int32)ul_ret;
    }

    pst_cfg_priv->en_wait_ack_for_sdt_reg = OAL_FALSE;

    ul_ret = hmac_sdt_recv_reg_cmd(pst_mac_vap, puc_buf, us_len);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_recv_reg_cmd::hmac_sdt_recv_reg_cmd fail!}\r\n");

        return (oal_int32)ul_ret;
    }

    pst_reg_frame = (wal_sdt_reg_frame_stru *)puc_buf;

    if (pst_reg_frame->uc_mode == MAC_SDT_MODE_READ) {
        wal_wake_lock();
        /*lint -e730*/ /* info, boolean argument to function */
        l_ret = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(pst_cfg_priv->st_wait_queue_for_sdt_reg,
                                                     (pst_cfg_priv->en_wait_ack_for_sdt_reg == OAL_TRUE),
                                                     2 * OAL_TIME_HZ); //lint !e665
        /*lint +e730*/
        if (l_ret == 0) {
            /* 超时 */
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_recv_reg_cmd::wal_netdev_open: wait timeout}");
            wal_wake_unlock();
            return -OAL_EINVAL;
        } else if (l_ret < 0) {
            /* 异常 */
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_recv_reg_cmd::wal_netdev_open:wait error}");
            wal_wake_unlock();
            return -OAL_EINVAL;
        }
        wal_wake_unlock();
        /*lint +e774*/
        /* 读取返回的寄存器值 */
        pst_reg_frame->ul_reg_val = *((oal_uint32 *)(pst_cfg_priv->ac_rsp_msg));

        if (OAL_UNLIKELY(g_st_oam_sdt_func_hook.p_sdt_report_data_func != OAL_PTR_NULL)) {
            pst_net_buf = oam_alloc_data2sdt((oal_uint16)OAL_SIZEOF(wal_sdt_reg_frame_stru));
            if (pst_net_buf == OAL_PTR_NULL) {
                return OAL_ERR_CODE_PTR_NULL;
            }

            if (memcpy_s(oal_netbuf_data(pst_net_buf), (oal_uint16)OAL_SIZEOF(wal_sdt_reg_frame_stru),
                         (oal_uint8 *)pst_reg_frame, (oal_uint16)OAL_SIZEOF(wal_sdt_reg_frame_stru)) != EOK) {
                OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_recv_reg_cmd::memcpy_s failed!");
                return -OAL_EINVAL;
            }

            oam_report_data2sdt(pst_net_buf, OAM_DATA_TYPE_REG_RW, OAM_PRIMID_TYPE_DEV_ACK);
        }
    }

    return OAL_SUCC;
}


oal_void wal_drv_cfg_func_hook_init(oal_void)
{
    g_st_wal_drv_func_hook.p_wal_recv_cfg_data_func     = wal_recv_config_cmd;
    g_st_wal_drv_func_hook.p_wal_recv_mem_data_func     = wal_recv_memory_cmd;
    g_st_wal_drv_func_hook.p_wal_recv_reg_data_func     = wal_recv_reg_cmd;
    g_st_wal_drv_func_hook.p_wal_recv_global_var_func   = wal_recv_global_var_cmd;
}


#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32 wal_config_rx_filter_val(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_rx_filter_val(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_rx_filter_en(mac_vap_stru *pst_mac_vap, uint16_t us_len, const uint8_t *puc_param)
{
    return hmac_config_set_rx_filter_en(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_get_rx_filter_en(mac_vap_stru *pst_mac_vap, uint16_t us_len, const uint8_t *puc_param)
{
    return hmac_config_get_rx_filter_en(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_report_ampdu_stat(mac_vap_stru *pst_mac_vap, uint16_t us_len, const uint8_t *puc_param)
{
    return hmac_config_report_ampdu_stat(pst_mac_vap, us_len, puc_param);
}
#endif


OAL_STATIC oal_uint32 wal_config_set_ampdu_aggr_num(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_ampdu_aggr_num(pst_mac_vap, us_len, puc_param);
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

OAL_STATIC uint32_t wal_config_set_ampdu_mmss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_ampdu_mmss(pst_mac_vap, us_len, puc_param);
}
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32 wal_config_freq_adjust(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_freq_adjust(pst_mac_vap, us_len, puc_param);
}
#endif


OAL_STATIC oal_uint32 wal_config_set_stbc_cap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_stbc_cap(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_ldpc_cap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_ldpc_cap(pst_mac_vap, us_len, puc_param);
}

#ifdef _PRE_WLAN_FEATURE_MCAST

OAL_STATIC oal_uint32 wal_config_m2u_snoop_on(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_m2u_snoop_on_param_stru *pst_m2u_snoop_on_param = OAL_PTR_NULL;
    hmac_vap_stru *pst_hmac_vap = OAL_PTR_NULL;
    hmac_m2u_stru *pst_m2u = OAL_PTR_NULL;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    pst_m2u = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG2(0, OAM_SF_M2U, "{wal_config_m2u_snoop_on:: pst_mac_vap/puc_param is null ptr %d, %d!}",
            pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_m2u_snoop_on_param = (mac_cfg_m2u_snoop_on_param_stru *)puc_param;

    /* uc_m2u_snoop_on */
    if (pst_m2u_snoop_on_param->uc_m2u_snoop_on == OAL_TRUE || pst_m2u_snoop_on_param->uc_m2u_snoop_on == OAL_FALSE) {
        pst_m2u->en_snoop_enable = pst_m2u_snoop_on_param->uc_m2u_snoop_on;
    } else {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_M2U,
            "{wal_config_m2u_snoop_on::pst_m2u_snoop_on_param->uc_m2u_snoop_on %d!}",
            pst_m2u_snoop_on_param->uc_m2u_snoop_on);
        return OAL_FAIL;
    }

    /* uc_m2u_mcast_mode */
    if (pst_m2u_snoop_on_param->uc_m2u_mcast_mode == HMAC_M2U_MCAST_TUNNEL ||
        pst_m2u_snoop_on_param->uc_m2u_mcast_mode == HMAC_M2U_MCAST_MAITAIN ||
        pst_m2u_snoop_on_param->uc_m2u_mcast_mode == HMAC_M2U_MCAST_TRANSLATE) {
        pst_m2u->en_mcast_mode = pst_m2u_snoop_on_param->uc_m2u_mcast_mode;
    } else {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_M2U,
            "{wal_config_m2u_snoop_on::pst_m2u_snoop_on_param->uc_m2u_mcast_mode %d!}",
            pst_m2u_snoop_on_param->uc_m2u_mcast_mode);
        return OAL_FAIL;
    }
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_add_m2u_deny_table(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_add_m2u_deny_table_stru *pst_m2u_deny_table_param = OAL_PTR_NULL;
    hmac_vap_stru *pst_hmac_vap = OAL_PTR_NULL;
    oal_uint32  ul_deny_group_addr1;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG2(0, OAM_SF_M2U, "{wal_add_m2u_deny_table:: pst_mac_vap/puc_param is null ptr %d, %d!}",
            pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_m2u_deny_table_param = (mac_add_m2u_deny_table_stru *)puc_param;

    ul_deny_group_addr1 = oal_byteorder_host_to_net_uint32(pst_m2u_deny_table_param->ul_deny_group_addr);
    if (ul_deny_group_addr1 >= HMAC_M2U_MIN_DENY_GROUP && ul_deny_group_addr1 <= HMAC_M2U_MAX_DENY_GROUP) {
        hmac_m2u_add_snoop_deny_entry(pst_hmac_vap, &ul_deny_group_addr1);
    } else {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_M2U,
            "{wal_add_m2u_deny_table::pst_m2u_deny_table_param->ul_deny_group_addr %x!}",
            pst_m2u_deny_table_param->ul_deny_group_addr);
        return OAL_FAIL;
    }
    return OAL_SUCC;
}


OAL_STATIC uint32_t wal_config_m2u_deny_table(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_clg_m2u_deny_table_stru *pst_clg_m2u_deny_table_param = OAL_PTR_NULL;
    hmac_vap_stru *pst_hmac_vap = OAL_PTR_NULL;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG2(0, OAM_SF_M2U, "{wal_config_m2u_deny_table:: pst_mac_vap/puc_param is null ptr %d, %d!}",
            pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_clg_m2u_deny_table_param = (mac_clg_m2u_deny_table_stru *)puc_param;

    /* 清空组播组黑名单 */
    if (pst_clg_m2u_deny_table_param->uc_m2u_clear_deny_table == 1) {
        hmac_m2u_clear_deny_table(pst_hmac_vap);
    }
    /* show组播组黑名单 */
    if (pst_clg_m2u_deny_table_param->uc_m2u_show_deny_table == 1) {
        hmac_m2u_show_snoop_deny_table(pst_hmac_vap);
    }
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_show_m2u_snoop_table(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_show_m2u_snoop_table_stru *pst_show_m2u_snoop_table_param = OAL_PTR_NULL;
    hmac_vap_stru *pst_hmac_vap = OAL_PTR_NULL;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG2(0, OAM_SF_M2U, "{wal_show_m2u_snoop_table:: pst_mac_vap/puc_param is null ptr %d, %d!}",
            pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_show_m2u_snoop_table_param = (mac_show_m2u_snoop_table_stru *)puc_param;

    /* show snoop 表 */
    if (pst_show_m2u_snoop_table_param->uc_m2u_show_snoop_table == 1) {
        hmac_m2u_print_all_snoop_list(pst_hmac_vap);
    } else {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_M2U,
            "{wal_show_m2u_snoop_table::pst_m2u_snoop_on_param->uc_m2u_snoop_on %d!}",
            pst_show_m2u_snoop_table_param->uc_m2u_show_snoop_table);
        return OAL_FAIL;
    }
    return OAL_SUCC;
}


OAL_STATIC uint32_t wal_config_igmp_packet_xmit(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    mac_cfg_mpdu_ampdu_tx_param_stru *pst_ampdu_tx_on_param = OAL_PTR_NULL;
    oal_uint8                       uc_skb_num;
    oal_uint8                       uc_skb_idx;
    oal_uint8                       uc_tid;
    oal_uint16                      us_packet_len;
    oal_net_device_stru            *pst_dev = OAL_PTR_NULL;
    oal_netbuf_stru                *past_netbuf[32] = {OAL_PTR_NULL}; // 32:包个数，无其它地方使用

    pst_ampdu_tx_on_param = (mac_cfg_mpdu_ampdu_tx_param_stru *)puc_param;
    uc_skb_num            = pst_ampdu_tx_on_param->uc_packet_num;
    uc_tid                = pst_ampdu_tx_on_param->uc_tid;
    us_packet_len         = pst_ampdu_tx_on_param->us_packet_len;

    pst_dev = hmac_vap_get_net_device(pst_mac_vap->uc_vap_id);

    if (uc_skb_num <= 32) {
        for (uc_skb_idx = 0; uc_skb_idx < uc_skb_num; uc_skb_idx++) {
            past_netbuf[uc_skb_idx] = wal_config_create_igmp_packet(us_packet_len, uc_tid,
                pst_ampdu_tx_on_param->auc_ra_mac,
                pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);
            if (past_netbuf[uc_skb_idx] == OAL_PTR_NULL) {
                return OAL_FAIL;
            }
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
                wal_vap_start_xmit(past_netbuf[uc_skb_idx], pst_dev);
#else
                wal_bridge_vap_xmit(past_netbuf[uc_skb_idx], pst_dev);
#endif
        }
    }
    return OAL_SUCC;
}
#endif


#ifdef _PRE_WLAN_FEATURE_PM
OAL_STATIC oal_uint32 wal_config_wifi_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32    ul_ret;

    if (OAL_UNLIKELY((pst_mac_vap == OAL_PTR_NULL) || (puc_param == OAL_PTR_NULL))) {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_config_wifi_enable::pst_mac_vap or puc_param null ptr error [%x],[%x].}",
            pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_wifi_enable(pst_mac_vap, us_len, puc_param);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_wifi_enable:: fail ret: [%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_config_pm_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_PM_INFO, us_len, puc_param);
}

OAL_STATIC oal_uint32 wal_config_pm_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_PM_EN, us_len, puc_param);
}

#endif


#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32 wal_config_show_device_memleak(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_DEVICE_MEM_LEAK, us_len, puc_param);
}

OAL_STATIC oal_uint32 wal_config_show_device_meminfo(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_DEVICE_MEM_INFO, us_len, puc_param);
}
#endif


OAL_STATIC uint32_t wal_config_remain_on_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_remain_on_channel(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_cancel_remain_on_channel(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_cancel_remain_on_channel(pst_mac_vap, us_len, puc_param);
}


#ifdef _PRE_WLAN_FEATURE_STA_PM
OAL_STATIC uint32_t wal_config_set_sta_pm_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    if (OAL_UNLIKELY((pst_mac_vap == OAL_PTR_NULL) || (puc_param == OAL_PTR_NULL))) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    return hmac_config_set_sta_pm_mode(pst_mac_vap, us_len, puc_param);
}


#ifdef _PRE_PSM_DEBUG_MODE
OAL_STATIC oal_uint32 wal_config_show_pm_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_SHOW_PS_INFO, us_len, puc_param);
}
#endif

OAL_STATIC oal_uint32 wal_config_set_pm_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_SET_PSM_PARAM, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_sta_pm_on(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    hmac_vap_stru                *pst_hmac_vap = OAL_PTR_NULL;
    mac_cfg_ps_open_stru         *pst_sta_pm_open = OAL_PTR_NULL;

    if (OAL_UNLIKELY((pst_mac_vap == OAL_PTR_NULL) || (puc_param == OAL_PTR_NULL))) {
        OAM_WARNING_LOG0(0, OAM_SF_PWR, "{wal_config_set_sta_pm_on::pst_mac_vap / puc_param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap    = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_PWR,
            "{wal_config_set_sta_pm_on::pst_hmac_vap null,vap state[%d].}", pst_mac_vap->en_vap_state);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_sta_pm_open = (mac_cfg_ps_open_stru *)puc_param;

    /* 如果上层主动dhcp成功此时取消超时开低功耗的定时器 */
    if ((pst_hmac_vap->st_ps_sw_timer.en_is_registerd == OAL_TRUE) &&
        (pst_sta_pm_open->uc_pm_enable > MAC_STA_PM_SWITCH_OFF)) {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_vap->st_ps_sw_timer));
    }

    return hmac_config_set_sta_pm_on(pst_mac_vap, us_len, puc_param);
}
#endif


#ifdef _PRE_WLAN_FEATURE_STA_UAPSD
OAL_STATIC uint32_t wal_config_set_uapsd_para(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    if (OAL_UNLIKELY((pst_mac_vap == OAL_PTR_NULL) || (puc_param == OAL_PTR_NULL))) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    return hmac_config_set_uapsd_para(pst_mac_vap, us_len, puc_param);
}
#endif


OAL_STATIC oal_uint32 wal_config_mgmt_tx(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_wpas_mgmt_tx(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_vap_classify_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_vap_classify_en(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_vap_classify_tid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_vap_classify_tid(pst_mac_vap, us_len, puc_param);
}
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32 wal_config_scan_test(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_scan_test(pst_mac_vap, us_len, puc_param);
}
#endif


OAL_STATIC oal_uint32 wal_config_bgscan_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_bgscan_enable(pst_mac_vap, us_len, puc_param);
}

#ifdef _PRE_WLAN_CFGID_DEBUG

OAL_STATIC oal_uint32 wal_config_mcs_set_check_enable(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_mcs_set_check_enable(pst_mac_vap, us_len, puc_param);
}
#endif


#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
OAL_STATIC uint32_t wal_config_query_station_stats(mac_vap_stru *pst_mac_vap, uint16_t us_len, const uint8_t *puc_param)
{
    return hmac_config_query_station_info(pst_mac_vap, us_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_FEATURE_HS20

OAL_STATIC oal_uint32 wal_config_set_qos_map(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_qos_map(pst_mac_vap, us_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_FEATURE_P2P

OAL_STATIC uint32_t wal_config_set_p2p_ps_ops(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_p2p_ps_ops(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_p2p_ps_noa(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_p2p_ps_noa(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_p2p_ps_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_p2p_ps_stat(pst_mac_vap, us_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_PROFLING_MIPS

OAL_STATIC oal_uint32 wal_config_set_mips(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_mips(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_show_mips(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_show_mips(pst_mac_vap, us_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD

OAL_STATIC uint32_t wal_config_enable_arp_offload(mac_vap_stru *pst_mac_vap, uint16_t us_len, const uint8_t *puc_param)
{
    return hmac_config_enable_arp_offload(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_show_arpoffload_info(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_show_arpoffload_info(pst_mac_vap, us_len, puc_param);
}
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32 wal_config_cfg_vap_h2d(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_cfg_vap_h2d(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_host_dev_init(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_host_dev_init(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_host_dev_exit(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_host_dev_exit(pst_mac_vap);
}


oal_uint32 wal_send_cali_data(oal_net_device_stru *pst_net_dev)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)

    mac_vap_stru    *mac_vap = OAL_PTR_NULL;
    host_cali_stru  *cali_data = OAL_PTR_NULL;
    mac_device_stru *mac_device = OAL_PTR_NULL;

    mac_device = mac_res_get_dev(0);
    if (mac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_vap::pst_mac_device is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(mac_device->uc_cfg_vap_id);
    cali_data = (host_cali_stru *)get_cali_data_buf_addr();
    hmac_send_cali_data(mac_vap, cali_data->cali_len, cali_data->cali_data);
#endif
    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_TCP_OPT

OAL_STATIC oal_uint32 wal_config_get_tcp_ack_stream_info(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_get_tcp_ack_stream_info(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_tx_tcp_ack_opt_enable(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_tx_tcp_ack_opt_enable(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_rx_tcp_ack_opt_enable(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_rx_tcp_ack_opt_enable(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_tx_tcp_ack_limit(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_tx_tcp_ack_limit(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_rx_tcp_ack_limit(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_rx_tcp_ack_limit(pst_mac_vap, us_len, puc_param);
}
#endif

#if ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 35))) || \
    (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#ifdef _PRE_WLAN_DFT_STAT

OAL_STATIC oal_uint32 wal_config_set_performance_log_switch(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_performance_log_switch(pst_mac_vap,
        WLAN_CFGID_SET_PERFORMANCE_LOG_SWITCH, us_len, puc_param);
}
#endif
#endif


OAL_STATIC oal_uint32 wal_config_set_max_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_max_user;

    ul_max_user = *((oal_uint32 *)puc_param);
    if ((IS_P2P_GO(pst_mac_vap) && (ul_max_user > WLAN_P2P_GO_ASSOC_USER_MAX_NUM_SPEC)) ||
        (ul_max_user > WLAN_ASSOC_USER_MAX_NUM_SPEC)) {
        ul_max_user = WLAN_ASSOC_USER_MAX_NUM_SPEC;
    }
    return hmac_config_set_max_user(pst_mac_vap, us_len, ul_max_user);
}

#ifdef _PRE_WLAN_FEATURE_ROAM

OAL_STATIC oal_uint32 wal_config_roam_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_roam_enable(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_roam_band(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_roam_band(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_roam_org(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_roam_org(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_roam_start(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_roam_start(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_roam_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_roam_info(pst_mac_vap, us_len, puc_param);
}
#endif // _PRE_WLAN_FEATURE_ROAM

#ifdef _PRE_WLAN_FEATURE_11R

OAL_STATIC oal_uint32 wal_config_set_ft_ies(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_ft_ies(pst_mac_vap, us_len, puc_param);
}
#endif // _PRE_WLAN_FEATURE_11R

#ifdef _PRE_WLAN_FEATURE_SAE
/* 执行SAE关联 */
OAL_STATIC uint32_t wal_config_external_auth(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param)
{
    return hmac_config_external_auth(mac_vap, len, param);
}
#endif /* _PRE_WLAN_FEATURE_SAE */

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST

OAL_STATIC uint32_t wal_config_enable_2040bss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_enable_2040bss(pst_mac_vap, us_len, puc_param);
}
#endif /* _PRE_WLAN_FEATURE_20_40_80_COEXIST */


#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
OAL_STATIC oal_uint32 wal_config_set_auto_freq_enable(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_auto_freq_enable(pst_mac_vap, us_len, puc_param);
}
#endif
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE

OAL_STATIC oal_uint32 wal_config_get_lauch_cap(mac_vap_stru *pst_mac_vap, oal_uint16 *us_len, oal_uint8 *puc_param)
{
#if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1131C_HOST)
    return hmac_config_get_lauch_cap(pst_mac_vap, us_len, puc_param);
#else
    return OAL_SUCC;
#endif
}


OAL_STATIC oal_uint32 wal_config_set_linkloss_threshold(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_linkloss_threshold(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_all_log_level(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_set_all_log_level(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_d2h_hcc_assemble_cnt(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_d2h_hcc_assemble_cnt(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_chn_est_ctrl(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_set_chn_est_ctrl(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_power_ref(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_power_ref(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_pm_cfg_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    return hmac_config_set_pm_cfg_param(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_cus_rf(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_cus_rf(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_set_cus_dts_cali(mac_vap_stru *pst_mac_vap, uint16_t us_len, const uint8_t *puc_param)
{
    return hmac_config_set_cus_dts_cali(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_cus_nvram_params(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_cus_nvram_params(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC uint32_t wal_config_dev_customize_info(mac_vap_stru *pst_mac_vap, uint16_t us_len, const uint8_t *puc_param)
{
    return hmac_config_dev_customize_info(pst_mac_vap, us_len, puc_param);
}
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */



OAL_STATIC oal_uint32 wal_config_vap_destroy(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    hmac_vap_stru *pst_hmac_vap;
    pst_hmac_vap  = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    return hmac_vap_destroy(pst_hmac_vap);
}

#ifdef _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN

OAL_STATIC oal_uint32 wal_config_set_tx_classify_switch(mac_vap_stru *pst_mac_vap,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_set_tx_classify_switch(pst_mac_vap, us_len, puc_param);
}
#endif  /* _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN */

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifndef CONFIG_HAS_EARLYSUSPEND

OAL_STATIC uint32_t wal_config_set_suspend_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    mac_device_stru *pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "wal_config_set_suspend_mode:pst_mac_device is null ptr!");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 0:亮屏 1:暗屏 */
    if (*puc_param == 0) {
        hmac_do_suspend_action(pst_mac_device, OAL_FALSE);
    } else {
        hmac_do_suspend_action(pst_mac_device, OAL_TRUE);
    }

    return OAL_SUCC;
}
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_IP_FILTER
/* 配置rx ip数据包过滤的参数 */
OAL_STATIC uint32_t wal_config_update_ip_filter(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *puc_param)
{
    return hmac_config_update_ip_filter(mac_vap, len, puc_param);
}


static uint32_t wal_config_assigned_filter(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param)
{
    return hmac_config_assigned_filter(mac_vap, len, param);
}
#endif

#ifdef _PRE_WLAN_FEATURE_APF
/*
 * 功能描述: wal下发apf命令
 * 修改历史:
 * 1.日    期: 2020年3月10日
 *   修改内容: 新生成函数
 */
OAL_STATIC uint32_t wal_config_apf_filter_cmd(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *puc_param)
{
    return hmac_config_apf_filter_cmd(mac_vap, len, puc_param);
}
#endif


OAL_STATIC uint32_t wal_config_set_high_power_switch(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param)
{
    uint32_t  ret;

    if (mac_vap == NULL || param == NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_high_power_switch::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ret = hmac_config_send_sub_event(mac_vap, len, param);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG0(mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_config_set_high_power_switch::hmac_config_send_sub_event error!}\r\n");
        return ret;
    }

    return ret;
}

OAL_STATIC uint32_t wal_config_forbit_open_auth(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param)
{
    return hmac_config_forbit_open_auth(mac_vap, len, param);
}

OAL_STATIC uint32_t wal_config_set_auth_rsp_time(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param)
{
    return hmac_config_set_auth_rsp_time(mac_vap, len, param);
}

#ifdef _PRE_WLAN_FEATURE_1131K_BTCOEX
OAL_STATIC uint32_t wal_config_set_btcoex_channel_switch(mac_vap_stru *mac_vap,
                                                         uint16_t len, const uint8_t *param)
{
    uint32_t  ret;

    if (mac_vap == NULL || param == NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_set_btcoex_channel_switch::pst_mac_vap/puc_param is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ret = hmac_config_send_sub_event(mac_vap, len, param);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG0(mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_config_set_btcoex_channel_switch::hmac_config_send_sub_event error!}\r\n");
        return ret;
    }

    return ret;
}
#endif

OAL_STATIC uint32_t wal_rx_filter_frag(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param)
{
    uint32_t  ret;

    if (mac_vap == NULL || param == NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_rx_filter_frag::pst_mac_vap/puc_param is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ret = hmac_config_send_sub_event(mac_vap, len, param);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG0(mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_rx_filter_frag::hmac_config_send_sub_event error!}\r\n");
        return ret;
    }

    return ret;
}

OAL_STATIC uint32_t wal_get_psm_info(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param)
{
    uint32_t  ret;

    if (mac_vap == NULL || param == NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_get_psm_info::pst_mac_vap/puc_param is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ret = hmac_config_send_sub_event(mac_vap, len, param);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG0(mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_get_psm_info::hmac_config_send_sub_event error!}\r\n");
        return ret;
    }

    return ret;
}

OAL_STATIC uint32_t wal_get_psm_bcn_info(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param)
{
    uint32_t  ret;

    if (mac_vap == NULL || param == NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_get_psm_bcn_info::pst_mac_vap/puc_param is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ret = hmac_config_send_sub_event(mac_vap, len, param);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG0(mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_get_psm_bcn_info:hmac_config_send_sub_event error}\r\n");
        return ret;
    }

    return ret;
}
/*lint -e19*/
oal_module_symbol(wal_drv_cfg_func_hook_init);
/*lint +e19*/

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

