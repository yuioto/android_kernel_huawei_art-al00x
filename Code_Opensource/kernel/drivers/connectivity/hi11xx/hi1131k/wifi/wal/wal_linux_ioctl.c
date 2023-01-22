

#include "wal_linux_ioctl.h"
#include "oal_ext_if.h"
#include "oal_profiling.h"
#include "oal_cfg80211.h"
#include "oal_atomic.h"

#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT
#include "oal_kernel_file.h"
#endif

#include "oam_ext_if.h"
#include "frw_ext_if.h"
#include "wlan_types.h"

#include "mac_vap.h"
#include "mac_resource.h"
#include "mac_regdomain.h"
#include "mac_ie.h"

#include "hmac_ext_if.h"
#include "hmac_chan_mgmt.h"

#include "wal_main.h"
#include "wal_ext_if.h"
#include "wal_config.h"
#include "wal_regdb.h"
#include "wal_linux_scan.h"
#include "wal_linux_bridge.h"
#include "wal_linux_flowctl.h"
#include "wal_linux_atcmdsrv.h"
#include "wal_linux_event.h"
#include "hmac_resource.h"
#include "hmac_p2p.h"
#include "securec.h"

#ifdef _PRE_WLAN_FEATURE_P2P
#include "wal_linux_cfg80211.h"
#endif

#include "wal_dfx.h"

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#include "oal_hcc_host_if.h"
#include "plat_cali.h"
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include <linux/notifier.h>
#include <linux/inetdevice.h>
#endif
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include <net/addrconf.h>
#endif
#include "hmac_arp_offload.h"
#include "oal_net.h"
#endif

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
#include "hmac_auto_adjust_freq.h"
#endif

#ifdef _PRE_WLAN_FEATURE_ROAM
#include "hmac_roam_main.h"
#endif // _PRE_WLAN_FEATURE_ROAM
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#include "hisi_customize_wifi.h"
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */
#if (((_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)) || \
    (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION))
#include "plat_pm_wlan.h"
#endif

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#include "lwip/netifapi.h"
#include "driver_hisi_common.h"
#endif
#include "plat_firmware.h"
#include "plat_debug.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_LINUX_IOCTL_C
#define MAX_PRIV_CMD_SIZE 4096

#ifdef _PRE_WLAN_FEATURE_IP_FILTER
#define MAX_HIPRIV_IP_FILTER_BTABLE_SIZE 129
#define WLAN_RX_FILTER_CNT        20 // ���������rxָ�����˵ı���������
#endif

#ifndef WIN32
extern oal_uint8 g_auc_mac_addr[];
#endif

typedef struct {
    oal_int8 *pc_country;                    /* �����ַ��� */
    mac_dfs_domain_enum_uint8 en_dfs_domain; /* DFS �״��׼ */
} wal_dfs_domain_entry_stru;

typedef struct {
    oal_uint32 ul_ap_max_user;           /* ap����û��� */
    oal_int8 ac_ap_mac_filter_mode[257]; /* AP mac��ַ�����������,�256 */
    oal_int32 l_ap_power_flag;           /* AP�ϵ��־ */
} wal_ap_config_stru;

// ȫ�ֱ�������
extern oal_uint32 g_auc_timer_debug[];
extern oal_uint32 g_ul_skb_data_alloc_fail_count;

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
unsigned char            g_auc_mac_addr[ETH_ALEN] = {0};
#endif
OAL_STATIC oal_proc_dir_entry_stru *g_pst_proc_entry;

OAL_STATIC wal_ap_config_stru g_st_ap_config_info = { 0 }; /* AP������Ϣ,��Ҫ��vap �������·��� */

#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
/* Just For UT */
OAL_STATIC oal_uint32 g_wal_wid_queue_init_flag = OAL_FALSE;
#endif
OAL_STATIC wal_msg_queue g_wal_wid_msg_queue;

/* hi1102-cb add sys for 51/02 */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && \
    ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION))
struct kobject *gp_sys_kobject;
oal_uint8 g_uc_custom_cali_done = OAL_FALSE;
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_int32 wal_hipriv_inetaddr_notifier_call(struct notifier_block *this, oal_ulong event, oal_void *ptr);

OAL_STATIC struct notifier_block wal_hipriv_notifier = {
    .notifier_call = wal_hipriv_inetaddr_notifier_call
};
#ifdef CONFIG_IPV6
oal_int32 wal_hipriv_inet6addr_notifier_call(struct notifier_block *this, oal_ulong event, oal_void *ptr);

OAL_STATIC struct notifier_block wal_hipriv_notifier_ipv6 = {
    .notifier_call = wal_hipriv_inet6addr_notifier_call
};
#endif
#endif
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
oal_int32 wal_netif_notify(oal_net_device_stru *pst_net_dev, oal_net_notify_stru *pst_notify);

/* Liteos �� wal_ioctl_set_max_user/wal_set_ap_max_user/wal_ioctl_set_ap_config ������ӿ� */
oal_int32 wal_init_max_sta_num(oal_net_device_stru *pst_net_dev, oal_uint32 ul_max_num);
#endif
#endif

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
/* ÿ�����µ�������vap��ɵĶ��ƻ�ֻ����һ�Σ�wlan p2p iface�����ظ����� */
OAL_STATIC oal_uint8 g_uc_cfg_once_flag = OAL_FALSE;
/* ֻ��staut��aput��һ���ϵ�ʱ��ini�����ļ��ж�ȡ���� */
OAL_STATIC oal_uint8 g_uc_cfg_flag = OAL_FALSE;
extern host_speed_freq_level_stru g_host_speed_freq_level[4];
extern device_speed_freq_level_stru g_device_speed_freq_level[4];
extern oal_uint32 band_5g_enabled;
extern oal_uint32 hcc_assemble_count;
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION) && (_PRE_TEST_MODE == _PRE_TEST_MODE_UT)
/* UTģʽ�µ���frw_event_process_all_event */
extern oal_void frw_event_process_all_event(oal_ulong ui_data);
#endif

#ifdef _PRE_WLAN_FEATURE_DFR
extern wal_dfr_info_stru g_st_dfr_info;
#endif // _PRE_WLAN_FEATURE_DFR

/* ��̬�������� */
OAL_STATIC oal_uint32 wal_hipriv_global_log_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_vap_log_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_vap_log_level(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_log_ratelimit(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32 wal_hipriv_log_lowpower(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_pm_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifndef CONFIG_HAS_EARLYSUSPEND
OAL_STATIC oal_int32 wal_ioctl_set_suspend_mode(oal_net_device_stru *pst_net_dev, oal_uint8 uc_suspend);
#endif
#endif
OAL_STATIC oal_uint32 wal_hipriv_set_ucast_data_dscr_param(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#ifdef _PRE_WLAN_FEATURE_WEB_CFG_FIXED_RATE
OAL_STATIC oal_uint32 wal_hipriv_set_mode_ucast_data_dscr_param(oal_net_device_stru *pst_net_dev,
    const oal_int8 *pc_param);
#endif
OAL_STATIC oal_uint32 wal_hipriv_set_mcast_data_dscr_param(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_bcast_data_dscr_param(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_ucast_mgmt_dscr_param(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_mbcast_mgmt_dscr_param(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);

OAL_STATIC oal_uint32 wal_hipriv_setcountry(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_getcountry(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);

#ifdef _PRE_WLAN_FEATURE_11D
OAL_STATIC oal_uint32 wal_hipriv_set_rd_by_ie_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif
oal_uint32 wal_hipriv_set_bw(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);

#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
oal_uint32 wal_hipriv_always_tx_1102(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif

oal_uint32 wal_hipriv_always_rx(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32 wal_hipriv_dync_txpower(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif
OAL_STATIC oal_uint32 wal_hipriv_get_thruput(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_user_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_add_vap(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_event_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);

#ifdef _PRE_WLAN_RF_110X_CALI_DPD
OAL_STATIC oal_uint32 wal_hipriv_start_dpd(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif
#ifdef _PRE_WLAN_CHIP_TEST
OAL_STATIC oal_uint32 wal_hipriv_beacon_offload_test(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif
OAL_STATIC oal_uint32 wal_hipriv_ota_beacon_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_ota_rx_dscr_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);

#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32 wal_hipriv_set_debug_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_get_tx_comp_cnt(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_get_all_reg_value(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#ifdef _PRE_WLAN_DFT_EVENT
OAL_STATIC oal_void wal_event_report_to_sdt(wal_msg_type_enum_uint8 en_msg_type, oal_uint8 *puc_param,
    wal_msg_stru *pst_cfg_msg);
#endif
#endif
OAL_STATIC oal_uint32 wal_hipriv_set_ether_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_80211_ucast_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);

#ifdef _PRE_DEBUG_MODE_USER_TRACK
OAL_STATIC oal_uint32 wal_hipriv_report_thrput_stat(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);

#endif
#ifdef _PRE_WLAN_FEATURE_TXOPPS
OAL_STATIC oal_uint32 wal_hipriv_set_txop_ps_machw(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_BTCOEX
OAL_STATIC oal_uint32 wal_hipriv_btcoex_status_print(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif

OAL_STATIC oal_uint32 wal_hipriv_set_80211_mcast_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_all_80211_ucast(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_all_ether_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_dhcp_arp_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32 wal_hipriv_report_vap_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_aifsn_cfg(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_cw_cfg(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif
OAL_STATIC oal_uint32 wal_hipriv_set_rssi_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);

OAL_STATIC oal_uint32 wal_hipriv_set_probe_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_get_mpdu_num(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_all_ota(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_oam_output(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_add_user(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_del_user(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_ampdu_start(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_auto_ba_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_addba_req(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_delba_req(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_mem_info(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_mem_leak(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32 wal_hipriv_device_mem_leak(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_memory_info(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);
#endif

OAL_STATIC uint32_t wal_hipriv_reg_info(oal_net_device_stru *net_dev, const int8_t *param);

#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST))
OAL_STATIC oal_uint32 wal_hipriv_sdio_flowctrl(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_HILINK
OAL_STATIC oal_uint32 wal_hipriv_set_monitor_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif
OAL_STATIC oal_uint32 wal_hipriv_set_mips_cycle_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_2040_channel_switch_prohibited(oal_net_device_stru *pst_net_dev,
    const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_FortyMHzIntolerant(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_2040_coext_support(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_rx_fcs_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
OAL_STATIC oal_uint32 wal_hipriv_dev_customize_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */
oal_int32 wal_netdev_open(oal_net_device_stru *pst_net_dev);
OAL_STATIC oal_int32 wal_net_device_ioctl(oal_net_device_stru *pst_net_dev, oal_ifreq_stru *pst_ifr, oal_int32 ul_cmd);
#if ((_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1131C_DEV) && (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1131C_HOST))
OAL_STATIC oal_int32 wal_ioctl_set_auth_mode(oal_net_device_stru *pst_net_dev,
    oal_net_dev_ioctl_data_stru *pst_ioctl_data);
OAL_STATIC oal_int32 wal_ioctl_set_country_code(oal_net_device_stru *pst_net_dev,
    oal_net_dev_ioctl_data_stru *pst_ioctl_data);
OAL_STATIC oal_int32 wal_ioctl_set_max_user(oal_net_device_stru *pst_net_dev,
    oal_net_dev_ioctl_data_stru *pst_ioctl_data);
OAL_STATIC oal_int32 wal_ioctl_nl80211_priv_connect(oal_net_device_stru *pst_net_dev,
    oal_net_dev_ioctl_data_stru *pst_ioctl_data);
OAL_STATIC oal_int32 wal_ioctl_nl80211_priv_disconnect(oal_net_device_stru *pst_net_dev,
    oal_net_dev_ioctl_data_stru *pst_ioctl_data);
OAL_STATIC oal_int32 wal_ioctl_set_channel(oal_net_device_stru *pst_net_dev,
    oal_net_dev_ioctl_data_stru *pst_ioctl_data);
OAL_STATIC oal_int32 wal_ioctl_set_wps_ie(oal_net_device_stru *pst_net_dev,
    oal_net_dev_ioctl_data_stru *pst_ioctl_data);
OAL_STATIC oal_int32 wal_ioctl_set_frag(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data);
OAL_STATIC oal_int32 wal_ioctl_set_rts(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data);
oal_int32 wal_ioctl_set_ssid(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data);
#endif
OAL_STATIC oal_net_device_stats_stru* wal_netdev_get_stats(oal_net_device_stru *pst_net_dev);
OAL_STATIC oal_int32 wal_netdev_set_mac_addr(oal_net_device_stru *pst_net_dev, void *p_addr);

OAL_STATIC oal_uint32 wal_hipriv_start_vap(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_beacon_interval(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_txpower(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
oal_uint32 wal_hipriv_set_mode(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_essid(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
oal_uint32 wal_hipriv_set_freq(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC int wal_ioctl_get_essid(oal_net_device_stru *pst_net_dev,
                                   oal_iw_request_info_stru *pst_info,
                                   oal_iwreq_data_union *pst_wrqu,
                                   char *pc_param);
OAL_STATIC int wal_ioctl_get_apaddr(oal_net_device_stru *pst_net_dev,
                                    oal_iw_request_info_stru *pst_info,
                                    oal_iwreq_data_union *pst_wrqu,
                                    char *pc_extra);
OAL_STATIC int wal_ioctl_get_iwname(oal_net_device_stru *pst_net_dev,
                                    oal_iw_request_info_stru *pst_info,
                                    oal_iwreq_data_union *pst_wrqu,
                                    char *pc_extra);
OAL_STATIC oal_uint32 wal_hipriv_amsdu_start(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_list_ap(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_list_sta(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_list_channel(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_regdomain_pwr(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_regdomain_pwr_priv(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_start_scan(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_start_join(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_start_deauth(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_dump_timer(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_kick_user(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_pause_tid(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_user_vip(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_vap_host(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_ampdu_tx_on(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_send_bar(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
oal_uint32 wal_hipriv_reg_write(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_dump_ba_bitmap(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_show_stat_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_clear_stat_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_user_stat_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_show_vap_pkt_stat(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_alg_cfg(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_timer_start(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_ampdu_amsdu_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_reset_device(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_reset_operate(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_amsdu_tx_on(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
OAL_STATIC oal_uint32 wal_hipriv_set_oma(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_proxysta_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_UAPSD
OAL_STATIC oal_uint32 wal_hipriv_uapsd_debug(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);
#endif
#ifdef _PRE_WLAN_DFT_STAT
OAL_STATIC oal_uint32 wal_hipriv_set_phy_stat_en(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_dbb_env_param(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_usr_queue_stat(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_report_vap_stat(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_report_all_stat(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
OAL_STATIC oal_uint32 wal_hipriv_get_hipkt_stat(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_flowctl_param(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_get_flowctl_stat(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif

#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32 wal_hipriv_report_ampdu_stat(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif

OAL_STATIC oal_uint32 wal_hipriv_set_ampdu_aggr_num(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32 wal_hipriv_freq_adjust(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif

OAL_STATIC oal_uint32 wal_hipriv_set_stbc_cap(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_ldpc_cap(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_tpc_log(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_cca_opt_log(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_ar_log(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_ar_test(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_dump_rx_dscr(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_dump_tx_dscr(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_dump_memory(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_show_tx_dscr_addr(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_alg(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_event_queue_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_dump_all_rx_dscr(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32 wal_hipriv_resume_rx_intr_fifo(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif
#ifdef _PRE_WLAN_CHIP_TEST
OAL_STATIC oal_uint32 wal_hipriv_send_pspoll(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_send_nulldata(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */

OAL_STATIC oal_uint32 wal_hipriv_frag_threshold(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_wmm_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_hide_ssid(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);

#ifdef _PRE_WLAN_PERFORM_STAT
OAL_STATIC oal_uint32 wal_hipriv_stat_tid_thrpt(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_stat_user_thrpt(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_stat_vap_thrpt(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_stat_tid_per(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_stat_tid_delay(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);

OAL_STATIC oal_uint32 wal_hipriv_display_tid_thrpt(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_display_user_thrpt(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_display_vap_thrpt(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_display_tid_per(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_display_tid_delay(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
OAL_STATIC oal_uint32 wal_hipriv_set_edca_opt_switch_sta(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_edca_opt_weight_sta(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_edca_opt_switch_ap(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_edca_opt_cycle_ap(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif

#ifdef _PRE_WLAN_CHIP_TEST
OAL_STATIC oal_uint32 wal_hipriv_lpm_soc_mode(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_lpm_chip_state(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_lpm_psm_param(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_lpm_smps_mode(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_lpm_smps_stub(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_lpm_txopps_set(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_lpm_txopps_tx_stub(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_lpm_wow_en(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);

OAL_STATIC oal_uint32 wal_hipriv_remove_user_lut(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_lpm_tx_data(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_lpm_tx_probe_request(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);

OAL_STATIC oal_uint32 wal_hipriv_send_frame(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_rx_pn(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_soft_retry(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_open_addr4(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_open_wmm_test(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_chip_test_open(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_coex(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_dfx(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
OAL_STATIC oal_uint32 wal_hipriv_enable_pmf(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif
OAL_STATIC oal_uint32 wal_hipriv_clear_all_stat(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_test_send_action(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */

#ifdef _PRE_WLAN_FEATURE_DFR
OAL_STATIC oal_uint32 wal_hipriv_test_dfr_start(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif // _PRE_WLAN_FEATURE_DFR
OAL_STATIC oal_uint32 wal_hipriv_set_mib(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_get_mib(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_thruput_bypass(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
OAL_STATIC oal_uint32 wal_hipriv_set_auto_freq(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif
OAL_STATIC oal_uint32 wal_hipriv_set_auto_protection(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);

OAL_STATIC oal_uint32 wal_hipriv_send_2040_coext(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_2040_coext_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_get_version(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#ifdef _PRE_WLAN_FEATURE_DAQ
OAL_STATIC oal_uint32 wal_hipriv_data_acq(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_UAPSD
OAL_STATIC oal_uint32 wal_hipriv_set_uapsd_cap(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_SMPS
#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32 wal_hipriv_get_smps_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_DFS
OAL_STATIC oal_uint32 wal_hipriv_dfs_radartool(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif
#ifdef _PRE_SUPPORT_ACS
OAL_STATIC oal_uint32 wal_hipriv_acs(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
OAL_STATIC oal_uint32 wal_hipriv_set_opmode_notify(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_get_user_nssbw(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);

#endif

#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32 wal_hipriv_set_rx_filter_val(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_get_rx_filter_val(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_set_rx_filter_en(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_get_rx_filter_en(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY

/* {"blacklist_add",           wal_hipriv_blacklist_add},           1 */
OAL_STATIC oal_uint32 wal_hipriv_blacklist_add(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
/* {"blacklist_del",           wal_hipriv_blacklist_del},           2 */
OAL_STATIC oal_uint32 wal_hipriv_blacklist_del(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
/* {"blacklist_mode",          wal_hipriv_set_blacklist_mode},      3 */
OAL_STATIC oal_uint32 wal_hipriv_set_blacklist_mode(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
/* {"blacklist_show",          wal_hipriv_blacklist_show},          4 wal_config_blacklist_show */
OAL_STATIC oal_uint32 wal_hipriv_blacklist_show(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
/* {"abl_on",                  wal_hipriv_set_abl_on},              5 */
OAL_STATIC oal_uint32 wal_hipriv_set_abl_on(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
/* {"abl_aging",               wal_hipriv_set_abl_aging},           6 */
OAL_STATIC oal_uint32 wal_hipriv_set_abl_aging(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
/* {"abl_threshold",           wal_hipriv_set_abl_threshold},       7 */
OAL_STATIC oal_uint32 wal_hipriv_set_abl_threshold(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
/* {"abl_reset",               wal_hipriv_set_abl_reset},           8 wal_config_set_autoblacklist_reset_time */
OAL_STATIC oal_uint32 wal_hipriv_set_abl_reset(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
/* {"isolation_mode",          wal_hipriv_set_isolation_mode},      9 */
OAL_STATIC oal_uint32 wal_hipriv_set_isolation_mode(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
/* {"isolation_type",          wal_hipriv_set_isolation_type},      10 */
OAL_STATIC oal_uint32 wal_hipriv_set_isolation_type(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
/* {"isolation_fwd",           wal_hipriv_set_isolation_fwd},       11 */
OAL_STATIC oal_uint32 wal_hipriv_set_isolation_fwd(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
/* {"isolation_clear",         wal_hipriv_set_isolation_clear},     12 wal_config_set_isolation_clear */
OAL_STATIC oal_uint32 wal_hipriv_set_isolation_clear(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
/* {"isolation_show",          wal_hipriv_set_isolation_show},      13 wal_config_isolation_show */
OAL_STATIC oal_uint32 wal_hipriv_set_isolation_show(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);

#endif
#ifdef _PRE_WLAN_FEATURE_MCAST
OAL_STATIC oal_uint32 wal_hipriv_m2u_snoop_on(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_m2u_add_deny_table(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_m2u_cfg_deny_table(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_m2u_show_snoop_table(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_igmp_packet_xmit(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);

#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
OAL_STATIC oal_uint32 wal_hipriv_proxyarp_on(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32 wal_hipriv_proxyarp_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif /* #ifdef _PRE_DEBUG_MODE */
#endif /* #ifdef _PRE_WLAN_FEATURE_PROXY_ARP */

#ifdef _PRE_WLAN_FEATURE_PM
OAL_STATIC oal_uint32 wal_hipriv_wifi_enable(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_pm_info(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_pm_enable(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);

#endif

#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32 wal_hipriv_scan_test(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
OAL_STATIC uint32_t wal_hipriv_mem_check(const oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif
#endif
OAL_STATIC oal_uint32 wal_hipriv_bgscan_enable(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);
#ifdef _PRE_WLAN_FEATURE_STA_PM
OAL_STATIC oal_uint32 wal_hipriv_sta_ps_mode(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);
#ifdef _PRE_PSM_DEBUG_MODE
OAL_STATIC oal_uint32 wal_hipriv_sta_ps_info(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);
#endif
extern oal_uint32 wal_hipriv_sta_pm_on(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_STA_UAPSD
OAL_STATIC oal_uint32 wal_hipriv_set_uapsd_para(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);
#endif
#if ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)) && \
    (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
/* hi1102-cb add sys for 51/02 */
OAL_STATIC oal_ssize_t wal_hipriv_sys_write(struct kobject *dev, struct kobj_attribute *attr, const char *buf,
    oal_size_t count);
OAL_STATIC oal_ssize_t wal_hipriv_sys_read(struct kobject *dev, struct kobj_attribute *attr, char *buf);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
OAL_STATIC struct kobj_attribute dev_attr_hipriv =
    __ATTR(hipriv, (OAL_S_IRUGO | OAL_S_IWUSR), wal_hipriv_sys_read, wal_hipriv_sys_write);
#endif
/* hi1102-cb add sys for 51/02 */
#endif

#ifdef _PRE_WLAN_FEATURE_P2P
#ifdef _PRE_WLAN_CHIP_TEST
OAL_STATIC oal_uint32 wal_hipriv_set_p2p_ps(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */
OAL_STATIC oal_int32 wal_ioctl_set_p2p_noa(oal_net_device_stru *pst_net_dev,
    mac_cfg_p2p_noa_param_stru *pst_p2p_noa_param);
OAL_STATIC oal_int32 wal_ioctl_reduce_sar(oal_net_device_stru *pst_net_dev, oal_uint8 uc_tx_power);
OAL_STATIC oal_int32 wal_ioctl_set_p2p_ops(oal_net_device_stru *pst_net_dev,
    mac_cfg_p2p_ops_param_stru *pst_p2p_ops_param);

#endif /* _PRE_WLAN_FEATURE_P2P */

#ifdef _PRE_WLAN_FEATURE_WAPI
#ifdef _PRE_WAPI_DEBUG
OAL_STATIC oal_uint32 wal_hipriv_show_wapi_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif /* #ifdef _PRE_WAPI_DEBUG */
#endif

#ifdef _PRE_WLAN_FEATURE_HS20
OAL_STATIC oal_int32 wal_ioctl_set_qos_map(oal_net_device_stru *pst_net_dev,
    hmac_cfg_qos_map_param_stru *pst_qos_map_param);
#endif /* #ifdef _PRE_WLAN_FEATURE_HS20 */

oal_int32 wal_ioctl_set_wps_p2p_ie(oal_net_device_stru *pst_net_dev, const oal_uint8 *puc_buf, oal_uint32 ul_len,
    en_app_ie_type_uint8 en_type);
#ifdef _PRE_WLAN_PROFLING_MIPS
OAL_STATIC oal_uint32 wal_hipriv_set_mips(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_show_mips(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif

OAL_STATIC oal_int32 wal_set_ap_max_user(oal_net_device_stru *pst_net_dev, oal_uint32 ul_ap_max_user);
OAL_STATIC oal_int32 wal_config_mac_filter(oal_net_device_stru *pst_net_dev, oal_int8 *pc_command);
OAL_STATIC oal_int32 wal_kick_sta(oal_net_device_stru *pst_net_dev, const oal_uint8 *auc_mac_addr);
OAL_STATIC int wal_ioctl_set_ap_config(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info,
    oal_iwreq_data_union *pst_wrqu, char *pc_extra);
OAL_STATIC int wal_ioctl_get_assoc_list(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info,
    oal_iwreq_data_union *pst_wrqu, char *pc_extra);
OAL_STATIC int wal_ioctl_set_mac_filters(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info,
    oal_iwreq_data_union *pst_wrqu, char *pc_extra);
OAL_STATIC int wal_ioctl_set_ap_sta_disassoc(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info,
    oal_iwreq_data_union *pst_wrqu, char *pc_extra);
OAL_STATIC oal_uint32 wal_get_parameter_from_cmd(oal_int8 *pc_cmd, oal_int8 *pc_arg, OAL_CONST oal_int8 *puc_token,
    oal_uint32 *pul_cmd_offset, oal_uint32 ul_param_max_len);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32 wal_hipriv_set_ampdu_mmss(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
oal_uint32 wal_hipriv_arp_offload_enable(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif
#ifdef _PRE_WLAN_TCP_OPT
OAL_STATIC oal_uint32 wal_hipriv_get_tcp_ack_stream_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_tcp_tx_ack_opt_enable(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_tcp_rx_ack_opt_enable(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_tcp_tx_ack_limit(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
OAL_STATIC oal_uint32 wal_hipriv_tcp_rx_ack_limit(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);

#endif
#ifdef _PRE_WLAN_DFT_STAT
OAL_STATIC oal_uint32 wal_hipriv_performance_log_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
oal_int32 wal_init_wlan_vap(oal_net_device_stru *pst_net_dev);
oal_int32 wal_deinit_wlan_vap(oal_net_device_stru *pst_net_dev);
oal_int32 wal_start_vap(oal_net_device_stru *pst_net_dev);
oal_int32 wal_stop_vap(oal_net_device_stru *pst_net_dev);
oal_int32 wal_set_mac_addr(oal_net_device_stru *pst_net_dev);
oal_int32 wal_init_wlan_netdev(oal_wiphy_stru *pst_wiphy, const char *dev_name);
oal_int32 wal_setup_ap(oal_net_device_stru *pst_net_dev);
#endif

#ifdef _PRE_WLAN_FEATURE_11D
#ifdef _PRE_WLAN_FEATURE_DFS // 1131_debug
oal_int32 wal_regdomain_update_for_dfs(oal_net_device_stru *pst_net_dev, oal_int8 *pc_country);
#endif
oal_int32 wal_regdomain_update(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_country);
#endif

#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
OAL_STATIC oal_uint32 wal_hipriv_send_cfg_uint32_data(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param,
    wlan_cfgid_enum_uint16 cfgid);
#endif

#ifdef _PRE_WLAN_FEATURE_ROAM
oal_uint32 wal_hipriv_roam_enable(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
oal_uint32 wal_hipriv_roam_band(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
oal_uint32 wal_hipriv_roam_org(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
oal_uint32 wal_hipriv_roam_start(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
oal_uint32 wal_hipriv_roam_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif // _PRE_WLAN_FEATURE_ROAM

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
OAL_STATIC oal_uint32 wal_hipriv_enable_2040bss(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN
OAL_STATIC oal_uint32 wal_hipriv_set_tx_classify_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif /* _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN */
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
oal_uint32 wal_set_ldpc_cap(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param, oal_uint32 ul_len);
#endif
oal_int32 hcc_flowctrl_info_print(oal_void *data, char *buf, oal_int32 buf_len);
OAL_STATIC oal_uint32 wal_hipriv_mcs_set_check_enable(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param);

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
OAL_STATIC int32_t wal_android_priv_set_ssid(oal_net_device_stru *net_dev, const uint8_t *command);
#endif

#ifdef _PRE_WLAN_FEATURE_IP_FILTER
#ifdef CONFIG_DOZE_FILTER
OAL_STATIC int32_t wal_android_priv_set_ip_filter(uint8_t *command, wal_android_wifi_priv_cmd_stru *priv_cmd);
#endif
OAL_STATIC uint32_t wal_hipriv_set_assigned_filter_switch(oal_net_device_stru *net_dev, const int8_t *param);
#endif
OAL_STATIC uint32_t wal_hipriv_set_str_cmd(oal_net_device_stru *net_dev, const int8_t *param);

#ifdef _PRE_WLAN_FEATURE_APF
OAL_STATIC uint32_t wal_hipriv_apf_filter_cmd(oal_net_device_stru *net_dev, const int8_t *param);
#endif
OAL_STATIC uint32_t wal_hipriv_cali_permission_switch(oal_net_device_stru *net_dev, const int8_t *param);
OAL_STATIC uint32_t wal_hipriv_rx_filter_frag(oal_net_device_stru *net_dev, const int8_t *param);

/*
 * ˽���������. ˽�������ʽ:
 *       �豸�� ������ ����
 * hipriv "Hisilicon0 create vap0"
 */
/* Android private command strings */
#define CMD_SET_AP_WPS_P2P_IE "SET_AP_WPS_P2P_IE"
#define CMD_P2P_SET_NOA "P2P_SET_NOA"
#define CMD_P2P_SET_PS "P2P_SET_PS"
#define CMD_SET_POWER_ON "SET_POWER_ON"
#define CMD_SET_POWER_MGMT_ON "SET_POWER_MGMT_ON"
#define CMD_COUNTRY "COUNTRY"
#define CMD_SET_QOS_MAP "SET_QOS_MAP"
#define CMD_TX_POWER "TX_POWER"
#define CMD_WPAS_GET_CUST "WPAS_GET_CUST"
#define CMD_SET_SSID "SET_SSID"
#define CMD_SETSUSPENDOPT "SETSUSPENDOPT"
#define CMD_SETSUSPENDMODE "SETSUSPENDMODE"
#ifdef _PRE_WLAN_FEATURE_IP_FILTER
#define CMD_SET_RX_FILTER_ENABLE "set_rx_filter_enable"
#define CMD_ADD_RX_FILTER_ITEMS "add_rx_filter_items"
#define CMD_CLEAR_RX_FILTERS "clear_rx_filters"
#define CMD_GET_RX_FILTER_PKT_STATE "get_rx_filter_pkt_state"
#define CMD_FILTER_SWITCH "FILTER"
#endif /* _PRE_WLAN_FEATURE_IP_FILTER */
#define CMD_SET_AUTH_RSP_TIME "set_auth_rsp_time"
#define CMD_SET_FORBIT_OPEN_AUTH "forbit_open_auth"
#define CMD_GET_APF_PKTS_CNT "GET_APF_PKTS_CNT"
#define CMD_GET_BEACON_CNT "GET_BEACON_CNT"

/* ˽��������Ʋ������Ⱥ� */
#define CMD_FILTER_SWITCH_LEN (OAL_STRLEN(CMD_FILTER_SWITCH))
#define CMD_SET_SSID_LEN (OAL_STRLEN(CMD_SET_SSID))

#ifdef WIN32
#define OAL_PAGE_SIZE 1024
oal_uint32 g_ul_skb_data_alloc_fail_count = 0;
oal_int32 hcc_flowctrl_info_print(oal_void *data, char *buf, oal_int32 buf_len)
{
    return 0;
}
#endif

#define CMD_SETSUSPENDMODE_LEN (OAL_STRLEN(CMD_SETSUSPENDMODE))

OAL_STATIC OAL_CONST wal_hipriv_cmd_entry_stru  g_ast_hipriv_cmd[] = {
    /****************************** ���ö��ⷢ����˽������ ************************************/
    { "info",                    wal_hipriv_vap_info }, /* ��ӡvap�����в�����Ϣ: hipriv "vap0 info" */
    /* ���ù��������� hipriv "Hisilicon0 setcountry param"paramȡֵΪ��д�Ĺ������֣����� CN US */
    { "setcountry",             wal_hipriv_setcountry },
    { "getcountry",             wal_hipriv_getcountry }, /* ��ѯ���������� hipriv "Hisilicon0 getcountry" */
#ifdef _PRE_WLAN_FEATURE_IP_FILTER
    /*
     * ip filter(���ܵ��Խӿ�)hipriv "wlan0 ip_filter cmd param0 param1 ...."
     * ����:��������:"wlan0 ip_filter set_rx_filter_enable 1/0"
     * ��պ�����:"wlan0 ip_filter clear_rx_filters"
     * ���ú�����:"wlan0 ip_filter add_rx_filter_items ��Ŀ����(0/1/2...)
     * ��������(protocol0 port0 protocol1 port1...)",��֧��20����Ŀ
     */
    { "ip_filter",               wal_hipriv_set_ip_filter },
    { "assigned_filter",         wal_hipriv_set_assigned_filter_switch }, /* ����ָ��rx����:[1~20] 0|1 */
#endif
    /* ��ӡָ��mac��ַuser�����в�����Ϣ: hipriv "vap0 userinfo XX XX XX XX XX XX(16����oal_strtohex)" */
    { "userinfo",                wal_hipriv_user_info },
    { "create",                  wal_hipriv_add_vap },  /* ����vap˽������Ϊ: hipriv "Hisilicon0 create vap0 ap|sta" */
#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
    /* ���ó���ģʽ: hipriv "vap0 al_tx <value: 0/1/2>  <len>" ����mac���ƣ�
     * 11a,b,g��ֻ֧��4095�������ݷ���,����ʹ��set_mcast_data�����ʽ�������
     */
    { "al_tx_1102",              wal_hipriv_always_tx_1102 },
#endif
    { "al_rx",                   wal_hipriv_always_rx },   /* ���ó���ģʽ: hipriv "vap0 al_rx <value: 0/1/2>" */
    { "rate",                    wal_hipriv_set_rate },  /* ����non-HTģʽ�µ�����: hipriv "vap0 rate  <value>" */
    { "mcs",                     wal_hipriv_set_mcs  },  /* ����HTģʽ�µ�����: hipriv "vap0 mcs <value>" */
    { "mcsac",                   wal_hipriv_set_mcsac }, /* ����VHTģʽ�µ�����: hipriv "vap0 mcsac <value>" */
    { "bw",                      wal_hipriv_set_bw },  /* ���ô���: hipriv "vap0 bw <value>" */
    { "freq",                   wal_hipriv_set_freq },  /* ����AP �ŵ� */
    { "mode",                   wal_hipriv_set_mode },  /* ����AP Э��ģʽ */
    /* ��ӡ��������Ϣ: hipriv "vap0 set_mcast_data <param name> <value>" */
    { "set_mcast_data",          wal_hipriv_set_mcast_data_dscr_param },
    /* ��ӡ����֡��FCS��ȷ�������Ϣ:hipriv "vap0 rx_fcs_info 0/1" 0/1 0���������1������� */
    { "rx_fcs_info",             wal_hipriv_rx_fcs_info },
    { "add_user",                wal_hipriv_add_user },
    /* ����ɾ���û�����������: hipriv "vap0 del_user xx xx xx xx xx xx(mac��ַ)" ���������ĳһ��VAP */
    { "del_user",                wal_hipriv_del_user },
    { "alg",                     wal_hipriv_alg },         /* alg */
    { "alg_tpc_log",             wal_hipriv_tpc_log },     /* tpc�㷨��־��������: */
    { "alg_cfg",                 wal_hipriv_alg_cfg },     /* �㷨��������: hipriv "vap0 alg_cfg sch_vi_limit 10" */
    {"sdio_flowctrl",            wal_hipriv_sdio_flowctrl},
    /* VAP�������־����: hipriv "Hisilicon0{VAPx} log_switch 0 | 1"��������������е�VAP */
    { "log_switch",              wal_hipriv_vap_log_switch },
    /* VAP������־���� hipriv "VAPX log_level {1|2}"  Warning��Error������־��VAPΪά�� */
    { "log_level",               wal_hipriv_vap_log_level },
#ifdef _PRE_WLAN_FEATURE_APF
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    {"apf_filter",       wal_hipriv_apf_filter_cmd},
#endif
#endif
    { "start_cali",              wal_hipriv_cali_permission_switch },

    /****************************** �������� *******************************************/
#ifdef PLATFORM_DEBUG_ENABLE
#ifdef _PRE_WLAN_FEATURE_PM
    { "pm_info",                 wal_hipriv_pm_info },     /* ����͹���PM��Ϣ hipriv "Hisilicon0 pm_info" */
    { "pm_enable",               wal_hipriv_pm_enable },   /* �򿪻�رյ͹��� hipriv "Hisilicon0 pm_enable 0|1" */
    { "enable",                  wal_hipriv_wifi_enable }, /* ������ر�wifi: hipriv "Hisilicon0 enable 0|1" */
#endif
    { "destroy",                 wal_hipriv_del_vap },  /* ɾ��vap˽������Ϊ: hipriv "vap0 destroy" */
    /* ȫ����־����:  hipriv "Hisilicon0 global_log_switch 0 | 1 */
    { "global_log_switch",       wal_hipriv_global_log_switch },
    /* ���Ե�INFO������־���� hipriv "Hisilicon0 log_ratelimit {type} {switch} {interval} {burst}" */
    { "log_ratelimit",           wal_hipriv_log_ratelimit },
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* log�͹���ģʽ: hipriv "Hisilicon0 log_pm 0 | 1"��log pmģʽ���� */
    { "log_pm",                  wal_hipriv_log_lowpower },
    /* log�͹���ģʽ: hipriv "Hisilicon0 pm_switch 0 | 1"��log pmģʽ���� */
    { "pm_switch",               wal_hipriv_pm_switch },
#endif
    /* OAM eventģ��Ŀ��ص�����: hipriv "Hisilicon0 event_switch 0 | 1"��������������е�VAP */
    { "event_switch",            wal_hipriv_event_switch },
#ifdef _PRE_WLAN_RF_110X_CALI_DPD
    { "start_dpd",               wal_hipriv_start_dpd }, /* Start DPD Calibration */
#endif
#ifdef _PRE_WLAN_CHIP_TEST
    /* �ֶ�����host sleep״̬�������ڲ���: hipriv "Hisilicon0 host_sleep 0 | 1" */
    { "beacon_offload_test",     wal_hipriv_beacon_offload_test },
#endif
    /* �����Ƿ��ϱ�beacon֡����: hipriv "Hisilicon0 ota_beacon_switch 0 | 1"��������������е�VAP */
    { "ota_beacon_on",           wal_hipriv_ota_beacon_switch },
    /* �����Ƿ��ϱ�����������֡����: hipriv "Hisilicon0 ota_rx_dscr_switch 0 | 1"��������������е�VAP */
    { "ota_switch",              wal_hipriv_ota_rx_dscr_switch },
    /* ����oamģ�����Ϣ��ӡλ������:hipriv "Hisilicon0 oam_output 0~4"��������������е�VAP */
    { "oam_output",              wal_hipriv_oam_output },
    /* ��������û�����������: hipriv "vap0 add_user xx xx xx xx xx xx(mac��ַ) 0 | 1(HT����λ) "���������ĳһ��VAP */
    /* ����AMPDU��������������: hipriv "vap0  ampdu_start xx xx xx xx xx xx(mac��ַ) tidno" ���������ĳһ��VAP */
    { "ampdu_start",             wal_hipriv_ampdu_start },
    /* �����Զ���ʼBA�Ự�Ŀ���:hipriv "vap0  auto_ba 0 | 1" ���������ĳһ��VAP */
    { "auto_ba",                 wal_hipriv_auto_ba_switch },
    /*
     * ���ý���BA�Ự����������:hipriv "vap0 addba_req xx xx xx xx xx xx(mac��ַ) tidno ba_policy buffsize timeout"
     * ���������ĳһ��VAP
     */
    { "addba_req",               wal_hipriv_addba_req },
    /* ����ɾ��BA�Ự����������: hipriv "vap0 delba_req xx xx xx xx xx xx(mac��ַ)tidno direction" ���������ĳһ��VAP */
    { "delba_req",               wal_hipriv_delba_req },
    /* ��ӡ�ڴ����Ϣ: hipriv "Hisilicon0 meminfo poolid" */
    { "meminfo",                 wal_hipriv_mem_info },
    /* ��ӡ�ڴ����Ϣ: hipriv "Hisilicon0 memleak poolid" */
    { "memleak",                 wal_hipriv_mem_leak },
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* ��ӡ�ڴ����Ϣ: hipriv "Hisilicon0 devicememleak poolid" */
    { "devicememleak",           wal_hipriv_device_mem_leak },
    /* ��ӡ�ڴ����Ϣ: hipriv "Hisilicon0 memoryinfo host/device" */
    { "memoryinfo",              wal_hipriv_memory_info },
#endif
    /* ��ӡ�Ĵ�����Ϣ: hipriv "wlan0 reginfo 16|32 regtype(soc/mac/phy) startaddr endaddr" */
    { "reginfo",                 wal_hipriv_reg_info },
#ifdef _PRE_WLAN_FEATURE_HILINK
    /* ����monitorģʽ������ر�:sh hipriv "Hisilicon0 set_monitor 0|1|2|3" */
    { "set_monitor",             wal_hipriv_set_monitor_switch },
#endif
    /* ����mips_cycle���㿪��: sh hipriv.sh "Hisilicon0 set_mips_cycle_test_switch 0|1" */
    { "set_mips_cycle_switch",   wal_hipriv_set_mips_cycle_switch },
    /* ����20/40�����Ƿ��ֹ�ŵ��л�: hipriv "vap0 2040_ch_swt_prohi 0|1" 0��ʾ����ֹ��1��ʾ���� */
    { "2040_ch_swt_prohi",       wal_hipriv_2040_channel_switch_prohibited },
    /* ����40MHz������λ: hipriv "vap0 2040_intolerant 0|1" 0��ʾ����������40MHz��1��ʾ��������40MHz */
    { "2040_intolerant",         wal_hipriv_set_FortyMHzIntolerant },
    /* ����20/40����ʹ��: hipriv "vap0 2040_coexistence 0|1" 0��ʾ20/40MHz����ʹ�ܣ�1��ʾ20/40MHz���治ʹ�� */
    { "2040_coexistence",        wal_hipriv_set_2040_coext_support },
    /* ��ӡ��������Ϣ: hipriv "vap0 set_ucast_data <param name> <value>" */
    { "set_ucast_data",          wal_hipriv_set_ucast_data_dscr_param },
    /* ��ӡ��������Ϣ: hipriv "vap0 set_bcast_data <param name> <value>" */
    { "set_bcast_data",          wal_hipriv_set_bcast_data_dscr_param },
    /* ��ӡ��������Ϣ: hipriv "vap0 set_ucast_mgmt <param name> <value>" */
    { "set_ucast_mgmt",          wal_hipriv_set_ucast_mgmt_dscr_param },
    /* ��ӡ��������Ϣ: hipriv "vap0 set_mbcast_mgmt <param name> <value>" */
    { "set_mbcast_mgmt",         wal_hipriv_set_mbcast_mgmt_dscr_param },
#ifdef _PRE_WLAN_FEATURE_WEB_CFG_FIXED_RATE
    /* ����ָ��ģʽ��������֡������: hipriv "vap0 set_mode_ucast_data <protocol_mode> <param name> <value>" */
    { "set_mode_ucast_data",     wal_hipriv_set_mode_ucast_data_dscr_param },
#endif
    { "get_thruput",             wal_hipriv_get_thruput }, /* ��ȡоƬ������������ hipriv "vap0 get_thruput >" */
#ifdef _PRE_DEBUG_MODE
    /* ���ö�̬����У׼���� hipriv "Hisilicon0 dync_txpower 0/1" 0:�ر� 1:�� */
    { "dync_txpower",            wal_hipriv_dync_txpower },
#endif
    /* ��ӡ�Ĵ�����Ϣ: hipriv "vap0 amsdu_start xx xx xx xx xx xx(mac��ַ10����oal_atoi) <max num> <max size> " */
    { "amsdu_start",             wal_hipriv_amsdu_start },
    { "list_ap",                 wal_hipriv_list_ap },           /* ��ӡSTAɨ�赽��AP�б�: hipriv "sta0 list_ap" */
    { "list_sta",                wal_hipriv_list_sta },          /* ��ӡAP������STA�б�: hipriv "sta0 list_sta" */
    { "start_scan",              wal_hipriv_start_scan },        /* ����staɨ��: hipriv "sta0 start_scan" */
    /* ����sta���벢��֤����: hipriv "sta0 start_join 1" 1��ʾɨ�赽��AP��deviceд�����±�� */
    { "start_join",              wal_hipriv_start_join },
    { "start_deauth",            wal_hipriv_start_deauth },      /* ����staȥ��֤: hipriv "vap0 start_deauth" */
    { "dump_timer",              wal_hipriv_dump_timer },        /* ��ӡ����timer��ά����Ϣ hipriv "vap0 dump_timer" */
    /* ɾ��1���û� hipriv "vap0 kick_user xx xx xx xx xx xx(mac��ַ)" */
    { "kick_user",               wal_hipriv_kick_user },
    /* ��ָͣ���û���ָ��tid hipriv "vap0 pause_tid xx xx xx xx xx xx(mac��ַ) tid_num 0\1" */
    { "pause_tid",               wal_hipriv_pause_tid },
    /* ����ĳ���û�ΪVIP���߷�VIP��sh hipriv.sh "vap0 set_user_vip xx xx xx xx xx xx(mac��ַ) 0\1" */
    { "set_user_vip",            wal_hipriv_set_user_vip },
    /* ����ĳ��vapΪhost����guest vap: sh hipriv.sh "vap0 st_vap_host 0\1" */
    { "set_vap_host",            wal_hipriv_set_vap_host },
    { "ampdu_tx_on",             wal_hipriv_ampdu_tx_on }, /* ������ر�ampdu���͹��� hipriv "vap0 ampdu_tx_on 0\1" */
    { "amsdu_tx_on",             wal_hipriv_amsdu_tx_on }, /* ������ر�ampdu���͹��� hipriv "vap0 amsdu_tx_on 0\1" */
    /* ָ��tid����bar hipriv "vap0 send_bar A6C758662817(mac��ַ) tid_num" */
    { "send_bar",                wal_hipriv_send_bar },
    /* �޸ļĴ�����Ϣ: hipriv "wlan0 regwrite 16|32 regtype(soc/mac/phy) addr val" */
    { "regwrite",                wal_hipriv_reg_write },
    /* ��ӡ����ba��bitmap hipriv "vap0 dump_ba_bitmap (tid_no) (RA)" */
    { "dump_ba_bitmap",          wal_hipriv_dump_ba_bitmap },
    /* ��ȡ����ά��ͳ����Ϣ: hipriv "Hisilicon0 wifi_stat_info" */
    { "wifi_stat_info",          wal_hipriv_show_stat_info },
    /* ��ȡĳһ��vap�µ��շ���ͳ����Ϣ: sh hipriv.sh "vap_name vap_pkt_stat" */
    { "vap_pkt_stat",            wal_hipriv_show_vap_pkt_stat },
    /* ��������ά��ͳ����Ϣ: hipriv "Hisilicon0 clear_stat_info" */
    { "clear_stat_info",         wal_hipriv_clear_stat_info },
    /* �ϱ�ĳ��user�µ�ά��ͳ����Ϣ: sh hipriv.sh "Hisilicon0 usr_stat_info usr_id" */
    { "usr_stat_info",           wal_hipriv_user_stat_info },
    { "timer_start",             wal_hipriv_timer_start }, /* ����5115Ӳ����ʱ��: hipriv "Hisilicon0 timer_start 0/1" */
    /* ����amsdu ampdu���ϾۺϹ��ܵĿ���:hipriv "vap0  ampdu_amsdu 0 | 1" ���������ĳһ��VAP */
    { "ampdu_amsdu",             wal_hipriv_ampdu_amsdu_switch },
    /*
     * ��λӲ��phy&mac: hipriv "Hisilicon0 reset_hw 0|1|2|3|4|5|6|8|9|10|11
     * (all|phy|mac|debug|mac_tsf|mac_cripto|mac_non_cripto|phy_AGC|phy_HT_optional|phy_VHT_optional|phy_dadar)
     * 0|1(reset phy reg) 0|1(reset mac reg)
     */
    { "reset_hw",                wal_hipriv_reset_device },
    /* ��λӲ��phy&mac: hipriv "Hisilicon0 reset_hw 0|1|2|3(all|phy|mac|debug) 0|1(reset phy reg) 0|1(reset mac reg) */
    { "reset_operate",           wal_hipriv_reset_operate },
    /* dump�����������������У�hipriv "Hisilicon0 dump_rx_dscr 0|1", 0:�����ȼ����� 1:��ͨ���ȼ����� */
    { "dump_rx_dscr",            wal_hipriv_dump_rx_dscr },
    /* dump�����������������У�hipriv "Hisilicon0 dump_tx_dscr value", valueȡֵ0~3����AC���Ͷ��У�4�������֡ */
    { "dump_tx_dscr",            wal_hipriv_dump_tx_dscr },
    /* dump�ڴ棬 hipriv "Hisilicon0 dump_memory 0xabcd len" */
    { "dump_memory",             wal_hipriv_dump_memory },
    /* ��ӡ�ڴ�������з�����������ַ hipriv "Hisilicon0 show_tx_dscr_addr" */
    { "show_tx_dscr_addr",       wal_hipriv_show_tx_dscr_addr },
    /* ֧���ŵ��б� hipriv "Hisilicon0 list_channel" */
    { "list_channel",            wal_hipriv_list_channel },
    /* ���ù���������͹��ʣ�hipriv "Hisilicon0 set_regdomain_pwr 20",��λdBm */
    { "set_regdomain_pwr",       wal_hipriv_set_regdomain_pwr },
    /* ���ù���������͹���(����ͻ�ƹ����������)��hipriv "Hisilicon0 set_regdomain_pwr_priv 20",��λdBm */
    { "set_regdomain_pwr_p",     wal_hipriv_set_regdomain_pwr_priv },
    /* ��ӡ�¼�������Ϣ������ӡ��ÿһ���ǿ��¼��������¼��ĸ������Լ�ÿһ���¼�ͷ��Ϣ, hipriv "Hisilicon0 event_queue" */
    { "event_queue",             wal_hipriv_event_queue_info },
    /* ��ӡ���еĽ���������, hipriv "Hisilicon0 dump_all_dscr" */
    { "dump_all_dscr",           wal_hipriv_dump_all_rx_dscr },
    /* ���÷�Ƭ���޵���������: hipriv "vap0 frag_threshold (len)" ���������ĳһ��VAP */
    { "frag_threshold",          wal_hipriv_frag_threshold },
    /* ��̬�������߹ر�wmm hipriv "vap0 wmm_switch 0|1"(0��ʹ�ܣ�1ʹ��)  */
    { "wmm_switch",              wal_hipriv_wmm_switch },
    /* ����ssid���ܿ������߹ر� wmm hipriv "Hisilicon0 hide_ssid 0|1"(0��ʹ�ܣ�1ʹ��) */
    { "hide_ssid",               wal_hipriv_hide_ssid },
    /*
     * ������̫��֡�ϱ��Ŀ���
     * sh hipriv.sh "vap0 ether_switch user_macaddr oam_ota_frame_direction_type_enum(֡����) 0|1(����)"
     */
    { "ether_switch",            wal_hipriv_set_ether_switch },
    /*
     * ����80211����֡�ϱ��Ŀ���
     * sh hipriv.sh "vap0 80211_uc_switch user_macaddr 0|1(֡����tx|rx) 0|1(֡����:����֡|����֡)
     * 0|1(֡���ݿ���) 0|1(CB����) 0|1(����������)"
     */
    { "80211_uc_switch",         wal_hipriv_set_80211_ucast_switch },
    /*
     * ����80211�鲥\�㲥֡�ϱ��Ŀ���
     * sh hipriv.sh "Hisilicon0 80211_mc_switch 0|1(֡����tx|rx) 0|1(֡����:����֡|����֡) 0|1(֡���ݿ���)
     * 0|1(CB����) 0|1(����������)"
     */
    { "80211_mc_switch",         wal_hipriv_set_80211_mcast_switch },
    /*
     * ����probe req��rsp�ϱ��Ŀ���
     * sh hipriv.sh "Hisilicon0 probe_switch 0|1(֡����tx|rx) 0|1(֡���ݿ���) 0|1(CB����) 0|1(����������)"
     */
    { "probe_switch",            wal_hipriv_set_probe_switch },
    /* ���ô�ӡ���ձ���rssi��Ϣ�Ŀ��أ�sh hipriv.sh "Hisilicon0 rssi_switch 0|1(��|�ر�) N(���N��֡��ӡ)" */
    { "rssi_switch",             wal_hipriv_set_rssi_switch },
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* ���ݱ��λ�ϱ�vap�Ķ�Ӧ��Ϣ sh hipriv.sh "wlan0 report_vap_info 1" */
    { "report_vap_info",         wal_hipriv_report_vap_info },
    /* wfaʹ�ã��̶�ָ��AC��aifsnֵ, sh hipriv.sh "Hisilicon0 aifsn_cfg 0|1(�ָ�|����) 0|1|2|3(be-vo) val" */
    { "aifsn_cfg",               wal_hipriv_aifsn_cfg },
    /* wfaʹ�ã��̶�ָ��AC��cwmaxminֵ, sh hipriv.sh "Hisilicon0 cw_cfg 0|1(�ָ�|����) 0|1|2|3(be-vo) val" */
    { "cw_cfg",                  wal_hipriv_cw_cfg },
#endif
    /* ��ȡdevice�º�ÿһ��tid�µ�ǰmpdu������sh hipriv.sh "vap_name mpdu_num user_macaddr" */
    { "mpdu_num",                wal_hipriv_get_mpdu_num },
     /*
      * ��������ota�ϱ������Ϊ1������������֡��cb���������������Ϊ0��ʲô������
      * sh hipriv.sh "Hisilicon0 set_all_ota 0|1"
      */
    { "set_all_ota",             wal_hipriv_set_all_ota },
    /*
     * ���������û��ĵ������أ�sh hipriv.sh "Hisilicon0 80211_uc_all 0|1(֡����tx|rx) 0|1(֡����:����֡|����֡)
     * 0|1(֡���ݿ���) 0|1(CB����) 0|1(����������)"
     */
    { "80211_uc_all",            wal_hipriv_set_all_80211_ucast },
    /* ���������û�����̫�����أ�sh hipriv.sh "Hisilicon0 ether_all 0|1(֡����tx|rx) 0|1(����)" */
    { "ether_all",               wal_hipriv_set_all_ether_switch },
    /* ���÷��͹㲥arp��dhcp���أ�sh hipriv.sh "Hisilicon0 dhcp_arp_switch 0|1(����)" */
    { "dhcp_arp_switch",         wal_hipriv_set_dhcp_arp_switch },
#ifdef _PRE_DEBUG_MODE_USER_TRACK
#endif
#ifdef _PRE_WLAN_FEATURE_TXOPPS
    /*
     * ����mac txop psʹ�ܼĴ���
     * sh hipriv.sh "stavap_name txopps_hw_en 0|1(txop_ps_en) 0|1(condition1) 0|1(condition2)"
     */
    { "txopps_hw_en",            wal_hipriv_set_txop_ps_machw },
#endif
#ifdef _PRE_WLAN_FEATURE_UAPSD
    /* uapsdά����Ϣ��sh hipriv "vap0 uapsd_debug 0|1|2(���û�|all user|���ͳ�Ƽ�����) xx:xx:xx:xx:xx:xx(mac��ַ)" */
    { "uapsd_debug",             wal_hipriv_uapsd_debug },
#endif
#ifdef _PRE_WLAN_FEATURE_BTCOEX
    { "coex_print",              wal_hipriv_btcoex_status_print }, /* ��ӡ����ά����Ϣ��sh hipriv.sh "coex_print" */
#endif
#ifdef _PRE_WLAN_DFT_STAT
    /*
     * ����phyͳ��ʹ�ܽڵ��ţ�һ�ο�������4����������Χ1~16
     * sh hipriv.sh "Hisilicon0 phy_stat_en idx1 idx2 idx3 idx4"
     */
    { "phy_stat_en",             wal_hipriv_set_phy_stat_en },
    /* �ϱ�����ֹͣ�ϱ��տڻ����������Ϣ: sh hipriv.sh "Hisilicon0 dbb_env_param 0|1" */
    { "dbb_env_param",           wal_hipriv_dbb_env_param },
    /* �ϱ����������û�����ͳ����Ϣ: sh hipriv.sh "vap_name usr_queue_stat XX:XX:XX:XX:XX:XX 0|1" */
    { "usr_queue_stat",          wal_hipriv_usr_queue_stat },
    /* �ϱ�����ֹͣ�ϱ�vap����ͳ����Ϣ: sh hipriv.sh "vap_name vap _stat  0|1" */
    { "vap_stat",                wal_hipriv_report_vap_stat },
    /* �ϱ�������������ά��ͳ����Ϣ: sh hipriv.sh "Hisilicon0 reprt_all_stat type(phy/machw/mgmt/irq/all) 0|1" */
    { "reprt_all_stat",          wal_hipriv_report_all_stat },
#endif
#ifdef _PRE_DEBUG_MODE
    /* �ϱ���������ampduά��ͳ����Ϣ: sh hipriv.sh "vap_name ampdu_stat XX:XX:XX:XX:XX:XX tid_no 0|1" */
    { "ampdu_stat",              wal_hipriv_report_ampdu_stat },
#endif
    /*
     * ����AMPDU�ۺϸ���:
     * sh hipriv.sh "Hisilicon0 ampdu_aggr_num aggr_num_switch aggr_num" ,aggr_num_switch��0ʱ��aggr_num��Ч
     */
    { "ampdu_aggr_num",          wal_hipriv_set_ampdu_aggr_num },

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    /*
     * Ƶƫ������������:
     * sh hipriv.sh "Hisilicon0 freq_adjust pll_int pll_frac" ,pll_int������Ƶϵ����pll_fracС����Ƶϵ��
     */
    { "freq_adjust",             wal_hipriv_freq_adjust },
#endif

    { "set_stbc_cap",           wal_hipriv_set_stbc_cap }, /* ����STBC���� */
    { "set_ldpc_cap",           wal_hipriv_set_ldpc_cap }, /* ����LDPC���� */
#ifdef _PRE_WLAN_FEATURE_STA_PM
    { "set_ps_mode",            wal_hipriv_sta_ps_mode },  /* ����PSPOLL���� sh hipriv.sh 'wlan0 set_ps_mode 3 0' */
#ifdef _PRE_PSM_DEBUG_MODE
    { "psm_info_debug",         wal_hipriv_sta_ps_info },  /* sta psm��ά��ͳ����Ϣsh hipriv.sh 'wlan0 psm_info_debug 1' */
#endif
    { "set_sta_pm_on",          wal_hipriv_sta_pm_on },    /* sh hipriv.sh 'wlan0 set_sta_pm_on xx xx xx xx */
#endif

#ifdef _PRE_WLAN_FEATURE_STA_UAPSD
    /* ����uapsd�Ĳ�����Ϣ sh hipriv.sh 'wlan0 set_uapsd_para 3 1 1 1 1 */
    { "set_uapsd_para",        wal_hipriv_set_uapsd_para },
#endif

#ifdef _PRE_WLAN_CHIP_TEST
    /* ˯�߻���оƬ, hipriv "Hisilicon0 lpm_chip_state 0|1|2(0:soft sleep��1:gpio sleep,2:work)" */
    { "lpm_chip_state",          wal_hipriv_lpm_chip_state },
    /*
     * ˯�߻���оƬ, hipriv "Hisilicon0 lpm_soc_mode 0|1|2|3|4
     * ����gating|PCIE RD BY PASS|mem precharge|PCIE L0-S|PCIE L1-0) 0|1(disable|enable)"
     */
    { "lpm_soc_mode",            wal_hipriv_lpm_soc_mode },
    /*
     * psm���ܼĴ�������,hipriv "Hisilicon0 lpm_psm_param 0|1|2(ps off|ps on|debug) 0|1(DTIM|listen intval)
     * xxx(listen intervalֵ) xxx(TBTT offset)"
     */
    { "lpm_psm_param",           wal_hipriv_lpm_psm_param },
    /* smps����ģʽ����, hipriv "Hisilicon0 lpm_smps_mode 0|1|2(off|static|dynamic)" */
    { "lpm_smps_mode",           wal_hipriv_lpm_smps_mode },
    /* smps ap������׮, hipriv "vap0 lpm_smps_stub 0|1|2(off|����|˫��) 0|1(�Ƿ�RTS)" */
    { "lpm_smps_stub",           wal_hipriv_lpm_smps_stub },
    /*
     * txop ps����ģʽ����
     * hipriv "Hisilicon0 lpm_txopps_set 0|1(off|on|debug) 0|1(contion1 off|on) 0|1(condition2 off|on)"
     */
    { "lpm_txopps_set",          wal_hipriv_lpm_txopps_set },
    /* txop ps�������Դ�׮����, hipriv "vap0 lpm_txopps_tx_stub 0|1|2(off|address|partial AID) xxx(�ڼ�������׮)" */
    { "lpm_txopps_tx_stub",      wal_hipriv_lpm_txopps_tx_stub },
    /* WOWģʽ����, hipriv "Hisilicon0 lpm_wow_en 0|1|2(0��ʹ��|1ʹ��|2������Ϣ) 0|1(Null data�Ƿ���)" " */
    { "lpm_wow_en",              wal_hipriv_lpm_wow_en },
    /* ���Է���, hipriv "vap0 lpm_tx_data xxx(����) xxx(����) xx:xx:xx:xx:xx:xx(Ŀ��mac) xxx(AC����)" */
    { "lpm_tx_data",             wal_hipriv_lpm_tx_data },
    /* ���Է���, hipriv "vap0 lpm_tx_probe_req 0|1(����|����) xx:xx:xx:xx:xx:xx(����ģʽ��BSSID)" */
    { "lpm_tx_probe_req",        wal_hipriv_lpm_tx_probe_request },
    /* ɾ���ָ��û�lut��, hipriv "vap0 remove_lut xx:xx:xx:xx:xx:xx(mac��ַ 16����) 0|1(�ָ�/ɾ��)" */
    { "remove_lut",              wal_hipriv_remove_user_lut },
    /* ָ��tid����bar hipriv "vap0 send_frame (type) (num) (Ŀ��mac)" */
    { "send_frame",              wal_hipriv_send_frame },
    { "set_rx_pn",               wal_hipriv_set_rx_pn },      /* ����RX_PN_LUT_CONFIG�Ĵ��� */
    /* ����software_retry ������ hipriv "Hisilicon0 set_sft_retry 0|1(0��ʹ�ܣ�1ʹ��)" */
    { "set_sft_retry",           wal_hipriv_set_soft_retry },
    /* ����macͷ����4��ַ hipriv "Hisilicon0 open_addr4 0|1(0��ʹ�ܣ�1ʹ��) */
    { "open_addr4",              wal_hipriv_open_addr4 },
    /* ����оƬ��֤���� hipriv "Hisilicon0 open_wmm_test 0|1|2|3() */
    { "open_wmm_test",           wal_hipriv_open_wmm_test },
    /* ����оƬ��֤���� hipriv "Hisilicon0 chip_test 0|1(0��ʹ�ܣ�1ʹ��) */
    { "chip_test",               wal_hipriv_chip_test_open },
    /* ���ù�����ƿ��� hipriv "Hisilicon0 coex_ctrl xxx(mac ctrlֵ) xxx(rf ctrlֵ)) */
    { "coex_ctrl",               wal_hipriv_set_coex },
    /* ����DFX���Կ��� sh hipriv.sh "Hisilicon0 dfx_en 0|1 */
    { "dfx_en",                  wal_hipriv_set_dfx },
    /* ����жϺ͹���֡ͳ����Ϣ hipriv "Hisilicon0 clear_all_stat" */
    { "clear_all_stat",          wal_hipriv_clear_all_stat },

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
    /* ����chip test��ǿ��ʹ��pmf���� (���ڹ���֮��) sh hipriv.sh "vap0 enable_pmf 0|1|2(0��ʹ�ܣ�1 enable, 2ǿ��)  */
    { "enable_pmf",              wal_hipriv_enable_pmf },
#endif
    /* ����action֡�ӿ� sh hipriv.sh "vap0 send_action XX(category) xx:xx:xx:xx:xx:xx(Ŀ�ĵ�ַ 16����) " */
    { "send_action",             wal_hipriv_test_send_action },
    { "send_pspoll",             wal_hipriv_send_pspoll },      /* sta��ps-poll��ap��sh hipriv "vap0 send_pspoll" */
    /* sta��null data��ap��֪ͨ����״̬��sh hipriv "vap0 send_nulldata 0|1(�Ƿ�������) 0|1(�Ƿ�qosnull) tid_no" */
    { "send_nulldata",           wal_hipriv_send_nulldata },
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */
#ifdef _PRE_WLAN_FEATURE_DFR
    /* dfr���ܴ�׮�����ӿ�??sh hipriv.sh "vap0 dfr_start 0(dfr�ӹ���:0-device�쳣��λ )" */
    { "dfr_start",              wal_hipriv_test_dfr_start },
#endif // _PRE_WLAN_FEATURE_DFR
    /* �㷨��ص����� */
    { "alg_ar_log",              wal_hipriv_ar_log },      /* autorate�㷨��־��������: */
    { "alg_ar_test",             wal_hipriv_ar_test },     /* autorate�㷨ϵͳ�������� */
    { "alg_cca_opt_log",         wal_hipriv_cca_opt_log }, /* cca�㷨��־��������: */
#ifdef _PRE_WLAN_FEATURE_DFS
    {"radartool",                wal_hipriv_dfs_radartool},
#endif
#ifdef _PRE_SUPPORT_ACS
    {"acs",                      wal_hipriv_acs},
#endif

#ifdef _PRE_WLAN_PERFORM_STAT
    /* ����ͳ������ */
    /*
     * ͳ��ָ��tid��������:
     * hipriv "vap0 stat_tid_thrpt xx xx xx xx xx xx(mac��ַ) tid_num stat_period(ͳ������ms) stat_num(ͳ�ƴ���)"
     */
    { "stat_tid_thrpt",          wal_hipriv_stat_tid_thrpt },
    /*
     * ͳ��ָ��user��������:
     * hipriv "vap0 stat_user_thrpt xx xx xx xx xx xx(mac��ַ) stat_period(ͳ������ms) stat_num(ͳ�ƴ���)"
     */
    { "stat_user_thrpt",         wal_hipriv_stat_user_thrpt },
    /* ͳ��ָ��tid��������: hipriv "vap0 stat_vap_thrpt stat_period(ͳ������ms) stat_num(ͳ�ƴ���)" */
    { "stat_vap_thrpt",          wal_hipriv_stat_vap_thrpt },
    /*
     * ͳ��ָ��tid��per:
     * hipriv "vap0 stat_tid_per xx xx xx xx xx xx(mac��ַ) tid_num stat_period(ͳ������ms) stat_num(ͳ�ƴ���)"
     */
    { "stat_tid_per",            wal_hipriv_stat_tid_per },
    /*
     * ͳ��ָ��tid��delay:
     * hipriv "vap0 stat_tid_delay xx xx xx xx xx xx(mac��ַ) tid_num stat_period(ͳ������ms) stat_num(ͳ�ƴ���)"
     */
    { "stat_tid_delay",          wal_hipriv_stat_tid_delay },

    /* ������ʾ���� */
    /* ͳ��ָ��tid��������: hipriv "vap0 dspl_tid_thrpt xx xx xx xx xx xx(mac��ַ)" */
    { "dspl_tid_thrpt",          wal_hipriv_display_tid_thrpt },
    /* ͳ��ָ��user��������: hipriv "vap0 dspl_user_thrpt xx xx xx xx xx xx(mac��ַ)" */
    { "dspl_user_thrpt",         wal_hipriv_display_user_thrpt },
    { "dspl_vap_thrpt",          wal_hipriv_display_vap_thrpt }, /* ͳ��ָ��tid��������: hipriv "vap0 dspl_vap_thrpt" */
    /* ͳ��ָ��tid��per: hipriv "vap0 dspl_tid_per xx xx xx xx xx xx(mac��ַ) tid_num" */
    { "dspl_tid_per",            wal_hipriv_display_tid_per },
    /* ͳ��ָ��tid��delay: hipriv "vap0 dspl_tid_delay xx xx xx xx xx xx(mac��ַ) tid_num" */
    { "dspl_tid_delay",          wal_hipriv_display_tid_delay },
#endif

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
    { "set_edca_switch_sta",     wal_hipriv_set_edca_opt_switch_sta }, /* STA�Ƿ���˽��edca�����Ż����� */
    { "set_edca_weight_sta",     wal_hipriv_set_edca_opt_weight_sta }, /* STA edca��������Ȩ�� */
    { "set_edca_switch_ap",      wal_hipriv_set_edca_opt_switch_ap },  /* �Ƿ���edca�Ż����� */
    { "set_edca_cycle_ap",       wal_hipriv_set_edca_opt_cycle_ap },   /* ����edca�������������� */
#endif

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
    { "get_hipkt_stat",          wal_hipriv_get_hipkt_stat },    /* ��ȡ�����ȼ����ĵ�ͳ����� */
    { "set_flowctl_param",       wal_hipriv_set_flowctl_param }, /* ����������ز��� */
    { "get_flowctl_stat",        wal_hipriv_get_flowctl_stat },  /* ��ȡ�������״̬��Ϣ */
#endif

#ifdef _PRE_DEBUG_MODE
    /* ά������:����ĳ��ֵ��ĳ������ */
    /*
     * ����ĳһ�־����debug���Ϳ���:
     * hipriv "Hisilicon0 debug_switch debug_type debug_value"������������豸�������ʹ��
     */
    { "debug_switch",            wal_hipriv_set_debug_switch },
    /*
     * ͳ�Ʒ�������ж��Ƿ�ʧ(�رվۺ�)
     * hipriv "Hisilicon0 tx_comp_cnt 0|1", 0��ʾ����ͳ�ƴ����� 1��ʾ��ʾͳ�ƴ�����������
     */
    { "tx_comp_cnt",             wal_hipriv_get_tx_comp_cnt },
    /* ���ý���֡���˸�״̬�µ�����ֵ:hipriv "Hisilicon0 set_rx_filter_val 0-Normal/1-Repeater mode status value" */
    { "set_rx_filter_val",       wal_hipriv_set_rx_filter_val },
    /* ���ý���֡���˸�״̬�µ�����ֵ:hipriv "Hisilicon0 get_rx_filter_val 0-Normal/1-Repeater mode status" */
    { "get_rx_filter_val",       wal_hipriv_get_rx_filter_val },
    /* ��ȡ����֡���˸�״̬�µ�����ֵ:hipriv "Hisilicon0 set_rx_filter_en 0-��/1-�ر� */
    { "set_rx_filter_en",        wal_hipriv_set_rx_filter_en },
    /* ��ȡ����֡���˸�״̬�µ�����ֵ:hipriv "Hisilicon0 get_rx_filter_en */
    { "get_rx_filter_en",        wal_hipriv_get_rx_filter_en },
    /* ��ȡ���мĴ�����ֵ: hipriv "Hisilicon0 get_all_regs" */
    { "get_all_regs",            wal_hipriv_get_all_reg_value },
#endif

    { "set_mib",                 wal_hipriv_set_mib },             /* ����VAP mibֵ */
    { "get_mib",                 wal_hipriv_get_mib },             /* ��ȡVAP mibֵ */
    { "thruput_bypass",          wal_hipriv_set_thruput_bypass },  /* ����thruput bypassά��� */
    { "auto_protection",         wal_hipriv_set_auto_protection }, /* �����Զ��������� */

    /* ����ά����� */
    /* ����20/40�������֡: hipriv "Hisilicon0 send_2040_coext coext_info chan_report" */
    { "send_2040_coext",         wal_hipriv_send_2040_coext },
    /* ��ӡvap������20/40���������Ϣ: hipriv "vap0 2040_coext_info" */
    { "2040_coext_info",         wal_hipriv_2040_coext_info },
    { "get_version",             wal_hipriv_get_version },     /* ��ȡ����汾: hipriv "vap0 get_version" */

#ifdef _PRE_WLAN_FEATURE_DAQ
    /* ��ȡ����汾: hipriv "Hisilicon0 data_acq 0/1/2/3/4 (length num depth) (channel mode data_th bit) (2) () ()" */
    { "data_acq",                wal_hipriv_data_acq },
#endif

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    { "set_oma",                 wal_hipriv_set_oma },         /* ����Proxy STA��oma��ַ" */
    /* proxystaģ��Ŀ��ص�����: hipriv "Hisilicon0 proxysta_switch 0 | 1"��������������е�VAP */
    { "proxysta_switch",         wal_hipriv_proxysta_switch },
#endif

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
    /* ����VAP����ģʽ֪ͨ: hipriv "vap0 set_opmode_notify 0/1"  0-��֧��; 1-֧�� */
    { "set_opmode_notify",       wal_hipriv_set_opmode_notify },
    /* ��������û�����������: hipriv "vap0 get_user_nssbw xx xx xx xx xx xx(mac��ַ) "  ���������ĳһ��VAP */
    { "get_user_nssbw",          wal_hipriv_get_user_nssbw },
#endif

#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY

    { "blacklist_add",           wal_hipriv_blacklist_add },       /* 1 */
    { "blacklist_del",           wal_hipriv_blacklist_del },       /* 2 */
    { "blacklist_mode",          wal_hipriv_set_blacklist_mode },  /* 3 */
    { "blacklist_show",          wal_hipriv_blacklist_show },      /* 4 wal_config_blacklist_show */
    { "abl_on",                  wal_hipriv_set_abl_on },          /* 5 */
    { "abl_aging",               wal_hipriv_set_abl_aging },       /* 6 */
    { "abl_threshold",           wal_hipriv_set_abl_threshold },   /* 7 */
    { "abl_reset",               wal_hipriv_set_abl_reset },       /* 8 wal_config_set_autoblacklist_reset_time */
    { "isolation_mode",          wal_hipriv_set_isolation_mode },  /* 9 */
    { "isolation_type",          wal_hipriv_set_isolation_type },  /* 10 */
    { "isolation_fwd",           wal_hipriv_set_isolation_fwd },   /* 11 */
    { "isolation_clear",         wal_hipriv_set_isolation_clear }, /* 12 wal_config_set_isolation_clear */
    { "isolation_show",          wal_hipriv_set_isolation_show },  /* 13 wal_config_isolation_show */

#endif
#ifdef _PRE_WLAN_FEATURE_MCAST
    { "m2u_snoop_on",            wal_hipriv_m2u_snoop_on }, /* ������ر�snoop���ع��� hipriv "vap0 m2u_snoop_on 0\1" */
    /* �����鲥������� hipriv "vap0 m2u_add_deny_table 224.1.1.1" */
    { "m2u_add_deny_table",      wal_hipriv_m2u_add_deny_table },
    /* �����鲥������� hipriv "vap0 m2u_cfg_deny_table 1 0" */
    { "m2u_cfg_deny_table",      wal_hipriv_m2u_cfg_deny_table },
    /* ��ӡ�鲥�� hipriv "vap0 m2u_show_snoop_table 1" */
    { "m2u_prt_sn_table",        wal_hipriv_m2u_show_snoop_table },
    /* ��Ŀ��STA/AP��������֡: hipriv "vap0 m2u_igmp_pkt_xmit (tid_no) (���ĸ���) (���ĳ���) (RA MAC)" */
    { "m2u_igmp_pkt_xmit",       wal_hipriv_igmp_packet_xmit },
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
    { "proxyarp_on",            wal_hipriv_proxyarp_on }, /* hipriv "vap0 proxyarp_on 0\1" */
#ifdef _PRE_DEBUG_MODE
    { "proxyarp_info",          wal_hipriv_proxyarp_info }, /* hipriv "vap0 proxyarp_info 0\1" */
#endif /* #ifdef _PRE_DEBUG_MODE */
#endif /* #ifdef _PRE_WLAN_FEATURE_PROXY_ARP */
#ifdef _PRE_WLAN_FEATURE_UAPSD
    { "uapsd_en_cap",           wal_hipriv_set_uapsd_cap }, /* hipriv "vap0 uapsd_en_cap 0\1" */
#endif
#ifdef _PRE_WLAN_FEATURE_SMPS
#ifdef _PRE_DEBUG_MODE
    { "smps_info",              wal_hipriv_get_smps_info }, /* hipriv "vap0 smps_info 0\1" */
#endif /* #ifdef _PRE_DEBUG_MODE */
#endif /* #ifdef _PRE_WLAN_FEATURE_SMPS */
#ifdef _PRE_DEBUG_MODE
    /*
     * ɨ��ģ��������� hipriv "Hisilicon0 scan_test param1 param2" param1
     * ȡֵ'2g' '5g' 'all' 1~14, 36~196; param2ȡֵ��Ӧwlan_channel_bandwidth_enum_uint8
     */
    { "scan_test",              wal_hipriv_scan_test },
#endif
    /* ɨ��ֹͣ�������� hipriv "Hisilicon0 bgscan_enable param1" param1ȡֵ'0' '1',��Ӧ�رպʹ򿪱���ɨ�� */
    { "bgscan_enable",          wal_hipriv_bgscan_enable },
#ifdef _PRE_WLAN_PROFLING_MIPS
    /* ����ĳ���̵�MIPSͳ�ƿ��أ�sh hipriv.sh "Hisilicon0 set_mips wal_mips_param_enum 0|1" */
    { "set_mips",               wal_hipriv_set_mips },
    /* ��ӡĳ���̵�MIPSͳ�ƽ����sh hipriv.sh "Hisilicon0 show_mips wal_mips_param_enum" */
    { "show_mips",              wal_hipriv_show_mips },
#endif
    { "essid",                  wal_hipriv_set_essid }, /* ����AP ssid */
    {"txpower",                 wal_hipriv_set_txpower},
    { "bintval",                wal_hipriv_set_beacon_interval }, /* ����AP beacon ���� */
    {"up",                      wal_hipriv_start_vap},
#ifdef _PRE_WLAN_FEATURE_11D
    /* �����Ƿ���ݹ���ap���¹�������Ϣ hipriv "Hisilicon0 set_rd_by_ie_switch 0/1" */
    { "set_rd_by_ie_switch",    wal_hipriv_set_rd_by_ie_switch },
#endif
#ifdef  _PRE_WLAN_FEATURE_P2P
#ifdef _PRE_WLAN_CHIP_TEST
    { "p2p_ps",                 wal_hipriv_set_p2p_ps }, /* ����P2P ���� sh hipriv.sh "vap0 p2p_ps noa/ops params */
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */
/* sh hipriv.sh "vap0 p2p_ps ops 0/1(0��ʹ�ܣ�1ʹ��) [0~255] ����OPS ������ct_window ���� */
/* sh hipriv.sh "vap0 p2p_ps noa start_time duration interval count ����NOA ���ܲ��� */
/* sh hipriv.sh "vap0 p2p_ps statistics 0/1(0 ���ͳ�ƣ�1�鿴ͳ��) P2P �ж�ͳ�� */
#endif /* _PRE_WLAN_FEATURE_P2P */

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    /* ʹ�ָܻ�rx intr fifo���Ĭ�ϲ����� hipriv "Hisilicon0 resume_rxintr_fifo 0|1" 1ʹ�� */
    { "resume_rx_intr_fifo",     wal_hipriv_resume_rx_intr_fifo },
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    { "ampdu_mmss",              wal_hipriv_set_ampdu_mmss }, /* ����AMPDU MMSS : sh hipriv.sh "vap0 ampdu_mmss 0~7" */
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
    /* ARP/ND�������ƺ͹㲥/�鲥���˿���:sh hipriv.sh "wlan0 arp_offload_enable  0/1(0�رգ�1��)" */
    { "arp_offload_enable",      wal_hipriv_arp_offload_enable },
#endif

#ifdef _PRE_WLAN_TCP_OPT
    /* ��ʾTCP ACK ����ͳ��ֵ sh hipriv.sh "vap0 get_tx_ack_stream_info */
    { "get_tcp_ack_stream_info", wal_hipriv_get_tcp_ack_stream_info },
    /* ���÷���TCP ACK�Ż�ʹ��  sh hipriv.sh "vap0 tcp_tx_ack_opt_enable 0 | 1 */
    { "tcp_tx_ack_opt_enable",   wal_hipriv_tcp_tx_ack_opt_enable },
    /* ���ý���TCP ACK�Ż�ʹ�� sh hipriv.sh "vap0 tcp_rx_ack_opt_enable 0 | 1 */
    { "tcp_rx_ack_opt_enable",   wal_hipriv_tcp_rx_ack_opt_enable },
    /* ���÷���TCP ACK LIMIT sh hipriv.sh "vap0 tcp_tx_ack_opt_limit X */
    { "tcp_tx_ack_opt_limit",    wal_hipriv_tcp_tx_ack_limit },
    /* ���ý���TCP ACKLIMIT  sh hipriv.sh "vap0 tcp_tx_ack_opt_limit X */
    { "tcp_rx_ack_opt_limit",    wal_hipriv_tcp_rx_ack_limit },

#endif

#ifdef _PRE_WLAN_FEATURE_WAPI
#ifdef _PRE_WAPI_DEBUG
    { "wapi_info",               wal_hipriv_show_wapi_info }, /* wapi hipriv "vap0 wal_hipriv_show_wapi_info " */
#endif /* #ifdef _PRE_DEBUG_MODE */
#endif /* #ifdef _PRE_WLAN_FEATURE_WAPI */

#ifdef _PRE_WLAN_DFT_STAT
    /* �������ܴ�ӡ���ƿ��� sh hipriv.sh "wlan0 performance_log_debug X Y, */
    { "performance_log_debug",  wal_hipriv_performance_log_switch },
/* ����X�Ǵ�ӡ�㣬��oal_performance_log_switch_enum���壬Y��ʹ�ܿ���,0�رգ�1�򿪡� */ /* X=255ʱ���������еĴ�ӡ���� */
/* ʹ��˵��:                                                     */
/* sh hipriv.sh "wlan0 performance_log_debug 0 0 :�رվۺϴ�ӡ   */
/* sh hipriv.sh "wlan0 performance_log_debug 0 1 :�򿪾ۺϴ�ӡ   */
/* sh hipriv.sh "wlan0 performance_log_debug 1 0 :��ӡ����ͳ��   */
/* sh hipriv.sh "wlan0 performance_log_debug 1 1 :�ۺ�ͳ����0    */
/* sh hipriv.sh "wlan0 performance_log_debug 255 0 :������п��ƿ��� */
/* sh hipriv.sh "wlan0 performance_log_debug 255 1 :�������п��ƿ��� */
#endif
#ifdef _PRE_WLAN_FEATURE_ROAM
    { "roam_enable",      wal_hipriv_roam_enable }, /* �������ο��� */
    { "roam_org",         wal_hipriv_roam_org },    /* ������������ */
    { "roam_band",        wal_hipriv_roam_band },   /* ��������Ƶ�� */
    { "roam_start",       wal_hipriv_roam_start },  /* ���β������� */
    { "roam_info",        wal_hipriv_roam_info },   /* ������Ϣ��ӡ */
#endif // _PRE_WLAN_FEATURE_ROAM
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    /* ����20/40 bssʹ��: hipriv "Hisilicon0 2040bss_enable 0|1" 0��ʾ20/40 bss�жϹرգ�1��ʾʹ�� */
    { "2040bss_enable",   wal_hipriv_enable_2040bss },
#endif
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    /* �����Զ���Ƶʹ��: hipriv "wlan0 auto_freq 0 0" �ڶ�������0��ʾ�رգ�1��ʾʹ�� */
    { "auto_freq",        wal_hipriv_set_auto_freq },
#endif
#endif

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    { "customize_info",  wal_hipriv_dev_customize_info }, /* ��ӡdevice�ඨ�ƻ���Ϣ */
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */
#ifdef _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN
    /* ����ҵ��ʶ���ܿ���: sh hipriv.sh "p2p-p2p0-0 set_tx_classify_switch 1/0"(1�򿪣�0�رգ�����Ĭ�Ͽ���) */
    { "set_tx_classify_switch", wal_hipriv_set_tx_classify_switch },
#endif /* _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN */
    { "mcs_check_enable",        wal_hipriv_mcs_set_check_enable },
    /* ����String����ͨ������: hipriv "wlan0 set_str 11ax_debug 3 tid 0 val 1 cnt 2"  */
    { "set_str",                 wal_hipriv_set_str_cmd },
#ifdef _PRE_DEBUG_MODE
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    { "mem_check",               wal_hipriv_mem_check },
#endif
#endif    /* _PRE_DEBUG_MODE */
#endif    /* PLATFORM_DEBUG_ENABLE */
    /* ����filter fragment frame: hipriv "wlan0 rx_filter_frag 0/1" ����0��ʾ�رգ�1��ʾʹ�� */
    { "rx_filter_frag",        wal_hipriv_rx_filter_frag },
};

/* net_device�Ϲҽӵ�net_device_ops���� */
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
oal_net_device_ops_stru g_st_wal_net_dev_ops = {
    .ndo_get_stats = wal_netdev_get_stats,
    .ndo_open = wal_netdev_open,
    .ndo_stop = wal_netdev_stop,
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
    .ndo_start_xmit = wal_vap_start_xmit,
#else
    .ndo_start_xmit = wal_bridge_vap_xmit,
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 35)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#else
    .ndo_set_multicast_list = OAL_PTR_NULL,
#endif

    .ndo_do_ioctl = wal_net_device_ioctl,
    .ndo_change_mtu = oal_net_device_change_mtu,
    .ndo_init = oal_net_device_init,

#if (defined(_PRE_WLAN_FEATURE_FLOWCTL) || defined(_PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL))
    .ndo_select_queue = wal_netdev_select_queue,
#endif

    .ndo_set_mac_address = wal_netdev_set_mac_addr,
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
    .ndo_netif_notify = wal_netif_notify
#else
    .ndo_netif_notify = OAL_PTR_NULL
#endif
#endif
};
#elif (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
oal_net_device_ops_stru g_st_wal_net_dev_ops = {
    oal_net_device_init,
    wal_netdev_open,
    wal_netdev_stop,
    wal_bridge_vap_xmit,
    OAL_PTR_NULL,
    wal_netdev_get_stats,
    wal_net_device_ioctl,
    oal_net_device_change_mtu,
    wal_netdev_set_mac_addr,
    OAL_PTR_NULL
};
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)

oal_ethtool_ops_stru g_st_wal_ethtool_ops = { 0 };
#endif

/* ****************************************************************************
  ��׼ioctl�������.
**************************************************************************** */
OAL_STATIC OAL_CONST oal_iw_handler g_ast_iw_handlers[] = {
    OAL_PTR_NULL,                         /* SIOCSIWCOMMIT, */
    (oal_iw_handler)wal_ioctl_get_iwname, /* SIOCGIWNAME, */
    OAL_PTR_NULL,                         /* SIOCSIWNWID, */
    OAL_PTR_NULL,                         /* SIOCGIWNWID, */
    OAL_PTR_NULL,                         /* SIOCSIWFREQ, ����Ƶ��/�ŵ� */
    OAL_PTR_NULL,                         /* SIOCGIWFREQ, ��ȡƵ��/�ŵ� */
    OAL_PTR_NULL,                         /* SIOCSIWMODE, ����bss type */
    OAL_PTR_NULL,                         /* SIOCGIWMODE, ��ȡbss type */
    OAL_PTR_NULL,                         /* SIOCSIWSENS, */
    OAL_PTR_NULL,                         /* SIOCGIWSENS, */
    OAL_PTR_NULL,                         /* SIOCSIWRANGE, */     /* not used */
    OAL_PTR_NULL,                         /* SIOCGIWRANGE, */
    OAL_PTR_NULL,                         /* SIOCSIWPRIV, */      /* not used */
    OAL_PTR_NULL,                         /* SIOCGIWPRIV, */      /* kernel code */
    OAL_PTR_NULL,                         /* SIOCSIWSTATS, */     /* not used */
    OAL_PTR_NULL,                         /* SIOCGIWSTATS, */
    OAL_PTR_NULL,                         /* SIOCSIWSPY, */
    OAL_PTR_NULL,                         /* SIOCGIWSPY, */
    OAL_PTR_NULL,                         /* -- hole -- */
    OAL_PTR_NULL,                         /* -- hole -- */
    OAL_PTR_NULL,                         /* SIOCSIWAP, */
    (oal_iw_handler)wal_ioctl_get_apaddr, /* SIOCGIWAP, */
    OAL_PTR_NULL,                         /* SIOCSIWMLME, */
    OAL_PTR_NULL,                         /* SIOCGIWAPLIST, */
    OAL_PTR_NULL,                         /* SIOCSIWSCAN, */
    OAL_PTR_NULL,                         /* SIOCGIWSCAN, */
    OAL_PTR_NULL,                         /* SIOCSIWESSID, ����ssid */
    (oal_iw_handler)wal_ioctl_get_essid,  /* SIOCGIWESSID, ��ȡssid */
    OAL_PTR_NULL,                         /* SIOCSIWNICKN */
    OAL_PTR_NULL,                         /* SIOCGIWNICKN */
    OAL_PTR_NULL,                         /* -- hole -- */
    OAL_PTR_NULL,                         /* -- hole -- */
    OAL_PTR_NULL,                         /* SIOCSIWRATE */
    OAL_PTR_NULL,                         /* SIOCGIWRATE */
    OAL_PTR_NULL,                         /* SIOCSIWRTS */
    OAL_PTR_NULL,                         /* SIOCGIWRTS */
    OAL_PTR_NULL,                         /* SIOCSIWFRAG */
    OAL_PTR_NULL,                         /* SIOCGIWFRAG */
    OAL_PTR_NULL,                         /* SIOCSIWTXPOW, ���ô��书������ */
    OAL_PTR_NULL,                         /* SIOCGIWTXPOW, ���ô��书������ */
    OAL_PTR_NULL,                         /* SIOCSIWRETRY */
    OAL_PTR_NULL,                         /* SIOCGIWRETRY */
    OAL_PTR_NULL,                         /* SIOCSIWENCODE */
    OAL_PTR_NULL,                         /* SIOCGIWENCODE */
    OAL_PTR_NULL,                         /* SIOCSIWPOWER */
    OAL_PTR_NULL,                         /* SIOCGIWPOWER */
    OAL_PTR_NULL,                         /* -- hole -- */
    OAL_PTR_NULL,                         /* -- hole -- */
    OAL_PTR_NULL,                         /* SIOCSIWGENIE */
    OAL_PTR_NULL,                         /* SIOCGIWGENIE */
    OAL_PTR_NULL,                         /* SIOCSIWAUTH */
    OAL_PTR_NULL,                         /* SIOCGIWAUTH */
    OAL_PTR_NULL,                         /* SIOCSIWENCODEEXT */
    OAL_PTR_NULL                          /* SIOCGIWENCODEEXT */
};

/* ****************************************************************************
  ˽��ioctl����������嶨��
**************************************************************************** */
OAL_STATIC OAL_CONST oal_iw_priv_args_stru g_ast_iw_priv_args[] = {
    {WAL_IOCTL_PRIV_SET_AP_CFG, OAL_IW_PRIV_TYPE_CHAR |  256, 0, "AP_SET_CFG" },
    {WAL_IOCTL_PRIV_AP_MAC_FLTR, OAL_IW_PRIV_TYPE_CHAR | 256, OAL_IW_PRIV_TYPE_CHAR | OAL_IW_PRIV_SIZE_FIXED |
     0, "AP_SET_MAC_FLTR"},
    {WAL_IOCTL_PRIV_AP_GET_STA_LIST, 0, OAL_IW_PRIV_TYPE_CHAR | 1024, "AP_GET_STA_LIST"},
    {WAL_IOCTL_PRIV_AP_STA_DISASSOC, OAL_IW_PRIV_TYPE_CHAR | 256, OAL_IW_PRIV_TYPE_CHAR | 0, "AP_STA_DISASSOC"},
};

/* ****************************************************************************
  ˽��ioctl�������.
**************************************************************************** */
OAL_STATIC OAL_CONST oal_iw_handler g_ast_iw_priv_handlers[] = {
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+0 */  /* sub-ioctl set ��� */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+1 */  /* sub-ioctl get ��� */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+2 */  /* setkey */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+3 */  /* setwmmparams */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+4 */  /* delkey */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+5 */  /* getwmmparams */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+6 */  /* setmlme */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+7 */  /* getchaninfo */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+8 */  /* setcountry */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+9 */  /* getcountry */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+10 */  /* addmac */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+11 */  /* getscanresults */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+12 */  /* delmac */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+13 */  /* getchanlist */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+14 */  /* setchanlist */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+15 */  /* kickmac */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+16 */  /* chanswitch */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+17 */  /* ��ȡģʽ, ��: iwpriv vapN get_mode */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+18 */  /* ����ģʽ, ��: iwpriv vapN mode 11g */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+19 */  /* getappiebuf */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+20 */  /* null */
    (oal_iw_handler)wal_ioctl_get_assoc_list,           /* SIOCWFIRSTPRIV+21 */  /* APUTȡ�ù���STA�б� */
    (oal_iw_handler)wal_ioctl_set_mac_filters,          /* SIOCWFIRSTPRIV+22 */  /* APUT����STA���� */
    (oal_iw_handler)wal_ioctl_set_ap_config,            /* SIOCWFIRSTPRIV+23 */  /* ����APUT���� */
    (oal_iw_handler)wal_ioctl_set_ap_sta_disassoc,      /* SIOCWFIRSTPRIV+24 */  /* APUTȥ����STA */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+25 */  /* getStatistics */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+26 */  /* sendmgmt */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+27 */  /* null  */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+28 */  /* null */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+29 */  /* getaclmac */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+30 */  /* sethbrparams */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+29 */  /* getaclmac */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+30 */  /* sethbrparams */
    OAL_PTR_NULL,                                       /* SIOCWFIRSTPRIV+31 */  /* setrxtimeout */
};

/* ****************************************************************************
  ��������iw_handler_def����
**************************************************************************** */
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
oal_iw_handler_def_stru g_st_iw_handler_def = {
    .standard           = g_ast_iw_handlers,
    .num_standard       = OAL_ARRAY_SIZE(g_ast_iw_handlers),
#ifdef CONFIG_WEXT_PRIV
    .private            = g_ast_iw_priv_handlers,
    .num_private        = OAL_ARRAY_SIZE(g_ast_iw_priv_handlers),
    .private_args       = g_ast_iw_priv_args,
    .num_private_args   = OAL_ARRAY_SIZE(g_ast_iw_priv_args),
#endif
    .get_wireless_stats = OAL_PTR_NULL
};

#elif (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
oal_iw_handler_def_stru g_st_iw_handler_def = {
    g_ast_iw_handlers,                       /* ��׼ioctl handler */
    OAL_ARRAY_SIZE(g_ast_iw_handlers),
    OAL_ARRAY_SIZE(g_ast_iw_priv_handlers),
    { 0, 0 },                                /* �ֽڶ��� */
    OAL_ARRAY_SIZE(g_ast_iw_priv_args),
    g_ast_iw_priv_handlers,                  /* ˽��ioctl handler */
    g_ast_iw_priv_args,
    OAL_PTR_NULL
};
#endif

/* ****************************************************************************
  Э��ģʽ�ַ�������
**************************************************************************** */
OAL_CONST wal_ioctl_mode_map_stru g_ast_mode_map[] = {
#ifdef _PRE_WLAN_FEATURE_5G
    /* legacy */
    {"11a",         WLAN_LEGACY_11A_MODE,       WLAN_BAND_5G,   WLAN_BAND_WIDTH_20M},
#endif /* _PRE_WLAN_FEATURE_5G */
    {"11b",         WLAN_LEGACY_11B_MODE,       WLAN_BAND_2G,   WLAN_BAND_WIDTH_20M},
    {"11bg",        WLAN_MIXED_ONE_11G_MODE,    WLAN_BAND_2G,   WLAN_BAND_WIDTH_20M},
    {"11g",         WLAN_MIXED_TWO_11G_MODE,    WLAN_BAND_2G,   WLAN_BAND_WIDTH_20M},

    /* 11n */
    {"11ng20",      WLAN_HT_MODE,               WLAN_BAND_2G,   WLAN_BAND_WIDTH_20M},
#ifdef _PRE_WLAN_FEATURE_5G
    {"11na20",      WLAN_HT_MODE,               WLAN_BAND_5G,   WLAN_BAND_WIDTH_20M},
    {"11na40plus",  WLAN_HT_MODE,               WLAN_BAND_5G,   WLAN_BAND_WIDTH_40PLUS},
    {"11na40minus", WLAN_HT_MODE,               WLAN_BAND_5G,   WLAN_BAND_WIDTH_40MINUS},
#endif /* _PRE_WLAN_FEATURE_5G */
    {"11ng40plus",  WLAN_HT_MODE,               WLAN_BAND_2G,   WLAN_BAND_WIDTH_40PLUS},
    {"11ng40minus", WLAN_HT_MODE,               WLAN_BAND_2G,   WLAN_BAND_WIDTH_40MINUS},

#ifdef _PRE_WLAN_FEATURE_5G
    /* 11ac */
    {"11ac20",              WLAN_VHT_MODE,  WLAN_BAND_5G,   WLAN_BAND_WIDTH_20M},
    {"11ac40plus",          WLAN_VHT_MODE,  WLAN_BAND_5G,   WLAN_BAND_WIDTH_40PLUS},
    {"11ac40minus",         WLAN_VHT_MODE,  WLAN_BAND_5G,   WLAN_BAND_WIDTH_40MINUS},
    {"11ac80plusplus",      WLAN_VHT_MODE,  WLAN_BAND_5G,   WLAN_BAND_WIDTH_80PLUSPLUS},
    {"11ac80plusminus",     WLAN_VHT_MODE,  WLAN_BAND_5G,   WLAN_BAND_WIDTH_80PLUSMINUS},
    {"11ac80minusplus",     WLAN_VHT_MODE,  WLAN_BAND_5G,   WLAN_BAND_WIDTH_80MINUSPLUS},
    {"11ac80minusminus",    WLAN_VHT_MODE,  WLAN_BAND_5G,   WLAN_BAND_WIDTH_80MINUSMINUS},
#endif /* _PRE_WLAN_FEATURE_5G */

    {"11ac2g20",            WLAN_VHT_MODE,  WLAN_BAND_2G,   WLAN_BAND_WIDTH_20M},
    {"11ac2g40plus",        WLAN_VHT_MODE,  WLAN_BAND_2G,   WLAN_BAND_WIDTH_40PLUS},
    {"11ac2g40minus",       WLAN_VHT_MODE,  WLAN_BAND_2G,   WLAN_BAND_WIDTH_40MINUS},
    /* 11n only and 11ac only, ����20M���� */
    {"11nonly2g",           WLAN_HT_ONLY_MODE,   WLAN_BAND_2G,   WLAN_BAND_WIDTH_20M},
#ifdef _PRE_WLAN_FEATURE_5G
    {"11nonly5g",           WLAN_HT_ONLY_MODE,   WLAN_BAND_5G,   WLAN_BAND_WIDTH_20M},
    {"11aconly",            WLAN_VHT_ONLY_MODE,  WLAN_BAND_5G,   WLAN_BAND_WIDTH_20M},
#endif /* _PRE_WLAN_FEATURE_5G */
    {OAL_PTR_NULL}
};

/* ע��! ����Ĳ���������Ҫ�� g_dmac_config_set_dscr_param�еĺ���˳���ϸ�һ��! */
OAL_STATIC oal_int8   *pauc_tx_dscr_param_name[WAL_DSCR_PARAM_BUTT] = {
    "fbm",
    "pgl",
    "mtpgl",
    "sae",
    "ta",
    "ra",
    "cc",
    "data0",
    "data1",
    "data2",
    "data3",
    "power",
    "shortgi",
    "preamble",
    "rtscts",
    "lsigtxop",
    "smooth",
    "snding",
    "txbf",
    "stbc",
    "rd_ess",
    "dyn_bw",
    "dyn_bw_exist",
    "ch_bw_exist"
};

OAL_STATIC OAL_CONST oal_int8   *pauc_bw_tbl[WLAN_BAND_ASSEMBLE_AUTO] = {
    "20",
    "rsv1",
    "rsv2",
    "rsv3",
    "40",
    "d40",
    "rsv6",
    "rsv7",
    "80",
    "d80",
    "rsv10",
    "rsv11",
    "160",
    "d160",
    "rsv14",
    "80_80"
};

OAL_STATIC OAL_CONST oal_int8   *pauc_non_ht_rate_tbl[WLAN_LEGACY_RATE_VALUE_BUTT] = {
    "1",
    "2",
    "5.5",
    "11",
    "rsv0",
    "rsv1",
    "rsv2",
    "rsv3",
    "48",
    "24",
    "12",
    "6",
    "54",
    "36",
    "18",
    "9"
};

OAL_STATIC OAL_CONST wal_ioctl_alg_cfg_stru g_ast_alg_cfg_map[] = {
    {"sch_vi_ctrl_ena",         MAC_ALG_CFG_SCHEDULE_VI_CTRL_ENA},
    {"sch_bebk_minbw_ena",      MAC_ALG_CFG_SCHEDULE_BEBK_MIN_BW_ENA},
    {"sch_mvap_sch_ena",        MAC_ALG_CFG_SCHEDULE_MVAP_SCH_ENA},
    {"sch_vi_sch_ms",           MAC_ALG_CFG_SCHEDULE_VI_SCH_LIMIT},
    {"sch_vo_sch_ms",           MAC_ALG_CFG_SCHEDULE_VO_SCH_LIMIT},
    {"sch_vi_drop_ms",          MAC_ALG_CFG_SCHEDULE_VI_DROP_LIMIT},
    {"sch_vi_ctrl_ms",          MAC_ALG_CFG_SCHEDULE_VI_CTRL_MS},
    {"sch_vi_life_ms",          MAC_ALG_CFG_SCHEDULE_VI_MSDU_LIFE_MS},
    {"sch_vo_life_ms",          MAC_ALG_CFG_SCHEDULE_VO_MSDU_LIFE_MS},
    {"sch_be_life_ms",          MAC_ALG_CFG_SCHEDULE_BE_MSDU_LIFE_MS},
    {"sch_bk_life_ms",          MAC_ALG_CFG_SCHEDULE_BK_MSDU_LIFE_MS},
    {"sch_vi_low_delay",        MAC_ALG_CFG_SCHEDULE_VI_LOW_DELAY_MS},
    {"sch_vi_high_delay",       MAC_ALG_CFG_SCHEDULE_VI_HIGH_DELAY_MS},
    {"sch_cycle_ms",            MAC_ALG_CFG_SCHEDULE_SCH_CYCLE_MS},
    {"sch_ctrl_cycle_ms",       MAC_ALG_CFG_SCHEDULE_TRAFFIC_CTRL_CYCLE},
    {"sch_cir_nvip_kbps",       MAC_ALG_CFG_SCHEDULE_CIR_NVIP_KBPS},
    {"sch_cir_nvip_be",         MAC_ALG_CFG_SCHEDULE_CIR_NVIP_KBPS_BE},
    {"sch_cir_nvip_bk",         MAC_ALG_CFG_SCHEDULE_CIR_NVIP_KBPS_BK},
    {"sch_cir_vip_kbps",        MAC_ALG_CFG_SCHEDULE_CIR_VIP_KBPS},
    {"sch_cir_vip_be",          MAC_ALG_CFG_SCHEDULE_CIR_VIP_KBPS_BE},
    {"sch_cir_vip_bk",          MAC_ALG_CFG_SCHEDULE_CIR_VIP_KBPS_BK},
    {"sch_cir_vap_kbps",        MAC_ALG_CFG_SCHEDULE_CIR_VAP_KBPS},
    {"sch_sm_delay_ms",         MAC_ALG_CFG_SCHEDULE_SM_TRAIN_DELAY},
    {"sch_drop_pkt_limit",      MAC_ALG_CFG_VIDEO_DROP_PKT_LIMIT},
    {"sch_flowctl_ena",         MAC_ALG_CFG_FLOWCTRL_ENABLE_FLAG},
    {"sch_log_start",           MAC_ALG_CFG_SCHEDULE_LOG_START},
    {"sch_log_end",             MAC_ALG_CFG_SCHEDULE_LOG_END},
    {"sch_vap_prio",            MAC_ALG_CFG_SCHEDULE_VAP_SCH_PRIO},

    {"txbf_switch",             MAC_ALG_CFG_TXBF_MASTER_SWITCH},
    {"txbf_txmode_enb",         MAC_ALG_CFG_TXBF_TXMODE_ENABLE},
    {"txbf_bfer_enb",           MAC_ALG_CFG_TXBF_TXBFER_ENABLE},
    {"txbf_bfee_enb",           MAC_ALG_CFG_TXBF_TXBFEE_ENABLE},
    {"txbf_11nbfee_enb",        MAC_ALG_CFG_TXBF_11N_BFEE_ENABLE},
    {"txbf_txstbc_enb",         MAC_ALG_CFG_TXBF_TXSTBC_ENABLE},
    {"txbf_rxstbc_enb",         MAC_ALG_CFG_TXBF_RXSTBC_ENABLE},
    {"txbf_2g_bfer",            MAC_ALG_CFG_TXBF_2G_BFER_ENABLE},
    {"txbf_2nss_bfer",          MAC_ALG_CFG_TXBF_2NSS_BFER_ENABLE},
    {"txbf_fix_mode",           MAC_ALG_CFG_TXBF_FIX_MODE},
    {"txbf_fix_sound",          MAC_ALG_CFG_TXBF_FIX_SOUNDING},
    {"txbf_log_enb",            MAC_ALG_CFG_TXBF_LOG_ENABLE},
    {"txbf_log_sta",            MAC_ALG_CFG_TXBF_RECORD_LOG_START},
    {"txbf_log_out",            MAC_ALG_CFG_TXBF_LOG_OUTPUT},
    /* ������ر���������Ӧ�㷨: sh hipriv.sh "vap0 alg_cfg ar_enable [1|0]" */
    { "ar_enable",               MAC_ALG_CFG_AUTORATE_ENABLE },
    /* ������ر�ʹ���������: sh hipriv.sh "vap0 alg_cfg ar_use_lowest [1|0]" */
    { "ar_use_lowest",           MAC_ALG_CFG_AUTORATE_USE_LOWEST_RATE },
    /* ���ö���ͳ�Ƶİ���Ŀ:sh hipriv.sh "vap0 alg_cfg ar_short_num [����Ŀ]" */
    { "ar_short_num",            MAC_ALG_CFG_AUTORATE_SHORT_STAT_NUM },
    /* ���ö���ͳ�Ƶİ�λ��ֵ:sh hipriv.sh "vap0 alg_cfg ar_short_shift [λ��ֵ]" */
    { "ar_short_shift",          MAC_ALG_CFG_AUTORATE_SHORT_STAT_SHIFT },
    /* ���ó���ͳ�Ƶİ���Ŀ:sh hipriv.sh "vap0 alg_cfg ar_long_num [����Ŀ]" */
    { "ar_long_num",             MAC_ALG_CFG_AUTORATE_LONG_STAT_NUM },
    /* ���ó���ͳ�Ƶİ�λ��ֵ:sh hipriv.sh "vap0 alg_cfg ar_long_shift [λ��ֵ]" */
    { "ar_long_shift",           MAC_ALG_CFG_AUTORATE_LONG_STAT_SHIFT },
    /* ������С̽������:sh hipriv.sh "vap0 alg_cfg ar_min_probe_no [����Ŀ]" */
    { "ar_min_probe_no",         MAC_ALG_CFG_AUTORATE_MIN_PROBE_INTVL_PKTNUM },
    /* �������̽������:sh hipriv.sh "vap0 alg_cfg ar_max_probe_no [����Ŀ]" */
    { "ar_max_probe_no",         MAC_ALG_CFG_AUTORATE_MAX_PROBE_INTVL_PKTNUM },
    /* ����̽�������ִ���:sh hipriv.sh "vap0 alg_cfg ar_keep_times [����]" */
    { "ar_keep_times",           MAC_ALG_CFG_AUTORATE_PROBE_INTVL_KEEP_TIMES },
    /* ����goodputͻ������(ǧ�ֱȣ���300):sh hipriv.sh "vap0 alg_cfg ar_delta_ratio [ǧ�ֱ�]" */
    { "ar_delta_ratio",          MAC_ALG_CFG_AUTORATE_DELTA_GOODPUT_RATIO },
    /* ����vi��per����(ǧ�ֱȣ���300):sh hipriv.sh "vap0 alg_cfg ar_vi_per_limit [ǧ�ֱ�]" */
    { "ar_vi_per_limit",         MAC_ALG_CFG_AUTORATE_VI_PROBE_PER_LIMIT },
    /* ����vo��per����(ǧ�ֱȣ���300):sh hipriv.sh "vap0 alg_cfg ar_vo_per_limit [ǧ�ֱ�]" */
    { "ar_vo_per_limit",         MAC_ALG_CFG_AUTORATE_VO_PROBE_PER_LIMIT },
    /* ����ampdu��durattionֵ:sh hipriv.sh "vap0 alg_cfg ar_ampdu_time [ʱ��ֵ]" */
    { "ar_ampdu_time",           MAC_ALG_CFG_AUTORATE_AMPDU_DURATION },
    /* ����mcs0�Ĵ���ʧ������:sh hipriv.sh "vap0 alg_cfg ar_cont_loss_num [����Ŀ]" */
    { "ar_cont_loss_num",        MAC_ALG_CFG_AUTORATE_MCS0_CONT_LOSS_NUM },
    /* ��������11b��rssi����:sh hipriv.sh "vap0 alg_cfg ar_11b_diff_rssi [��ֵ]" */
    { "ar_11b_diff_rssi",        MAC_ALG_CFG_AUTORATE_UP_PROTOCOL_DIFF_RSSI },
    /*
     * ����rtsģʽ:sh hipriv.sh "vap0 alg_cfg ar_rts_mode [0(������)|1(����)|2(rate[0]��̬RTS,
     * rate[1..3]����RTS)|3(rate[0]����RTS, rate[1..3]����RTS)]"
     */
    { "ar_rts_mode",             MAC_ALG_CFG_AUTORATE_RTS_MODE },
    /* ����Legacy�װ�����������:sh hipriv.sh "vap0 alg_cfg ar_legacy_loss [��ֵ]" */
    { "ar_legacy_loss",          MAC_ALG_CFG_AUTORATE_LEGACY_1ST_LOSS_RATIO_TH },
    /* ����Legacy�װ�����������:sh hipriv.sh "vap0 alg_cfg ar_ht_vht_loss [��ֵ]" */
    { "ar_ht_vht_loss",          MAC_ALG_CFG_AUTORATE_HT_VHT_1ST_LOSS_RATIO_TH },
    /*
     * ��ʼ����ͳ����־:sh hipriv.sh "vap0 alg_cfg ar_stat_log_do [mac��ַ] [ҵ�����] [����Ŀ]"
     * ��: sh hipriv.sh "vap0 alg_cfg ar_stat_log_do 06:31:04:E3:81:02 1 1000"
     */
    { "ar_stat_log_do",          MAC_ALG_CFG_AUTORATE_STAT_LOG_START },
    /*
     * ��ʼ����ѡ����־:sh hipriv.sh "vap0 alg_cfg ar_sel_log_do [mac��ַ] [ҵ�����] [����Ŀ]"
     * ��: sh hipriv.sh "vap0 alg_cfg ar_sel_log_do 06:31:04:E3:81:02 1 200"
     */
    { "ar_sel_log_do",           MAC_ALG_CFG_AUTORATE_SELECTION_LOG_START },
    /*
     * ��ʼ�̶�������־:sh hipriv.sh "vap0 alg_cfg ar_fix_log_do [mac��ַ] [tidno] [per����]"
     * ��: sh hipriv.sh "vap0 alg_cfg ar_sel_log_do 06:31:04:E3:81:02 1 200"
     */
    { "ar_fix_log_do",           MAC_ALG_CFG_AUTORATE_FIX_RATE_LOG_START },
    /*
     * ��ʼ�ۺ�����Ӧ��־:sh hipriv.sh "vap0 alg_cfg ar_fix_log_do [mac��ַ] [tidno]"
     * ��: sh hipriv.sh "vap0 alg_cfg ar_sel_log_do 06:31:04:E3:81:02 1 "
     */
    { "ar_aggr_log_do",          MAC_ALG_CFG_AUTORATE_AGGR_STAT_LOG_START },
    /* ��ӡ����ͳ����־:sh hipriv.sh "vap0 alg_cfg ar_st_log_out 06:31:04:E3:81:02" */
    { "ar_st_log_out",           MAC_ALG_CFG_AUTORATE_STAT_LOG_WRITE },
    /* ��ӡ����ѡ����־:sh hipriv.sh "vap0 alg_cfg ar_sel_log_out 06:31:04:E3:81:02" */
    { "ar_sel_log_out",          MAC_ALG_CFG_AUTORATE_SELECTION_LOG_WRITE },
    /* ��ӡ�̶�������־:sh hipriv.sh "vap0 alg_cfg ar_fix_log_out 06:31:04:E3:81:02" */
    { "ar_fix_log_out",          MAC_ALG_CFG_AUTORATE_FIX_RATE_LOG_WRITE },
    /* ��ӡ�̶�������־:sh hipriv.sh "vap0 alg_cfg ar_fix_log_out 06:31:04:E3:81:02" */
    { "ar_aggr_log_out",         MAC_ALG_CFG_AUTORATE_AGGR_STAT_LOG_WRITE },
    /* ��ӡ���ʼ���:sh hipriv.sh "vap0 alg_cfg ar_disp_rateset 06:31:04:E3:81:02" */
    { "ar_disp_rateset",         MAC_ALG_CFG_AUTORATE_DISPLAY_RATE_SET },
    /* ���ù̶�����:sh hipriv.sh "vap0 alg_cfg ar_cfg_fix_rate 06:31:04:E3:81:02 0" */
    { "ar_cfg_fix_rate",         MAC_ALG_CFG_AUTORATE_CONFIG_FIX_RATE },
    /* ��ӡ�������ʼ���:sh hipriv.sh "vap0 alg_cfg ar_disp_rx_rate 06:31:04:E3:81:02" */
    { "ar_disp_rx_rate",         MAC_ALG_CFG_AUTORATE_DISPLAY_RX_RATE },
    /* ������ر���������Ӧ��־: sh hipriv.sh "vap0 alg_cfg ar_log_enable [1|0]" */
    { "ar_log_enable",           MAC_ALG_CFG_AUTORATE_LOG_ENABLE },
    /* ��������VO����: sh hipriv.sh "vap0 alg_cfg ar_max_vo_rate [����ֵ]" */
    { "ar_max_vo_rate",          MAC_ALG_CFG_AUTORATE_VO_RATE_LIMIT },
    /* ������˥����per����ֵ: sh hipriv.sh "vap0 alg_cfg ar_fading_per_th [per����ֵ(ǧ����)]" */
    { "ar_fading_per_th",        MAC_ALG_CFG_AUTORATE_JUDGE_FADING_PER_TH },
    /* ���þۺ�����Ӧ����: sh hipriv.sh "vap0 alg_cfg ar_aggr_opt [1|0]" */
    { "ar_aggr_opt",             MAC_ALG_CFG_AUTORATE_AGGR_OPT },
    /* ���þۺ�����Ӧ̽����: sh hipriv.sh "vap0 alg_cfg ar_aggr_pb_intvl [̽����]" */
    { "ar_aggr_pb_intvl",        MAC_ALG_CFG_AUTORATE_AGGR_PROBE_INTVL_NUM },
    /* ���þۺ�����Ӧͳ����λֵ: sh hipriv.sh "vap0 alg_cfg ar_aggr_st_shift [ͳ����λֵ]" */
    { "ar_aggr_st_shift",        MAC_ALG_CFG_AUTORATE_AGGR_STAT_SHIFT },
    /* ����DBACģʽ�µ����ۺ�ʱ��: sh hipriv.sh "vap0 alg_cfg ar_dbac_aggrtime [���ۺ�ʱ��(us)]" */
    { "ar_dbac_aggrtime",        MAC_ALG_CFG_AUTORATE_DBAC_AGGR_TIME },
    /* ���õ����õ�VI״̬: sh hipriv.sh "vap0 alg_cfg ar_dbg_vi_status [0/1/2]" */
    { "ar_dbg_vi_status",        MAC_ALG_CFG_AUTORATE_DBG_VI_STATUS },
    /* �ۺ�����Ӧlog����: sh hipriv.sh "vap0 alg_cfg ar_dbg_aggr_log [0/1]" */
    { "ar_dbg_aggr_log",         MAC_ALG_CFG_AUTORATE_DBG_AGGR_LOG },
    /* �������ʱ仯ʱ�����оۺ�̽��ı�����: sh hipriv.sh "vap0 alg_cfg ar_aggr_pck_num [������]" */
    { "ar_aggr_pck_num",         MAC_ALG_CFG_AUTORATE_AGGR_NON_PROBE_PCK_NUM },
    /* ��С�ۺ�ʱ������: sh hipriv.sh "vap0 alg_cfg ar_aggr_min_idx [����ֵ]" */
    { "ar_min_aggr_idx",         MAC_ALG_CFG_AUTORATE_AGGR_MIN_AGGR_TIME_IDX },
    /* �������ۺ���Ŀ: sh hipriv.sh "vap0 alg_cfg ar_max_aggr_num [�ۺ���Ŀ]" */
    { "ar_max_aggr_num",         MAC_ALG_CFG_AUTORATE_MAX_AGGR_NUM },
    /* ������ͽ�MCS���ƾۺ�Ϊ1��PER����: sh hipriv.sh "vap0 alg_cfg ar_1mpdu_per_th [per����ֵ(ǧ����)]" */
    { "ar_1mpdu_per_th",         MAC_ALG_CFG_AUTORATE_LIMIT_1MPDU_PER_TH },

    /* ������رչ���̽�����: sh hipriv.sh "vap0 alg_cfg ar_btcoxe_probe [1|0]" */
    { "ar_btcoxe_probe",         MAC_ALG_CFG_AUTORATE_BTCOEX_PROBE_ENABLE },
    /* ������رչ���ۺϻ���: sh hipriv.sh "vap0 alg_cfg ar_btcoxe_aggr [1|0]" */
    { "ar_btcoxe_aggr",          MAC_ALG_CFG_AUTORATE_BTCOEX_AGGR_ENABLE },
    /* ���ù���ͳ��ʱ��������: sh hipriv.sh "vap0 alg_cfg ar_coxe_intvl [ͳ������ms]" */
    { "ar_coxe_intvl",           MAC_ALG_CFG_AUTORATE_COEX_STAT_INTVL },
    /* ���ù���abort�ͱ������޲���: sh hipriv.sh "vap0 alg_cfg ar_coxe_low_th [ǧ����]" */
    { "ar_coxe_low_th",          MAC_ALG_CFG_AUTORATE_COEX_LOW_ABORT_TH },
    /* ���ù���abort�߱������޲���: sh hipriv.sh "vap0 alg_cfg ar_coxe_high_th [ǧ����]" */
    { "ar_coxe_high_th",         MAC_ALG_CFG_AUTORATE_COEX_HIGH_ABORT_TH },
    /* ���ù���ۺ���ĿΪ1�����޲���: sh hipriv.sh "vap0 alg_cfg ar_coxe_agrr_th [ǧ����]" */
    { "ar_coxe_agrr_th",         MAC_ALG_CFG_AUTORATE_COEX_AGRR_NUM_ONE_TH },

    /* ��̬��������ʹ�ܿ���: sh hipriv.sh "vap0 alg_cfg ar_dyn_bw_en [0/1]" */
    { "ar_dyn_bw_en",            MAC_ALG_CFG_AUTORATE_DYNAMIC_BW_ENABLE },
    /* �����������Ż�����: sh hipriv.sh "vap0 alg_cfg ar_thpt_wave_opt [0/1]" */
    { "ar_thpt_wave_opt",        MAC_ALG_CFG_AUTORATE_THRPT_WAVE_OPT },
    /*
     * �����ж�������������goodput�����������(ǧ����):
     * sh hipriv.sh "vap0 alg_cfg ar_gdpt_diff_th [goodput����������(ǧ����)]"
     */
    { "ar_gdpt_diff_th",         MAC_ALG_CFG_AUTORATE_GOODPUT_DIFF_TH },
    /* �����ж�������������PER��������(ǧ����): sh hipriv.sh "vap0 alg_cfg ar_per_worse_th [PER�������(ǧ����)]" */
    { "ar_per_worse_th",         MAC_ALG_CFG_AUTORATE_PER_WORSE_TH },
    /* ���÷�RTS�յ�CTS����DATA������BA�ķ�������жϴ�������: sh hipriv.sh "vap0 alg_cfg ar_cts_no_ba_num [����]" */
    { "ar_cts_no_ack_num",       MAC_ALG_CFG_AUTORATE_RX_CTS_NO_BA_NUM },
    /* �����Ƿ�֧��voiceҵ��ۺ�: sh hipriv.sh "vap0 alg_cfg ar_vo_aggr [0/1]" */
    { "ar_vo_aggr",              MAL_ALG_CFG_AUTORATE_VOICE_AGGR },
    /*
     * ���ÿ���ƽ��ͳ�Ƶ�ƽ������ƫ����:
     * sh hipriv.sh "vap0 alg_cfg ar_fast_smth_shft [ƫ����]" (ȡ255��ʾȡ������ƽ��)
     */
    { "ar_fast_smth_shft",       MAC_ALG_CFG_AUTORATE_FAST_SMOOTH_SHIFT },
    /* ���ÿ���ƽ��ͳ�Ƶ���С�ۺ���Ŀ����: sh hipriv.sh "vap0 alg_cfg ar_fast_smth_aggr_num [��С�ۺ���Ŀ]" */
    { "ar_fast_smth_aggr_num",   MAC_ALG_CFG_AUTORATE_FAST_SMOOTH_AGGR_NUM },
    /* ����short GI�ͷ���PER����ֵ(ǧ����): sh hipriv.sh "vap0 alg_cfg ar_sgi_punish_per [PER����ֵ(ǧ����)]" */
    { "ar_sgi_punish_per",       MAC_ALG_CFG_AUTORATE_SGI_PUNISH_PER },
    /* ����short GI�ͷ��ĵȴ�̽����Ŀ: sh hipriv.sh "vap0 alg_cfg ar_sgi_punish_num [�ȴ�̽����Ŀ]" */
    { "ar_sgi_punish_num",       MAC_ALG_CFG_AUTORATE_SGI_PUNISH_NUM },

    {"sm_train_num",            MAC_ALG_CFG_SMARTANT_TRAINING_PACKET_NUMBER},

    /* ������������non-directʹ��: sh hipriv.sh "vap0 alg_cfg anti_inf_imm_en 0|1" */
    { "anti_inf_imm_en",         MAC_ALG_CFG_ANTI_INTF_IMM_ENABLE },
    /* ������������dynamic unlockʹ��: sh hipriv.sh "vap0 alg_cfg anti_inf_unlock_en 0|1" */
    { "anti_inf_unlock_en",      MAC_ALG_CFG_ANTI_INTF_UNLOCK_ENABLE },
    /* ������������rssiͳ������: sh hipriv.sh "vap0 anti_inf_stat_time [time]" */
    { "anti_inf_stat_time",      MAC_ALG_CFG_ANTI_INTF_RSSI_STAT_CYCLE },
    /* ������������unlock�ر�����: sh hipriv.sh "vap0 anti_inf_off_time [time]" */
    { "anti_inf_off_time",       MAC_ALG_CFG_ANTI_INTF_UNLOCK_CYCLE },
    /* ������������unlock�رճ���ʱ��: sh hipriv.sh "vap0 anti_inf_off_dur [time]" */
    { "anti_inf_off_dur",        MAC_ALG_CFG_ANTI_INTF_UNLOCK_DUR_TIME },
    /* ������nav����ʹ��: sh hipriv.sh "vap0 alg_cfg anti_inf_nav_en 0|1" */
    { "anti_inf_nav_en",         MAC_ALG_CFG_ANTI_INTF_NAV_IMM_ENABLE },
    /* ����������goodput�½�����: sh hipriv.sh "vap0 alg_cfg anti_inf_gd_th [num]" */
    { "anti_inf_gd_th",          MAC_ALG_CFG_ANTI_INTF_GOODPUT_FALL_TH },
    /* ����������̽�Ᵽ�����������: sh hipriv.sh "vap0 alg_cfg anti_inf_keep_max [num]" */
    { "anti_inf_keep_max",       MAC_ALG_CFG_ANTI_INTF_KEEP_CYC_MAX_NUM },
    /* ����������̽�Ᵽ�����������: sh hipriv.sh "vap0 alg_cfg anti_inf_keep_min [num]" */
    { "anti_inf_keep_min",       MAC_ALG_CFG_ANTI_INTF_KEEP_CYC_MIN_NUM },
    /* �����������Ƿ�ʹ��tx time̽��: sh hipriv.sh "vap0 anti_inf_tx_pro_en 0|1" */
    { "anti_inf_per_pro_en",     MAC_ALG_CFG_ANTI_INTF_PER_PROBE_EN },
    /* tx time�½�����: sh hipriv.sh "vap0 alg_cfg anti_inf_txtime_th [val]" */
    { "anti_inf_txtime_th",      MAC_ALG_CFG_ANTI_INTF_TX_TIME_FALL_TH },
    /* per�½�����: sh hipriv.sh "vap0 alg_cfg anti_inf_per_th [val]" */
    { "anti_inf_per_th",         MAC_ALG_CFG_ANTI_INTF_PER_FALL_TH },
    /* goodput��������: sh hipriv.sh "vap0 alg_cfg anti_inf_gd_jitter_th [val]" */
    { "anti_inf_gd_jitter_th",   MAC_ALG_CFG_ANTI_INTF_GOODPUT_JITTER_TH },

    /* ����������debug�Ĵ�ӡ��Ϣ: sh hipriv.sh "vap0 alg_cfg anti_inf_debug_mode 0|1|2" */
    { "anti_inf_debug_mode",     MAC_ALG_CFG_ANTI_INTF_DEBUG_MODE },
    /* ͬƵ���ż������: sh hipriv.sh "vap0 alg_cfg edca_opt_co_ch_time [time]" */
    { "edca_opt_co_ch_time",     MAC_ALG_CFG_EDCA_OPT_CO_CH_DET_CYCLE },
    /* apģʽ��edca�Ż�ʹ��ģʽ: sh hipriv.sh "vap0 alg_cfg edca_opt_en_ap 0|1|2" */
    { "edca_opt_en_ap",          MAC_ALG_CFG_EDCA_OPT_AP_EN_MODE },
    /* staģʽ��edca�Ż�ʹ��ģʽ: sh hipriv.sh "vap0 alg_cfg edca_opt_en_sta 0|1" */
    { "edca_opt_en_sta",         MAC_ALG_CFG_EDCA_OPT_STA_EN },
    /* staģʽ��edca�Ż���weightingϵ��: sh hipriv.sh "vap0 alg_cfg edca_opt_sta_weight 0~3" */
    { "edca_opt_sta_weight",     MAC_ALG_CFG_EDCA_OPT_STA_WEIGHT },
    /* non-direct��ռ�ձ����� sh hipriv.sh "vap0 alg_cfg edca_opt_nondir_th [val]" */
    { "edca_opt_nondir_th",      MAC_ALG_CFG_EDCA_OPT_NONDIR_TH },
    /* apģʽ��UDPҵ���Ӧ���б����� sh hipriv.sh "vap0 alg_cfg edca_opt_th_udp [val]" */
    { "edca_opt_th_udp",         MAC_ALG_CFG_EDCA_OPT_TH_UDP },
    /* apģʽ��tcPҵ���Ӧ���б����� sh hipriv.sh "vap0 alg_cfg edca_opt_th_tcp [val]" */
    { "edca_opt_th_tcp",         MAC_ALG_CFG_EDCA_OPT_TH_TCP },
    { "edca_opt_debug_mode",     MAC_ALG_CFG_EDCA_OPT_DEBUG_MODE },       /* �Ƿ��ӡ�����Ϣ�������ڱ��ذ汾���� */

    /* CCA�Ż�����ʹ��: sh hipriv.sh "vap0 alg_cfg cca_opt_alg_en_mode 0|1" */
    { "cca_opt_alg_en_mode",         MAC_ALG_CFG_CCA_OPT_ALG_EN_MODE },
    /* CCA�Ż�DEBUGģʽ����: sh hipriv.sh "vap0 alg_cfg cca_opt_debug_mode 0|1" */
    { "cca_opt_debug_mode",          MAC_ALG_CFG_CCA_OPT_DEBUG_MODE },
    /* CCA�Ż�T1��ʱ����:sh hipriv.sh "vap0 alg_cfg cca_opt_set_t1_counter_time [time]" */
    { "cca_opt_set_t1_counter_time", MAC_ALG_CFG_CCA_OPT_SET_T1_COUNTER_TIME },
    /* CCA�Ż�T2��ʱ����:sh hipriv.sh "vap0 alg_cfg cca_opt_set_t2_counter_time [time]" */
    { "cca_opt_set_t2_counter_time", MAC_ALG_CFG_CCA_OPT_SET_T2_COUNTER_TIME },
    /* CCA�Ż��ж��Ƿ����ƽ��RSSI�Ŀ��й��ʷ�0ֵ�Ĵ�������:sh hipriv.sh "vap0 alg_cfg cca_opt_set_ilde_cnt_th [val]" */
    { "cca_opt_set_ilde_cnt_th",     MAC_ALG_CFG_CCA_OPT_SET_ILDE_CNT_TH },
    /* CCA�Ż��ж��Ƿ����ǿ��Ƶ����Ƶ���ŵķ�æ����ֵ:sh hipriv.sh "vap0 alg_cfg cca_opt_set_duty_cyc_th [val]" */
    { "cca_opt_set_duty_cyc_th",     MAC_ALG_CFG_CCA_OPT_SET_DUTY_CYC_TH },
    /* CCA�Ż��ж��Ƿ������Ƶ����Ƶ���ŵ�sync error��ֵ:sh hipriv.sh "vap0 alg_cfg cca_opt_set_aveg_rssi_th [val]" */
    { "cca_opt_set_aveg_rssi_th",    MAC_ALG_CFG_CCA_OPT_SET_AVEG_RSSI_TH },
    /*
     * CCA�Ż��ж��Ƿ������Ƶ����Ƶ���ŵ�pri20/40/80�������ֵ:
     * sh hipriv.sh "vap0 alg_cfg cca_opt_set_chn_scan_cyc [val]"
     */
    { "cca_opt_set_chn_scan_cyc",    MAC_ALG_CFG_CCA_OPT_SET_CHN_SCAN_CYC },
    /* CCA�Ż��ŵ�ɨ���ʱ��(ms):sh hipriv.sh "vap0 alg_cfg cca_opt_set_sync_err_th [time]" */
    { "cca_opt_set_sync_err_th",     MAC_ALG_CFG_CCA_OPT_SET_SYNC_ERR_TH },
    /* CCA�Ż��ŵ�ɨ���ʱ��(ms):sh hipriv.sh "vap0 alg_cfg cca_opt_set_sync_err_th [time]" */
    { "cca_opt_set_cca_th_debug",    MAC_ALG_CFG_CCA_OPT_SET_CCA_TH_DEBUG },
    /* CCA log���� sh hipriv.sh "vap0 alg_cfg cca_opt_log 0|1" */
    { "cca_opt_log",                 MAC_ALG_CFG_CCA_OPT_LOG },
    /* ��ʼͳ����־:sh hipriv.sh "vap0 alg_cca_opt_log cca_opt_stat_log_do [val]"  */
    { "cca_opt_stat_log_do",         MAC_ALG_CFG_CCA_OPT_STAT_LOG_START },
    /* ��ӡͳ����־:sh hipriv.sh "vap0 alg_cca_opt_log cca_opt_stat_log_out" */
    { "cca_opt_stat_log_out",        MAC_ALG_CFG_CCA_OPT_STAT_LOG_WRITE },
    /* CCA����������ײ������(ms):sh hipriv.sh "vap0 alg_cfg cca_opt_set_collision_ratio_th [val]" */
    { "cca_opt_set_collision_ratio_th", MAC_ALG_CFG_CCA_OPT_SET_COLLISION_RATIO_TH },
    /* CCA��������goddput����(ms):sh hipriv.sh "vap0 alg_cfg cca_opt_set_goodput_loss_th [val]" */
    { "cca_opt_set_goodput_loss_th", MAC_ALG_CFG_CCA_OPT_SET_GOODPUT_LOSS_TH },
    /* CCA�����������̽����(ms):sh hipriv.sh "vap0 alg_cfg cca_opt_set_max_intvl_num [val]" */
    { "cca_opt_set_max_intvl_num",   MAC_ALG_CFG_CCA_OPT_SET_MAX_INTVL_NUM },
    /* CCA�޸��ż���������(ms):sh hipriv.sh "vap0 alg_cfg cca_opt_set_non_intf_cyc_num [val]" */
    { "cca_opt_set_non_intf_cyc_num",       MAC_ALG_CFG_CCA_OPT_NON_INTF_CYCLE_NUM_TH },
    /* CCA�޸��ż���������(ms):sh hipriv.sh "vap0 alg_cfg cca_opt_set_non_intf_duty_cyc_th [val]" */
    { "cca_opt_set_non_intf_duty_cyc_th",   MAC_ALG_CFG_CCA_OPT_NON_INTF_DUTY_CYC_TH },

    { "tpc_mode",                MAC_ALG_CFG_TPC_MODE },                          /* ����TPC����ģʽ */
    { "tpc_dbg",                 MAC_ALG_CFG_TPC_DEBUG },                         /* ����TPC��debug���� */
    /* ����TPC���ʵȼ�(0,1,2,3), �ڹ̶�����ģʽ��ʹ�� */
    { "tpc_pow_lvl",             MAC_ALG_CFG_TPC_POWER_LEVEL },
    /* ����TPC��log����:sh hipriv.sh "vap0 alg_cfg tpc_log 1 */
    { "tpc_log",                 MAC_ALG_CFG_TPC_LOG },
    /*
     * ��ʼ����ͳ����־:sh hipriv.sh "vap0 alg_tpc_log tpc_stat_log_do [mac��ַ] [ҵ�����] [����Ŀ]"
     * ��: sh hipriv.sh "vap0 alg_tpc_log tpc_stat_log_do 06:31:04:E3:81:02 1 1000"
     */
    { "tpc_stat_log_do",         MAC_ALG_CFG_TPC_STAT_LOG_START },
    /* ��ӡ����ͳ����־:sh hipriv.sh "vap0 alg_tpc_log tpc_stat_log_out 06:31:04:E3:81:02" */
    { "tpc_stat_log_out",        MAC_ALG_CFG_TPC_STAT_LOG_WRITE },
    /*
     * ��ʼÿ��ͳ����־:sh hipriv.sh "vap0 alg_tpc_log tpc_pkt_log_do [mac��ַ] [ҵ�����] [����Ŀ]"
     * ��: sh hipriv.sh "vap0 alg_tpc_log tpc_pkt_log_do 06:31:04:E3:81:02 1 1000"
     */
    { "tpc_pkt_log_do",          MAC_ALG_CFG_TPC_PER_PKT_LOG_START },
    /* ��ȡ����֡����:sh hipriv.sh "vap0 alg_tpc_log tpc_get_frame_pow beacon_pow" */
    { "tpc_get_frame_pow",       MAC_ALG_CFG_TPC_GET_FRAME_POW },
    { "tpc_mag_frm_pow_lvl",     MAC_ALG_CFG_TPC_MANAGEMENT_MCAST_FRM_POWER_LEVEL }, /* TPC����֡�Ͷಥ֡���ʵȼ� */
    { "tpc_ctl_frm_pow_lvl",     MAC_ALG_CFG_TPC_CONTROL_FRM_POWER_LEVEL },          /* TPC����֡���ʵȼ� */
    { "tpc_reset_stat",          MAC_ALG_CFG_TPC_RESET_STAT },                    /* �ͷ�ͳ���ڴ� */
    { "tpc_reset_pkt",           MAC_ALG_CFG_TPC_RESET_PKT },                     /* �ͷ�ÿ���ڴ� */
    { "tpc_over_temp_th",        MAC_ALG_CFG_TPC_OVER_TMP_TH },                   /* TPC�������� */
    { "tpc_dpd_enable_rate",     MAC_ALG_CFG_TPC_DPD_ENABLE_RATE },               /* ����DPD��Ч������INDEX */
    { "tpc_target_rate_11b",     MAC_ALG_CFG_TPC_TARGET_RATE_11B },               /* 11bĿ���������� */
    { "tpc_target_rate_11ag",    MAC_ALG_CFG_TPC_TARGET_RATE_11AG },              /* 11agĿ���������� */
    { "tpc_target_rate_11n20",   MAC_ALG_CFG_TPC_TARGET_RATE_HT40 },              /* 11n20Ŀ���������� */
    { "tpc_target_rate_11n40",   MAC_ALG_CFG_TPC_TARGET_RATE_HT40 },              /* 11n40Ŀ���������� */
    { "tpc_target_rate_11ac20",  MAC_ALG_CFG_TPC_TARGET_RATE_VHT20 },             /* 11ac20Ŀ���������� */
    { "tpc_target_rate_11ac40",  MAC_ALG_CFG_TPC_TARGET_RATE_VHT40 },             /* 11ac40Ŀ���������� */
    { "tpc_target_rate_11ac80",  MAC_ALG_CFG_TPC_TARGET_RATE_VHT80 },             /* 11ac80Ŀ���������� */
    /* ��ӡTPC����־��Ϣ:sh hipriv.sh "vap0 alg_cfg tpc_show_log_info */
    { "tpc_show_log_info",       MAC_ALG_CFG_TPC_SHOW_LOG_INFO },
    { "tpc_no_margin_pow",       MAC_ALG_CFG_TPC_NO_MARGIN_POW },                 /* 51����û���������� */
    /* tx power�ڴ��ڲ�ƽ̹��tpc���й���������Ĭ��Ϊ0 */
    { "tpc_power_amend",         MAC_ALG_CFG_TPC_POWER_AMEND },

    {OAL_PTR_NULL}
};
#ifdef _PRE_WLAN_FEATURE_DFS
OAL_CONST wal_dfs_domain_entry_stru g_ast_dfs_domain_table[] = {
    {"AE", MAC_DFS_DOMAIN_ETSI},
    {"AL", MAC_DFS_DOMAIN_NULL},
    {"AM", MAC_DFS_DOMAIN_ETSI},
    {"AN", MAC_DFS_DOMAIN_ETSI},
    {"AR", MAC_DFS_DOMAIN_FCC},
    {"AT", MAC_DFS_DOMAIN_ETSI},
    {"AU", MAC_DFS_DOMAIN_FCC},
    {"AZ", MAC_DFS_DOMAIN_ETSI},
    {"BA", MAC_DFS_DOMAIN_ETSI},
    {"BE", MAC_DFS_DOMAIN_ETSI},
    {"BG", MAC_DFS_DOMAIN_ETSI},
    {"BH", MAC_DFS_DOMAIN_ETSI},
    {"BL", MAC_DFS_DOMAIN_NULL},
    {"BN", MAC_DFS_DOMAIN_ETSI},
    {"BO", MAC_DFS_DOMAIN_ETSI},
    {"BR", MAC_DFS_DOMAIN_FCC},
    {"BY", MAC_DFS_DOMAIN_ETSI},
    {"BZ", MAC_DFS_DOMAIN_ETSI},
    {"CA", MAC_DFS_DOMAIN_FCC},
    {"CH", MAC_DFS_DOMAIN_ETSI},
    {"CL", MAC_DFS_DOMAIN_NULL},
    {"CN", MAC_DFS_DOMAIN_NULL},
    {"CO", MAC_DFS_DOMAIN_FCC},
    {"CR", MAC_DFS_DOMAIN_FCC},
    {"CS", MAC_DFS_DOMAIN_ETSI},
    {"CY", MAC_DFS_DOMAIN_ETSI},
    {"CZ", MAC_DFS_DOMAIN_ETSI},
    {"DE", MAC_DFS_DOMAIN_ETSI},
    {"DK", MAC_DFS_DOMAIN_ETSI},
    {"DO", MAC_DFS_DOMAIN_FCC},
    {"DZ", MAC_DFS_DOMAIN_NULL},
    {"EC", MAC_DFS_DOMAIN_FCC},
    {"EE", MAC_DFS_DOMAIN_ETSI},
    {"EG", MAC_DFS_DOMAIN_ETSI},
    {"ES", MAC_DFS_DOMAIN_ETSI},
    {"FI", MAC_DFS_DOMAIN_ETSI},
    {"FR", MAC_DFS_DOMAIN_ETSI},
    {"GB", MAC_DFS_DOMAIN_ETSI},
    {"GE", MAC_DFS_DOMAIN_ETSI},
    {"GR", MAC_DFS_DOMAIN_ETSI},
    {"GT", MAC_DFS_DOMAIN_FCC},
    {"HK", MAC_DFS_DOMAIN_FCC},
    {"HN", MAC_DFS_DOMAIN_FCC},
    {"HR", MAC_DFS_DOMAIN_ETSI},
    {"HU", MAC_DFS_DOMAIN_ETSI},
    {"ID", MAC_DFS_DOMAIN_NULL},
    {"IE", MAC_DFS_DOMAIN_ETSI},
    {"IL", MAC_DFS_DOMAIN_ETSI},
    {"IN", MAC_DFS_DOMAIN_NULL},
    {"IQ", MAC_DFS_DOMAIN_NULL},
    {"IR", MAC_DFS_DOMAIN_NULL},
    {"IS", MAC_DFS_DOMAIN_ETSI},
    {"IT", MAC_DFS_DOMAIN_ETSI},
    {"JM", MAC_DFS_DOMAIN_FCC},
    {"JO", MAC_DFS_DOMAIN_ETSI},
    {"JP", MAC_DFS_DOMAIN_MKK},
    {"KP", MAC_DFS_DOMAIN_NULL},
    {"KR", MAC_DFS_DOMAIN_KOREA},
    {"KW", MAC_DFS_DOMAIN_ETSI},
    {"KZ", MAC_DFS_DOMAIN_NULL},
    {"LB", MAC_DFS_DOMAIN_NULL},
    {"LI", MAC_DFS_DOMAIN_ETSI},
    {"LK", MAC_DFS_DOMAIN_FCC},
    {"LT", MAC_DFS_DOMAIN_ETSI},
    {"LU", MAC_DFS_DOMAIN_ETSI},
    {"LV", MAC_DFS_DOMAIN_ETSI},
    {"MA", MAC_DFS_DOMAIN_NULL},
    {"MC", MAC_DFS_DOMAIN_ETSI},
    {"MK", MAC_DFS_DOMAIN_ETSI},
    {"MO", MAC_DFS_DOMAIN_FCC},
    {"MT", MAC_DFS_DOMAIN_ETSI},
    {"MX", MAC_DFS_DOMAIN_FCC},
    {"MY", MAC_DFS_DOMAIN_FCC},
    {"NG", MAC_DFS_DOMAIN_NULL},
    {"NL", MAC_DFS_DOMAIN_ETSI},
    {"NO", MAC_DFS_DOMAIN_ETSI},
    {"NP", MAC_DFS_DOMAIN_NULL},
    {"NZ", MAC_DFS_DOMAIN_FCC},
    {"OM", MAC_DFS_DOMAIN_FCC},
    {"PA", MAC_DFS_DOMAIN_FCC},
    {"PE", MAC_DFS_DOMAIN_FCC},
    {"PG", MAC_DFS_DOMAIN_FCC},
    {"PH", MAC_DFS_DOMAIN_FCC},
    {"PK", MAC_DFS_DOMAIN_NULL},
    {"PL", MAC_DFS_DOMAIN_ETSI},
    {"PR", MAC_DFS_DOMAIN_FCC},
    {"PT", MAC_DFS_DOMAIN_ETSI},
    {"QA", MAC_DFS_DOMAIN_NULL},
    {"RO", MAC_DFS_DOMAIN_ETSI},
    {"RU", MAC_DFS_DOMAIN_FCC},
    {"SA", MAC_DFS_DOMAIN_FCC},
    {"SE", MAC_DFS_DOMAIN_ETSI},
    {"SG", MAC_DFS_DOMAIN_NULL},
    {"SI", MAC_DFS_DOMAIN_ETSI},
    {"SK", MAC_DFS_DOMAIN_ETSI},
    {"SV", MAC_DFS_DOMAIN_FCC},
    {"SY", MAC_DFS_DOMAIN_NULL},
    {"TH", MAC_DFS_DOMAIN_FCC},
    {"TN", MAC_DFS_DOMAIN_ETSI},
    {"TR", MAC_DFS_DOMAIN_ETSI},
    {"TT", MAC_DFS_DOMAIN_FCC},
    {"TW", MAC_DFS_DOMAIN_NULL},
    {"UA", MAC_DFS_DOMAIN_NULL},
    {"US", MAC_DFS_DOMAIN_FCC},
    {"UY", MAC_DFS_DOMAIN_FCC},
    {"UZ", MAC_DFS_DOMAIN_FCC},
    {"VE", MAC_DFS_DOMAIN_FCC},
    {"VN", MAC_DFS_DOMAIN_ETSI},
    {"YE", MAC_DFS_DOMAIN_NULL},
    {"ZA", MAC_DFS_DOMAIN_FCC},
    {"ZW", MAC_DFS_DOMAIN_NULL},
};
#endif /* _PRE_WLAN_FEATURE_DFS */


OAL_STATIC oal_uint32 wal_get_cmd_one_arg(const oal_int8 *pc_cmd, oal_int8 *pc_arg, oal_uint32 arg_len,
    oal_uint32 *pul_cmd_offset)
{
    const oal_int8 *pc_cmd_copy = OAL_PTR_NULL;
    oal_uint32 ul_pos = 0;

    if (OAL_UNLIKELY((pc_cmd == OAL_PTR_NULL) || (pc_arg == OAL_PTR_NULL) || (pul_cmd_offset == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_get_cmd_one_arg::pc_cmd/pc_arg/pul_cmd_offset null ptr error}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pc_cmd_copy = pc_cmd;

    /* ȥ���ַ�����ʼ�Ŀո� */
    while (*pc_cmd_copy == ' ') {
        ++pc_cmd_copy;
    }

    while ((*pc_cmd_copy != ' ') && (*pc_cmd_copy != '\0')) {
        pc_arg[ul_pos] = *pc_cmd_copy;
        ++ul_pos;
        ++pc_cmd_copy;

        if (OAL_UNLIKELY(ul_pos >= arg_len)) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY,
                "{wal_get_cmd_one_arg::ul_pos >= WAL_HIPRIV_CMD_NAME_MAX_LEN, ul_pos %d!}\r\n", ul_pos);
            return OAL_ERR_CODE_ARRAY_OVERFLOW;
        }
    }

    pc_arg[ul_pos] = '\0';

    /* �ַ�������β�����ش����� */
    if (ul_pos == 0) {
        OAM_INFO_LOG0(0, OAM_SF_ANY, "{wal_get_cmd_one_arg::return param pc_arg is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    *pul_cmd_offset = (oal_uint32)(pc_cmd_copy - pc_cmd);

    return OAL_SUCC;
}


oal_void wal_msg_queue_init(oal_void)
{
    memset_s((oal_void *)&g_wal_wid_msg_queue, OAL_SIZEOF(g_wal_wid_msg_queue), 0, OAL_SIZEOF(g_wal_wid_msg_queue));
    oal_dlist_init_head(&g_wal_wid_msg_queue.st_head);
    g_wal_wid_msg_queue.count = 0;
    oal_spin_lock_init(&g_wal_wid_msg_queue.st_lock);
    OAL_WAIT_QUEUE_INIT_HEAD(&g_wal_wid_msg_queue.st_wait_queue);
}

OAL_STATIC oal_void _wal_msg_request_add_queue_(wal_msg_request_stru *pst_msg)
{
    oal_dlist_add_tail(&pst_msg->pst_entry, &g_wal_wid_msg_queue.st_head);
    g_wal_wid_msg_queue.count++;
}


oal_uint32 wal_get_request_msg_count(oal_void)
{
    return g_wal_wid_msg_queue.count;
}

oal_uint32 wal_check_and_release_msg_resp(wal_msg_stru *pst_rsp_msg)
{
    wal_msg_write_rsp_stru *pst_write_rsp_msg = OAL_PTR_NULL;
    if (pst_rsp_msg != OAL_PTR_NULL) {
        oal_uint32 ul_err_code;
        wlan_cfgid_enum_uint16 en_wid;
        pst_write_rsp_msg = (wal_msg_write_rsp_stru *)(pst_rsp_msg->auc_msg_data);
        ul_err_code = pst_write_rsp_msg->ul_err_code;
        en_wid = pst_write_rsp_msg->en_wid;
        oal_free(pst_rsp_msg);

        if (ul_err_code != OAL_SUCC) {
            OAM_WARNING_LOG2(0, OAM_SF_SCAN, "{wal_check_and_release_msg_resp::detect err code:[%u],wid:[%u]}",
                ul_err_code, en_wid);
            return ul_err_code;
        }
    }

    return OAL_SUCC;
}


oal_void wal_msg_request_add_queue(wal_msg_request_stru *pst_msg)
{
#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
    if (g_wal_wid_queue_init_flag == OAL_FALSE) {
        wal_msg_queue_init();
        g_wal_wid_queue_init_flag = OAL_TRUE;
    }
#endif
    oal_spin_lock_bh(&g_wal_wid_msg_queue.st_lock);
    _wal_msg_request_add_queue_(pst_msg);
    oal_spin_unlock_bh(&g_wal_wid_msg_queue.st_lock);
}

OAL_STATIC oal_void _wal_msg_request_remove_queue_(wal_msg_request_stru *pst_msg)
{
    g_wal_wid_msg_queue.count--;
    oal_dlist_delete_entry(&pst_msg->pst_entry);
}


oal_void wal_msg_request_remove_queue(wal_msg_request_stru *pst_msg)
{
    oal_spin_lock_bh(&g_wal_wid_msg_queue.st_lock);
    _wal_msg_request_remove_queue_(pst_msg);
    oal_spin_unlock_bh(&g_wal_wid_msg_queue.st_lock);
}


oal_int32 wal_set_msg_response_by_addr(oal_ulong addr, oal_void *pst_resp_mem, oal_uint32 ul_resp_ret,
    oal_uint32 uc_rsp_len)
{
    oal_int32 l_ret = -OAL_EINVAL;
    oal_dlist_head_stru *pst_pos = OAL_PTR_NULL;
    oal_dlist_head_stru *pst_entry_temp = OAL_PTR_NULL;
    wal_msg_request_stru *pst_request = OAL_PTR_NULL;

    oal_spin_lock_bh(&g_wal_wid_msg_queue.st_lock);
    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_pos, pst_entry_temp, (&g_wal_wid_msg_queue.st_head))
    {
        pst_request = (wal_msg_request_stru *)OAL_DLIST_GET_ENTRY(pst_pos, wal_msg_request_stru, pst_entry);
        if (pst_request->ul_request_address == (oal_ulong)addr) {
            /* address match */
            if (OAL_UNLIKELY(pst_request->pst_resp_mem != NULL)) {
                OAM_ERROR_LOG0(0, OAM_SF_ANY,
                    "{wal_set_msg_response_by_addr::wal_set_msg_response_by_addr response had been set!");
            }
            pst_request->pst_resp_mem = pst_resp_mem;
            pst_request->ul_ret = ul_resp_ret;
            pst_request->ul_resp_len = uc_rsp_len;
            l_ret = OAL_SUCC;
            break;
        }
    }
    oal_spin_unlock_bh(&g_wal_wid_msg_queue.st_lock);

    return l_ret;
}


oal_uint32  wal_alloc_cfg_event(
                 oal_net_device_stru *pst_net_dev,
                 frw_event_mem_stru **ppst_event_mem,
                 const oal_void      *pst_resp_addr,
                 wal_msg_stru       **ppst_cfg_msg,
                 oal_uint16           us_len)
{
    mac_vap_stru *pst_vap = OAL_PTR_NULL;
    frw_event_mem_stru *pst_event_mem = OAL_PTR_NULL;
    frw_event_stru *pst_event = OAL_PTR_NULL;
    oal_uint16 us_resp_len = 0;

    wal_msg_rep_hdr *pst_rep_hdr = NULL;

    pst_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(pst_vap == OAL_PTR_NULL)) {
        /* ���wifi�ر�״̬�£��·�hipriv������ʾerror��־ */
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_alloc_cfg_event::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    us_resp_len += OAL_SIZEOF(wal_msg_rep_hdr);

    us_len += us_resp_len;

    pst_event_mem = FRW_EVENT_ALLOC(us_len);
    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG2(pst_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_alloc_cfg_event::pst_event_mem null ptr error,request size:us_len:%d,resp_len:%d}", us_len,
            us_resp_len);
        return OAL_ERR_CODE_PTR_NULL;
    }

    *ppst_event_mem = pst_event_mem; /* ���θ�ֵ */

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    /* ��д�¼�ͷ */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_CRX,
                       WAL_HOST_CRX_SUBTYPE_CFG,
                       us_len,
                       FRW_EVENT_PIPELINE_STAGE_0,
                       pst_vap->uc_chip_id,
                       pst_vap->uc_device_id,
                       pst_vap->uc_vap_id);

    /* fill the resp hdr */
    pst_rep_hdr = (wal_msg_rep_hdr *)pst_event->auc_event_data;
    if (pst_resp_addr == NULL) {
        /* no response */
        pst_rep_hdr->ul_request_address = (oal_ulong)0;
    } else {
        /* need response */
        pst_rep_hdr->ul_request_address = (oal_ulong)(uintptr_t)pst_resp_addr;
    }

    *ppst_cfg_msg = (wal_msg_stru *)((oal_uint8 *)pst_event->auc_event_data + us_resp_len); //lint !e416  /* ���θ�ֵ */

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE oal_int32 wal_request_wait_event_condition(wal_msg_request_stru *pst_msg_stru)
{
    oal_int32 l_ret = OAL_FALSE;
    oal_spin_lock_bh(&g_wal_wid_msg_queue.st_lock);
    if ((pst_msg_stru->pst_resp_mem != NULL) || (pst_msg_stru->ul_ret != OAL_SUCC)) {
        l_ret = OAL_TRUE;
    }
    oal_spin_unlock_bh(&g_wal_wid_msg_queue.st_lock);
    return l_ret;
}

oal_void wal_cfg_msg_task_sched(oal_void)
{
    OAL_WAIT_QUEUE_WAKE_UP(&g_wal_wid_msg_queue.st_wait_queue);
}


oal_int32 wal_send_cfg_event(oal_net_device_stru    *pst_net_dev,
                             wal_msg_type_enum_uint8 en_msg_type,
                             oal_uint16              us_len,
                             const oal_uint8        *puc_param,
                             oal_bool_enum_uint8     en_need_rsp,
                             wal_msg_stru          **ppst_rsp_msg)
{
    wal_msg_stru *pst_cfg_msg = OAL_PTR_NULL;
    frw_event_mem_stru *pst_event_mem = OAL_PTR_NULL;
    wal_msg_stru *pst_rsp_msg = OAL_PTR_NULL;
    oal_uint32 ul_ret;
    oal_int32 l_ret;
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
#endif

    DECLARE_WAL_MSG_REQ_STRU(st_msg_request);
    WAL_MSG_REQ_STRU_INIT(st_msg_request);

    if (ppst_rsp_msg != NULL) {
        *ppst_rsp_msg = NULL;
    }

    if (OAL_WARN_ON(OAL_VALUE_EQ_ALL3(en_need_rsp, ppst_rsp_msg, OAL_TRUE, OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_send_cfg_event::OAL_PTR_NULL == ppst_rsp_msg!}\r\n");
        return -OAL_EINVAL;
    }

    /* �����¼� */
    ul_ret = wal_alloc_cfg_event(pst_net_dev, &pst_event_mem,
                                 ((en_need_rsp == OAL_TRUE) ? &st_msg_request : NULL),
                                 &pst_cfg_msg,
                                 WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_send_cfg_event::wal_alloc_cfg_event return err code %d!}\r\n", ul_ret);
        return -OAL_ENOMEM;
    }

    /* ��д������Ϣ */
    WAL_CFG_MSG_HDR_INIT(&(pst_cfg_msg->st_msg_hdr), en_msg_type, us_len, WAL_GET_MSG_SN());

    /* ��дWID��Ϣ */
    if (memcpy_s(pst_cfg_msg->auc_msg_data, us_len, puc_param, us_len) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_send_cfg_event::memcpy_s failed!");
        FRW_EVENT_FREE(pst_event_mem);
        return -OAL_EINVAL;
    }

#ifdef _PRE_WLAN_DFT_EVENT
    wal_event_report_to_sdt(en_msg_type, puc_param, pst_cfg_msg);
#endif

    if (en_need_rsp == OAL_TRUE) {
        /* add queue before post event! */
        wal_msg_request_add_queue(&st_msg_request);
    }

/* �ַ��¼� */
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_send_cfg_event::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr.}");
        FRW_EVENT_FREE(pst_event_mem);
        return -OAL_EINVAL;
    }

    frw_event_post_event(pst_event_mem, pst_mac_vap->ul_core_id);
#else
    frw_event_dispatch_event(pst_event_mem);
#endif
    FRW_EVENT_FREE(pst_event_mem);

    /* win32 UTģʽ������һ���¼����� */
#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION) && (_PRE_TEST_MODE == _PRE_TEST_MODE_UT)
    frw_event_process_all_event(0);
#endif

    if (en_need_rsp != OAL_TRUE) {
        return OAL_SUCC;
    }

    /* context can't in interrupt */
    if (OAL_WARN_ON(oal_in_interrupt())) { //lint !e516
        DECLARE_DFT_TRACE_KEY_INFO("wal_cfg_in_interrupt", OAL_DFT_TRACE_EXCEP);
    }

    if (OAL_WARN_ON(oal_in_atomic())) {
        DECLARE_DFT_TRACE_KEY_INFO("wal_cfg_in_atomic", OAL_DFT_TRACE_EXCEP);
    }

    /* **************************************************************************
        �ȴ��¼�����
    ************************************************************************** */
    wal_wake_lock();

    /*lint -e730*/ /* info, boolean argument to function */
    l_ret = OAL_WAIT_EVENT_TIMEOUT(g_wal_wid_msg_queue.st_wait_queue,
                                   wal_request_wait_event_condition(&st_msg_request) == OAL_TRUE,
                                   30 * OAL_TIME_HZ); //lint !e665 !e666
    /*lint +e730*/

    /* response had been set, remove it from the list */
    if (en_need_rsp == OAL_TRUE) {
        wal_msg_request_remove_queue(&st_msg_request);
    }

    if (OAL_WARN_ON(l_ret == 0)) { //lint !e730
        /* ��ʱ */
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_send_cfg_event:: wait queue timeout,30s!}\r\n");

        OAL_IO_PRINT("[E]timeout,request info:%p,ret=%u,addr:0x%lx\n", st_msg_request.pst_resp_mem,
            st_msg_request.ul_ret, st_msg_request.ul_request_address);
        if (st_msg_request.pst_resp_mem != NULL) {
            oal_free(st_msg_request.pst_resp_mem);
        }
        wal_wake_unlock();
        DECLARE_DFT_TRACE_KEY_INFO("wal_send_cfg_timeout", OAL_DFT_TRACE_FAIL);
        /* ��ӡCFG EVENT�ڴ棬���㶨λ�� */
        oal_print_hex_dump((oal_uint8 *)pst_cfg_msg, (WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len), 32, "cfg event: ");
        frw_event_queue_info();
        return -OAL_ETIMEDOUT;
    }
    /*lint +e774*/
    pst_rsp_msg = (wal_msg_stru *)(st_msg_request.pst_resp_mem);
    if (pst_rsp_msg == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_send_cfg_event:: msg mem null!}");
        /*lint -e613*/
        *ppst_rsp_msg = OAL_PTR_NULL;
        /*lint +e613*/
        wal_wake_unlock();
        return -OAL_EFAUL;
    }

    if (pst_rsp_msg->st_msg_hdr.us_msg_len == 0) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_send_cfg_event:: no msg resp!}");
        /*lint -e613*/
        *ppst_rsp_msg = OAL_PTR_NULL;
        /*lint +e613*/
        oal_free(pst_rsp_msg);
        wal_wake_unlock();
        return -OAL_EFAUL;
    }
    /* ���������¼����ص�״̬��Ϣ */
    /*lint -e613*/
    *ppst_rsp_msg = pst_rsp_msg;
    /*lint +e613*/
    wal_wake_unlock();
    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_P2P


wlan_p2p_mode_enum_uint8 wal_wireless_iftype_to_mac_p2p_mode(enum nl80211_iftype en_iftype)
{
    wlan_p2p_mode_enum_uint8 en_p2p_mode = WLAN_LEGACY_VAP_MODE;

    switch (en_iftype) {
        case NL80211_IFTYPE_P2P_CLIENT:
            en_p2p_mode = WLAN_P2P_CL_MODE;
            break;
        case NL80211_IFTYPE_P2P_GO:
            en_p2p_mode = WLAN_P2P_GO_MODE;
            break;
        case NL80211_IFTYPE_P2P_DEVICE:
            en_p2p_mode = WLAN_P2P_DEV_MODE;
            break;
        case NL80211_IFTYPE_AP:
        case NL80211_IFTYPE_STATION:
            en_p2p_mode = WLAN_LEGACY_VAP_MODE;
            break;
        default:
            en_p2p_mode = WLAN_P2P_BUTT;
    }
    return en_p2p_mode;
}
#endif


oal_int32 wal_cfg_vap_h2d_event(oal_net_device_stru *pst_net_dev)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_wireless_dev_stru      *pst_wdev = OAL_PTR_NULL;
    mac_wiphy_priv_stru        *pst_wiphy_priv = OAL_PTR_NULL;
    hmac_vap_stru              *pst_cfg_hmac_vap = OAL_PTR_NULL;
    mac_device_stru            *pst_mac_device = OAL_PTR_NULL;
    oal_net_device_stru        *pst_cfg_net_dev = OAL_PTR_NULL;

    oal_int32                   l_ret;
    wal_msg_stru                *pst_rsp_msg = OAL_PTR_NULL;
    oal_uint32                  ul_err_code;
    wal_msg_write_stru          st_write_msg;

    if (pst_net_dev == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_cfg_vap_h2d_event::pst_net_dev is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_wdev = OAL_NETDEVICE_WDEV(pst_net_dev);
    if (pst_wdev == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_cfg_vap_h2d_event::pst_wdev is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_wiphy_priv = (mac_wiphy_priv_stru *)oal_wiphy_priv(pst_wdev->wiphy);
    if (pst_wiphy_priv == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_cfg_vap_h2d_event::pst_wiphy_priv is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = pst_wiphy_priv->pst_mac_device;
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_cfg_vap_h2d_event::pst_mac_device is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_cfg_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_device->uc_cfg_vap_id);
    if (pst_cfg_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfg_vap_h2d_event::mac_res_get_hmac_vap fail.vap_id[%u]}",
            pst_mac_device->uc_cfg_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_cfg_net_dev = pst_cfg_hmac_vap->pst_net_device;
    if (pst_cfg_net_dev == NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_cfg_vap_h2d_event::pst_cfg_net_dev is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* **************************************************************************
    ���¼���wal�㴦��
    ************************************************************************** */
    /* ��д��Ϣ */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_CFG_VAP_H2D, OAL_SIZEOF(mac_cfg_vap_h2d_stru));
    ((mac_cfg_vap_h2d_stru *)st_write_msg.auc_value)->pst_net_dev = pst_cfg_net_dev;

    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_vap_h2d_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfg_vap_h2d_event::wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* ��������Ϣ */
    ul_err_code = wal_check_and_release_msg_resp(pst_rsp_msg);
    if (ul_err_code != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfg_vap_h2d_event::hmac cfg vap h2d fail,err code[%u]\r\n", ul_err_code);
        return -OAL_EINVAL;
    }

#endif

    return OAL_SUCC;
}
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_int32 wal_host_dev_config(oal_net_device_stru *pst_net_dev, wlan_cfgid_enum_uint16 en_wid)
{
    oal_wireless_dev_stru      *pst_wdev = OAL_PTR_NULL;
    mac_wiphy_priv_stru        *pst_wiphy_priv = OAL_PTR_NULL;
    hmac_vap_stru              *pst_cfg_hmac_vap = OAL_PTR_NULL;
    mac_device_stru            *pst_mac_device = OAL_PTR_NULL;
    oal_net_device_stru        *pst_cfg_net_dev = OAL_PTR_NULL;

    oal_int32                   l_ret;
    wal_msg_stru               *pst_rsp_msg = OAL_PTR_NULL;
    oal_uint32                  ul_err_code;
    wal_msg_write_stru          st_write_msg;

    pst_wdev = OAL_NETDEVICE_WDEV(pst_net_dev);
    if (pst_wdev == NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_host_dev_config::pst_wdev is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_wiphy_priv = (mac_wiphy_priv_stru *)oal_wiphy_priv(pst_wdev->wiphy);
    if (pst_wiphy_priv == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_host_dev_config::pst_mac_device is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = pst_wiphy_priv->pst_mac_device;
    if (pst_mac_device == NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_host_dev_config::pst_mac_device is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_cfg_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_device->uc_cfg_vap_id);
    if (pst_cfg_hmac_vap == NULL) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_host_dev_config::pst_cfg_hmac_vap is null vap_id:%d!}\r\n",
            pst_mac_device->uc_cfg_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_cfg_net_dev = pst_cfg_hmac_vap->pst_net_device;
    if (pst_cfg_net_dev == NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_host_dev_config::pst_cfg_net_dev is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* **************************************************************************
    ���¼���wal�㴦��
    ************************************************************************** */
    /* ��д��Ϣ */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, en_wid, 0);

    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH,
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfg_vap_h2d_event::wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* ��������Ϣ */
    ul_err_code = wal_check_and_release_msg_resp(pst_rsp_msg);
    if (ul_err_code != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfg_vap_h2d_event::hmac cfg vap h2d fail,err code[%u]\r\n", ul_err_code);
        return -OAL_EINVAL;
    }

    return OAL_SUCC;
}


oal_int32 wal_host_dev_init(oal_net_device_stru *pst_net_dev)
{
    return wal_host_dev_config(pst_net_dev, WLAN_CFGID_HOST_DEV_INIT);
}


oal_int32 wal_host_dev_exit(oal_net_device_stru *pst_net_dev)
{
    return wal_host_dev_config(pst_net_dev, WLAN_CFGID_HOST_DEV_EXIT);
}
#endif


#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#ifdef _PRE_WLAN_FEATURE_ROAM

OAL_STATIC void hwifi_config_host_global_ini_roaming_param(void)
{
    int32_t val;
    /* ����   */
    val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_ROAM_SWITCH);
    g_st_wlan_customize.uc_roam_switch =
        OAL_VALUE_EQ_ANY2(val, 0, 1) ? (uint8_t)val : g_st_wlan_customize.uc_roam_switch;
    g_st_wlan_customize.uc_roam_scan_band = band_5g_enabled ? (BIT0 | BIT1) : BIT0;
    val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_SCAN_ORTHOGONAL);
    g_st_wlan_customize.uc_roam_scan_orthogonal =
        (val >= 1) ? (uint8_t)val : g_st_wlan_customize.uc_roam_scan_orthogonal;

    val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_TRIGGER_B);
    g_st_wlan_customize.c_roam_trigger_b = (int8_t)val;

    val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_TRIGGER_A);
    g_st_wlan_customize.c_roam_trigger_a = (int8_t)val;

    val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_DELTA_B);
    g_st_wlan_customize.c_roam_delta_b = (int8_t)val;

    val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_DELTA_A);
    g_st_wlan_customize.c_roam_delta_a = (int8_t)val;
    return;
}
#endif


OAL_STATIC oal_int32 hwifi_config_host_global_ini_param(oal_void)
{
    oal_int32 l_val;

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ

    oal_uint32 cfg_id;
    oal_uint32 ul_val;
    oal_int32 l_cfg_value;
    oal_int8 *pc_tmp = OAL_PTR_NULL;
    host_speed_freq_level_stru host_freq_level[4];
    device_speed_freq_level_stru device_freq_level[4];
    oal_uint8 uc_flag = OAL_FALSE;
    oal_uint8 uc_index;
    oal_uint32 l_ret = EOK;

    memset_s(device_freq_level, OAL_SIZEOF(device_freq_level), 0, OAL_SIZEOF(device_freq_level));
#endif /* #ifdef _PRE_WLAN_FEATURE_AUTO_FREQ */
    /* ******************************************* ���� ******************************************* */
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_AMPDU_TX_MAX_NUM);
    g_st_wlan_customize.ul_ampdu_tx_max_num = OAL_VALUE_IN_VALID_RANGE(l_val, 1, WLAN_AMPDU_TX_MAX_NUM) ?
        (oal_uint32)l_val : g_st_wlan_customize.ul_ampdu_tx_max_num;
    OAL_IO_PRINT("hisi_customize_wifi::ampdu_tx_max_num:%d", g_st_wlan_customize.ul_ampdu_tx_max_num);
#ifdef _PRE_WLAN_FEATURE_ROAM
    hwifi_config_host_global_ini_roaming_param();
#endif /* #ifdef _PRE_WLAN_FEATURE_ROAM */

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
    /* ******************************************* �Զ���Ƶ ******************************************* */
    /* config g_host_speed_freq_level */
    pc_tmp = (oal_int8 *)&host_freq_level;
    for (cfg_id = WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_0; cfg_id <= WLAN_CFG_INIT_DDR_FREQ_LIMIT_LEVEL_3; ++cfg_id) {
        ul_val = hwifi_get_init_value(CUS_TAG_INI, cfg_id);
        *(oal_uint32 *)pc_tmp = ul_val;
        pc_tmp += 4;
    }
    /* config g_device_speed_freq_level */
    pc_tmp = (oal_int8 *)&device_freq_level;
    for (cfg_id = WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_0; cfg_id <= WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_3; ++cfg_id) {
        l_cfg_value = hwifi_get_init_value(CUS_TAG_INI, cfg_id);
        if (OAL_VALUE_IN_VALID_RANGE(l_cfg_value, FREQ_IDLE, FREQ_HIGHEST)) {
            *pc_tmp = l_cfg_value;
            pc_tmp += 4;
        } else {
            uc_flag = OAL_TRUE;
            break;
        }
    }

    if (!uc_flag) {
        l_ret += memcpy_s(&g_host_speed_freq_level, OAL_SIZEOF(g_host_speed_freq_level), &host_freq_level,
            OAL_SIZEOF(host_freq_level));
        l_ret += memcpy_s(&g_device_speed_freq_level, OAL_SIZEOF(g_device_speed_freq_level),
            &device_freq_level, OAL_SIZEOF(device_freq_level));
        if (l_ret != EOK) {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "hwifi_config_host_global_ini_param::memcpy_s failed!");
            return OAL_FAIL;
        }
        for (uc_index = 0; uc_index < 4; uc_index++) {
            OAM_WARNING_LOG4(0, OAM_SF_ANY, "{ul_speed_level = %d,ul_min_cpu_freq = %d,uc_device_type = %d}\r\n",
                g_host_speed_freq_level[uc_index].ul_speed_level, g_host_speed_freq_level[uc_index].ul_min_cpu_freq,
                g_host_speed_freq_level[uc_index].ul_min_ddr_freq, g_device_speed_freq_level[uc_index].uc_device_type);
        }
    }
#endif /* #ifdef _PRE_WLAN_FEATURE_AUTO_FREQ */
    /* ******************************************* ɨ�� ******************************************* */
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RANDOM_MAC_ADDR_SCAN);
    g_st_wlan_customize.uc_random_mac_addr_scan = !!l_val;
    /* ******************************************* CAPABILITY ******************************************* */
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_DISABLE_CAPAB_2GHT40);
    g_st_wlan_customize.uc_disable_capab_2ght40 = (oal_uint8) !!l_val;
    return OAL_SUCC;
}


OAL_STATIC oal_void hwifi_config_init_ini_perf(oal_net_device_stru *pst_cfg_net_dev)
{
    wal_msg_write_stru st_write_msg;
    oal_int32          l_ret;
    oal_int8           pc_param[18]    = {0};
    oal_int8           pc_tmp[8]       = {0};
    oal_uint16         us_len;
    oal_uint8          uc_sdio_assem_h2d;
    oal_uint8          uc_sdio_assem_d2h;
    oal_uint32         l_memcpy_ret = EOK;

    /* SDIO FLOWCTRL */
    // device�����Ϸ����ж�
    oal_itoa(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_USED_MEM_FOR_STOP), pc_param, 8);
    oal_itoa(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_USED_MEM_FOR_START), pc_tmp, 8);
    pc_param[OAL_STRLEN(pc_param)] = ' ';
    l_memcpy_ret += memcpy_s(pc_param + strlen(pc_param), sizeof(pc_param) - strlen(pc_param), pc_tmp, strlen(pc_tmp));
    l_memcpy_ret += memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, pc_param, OAL_STRLEN(pc_param));
    if (l_memcpy_ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hwifi_config_init_ini_perf::memcpy_s failed!");
        return;
    }
    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';
    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SDIO_FLOWCTRL, us_len);

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hwifi_config_init_ini_perf::return err code [%d]!}\r\n", l_ret);
    }

    /* SDIO ASSEMBLE COUNT:H2D */
    uc_sdio_assem_h2d = (oal_uint8)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_SDIO_H2D_ASSEMBLE_COUNT);
    // �ж�ֵ�ĺϷ���
    if (uc_sdio_assem_h2d >= 1 && uc_sdio_assem_h2d <= HISDIO_HOST2DEV_SCATT_MAX) {
        hcc_assemble_count = uc_sdio_assem_h2d;
    } else {
        OAM_ERROR_LOG2(0, OAM_SF_ANY,
            "{hwifi_config_init_ini_perf::sdio_assem_h2d[%d] out of range(0,%d], check value in ini file!}\r\n",
            uc_sdio_assem_h2d, HISDIO_HOST2DEV_SCATT_MAX);
    }

    /* SDIO ASSEMBLE COUNT:D2H */
    uc_sdio_assem_d2h = (oal_uint8)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_SDIO_D2H_ASSEMBLE_COUNT);
    // �ж�ֵ�ĺϷ���
    if (uc_sdio_assem_d2h >= 1 && uc_sdio_assem_d2h <= HISDIO_DEV2HOST_SCATT_MAX) {
        WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_D2H_HCC_ASSEMBLE_CNT, OAL_SIZEOF(oal_int32));
        *((oal_int32 *)(st_write_msg.auc_value)) = uc_sdio_assem_d2h;

        l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_FALSE,
                                   OAL_PTR_NULL);
        if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hwifi_config_init_ini_perf::return err code [%d]!}\r\n", l_ret);
        }
    } else {
        OAM_ERROR_LOG2(0, OAM_SF_ANY,
            "{hwifi_config_init_ini_perf::sdio_assem_d2h[%d] out of range(0,%d], check value in ini file!}\r\n",
            uc_sdio_assem_d2h, HISDIO_DEV2HOST_SCATT_MAX);
    }
}


OAL_STATIC oal_void hwifi_config_init_ini_linkloss(oal_net_device_stru *pst_cfg_net_dev)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_LINKLOSS_THRESHOLD, OAL_SIZEOF(mac_cfg_linkloss_threshold));
    ((mac_cfg_linkloss_threshold *)(st_write_msg.auc_value))->uc_linkloss_threshold_wlan_near =
        (oal_uint8)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_WLAN_NEAR);
    ((mac_cfg_linkloss_threshold *)(st_write_msg.auc_value))->uc_linkloss_threshold_wlan_far =
        (oal_uint8)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_WLAN_FAR);
    ((mac_cfg_linkloss_threshold *)(st_write_msg.auc_value))->uc_linkloss_threshold_p2p =
        (oal_uint8)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_P2P);

    l_ret = wal_send_cfg_event(pst_cfg_net_dev, WAL_MSG_TYPE_WRITE,
        WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_linkloss_threshold), (oal_uint8 *)&st_write_msg, OAL_FALSE,
        OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hwifi_config_init_ini_linkloss::wal_send_cfg_event return err code [%d]!}\r\n",
            l_ret);
    }
}


OAL_STATIC oal_void hwifi_config_init_ini_country(oal_net_device_stru *pst_cfg_net_dev)
{
    oal_int32 l_ret;

    l_ret = (oal_int32)wal_hipriv_setcountry(pst_cfg_net_dev, hwifi_get_country_code());
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hwifi_config_init_ini_country::wal_send_cfg_event return err code [%d]!}\r\n",
            l_ret);
    }
}


OAL_STATIC oal_void hwifi_config_init_ini_auto_freq(oal_net_device_stru *pst_cfg_net_dev)
{
    oal_int8                    c_switch;
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_cfg_rst;
    oal_uint16                  us_len;
    mac_cfg_set_auto_freq_stru *pst_set_auto_freq = OAL_PTR_NULL;

    c_switch = (oal_int8)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_AUTO_FREQ_ENABLE);
    if ((c_switch != FREQ_LOCK_ENABLE) && (c_switch != FREQ_LOCK_DISABLE)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hwifi_config_init_ini_auto_freq autofreq_value(%d) invalid!",
            (oal_uint8)c_switch);
        return;
    }

    pst_set_auto_freq = (mac_cfg_set_auto_freq_stru *)(st_write_msg.auc_value);
    pst_set_auto_freq->uc_cmd_type = CMD_SET_AUTO_FREQ_ENDABLE;
    pst_set_auto_freq->uc_value = c_switch;
    us_len = OAL_SIZEOF(mac_cfg_set_auto_freq_stru);
    /* **************************************************************************
                              ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_AUTO_FREQ_ENABLE, us_len);
    l_cfg_rst = wal_send_cfg_event(pst_cfg_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_FALSE,
                                   OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_cfg_rst != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_autofreq_enable::wal_send_cfg_event return err_code [%d]!}\r\n", l_cfg_rst);
    }
}


OAL_STATIC oal_void hwifi_config_init_ini_pm(oal_net_device_stru *pst_cfg_net_dev)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_int32 l_switch;
    oal_int32 l_dtim_setting;
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    oal_int32 l_rssi_descend_protocol_limit;
#endif
    /* ���� */
    l_switch = !!hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_POWERMGMT_SWITCH);

    /* DTIM�������ø���PM_SWITCH�͹��Ķ��ƻ������·���DTIM[7:1],PM[0] */
    l_dtim_setting = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_STA_DTIM_SETTING);
    l_switch = (((oal_uint32)l_dtim_setting) << 1) | (oal_uint32)l_switch;
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    /* ���������л����� */
    l_rssi_descend_protocol_limit = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RSSI_DESCEND_PROTOCOL_LIMIT);
    if (l_rssi_descend_protocol_limit != 0 && l_rssi_descend_protocol_limit != 1) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "hwifi_config_init_ini_pm::set err rssi_descend_protocol_limit %d",
            l_rssi_descend_protocol_limit);
        l_rssi_descend_protocol_limit = 1;
    }
    l_switch = (l_rssi_descend_protocol_limit << 16) | l_switch;
#endif

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_PM_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_switch;

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hwifi_config_init_ini_pm::return err code [%d]!}\r\n", l_ret);
    }
}


OAL_STATIC oal_void hwifi_config_init_ini_log(oal_net_device_stru *pst_cfg_net_dev)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_int32 l_loglevel;

    /* log_level */
    l_loglevel = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_LOGLEVEL);
    if (l_loglevel < OAM_LOG_LEVEL_ERROR || l_loglevel > OAM_LOG_LEVEL_INFO) {
        OAM_ERROR_LOG3(0, OAM_SF_ANY,
            "{hwifi_config_init_ini_log::loglevel[%d] out of range[%d,%d], check value in ini file!}\r\n", l_loglevel,
            OAM_LOG_LEVEL_ERROR, OAM_LOG_LEVEL_INFO);
        return;
    }

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_ALL_LOG_LEVEL, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_loglevel;
    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hwifi_config_init_ini_log::return err code[%d]!}\r\n", l_ret);
    }
}


OAL_STATIC oal_void hwifi_config_init_ini_phy(oal_net_device_stru *pst_cfg_net_dev)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint32 ul_chn_est_ctrl;
    oal_uint32 ul_2g_pwr_ref;

    /* CHN_EST_CTRL */
    ul_chn_est_ctrl = (oal_uint32)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_CHN_EST_CTRL);
    if (ul_chn_est_ctrl == CHN_EST_CTRL_EVB || ul_chn_est_ctrl == CHN_EST_CTRL_FPGA ||
        ul_chn_est_ctrl == CHN_EST_CTRL_MATE7) {
        WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_CHN_EST_CTRL, OAL_SIZEOF(oal_uint32));
        *((oal_uint32 *)(st_write_msg.auc_value)) = ul_chn_est_ctrl;
        l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_FALSE,
                                   OAL_PTR_NULL);
        if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hwifi_config_init_ini_phy::return err code [%d]!}\r\n", l_ret);
        }
    } else {
        OAM_ERROR_LOG1(0, OAM_SF_ANY,
            "{hwifi_config_init_ini_phy::chn_est_ctrl[0x%x] value not correct, check value in ini file!}\r\n",
            ul_chn_est_ctrl);
    }

    /* POWER_REF */
    ul_2g_pwr_ref = (oal_uint32)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_POWER_REF_2G);
    if ((ul_2g_pwr_ref == PHY_POWER_REF_2G_3798) || (ul_2g_pwr_ref == PHY_POWER_REF_2G_EVB0)) {
        WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_POWER_REF, OAL_SIZEOF(mac_cfg_power_ref));
        ((mac_cfg_power_ref *)(st_write_msg.auc_value))->ul_power_ref_2g = ul_2g_pwr_ref;

        l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_power_ref),
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_FALSE,
                                   OAL_PTR_NULL);
        if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hwifi_config_init_ini_phy::wal_send_cfg_event return err code [%d]!}\r\n",
                l_ret);
        }
    } else {
        OAM_ERROR_LOG1(0, OAM_SF_ANY,
            "{hwifi_config_init_ini_phy::2g_pwr_ref[0x%x] value not correct, check value in ini file!}\r\n",
            ul_2g_pwr_ref);
    }
}


OAL_STATIC oal_void hwifi_config_init_ini_clock(oal_net_device_stru *pst_cfg_net_dev)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint32 ul_freq;
    oal_uint8 uc_type;

    ul_freq = (oal_uint32)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RTS_CLK_FREQ);
    uc_type = (oal_uint8) !!hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_CLK_TYPE);

    if (ul_freq < RTC_CLK_FREQ_MIN || ul_freq > RTC_CLK_FREQ_MAX) {
        OAM_ERROR_LOG3(0, OAM_SF_ANY,
            "{hwifi_config_init_ini_clock::clock_freq[%d] out of range[%d,%d], check value in ini file!}\r\n", ul_freq,
            RTC_CLK_FREQ_MIN, RTC_CLK_FREQ_MAX);
        return;
    }

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_PM_CFG_PARAM, OAL_SIZEOF(st_pm_cfg_param));
    ((st_pm_cfg_param *)(st_write_msg.auc_value))->ul_rtc_clk_freq = ul_freq;
    ((st_pm_cfg_param *)(st_write_msg.auc_value))->uc_clk_type = uc_type;

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_pm_cfg_param),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hwifi_config_init_ini_clock::wal_send_cfg_event return err code [%d]!}\r\n",
            l_ret);
    }
}

OAL_STATIC oal_void hwifi_config_init_ini_high_power(oal_net_device_stru *pst_cfg_net_dev)
{
    wal_msg_write_stru write_msg;
    sub_type_load_stru *high_power;
    int32_t            ret;
    uint32_t           len;
    uint8_t            high_power_switch ;

    high_power_switch = (uint8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_HIGH_POWER_SWITCH);
    if ((high_power_switch != OAL_FALSE) && (high_power_switch != OAL_TRUE)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hwifi_config_init_ini_high_power::err value:%d}", high_power_switch);
        return;
    }
    high_power = (sub_type_load_stru*)(write_msg.auc_value);
    high_power->en_sub_type = MAC_SUB_HIGN_POWER_SWITCH_EVENT;
    high_power->us_buf_len  = sizeof(high_power_switch);
    high_power->auc_buf[0]  = high_power_switch;

    len = sizeof(sub_type_load_stru) - WLAN_SUB_EVENT_MAX_LEN + high_power->us_buf_len;

    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_HIGH_POWER_SWITCH, len);

    ret = wal_send_cfg_event(pst_cfg_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + len,
                             (uint8_t *)&write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hwifi_config_init_ini_high_power::return err code [%d]!}\r\n", ret);
    }
}


#ifdef _PRE_WLAN_FEATURE_1131K_BTCOEX
OAL_STATIC oal_void hwifi_config_init_ini_btcoex_channel(oal_net_device_stru *cfg_net_dev)
{
    wal_msg_write_stru write_msg = { 0 };
    sub_type_load_stru *btcoex_switch = NULL;
    int32_t            ret;
    uint32_t           len;
    uint8_t            btcoex_channel_switch;

    btcoex_channel_switch = (uint8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_BTCOEX_CHANNEL_SWITCH);
    if ((btcoex_channel_switch != OAL_FALSE) && (btcoex_channel_switch != OAL_TRUE)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hwifi_config_init_ini_btcoex_channel::err value: %d}", btcoex_channel_switch);
        return;
    }

#ifdef DHUNG_WP_FACTORY_MODE
    /* ���߲�������ͨ�� */
    btcoex_channel_switch = OAL_FALSE;
#endif
    OAM_WARNING_LOG2(0, OAM_SF_COEX, "hwifi_config_init_ini_btcoex_channel::btcoex channel switch cfg [%d] value [%d]",
                     WLAN_CFG_INIT_BTCOEX_CHANNEL_SWITCH, btcoex_channel_switch);

    btcoex_switch = (sub_type_load_stru *)(write_msg.auc_value);
    btcoex_switch->en_sub_type = MAC_SUB_BTCOEX_CHANNEL_SWITCH_EVENT;
    btcoex_switch->us_buf_len  = sizeof(uint8_t);
    btcoex_switch->auc_buf[0]  = btcoex_channel_switch;

    len = sizeof(sub_type_load_stru) - WLAN_SUB_EVENT_MAX_LEN + btcoex_switch->us_buf_len;

    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_BTCOEX_CHANNEL_SWITCH, len);

    ret = wal_send_cfg_event(cfg_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + len,
                             (uint8_t *)&write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hwifi_config_init_ini_btcoex_channel::return err code [%d]!}\r\n", ret);
    }
}
#endif


static oal_bool_enum deploy_2g_rf(mac_cfg_customize_rf *pst_customize_rf)
{
    oal_uint8 uc_idx;
    oal_int8 c_rf_power_loss;
    /* ����: 2g rf */
    for (uc_idx = 0; uc_idx < MAC_NUM_2G_BAND; ++uc_idx) {
        /* ��ȡ��2p4g ��band 0.25db��0.1db���ȵ�����ֵ */
        c_rf_power_loss =
            (oal_int8)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_2G_BAND1 + uc_idx);
        if ((c_rf_power_loss >= RF_LINE_TXRX_GAIN_DB_2G_MIN) && (c_rf_power_loss <= 0)) {
            pst_customize_rf->ac_gain_db_2g[uc_idx].c_rf_gain_db_2g_mult4 = c_rf_power_loss;
        } else {
            /* ֵ������Ч��Χ������ʧ�� */
            OAM_ERROR_LOG1(0, OAM_SF_CALIBRATE, "deploy_2g_rf::c_mult4[%d]", (oal_uint8)c_rf_power_loss);
            return OAL_FALSE;
        }
    }
    return OAL_TRUE;
}

#ifdef _PRE_WLAN_FEATURE_5G

static oal_bool_enum deploy_5g_rf(mac_cfg_customize_rf *pst_customize_rf)
{
    /* ����: 5g rf */
    pst_customize_rf->c_rf_line_rx_gain_db_5g =
        (oal_int8)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_LINE_RX_GAIN_DB_5G);
    pst_customize_rf->c_lna_gain_db_5g = (oal_int8)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_LNA_GAIN_DB_5G);
    pst_customize_rf->c_rf_line_tx_gain_db_5g =
        (oal_int8)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_LINE_TX_GAIN_DB_5G);
    pst_customize_rf->uc_ext_switch_isexist_5g =
        (oal_uint8) !!hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_EXT_SWITCH_ISEXIST_5G);
    pst_customize_rf->uc_ext_pa_isexist_5g =
        (oal_uint8) !!hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_EXT_PA_ISEXIST_5G);
    pst_customize_rf->uc_ext_lna_isexist_5g =
        (oal_uint8) !!hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_EXT_LNA_ISEXIST_5G);
    pst_customize_rf->us_lna_on2off_time_ns_5g =
        (oal_uint16)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_LNA_ON2OFF_TIME_NS_5G);
    pst_customize_rf->us_lna_off2on_time_ns_5g =
        (oal_uint16)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_LNA_OFF2ON_TIME_NS_5G);

    if (!((pst_customize_rf->c_rf_line_rx_gain_db_5g >= RF_LINE_TXRX_GAIN_DB_5G_MIN &&
        pst_customize_rf->c_rf_line_rx_gain_db_5g <= 0) &&
        (pst_customize_rf->c_rf_line_tx_gain_db_5g >= RF_LINE_TXRX_GAIN_DB_5G_MIN &&
        pst_customize_rf->c_rf_line_tx_gain_db_5g <= 0) &&
        (pst_customize_rf->c_lna_gain_db_5g >= LNA_GAIN_DB_MIN &&
        pst_customize_rf->c_lna_gain_db_5g <= LNA_GAIN_DB_MAX))) {
        OAM_ERROR_LOG2(0, OAM_SF_ANY,
            "{deploy_5g_rf::cfg_id_range[%d,%d] value out of range, please check these values!}\r\n",
            WLAN_CFG_INIT_RF_LINE_RX_GAIN_DB_5G, WLAN_CFG_INIT_LNA_OFF2ON_TIME_NS_5G);
        /* ֵ������Ч��Χ������ʧ�� */
        return OAL_FALSE;
    }
    return OAL_TRUE;
}
#endif


OAL_STATIC oal_void hwifi_config_init_ini_rf(oal_net_device_stru *pst_cfg_net_dev)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint32 ul_offset = 0;
    oal_uint8 uc_level;                                             /* �������ʵĵ�λ:0-2 */
    oal_uint8 uc_tx_pwr_comp_base = WLAN_CFG_INIT_TX_RATIO_LEVEL_0; /* ���ʲ�������ʼIDֵ */
    oal_uint8 uc_error_param = OAL_FALSE; /* ������Ч�Ա�־���������ֵ���Ϸ�����Ϊ1�����в������·� */
    mac_cfg_customize_rf *pst_customize_rf = OAL_PTR_NULL;
    mac_cfg_customize_tx_pwr_comp_stru *pst_tx_pwr_comp = OAL_PTR_NULL;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_CUS_RF,
        OAL_SIZEOF(mac_cfg_customize_rf) + OAL_SIZEOF(mac_cfg_customize_tx_pwr_comp_stru));

    pst_customize_rf = (mac_cfg_customize_rf *)(st_write_msg.auc_value);
    ul_offset += OAL_SIZEOF(mac_cfg_customize_rf);

    if (deploy_2g_rf(pst_customize_rf) == OAL_FALSE) {
        uc_error_param = OAL_TRUE;
    }
#ifdef _PRE_WLAN_FEATURE_5G
    if (band_5g_enabled) {
        if (deploy_5g_rf(pst_customize_rf) == OAL_FALSE) {
            uc_error_param = OAL_TRUE;
        }
    }
#endif
    /* ����: ���ʲ��� */
    pst_tx_pwr_comp = (mac_cfg_customize_tx_pwr_comp_stru *)(st_write_msg.auc_value + ul_offset);
    for (uc_level = 0; uc_level < 3; ++uc_level) {
        pst_tx_pwr_comp->ast_txratio2pwr[uc_level].us_tx_ratio =
            (oal_uint16)hwifi_get_init_value(CUS_TAG_INI, uc_tx_pwr_comp_base + 2 * uc_level);
        /* �ж�txռ�ձ��Ƿ���Ч */
        if (pst_tx_pwr_comp->ast_txratio2pwr[uc_level].us_tx_ratio > TX_RATIO_MAX) {
            OAM_ERROR_LOG2(0, OAM_SF_ANY,
                "{hwifi_config_init_ini_rf::cfg_id[%d]:tx_ratio[%d] out of range, please check the value!}\r\n",
                uc_tx_pwr_comp_base + 2 * uc_level, pst_tx_pwr_comp->ast_txratio2pwr[uc_level].us_tx_ratio);
            /* ֵ������Ч��Χ�������ΪTRUE */
            uc_error_param = OAL_TRUE;
        }
        pst_tx_pwr_comp->ast_txratio2pwr[uc_level].us_tx_pwr_comp_val =
            (oal_uint16)hwifi_get_init_value(CUS_TAG_INI, uc_tx_pwr_comp_base + 2 * uc_level + 1);
        /* �жϷ��书�ʲ���ֵ�Ƿ���Ч */
        if (pst_tx_pwr_comp->ast_txratio2pwr[uc_level].us_tx_pwr_comp_val > TX_PWR_COMP_VAL_MAX) {
            OAM_ERROR_LOG2(0, OAM_SF_ANY,
                "{hwifi_config_init_ini_rf::cfg_id[%d]:tx_pwr_comp_val[%d] out of range, please check the value!}\r\n",
                uc_tx_pwr_comp_base + 2 * uc_level + 1, pst_tx_pwr_comp->ast_txratio2pwr[uc_level].us_tx_pwr_comp_val);
            /* ֵ������Ч��Χ�������ΪTRUE */
            uc_error_param = OAL_TRUE;
        }
    }
    pst_tx_pwr_comp->ul_more_pwr = (oal_uint32)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_MORE_PWR);
    /* �жϸ����¶ȶ��ⲹ���ķ��书��ֵ�Ƿ���Ч */
    if (pst_tx_pwr_comp->ul_more_pwr > MORE_PWR_MAX) {
        OAM_ERROR_LOG2(0, OAM_SF_ANY,
            "{hwifi_config_init_ini_rf::cfg_id[%d]:more_pwr[%d] out of range, please check the value!}\r\n",
            WLAN_CFG_INIT_MORE_PWR, pst_tx_pwr_comp->ul_more_pwr);
        /* ֵ������Ч��Χ�������ΪTRUE */
        uc_error_param = OAL_TRUE;
    }

    /* ��������������в���ȷ�ģ�ֱ�ӷ��� */
    if (uc_error_param) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY,
            "{hwifi_config_init_ini_rf::one or more params have wrong value, do not send cfg event!}\r\n");
        return;
    }
    /* ������в���������Ч��Χ�ڣ����·�����ֵ */
    l_ret = wal_send_cfg_event(pst_cfg_net_dev, WAL_MSG_TYPE_WRITE,
        WAL_MSG_WRITE_MSG_HDR_LENGTH + sizeof(mac_cfg_customize_rf) + sizeof(mac_cfg_customize_tx_pwr_comp_stru),
        (oal_uint8 *)&st_write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{EVENT[wal_send_cfg_event] failed, return err code [%d]!}\r\n", l_ret);
    }
}


uint8_t get_txpwr_and_scale_from_ini(mac_cus_band_edge_limit_stru *band_edge_limit)
{
    uint8_t error_param = OAL_FALSE;
    const uint8_t fcc_auth_band_num = MAC_NUM_2G_BAND;
    uint8_t idx;
    uint8_t max_txpwr;
    uint8_t dbb_scale;

    /* ��ֵidx��txpwr */
    for (idx = 0; idx < fcc_auth_band_num; idx++) {
        max_txpwr = (uint8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_BAND_EDGE_LIMIT_TXPWR_START + idx);
        if (OAL_VALUE_IN_VALID_RANGE2(max_txpwr, error_param, MAX_TXPOWER_MIN, MAX_TXPOWER_MAX)) {
            band_edge_limit[idx].uc_index = idx;
            band_edge_limit[idx].uc_max_txpower = max_txpwr;
        } else {
            /* ֵ������Ч��Χ�������ΪTRUE */
            error_param = OAL_TRUE;
            OAM_ERROR_LOG2(0, OAM_SF_ANY, "{hwifi_config_init_dts_cali::[dts]value out of range, config id[%d],%d}",
                WLAN_CFG_DTS_BAND_EDGE_LIMIT_TXPWR_START + idx, max_txpwr);
            OAM_ERROR_LOG2(0, OAM_SF_ANY, "MIN=%d MAX=%d", MAX_TXPOWER_MIN, MAX_TXPOWER_MAX);
        }
    }
    /* ��ֵscale */
    for (idx = 0; idx < fcc_auth_band_num; idx++) {
        dbb_scale = (uint8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_BAND_EDGE_LIMIT_SCALE_START + idx);
        if (dbb_scale > 0 && dbb_scale <= MAX_DBB_SCALE && !error_param) {
            band_edge_limit[idx].uc_dbb_scale = dbb_scale;
        } else {
            /* ֵ������Ч��Χ�������ΪTRUE */
            error_param = OAL_TRUE;
            OAM_ERROR_LOG2(0, OAM_SF_ANY, "{hwifi_config_init_dts_cali::[dts]value out of range, config id[%d] %d}",
                WLAN_CFG_DTS_BAND_EDGE_LIMIT_SCALE_START + idx, dbb_scale);
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "MAX=%d", MAX_DBB_SCALE);
        }
    }
    return error_param;
}


OAL_STATIC oal_uint32 hwifi_config_init_ini_cali(oal_net_device_stru *pst_cfg_net_dev)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    mac_cus_dts_cali_stru st_cus_cali;
    oal_uint8 uc_error_param = OAL_FALSE; /* ������Ч�Ա�־���������ֵ���Ϸ�����Ϊ1�����в������·� */
    oal_uint8 uc_idx;                 /* �ṹ�������±� */
    oal_uint32 ul_offset = 0;
    oal_uint32 l_memcpy_ret = EOK;
    mac_cus_band_edge_limit_stru *pst_band_edge_limit = OAL_PTR_NULL;
    host_cali_stru *cali_data = OAL_PTR_NULL;
    uint8_t cali_permission = OAL_TRUE;

    memset_s(&st_write_msg, OAL_SIZEOF(wal_msg_write_stru), 0, OAL_SIZEOF(wal_msg_write_stru));
    memset_s(&st_cus_cali, OAL_SIZEOF(mac_cus_dts_cali_stru), 0, OAL_SIZEOF(mac_cus_dts_cali_stru));

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    cali_data = (host_cali_stru *)get_cali_data_buf_addr();
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "hwifi_config_init_ini_cali cali_save_flag = %d", cali_data->saved_flag);
    if (cali_data->saved_flag == OAL_TRUE) {
        wal_send_cali_data(pst_cfg_net_dev);
        cali_permission = api_get_wifi_cali_apply();
    }
#endif
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "hwifi_config_init_ini_cali uc_cali_permission = %d", cali_permission);
    /* ����: TXPWR_PA_DC_REF */
    /* 2G REF: ��3���ŵ� */
    for (uc_idx = 0; uc_idx < MAC_NUM_2G_BAND; uc_idx++) {
        oal_int16 s_ref_val = (oal_int16)hwifi_get_init_value(CUS_TAG_INI,
            WLAN_CFG_INIT_RF_TXPWR_CALI_REF_2G_VAL_BAND1 + uc_idx);
        if (s_ref_val >= CALI_TXPWR_PA_DC_REF_MIN && s_ref_val <= CALI_TXPWR_PA_DC_REF_MAX) {
            st_cus_cali.aus_cali_txpwr_pa_dc_ref_2g_val[uc_idx] = s_ref_val;
        } else {
            /* ֵ������Ч��Χ�������ΪTRUE */
            uc_error_param = OAL_TRUE;
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hwifi_config_init_dts_cali::[dts]2g ref value out of range, config id[%d], "
                "check the value in dts file!}\r\n", WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_START + uc_idx);
        }
    }

    /* ��������������в���ȷ�ģ�ֱ�ӷ��� */
    if (uc_error_param == OAL_TRUE) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hwifi_config_init_dts_cali::params have wrong value,not send cfg event!}\r\n");
        return OAL_FAIL;
    }

    /* ������в���������Ч��Χ�ڣ����·�����ֵ */
    l_memcpy_ret += memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, (oal_int8 *)&st_cus_cali,
        OAL_SIZEOF(mac_cus_dts_cali_stru));
    ul_offset += OAL_SIZEOF(mac_cus_dts_cali_stru);

    /* ����: FCC��֤ */
    /* �����ڴ��űߴ�������Ϣ,�������������ͷ�,�����ڴ��С: 6 * 4 = 24�ֽ� */
    pst_band_edge_limit = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL,
        NUM_OF_BAND_EDGE_LIMIT * OAL_SIZEOF(mac_cus_band_edge_limit_stru), OAL_TRUE);
    if (pst_band_edge_limit == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hwifi_config_init_dts_cali::alloc fcc auth mem fail, return null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_error_param = get_txpwr_and_scale_from_ini(pst_band_edge_limit);
    /* ��������������в���ȷ�ģ�ֱ�ӷ��� */
    if (uc_error_param) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{one or more params have wrong value, do not send cfg event!}\r\n");
        /* �ͷ�pst_band_edge_limit�ڴ� */
        OAL_MEM_FREE(pst_band_edge_limit, OAL_TRUE);
        return OAL_FAIL;
    }

    l_memcpy_ret += memcpy_s(st_write_msg.auc_value + ul_offset, WAL_MSG_WRITE_MAX_LEN - ul_offset,
        (oal_int8 *)pst_band_edge_limit, NUM_OF_BAND_EDGE_LIMIT * OAL_SIZEOF(mac_cus_band_edge_limit_stru));
    ul_offset += (NUM_OF_BAND_EDGE_LIMIT * OAL_SIZEOF(mac_cus_band_edge_limit_stru));

    l_memcpy_ret += memcpy_s(st_write_msg.auc_value + ul_offset, WAL_MSG_WRITE_MAX_LEN - ul_offset, &cali_permission,
        OAL_SIZEOF(int8_t));
    ul_offset += OAL_SIZEOF(int8_t);
    if (l_memcpy_ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hwifi_config_init_ini_rf::memcpy_s failed!");
        OAL_MEM_FREE(pst_band_edge_limit, OAL_TRUE);
        return OAL_FAIL;
    }

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_CUS_DTS_CALI, ul_offset);
    l_ret = wal_send_cfg_event(pst_cfg_net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + ul_offset,
        (oal_uint8 *)&st_write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_send_cfg_event failed, error no[%d]!}\r\n", l_ret);
        OAL_MEM_FREE(pst_band_edge_limit, OAL_TRUE);
        return (oal_uint32)l_ret;
    }
    OAL_MEM_FREE(pst_band_edge_limit, OAL_TRUE);
    return OAL_SUCC;
}

#if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1131C_HOST)

OAL_STATIC oal_uint32 hwifi_config_init_dts_cali(oal_net_device_stru *pst_cfg_net_dev)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    mac_cus_dts_rf_reg st_rf_reg;
    mac_cus_dts_cali_stru st_cus_cali;
    oal_uint32 ul_cfg_id;
    oal_int8 *pc_tmp = OAL_PTR_NULL;      /* ����ָ�룬���Ա���wifi btУ׼ֵ */
    oal_int32 l_cfg_value;                /* ��ʱ��������Ӷ��ƻ�get�ӿ��л�ȡ��ֵ */
    oal_uint8 uc_error_param = OAL_FALSE; /* ������Ч�Ա�־���������ֵ���Ϸ�����Ϊ1�����в������·� */
    mac_cus_band_edge_limit_stru *pst_band_edge_limit = OAL_PTR_NULL;
    oal_uint8 uc_idx; /* �ṹ�������±� */
    oal_uint32 ul_offset = 0;
    oal_uint8 uc_fcc_auth_band_num;        /* ʵ����Ҫ�������õ�FCC��֤band����2g:3��,5g:3�� */
    oal_uint16 *pus_rf_reg = OAL_PTR_NULL; /* ָ��rf���õĵ�һ���Ĵ��� */
    oal_uint32 l_memcpy_ret = EOK;

    /* ����: TXPWR_PA_DC_REF */
    /* 2G REF: ��13���ŵ� */
    for (uc_idx = 0; uc_idx < 13; ++uc_idx) {
        oal_int16 s_ref_val =
            (oal_int16)hwifi_get_init_value(CUS_TAG_DTS, WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_START + uc_idx);
        if (OAL_VALUE_IN_VALID_RANGE(s_ref_val, 0, CALI_TXPWR_PA_DC_REF_MAX)) {
            st_cus_cali.aus_cali_txpwr_pa_dc_ref_2g_val[uc_idx] = s_ref_val;
        } else {
            /* ֵ������Ч��Χ�������ΪTRUE */
            uc_error_param = OAL_TRUE;
            OAM_ERROR_LOG1(0, OAM_SF_ANY,
                "{hwifi_config_init_dts_cali::[dts]2g ref value out of range, config id[%d], check the value in dts "
                "file!}\r\n",
                WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_START + uc_idx);
        }
    }
#ifdef _PRE_WLAN_FEATURE_5G
    /* 5G REF: ��7��band */
    if (band_5g_enabled) {
        oal_int16 *ps_ref_5g = &st_cus_cali.us_cali_txpwr_pa_dc_ref_5g_val_band1;
        for (uc_idx = 0; uc_idx < 7; ++uc_idx) {
            oal_int16 s_ref_val =
                (oal_int16)hwifi_get_init_value(CUS_TAG_DTS, WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_START + uc_idx);
            if (s_ref_val >= 0 && s_ref_val <= CALI_TXPWR_PA_DC_REF_MAX) {
                *(ps_ref_5g + uc_idx) = s_ref_val;
            } else {
                /* ֵ������Ч��Χ�������ΪTRUE */
                uc_error_param = OAL_TRUE;
                OAM_ERROR_LOG1(0, OAM_SF_ANY,
                    "{hwifi_config_init_dts_cali::[dts]5g ref value out of range, config id[%d], check the value in "
                    "dts file!}\r\n",
                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_START + uc_idx);
            }
        }
    }
#endif /* _PRE_WLAN_FEATURE_5G */
    /* ����: BAND 5G ENABLE */
    st_cus_cali.uc_band_5g_enable = !!band_5g_enabled;

    /* ����: �������ȵ�λ */
    st_cus_cali.uc_tone_amp_grade = (oal_uint8)hwifi_get_init_value(CUS_TAG_DTS, WLAN_CFG_DTS_CALI_TONE_AMP_GRADE);
    st_cus_cali.uc_bt_tone_amp_grade =
        (oal_uint8)hwifi_get_init_value(CUS_TAG_DTS, WLAN_CFG_DTS_BT_CALI_TONE_AMP_GRADE);

    /* ����: FCC��֤ */
    /* �����ڴ��űߴ�������Ϣ,�������������ͷ�,�����ڴ��С: 6                      *            4 = 24�ֽ� */
    pst_band_edge_limit = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL,
        NUM_OF_BAND_EDGE_LIMIT * OAL_SIZEOF(mac_cus_band_edge_limit_stru), OAL_TRUE);
    if (pst_band_edge_limit == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hwifi_config_init_dts_cali::alloc fcc auth mem fail, return null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* ʵ����Ҫ�������õ�FCC��֤band�� */
    uc_fcc_auth_band_num = band_5g_enabled ? NUM_OF_BAND_EDGE_LIMIT : NUM_OF_BAND_EDGE_LIMIT / 2;
    /* ��ֵidx��txpwr */
    for (uc_idx = 0; uc_idx < uc_fcc_auth_band_num; ++uc_idx) {
        oal_uint8 uc_max_txpwr =
            (oal_uint8)hwifi_get_init_value(CUS_TAG_DTS, WLAN_CFG_DTS_BAND_EDGE_LIMIT_TXPWR_START + uc_idx);
        if (uc_max_txpwr >= MAX_TXPOWER_MIN && uc_max_txpwr <= MAX_TXPOWER_MAX && !uc_error_param) {
            pst_band_edge_limit[uc_idx].uc_index = uc_idx;
            pst_band_edge_limit[uc_idx].uc_max_txpower = uc_max_txpwr;
        } else {
            /* ֵ������Ч��Χ�������ΪTRUE */
            uc_error_param = OAL_TRUE;
            OAM_ERROR_LOG1(0, OAM_SF_ANY,
                "{hwifi_config_init_dts_cali::[dts]value out of range, config id[%d], check the value in dts "
                "file!}\r\n",
                WLAN_CFG_DTS_BAND_EDGE_LIMIT_TXPWR_START + uc_idx);
        }
    }
    /* ��ֵscale */
    for (uc_idx = 0; uc_idx < uc_fcc_auth_band_num; ++uc_idx) {
        oal_uint8 uc_dbb_scale =
            (oal_uint8)hwifi_get_init_value(CUS_TAG_DTS, WLAN_CFG_DTS_BAND_EDGE_LIMIT_SCALE_START + uc_idx);
        if (uc_dbb_scale > 0 && uc_dbb_scale <= MAX_DBB_SCALE && !uc_error_param) {
            pst_band_edge_limit[uc_idx].uc_dbb_scale = uc_dbb_scale;
        } else {
            /* ֵ������Ч��Χ�������ΪTRUE */
            uc_error_param = OAL_TRUE;
            OAM_ERROR_LOG1(0, OAM_SF_ANY,
                "{hwifi_config_init_dts_cali::[dts]value out of range, config id[%d], check the value in dts "
                "file!}\r\n",
                WLAN_CFG_DTS_BAND_EDGE_LIMIT_SCALE_START + uc_idx);
        }
    }

    /* bt У׼ֵ */
    pc_tmp = (oal_int8 *)&st_cus_cali.us_cali_bt_txpwr_pa_ref_band1;
    for (ul_cfg_id = WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND1; ul_cfg_id <= WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND8;
        ++ul_cfg_id) {
        l_cfg_value = hwifi_get_init_value(CUS_TAG_DTS, ul_cfg_id);
        if (l_cfg_value >= CALI_TXPWR_PA_DC_REF_MIN && l_cfg_value <= CALI_BT_TXPWR_PA_DC_REF_MAX) {
            *(oal_uint16 *)pc_tmp = l_cfg_value;
            pc_tmp += 2;
        } else {
            /* ֵ������Ч��Χ�������ΪTRUE */
            uc_error_param = OAL_TRUE;
            OAM_ERROR_LOG1(0, OAM_SF_ANY,
                "{hwifi_config_init_dts_cali::[dts]value out of range, config id[%d], check the value in dts "
                "file!}\r\n",
                ul_cfg_id);
        }
    }
    /* bt Ƶ��ֵ */
    pc_tmp = (oal_int8 *)&st_cus_cali.us_cali_bt_txpwr_numb;
    for (ul_cfg_id = WLAN_CFG_DTS_BT_CALI_TXPWR_PA_NUM; ul_cfg_id <= WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE8; ++ul_cfg_id) {
        l_cfg_value = hwifi_get_init_value(CUS_TAG_DTS, ul_cfg_id);
        if (l_cfg_value >= CALI_TXPWR_PA_DC_FRE_MIN && l_cfg_value <= CALI_TXPWR_PA_DC_FRE_MAX) {
            *(oal_uint16 *)pc_tmp = l_cfg_value;
            pc_tmp += 2;
        } else {
            /* ֵ������Ч��Χ�������ΪTRUE */
            uc_error_param = OAL_TRUE;
            OAM_ERROR_LOG1(0, OAM_SF_ANY,
                "{hwifi_config_init_dts_cali::[dts]value out of range, config id[%d], check the value in dts "
                "file!}\r\n",
                ul_cfg_id);
        }
    }

    /* ����: RF�Ĵ��� */
    pus_rf_reg = (oal_uint16 *)&st_rf_reg;
    for (ul_cfg_id = WLAN_CFG_DTS_RF_FIRST; ul_cfg_id <= WLAN_CFG_DTS_RF_LAST; ++ul_cfg_id) {
        oal_uint16 us_reg_val = (oal_uint16)hwifi_get_init_value(CUS_TAG_DTS, ul_cfg_id);
        *(pus_rf_reg + ul_cfg_id - WLAN_CFG_DTS_RF_FIRST) = us_reg_val;
    }

    /* ��������������в���ȷ�ģ�ֱ�ӷ��� */
    if (uc_error_param) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY,
            "{hwifi_config_init_ini_rf::one or more params have wrong value, do not send cfg event!}\r\n");
        /* �ͷ�pst_band_edge_limit�ڴ� */
        OAL_MEM_FREE(pst_band_edge_limit, OAL_TRUE);
        return OAL_FAIL;
    }
    /* ������в���������Ч��Χ�ڣ����·�����ֵ */
    l_memcpy_ret += memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, (oal_int8 *)&st_cus_cali,
        OAL_SIZEOF(mac_cus_dts_cali_stru));
    ul_offset += OAL_SIZEOF(mac_cus_dts_cali_stru);
    l_memcpy_ret += memcpy_s(st_write_msg.auc_value + ul_offset, WAL_MSG_WRITE_MAX_LEN - ul_offset,
        (oal_int8 *)pst_band_edge_limit, NUM_OF_BAND_EDGE_LIMIT * OAL_SIZEOF(mac_cus_band_edge_limit_stru));
    ul_offset += (NUM_OF_BAND_EDGE_LIMIT * OAL_SIZEOF(mac_cus_band_edge_limit_stru));
    l_memcpy_ret += memcpy_s(st_write_msg.auc_value + ul_offset, WAL_MSG_WRITE_MAX_LEN - ul_offset,
        (oal_int8 *)&st_rf_reg, OAL_SIZEOF(mac_cus_dts_rf_reg));
    if (l_memcpy_ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hwifi_config_init_dts_cali::memcpy_s failed!");
        OAL_MEM_FREE(pst_band_edge_limit, OAL_TRUE);
        return OAL_FAIL;
    }
    ul_offset += OAL_SIZEOF(mac_cus_dts_rf_reg);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_CUS_DTS_CALI, ul_offset);
    l_ret = wal_send_cfg_event(pst_cfg_net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + ul_offset,
        (oal_uint8 *)&st_write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hwifi_config_init_dts_cali::wal_send_cfg_event failed,l_ret[%d]!}\r\n", l_ret);
        OAL_MEM_FREE(pst_band_edge_limit, OAL_TRUE);
        return (oal_uint32)l_ret;
    }
    /* �ͷ�pst_band_edge_limit�ڴ� */
    OAL_MEM_FREE(pst_band_edge_limit, OAL_TRUE);
    return OAL_SUCC;
}
#endif /* #if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1131C_HOST) */


oal_void hwifi_config_init_dbb_main(oal_net_device_stru *pst_cfg_net_dev)
{
    dbb_scaling_stru st_dbb_scaling_params;

    oal_uint8 uc_idx;
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint32 ul_offset = 0;

    memset_s(&st_dbb_scaling_params, OAL_SIZEOF(dbb_scaling_stru), 0, OAL_SIZEOF(dbb_scaling_stru));
    memset_s(&st_write_msg, OAL_SIZEOF(wal_msg_write_stru), 0, OAL_SIZEOF(wal_msg_write_stru));
    /* �ṹ�����鸳ֵ */
    for (uc_idx = 0; uc_idx < 9; uc_idx++) {
        oal_uint32 ul_ref_val =
            (oal_uint32)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_PHY_SCALING_VALUE_11B + uc_idx);
        st_dbb_scaling_params.ul_dbb_scale[uc_idx] = ul_ref_val;
    }

    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, (oal_int8 *)&st_dbb_scaling_params,
        OAL_SIZEOF(dbb_scaling_stru)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hwifi_config_init_nvram_main::memcpy_s failed!");
        return;
    }
    ul_offset += OAL_SIZEOF(dbb_scaling_stru);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_CUS_NVRAM_PARAM, ul_offset);

    l_ret = wal_send_cfg_event(pst_cfg_net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + ul_offset,
        (oal_uint8 *)&st_write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hwifi_config_init_nvram_main::return err code [%d]!}\r\n", l_ret);
    }
}


OAL_STATIC oal_void hwifi_config_init_ini_main(oal_net_device_stru *pst_cfg_net_dev)
{
    /* У׼ */
    hwifi_config_init_ini_cali(pst_cfg_net_dev);
    /* DBB scaling */
    hwifi_config_init_dbb_main(pst_cfg_net_dev);
    /* ���� */
    hwifi_config_init_ini_perf(pst_cfg_net_dev);
    /* LINKLOSS */
    hwifi_config_init_ini_linkloss(pst_cfg_net_dev);
    /* ������ */
    hwifi_config_init_ini_country(pst_cfg_net_dev);
    /* �Զ���Ƶ */
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
    hwifi_config_init_ini_auto_freq(pst_cfg_net_dev);
#endif
    /* �͹��� */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    hwifi_config_init_ini_pm(pst_cfg_net_dev);
#endif
    /* ��ά�ɲ� */
    hwifi_config_init_ini_log(pst_cfg_net_dev);
    /* PHY�㷨 */
    hwifi_config_init_ini_phy(pst_cfg_net_dev);
    /* ʱ�� */
    hwifi_config_init_ini_clock(pst_cfg_net_dev);
    /* RF */
    hwifi_config_init_ini_rf(pst_cfg_net_dev);
    /* �߹��Ŀ��� */
    hwifi_config_init_ini_high_power(pst_cfg_net_dev);
#ifdef _PRE_WLAN_FEATURE_1131K_BTCOEX
    hwifi_config_init_ini_btcoex_channel(pst_cfg_net_dev);
#endif
}

#if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1131C_HOST)


oal_void hwifi_config_init_nvram_main(oal_net_device_stru *pst_cfg_net_dev)
{
    /* nvram �����ṹ�� */
    struct nvram_params_stru {
        oal_uint8 uc_index;       /* �±��ʾƫ�� */
        oal_uint8 uc_max_txpower; /* ����͹��� */
        oal_uint8 uc_dbb_scale;   /* DBB��ֵ */
        oal_uint8 uc_resv[1];
    };
    struct nvram_params_stru *pst_nvram_params;
    oal_uint8 auc_nv_params[NUM_OF_NV_PARAMS];
    oal_uint8 uc_dpd_enable;
    oal_uint8 uc_idx;
    oal_uint8 uc_error_param = OAL_FALSE; /* �������Ϸ�����־1�����·� */
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint32 ul_offset = NUM_OF_NV_MAX_TXPOWER * OAL_SIZEOF(struct nvram_params_stru);
    oal_uint32 l_memcpy_ret = EOK;

    l_memcpy_ret +=
        memcpy_s(auc_nv_params, OAL_SIZEOF(auc_nv_params), hwifi_get_nvram_params(), OAL_SIZEOF(auc_nv_params));

    /* �����ڴ���NVRAM������Ϣ,�������������ͷ�,�ڴ��С:46 * 4 = 164�ֽ� */
    pst_nvram_params = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, ul_offset, OAL_TRUE);
    if (pst_nvram_params == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hwifi_config_init_nvram_main::alloc nvram params mem fail!}\r\n");
        return;
    }
    /* �ṹ�����鸳ֵ */
    for (uc_idx = 0; uc_idx < NUM_OF_NV_MAX_TXPOWER; ++uc_idx) {
        pst_nvram_params[uc_idx].uc_index = uc_idx;
        if (auc_nv_params[2 * uc_idx] > 0 && auc_nv_params[2 * uc_idx] <= MAX_TXPOWER_MAX &&
            auc_nv_params[2 * uc_idx + 1] <= 0xEE) {
            pst_nvram_params[uc_idx].uc_max_txpower = auc_nv_params[2 * uc_idx];
            pst_nvram_params[uc_idx].uc_dbb_scale = auc_nv_params[2 * uc_idx + 1];
        } else {
            uc_error_param = OAL_TRUE;
            OAM_ERROR_LOG3(0, OAM_SF_ANY,
                "{hwifi_config_init_nvram_main:: %dth pwr[0x%x] or scale[0x%x] or both out of range, check the value "
                "in dts file or nvm!}\r\n",
                uc_idx, auc_nv_params[2 * uc_idx], auc_nv_params[2 * uc_idx + 1]);
        }
    }
    /* ��ȡdpd enable, nvĬ�ϲ�д����ֵ������0��ʾ�򿪣�1��ʾ�ر� */
    uc_dpd_enable = !auc_nv_params[2 * uc_idx];

    if (uc_error_param) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY,
            "{hwifi_config_init_nvram_main::one or more params not correct, check value in dts file or nvram!}\r\n");
        /* �ͷ�pst_nvram_params�ڴ� */
        OAL_MEM_FREE(pst_nvram_params, OAL_TRUE);
        return;
    }

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_CUS_NVRAM_PARAM, ul_offset + OAL_SIZEOF(oal_int32));
    l_memcpy_ret += memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, (oal_int8 *)pst_nvram_params, ul_offset);
    if (l_memcpy_ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hwifi_config_init_nvram_main::memcpy_s failed!");
        OAL_MEM_FREE(pst_nvram_params, OAL_TRUE);
        return;
    }
    *(&st_write_msg.auc_value[0] + ul_offset) = uc_dpd_enable;

    l_ret = wal_send_cfg_event(pst_cfg_net_dev, WAL_MSG_TYPE_WRITE,
        WAL_MSG_WRITE_MSG_HDR_LENGTH + ul_offset + OAL_SIZEOF(oal_int32), (oal_uint8 *)&st_write_msg, OAL_FALSE,
        OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hwifi_config_init_nvram_main::return err code [%d]!}\r\n", l_ret);
    }

    /* �ͷ�pst_nvram_params�ڴ� */
    OAL_MEM_FREE(pst_nvram_params, OAL_TRUE);
}

oal_uint32 hwifi_config_init_dts_main(oal_net_device_stru *pst_cfg_net_dev)
{
    /* У׼ */
    return hwifi_config_init_dts_cali(pst_cfg_net_dev);
}
#endif /* #if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1131C_HOST) */

OAL_STATIC oal_int32 hwifi_config_init_ini_wlan(oal_net_device_stru *pst_net_dev)
{
    return OAL_SUCC;
}

OAL_STATIC oal_int32 hwifi_config_init_ini_p2p(oal_net_device_stru *pst_net_dev)
{
    return OAL_SUCC;
}


oal_int32 hwifi_config_init_ini(oal_net_device_stru *pst_net_dev)
{
    oal_net_device_stru   *pst_cfg_net_dev = OAL_PTR_NULL;
    mac_vap_stru          *pst_mac_vap = OAL_PTR_NULL;
    oal_wireless_dev_stru *pst_wdev = OAL_PTR_NULL;
    mac_wiphy_priv_stru   *pst_wiphy_priv = OAL_PTR_NULL;
    mac_vap_stru          *pst_cfg_mac_vap = OAL_PTR_NULL;
    hmac_vap_stru         *pst_cfg_hmac_vap = OAL_PTR_NULL;
    mac_device_stru       *pst_mac_device = OAL_PTR_NULL;

    if (pst_net_dev == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hwifi_config_init_ini::pst_net_dev is null!}\r\n");
        return -OAL_EINVAL;
    }

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (pst_mac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hwifi_config_init_ini::pst_mac_vap is null}\r\n");
        return -OAL_EINVAL;
    }

    pst_wdev = OAL_NETDEVICE_WDEV(pst_net_dev);
    if (pst_wdev == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hwifi_config_init_ini::pst_wdev is null!}\r\n");
        return -OAL_EFAUL;
    }

    pst_wiphy_priv = (mac_wiphy_priv_stru *)oal_wiphy_priv(pst_wdev->wiphy);
    if (pst_wiphy_priv == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hwifi_config_init_ini::pst_wiphy_priv is null!}\r\n");
        return -OAL_EFAUL;
    }

    pst_mac_device = pst_wiphy_priv->pst_mac_device;

    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_vap::pst_mac_device is null!}\r\n");
        return -OAL_EFAUL;
    }

    pst_cfg_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->uc_cfg_vap_id);
    if (pst_cfg_mac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_init_wlan_vap::pst_cfg_mac_vap is null, vap_id:%d!}\r\n",
            pst_mac_device->uc_cfg_vap_id);
        return -OAL_EFAUL;
    }
    pst_cfg_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_device->uc_cfg_vap_id);
    if (pst_cfg_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_init_wlan_vap::pst_cfg_hmac_vap is null, vap_id:%d!}\r\n",
            pst_mac_device->uc_cfg_vap_id);
        return -OAL_EFAUL;
    }

    pst_cfg_net_dev = pst_cfg_hmac_vap->pst_net_device;

    if (pst_cfg_net_dev == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_vap::pst_cfg_net_dev is null!}\r\n");
        return -OAL_EFAUL;
    }

    if ((pst_wdev->iftype == NL80211_IFTYPE_STATION) || (pst_wdev->iftype == NL80211_IFTYPE_P2P_DEVICE) ||
        (pst_wdev->iftype == NL80211_IFTYPE_AP)) {
        if (!g_uc_cfg_once_flag) {
#if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1131C_HOST)
            /* 1131ֻ����ini�ļ���ʽ,������dts��nv��ʽ���ж��ƻ��������� */
            if (pst_wdev->iftype == NL80211_IFTYPE_AP) {
                hwifi_config_init_dts_main(pst_cfg_net_dev);
            }
            hwifi_config_init_nvram_main(pst_cfg_net_dev);
#endif
            hwifi_config_init_ini_main(pst_cfg_net_dev);
            g_uc_cfg_once_flag = OAL_TRUE;
        }
        if ((oal_strcmp("wlan0", pst_net_dev->name)) == 0) {
            hwifi_config_init_ini_wlan(pst_net_dev);
        }
#ifdef _PRE_WLAN_FEATURE_P2P
        else if ((oal_strcmp("p2p0", pst_net_dev->name)) == 0) {
            hwifi_config_init_ini_p2p(pst_net_dev);
        }
#endif
        else {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hwifi_config_init_ini::net_dev is not wlan0 or p2p0!}\r\n");
            return OAL_SUCC;
        }
    }

    return OAL_SUCC;
}


oal_void hwifi_config_init_force(oal_void)
{
    /* �����ϵ�ʱ��ΪFALSE */
    g_uc_cfg_once_flag = OAL_FALSE;
    if (!g_uc_cfg_flag) {
        {
            hwifi_config_init(CUS_TAG_INI);
            hwifi_config_host_global_ini_param();
        }
        g_uc_cfg_flag = OAL_TRUE;
    }
}

void hwifi_config_init_once(void)
{
    hwifi_config_init(CUS_TAG_INI);
    hwifi_config_host_global_ini_param();
}
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && \
    ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION))
#ifdef _PRE_WLAN_FEATURE_DFR

OAL_STATIC oal_bool_enum_uint8 wal_dfr_recovery_check(oal_net_device_stru *pst_net_dev)
{
    oal_wireless_dev_stru *pst_wdev = OAL_PTR_NULL;
    mac_wiphy_priv_stru   *pst_wiphy_priv = OAL_PTR_NULL;
    mac_device_stru       *pst_mac_device = OAL_PTR_NULL;

    if (pst_net_dev == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_dfr_recovery_check::pst_net_dev is null!}\r\n");
        return OAL_FALSE;
    }

    pst_wdev = OAL_NETDEVICE_WDEV(pst_net_dev);
    if (pst_wdev == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_dfr_recovery_check::pst_wdev is null!}\r\n");
        return OAL_FALSE;
    }

    pst_wiphy_priv = (mac_wiphy_priv_stru *)oal_wiphy_priv(pst_wdev->wiphy);
    if (pst_wiphy_priv == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_dfr_recovery_check::pst_wiphy_priv is null!}\r\n");
        return OAL_FALSE;
    }

    pst_mac_device = pst_wiphy_priv->pst_mac_device;
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_dfr_recovery_check::pst_mac_device is null!}\r\n");
        return OAL_FALSE;
    }

    OAM_WARNING_LOG2(0, OAM_SF_ANY, "{wal_dfr_recovery_check::recovery_flag:%d, uc_vap_num:%d.}\r\n",
        g_st_dfr_info.bit_ready_to_recovery_flag, pst_mac_device->uc_vap_num);

    if ((g_st_dfr_info.bit_ready_to_recovery_flag == OAL_TRUE) && (!pst_mac_device->uc_vap_num)) {
        /* DFR�ָ�,�ڴ���ҵ��VAPǰ�·�У׼�Ȳ���,ֻ�·�һ�� */
        return OAL_TRUE;
    }

    return OAL_FALSE;
}
#endif /* #ifdef _PRE_WLAN_FEATURE_DFR */
#endif


OAL_STATIC void wal_netdev_open_flush_p2p_random_mac(oal_net_device_stru *net_dev)
{
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    /*
     * p2p interface mac��ַ����ֻ������add virtual������,
     * ���ܱ�֤�ϲ��·������mac���µ�mib,�˴�ˢ��mac addr
     */
    if ((oal_strncmp("p2p-p2p0", net_dev->name, OAL_STRLEN("p2p-p2p0")) == 0) ||
        (oal_strncmp("p2p-wlan0", net_dev->name, OAL_STRLEN("p2p-wlan0")) == 0)) {
        wal_set_random_mac_to_mib(net_dev);
    }
#endif
}


OAL_STATIC oal_int32 _wal_netdev_open(oal_net_device_stru *pst_net_dev)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    wal_msg_stru *pst_rsp_msg = OAL_PTR_NULL;
    oal_uint32 ul_err_code;
#ifdef _PRE_WLAN_FEATURE_P2P
    wlan_p2p_mode_enum_uint8 en_p2p_mode;
    oal_wireless_dev_stru *pst_wdev = OAL_PTR_NULL;
#endif
    if (OAL_UNLIKELY(pst_net_dev == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_netdev_open::pst_net_dev is null ptr!}\r\n");
        return -OAL_EFAUL;
    }

    PS_PRINT_WARNING("wal_netdev_open,dev_name is:%.16s\n", pst_net_dev->name);
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_netdev_open::iftype:%d.!}\r\n", pst_net_dev->ieee80211_ptr->iftype);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && \
    ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION))
    g_uc_netdev_is_open = OAL_TRUE;
    l_ret = wlan_pm_open();
    if (l_ret == OAL_FAIL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_netdev_open::wlan_pm_open Fail!}\r\n");
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_OPEN, CHR_WIFI_DRV_ERROR_POWER_ON);
        return -OAL_EFAIL;
    } else if (l_ret != OAL_ERR_CODE_ALREADY_OPEN) {
#ifdef _PRE_WLAN_FEATURE_DFR
        wal_dfr_init_param();
#endif
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
        /* �����ϵ�ʱ��ΪFALSE */
        hwifi_config_init_force(); /* 1131C-debug */
#endif
        // �����ϵ糡�����·�����VAP
        l_ret = wal_cfg_vap_h2d_event(pst_net_dev);
        if (l_ret != OAL_SUCC) {
            OAL_IO_PRINT("wal_cfg_vap_h2d_event FAIL %d \r\n", l_ret);
            return -OAL_EFAIL;
        }
        OAL_IO_PRINT("wal_cfg_vap_h2d_event succ \r\n");
    }
#ifdef _PRE_WLAN_FEATURE_DFR
    else if (wal_dfr_recovery_check(pst_net_dev)) {
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
        g_uc_custom_cali_done = OAL_TRUE;
        hwifi_config_init_force(); /* 1131C-debug */
#endif
        // �����ϵ糡�����·�����VAP
        l_ret = wal_cfg_vap_h2d_event(pst_net_dev);
        if (l_ret != OAL_SUCC) {
            OAL_IO_PRINT("DFR:wal_cfg_vap_h2d_event FAIL %d \r\n", l_ret);
            return -OAL_EFAIL;
        }
        OAL_IO_PRINT("DFR:wal_cfg_vap_h2d_event succ \r\n");
    }
#endif
#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

    if (g_st_ap_config_info.l_ap_power_flag == OAL_TRUE) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_netdev_open::power state is on,in ap mode, start vap later.}\r\n");

        /* �˱�����ʱ��һ�Σ���ֹ Android framework����ģʽ�л�ǰ�·�����up���� */
        g_st_ap_config_info.l_ap_power_flag = OAL_FALSE;
        oal_net_tx_wake_all_queues(pst_net_dev); /* �������Ͷ��� */
        return OAL_SUCC;
    }

    /* �ϵ�host device_stru��ʼ�� */
    l_ret = wal_host_dev_init(pst_net_dev);
    if (l_ret != OAL_SUCC) {
        OAL_IO_PRINT("wal_host_dev_init FAIL %d \r\n", l_ret);
        return -OAL_EFAIL;
    }

    if (((pst_net_dev->ieee80211_ptr->iftype == NL80211_IFTYPE_STATION) ||
        (pst_net_dev->ieee80211_ptr->iftype == NL80211_IFTYPE_P2P_DEVICE)) &&
        (((oal_strcmp("wlan0", pst_net_dev->name)) == 0) || ((oal_strcmp("p2p0", pst_net_dev->name)) == 0))) {
        if ((wal_init_wlan_vap(pst_net_dev)) != OAL_SUCC) {
            chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_OPEN, CHR_WIFI_DRV_ERROR_POWER_ON);
            return -OAL_EFAIL;
        }
    } else if (pst_net_dev->ieee80211_ptr->iftype == NL80211_IFTYPE_AP) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_netdev_open::ap mode,no need to start vap.!}\r\n");
        oal_net_tx_wake_all_queues(pst_net_dev); /* �������Ͷ��� */
        return OAL_SUCC;
    }
#endif

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    hwifi_config_init_ini(pst_net_dev);
    wal_netdev_open_flush_p2p_random_mac(pst_net_dev);
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */
    /* **************************************************************************
        ���¼���wal�㴦��
    ************************************************************************** */
    /* ��д��Ϣ */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_START_VAP, OAL_SIZEOF(mac_cfg_start_vap_param_stru));
    ((mac_cfg_start_vap_param_stru *)st_write_msg.auc_value)->pst_net_dev = pst_net_dev;
#ifdef _PRE_WLAN_FEATURE_P2P
    pst_wdev = pst_net_dev->ieee80211_ptr;
    en_p2p_mode = wal_wireless_iftype_to_mac_p2p_mode(pst_wdev->iftype);
    if (en_p2p_mode == WLAN_P2P_BUTT) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_netdev_open::wal_wireless_iftype_to_mac_p2p_mode return BUFF}\r\n");
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_OPEN, CHR_WIFI_DRV_ERROR_POWER_ON);
        return -OAL_EINVAL;
    }
    ((mac_cfg_start_vap_param_stru *)st_write_msg.auc_value)->en_p2p_mode = en_p2p_mode;
#endif
    ((mac_cfg_start_vap_param_stru *)st_write_msg.auc_value)->en_mgmt_rate_init_flag = OAL_TRUE;

    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_start_vap_param_stru), (oal_uint8 *)&st_write_msg, OAL_TRUE, &pst_rsp_msg);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_netdev_open::wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* ��������Ϣ */
    ul_err_code = wal_check_and_release_msg_resp(pst_rsp_msg);
    if (ul_err_code != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_netdev_open::hmac start vap fail,err code[%u]!}\r\n", ul_err_code);
        return -OAL_EINVAL;
    }
    if ((OAL_NETDEVICE_FLAGS(pst_net_dev) & OAL_IFF_RUNNING) == 0) {
        OAL_NETDEVICE_FLAGS(pst_net_dev) |= OAL_IFF_RUNNING;
    }

    oal_net_tx_wake_all_queues(pst_net_dev); /* �������Ͷ��� */

    return OAL_SUCC;
}

oal_int32 wal_netdev_open(oal_net_device_stru *pst_net_dev)
{
    oal_int32 ret;

    if (pst_net_dev == OAL_PTR_NULL) {
        return -OAL_EFAUL;
    }
    if (OAL_NETDEVICE_FLAGS(pst_net_dev) & OAL_IFF_RUNNING) {
        return OAL_SUCC;
    }
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_mutex_lock(&g_dfr_mutex);
#endif
    wal_wake_lock();
    ret = _wal_netdev_open(pst_net_dev);
    wal_wake_unlock();
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_mutex_unlock(&g_dfr_mutex);
#endif

    if (ret != OAL_SUCC) {
        chr_exception(chr_wifi_drv(CHR_WIFI_DRV_EVENT_OPEN, CHR_WIFI_DRV_ERROR_POWER_ON));
    }
    return ret;
}
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE


oal_uint32 wal_custom_cali(oal_void)
{
    oal_net_device_stru *pst_net_dev;
    oal_uint32 ul_ret;

    pst_net_dev = oal_dev_get_by_name("Hisilicon0");
    if (pst_net_dev != OAL_PTR_NULL) {
        /* ����oal_dev_get_by_name�󣬱������oal_dev_putʹnet_dev�����ü�����һ */
        oal_dev_put(pst_net_dev);

        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hisi_customize_wifi host_module_init::the net_device is already exist!}\r\n");
    }

    /* �·����� */
    ul_ret = hwifi_config_init_ini_cali(pst_net_dev);

    return ul_ret;
}


int32_t wal_set_custom_process_func(custom_cali_func fun)
{
    struct custom_process_func_handler *pst_custom_process_func_handler;
    pst_custom_process_func_handler = oal_get_custom_process_func();
    if (pst_custom_process_func_handler == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_set_auto_freq_process_func get handler failed!}");
    } else {
        pst_custom_process_func_handler->p_custom_cali_func = fun;
    }

    return OAL_SUCC;
}
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)

OAL_STATIC oal_int32 wal_set_power_on(oal_net_device_stru *pst_net_dev, oal_int32 power_flag)
{
    oal_int32 l_ret;

    // ap���µ磬����VAP
    if (power_flag == 0) { // �µ�
        /* �µ�host device_struȥ��ʼ�� */
        wal_host_dev_exit(pst_net_dev);

        wal_wake_lock();
        wlan_pm_close();
        wal_wake_unlock();

        g_st_ap_config_info.l_ap_power_flag = OAL_FALSE;
    } else if (power_flag == 1) { // �ϵ�
        g_st_ap_config_info.l_ap_power_flag = OAL_TRUE;

        wal_wake_lock();
        g_uc_netdev_is_open = OAL_TRUE;
        l_ret = wlan_pm_open();
        wal_wake_unlock();
        if (l_ret == OAL_FAIL) {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_set_power_on::wlan_pm_open Fail!}\r\n");
            return -OAL_EFAIL;
        } else if (l_ret != OAL_ERR_CODE_ALREADY_OPEN) {
#ifdef _PRE_WLAN_FEATURE_DFR
            wal_dfr_init_param();
#endif
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
            /* �����ϵ�ʱ��ΪFALSE */
            hwifi_config_init_force();
#endif
            // �����ϵ糡�����·�����VAP
            l_ret = wal_cfg_vap_h2d_event(pst_net_dev);
            if (l_ret != OAL_SUCC) {
                return -OAL_EFAIL;
            }
        }

        /* �ϵ�host device_stru��ʼ�� */
        l_ret = wal_host_dev_init(pst_net_dev);
        if (l_ret != OAL_SUCC) {
            OAL_IO_PRINT("wal_set_power_on FAIL %d \r\n", l_ret);
            return -OAL_EFAIL;
        }
    } else {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_set_power_on::pupower_flag:%d error.}\r\n", power_flag);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_void wal_set_power_mgmt_on(oal_ulong power_mgmt_flag)
{
    struct wlan_pm_s *pst_wlan_pm = OAL_PTR_NULL;
    pst_wlan_pm = wlan_pm_get_drv();
    if (pst_wlan_pm != NULL) {
        /* apģʽ�£��Ƿ������µ����,1:����,0:������ */
        pst_wlan_pm->ul_apmode_allow_pm_flag = power_mgmt_flag;
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_set_power_mgmt_on::wlan_pm_get_drv return null.");
    }
}
#endif /*  (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) */

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

oal_int32 wal_netdev_stop_ap(oal_net_device_stru *pst_net_dev)
{
    oal_int32 l_ret;

    if (pst_net_dev->ieee80211_ptr->iftype != NL80211_IFTYPE_AP) {
        return OAL_SUCC;
    }

    /* ����ɨ��,�Է���20/40Mɨ������йر�AP */
    wal_force_scan_complete(pst_net_dev, OAL_TRUE);

    /* AP�ر��л���STAģʽ,ɾ�����vap */
    if (wal_stop_vap(pst_net_dev) != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{wal_netdev_stop_ap::wal_stop_vap enter a error.}");
    }
    l_ret = wal_deinit_wlan_vap(pst_net_dev);
    if (l_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{wal_netdev_stop_ap::wal_deinit_wlan_vap enter a error.}");
        return l_ret;
    }

    /* Del aput����Ҫ�л�netdev iftype״̬��station */
    pst_net_dev->ieee80211_ptr->iftype = NL80211_IFTYPE_STATION;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    /* aput�µ� */
    wal_set_power_mgmt_on(OAL_TRUE);
    l_ret = wal_set_power_on(pst_net_dev, OAL_FALSE);
    if (l_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_netdev_stop_ap::wal_set_power_on fail [%d]!}", l_ret);
        return l_ret;
    }
#endif /* (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) */
    return OAL_SUCC;
}


OAL_STATIC oal_int32 wal_netdev_stop_sta_p2p(oal_net_device_stru *pst_net_dev)
{
    oal_int32 l_ret;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    mac_device_stru *pst_mac_device = OAL_PTR_NULL;

    /* wlan0/p2p0 downʱ ɾ��VAP */
    if (((pst_net_dev->ieee80211_ptr->iftype == NL80211_IFTYPE_STATION) ||
        (pst_net_dev->ieee80211_ptr->iftype == NL80211_IFTYPE_P2P_DEVICE)) &&
        (((oal_strcmp("wlan0", pst_net_dev->name)) == 0) || ((oal_strcmp("p2p0", pst_net_dev->name)) == 0))) {
#ifdef _PRE_WLAN_FEATURE_P2P
        /* ����ɾ��p2pС�� */
        pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
        if (pst_mac_vap == OAL_PTR_NULL) {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_netdev_stop_sta_p2p::pst_mac_vap is null, netdev released.}\r\n");
            return OAL_SUCC;
        }
        pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
        /* ����ɾ��p2pС�� */
        if (pst_mac_device != NULL) {
            wal_del_p2p_group(pst_mac_device);
        }
#endif

        l_ret = wal_deinit_wlan_vap(pst_net_dev);
        if (l_ret != OAL_SUCC) {
            return l_ret;
        }
    }
    return OAL_SUCC;
}

#endif /* #if defined(_PRE_PRODUCT_ID_HI110X_HOST) */


OAL_STATIC oal_int32 _wal_netdev_stop(oal_net_device_stru *pst_net_dev)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_err_code;
    wal_msg_stru *pst_rsp_msg = OAL_PTR_NULL;
    oal_int32 l_ret;
#ifdef _PRE_WLAN_FEATURE_P2P
    wlan_p2p_mode_enum_uint8 en_p2p_mode;
    oal_wireless_dev_stru *pst_wdev = OAL_PTR_NULL;
#endif

    if (OAL_UNLIKELY(pst_net_dev == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_netdev_stop::pst_net_dev is null ptr!}\r\n");
        return -OAL_EFAUL;
    }

    /* stop the netdev's queues */
    oal_net_tx_stop_all_queues(pst_net_dev); /* ֹͣ���Ͷ��� */
    wal_force_scan_complete(pst_net_dev, OAL_TRUE);

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_netdev_stop::iftype:%d.!}\r\n", pst_net_dev->ieee80211_ptr->iftype);

    OAL_IO_PRINT("wal_netdev_stop,dev_name is:%.16s\n", pst_net_dev->name);

    /* APģʽ��,��ģʽ�л�ʱdown��ɾ�� vap */
    if (pst_net_dev->ieee80211_ptr->iftype == NL80211_IFTYPE_AP) {
        l_ret = wal_netdev_stop_ap(pst_net_dev);
        return l_ret;
    }
#endif

    /* ���netdev����running״̬����ֱ�ӷ��سɹ� */
    if ((OAL_NETDEVICE_FLAGS(pst_net_dev) & OAL_IFF_RUNNING) == 0) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_netdev_stop::vap is already down!}\r\n");
        return OAL_SUCC;
    }

    /* **************************************************************************
                           ���¼���wal�㴦��
    ************************************************************************** */
    /* ��дWID��Ϣ */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DOWN_VAP, OAL_SIZEOF(mac_cfg_down_vap_param_stru));
    ((mac_cfg_down_vap_param_stru *)st_write_msg.auc_value)->pst_net_dev = pst_net_dev;
#ifdef _PRE_WLAN_FEATURE_P2P
    pst_wdev = pst_net_dev->ieee80211_ptr;
    en_p2p_mode = wal_wireless_iftype_to_mac_p2p_mode(pst_wdev->iftype);
    if (en_p2p_mode == WLAN_P2P_BUTT) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_netdev_stop::wal_wireless_iftype_to_mac_p2p_mode return BUFF}\r\n");
        return -OAL_EINVAL;
    }
    ((mac_cfg_start_vap_param_stru *)st_write_msg.auc_value)->en_p2p_mode = en_p2p_mode;
#endif

    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE,
        WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_down_vap_param_stru), (oal_uint8 *)&st_write_msg, OAL_TRUE,
        &pst_rsp_msg);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        if (OAL_NET_DEV_PRIV(pst_net_dev) == OAL_PTR_NULL) {
            /* �ر�net_device���������Ӧvap ��null�����flags running��־ */
            OAL_NETDEVICE_FLAGS(pst_net_dev) &= (~OAL_IFF_RUNNING);
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_netdev_stop::net_device's vap is null, set flag not running}");
        }
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_netdev_stop::wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* ��������Ϣ */
    ul_err_code = wal_check_and_release_msg_resp(pst_rsp_msg);
    if (ul_err_code != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_netdev_stop::hmac stop vap fail!err code [%d]}\r\n", ul_err_code);
        return -OAL_EFAIL;
    }

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
    /* wlan0/p2p0 downʱ ɾ��VAP */
    l_ret = wal_netdev_stop_sta_p2p(pst_net_dev);
    if (l_ret != OAL_SUCC) {
        return l_ret;
    }
#endif

    return OAL_SUCC;
}

oal_int32 wal_netdev_stop(oal_net_device_stru *pst_net_dev)
{
    oal_int32 ret;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_mutex_lock(&g_dfr_mutex);
#endif
    wal_wake_lock();
    ret = _wal_netdev_stop(pst_net_dev);
    wal_wake_unlock();
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_mutex_unlock(&g_dfr_mutex);
#endif
    return ret;
}


OAL_STATIC oal_net_device_stats_stru* wal_netdev_get_stats(oal_net_device_stru *pst_net_dev)
{
    oal_net_device_stats_stru *pst_stats = OAL_PTR_NULL;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    oam_stat_info_stru *pst_oam_stat = OAL_PTR_NULL;
    oam_vap_stat_info_stru *pst_oam_vap_stat = OAL_PTR_NULL;

    if (pst_net_dev == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_netdev_get_stats netdev is null");
        return OAL_PTR_NULL;
    }
    pst_stats = &(pst_net_dev->stats);
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    pst_oam_stat = OAM_STAT_GET_STAT_ALL();

    if (pst_mac_vap == NULL) {
        return pst_stats;
    }

    if (pst_mac_vap->uc_vap_id >= WLAN_VAP_SUPPORT_MAX_NUM_LIMIT) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_netdev_get_stats error vap id %u", pst_mac_vap->uc_vap_id);
        return pst_stats;
    }

    pst_oam_vap_stat = &(pst_oam_stat->ast_vap_stat_info[pst_mac_vap->uc_vap_id]);

    /* ����ͳ����Ϣ��net_device */
    pst_stats->rx_packets = pst_oam_vap_stat->ul_rx_pkt_to_lan;
    pst_stats->rx_bytes = pst_oam_vap_stat->ul_rx_bytes_to_lan;

    pst_stats->tx_packets = pst_oam_vap_stat->ul_tx_pkt_num_from_lan;
    pst_stats->tx_bytes = pst_oam_vap_stat->ul_tx_bytes_from_lan;
    return pst_stats;
}


OAL_STATIC oal_int32 wal_report_device_info(oal_net_device_stru *pst_net_dev, oal_uint32 ul_flag_value)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;

    if (pst_net_dev == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_report_device_info::pst_net_dev is NULL}\r\n");
        return OAL_FAIL;
    }

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_REPORT_VAP_INFO, OAL_SIZEOF(ul_flag_value));

    /* ��д��Ϣ�壬���� */
    *(oal_uint32 *)(st_write_msg.auc_value) = ul_flag_value;

    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE,
        WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(ul_flag_value), (oal_uint8 *)&st_write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_vap_info::wal_send_cfg_event return err code [%d]!}\r\n",
            l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}


oal_void wal_get_wifi_debug_info(oal_void)
{
    oal_int32 l_ret;
    struct hcc_handler *hcc = OAL_PTR_NULL;
    oal_int8 *pc_buf = OAL_PTR_NULL;
    oal_net_device_stru *pst_net_dev = OAL_PTR_NULL;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    oam_stat_info_stru *pst_oam_stat = OAL_PTR_NULL;
    oam_vap_stat_info_stru *pst_oam_vap_stat = OAL_PTR_NULL;
    oal_uint8 uc_hmac_vap_count;
    oal_uint16 us_queue_idx;
#if _PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION
    OAL_IO_PRINT("g_ul_alloc_fail_count value : %d\n", g_ul_skb_data_alloc_fail_count);
#endif
    OAL_IO_PRINT("g_auc_timer_debug 1:%u, 2:%u, 3:%u, 4:%u\n", g_auc_timer_debug[0], g_auc_timer_debug[1],
        g_auc_timer_debug[2], g_auc_timer_debug[3]);
    pc_buf = (oal_int8 *)oal_memalloc(OAL_PAGE_SIZE);
    if (pc_buf == NULL) {
        OAL_IO_PRINT("fun:hisi_wifi_debug_info,buf malloc fail!\n");
        return;
    }

    memset_s(pc_buf, OAL_PAGE_SIZE, 0, OAL_PAGE_SIZE);
    hcc = hcc_get_default_handler();
    if (hcc == NULL) {
        OAL_IO_PRINT("fun:hisi_wifi_debug_info,get hcc handler failed!\n");
        oal_free(pc_buf);
        return;
    }
    l_ret = hcc_flowctrl_info_print(hcc, pc_buf, OAL_PAGE_SIZE - 1);
    if (l_ret == 0) {
        OAL_IO_PRINT("call function error!\n");
    } else {
        OAL_IO_PRINT("%s", pc_buf);
    }
    oal_free(pc_buf);

    for (us_queue_idx = 0; us_queue_idx < HCC_QUEUE_COUNT; us_queue_idx++) {
        OAL_IO_PRINT("Q[%d]:bst_lmt[%d],low_wl[%d],high_wl[%d]\r\n", us_queue_idx,
            hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].burst_limit,
            hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].flow_ctrl.low_waterline,
            hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].flow_ctrl.high_waterline);

        OAL_IO_PRINT("HCC TX      wlan_q[%d],stoped?[%d],q_len[%d],total_pkt[%d],loss_pkt[%d]\r\n",
            hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].wlan_queue_id,
            hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].flow_ctrl.is_stopped,
            oal_netbuf_list_len(&hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].data_queue),
            hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].total_pkts,
            hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].loss_pkts);
    }

    for (us_queue_idx = 0; us_queue_idx < HCC_QUEUE_COUNT; us_queue_idx++) {
        OAL_IO_PRINT("HCC RX      wlan_q[%d],total_pkt[%d]\r\n", us_queue_idx,
            hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[us_queue_idx].total_pkts);
    }

    for (uc_hmac_vap_count = 0; uc_hmac_vap_count < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; uc_hmac_vap_count++) {
        OAL_IO_PRINT("++++++++++++++++vap %u------------\n", uc_hmac_vap_count);
        pst_net_dev = hmac_vap_get_net_device(uc_hmac_vap_count);
        if (pst_net_dev == NULL) {
            OAL_IO_PRINT("cmd_get_hisi_wifi_debug_info:NULL == pst_net_dev\n");
            continue;
        }

        pst_oam_stat = OAM_STAT_GET_STAT_ALL();
        pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
        if (pst_mac_vap == NULL) {
            OAL_IO_PRINT("NULL == pst_mac_vap\n");
            continue;
        }

        if (pst_mac_vap->uc_vap_id >= WLAN_VAP_SUPPORT_MAX_NUM_LIMIT) {
            OAL_IO_PRINT("get_hisi_wifi_debug_info error vap id %u", pst_mac_vap->uc_vap_id);
            continue;
        }

        if (pst_mac_vap->en_vap_state != MAC_VAP_STATE_UP) {
            OAL_IO_PRINT("get_hisi_wifi_debug_info MAC_VAP_STATE_UP != pst_mac_vap->en_vap_state\n");
            continue;
        }

        OAL_IO_PRINT("en_vap_mode=%u\n", pst_mac_vap->en_vap_mode);
        /* ����Ϊ0��ʾȡdevice vap�Զ�����Ϣ��Ϣ����patchʵ�� */
        wal_report_device_info(pst_net_dev, 0);
        pst_oam_vap_stat = &(pst_oam_stat->ast_vap_stat_info[pst_mac_vap->uc_vap_id]);

        /* ��ӡͳ����Ϣ */
        OAL_IO_PRINT("ul_rx_pkt_to_lan=%u\n", pst_oam_vap_stat->ul_rx_pkt_to_lan);
        OAL_IO_PRINT("ul_rx_bytes_to_lan=%u\n", pst_oam_vap_stat->ul_rx_bytes_to_lan);
        OAL_IO_PRINT("ul_tx_pkt_num_from_lan=%u\n", pst_oam_vap_stat->ul_tx_pkt_num_from_lan);
        OAL_IO_PRINT("ul_tx_bytes_from_lan=%u\n", pst_oam_vap_stat->ul_tx_bytes_from_lan);
    }
}


OAL_STATIC oal_int32 _wal_netdev_set_mac_addr(oal_net_device_stru *pst_net_dev, void *p_addr)
{
    oal_sockaddr_stru     *pst_mac_addr = OAL_PTR_NULL;
    oal_wireless_dev_stru *pst_wdev = OAL_PTR_NULL; /* ����P2P ������p2p0 �� p2p-p2p0 MAC ��ַ��wlan0 ��ȡ */

    if (OAL_UNLIKELY((pst_net_dev == OAL_PTR_NULL) || (p_addr == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_netdev_set_mac_addr::pst_net_dev or p_addr null ptr!}");
        return -OAL_EFAUL;
    }

    if (oal_netif_running(pst_net_dev)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_netdev_set_mac_addr::cannot set address; device running!}\r\n");
        return -OAL_EBUSY;
    }
    /*lint +e774*/ /*lint +e506*/

    pst_mac_addr = (oal_sockaddr_stru *)p_addr;

    if (ETHER_IS_MULTICAST(pst_mac_addr->sa_data)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_netdev_set_mac_addr::can not set group/broadcast addr!}\r\n");
        return -OAL_EINVAL;
    }

    oal_set_mac_addr((oal_uint8 *)(pst_net_dev->dev_addr), (oal_uint8 *)(pst_mac_addr->sa_data));

    pst_wdev = (oal_wireless_dev_stru *)pst_net_dev->ieee80211_ptr;
    if (pst_wdev == OAL_PTR_NULL) {
        return -OAL_EFAUL;
    }
    OAM_WARNING_LOG4(0, OAM_SF_ANY, "{wal_netdev_set_mac_addr::iftype [%d], mac_addr[%.2x:xx:xx:xx:%.2x:%.2x]..}",
                     pst_wdev->iftype, pst_net_dev->dev_addr[0], pst_net_dev->dev_addr[4], pst_net_dev->dev_addr[5]);

    /* 1131���return���޷�ͨ����������mac��ַ���Ĵ��� */
    return OAL_SUCC;
}

OAL_STATIC oal_int32 wal_netdev_set_mac_addr(oal_net_device_stru *pst_net_dev, void *p_addr)
{
    oal_int32 ret;
    wal_wake_lock();
    ret = _wal_netdev_set_mac_addr(pst_net_dev, p_addr);
    wal_wake_unlock();
    if (ret != OAL_SUCC) {
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI,
            CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_OPEN, CHR_WIFI_DRV_ERROR_POWER_ON_SET_MAC_ADDR);
    }
    return ret;
}


OAL_STATIC oal_uint32 wal_ioctl_judge_input_param_length(wal_android_wifi_priv_cmd_stru *pst_priv_cmd,
    oal_uint32 ul_cmd_length, oal_uint16 us_adjust_length)
{
    /* ����+1Ϊ �ַ�����������������м�Ŀո��ַ� */
    if (pst_priv_cmd->l_total_len < ((oal_int32)(ul_cmd_length + 1 + us_adjust_length))) {
        /* ��γ��Ȳ�����Ҫ�� */
        return OAL_FAIL;
    }
    return OAL_SUCC;
}

OAL_STATIC int32_t wal_ioctl_priv_set_cmd_country(oal_net_device_stru *net_dev, int8_t *command)
{
#ifdef _PRE_WLAN_FEATURE_11D
    const int8_t *country_code = NULL;
    int8_t       tmp_country_code[3] = {0};
    int32_t      ret;

    /* ��ʽ:COUNTRY CN */
    if (OAL_STRLEN(command) < (OAL_STRLEN((int8_t *)CMD_COUNTRY) + 3)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_priv_set_cmd_country::puc_command len error.}");
        return -OAL_EFAIL;
    }
    country_code = command + OAL_STRLEN((int8_t *)CMD_COUNTRY) + 1;
    if (memcpy_s(tmp_country_code, sizeof(tmp_country_code), country_code, 2) != EOK) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_priv_set_cmd_country::memcpy_s fail [%d]!}");
        return -OAL_EFAIL;
    }

#ifdef _PRE_WLAN_FEATURE_DFS // 1131_debug
    ret = wal_regdomain_update_for_dfs(net_dev, tmp_country_code);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_priv_set_cmd_country::wal_regdomain_update_for_dfs err[%d]!}", ret);
        return -OAL_EFAIL;
    }
#endif

    ret = wal_regdomain_update(net_dev, tmp_country_code);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_priv_set_cmd_country::wal_regdomain_update err code [%d]!}", ret);
        return -OAL_EFAIL;
    }
#else
    OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_android_priv_cmd::_PRE_WLAN_FEATURE_11D is not define!}");
#endif
    return OAL_SUCC;
}

OAL_STATIC int32_t wal_psm_query_wait_complete(hmac_psm_flt_stat_query_stru *hmac_psm_query)
{
    hmac_psm_query->complete_flag = OAL_FALSE;
    /*lint -e730 -e740 -e774*/
    return OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(hmac_psm_query->wait_queue,
                                                (hmac_psm_query->complete_flag == OAL_TRUE),
                                                OAL_TIME_HZ);
    /*lint +e730 +e740 +e774*/
}

oal_uint32 wal_ioctl_get_psm_info(oal_net_device_stru *net_dev, oal_ifreq_stru *ifr)
{
    mac_vap_stru *mac_vap = OAL_NET_DEV_PRIV(net_dev);
    hmac_device_stru *hmac_device = OAL_PTR_NULL;
    wal_msg_write_stru write_msg = { 0 };
    sub_type_load_stru *rx_filter_frag_param = NULL;
    uint16_t len;
    int32_t ret;
    hmac_psm_flt_stat_query_stru *hmac_psm_query = OAL_PTR_NULL;
    mac_psm_query_stat_stru  *psm_stat = OAL_PTR_NULL;

    if (mac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{wal_ioctl_get_psm_info::pst_mac_vap get from netdev or pst_ifr is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    hmac_device = hmac_res_get_mac_dev(mac_vap->uc_device_id);
    if (hmac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{wal_ioctl_get_psm_info::hmac_device is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ���¼���wal�㴦�� */
    rx_filter_frag_param = (sub_type_load_stru *)(write_msg.auc_value);
    rx_filter_frag_param->en_sub_type = MAC_SUB_WLAN_CFGID_CHR_GET_CHIP_INFO;
    rx_filter_frag_param->us_buf_len  = 0;
    len = sizeof(sub_type_load_stru) - WLAN_SUB_EVENT_MAX_LEN + rx_filter_frag_param->us_buf_len;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_PSM_GET_TXRX_INFO, len);
    ret = wal_send_cfg_event(net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + len,
                             (uint8_t *)&write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_ioctl_get_psm_info::return err code [%d]!}\r\n", ret);
        return (uint32_t)ret;
    }

    hmac_psm_query = &hmac_device->psm_flt_stat_query;
    ret = wal_psm_query_wait_complete(hmac_psm_query);
    /* ��ʱ���쳣 */
    if (ret <= 0) {
        OAM_WARNING_LOG1(mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_ioctl_get_psm_info::fail! ret:%d}", ret);
        return OAL_FAIL;
    }

    psm_stat = &hmac_psm_query->psm_stat;
    if (psm_stat->query_item > MAC_PSM_QUERY_MSG_MAX_STAT_ITEM) {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_ioctl_get_psm_info::query_item invalid[%d]}", psm_stat->query_item);
    }

    if (ifr == OAL_PTR_NULL) {
        return OAL_SUCC;
    }

    /* 8Ϊhdr���� */
    if (oal_copy_to_user(((uint8_t *)ifr->ifr_data) + 8, psm_stat->val, psm_stat->query_item * OAL_SIZEOF(uint32_t))) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{wal_ioctl_get_psm_info::Failed to copy ioctl_data to user !}");
        return OAL_FAIL;
    }

    return OAL_SUCC;
}

OAL_STATIC int32_t wal_psm_bcn_query_wait_complete(hmac_psm_beacon_query_stru *hmac_psm_bcn_query)
{
    hmac_psm_bcn_query->complete_flag = OAL_FALSE;
    /*lint -e730 -e740 -e774*/
    return OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(hmac_psm_bcn_query->wait_queue,
                                                (hmac_psm_bcn_query->complete_flag == OAL_TRUE),
                                                OAL_TIME_HZ);
    /*lint +e730 +e740 +e774*/
}

uint32_t wal_ioctl_get_psm_bcn_info(oal_net_device_stru *net_dev, oal_ifreq_stru *ifr)
{
    mac_vap_stru *mac_vap = OAL_NET_DEV_PRIV(net_dev);
    hmac_device_stru *hmac_device = OAL_PTR_NULL;
    wal_msg_write_stru write_msg = { 0 };
    sub_type_load_stru *event_subtype_param = NULL;
    uint16_t len;
    int32_t ret;
    hmac_psm_beacon_query_stru *hmac_psm_query = OAL_PTR_NULL;
    mac_psm_query_stat_stru  *psm_stat = OAL_PTR_NULL;

    if (mac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{wal_ioctl_get_psm_bcn_info::pst_mac_vap get from netdev or ifr is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    hmac_device = hmac_res_get_mac_dev(mac_vap->uc_device_id);
    if (hmac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{wal_ioctl_get_psm_bcn_info::hmac_device is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ���¼���wal�㴦�� */
    event_subtype_param = (sub_type_load_stru *)(write_msg.auc_value);
    event_subtype_param->en_sub_type = MAC_SUB_WLAN_CFGID_CHR_GET_BEACON_INFO;
    event_subtype_param->us_buf_len  = 0;
    len = sizeof(sub_type_load_stru) - WLAN_SUB_EVENT_MAX_LEN + event_subtype_param->us_buf_len;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_PSM_GET_BEACON_INFO, len);
    ret = wal_send_cfg_event(net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + len,
                             (uint8_t *)&write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_ioctl_get_psm_bcn_info::return err code [%d]!}\r\n", ret);
        return (uint32_t)ret;
    }

    hmac_psm_query = &hmac_device->psm_beacon_query;
    ret = wal_psm_bcn_query_wait_complete(hmac_psm_query);
    /* ��ʱ���쳣 */
    if (ret <= 0) {
        OAM_WARNING_LOG1(mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_ioctl_get_psm_bcn_info::fail! ret:%d}", ret);
        return OAL_FAIL;
    }

    psm_stat = &hmac_psm_query->psm_stat;
    if (psm_stat->query_item > MAC_PSM_QUERY_MSG_MAX_STAT_ITEM) {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_ioctl_get_psm_bcn_info::query_item invalid[%d]}", psm_stat->query_item);
    }

    if (ifr == OAL_PTR_NULL) {
        return OAL_SUCC;
    }

    /* 8Ϊhdr���� */
    if (oal_copy_to_user(((uint8_t *)ifr->ifr_data) + 8, psm_stat->val, psm_stat->query_item * OAL_SIZEOF(uint32_t))) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{wal_ioctl_get_psm_bcn_info::Failed to copy ioctl_data to user !}");
        return OAL_FAIL;
    }

    return OAL_SUCC;
}


oal_int32 wal_android_priv_cmd(oal_net_device_stru *pst_net_dev, oal_ifreq_stru *pst_ifr, oal_int32 ul_cmd)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

    wal_android_wifi_priv_cmd_stru st_priv_cmd = { 0 };
    oal_uint8 *pc_command = OAL_PTR_NULL;
    oal_int32 l_ret;
    oal_uint32 l_memcpy_ret = EOK;

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if (!capable(CAP_NET_ADMIN)) {
        return -EPERM;
    }
#endif

    if (pst_ifr->ifr_data == OAL_PTR_NULL) {
        l_ret = -OAL_EINVAL;
        return l_ret;
    }
#ifdef _PRE_WLAN_FEATURE_DFR
    if (g_st_dfr_info.bit_device_reset_process_flag) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_vendor_priv_cmd::dfr_process_status[%d]!}",
            g_st_dfr_info.bit_device_reset_process_flag);
        return OAL_SUCC;
    }
#endif // _PRE_WLAN_FEATURE_DFR
    if (oal_copy_from_user((oal_uint8 *)&st_priv_cmd, pst_ifr->ifr_data, sizeof(wal_android_wifi_priv_cmd_stru))) {
        l_ret = -OAL_EINVAL;
        return l_ret;
    }

    if (OAL_VALUE_NOT_IN_VALID_RANGE(st_priv_cmd.l_total_len, 0, MAX_PRIV_CMD_SIZE)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_android_priv_cmd::too long priavte command. len:%d. }\r\n",
            st_priv_cmd.l_total_len);
        l_ret = -OAL_EINVAL;
        return l_ret;
    }

    /* �����ڴ汣��wpa_supplicant �·������������ */
    pc_command = oal_memalloc((oal_uint32)(st_priv_cmd.l_total_len + 1)); /* total len Ϊpriv cmd ����buffer ���� */
    if (pc_command == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_android_priv_cmd::mem alloc failed.}\r\n");

        l_ret = -OAL_ENOMEM;
        return l_ret;
    }

    /* ����wpa_supplicant ����ں�̬�� */
    memset_s(pc_command, (oal_uint32)(st_priv_cmd.l_total_len + 1), 0, (oal_uint32)(st_priv_cmd.l_total_len + 1));

    l_ret = (oal_int32)oal_copy_from_user(pc_command, ((oal_uint8 *)pst_ifr->ifr_data) + 8,
        (oal_uint32)(st_priv_cmd.l_total_len));
    if (l_ret != 0) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_android_priv_cmd::oal_copy_from_user: -OAL_EFAIL }\r\n");
        l_ret = -OAL_EFAIL;
        oal_free(pc_command);
        return l_ret;
    }
    pc_command[st_priv_cmd.l_total_len] = '\0';
    OAM_WARNING_LOG2(0, OAM_SF_ANY, "{wal_android_priv_cmd::Android private cmd total_len:%d, used_len:%d}\r\n",
        st_priv_cmd.l_total_len, st_priv_cmd.l_used_len);

    if (oal_strncmp((oal_int8 *)pc_command, CMD_SET_AP_WPS_P2P_IE, OAL_STRLEN(CMD_SET_AP_WPS_P2P_IE)) == 0) {
        oal_uint32 skip = OAL_STRLEN(CMD_SET_AP_WPS_P2P_IE) + 1;
        /* �ṹ������ */
        oal_app_ie_stru *pst_wps_p2p_ie = OAL_PTR_NULL;
        /* �ⲿ��������жϣ��ⲿ�������ݳ��ȱ���Ҫ����oal_app_ie_stru�ṹ��ͷ����С */
        l_ret = wal_ioctl_judge_input_param_length(&st_priv_cmd, OAL_STRLEN(CMD_SET_AP_WPS_P2P_IE),
            OAL_OFFSET_OF(oal_app_ie_stru, auc_ie)); //lint !e78
        if (l_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_vendor_priv_cmd::header length is too short! at least need [%d]!}",
                (skip + OAL_OFFSET_OF(oal_app_ie_stru, auc_ie))); //lint !e78
            oal_free(pc_command);
            return -OAL_EFAIL;
        }

        pst_wps_p2p_ie = (oal_app_ie_stru *)(pc_command + skip);
        if ((skip + pst_wps_p2p_ie->ul_ie_len + OAL_OFFSET_OF(oal_app_ie_stru, auc_ie)) //lint !e78
            > (oal_uint32)st_priv_cmd.l_total_len) {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_android_priv_cmd::SET_AP_WPS_P2P_IE para len is short. need %d.}\r\n",
                (skip + pst_wps_p2p_ie->ul_ie_len));
            oal_free(pc_command);
            return -OAL_EFAIL;
        }

        l_ret = wal_ioctl_set_wps_p2p_ie(pst_net_dev, pst_wps_p2p_ie->auc_ie, pst_wps_p2p_ie->ul_ie_len,
            pst_wps_p2p_ie->en_app_ie_type);
    } else if (oal_strncmp((int8_t *)pc_command, CMD_GET_APF_PKTS_CNT, OAL_STRLEN(CMD_GET_APF_PKTS_CNT)) == 0) {
        l_ret = wal_ioctl_get_psm_info(pst_net_dev, pst_ifr);
    } else if (oal_strncmp((int8_t *)pc_command, CMD_GET_BEACON_CNT, OAL_STRLEN(CMD_GET_BEACON_CNT)) == 0) {
        l_ret = wal_ioctl_get_psm_bcn_info(pst_net_dev, pst_ifr);
    }
#ifdef _PRE_WLAN_FEATURE_P2P
    else if (oal_strncmp((oal_int8 *)pc_command, CMD_P2P_SET_NOA, OAL_STRLEN(CMD_P2P_SET_NOA)) == 0) {
        oal_uint32 skip = OAL_STRLEN(CMD_P2P_SET_NOA) + 1;
        mac_cfg_p2p_noa_param_stru st_p2p_noa_param;
        if ((skip + OAL_SIZEOF(st_p2p_noa_param)) > (oal_uint32)st_priv_cmd.l_total_len) {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_android_priv_cmd::CMD_P2P_SET_NOA param len is short. need %d.}\r\n",
                skip + OAL_SIZEOF(st_p2p_noa_param));
            oal_free(pc_command);
            return -OAL_EFAIL;
        }
        l_memcpy_ret += memcpy_s(&st_p2p_noa_param, OAL_SIZEOF(st_p2p_noa_param), pc_command + skip,
            OAL_SIZEOF(mac_cfg_p2p_noa_param_stru));

        l_ret = wal_ioctl_set_p2p_noa(pst_net_dev, &st_p2p_noa_param);
    } else if (oal_strncmp((oal_int8 *)pc_command, CMD_P2P_SET_PS, OAL_STRLEN(CMD_P2P_SET_PS)) == 0) {
        oal_uint32 skip = OAL_STRLEN(CMD_P2P_SET_PS) + 1;
        mac_cfg_p2p_ops_param_stru st_p2p_ops_param;
        if ((skip + OAL_SIZEOF(st_p2p_ops_param)) > (oal_uint32)st_priv_cmd.l_total_len) {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_android_priv_cmd::CMD_P2P_SET_PS param len is too short.need %d.}\r\n",
                skip + OAL_SIZEOF(st_p2p_ops_param));
            oal_free(pc_command);
            return -OAL_EFAIL;
        }
        l_memcpy_ret += memcpy_s(&st_p2p_ops_param, OAL_SIZEOF(st_p2p_ops_param), pc_command + skip,
            OAL_SIZEOF(mac_cfg_p2p_ops_param_stru));

        l_ret = wal_ioctl_set_p2p_ops(pst_net_dev, &st_p2p_ops_param);
    }
#endif

#ifdef _PRE_WLAN_FEATURE_HS20
    else if (0 == oal_strncmp((oal_int8 *)pc_command, CMD_SET_QOS_MAP, OAL_STRLEN(CMD_SET_QOS_MAP))) {
        oal_uint32 skip = OAL_STRLEN(CMD_SET_QOS_MAP) + 1;
        hmac_cfg_qos_map_param_stru st_qos_map_param;
        if ((skip + OAL_SIZEOF(st_qos_map_param)) > (oal_uint32)st_priv_cmd.l_total_len) {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_android_priv_cmd::CMD_SET_QOS_MAP param len is too short.need %d.}\r\n",
                skip + OAL_SIZEOF(st_qos_map_param));
            oal_free(pc_command);
            return -OAL_EFAIL;
        }
        l_memcpy_ret += memcpy_s(&st_qos_map_param, OAL_SIZEOF(st_qos_map_param), pc_command + skip,
            OAL_SIZEOF(hmac_cfg_qos_map_param_stru));

        l_ret = wal_ioctl_set_qos_map(pst_net_dev, &st_qos_map_param);
    }
#endif
    else if (0 == oal_strncmp((oal_int8 *)pc_command, CMD_SET_POWER_ON, OAL_STRLEN(CMD_SET_POWER_ON))) {
        oal_int32 power_flag;
        oal_uint32 command_len = OAL_STRLEN((oal_int8 *)pc_command);
        /* ��ʽ:SET_POWER_ON 1 or SET_POWER_ON 0 */
        if (command_len < (OAL_STRLEN((oal_int8 *)CMD_SET_POWER_ON) + 2)) {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_android_priv_cmd:CMD_SET_POWER_ON cmd len must equal or larger than 18."
                " Now the cmd len:%d.}", command_len);

            oal_free(pc_command);
            return -OAL_EFAIL;
        }

        power_flag = oal_atoi((oal_int8 *)pc_command + OAL_STRLEN((oal_int8 *)CMD_SET_POWER_ON) + 1);

        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_android_priv_cmd:CMD_SET_POWER_ON command,powerflag:%d}\r\n", power_flag);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && \
    ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION))
        // ap���µ磬����VAP
        if (power_flag == 0) { // �µ�
            if (oal_strcmp("wlan0", pst_net_dev->name) == 0) {
                /* �µ�host device_struȥ��ʼ�� */
                wal_host_dev_exit(pst_net_dev);

                wal_wake_lock();
                wlan_pm_close();
                wal_wake_unlock();
            }
            g_st_ap_config_info.l_ap_power_flag = OAL_FALSE;
        } else if (power_flag == 1) { // �ϵ�
            g_st_ap_config_info.l_ap_power_flag = OAL_TRUE;

            wal_wake_lock();
            g_uc_netdev_is_open = OAL_TRUE;
            l_ret = wlan_pm_open();
            wal_wake_unlock();
            if (l_ret == OAL_FAIL) {
                OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_android_priv_cmd::wlan_pm_open Fail!}\r\n");
                oal_free(pc_command);
                return -OAL_EFAIL;
            } else if (l_ret != OAL_ERR_CODE_ALREADY_OPEN) {
#ifdef _PRE_WLAN_FEATURE_DFR
                wal_dfr_init_param();
#endif
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
                /* �����ϵ�ʱ��ΪFALSE */
                hwifi_config_init_force(); /* 1131C-debug */
#endif
                // �����ϵ糡�����·�����VAP
                l_ret = wal_cfg_vap_h2d_event(pst_net_dev);
                if (l_ret != OAL_SUCC) {
                    oal_free(pc_command);
                    return -OAL_EFAIL;
                }
            }

            /* �ϵ�host device_stru��ʼ�� */
            l_ret = wal_host_dev_init(pst_net_dev);
            if (l_ret != OAL_SUCC) {
                OAL_IO_PRINT("wal_host_dev_init FAIL %d \r\n", l_ret);
                oal_free(pc_command);
                return -OAL_EFAIL;
            }
        } else {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_android_priv_cmd::pupower_flag:%d error.}\r\n", power_flag);
            oal_free(pc_command);
            return -OAL_EFAIL;
        }
#endif
    } else if (0 == oal_strncmp((oal_int8 *)pc_command, CMD_SET_POWER_MGMT_ON, OAL_STRLEN(CMD_SET_POWER_MGMT_ON))) {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && \
    ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION))
        struct wlan_pm_s *pst_wlan_pm = OAL_PTR_NULL;
#endif

        oal_ulong power_mgmt_flag; /* APģʽ,Ĭ�ϵ�Դ�����ǿ����� */
        oal_uint32 command_len = OAL_STRLEN((oal_int8 *)pc_command);
        /* ��ʽ:CMD_SET_POWER_MGMT_ON 1 or CMD_SET_POWER_MGMT_ON 0 */
        if (command_len < (OAL_STRLEN((oal_int8 *)CMD_SET_POWER_MGMT_ON) + 2)) {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_android_priv_cmd::CMD_SET_POWER_MGMT_ON cmd len:%d is error.}\r\n",
                command_len);

            oal_free(pc_command);
            return -OAL_EFAIL;
        }

        power_mgmt_flag =
            (oal_ulong)((oal_uint32)oal_atoi((oal_int8 *)pc_command + OAL_STRLEN(CMD_SET_POWER_MGMT_ON) + 1));

        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_android_priv_cmd::CMD_SET_POWER_MGMT_ON command,power_mgmt flag:%u}\r\n",
            power_mgmt_flag);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && \
    ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION))
        pst_wlan_pm = wlan_pm_get_drv();
        if (pst_wlan_pm != NULL) {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_android_priv_cmd::wlan_pm_get_drv get pst_wlan_pm.");
            pst_wlan_pm->ul_apmode_allow_pm_flag = power_mgmt_flag;
        }
#endif
    } else if (0 == oal_strncmp((oal_int8 *)pc_command, CMD_COUNTRY, OAL_STRLEN(CMD_COUNTRY))) {
        l_ret = wal_ioctl_priv_set_cmd_country(pst_net_dev, (int8_t *)pc_command);
    }
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifndef CONFIG_HAS_EARLYSUSPEND
    else if (0 == oal_strncasecmp((oal_int8 *)pc_command, CMD_SETSUSPENDMODE, OAL_STRLEN(CMD_SETSUSPENDMODE))) {
        l_ret = wal_ioctl_judge_input_param_length(&st_priv_cmd, CMD_SETSUSPENDMODE_LEN, 1);
        if (l_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY,
                "{wal_vendor_priv_cmd_etc:: CMD_SETSUSPENDMODE length is too short! at least need [%d]!}\r\n",
                (CMD_SETSUSPENDMODE_LEN + 2));
            oal_free(pc_command);
            return -OAL_EFAIL;
        }
        l_ret = wal_ioctl_set_suspend_mode(pst_net_dev, *(pc_command + CMD_SETSUSPENDMODE_LEN + 1) - '0');
    }
#endif
#endif
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    else if (oal_strncmp((oal_int8 *)pc_command, CMD_TX_POWER, OAL_STRLEN(CMD_TX_POWER)) == 0) {
        oal_uint8 uc_txpwr;
        oal_uint32 ul_skip = OAL_STRLEN((oal_int8 *)CMD_TX_POWER) + 1;

        /* ��ʽ: TX_POWER 10 �� TX_POWER 255 */
        if (wal_ioctl_judge_input_param_length(&st_priv_cmd, OAL_STRLEN(CMD_TX_POWER), 1) != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY,
                "{wal_vendor_priv_cmd:: TX_POWER length is too short! at least need [%d]!}\r\n",
                OAL_STRLEN(CMD_TX_POWER) + 2);
            oal_free(pc_command);
            return -OAL_EFAIL;
        }

        uc_txpwr = (oal_uint8)oal_atoi((oal_int8 *)pc_command + ul_skip);
        l_ret = wal_ioctl_reduce_sar(pst_net_dev, uc_txpwr);
        if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_android_priv_cmd::return err code [%d]!}\r\n", l_ret);
            oal_free(pc_command);
            /* ������ӡ�����룬���سɹ�����ֹsupplicant �ۼ�4�� ioctlʧ�ܵ���wifi�쳣���� */
            return OAL_SUCC;
        }
    } else if (oal_strncmp((oal_int8 *)pc_command, CMD_SET_SSID, OAL_STRLEN(CMD_SET_SSID)) == 0) {  /* �����޸�AP��SSID��Ϣ */
        l_ret = wal_android_priv_set_ssid(pst_net_dev, pc_command);
        if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_android_priv_cmd::wal_android_priv_set_ssid fail}\r\n");
            oal_free(pc_command);
            return -OAL_EFAIL;
        }
    }
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    else if (oal_strncmp((oal_int8 *)pc_command, CMD_WPAS_GET_CUST, OAL_STRLEN(CMD_WPAS_GET_CUST)) == 0) {
        /* ��buf���� */
        memset_s(pc_command, st_priv_cmd.l_total_len + 1, 0, st_priv_cmd.l_total_len + 1);
        pc_command[st_priv_cmd.l_total_len] = '\0';

        /* ��ȡȫ�����ƻ����ã���������ȡdisable_capab_ht40 */
        hwifi_config_init_force();

        /* ��ֵht40��ֹλ */
        *pc_command = g_st_wlan_customize.uc_disable_capab_2ght40;

        if (oal_copy_to_user(((oal_uint8 *)pst_ifr->ifr_data) + 8, pc_command, (oal_uint32)(st_priv_cmd.l_total_len))) {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_android_priv_cmd: Failed to copy ioctl_data to user !");
            oal_free(pc_command);
            /* ���ش���֪ͨsupplicant����ʧ�ܣ�supplicant���������������� */
            return -OAL_EFAIL;
        }
    }
#endif
#endif
#ifdef _PRE_WLAN_FEATURE_IP_FILTER
    else if (oal_strncmp((oal_int8 *)pc_command, CMD_FILTER_SWITCH, CMD_FILTER_SWITCH_LEN) == 0) {
#ifdef CONFIG_DOZE_FILTER
        l_ret = wal_android_priv_set_ip_filter(pc_command, &st_priv_cmd);
        if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_vendor_priv_cmd::CMD_FILTER_SWITCH return err code [%d]!}", l_ret);
            oal_free(pc_command);
            return -OAL_EFAIL;
        }
#else
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_vendor_priv_cmd::Not support CMD_FILTER_SWITCH.}");
#endif
    }
#endif /* _PRE_WLAN_FEATURE_IP_FILTER */
    else {
        /* �������ڲ�֧�ֵ�������سɹ��������ϲ�wpa_supplicant��Ϊioctlʧ�ܣ������쳣����wifi */
        l_ret = OAL_SUCC;
    }

    if (l_memcpy_ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_vendor_priv_cmd::memcpy fail!");
        oal_free(pc_command);
        return -OAL_EFAIL;
    }

    oal_free(pc_command);
    return l_ret;
#else
    return OAL_SUCC;
#endif
}


#if ((_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1131C_DEV) && (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1131C_HOST))
oal_int32 wal_witp_wifi_priv_cmd(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    oal_int32 l_ret;
    switch (pst_ioctl_data->l_cmd) {
        case HWIFI_IOCTL_CMD_GET_STA_ASSOC_REQ_IE: {
            l_ret = wal_ioctl_get_assoc_req_ie(pst_net_dev, pst_ioctl_data);

            break;
        }
        case HWIFI_IOCTL_CMD_SET_AP_AUTH_ALG: {
            l_ret = wal_ioctl_set_auth_mode(pst_net_dev, pst_ioctl_data);

            break;
        }
        case HWIFI_IOCTL_CMD_SET_COUNTRY: {
            l_ret = wal_ioctl_set_country_code(pst_net_dev, pst_ioctl_data);
            break;
        }
        case HWIFI_IOCTL_CMD_SET_WPS_IE: {
            l_ret = wal_ioctl_set_wps_ie(pst_net_dev, pst_ioctl_data);
            break;
        }
        case HWIFI_IOCTL_CMD_SET_SSID: {
            l_ret = wal_ioctl_set_ssid(pst_net_dev, pst_ioctl_data);
            break;
        }
        case HWIFI_IOCTL_CMD_SET_MAX_USER: {
            l_ret = wal_ioctl_set_max_user(pst_net_dev, pst_ioctl_data);
            break;
        }
        case HWIFI_IOCTL_CMD_PRIV_CONNECT: {
            l_ret = wal_ioctl_nl80211_priv_connect(pst_net_dev, pst_ioctl_data);
            break;
        }
        case HWIFI_IOCTL_CMD_PRIV_DISCONNECT: {
            l_ret = wal_ioctl_nl80211_priv_disconnect(pst_net_dev, pst_ioctl_data);
            break;
        }
        case HWIFI_IOCTL_CMD_SET_FREQ: {
            l_ret = wal_ioctl_set_channel(pst_net_dev, pst_ioctl_data);
            break;
        }
        case HWIFI_IOCTL_CMD_SET_FRAG: {
            l_ret = wal_ioctl_set_frag(pst_net_dev, pst_ioctl_data);
            break;
        }
        case HWIFI_IOCTL_CMD_SET_RTS: {
            l_ret = wal_ioctl_set_rts(pst_net_dev, pst_ioctl_data);
            break;
        }

        default:
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_witp_wifi_priv_cmd::no matched CMD !}\r\n");
            l_ret = -OAL_EFAIL;
            break;
    }

    return (l_ret);
}
#endif


oal_int32 wal_net_device_ioctl(oal_net_device_stru *pst_net_dev, oal_ifreq_stru *pst_ifr, oal_int32 ul_cmd)
{
    int32_t ret;

    if ((pst_net_dev == OAL_PTR_NULL) || (pst_ifr == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_net_device_ioctl::null param}");
        return -OAL_EFAUL;
    }

    if (pst_ifr->ifr_data == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_net_device_ioctl::pst_ifr->ifr_data is NULL!}\r\n");
        return -OAL_EFAUL;
    }

    /* 1102 wpa_supplicant ͨ��ioctl �·����� */
    if (WAL_SIOCDEVPRIVATE + 1 == ul_cmd) {
        ret = wal_android_priv_cmd(pst_net_dev, pst_ifr, ul_cmd);
        return ret;
    }
#if (_PRE_OS_VERSION_WIN32 != _PRE_OS_VERSION)

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* atcmdsrv ͨ��ioctl�·����� */
    else if ((WAL_SIOCDEVPRIVATE + 2) == ul_cmd) {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        if (!capable(CAP_NET_ADMIN)) {
            return -EPERM;
        }
#endif
#ifdef PLATFORM_DEBUG_ENABLE
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "atcmdsrv_ioctl_cmd,cmd=0x%x", ul_cmd);
        wal_wake_lock();
        ret = wal_atcmdsrv_wifi_priv_cmd(pst_net_dev, pst_ifr, ul_cmd);
        wal_wake_unlock();
#else
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "unsport atcmdsrv_ioctl_cmd");
#endif
        return ret;
    }
#endif
#endif
    else {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_net_device_ioctl::unrecognised cmd, %d!}\r\n", ul_cmd);
        return OAL_SUCC;
    }
}


oal_uint32 wal_hipriv_set_mode(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_int8                 ac_mode_str[WAL_HIPRIV_CMD_NAME_MAX_LEN] = { 0 };     /* Ԥ��Э��ģʽ�ַ����ռ� */
    oal_uint8                uc_prot_idx;
    mac_cfg_mode_param_stru *pst_mode_param = OAL_PTR_NULL;
    wal_msg_write_stru       st_write_msg;
    oal_uint32               ul_off_set;
    oal_uint32               ul_ret;
    oal_int32                l_ret;

    if (OAL_UNLIKELY((pst_net_dev == OAL_PTR_NULL) || (pc_param == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_mode::pst_net_dev/p_param null ptr error}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* pc_paramָ����ģʽ����, ����ȡ����ŵ�ac_mode_str�� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_mode_str, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mode::wal_get_cmd_one_arg vap name return err_code %d!}\r\n",
            ul_ret);
        return ul_ret;
    }

    ac_mode_str[OAL_SIZEOF(ac_mode_str) - 1] = '\0'; /* ȷ����null��β */

    for (uc_prot_idx = 0; g_ast_mode_map[uc_prot_idx].pc_name != OAL_PTR_NULL; uc_prot_idx++) {
        l_ret = oal_strcmp(g_ast_mode_map[uc_prot_idx].pc_name, ac_mode_str);
        if (l_ret == 0) {
            break;
        }
    }

    if (g_ast_mode_map[uc_prot_idx].pc_name == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_mode::unrecognized protocol string!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* **************************************************************************
        ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_MODE, OAL_SIZEOF(mac_cfg_mode_param_stru));

    pst_mode_param = (mac_cfg_mode_param_stru *)(st_write_msg.auc_value);
    pst_mode_param->en_protocol = g_ast_mode_map[uc_prot_idx].en_mode;
    pst_mode_param->en_band = g_ast_mode_map[uc_prot_idx].en_band;
    pst_mode_param->en_bandwidth = g_ast_mode_map[uc_prot_idx].en_bandwidth;

    OAM_INFO_LOG3(0, OAM_SF_CFG, "{wal_hipriv_set_mode::protocol[%d],band[%d],bandwidth[%d]!}\r\n",
        pst_mode_param->en_protocol, pst_mode_param->en_band, pst_mode_param->en_bandwidth);

    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_mode_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mode::wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_essid(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint8                uc_ssid_len;
    oal_int32                l_ret;
    wal_msg_write_stru       st_write_msg;
    mac_cfg_ssid_param_stru *pst_param = OAL_PTR_NULL;
    mac_vap_stru            *pst_mac_vap = OAL_PTR_NULL;
    oal_uint32               ul_off_set;
    oal_int8                *pc_ssid = OAL_PTR_NULL;
    oal_int8                 ac_ssid[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32               ul_ret;

    if ((pst_net_dev == OAL_PTR_NULL) || (pc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_essid::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (pst_mac_vap == NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_essid::pst_mac_vap is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        /* �豸��up״̬����APʱ�����������ã�������down */
        if ((OAL_IFF_RUNNING & OAL_NETDEVICE_FLAGS(pst_net_dev)) != 0) {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                "{wal_hipriv_set_essid::device is busy, please down it firste %d!}\r\n",
                OAL_NETDEVICE_FLAGS(pst_net_dev));
            return OAL_ERR_CODE_ALREADY_OPEN;
        }
    }

    /* pc_paramָ����ģʽ����, ����ȡ����ŵ�ac_mode_str�� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_ssid, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_essid::wal_get_cmd_one_arg vap name return err_code %d!}",
            ul_ret);
        return ul_ret;
    }

    pc_ssid = ac_ssid;
    pc_ssid = oal_strim(ac_ssid); /* ȥ���ַ�����ʼ��β�Ŀո� */
    uc_ssid_len = (oal_uint8)OAL_STRLEN(pc_ssid);
    if (uc_ssid_len > WLAN_SSID_MAX_LEN - 1) { /* -1Ϊ\0Ԥ���ռ� */
        uc_ssid_len = WLAN_SSID_MAX_LEN - 1;
    }

    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_hipriv_set_essid:: ssid length is %d!}\r\n", uc_ssid_len);
    /* **************************************************************************
        ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SSID, OAL_SIZEOF(mac_cfg_ssid_param_stru));

    /* ��дWID��Ӧ�Ĳ��� */
    pst_param = (mac_cfg_ssid_param_stru *)(st_write_msg.auc_value);
    pst_param->uc_ssid_len = uc_ssid_len;
    if (memcpy_s(pst_param->ac_ssid, WLAN_SSID_MAX_LEN, pc_ssid, uc_ssid_len) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_set_essid::memcpy_s failed!");
        return OAL_FAIL;
    }

    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ssid_param_stru),
                               (oal_uint8 *)&st_write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_hipriv_set_essid:: wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


oal_uint32 wal_hipriv_set_freq(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_freq;
    oal_uint32 ul_off_set;
    oal_int8 ac_freq[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;

    if ((pst_net_dev == OAL_PTR_NULL) || (pc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_freq::pst_net_dev or pc_param is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* pc_paramָ���´�����net_device��name, ����ȡ����ŵ�ac_name�� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_freq, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_freq::wal_get_cmd_one_arg vap name return err_code %d!}\r\n",
            ul_ret);
        return ul_ret;
    }

    l_freq = oal_atoi(ac_freq);
    OAM_INFO_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_freq::l_freq = %d!}\r\n", l_freq);

    /* **************************************************************************
        ���¼���wal�㴦��
    ************************************************************************** */
    /* ��д��Ϣ */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_CURRENT_CHANEL, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_freq;

    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
        (oal_uint8 *)&st_write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_freq::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32 wal_hipriv_set_txpower(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_int32 l_pwer;
    oal_uint32 ul_off_set;
    oal_int8 ac_val[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_val, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_txpower::wal_get_cmd_one_arg vap name return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    l_pwer = oal_atoi(ac_val);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TX_POWER, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_pwer;

    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
        (oal_uint8 *)&st_write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_txpower::return err code %d!}", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_beacon_interval(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_beacon_interval;
    oal_uint32 ul_off_set;
    oal_int8 ac_beacon_interval[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;

    /* �豸��up״̬���������ã�������down */
    if ((OAL_IFF_RUNNING & OAL_NETDEVICE_FLAGS(pst_net_dev)) != 0) {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_hipriv_set_beacon_interval::device is busy, please down it firs %d!}\r\n",
            OAL_NETDEVICE_FLAGS(pst_net_dev));
        return OAL_ERR_CODE_ALREADY_OPEN;
    }

    /* pc_paramָ���´�����net_device��name, ����ȡ����ŵ�ac_name�� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_beacon_interval, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG,
            "{wal_hipriv_set_beacon_interval::wal_get_cmd_one_arg vap name return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    l_beacon_interval = oal_atoi(ac_beacon_interval);
    OAM_INFO_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_beacon_interval::l_beacon_interval = %d!}\r\n", l_beacon_interval);

    /* **************************************************************************
        ���¼���wal�㴦��
    ************************************************************************** */
    /* ��д��Ϣ */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_BEACON_INTERVAL, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_beacon_interval;

    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
        (oal_uint8 *)&st_write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_set_beacon_interval::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_start_vap(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    OAM_ERROR_LOG0(0, OAM_SF_CFG, "DEBUG:: priv start enter.");
    wal_netdev_open(pst_net_dev);
    return OAL_SUCC;
}

/*
 * �� �� ��  : wal_octl_get_essid
 * ��������  : ��ȡssid
 */
OAL_STATIC int wal_ioctl_get_essid(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info,
    oal_iwreq_data_union *pst_data, char *pc_ssid)
{
    oal_int32 l_ret;
    wal_msg_query_stru st_query_msg;
    mac_cfg_ssid_param_stru *pst_ssid = OAL_PTR_NULL;
    wal_msg_stru *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_rsp_stru *pst_query_rsp_msg = OAL_PTR_NULL;
    oal_iw_point_stru *pst_essid = (oal_iw_point_stru *)pst_data;

    /* ���¼���wal�㴦�� */
    st_query_msg.en_wid = WLAN_CFGID_SSID;

    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_QUERY, WAL_MSG_WID_LENGTH, (oal_uint8 *)&st_query_msg,
        OAL_TRUE, &pst_rsp_msg);
    if (l_ret != OAL_SUCC || pst_rsp_msg == OAL_PTR_NULL) {
        if (pst_rsp_msg != OAL_PTR_NULL) {
            oal_free(pst_rsp_msg);
        }
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_get_essid:: wal_send_cfg_event return err code %d!}", l_ret);
        return -OAL_EFAIL;
    }

    /* ��������Ϣ */
    pst_query_rsp_msg = (wal_msg_rsp_stru *)(pst_rsp_msg->auc_msg_data);

    /* ҵ���� */
    pst_ssid = (mac_cfg_ssid_param_stru *)(pst_query_rsp_msg->auc_value);
    pst_essid->flags = 1; /* ���ó��α�־Ϊ��Ч */
    pst_essid->length = OAL_MIN(pst_ssid->uc_ssid_len, OAL_IEEE80211_MAX_SSID_LEN);
    if (memcpy_s(pc_ssid, OAL_IEEE80211_MAX_SSID_LEN, pst_ssid->ac_ssid, pst_essid->length) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_ioctl_get_essid::memcpy_s failed!");
        oal_free(pst_rsp_msg);
        return OAL_FAIL;
    }

    oal_free(pst_rsp_msg);
    return OAL_SUCC;
}


OAL_STATIC int wal_ioctl_get_apaddr(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info,
    oal_iwreq_data_union *pst_wrqu, char *pc_extra)
{
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    oal_sockaddr_stru *pst_addr = (oal_sockaddr_stru *)pst_wrqu;
    oal_uint8 auc_zero_addr[WLAN_MAC_ADDR_LEN] = {0};

    if ((pst_net_dev == OAL_PTR_NULL) || (pst_addr == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_ioctl_get_apaddr::param null}");
        return -OAL_EINVAL;
    }

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (pst_mac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_get_apaddr::pst_mac_vap is null!}\r\n");
        return -OAL_EFAUL;
    }

    if (pst_mac_vap->en_vap_state == MAC_VAP_STATE_UP) {
        oal_set_mac_addr((oal_uint8 *)pst_addr->sa_data, pst_mac_vap->auc_bssid);
    } else {
        oal_set_mac_addr((oal_uint8 *)pst_addr->sa_data, auc_zero_addr);
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_11D

OAL_STATIC oal_bool_enum_uint8 wal_is_alpha_upper(oal_int8 c_letter)
{
    if (c_letter >= 'A' && c_letter <= 'Z') {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}


oal_uint8 wal_regdomain_get_band(oal_uint32 ul_start_freq, oal_uint32 ul_end_freq)
{
    if (ul_start_freq > 2400 && ul_end_freq < 2500) {
        return MAC_RC_START_FREQ_2;
    }
#ifdef _PRE_WLAN_FEATURE_5G
    else if (ul_start_freq > 5000 && ul_end_freq < 5870) {
        return MAC_RC_START_FREQ_5;
    } else if (ul_start_freq > 4900 && ul_end_freq < 4999) {
        return MAC_RC_START_FREQ_5;
    }
#endif /* _PRE_WLAN_FEATURE_5G */
    else {
        return MAC_RC_START_FREQ_BUTT;
    }
}


oal_uint8 wal_regdomain_get_bw(oal_uint8 uc_bw)
{
    oal_uint8 uc_bw_map;

    switch (uc_bw) {
        case 80:
            uc_bw_map = MAC_CH_SPACING_80MHZ;
            break;
        case 40:
            uc_bw_map = MAC_CH_SPACING_40MHZ;
            break;
        case 20:
            uc_bw_map = MAC_CH_SPACING_20MHZ;
            break;
        default:
            uc_bw_map = MAC_CH_SPACING_BUTT;
            break;
    };

    return uc_bw_map;
}


oal_uint32 wal_regdomain_get_channel_2g(oal_uint32 ul_start_freq, oal_uint32 ul_end_freq)
{
    oal_uint32 ul_freq;
    oal_uint32 ul_i;
    oal_uint32 ul_ch_bmap = 0;

    for (ul_freq = ul_start_freq + 10; ul_freq <= (ul_end_freq - 10); ul_freq++) {
        for (ul_i = 0; ul_i < MAC_CHANNEL_FREQ_2_BUTT; ul_i++) {
            if (ul_freq == g_ast_freq_map_2g[ul_i].us_freq) {
                ul_ch_bmap |= (1 << ul_i);
            }
        }
    }

    return ul_ch_bmap;
}
#ifdef _PRE_WLAN_FEATURE_5G

oal_uint32 wal_regdomain_get_channel_5g(oal_uint32 ul_start_freq, oal_uint32 ul_end_freq)
{
    oal_uint32 ul_freq;
    oal_uint32 ul_i;
    oal_uint32 ul_ch_bmap = 0;

    for (ul_freq = ul_start_freq + 10; ul_freq <= (ul_end_freq - 10); ul_freq += 5) {
        for (ul_i = 0; ul_i < MAC_CHANNEL_FREQ_5_BUTT; ul_i++) {
            if (ul_freq == g_ast_freq_map_5g[ul_i].us_freq) {
                ul_ch_bmap |= (1 << ul_i);
            }
        }
    }

    return ul_ch_bmap;
}
#endif /* _PRE_WLAN_FEATURE_5G */


oal_uint32 wal_regdomain_get_channel(oal_uint8 uc_band, oal_uint32 ul_start_freq, oal_uint32 ul_end_freq)
{
    oal_uint32 ul_ch_bmap = 0;

    switch (uc_band) {
        case MAC_RC_START_FREQ_2:
            ul_ch_bmap = wal_regdomain_get_channel_2g(ul_start_freq, ul_end_freq);
            break;
#ifdef _PRE_WLAN_FEATURE_5G
        case MAC_RC_START_FREQ_5:
            ul_ch_bmap = wal_regdomain_get_channel_5g(ul_start_freq, ul_end_freq);
            break;
#endif /* _PRE_WLAN_FEATURE_5G */
        default:
            break;
    }

    return ul_ch_bmap;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 3, 0))
extern oal_ieee80211_supported_band hi1151_band_2ghz;

oal_uint32 wal_linux_update_wiphy_channel_list_num(oal_net_device_stru *pst_net_dev, oal_wiphy_stru *pst_wiphy)
{
    oal_uint16 us_len;
    oal_uint32 ul_ret;
    mac_vendor_cmd_channel_list_stru st_channel_list;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;

    if (pst_wiphy == OAL_PTR_NULL || pst_net_dev == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_linux_update_wiphy_channel_list_num::null param}");
        return OAL_PTR_NULL;
    }

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (pst_mac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_linux_update_wiphy_channel_list_num::NET_DEV_PRIV is NULL.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_vendor_cmd_get_channel_list(pst_mac_vap, &us_len, (oal_uint8 *)(&st_channel_list));
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_linux_update_wiphy_channel_list_num::get_channel_list fail. %d}", ul_ret);
        return ul_ret;
    }

    /* ֻ����2G�ŵ�������5G �ŵ����ڴ���DFS �����Ҵ�����㲢�����⣬����Ҫ�޸� */
    hi1151_band_2ghz.n_channels = st_channel_list.uc_channel_num_2g;

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_linux_update_wiphy_channel_list_num::2g_channel_num = %d}",
        st_channel_list.uc_channel_num_2g);
#ifdef _PRE_WLAN_FEATURE_5G
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_linux_update_wiphy_channel_list_num::5g_channel_num = %d}",
        st_channel_list.uc_channel_num_5g);
#endif
    return OAL_SUCC;
}
#endif


OAL_STATIC OAL_INLINE oal_void wal_get_dfs_domain(mac_regdomain_info_stru *pst_mac_regdom,
    OAL_CONST oal_int8 *pc_country)
{
#ifdef _PRE_WLAN_FEATURE_DFS
    oal_uint32 u_idx;
    oal_uint32 ul_size = OAL_ARRAY_SIZE(g_ast_dfs_domain_table);

    for (u_idx = 0; u_idx < ul_size; u_idx++) {
        if (0 == oal_strcmp(g_ast_dfs_domain_table[u_idx].pc_country, pc_country)) {
            pst_mac_regdom->en_dfs_domain = g_ast_dfs_domain_table[u_idx].en_dfs_domain;

            return;
        }
    }
#endif /* _PRE_WLAN_FEATURE_DFS */

    pst_mac_regdom->en_dfs_domain = MAC_DFS_DOMAIN_NULL;
}


OAL_STATIC oal_void wal_regdomain_fill_info(OAL_CONST oal_ieee80211_regdomain_stru *pst_regdom,
    mac_regdomain_info_stru *pst_mac_regdom)
{
    oal_uint32 ul_i;
    oal_uint32 ul_start;
    oal_uint32 ul_end;
    oal_uint8 uc_band;
    oal_uint8 uc_bw;

    /* ���ƹ����ַ��� */
    pst_mac_regdom->ac_country[0] = pst_regdom->alpha2[0];
    pst_mac_regdom->ac_country[1] = pst_regdom->alpha2[1];
    pst_mac_regdom->ac_country[2] = 0;

    /* ��ȡDFS��֤��׼���� */
    wal_get_dfs_domain(pst_mac_regdom, pst_regdom->alpha2);

    /* ����������� */
    pst_mac_regdom->uc_regclass_num = (oal_uint8)pst_regdom->n_reg_rules;

    /* ����������Ϣ */
    for (ul_i = 0; ul_i < pst_regdom->n_reg_rules; ul_i++) {
        /* ��д�������Ƶ��(2.4G��5G) */
        ul_start = pst_regdom->reg_rules[ul_i].freq_range.start_freq_khz / 1000;
        ul_end = pst_regdom->reg_rules[ul_i].freq_range.end_freq_khz / 1000;
        uc_band = wal_regdomain_get_band(ul_start, ul_end);
        pst_mac_regdom->ast_regclass[ul_i].en_start_freq = uc_band;

        /* ��д����������������� */
        uc_bw = (oal_uint8)(pst_regdom->reg_rules[ul_i].freq_range.max_bandwidth_khz / 1000);
        pst_mac_regdom->ast_regclass[ul_i].en_ch_spacing = wal_regdomain_get_bw(uc_bw);

        /* ��д�������ŵ�λͼ */
        pst_mac_regdom->ast_regclass[ul_i].ul_channel_bmap = wal_regdomain_get_channel(uc_band, ul_start, ul_end);

        /* ��ǹ�������Ϊ */
        pst_mac_regdom->ast_regclass[ul_i].uc_behaviour_bmap = 0;

        if (pst_regdom->reg_rules[ul_i].flags & NL80211_RRF_DFS) {
            pst_mac_regdom->ast_regclass[ul_i].uc_behaviour_bmap |= MAC_RC_DFS;
        }

        /* ��串���������͹��� */
        pst_mac_regdom->ast_regclass[ul_i].uc_coverage_class = 0;
        pst_mac_regdom->ast_regclass[ul_i].uc_max_reg_tx_pwr =
            (oal_uint8)(pst_regdom->reg_rules[ul_i].power_rule.max_eirp / 100);
        pst_mac_regdom->ast_regclass[ul_i].uc_max_tx_pwr =
            (oal_uint8)(pst_regdom->reg_rules[ul_i].power_rule.max_eirp / 100);
    }
}

OAL_STATIC int32_t wal_regdomain_update_send_event(oal_net_device_stru *net_dev, mac_regdomain_info_stru *mac_regdom)
{
    wal_msg_stru *rsp_msg = OAL_PTR_NULL;
    wal_msg_write_stru write_msg;
    mac_cfg_country_stru *country_code_param = OAL_PTR_NULL;
    int32_t ret;

    /* **************************************************************************
        ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_COUNTRY, OAL_SIZEOF(mac_cfg_country_stru));

    /* ��дWID��Ӧ�Ĳ��� */
    country_code_param = (mac_cfg_country_stru *)(write_msg.auc_value);
    country_code_param->p_mac_regdom = mac_regdom;

    /* ������Ϣ����Ȼû��ʹ�÷�����Ϣ��������Ҫ�ȴ��¼����أ�����pst_rep_msg����ΪNULL */
    ret = wal_send_cfg_event(net_dev, WAL_MSG_TYPE_WRITE,
        WAL_MSG_WRITE_MSG_HDR_LENGTH + sizeof(mac_cfg_country_stru), (uint8_t *)&write_msg, OAL_TRUE, &rsp_msg);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_regdomain_update::return err code %d!}\r\n", ret);
        OAL_MEM_FREE(mac_regdom, OAL_TRUE);
        if (rsp_msg != OAL_PTR_NULL) {
            oal_free(rsp_msg);
        }
        return ret;
    }
    oal_free(rsp_msg);
    return OAL_SUCC;
}


oal_int32 wal_regdomain_update(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_country)
{
    OAL_CONST oal_ieee80211_regdomain_stru *pst_regdom;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_uint8 uc_dev_id;
    mac_device_stru *pst_device = OAL_PTR_NULL;
    mac_board_stru *pst_hmac_board = OAL_PTR_NULL;
#endif
    oal_uint16 us_size;
    mac_regdomain_info_stru *pst_mac_regdom = OAL_PTR_NULL;
    oal_int32 l_ret;

#ifndef _PRE_PLAT_FEATURE_CUSTOMIZE
    oal_int8 *pc_current_country = OAL_PTR_NULL;
#endif
    if (!wal_is_alpha_upper(pc_country[0]) || !wal_is_alpha_upper(pc_country[1])) {
        if ((pc_country[0] == '9') && (pc_country[1] == '9')) {
            OAM_INFO_LOG0(0, OAM_SF_ANY, "{wal_regdomain_update::set regdomain to 99!}\r\n");
        } else {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_regdomain_update::country str is invalid!}\r\n");
            return -OAL_EINVAL;
        }
    }

#ifndef _PRE_PLAT_FEATURE_CUSTOMIZE
    pc_current_country = mac_regdomain_get_country();
    /* ��ǰ��������Ҫ���õĹ�����һ�£�ֱ�ӷ��� */
    if ((pc_country[0] == pc_current_country[0]) && (pc_country[1] == pc_current_country[1])) {
        return OAL_SUCC;
    }
#endif /* #ifndef _PRE_PLAT_FEATURE_CUSTOMIZE */

#if _PRE_OS_VERSION == _PRE_OS_VERSION_LINUX
    hwifi_set_country_code(pc_country, COUNTRY_CODE_LEN);
#endif

    pst_regdom = wal_regdb_find_db(pc_country);
    if (pst_regdom == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_regdomain_update::no regdomain db was found!}\r\n");
        return -OAL_EINVAL;
    }

    us_size = (oal_uint16)(OAL_SIZEOF(mac_regclass_info_stru) * pst_regdom->n_reg_rules + MAC_RD_INFO_LEN);

    /* �����ڴ��Ź�������Ϣ�����ڴ�ָ����Ϊ�¼�payload����ȥ */
    /* �˴�������ڴ����¼��������ͷ�(hmac_config_set_country) */
    pst_mac_regdom = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, us_size, OAL_TRUE);
    if (pst_mac_regdom == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_regdomain_update::alloc regdom mem fail, return null ptr!}\r\n");
        return -OAL_ENOMEM;
    }

    wal_regdomain_fill_info(pst_regdom, pst_mac_regdom);
    l_ret = wal_regdomain_update_send_event(pst_net_dev, pst_mac_regdom);
    if (l_ret != OAL_SUCC) {
        return l_ret;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* ����������������Ҫͬʱ�������ں�ע��Ĺ����� */
    hmac_board_get_instance(&pst_hmac_board);
    uc_dev_id = pst_hmac_board->ast_chip[0].auc_device_id[0];
    pst_device = mac_res_get_dev(uc_dev_id);
    if ((pst_device != OAL_PTR_NULL) && (pst_device->pst_wiphy != OAL_PTR_NULL)) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 3, 0))
        
        wal_linux_update_wiphy_channel_list_num(pst_net_dev, pst_device->pst_wiphy);
#endif
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_regdomain_update::update regdom to kernel.}\r\n");
        wal_cfg80211_reset_bands();
        oal_wiphy_apply_custom_regulatory(pst_device->pst_wiphy, pst_regdom);
        
        wal_cfg80211_save_bands();
    }
#endif

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_DFS // 1131_debug

oal_int32 wal_regdomain_update_for_dfs(oal_net_device_stru *pst_net_dev, oal_int8 *pc_country)
{
    OAL_CONST oal_ieee80211_regdomain_stru *pst_regdom;
    oal_uint16 us_size;
    mac_regdomain_info_stru *pst_mac_regdom = OAL_PTR_NULL;
    wal_msg_write_stru st_write_msg;
    mac_dfs_domain_enum_uint8 *pst_param = OAL_PTR_NULL;
    oal_int32 l_ret;
    oal_int8 *pc_current_country = OAL_PTR_NULL;

    if (!wal_is_alpha_upper(pc_country[0]) || !wal_is_alpha_upper(pc_country[1])) {
        if ((pc_country[0] == '9') && (pc_country[1] == '9')) {
            OAM_INFO_LOG0(0, OAM_SF_ANY, "{wal_regdomain_update_for_dfs::set regdomain to 99!}\r\n");
        } else {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_regdomain_update_for_dfs::country str is invalid!}\r\n");
            return -OAL_EINVAL;
        }
    }

    pc_current_country = mac_regdomain_get_country();
    /* ��ǰ��������Ҫ���õĹ�����һ�£�ֱ�ӷ��� */
    if ((pc_country[0] == pc_current_country[0]) && (pc_country[1] == pc_current_country[1])) {
        return OAL_SUCC;
    }

    pst_regdom = wal_regdb_find_db(pc_country);
    if (pst_regdom == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_regdomain_update_for_dfs::no regdomain db was found!}\r\n");
        return -OAL_EINVAL;
    }

    us_size = (oal_uint16)(OAL_SIZEOF(mac_regclass_info_stru) * pst_regdom->n_reg_rules + MAC_RD_INFO_LEN);

    /* �����ڴ��Ź�������Ϣ,�ڱ������������ͷ� */
    pst_mac_regdom = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, us_size, OAL_TRUE);
    if (pst_mac_regdom == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_regdomain_update_for_dfs::alloc regdom mem fail, return null ptr!}\r\n");
        return -OAL_ENOMEM;
    }

    wal_regdomain_fill_info(pst_regdom, pst_mac_regdom);

    /* **************************************************************************
        ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_COUNTRY_FOR_DFS, OAL_SIZEOF(mac_dfs_domain_enum_uint8));

    /* ��дWID��Ӧ�Ĳ��� */
    pst_param = (mac_dfs_domain_enum_uint8 *)(st_write_msg.auc_value);
    *pst_param = pst_mac_regdom->en_dfs_domain;

    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE,
        WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_dfs_domain_enum_uint8), (oal_uint8 *)&st_write_msg, OAL_FALSE,
        OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        /* pst_mac_regdom�ڴ棬�˴��ͷ� */
        OAL_MEM_FREE(pst_mac_regdom, OAL_TRUE);
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_regdomain_update_for_dfs::return err code %d!}\r\n", l_ret);
        return l_ret;
    }
    /* pst_mac_regdom�ڴ棬�˴��ͷ� */
    OAL_MEM_FREE(pst_mac_regdom, OAL_TRUE);

    return OAL_SUCC;
}

#endif


oal_uint32 wal_regdomain_update_sta(oal_uint8 uc_vap_id)
{
    oal_int8 *pc_desired_country = OAL_PTR_NULL;

    oal_net_device_stru *pst_net_dev = OAL_PTR_NULL;
    oal_int32 l_ret;
    oal_bool_enum_uint8 us_updata_rd_by_ie_switch;

    hmac_vap_get_updata_rd_by_ie_switch(uc_vap_id, &us_updata_rd_by_ie_switch);

    if (us_updata_rd_by_ie_switch == OAL_TRUE) {
        pc_desired_country = hmac_vap_get_desired_country(uc_vap_id);
        if (OAL_UNLIKELY(pc_desired_country == OAL_PTR_NULL)) {
            OAM_ERROR_LOG0(uc_vap_id, OAM_SF_ANY, "{wal_regdomain_update_sta::pc_desired_country is null ptr!}\r\n");
            return OAL_ERR_CODE_PTR_NULL;
        }

        /* �����Ĺ�����ȫΪ0����ʾ�Զ�AP�Ĺ����벻���ڣ�����sta��ǰĬ�ϵĹ����� */
        if ((pc_desired_country[0] == 0) && (pc_desired_country[1] == 0)) {
            OAM_INFO_LOG0(uc_vap_id, OAM_SF_ANY,
                "{wal_regdomain_update_sta::ap does not have country ie, use default!}\r\n");
            return OAL_SUCC;
        }

        pst_net_dev = hmac_vap_get_net_device(uc_vap_id);

#ifdef _PRE_WLAN_FEATURE_DFS // 1131_debug
        l_ret = wal_regdomain_update_for_dfs(pst_net_dev, pc_desired_country);
        if (l_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(uc_vap_id, OAM_SF_ANY, "{wal_regdomain_update_sta::wal_regdomain_update err code %d!}\r\n",
                l_ret);
            return OAL_FAIL;
        }
#endif

        l_ret = wal_regdomain_update(pst_net_dev, pc_desired_country);
        if (l_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(uc_vap_id, OAM_SF_ANY, "{wal_regdomain_update_sta::wal_regdomain_update err code %d!}\r\n",
                l_ret);
            return OAL_FAIL;
        }

        OAM_INFO_LOG2(uc_vap_id, OAM_SF_ANY, "{wal_regdomain_update_sta::country is %u, %u!}\r\n",
            (oal_uint8)pc_desired_country[0], (oal_uint8)pc_desired_country[1]);
    } else {
        OAM_INFO_LOG0(uc_vap_id, OAM_SF_ANY, "{wal_regdomain_update_sta::us_updata_rd_by_ie_switch is OAL_FALSE!}\r\n");
    }
    return OAL_SUCC;
}


oal_uint32 wal_hipriv_set_rd_by_ie_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_switch_enum_uint8 *pst_set_rd_by_ie_switch = OAL_PTR_NULL;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_switch_enum_uint8 en_rd_by_ie_switch;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_RD_IE_SWITCH, OAL_SIZEOF(oal_switch_enum_uint8));

    /* ��������������������� */
    pst_set_rd_by_ie_switch = (oal_switch_enum_uint8 *)(st_write_msg.auc_value);

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_rd_by_ie_switch::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    en_rd_by_ie_switch = (oal_uint8)oal_atoi(ac_name);
    *pst_set_rd_by_ie_switch = en_rd_by_ie_switch;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_switch_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rd_by_ie_switch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif


#ifdef _PRE_WLAN_FEATURE_PM
OAL_STATIC oal_uint32 wal_hipriv_wifi_enable(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32          l_tmp;
    oal_uint32         ul_off_set;
    oal_int8           ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32          l_ret;
    oal_uint32         ul_ret;

    /*
     * OAM logģ��Ŀ��ص�����: hipriv "Hisilicon0 enable 0 | 1"
     * �˴���������"1"��"0"����ac_name
     */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_wifi_enable::wal_get_cmd_one_arg return err_code %d!}", ul_ret);
        return ul_ret;
    }

    /* ��Խ������Ĳ�ͬ�����logģ����в�ͬ������ */
    if ((oal_strcmp("0", ac_name)) == 0) {
        l_tmp = 0;
    } else if ((oal_strcmp("1", ac_name)) == 0) {
        l_tmp = 1;
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_wifi_enable::command param is error!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_WIFI_EN, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp; /* ��������������� */

    l_ret = wal_send_cfg_event(pst_cfg_net_dev, WAL_MSG_TYPE_WRITE,
        WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32), (oal_uint8 *)&st_write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_wifi_enable::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_pm_info(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PM_INFO, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pm_info::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_pm_enable(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32          l_tmp;
    oal_uint32         ul_off_set;
    oal_int8           ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32          l_ret;
    oal_uint32         ul_ret;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_wifi_enable::wal_get_cmd_one_arg return err_code %d!}", ul_ret);
        return ul_ret;
    }

    /* ��Խ������Ĳ�ͬ�����logģ����в�ͬ������ */
    if ((oal_strcmp("0", ac_name)) == 0) {
        l_tmp = 0;
    } else if ((oal_strcmp("1", ac_name)) == 0) {
        l_tmp = 1;
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_wifi_enable::command param is error!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PM_EN, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp; /* ��������������� */

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pm_enable::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

oal_int32 wal_set_random_mac_switch_case(oal_net_device_stru *pst_net_dev, mac_vap_stru *pst_mac_vap,
    oal_wireless_dev_stru *pst_wdev, const oal_uint8 *auc_primary_mac_addr, oal_uint8 *en_p2p_mode)
{
    oal_uint32 l_ret = EOK;

    if (OAL_ANY_NULL_PTR3(pst_net_dev, pst_mac_vap, pst_wdev)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_set_random_mac_switch_case::param NULL");
        return -OAL_ENOMEM;
    }
    switch (pst_wdev->iftype) {
        case NL80211_IFTYPE_P2P_DEVICE:
            *en_p2p_mode = WLAN_P2P_DEV_MODE;

            /* ����P2P device MAC ��ַ��������mac ��ַbit ����Ϊ1 */
            l_ret += memcpy_s(pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_p2p0_dot11StationID,
                WLAN_MAC_ADDR_LEN, OAL_NETDEVICE_MAC_ADDR(pst_net_dev), WLAN_MAC_ADDR_LEN);
            break;
        case NL80211_IFTYPE_P2P_CLIENT:
            *en_p2p_mode = WLAN_P2P_CL_MODE;
            /* ����P2P interface MAC ��ַ */
            l_ret += memcpy_s(pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID, WLAN_MAC_ADDR_LEN,
                auc_primary_mac_addr, WLAN_MAC_ADDR_LEN);
            pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID[0] |= 0x02;
            pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID[4] ^= 0x80;
            break;
        case NL80211_IFTYPE_P2P_GO:
            *en_p2p_mode = WLAN_P2P_GO_MODE;
            /* ����P2P interface MAC ��ַ */
            l_ret += memcpy_s(pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID, WLAN_MAC_ADDR_LEN,
                auc_primary_mac_addr, WLAN_MAC_ADDR_LEN);
            pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID[0] |= 0x02;
            pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID[4] ^= 0x80;
            break;
        default:
            if ((oal_strcmp("p2p0", pst_net_dev->name)) == 0) {
                *en_p2p_mode = WLAN_P2P_DEV_MODE;
                /* ����P2P device MAC ��ַ��������mac ��ַbit ����Ϊ1 */
                l_ret += memcpy_s(pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_p2p0_dot11StationID,
                    WLAN_MAC_ADDR_LEN, OAL_NETDEVICE_MAC_ADDR(pst_net_dev), WLAN_MAC_ADDR_LEN);
                break;
            }

            l_ret += memcpy_s(pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID, WLAN_MAC_ADDR_LEN,
                OAL_NETDEVICE_MAC_ADDR(pst_net_dev), WLAN_MAC_ADDR_LEN);
            break;
    }
    if (l_ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_set_random_mac_to_mib::memcpy_s failed!");
        return -OAL_ENOMEM;
    }
    return OAL_SUCC;
}


oal_int32 wal_set_random_mac_to_mib(oal_net_device_stru *pst_net_dev)
{
    oal_uint32                    ul_ret;
    frw_event_mem_stru           *pst_event_mem = OAL_PTR_NULL;
    wal_msg_stru                 *pst_cfg_msg = OAL_PTR_NULL;
    wal_msg_write_stru           *pst_write_msg = OAL_PTR_NULL;
    mac_cfg_staion_id_param_stru *pst_param = OAL_PTR_NULL;
    mac_vap_stru                 *pst_mac_vap = OAL_PTR_NULL;
    oal_uint8                    *puc_mac_addr = OAL_PTR_NULL;
#ifdef _PRE_WLAN_FEATURE_P2P
    oal_wireless_dev_stru        *pst_wdev = OAL_PTR_NULL; /* ����P2P ������p2p0 �� p2p-p2p0 MAC ��ַ��wlan0 ��ȡ */
    wlan_p2p_mode_enum_uint8      en_p2p_mode = WLAN_LEGACY_VAP_MODE;
    oal_uint8                    *auc_p2p0_addr = OAL_PTR_NULL;
    oal_int32                     l_ret = EOK;
#endif
    oal_uint8                    *auc_wlan_addr = OAL_PTR_NULL;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (pst_mac_vap == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_set_random_mac_to_mib::pst_mac_vap NULL}");
        return OAL_FAIL;
    }

    if (pst_mac_vap->pst_mib_info == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_set_random_mac_to_mib::vap->mib_info is NULL !}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    auc_wlan_addr = mac_mib_get_StationID(pst_mac_vap);
#ifdef _PRE_WLAN_FEATURE_P2P
    auc_p2p0_addr = mac_mib_get_p2p0_dot11StationID(pst_mac_vap);
#endif

#ifdef _PRE_WLAN_FEATURE_P2P
    pst_wdev = pst_net_dev->ieee80211_ptr;
    switch (pst_wdev->iftype) {
        case NL80211_IFTYPE_P2P_DEVICE:
            en_p2p_mode = WLAN_P2P_DEV_MODE;

            /* ����P2P device MAC ��ַ��������mac ��ַbit ����Ϊ1 */
            l_ret = memcpy_s(auc_p2p0_addr, WLAN_MAC_ADDR_LEN, OAL_NETDEVICE_MAC_ADDR(pst_net_dev), WLAN_MAC_ADDR_LEN);
            break;
        case NL80211_IFTYPE_P2P_CLIENT:
        case NL80211_IFTYPE_P2P_GO:
            en_p2p_mode = (pst_wdev->iftype == NL80211_IFTYPE_P2P_GO) ? WLAN_P2P_GO_MODE : WLAN_P2P_CL_MODE;
            /* �����ϲ��·�ֵ������P2P interface MAC ��ַ */
            /* �ϲ㲻�·���������mac��ַ,��wal_cfg80211_add_p2p_interface_init��ʼ�� */
            l_ret += memcpy_s(auc_wlan_addr, WLAN_MAC_ADDR_LEN, OAL_NETDEVICE_MAC_ADDR(pst_net_dev), WLAN_MAC_ADDR_LEN);
            break;

        default:
            if ((oal_strcmp("p2p0", pst_net_dev->name)) == 0) {
                en_p2p_mode = WLAN_P2P_DEV_MODE;
                /* ����P2P device MAC ��ַ��������mac ��ַbit ����Ϊ1 */
                l_ret += memcpy_s(auc_p2p0_addr, WLAN_MAC_ADDR_LEN,
                                  OAL_NETDEVICE_MAC_ADDR(pst_net_dev), WLAN_MAC_ADDR_LEN);
                break;
            }

            en_p2p_mode = WLAN_LEGACY_VAP_MODE;
            l_ret += memcpy_s(auc_wlan_addr, WLAN_MAC_ADDR_LEN, OAL_NETDEVICE_MAC_ADDR(pst_net_dev), WLAN_MAC_ADDR_LEN);
            break;
    }
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_set_random_mac_to_mib::p2p mode [%d]}", en_p2p_mode);
    if (l_ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_set_random_mac_to_mib::memcpy fail!");
        return OAL_FAIL;
    }
#else
    /* random mac will be used. hi1102-cb (#include <linux/etherdevice.h>)    */
    wal_set_random_mac_addr(auc_wlan_addr);
#endif
    /*
     * send the random mac to dmac
     * ���¼���wal�㴦��   copy from wal_netdev_set_mac_addr()
     * ��Ϊ����ͨ�õ�config�ӿ�
     */
    ul_ret = wal_alloc_cfg_event(pst_net_dev, &pst_event_mem, NULL, &pst_cfg_msg,
        (WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_staion_id_param_stru))); /* �����¼� */
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_set_random_mac_to_mib() fail; return %d!}", ul_ret);
        return -OAL_ENOMEM;
    }

    /* ��д������Ϣ */
    WAL_CFG_MSG_HDR_INIT(&(pst_cfg_msg->st_msg_hdr),
                         WAL_MSG_TYPE_WRITE,
                         WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_staion_id_param_stru),
                         WAL_GET_MSG_SN());

    /* ��дWID��Ϣ */
    pst_write_msg = (wal_msg_write_stru *)pst_cfg_msg->auc_msg_data;
    WAL_WRITE_MSG_HDR_INIT(pst_write_msg, WLAN_CFGID_STATION_ID, OAL_SIZEOF(mac_cfg_staion_id_param_stru));

    pst_param = (mac_cfg_staion_id_param_stru *)pst_write_msg->auc_value; /* ��дWID��Ӧ�Ĳ��� */
#ifdef _PRE_WLAN_FEATURE_P2P
    /* ���ʹ��P2P����Ҫ��netdevice ��Ӧ��P2P ģʽ�����ò��������õ�hmac ��dmac */
    /* �Ա�ײ�ʶ���䵽p2p0 ��p2p-p2p0 cl */
    pst_param->en_p2p_mode = en_p2p_mode;
    if (en_p2p_mode == WLAN_P2P_DEV_MODE) {
        puc_mac_addr = mac_mib_get_p2p0_dot11StationID(pst_mac_vap);
    } else
#endif
    {
        puc_mac_addr = mac_mib_get_StationID(pst_mac_vap);
    }
    oal_set_mac_addr(pst_param->auc_station_id, puc_mac_addr);

    OAM_WARNING_LOG4(0, OAM_SF_ANY, "{wal_set_random_mac_to_mib [%.2x:%.2x:xx:xx:%.2x:%.2x]}",
                     puc_mac_addr[0], puc_mac_addr[1], puc_mac_addr[4], puc_mac_addr[5]);

    frw_event_dispatch_event(pst_event_mem); /* �ַ��¼� */
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL

OAL_STATIC oal_uint32 wal_hipriv_get_hipkt_stat(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;

    /* �����¼��ڴ� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_HIPKT_STAT, OAL_SIZEOF(oal_uint8));

    /* ���¼���wal�㴦�� */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_hipkt_stat:: return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_flowctl_param(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32         ul_ret;
    oal_uint32         ul_off_set = 0;
    oal_int32          l_ret;
    oal_int8           ac_param[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    mac_cfg_flowctl_param_stru  st_flowctl_param;
    mac_cfg_flowctl_param_stru *pst_param = OAL_PTR_NULL;

    // sh hipriv.sh "Hisilicon0 set_flowctl_param 0/1/2/3 20 20 40"
    // 0/1/2/3 �ֱ����be,bk,vi,vo
    /* ��ȡ�������Ͳ��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_param, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_flowctl_param::wal_get_cmd_one_arg return err[%d]!}", ul_ret);
        return ul_ret;
    }
    st_flowctl_param.uc_queue_type = (oal_uint8)oal_atoi(ac_param);

    /* ���ö��ж�Ӧ��ÿ�ε��ȱ��ĸ��� */
    pc_param += ul_off_set;
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_param, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_flowctl_param::wal_get_cmd_one_arg burst_limit return err_code %d!}\r\n", ul_ret);
        return (oal_uint32)ul_ret;
    }
    st_flowctl_param.us_burst_limit = (oal_uint16)oal_atoi(ac_param);

    /* ���ö��ж�Ӧ������low_waterline */
    pc_param += ul_off_set;
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_param, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_flowctl_param::wal_get_cmd_one_arg low_waterline return err_code %d!}\r\n", ul_ret);
        return (oal_uint32)ul_ret;
    }
    st_flowctl_param.us_low_waterline = (oal_uint16)oal_atoi(ac_param);

    /* ���ö��ж�Ӧ������high_waterline */
    pc_param += ul_off_set;
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_param, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_flowctl_param::wal_get_cmd_one_arg high_waterline return err_code %d!}\r\n", ul_ret);
        return (oal_uint32)ul_ret;
    }
    st_flowctl_param.us_high_waterline = (oal_uint16)oal_atoi(ac_param);

    /* �����¼��ڴ� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_FLOWCTL_PARAM, OAL_SIZEOF(mac_cfg_flowctl_param_stru));
    pst_param = (mac_cfg_flowctl_param_stru *)(st_write_msg.auc_value);

    pst_param->uc_queue_type = st_flowctl_param.uc_queue_type;
    pst_param->us_burst_limit = st_flowctl_param.us_burst_limit;
    pst_param->us_low_waterline = st_flowctl_param.us_low_waterline;
    pst_param->us_high_waterline = st_flowctl_param.us_high_waterline;

    /* ���¼���wal�㴦�� */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_flowctl_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_flowctl_param:: return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_get_flowctl_stat(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;

    // sh hipriv.sh "Hisilicon0 get_flowctl_stat"
    /* �����¼��ڴ� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_FLOWCTL_STAT, OAL_SIZEOF(oal_uint8));

    /* ���¼���wal�㴦�� */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_flowctl_stat:: return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32 wal_hipriv_setcountry(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
#ifdef _PRE_WLAN_FEATURE_11D
    oal_int32 l_ret;
    oal_uint32 ul_ret;
    oal_uint32 ul_off_set;
    oal_int8 ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int8 *puc_para = OAL_PTR_NULL;

    /* �豸��up״̬���������ã�������down */
    if ((OAL_IFF_RUNNING & OAL_NETDEVICE_FLAGS(pst_net_dev)) != 0) {
        OAM_INFO_LOG1(0, OAM_SF_ANY, "{wal_hipriv_setcountry::country is %d, %d!}\r\n",
            OAL_NETDEVICE_FLAGS(pst_net_dev));
        return OAL_EBUSY;
    }
    /* ��ȡ�������ַ��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_setcountry::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }
    puc_para = &ac_arg[0];

#ifdef _PRE_WLAN_FEATURE_DFS // 1131_debug
    l_ret = wal_regdomain_update_for_dfs(pst_net_dev, puc_para);
    if (l_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_setcountry::regdomain_update return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
#endif

    l_ret = wal_regdomain_update(pst_net_dev, puc_para);
    if (l_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_setcountry::regdomain_update return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

#else
    OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_setcountry::_PRE_WLAN_FEATURE_11D is not define!}\r\n");

#endif
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_getcountry(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
#ifdef _PRE_WLAN_FEATURE_11D
    wal_msg_query_stru st_query_msg;
    wal_msg_rsp_stru   *query_rsp_msg = NULL;
    int8_t             tmp_buff[OAM_PRINT_FORMAT_LENGTH] = {0};
    wal_msg_stru       *rsp_msg = NULL;
    oal_int32 l_ret;

    /* ���¼���wal�㴦�� */
    st_query_msg.en_wid = WLAN_CFGID_COUNTRY;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_QUERY,
                               WAL_MSG_WID_LENGTH,
                               (oal_uint8 *)&st_query_msg,
                               OAL_TRUE,
                               &rsp_msg);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        if (rsp_msg != NULL) {
            oal_free(rsp_msg);
        }
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_getcountry::wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
    /* ��������Ϣ */
    query_rsp_msg = (wal_msg_rsp_stru *)(rsp_msg->auc_msg_data);

    l_ret = snprintf_s(tmp_buff, sizeof(tmp_buff), sizeof(tmp_buff) - 1, "getcountry code is : %c%c.\n",
                       query_rsp_msg->auc_value[0], query_rsp_msg->auc_value[1]);
    if (l_ret < 0) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "wal_hipriv_getcountry::snprintf_s error!");
        oal_free(rsp_msg);
        oam_print(tmp_buff);
        return OAL_FAIL;
    }

    OAM_WARNING_LOG3(0, OAM_SF_CFG, "{wal_hipriv_getcountry:: %c, %c, len %d}",
        query_rsp_msg->auc_value[0], query_rsp_msg->auc_value[1], query_rsp_msg->us_len);
    oal_free(rsp_msg);
    oam_print(tmp_buff);
#else
    OAM_INFO_LOG0(0, OAM_SF_ANY, "{wal_hipriv_getcountry::_PRE_WLAN_FEATURE_11D is not define!}\r\n");
#endif

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_add_vap(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param)
{
    oal_net_device_stru        *pst_net_dev = OAL_PTR_NULL;
    oal_uint32                  ul_ret;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int8                    ac_mode[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    wlan_vap_mode_enum_uint8    en_mode;
    oal_wireless_dev_stru      *pst_wdev = OAL_PTR_NULL;
    mac_vap_stru               *pst_cfg_mac_vap = OAL_PTR_NULL;
    mac_device_stru            *pst_mac_device = OAL_PTR_NULL;
#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
    wal_msg_write_stru          st_write_msg;
    wal_msg_stru               *pst_rsp_msg = OAL_PTR_NULL;
    oal_uint32                  ul_err_code;
    oal_int32                   l_ret;
    mac_vap_stru               *pst_mac_vap = OAL_PTR_NULL;

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    mac_vap_stru               *pst_cfg_mac_vap_proxysta = OAL_PTR_NULL;
    mac_device_stru            *pst_mac_device_proxysta = OAL_PTR_NULL;
    oal_bool_enum_uint8         en_is_proxysta      = OAL_FALSE;
    oal_bool_enum_uint8         en_is_main_proxysta = OAL_FALSE;
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_P2P
    wlan_p2p_mode_enum_uint8    en_p2p_mode = WLAN_LEGACY_VAP_MODE;
#endif

    /* pc_paramָ���´�����net_device��name, ����ȡ����ŵ�ac_name�� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_add_vap::wal_get_cmd_one_arg vap name return err%d!}", ul_ret);
        return ul_ret;
    }

    /* ac_name length��Ӧ����OAL_IF_NAME_SIZE */
    if (OAL_STRLEN(ac_name) >= OAL_IF_NAME_SIZE) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_add_vap:: vap name overlength is %d!}\r\n", OAL_STRLEN(ac_name));
        /* ��������vap name��Ϣ */
        oal_print_hex_dump((oal_uint8 *)ac_name, OAL_IF_NAME_SIZE, 32, "vap name lengh is overlong:");
        return OAL_FAIL;
    }

    pc_param += ul_off_set;

    /* pc_param ָ��'ap|sta', ����ȡ���ŵ�ac_mode�� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_mode, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_add_vap::wal_get_cmd_one_arg vap name return err %d!}", ul_ret);
        return (oal_uint32)ul_ret;
    }
#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    /* ��ȡmac device */
    pst_cfg_mac_vap_proxysta = OAL_NET_DEV_PRIV(pst_cfg_net_dev);
    pst_mac_device_proxysta = mac_res_get_dev(pst_cfg_mac_vap_proxysta->uc_device_id);
    if (OAL_UNLIKELY(pst_mac_device_proxysta == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_PROXYSTA, "{wal_hipriv_add_vap:: pst_mac_device is null ptr!}");
        return OAL_ERR_CODE_PTR_NULL;
    }
#endif
#endif

    /* ����ac_mode�ַ�����Ӧ��ģʽ */
    if ((oal_strcmp("ap", ac_mode)) == 0) {
        en_mode = WLAN_VAP_MODE_BSS_AP;
    } else if ((oal_strcmp("sta", ac_mode)) == 0) {
        en_mode = WLAN_VAP_MODE_BSS_STA;
#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
        if (pst_mac_device_proxysta->st_cap_flag.bit_proxysta == OAL_TRUE) {
            en_is_proxysta = OAL_TRUE;
            en_is_main_proxysta = OAL_TRUE;
        }
#endif
#endif
    }
#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    else if ((oal_strcmp("proxysta", ac_mode)) == 0) {
        en_mode = WLAN_VAP_MODE_BSS_STA;
        if (pst_mac_device_proxysta->st_cap_flag.bit_proxysta == OAL_TRUE) {
            en_is_proxysta = OAL_TRUE;
            en_is_main_proxysta = OAL_FALSE;
            OAM_INFO_LOG2(0, OAM_SF_PROXYSTA, "{wal_hipriv_add_vap::en_is_proxysta:%d,en_is_main_proxysta:%d}",
                en_is_proxysta, en_is_main_proxysta);
        }
    }
#endif
#endif
#ifdef _PRE_WLAN_FEATURE_P2P
    /* ����P2P ���VAP */
    else if ((oal_strcmp("p2p_device", ac_mode)) == 0) {
        en_mode = WLAN_VAP_MODE_BSS_STA;
        en_p2p_mode = WLAN_P2P_DEV_MODE;
    } else if ((oal_strcmp("p2p_cl", ac_mode)) == 0) {
        en_mode = WLAN_VAP_MODE_BSS_STA;
        en_p2p_mode = WLAN_P2P_CL_MODE;
    } else if ((oal_strcmp("p2p_go", ac_mode)) == 0) {
        en_mode = WLAN_VAP_MODE_BSS_AP;
        en_p2p_mode = WLAN_P2P_GO_MODE;
    }
#endif /* _PRE_WLAN_FEATURE_P2P */
    else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_add_vap::the mode param is invalid!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ���������net device�Ѿ����ڣ�ֱ�ӷ��� */
    /* ����dev_name�ҵ�dev */
    pst_net_dev = oal_dev_get_by_name(ac_name);
    if (pst_net_dev != OAL_PTR_NULL) {
        /* ����oal_dev_get_by_name�󣬱������oal_dev_putʹnet_dev�����ü�����һ */
        oal_dev_put(pst_net_dev);

        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_add_vap::the net_device is already exist!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡmac device */
    pst_cfg_mac_vap = OAL_NET_DEV_PRIV(pst_cfg_net_dev);
    pst_mac_device = mac_res_get_dev(pst_cfg_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(pst_cfg_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_hipriv_add_vap::pst_mac_device is null ptr!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#if defined(_PRE_WLAN_FEATURE_FLOWCTL)
    /* �˺�����һ����δ���˽�г��ȣ��˴����漰Ϊ0 */
    pst_net_dev = oal_net_alloc_netdev_mqs(0, ac_name, oal_ether_setup, WAL_NETDEV_SUBQUEUE_MAX_NUM, 1);
#elif defined(_PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL)
    /* �˺�����һ����δ���˽�г��ȣ��˴����漰Ϊ0 */
    pst_net_dev = oal_net_alloc_netdev_mqs(0, ac_name, oal_ether_setup, WLAN_NET_QUEUE_BUTT, 1);
#else
    pst_net_dev = oal_net_alloc_netdev(0, ac_name, oal_ether_setup); /* �˺�����һ����δ���˽�г��ȣ��˴����漰Ϊ0 */
#endif
    if (OAL_UNLIKELY(pst_net_dev == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(pst_cfg_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_hipriv_add_vap::pst_net_dev null ptr error!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_wdev =
        (oal_wireless_dev_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(oal_wireless_dev_stru), OAL_FALSE);
    if (OAL_UNLIKELY(pst_wdev == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_cfg_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_hipriv_add_vap::alloc mem, pst_wdev is null!}");
        oal_net_free_netdev(pst_net_dev);
        return OAL_ERR_CODE_PTR_NULL;
    }

    memset_s(pst_wdev, OAL_SIZEOF(oal_wireless_dev_stru), 0, OAL_SIZEOF(oal_wireless_dev_stru));

    /* ��netdevice���и�ֵ */
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifdef CONFIG_WIRELESS_EXT
    pst_net_dev->wireless_handlers = &g_st_iw_handler_def;
#endif /* CONFIG_WIRELESS_EXT */
#else
    pst_net_dev->wireless_handlers = &g_st_iw_handler_def;
#endif
    pst_net_dev->netdev_ops = &g_st_wal_net_dev_ops;

    OAL_NETDEVICE_DESTRUCTOR(pst_net_dev) = oal_net_free_netdev;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 44)) && (_PRE_OS_VERSION_LITEOS != _PRE_OS_VERSION)
    OAL_NETDEVICE_MASTER(pst_net_dev) = OAL_PTR_NULL;
#endif

    OAL_NETDEVICE_IFALIAS(pst_net_dev) = OAL_PTR_NULL;
    OAL_NETDEVICE_WATCHDOG_TIMEO(pst_net_dev) = 5;
    OAL_NETDEVICE_WDEV(pst_net_dev) = pst_wdev;
    OAL_NETDEVICE_QDISC(pst_net_dev, OAL_PTR_NULL);
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
#endif

    pst_wdev->netdev = pst_net_dev;

    if (en_mode == WLAN_VAP_MODE_BSS_AP) {
        pst_wdev->iftype = NL80211_IFTYPE_AP;
    } else if (en_mode == WLAN_VAP_MODE_BSS_STA) {
        pst_wdev->iftype = NL80211_IFTYPE_STATION;
    }
#ifdef _PRE_WLAN_FEATURE_P2P
    if (en_p2p_mode == WLAN_P2P_DEV_MODE) {
        pst_wdev->iftype = NL80211_IFTYPE_P2P_DEVICE;
    } else if (en_p2p_mode == WLAN_P2P_CL_MODE) {
        pst_wdev->iftype = NL80211_IFTYPE_P2P_CLIENT;
    } else if (en_p2p_mode == WLAN_P2P_GO_MODE) {
        pst_wdev->iftype = NL80211_IFTYPE_P2P_GO;
    }
#endif /* _PRE_WLAN_FEATURE_P2P */

    pst_wdev->wiphy = pst_mac_device->pst_wiphy;

    OAL_NETDEVICE_FLAGS(pst_net_dev) &= ~OAL_IFF_RUNNING; /* ��net device��flag��Ϊdown */
    wal_set_mac_addr(pst_net_dev);
#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)

    /* ���¼���wal�㴦�� */
    /* ��д��Ϣ */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADD_VAP, OAL_SIZEOF(mac_cfg_add_vap_param_stru));
    ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->pst_net_dev = pst_net_dev;
    ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->en_vap_mode = en_mode;
    ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->uc_cfg_vap_indx = pst_cfg_mac_vap->uc_vap_id;
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    if (pst_mac_device_proxysta->st_cap_flag.bit_proxysta == OAL_TRUE) {
        ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->en_is_proxysta = en_is_proxysta;
        ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->en_is_main_proxysta = en_is_main_proxysta;
    }
#endif
#ifdef _PRE_WLAN_FEATURE_P2P
    ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->en_p2p_mode = en_p2p_mode;
#endif
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->bit_11ac2g_enable =
        (oal_uint8) !!hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_11AC2G_ENABLE);
    ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->bit_disable_capab_2ght40 =
        g_st_wlan_customize.uc_disable_capab_2ght40;
#endif
    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_cfg_net_dev, WAL_MSG_TYPE_WRITE,
        WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_add_vap_param_stru), (oal_uint8 *)&st_write_msg, OAL_TRUE,
        &pst_rsp_msg);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAL_MEM_FREE(pst_wdev, OAL_FALSE);
        oal_net_free_netdev(pst_net_dev);
        OAM_WARNING_LOG1(pst_cfg_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_hipriv_add_vap::return err code %d!}", l_ret);
        return (oal_uint32)l_ret;
    }

    /* ��ȡ���صĴ����� */
    ul_err_code = wal_check_and_release_msg_resp(pst_rsp_msg);
    if (ul_err_code != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_cfg_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_hipriv_add_vap::hmac add vap fail,err code[%u]!}\r\n", ul_err_code);
        /* �쳣�����ͷ��ڴ� */
        OAL_MEM_FREE(pst_wdev, OAL_FALSE);
        oal_net_free_netdev(pst_net_dev);
        return ul_err_code;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

    if ((en_p2p_mode == WLAN_LEGACY_VAP_MODE) && (pst_mac_device->st_p2p_info.pst_primary_net_device == OAL_PTR_NULL)) {
        /* �������wlan0�� �򱣴�wlan0 Ϊ��net_device,p2p0 ��p2p-p2p0 MAC ��ַ����netdevice ��ȡ */
        pst_mac_device->st_p2p_info.pst_primary_net_device = pst_net_dev;
    }

    if (wal_set_random_mac_to_mib(pst_net_dev) != OAL_SUCC) {
        /* �쳣�����ͷ��ڴ� */
        /* �쳣�����ͷ��ڴ� */
        OAL_MEM_FREE(pst_wdev, OAL_FALSE);
        oal_net_free_netdev(pst_net_dev);
        return OAL_ERR_CODE_PTR_NULL;
    }
#endif

    /* ����netdevice��MAC��ַ��MAC��ַ��HMAC�㱻��ʼ����MIB�� */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
#ifdef _PRE_WLAN_FEATURE_P2P
    if (en_p2p_mode == WLAN_P2P_DEV_MODE) {
        oal_set_mac_addr((oal_uint8 *)OAL_NETDEVICE_MAC_ADDR(pst_net_dev),
            pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_p2p0_dot11StationID);

        pst_mac_device->st_p2p_info.uc_p2p0_vap_idx = pst_mac_vap->uc_vap_id;
    } else
#endif
    {
        oal_set_mac_addr((oal_uint8 *)OAL_NETDEVICE_MAC_ADDR(pst_net_dev),
            pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);
    }

#endif // (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
    /* ע��net_device */
    ul_ret = (oal_uint32)oal_net_register_netdev(pst_net_dev);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_add_vap::oal_net_register_netdev return error code %d!}", ul_ret);
#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
        /* �쳣�����ͷ��ڴ� */
        /* ��ɾ��vap�¼��ͷŸ������vap  */
        WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DEL_VAP, OAL_SIZEOF(mac_cfg_del_vap_param_stru));

        l_ret = wal_send_cfg_event(pst_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_del_vap_param_stru),
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_TRUE,
                                   &pst_rsp_msg);

        if (wal_check_and_release_msg_resp(pst_rsp_msg) != OAL_SUCC) {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_add_vap::wal_check_and_release_msg_resp fail.}");
        }
        if (l_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_add_vap::wal_send_cfg_event fail,err code %d!}\r\n", l_ret);
        }

        OAL_MEM_FREE(pst_wdev, OAL_FALSE);
#endif // (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
        oal_net_free_netdev(pst_net_dev);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32 wal_hipriv_del_vap(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)

    wal_msg_write_stru       st_write_msg;
    wal_msg_stru            *pst_rsp_msg = OAL_PTR_NULL;
    oal_int32                l_ret;
#ifdef _PRE_WLAN_FEATURE_P2P
    oal_wireless_dev_stru   *pst_wdev = OAL_PTR_NULL;
    wlan_p2p_mode_enum_uint8 en_p2p_mode;
#endif
#endif
    if (OAL_UNLIKELY((pst_net_dev == OAL_PTR_NULL) || (pc_param == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_del_vap::pst_net_dev or pc_param null ptr error}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �豸��up״̬������ɾ����������down */
    if (OAL_UNLIKELY((OAL_IFF_RUNNING & OAL_NETDEVICE_FLAGS(pst_net_dev)) != 0)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_del_vap::device is busy, please down it first %d!}\r\n",
            OAL_NETDEVICE_FLAGS(pst_net_dev));
        return OAL_ERR_CODE_CONFIG_BUSY;
    }
#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
    /* ���¼���wal�㴦�� */
    // ɾ��vap ʱ��Ҫ��������ֵ��
    ((mac_cfg_del_vap_param_stru *)st_write_msg.auc_value)->pst_net_dev = pst_net_dev;
#ifdef _PRE_WLAN_FEATURE_P2P
    pst_wdev = pst_net_dev->ieee80211_ptr;
    en_p2p_mode = wal_wireless_iftype_to_mac_p2p_mode(pst_wdev->iftype);
    if (en_p2p_mode == WLAN_P2P_BUTT) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_del_vap::wal_wireless_iftype_to_mac_p2p_mode return BUFF}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }
    ((mac_cfg_del_vap_param_stru *)st_write_msg.auc_value)->en_p2p_mode = en_p2p_mode;

#endif

    OAL_MEM_FREE(OAL_NETDEVICE_WDEV(pst_net_dev), OAL_TRUE);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DEL_VAP, OAL_SIZEOF(mac_cfg_del_vap_param_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_del_vap_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if (wal_check_and_release_msg_resp(pst_rsp_msg) != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_del_vap::wal_check_and_release_msg_resp fail}");
    }

    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_del_vap::return err code %d}\r\n", l_ret);
        /* ȥע�� */
        oal_net_unregister_netdev(pst_net_dev);
        return (oal_uint32)l_ret;
    }
#endif // (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
    /* ȥע�� */
    oal_net_unregister_netdev(pst_net_dev);

    return OAL_SUCC;
}


oal_uint32 wal_hipriv_vap_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg = {0};
    oal_int32 l_ret;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_VAP_INFO, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_vap_info::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_2040_channel_switch_prohibited(oal_net_device_stru *pst_net_dev,
                                                                const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint8 uc_csp;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_2040_channel_switch_prohibited::wal_get_cmd_one_arg return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    if ((oal_strcmp("0", ac_name)) == 0) {
        uc_csp = 0;
    } else if ((oal_strcmp("1", ac_name)) == 0) {
        uc_csp = 1;
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY,
            "{wal_hipriv_2040_channel_switch_prohibited::the channel_switch_prohibited switch command is error!}");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_2040_CHASWI_PROHI, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = uc_csp; /* ��������������� */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_2040_channel_switch_prohibited::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_FortyMHzIntolerant(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint8 uc_csp;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_FortyMHzIntolerant::wal_get_cmd_one_arg return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    if ((oal_strcmp("0", ac_name)) == 0) {
        uc_csp = 0;
    } else if ((oal_strcmp("1", ac_name)) == 0) {
        uc_csp = 1;
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY,
            "{wal_hipriv_set_FortyMHzIntolerant::the 2040_intolerant command is error!}");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_2040_INTOLERANT, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = uc_csp; /* ��������������� */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_FortyMHzIntolerant::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_2040_coext_support(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint8 uc_csp;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_2040_coext_support::wal_get_cmd_one_arg return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    if ((oal_strcmp("0", ac_name)) == 0) {
        uc_csp = 0;
    } else if ((oal_strcmp("1", ac_name)) == 0) {
        uc_csp = 1;
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY,
            "{wal_hipriv_set_2040_coext_support::the 2040_coexistence command is erro!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_2040_COEXISTENCE, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = uc_csp; /* ��������������� */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_2040_coext_support::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_rx_fcs_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;
    mac_cfg_rx_fcs_info_stru   *pst_rx_fcs_info = OAL_PTR_NULL;
    mac_cfg_rx_fcs_info_stru    st_rx_fcs_info;  /* ��ʱ�����ȡ��use����Ϣ */
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    oal_int32                   i_leftime;
    mac_vap_stru               *pst_mac_vap = OAL_PTR_NULL;
    hmac_vap_stru              *pst_hmac_vap = OAL_PTR_NULL;
    oal_int32                   l_rx_pckg_succ_num;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (pst_mac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_dbb_num::OAL_NET_DEV_PRIV, return null!}");
        return OAL_EINVAL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_atcmsrv_ioctl_get_dbb_num::mac_res_get_hmac_vap failed!}");
        return OAL_FAIL;
    }
#endif

    /* ��ӡ����֡��FCS��ȷ�������Ϣ:sh hipriv.sh "vap0 rx_fcs_info 0/1 1-4" 0/1  0���������1������� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_rx_fcs_info::wal_get_cmd_one_arg return err_code %d!}", ul_ret);
        return ul_ret;
    }

    st_rx_fcs_info.ul_data_op = (oal_uint32)oal_atoi(ac_name);
    if (st_rx_fcs_info.ul_data_op > 1) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_rx_fcs_info::the ul_data_op command is error!}");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_rx_fcs_info::wal_get_cmd_one_arg return err_code %d!}", ul_ret);
        return ul_ret;
    }

    st_rx_fcs_info.ul_print_info = (oal_uint32)oal_atoi(ac_name);
    if (st_rx_fcs_info.ul_print_info > 4) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_rx_fcs_info::the ul_print_info command is error!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

/* ���¼���wal�㴦�� */
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    pst_hmac_vap->st_atcmdsrv_get_status.uc_get_rx_pkct_flag = OAL_FALSE;
#endif
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RX_FCS_INFO, OAL_SIZEOF(mac_cfg_rx_fcs_info_stru));

    /* ��������������� */
    pst_rx_fcs_info = (mac_cfg_rx_fcs_info_stru *)(st_write_msg.auc_value);
    pst_rx_fcs_info->ul_data_op = st_rx_fcs_info.ul_data_op;
    pst_rx_fcs_info->ul_print_info = st_rx_fcs_info.ul_print_info;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_rx_fcs_info_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_rx_fcs_info::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    /* �����ȴ�dmac�ϱ� */
    i_leftime = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(pst_hmac_vap->query_wait_q,
        (oal_uint32)(pst_hmac_vap->st_atcmdsrv_get_status.uc_get_rx_pkct_flag == OAL_TRUE), WAL_ATCMDSRB_GET_RX_PCKT);
    if (i_leftime == 0) {
        /* ��ʱ��û���ϱ�ɨ����� */
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_atcmsrv_ioctl_get_rx_pckg::dbb_num wait for %ld ms timeout!}",
            ((WAL_ATCMDSRB_DBB_NUM_TIME * 1000) / OAL_TIME_HZ));
        return OAL_EINVAL;
    } else if (i_leftime < 0) {
        /* ��ʱ���ڲ����� */
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_atcmsrv_ioctl_get_rx_pckg::dbb_num wait for %ld ms error!}",
            ((WAL_ATCMDSRB_DBB_NUM_TIME * 1000) / OAL_TIME_HZ));
        return OAL_EINVAL;
    } else {
        /* ��������  */
        OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_atcmsrv_ioctl_get_rx_pckg::dbb_num wait for %ld ms error!}",
            ((WAL_ATCMDSRB_DBB_NUM_TIME * 1000) / OAL_TIME_HZ));

        l_rx_pckg_succ_num = (oal_long)pst_hmac_vap->st_atcmdsrv_get_status.ul_rx_pkct_succ_num;
        OAL_IO_PRINT("{wal_hipriv_rx_fcs_info} rx_pckg_succ_num is^%d^\n", l_rx_pckg_succ_num);
    }
#endif

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_global_log_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_int32 l_switch_val;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;

    /* ��ȡ����״ֵ̬ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_global_log_switch::error code[%d]}\r\n", ul_ret);
        return ul_ret;
    }

    if ((oal_strcmp("0", ac_name) != 0) && (oal_strcmp("1", ac_name) != 0)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_global_log_switch::invalid switch value}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    l_switch_val = oal_atoi(ac_name);

    return oam_log_set_global_switch((oal_switch_enum_uint8)l_switch_val);
}


OAL_STATIC oal_uint32 wal_hipriv_vap_log_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    oal_int32 l_switch_val;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_vap_log_switch::null pointer.}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ����״ֵ̬ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_global_log_switch::error code[%d]}\r\n", ul_ret);
        return ul_ret;
    }

    if ((oal_strcmp("0", ac_name) != 0) && (oal_strcmp("1", ac_name) != 0)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_global_log_switch::invalid switch value}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    l_switch_val = oal_atoi(ac_name);

    return oam_log_set_vap_switch(pst_mac_vap->uc_vap_id, (oal_switch_enum_uint8)l_switch_val);
}


OAL_STATIC oal_uint32 wal_hipriv_vap_log_level(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    oam_log_level_enum_uint8 en_level_val;
    oal_uint32 ul_off_set;
    oal_int8 ac_param[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    wal_msg_write_stru st_write_msg;
#endif

    /*
     * OAM logģ��Ŀ��ص�����: hipriv "Hisilicon0[vapx] log_level {1/2}"
     * 1-2(error��warning)������־��vap����Ϊά�ȣ�
     */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_vap_log_level::null pointer.}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ��־���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_param, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        return ul_ret;
    }

    en_level_val = (oam_log_level_enum_uint8)oal_atoi(ac_param);
    if ((en_level_val < OAM_LOG_LEVEL_ERROR) || (en_level_val > OAM_LOG_LEVEL_INFO)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_feature_log_level::invalid switch value[%d].}", en_level_val);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    ul_ret = oam_log_set_vap_level(pst_mac_vap->uc_vap_id, en_level_val);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    // Ŀǰ֧��02 device ����log ���� ���������ĺϲ�����
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_LOG_LEVEL, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = en_level_val;
    ul_ret |= (oal_uint32)wal_send_cfg_event(pst_net_dev,
                                             WAL_MSG_TYPE_WRITE,
                                             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                                             (oal_uint8 *)&st_write_msg,
                                             OAL_FALSE,
                                             OAL_PTR_NULL);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_vap_log_level::return err code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }

#endif
    return ul_ret;
}


OAL_STATIC oal_uint32 wal_hipriv_log_ratelimit(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oam_ratelimit_stru st_ratelimit;
    oam_ratelimit_type_enum_uint8 en_ratelimit_type;
    oal_uint32 ul_off_set;
    oal_int8 ac_param[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;

    /*
     * OAM log printk������������: hipriv "Hisilicon0[vapx] {log_ratelimit} {printk(0)/sdt(1)}{switch(0/1)} {interval}
     * {burst}"
     */
    st_ratelimit.en_ratelimit_switch = OAL_SWITCH_OFF;
    st_ratelimit.ul_interval = OAM_RATELIMIT_DEFAULT_INTERVAL;
    st_ratelimit.ul_burst = OAM_RATELIMIT_DEFAULT_BURST;

    /* ��ȡ�������� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_param, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        return ul_ret;
    }
    pc_param += ul_off_set;

    en_ratelimit_type = (oam_ratelimit_type_enum_uint8)oal_atoi(ac_param);

    /* ��ȡ����״̬ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_param, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        return ul_ret;
    }
    pc_param += ul_off_set;

    st_ratelimit.en_ratelimit_switch = (oal_switch_enum_uint8)oal_atoi(ac_param);

    if (st_ratelimit.en_ratelimit_switch == OAL_SWITCH_ON) {
        /* ��ȡintervalֵ */
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_param, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
        if (ul_ret != OAL_SUCC) {
            return ul_ret;
        }
        pc_param += ul_off_set;

        st_ratelimit.ul_interval = (oal_uint32)oal_atoi(ac_param);

        /* ��ȡburstֵ */
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_param, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
        if (ul_ret != OAL_SUCC) {
            return ul_ret;
        }
        pc_param += ul_off_set;

        st_ratelimit.ul_burst = (oal_uint32)oal_atoi(ac_param);
    }

    return oam_log_set_ratelimit_param(en_ratelimit_type, &st_ratelimit);
}


#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32 wal_hipriv_log_lowpower(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_tmp;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32 l_ret;
    oal_uint32 ul_ret;

    /*
     * OAM eventģ��Ŀ��ص�����: hipriv "Hisilicon0 log_pm 0 | 1"
     * �˴���������"1"��"0"����ac_name
     */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_log_lowpower::wal_get_cmd_one_arg return err_code[%d]}", ul_ret);
        return ul_ret;
    }

    /* ��Խ������Ĳ�ͬ�����eventģ����в�ͬ������ */
    if ((oal_strcmp("0", ac_name)) == 0) {
        l_tmp = 0;
    } else if ((oal_strcmp("1", ac_name)) == 0) {
        l_tmp = 1;
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_log_lowpower::the log switch command is error!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_LOG_PM, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp; /* ��������������� */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_event_switch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_pm_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_tmp;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32 l_ret;
    oal_uint32 ul_ret;

    /*
     * OAM eventģ��Ŀ��ص�����: hipriv "Hisilicon0 wal_hipriv_pm_switch 0 | 1"
     * �˴���������"1"��"0"����ac_name
     */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pm_switch::wal_get_cmd_one_arg return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }

    l_tmp = (oal_int32)oal_atoi(ac_name);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_PM_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp; /* ��������������� */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pm_switch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32 wal_hipriv_event_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_tmp;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32 l_ret;
    oal_uint32 ul_ret;

    /*
     * OAM eventģ��Ŀ��ص�����: hipriv "Hisilicon0 event_switch 0 | 1"
     * �˴���������"1"��"0"����ac_name
     */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_event_switch::wal_get_cmd_one_arg return err_code[%d]}", ul_ret);
        return ul_ret;
    }

    /* ��Խ������Ĳ�ͬ�����eventģ����в�ͬ������ */
    if ((oal_strcmp("0", ac_name)) == 0) {
        l_tmp = 0;
    } else if ((oal_strcmp("1", ac_name)) == 0) {
        l_tmp = 1;
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_event_switch::the log switch command is error!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_EVENT_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp; /* ��������������� */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_event_switch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_ether_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32 l_ret;
    oal_uint32 ul_ret;
    mac_cfg_eth_switch_param_stru st_eth_switch_param;

    /* "vap0 ether_switch user_macaddr oam_ota_frame_direction_type_enum(֡����) 0|1(����)" */
    memset_s(&st_eth_switch_param, OAL_SIZEOF(mac_cfg_eth_switch_param_stru), 0,
        OAL_SIZEOF(mac_cfg_eth_switch_param_stru));

    /* ��ȡmac��ַ */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, st_eth_switch_param.auc_user_macaddr, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_ether_switch::wal_hipriv_get_mac_addr return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* ��ȡ��̫��֡���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ether_switch::wal_get_cmd_one_arg return err[%d]}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_eth_switch_param.en_frame_direction = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡ���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ether_switch::wal_get_cmd_one_arg return err[%d]}", ul_ret);
        return ul_ret;
    }
    st_eth_switch_param.en_switch = (oal_uint8)oal_atoi(ac_name);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ETH_SWITCH, OAL_SIZEOF(st_eth_switch_param));

    /* ��������������� */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, (const oal_void *)&st_eth_switch_param,
        OAL_SIZEOF(st_eth_switch_param)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_set_ether_switch::memcpy_s failed!");
        return OAL_FAIL;
    }

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_eth_switch_param),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ether_switch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_80211_ucast_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32 l_ret;
    oal_uint32 ul_ret;
    mac_cfg_80211_ucast_switch_stru st_80211_ucast_switch;

    /*
     * sh hipriv.sh "vap0 80211_uc_switch user_macaddr 0|1(֡����tx|rx) 0|1(֡����:����֡|����֡)
     *                                                0|1(֡���ݿ���) 0|1(CB����) 0|1(����������)"
     */
    memset_s(&st_80211_ucast_switch, OAL_SIZEOF(mac_cfg_80211_ucast_switch_stru), 0,
        OAL_SIZEOF(mac_cfg_80211_ucast_switch_stru));

    /* ��ȡmac��ַ */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, st_80211_ucast_switch.auc_user_macaddr, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_80211_ucast_switch::wal_hipriv_get_mac_addr return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* ��ȡ80211֡���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_80211_ucast_switch::get 80211 ucast frame direction return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_frame_direction = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡ֡���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_80211_ucast_switch::get ucast frame type return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_frame_type = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡ֡���ݴ�ӡ���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_80211_ucast_switch::get ucast frame content switch  return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_frame_switch = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡ֡CB�ֶδ�ӡ���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_80211_ucast_switch::get ucast frame cb switch return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_cb_switch = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡ��������ӡ���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_80211_ucast_switch::get ucast frame dscr switch return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_dscr_switch = (oal_uint8)oal_atoi(ac_name);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_80211_UCAST_SWITCH, OAL_SIZEOF(st_80211_ucast_switch));

    /* ��������������� */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, (const oal_void *)&st_80211_ucast_switch,
        OAL_SIZEOF(st_80211_ucast_switch)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_set_80211_ucast_switch::memcpy_s failed!");
        return OAL_FAIL;
    }

    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_80211_ucast_switch),
                               (oal_uint8 *)&st_write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_ucast_switch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_TXOPPS

OAL_STATIC oal_uint32 wal_hipriv_set_txop_ps_machw(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32 l_ret;
    oal_uint32 ul_ret;
    mac_txopps_machw_param_stru st_txopps_machw_param = { 0 };

    /* sh hipriv.sh "stavap_name txopps_hw_en 0|1(txop_ps_en) 0|1(condition1) 0|1(condition2)" */
    /* ��ȡtxop psʹ�ܿ��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_TXOP,
            "{wal_hipriv_set_txop_ps_machw::get machw txop_ps en return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_txopps_machw_param.en_machw_txopps_en = (oal_switch_enum_uint8)oal_atoi(ac_name);

    /* ��ȡtxop ps condition1ʹ�ܿ��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_TXOP,
            "{wal_hipriv_set_txop_ps_machw::get machw txop_ps condition1 return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_txopps_machw_param.en_machw_txopps_condition1 = (oal_switch_enum_uint8)oal_atoi(ac_name);

    /* ��ȡtxop ps condition2ʹ�ܿ��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_TXOP,
            "{wal_hipriv_set_txop_ps_machw::get machw txop_ps condition2 return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_txopps_machw_param.en_machw_txopps_condition2 = (oal_switch_enum_uint8)oal_atoi(ac_name);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TXOP_PS_MACHW, OAL_SIZEOF(st_txopps_machw_param));

    /* ��������������� */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, (const oal_void *)&st_txopps_machw_param,
        OAL_SIZEOF(st_txopps_machw_param)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_set_txop_ps_machw::memcpy_s failed!");
        return OAL_FAIL;
    }

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_txopps_machw_param),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_TXOP, "{wal_hipriv_set_txop_ps_machw::return err code[%d]!}\r\n", ul_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_BTCOEX

OAL_STATIC oal_uint32 wal_hipriv_btcoex_status_print(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;

    memset_s((oal_uint8 *)&st_write_msg, OAL_SIZEOF(st_write_msg), 0, OAL_SIZEOF(st_write_msg));

    /* sh hipriv.sh "vap_name coex_print" */
    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_BTCOEX_STATUS_PRINT, OAL_SIZEOF(oal_uint32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_TXOP, "{wal_hipriv_btcoex_status_print::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32 wal_hipriv_set_80211_mcast_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32 l_ret;
    oal_uint32 ul_ret;
    mac_cfg_80211_mcast_switch_stru st_80211_mcast_switch = { 0 };

    memset_s((oal_uint8 *)&st_write_msg, OAL_SIZEOF(st_write_msg), 0, OAL_SIZEOF(st_write_msg));
    /*
     * sh hipriv.sh "Hisilicon0 80211_mc_switch 0|1(֡����tx|rx) 0|1(֡����:����֡|����֡)
     *                                         0|1(֡���ݿ���) 0|1(CB����) 0|1(����������)"
     */
    /* ��ȡ80211֡���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_80211_mcast_switch::get 80211 mcast frame direction return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_mcast_switch.en_frame_direction = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡ֡���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_80211_mcast_switch::get mcast frame type return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_mcast_switch.en_frame_type = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡ֡���ݴ�ӡ���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_80211_mcast_switch::get mcast frame content switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_mcast_switch.en_frame_switch = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡ֡CB�ֶδ�ӡ���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_80211_mcast_switch::get mcast frame cb switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_mcast_switch.en_cb_switch = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡ��������ӡ���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_80211_mcast_switch::get mcast frame dscr switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_mcast_switch.en_dscr_switch = (oal_uint8)oal_atoi(ac_name);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_80211_MCAST_SWITCH, OAL_SIZEOF(st_80211_mcast_switch));

    /* ��������������� */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, (const oal_void *)&st_80211_mcast_switch,
        OAL_SIZEOF(st_80211_mcast_switch)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_set_80211_mcast_switch::memcpy_s failed!");
        return OAL_FAIL;
    }

    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_80211_mcast_switch),
                               (oal_uint8 *)&st_write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_mcast_switch::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_all_80211_ucast(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                      ul_ret;
    mac_cfg_80211_ucast_switch_stru ucast_switch;
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint32                      l_memcpy_ret = EOK;

    /*
     * sh hipriv.sh "Hisilicon0 80211_uc_all 0|1(֡����tx|rx) 0|1(֡����:����֡|����֡)
     *                                       0|1(֡���ݿ���) 0|1(CB����) 0|1(����������)"
     */
    memset_s(&ucast_switch, sizeof(mac_cfg_80211_ucast_switch_stru), 0, sizeof(mac_cfg_80211_ucast_switch_stru));

    /* ��ȡ80211֡���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{set_all_80211_ucast::get 80211 ucast frame direction err_code[%d]}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    ucast_switch.en_frame_direction = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡ֡���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_all_80211_ucast::get ucast frame type return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    ucast_switch.en_frame_type = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡ֡���ݴ�ӡ���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_all_80211_ucast::get ucast frame content switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    ucast_switch.en_frame_switch = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡ֡CB�ֶδ�ӡ���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_all_80211_ucast::get ucast frame cb switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    ucast_switch.en_cb_switch = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡ��������ӡ���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_all_80211_ucast::get ucast frame dscr switch return err_code[%d]!}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    ucast_switch.en_dscr_switch = (oal_uint8)oal_atoi(ac_name);

    /* ���ù㲥mac��ַ */
    l_memcpy_ret += memcpy_s(ucast_switch.auc_user_macaddr, WLAN_MAC_ADDR_LEN, BROADCAST_MACADDR, WLAN_MAC_ADDR_LEN);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_80211_UCAST_SWITCH, OAL_SIZEOF(ucast_switch));

    /* ��������������� */
    l_memcpy_ret += memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, &ucast_switch, sizeof(ucast_switch));
    if (l_memcpy_ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_set_80211_all_ucast_switch::memcpy_s failed!");
        return OAL_FAIL;
    }
    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + sizeof(ucast_switch),
                               (oal_uint8 *)&st_write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_all_ucast_switch::return err code [%d]!}", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_all_ether_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_uint8 uc_user_num;
    oal_uint8 uc_frame_direction;
    oal_uint8 uc_switch;

    /* sh hipriv.sh "Hisilicon0 ether_all 0|1(֡����tx|rx) 0|1(����)" */
    /* ��ȡ��̫��֡���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_all_ether_switch::get eth frame direction return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    uc_frame_direction = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡ֡���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_all_ether_switch::get eth type return err_code[%d]!}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    uc_switch = (oal_uint8)oal_atoi(ac_name);

    /* ���ÿ��� */
    for (uc_user_num = 0; uc_user_num < WLAN_ACTIVE_USER_MAX_NUM + WLAN_MAX_MULTI_USER_NUM_SPEC; uc_user_num++) {
        oam_report_eth_frame_set_switch(uc_user_num, uc_switch, uc_frame_direction);
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_dhcp_arp_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint8 uc_switch;

    /* sh hipriv.sh "Hisilicon0 dhcp_arp_switch 0|1(����)" */
    /* ��ȡ֡���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_dhcp_arp_switch::get switch return err_code[%d]!}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    uc_switch = (oal_uint8)oal_atoi(ac_name);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_DHCP_ARP, OAL_SIZEOF(oal_uint32));
    *((oal_int32 *)(st_write_msg.auc_value)) = (oal_uint32)uc_switch;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_dhcp_arp_switch::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32 wal_hipriv_report_vap_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_uint32 ul_flag_value;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;

    /* sh hipriv.sh "wlan0 report_vap_info  flags_value" */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_vap_info::wal_get_cmd_one_arg return err[%d]!}", ul_ret);
        return ul_ret;
    }

    ul_flag_value = (oal_uint32)oal_atoi(ac_name);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_REPORT_VAP_INFO, OAL_SIZEOF(ul_flag_value));

    /* ��д��Ϣ�壬���� */
    *(oal_uint32 *)(st_write_msg.auc_value) = ul_flag_value;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(ul_flag_value),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_vap_info::wal_send_cfg_event return err code [%d]}", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32 wal_hipriv_set_rssi_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    mac_rssi_debug_switch_stru st_rssi_switch;
    oal_int32 l_ret;
    oal_uint32 ul_ret;

    /* sh hipriv.sh "Hisilicon0 rssi_switch  0|1(�ر�|��) N(ÿ��N�����Ĵ�ӡһ��)" */
    memset_s(&st_rssi_switch, OAL_SIZEOF(st_rssi_switch), 0, OAL_SIZEOF(st_rssi_switch));

    /* ��ȡ֡���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rssi_switch::get rssi switch on, return err[%d]}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_rssi_switch.ul_rssi_debug_switch = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡ֡���ݴ�ӡ���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_rssi_switch::get rx comp isr interval switch info, return err_code[%d]!}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_rssi_switch.ul_rx_comp_isr_interval = (oal_uint8)oal_atoi(ac_name);

    OAM_INFO_LOG2(0, OAM_SF_ANY, "{wal_hipriv_set_rssi_switch::rssi_switch: %d, interval: %d.}",
        st_rssi_switch.ul_rssi_debug_switch, st_rssi_switch.ul_rx_comp_isr_interval);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RSSI_SWITCH, OAL_SIZEOF(st_rssi_switch));

    /* ��������������� */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, (const oal_void *)&st_rssi_switch,
        OAL_SIZEOF(st_rssi_switch)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_set_rssi_switch::memcpy_s failed!");
        return OAL_FAIL;
    }

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_rssi_switch),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rssi_switch::return err code[%d]!}", ul_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32 wal_hipriv_aifsn_cfg(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    mac_edca_cfg_stru st_edca_cfg;
    oal_int32 l_ret;
    oal_uint32 ul_ret;

    memset_s(&st_edca_cfg, OAL_SIZEOF(st_edca_cfg), 0, OAL_SIZEOF(st_edca_cfg));

    /* ��ȡ���ÿ��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_aifsn_cfg::get wfa switch fail, return err_code[%d]!}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_edca_cfg.en_switch = (oal_bool_enum_uint8)oal_atoi(ac_name);

    /* ��ȡac */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_aifsn_cfg::get wfa ac fail, return err_code[%d]!}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_edca_cfg.en_ac = (wlan_wme_ac_type_enum_uint8)oal_atoi(ac_name);

    if (st_edca_cfg.en_switch == OAL_TRUE) {
        /* ��ȡ����ֵ */
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_aifsn_cfg::get wfa val fail, return err_code[%d]!}", ul_ret);
            return ul_ret;
        }
        pc_param += ul_off_set;
        st_edca_cfg.us_val = (oal_uint16)oal_atoi(ac_name);
    }
    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_WFA_CFG_AIFSN, OAL_SIZEOF(st_edca_cfg));

    /* ��������������� */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, (const oal_void *)&st_edca_cfg,
        OAL_SIZEOF(st_edca_cfg)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_aifsn_cfg::memcpy_s failed!");
        return OAL_FAIL;
    }

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_edca_cfg),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_aifsn_cfg::return err code[%d]!}", ul_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_cw_cfg(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    mac_edca_cfg_stru st_edca_cfg;
    oal_int32 l_ret;
    oal_uint32 ul_ret;

    memset_s(&st_edca_cfg, OAL_SIZEOF(st_edca_cfg), 0, OAL_SIZEOF(st_edca_cfg));

    /* ��ȡ���ÿ��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_cw_cfg::get wfa switch fail, return err_code[%d]!}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_edca_cfg.en_switch = (oal_bool_enum_uint8)oal_atoi(ac_name);

    /* ��ȡac */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_cw_cfg::get wfa ac fail, return err_code[%d]!}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_edca_cfg.en_ac = (wlan_wme_ac_type_enum_uint8)oal_atoi(ac_name);

    if (st_edca_cfg.en_switch == OAL_TRUE) {
        /* ��ȡ����ֵ */
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_cw_cfg::get wfa val fail, return err_code[%d]!}", ul_ret);
            return ul_ret;
        }
        pc_param += ul_off_set;
        st_edca_cfg.us_val = (oal_uint16)oal_strtol(ac_name, OAL_PTR_NULL, 0);
    }
    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_WFA_CFG_CW, OAL_SIZEOF(st_edca_cfg));

    /* ��������������� */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, (const oal_void *)&st_edca_cfg,
        OAL_SIZEOF(st_edca_cfg)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_aifsn_cfg::memcpy_s failed!");
        return OAL_FAIL;
    }

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_edca_cfg),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_aifsn_cfg::return err code[%d]!}", ul_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32 wal_hipriv_set_probe_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32 l_ret;
    oal_uint32 ul_ret;
    mac_cfg_probe_switch_stru st_probe_switch;

    /*
     * sh hipriv.sh "Hisilicon0 probe_switch 0|1(֡����tx|rx) 0|1(֡���ݿ���)
     *                                       0|1(CB����) 0|1(����������)"
     */
    /* ��ȡ֡���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_probe_switch::get probe direction return err[%d]}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_probe_switch.en_frame_direction = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡ֡���ݴ�ӡ���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_probe_switch::get probe frame content switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_probe_switch.en_frame_switch = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡ֡CB�ֶδ�ӡ���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_probe_switch::get probe frame cb switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_probe_switch.en_cb_switch = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡ��������ӡ���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_probe_switch::get probe frame dscr switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_probe_switch.en_dscr_switch = (oal_uint8)oal_atoi(ac_name);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PROBE_SWITCH, OAL_SIZEOF(st_probe_switch));

    /* ��������������� */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, (const oal_void *)&st_probe_switch,
        OAL_SIZEOF(st_probe_switch)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_set_80211_ucast_switch::memcpy_s failed!");
        return OAL_FAIL;
    }

    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_probe_switch),
                               (oal_uint8 *)&st_write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_ucast_switch::return err code[%d]!}\r\n", ul_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_get_mpdu_num(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int32 l_ret;
    oal_uint32 ul_ret;
    mac_cfg_get_mpdu_num_stru st_param;

    /* sh hipriv.sh "vap_name mpdu_num user_macaddr" */
    memset_s(&st_param, OAL_SIZEOF(mac_cfg_get_mpdu_num_stru), 0, OAL_SIZEOF(mac_cfg_get_mpdu_num_stru));

    /* ��ȡ�û�mac��ַ */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, st_param.auc_user_macaddr, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        return ul_ret;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_MPDU_NUM, OAL_SIZEOF(st_param));

    /* ��������������� */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, &st_param, OAL_SIZEOF(st_param)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_get_mpdu_num::memcpy_s failed!");
        return OAL_FAIL;
    }
    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_param),
        (oal_uint8 *)&st_write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_all_ota(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32 l_param;
    wal_msg_write_stru st_write_msg;

    /* ��ȡ���� sh hipriv.sh "Hisilicon0 set_all_ota 0|1" */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        return ul_ret;
    }

    l_param = oal_atoi((const oal_int8 *)ac_name);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_ALL_OTA, OAL_SIZEOF(oal_uint32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_param;

    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
        (oal_uint8 *)&st_write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_all_ota::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_RF_110X_CALI_DPD
OAL_STATIC oal_uint32 wal_hipriv_start_dpd(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_START_DPD, OAL_SIZEOF(wal_specific_event_type_param_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(wal_specific_event_type_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif
#ifdef _PRE_WLAN_CHIP_TEST

OAL_STATIC oal_uint32 wal_hipriv_beacon_offload_test(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    wal_msg_write_stru st_write_msg;
    oal_uint8 *uc_param = OAL_PTR_NULL;
    oal_uint32 ul_off_set = 0;
    oal_int32 l_ret;
    oal_uint32 ul_ret;
    oal_uint8 i;

    uc_param = (oal_uint8 *)st_write_msg.auc_value;

    /* hipriv "Hisilicon0 beacon_offload_test param0 param1 param2 param3", */
    for (i = 0; i < 4; i++) {
        pc_param += ul_off_set;
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_beacon_offload_test::wal_get_cmd_one_arg param[%d] err!}", i);
            return ul_ret;
        }
        *uc_param = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);
        uc_param++;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_BEACON_OFFLOAD_TEST,
        OAL_SIZEOF(wal_specific_event_type_param_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32 wal_hipriv_ota_beacon_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_param;
    oal_uint32 ul_off_set = 0;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32 l_ret;
    oal_uint32 ul_ret;

    /* OAM otaģ��Ŀ��ص�����: hipriv "Hisilicon0 ota_beacon_switch 0 | 1"
     */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ota_beacon_switch::wal_get_cmd_one_arg fails!}\r\n");
        return ul_ret;
    }
    l_param = oal_atoi((const oal_int8 *)ac_name);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_OTA_BEACON_SWITCH, OAL_SIZEOF(oal_uint32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_param;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ota_beacon_switch::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_ota_rx_dscr_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_param;
    oal_uint32 ul_off_set = 0;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32 l_ret;
    oal_uint32 ul_ret;

    /* OAM otaģ��Ŀ��ص�����: hipriv "Hisilicon0 ota_rx_dscr_switch 0 | 1" */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ota_rx_dscr_switch::wal_get_cmd_one_arg fails!}\r\n");
        return ul_ret;
    }

    l_param = oal_atoi((const oal_int8 *)ac_name);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_OTA_RX_DSCR_SWITCH, OAL_SIZEOF(oal_uint32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_param;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ota_rx_dscr_switch::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_DEBUG_MODE


OAL_STATIC oal_uint32 wal_hipriv_get_tx_comp_cnt(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32 ul_ret;
    oal_uint32 ul_off_set = 0;
    oal_uint8 uc_stat_flag;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (pst_mac_vap == NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_get_tx_comp_cnt::pst_mac_vap is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*
     * ͳ�Ʒ�������ж��Ƿ�ʧ(�رվۺ�) sh hipriv.sh "Hisilicon0 tx_comp_cnt 0|1", 0��ʾ����ͳ�ƴ�����
     * 1��ʾ��ʾͳ�ƴ�����������",
     */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_get_tx_comp_cnt::wal_get_cmd_one_arg fails!}\r\n");
        return ul_ret;
    }
    uc_stat_flag = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);
    if (uc_stat_flag == 0) {
        g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_data_num = 0;
        g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_mgnt_num = 0;
        g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_total_num = 0;
        g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_uh1_num = 0;
        g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_uh2_num = 0;
        g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_bh1_num = 0;
        g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_bh2_num = 0;
        g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_bh3_num = 0;
    } else {
        OAL_IO_PRINT("ul_tx_data_num = %d\n", g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_data_num);
        OAL_IO_PRINT("ul_tx_mgnt_num = %d\n", g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_mgnt_num);
        OAL_IO_PRINT("ul_tx_complete_total_num = %d\n",
            g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_total_num);
        OAL_IO_PRINT("ul_tx_complete_uh1_num = %d\n",
            g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_uh1_num);
        OAL_IO_PRINT("ul_tx_complete_uh2_num = %d\n",
            g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_uh2_num);
        OAL_IO_PRINT("ul_tx_complete_bh1_num = %d\n",
            g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_bh1_num);
        OAL_IO_PRINT("ul_tx_complete_bh2_num = %d\n",
            g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_bh2_num);
        OAL_IO_PRINT("ul_tx_complete_bh3_num = %d\n",
            g_ast_tx_complete_stat[pst_mac_vap->uc_device_id].ul_tx_complete_bh3_num);
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_debug_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32 ul_off_set = 0;
    oal_uint8 uc_debug_type;
    oal_int8 ac_debug_type[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_uint8 uc_idx;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_debug_type, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAL_IO_PRINT("Error:wal_hipriv_set_debug_switch[%d] wal_get_cmd_one_arg return error code[%d]! \n", __LINE__,
            ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    if (ac_debug_type[0] == '?') {
        OAL_IO_PRINT("debug_switch <debug_type> <switch_state> \n");
        OAL_IO_PRINT("               0      0/1     -- when set register echo read value \n");
        return OAL_SUCC;
    }

    uc_debug_type = (oal_uint8)oal_atoi((const oal_int8 *)ac_debug_type);
    if (uc_debug_type >= MAX_DEBUG_TYPE_NUM) {
        OAL_IO_PRINT("Info: <debug_type> should be less than %d. \n", MAX_DEBUG_TYPE_NUM);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_debug_type, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAL_IO_PRINT("Error:wal_hipriv_set_debug_switch[%d] wal_get_cmd_one_arg return error code[%d]! \n", __LINE__,
            ul_ret);
        return ul_ret;
    }

    g_aul_debug_feature_switch[uc_debug_type] = (oal_uint32)oal_atoi((const oal_int8 *)ac_debug_type);
    if ((g_aul_debug_feature_switch[uc_debug_type] != OAL_SWITCH_ON) &&
        (g_aul_debug_feature_switch[uc_debug_type] != OAL_SWITCH_OFF)) {
        OAL_IO_PRINT("Error:switch_value must be 0 or 1. \n");
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    OAL_IO_PRINT("<debug_type>   <switch_value> \n");
    for (uc_idx = 0; uc_idx < MAX_DEBUG_TYPE_NUM; uc_idx++) {
        OAL_IO_PRINT("  %d          %d \n", uc_idx, g_aul_debug_feature_switch[uc_idx]);
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_DAQ

OAL_STATIC oal_uint32 wal_hipriv_data_acq(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint16 us_len;

    if (OAL_UNLIKELY(OAL_STRLEN(pc_param) >= WAL_MSG_WRITE_MAX_LEN)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_data_acq:: pc_param overlength is %d}\n", OAL_STRLEN(pc_param));
        return OAL_FAIL;
    }

    /* ���¼���wal�㴦�� */
    while (*pc_param == ' ') {
        ++pc_param;
    }
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, pc_param, OAL_STRLEN(pc_param)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_data_acq::memcpy_s failed!");
        return OAL_FAIL;
    }

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DATA_ACQ, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_data_acq::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_FEATURE_UAPSD

OAL_STATIC oal_uint32 wal_hipriv_set_uapsd_cap(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_tmp;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32 l_ret;
    oal_uint32 ul_ret;

    /* �˴���������"1"��"0"����ac_name */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_uapsd_cap::wal_get_cmd_one_arg return err_code[%d]}", ul_ret);
        return ul_ret;
    }

    /* ��Խ������Ĳ�ͬ�����UAPSD���ؽ��в�ͬ������ */
    if ((oal_strcmp("0", ac_name)) == 0) {
        l_tmp = 0;
    } else if ((oal_strcmp("1", ac_name)) == 0) {
        l_tmp = 1;
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_uapsd_cap::the log switch command is error!}\r\n")
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_UAPSD_EN, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp; /* ��������������� */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_event_switch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_SMPS
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32 wal_hipriv_get_smps_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint16 us_len;

    /* ���¼���wal�㴦�� */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, pc_param, OAL_STRLEN(pc_param)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_smps_info::memcpy_s failed!");
        return OAL_FAIL;
    }

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_SMPS_INFO, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_smps_info::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif
#endif


OAL_STATIC oal_uint32 wal_hipriv_oam_output(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_tmp;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;

    /* OAM logģ��Ŀ��ص�����: hipriv "Hisilicon0 log_level 0~3" �˴���������"1"��"0"����ac_name */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_oam_output::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    /* ��Խ������Ĳ�ͬ�����logģ����в�ͬ������ ȡֵ:oam_output_type_enum_uint8 */
    l_tmp = oal_atoi(ac_name);
    if (l_tmp >= OAM_OUTPUT_TYPE_BUTT) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_oam_output::output type invalid [%d]!}\r\n", l_tmp);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_OAM_OUTPUT_TYPE, OAL_SIZEOF(oal_int32));
    /* ��������������� */
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_oam_output::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_add_user(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_add_user_param_stru *pst_add_user_param = OAL_PTR_NULL;
    mac_cfg_add_user_param_stru st_add_user_param; /* ��ʱ�����ȡ��use����Ϣ */
    oal_uint32 ul_get_addr_idx;

    /* ��������û�����������: hipriv "vap0 add_user xx xx xx xx xx xx(mac��ַ) 0 | 1(HT����λ) " ���������ĳһ��VAP */
    /* ��ȡmac��ַ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_add_user::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    memset_s((oal_uint8 *)&st_add_user_param, OAL_SIZEOF(mac_cfg_add_user_param_stru), 0,
        OAL_SIZEOF(mac_cfg_add_user_param_stru));
    oal_strtoaddr(ac_name, sizeof(ac_name), st_add_user_param.auc_mac_addr, WLAN_MAC_ADDR_LEN);
    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    /* ��ȡ�û���HT��ʶ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_add_user::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* ��Խ������Ĳ�ͬ�����user��HT�ֶν��в�ͬ������ */
    if ((oal_strcmp("0", ac_name)) == 0) {
        st_add_user_param.en_ht_cap = 0;
    } else if ((oal_strcmp("1", ac_name)) == 0) {
        st_add_user_param.en_ht_cap = 1;
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_add_user::the mod switch command is error!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADD_USER, OAL_SIZEOF(mac_cfg_add_user_param_stru));

    /* ��������������� */
    pst_add_user_param = (mac_cfg_add_user_param_stru *)(st_write_msg.auc_value);
    for (ul_get_addr_idx = 0; ul_get_addr_idx < WLAN_MAC_ADDR_LEN; ul_get_addr_idx++) {
        pst_add_user_param->auc_mac_addr[ul_get_addr_idx] = st_add_user_param.auc_mac_addr[ul_get_addr_idx];
    }
    pst_add_user_param->en_ht_cap = st_add_user_param.en_ht_cap;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_add_user_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_add_user::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_del_user(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_del_user_param_stru *pst_del_user_param = OAL_PTR_NULL;
    mac_cfg_del_user_param_stru st_del_user_param; /* ��ʱ�����ȡ��use����Ϣ */
    oal_uint32 ul_get_addr_idx;

    /* ����ɾ���û�����������: hipriv "vap0 del_user xx xx xx xx xx xx(mac��ַ)" ���������ĳһ��VAP */
    /* ��ȡmac��ַ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_del_user::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    memset_s((oal_uint8 *)&st_del_user_param, OAL_SIZEOF(mac_cfg_del_user_param_stru), 0,
        OAL_SIZEOF(mac_cfg_del_user_param_stru));
    oal_strtoaddr(ac_name, sizeof(ac_name), st_del_user_param.auc_mac_addr, WLAN_MAC_ADDR_LEN);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DEL_USER, OAL_SIZEOF(mac_cfg_add_user_param_stru));

    /* ��������������� */
    pst_del_user_param = (mac_cfg_add_user_param_stru *)(st_write_msg.auc_value);
    for (ul_get_addr_idx = 0; ul_get_addr_idx < WLAN_MAC_ADDR_LEN; ul_get_addr_idx++) {
        pst_del_user_param->auc_mac_addr[ul_get_addr_idx] = st_del_user_param.auc_mac_addr[ul_get_addr_idx];
    }

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_add_user_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_del_user::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_ampdu_start(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_ampdu_start_param_stru *pst_ampdu_start_param = OAL_PTR_NULL;
    mac_cfg_ampdu_start_param_stru st_ampdu_start_param; /* ��ʱ�����ȡ��use����Ϣ */
    oal_uint32 ul_get_addr_idx;

    /* ����AMPDU��������������: hipriv "Hisilicon0  ampdu_start xx xx xx xx xx xx(mac��ַ) tidno ack_policy" */
    /* ��ȡmac��ַ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_start::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    memset_s((oal_uint8 *)&st_ampdu_start_param, OAL_SIZEOF(mac_cfg_ampdu_start_param_stru), 0,
        OAL_SIZEOF(mac_cfg_ampdu_start_param_stru));
    oal_strtoaddr(ac_name, sizeof(ac_name), st_ampdu_start_param.auc_mac_addr, WLAN_MAC_ADDR_LEN);

    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    /* ��ȡtid */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_start::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    if (OAL_STRLEN(ac_name) > 2) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ampdu_start::the ampdu start command is erro!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    st_ampdu_start_param.uc_tidno = (oal_uint8)oal_atoi(ac_name);
    if (st_ampdu_start_param.uc_tidno >= WLAN_TID_MAX_NUM) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_ampdu_start::the ampdu start command is error! uc_tidno is  [%d]!}",
            st_ampdu_start_param.uc_tidno);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_AMPDU_START, OAL_SIZEOF(mac_cfg_ampdu_start_param_stru));

    /* ��������������� */
    pst_ampdu_start_param = (mac_cfg_ampdu_start_param_stru *)(st_write_msg.auc_value);
    for (ul_get_addr_idx = 0; ul_get_addr_idx < WLAN_MAC_ADDR_LEN; ul_get_addr_idx++) {
        pst_ampdu_start_param->auc_mac_addr[ul_get_addr_idx] = st_ampdu_start_param.auc_mac_addr[ul_get_addr_idx];
    }

    pst_ampdu_start_param->uc_tidno = st_ampdu_start_param.uc_tidno;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ampdu_start_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_start::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_ampdu_amsdu_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_tmp;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32 l_ret;
    oal_uint32 ul_ret;

    /* �����Զ���ʼBA�Ự�Ŀ���:hipriv "vap0  auto_ba 0 | 1" ���������ĳһ��VAP */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_ampdu_amsdu_switch::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* ��Խ������Ĳ�ͬ�����AUTO BA���в�ͬ������ */
    if ((oal_strcmp("0", ac_name)) == 0) {
        l_tmp = 0;
    } else if ((oal_strcmp("1", ac_name)) == 0) {
        l_tmp = 1;
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ampdu_amsdu_switch::the auto ba switch command is error!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_AMSDU_AMPDU_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp; /* ��������������� */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_amsdu_switch::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_auto_ba_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_tmp;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32 l_ret;
    oal_uint32 ul_ret;

    /* �����Զ���ʼBA�Ự�Ŀ���:hipriv "vap0  auto_ba 0 | 1" ���������ĳһ��VAP */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_auto_ba_switch::wal_get_cmd_one_arg return err [%d]!}", ul_ret);
        return ul_ret;
    }

    /* ��Խ������Ĳ�ͬ�����AUTO BA���в�ͬ������ */
    if ((oal_strcmp("0", ac_name)) == 0) {
        l_tmp = 0;
    } else if ((oal_strcmp("1", ac_name)) == 0) {
        l_tmp = 1;
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_auto_ba_switch::the auto ba switch command is error!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_AUTO_BA_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp; /* ��������������� */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_auto_ba_switch::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_addba_req(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_addba_req_param_stru *pst_addba_req_param = OAL_PTR_NULL;
    mac_cfg_addba_req_param_stru st_addba_req_param; /* ��ʱ�����ȡ��addba req����Ϣ */
    oal_uint32 ul_get_addr_idx;

    /*
     * ����AMPDU�رյ���������:
     * hipriv "Hisilicon0 addba_req xx xx xx xx xx xx(mac��ַ) tidno ba_policy buffsize timeout"
     */
    /* ��ȡmac��ַ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    memset_s((oal_uint8 *)&st_addba_req_param, OAL_SIZEOF(mac_cfg_addba_req_param_stru), 0,
        OAL_SIZEOF(mac_cfg_addba_req_param_stru));
    oal_strtoaddr(ac_name, sizeof(ac_name), st_addba_req_param.auc_mac_addr, WLAN_MAC_ADDR_LEN);

    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    /* ��ȡtid */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    if (OAL_STRLEN(ac_name) > 2) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_addba_req::the addba req command is error!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    st_addba_req_param.uc_tidno = (oal_uint8)oal_atoi(ac_name);
    if (st_addba_req_param.uc_tidno >= WLAN_TID_MAX_NUM) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::the addba req command is error!uc_tidno is [%d]!}\r\n",
            st_addba_req_param.uc_tidno);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pc_param = pc_param + ul_off_set;

    /* ��ȡba_policy */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    st_addba_req_param.en_ba_policy = (oal_uint8)oal_atoi(ac_name);
    if (st_addba_req_param.en_ba_policy != MAC_BA_POLICY_IMMEDIATE) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::the ba policy is not correct! ba_policy is[%d]!}\r\n",
            st_addba_req_param.en_ba_policy);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pc_param = pc_param + ul_off_set;

    /* ��ȡbuffsize */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    st_addba_req_param.us_buff_size = (oal_uint16)oal_atoi(ac_name);

    pc_param = pc_param + ul_off_set;

    /* ��ȡtimeoutʱ�� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    st_addba_req_param.us_timeout = (oal_uint16)oal_atoi(ac_name);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADDBA_REQ, OAL_SIZEOF(mac_cfg_addba_req_param_stru));

    /* ��������������� */
    pst_addba_req_param = (mac_cfg_addba_req_param_stru *)(st_write_msg.auc_value);
    for (ul_get_addr_idx = 0; ul_get_addr_idx < WLAN_MAC_ADDR_LEN; ul_get_addr_idx++) {
        pst_addba_req_param->auc_mac_addr[ul_get_addr_idx] = st_addba_req_param.auc_mac_addr[ul_get_addr_idx];
    }

    pst_addba_req_param->uc_tidno = st_addba_req_param.uc_tidno;
    pst_addba_req_param->en_ba_policy = st_addba_req_param.en_ba_policy;
    pst_addba_req_param->us_buff_size = st_addba_req_param.us_buff_size;
    pst_addba_req_param->us_timeout = st_addba_req_param.us_timeout;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_addba_req_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_delba_req(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_delba_req_param_stru *pst_delba_req_param = OAL_PTR_NULL;
    mac_cfg_delba_req_param_stru st_delba_req_param; /* ��ʱ�����ȡ��addba req����Ϣ */
    oal_uint32 ul_get_addr_idx;

    /*
     * ����AMPDU�رյ���������:
     * hipriv "Hisilicon0 delba_req xx xx xx xx xx xx(mac��ַ) tidno direction reason_code"
     */
    /* ��ȡmac��ַ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delba_req::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    memset_s((oal_uint8 *)&st_delba_req_param, OAL_SIZEOF(mac_cfg_delba_req_param_stru), 0,
        OAL_SIZEOF(mac_cfg_delba_req_param_stru));
    oal_strtoaddr(ac_name, sizeof(ac_name), st_delba_req_param.auc_mac_addr, WLAN_MAC_ADDR_LEN);

    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    /* ��ȡtid */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delba_req::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    if (OAL_STRLEN(ac_name) > 2) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_delba_req::the delba_req req command is error!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    st_delba_req_param.uc_tidno = (oal_uint8)oal_atoi(ac_name);
    if (st_delba_req_param.uc_tidno >= WLAN_TID_MAX_NUM) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_delba_req::the delba_req req command is error! uc_tidno is[%d]!}\r\n",
            st_delba_req_param.uc_tidno);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pc_param = pc_param + ul_off_set;

    /* ��ȡdirection */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delba_req::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    st_delba_req_param.en_direction = (oal_uint8)oal_atoi(ac_name);
    if (st_delba_req_param.en_direction >= MAC_BUTT_DELBA) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delba_req::the direction is not correct! direction is[%d]!}\r\n",
            st_delba_req_param.en_direction);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DELBA_REQ, OAL_SIZEOF(mac_cfg_delba_req_param_stru));

    /* ��������������� */
    pst_delba_req_param = (mac_cfg_delba_req_param_stru *)(st_write_msg.auc_value);
    for (ul_get_addr_idx = 0; ul_get_addr_idx < WLAN_MAC_ADDR_LEN; ul_get_addr_idx++) {
        pst_delba_req_param->auc_mac_addr[ul_get_addr_idx] = st_delba_req_param.auc_mac_addr[ul_get_addr_idx];
    }

    pst_delba_req_param->uc_tidno = st_delba_req_param.uc_tidno;
    pst_delba_req_param->en_direction = st_delba_req_param.en_direction;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_delba_req_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delba_req::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_user_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    mac_vap_stru                 *pst_mac_vap = OAL_PTR_NULL;
    wal_msg_write_stru            st_write_msg;
    oal_int32                     l_ret;
    mac_cfg_user_info_param_stru *pst_user_info_param = OAL_PTR_NULL;
    oal_uint8                     auc_mac_addr[6] = { 0 };    /* ��ʱ�����ȡ��use��mac��ַ��Ϣ */
    oal_uint8                     uc_char_index;
    oal_uint16                    us_user_idx;

    /* ȥ���ַ����Ŀո� */
    pc_param++;

    /* ��ȡmac��ַ,16����ת�� */
    for (uc_char_index = 0; uc_char_index < 12; uc_char_index++) {
        if (*pc_param == ':') {
            pc_param++;
            if (uc_char_index != 0) {
                uc_char_index--;
            }

            continue;
        }

        auc_mac_addr[uc_char_index / 2] =
            (oal_uint8)(auc_mac_addr[uc_char_index / 2] * 16 * (uc_char_index % 2) + oal_strtohex(pc_param));
        pc_param++;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_USER_INFO, OAL_SIZEOF(mac_cfg_user_info_param_stru));

    /* ����mac��ַ���û� */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);

    l_ret = (oal_int32)mac_vap_find_user_by_macaddr(pst_mac_vap, auc_mac_addr, &us_user_idx);
    if (l_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_user_info::no such user!}\r\n");
        return OAL_FAIL;
    }

    /* ��������������� */
    pst_user_info_param = (mac_cfg_user_info_param_stru *)(st_write_msg.auc_value);
    pst_user_info_param->us_user_idx = us_user_idx;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_user_info_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_hipriv_user_info::return err code [%d]!}", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_ucast_data_dscr_param(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_set_dscr_param_stru *pst_set_dscr_param = OAL_PTR_NULL;
    wal_dscr_param_enum_uint8 en_param_index;
    oal_int8 ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_DSCR, OAL_SIZEOF(mac_cfg_set_dscr_param_stru));

    /* ��������������������� */
    pst_set_dscr_param = (mac_cfg_set_dscr_param_stru *)(st_write_msg.auc_value);

    /* ��ȡ�������ֶ����������ַ��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_ucast_data_dscr_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* ������������һ���ֶ� */
    for (en_param_index = 0; en_param_index < WAL_DSCR_PARAM_BUTT; en_param_index++) {
        if (!oal_strcmp(pauc_tx_dscr_param_name[en_param_index], ac_arg)) {
            break;
        }
    }

    /* ��������Ƿ��� */
    if (en_param_index == WAL_DSCR_PARAM_BUTT) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_data_dscr_param::no such param for tx dscr!}\r\n");
        return OAL_FAIL;
    }

    pst_set_dscr_param->uc_function_index = en_param_index;

    /* ����Ҫ����Ϊ����ֵ */
    pst_set_dscr_param->l_value = oal_strtol(pc_param, OAL_PTR_NULL, 0);

    /* ��������֡���������� tpye = MAC_VAP_CONFIG_UCAST_DATA */
    pst_set_dscr_param->en_type = MAC_VAP_CONFIG_UCAST_DATA;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_dscr_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_data_dscr_param::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_WEB_CFG_FIXED_RATE

OAL_STATIC oal_uint32 wal_hipriv_set_mode_ucast_data_dscr_param(oal_net_device_stru *pst_net_dev,
    const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_set_dscr_param_stru *pst_set_dscr_param = OAL_PTR_NULL;
    wal_dscr_param_enum_uint8 en_param_index;
    oal_int8 ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_DSCR, OAL_SIZEOF(mac_cfg_set_dscr_param_stru));

    /* ��������������������� */
    pst_set_dscr_param = (mac_cfg_set_dscr_param_stru *)(st_write_msg.auc_value);

    /* ��ȡ�������ֶ����������ַ��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_mode_ucast_data_dscr_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* �������õ�Э��ģʽ */
    if (!oal_strcmp("11ac", ac_arg)) {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_VHT_UCAST_DATA;
    } else if (!oal_strcmp("11n", ac_arg)) {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_HT_UCAST_DATA;
    } else if (!oal_strcmp("11ag", ac_arg)) {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_11AG_UCAST_DATA;
    } else if (!oal_strcmp("11b", ac_arg)) {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_11B_UCAST_DATA;
    } else {
        OAM_ERROR_LOG0(0, OAM_SF_ANY,
            "{wal_hipriv_set_mode_ucast_data_dscr_param:: no such param for protocol_mode!}\r\n");
        return OAL_FAIL;
    }

    /* ��ȡ�������ֶ����������ַ��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_mode_ucast_data_dscr_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* ������������һ���ֶ� */
    for (en_param_index = 0; en_param_index < WAL_DSCR_PARAM_BUTT; en_param_index++) {
        if (!oal_strcmp(pauc_tx_dscr_param_name[en_param_index], ac_arg)) {
            break;
        }
    }

    /* ��������Ƿ��� */
    if (en_param_index == WAL_DSCR_PARAM_BUTT) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_mode_ucast_data_dscr_param::no such param for tx dscr!}\r\n");
        return OAL_FAIL;
    }

    pst_set_dscr_param->uc_function_index = en_param_index;

    /* ����Ҫ����Ϊ����ֵ */
    pst_set_dscr_param->l_value = oal_strtol(pc_param, OAL_PTR_NULL, 0);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_dscr_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mode_ucast_data_dscr_param::return err code [%d]!}", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32 wal_hipriv_set_mcast_data_dscr_param(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_set_dscr_param_stru *pst_set_dscr_param = OAL_PTR_NULL;
    wal_dscr_param_enum_uint8 en_param_index;
    oal_int8 ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_DSCR, OAL_SIZEOF(mac_cfg_set_dscr_param_stru));

    /* ��������������������� */
    pst_set_dscr_param = (mac_cfg_set_dscr_param_stru *)(st_write_msg.auc_value);

    /* ��ȡ�������ֶ����������ַ��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_mcast_data_dscr_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* ������������һ���ֶ� */
    for (en_param_index = 0; en_param_index < WAL_DSCR_PARAM_BUTT; en_param_index++) {
        if (!oal_strcmp(pauc_tx_dscr_param_name[en_param_index], ac_arg)) {
            break;
        }
    }

    /* ��������Ƿ��� */
    if (en_param_index == WAL_DSCR_PARAM_BUTT) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_mcast_data_dscr_param::no such param for tx dscr!}\r\n");
        return OAL_FAIL;
    }

    pst_set_dscr_param->uc_function_index = en_param_index;

    /* ����Ҫ����Ϊ����ֵ */
    pst_set_dscr_param->l_value = oal_strtol(pc_param, OAL_PTR_NULL, 0);

    /* �鲥����֡���������� tpye = MAC_VAP_CONFIG_MCAST_DATA */
    pst_set_dscr_param->en_type = MAC_VAP_CONFIG_MCAST_DATA;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_dscr_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mcast_data_dscr_param::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_bcast_data_dscr_param(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_set_dscr_param_stru *pst_set_dscr_param = OAL_PTR_NULL;
    wal_dscr_param_enum_uint8 en_param_index;
    oal_int8 ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_DSCR, OAL_SIZEOF(mac_cfg_set_dscr_param_stru));

    /* ��������������������� */
    pst_set_dscr_param = (mac_cfg_set_dscr_param_stru *)(st_write_msg.auc_value);

    /* ��ȡ�������ֶ����������ַ��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_bcast_data_dscr_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* ������������һ���ֶ� */
    for (en_param_index = 0; en_param_index < WAL_DSCR_PARAM_BUTT; en_param_index++) {
        if (!oal_strcmp(pauc_tx_dscr_param_name[en_param_index], ac_arg)) {
            break;
        }
    }

    /* ��������Ƿ��� */
    if (en_param_index == WAL_DSCR_PARAM_BUTT) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_bcast_data_dscr_param::no such param for tx dscr!}\r\n");
        return OAL_FAIL;
    }

    pst_set_dscr_param->uc_function_index = en_param_index;

    /* ����Ҫ����Ϊ����ֵ */
    pst_set_dscr_param->l_value = oal_strtol(pc_param, OAL_PTR_NULL, 0);

    /* �㲥����֡���������� tpye = MAC_VAP_CONFIG_BCAST_DATA */
    pst_set_dscr_param->en_type = MAC_VAP_CONFIG_BCAST_DATA;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_dscr_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_bcast_data_dscr_param::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_ucast_mgmt_dscr_param(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_set_dscr_param_stru *pst_set_dscr_param = OAL_PTR_NULL;
    wal_dscr_param_enum_uint8 en_param_index;
    oal_int8 ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint8 uc_band;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_DSCR, OAL_SIZEOF(mac_cfg_set_dscr_param_stru));

    /* ��������������������� */
    pst_set_dscr_param = (mac_cfg_set_dscr_param_stru *)(st_write_msg.auc_value);

    /* sh hipriv.sh "vap0 set_ucast_mgmt data0 2 8389137" */
    /* ����data0 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_ucast_mgmt_dscr_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* ������������һ���ֶ� */
    for (en_param_index = 0; en_param_index < WAL_DSCR_PARAM_BUTT; en_param_index++) {
        if (!oal_strcmp(pauc_tx_dscr_param_name[en_param_index], ac_arg)) {
            break;
        }
    }

    /* ��������Ƿ��� */
    if (en_param_index == WAL_DSCR_PARAM_BUTT) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_mgmt_dscr_param::no such param for tx dscr!}\r\n");
        return OAL_FAIL;
    }

    pst_set_dscr_param->uc_function_index = en_param_index;

    /* ����Ҫ����Ϊ�ĸ�Ƶ�εĵ�������֡ 2G or 5G */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_ucast_mgmt_dscr_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    uc_band = (oal_uint8)oal_atoi(ac_arg);
    /* ��������֡���������� tpye = MAC_VAP_CONFIG_UCAST_MGMT 2Ϊ2G,����Ϊ5G  */
    if (uc_band == WLAN_BAND_2G) {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_UCAST_MGMT_2G;
    } else {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_UCAST_MGMT_5G;
    }

    /* ����Ҫ����Ϊ�������� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_ucast_mgmt_dscr_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_set_dscr_param->l_value = oal_strtol(ac_arg, OAL_PTR_NULL, 0);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_dscr_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_mgmt_dscr_param::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_mbcast_mgmt_dscr_param(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_set_dscr_param_stru *pst_set_dscr_param = OAL_PTR_NULL;
    wal_dscr_param_enum_uint8 en_param_index;
    oal_int8 ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint8 uc_band;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_DSCR, OAL_SIZEOF(mac_cfg_set_dscr_param_stru));

    /* ��������������������� */
    pst_set_dscr_param = (mac_cfg_set_dscr_param_stru *)(st_write_msg.auc_value);

    /* sh hipriv.sh "vap0 set_mcast_mgmt data0 5 8389137" */
    /* ����data0 */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_mbcast_mgmt_dscr_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* ������������һ���ֶ� */
    for (en_param_index = 0; en_param_index < WAL_DSCR_PARAM_BUTT; en_param_index++) {
        if (!oal_strcmp(pauc_tx_dscr_param_name[en_param_index], ac_arg)) {
            break;
        }
    }

    /* ��������Ƿ��� */
    if (en_param_index == WAL_DSCR_PARAM_BUTT) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_mbcast_mgmt_dscr_param::no such param for tx dscr!}\r\n");
        return OAL_FAIL;
    }

    pst_set_dscr_param->uc_function_index = en_param_index;

    /* ����Ҫ����Ϊ�ĸ�Ƶ�εĵ�������֡ 2G or 5G */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_mbcast_mgmt_dscr_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    uc_band = (oal_uint8)oal_atoi(ac_arg);
    /* ��������֡���������� tpye = MAC_VAP_CONFIG_UCAST_MGMT 2Ϊ2G,����Ϊ5G  */
    if (uc_band == WLAN_BAND_2G) {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_MBCAST_MGMT_2G;
    } else {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_MBCAST_MGMT_5G;
    }

    /* ����Ҫ����Ϊ�������� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_mbcast_mgmt_dscr_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_set_dscr_param->l_value = oal_strtol(ac_arg, OAL_PTR_NULL, 0);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_dscr_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mbcast_mgmt_dscr_param::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


oal_uint32 wal_hipriv_set_rate(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_non_ht_rate_stru *pst_set_rate_param = OAL_PTR_NULL;
    wlan_legacy_rate_value_enum_uint8 en_rate_index;
    oal_int8 ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_RATE, OAL_SIZEOF(mac_cfg_non_ht_rate_stru));

    /* ��������������������� */
    pst_set_rate_param = (mac_cfg_non_ht_rate_stru *)(st_write_msg.auc_value);

    /* ��ȡ����ֵ�ַ��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rate::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* ����������Ϊ��һ������ */
    for (en_rate_index = 0; en_rate_index < WLAN_LEGACY_RATE_VALUE_BUTT; en_rate_index++) {
        if (!oal_strcmp(pauc_non_ht_rate_tbl[en_rate_index], ac_arg)) {
            break;
        }
    }

    /* ������������TX�������е�Э��ģʽ */
    if (en_rate_index <= WLAN_SHORT_11b_11_M_BPS) {
        pst_set_rate_param->en_protocol_mode = WLAN_11B_PHY_PROTOCOL_MODE;
    } else if (en_rate_index >= WLAN_LEGACY_OFDM_48M_BPS && en_rate_index <= WLAN_LEGACY_OFDM_9M_BPS) {
        pst_set_rate_param->en_protocol_mode = WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE;
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_rate::invalid rate!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ����Ҫ����Ϊ����ֵ */
    pst_set_rate_param->en_rate = en_rate_index;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_non_ht_rate_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rate::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
    return OAL_SUCC;
}


oal_uint32 wal_hipriv_set_mcs(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_tx_comp_stru *pst_set_mcs_param = OAL_PTR_NULL;
    oal_int32 l_mcs;
    oal_int8 ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32 l_idx = 0;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_MCS, OAL_SIZEOF(mac_cfg_tx_comp_stru));

    /* ��������������������� */
    pst_set_mcs_param = (mac_cfg_tx_comp_stru *)(st_write_msg.auc_value);

    /* ��ȡ����ֵ�ַ��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mcs::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* ��������Ϸ��Լ�� */
    while (ac_arg[l_idx] != '\0') {
        if (isdigit(ac_arg[l_idx])) {
            l_idx++;
            continue;
        } else {
            l_idx++;
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_mcs::input illegal!}\r\n");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    /* ����Ҫ����Ϊ����ֵ */
    l_mcs = oal_atoi(ac_arg);
    if (l_mcs < WAL_HIPRIV_HT_MCS_MIN || l_mcs > WAL_HIPRIV_HT_MCS_MAX) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mcs::input val out of range [%d]!}\r\n", l_mcs);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pst_set_mcs_param->uc_param = (oal_uint8)l_mcs;
    pst_set_mcs_param->en_protocol_mode = WLAN_HT_PHY_PROTOCOL_MODE;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_tx_comp_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mcs::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
    return OAL_SUCC;
}


oal_uint32 wal_hipriv_set_mcsac(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_tx_comp_stru *pst_set_mcs_param = OAL_PTR_NULL;
    oal_int32 l_mcs;
    oal_int8 ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32 l_idx = 0;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_MCSAC, OAL_SIZEOF(mac_cfg_tx_comp_stru));

    /* ��������������������� */
    pst_set_mcs_param = (mac_cfg_tx_comp_stru *)(st_write_msg.auc_value);

    /* ��ȡ����ֵ�ַ��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mcsac::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    /* ��������Ϸ��Լ�� */
    while (ac_arg[l_idx] != '\0') {
        if (isdigit(ac_arg[l_idx])) {
            l_idx++;
            continue;
        } else {
            l_idx++;
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_mcsac::input illegal!}\r\n");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    /* ����Ҫ����Ϊ����ֵ */
    l_mcs = oal_atoi(ac_arg);
    if (l_mcs < WAL_HIPRIV_VHT_MCS_MIN || l_mcs > WAL_HIPRIV_VHT_MCS_MAX) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mcs::input val out of range [%d]!}\r\n", l_mcs);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pst_set_mcs_param->uc_param = (oal_uint8)l_mcs;
    pst_set_mcs_param->en_protocol_mode = WLAN_VHT_PHY_PROTOCOL_MODE;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_tx_comp_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mcsac::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
    return OAL_SUCC;
}


oal_uint32 wal_hipriv_set_bw(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_tx_comp_stru *pst_set_bw_param = OAL_PTR_NULL;
    hal_channel_assemble_enum_uint8 en_bw_index;
    oal_int8 ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_BW, OAL_SIZEOF(mac_cfg_tx_comp_stru));

    /* ��������������������� */
    pst_set_bw_param = (mac_cfg_tx_comp_stru *)(st_write_msg.auc_value);

    /* ��ȡ����ֵ�ַ��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_bw::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* ����Ҫ����Ϊ����ֵ */
    for (en_bw_index = 0; en_bw_index < WLAN_BAND_ASSEMBLE_AUTO; en_bw_index++) {
        if (!oal_strcmp(pauc_bw_tbl[en_bw_index], ac_arg)) {
            break;
        }
    }

    /* ��������Ƿ��� */
    if (en_bw_index >= WLAN_BAND_ASSEMBLE_AUTO) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_bw::not support this bandwidth!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pst_set_bw_param->uc_param = (oal_uint8)(en_bw_index);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_tx_comp_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_bw::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
    return OAL_SUCC;
}


#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
oal_uint32 wal_hipriv_always_tx_1102(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_tx_comp_stru *pst_set_bcast_param = OAL_PTR_NULL;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_switch_enum_uint8 en_tx_flag;
    mac_rf_payload_enum_uint8 en_payload_flag = RF_PAYLOAD_ALL_ZERO;
    oal_uint32 ul_len = 0;
    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_ALWAYS_TX_1102, OAL_SIZEOF(mac_cfg_tx_comp_stru));

    /* ��������������������� */
    pst_set_bcast_param = (mac_cfg_tx_comp_stru *)(st_write_msg.auc_value);

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_always_tx_1102::wal_get_cmd_one_arg return err[%d]!}", ul_ret);
        return ul_ret;
    }
    en_tx_flag = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;
    if (en_tx_flag >= OAL_SWITCH_BUTT) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_hipriv_always_tx_1102: param error %d", en_tx_flag);
        return OAL_ERR_CODE_MAGIC_NUM_FAIL;
    }
    if (en_tx_flag != OAL_SWITCH_OFF) {
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_always_tx_1102::wal_get_cmd_one_arg err_code [%d]!}", ul_ret);
            return ul_ret;
        }
        pc_param = pc_param + ul_off_set;
        en_payload_flag = (oal_uint8)oal_atoi(ac_name);
        if (en_payload_flag >= RF_PAYLOAD_BUTT) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_always_tx_1102::payload flag err[%d]!}", en_payload_flag);
            return OAL_ERR_CODE_INVALID_CONFIG;
        }

        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_always_tx_1102::wal_get_cmd_one_arg err_code [%d]!}", ul_ret);
            return ul_ret;
        }
        ul_len = (oal_uint16)oal_atoi(ac_name);
        if (ul_len > 65535) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_always_tx_1102::len [%u] overflow!}", ul_len);
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
        pc_param += ul_off_set;
    }

    /* �رյ�����²���Ҫ��������Ĳ��� */
    pst_set_bcast_param->en_payload_flag = en_payload_flag;
    pst_set_bcast_param->ul_payload_len = ul_len;
    pst_set_bcast_param->uc_param = en_tx_flag;

    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE,
        WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_tx_comp_stru),
        (oal_uint8 *)&st_write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_always_tx_1102::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


oal_uint32 wal_hipriv_always_rx(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_int8 ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint8 uc_rx_flag;
    oal_int32 l_idx = 0;

    memset_s(&st_write_msg, OAL_SIZEOF(wal_msg_write_stru), 0, OAL_SIZEOF(wal_msg_write_stru));
    /* ��ȡ����ģʽ���ر�־ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_always_rx::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    /* ��������Ϸ��Լ�� */
    while (ac_arg[l_idx] != '\0') {
        if (isdigit(ac_arg[l_idx])) {
            l_idx++;
            continue;
        } else {
            l_idx++;
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_always_rx::input illegal!}\r\n");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    /* ���������ֵ�ַ���ת��Ϊ���� */
    uc_rx_flag = (oal_uint8)oal_atoi(ac_arg);
    if (uc_rx_flag > HAL_ALWAYS_RX_RESERVED) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_always_rx::input should be 0 or 1.}");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    *(oal_uint8 *)(st_write_msg.auc_value) = uc_rx_flag;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_ALWAYS_RX, OAL_SIZEOF(oal_uint8));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_always_rx::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32 wal_hipriv_dync_txpower(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_int8 ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint8 uc_dync_power_flag;
    oal_int32 l_idx = 0;

    /* ��ȡ��̬����У׼���ر�־ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dync_txpower::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    /* ��������Ϸ��Լ�� */
    while (ac_arg[l_idx] != '\0') {
        if (isdigit(ac_arg[l_idx])) {
            l_idx++;
            continue;
        } else {
            l_idx++;
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_dync_txpower::input illegal!}\r\n");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    /* ���������ֵ�ַ���ת��Ϊ���� */
    uc_dync_power_flag = (oal_uint8)oal_atoi(ac_arg);

    *(oal_uint8 *)(st_write_msg.auc_value) = uc_dync_power_flag;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DYNC_TXPOWER, OAL_SIZEOF(oal_uint8));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dync_txpower::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32 wal_hipriv_get_thruput(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_int8 ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint8 uc_stage;
    oal_int32 l_idx = 0;

    /* ��ȡ���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_thruput::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    /* ��������Ϸ��Լ�� */
    while (ac_arg[l_idx] != '\0') {
        if (isdigit(ac_arg[l_idx])) {
            l_idx++;
            continue;
        } else {
            l_idx++;
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_get_thruput::input illegal!}\r\n");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    /* ���������ֵ�ַ���ת��Ϊ���� */
    uc_stage = (oal_uint8)oal_atoi(ac_arg);

    *(oal_uint8 *)(st_write_msg.auc_value) = uc_stage;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_THRUPUT, OAL_SIZEOF(oal_uint8));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_thruput::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_amsdu_start(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_amsdu_start_param_stru *pst_amsdu_start_param = OAL_PTR_NULL;

    /* ���¼���wal�㴦�� */
    /* ��д��Ϣ */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_AMSDU_START, OAL_SIZEOF(mac_cfg_amsdu_start_param_stru));

    /* ��������������������� */
    pst_amsdu_start_param = (mac_cfg_amsdu_start_param_stru *)(st_write_msg.auc_value);

    /* ��ȡmac��ַ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_amsdu_start::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, sizeof(ac_name), pst_amsdu_start_param->auc_mac_addr, WLAN_MAC_ADDR_LEN);
    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_amsdu_start::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_amsdu_start_param->uc_amsdu_max_num = (oal_uint8)oal_atoi(ac_name);

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_amsdu_start::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_amsdu_start_param->us_amsdu_max_size = (oal_uint16)oal_atoi(ac_name);

    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_amsdu_start_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_amsdu_start::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC int wal_ioctl_get_iwname(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info,
    oal_iwreq_data_union *pst_wrqu, char *pc_extra)
{
    oal_int8 ac_iwname[] = "IEEE 802.11";

    if ((pst_net_dev == OAL_PTR_NULL) || (pst_wrqu == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_ioctl_get_iwname::param null}");
        return -OAL_EINVAL;
    }

    if (memcpy_s(pst_wrqu->name, OAL_IF_NAME_SIZE, ac_iwname,
        OAL_MIN(OAL_SIZEOF(ac_iwname), OAL_SIZEOF(pst_wrqu->name))) != EOK) { //lint !e506
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_ioctl_get_iwname::memcpy_s failed!");
        return OAL_FAIL;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_find_cmd(oal_int8 *pc_cmd_name, oal_uint8 *puc_cmd_id)
{
    oal_uint8 en_cmd_idx;
    int l_ret;

    if (OAL_UNLIKELY((pc_cmd_name == OAL_PTR_NULL) || (puc_cmd_id == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_find_cmd::pc_cmd_name/puc_cmd_id null ptr error}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    for (en_cmd_idx = 0; en_cmd_idx < OAL_ARRAY_SIZE(g_ast_hipriv_cmd); en_cmd_idx++) {
        l_ret = oal_strcmp(g_ast_hipriv_cmd[en_cmd_idx].pc_cmd_name, pc_cmd_name);
        if (l_ret == 0) {
            *puc_cmd_id = en_cmd_idx;

            return OAL_SUCC;
        }
    }

    OAM_IO_PRINTK("cmd name[%s] is not exist. \r\n", pc_cmd_name);
    return OAL_FAIL;
}


OAL_STATIC oal_uint32 wal_hipriv_get_cmd_net_dev(oal_int8 *pc_cmd, oal_net_device_stru **ppst_net_dev,
    oal_uint32 *pul_off_set)
{
    oal_net_device_stru *pst_net_dev = OAL_PTR_NULL;
    oal_int8 ac_dev_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;

    if (OAL_UNLIKELY((pc_cmd == OAL_PTR_NULL) || (ppst_net_dev == OAL_PTR_NULL) || (pul_off_set == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY,
            "{wal_hipriv_get_cmd_net_dev::pc_cmd/ppst_net_dev/pul_off_set null ptr error}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = wal_get_cmd_one_arg(pc_cmd, ac_dev_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, pul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_cmd_net_dev::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    /* ����dev_name�ҵ�dev */
    pst_net_dev = oal_dev_get_by_name(ac_dev_name);
    if (pst_net_dev == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_get_cmd_net_dev::oal_dev_get_by_name return null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ����oal_dev_get_by_name�󣬱������oal_dev_putʹnet_dev�����ü�����һ */
    oal_dev_put(pst_net_dev);

    *ppst_net_dev = pst_net_dev;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_get_cmd_id(oal_int8 *pc_cmd, oal_uint8 *puc_cmd_id, oal_uint32 *pul_off_set)
{
    oal_uint8 en_cmd_id;
    oal_int8 ac_cmd_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;

    if (OAL_UNLIKELY((pc_cmd == OAL_PTR_NULL) || (puc_cmd_id == OAL_PTR_NULL) || (pul_off_set == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY,
            "{wal_hipriv_get_cmd_id::pc_cmd/puc_cmd_id/pul_off_set null ptr error}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = wal_get_cmd_one_arg(pc_cmd, ac_cmd_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, pul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_cmd_id::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    /* �����������ҵ�����ö�� */
    ul_ret = wal_hipriv_find_cmd(ac_cmd_name, &en_cmd_id);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_cmd_id::wal_hipriv_find_cmd return error cod [%d]!}", ul_ret);
        return ul_ret;
    }

    *puc_cmd_id = en_cmd_id;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_parse_cmd(oal_int8 *pc_cmd)
{
    oal_net_device_stru *pst_net_dev = OAL_PTR_NULL;
    oal_uint8 en_cmd;
    oal_uint32 ul_off_set = 0;
    oal_uint32 ul_ret;
    if (OAL_UNLIKELY(pc_cmd == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_parse_cmd::pc_cmd null ptr error!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*
     * cmd��ʽԼ��
     * �����豸�� ����      ����   Hisilicon0 create vap0
     * 1~15Byte   1~15Byte
     */
    ul_ret = wal_hipriv_get_cmd_net_dev(pc_cmd, &pst_net_dev, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_parse_cmd::wal_hipriv_get_cmd_net_dev return error code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    pc_cmd += ul_off_set;
    ul_ret = wal_hipriv_get_cmd_id(pc_cmd, &en_cmd, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_parse_cmd::wal_hipriv_get_cmd_id return error[%d]!}", ul_ret);
        return ul_ret;
    }

    pc_cmd += ul_off_set;

    /* ���������Ӧ�ĺ��� */
    ul_ret = g_ast_hipriv_cmd[en_cmd].p_func(pst_net_dev, pc_cmd);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_parse_cmd::g_ast_hipriv_cmd return error code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    return OAL_SUCC;
}

#if ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)) && \
    (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
oal_ssize_t wal_hipriv_cmd_sys_write(struct kobject *dev, struct kobj_attribute *attr, const char *pc_buffer,
    oal_size_t count)
{
    return wal_hipriv_sys_write(dev, attr, pc_buffer, count);
}
#endif


OAL_STATIC oal_ssize_t wal_hipriv_sys_write(struct kobject *dev, struct kobj_attribute *attr, const char *pc_buffer,
    oal_size_t count)
{
    oal_int8 *pc_cmd = OAL_PTR_NULL;
    oal_uint32 ul_ret;
    oal_uint32 ul_len = (oal_uint32)count;

    if (ul_len > WAL_HIPRIV_CMD_MAX_LEN) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sys_write::ul_len>WAL_HIPRIV_CMD_MAX_LEN, ul_len [%d]!}", ul_len);
        return -OAL_EINVAL;
    }

    pc_cmd = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, WAL_HIPRIV_CMD_MAX_LEN, OAL_TRUE);
    if (OAL_UNLIKELY(pc_cmd == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_proc_write::alloc mem return null ptr!}\r\n");
        return -OAL_ENOMEM;
    }

    memset_s(pc_cmd, WAL_HIPRIV_CMD_MAX_LEN, 0, WAL_HIPRIV_CMD_MAX_LEN);

    if (memcpy_s(pc_cmd, WAL_HIPRIV_CMD_MAX_LEN, pc_buffer, ul_len) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_proc_write::memcpy_s failed!");
        OAL_MEM_FREE(pc_cmd, OAL_TRUE);
        return -OAL_ENOMEM;
    }

    pc_cmd[ul_len - 1] = '\0';

    OAM_IO_PRINTK(" %s\n", pc_cmd);

    ul_ret = wal_hipriv_parse_cmd(pc_cmd);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_proc_write::parse cmd return err code[%d]!}\r\n", ul_ret);
    }

    OAL_MEM_FREE(pc_cmd, OAL_TRUE);

    return (oal_int32)ul_len;
}


#define SYS_READ_MAX_STRING_LEN (4096 - 40) /* ��ǰ�����ַ�����20�ֽ��ڣ�Ԥ��40��֤���ᳬ�� */
OAL_STATIC oal_ssize_t wal_hipriv_sys_read(struct kobject *dev, struct kobj_attribute *attr, char *pc_buffer)
{
    oal_uint32 ul_cmd_idx;
    oal_int32 buff_index = 0;

    for (ul_cmd_idx = 0; ul_cmd_idx < OAL_ARRAY_SIZE(g_ast_hipriv_cmd); ul_cmd_idx++) {
        buff_index += snprintf_s(pc_buffer + buff_index, (SYS_READ_MAX_STRING_LEN - buff_index),
            (SYS_READ_MAX_STRING_LEN - buff_index) - 1, "\t%s\n", g_ast_hipriv_cmd[ul_cmd_idx].pc_cmd_name);
        if (buff_index > SYS_READ_MAX_STRING_LEN) {
            buff_index += snprintf_s(pc_buffer + buff_index, (SYS_READ_MAX_STRING_LEN - buff_index),
                (SYS_READ_MAX_STRING_LEN - buff_index) - 1, "\tmore...\n");
            break;
        }
    }
    if (buff_index < EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_sys_read::snprintf_s failed !\r\n");
        return OAL_FAIL;
    }
    return buff_index;
}

#endif /* _PRE_OS_VERSION_LINUX */


OAL_STATIC oal_int32 wal_hipriv_proc_write(oal_file_stru *pst_file, const oal_int8 *pc_buffer, oal_uint32 ul_len,
    oal_void *p_data)
{
    oal_int8 *pc_cmd = OAL_PTR_NULL;
    oal_uint32 ul_ret;

    if (ul_len > WAL_HIPRIV_CMD_MAX_LEN) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_proc_write::ul_len>WAL_HIPRIV_CMD_MAX_LEN, ul_len [%d]!}", ul_len);
        return -OAL_EINVAL;
    }

    pc_cmd = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, WAL_HIPRIV_CMD_MAX_LEN, OAL_TRUE);
    if (OAL_UNLIKELY(pc_cmd == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_proc_write::alloc mem return null ptr!}\r\n");
        return -OAL_ENOMEM;
    }

    memset_s(pc_cmd, WAL_HIPRIV_CMD_MAX_LEN, 0, WAL_HIPRIV_CMD_MAX_LEN);

    ul_ret = oal_copy_from_user((oal_void *)pc_cmd, pc_buffer, ul_len);
    /* copy_from_user������Ŀ���Ǵ��û��ռ俽�����ݵ��ں˿ռ䣬ʧ�ܷ���û�б��������ֽ������ɹ�����0 */
    if (ul_ret > 0) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_proc_write::oal_copy_from_user return ul_ret[%d]!}\r\n", ul_ret);
        OAL_MEM_FREE(pc_cmd, OAL_TRUE);

        return -OAL_EFAUL;
    }

    pc_cmd[ul_len - 1] = '\0';

    ul_ret = wal_hipriv_parse_cmd(pc_cmd);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_proc_write::parse cmd return err code[%d]!}\r\n", ul_ret);
    }

    OAL_MEM_FREE(pc_cmd, OAL_TRUE);

    return (oal_int32)ul_len;
}


oal_uint32 wal_hipriv_create_proc(oal_void *p_proc_arg)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && \
    ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION))
    oal_uint32 ul_ret;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 35)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    g_pst_proc_entry = OAL_PTR_NULL;
#else

    /*
     * 420ʮ���ƶ�Ӧ�˽�����0644 linuxģʽ���� S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
     * S_IRUSR�ļ������߾߿ɶ�ȡȨ��, S_IWUSR�ļ������߾߿�д��Ȩ��, S_IRGRP�û���߿ɶ�ȡȨ��,
     * S_IROTH�����û��߿ɶ�ȡȨ��
     */
    g_pst_proc_entry = oal_create_proc_entry(WAL_HIPRIV_PROC_ENTRY_NAME, 420, NULL);
    if (g_pst_proc_entry == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_create_proc::oal_create_proc_entry return null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    g_pst_proc_entry->data = p_proc_arg;
    g_pst_proc_entry->nlink = 1; /* linux����procĬ��ֵ */
    g_pst_proc_entry->read_proc = OAL_PTR_NULL;

    g_pst_proc_entry->write_proc = (write_proc_t *)wal_hipriv_proc_write;

#endif
    /* hi1102-cb add sys for 51/02 */
#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT
    gp_sys_kobject = oal_get_sysfs_root_object();
    if (gp_sys_kobject == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_create_proc::get sysfs root object failed!}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    ul_ret = (uint32_t)oal_debug_sysfs_create_file(gp_sys_kobject, &dev_attr_hipriv.attr);
    if (ul_ret) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_create_proc::oal_debug_sysfs_create_file create failed!}");
        ul_ret = OAL_ERR_CODE_PTR_NULL;
    }
    return ul_ret;
#else
    return OAL_SUCC;
#endif
}


oal_uint32 wal_hipriv_remove_proc(void)
{
    /* ж��ʱɾ��sysfs */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION))
    oal_debug_sysfs_remove_file(gp_sys_kobject, &dev_attr_hipriv.attr);
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)

    oal_remove_proc_entry(WAL_HIPRIV_PROC_ENTRY_NAME, NULL);
#elif (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)

    oal_remove_proc_entry(WAL_HIPRIV_PROC_ENTRY_NAME, g_pst_proc_entry);
#endif
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_mem_info(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param)
{
    oal_int8 auc_token[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_mem_pool_id_enum_uint8 en_pool_id;
    oal_uint32 ul_off_set;
    oal_uint32 ul_ret;

    /* ��μ�� */
    if (OAL_UNLIKELY(pst_cfg_net_dev == OAL_PTR_NULL) || OAL_UNLIKELY(pc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_mem_info::pst_net_dev or pc_param null ptr error}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ�ڴ��ID */
    ul_ret = wal_get_cmd_one_arg(pc_param, auc_token, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_mem_info::wal_get_cmd_one_arg return error code [%d]!}", ul_ret);
        return ul_ret;
    }

    en_pool_id = (oal_mem_pool_id_enum_uint8)oal_atoi(auc_token);

    /* ��ӡ�ڴ����Ϣ */
    oal_mem_info(en_pool_id);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_mem_leak(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param)
{
    oal_int8 auc_token[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_mem_pool_id_enum_uint8 en_pool_id;
    oal_uint32 ul_off_set;
    oal_uint32 ul_ret;

    /* ��μ�� */
    if (OAL_UNLIKELY(pst_cfg_net_dev == OAL_PTR_NULL) || OAL_UNLIKELY(pc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_mem_leak::pst_net_dev or pc_param null ptr error}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ�ڴ��ID */
    ul_ret = wal_get_cmd_one_arg(pc_param, auc_token, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_mem_leak::wal_get_cmd_one_arg return error code [%d]!}", ul_ret);
        return ul_ret;
    }

    en_pool_id = (oal_mem_pool_id_enum_uint8)oal_atoi(auc_token);
    if (en_pool_id > OAL_MEM_POOL_ID_SDT_NETBUF) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_mem_leak::mem pool id exceeds,en_pool_id[%d]!}\r\n", en_pool_id);
        return OAL_SUCC;
    }

    /* ����ڴ��й©�ڴ�� */
    oal_mem_leak(en_pool_id);

    return OAL_SUCC;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32 wal_hipriv_device_mem_leak(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint8 uc_pool_id;
    mac_device_pool_id_stru *pst_pool_id_param = OAL_PTR_NULL;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_device_mem_leak::wal_get_cmd_one_arg return err[%d]!}", ul_ret);
        return ul_ret;
    }

    uc_pool_id = (oal_uint8)oal_atoi(ac_name);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DEVICE_MEM_LEAK, OAL_SIZEOF(mac_device_pool_id_stru));

    /* ��������������� */
    pst_pool_id_param = (mac_device_pool_id_stru *)(st_write_msg.auc_value);
    pst_pool_id_param->uc_pool_id = uc_pool_id;

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_device_pool_id_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_hipriv_device_mem_leak::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_memory_info(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param)
{
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint8 uc_pool_id;
    mac_device_pool_id_stru *pst_pool_id_param = OAL_PTR_NULL;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_memory_info::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    if ((oal_strcmp("host", ac_name)) == 0) {
        oal_mem_print_pool_info();
    } else if ((oal_strcmp("device", ac_name)) == 0) {
        hcc_print_device_mem_info();
    } else {
        uc_pool_id = (oal_uint8)oal_atoi(ac_name);
        /* ���¼���wal�㴦�� */
        WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DEVICE_MEM_INFO, OAL_SIZEOF(mac_device_pool_id_stru));

        /* ��������������� */
        pst_pool_id_param = (mac_device_pool_id_stru *)(st_write_msg.auc_value);
        pst_pool_id_param->uc_pool_id = uc_pool_id;

        l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_device_pool_id_stru),
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_FALSE,
                                   OAL_PTR_NULL);
        if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
            OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_hipriv_memory_info::return err code [%d]!}\r\n", l_ret);
            return (oal_uint32)l_ret;
        }
    }

    return OAL_SUCC;
}

#endif


OAL_STATIC uint32_t wal_hipriv_reg_info(oal_net_device_stru *net_dev, const int8_t *param)
{
    wal_msg_write_stru write_msg;
    int32_t ret;
    uint16_t len;

    /* ���¼���wal�㴦�� */
    if (memcpy_s(write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, param, OAL_STRLEN(param)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_reg_info::memcpy_s failed!");
        return OAL_FAIL;
    }

    write_msg.auc_value[OAL_STRLEN(param)] = '\0';

    len = (oal_uint16)(OAL_STRLEN(param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_REG_INFO, len);

    ret = wal_send_cfg_event(net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + len,
                             (uint8_t *)&write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_reg_info::return err code [%d]!}\r\n", ret);
        return (uint32_t)ret;
    }

    return OAL_SUCC;
}

#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST))

OAL_STATIC oal_uint32 wal_hipriv_sdio_flowctrl(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint16 us_len;
    if (OAL_UNLIKELY(OAL_STRLEN(pc_param) >= WAL_MSG_WRITE_MAX_LEN)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sdio_flowctrl:: pc_param overlength is %d}\n",
            OAL_STRLEN(pc_param));
        return OAL_FAIL;
    }

    /* ���¼���wal�㴦�� */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, pc_param, OAL_STRLEN(pc_param)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_sdio_flowctrl::memcpy_s failed!");
        return OAL_FAIL;
    }

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SDIO_FLOWCTRL, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sdio_flowctrl::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_FEATURE_HILINK

oal_uint32 wal_hipriv_set_monitor_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    wal_hilink_enum_uint8 uc_tmp;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32 l_ret;
    oal_uint32 ul_ret;

    /* OAM logģ��Ŀ��ص�����: hipriv "Hisilicon0 set_monitor 0 | 1" �˴���������"1"��"0"����ac_name */
    OAM_WARNING_LOG0(0, OAM_SF_ANY, "wal_hipriv_set_monitor_switch::[rl] in");
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_monitor_switch::wal_get_cmd_one_arg return err[%d]!}", ul_ret);
        return ul_ret;
    }

    /* ��Խ������Ĳ�ͬ�����logģ����в�ͬ������ */
    if ((oal_strcmp("0", ac_name)) == 0) {
        uc_tmp = WAL_HILINK_OTHER_BSS_CAST_DATA_CLOSE;
    } else if ((oal_strcmp("1", ac_name)) == 0) {
        uc_tmp = WAL_HILINK_OTHER_BSS_CAST_DATA_OPEN;
    } else if ((oal_strcmp("2", ac_name)) == 0) {
        uc_tmp = WAL_HILINK_MONITOR_CLOSE;
    } else if ((oal_strcmp("3", ac_name)) == 0) {
        uc_tmp = WAL_HILINK_MONITOR_OPEN;
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_monitor_switch::command param is error!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_MONITOR_EN, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = uc_tmp; /* ��������������� */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_monitor_switch::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32 wal_hipriv_set_mips_cycle_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint8 uc_param = 0;
    oal_uint32 ul_off_set = 0;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;

    /* MIPS_CYCLE �����ر�����: hipriv.sh "Hisilicon0 set_mips_cycle_switch 0 | 1" �˴���������"1"��"0"����ac_name */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_mips_cycle_switch::wal_get_cmd_one_arg return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    /* ��Խ������Ĳ�ͬ�����logģ����в�ͬ������ */
    if ((oal_strcmp("0", ac_name)) == 0) {
        uc_param = 0;
    } else if ((oal_strcmp("1", ac_name)) == 0) {
        uc_param = 1;
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_mips_cycle_switch::command param is error!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_MIPS_CYCLE_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = uc_param; /* ��������������� */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mips_cycle_switch::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_list_ap(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LIST_AP, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_list_ap::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_list_sta(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LIST_STA, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_list_sta::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_list_channel(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LIST_CHAN, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_list_channel::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_regdomain_pwr(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_int32 l_pwr;
    wal_msg_write_stru st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "wal_hipriv_set_regdomain_pwr, get arg return err %d", ul_ret);
        return ul_ret;
    }

    l_pwr = oal_atoi(ac_name);
    if (l_pwr <= 0 || l_pwr > 100) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "invalid value, %d", l_pwr);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_REGDOMAIN_PWR, OAL_SIZEOF(mac_cfg_regdomain_max_pwr_stru));

    ((mac_cfg_regdomain_max_pwr_stru *)st_write_msg.auc_value)->uc_pwr = (oal_uint8)l_pwr;
    ((mac_cfg_regdomain_max_pwr_stru *)st_write_msg.auc_value)->en_exceed_reg = OAL_FALSE;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (l_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_set_regdomain_pwr::wal_send_cfg_event fail.return err code %d}",
            l_ret);
    }

    return (oal_uint32)l_ret;
}


OAL_STATIC oal_uint32 wal_hipriv_set_regdomain_pwr_priv(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint32 ul_pwr;
    wal_msg_write_stru st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "wal_hipriv_set_regdomain_pwr, get arg return err %d", ul_ret);
        return ul_ret;
    }

    ul_pwr = (oal_uint32)oal_atoi(ac_name);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_REGDOMAIN_PWR, OAL_SIZEOF(oal_int32));

    ((mac_cfg_regdomain_max_pwr_stru *)st_write_msg.auc_value)->uc_pwr = (oal_uint8)ul_pwr;
    ((mac_cfg_regdomain_max_pwr_stru *)st_write_msg.auc_value)->en_exceed_reg = OAL_TRUE;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (l_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_set_regdomain_pwr::wal_send_cfg_event fail.return err code %d}",
            l_ret);
    }

    return (oal_uint32)l_ret;
}


OAL_STATIC oal_uint32 wal_hipriv_event_queue_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    return frw_event_queue_info();
}


OAL_STATIC oal_uint32 wal_hipriv_dump_all_rx_dscr(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DUMP_ALL_RX_DSCR, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_all_rx_dscr::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_start_scan(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
#ifdef _PRE_WLAN_FEATURE_P2P
    oal_uint8 uc_is_p2p0_scan;
#endif /* _PRE_WLAN_FEATURE_P2P */

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_START_SCAN, OAL_SIZEOF(oal_int32));

#ifdef _PRE_WLAN_FEATURE_P2P
    uc_is_p2p0_scan = (oal_memcmp(pst_net_dev->name, "p2p0", OAL_STRLEN("p2p0")) == 0) ? 1 : 0;
    st_write_msg.auc_value[0] = uc_is_p2p0_scan;
#endif /* _PRE_WLAN_FEATURE_P2P */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_start_scan::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_start_join(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint32 ul_ret;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_START_JOIN, OAL_SIZEOF(oal_int32));

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_start_join::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    /* ��Ҫ����AP�ı�Ÿ��Ƶ��¼�msg�У�AP��������ֵ�ASSCI�룬������4���ֽ� */
    if (memcpy_s((oal_int8 *)st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, (oal_int8 *)ac_name,
        OAL_SIZEOF(oal_int32)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_start_join::memcpy_s failed!");
        return OAL_FAIL;
    }

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_start_join::return err codereturn err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_start_deauth(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_START_DEAUTH, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_start_deauth::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_dump_timer(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DUMP_TIEMR, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_timer::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_kick_user(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_kick_user_param_stru *pst_kick_user_param = OAL_PTR_NULL;
    oal_uint8 auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};

    /* ȥ����1���û������� hipriv "vap0 kick_user xx:xx:xx:xx:xx:xx" */
    /* ��ȡmac��ַ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_kick_user::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, sizeof(ac_name), auc_mac_addr, WLAN_MAC_ADDR_LEN);
    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_KICK_USER, OAL_SIZEOF(mac_cfg_kick_user_param_stru));

    /* ��������������� */
    pst_kick_user_param = (mac_cfg_kick_user_param_stru *)(st_write_msg.auc_value);
    oal_set_mac_addr(pst_kick_user_param->auc_mac_addr, auc_mac_addr);

    /* ��дȥ����reason code */
    pst_kick_user_param->us_reason_code = MAC_UNSPEC_REASON;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_kick_user_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_kick_user::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_pause_tid(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_pause_tid_param_stru *pst_pause_tid_param = OAL_PTR_NULL;
    oal_uint8 auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};
    oal_uint8 uc_tid;
    /* ȥ����1���û������� hipriv "vap0 kick_user xx xx xx xx xx xx" */
    /* ��ȡmac��ַ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pause_tid::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, sizeof(ac_name), auc_mac_addr, WLAN_MAC_ADDR_LEN);
    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pause_tid::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    uc_tid = (oal_uint8)oal_atoi(ac_name);

    pc_param = pc_param + ul_off_set;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PAUSE_TID, OAL_SIZEOF(mac_cfg_pause_tid_param_stru));

    /* ��������������� */
    pst_pause_tid_param = (mac_cfg_pause_tid_param_stru *)(st_write_msg.auc_value);
    oal_set_mac_addr(pst_pause_tid_param->auc_mac_addr, auc_mac_addr);
    pst_pause_tid_param->uc_tid = uc_tid;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pause_tid::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    pst_pause_tid_param->uc_is_paused = (oal_uint8)oal_atoi(ac_name);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_pause_tid_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pause_tid::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_user_vip(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_user_vip_param_stru *pst_user_vip_param = OAL_PTR_NULL;
    oal_uint8 auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};
    oal_uint8 uc_vip_flag;

    /* �����û�Ϊvip�û�: 0 �����VIP�û���1����VIP�û� sh hipriv.sh "vap0 set_user_vip xx xx xx xx xx xx 0|1" */
    /* ��ȡmac��ַ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_user_vip::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, sizeof(ac_name), auc_mac_addr, WLAN_MAC_ADDR_LEN);
    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_user_vip::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    uc_vip_flag = (oal_uint8)oal_atoi(ac_name);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_USER_VIP, OAL_SIZEOF(mac_cfg_pause_tid_param_stru));

    /* ��������������� */
    pst_user_vip_param = (mac_cfg_user_vip_param_stru *)(st_write_msg.auc_value);
    oal_set_mac_addr(pst_user_vip_param->auc_mac_addr, auc_mac_addr);
    pst_user_vip_param->uc_vip_flag = uc_vip_flag;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_user_vip_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_user_vip::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_vap_host(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint8 uc_host_flag;

    /* ����vap��host flag: 0 ����guest vap, 1����host vap�� sh hipriv.sh "vap0 set_host 0|1" */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_vap_host::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    uc_host_flag = (oal_uint8)oal_atoi(ac_name);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_VAP_HOST, OAL_SIZEOF(oal_uint8));

    /* ��������������� */
    *((oal_uint8 *)(st_write_msg.auc_value)) = uc_host_flag;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_vap_host::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_send_bar(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_pause_tid_param_stru *pst_pause_tid_param = OAL_PTR_NULL;
    oal_uint8 auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};
    oal_uint8 uc_tid;

    /* ��ȡmac��ַ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_bar::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, sizeof(ac_name), auc_mac_addr, WLAN_MAC_ADDR_LEN);
    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_bar::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    uc_tid = (oal_uint8)oal_atoi(ac_name);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SEND_BAR, OAL_SIZEOF(mac_cfg_pause_tid_param_stru));

    /* ��������������� */
    pst_pause_tid_param = (mac_cfg_pause_tid_param_stru *)(st_write_msg.auc_value);
    oal_set_mac_addr(pst_pause_tid_param->auc_mac_addr, auc_mac_addr);
    pst_pause_tid_param->uc_tid = uc_tid;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_pause_tid_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_bar::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_amsdu_tx_on(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_ampdu_tx_on_param_stru *pst_aggr_tx_on_param = OAL_PTR_NULL;
    oal_uint8 uc_aggr_tx_on;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_amsdu_tx_on::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    uc_aggr_tx_on = (oal_uint8)oal_atoi(ac_name);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_AMSDU_TX_ON, OAL_SIZEOF(mac_cfg_ampdu_tx_on_param_stru));

    /* ��������������� */
    pst_aggr_tx_on_param = (mac_cfg_ampdu_tx_on_param_stru *)(st_write_msg.auc_value);
    pst_aggr_tx_on_param->uc_aggr_tx_on = uc_aggr_tx_on;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ampdu_tx_on_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_amsdu_tx_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_PROXYSTA

OAL_STATIC oal_uint32 wal_hipriv_set_oma(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_set_oma_param_stru *pst_set_oma_param = OAL_PTR_NULL;
    oal_uint8 auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};

    /* ����Proxy STA ��OMA��ַ���� sh hipriv.sh "vap0 set_vma xx xx xx xx xx xx" */
    /* ��ȡmac��ַ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_PROXYSTA, "{wal_hipriv_set_oma::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, sizeof(ac_name), auc_mac_addr, WLAN_MAC_ADDR_LEN);

    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_OMA, OAL_SIZEOF(mac_cfg_set_oma_param_stru));

    /* ��������������� */
    pst_set_oma_param = (mac_cfg_set_oma_param_stru *)(st_write_msg.auc_value);
    oal_set_mac_addr(pst_set_oma_param->auc_mac_addr, auc_mac_addr);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_oma_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_oma::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_proxysta_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_tmp;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32 l_ret;
    oal_uint32 ul_ret;

    /* proxystaģ��Ŀ��ص�����: hipriv "Hisilicon0 proxysta_switch 0 | 1" �˴���������"1"��"0"����ac_name */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        return ul_ret;
    }

    /* ��Խ������Ĳ�ͬ�����proxystaģ����в�ͬ������ */
    if ((oal_strcmp("0", ac_name)) == 0) {
        l_tmp = 0;
    } else if ((oal_strcmp("1", ac_name)) == 0) {
        l_tmp = 1;
    } else {
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PROXYSTA_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp; /* ��������������� */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif


OAL_STATIC oal_uint32 wal_hipriv_frag_threshold(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_ret;
    oal_int32 l_cfg_rst;
    oal_uint16 us_len;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;
    mac_cfg_frag_threshold_stru *pst_threshold = OAL_PTR_NULL;
    oal_uint32 ul_threshold;

    /* ��ȡ��Ƭ���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_frag_threshold::wal_get_cmd_one_arg return err[%d]!}", ul_ret);
        return ul_ret;
    }
    ul_threshold = (oal_uint16)oal_atoi(ac_name);
    pc_param += ul_off_set;

    if ((ul_threshold < WLAN_FRAG_THRESHOLD_MIN) || (ul_threshold > WLAN_FRAG_THRESHOLD_MAX)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_frag_threshold::ul_threshold value error [%d]!}", ul_threshold);
        return OAL_FAIL;
    }

    pst_threshold = (mac_cfg_frag_threshold_stru *)(st_write_msg.auc_value);
    pst_threshold->ul_frag_threshold = ul_threshold;

    /* ���¼���wal�㴦�� */
    us_len = OAL_SIZEOF(mac_cfg_frag_threshold_stru);
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_FRAG_THRESHOLD_REG, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_FALSE,
                                   OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_cfg_rst != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_frag_threshold::return err code [%d]!}\r\n", l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_wmm_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_ret;
    oal_int32 l_cfg_rst;
    oal_uint16 us_len;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;
    oal_uint8 uc_open_wmm;

    /* ��ȡ�趨��ֵ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_wmm_switch::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }
    uc_open_wmm = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;

    /* ���¼���wal�㴦�� */
    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_open_wmm;
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_WMM_SWITCH, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_FALSE,
                                   OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_cfg_rst != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_wmm_switch::return err code [%d]!}\r\n", l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_hide_ssid(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_ret;
    oal_int32 l_cfg_rst;
    oal_uint16 us_len;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;
    oal_uint8 uc_hide_ssid;

    /* ��ȡ�趨��ֵ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_hide_ssid::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }
    uc_hide_ssid = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;

    /* ���¼���wal�㴦�� */
    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_hide_ssid;
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_HIDE_SSID, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_FALSE,
                                   OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_cfg_rst != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_hide_ssid::return err code [%d]!}\r\n", l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32 wal_hipriv_resume_rx_intr_fifo(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_resume_rx_intr_fifo_stru *pst_param = OAL_PTR_NULL;
    oal_uint8 uc_is_on;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_resume_rx_intr_fifo::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    uc_is_on = (oal_uint8)oal_atoi(ac_name);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RESUME_RX_INTR_FIFO, OAL_SIZEOF(mac_cfg_resume_rx_intr_fifo_stru));

    /* ��������������� */
    pst_param = (mac_cfg_resume_rx_intr_fifo_stru *)(st_write_msg.auc_value);
    pst_param->uc_is_on = uc_is_on;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_resume_rx_intr_fifo_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_resume_rx_intr_fifo::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

OAL_STATIC oal_uint32 wal_hipriv_ampdu_tx_on(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_ampdu_tx_on_param_stru *pst_aggr_tx_on_param = OAL_PTR_NULL;
    oal_uint8 uc_aggr_tx_on;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_tx_on::wal_get_cmd_one_arg return err_code [%d]!}", ul_ret);
        return ul_ret;
    }

    uc_aggr_tx_on = (oal_uint8)oal_atoi(ac_name);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_AMPDU_TX_ON, OAL_SIZEOF(mac_cfg_ampdu_tx_on_param_stru));

    /* ��������������� */
    pst_aggr_tx_on_param = (mac_cfg_ampdu_tx_on_param_stru *)(st_write_msg.auc_value);
    pst_aggr_tx_on_param->uc_aggr_tx_on = uc_aggr_tx_on;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ampdu_tx_on_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_tx_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_reset_device(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint16 us_len;

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, pc_param, OAL_STRLEN(pc_param)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_reset_device::memcpy_s failed!");
        return OAL_FAIL;
    }

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RESET_HW, us_len);

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_reset_device::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_reset_operate(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint16 us_len;
    if (OAL_UNLIKELY(OAL_STRLEN(pc_param) >= WAL_MSG_WRITE_MAX_LEN)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_reset_operate:: pc_param overlength is %d}\n",
            OAL_STRLEN(pc_param));
        return OAL_FAIL;
    }

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, pc_param, OAL_STRLEN(pc_param)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_reset_operate::memcpy_s failed!");
        return OAL_FAIL;
    }

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RESET_HW_OPERATE, us_len);

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_reset_operate::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_UAPSD

OAL_STATIC oal_uint32 wal_hipriv_uapsd_debug(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint16 us_len;

    if (OAL_UNLIKELY(OAL_STRLEN(pc_param) >= WAL_MSG_WRITE_MAX_LEN)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_uapsd_debug:: pc_param overlength is %d}\n", OAL_STRLEN(pc_param));
        return OAL_FAIL;
    }

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, pc_param, OAL_STRLEN(pc_param)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_uapsd_debug::memcpy_s failed!");
        return OAL_FAIL;
    }

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_UAPSD_DEBUG, us_len);

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_uapsd_debug::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_DFT_STAT

OAL_STATIC oal_uint32 wal_hipriv_set_phy_stat_en(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oam_stats_phy_node_idx_stru st_phy_stat_node_idx;
    oal_uint8 uc_loop;

    /* sh hipriv.sh "Hisilicon0 phy_stat_en idx1 idx2 idx3 idx4" */
    for (uc_loop = 0; uc_loop < OAM_PHY_STAT_NODE_ENABLED_MAX_NUM; uc_loop++) {
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY,
                "{wal_hipriv_set_phy_stat_en::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }

        st_phy_stat_node_idx.auc_node_idx[uc_loop] = (oal_uint8)oal_atoi(ac_name);

        /* �������Ƿ�Ϸ���������Χ��1~16 */
        if (st_phy_stat_node_idx.auc_node_idx[uc_loop] < OAM_PHY_STAT_ITEM_MIN_IDX ||
            st_phy_stat_node_idx.auc_node_idx[uc_loop] > OAM_PHY_STAT_ITEM_MAX_IDX) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY,
                "{wal_hipriv_set_phy_stat_en::stat_item_idx invalid! should between 1 and 16!}.",
                st_phy_stat_node_idx.auc_node_idx[uc_loop]);
            return OAL_ERR_CODE_INVALID_CONFIG;
        }

        pc_param += ul_off_set;
    }

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PHY_STAT_EN, OAL_SIZEOF(st_phy_stat_node_idx));

    /* ��д��Ϣ�壬���� */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, &st_phy_stat_node_idx,
        OAL_SIZEOF(st_phy_stat_node_idx)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_set_phy_stat_en::memcpy_s failed!");
        return OAL_FAIL;
    }

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_phy_stat_node_idx),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_phy_stat_en::wal_send_cfg_event return err[%d]!}", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_dbb_env_param(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint8 uc_param;

    /* sh hipriv.sh "Hisilicon0 dbb_env_param 0|1" */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dbb_env_param::wal_get_cmd_one_arg return err[%d]!}", ul_ret);
        return ul_ret;
    }

    uc_param = (oal_uint8)oal_atoi(ac_name);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DBB_ENV_PARAM, OAL_SIZEOF(uc_param));

    /* ��д��Ϣ�壬���� */
    st_write_msg.auc_value[0] = uc_param;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(uc_param),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dbb_env_param::wal_send_cfg_event return err code [%d]!}", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_usr_queue_stat(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_usr_queue_param_stru st_usr_queue_param;

    /* sh hipriv.sh "vap_name usr_queue_stat XX:XX:XX:XX:XX:XX 0|1" */
    memset_s((oal_uint8 *)&st_write_msg, OAL_SIZEOF(wal_msg_write_stru), 0, OAL_SIZEOF(wal_msg_write_stru));
    memset_s((oal_uint8 *)&st_usr_queue_param, OAL_SIZEOF(mac_cfg_usr_queue_param_stru), 0,
        OAL_SIZEOF(mac_cfg_usr_queue_param_stru));

    /* ��ȡ�û�mac��ַ */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, st_usr_queue_param.auc_user_macaddr, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_usr_queue_stat::wal_hipriv_get_mac_addr return [%d].}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_usr_queue_stat::wal_get_cmd_one_arg return err[%d]!}", ul_ret);
        return ul_ret;
    }

    st_usr_queue_param.uc_param = (oal_uint8)oal_atoi(ac_name);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_USR_QUEUE_STAT, OAL_SIZEOF(st_usr_queue_param));

    /* ��д��Ϣ�壬���� */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, &st_usr_queue_param, OAL_SIZEOF(st_usr_queue_param)) !=
        EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_usr_queue_stat::memcpy_s failed!");
        return OAL_FAIL;
    }

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_usr_queue_param),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_usr_queue_stat::wal_send_cfg_event return err code [%d]!}", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_report_vap_stat(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint8 uc_param;

    /* sh hipriv.sh "vap_name vap _stat  0|1" */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_vap_stat::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    uc_param = (oal_uint8)oal_atoi(ac_name);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_VAP_STAT, OAL_SIZEOF(uc_param));

    /* ��д��Ϣ�壬���� */
    st_write_msg.auc_value[0] = uc_param;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(uc_param),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_vap_stat::wal_send_cfg_event return err code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_report_all_stat(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint16 us_len;

    /* sh hipriv.sh "Hisilicon0 reprt_all_stat type(phy/machw/mgmt/irq/all)  0|1" */
    /* ��ȡrepot���� */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, pc_param, OAL_STRLEN(pc_param)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_report_all_stat::memcpy_s failed!");
        return OAL_FAIL;
    }
    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';
    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);
    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ALL_STAT, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_all_stat::wal_send_cfg_event return err code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif

#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32 wal_hipriv_report_ampdu_stat(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_ampdu_stat_stru st_ampdu_param;

    /* sh hipriv.sh "vap_name ampdu_stat XX:XX:XX:XX:XX:XX tid_no 0|1" */
    /* ��ȡ�û�mac��ַ */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, st_ampdu_param.auc_user_macaddr, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_ampdu_stat::wal_hipriv_get_mac_addr return [%d].}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_ampdu_stat::wal_get_cmd_one_arg return err_code [%d]!}",
            ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    st_ampdu_param.uc_tid_no = (oal_uint8)oal_atoi(ac_name);
    if (st_ampdu_param.uc_tid_no > WLAN_TID_MAX_NUM) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_ampdu_stat::input tid_no invalid. tid_no = [%d]!}\r\n",
            st_ampdu_param.uc_tid_no);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_ampdu_stat::wal_get_cmd_one_arg return err_code [%d]!}",
            ul_ret);
        return ul_ret;
    }

    st_ampdu_param.uc_param = (oal_uint8)oal_atoi(ac_name);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_REPORT_AMPDU_STAT, OAL_SIZEOF(st_ampdu_param));

    /* ��д��Ϣ�壬���� */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, &st_ampdu_param, OAL_SIZEOF(st_ampdu_param)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_report_ampdu_stat::memcpy_s failed!");
        return OAL_FAIL;
    }

    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_ampdu_param),
                               (oal_uint8 *)&st_write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_ampdu_stat::wal_send_cfg_event return err code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif


OAL_STATIC oal_uint32 wal_hipriv_set_ampdu_aggr_num(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    mac_cfg_aggr_num_stru st_aggr_num_ctl = { 0 };
    oal_uint32 ul_ret;
    oal_int32 l_ret;

    memset_s((oal_uint8 *)&st_write_msg, OAL_SIZEOF(st_write_msg), 0, OAL_SIZEOF(st_write_msg));
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_ampdu_aggr_num::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    st_aggr_num_ctl.uc_aggr_num_switch = (oal_uint8)oal_atoi(ac_name);
    if (st_aggr_num_ctl.uc_aggr_num_switch == 0) {
        /* ��ָ���ۺϸ���ʱ���ۺϸ����ָ�Ϊ0 */
        st_aggr_num_ctl.uc_aggr_num = 0;
    } else {
        /* ��ȡ�ۺϸ��� */
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY,
                "{wal_hipriv_set_ampdu_aggr_num::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }

        st_aggr_num_ctl.uc_aggr_num = (oal_uint8)oal_atoi(ac_name);

        /* �����ۺ���������ж� */
        if (st_aggr_num_ctl.uc_aggr_num > WLAN_AMPDU_TX_MAX_NUM) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ampdu_aggr_num::exceed max aggr num [%d]!}\r\n",
                st_aggr_num_ctl.uc_aggr_num);
            return ul_ret;
        }
    }

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_AGGR_NUM, OAL_SIZEOF(st_aggr_num_ctl));

    /* ��д��Ϣ�壬���� */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, &st_aggr_num_ctl, OAL_SIZEOF(st_aggr_num_ctl)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_set_ampdu_aggr_num::memcpy_s failed!");
        return OAL_FAIL;
    }

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_aggr_num_ctl),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ampdu_aggr_num::wal_send_cfg_event return err code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32 wal_hipriv_freq_adjust(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_int8 *pc_token = OAL_PTR_NULL;
    oal_int8 *pc_end = OAL_PTR_NULL;
    oal_int8 *pc_ctx = OAL_PTR_NULL;
    oal_int8 *pc_sep = " ";
    mac_cfg_freq_adjust_stru st_freq_adjust_ctl;

    /* ��ȡ������Ƶ */
    pc_token = oal_strtok(pc_param, pc_sep, &pc_ctx);
    if (pc_token == NULL) {
        return OAL_FAIL;
    }

    st_freq_adjust_ctl.us_pll_int = (oal_uint16)oal_strtol(pc_token, &pc_end, 16);

    /* ��ȡС����Ƶ */
    pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
    if (pc_token == NULL) {
        return OAL_FAIL;
    }

    st_freq_adjust_ctl.us_pll_frac = (oal_uint16)oal_strtol(pc_token, &pc_end, 16);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_FREQ_ADJUST, OAL_SIZEOF(st_freq_adjust_ctl));

    /* ��д��Ϣ�壬���� */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, &st_freq_adjust_ctl, OAL_SIZEOF(st_freq_adjust_ctl)) !=
        EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_freq_adjust::memcpy_s failed!");
        return OAL_FAIL;
    }

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_freq_adjust_ctl),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_freq_adjust::wal_send_cfg_event return err code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32 wal_hipriv_set_stbc_cap(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint32 ul_value;

    if (OAL_UNLIKELY((pst_cfg_net_dev == OAL_PTR_NULL) || (pc_param == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY,
            "{wal_hipriv_set_stbc_cap::pst_cfg_net_dev or pc_param null ptr error}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* STBC���ÿ��ص�����: hipriv "vap0 set_stbc_cap 0 | 1" �˴���������"1"��"0"����ac_name */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_stbc_cap::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    /* ��Խ������Ĳ�ͬ�������TDLS���ÿ��� */
    if ((oal_strcmp("0", ac_name)) == 0) {
        ul_value = 0;
    } else if ((oal_strcmp("1", ac_name)) == 0) {
        ul_value = 1;
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_stbc_cap::the set stbc command is error!}");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_STBC_CAP, OAL_SIZEOF(oal_uint32));

    /* ��������������� */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_value;

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_stbc_cap::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_ldpc_cap(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint32 ul_value;

    if (OAL_UNLIKELY((pst_cfg_net_dev == OAL_PTR_NULL) || (pc_param == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY,
            "{wal_hipriv_set_ldpc_cap::pst_cfg_net_dev or pc_param null ptr error}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* LDPC���ÿ��ص�����: hipriv "vap0 set_ldpc_cap 0 | 1" �˴���������"1"��"0"����ac_name */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ldpc_cap::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    /* ��Խ������Ĳ�ͬ�������TDLS���ÿ��� */
    if ((oal_strcmp("0", ac_name)) == 0) {
        ul_value = 0;
    } else if ((oal_strcmp("1", ac_name)) == 0) {
        ul_value = 1;
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_ldpc_cap::the set ldpc command is error}");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_LDPC_CAP, OAL_SIZEOF(oal_uint32));

    /* ��������������� */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_value;

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ldpc_cap::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)

oal_uint32 wal_set_ldpc_cap(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param, oal_uint32 ul_len)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;

    if (OAL_UNLIKELY((pst_cfg_net_dev == OAL_PTR_NULL) || (pc_param == OAL_PTR_NULL))) {
        OAM_ERROR_LOG2(0, OAM_SF_ANY,
            "{wal_hipriv_set_ldpc_cap::pst_cfg_net_dev or pc_param null ptr error %d, %d!}\r\n", pst_cfg_net_dev,
            pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_LDPC_CAP, ul_len);
    if (memcpy_s(&(st_write_msg.auc_value), WAL_MSG_WRITE_MAX_LEN, pc_param, ul_len) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_set_ldpc_cap::memcpy_s failed!");
        return OAL_FAIL;
    }
    /* **************************************************************************
                     ���¼���wal�㴦��
    ***************************************************************************/
    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + ul_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ldpc_cap::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


oal_uint32 wal_hipriv_dump_rx_dscr(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint32 ul_value;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_rx_dscr::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    ul_value = (oal_uint8)oal_atoi(ac_name);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DUMP_RX_DSCR, OAL_SIZEOF(oal_uint32));

    /* ��������������� */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_value;

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_rx_dscr::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_dump_tx_dscr(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint32 ul_value;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_tx_dscr::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    ul_value = (oal_uint8)oal_atoi(ac_name);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DUMP_TX_DSCR, OAL_SIZEOF(oal_uint32));

    /* ��������������� */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_value;

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_tx_dscr::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_dump_memory(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_addr[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int8 ac_len[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint32 ul_len;
    oal_uint32 ul_addr;
    mac_cfg_dump_memory_stru *pst_cfg = OAL_PTR_NULL;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_addr, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_memory::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    pc_param += ul_off_set;
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_len, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_memory::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    /* ��ַ�ַ���ת��16λ��ַ */
    ul_addr = (oal_uint32)oal_strtol(ac_addr, 0, 16);
    ul_len = (oal_uint32)oal_atoi(ac_len);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DUMP_MEMORY, OAL_SIZEOF(mac_cfg_dump_memory_stru));

    /* ��������������� */
    pst_cfg = (mac_cfg_dump_memory_stru *)(st_write_msg.auc_value);

    pst_cfg->ul_addr = ul_addr;
    pst_cfg->ul_len = ul_len;

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_dump_memory_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_memory::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_show_tx_dscr_addr(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param)
{
#ifdef _PRE_DEBUG_MODE
    oal_uint32 ul_mem_idx;
    oal_uint16 tx_dscr_idx;
    oal_mempool_tx_dscr_addr *pst_tx_dscr_addr = oal_mem_get_tx_dscr_addr();

    /* ��μ�� */
    if (OAL_UNLIKELY(pst_cfg_net_dev == OAL_PTR_NULL) || OAL_UNLIKELY(pc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_show_tx_dscr_addr::pst_net_dev or pc_param null "
            "ptr error [%d] [%d]!}\r\n", pst_cfg_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_tx_dscr_addr == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_show_tx_dscr_addr::pst_tx_dscr_addr is NULL!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_IO_PRINT("Allocated addr\n");
    for (ul_mem_idx = 0; ul_mem_idx < pst_tx_dscr_addr->us_tx_dscr_cnt; ul_mem_idx++) {
        OAL_IO_PRINT("[%d]0x%x\t", ul_mem_idx, (oal_uint32)pst_tx_dscr_addr->ul_tx_dscr_addr[ul_mem_idx]);
    }
    OAL_IO_PRINT("\n");

    OAL_IO_PRINT("Released addr\n");
    for (ul_mem_idx = 0; ul_mem_idx < OAL_TX_DSCR_ITEM_NUM; ul_mem_idx++) {
        if (pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_released_addr != 0) {
            OAL_IO_PRINT("Addr:0x%x\tFileId:%d\tLineNum:%d\tTimeStamp:%u\n",
                (oal_uint32)pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_released_addr,
                pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_release_file_id,
                pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_release_line_num,
                pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_release_ts);
        }
    }

    OAL_IO_PRINT("Tx complete int:\n");
    for (ul_mem_idx = 0; ul_mem_idx < OAL_TX_DSCR_ITEM_NUM; ul_mem_idx++) {
        if (pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_tx_dscr_in_up_intr != 0) {
            OAL_IO_PRINT("Up tx addr:0x%x\tts:%u  |  Dn tx addr:0x%x\tts:%u\n",
                pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_tx_dscr_in_up_intr,
                pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_up_intr_ts,
                pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_tx_dscr_in_dn_intr,
                pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_dn_intr_ts);
            OAL_IO_PRINT("tx dscr in q[%d] mpdu_num[%d]:", pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].uc_q_num,
                pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].uc_mpdu_num);
            for (tx_dscr_idx = 0; tx_dscr_idx < OAL_MAX_TX_DSCR_CNT_IN_LIST &&
                pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_tx_dscr_in_q[tx_dscr_idx] != 0; tx_dscr_idx++) {
                OAL_IO_PRINT("0x%x\t", pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_tx_dscr_in_q[tx_dscr_idx]);
            }
            OAL_IO_PRINT("\n-------------------------------------------\n");
        }
    }
#endif
    return OAL_SUCC;
}


oal_uint32 wal_hipriv_reg_write(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint16 us_len;

    /* **************************************************************************
                              ���¼���wal�㴦��
    ************************************************************************** */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, pc_param, OAL_STRLEN(pc_param)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_reg_write::memcpy_s failed!");
        return OAL_FAIL;
    }

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_REG_WRITE, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_reg_write::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_dump_ba_bitmap(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_mpdu_ampdu_tx_param_stru *pst_aggr_tx_on_param = OAL_PTR_NULL;
    oal_uint8 uc_tid;
    oal_uint8 auc_ra_addr[WLAN_MAC_ADDR_LEN] = {0};

    /* ��ȡtid */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_ba_bitmap::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    uc_tid = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* ��ȡMAC��ַ�ַ��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_ba_bitmap::get mac err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    /* ��ַ�ַ���ת��ַ���� */
    oal_strtoaddr(ac_name, sizeof(ac_name), auc_ra_addr, WLAN_MAC_ADDR_LEN);
    pc_param += ul_off_set;

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DUMP_BA_BITMAP, OAL_SIZEOF(mac_cfg_mpdu_ampdu_tx_param_stru));

    /* ��������������� */
    pst_aggr_tx_on_param = (mac_cfg_mpdu_ampdu_tx_param_stru *)(st_write_msg.auc_value);
    pst_aggr_tx_on_param->uc_tid = uc_tid;
    oal_set_mac_addr(pst_aggr_tx_on_param->auc_ra_mac, auc_ra_addr);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_mpdu_ampdu_tx_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_ba_bitmap::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32 wal_hipriv_alg(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg; // st_write_msg can only carry bytes less than 48
    oal_int32 l_ret;
    oal_uint32 ul_off_set;
    oal_int8 ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int8 *pc_tmp = (oal_int8 *)pc_param;
    oal_uint16 us_config_len;
    oal_uint16 us_param_len;
    oal_uint32 l_memcpy_ret = EOK;

    mac_ioctl_alg_config_stru st_alg_config;

    st_alg_config.uc_argc = 0;
    while (wal_get_cmd_one_arg(pc_tmp, ac_arg, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set) == OAL_SUCC) {
        st_alg_config.auc_argv_offset[st_alg_config.uc_argc] =
            (oal_uint8)((oal_uint8)(pc_tmp - pc_param) + (oal_uint8)ul_off_set - (oal_uint8)OAL_STRLEN(ac_arg));
        pc_tmp += ul_off_set;
        st_alg_config.uc_argc++;

        if (st_alg_config.uc_argc > DMAC_ALG_CONFIG_MAX_ARG) {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_alg::wal_hipriv_alg error, argc too big [%d]!}",
                st_alg_config.uc_argc);
            return OAL_FAIL;
        }
    }

    if (st_alg_config.uc_argc == 0) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_alg::argc=0!}\r\n");
        return OAL_FAIL;
    }

    us_param_len = (oal_uint16)OAL_STRLEN(pc_param);
    if (us_param_len > WAL_MSG_WRITE_MAX_LEN - 1 - OAL_SIZEOF(mac_ioctl_alg_config_stru)) {
        return OAL_FAIL;
    }

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    us_config_len = OAL_SIZEOF(mac_ioctl_alg_config_stru) + us_param_len + 1;
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ALG, us_config_len);
    l_memcpy_ret += memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN,
        &st_alg_config, OAL_SIZEOF(mac_ioctl_alg_config_stru));
    l_memcpy_ret += memcpy_s(st_write_msg.auc_value + OAL_SIZEOF(mac_ioctl_alg_config_stru),
        WAL_MSG_WRITE_MAX_LEN - OAL_SIZEOF(mac_ioctl_alg_config_stru), pc_param, us_param_len + 1);
    if (l_memcpy_ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_alg::memcpy_s failed!");
        return OAL_FAIL;
    }
    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + us_config_len,
                               (oal_uint8 *)&st_write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_alg::wal_send_cfg_event return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#ifdef _PRE_WLAN_FEATURE_DFS

OAL_STATIC oal_uint32 wal_hipriv_dfs_radartool(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint16 us_len;
    oal_int32 l_ret;

    if (OAL_UNLIKELY(OAL_STRLEN(pc_param) >= WAL_MSG_WRITE_MAX_LEN)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dfs_radartool:: pc_param overlength is %d}\n",
            OAL_STRLEN(pc_param));
        oal_print_hex_dump((oal_uint8 *)pc_param, WAL_MSG_WRITE_MAX_LEN, 32,
            "wal_hipriv_dfs_radartool: param is overlong:");
        return OAL_FAIL;
    }

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, pc_param, OAL_STRLEN(pc_param)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_dfs_radartool::memcpy_s failed!");
        return OAL_FAIL;
    }

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RADARTOOL, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dfs_radartool::return err code [%d]!}\r\n", l_ret);

        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif /* end of _PRE_WLAN_FEATURE_DFS */
#ifdef _PRE_SUPPORT_ACS

OAL_STATIC oal_uint32 wal_hipriv_acs(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint16 us_len;
    oal_int32 l_ret;

    if (OAL_UNLIKELY(OAL_STRLEN(pc_param) >= WAL_MSG_WRITE_MAX_LEN)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_acs:: pc_param overlength is %d}\n", OAL_STRLEN(pc_param));
        oal_print_hex_dump((oal_uint8 *)pc_param, WAL_MSG_WRITE_MAX_LEN, 32, "wal_hipriv_acs: param is overlong:");
        return OAL_FAIL;
    }

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, pc_param, OAL_STRLEN(pc_param)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_acs::memcpy_s failed!");
        return OAL_FAIL;
    }

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ACS_CONFIG, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_acs::return err code [%d]!}\r\n", l_ret);

        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32 wal_hipriv_show_stat_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oam_stats_report_info_to_sdt(OAM_OTA_TYPE_DEV_STAT_INFO);
    oam_stats_report_info_to_sdt(OAM_OTA_TYPE_VAP_STAT_INFO);
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_show_vap_pkt_stat(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;

    /* **************************************************************************
                                 ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_VAP_PKT_STAT, OAL_SIZEOF(oal_uint32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_vap_pkt_stat::wal_send_cfg_event return err code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_cca_opt_log(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)

{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    mac_ioctl_alg_cca_opt_log_param_stru *pst_alg_cca_opt_log_param = OAL_PTR_NULL;
    wal_ioctl_alg_cfg_stru st_alg_cfg;
    oal_uint8 uc_map_index = 0;
    oal_int32 l_ret;

    pst_alg_cca_opt_log_param = (mac_ioctl_alg_cca_opt_log_param_stru *)(st_write_msg.auc_value);

    /* ��ȡ���ò������� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_cca_opt_log::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    pc_param = pc_param + ul_off_set;

    /* Ѱ��ƥ������� */
    st_alg_cfg = g_ast_alg_cfg_map[0];
    while (st_alg_cfg.pc_name != OAL_PTR_NULL) {
        if (oal_strcmp(st_alg_cfg.pc_name, ac_name) == 0) {
            break;
        }
        st_alg_cfg = g_ast_alg_cfg_map[++uc_map_index];
    }

    /* û���ҵ���Ӧ������򱨴� */
    if (st_alg_cfg.pc_name == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_cca_opt_log::invalid alg_cfg command!}\r\n");
        return OAL_FAIL;
    }

    /* ��¼�����Ӧ��ö��ֵ */
    pst_alg_cca_opt_log_param->en_alg_cfg = g_ast_alg_cfg_map[uc_map_index].en_alg_cfg;

    /* ���ֻ�ȡ�ض�֡���ʺ�ͳ����־�����:��ȡ����ֻ���ȡ֡���� */
    if (pst_alg_cca_opt_log_param->en_alg_cfg == MAC_ALG_CFG_CCA_OPT_STAT_LOG_START) {
        /* ��ȡ���ò������� */
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_cca_opt_log::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
                ul_ret);
            return ul_ret;
        }

        /* ��¼���� */
        pst_alg_cca_opt_log_param->us_value = (oal_uint16)oal_atoi(ac_name);
    }

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ALG_PARAM, OAL_SIZEOF(mac_ioctl_alg_cca_opt_log_param_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_ioctl_alg_cca_opt_log_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_clear_stat_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oam_stats_clear_stat_info();
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_user_stat_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_int32 l_tmp;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;

    /* sh hipriv.sh "Hisilicon0 usr_stat_info usr_id" */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_user_stat_info::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    l_tmp = oal_atoi(ac_name);
    if ((l_tmp < 0) || (l_tmp >= WLAN_ACTIVE_USER_MAX_NUM)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_user_stat_info::user id invalid [%d]!}\r\n", l_tmp);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    oam_stats_report_usr_info((oal_uint16)l_tmp);
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_timer_start(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint8 uc_timer_switch;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_timer_start::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);

        return ul_ret;
    }

    uc_timer_switch = (oal_uint8)oal_atoi(ac_name);
    if (uc_timer_switch >= 2) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_timer_start::invalid choicee [%d]!}\r\n", uc_timer_switch);
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TIMER_START, OAL_SIZEOF(oal_uint8));

    /* ��������������� */
    *((oal_uint8 *)(st_write_msg.auc_value)) = uc_timer_switch;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_timer_start::wal_send_cfg_event return err code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_DFR

OAL_STATIC oal_uint32 wal_hipriv_test_dfr_start(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32 ul_cfg_rst;
    oal_wireless_dev_stru *pst_wdev = OAL_PTR_NULL;
    mac_wiphy_priv_stru *pst_wiphy_priv = OAL_PTR_NULL;
    mac_device_stru *pst_mac_device = OAL_PTR_NULL;

    pst_wdev = OAL_NETDEVICE_WDEV(pst_net_dev);
    if (pst_wdev == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_test_dfr_start::pst_wdev is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_wiphy_priv = (mac_wiphy_priv_stru *)oal_wiphy_priv(pst_wdev->wiphy);
    if (pst_wiphy_priv == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_test_dfr_start::pst_wiphy_priv is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = pst_wiphy_priv->pst_mac_device;
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_test_dfr_start::pst_mac_device is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }
    g_st_dfr_info.bit_device_reset_enable = OAL_TRUE;
    g_st_dfr_info.bit_device_reset_process_flag = OAL_FALSE;

    ul_cfg_rst = wal_dfr_excp_rx(pst_mac_device->uc_device_id, 0);
    if (OAL_UNLIKELY(ul_cfg_rst != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_test_dfr_start::wal_send_cfg_event return err_code [%d]!}\r\n",
            ul_cfg_rst);
        return ul_cfg_rst;
    }
    return OAL_SUCC;
}

#endif // _PRE_WLAN_FEATURE_DFR


OAL_STATIC oal_uint32 wal_hipriv_alg_cfg(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    mac_ioctl_alg_param_stru *pst_alg_param = OAL_PTR_NULL;
    wal_ioctl_alg_cfg_stru st_alg_cfg;
    oal_uint8 uc_map_index = 0;
    oal_int32 l_ret;

    pst_alg_param = (mac_ioctl_alg_param_stru *)(st_write_msg.auc_value);

    /* ��ȡ���ò������� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_alg_cfg::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* Ѱ��ƥ������� */
    st_alg_cfg = g_ast_alg_cfg_map[0];
    while (st_alg_cfg.pc_name != OAL_PTR_NULL) {
        if (oal_strcmp(st_alg_cfg.pc_name, ac_name) == 0) {
            break;
        }
        st_alg_cfg = g_ast_alg_cfg_map[++uc_map_index];
    }

    /* û���ҵ���Ӧ������򱨴� */
    if (st_alg_cfg.pc_name == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_alg_cfg::invalid alg_cfg command!}\r\n");
        return OAL_FAIL;
    }

    /* ��¼�����Ӧ��ö��ֵ */
    pst_alg_param->en_alg_cfg = g_ast_alg_cfg_map[uc_map_index].en_alg_cfg;

    /* ��ȡ��������ֵ */
    ul_ret = wal_get_cmd_one_arg(pc_param + ul_off_set, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_alg_cfg::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* ��¼��������ֵ */
    pst_alg_param->ul_value = (oal_uint32)oal_atoi(ac_name);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ALG_PARAM, OAL_SIZEOF(mac_ioctl_alg_param_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_ioctl_alg_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_tpc_log(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_ioctl_alg_tpc_log_param_stru *pst_alg_tpc_log_param = OAL_PTR_NULL;
    wal_ioctl_alg_cfg_stru st_alg_cfg;
    oal_uint8 uc_map_index = 0;
    oal_bool_enum_uint8 en_stop_flag = OAL_FALSE;

    pst_alg_tpc_log_param = (mac_ioctl_alg_tpc_log_param_stru *)(st_write_msg.auc_value);

    /* ��ȡ���ò������� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_tpc_log::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param = pc_param + ul_off_set;

    /* Ѱ��ƥ������� */
    st_alg_cfg = g_ast_alg_cfg_map[0];
    while (st_alg_cfg.pc_name != OAL_PTR_NULL) {
        if (oal_strcmp(st_alg_cfg.pc_name, ac_name) == 0) {
            break;
        }
        st_alg_cfg = g_ast_alg_cfg_map[++uc_map_index];
    }

    /* û���ҵ���Ӧ������򱨴� */
    if (st_alg_cfg.pc_name == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_tpc_log::invalid alg_cfg command!}\r\n");
        return OAL_FAIL;
    }

    /* ��¼�����Ӧ��ö��ֵ */
    pst_alg_tpc_log_param->en_alg_cfg = g_ast_alg_cfg_map[uc_map_index].en_alg_cfg;

    /* ���ֻ�ȡ�ض�֡���ʺ�ͳ����־�����:��ȡ����ֻ���ȡ֡���� */
    if (pst_alg_tpc_log_param->en_alg_cfg == MAC_ALG_CFG_TPC_GET_FRAME_POW) {
        /* ��ȡ���ò������� */
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_tpc_log::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
                ul_ret);
            return ul_ret;
        }
        /* ��¼�����Ӧ��֡���� */
        pst_alg_tpc_log_param->pc_frame_name = ac_name;
    } else {
        ul_ret = wal_hipriv_get_mac_addr(pc_param, pst_alg_tpc_log_param->auc_mac_addr, &ul_off_set);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_tpc_log::wal_hipriv_get_mac_addr failed!}\r\n");
            return ul_ret;
        }
        pc_param += ul_off_set;

        while ((*pc_param == ' ') || (*pc_param == '\0')) {
            if (*pc_param == '\0') {
                en_stop_flag = OAL_TRUE;
                break;
            }
            ++pc_param;
        }

        /* ��ȡҵ������ֵ */
        if (en_stop_flag != OAL_TRUE) {
            ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
            if (ul_ret != OAL_SUCC) {
                OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_tpc_log::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
                    ul_ret);
                return ul_ret;
            }

            pst_alg_tpc_log_param->uc_ac_no = (oal_uint8)oal_atoi(ac_name);
            pc_param = pc_param + ul_off_set;

            en_stop_flag = OAL_FALSE;
            while ((*pc_param == ' ') || (*pc_param == '\0')) {
                if (*pc_param == '\0') {
                    en_stop_flag = OAL_TRUE;
                    break;
                }
                ++pc_param;
            }

            if (en_stop_flag != OAL_TRUE) {
                /* ��ȡ��������ֵ */
                ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
                if (ul_ret != OAL_SUCC) {
                    OAM_WARNING_LOG1(0, OAM_SF_ANY,
                        "{wal_hipriv_tpc_log::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
                    return ul_ret;
                }

                /* ��¼��������ֵ */
                pst_alg_tpc_log_param->us_value = (oal_uint16)oal_atoi(ac_name);
            }
        }
    }

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ALG_PARAM, OAL_SIZEOF(mac_ioctl_alg_tpc_log_param_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_ioctl_alg_tpc_log_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_ar_log(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    mac_ioctl_alg_ar_log_param_stru *pst_alg_ar_log_param = OAL_PTR_NULL;
    wal_ioctl_alg_cfg_stru st_alg_cfg;
    oal_uint8 uc_map_index = 0;
    oal_int32 l_ret;
    oal_bool_enum_uint8 en_stop_flag = OAL_FALSE;

    pst_alg_ar_log_param = (mac_ioctl_alg_ar_log_param_stru *)(st_write_msg.auc_value);

    /* ��ȡ���ò������� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ar_log::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param = pc_param + ul_off_set;

    /* Ѱ��ƥ������� */
    st_alg_cfg = g_ast_alg_cfg_map[0];
    while (st_alg_cfg.pc_name != OAL_PTR_NULL) {
        if (oal_strcmp(st_alg_cfg.pc_name, ac_name) == 0) {
            break;
        }
        st_alg_cfg = g_ast_alg_cfg_map[++uc_map_index];
    }

    /* û���ҵ���Ӧ������򱨴� */
    if (st_alg_cfg.pc_name == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ar_log::invalid alg_cfg command!}\r\n");
        return OAL_FAIL;
    }

    /* ��¼�����Ӧ��ö��ֵ */
    pst_alg_ar_log_param->en_alg_cfg = g_ast_alg_cfg_map[uc_map_index].en_alg_cfg;

    ul_ret = wal_hipriv_get_mac_addr(pc_param, pst_alg_ar_log_param->auc_mac_addr, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ar_log::wal_hipriv_get_mac_addr failed!}\r\n");
        return ul_ret;
    }
    pc_param += ul_off_set;

    while ((*pc_param == ' ') || (*pc_param == '\0')) {
        if (*pc_param == '\0') {
            en_stop_flag = OAL_TRUE;
            break;
        }
        ++pc_param;
    }

    /* ��ȡҵ������ֵ */
    if (en_stop_flag != OAL_TRUE) {
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ar_log::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
                ul_ret);
            return ul_ret;
        }

        pst_alg_ar_log_param->uc_ac_no = (oal_uint8)oal_atoi(ac_name);
        pc_param = pc_param + ul_off_set;

        en_stop_flag = OAL_FALSE;
        while ((*pc_param == ' ') || (*pc_param == '\0')) {
            if (*pc_param == '\0') {
                en_stop_flag = OAL_TRUE;
                break;
            }
            ++pc_param;
        }

        if (en_stop_flag != OAL_TRUE) {
            /* ��ȡ��������ֵ */
            ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
            if (ul_ret != OAL_SUCC) {
                OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ar_log::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
                    ul_ret);
                return ul_ret;
            }

            /* ��¼��������ֵ */
            pst_alg_ar_log_param->us_value = (oal_uint16)oal_atoi(ac_name);
        }
    }

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ALG_PARAM, OAL_SIZEOF(mac_ioctl_alg_ar_log_param_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_ioctl_alg_ar_log_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_ar_test(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_offset;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    mac_ioctl_alg_ar_test_param_stru *pst_alg_ar_test_param = OAL_PTR_NULL;
    wal_ioctl_alg_cfg_stru st_alg_cfg;
    oal_uint8 uc_map_index = 0;
    oal_int32 l_ret;

    pst_alg_ar_test_param = (mac_ioctl_alg_ar_test_param_stru *)(st_write_msg.auc_value);

    /* ��ȡ���ò������� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_offset);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ar_test::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param = pc_param + ul_offset;

    /* Ѱ��ƥ������� */
    st_alg_cfg = g_ast_alg_cfg_map[0];
    while (st_alg_cfg.pc_name != OAL_PTR_NULL) {
        if (oal_strcmp(st_alg_cfg.pc_name, ac_name) == 0) {
            break;
        }
        st_alg_cfg = g_ast_alg_cfg_map[++uc_map_index];
    }

    /* û���ҵ���Ӧ������򱨴� */
    if (st_alg_cfg.pc_name == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ar_test::invalid alg_cfg command!}\r\n");
        return OAL_FAIL;
    }

    /* ��¼�����Ӧ��ö��ֵ */
    pst_alg_ar_test_param->en_alg_cfg = g_ast_alg_cfg_map[uc_map_index].en_alg_cfg;

    ul_ret = wal_hipriv_get_mac_addr(pc_param, pst_alg_ar_test_param->auc_mac_addr, &ul_offset);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ar_test::wal_hipriv_get_mac_addr failed!}\r\n");
        return ul_ret;
    }
    pc_param += ul_offset;

    /* ��ȡ��������ֵ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_offset);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ar_test::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* ��¼��������ֵ */
    pst_alg_ar_test_param->us_value = (oal_uint16)oal_atoi(ac_name);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ALG_PARAM, OAL_SIZEOF(mac_ioctl_alg_ar_test_param_stru));
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_ioctl_alg_ar_test_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE


OAL_STATIC oal_uint32 wal_hipriv_dev_customize_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;

    /* **************************************************************************
        ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SHOW_DEV_CUSTOMIZE_INFOS, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dev_customize_info::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

#if ((_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1131C_DEV) && (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1131C_HOST))


OAL_STATIC oal_int32 wal_ioctl_set_auth_mode(oal_net_device_stru *pst_net_dev,
    oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    oal_uint16 us_len = sizeof(oal_uint32);
    oal_uint32 ul_ret;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_auth_mode::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr!}\r\n");
        return -OAL_EINVAL;
    }

    ul_ret = hmac_config_set_auth_mode(pst_mac_vap, us_len, (oal_uint8 *)&(pst_ioctl_data->pri_data.auth_params));
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_ioctl_set_auth_mode::return err code [%d]!}\r\n",
            ul_ret);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_int32 wal_ioctl_set_max_user(oal_net_device_stru *pst_net_dev,
    oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    oal_uint16 us_len = sizeof(oal_uint32);
    oal_uint32 ul_ret;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_max_user::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr!}\r\n");
        return -OAL_EINVAL;
    }

    ul_ret = hmac_config_set_max_user(pst_mac_vap, us_len, (pst_ioctl_data->pri_data.ul_vap_max_user));
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_ioctl_set_max_user::hmac_config_set_max_user return err code [%d]!}\r\n", ul_ret);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}


oal_int32 wal_ioctl_set_ssid(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    oal_uint16 us_len;
    oal_uint32 ul_ret;
    mac_cfg_ssid_param_stru st_ssid_param;
    oal_uint8 uc_ssid_temp;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_ssid::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr!}\r\n");
        return -OAL_EINVAL;
    }
    /* ��֤�����strlen������� */
    pst_ioctl_data->pri_data.ssid[OAL_IEEE80211_MAX_SSID_LEN + 3] = '\0';
    uc_ssid_temp = (oal_uint8)OAL_STRLEN((oal_int8 *)pst_ioctl_data->pri_data.ssid);
    us_len = OAL_MIN(uc_ssid_temp, OAL_IEEE80211_MAX_SSID_LEN);

    st_ssid_param.uc_ssid_len = (oal_uint8)us_len;
    if (memcpy_s(st_ssid_param.ac_ssid, WLAN_SSID_MAX_LEN, (const oal_void *)(pst_ioctl_data->pri_data.ssid),
        (oal_uint32)us_len) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_ioctl_set_ssid::memcpy_s failed!");
        return OAL_FAIL;
    }
    ul_ret = hmac_config_set_ssid(pst_mac_vap, OAL_SIZEOF(st_ssid_param), (oal_uint8 *)&st_ssid_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_ioctl_set_ssid::hmac_config_set_ssid return err code [%d]!}\r\n", ul_ret);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}


oal_int32 wal_ioctl_set_country_code(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
#ifdef _PRE_WLAN_FEATURE_11D
    oal_int8 auc_country_code[4] = {0};
    oal_int32 l_ret;

    if (memcpy_s(auc_country_code, sizeof(auc_country_code), pst_ioctl_data->pri_data.country_code.auc_country_code,
        sizeof(auc_country_code) - 1) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_ioctl_set_country_code::memcpy_s failed!");
        return -OAL_EFAIL;
    }

#ifdef _PRE_WLAN_FEATURE_DFS // 1131_debug
    l_ret = wal_regdomain_update_for_dfs(pst_net_dev, auc_country_code);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_country_code::return err code [%d]!}\r\n", l_ret);
        return -OAL_EFAIL;
    }
#endif

    l_ret = wal_regdomain_update(pst_net_dev, auc_country_code);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_country_code::return err code [%d]!}\r\n", l_ret);
        return -OAL_EFAIL;
    }

#else
    OAM_INFO_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_country_code::_PRE_WLAN_FEATURE_11D is not define!}\r\n");
#endif

    return OAL_SUCC;
}


static int32_t set_priv_connect_param(mac_cfg80211_connect_param_stru *cfg80211_connect_param,
                                      const oal_net_dev_ioctl_data_stru *ioctl_data)
{
    uint8_t uc_loop;
    uint8_t uc_pairwise_cipher_num;
    uint8_t uc_akm_suite_num;
    /* ���ü��ܲ��� */
    if (ioctl_data->pri_data.cfg80211_connect_params.en_privacy != 0) {
        if ((ioctl_data->pri_data.cfg80211_connect_params.uc_wep_key_len != 0) &&
            (ioctl_data->pri_data.cfg80211_connect_params.st_crypto.n_akm_suites == 0)) {
            /* ����wep������Ϣ */
            cfg80211_connect_param.puc_wep_key = ioctl_data->pri_data.cfg80211_connect_params.puc_wep_key;
            cfg80211_connect_param.uc_wep_key_len =
                ioctl_data->pri_data.cfg80211_connect_params.uc_wep_key_len;
            cfg80211_connect_param.uc_wep_key_index =
                ioctl_data->pri_data.cfg80211_connect_params.uc_wep_key_index;
            cfg80211_connect_param.st_crypto.cipher_group =
                (oal_uint8)ioctl_data->pri_data.cfg80211_connect_params.st_crypto.cipher_group;
        } else if (ioctl_data->pri_data.cfg80211_connect_params.st_crypto.n_akm_suites != 0) {
            /* ����WPA/WPA2 ������Ϣ */
            cfg80211_connect_param.st_crypto.wpa_versions =
                (oal_uint8)ioctl_data->pri_data.cfg80211_connect_params.st_crypto.wpa_versions;
            cfg80211_connect_param.st_crypto.cipher_group =
                (oal_uint8)ioctl_data->pri_data.cfg80211_connect_params.st_crypto.cipher_group;
            cfg80211_connect_param.st_crypto.n_ciphers_pairwise =
                (oal_uint8)ioctl_data->pri_data.cfg80211_connect_params.st_crypto.n_ciphers_pairwise;
            cfg80211_connect_param.st_crypto.n_akm_suites =
                (oal_uint8)ioctl_data->pri_data.cfg80211_connect_params.st_crypto.n_akm_suites;
            cfg80211_connect_param.st_crypto.control_port =
                (oal_uint8)ioctl_data->pri_data.cfg80211_connect_params.st_crypto.control_port;

            uc_pairwise_cipher_num = cfg80211_connect_param.st_crypto.n_ciphers_pairwise;
            for (uc_loop = 0; uc_loop < uc_pairwise_cipher_num; uc_loop++) {
                cfg80211_connect_param.st_crypto.ciphers_pairwise[uc_loop] =
                    (oal_uint8)ioctl_data->pri_data.cfg80211_connect_params.st_crypto.ciphers_pairwise[uc_loop];
            }

            uc_akm_suite_num = cfg80211_connect_param.st_crypto.n_akm_suites;
            for (uc_loop = 0; uc_loop < uc_akm_suite_num; uc_loop++) {
                cfg80211_connect_param.st_crypto.akm_suites[uc_loop] =
                    (oal_uint8)ioctl_data->pri_data.cfg80211_connect_params.st_crypto.akm_suites[uc_loop];
            }
        } else {
            return -OAL_EFAIL;
        }
    }
    return OAL_SUCC;
}


oal_int32 wal_ioctl_nl80211_priv_connect(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    mac_cfg80211_connect_param_stru st_mac_cfg80211_connect_param;
    oal_int32 l_ret;

    /* ��ʼ���������Ӳ��� */
    memset_s(&st_mac_cfg80211_connect_param, OAL_SIZEOF(mac_cfg80211_connect_param_stru), 0,
        OAL_SIZEOF(mac_cfg80211_connect_param_stru));

    /* �����ں��·��� freq to channel_number eg.1,2,36,40...  */
    st_mac_cfg80211_connect_param.uc_channel =
        (oal_uint8)oal_ieee80211_frequency_to_channel(pst_ioctl_data->pri_data.cfg80211_connect_params.l_freq);

    /* �����ں��·��� ssid */
    st_mac_cfg80211_connect_param.puc_ssid = (oal_uint8 *)pst_ioctl_data->pri_data.cfg80211_connect_params.puc_ssid;
    st_mac_cfg80211_connect_param.uc_ssid_len = (oal_uint8)pst_ioctl_data->pri_data.cfg80211_connect_params.ssid_len;

    /* �����ں��·��� bssid */
    st_mac_cfg80211_connect_param.puc_bssid = (oal_uint8 *)pst_ioctl_data->pri_data.cfg80211_connect_params.puc_bssid;

    /* �����ں��·��İ�ȫ��ز��� */
    /* ��֤���� */
    st_mac_cfg80211_connect_param.en_auth_type = pst_ioctl_data->pri_data.cfg80211_connect_params.en_auth_type;

    /* �������� */
    st_mac_cfg80211_connect_param.en_privacy = pst_ioctl_data->pri_data.cfg80211_connect_params.en_privacy;

    /* IE�·� */
    st_mac_cfg80211_connect_param.puc_ie = pst_ioctl_data->pri_data.cfg80211_connect_params.puc_ie;
    st_mac_cfg80211_connect_param.ul_ie_len = (oal_uint32)pst_ioctl_data->pri_data.cfg80211_connect_params.ie_len;

    if (set_priv_connect_param(&st_mac_cfg80211_connect_param, pst_ioctl_data) != OAL_SUCC) {
        OAM_ERROR_LOG3(0, OAM_SF_ANY,
            "{wal_ioctl_nl80211_priv_connect::set_key fail! uc_wep_key_len[%d] n_akm_suites[%d]} puc_wep_key[0x%x]",
            pst_ioctl_data->pri_data.cfg80211_connect_params.uc_wep_key_len,
            pst_ioctl_data->pri_data.cfg80211_connect_params.st_crypto.n_akm_suites,
            pst_ioctl_data->pri_data.cfg80211_connect_params.puc_wep_key);
        return -OAL_EFAIL;
    }

    /* ���¼����������������� */
    l_ret = wal_cfg80211_start_connect(pst_net_dev, &st_mac_cfg80211_connect_param);
    if (l_ret != OAL_SUCC) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_ioctl_nl80211_priv_connect::wal_cfg80211_start_connect fail!}\r\n");
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}


oal_int32 wal_ioctl_nl80211_priv_disconnect(oal_net_device_stru *pst_net_dev,
    oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    mac_cfg_kick_user_param_stru st_mac_cfg_kick_user_param;
    oal_int32 l_ret;
    mac_user_stru *pst_mac_user = OAL_PTR_NULL;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;

    /* �����ں��·���connect���� */
    memset_s(&st_mac_cfg_kick_user_param, OAL_SIZEOF(mac_cfg_kick_user_param_stru), 0,
        OAL_SIZEOF(mac_cfg_kick_user_param_stru));

    /* �����ں��·���ȥ����ԭ��  */
    st_mac_cfg_kick_user_param.us_reason_code = pst_ioctl_data->pri_data.kick_user_params.us_reason_code;

    /* ��д��sta������ap mac ��ַ */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY,
            "{wal_ioctl_nl80211_priv_disconnect::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr.}\r\n");
        return -OAL_EINVAL;
    }
    pst_mac_user = mac_res_get_mac_user(pst_mac_vap->uc_assoc_vap_id);
    if (pst_mac_user == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_ioctl_nl80211_priv_disconnect:: mac_res_get_mac_user pst_mac_user  is nul!}\r\n");
        return OAL_SUCC;
    }

    if (memcpy_s(st_mac_cfg_kick_user_param.auc_mac_addr, WLAN_MAC_ADDR_LEN, pst_mac_user->auc_user_mac_addr,
        WLAN_MAC_ADDR_LEN) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_ioctl_nl80211_priv_disconnect::memcpy_s failed!");
        return -OAL_EFAIL;
    }

    l_ret = wal_cfg80211_start_disconnect(pst_net_dev, &st_mac_cfg_kick_user_param);
    if (l_ret != OAL_SUCC) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_ioctl_nl80211_priv_disconnect::wal_cfg80211_start_disconnect fail!}\r\n");
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}


oal_int32 wal_ioctl_set_channel(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    mac_cfg_channel_param_stru *pst_channel_param = OAL_PTR_NULL;
    wal_msg_write_stru st_write_msg;
    mac_device_stru *pst_device = OAL_PTR_NULL;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    oal_wiphy_stru *pst_wiphy = OAL_PTR_NULL;
    oal_ieee80211_channel_stru *pst_channel = OAL_PTR_NULL;
    wlan_channel_bandwidth_enum_uint8 en_bandwidth;
    oal_int32 l_freq;
    oal_int32 l_channel;
    oal_int32 l_sec_channel_offset;
    oal_int32 l_center_freq1;
    oal_int32 l_bandwidth;
    oal_int32 l_ret;
    wal_msg_stru *pst_rsp_msg = OAL_PTR_NULL;
    oal_uint32 ul_err_code;

    l_freq = pst_ioctl_data->pri_data.freq.l_freq;
    l_channel = pst_ioctl_data->pri_data.freq.l_channel;
    l_sec_channel_offset = pst_ioctl_data->pri_data.freq.l_sec_channel_offset;
    l_center_freq1 = pst_ioctl_data->pri_data.freq.l_center_freq1;
    l_bandwidth = pst_ioctl_data->pri_data.freq.l_bandwidth;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_channel::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr.}\r\n");
        return -OAL_EINVAL;
    }

    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_ioctl_set_channel::pst_device is null!}\r\n");
        return -OAL_EINVAL;
    }

    pst_wiphy = pst_device->pst_wiphy;
    pst_channel = oal_ieee80211_get_channel(pst_wiphy, l_freq);
    l_channel = pst_channel->hw_value;

    /* �ж��ŵ��ڲ��ڹ������� */
    l_ret = (oal_int32)mac_is_channel_num_valid(pst_channel->band, (oal_uint8)l_channel);
    if (l_ret != OAL_SUCC) {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_ioctl_set_channel::channel num is invalid. band, ch num [%d] [%d]!}\r\n", pst_channel->band,
            l_channel);
        return -OAL_EINVAL;
    }

    /* �����ں˴���ֵ��WITP ����ֵת�� */
    if (l_bandwidth == 80) {
        en_bandwidth =
            mac_get_bandwith_from_center_freq_seg0((oal_uint8)l_channel, (oal_uint8)((l_center_freq1 - 5000) / 5));
    } else if (l_bandwidth == 40) {
        switch (l_sec_channel_offset) {
            case -1:
                en_bandwidth = WLAN_BAND_WIDTH_40MINUS;
                break;
            case 1:
                en_bandwidth = WLAN_BAND_WIDTH_40PLUS;
                break;
            default:
                en_bandwidth = WLAN_BAND_WIDTH_20M;
                break;
        }
    } else {
        en_bandwidth = WLAN_BAND_WIDTH_20M;
    }

    /* **************************************************************************
        ���¼���wal�㴦��
    ************************************************************************** */
    /* ��д��Ϣ */
    pst_channel_param = (mac_cfg_channel_param_stru *)(st_write_msg.auc_value);
    pst_channel_param->uc_channel = (oal_uint8)pst_channel->hw_value;
    pst_channel_param->en_band = pst_channel->band;
    pst_channel_param->en_bandwidth = en_bandwidth;
#ifdef _PRE_WLAN_FEATURE_HILINK
    pst_channel_param->uc_hilink_flag = OAL_SWITCH_OFF;
#endif

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_CFG80211_SET_CHANNEL, OAL_SIZEOF(mac_cfg_channel_param_stru));

    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_channel_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);
    if ((l_ret != OAL_SUCC) || (pst_rsp_msg == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_channel:: wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* ��ȡ���صĴ����� */
    ul_err_code = wal_check_and_release_msg_resp(pst_rsp_msg);
    if (ul_err_code != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_channel::wal_send_cfg_event return err code:[%d]!}\r\n",
            ul_err_code);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}
#endif


oal_int32 wal_ioctl_set_wps_p2p_ie(oal_net_device_stru *pst_net_dev, const oal_uint8 *puc_buf, oal_uint32 ul_len,
    en_app_ie_type_uint8 en_type)
{
    wal_msg_write_stru st_write_msg;
    wal_msg_stru *pst_rsp_msg = OAL_PTR_NULL;
    oal_uint32 err_code;
    oal_app_ie_stru wps_p2p_ie;
    oal_int32 l_ret;
    oal_uint32 l_memcpy_ret = EOK;
    mac_vap_stru *mac_vap = NULL;

    if (ul_len > WLAN_WPS_IE_MAX_SIZE) {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_ioctl_set_wps_p2p_ie:: wrong ul_len: [%u]!}\r\n", ul_len);
        return -OAL_EFAIL;
    }

    mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (mac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{wal_ioctl_set_wps_p2p_ie_etc::pst_mac_vap null}");
        return -OAL_EINVAL;
    }

    memset_s(&wps_p2p_ie, OAL_SIZEOF(wps_p2p_ie), 0, OAL_SIZEOF(wps_p2p_ie));
    switch (en_type) {
        case OAL_APP_BEACON_IE:
        case OAL_APP_PROBE_RSP_IE:
        case OAL_APP_ASSOC_RSP_IE:
            wps_p2p_ie.en_app_ie_type = en_type;
            wps_p2p_ie.ul_ie_len = ul_len;
            l_memcpy_ret += memcpy_s(wps_p2p_ie.auc_ie, WLAN_WPS_IE_MAX_SIZE, puc_buf, ul_len);
            break;
        default:
            OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_ioctl_set_wps_p2p_ie:: wrong type: [%x]!}\r\n", en_type);
            return -OAL_EFAIL;
    }

    l_memcpy_ret += memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, &wps_p2p_ie, OAL_SIZEOF(wps_p2p_ie));
    if (l_memcpy_ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_ioctl_set_wps_p2p_ie::memcpy_s failed!");
        return -OAL_EFAIL;
    }
    /* **************************************************************************
        ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_WPS_P2P_IE, OAL_SIZEOF(wps_p2p_ie));

    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(wps_p2p_ie),
                               (uint8_t *)&st_write_msg, OAL_TRUE, &pst_rsp_msg);
    if ((l_ret != OAL_SUCC) || (pst_rsp_msg == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(0, OAM_SF_P2P, "{wal_ioctl_set_wps_p2p_ie:: wal_alloc_cfg_event return err code %d!}", l_ret);
        return l_ret;
    }

    /* ��ȡ���صĴ����� */
    err_code = wal_check_and_release_msg_resp(pst_rsp_msg);
    if (err_code != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_P2P, "{wal_ioctl_set_wps_p2p_ie::wal_send_cfg_event return errcode: [%d]!}", err_code);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}
#ifdef _PRE_WLAN_FEATURE_P2P

OAL_STATIC oal_int32 wal_ioctl_set_p2p_noa(oal_net_device_stru *pst_net_dev,
    mac_cfg_p2p_noa_param_stru *pst_p2p_noa_param)
{
    wal_msg_write_stru st_write_msg;
    wal_msg_stru *pst_rsp_msg = OAL_PTR_NULL;
    oal_uint32 ul_err_code;
    oal_int32 l_ret;

    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, pst_p2p_noa_param,
        OAL_SIZEOF(mac_cfg_p2p_noa_param_stru)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_ioctl_set_p2p_noa::memcpy_s failed!");
        return OAL_FAIL;
    }

    /* **************************************************************************
        ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_P2P_PS_NOA, OAL_SIZEOF(mac_cfg_p2p_noa_param_stru));

    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_p2p_noa_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);
    if ((l_ret != OAL_SUCC) || (pst_rsp_msg == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(0, OAM_SF_P2P, "{wal_ioctl_set_p2p_noa:: wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* ��ȡ���صĴ����� */
    ul_err_code = wal_check_and_release_msg_resp(pst_rsp_msg);
    if (ul_err_code != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_P2P, "{wal_ioctl_set_p2p_noa::wal_send_cfg_event return err code:  [%d]!}\r\n",
            ul_err_code);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_int32 wal_ioctl_set_p2p_ops(oal_net_device_stru *pst_net_dev,
    mac_cfg_p2p_ops_param_stru *pst_p2p_ops_param)
{
    wal_msg_write_stru st_write_msg;
    wal_msg_stru *pst_rsp_msg = OAL_PTR_NULL;
    oal_uint32 ul_err_code;
    oal_int32 l_ret;

    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, pst_p2p_ops_param,
        OAL_SIZEOF(mac_cfg_p2p_ops_param_stru)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_ioctl_set_p2p_ops::memcpy_s failed!");
        return OAL_FAIL;
    }

    /* **************************************************************************
        ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_P2P_PS_OPS, OAL_SIZEOF(mac_cfg_p2p_ops_param_stru));

    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_p2p_ops_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);
    if ((l_ret != OAL_SUCC) || (pst_rsp_msg == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(0, OAM_SF_P2P, "{wal_ioctl_set_p2p_ops:: wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* ��ȡ���صĴ����� */
    ul_err_code = wal_check_and_release_msg_resp(pst_rsp_msg);
    if (ul_err_code != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_P2P, "{wal_ioctl_set_p2p_ops::wal_send_cfg_event return err code:[%d]!}\r\n",
            ul_err_code);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}
#endif

#if defined(_PRE_WLAN_FEATURE_APF) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
/*
 * ��������: hipriv���� ��ʾapf filter��sdt
 * 1.��    ��: 2020��3��9��
 * �޸�����: �����ɺ���
 */
OAL_STATIC uint32_t wal_hipriv_apf_filter_cmd(oal_net_device_stru *net_dev, const int8_t *param)
{
    wal_msg_write_stru write_msg = {0};
    int32_t mem_result;
    uint32_t ret;
    uint32_t offset = 0;
    oal_bool_enum_uint8 cmd_updata = OAL_FALSE;
    int8_t param_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    mac_apf_filter_cmd_stru apf_filter_cmd;
    memset_s(&apf_filter_cmd, OAL_SIZEOF(mac_apf_filter_cmd_stru), 0, OAL_SIZEOF(mac_apf_filter_cmd_stru));

    do {
        ret = wal_get_cmd_one_arg(param, param_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &offset);
        if (ret != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_apf_filter_cmd::cmd format err, mem_result:%d;!!}\r\n", ret);
            return ret;
        }
        param += offset;

        if (cmd_updata == OAL_FALSE) {
            cmd_updata = OAL_TRUE;
        } else if (offset == 0) {
            break;
        }

        if (oal_strcmp("list", param_name) == 0) {
            apf_filter_cmd.en_cmd_type = MAC_APF_GET_FILTER_CMD;
        } else if (oal_strcmp("enable", param_name) == 0) {
            apf_filter_cmd.en_cmd_type = MAC_APF_ENABLE_FILTER_CMD;
        } else if (oal_strcmp("disable", param_name) == 0) {
            apf_filter_cmd.en_cmd_type = MAC_APF_DISABLE_FILTER_CMD;
        } else {
            OAL_IO_PRINT("CMD format::sh hipriv.sh 'wlan0 apf_filter [list] [enable] [disable]'");
            OAM_ERROR_LOG0(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 apf_filter [list] [enable] [disable]'!!}");
            return OAL_FAIL;
        }
    } while (*param != '\0');
    // ���¼���wal�㴦��
    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_SET_APF_FILTER, OAL_SIZEOF(apf_filter_cmd));
    mem_result = memcpy_s(write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, &apf_filter_cmd, OAL_SIZEOF(apf_filter_cmd));
    if (mem_result != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_apf_filter_list::memcpy fail!");
        return OAL_FAIL;
    }

    /* ������Ϣ */
    mem_result = wal_send_cfg_event(net_dev, WAL_MSG_TYPE_WRITE,
        WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(apf_filter_cmd), (uint8_t *)&write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(mem_result != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_apf_filter_list::return err code [%d]!}\r\n", mem_result);
        return (uint32_t)mem_result;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_HS20

OAL_STATIC oal_int32 wal_ioctl_set_qos_map(oal_net_device_stru *pst_net_dev,
    hmac_cfg_qos_map_param_stru *pst_qos_map_param)
{
    wal_msg_write_stru st_write_msg;
    wal_msg_stru *pst_rsp_msg = OAL_PTR_NULL;
    oal_uint32 ul_err_code;
    oal_int32 l_ret;

    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, pst_qos_map_param,
        OAL_SIZEOF(hmac_cfg_qos_map_param_stru)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_ioctl_set_qos_map::memcpy_s failed!");
        return OAL_FAIL;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_QOS_MAP, OAL_SIZEOF(hmac_cfg_qos_map_param_stru));

    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(hmac_cfg_qos_map_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    /* ��ȡ���صĴ����� */
    ul_err_code = wal_check_and_release_msg_resp(pst_rsp_msg);
    if (ul_err_code != OAL_SUCC) {
        OAM_ERROR_LOG2(0, OAM_SF_HS20, "{wal_ioctl_set_qos_map::wal_send_cfg_event return err code: [%x] [%x]!}\r\n",
            l_ret, ul_err_code);
        return -OAL_EFAIL;
    }
    return OAL_SUCC;
}
#endif // _PRE_WLAN_FEATURE_HS20

#if ((_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1131C_DEV) && (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1131C_HOST))

oal_int32 wal_ioctl_set_wps_ie(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    wal_msg_write_stru st_write_msg;
    oal_app_ie_stru st_wps_ie;
    wal_msg_stru *pst_rsp_msg = OAL_PTR_NULL;
    oal_uint32 ul_err_code;
    oal_int32 l_ret;

    memset_s(&st_wps_ie, OAL_SIZEOF(oal_app_ie_stru), 0, OAL_SIZEOF(oal_app_ie_stru));
    st_wps_ie.ul_ie_len = pst_ioctl_data->pri_data.st_app_ie.ul_ie_len;
    st_wps_ie.en_app_ie_type = pst_ioctl_data->pri_data.st_app_ie.en_app_ie_type;
    l_ret = (oal_int32)oal_copy_from_user(st_wps_ie.auc_ie, pst_ioctl_data->pri_data.st_app_ie.auc_ie,
                                          st_wps_ie.ul_ie_len);
    /* copy_from_user������Ŀ���Ǵ��û��ռ俽�����ݵ��ں˿ռ䣬ʧ�ܷ���û�б��������ֽ������ɹ�����0 */
    if (l_ret != 0) {
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{wal_ioctl_set_wps_ie::copy app ie fail.ie_type[%d], ie_len[%d]}",
            st_wps_ie.en_app_ie_type, st_wps_ie.ul_ie_len);
        return -OAL_EFAIL;
    }

    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, &st_wps_ie, OAL_SIZEOF(oal_app_ie_stru)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_ioctl_set_wps_ie::memcpy_s failed!");
        return OAL_FAIL;
    }

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_WPS_IE, OAL_SIZEOF(oal_app_ie_stru));

    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_app_ie_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);
    if ((l_ret != OAL_SUCC) || (pst_rsp_msg == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(0, OAM_SF_P2P, "{wal_ioctl_set_wps_ie:: wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* ��ȡ���صĴ����� */
    ul_err_code = wal_check_and_release_msg_resp(pst_rsp_msg);
    if (ul_err_code != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_P2P, "{wal_ioctl_set_wps_ie::wal_send_cfg_event return err code:[%x]!}\r\n",
            ul_err_code);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}


oal_int32 wal_ioctl_set_frag(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    wal_msg_write_stru st_write_msg;
    mac_cfg_frag_threshold_stru *pst_threshold = OAL_PTR_NULL;
    oal_uint32 ul_threshold;
    oal_uint16 us_len;
    oal_int32 l_ret;

    /* ��ȡ��Ƭ���� */
    ul_threshold = (oal_uint32)pst_ioctl_data->pri_data.l_frag;

    pst_threshold = (mac_cfg_frag_threshold_stru *)(st_write_msg.auc_value);
    pst_threshold->ul_frag_threshold = ul_threshold;

    /* ���¼���wal�㴦�� */
    us_len = OAL_SIZEOF(mac_cfg_frag_threshold_stru);
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_FRAG_THRESHOLD_REG, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_frag::return err code [%d]!}\r\n", l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}


oal_int32 wal_ioctl_set_rts(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    wal_msg_write_stru st_write_msg;
    mac_cfg_rts_threshold_stru *pst_threshold = OAL_PTR_NULL;
    oal_uint32 ul_threshold;
    oal_uint16 us_len;
    oal_int32 l_ret;

    /* ��ȡ��Ƭ���� */
    ul_threshold = (oal_uint32)pst_ioctl_data->pri_data.l_rts;

    pst_threshold = (mac_cfg_rts_threshold_stru *)(st_write_msg.auc_value);
    pst_threshold->ul_rts_threshold = ul_threshold;

    OAM_INFO_LOG1(0, OAM_SF_CFG, "{wal_ioctl_set_rts::rts [%d]!}\r\n", ul_threshold);
    /* **************************************************************************
                              ���¼���wal�㴦��
    ************************************************************************** */
    us_len = OAL_SIZEOF(mac_cfg_rts_threshold_stru);
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_RTS_THRESHHOLD, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_ioctl_set_rts::return err code [%d]!}\r\n", l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_int32 wal_ioctl_reduce_sar(oal_net_device_stru *pst_net_dev, oal_uint8 uc_tx_power)
{
    oal_int32 l_ret;
    wal_msg_write_stru st_write_msg;

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_ioctl_reduce_sar::supplicant set tx_power[%d] for reduce SAR purpose.\r\n",
        uc_tx_power);
    if (uc_tx_power > 100) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG,
            "wal_ioctl_reduce_sar::reduce sar failed, reason:invalid tx_power[%d] set by supplicant!", uc_tx_power);
        return -OAL_EINVAL;
    }
    /* vapδ����ʱ��������supplicant���� */
    if (OAL_NET_DEV_PRIV(pst_net_dev) == NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "wal_ioctl_reduce_sar::vap not created yet, ignore the cmd!");
        return -OAL_EINVAL;
    }
    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_REDUCE_SAR, OAL_SIZEOF(oal_uint8));
    *((oal_uint8 *)st_write_msg.auc_value) = uc_tx_power;
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_ioctl_reduce_sar::wal_send_cfg_event failed, error no[%d]!\r\n", l_ret);
        return l_ret;
    }
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_get_parameter_from_cmd(oal_int8 *pc_cmd, oal_int8 *pc_arg, OAL_CONST oal_int8 *puc_token,
    oal_uint32 *pul_cmd_offset, oal_uint32 ul_param_max_len)
{
    oal_int8 *pc_cmd_copy = OAL_PTR_NULL;
    oal_int8 ac_cmd_copy[WAL_IOCTL_PRIV_SUBCMD_MAX_LEN] = {0};
    oal_uint32 ul_pos = 0;
    oal_uint32 ul_arg_len;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pc_cmd) || (OAL_PTR_NULL == pc_arg) || (OAL_PTR_NULL == pul_cmd_offset))) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY,
            "{wal_get_parameter_from_cmd::pc_cmd/pc_arg/pul_cmd_offset null ptr error}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pc_cmd_copy = pc_cmd;

    /* ȥ���ַ�����ʼ�Ķ��� */
    while (',' == *pc_cmd_copy) {
        ++pc_cmd_copy;
    }
    /* ȡ�ö���ǰ���ַ��� */
    while ((',' != *pc_cmd_copy) && ('\0' != *pc_cmd_copy)) {
        ac_cmd_copy[ul_pos] = *pc_cmd_copy;
        ++ul_pos;
        ++pc_cmd_copy;

        if (OAL_UNLIKELY(ul_pos >= ul_param_max_len)) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY,
                "{wal_get_parameter_from_cmd::ul_pos >= WAL_HIPRIV_CMD_NAME_MAX_LEN, ul_pos %d!}\r\n", ul_pos);
            return OAL_ERR_CODE_ARRAY_OVERFLOW;
        }
    }
    ac_cmd_copy[ul_pos] = '\0';
    /* �ַ�������β�����ش����� */
    if (0 == ul_pos) {
        OAM_INFO_LOG0(0, OAM_SF_ANY, "{wal_get_parameter_from_cmd::return param pc_arg is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }
    *pul_cmd_offset = (oal_uint32)(pc_cmd_copy - pc_cmd);

    /* ����ַ����Ƿ����������ǰ�������ַ� */
    if (0 != oal_memcmp(ac_cmd_copy, puc_token, OAL_STRLEN(puc_token))) {
        return OAL_FAIL;
    } else {
        /* �۳�ǰ�������ַ����ش����� */
        ul_arg_len = OAL_STRLEN(ac_cmd_copy) - OAL_STRLEN(puc_token);
        if (memcpy_s(pc_arg, WAL_IOCTL_PRIV_SUBCMD_MAX_LEN, ac_cmd_copy + OAL_STRLEN(puc_token), ul_arg_len) != EOK) {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_get_parameter_from_cmd::memcpy_s failed!");
            return OAL_FAIL;
        }
        pc_arg[ul_arg_len] = '\0';
    }
    return OAL_SUCC;
}


OAL_STATIC oal_int32 wal_set_ap_max_user(oal_net_device_stru *pst_net_dev, oal_uint32 ul_ap_max_user)
{
    wal_msg_write_stru st_write_msg;
    wal_msg_stru *pst_rsp_msg = OAL_PTR_NULL;
    oal_uint32 ul_err_code;
    oal_int32 l_ret;

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_set_ap_max_user:: ap_max_user is : %u.}\r\n", ul_ap_max_user);

    if (ul_ap_max_user == 0) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_set_ap_max_user::invalid ap max user(%u),ignore this set.}\r\n",
            ul_ap_max_user);
        return OAL_SUCC;
    }

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_MAX_USER, OAL_SIZEOF(ul_ap_max_user));
    *((oal_uint32 *)st_write_msg.auc_value) = ul_ap_max_user;
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(ul_ap_max_user),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);
    if ((l_ret != OAL_SUCC) || (pst_rsp_msg == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_set_ap_max_user:: wal_send_cfg_event return err code %d!}\r\n", l_ret);

        return l_ret;
    }

    /* ��ȡ���صĴ����� */
    ul_err_code = wal_check_and_release_msg_resp(pst_rsp_msg);
    if (ul_err_code != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_set_ap_max_user::wal_send_cfg_event return err code: [%d]!}\r\n",
            ul_err_code);
        return -OAL_EFAIL;
    }

    /* ÿ����������û�����ɺ󣬶����Ϊ�Ƿ�ֵ0 */
    g_st_ap_config_info.ul_ap_max_user = 0;

    return l_ret;
}


OAL_STATIC oal_int32 wal_config_mac_filter(oal_net_device_stru *pst_net_dev, oal_int8 *pc_command)
{
    oal_int8 ac_parsed_command[WAL_IOCTL_PRIV_SUBCMD_MAX_LEN] = {0};
    oal_int8 *pc_parse_command = OAL_PTR_NULL;
    oal_uint32 ul_mac_mode;
    oal_uint32 ul_mac_cnt;
    oal_uint32 ul_i;
#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
    wal_msg_write_stru st_write_msg;
    oal_uint16 us_len;
    wal_msg_stru *pst_rsp_msg = OAL_PTR_NULL;
    oal_uint32 ul_err_code;
    oal_int32 l_ret;
#endif
    oal_uint32 ul_ret;
    oal_uint32 ul_off_set;

    if (pc_command == OAL_PTR_NULL) {
        return -OAL_EINVAL;
    }
    pc_parse_command = pc_command;

    /* ����MAC_MODE */
    ul_ret = wal_get_parameter_from_cmd(pc_parse_command, ac_parsed_command, "MAC_MODE=", &ul_off_set,
        WAL_IOCTL_PRIV_SUBCMD_MAX_LEN);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_config_mac_filter::wal_get_parameter_from_cmd return err_code %u.}\r\n",
            ul_ret);
        return -OAL_EINVAL;
    }
    /* �������Ƿ�Ϸ� 0,1,2 */
    ul_mac_mode = (oal_uint32)oal_atoi(ac_parsed_command);
    if (ul_mac_mode > 2) {
        OAM_WARNING_LOG4(0, OAM_SF_ANY, "{wal_config_mac_filter::invalid MAC_MODE[%c%c%c%c]!}\r\n",
            (oal_uint8)ac_parsed_command[0], (oal_uint8)ac_parsed_command[1], (oal_uint8)ac_parsed_command[2],
            (oal_uint8)ac_parsed_command[3]);
        return -OAL_EINVAL;
    }

    /* ���ù���ģʽ */
#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
    ul_ret = wal_hipriv_send_cfg_uint32_data(pst_net_dev, ac_parsed_command, WLAN_CFGID_BLACKLIST_MODE);
    if (ul_ret != OAL_SUCC) {
        return (oal_int32)ul_ret;
    }
#endif
    /* ����MAC_CNT */
    pc_parse_command += ul_off_set;
    ul_ret = wal_get_parameter_from_cmd(pc_parse_command, ac_parsed_command, "MAC_CNT=", &ul_off_set,
        WAL_IOCTL_PRIV_SUBCMD_MAX_LEN);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_config_mac_filter::wal_get_parameter_from_cmd return err_code [%u]!}\r\n",
            ul_ret);
        return -OAL_EINVAL;
    }
    ul_mac_cnt = (oal_uint32)oal_atoi(ac_parsed_command);

    for (ul_i = 0; ul_i < ul_mac_cnt; ul_i++) {
        pc_parse_command += ul_off_set;
        ul_ret = wal_get_parameter_from_cmd(pc_parse_command, ac_parsed_command, "MAC=", &ul_off_set,
            WAL_IOCTL_PRIV_SUBCMD_MAX_LEN);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY,
                "{wal_config_mac_filter::wal_get_parameter_from_cmd return err_code [%u]!}\r\n", ul_ret);
            return -OAL_EINVAL;
        }
        /* 5.1  �������Ƿ����MAC���� */
        if (WLAN_MAC_ADDR_LEN * 2 != OAL_STRLEN(ac_parsed_command)) {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_mac_filter::invalid MAC format}\r\n");
            return -OAL_EINVAL;
        }
        /* 6. ��ӹ����豸 */
#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
        /* **************************************************************************
                             ���¼���wal�㴦��
        ************************************************************************** */
        memset_s((oal_uint8 *)&st_write_msg, OAL_SIZEOF(st_write_msg), 0, OAL_SIZEOF(st_write_msg));
        /* ���ַ� ac_name ת�������� mac_add[6] */
        oal_strtoaddr(ac_parsed_command, sizeof(ac_parsed_command), st_write_msg.auc_value, WLAN_MAC_ADDR_LEN);
        us_len = OAL_MAC_ADDR_LEN;

        if (ul_i == (ul_mac_cnt - 1)) {
            /* �����е�mac��ַ�������ɺ󣬲Ž��й����û�ȷ�ϣ��Ƿ���Ҫɾ�� */
            WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADD_BLACK_LIST, us_len);
        } else {
            WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADD_BLACK_LIST_ONLY, us_len);
        }

        /* 6.1  ������Ϣ */
        l_ret = wal_send_cfg_event(pst_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_TRUE,
                                   &pst_rsp_msg);
        if ((l_ret != OAL_SUCC) || (pst_rsp_msg == OAL_PTR_NULL)) {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_config_mac_filter:: wal_send_cfg_event return err code %d!}", l_ret);
            return l_ret;
        }

        /* 6.2  ��ȡ���صĴ����� */
        ul_err_code = wal_check_and_release_msg_resp(pst_rsp_msg);
        if (ul_err_code != OAL_SUCC) {
            OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_config_mac_filter::wal_send_cfg_event return err code:[%x]!}\r\n",
                ul_err_code);
            return -OAL_EFAIL;
        }
#endif
    }

    /* ÿ���������mac��ַ���˺���մ��м���� */
    memset_s(g_st_ap_config_info.ac_ap_mac_filter_mode, OAL_SIZEOF(g_st_ap_config_info.ac_ap_mac_filter_mode), 0,
        OAL_SIZEOF(g_st_ap_config_info.ac_ap_mac_filter_mode));

    return OAL_SUCC;
}


OAL_STATIC oal_int32 wal_kick_sta(oal_net_device_stru *pst_net_dev, const unsigned char *auc_mac_addr)
{
#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
    wal_msg_write_stru st_write_msg;
    wal_msg_stru *pst_rsp_msg = OAL_PTR_NULL;
    oal_uint32 ul_err_code;
    mac_cfg_kick_user_param_stru *pst_kick_user_param;
    oal_int32 l_ret;
#endif

    if (auc_mac_addr == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_kick_sta::argument auc_mac_addr is null.\n");
        return -OAL_EFAIL;
    }

#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_KICK_USER, OAL_SIZEOF(mac_cfg_kick_user_param_stru));

    pst_kick_user_param = (mac_cfg_kick_user_param_stru *)(st_write_msg.auc_value);
    oal_set_mac_addr(pst_kick_user_param->auc_mac_addr, auc_mac_addr);

    pst_kick_user_param->us_reason_code = MAC_AUTH_NOT_VALID;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_kick_user_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);
    if ((l_ret != OAL_SUCC) || (pst_rsp_msg == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_kick_sta:: wal_send_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* 4.4  ��ȡ���صĴ����� */
    ul_err_code = wal_check_and_release_msg_resp(pst_rsp_msg);
    if (ul_err_code != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_kick_sta::wal_send_cfg_event return err code: [%x]!}\r\n", ul_err_code);
        return -OAL_EFAIL;
    }
#endif

    return OAL_SUCC;
}


OAL_STATIC int wal_ioctl_set_ap_config(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info,
    oal_iwreq_data_union *pst_wrqu, char *pc_extra)
{
    oal_int8 *pc_command = OAL_PTR_NULL;
    oal_int8 *pc_parse_command = OAL_PTR_NULL;
    oal_int32 l_ret = OAL_SUCC;
    oal_uint32 ul_ret;
    oal_int8 ac_parse_command[WAL_IOCTL_PRIV_SUBCMD_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;

    if (OAL_UNLIKELY(pst_net_dev == OAL_PTR_NULL || pst_wrqu == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_ap_config:: param is OAL_PTR_NULL}");
        return -OAL_EFAIL;
    }

    /* 1. �����ڴ汣��netd �·������������ */
    pc_command = oal_memalloc((oal_int32)(pst_wrqu->data.length + 1));
    if (pc_command == OAL_PTR_NULL) {
        return -OAL_ENOMEM;
    }
    /* 2. ����netd ����ں�̬�� */
    memset_s(pc_command, (oal_uint32)(pst_wrqu->data.length + 1), 0, (oal_uint32)(pst_wrqu->data.length + 1));
    ul_ret = oal_copy_from_user(pc_command, pst_wrqu->data.pointer, (oal_uint32)(pst_wrqu->data.length));
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_ap_config::oal_copy_from_user: -OAL_EFAIL }\r\n");
        oal_free(pc_command);
        return -OAL_EFAIL;
    }
    pc_command[pst_wrqu->data.length] = '\0';

    OAL_IO_PRINT("wal_ioctl_set_ap_config,data len:%u,command is:%s\n", (oal_uint32)pst_wrqu->data.length, pc_command);

    pc_parse_command = pc_command;
    /* 3.   �������� */
    /* 3.1  ����ASCII_CMD */
    ul_ret = wal_get_parameter_from_cmd(pc_parse_command, ac_parse_command, "ASCII_CMD=", &ul_off_set,
        WAL_IOCTL_PRIV_SUBCMD_MAX_LEN);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_ioctl_set_ap_config::wal_get_parameter_from_cmd ASCII_CMD return err_code [%u]!}\r\n", ul_ret);
        oal_free(pc_command);
        return -OAL_EINVAL;
    }

    if ((oal_strcmp("AP_CFG", ac_parse_command) != 0)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_ap_config::sub_command != 'AP_CFG' }");
        OAL_IO_PRINT("{wal_ioctl_set_ap_config::sub_command %6s...!= 'AP_CFG' }", ac_parse_command);
        oal_free(pc_command);
        return -OAL_EINVAL;
    }

    /* 3.2  ����CHANNEL��Ŀǰ������netd�·���channel��Ϣ */
    pc_parse_command += ul_off_set;
    ul_ret = wal_get_parameter_from_cmd(pc_parse_command, ac_parse_command, "CHANNEL=", &ul_off_set,
        WAL_IOCTL_PRIV_SUBCMD_MAX_LEN);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_ioctl_set_ap_config::wal_get_parameter_from_cmd CHANNEL return err_code [%u]!}\r\n", ul_ret);
        oal_free(pc_command);
        return -OAL_EINVAL;
    }

    /* 3.3  ����MAX_SCB */
    pc_parse_command += ul_off_set;
    ul_ret = wal_get_parameter_from_cmd(pc_parse_command, ac_parse_command, "MAX_SCB=", &ul_off_set,
        WAL_IOCTL_PRIV_SUBCMD_MAX_LEN);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_ioctl_set_ap_config::wal_get_parameter_from_cmd MAX_SCB return err_code [%u]!}\r\n", ul_ret);
        oal_free(pc_command);
        return -OAL_EINVAL;
    }

    g_st_ap_config_info.ul_ap_max_user = (oal_uint32)oal_atoi(ac_parse_command);

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_ap_config:: 1131_debug:: ul_ap_max_user = [%d]!}\r\n",
        g_st_ap_config_info.ul_ap_max_user);

    if (OAL_NET_DEV_PRIV(pst_net_dev) != OAL_PTR_NULL) {
        l_ret = wal_set_ap_max_user(pst_net_dev, (oal_uint32)oal_atoi(ac_parse_command));
    }

    /* 5. �����ͷ��ڴ� */
    oal_free(pc_command);
    return l_ret;
}


OAL_STATIC int wal_ioctl_get_assoc_list(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info,
    oal_iwreq_data_union *pst_wrqu, char *pc_extra)
{
    oal_int32 l_ret;
    wal_msg_query_stru st_query_msg;
    wal_msg_stru *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_rsp_stru *pst_query_rsp_msg = OAL_PTR_NULL;
    oal_int8 *pc_sta_list = OAL_PTR_NULL;
    oal_netbuf_stru *pst_response_netbuf = OAL_PTR_NULL;
    oal_uint32 l_memcpy_ret = EOK;

    if (OAL_UNLIKELY(pst_net_dev == OAL_PTR_NULL || pst_info == OAL_PTR_NULL || pst_wrqu == OAL_PTR_NULL ||
        pc_extra == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_get_assoc_list:: param is OAL_PTR_NULL}");
        return -OAL_EFAIL;
    }

    /* �ϲ����κ�ʱ�򶼿����·��������Ҫ���жϵ�ǰnetdev��״̬����ʱ���� */
    if (OAL_UNLIKELY(OAL_NET_DEV_PRIV(pst_net_dev) == OAL_PTR_NULL)) {
        return -OAL_EFAIL;
    }

    /* **************************************************************************
        ���¼���wal�㴦��
    ************************************************************************** */
    st_query_msg.en_wid = WLAN_CFGID_GET_STA_LIST;

    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_QUERY, WAL_MSG_WID_LENGTH,
                               (oal_uint8 *)&st_query_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);
    if ((l_ret != OAL_SUCC) || (pst_rsp_msg == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_ioctl_get_assoc_list:: wal_alloc_cfg_event return err code %d!}\r\n",
            l_ret);
        return l_ret;
    }

    /* ��������Ϣ */
    pst_query_rsp_msg = (wal_msg_rsp_stru *)(pst_rsp_msg->auc_msg_data);
    /* ҵ���� */
    if (pst_query_rsp_msg->us_len >= OAL_SIZEOF(oal_netbuf_stru *)) {
        /* ��ȡhmac�����netbufָ�� */
        l_memcpy_ret += memcpy_s(&pst_response_netbuf, OAL_SIZEOF(oal_netbuf_stru *), pst_query_rsp_msg->auc_value,
            OAL_SIZEOF(oal_netbuf_stru *));
        if (pst_response_netbuf != NULL) {
            /* ����ap�����sta��ַ��Ϣ */
            pc_sta_list = (oal_int8 *)OAL_NETBUF_DATA(pst_response_netbuf);
            pst_wrqu->data.length = (oal_uint16)(OAL_NETBUF_LEN(pst_response_netbuf) + 1);
            l_memcpy_ret += memcpy_s(pc_extra, pst_wrqu->data.length, pc_sta_list, pst_wrqu->data.length);
            pc_extra[OAL_NETBUF_LEN(pst_response_netbuf)] = '\0';
            oal_netbuf_free(pst_response_netbuf);
        } else {
            l_ret = -OAL_ENOMEM;
        }
    } else {
        oal_print_hex_dump((oal_uint8 *)pst_rsp_msg->auc_msg_data, pst_query_rsp_msg->us_len, 32, "query msg: ");
        l_ret = -OAL_EINVAL;
    }
    if (l_memcpy_ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_ioctl_get_assoc_list::memcpy_s failed!");
        oal_free(pst_rsp_msg);
        return OAL_FAIL;
    }
    if (l_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_ioctl_get_assoc_list::process failed,ret=%d}", l_ret);
    } else {
        OAL_IO_PRINT("wal_ioctl_get_assoc_list,pc_sta_list is:%s,len:%d\n", pc_extra, pst_wrqu->data.length);
    }

    oal_free(pst_rsp_msg);
    return l_ret;
}


static uint32_t get_command_from_user(int8_t *pc_command, uint32_t command_len, const oal_iwreq_data_union *pst_wrqu)
{
    uint32_t ul_ret;

    /* 2. ����netd ����ں�̬�� */
    memset_s(pc_command, command_len, 0, command_len);
    ul_ret = oal_copy_from_user(pc_command, pst_wrqu->data.pointer, command_len - 1);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{get_command_from_user::oal_copy_from_user failed. ret=%u}", ul_ret);
        return OAL_EFAIL;
    }
    pc_command[command_len - 1] = '\0';
    OAL_IO_PRINT("get_command_from_user, data len:%d, command is:%s\n", command_len - 1, pc_command);

    memset_s(g_st_ap_config_info.ac_ap_mac_filter_mode, OAL_SIZEOF(g_st_ap_config_info.ac_ap_mac_filter_mode), 0,
        OAL_SIZEOF(g_st_ap_config_info.ac_ap_mac_filter_mode));
    if (strncpy_s(g_st_ap_config_info.ac_ap_mac_filter_mode, sizeof(g_st_ap_config_info.ac_ap_mac_filter_mode),
        pc_command, OAL_SIZEOF(g_st_ap_config_info.ac_ap_mac_filter_mode)) != EOK) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{get_command_from_user::strncpy_s failed !");
        return OAL_EINVAL;
    }
    return OAL_SUCC;
}

OAL_STATIC int get_mac_cnt_and_mode_from_command(int8_t *command, uint32_t *mac_cnt, uint32_t *mac_mode)
{
    uint32_t ret;
    int8_t parsed_command[WAL_IOCTL_PRIV_SUBCMD_MAX_LEN] = {0};
    uint32_t offset = 0;

    /* 3  ����MAC_MODE */
    ret = wal_get_parameter_from_cmd(command, parsed_command, "MAC_MODE=", &offset,
        WAL_IOCTL_PRIV_SUBCMD_MAX_LEN);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_ioctl_set_mac_filters::wal_get_parameter_from_cmd return err_code [%u]!}\r\n", ret);
        return -OAL_EINVAL;
    }
    /* 3.1 �������Ƿ�Ϸ� 0,1,2 */
    *mac_mode = (uint32_t)oal_atoi(parsed_command);
    if (*mac_mode > 2) {
        OAM_WARNING_LOG4(0, OAM_SF_ANY, "{wal_ioctl_set_mac_filters::invalid MAC_MODE[%c%c%c%c]!}",
            (uint8_t)parsed_command[0], (uint8_t)parsed_command[1], (uint8_t)parsed_command[2],
            (uint8_t)parsed_command[3]);
        return -OAL_EINVAL;
    }

    /* 5 ����MAC_CNT */
    command += offset;  //lint !e413
    ret = wal_get_parameter_from_cmd(command, parsed_command, "MAC_CNT=", &offset,
        WAL_IOCTL_PRIV_SUBCMD_MAX_LEN);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_ioctl_set_mac_filters::wal_get_parameter_from_cmd return err_code [%u]!}\r\n", ret);
        return -OAL_EINVAL;
    }
    *mac_cnt = (uint32_t)oal_atoi(parsed_command);
    return OAL_SUCC;
}


OAL_STATIC int wal_ioctl_set_mac_filters(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info,
    oal_iwreq_data_union *pst_wrqu, char *pc_extra)
{
    oal_int8 *pc_command = OAL_PTR_NULL;
    uint32_t command_len;
    oal_int32 l_ret = 0;
    oal_uint32 ul_mac_mode;
    oal_uint32 ul_mac_cnt;
    oal_uint8 auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};

    if (OAL_ANY_NULL_PTR4(pst_net_dev, pst_info, pst_wrqu, pc_extra)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_mac_filters:: param is NULL}");
        return -OAL_EFAIL;
    }

    if (OAL_NET_DEV_PRIV(pst_net_dev) == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_mac_filters::netdevice vap is null,just save it.}\r\n");
        oal_free(pc_command);
        return OAL_SUCC;
    }

    command_len = pst_wrqu->data.length + 1;
    /* 1. �����ڴ汣��netd �·������������ */
    pc_command = oal_memalloc(command_len);
    if (pc_command == NULL) {
        return OAL_ENOMEM;
    }
    if (get_command_from_user(pc_command, command_len, pst_wrqu) != OAL_SUCC) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_mac_filters::get_command_from_user failed.}");
        oal_free(pc_command);
        return -OAL_ENOMEM;
    }

    if (get_mac_cnt_and_mode_from_command(pc_command, &ul_mac_cnt, &ul_mac_mode) != OAL_SUCC) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_mac_filters::get_mac_cnt_and_mode_from_command failed.}");
        oal_free(pc_command);
        return -OAL_EINVAL;
    }

    wal_config_mac_filter(pst_net_dev, pc_command);

    /* ����ǰ�����ģʽ�����·�����MAC��ַΪ�գ����������κ��豸��������Ҫȥ���������Ѿ�������STA */
    if ((ul_mac_cnt == 0) && (ul_mac_mode == 2)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_mac_filters::delete all user!}");
        memset_s(auc_mac_addr, WLAN_MAC_ADDR_LEN, 0xff, OAL_ETH_ALEN);
        l_ret = wal_kick_sta(pst_net_dev, auc_mac_addr);
    }

    oal_free(pc_command);
    return l_ret;
}


OAL_STATIC int wal_ioctl_set_ap_sta_disassoc(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info,
    oal_iwreq_data_union *pst_wrqu, char *pc_extra)
{
    oal_int8 *pc_command = OAL_PTR_NULL;
    oal_int32 l_ret;
    oal_uint32 ul_ret;
    oal_int8 ac_parsed_command[WAL_IOCTL_PRIV_SUBCMD_MAX_LEN] = {0};
    oal_uint8 auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};
    oal_uint32 ul_off_set;
    if (OAL_UNLIKELY(pst_net_dev == OAL_PTR_NULL || pst_wrqu == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_mac_filters:: param is OAL_PTR_NULL}");
        return -OAL_EFAIL;
    }

    /* 1. �����ڴ汣��netd �·������������ */
    pc_command = oal_memalloc((oal_int32)(pst_wrqu->data.length + 1));
    if (pc_command == OAL_PTR_NULL) {
        return -OAL_ENOMEM;
    }

    /* 2. ����netd ����ں�̬�� */
    memset_s(pc_command, (oal_uint32)(pst_wrqu->data.length + 1), 0, (oal_uint32)(pst_wrqu->data.length + 1));
    ul_ret = oal_copy_from_user(pc_command, pst_wrqu->data.pointer, (oal_uint32)(pst_wrqu->data.length));
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_ap_sta_disassoc::oal_copy_from_user: -OAL_EFAIL }\r\n");
        oal_free(pc_command);
        return -OAL_EFAIL;
    }
    pc_command[pst_wrqu->data.length] = '\0';

    OAL_IO_PRINT("wal_ioctl_set_ap_sta_disassoc,command is:%s\n", pc_command);

    /* 3. ���������ȡMAC */
    ul_ret =
        wal_get_parameter_from_cmd(pc_command, ac_parsed_command, "MAC=", &ul_off_set, WAL_IOCTL_PRIV_SUBCMD_MAX_LEN);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_ioctl_set_ap_sta_disassoc::wal_get_parameter_from_cmd MAC return err_code [%u]!}\r\n", ul_ret);
        oal_free(pc_command);
        return -OAL_EINVAL;
    }
    /* 3.1  �������Ƿ����MAC���� */
    if (WLAN_MAC_ADDR_LEN * 2 != OAL_STRLEN(ac_parsed_command)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_ap_sta_disassoc::invalid MAC format}\r\n");
        oal_free(pc_command);
        return -OAL_EINVAL;
    }
    /* ���ַ� ac_name ת�������� mac_add[6] */
    oal_strtoaddr(ac_parsed_command, sizeof(ac_parsed_command), auc_mac_addr, WLAN_MAC_ADDR_LEN);

    OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_ap_sta_disassoc::Geting CMD from APP to DISASSOC!!}");
    l_ret = wal_kick_sta(pst_net_dev, auc_mac_addr);

    /* 5. �����ͷ��ڴ� */
    oal_free(pc_command);
    return l_ret;
}

#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_WLAN_DFT_EVENT

OAL_STATIC oal_void wal_event_report_to_sdt(wal_msg_type_enum_uint8 en_msg_type, oal_uint8 *puc_param,
    wal_msg_stru *pst_cfg_msg)
{
    oam_event_type_enum_uint16 en_event_type = OAM_EVENT_TYPE_BUTT;
    oal_uint8                    auc_event[50] = {0};
    oal_uint32 l_memcpy_ret = EOK;

    if (en_msg_type == WAL_MSG_TYPE_QUERY) {
        en_event_type = OAM_EVENT_WID_QUERY;
    } else if (en_msg_type == WAL_MSG_TYPE_WRITE) {
        en_event_type = OAM_EVENT_WID_WRITE;
    }

    /* ����WID,������ǰ�����ֽ���WID */
    l_memcpy_ret +=
        memcpy_s((oal_void *)auc_event, sizeof(auc_event), (const oal_void *)puc_param, OAL_SIZEOF(oal_uint16));

    /* ������Ϣͷ */
    l_memcpy_ret += memcpy_s((void *)&auc_event[2], sizeof(auc_event) - 2, (const void *)&(pst_cfg_msg->st_msg_hdr),
        OAL_SIZEOF(wal_msg_hdr_stru));
    if (l_memcpy_ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_event_report_to_sdt::memcpy_s failed!");
        return OAL_FAIL;
    }
    WAL_EVENT_WID(BROADCAST_MACADDR, 0, en_event_type, auc_event);
}
#endif

OAL_STATIC oal_uint32 wal_hipriv_get_all_reg_value(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint16 us_len;

    /* **************************************************************************
                              ���¼���wal�㴦��
    ************************************************************************** */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, pc_param, OAL_STRLEN(pc_param)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_get_all_reg_value::memcpy_s failed!");
        return OAL_FAIL;
    }

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_ALL_REG_VALUE, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_all_reg_value::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


oal_uint32 wal_hipriv_get_mac_addr(const oal_int8 *pc_param, oal_uint8 auc_mac_addr[], oal_uint32 *pul_total_offset)
{
    oal_uint32 ul_off_set = 0;
    oal_uint32 ul_ret;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};

    /* ��ȡmac��ַ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_mac_addr::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, sizeof(ac_name), auc_mac_addr, WLAN_MAC_ADDR_LEN);

    *pul_total_offset = ul_off_set;

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP

OAL_STATIC oal_uint32 wal_hipriv_set_edca_opt_switch_sta(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint8 uc_flag;
    oal_uint8 *puc_value = 0;
    oal_uint32 ul_ret;
    oal_uint32 ul_off_set = 0;
    oal_int32 l_ret;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};

    // sh hipriv.sh "vap0 set_edca_switch_sta 1/0"
    /* ��ȡmac_vap */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (pst_mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_STA) {
        OAM_WARNING_LOG0(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_switch_sta:: only STA_MODE support}");
        return OAL_FAIL;
    }

    /* ��ȡ���ò��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA,
            "{wal_hipriv_set_edca_opt_switch_sta::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    uc_flag = (oal_uint8)oal_atoi(ac_name);
    /* �Ƿ����ò��� */
    if (uc_flag > 1) {
        OAM_WARNING_LOG0(0, OAM_SF_EDCA, "wal_hipriv_set_edca_opt_switch_sta, invalid config, should be 0 or 1");
        return OAL_SUCC;
    }

    /* �����¼��ڴ� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_EDCA_OPT_SWITCH_STA, OAL_SIZEOF(oal_uint8));
    puc_value = (oal_uint8 *)(st_write_msg.auc_value);
    *puc_value = uc_flag;

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_switch_sta:: return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_edca_opt_weight_sta(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint8 uc_weight;
    oal_uint8 *puc_value = 0;
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint32 ul_off_set = 0;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};

    // sh hipriv.sh "vap0 set_edca_weight_sta 1"
    if (OAL_NET_DEV_PRIV(pst_net_dev) == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_edca_opt_weight_sta::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡmac_vap */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (pst_mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_STA) {
        OAM_WARNING_LOG0(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_cycle_ap:: only AP_MODE support}");
        return OAL_FAIL;
    }

    /* ��ȡ����ֵ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA,
            "{wal_hipriv_set_edca_opt_cycle_ap::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /*lint -e734*/
    uc_weight = (oal_uint32)oal_atoi(ac_name);
    /*lint +e734*/
    /* ���Ȩ��Ϊ3 */
    if (uc_weight > 3) {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA, "wal_hipriv_set_edca_opt_weight_sta: valid value is between 0 and %d", 3);
        return OAL_FAIL;
    }

    /* �����¼��ڴ� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_EDCA_OPT_WEIGHT_STA, OAL_SIZEOF(oal_uint8));
    puc_value = (oal_uint8 *)(st_write_msg.auc_value);
    *puc_value = uc_weight;

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_weight_sta:: return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_edca_opt_switch_ap(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint8 uc_flag;
    oal_uint8 *puc_value = 0;
    oal_uint32 ul_ret;
    oal_uint32 ul_off_set = 0;
    oal_int32 l_ret;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};

    // sh hipriv.sh "vap0 set_edca_switch_ap 1/0"
    if (OAL_NET_DEV_PRIV(pst_net_dev) == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_edca_opt_switch_ap::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡmac_vap */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (pst_mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_AP) {
        OAM_WARNING_LOG0(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_cycle_ap:: only AP_MODE support}");
        return OAL_FAIL;
    }

    /* ��ȡ���ò��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA,
            "{wal_hipriv_set_edca_opt_cycle_ap::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    uc_flag = (oal_uint8)oal_atoi(ac_name);
    /* �Ƿ����ò��� */
    if (uc_flag > 1) {
        OAM_WARNING_LOG0(0, OAM_SF_EDCA, "wal_hipriv_set_edca_opt_cycle_ap, invalid config, should be 0 or 1");
        return OAL_SUCC;
    }

    /* �����¼��ڴ� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_EDCA_OPT_SWITCH_AP, OAL_SIZEOF(oal_uint8));
    puc_value = (oal_uint8 *)(st_write_msg.auc_value);
    *puc_value = uc_flag;

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_switch_ap:: return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_edca_opt_cycle_ap(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_cycle_ms;
    oal_uint32 *pul_value = 0;
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint32 ul_off_set = 0;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};

    // sh hipriv.sh "vap0 set_edca_cycle_ap 200"
    if (OAL_NET_DEV_PRIV(pst_net_dev) == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_edca_opt_cycle_ap::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡmac_vap */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (pst_mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_AP) {
        OAM_WARNING_LOG0(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_cycle_ap:: only AP_MODE support}");
        return OAL_FAIL;
    }

    /* ��ȡ����ֵ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA,
            "{wal_hipriv_set_edca_opt_cycle_ap::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    ul_cycle_ms = (oal_uint32)oal_atoi(ac_name);

    /* �����¼��ڴ� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_EDCA_OPT_CYCLE_AP, OAL_SIZEOF(oal_uint32));
    pul_value = (oal_uint32 *)(st_write_msg.auc_value);
    *pul_value = ul_cycle_ms;

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_cycle_ap:: return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif


#ifdef _PRE_WLAN_PERFORM_STAT

OAL_STATIC oal_uint32 wal_hipriv_stat_tid_thrpt(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint8 auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint32 ul_total_offset = 0;
    mac_cfg_stat_param_stru *pst_stat_param = OAL_PTR_NULL;

    /* vap0 stat_tid_thrpt xx xx xx xx xx xx(mac��ַ) tid_num stat_period(ͳ������ms) stat_num(ͳ�ƴ���) */
    if (OAL_NET_DEV_PRIV(pst_net_dev) == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_thrpt::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �����¼��ڴ� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_STAT, OAL_SIZEOF(mac_cfg_stat_param_stru));
    pst_stat_param = (mac_cfg_stat_param_stru *)(st_write_msg.auc_value);

    pst_stat_param->en_stat_type = MAC_STAT_TYPE_TID_THRPT;
    pst_stat_param->uc_vap_id = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* ��ȡmac��ַ */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, auc_mac_addr, &ul_total_offset);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_thrpt::wal_hipriv_get_mac_addr failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_stat_param->auc_mac_addr, auc_mac_addr);

    /* ��ȡtidno */
    pc_param = pc_param + ul_total_offset;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_thrpt::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    pst_stat_param->uc_tidno = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡͳ������ */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_thrpt::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    pst_stat_param->us_stat_period = (oal_uint16)oal_atoi(ac_name);

    /* ��ȡͳ�ƴ��� */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_thrpt::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    pst_stat_param->us_stat_num = (oal_uint16)oal_atoi(ac_name);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_stat_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_stat_tid_thrpt::wal_hipriv_stat_tid_thrpt return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_stat_user_thrpt(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint8 auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint32 ul_total_offset = 0;
    mac_cfg_stat_param_stru *pst_stat_param = OAL_PTR_NULL;

    /* vap0 stat_user_thrpt xx xx xx xx xx xx(mac��ַ) stat_period(ͳ������ms) stat_num(ͳ�ƴ���) */
    if (OAL_NET_DEV_PRIV(pst_net_dev) == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_user_thrpt::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �����¼��ڴ� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_STAT, OAL_SIZEOF(mac_cfg_stat_param_stru));
    pst_stat_param = (mac_cfg_stat_param_stru *)(st_write_msg.auc_value);

    pst_stat_param->en_stat_type = MAC_STAT_TYPE_USER_THRPT;
    pst_stat_param->uc_vap_id = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* ��ȡmac��ַ */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, auc_mac_addr, &ul_total_offset);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_user_thrpt::wal_hipriv_get_mac_addr failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_stat_param->auc_mac_addr, auc_mac_addr);

    /* ��ȡͳ������ */
    pc_param = pc_param + ul_total_offset;
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_user_thrpt::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    pst_stat_param->us_stat_period = (oal_uint16)oal_atoi(ac_name);

    /* ��ȡͳ�ƴ��� */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_user_thrpt::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    pst_stat_param->us_stat_num = (oal_uint16)oal_atoi(ac_name);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_stat_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_stat_user_thrpt::wal_hipriv_stat_tid_thrpt return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_stat_vap_thrpt(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_stat_param_stru *pst_stat_param = OAL_PTR_NULL;

    /* vap0 stat_vap_thrpt stat_period(ͳ������ms) stat_num(ͳ�ƴ���) */
    if (OAL_NET_DEV_PRIV(pst_net_dev) == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_vap_thrpt::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �����¼��ڴ� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_STAT, OAL_SIZEOF(mac_cfg_stat_param_stru));
    pst_stat_param = (mac_cfg_stat_param_stru *)(st_write_msg.auc_value);

    pst_stat_param->en_stat_type = MAC_STAT_TYPE_VAP_THRPT;
    pst_stat_param->uc_vap_id = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* ��ȡͳ������ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_vap_thrpt::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    pst_stat_param->us_stat_period = (oal_uint16)oal_atoi(ac_name);

    /* ��ȡͳ�ƴ��� */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_vap_thrpt::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    pst_stat_param->us_stat_num = (oal_uint16)oal_atoi(ac_name);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_stat_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_stat_vap_thrpt::wal_hipriv_stat_tid_thrpt return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_stat_tid_per(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint8 auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_stat_param_stru *pst_stat_param = OAL_PTR_NULL;
    oal_uint32 ul_total_offset = 0;

    /* vap0 stat_tid_per xx xx xx xx xx xx(mac��ַ) tid_num stat_period(ͳ������ms) stat_num(ͳ�ƴ���) */
    if (OAL_NET_DEV_PRIV(pst_net_dev) == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_vap_thrpt::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �����¼��ڴ� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_STAT, OAL_SIZEOF(mac_cfg_stat_param_stru));
    pst_stat_param = (mac_cfg_stat_param_stru *)(st_write_msg.auc_value);

    pst_stat_param->en_stat_type = MAC_STAT_TYPE_TID_PER;
    pst_stat_param->uc_vap_id = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* ��ȡmac��ַ */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, auc_mac_addr, &ul_total_offset);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_per::wal_hipriv_get_mac_addr failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_stat_param->auc_mac_addr, auc_mac_addr);

    /* ��ȡtidno */
    pc_param = pc_param + ul_total_offset;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_per::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    pst_stat_param->uc_tidno = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡͳ������ */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_per::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    pst_stat_param->us_stat_period = (oal_uint16)oal_atoi(ac_name);

    /* ��ȡͳ�ƴ��� */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_per::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    pst_stat_param->us_stat_num = (oal_uint16)oal_atoi(ac_name);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_stat_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_per::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_stat_tid_delay(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint8 auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_stat_param_stru *pst_stat_param = OAL_PTR_NULL;
    oal_uint32 ul_total_offset = 0;

    /* vap0 stat_tid_delay xx xx xx xx xx xx(mac��ַ) tid_num stat_period(ͳ������ms) stat_num(ͳ�ƴ���) */
    if (OAL_NET_DEV_PRIV(pst_net_dev) == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_delay::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �����¼��ڴ� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_STAT, OAL_SIZEOF(mac_cfg_stat_param_stru));
    pst_stat_param = (mac_cfg_stat_param_stru *)(st_write_msg.auc_value);

    pst_stat_param->en_stat_type = MAC_STAT_TYPE_TID_DELAY;
    pst_stat_param->uc_vap_id = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* ��ȡmac��ַ */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, auc_mac_addr, &ul_total_offset);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_delay::wal_hipriv_get_mac_addr failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_stat_param->auc_mac_addr, auc_mac_addr);

    /* ��ȡtidno */
    pc_param = pc_param + ul_total_offset;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_delay::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    pst_stat_param->uc_tidno = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡͳ������ */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_delay::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    pst_stat_param->us_stat_period = (oal_uint16)oal_atoi(ac_name);

    /* ��ȡͳ�ƴ��� */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_delay::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    pst_stat_param->us_stat_num = (oal_uint16)oal_atoi(ac_name);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_stat_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_delay::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_display_tid_thrpt(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint8 auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_display_param_stru *pst_display_param = OAL_PTR_NULL;
    oal_uint32 ul_total_offset = 0;

    /* vap0 stat_tid_thrpt xx xx xx xx xx xx(mac��ַ) tid_num stat_period(ͳ������ms) stat_num(ͳ�ƴ���) */
    if (OAL_NET_DEV_PRIV(pst_net_dev) == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_display_tid_thrpt::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �����¼��ڴ� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_DISPLAY, OAL_SIZEOF(mac_cfg_display_param_stru));
    pst_display_param = (mac_cfg_display_param_stru *)(st_write_msg.auc_value);

    pst_display_param->en_stat_type = MAC_STAT_TYPE_TID_THRPT;
    pst_display_param->uc_vap_id = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* ��ȡmac��ַ */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, auc_mac_addr, &ul_total_offset);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_display_tid_thrpt::wal_hipriv_get_mac_addr failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_display_param->auc_mac_addr, auc_mac_addr);

    /* ��ȡtidno */
    pc_param = pc_param + ul_total_offset;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_tid_thrpt::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    pst_display_param->uc_tidno = (oal_uint8)oal_atoi(ac_name);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_display_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_tid_thrpt::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_display_user_thrpt(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint8 auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_display_param_stru *pst_display_param = OAL_PTR_NULL;
    oal_uint32 ul_total_offset = 0;

    /* vap0 stat_user_thrpt xx xx xx xx xx xx(mac��ַ) stat_period(ͳ������ms) stat_num(ͳ�ƴ���) */
    if (OAL_NET_DEV_PRIV(pst_net_dev) == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_display_user_thrpt::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �����¼��ڴ� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_DISPLAY, OAL_SIZEOF(mac_cfg_display_param_stru));
    pst_display_param = (mac_cfg_display_param_stru *)(st_write_msg.auc_value);

    pst_display_param->en_stat_type = MAC_STAT_TYPE_USER_THRPT;
    pst_display_param->uc_vap_id = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* ��ȡmac��ַ */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, auc_mac_addr, &ul_total_offset);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_display_user_thrpt::wal_hipriv_get_mac_addr failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_display_param->auc_mac_addr, auc_mac_addr);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_display_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_user_thrpt::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_display_vap_thrpt(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    mac_cfg_display_param_stru *pst_display_param = OAL_PTR_NULL;

    /* vap0 stat_vap_thrpt stat_period(ͳ������ms) stat_num(ͳ�ƴ���) */
    if (OAL_NET_DEV_PRIV(pst_net_dev) == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_display_vap_thrpt::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �����¼��ڴ� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_DISPLAY, OAL_SIZEOF(mac_cfg_display_param_stru));
    pst_display_param = (mac_cfg_display_param_stru *)(st_write_msg.auc_value);

    pst_display_param->en_stat_type = MAC_STAT_TYPE_VAP_THRPT;
    pst_display_param->uc_vap_id = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_display_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_vap_thrpt::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_display_tid_per(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint8 auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_display_param_stru *pst_display_param = OAL_PTR_NULL;
    oal_uint32 ul_total_offset = 0;

    /* vap0 stat_tid_per xx xx xx xx xx xx(mac��ַ) tid_num stat_period(ͳ������ms) stat_num(ͳ�ƴ���) */
    if (OAL_NET_DEV_PRIV(pst_net_dev) == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_display_tid_per::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �����¼��ڴ� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_DISPLAY, OAL_SIZEOF(mac_cfg_display_param_stru));
    pst_display_param = (mac_cfg_display_param_stru *)(st_write_msg.auc_value);

    pst_display_param->en_stat_type = MAC_STAT_TYPE_TID_PER;
    pst_display_param->uc_vap_id = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* ��ȡmac��ַ */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, auc_mac_addr, &ul_total_offset);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_display_tid_per::wal_hipriv_get_mac_addr failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_display_param->auc_mac_addr, auc_mac_addr);

    /* ��ȡtidno */
    pc_param = pc_param + ul_total_offset;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_tid_per::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    pst_display_param->uc_tidno = (oal_uint8)oal_atoi(ac_name);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_display_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_tid_per::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_display_tid_delay(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint8 auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_display_param_stru *pst_display_param = OAL_PTR_NULL;
    oal_uint32 ul_total_offset = 0;

    /* vap0 stat_tid_delay xx xx xx xx xx xx(mac��ַ) tid_num stat_period(ͳ������ms) stat_num(ͳ�ƴ���) */
    if (OAL_NET_DEV_PRIV(pst_net_dev) == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_display_tid_delay::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �����¼��ڴ� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_DISPLAY, OAL_SIZEOF(mac_cfg_display_param_stru));
    pst_display_param = (mac_cfg_display_param_stru *)(st_write_msg.auc_value);

    pst_display_param->en_stat_type = MAC_STAT_TYPE_TID_DELAY;
    pst_display_param->uc_vap_id = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* ��ȡmac��ַ */
    ul_ret = wal_hipriv_get_mac_addr(pc_param, auc_mac_addr, &ul_total_offset);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_display_tid_delay::wal_hipriv_get_mac_addr failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_display_param->auc_mac_addr, auc_mac_addr);

    /* ��ȡtidno */
    pc_param = pc_param + ul_total_offset;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_tid_delay::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    pst_display_param->uc_tidno = (oal_uint8)oal_atoi(ac_name);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_display_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_tid_delay::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_CHIP_TEST

OAL_STATIC oal_uint32 wal_hipriv_lpm_soc_mode(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    mac_cfg_lpm_soc_set_stru *pst_set_para = OAL_PTR_NULL;
    oal_uint32 ul_off_set = 0;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};

    /*
     * SOC���ܲ���ģʽ����, hipriv "Hisilicon0 lpm_soc_mode 0|1|2|3|4(����gating|PCIE RD BY PASS|mem precharge|PCIE
     * L0-S|PCIE L1-0) 0|1(disable|enable) pcie_idle(PCIE�͹��Ŀ���ʱ��1~7us) "
     */
    pst_set_para = (mac_cfg_lpm_soc_set_stru *)(st_write_msg.auc_value);
    /* ��������������� */
    memset_s(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, 0, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    /* ��ȡ����ģʽ */
    wal_get_cmd_one_arg((oal_int8 *)pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    pst_set_para->en_mode = (mac_lpm_soc_set_enum_uint8)oal_atoi(ac_name);
    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    /* ��ȡ�������ǹر� */
    wal_get_cmd_one_arg((oal_int8 *)pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    pst_set_para->uc_on_off = (oal_uint8)oal_atoi(ac_name);
    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    /* ��ȡPCIE����ʱ������ */
    wal_get_cmd_one_arg((oal_int8 *)pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    pst_set_para->uc_pcie_idle = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_SOC_MODE, OAL_SIZEOF(mac_cfg_lpm_soc_set_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_soc_set_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_soc_mode::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_lpm_chip_state(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set;
    mac_cfg_lpm_sleep_para_stru *pst_set_para = OAL_PTR_NULL;

    pst_set_para = (mac_cfg_lpm_sleep_para_stru *)(st_write_msg.auc_value);
    /* ��������������� */
    memset_s(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, 0, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    pst_set_para->uc_pm_switch = (mac_lpm_state_enum_uint8)oal_atoi(ac_name);
    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    /* ��ȡ��ʱ˯�߲��� */
    wal_get_cmd_one_arg((oal_int8 *)pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    pst_set_para->us_sleep_ms = (oal_uint16)oal_atoi(ac_name);

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_CHIP_STATE, OAL_SIZEOF(mac_cfg_lpm_sleep_para_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_sleep_para_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_chip_state::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_lpm_psm_param(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    mac_cfg_lpm_psm_param_stru *pst_psm_para = OAL_PTR_NULL;
    oal_uint32 ul_off_set = 0;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};

    /*
     * psm���ܼĴ�������, hipriv "Hisilicon0 lpm_psm_param 0|1(ps off|ps on) 0|1(DTIM|listen intval) xxx(listen
     * intervalֵ) xxx(TBTT offset)"
     */
    pst_psm_para = (mac_cfg_lpm_psm_param_stru *)(st_write_msg.auc_value);
    /* ��������������� */
    memset_s(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, 0, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    /* ��ȡ�����Ƿ��� */
    wal_get_cmd_one_arg((oal_int8 *)pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    pst_psm_para->uc_psm_on = (oal_uint8)oal_atoi(ac_name);
    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    /* ��ȡ��DTIM���ѻ���listen interval���� */
    wal_get_cmd_one_arg((oal_int8 *)pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    pst_psm_para->uc_psm_wakeup_mode = (oal_uint8)oal_atoi(ac_name);
    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    /* ��ȡlisten interval��ֵ */
    wal_get_cmd_one_arg((oal_int8 *)pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    pst_psm_para->us_psm_listen_interval = (oal_uint8)oal_atoi(ac_name);
    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    /* ��ȡTBTT�ж���ǰ����ֵ */
    wal_get_cmd_one_arg((oal_int8 *)pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    pst_psm_para->us_psm_tbtt_offset = (oal_uint8)oal_atoi(ac_name);
    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_PSM_PARAM, OAL_SIZEOF(mac_cfg_lpm_psm_param_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_psm_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_psm_param::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_lpm_smps_mode(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set;
    oal_uint8 uc_smps_mode;

    wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);

    uc_smps_mode = (oal_uint8)oal_atoi(ac_name);
    if (uc_smps_mode >= 3) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_smps_mode::invalid choice [%d]!}\r\n", uc_smps_mode);

        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }
    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    /* ��������������� */
    *((oal_uint8 *)(st_write_msg.auc_value)) = uc_smps_mode;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_SMPS_MODE, OAL_SIZEOF(oal_uint8));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_smps_mode::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_lpm_smps_stub(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    mac_cfg_lpm_smps_stub_stru *pst_smps_stub = OAL_PTR_NULL;
    oal_uint32 ul_off_set = 0;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};

    /* smps ap������׮, hipriv "vap0 lpm_smps_stub 0|1|2(off|����|˫��) 0|1(�Ƿ�RTS) */
    /* ��������������� */
    pst_smps_stub = (mac_cfg_lpm_smps_stub_stru *)(st_write_msg.auc_value);
    memset_s(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, 0, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    /* ��ȡ׮���� */
    wal_get_cmd_one_arg((oal_int8 *)pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    pst_smps_stub->uc_stub_type = (oal_uint8)oal_atoi(ac_name);
    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    /* RTS */
    wal_get_cmd_one_arg((oal_int8 *)pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    pst_smps_stub->uc_rts_en = (oal_uint8)oal_atoi(ac_name);
    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_SMPS_STUB, OAL_SIZEOF(mac_cfg_lpm_smps_stub_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_smps_stub_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_smps_stub::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_lpm_txopps_set(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    mac_cfg_lpm_txopps_set_stru *pst_txopps_set = OAL_PTR_NULL;
    oal_uint32 ul_off_set = 0;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};

    /*
     * txop ps���ܼĴ�������, hipriv "Hisilicon0 lpm_txopps_set 0|1(off|on|debug) 0|1(contion1 off|on) 0|1(condition2
     * off|on)"
     */
    /* ��������������� */
    pst_txopps_set = (mac_cfg_lpm_txopps_set_stru *)(st_write_msg.auc_value);
    memset_s(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, 0, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    /* ��ȡ�����Ƿ��� */
    wal_get_cmd_one_arg((oal_int8 *)pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    pst_txopps_set->uc_txop_ps_on = (oal_uint8)oal_atoi(ac_name);
    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    /* ��ȡcondition1 */
    wal_get_cmd_one_arg((oal_int8 *)pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    pst_txopps_set->uc_conditon1 = (oal_uint8)oal_atoi(ac_name);
    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    /* ��ȡcondition2 */
    wal_get_cmd_one_arg((oal_int8 *)pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    pst_txopps_set->uc_conditon2 = (oal_uint8)oal_atoi(ac_name);
    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_TXOP_PS_SET, OAL_SIZEOF(mac_cfg_lpm_txopps_set_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_txopps_set_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_txopps_set::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_lpm_txopps_tx_stub(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    mac_cfg_lpm_txopps_tx_stub_stru *pst_txopps_tx_stub = OAL_PTR_NULL;
    oal_uint32 ul_off_set = 0;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};

    /* txop ps�������Դ�׮����, hipriv "vap0 lpm_txopps_tx_stub 0|1(off|on) xxx(�ڼ�������׮)" */
    /* ��������������� */
    pst_txopps_tx_stub = (mac_cfg_lpm_txopps_tx_stub_stru *)(st_write_msg.auc_value);
    memset_s(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, 0, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    /* ��ȡ׮���� */
    wal_get_cmd_one_arg((oal_int8 *)pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    pst_txopps_tx_stub->uc_stub_on = (oal_uint8)oal_atoi(ac_name);
    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    /* ��ȡ�ڼ������Ĵ�׮ */
    wal_get_cmd_one_arg((oal_int8 *)pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    pst_txopps_tx_stub->us_begin_num = (oal_uint8)oal_atoi(ac_name);
    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_TXOP_TX_STUB, OAL_SIZEOF(mac_cfg_lpm_txopps_tx_stub_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_txopps_tx_stub_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_txopps_tx_stub::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_lpm_tx_data(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    mac_cfg_lpm_tx_data_stru *pst_lpm_tx_data = OAL_PTR_NULL;
    oal_uint32 ul_off_set = 0;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};

    /* ���Է���, hipriv "vap0 lpm_tx_data xxx(����) xxx(����) xx:xx:xx:xx:xx:xx(Ŀ��mac) xxx(AC����)" */
    pst_lpm_tx_data = (mac_cfg_lpm_tx_data_stru *)(st_write_msg.auc_value);
    memset_s(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, 0, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    /* ��ȡ�������� */
    wal_get_cmd_one_arg((oal_int8 *)pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    pst_lpm_tx_data->us_num = (oal_uint16)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* ��ȡ�������� */
    wal_get_cmd_one_arg((oal_int8 *)pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    pst_lpm_tx_data->us_len = (oal_uint16)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* ��ȡĿ�ĵ�ַ */
    wal_get_cmd_one_arg((oal_int8 *)pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    oal_strtoaddr(ac_name, pst_lpm_tx_data->auc_da);
    pc_param = pc_param + ul_off_set;

    /* ��ȡ����AC���� */
    wal_get_cmd_one_arg((oal_int8 *)pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    pst_lpm_tx_data->uc_ac = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;
    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_TX_DATA, OAL_SIZEOF(mac_cfg_lpm_tx_data_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_tx_data_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_tx_data::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_lpm_tx_probe_request(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    mac_cfg_lpm_tx_data_stru *pst_lpm_tx_data = OAL_PTR_NULL;
    oal_uint32 ul_off_set = 0;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};

    /* ���Է���, hipriv "vap0 lpm_tx_probe_request 0|1(����|����) xx:xx:xx:xx:xx:xx(����ģʽ��BSSID)" */
    pst_lpm_tx_data = (mac_cfg_lpm_tx_data_stru *)(st_write_msg.auc_value);
    memset_s(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, 0, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    /* ��ȡ����or����probe request */
    wal_get_cmd_one_arg((oal_int8 *)pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    pst_lpm_tx_data->uc_positive = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* ��ȡbssid */
    wal_get_cmd_one_arg((oal_int8 *)pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    oal_strtoaddr(ac_name, pst_lpm_tx_data->auc_da);
    pc_param = pc_param + ul_off_set;

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_TX_PROBE_REQUEST, OAL_SIZEOF(mac_cfg_lpm_tx_data_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_tx_data_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_lpm_tx_probe_request::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_lpm_wow_en(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set;
    mac_cfg_lpm_wow_en_stru *pst_lpm_wow_en = OAL_PTR_NULL;

    pst_lpm_wow_en = (mac_cfg_lpm_wow_en_stru *)(st_write_msg.auc_value);
    memset_s(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, 0, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    pst_lpm_wow_en->uc_en = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    pst_lpm_wow_en->uc_null_wake = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;
    /***************************************************************************
                                ���¼���wal�㴦��
    ***************************************************************************/
    /* ��������������� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_WOW_EN, OAL_SIZEOF(mac_cfg_lpm_wow_en_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_wow_en_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_wow_en::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


oal_uint32 wal_hipriv_remove_user_lut(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    mac_cfg_remove_lut_stru *pst_param = OAL_PTR_NULL; /* ���︴��ɾ���û���������Ľṹ�� */
    oal_uint32 ul_off_set = 0;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_uint8 auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    oal_uint16 us_user_idx;

    /* ɾ���ָ��û�lut��, hipriv "vap0 remove_lut xx:xx:xx:xx:xx:xx(mac��ַ) 0|1(�ָ�/ɾ��)" */
    pst_param = (mac_cfg_remove_lut_stru *)(st_write_msg.auc_value);

    /* ��ȡMAC��ַ�ַ��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_remove_user_lut::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
    }

    /* ��ַ�ַ���ת��ַ���� */
    oal_strtoaddr(ac_name, auc_mac_addr);

    /* ��ȡ �ָ�/ɾ�� ��ʶ */
    pc_param += ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_remove_user_lut::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    pst_param->uc_is_remove = (oal_uint8)oal_atoi(ac_name);

    /* ����mac��ַ���û� */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);

    ul_ret = mac_vap_find_user_by_macaddr(pst_mac_vap, auc_mac_addr, &us_user_idx);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_remove_user_lut::no such user!}\r\n");

        return ul_ret;
    }

    pst_param->us_user_idx = us_user_idx;

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_REMOVE_LUT, OAL_SIZEOF(mac_cfg_kick_user_param_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_kick_user_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_hipriv_remove_user_lut::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_send_frame(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_offset;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    mac_cfg_send_frame_param_stru *pst_test_send_frame = OAL_PTR_NULL;
    oal_uint8 auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};

    mac_test_frame_type_enum_uint8 en_frame_type;
    oal_uint8 uc_pkt_num;

    /* ��ȡ֡���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_offset);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_frame::get frame type err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    en_frame_type = (mac_test_frame_type_enum_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_offset;

    /* ��ȡ֡��Ŀ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_offset);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_frame::get frame num err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    uc_pkt_num = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_offset;

    /* ��ȡMAC��ַ�ַ��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_offset);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_frame::get mac err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    /* ��ַ�ַ���ת��ַ���� */
    oal_strtoaddr(ac_name, auc_mac_addr);
    pc_param += ul_offset;

    /* **************************************************************************
                                 ���¼���dmac�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SEND_FRAME, OAL_SIZEOF(mac_cfg_send_frame_param_stru));

    /* ��������������� */
    pst_test_send_frame = (mac_cfg_send_frame_param_stru *)(st_write_msg.auc_value);
    oal_set_mac_addr(pst_test_send_frame->auc_mac_ra, auc_mac_addr);
    pst_test_send_frame->en_frame_type = en_frame_type;
    pst_test_send_frame->uc_pkt_num = uc_pkt_num;

    ul_ret = (oal_uint32)wal_send_cfg_event(pst_net_dev,
                                            WAL_MSG_TYPE_WRITE,
                                            WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_send_frame_param_stru),
                                            (oal_uint8 *)&st_write_msg,
                                            OAL_FALSE,
                                            OAL_PTR_NULL);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_frame::wal_send_cfg_event return err_code [%d]!}\r\n",
            ul_ret);
        return (oal_uint32)ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_rx_pn(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_ret;
    oal_int32 l_cfg_rst;
    oal_uint16 us_len;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;
    oal_uint8 auc_mac_addr[OAL_MAC_ADDR_LEN];
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    oal_uint16 us_user_idx;
    mac_cfg_set_rx_pn_stru *pst_rx_pn = OAL_PTR_NULL;
    oal_uint16 us_pn;
    /* ��ȡMAC��ַ�ַ��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rx_pn::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    /* ��ַ�ַ���ת��ַ���� */
    oal_strtoaddr(ac_name, auc_mac_addr);
    pc_param += ul_off_set;

    /* ��ȡpn�� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rx_pn::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    us_pn = (oal_uint16)oal_atoi(ac_name);
    pc_param += ul_off_set;

    /* ����mac��ַ���û� */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    ul_ret = mac_vap_find_user_by_macaddr(pst_mac_vap, auc_mac_addr, &us_user_idx);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_rx_pn::no such user!}\r\n");

        return ul_ret;
    }

    pst_rx_pn = (mac_cfg_set_rx_pn_stru *)(st_write_msg.auc_value);
    pst_rx_pn->us_rx_pn = us_pn;
    pst_rx_pn->us_user_idx = us_user_idx;
    /* **************************************************************************
                              ���¼���wal�㴦��
    ************************************************************************** */
    us_len = OAL_SIZEOF(mac_cfg_set_rx_pn_stru);
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_RX_PN_REG, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_FALSE,
                                   OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_cfg_rst != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_hipriv_set_rx_pn::wal_send_cfg_event return err_code [%d]!}\r\n", l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_soft_retry(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_ret;
    oal_int32 l_cfg_rst;
    oal_uint16 us_len;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;
    oal_uint8 uc_software_retry;
    oal_uint8 uc_retry_test;
    mac_cfg_set_soft_retry_stru *pst_soft_retry = OAL_PTR_NULL;
    /* �Ƿ�Ϊtest�����ֵ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_soft_retry::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    uc_retry_test = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;

    /* ��ȡ�趨��ֵ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_soft_retry::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    uc_software_retry = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;

    pst_soft_retry = (mac_cfg_set_soft_retry_stru *)(st_write_msg.auc_value);
    pst_soft_retry->uc_retry_test = uc_retry_test;
    pst_soft_retry->uc_software_retry = uc_software_retry;
    /* **************************************************************************
                              ���¼���wal�㴦��
    ************************************************************************** */
    us_len = OAL_SIZEOF(mac_cfg_set_soft_retry_stru);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_SOFT_RETRY, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_FALSE,
                                   OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_cfg_rst != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_soft_retry::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_open_addr4(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_ret;
    oal_int32 l_cfg_rst;
    oal_uint16 us_len;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;
    oal_uint8 uc_open_addr4;

    /* ��ȡ�趨��ֵ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_open_addr4::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    uc_open_addr4 = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;

    /* **************************************************************************
                              ���¼���wal�㴦��
    ************************************************************************** */
    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_open_addr4;
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_OPEN_ADDR4, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_FALSE,
                                   OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_cfg_rst != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_open_addr4::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_open_wmm_test(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_ret;
    oal_int32 l_cfg_rst;
    oal_uint16 us_len;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;
    oal_uint8 uc_open_wmm;

    /* ��ȡ�趨��ֵ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_open_wmm_test::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    uc_open_wmm = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;

    /* **************************************************************************
                              ���¼���wal�㴦��
    ************************************************************************** */
    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_open_wmm;
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_OPEN_WMM_TEST, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_FALSE,
                                   OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_cfg_rst != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_open_wmm_test::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_chip_test_open(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_ret;
    oal_int32 l_cfg_rst;
    oal_uint16 us_len;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;
    oal_uint8 uc_chip_test_open;

    /* ��ȡ�趨��ֵ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_chip_test_open::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    uc_chip_test_open = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;

    /* **************************************************************************
                              ���¼���wal�㴦��
    ************************************************************************** */
    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_chip_test_open;
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_CHIP_TEST_OPEN, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_FALSE,
                                   OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_cfg_rst != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_chip_test_open::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_coex(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_ret;
    oal_int32 l_cfg_rst;
    oal_uint16 us_len;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;
    oal_uint32 ul_mac_ctrl;
    oal_uint32 ul_rf_ctrl;
    mac_cfg_coex_ctrl_param_stru *pst_coex_ctrl = OAL_PTR_NULL;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_coex::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    ul_mac_ctrl = (oal_uint32)oal_atoi(ac_name);
    pc_param += ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_coex::wal_get_cmd_2nd_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    ul_rf_ctrl = (oal_uint32)oal_atoi(ac_name);
    pc_param += ul_off_set;

    /* **************************************************************************
                              ���¼���wal�㴦��
    ************************************************************************** */
    pst_coex_ctrl = (mac_cfg_coex_ctrl_param_stru *)(st_write_msg.auc_value);
    pst_coex_ctrl->ul_mac_ctrl = ul_mac_ctrl;
    pst_coex_ctrl->ul_rf_ctrl = ul_rf_ctrl;

    us_len = OAL_SIZEOF(mac_cfg_coex_ctrl_param_stru);
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_COEX, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_FALSE,
                                   OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_cfg_rst != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_coex::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_dfx(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_tmp;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32 l_ret;
    oal_uint32 ul_ret;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_dfx::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* ��Խ������Ĳ�ͬ�����logģ����в�ͬ������ */
    if ((oal_strcmp("0", ac_name)) == 0) {
        l_tmp = 0;
    } else if ((oal_strcmp("1", ac_name)) == 0) {
        l_tmp = 1;
    } else {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_dfx::the log switch command is error [%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DFX_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp; /* ��������������� */

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_dfx::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)

OAL_STATIC oal_uint32 wal_hipriv_enable_pmf(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_ret;

    oal_int32 l_cfg_rst;
    oal_uint16 us_len;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;
    oal_uint8 uc_chip_test_open;

    /* ��ȡ�趨��ֵ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_enable_pmf::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    uc_chip_test_open = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;

    /* **************************************************************************
                              ���¼���wal�㴦��
    ************************************************************************** */
    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_chip_test_open;
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PMF_ENABLE, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_FALSE,
                                   OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_cfg_rst != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_enable_pmf::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32 wal_hipriv_test_send_action(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_ret;
    oal_int32 l_cfg_rst;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;
    mac_cfg_send_action_param_stru st_action_param;

    memset_s(&st_action_param, OAL_SIZEOF(mac_cfg_send_action_param_stru), 0,
        OAL_SIZEOF(mac_cfg_send_action_param_stru));

    /* ��ȡuc_category�趨��ֵ */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_test_send_action::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    st_action_param.uc_category = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;

    /* ��ȡĿ�ĵ�ַ */
    ul_ret = wal_get_cmd_one_arg((oal_int8 *)pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_test_send_action::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, st_action_param.auc_mac_da);
    pc_param = pc_param + ul_off_set;
    /* **************************************************************************
                              ���¼���wal�㴦��
    ************************************************************************** */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, &st_action_param,
        OAL_SIZEOF(mac_cfg_send_action_param_stru)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_test_send_action::memcpy_s failed!");
        return OAL_FAIL;
    }
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SEND_ACTION, OAL_SIZEOF(mac_cfg_send_action_param_stru));

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_send_action_param_stru),
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_FALSE,
                                   OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_cfg_rst != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_test_send_action::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_send_pspoll(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SEND_PSPOLL, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_pspoll::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_send_nulldata(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    mac_cfg_tx_nulldata_stru *pst_tx_nulldata = OAL_PTR_NULL;
    oal_uint32 ul_off_set = 0;
    oal_int8                     ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;

    pst_tx_nulldata = (mac_cfg_tx_nulldata_stru *)st_write_msg.auc_value;
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_nulldata::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_tx_nulldata->l_is_psm = oal_atoi((const oal_int8 *)ac_name);

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_nulldata::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_tx_nulldata->l_is_qos = oal_atoi((const oal_int8 *)ac_name);

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_nulldata::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_tx_nulldata->l_tidno = oal_atoi((const oal_int8 *)ac_name);

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SEND_NULLDATA, OAL_SIZEOF(mac_cfg_tx_nulldata_stru));
    if (memcpy_s((oal_void *)st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, (const oal_void *)pst_tx_nulldata,
        OAL_SIZEOF(mac_cfg_tx_nulldata_stru)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_send_nulldata::memcpy_s failed!");
        return OAL_FAIL;
    }

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_tx_nulldata_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_nulldata::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_clear_all_stat(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;

    /* **************************************************************************
                                 ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_CLEAR_ALL_STAT, OAL_SIZEOF(oal_uint32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_clear_all_stat::wal_send_cfg_event return err code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif


OAL_STATIC oal_uint32 wal_hipriv_set_mib(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_ret;
    oal_int32 l_cfg_rst;
    oal_uint16 us_len;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;
    oal_uint32 ul_mib_idx;
    oal_uint32 ul_mib_value;
    mac_cfg_set_mib_stru *pst_set_mib = OAL_PTR_NULL;

    /* ��ȡ�趨mib���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mib::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    ul_mib_idx = (oal_uint32)oal_atoi(ac_name);

    /* ��ȡ�趨�� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mib::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return (oal_uint32)ul_ret;
    }
    pc_param += ul_off_set;
    ul_mib_value = (oal_uint32)oal_atoi(ac_name);

    pst_set_mib = (mac_cfg_set_mib_stru *)(st_write_msg.auc_value);
    pst_set_mib->ul_mib_idx = ul_mib_idx;
    pst_set_mib->ul_mib_value = ul_mib_value;
    us_len = OAL_SIZEOF(mac_cfg_set_mib_stru);
    /* **************************************************************************
                              ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_MIB, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_FALSE,
                                   OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_cfg_rst != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mib::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_get_mib(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_ret;
    oal_int32 l_cfg_rst;
    oal_uint16 us_len;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;
    oal_uint32 ul_mib_idx;

    /* ��ȡmib���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_mib::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    ul_mib_idx = (oal_uint32)oal_atoi(ac_name);

    us_len = OAL_SIZEOF(oal_uint32);
    *(oal_uint32 *)(st_write_msg.auc_value) = ul_mib_idx;
    /* **************************************************************************
                              ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_MIB, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_FALSE,
                                   OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_cfg_rst != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_mib::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_thruput_bypass(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_ret;
    oal_int32 l_cfg_rst;
    oal_uint16 us_len;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;
    oal_uint8 uc_bypass_type;
    oal_uint8 uc_value;
    mac_cfg_set_thruput_bypass_stru *pst_set_bypass = OAL_PTR_NULL;

    /* ��ȡ�趨mib���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_thruput_bypass::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    uc_bypass_type = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡ�趨�� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_thruput_bypass::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return (oal_uint32)ul_ret;
    }
    pc_param += ul_off_set;
    uc_value = (oal_uint8)oal_atoi(ac_name);

    pst_set_bypass = (mac_cfg_set_thruput_bypass_stru *)(st_write_msg.auc_value);
    pst_set_bypass->uc_bypass_type = uc_bypass_type;
    pst_set_bypass->uc_value = uc_value;
    us_len = OAL_SIZEOF(mac_cfg_set_thruput_bypass_stru);
    /* **************************************************************************
                              ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_THRUPUT_BYPASS, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_FALSE,
                                   OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_cfg_rst != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_thruput_bypass::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
OAL_STATIC oal_uint32 wal_hipriv_set_auto_freq(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_ret;
    oal_int32 l_cfg_rst;
    oal_uint16 us_len;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;
    oal_uint8 uc_cmd_type;
    oal_uint8 uc_value;
    mac_cfg_set_auto_freq_stru *pst_set_auto_freq = OAL_PTR_NULL;

    /* ��ȡ�趨mib���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_thruput_bypass::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    uc_cmd_type = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡ�趨�� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_thruput_bypass::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return (oal_uint32)ul_ret;
    }
    pc_param += ul_off_set;
    uc_value = (oal_uint8)oal_atoi(ac_name);

    pst_set_auto_freq = (mac_cfg_set_auto_freq_stru *)(st_write_msg.auc_value);
    pst_set_auto_freq->uc_cmd_type = uc_cmd_type;
    pst_set_auto_freq->uc_value = uc_value;
    us_len = OAL_SIZEOF(mac_cfg_set_thruput_bypass_stru);
    /* **************************************************************************
                              ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_AUTO_FREQ_ENABLE, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_FALSE,
                                   OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_cfg_rst != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_thruput_bypass::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}
#endif
#ifdef _PRE_WLAN_DFT_STAT

OAL_STATIC oal_uint32 wal_hipriv_performance_log_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_ret;
    oal_int32 l_cfg_rst;
    oal_uint16 us_len;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;
    oal_uint8 uc_performance_switch_type;
    oal_uint8 uc_value;
    mac_cfg_set_performance_log_switch_stru *pst_set_performance_log_switch = OAL_PTR_NULL;

    /* ��ȡ�趨mib���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_performance_log_switch::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    uc_performance_switch_type = (oal_uint8)oal_atoi(ac_name);

    /* ��ȡ�趨�� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_performance_log_switch::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return (oal_uint32)ul_ret;
    }
    pc_param += ul_off_set;
    uc_value = (oal_uint8)oal_atoi(ac_name);

    pst_set_performance_log_switch = (mac_cfg_set_performance_log_switch_stru *)(st_write_msg.auc_value);
    pst_set_performance_log_switch->uc_performance_log_switch_type = uc_performance_switch_type;
    pst_set_performance_log_switch->uc_value = uc_value;
    us_len = OAL_SIZEOF(mac_cfg_set_performance_log_switch_stru);
    OAM_WARNING_LOG2(0, OAM_SF_ANY,
        "{wal_hipriv_performance_log_switch::uc_performance_switch_type = %d, uc_value = %d!}\r\n",
        uc_performance_switch_type, uc_value);
    /* **************************************************************************
                              ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_PERFORMANCE_LOG_SWITCH, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_FALSE,
                                   OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_cfg_rst != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_performance_log_switch::wal_send_cfg_event return err_code [%d]!}\r\n", l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32 wal_hipriv_set_auto_protection(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_ret;
    oal_int32 l_cfg_rst;
    oal_uint16 us_len;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;
    oal_uint32 ul_auto_protection_flag;

    /* ��ȡmib���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_auto_protection::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    ul_auto_protection_flag = (oal_uint32)oal_atoi(ac_name);

    us_len = OAL_SIZEOF(oal_uint32);
    *(oal_uint32 *)(st_write_msg.auc_value) = ul_auto_protection_flag;
    /* **************************************************************************
                              ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_AUTO_PROTECTION, us_len);

    l_cfg_rst = wal_send_cfg_event(pst_net_dev,
                                   WAL_MSG_TYPE_WRITE,
                                   WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                                   (oal_uint8 *)&st_write_msg,
                                   OAL_FALSE,
                                   OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_cfg_rst != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_auto_protection::wal_send_cfg_event return err_code [%d]!}\r\n", l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP

OAL_STATIC oal_uint32 wal_hipriv_proxyarp_on(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_bool_enum_uint8 en_proxyarp_on;
    mac_proxyarp_en_stru *pst_proxyarp_on_param = OAL_PTR_NULL;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_proxyarp_on::get cmd  err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    en_proxyarp_on = (oal_uint8)oal_atoi(ac_name);
    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PROXYARP_EN, OAL_SIZEOF(mac_proxyarp_en_stru));

    /* ��������������� */
    pst_proxyarp_on_param = (mac_proxyarp_en_stru *)(st_write_msg.auc_value);
    pst_proxyarp_on_param->en_proxyarp = en_proxyarp_on;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_proxyarp_en_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_proxyarp_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32 wal_hipriv_proxyarp_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PROXYARP_INFO, OAL_SIZEOF(mac_cfg_m2u_snoop_on_param_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_proxyarp_info::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif /* #ifdef _PRE_DEBUG_MODE */

#endif /* #ifdef _PRE_WLAN_FEATURE_PROXY_ARP */

#ifdef _PRE_WLAN_FEATURE_WAPI

#ifdef _PRE_WAPI_DEBUG
OAL_STATIC oal_uint32 wal_hipriv_show_wapi_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    mac_cfg_user_info_param_stru *pst_user_info_param = OAL_PTR_NULL;
    oal_uint8 auc_mac_addr[6] = { 0 }; /* ��ʱ�����ȡ��use��mac��ַ��Ϣ */
    oal_uint8 uc_char_index;
    oal_uint16 us_user_idx;
    /* ȥ���ַ����Ŀո� */
    pc_param++;

    /* ��ȡmac��ַ,16����ת�� */
    for (uc_char_index = 0; uc_char_index < 12; uc_char_index++) {
        if (*pc_param == ':') {
            pc_param++;
            if (uc_char_index != 0) {
                uc_char_index--;
            }

            continue;
        }

        auc_mac_addr[uc_char_index / 2] =
            (oal_uint8)(auc_mac_addr[uc_char_index / 2] * 16 * (uc_char_index % 2) + oal_strtohex(pc_param));
        pc_param++;
    }

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_WAPI_INFO, OAL_SIZEOF(mac_cfg_user_info_param_stru));

    /* ����mac��ַ���û� */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);

    l_ret = (oal_int32)mac_vap_find_user_by_macaddr(pst_mac_vap, auc_mac_addr, &us_user_idx);
    if (l_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_user_info::no such user!}\r\n");
        return OAL_FAIL;
    }

    /* ��������������� */
    pst_user_info_param = (mac_cfg_user_info_param_stru *)(st_write_msg.auc_value);
    pst_user_info_param->us_user_idx = us_user_idx;

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_hipriv_show_wapi_info::us_user_idx %u", us_user_idx);
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_user_info_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_hipriv_user_info::return err code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif /* #ifdef WAPI_DEBUG_MODE */

#endif /* #ifdef _PRE_WLAN_FEATURE_WAPI */


OAL_STATIC oal_uint32 wal_hipriv_send_2040_coext(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint32 ul_ret;
    oal_uint32 ul_off_set = 0;
    mac_cfg_set_2040_coexist_stru *pst_2040_coexist = OAL_PTR_NULL;

    /* **************************************************************************
                              ���¼���wal�㴦��
    ************************************************************************** */
    pst_2040_coexist = (mac_cfg_set_2040_coexist_stru *)st_write_msg.auc_value;
    /* ��ȡmib���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_2040_coext::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_2040_coexist->ul_coext_info = (oal_uint32)oal_atoi(ac_name);

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_2040_coext::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_2040_coexist->ul_channel_report = (oal_uint32)oal_atoi(ac_name);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SEND_2040_COEXT, OAL_SIZEOF(mac_cfg_set_2040_coexist_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_2040_coexist_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_2040_coext::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_2040_coext_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint16 us_len;

    if (OAL_UNLIKELY(OAL_STRLEN(pc_param) >= WAL_MSG_WRITE_MAX_LEN)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_2040_coext_info:: pc_param overlength is %d}\n",
            OAL_STRLEN(pc_param));
        oal_print_hex_dump((oal_uint8 *)pc_param, WAL_MSG_WRITE_MAX_LEN, 32,
            "wal_hipriv_2040_coext_info: param is overlong:");
        return OAL_FAIL;
    }

    /* **************************************************************************
                              ���¼���wal�㴦��
    ************************************************************************** */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, pc_param, OAL_STRLEN(pc_param)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_2040_coext_info::memcpy_s failed!");
        return OAL_FAIL;
    }

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_2040_COEXT_INFO, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_2040_coext_info::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_get_version(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint16 us_len;

    /* **************************************************************************
                              ���¼���wal�㴦��
    ************************************************************************** */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, pc_param, OAL_STRLEN(pc_param)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_get_version::memcpy_s failed!");
        return OAL_FAIL;
    }

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_VERSION, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_version::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY

OAL_STATIC oal_uint32 wal_hipriv_set_opmode_notify(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint16 us_len;
    oal_uint32 ul_ret;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;
    oal_uint8 uc_value;

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_opmode_notify::wal_get_cmd_one_arg fails!}\r\n");
        return ul_ret;
    }

    pc_param += ul_off_set;
    uc_value = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);

    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_value;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_OPMODE_NOTIFY, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG,
            "{wal_hipriv_set_opmode_notify::wal_hipriv_reset_device return err code = [%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_get_user_nssbw(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_add_user_param_stru *pst_add_user_param = OAL_PTR_NULL;
    mac_cfg_add_user_param_stru st_add_user_param; /* ��ʱ�����ȡ��use����Ϣ */
    oal_uint32 ul_get_addr_idx;

    /* ��ȡ�û�����Ϳռ�����Ϣ: hipriv "vap0 add_user xx xx xx xx xx xx(mac��ַ)" */
    memset_s((oal_void *)&st_add_user_param, OAL_SIZEOF(mac_cfg_add_user_param_stru), 0,
        OAL_SIZEOF(mac_cfg_add_user_param_stru));
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_add_user::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, sizeof(ac_name), st_add_user_param.auc_mac_addr, WLAN_MAC_ADDR_LEN);

    /* ƫ�ƣ�ȡ��һ������ */
    pc_param = pc_param + ul_off_set;

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    /* ��������������� */
    pst_add_user_param = (mac_cfg_add_user_param_stru *)(st_write_msg.auc_value);
    for (ul_get_addr_idx = 0; ul_get_addr_idx < WLAN_MAC_ADDR_LEN; ul_get_addr_idx++) {
        pst_add_user_param->auc_mac_addr[ul_get_addr_idx] = st_add_user_param.auc_mac_addr[ul_get_addr_idx];
    }

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_USER_RSSBW, OAL_SIZEOF(mac_cfg_add_user_param_stru));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_add_user_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_add_user::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif

#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32 wal_hipriv_rx_filter_val(oal_int8 **pc_param, hmac_cfg_rx_filter_stru *pst_rx_filter_val)
{
    oal_uint32 ul_off_set = 0;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;

    ul_ret = wal_get_cmd_one_arg(*pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_rx_filter_val::wal_get_cmd_one_arg return err_code[%d]}\r\n",
            ul_ret);
        return ul_ret;
    }

    *pc_param += ul_off_set;

    pst_rx_filter_val->uc_dev_mode = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);
    if (pst_rx_filter_val->uc_dev_mode > 1) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_rx_filter_val::st_rx_filter_val.uc_dev_mode is exceed.[%d]}\r\n",
            pst_rx_filter_val->uc_dev_mode);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    ul_ret = wal_get_cmd_one_arg(*pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_rx_filter_val::wal_get_cmd_one_arg return err_code[%d]}\r\n",
            ul_ret);
        return ul_ret;
    }
    *pc_param += ul_off_set;

    pst_rx_filter_val->uc_vap_mode = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);

    if (pst_rx_filter_val->uc_vap_mode >= WLAN_VAP_MODE_BUTT) {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_hipriv_rx_filter_val::uc_dev_mode is exceed! uc_dev_mode = [%d].}\r\n",
            pst_rx_filter_val->uc_vap_mode);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    ul_ret = wal_get_cmd_one_arg(*pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rx_filter_val::wal_get_cmd_one_arg return err_code[%d]}\r\n",
            ul_ret);
        return ul_ret;
    }
    *pc_param += ul_off_set;
    pst_rx_filter_val->uc_vap_status = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);

    if (pst_rx_filter_val->uc_vap_status >= MAC_VAP_STATE_BUTT) {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_hipriv_rx_filter_val::uc_dev_mode is exceed! uc_dev_mode = [%d].}\r\n",
            pst_rx_filter_val->uc_vap_status);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_rx_filter_val(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    hmac_cfg_rx_filter_stru st_rx_filter_val = { 0 };
    wal_msg_write_stru st_write_msg;
    oal_int8 *pc_token = OAL_PTR_NULL;
    oal_int8 *pc_end = OAL_PTR_NULL;
    oal_int8 *pc_ctx = OAL_PTR_NULL;
    oal_int8 *pc_sep = " ";

    ul_ret = wal_hipriv_rx_filter_val(&pc_param, &st_rx_filter_val);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_rx_filter_val::wal_hipriv_rx_filter_val return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }

    /* 0--дĳһVAP״̬��֡����ֵ */
    st_rx_filter_val.uc_write_read = 0;

    /* ��ȡ��Ҫд���ֵ */
    pc_token = oal_strtok(pc_param, pc_sep, &pc_ctx);
    if (pc_token == NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_rx_filter_val::pc_token is null}\r\n");
        return OAL_FAIL;
    }

    st_rx_filter_val.ul_val = (oal_uint32)oal_strtol(pc_token, &pc_end, 16);

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RX_FILTER_VAL, OAL_SIZEOF(hmac_cfg_rx_filter_stru));

    /* ��������������� */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, &st_rx_filter_val,
        OAL_SIZEOF(hmac_cfg_rx_filter_stru)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_set_rx_filter_val::memcpy_s failed!");
        return OAL_FAIL;
    }

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(hmac_cfg_rx_filter_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rx_filter_val::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_get_rx_filter_val(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    hmac_cfg_rx_filter_stru st_rx_filter_val = { 0 };
    wal_msg_write_stru st_write_msg;

    ul_ret = wal_hipriv_rx_filter_val(&pc_param, &st_rx_filter_val);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_set_rx_filter_val::wal_hipriv_rx_filter_val return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }

    /* 1--��ĳһVAP״̬��֡����ֵ */
    st_rx_filter_val.uc_write_read = 1;
    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RX_FILTER_VAL, OAL_SIZEOF(hmac_cfg_rx_filter_stru));

    /* ��������������� */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, &st_rx_filter_val,
        OAL_SIZEOF(hmac_cfg_rx_filter_stru)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_set_rx_filter_val::memcpy_s failed!");
        return OAL_FAIL;
    }

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(hmac_cfg_rx_filter_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rx_filter_val::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_rx_filter_en(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint16 us_len;
    oal_uint32 ul_ret;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;
    oal_uint8 uc_rx_filter_en;

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_vap_nss::wal_get_cmd_one_arg fails!}\r\n");
        return ul_ret;
    }

    pc_param += ul_off_set;
    uc_rx_filter_en = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);

    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_rx_filter_en;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_RX_FILTER_EN, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG,
            "{wal_hipriv_set_rx_filter_en::wal_hipriv_reset_device return err code = [%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_get_rx_filter_en(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint16 us_len;

    /* **************************************************************************
                              ���¼���wal�㴦��
    ************************************************************************** */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, pc_param, OAL_STRLEN(pc_param)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_get_version::memcpy_s failed!");
        return OAL_FAIL;
    }

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_RX_FILTER_EN, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_version::wal_send_cfg_event return err_code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY

OAL_STATIC oal_uint32 wal_hipriv_blacklist_add(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint16 us_len;
    oal_uint32 ul_ret;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_blacklist_add:wal_get_cmd_one_arg fail!}\r\n");
        return ul_ret;
    }
    memset_s((oal_uint8 *)&st_write_msg, OAL_SIZEOF(wal_msg_write_stru), 0, OAL_SIZEOF(wal_msg_write_stru));

    oal_strtoaddr(ac_name, sizeof(ac_name),
        st_write_msg.auc_value, WLAN_MAC_ADDR_LEN); /* ���ַ� ac_name ת�������� mac_add[6] */

    us_len = OAL_MAC_ADDR_LEN;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADD_BLACK_LIST, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_blacklist_add:wal_send_cfg_event return[%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_blacklist_del(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint16 us_len;
    oal_uint32 ul_ret;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_blacklist_add:wal_get_cmd_one_arg fail!}\r\n");
        return ul_ret;
    }
    memset_s((oal_uint8 *)&st_write_msg, OAL_SIZEOF(wal_msg_write_stru), 0, OAL_SIZEOF(wal_msg_write_stru));

    oal_strtoaddr(ac_name, sizeof(ac_name),
        st_write_msg.auc_value, WLAN_MAC_ADDR_LEN); /* ���ַ� ac_name ת�������� mac_add[6] */

    us_len = OAL_MAC_ADDR_LEN;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DEL_BLACK_LIST, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_blacklist_add:wal_send_cfg_event return[%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_send_cfg_uint32_data(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param,
    wlan_cfgid_enum_uint16 cfgid)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;
    oal_uint16 us_len;
    oal_uint32 ul_ret;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set = 0;
    oal_uint32 set_value;

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_send_cfg_uint32_data:wal_get_cmd_one_arg fail!}\r\n");
        return ul_ret;
    }

    pc_param += ul_off_set;
    set_value = (oal_uint32)oal_atoi((const oal_int8 *)ac_name);

    us_len = 4;
    *(oal_uint32 *)(st_write_msg.auc_value) = set_value;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, cfgid, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_send_cfg_uint32_data:wal_send_cfg_event return [%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_blacklist_mode(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data(pst_net_dev, pc_param, WLAN_CFGID_BLACKLIST_MODE);
}


OAL_STATIC oal_uint32 wal_hipriv_blacklist_show(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data(pst_net_dev, pc_param, WLAN_CFGID_BLACKLIST_SHOW);
}


OAL_STATIC oal_uint32 wal_hipriv_set_abl_on(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data(pst_net_dev, pc_param, WLAN_CFGID_AUTOBLACKLIST_ON);
}


OAL_STATIC oal_uint32 wal_hipriv_set_abl_aging(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data(pst_net_dev, pc_param, WLAN_CFGID_AUTOBLACKLIST_AGING);
}


OAL_STATIC oal_uint32 wal_hipriv_set_abl_threshold(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data(pst_net_dev, pc_param, WLAN_CFGID_AUTOBLACKLIST_THRESHOLD);
}


OAL_STATIC oal_uint32 wal_hipriv_set_abl_reset(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data(pst_net_dev, pc_param, WLAN_CFGID_AUTOBLACKLIST_RESET);
}


OAL_STATIC oal_uint32 wal_hipriv_set_isolation_mode(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data(pst_net_dev, pc_param, WLAN_CFGID_ISOLATION_MODE);
}


OAL_STATIC oal_uint32 wal_hipriv_set_isolation_type(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data(pst_net_dev, pc_param, WLAN_CFGID_ISOLATION_TYPE);
}


OAL_STATIC oal_uint32 wal_hipriv_set_isolation_fwd(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data(pst_net_dev, pc_param, WLAN_CFGID_ISOLATION_FORWARD);
}


OAL_STATIC oal_uint32 wal_hipriv_set_isolation_clear(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data(pst_net_dev, pc_param, WLAN_CFGID_ISOLATION_CLEAR);
}


OAL_STATIC oal_uint32 wal_hipriv_set_isolation_show(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data(pst_net_dev, pc_param, WLAN_CFGID_ISOLATION_SHOW);
}

#endif /* _PRE_WLAN_FEATURE_CUSTOM_SECURITY */
#ifdef _PRE_WLAN_FEATURE_MCAST

OAL_STATIC oal_uint32 wal_hipriv_m2u_snoop_on(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint8 uc_m2u_snoop_on;
    oal_uint8 uc_m2u_mcast_mode;
    mac_cfg_m2u_snoop_on_param_stru *pst_m2u_snoop_on_param = OAL_PTR_NULL;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_m2u_snoop_on::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    uc_m2u_snoop_on = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_m2u_snoop_on::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    uc_m2u_mcast_mode = (oal_uint8)oal_atoi(ac_name);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_M2U_SNOOP_ON, OAL_SIZEOF(mac_cfg_m2u_snoop_on_param_stru));

    /* ��������������� */
    pst_m2u_snoop_on_param = (mac_cfg_m2u_snoop_on_param_stru *)(st_write_msg.auc_value);
    pst_m2u_snoop_on_param->uc_m2u_snoop_on = uc_m2u_snoop_on;
    pst_m2u_snoop_on_param->uc_m2u_mcast_mode = uc_m2u_mcast_mode;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_m2u_snoop_on_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_m2u_snoop_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_m2u_add_deny_table(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint32 ul_deny_group_addr;
    mac_add_m2u_deny_table_stru *pst_m2u_deny_table_param = OAL_PTR_NULL;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_m2u_add_deny_table::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    ul_deny_group_addr = oal_in_aton((oal_uint8 *)ac_name);

    pc_param = pc_param + ul_off_set;

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_ADD_M2U_DENY_TABLE, OAL_SIZEOF(mac_add_m2u_deny_table_stru));

    /* ��������������� */
    pst_m2u_deny_table_param = (mac_add_m2u_deny_table_stru *)(st_write_msg.auc_value);
    pst_m2u_deny_table_param->ul_deny_group_addr = ul_deny_group_addr;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_add_m2u_deny_table_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_m2u_add_deny_table::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_m2u_cfg_deny_table(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint8 uc_m2u_clear_deny_table;
    oal_uint8 uc_m2u_show_deny_table;
    mac_clg_m2u_deny_table_stru *pst_m2u_deny_table_param = OAL_PTR_NULL;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_m2u_cfg_deny_table::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    uc_m2u_clear_deny_table = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_m2u_cfg_deny_table::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    uc_m2u_show_deny_table = (oal_uint8)oal_atoi(ac_name);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_M2U_DENY_TABLE, OAL_SIZEOF(mac_clg_m2u_deny_table_stru));

    /* ��������������� */
    pst_m2u_deny_table_param = (mac_clg_m2u_deny_table_stru *)(st_write_msg.auc_value);
    pst_m2u_deny_table_param->uc_m2u_clear_deny_table = uc_m2u_clear_deny_table;
    pst_m2u_deny_table_param->uc_m2u_show_deny_table = uc_m2u_show_deny_table;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_clg_m2u_deny_table_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_m2u_snoop_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_m2u_show_snoop_table(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint8 uc_m2u_show_snoop_table;
    mac_show_m2u_snoop_table_stru *pst_m2u_show_snoop_table_param = OAL_PTR_NULL;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_m2u_cfg_deny_table::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    uc_m2u_show_snoop_table = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_SHOW_M2U_SNOOP_TABLE, OAL_SIZEOF(mac_show_m2u_snoop_table_stru));

    /* ��������������� */
    pst_m2u_show_snoop_table_param = (mac_show_m2u_snoop_table_stru *)(st_write_msg.auc_value);
    pst_m2u_show_snoop_table_param->uc_m2u_show_snoop_table = uc_m2u_show_snoop_table;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_show_m2u_snoop_table_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_m2u_snoop_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32 wal_hipriv_igmp_packet_xmit(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_mpdu_ampdu_tx_param_stru *pst_aggr_tx_on_param = OAL_PTR_NULL;
    oal_uint8 uc_packet_num;
    oal_uint8 uc_tid;
    oal_uint16 uc_packet_len;
    oal_uint8 auc_ra_addr[WLAN_MAC_ADDR_LEN] = {0};

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_packet_xmit::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    uc_tid = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    pc_param = pc_param + ul_off_set;
    uc_packet_num = (oal_uint8)oal_atoi(ac_name);

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_packet_xmit::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    uc_packet_len = (oal_uint16)oal_atoi(ac_name);
    pc_param += ul_off_set;

    /* ��ȡMAC��ַ�ַ��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_packet_xmit::get mac err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    /* ��ַ�ַ���ת��ַ���� */
    oal_strtoaddr(ac_name, sizeof(ac_name), auc_ra_addr, WLAN_MAC_ADDR_LEN);
    pc_param += ul_off_set;

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_IGMP_PACKET_XMIT, OAL_SIZEOF(mac_cfg_mpdu_ampdu_tx_param_stru));

    /* ��������������� */
    pst_aggr_tx_on_param = (mac_cfg_mpdu_ampdu_tx_param_stru *)(st_write_msg.auc_value);
    pst_aggr_tx_on_param->uc_packet_num = uc_packet_num;
    pst_aggr_tx_on_param->uc_tid = uc_tid;
    pst_aggr_tx_on_param->us_packet_len = uc_packet_len;
    oal_set_mac_addr(pst_aggr_tx_on_param->auc_ra_mac, auc_ra_addr);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_mpdu_ampdu_tx_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32 wal_hipriv_bgscan_enable(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32 ul_off_set;
    oal_int8 ac_stop[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    wal_msg_write_stru st_write_msg;
    oal_bool_enum_uint8 *pen_bgscan_enable_flag = OAL_PTR_NULL;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_stop, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set); // �����������2---->80
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "wal_hipriv_scan_stop: get first arg fail.");
        return OAL_FAIL;
    }

    /* **************************************************************************
                            ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFIGD_BGSCAN_ENABLE, OAL_SIZEOF(oal_bool_enum_uint8));

    /* ��������������� */
    pen_bgscan_enable_flag = (oal_bool_enum_uint8 *)(st_write_msg.auc_value);
    *pen_bgscan_enable_flag = (oal_bool_enum_uint8)oal_atoi(ac_stop);

    OAM_WARNING_LOG1(0, OAM_SF_SCAN, "wal_hipriv_scan_stop:: bgscan_enable_flag= %d.", *pen_bgscan_enable_flag);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_bool_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (l_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::wal_send_cfg_event fail.return err code [%d]!}",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32 wal_hipriv_scan_test(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32 ul_off_set;
    oal_int8 ac_name[15] = {0};
    oal_int8 ac_scan_type[15];
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint8 uc_bandwidth;
    wal_msg_write_stru st_write_msg;
    mac_ioctl_scan_test_config_stru *pst_scan_test = OAL_PTR_NULL;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_scan_type, OAL_SIZEOF(ac_scan_type), &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "wal_hipriv_scan_test: get first arg fail.");
        return OAL_FAIL;
    }

    pc_param = pc_param + ul_off_set;
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, OAL_SIZEOF(ac_name), &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "wal_hipriv_scan_test: get second arg fail.");
        return OAL_FAIL;
    }
    uc_bandwidth = (oal_uint8)oal_atoi(ac_name);

    /* **************************************************************************
                            ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SCAN_TEST, OAL_SIZEOF(mac_ioctl_scan_test_config_stru));

    /* ��������������� */
    pst_scan_test = (mac_ioctl_scan_test_config_stru *)(st_write_msg.auc_value);
    if (memcpy_s(pst_scan_test->ac_scan_type, sizeof(pst_scan_test->ac_scan_type), ac_scan_type,
        sizeof(ac_scan_type)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_scan_test::memcpy_s failed!");
        return OAL_FAIL;
    }
    pst_scan_test->en_bandwidth = uc_bandwidth;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_ioctl_scan_test_config_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (l_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::wal_send_cfg_event fail.return err[%d]!}", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif
#ifdef _PRE_DEBUG_MODE
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)

OAL_STATIC uint32_t wal_hipriv_mem_check(const oal_net_device_stru *pst_net_dev, const int8_t *pc_param)
{
    int32_t ret = -OAL_EFAIL;

    if ((pst_net_dev == OAL_PTR_NULL) || (pc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_mem_check::pst_net_dev or pc_param null ptr error [%d] [%d]!}\r\n",
            pst_net_dev, pc_param);
        return ret;
    }

    ret = device_mem_check();
    if (ret != OAL_SUCC) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_mem_check::fail!}");
    }
    return ret;
}
#endif
#endif
#ifdef _PRE_WLAN_FEATURE_STA_PM

OAL_STATIC oal_uint32 wal_hipriv_sta_ps_mode(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint8 uc_vap_ps_mode;
    mac_cfg_ps_mode_param_stru *pst_ps_mode_param = OAL_PTR_NULL;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sta_ps_enable::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    uc_vap_ps_mode = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_PS_MODE, OAL_SIZEOF(mac_cfg_ps_mode_param_stru));

    /* ��������������� */
    pst_ps_mode_param = (mac_cfg_ps_mode_param_stru *)(st_write_msg.auc_value);
    pst_ps_mode_param->uc_vap_ps_mode = uc_vap_ps_mode;

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ps_mode_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_hipriv_sta_ps_enable::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_PSM_DEBUG_MODE

OAL_STATIC oal_uint32 wal_hipriv_sta_ps_info(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint8 uc_psm_info_enable;
    oal_uint8 uc_psm_debug_mode;
    mac_cfg_ps_info_stru *pst_ps_info = OAL_PTR_NULL;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sta_ps_info::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    uc_psm_info_enable = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sta_ps_info::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    uc_psm_debug_mode = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SHOW_PS_INFO, OAL_SIZEOF(mac_cfg_ps_info_stru));

    /* ��������������� */
    pst_ps_info = (mac_cfg_ps_info_stru *)(st_write_msg.auc_value);
    pst_ps_info->uc_psm_info_enable = uc_psm_info_enable;
    pst_ps_info->uc_psm_debug_mode = uc_psm_debug_mode;

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ps_info_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_hipriv_sta_ps_info::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif


oal_uint32 wal_hipriv_sta_pm_on(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint8 uc_sta_pm_open;
    mac_cfg_ps_open_stru *pst_sta_pm_open = OAL_PTR_NULL;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sta_pm_open::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    uc_sta_pm_open = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_STA_PM_ON, OAL_SIZEOF(mac_cfg_ps_open_stru));

    /* ��������������� */
    pst_sta_pm_open = (mac_cfg_ps_open_stru *)(st_write_msg.auc_value);
    /* MAC_STA_PM_SWITCH_ON / MAC_STA_PM_SWITCH_OFF */
    pst_sta_pm_open->uc_pm_enable = uc_sta_pm_open;
    pst_sta_pm_open->uc_pm_ctrl_type = MAC_STA_PM_CTRL_TYPE_HOST;

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ps_open_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_hipriv_sta_pm_open::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_FEATURE_STA_UAPSD

OAL_STATIC oal_uint32 wal_hipriv_set_uapsd_para(oal_net_device_stru *pst_cfg_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    mac_cfg_uapsd_sta_stru *pst_uapsd_param = OAL_PTR_NULL;
    oal_uint8 uc_max_sp_len;
    oal_uint8 uc_ac;
    oal_uint8 uc_delivery_enabled[WLAN_WME_AC_BUTT];
    oal_uint8 uc_trigger_enabled[WLAN_WME_AC_BUTT];

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_uapsd_para::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }

    uc_max_sp_len = (oal_uint8)oal_atoi(ac_name);

    for (uc_ac = 0; uc_ac < WLAN_WME_AC_BUTT; uc_ac++) {
        pc_param = pc_param + ul_off_set;
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY,
                "{wal_hipriv_set_uapsd_para::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }

        /* delivery_enabled�Ĳ������� */
        uc_delivery_enabled[uc_ac] = (oal_uint8)oal_atoi(ac_name);

        /* trigger_enabled ���������� */
        uc_trigger_enabled[uc_ac] = (oal_uint8)oal_atoi(ac_name);
    }
    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_UAPSD_PARA, OAL_SIZEOF(mac_cfg_uapsd_sta_stru));

    /* ��������������� */
    pst_uapsd_param = (mac_cfg_uapsd_sta_stru *)(st_write_msg.auc_value);
    pst_uapsd_param->uc_max_sp_len = uc_max_sp_len;
    for (uc_ac = 0; uc_ac < WLAN_WME_AC_BUTT; uc_ac++) {
        pst_uapsd_param->uc_delivery_enabled[uc_ac] = uc_delivery_enabled[uc_ac];
        pst_uapsd_param->uc_trigger_enabled[uc_ac] = uc_trigger_enabled[uc_ac];
    }

    l_ret = wal_send_cfg_event(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_uapsd_sta_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_hipriv_set_uapsd_para::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_P2P
#ifdef _PRE_WLAN_CHIP_TEST

OAL_STATIC oal_uint32 wal_parse_ops_param(const oal_int8 *pc_param, mac_cfg_p2p_ops_param_stru *pst_p2p_ops_param)
{
    oal_uint32 ul_ret;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set;
    oal_int32 l_ct_window;

    /* ������һ���������Ƿ�ʹ��OPS ���� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_ops_param::wal_get_cmd_one_arg 1 return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    if ((oal_strcmp("0", ac_name)) == 0) {
        pst_p2p_ops_param->en_ops_ctrl = OAL_FALSE;
    } else if ((oal_strcmp("1", ac_name)) == 0) {
        pst_p2p_ops_param->en_ops_ctrl = OAL_TRUE;
    } else {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_ops_param::the log switch command[%c] is error!}",
            (oal_uint8)ac_name[0]);
        OAL_IO_PRINT("{wal_parse_ops_param::the log switch command is error [%6s....]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* �����ڶ���������OPS ����CT Window */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    OAL_IO_PRINT("wal_parse_ops_param:ct window %s\r\n", ac_name);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_ops_param::wal_get_cmd_one_arg 2 return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    l_ct_window = oal_atoi(ac_name);
    if (l_ct_window < 0 || l_ct_window > 255) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_ops_param::ct window out off range [%d]!}\r\n", l_ct_window);
        return OAL_FAIL;
    } else {
        pst_p2p_ops_param->uc_ct_window = (oal_uint8)l_ct_window;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_parse_noa_param(const oal_int8 *pc_param, mac_cfg_p2p_noa_param_stru *pst_p2p_noa_param)
{
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_off_set;
    oal_int32 l_count;
    oal_uint32 ul_ret;

    /* ������һ��������start_time */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_parse_noa_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    pst_p2p_noa_param->ul_start_time = (oal_uint32)oal_atoi(ac_name);

    /* �����ڶ���������dulration */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_noa_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    pst_p2p_noa_param->ul_duration = (oal_uint32)oal_atoi(ac_name);

    /* ����������������interval */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_noa_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    pst_p2p_noa_param->ul_interval = (oal_uint32)oal_atoi(ac_name);

    /* �������ĸ�������count */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_noa_param::wal_get_cmd_one_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    l_count = oal_atoi(ac_name);
    if (l_count < 0 || l_count > 255) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_ops_param::ct window out off range [%d]!}\r\n", l_count);
        return OAL_FAIL;
    } else {
        pst_p2p_noa_param->uc_count = (oal_uint8)l_count;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_p2p_ps(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    mac_cfg_p2p_ops_param_stru st_p2p_ops_param;
    mac_cfg_p2p_noa_param_stru st_p2p_noa_param;
    mac_cfg_p2p_stat_param_stru st_p2p_stat_param;
    oal_int32 l_ret;
    oal_uint32 ul_ret;
    oal_uint16 us_len;
    oal_uint32 l_memcpy_ret = EOK;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_set_p2p_ps::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* ��Խ������Ĳ�ͬ�����logģ����в�ͬ������ */
    if ((oal_strcmp("ops", ac_name)) == 0) {
        /* ����P2P OPS ���ܲ��� */
        ul_ret = wal_parse_ops_param(pc_param, &st_p2p_ops_param);
        if (ul_ret != OAL_SUCC) {
            return ul_ret;
        }
        OAM_INFO_LOG2(0, OAM_SF_CFG, "{wal_hipriv_set_p2p_ps ops::ctrl[%d], ct_window[%d]!}\r\n",
            st_p2p_ops_param.en_ops_ctrl, st_p2p_ops_param.uc_ct_window);
        us_len = OAL_SIZEOF(mac_cfg_p2p_ops_param_stru);
        WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_P2P_PS_OPS, OAL_SIZEOF(mac_cfg_p2p_ops_param_stru));
        l_memcpy_ret += memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, &st_p2p_ops_param,
            OAL_SIZEOF(mac_cfg_p2p_ops_param_stru));
    } else if ((oal_strcmp("noa", ac_name)) == 0) {
        /* ����P2P NOA ���ܲ��� */
        ul_ret = wal_parse_noa_param(pc_param, &st_p2p_noa_param);
        if (ul_ret != OAL_SUCC) {
            return ul_ret;
        }
        OAM_INFO_LOG4(0, OAM_SF_CFG,
            "{wal_hipriv_set_p2p_ps noa::start_time[%d], duration[%d], interval[%d], count[%d]!}\r\n",
            st_p2p_noa_param.ul_start_time, st_p2p_noa_param.ul_duration, st_p2p_noa_param.ul_interval,
            st_p2p_noa_param.uc_count);
        us_len = OAL_SIZEOF(mac_cfg_p2p_noa_param_stru);
        WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_P2P_PS_NOA, OAL_SIZEOF(mac_cfg_p2p_noa_param_stru));
        l_memcpy_ret += memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, &st_p2p_noa_param,
            OAL_SIZEOF(mac_cfg_p2p_noa_param_stru));
    } else if ((oal_strcmp("statistics", ac_name)) == 0) {
        /* ��ȡP2P����ͳ�� */
        /* �����������鿴����ͳ�� */
        ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_ops_param::wal_get_cmd_one_arg 1 return err_code [%d]!}\r\n",
                ul_ret);
            return ul_ret;
        }
        pc_param += ul_off_set;
        if ((oal_strcmp("0", ac_name)) == 0) {
            st_p2p_stat_param.uc_p2p_statistics_ctrl = 0;
        } else if ((oal_strcmp("1", ac_name)) == 0) {
            st_p2p_stat_param.uc_p2p_statistics_ctrl = 1;
        } else {
            OAM_WARNING_LOG0(0, OAM_SF_CFG, "{wal_hipriv_set_p2p_ps statistics::wrong parm\r\n}");
            return OAL_FAIL;
        }

        us_len = OAL_SIZEOF(mac_cfg_p2p_stat_param_stru);
        WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_P2P_PS_STAT, us_len);
        l_memcpy_ret += memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, &st_p2p_stat_param, us_len);
        if (l_memcpy_ret != EOK) {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_set_p2p_ps::memcpy_s failed!");
            return OAL_FAIL;
        }
        OAM_INFO_LOG2(0, OAM_SF_CFG, "{wal_hipriv_set_p2p_ps statistics::ctrl[%d], len:%d!}\r\n",
            st_p2p_stat_param.uc_p2p_statistics_ctrl, us_len);
    } else {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_p2p_ps::the log switch command is error [%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_p2p_ps::wal_send_cfg_event return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */
#endif /* _PRE_WLAN_FEATURE_P2P */

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32 wal_hipriv_set_ampdu_mmss(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    mac_cfg_ampdu_mmss_stru st_ampdu_mmss_cfg;
    oal_uint32 ul_ret;
    oal_int32 l_ret;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ampdu_mmss::wal_get_cmd_one_arg return err_code [%d]!}\r\n",
            ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    st_ampdu_mmss_cfg.uc_mmss_val = (oal_uint8)oal_atoi(ac_name);
    if (st_ampdu_mmss_cfg.uc_mmss_val > 7) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ampdu_mmss::mmss invilid [%d]!}\r\n",
            st_ampdu_mmss_cfg.uc_mmss_val);
        OAL_IO_PRINT("{wal_hipriv_set_ampdu_mmss::mmss invilid [%d]!}\r\n", st_ampdu_mmss_cfg.uc_mmss_val);

        return OAL_FAIL;
    }

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_AMPDU_MMSS, OAL_SIZEOF(st_ampdu_mmss_cfg));

    /* ��д��Ϣ�壬���� */
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, &st_ampdu_mmss_cfg, OAL_SIZEOF(st_ampdu_mmss_cfg)) !=
        EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_set_ampdu_mmss::memcpy_s failed!");
        return OAL_FAIL;
    }

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_ampdu_mmss_cfg),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ampdu_mmss::wal_send_cfg_event return err code [%d]!}\r\n",
            l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_PROFLING_MIPS
OAL_STATIC oal_uint32 wal_hipriv_set_mips(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32 l_mips_type;
    oal_int32 l_switch;
    wal_msg_write_stru st_write_msg;
    oal_mips_type_param_stru *pst_mips_type_param = OAL_PTR_NULL;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        return ul_ret;
    }
    l_mips_type = oal_atoi((const oal_int8 *)ac_name);

    ul_ret = wal_get_cmd_one_arg(pc_param + ul_off_set, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        return ul_ret;
    }
    l_switch = oal_atoi((const oal_int8 *)ac_name);

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_MIPS, OAL_SIZEOF(oal_mips_type_param_stru));
    pst_mips_type_param = (oal_mips_type_param_stru *)st_write_msg.auc_value;
    pst_mips_type_param->l_mips_type = l_mips_type;
    pst_mips_type_param->l_switch = l_switch;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_mips_type_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mips::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_show_mips(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32 l_mips_type;
    wal_msg_write_stru st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        return ul_ret;
    }
    l_mips_type = oal_atoi((const oal_int8 *)ac_name);

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SHOW_MIPS, OAL_SIZEOF(oal_uint32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_mips_type;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_mips::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD

oal_uint32 wal_hipriv_arp_offload_enable(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint32 ul_off_set;
    oal_switch_enum_uint8 en_switch;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    wal_msg_write_stru st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "wal_hipriv_arp_offload_enable return err_code: %d", ul_ret);
        return ul_ret;
    }
    en_switch = (oal_switch_enum_uint8)oal_atoi(ac_name);

    /* **************************************************************************
                                ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ENABLE_ARP_OFFLOAD, OAL_SIZEOF(oal_switch_enum_uint8));
    *(oal_switch_enum_uint8 *)(st_write_msg.auc_value) = en_switch;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_switch_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_arp_offload_enable::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


#ifdef _PRE_WLAN_TCP_OPT

OAL_STATIC oal_uint32 wal_hipriv_get_tcp_ack_stream_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_TCP_ACK_STREAM_INFO, OAL_SIZEOF(oal_uint32));

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_arpoffload_info::wal_send_cfg_event err[%d]!}", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_tcp_tx_ack_opt_enable(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint32 ul_val;
    wal_msg_write_stru st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_tcp_tx_ack_opt_enable::wal_get_cmd_one_arg vap name return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    pc_param += ul_off_set;

    ul_val = (oal_uint32)oal_atoi((const oal_int8 *)ac_name);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TX_TCP_ACK_OPT_ENALBE, OAL_SIZEOF(oal_uint32));

    /* ��������������� */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_val;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (l_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_show_arpoffload_info::wal_send_cfg_event fail.return err code [%ud]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_tcp_rx_ack_opt_enable(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32 ul_off_set;
    oal_int8 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32 ul_ret;
    oal_int32 l_ret;
    oal_uint32 ul_val;
    wal_msg_write_stru st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_hipriv_tcp_rx_ack_opt_enable::wal_get_cmd_one_arg vap name return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    ul_val = (oal_uint32)oal_atoi((const oal_int8 *)ac_name);

    /* **************************************************************************
                             ���¼���wal�㴦��
    ************************************************************************** */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RX_TCP_ACK_OPT_ENALBE, OAL_SIZEOF(oal_uint32));

    /* ��������������� */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_val;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_arpoffload_info::wal_send_cfg_event err[%d]!}\r\n", l_ret);
         return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_tcp_tx_ack_limit(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_val;
    wal_msg_write_stru              st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_tcp_tx_ack_limit::wal_get_cmd_one_arg vap name err[%d]!}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    ul_val = (oal_uint32)oal_atoi((const oal_int8 *)ac_name);

    /***************************************************************************
                             ���¼���wal�㴦��
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TX_TCP_ACK_OPT_LIMIT, OAL_SIZEOF(oal_uint32));

    /* ��������������� */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_val;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_arpoffload_info::wal_send_cfg_event err[%d]!}", l_ret);
         return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32  wal_hipriv_tcp_rx_ack_limit(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_val;
    wal_msg_write_stru              st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_tcp_tx_ack_limit::wal_get_cmd_one_arg vap name err[%d]!}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    ul_val = (oal_uint32)oal_atoi((const oal_int8 *)ac_name);

    /***************************************************************************
                             ���¼���wal�㴦��
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RX_TCP_ACK_OPT_LIMIT, OAL_SIZEOF(oal_uint32));

    /* ��������������� */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_val;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_arpoffload_info::wal_send_cfg_event err[%d]!}", l_ret);
         return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

oal_int32 wal_start_vap(oal_net_device_stru *pst_net_dev)
{
    wal_msg_write_stru      st_write_msg;
    oal_int32               l_ret;
    wal_msg_stru           *pst_rsp_msg = OAL_PTR_NULL;
    oal_uint32              ul_err_code;
#ifdef _PRE_WLAN_FEATURE_P2P
    wlan_p2p_mode_enum_uint8 en_p2p_mode;
    oal_wireless_dev_stru   *pst_wdev = OAL_PTR_NULL;
#endif

    if (OAL_UNLIKELY(pst_net_dev == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_start_vap::pst_net_dev is null ptr!}\r\n");
        return -OAL_EFAUL;
    }

    OAL_IO_PRINT("wal_start_vap,dev_name is:%.16s\n", pst_net_dev->name);
    /***************************************************************************
        ���¼���wal�㴦��
    ***************************************************************************/
    /* ��д��Ϣ */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_START_VAP, OAL_SIZEOF(mac_cfg_start_vap_param_stru));
    ((mac_cfg_start_vap_param_stru *)st_write_msg.auc_value)->pst_net_dev = pst_net_dev;
#ifdef _PRE_WLAN_FEATURE_P2P
    pst_wdev    = pst_net_dev->ieee80211_ptr;
    en_p2p_mode = wal_wireless_iftype_to_mac_p2p_mode(pst_wdev->iftype);
    if (en_p2p_mode == WLAN_P2P_BUTT) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_start_vap::wal_wireless_iftype_to_mac_p2p_mode return BUFF}\r\n");
        return -OAL_EINVAL;
    }
    ((mac_cfg_start_vap_param_stru *)st_write_msg.auc_value)->en_p2p_mode = en_p2p_mode;

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_start_vap::en_p2p_mode:%d}\r\n", en_p2p_mode);
#endif
    ((mac_cfg_start_vap_param_stru *)st_write_msg.auc_value)->en_mgmt_rate_init_flag = OAL_TRUE;

    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_start_vap_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_start_vap::wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* ��������Ϣ */
    ul_err_code = wal_check_and_release_msg_resp(pst_rsp_msg);
    if (ul_err_code != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_start_vap::hmac start vap fail, err code[%d]!}\r\n", ul_err_code);
        return -OAL_EINVAL;
    }
    if ((OAL_NETDEVICE_FLAGS(pst_net_dev) & OAL_IFF_RUNNING) == 0) {
        OAL_NETDEVICE_FLAGS(pst_net_dev) |= OAL_IFF_RUNNING;
    }

    /* APģʽ,����VAP��,�������Ͷ��� */
    oal_net_tx_wake_all_queues(pst_net_dev); /* �������Ͷ��� */

    return OAL_SUCC;
}


oal_int32  wal_stop_vap(oal_net_device_stru *pst_net_dev)
{
    wal_msg_write_stru      st_write_msg;
    wal_msg_stru           *pst_rsp_msg = OAL_PTR_NULL;
    oal_int32               l_ret;
#ifdef _PRE_WLAN_FEATURE_P2P
    wlan_p2p_mode_enum_uint8 en_p2p_mode;
    oal_wireless_dev_stru   *pst_wdev = OAL_PTR_NULL;
#endif

    if (OAL_UNLIKELY(pst_net_dev == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_stop_vap::pst_net_dev is null ptr!}\r\n");
        return -OAL_EFAUL;
    }

    /* �������up״̬������ֱ�ӷ��سɹ�,��ֹnetdevice״̬��VAP״̬��һ�µ���� */
    if ((OAL_NETDEVICE_FLAGS(pst_net_dev) & OAL_IFF_RUNNING) == 0) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_stop_vap::vap is already down,continue to reset hmac vap state.}\r\n");
    }

    OAL_IO_PRINT("wal_stop_vap,dev_name is:%.16s\n", pst_net_dev->name);

    /***************************************************************************
                           ���¼���wal�㴦��
    ***************************************************************************/
    /* ��дWID��Ϣ */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DOWN_VAP, OAL_SIZEOF(mac_cfg_down_vap_param_stru));
    ((mac_cfg_down_vap_param_stru *)st_write_msg.auc_value)->pst_net_dev = pst_net_dev;
#ifdef _PRE_WLAN_FEATURE_P2P
    pst_wdev    = pst_net_dev->ieee80211_ptr;
    en_p2p_mode = wal_wireless_iftype_to_mac_p2p_mode(pst_wdev->iftype);
    if (en_p2p_mode == WLAN_P2P_BUTT) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_stop_vap::wal_wireless_iftype_to_mac_p2p_mode return BUFF}\r\n");
        return -OAL_EINVAL;
    }
    ((mac_cfg_start_vap_param_stru *)st_write_msg.auc_value)->en_p2p_mode = en_p2p_mode;

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_stop_vap::en_p2p_mode:%d}\r\n", en_p2p_mode);
#endif

    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_down_vap_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if (wal_check_and_release_msg_resp(pst_rsp_msg) != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "wal_stop_vap::wal_check_and_release_msg_resp fail");
    }

    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_stop_vap::wal_alloc_cfg_event return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    return OAL_SUCC;
}


oal_int32 wal_init_wlan_vap(oal_net_device_stru *pst_net_dev)
{
    oal_net_device_stru        *pst_cfg_net_dev = OAL_PTR_NULL;
    wal_msg_write_stru          st_write_msg;
    wal_msg_stru               *pst_rsp_msg = OAL_PTR_NULL;
    oal_uint32                  err_code;
    mac_vap_stru               *pst_mac_vap = OAL_PTR_NULL;
    oal_wireless_dev_stru      *pst_wdev = OAL_PTR_NULL;
    mac_wiphy_priv_stru        *pst_wiphy_priv = OAL_PTR_NULL;
    mac_vap_stru               *pst_cfg_mac_vap = OAL_PTR_NULL;
    hmac_vap_stru              *pst_cfg_hmac_vap = OAL_PTR_NULL;
    mac_device_stru            *pst_mac_device = OAL_PTR_NULL;
    oal_int32                   l_ret;

    wlan_vap_mode_enum_uint8    en_vap_mode;
#ifdef _PRE_WLAN_FEATURE_P2P
    wlan_p2p_mode_enum_uint8    en_p2p_mode = WLAN_LEGACY_VAP_MODE;
#endif

    if (pst_net_dev == NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_vap::pst_net_dev is null!}\r\n");
        return -OAL_EINVAL;
    }

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (pst_mac_vap != NULL) {
        if (pst_mac_vap->en_vap_state != MAC_VAP_STATE_BUTT) {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_vap::pst_mac_vap is already exist}\r\n");
            return OAL_SUCC;
        }
        /* netdev�µ�vap�Ѿ���ɾ������Ҫ���´����͹��� */
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_init_wlan_vap::pst_mac_vap is already free, need creat again!!}");
        OAL_NET_DEV_PRIV(pst_net_dev) = OAL_PTR_NULL;
    }

    pst_wdev = OAL_NETDEVICE_WDEV(pst_net_dev);
    if (pst_wdev == NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_vap::pst_wdev is null!}\r\n");
        return -OAL_EFAUL;
    }

    pst_wiphy_priv  = (mac_wiphy_priv_stru *)oal_wiphy_priv(pst_wdev->wiphy);
    if (pst_wiphy_priv == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_vap::pst_wiphy_priv is null!}\r\n");
        return -OAL_EFAUL;
    }

    pst_mac_device  = pst_wiphy_priv->pst_mac_device;
    if (pst_mac_device == NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_vap::pst_mac_device is null!}\r\n");
        return -OAL_EFAUL;
    }

    pst_cfg_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->uc_cfg_vap_id);
    if (pst_cfg_mac_vap == NULL) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_init_wlan_vap::pst_cfg_mac_vap is null! vap_id:%d}", pst_mac_device->uc_cfg_vap_id);
        return -OAL_EFAUL;
    }
    pst_cfg_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_device->uc_cfg_vap_id);
    if (pst_cfg_hmac_vap == NULL) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_init_wlan_vap::pst_cfg_hmac_vap is null! vap_id:%d}", pst_mac_device->uc_cfg_vap_id);
        return -OAL_EFAUL;
    }

    pst_cfg_net_dev = pst_cfg_hmac_vap->pst_net_device;
    if (pst_cfg_net_dev == NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_vap::pst_cfg_net_dev is null!}\r\n");
        return -OAL_EFAUL;
    }

    /* ������WIFI��AP��ʱ����VAP */
    if ((pst_wdev->iftype == NL80211_IFTYPE_STATION) || (pst_wdev->iftype == NL80211_IFTYPE_P2P_DEVICE)) {
        if ((oal_strcmp("wlan0", pst_net_dev->name)) == 0) {
            en_vap_mode = WLAN_VAP_MODE_BSS_STA;
        }
#ifdef _PRE_WLAN_FEATURE_P2P
        else if ((oal_strcmp("p2p0", pst_net_dev->name)) == 0) {
            en_vap_mode = WLAN_VAP_MODE_BSS_STA;
            en_p2p_mode = WLAN_P2P_DEV_MODE;
        }
#endif
        else {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_vap::net_dev is not wlan0 or p2p0!}\r\n");
            return OAL_SUCC;
        }
    } else if (pst_wdev->iftype == NL80211_IFTYPE_AP) {
        en_vap_mode = WLAN_VAP_MODE_BSS_AP;
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_vap::net_dev is not wlan0 or p2p0!}\r\n");
        return OAL_SUCC;
    }

#ifdef _PRE_WLAN_FEATURE_P2P
    OAM_WARNING_LOG2(0, OAM_SF_ANY, "{wal_init_wlan_vap::en_vap_mode:%d,en_p2p_mode:%d}\r\n",
                     en_vap_mode, en_p2p_mode);
#endif

    /***************************************************************************
        ���¼���wal�㴦��
    ***************************************************************************/
    /* ��д��Ϣ */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADD_VAP, OAL_SIZEOF(mac_cfg_add_vap_param_stru));
    ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->pst_net_dev = pst_net_dev;
    ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->en_vap_mode = en_vap_mode;
    ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->uc_cfg_vap_indx = pst_cfg_mac_vap->uc_vap_id;
#ifdef _PRE_WLAN_FEATURE_P2P
    ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->en_p2p_mode = en_p2p_mode;
#endif
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->bit_11ac2g_enable =
        (oal_uint8)!!hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_11AC2G_ENABLE);
    ((mac_cfg_add_vap_param_stru *)st_write_msg.auc_value)->bit_disable_capab_2ght40 =
        g_st_wlan_customize.uc_disable_capab_2ght40;
#endif
    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_cfg_net_dev, WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_add_vap_param_stru),
                               (oal_uint8 *)&st_write_msg, OAL_TRUE, &pst_rsp_msg);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_cfg_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_init_wlan_vap::return err code %d!}", l_ret);
        return -OAL_EFAIL;
    }

    /* ��ȡ���صĴ����� */
    err_code = wal_check_and_release_msg_resp(pst_rsp_msg);
    if (err_code != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_cfg_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_init_wlan_vap::hmac add vap err[%u]}", err_code);
        return -OAL_EFAIL;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    if (wal_set_random_mac_to_mib(pst_net_dev) != OAL_SUCC) {
        OAM_WARNING_LOG0(pst_cfg_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_init_wlan_vap::wal_set_random_mac_to_mib err}");
        return -OAL_EFAUL;
    }
#endif

    /* ����netdevice��MAC��ַ��MAC��ַ��HMAC�㱻��ʼ����MIB�� */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (pst_mac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_vap::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr.}");
        return -OAL_EINVAL;
    }

#ifdef _PRE_WLAN_FEATURE_P2P
    if (en_p2p_mode == WLAN_P2P_DEV_MODE) {
        pst_mac_device->st_p2p_info.uc_p2p0_vap_idx = pst_mac_vap->uc_vap_id;
    }
#endif

    if (pst_wdev->iftype == NL80211_IFTYPE_AP) {
        /* APģʽ��ʼ������ʼ����������û�����mac��ַ����ģʽ */
        if (g_st_ap_config_info.ul_ap_max_user > 0) {
            wal_set_ap_max_user(pst_net_dev, g_st_ap_config_info.ul_ap_max_user);
        }

        if (OAL_STRLEN(g_st_ap_config_info.ac_ap_mac_filter_mode) > 0) {
            wal_config_mac_filter(pst_net_dev, g_st_ap_config_info.ac_ap_mac_filter_mode);
        }
    }

    return OAL_SUCC;
}


oal_int32 wal_deinit_wlan_vap(oal_net_device_stru *pst_net_dev)
{
    wal_msg_write_stru st_write_msg;
    wal_msg_stru      *pst_rsp_msg = OAL_PTR_NULL;
    mac_vap_stru      *pst_mac_vap = OAL_PTR_NULL;
    oal_int32          l_ret;
    oal_int32          l_del_vap_flag = OAL_TRUE;

#ifdef _PRE_WLAN_FEATURE_P2P
    wlan_p2p_mode_enum_uint8    en_p2p_mode = WLAN_LEGACY_VAP_MODE;
#endif

    if (OAL_UNLIKELY(pst_net_dev == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_deinit_wlan_vap::pst_del_vap_param null ptr !}\r\n");
        return -OAL_EINVAL;
    }

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (pst_mac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_deinit_wlan_vap::pst_mac_vap is already null}\r\n");
        return OAL_SUCC;
    }

    /* ������WIFI��AP�ر�ʱɾ��VAP */
    if (((oal_strcmp("wlan0", pst_net_dev->name)) != 0) && ((oal_strcmp("p2p0", pst_net_dev->name)) != 0) &&
        ((oal_strcmp("wlan1", pst_net_dev->name)) != 0)) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_deinit_wlan_vap::net_dev is not wlan0 or wlan1 or p2p0!}");
        return OAL_SUCC;
    }

#ifdef _PRE_WLAN_FEATURE_P2P
    if (oal_strcmp("p2p0", pst_net_dev->name) == 0) {
        en_p2p_mode = WLAN_P2P_DEV_MODE;
    }
#endif

#ifdef _PRE_WLAN_FEATURE_P2P
    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_deinit_wlan_vap::en_p2p_mode:%d}\r\n", en_p2p_mode);
#endif

    /***************************************************************************
                                ���¼���wal�㴦��
    ***************************************************************************/
    // ɾ��vap ʱ��Ҫ��������ֵ��
    ((mac_cfg_del_vap_param_stru *)st_write_msg.auc_value)->pst_net_dev = pst_net_dev;
#ifdef _PRE_WLAN_FEATURE_P2P
    ((mac_cfg_del_vap_param_stru *)st_write_msg.auc_value)->en_p2p_mode = en_p2p_mode;
#endif

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DEL_VAP, OAL_SIZEOF(mac_cfg_del_vap_param_stru));
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_del_vap_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if (wal_check_and_release_msg_resp(pst_rsp_msg) != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "wal_deinit_wlan_vap::wal_check_and_release_msg_resp fail.");
        /* can't set net dev's vap ptr to null when
          del vap wid process failed! */
        l_del_vap_flag = OAL_FALSE;
    }

    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_deinit_wlan_vap::return error code %d}\r\n", l_ret);
        if ((l_ret == -OAL_ENOMEM) || (l_ret == -OAL_EFAIL)) {
            /* wid had't processed */
           l_del_vap_flag = OAL_FALSE;
        }
    }

    if (l_del_vap_flag == OAL_TRUE) {
        OAL_NET_DEV_PRIV(pst_net_dev) = NULL;
    }

    return l_ret;
}


oal_int32 wal_macaddr_check(const oal_uint8 *puc_macaddr)
{
    if ((mac_addr_is_zero(puc_macaddr) == OAL_TRUE) || ((puc_macaddr[0] & 0x1) == 0x1)) {
        return OAL_FAIL;
    }

   return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_11D

oal_uint32  wal_set_country(oal_uint8 *puc_param)
{
    oal_int32                        l_ret;
    oal_int8                        *puc_para = OAL_PTR_NULL;
    oal_net_device_stru             *pst_net_dev = OAL_PTR_NULL;

    if (puc_param == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "wal_set_country:: puc_param is NULL!");
        return OAL_FAIL;
    }

    pst_net_dev = oal_dev_get_by_name("Hisilicon0");
    if (pst_net_dev == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "wal_set_country:: pointer is NULL!");
        return OAL_FAIL;
    }
    /* �豸��up״̬���������ã�������down */
    if ((OAL_IFF_RUNNING & OAL_NETDEVICE_FLAGS(pst_net_dev)) != 0) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_set_country::country is %d, %d!}\r\n", OAL_NETDEVICE_FLAGS(pst_net_dev));
        return OAL_EBUSY;
    }

    puc_para = (oal_int8 *)puc_param;

    l_ret = wal_regdomain_update(pst_net_dev, puc_para);
    if (l_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_set_country::regdomain_update return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif /* #ifdef _PRE_WLAN_FEATURE_11D */

#ifdef _PRE_WLAN_FEATURE_STA_PM

oal_int32 wal_set_pm_on(oal_net_device_stru *pst_cfg_net_dev, const oal_void *p_buf)
{
    mac_vap_stru                       *pst_mac_vap = OAL_PTR_NULL;
    mac_cfg_ps_open_stru               *pst_sta_pm_open = OAL_PTR_NULL;
    oal_uint32                          res;
    wal_msg_write_stru                  st_write_msg;

#ifndef WIN32
    /* host�͹���û�п�,��ʱ����device�ĵ͹��� */
    if (!g_wlan_pm_switch) {
        return OAL_SUCC;
    }
#endif

    if (p_buf == OAL_PTR_NULL || pst_cfg_net_dev == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, 0, "hwal_ioctl_set_pm_on:p_buf parameter pr pst_cfg_net_dev NULL.");
        return -OAL_EFAIL;
    }

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_cfg_net_dev);
    if (pst_mac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, 0, "hwal_ioctl_set_pm_on:pst_mac_vap NULL.");
        return -OAL_EFAIL;
    }
    /***************************************************************************
                             ���¼���wal�㴦��
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_STA_PM_ON, OAL_SIZEOF(mac_cfg_ps_open_stru));

    /* ��������������� */
    pst_sta_pm_open = (mac_cfg_ps_open_stru *)(st_write_msg.auc_value);
    /* MAC_STA_PM_SWITCH_ON / MAC_STA_PM_SWITCH_OFF */
    pst_sta_pm_open->uc_pm_enable      = *((uint8_t *)p_buf);
    pst_sta_pm_open->uc_pm_ctrl_type   = MAC_STA_PM_CTRL_TYPE_HOST;

    res = wal_send_cfg_event(pst_cfg_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ps_open_stru),
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);
    if (res != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_PWR, "{hwal_ioctl_set_pm_on::wal_send_cfg_event err[%d]}", res);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}
#endif /* _PRE_WLAN_FEATURE_STA_PM */

#ifndef WIN32

oal_int32 wal_set_wlan_pm_state(oal_uint32 ul_pm_state)
{
    oal_uint32 ul_ret = OAL_FAIL;

    /* �ͻ�����: ��Ҫ��ҵ���ж�̬�Ŀ���ҵ��͹��ģ�ͨ��״̬��Ҫ����ҵ��͹��ĵ�״̬ */
    if (ul_pm_state == 1) {
        /* ҵ��͹��Ĵ򿪣�ͨ�����Խ���˯�߻��Ѷ��� */
        g_wlan_pm_switch = OAL_TRUE;
        ul_ret = wlan_pm_enable();
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG0(0, OAM_SF_CFG, "wal_set_wlan_pm_state:: pm enable fail!");
        }
        return ul_ret;
    } else if (ul_pm_state == 0) {
         /* ҵ��͹��Ĺرգ�ͨ��������˯�߻��Ѷ��� */
        g_wlan_pm_switch = OAL_FALSE;
        ul_ret = wlan_pm_disable();
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG0(0, OAM_SF_CFG, "wal_set_wlan_pm_state:: pm disable fail!");
        }
        return ul_ret;
    } else {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "wal_set_wlan_pm_state:: error state [%d]", ul_pm_state)
    }

    return ul_ret;
}
#endif /* WIN32 */

#ifndef WIN32
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#define ALTX_PARA_NUM 4
#define ALTX_PARA_LEN 20
#define ALRX_PARA_NUM 3
#define ALRX_PARA_LEN 20

static oal_int8 ac_mode_buffer[ALRX_PARA_LEN] = {0};


oal_int32  wal_rx_fcs_info(void)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_ret;
    mac_cfg_rx_fcs_info_stru   *pst_rx_fcs_info = OAL_PTR_NULL;
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    oal_int32                   i_leftime;
    mac_vap_stru               *pst_mac_vap = OAL_PTR_NULL;
    hmac_vap_stru              *pst_hmac_vap = OAL_PTR_NULL;
    oal_int32                   l_rx_pckg_succ_num = -1;
    oal_net_device_stru        *pst_net_dev = OAL_PTR_NULL;

    pst_net_dev = oal_dev_get_by_name("wlan0");
    if (pst_net_dev == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "wal_rx_fcs_info:: pointer is NULL!");
        return OAL_FAIL;
    }

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (pst_mac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_rx_fcs_info::OAL_NET_DEV_PRIV, return null!}");
        return OAL_EINVAL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_rx_fcs_info::mac_res_get_hmac_vap failed!}");
        return OAL_FAIL;
    }
#endif

    /***************************************************************************
                                ���¼���wal�㴦��
    ***************************************************************************/
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    pst_hmac_vap->st_atcmdsrv_get_status.uc_get_rx_pkct_flag = OAL_FALSE;
#endif
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RX_FCS_INFO, OAL_SIZEOF(mac_cfg_rx_fcs_info_stru));

    /* ��������������� */
    pst_rx_fcs_info = (mac_cfg_rx_fcs_info_stru *)(st_write_msg.auc_value);
    pst_rx_fcs_info->ul_data_op    = 1;
    pst_rx_fcs_info->ul_print_info = 2;

    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_rx_fcs_info_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_rx_fcs_info::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    /* �����ȴ�dmac�ϱ� */
    i_leftime = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(pst_hmac_vap->query_wait_q,
        (oal_uint32)(pst_hmac_vap->st_atcmdsrv_get_status.uc_get_rx_pkct_flag == OAL_TRUE), WAL_ATCMDSRB_GET_RX_PCKT);
    if (i_leftime == 0) {
        /* ��ʱ��û���ϱ�ɨ����� */
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_rx_fcs_info::dbb_num wait for %ld ms timeout!}",
                         ((WAL_ATCMDSRB_DBB_NUM_TIME * 1000) / OAL_TIME_HZ));
        return OAL_EINVAL;
    } else if (i_leftime < 0) {
        /* ��ʱ���ڲ����� */
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_rx_fcs_info::dbb_num wait for %ld ms error!}",
                         ((WAL_ATCMDSRB_DBB_NUM_TIME * 1000) / OAL_TIME_HZ));
        return OAL_EINVAL;
    } else {
        /* ��������  */
        OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_rx_fcs_info::dbb_num wait for %ld ms error!}",
                      ((WAL_ATCMDSRB_DBB_NUM_TIME * 1000) / OAL_TIME_HZ));

        l_rx_pckg_succ_num = (oal_long)pst_hmac_vap->st_atcmdsrv_get_status.ul_rx_pkct_succ_num;
        OAL_IO_PRINT("{wal_hipriv_rx_fcs_info} rx_pckg_succ_num is^%d^\n", l_rx_pckg_succ_num);
    }
#endif

    return l_rx_pckg_succ_num;
}


oal_uint32 wal_set_always_tx(const oal_int8 *pc_param)
{
    oal_uint32                  ul_ret;
    oal_int8                    ac_name[ALTX_PARA_LEN] = {0};
    oal_int8                    ac_open_tx[] = "1 2 2000";
    oal_int8                    ac_close_tx[] = "0";
    oal_int8                    ac_bw_20[] = "20";
    oal_int8                    ac_bw_40[] = "40";
    oal_int8                    *pc_bw = ac_bw_20;
    oal_int8                    ac_ng40_add[] = "plus";
    oal_net_device_stru         *pst_net_dev = OAL_PTR_NULL;
    oal_int32                   l_ret = EOK;

    pst_net_dev  = oal_dev_get_by_name("wlan0");
    if (OAL_ANY_NULL_PTR2(pc_param, pst_net_dev)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_set_always_tx:: pc_param, pst_net_devis NULL!");
        return OAL_ERR_CODE_PTR_NULL;
    }

    // ÿ�γ���ǰ���س���
    ul_ret = wal_hipriv_always_tx_1102(pst_net_dev, ac_close_tx);
    if (ul_ret != OAL_SUCC) {
         OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_set_always_tx::always_tx_113x return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    /* ����ǹرճ�������ֱ�ӹرգ������������Ĳ��� */
    if (!oal_strcmp(ac_close_tx, pc_param)) {
        return OAL_SUCC;
    }

    pc_param = pc_param + ALTX_PARA_LEN;
    // ����ģʽ
    // ģʽ��ͬ���Ͳ�����ģʽ�ʹ���
    memset_s(ac_mode_buffer, sizeof(ac_mode_buffer), 0, sizeof(ac_mode_buffer));
    l_ret += strncpy_s(ac_mode_buffer, ALRX_PARA_LEN, pc_param, sizeof(ac_mode_buffer));
    l_ret += strncpy_s(ac_name, ALTX_PARA_LEN, pc_param, sizeof(ac_name));
    if (l_ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_set_always_tx::strncpy_s failed !");
        return OAL_FAIL;
    }

    if (!oal_strcmp("11ng40", ac_name)) {
        if (strncat_s(ac_name, ALTX_PARA_LEN, ac_ng40_add, sizeof(ac_ng40_add)) != EOK) {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_set_always_tx::strncat_s failed !");
            return OAL_FAIL;
        }
        pc_bw = ac_bw_40;
    }
    ul_ret = wal_hipriv_set_mode(pst_net_dev, ac_name);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_set_always_tx::set_mode return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    ul_ret = wal_hipriv_set_bw(pst_net_dev, pc_bw);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_set_always_tx::set_bw return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    // ����Ƶ��
    pc_param = pc_param + ALTX_PARA_LEN;

    ul_ret = wal_hipriv_set_freq(pst_net_dev, pc_param);
    if (ul_ret != OAL_SUCC) {
         OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_set_always_tx::set_freq return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    // �������ʣ���ģʽ�ݴ棬�ж�ѡ��rate ����mcs
    pc_param = pc_param + ALTX_PARA_LEN;

    if (!oal_strstr(ac_mode_buffer, "11ng")) {
        ul_ret = wal_hipriv_set_rate(pst_net_dev, pc_param);
        if (ul_ret != OAL_SUCC) {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_set_always_tx::set_rate return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }
    } else {
        ul_ret = wal_hipriv_set_mcs(pst_net_dev, pc_param);
        if (ul_ret != OAL_SUCC) {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_set_always_tx::set_mcs return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }
    }

    // �򿪳���
    ul_ret = wal_hipriv_always_tx_1102(pst_net_dev, ac_open_tx);
    if (ul_ret != OAL_SUCC) {
         OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_set_always_tx::always_tx_1131 return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32 wal_set_always_rx(const oal_int8 *pc_param)
{
    oal_uint32                  ul_ret;
    oal_int8                    ac_name[ALRX_PARA_LEN] = {0};
    oal_int8                    ac_close_rx[] = "0";
    oal_int8                    ac_open_rx[] = "1";
    oal_int8                    ac_bw_20[] = "20";
    oal_int8                    ac_bw_40[] = "40";
    oal_int8                    *pc_bw = ac_bw_20;
    oal_int8                    ac_ng40_add[] = "plus";
    oal_net_device_stru         *pst_net_dev = OAL_PTR_NULL;
    oal_int32                   l_ret = EOK;

    pst_net_dev  = oal_dev_get_by_name("wlan0");

    if (pc_param == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_set_always_rx::ptr is null\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (pst_net_dev == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_set_always_rx::pst_net_dev is null\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    // ����ÿ�γ���ǰ���س���
    ul_ret = wal_hipriv_always_rx(pst_net_dev, ac_close_rx);
    if (ul_ret != OAL_SUCC) {
         OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_set_always_rx::always_rx_113x return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    /* ����ǹرճ��գ���ֱ�ӹرգ������������Ĳ��� */
    if (!oal_strcmp(ac_close_rx, pc_param)) {
        return OAL_SUCC;
    }

    pc_param = pc_param + ALRX_PARA_LEN;

    if (oal_strcmp(pc_param, ac_mode_buffer)) {
        memset_s(ac_mode_buffer, sizeof(ac_mode_buffer), 0, sizeof(ac_mode_buffer));
        l_ret += strncpy_s(ac_mode_buffer, ALRX_PARA_LEN, pc_param, sizeof(ac_mode_buffer));
        // ����ģʽ
        l_ret += strncpy_s(ac_name, ALRX_PARA_LEN, pc_param, sizeof(ac_name));
        if (l_ret != EOK) {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_set_always_rx::strncpy_s failed !\r\n");
            return OAL_FAIL;
        }

        if (!oal_strcmp("11ng40", ac_name)) {
            if (strncat_s(ac_name, ALRX_PARA_LEN, ac_ng40_add, sizeof(ac_ng40_add)) != EOK) {
                OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_set_always_rx::strncat_s failed !\r\n");
                return OAL_FAIL;
            }
            pc_bw = ac_bw_40;
        }

        ul_ret = wal_hipriv_set_mode(pst_net_dev, ac_name);
        if (ul_ret != OAL_SUCC) {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_set_always_rx::set_mode return err_code [%d]!", ul_ret);
            return ul_ret;
        }

        ul_ret = wal_hipriv_set_bw(pst_net_dev, pc_bw);
        if (ul_ret != OAL_SUCC) {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_set_always_rx::set_bw return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }
    }

    // ����Ƶ��
    pc_param = pc_param + ALRX_PARA_LEN;

    ul_ret = wal_hipriv_set_freq(pst_net_dev, pc_param);
    if (ul_ret != OAL_SUCC) {
         OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_set_always_rx::set_freq return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    // �򿪳���
    ul_ret = wal_hipriv_always_rx(pst_net_dev, ac_open_rx);
    if (ul_ret != OAL_SUCC) {
         OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_set_always_rx::always_rx_113x return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    return OAL_SUCC;
}
oal_int32 wal_update_macaddr_to_netdev_and_lwip(oal_uint8 *puc_macaddr)
{
    oal_net_device_stru *pst_net_dev = OAL_PTR_NULL;
    oal_uint32 l_ret = EOK;

    if (puc_macaddr == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "wal_update_macaddr_to_netdev_and_lwip:: pointer is NULL!");
        return OAL_FAIL;
    }

    pst_net_dev = oal_dev_get_by_name("wlan0");
    if (pst_net_dev == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "wal_update_macaddr_to_netdev_and_lwip:: pointer is NULL!");
        return OAL_FAIL;
    }

    if (wal_macaddr_check(puc_macaddr) != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "wal_update_macaddr_to_netdev_and_lwip:: Mac address is error!");
        return OAL_FAIL;
    }

    /* ����netdev�е�mac��ַ */
    l_ret += memcpy_s(pst_net_dev->dev_addr, sizeof(pst_net_dev->dev_addr), puc_macaddr, WLAN_MAC_ADDR_LEN);
    /* ����liwp�е�mac��ַ */
    if (pst_net_dev->pst_lwip_netif == NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "wal_update_macaddr_to_netdev_and_lwip:: pointer is NULL!");
        return OAL_FAIL;
    } else {
        l_ret += memcpy_s(pst_net_dev->pst_lwip_netif->hwaddr,
            sizeof(pst_net_dev->pst_lwip_netif->hwaddr), puc_macaddr, WLAN_MAC_ADDR_LEN);
    }
    if (l_ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_update_macaddr_to_netdev_and_lwip::memcpy_s failed!");
        return OAL_FAIL;
    }
    return OAL_SUCC;
}
#endif

oal_int32 wal_update_macaddr(oal_uint8 *puc_macaddr)
{
    oal_uint8       uc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};
    efuse_info_stru *pst_efuse_info = OAL_PTR_NULL;

    if (puc_macaddr == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "wal_update_macaddr:: pointer is NULL!");
        return OAL_FAIL;
    }

    /* Mac��ַ����ʹ���û����õ�ֵ������û�û�����ã���ʹ��оƬ��efuseֵ�����efuseֵδд�����������Mac��ַ */
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    if (wal_macaddr_check(g_auc_mac_addr) == OAL_SUCC) {
        if (memcpy_s(puc_macaddr, WLAN_MAC_ADDR_LEN + 1, g_auc_mac_addr, WLAN_MAC_ADDR_LEN) != EOK) {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_update_macaddr::memcpy_s failed!");
            return OAL_FAIL;
        }
        return OAL_SUCC;
    }else
#endif
    {
        pst_efuse_info = get_efuse_info_handler();
        if (pst_efuse_info == OAL_PTR_NULL) {
            return OAL_FAIL;
        }
    }

    uc_mac_addr[0] = pst_efuse_info->mac_h & 0xff;
    uc_mac_addr[1] = (pst_efuse_info->mac_h & 0xff00) >> 8;
    uc_mac_addr[2] = pst_efuse_info->mac_m & 0xff;
    uc_mac_addr[3] = (pst_efuse_info->mac_m & 0xff00) >> 8;
    uc_mac_addr[4] = pst_efuse_info->mac_l & 0xff;
    uc_mac_addr[5] = (pst_efuse_info->mac_l & 0xff00) >> 8;

    if (wal_macaddr_check(uc_mac_addr) == OAL_SUCC) {
        if (memcpy_s(puc_macaddr, WLAN_MAC_ADDR_LEN + 1, uc_mac_addr, WLAN_MAC_ADDR_LEN) != EOK) {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_update_macaddr::memcpy_s failed!");
            return OAL_FAIL;
        }
    } else {
        return OAL_FAIL;
    }
    return OAL_SUCC;
 }
#endif

oal_uint8* wal_get_mac_addr(OAL_CONST oal_int8 *pc_name)
{
    oal_net_device_stru        *pst_net_dev = OAL_PTR_NULL;

    if (OAL_PTR_NULL == pc_name) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_get_mac_addr pc_name NULL.}\r\n");
        return OAL_PTR_NULL;
    }

    pst_net_dev = oal_dev_get_by_name(pc_name);
    if (OAL_PTR_NULL == pst_net_dev) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_get_mac_addr: pst_net_dev is NULL!\n");
        return OAL_PTR_NULL;
    }

    return (oal_uint8 *)OAL_NETDEVICE_MAC_ADDR(pst_net_dev);
}

void wal_set_random_mac_addr(uint8_t *primary_mac_addr)
{
    oal_random_ether_addr(primary_mac_addr);
    primary_mac_addr[0] &= (~0x02);
    primary_mac_addr[1] = 0x11;
    primary_mac_addr[2] = 0x31;
}


oal_int32 wal_set_mac_addr(oal_net_device_stru *pst_net_dev)
{
    oal_uint8                     auc_primary_mac_addr[WLAN_MAC_ADDR_LEN] = { 0 };    /* MAC��ַ */
    oal_wireless_dev_stru        *pst_wdev = OAL_PTR_NULL;
    mac_wiphy_priv_stru          *pst_wiphy_priv = OAL_PTR_NULL;
    mac_device_stru              *pst_mac_device = OAL_PTR_NULL;

    pst_wdev = OAL_NETDEVICE_WDEV(pst_net_dev);
    pst_wiphy_priv = (mac_wiphy_priv_stru *)(oal_wiphy_priv(pst_wdev->wiphy));
    pst_mac_device = pst_wiphy_priv->pst_mac_device;

#ifdef _PRE_WLAN_FEATURE_P2P
    if (OAL_UNLIKELY(pst_mac_device->st_p2p_info.pst_primary_net_device == OAL_PTR_NULL)) {
        /* random mac will be used. hi1102-cb (#include <linux/etherdevice.h>)    */
        wal_set_random_mac_addr(auc_primary_mac_addr);
    } else {
#ifndef _PRE_PC_LINT
        if (OAL_LIKELY(OAL_NETDEVICE_MAC_ADDR(pst_mac_device->st_p2p_info.pst_primary_net_device) != OAL_PTR_NULL)) {
            if (memcpy_s(auc_primary_mac_addr, WLAN_MAC_ADDR_LEN,
                OAL_NETDEVICE_MAC_ADDR(pst_mac_device->st_p2p_info.pst_primary_net_device), WLAN_MAC_ADDR_LEN) != EOK) {
                OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_set_mac_addr::memcpy_s failed!");
                return OAL_FAIL;
            }
        } else {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_get_mac_addr() pst_primary_net_device; addr is null}\r\n");
            return OAL_FAIL;
        }
#endif
    }

    switch (pst_wdev->iftype) {
        case NL80211_IFTYPE_P2P_DEVICE: {
            /* ����P2P device MAC ��ַ��������mac ��ַbit ����Ϊ1 */
            auc_primary_mac_addr[0] |= 0x02;
            oal_set_mac_addr((oal_uint8 *)OAL_NETDEVICE_MAC_ADDR(pst_net_dev), auc_primary_mac_addr);

            break;
        }
        default: {
#if (defined _PRE_PLAT_FEATURE_CUSTOMIZE) && (defined HISI_NVRAM_SUPPORT)
            hwifi_get_mac_addr(auc_primary_mac_addr);
            auc_primary_mac_addr[0] &= (~0x02);
#else
#ifndef WIN32
            if (wal_update_macaddr(auc_primary_mac_addr) != OAL_SUCC)
#endif
            {
                wal_set_random_mac_addr(auc_primary_mac_addr);
            }
#endif
            oal_set_mac_addr((oal_uint8 *)OAL_NETDEVICE_MAC_ADDR(pst_net_dev), auc_primary_mac_addr);
            break;
        }
    }
#else
    wal_set_random_mac_addr(auc_primary_mac_addr);

    oal_set_mac_addr((oal_uint8 *)OAL_NETDEVICE_MAC_ADDR(pst_net_dev), auc_primary_mac_addr);
#endif

    return OAL_SUCC;
}


oal_int32 wal_init_wlan_netdev(oal_wiphy_stru *pst_wiphy, const char *device_name)
{
    oal_net_device_stru        *pst_net_dev = OAL_PTR_NULL;
    oal_wireless_dev_stru      *pst_wdev = OAL_PTR_NULL;
    mac_wiphy_priv_stru        *pst_wiphy_priv = OAL_PTR_NULL;
    enum nl80211_iftype         en_type;
    oal_int32                   l_ret;
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    struct netif               *pst_netif = NULL;
#endif
    if ((pst_wiphy == NULL) || (device_name == NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_netdev::pst_wiphy or dev_name is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if ((oal_strcmp("wlan0", device_name)) == 0) {
        en_type = NL80211_IFTYPE_STATION;
    } else if ((oal_strcmp("p2p0", device_name)) == 0) {
        en_type = NL80211_IFTYPE_P2P_DEVICE;
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_netdev::dev name is not wlan0 or p2p0}\r\n");
        return OAL_SUCC;
    }
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_init_wlan_netdev::en_type is %d}\r\n", en_type);

    /* ���������net device�Ѿ����ڣ�ֱ�ӷ��� */
    /* ����dev_name�ҵ�dev */
    pst_net_dev = oal_dev_get_by_name(device_name);
    if (pst_net_dev != OAL_PTR_NULL) {
        /* ����oal_dev_get_by_name�󣬱������oal_dev_putʹnet_dev�����ü�����һ */
        oal_dev_put(pst_net_dev);

        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_netdev::the net_device is already exist!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

#if defined(_PRE_WLAN_FEATURE_FLOWCTL)
    /* oal_net_alloc_netdev_mqs������һ����δ���˽�г��ȣ��˴����漰Ϊ0 */
    pst_net_dev = oal_net_alloc_netdev_mqs(0, device_name, oal_ether_setup, WAL_NETDEV_SUBQUEUE_MAX_NUM, 1);
#elif defined(_PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL)
    pst_net_dev = oal_net_alloc_netdev_mqs(0, device_name, oal_ether_setup, WLAN_NET_QUEUE_BUTT, 1);
#else
    pst_net_dev = oal_net_alloc_netdev(0, device_name, oal_ether_setup);
#endif
    if (OAL_UNLIKELY(pst_net_dev == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_netdev::oal_net_alloc_netdev return null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_wdev = (oal_wireless_dev_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, sizeof(oal_wireless_dev_stru), OAL_FALSE);
    if (OAL_UNLIKELY(pst_wdev == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_init_wlan_netdev::alloc mem, pst_wdev is null ptr!}\r\n");
        oal_net_free_netdev(pst_net_dev);
        return OAL_ERR_CODE_PTR_NULL;
    }

    memset_s(pst_wdev, OAL_SIZEOF(oal_wireless_dev_stru), 0, OAL_SIZEOF(oal_wireless_dev_stru));

    /* ��netdevice���и�ֵ */
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifdef CONFIG_WIRELESS_EXT
    pst_net_dev->wireless_handlers             = &g_st_iw_handler_def;
#endif /* CONFIG_WIRELESS_EXT */
#else
    pst_net_dev->wireless_handlers             = &g_st_iw_handler_def;
#endif
    pst_net_dev->netdev_ops                    = &g_st_wal_net_dev_ops;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    pst_net_dev->ethtool_ops                   = &g_st_wal_ethtool_ops;
#endif

    OAL_NETDEVICE_DESTRUCTOR(pst_net_dev)      = oal_net_free_netdev;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,10,44))
    OAL_NETDEVICE_MASTER(pst_net_dev)          = OAL_PTR_NULL;
#endif

    OAL_NETDEVICE_IFALIAS(pst_net_dev)         = OAL_PTR_NULL;
    OAL_NETDEVICE_WATCHDOG_TIMEO(pst_net_dev)  = 5;
    OAL_NETDEVICE_WDEV(pst_net_dev)            = pst_wdev;
    OAL_NETDEVICE_QDISC(pst_net_dev, OAL_PTR_NULL);

    pst_wdev->netdev = pst_net_dev;
    pst_wdev->iftype = en_type;
    pst_wdev->wiphy = pst_wiphy;
    pst_wiphy_priv = (mac_wiphy_priv_stru *)(oal_wiphy_priv(pst_wiphy));

#ifdef _PRE_WLAN_FEATURE_P2P
    if (en_type == NL80211_IFTYPE_STATION) {
        /* �������wlan0�� �򱣴�wlan0 Ϊ��net_device,p2p0 ��p2p-p2p0 MAC ��ַ����netdevice ��ȡ */
        pst_wiphy_priv->pst_mac_device->st_p2p_info.pst_primary_net_device = pst_net_dev;
    } else if (en_type == NL80211_IFTYPE_P2P_DEVICE) {
        pst_wiphy_priv->pst_mac_device->st_p2p_info.pst_p2p_net_device = pst_net_dev;
    }
#endif
    OAL_NETDEVICE_FLAGS(pst_net_dev) &= ~OAL_IFF_RUNNING;   /* ��net device��flag��Ϊdown */

    wal_set_mac_addr(pst_net_dev);

    /* ע��net_device */
    l_ret = oal_net_register_netdev(pst_net_dev);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_init_wlan_netdev::oal_net_register_netdev return error[%d]!}", l_ret);

        OAL_MEM_FREE(pst_wdev, OAL_FALSE);
        oal_net_free_netdev(pst_net_dev);

        return l_ret;
    }
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    if ((oal_strcmp("wlan0", device_name)) == 0) {
        pst_netif = netif_find("wlan0");
        if (pst_netif == OAL_PTR_NULL) {
           OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_init_wlan_netdev:pst_netif is null");
           return OAL_ERR_CODE_PTR_NULL;
        }
        netifapi_netif_set_default(pst_netif);
    }
#endif
    return OAL_SUCC;
}


oal_int32  wal_setup_ap(oal_net_device_stru *pst_net_dev)
{
    oal_int32 l_ret;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    wal_set_power_mgmt_on(OAL_FALSE);
    l_ret = wal_set_power_on(pst_net_dev, OAL_TRUE);
    if (l_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_setup_ap::wal_set_power_on fail [%d]!}", l_ret);
        return l_ret;
    }
#endif /* (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) */
    if (OAL_NETDEVICE_FLAGS(pst_net_dev) & OAL_IFF_RUNNING) {
        /* �л���APǰ��������豸����UP״̬����Ҫ��down wlan0�����豸 */
        OAL_IO_PRINT("wal_setup_ap:stop netdevice:%.16s", pst_net_dev->name);
        wal_netdev_stop(pst_net_dev);
    }

    pst_net_dev->ieee80211_ptr->iftype = NL80211_IFTYPE_AP;

    l_ret = wal_init_wlan_vap(pst_net_dev);
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    if (l_ret == OAL_SUCC) {
        hwifi_config_init_ini(pst_net_dev);
    }
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */
    return l_ret;
}

/*
 * ��������  : ��OTA����
 * 1.��    ��: 2019��6��22��
 *   �޸�����: �����ɺ���
 */
oal_uint32 wal_set_ota_test(oal_net_device_stru *pst_netdev)
{
    oal_int8      *pc_close_param = "0";
    oal_uint32      ul_ret;

    if (pst_netdev == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, 0, "wal_set_ota_test::pst_netdev null");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = wal_hipriv_pm_switch(pst_netdev, pc_close_param);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG0(0, 0, "wal_set_ota_test::wal_hipriv_pm_switch");
        return ul_ret;
    }
    ul_ret = wal_hipriv_bgscan_enable(pst_netdev, pc_close_param);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG0(0, 0, "wal_set_ota_test::wal_hipriv_bgscan_enable");
        return ul_ret;
    }
    ul_ret = wal_hipriv_alg_cfg(pst_netdev, "tpc_mode 1");
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG0(0, 0, "wal_set_ota_test::wal_hipriv_alg_cfg 1");
        return ul_ret;
    }
    ul_ret = wal_hipriv_alg_cfg(pst_netdev, "tpc_pow_lvl 0");
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG0(0, 0, "wal_set_ota_test::wal_hipriv_alg_cfg 2");
        return ul_ret;
    }
    ul_ret = wal_hipriv_set_auto_protection(pst_netdev, pc_close_param);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG0(0, 0, "wal_set_ota_test::wal_hipriv_set_auto_protection");
        return ul_ret;
    }
    ul_ret = wal_hipriv_alg_cfg(pst_netdev, "ar_rts_mode 0");
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG0(0, 0, "wal_set_ota_test::wal_hipriv_alg_cfg 3");
        return ul_ret;
    }

    return ul_ret;
}

#endif

#ifdef _PRE_WLAN_FEATURE_ROAM

oal_uint32 wal_hipriv_roam_enable(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32                 ul_ret;
    oal_int32                  l_ret;
    oal_uint32                 ul_off_set;
    oal_bool_enum_uint8        en_enable;
    oal_int8                   ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    wal_msg_write_stru         st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "roam enable type return err_code [%d]", ul_ret);
        return ul_ret;
    }
    en_enable = (oal_bool_enum_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                ���¼���wal�㴦��
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ROAM_ENABLE, OAL_SIZEOF(oal_uint32));
    *((oal_bool_enum_uint8 *)(st_write_msg.auc_value)) = en_enable;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_bool_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_roam_enable::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

oal_uint32 wal_hipriv_roam_org(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32                 ul_ret;
    oal_int32                  l_ret;
    oal_uint32                 ul_off_set;
    oal_uint8                  uc_org;
    oal_int8                   ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    wal_msg_write_stru         st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "roam org type return err_code[%d]", ul_ret);
        return ul_ret;
    }
    uc_org = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                ���¼���wal�㴦��
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ROAM_ORG, OAL_SIZEOF(oal_uint32));
    *((oal_uint8 *)(st_write_msg.auc_value)) = uc_org;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_bool_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{wal_hipriv_roam_org::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

oal_uint32 wal_hipriv_roam_band(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32                 ul_ret;
    oal_int32                  l_ret;
    oal_uint32                 ul_off_set;
    oal_uint8                  uc_band;
    oal_int8                   ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    wal_msg_write_stru         st_write_msg;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "roam band type return err_code[%d]", ul_ret);
        return ul_ret;
    }
    uc_band = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                ���¼���wal�㴦��
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ROAM_BAND, OAL_SIZEOF(oal_uint32));
    *((oal_uint8 *)(st_write_msg.auc_value)) = uc_band;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_bool_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{wal_hipriv_roam_band::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

oal_uint32 wal_hipriv_roam_start(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_int32                  l_ret;
    oal_bool_enum_uint8        en_enable;
    wal_msg_write_stru         st_write_msg;

    en_enable = 1;

    /***************************************************************************
                                ���¼���wal�㴦��
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ROAM_START, OAL_SIZEOF(oal_uint32));
    *((oal_bool_enum_uint8 *)(st_write_msg.auc_value)) = en_enable;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_bool_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{wal_hipriv_roam_enable::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


oal_uint32 wal_hipriv_roam_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_int32                  l_ret;
    oal_bool_enum_uint8        en_enable;
    wal_msg_write_stru         st_write_msg;

    en_enable = 1;

    /***************************************************************************
                                ���¼���wal�㴦��
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ROAM_INFO, OAL_SIZEOF(oal_uint32));
    *((oal_bool_enum_uint8 *)(st_write_msg.auc_value)) = en_enable;

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_bool_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{wal_hipriv_roam_enable::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif // _PRE_WLAN_FEATURE_ROAM

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST

OAL_STATIC oal_uint32  wal_hipriv_enable_2040bss(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                      ul_ret;
    oal_uint8                       uc_2040bss_switch;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
         OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_enable_2040bss::wal_get_cmd_one_arg return err[%d]!}", ul_ret);
         return ul_ret;
    }

    if ((oal_strcmp("0", ac_name) != 0) && (oal_strcmp("1", ac_name) != 0)) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{wal_hipriv_enable_2040bss::invalid parameter.}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    uc_2040bss_switch = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             ���¼���wal�㴦��
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_2040BSS_ENABLE, OAL_SIZEOF(oal_uint8));
    *((oal_uint8 *)(st_write_msg.auc_value)) = uc_2040bss_switch;  /* ��������������� */

    ul_ret = (oal_uint32)wal_send_cfg_event(pst_net_dev,
                                            WAL_MSG_TYPE_WRITE,
                                            WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                                            (oal_uint8 *)&st_write_msg,
                                            OAL_FALSE,
                                            OAL_PTR_NULL);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_enable_2040bss::return err code %d!}\r\n", ul_ret);
    }

    return ul_ret;
}
#endif /* _PRE_WLAN_FEATURE_20_40_80_COEXIST */


#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)

oal_int32 wal_netif_notify(oal_net_device_stru *pst_net_dev, oal_net_notify_stru *pst_notify)
{
    mac_vap_stru        *pst_mac_vap    = OAL_PTR_NULL;
    oal_uint32           ul_ip_addr;
    oal_uint32           notify_type;

    if ((pst_net_dev == OAL_PTR_NULL) || (pst_notify == OAL_PTR_NULL)) {
        OAM_ERROR_LOG2(0, 0, "wal_netif_notify: PARAM NULL, net_dev[0x%08x],notify[0x%08x].",
            pst_net_dev, pst_notify);
        return -OAL_EFAIL;
    }

    ul_ip_addr      = pst_notify->ul_ip_addr;
    notify_type  = (pst_notify->ul_notify_type & 0xFFFF);

    if (pst_net_dev->netdev_ops != &g_st_wal_net_dev_ops) {
        return NOTIFY_DONE;
    }

    pst_mac_vap = (mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(pst_mac_vap == NULL)) {
        OAM_WARNING_LOG1(0, OAM_SF_PWR,
            "{wal_netif_notify::Get mac vap failed, when %d(UP:1 DOWN:2 UNKNOWN:others) ipv4 address.}", notify_type);
        return NOTIFY_DONE;
    }

    wal_wake_lock();

    switch (notify_type) {
        case NETDEV_UP:
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_PWR, "{wal_netif_notify::Up IPv4[%d.X.X.%d].}",
                             ((oal_uint8 *)&(ul_ip_addr))[0],
                             ((oal_uint8 *)&(ul_ip_addr))[3]);
            hmac_arp_offload_set_ip_addr(pst_mac_vap, DMAC_CONFIG_IPV4, DMAC_IP_ADDR_ADD, &(ul_ip_addr));

            if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
                /* ��ȡ��IP��ַ��ʱ�����͹��� */
#if _PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE
                wlan_pm_set_timeout(WLAN_SLEEP_DEFAULT_CHECK_CNT);
#endif
            }
            break;

        case NETDEV_DOWN:
#if _PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE
            wlan_pm_set_timeout(WLAN_SLEEP_LONG_CHECK_CNT);
#endif
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_PWR, "{wal_netif_notify::Down IPv4[%d.X.X.%d].}",
                ((oal_uint8 *)&(ul_ip_addr))[0], ((oal_uint8 *)&(ul_ip_addr))[3]);

            hmac_arp_offload_set_ip_addr(pst_mac_vap, DMAC_CONFIG_IPV4, DMAC_IP_ADDR_DEL, &(ul_ip_addr));
            break;

        default:
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_PWR,
                "{wal_netif_notify::Unknown notifier event[%d].}", notify_type);
            break;
    }

    wal_wake_unlock();

    return NOTIFY_DONE;
}


oal_int32 wal_init_max_sta_num(oal_net_device_stru *pst_net_dev, oal_uint32 ul_max_num)
{
    if ((pst_net_dev == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(0, 0, "wal_init_max_sta_num: PARAM NULL, net_dev[0x%08x].", pst_net_dev);
        return -OAL_EFAIL;
    }

    OAM_WARNING_LOG1(0, 0, "wal_init_max_sta_num: ul_max_num[%d].", ul_max_num);
    g_st_ap_config_info.ul_ap_max_user = ul_max_num;

    return OAL_SUCC;
}
#endif


oal_uint32 wal_hipriv_register_inetaddr_notifier(oal_void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if (register_inetaddr_notifier(&wal_hipriv_notifier) == 0) {
        return OAL_SUCC;
    }

    OAM_ERROR_LOG0(0, OAM_SF_PWR, "{wal_hipriv_register_inetaddr_notifier::register inetaddr notifier failed.}");
    return OAL_FAIL;
#else
    return OAL_SUCC;
#endif
}


oal_uint32 wal_hipriv_unregister_inetaddr_notifier(oal_void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if (unregister_inetaddr_notifier(&wal_hipriv_notifier) == 0) {
        return OAL_SUCC;
    }

    OAM_ERROR_LOG0(0, OAM_SF_PWR,
        "{wal_hipriv_unregister_inetaddr_notifier::hmac_unregister inetaddr notifier failed.}");
    return OAL_FAIL;
#else
    return OAL_SUCC;
#endif
}


oal_uint32 wal_hipriv_register_inet6addr_notifier(oal_void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifdef CONFIG_IPV6
    if (register_inet6addr_notifier(&wal_hipriv_notifier_ipv6) == 0) {
        return OAL_SUCC;
    }

    OAM_ERROR_LOG0(0, OAM_SF_PWR, "{wal_hipriv_register_inet6addr_notifier::register inetaddr6 notifier failed.}");
    return OAL_FAIL;
#endif
#endif
    return OAL_SUCC;
}


oal_uint32 wal_hipriv_unregister_inet6addr_notifier(oal_void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifdef CONFIG_IPV6
    if (unregister_inet6addr_notifier(&wal_hipriv_notifier_ipv6) == 0) {
        return OAL_SUCC;
    }

    OAM_ERROR_LOG0(0, OAM_SF_PWR,
        "{wal_hipriv_unregister_inet6addr_notifier::hmac_unregister inetaddr6 notifier failed.}");
    return OAL_FAIL;
#endif
#endif
    return OAL_SUCC;
}


#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)

oal_int32 wal_hipriv_inetaddr_notifier_call(struct notifier_block *this, oal_ulong event, oal_void *ptr)
{
    /*
     * Notification mechanism from kernel to our driver. This function is called by the Linux kernel
     * whenever there is an event related to an IP address.
     * ptr : kernel provided pointer to IP address that has changed
     */
    struct in_ifaddr    *pst_ifa       = (struct in_ifaddr *)ptr;
    mac_vap_stru        *pst_mac_vap   = OAL_PTR_NULL;
#ifdef _PRE_WLAN_FEATURE_ROAM
    hmac_vap_stru       *pst_hmac_vap  = OAL_PTR_NULL;
#endif
    mac_cfg_ps_open_stru st_ps_open  = {0};

    if (OAL_UNLIKELY(pst_ifa == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "{wal_hipriv_inetaddr_notifier_call::pst_ifa is NULL.}");
        return NOTIFY_DONE;
    }
    if (OAL_UNLIKELY(pst_ifa->ifa_dev->dev == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "{wal_hipriv_inet6addr_notifier_call::pst_ifa->idev->dev is NULL.}");
        return NOTIFY_DONE;
    }

    /* Filter notifications meant for non Hislicon devices */
    if (pst_ifa->ifa_dev->dev->netdev_ops != &g_st_wal_net_dev_ops) {
        return NOTIFY_DONE;
    }

    pst_mac_vap = (mac_vap_stru *)OAL_NET_DEV_PRIV(pst_ifa->ifa_dev->dev);
    if (OAL_UNLIKELY(pst_mac_vap == NULL)) {
        OAM_WARNING_LOG1(0, OAM_SF_PWR,
            "{wal_hipriv_inetaddr_notifier_call::Get mac vap err, when %d(UP:1 DOWN:2 UNKNOWN:others) ipv4 address.}",
            event);
        return NOTIFY_DONE;
    }

    wal_wake_lock();

    if (event == NETDEV_UP) {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_PWR, "{wal_hipriv_inetaddr_notifier_call::Up IPv4[%d.X.X.%d].}",
            ((oal_uint8 *)&(pst_ifa->ifa_address))[0], ((oal_uint8 *)&(pst_ifa->ifa_address))[3]);
        hmac_arp_offload_set_ip_addr(pst_mac_vap, DMAC_CONFIG_IPV4, DMAC_IP_ADDR_ADD, &(pst_ifa->ifa_address));

        if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
            /* ��ȡ��IP��ַ��ʱ�����͹��� */
#if _PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE
            wlan_pm_set_timeout(WLAN_SLEEP_DEFAULT_CHECK_CNT);
#endif
        /* linuxû��Ӧ���µ͹����������Ҫ��ȡ��ipʱ�Լ��µ͹��Ŀ������� */
            st_ps_open.uc_pm_ctrl_type = MAC_STA_PM_CTRL_TYPE_HOST;
            st_ps_open.uc_pm_enable    = MAC_STA_PM_SWITCH_ON;
            hmac_config_set_sta_pm_on(pst_mac_vap, OAL_SIZEOF(mac_cfg_ps_open_stru), (oal_uint8 *)&st_ps_open);

            /* ��ȡ��IP��ַ��ʱ��֪ͨ���μ�ʱ */
#ifdef _PRE_WLAN_FEATURE_ROAM
            pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
            if (pst_hmac_vap == OAL_PTR_NULL) {
                OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                    "{wal_hipriv_inetaddr_notifier_call:: pst_hmac_vap null.uc_vap_id[%d]}", pst_mac_vap->uc_vap_id);

                wal_wake_unlock();
                return NOTIFY_DONE;
            }
            hmac_roam_wpas_connect_state_notify(pst_hmac_vap, WPAS_CONNECT_STATE_IPADDR_OBTAINED);
#endif
        }
    } else if (event == NETDEV_DOWN) {
#if _PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE
        wlan_pm_set_timeout(WLAN_SLEEP_LONG_CHECK_CNT);
#endif
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_PWR,
            "{wal_hipriv_inetaddr_notifier_call::Down IPv4[%d.X.X.%d].}",
            ((oal_uint8 *)&(pst_ifa->ifa_address))[0], ((oal_uint8 *)&(pst_ifa->ifa_address))[3]);

        hmac_arp_offload_set_ip_addr(pst_mac_vap, DMAC_CONFIG_IPV4, DMAC_IP_ADDR_DEL, &(pst_ifa->ifa_address));

        if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
            /* ��ȡ��IP��ַ��ʱ��֪ͨ���μ�ʱ */
#ifdef _PRE_WLAN_FEATURE_ROAM
            pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
            if (pst_hmac_vap == OAL_PTR_NULL) {
                OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                    "{wal_hipriv_inetaddr_notifier_call:: pst_hmac_vap null.uc_vap_id[%d]}", pst_mac_vap->uc_vap_id);

                wal_wake_unlock();
                return NOTIFY_DONE;
            }
            hmac_roam_wpas_connect_state_notify(pst_hmac_vap, WPAS_CONNECT_STATE_IPADDR_REMOVED);
#endif
        }
    } else {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_PWR,
            "{wal_hipriv_inetaddr_notifier_call::Unknown notifier event[%d].}", event);
    }

    wal_wake_unlock();
    return NOTIFY_DONE;
}


oal_int32 wal_hipriv_inet6addr_notifier_call(struct notifier_block *this, oal_ulong event, oal_void *ptr)
{
    /*
     * Notification mechanism from kernel to our driver. This function is called by the Linux kernel
     * whenever there is an event related to an IP address.
     * ptr : kernel provided pointer to IP address that has changed
     */
    struct inet6_ifaddr    *pst_ifa       = (struct inet6_ifaddr *)ptr;
    mac_vap_stru           *pst_mac_vap = OAL_PTR_NULL;

    if (OAL_UNLIKELY(pst_ifa == NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "{wal_hipriv_inet6addr_notifier_call::pst_ifa is NULL.}");
        return NOTIFY_DONE;
    }
    if (OAL_UNLIKELY(pst_ifa->idev->dev == NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "{wal_hipriv_inet6addr_notifier_call::pst_ifa->idev->dev is NULL.}");
        return NOTIFY_DONE;
     }


    /* Filter notifications meant for non Hislicon devices */
    if (pst_ifa->idev->dev->netdev_ops != &g_st_wal_net_dev_ops) {
        return NOTIFY_DONE;
    }

    pst_mac_vap = (mac_vap_stru *)OAL_NET_DEV_PRIV(pst_ifa->idev->dev);
    if (OAL_UNLIKELY(pst_mac_vap == NULL)) {
        OAM_WARNING_LOG1(0, OAM_SF_PWR,
            "{wal_hipriv_inet6addr_notifier_call::Get mac vap err, when %d(UP:1 DOWN:2 UNKNOWN:others) ipv6 address.}",
            event);
        return NOTIFY_DONE;
    }

    switch (event) {
        case NETDEV_UP: {
            OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_PWR,
                "{wal_hipriv_inet6addr_notifier_call::UP IPv6[%04x:%04x:XXXX:XXXX:XXXX:XXXX:%04x:%04x].}",
                OAL_NET2HOST_SHORT((pst_ifa->addr.s6_addr16)[0]),
                OAL_NET2HOST_SHORT((pst_ifa->addr.s6_addr16)[1]),
                OAL_NET2HOST_SHORT((pst_ifa->addr.s6_addr16)[6]),
                OAL_NET2HOST_SHORT((pst_ifa->addr.s6_addr16)[7]));
            hmac_arp_offload_set_ip_addr(pst_mac_vap, DMAC_CONFIG_IPV6, DMAC_IP_ADDR_ADD, &(pst_ifa->addr));
            break;
        }

        case NETDEV_DOWN: {
            OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_PWR,
                "{wal_hipriv_inet6addr_notifier_call::DOWN IPv6[%04x:%04x:XXXX:XXXX:XXXX:XXXX:%04x:%04x].}",
                OAL_NET2HOST_SHORT((pst_ifa->addr.s6_addr16)[0]),
                OAL_NET2HOST_SHORT((pst_ifa->addr.s6_addr16)[1]),
                OAL_NET2HOST_SHORT((pst_ifa->addr.s6_addr16)[6]),
                OAL_NET2HOST_SHORT((pst_ifa->addr.s6_addr16)[7]));
            hmac_arp_offload_set_ip_addr(pst_mac_vap, DMAC_CONFIG_IPV6, DMAC_IP_ADDR_DEL, &(pst_ifa->addr));
            break;
        }

        default: {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_PWR,
                "{wal_hipriv_inet6addr_notifier_call::Unknown notifier event[%d].}", event);
            break;
        }
    }

    return NOTIFY_DONE;
}
#endif /* #if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) */
#endif /* #ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD */

#ifdef _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN

OAL_STATIC oal_uint32  wal_hipriv_set_tx_classify_switch(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    wal_msg_write_stru  st_write_msg;
    oal_uint8        uc_flag ;
    oal_uint8       *puc_value = 0;
    oal_uint32       ul_ret;
    oal_uint32       ul_off_set = 0;
    oal_int32        l_ret;
    oal_int8         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};

    // sh hipriv.sh "p2p-p2p0-0 set_tx_classify_switch 1/0"
    /* ��ȡ���ò��� */
    ul_ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
         OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_hipriv_set_tx_classify_switch::wal_get_cmd_one_arg err[%d]!}", ul_ret);
         return ul_ret;
    }

    uc_flag = (oal_uint8)oal_atoi(ac_name);
    /* �Ƿ����ò��� */
    if (uc_flag > 1) {
        OAM_WARNING_LOG0(0, OAM_SF_EDCA, "wal_hipriv_set_tx_classify_switch::invalid config, should be 0 or 1");
        return OAL_SUCC;
    }

    /* �����¼��ڴ� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TX_CLASSIFY_LAN_TO_WLAN_SWITCH, OAL_SIZEOF(oal_uint8));
    puc_value = (oal_uint8 *)(st_write_msg.auc_value);
    *puc_value = uc_flag;

    /***************************************************************************
                             ���¼���wal�㴦��
    ***************************************************************************/
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_hipriv_set_tx_classify_switch:: return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif  /* _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN */

OAL_STATIC oal_uint32  wal_hipriv_mcs_set_check_enable(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param)
{
    oal_uint32                           ul_off_set;
    oal_int8                             ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                           ul_ret;
    oal_int32                            l_ret;
    wal_msg_write_stru                   st_write_msg;
    oal_bool_enum_uint8                 *pen_mcs_set_check_enable = OAL_PTR_NULL;

    ul_ret = wal_get_cmd_one_arg(pc_param, ac_arg, WAL_HIPRIV_CMD_NAME_MAX_LEN, &ul_off_set);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "wal_hipriv_ht_check_stop: get first arg fail.");
        return OAL_FAIL;
    }

    /***************************************************************************
                            ���¼���wal�㴦��
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFIGD_MCS_SET_CHECK_ENABLE, OAL_SIZEOF(oal_bool_enum_uint8));

    /* ��������������� */
    pen_mcs_set_check_enable  = (oal_bool_enum_uint8 *)(st_write_msg.auc_value);
    *pen_mcs_set_check_enable = (oal_bool_enum_uint8)oal_atoi(ac_arg);

    OAM_WARNING_LOG1(0, OAM_SF_SCAN, "wal_hipriv_mcs_set_check_enable_flag= %d.", *pen_mcs_set_check_enable);

    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_bool_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (l_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_hipriv_mcs_set_check_enable::wal_send_cfg_event err[%d]!", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC uint32_t wal_hipriv_cali_permission_switch(oal_net_device_stru *net_dev, const int8_t *param)
{
    uint32_t offset = 0;
    int8_t   arg[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    uint32_t ret;
    int8_t   permission;

    OAL_IO_PRINT("{wal_hipriv_cali_permission_switch.start}");
    ret = wal_get_cmd_one_arg(param, arg, WAL_HIPRIV_CMD_NAME_MAX_LEN, &offset);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "wal_hipriv_cali_permission_switch: get first arg fail.");
        return OAL_FAIL;
    }

    permission = (int8_t)oal_atoi(arg);
    if (permission > 1) {
        OAM_WARNING_LOG0(0, OAM_SF_EDCA, "wal_hipriv_cali_permission_switch::invalid config, should be 0 or 1");
        return OAL_SUCC;
    }

    api_set_wifi_cali_apply(permission);
    return OAL_SUCC;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifndef CONFIG_HAS_EARLYSUSPEND

OAL_STATIC oal_int32 wal_ioctl_set_suspend_mode(oal_net_device_stru *pst_net_dev, oal_uint8 uc_suspend)
{
    wal_msg_write_stru st_write_msg;
    oal_int32 l_ret;

    if (OAL_UNLIKELY((pst_net_dev == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_ioctl_set_suspend_mode::pst_net_dev null ptr error!}");
        return -OAL_EFAUL;
    }

    st_write_msg.auc_value[0] = uc_suspend;

    /***************************************************************************
        ���¼���wal�㴦��
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_SUSPEND_MODE, OAL_SIZEOF(uc_suspend));

    /* ������Ϣ */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(uc_suspend),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    return l_ret;
}
#endif
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
/* ����supplicant�·���������SSID */
OAL_STATIC int32_t wal_android_priv_set_ssid(oal_net_device_stru *net_dev, const uint8_t *command)
{
    mac_cfg_ssid_param_stru ssid_param = {0};
    mac_vap_stru            *mac_vap   = OAL_PTR_NULL;
    uint32_t                ret;
    const uint32_t          skip       = CMD_SET_SSID_LEN + 1;

    mac_vap = OAL_NET_DEV_PRIV(net_dev);
    if (mac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_android_priv_set_ssid::pst_mac_vap is null.}");
        return -OAL_EINVAL;
    }
    ssid_param.uc_ssid_len = (uint8_t)OAL_STRLEN((int8_t *)command + skip);
    if (ssid_param.uc_ssid_len > OAL_SIZEOF(ssid_param.ac_ssid)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_android_priv_set_ssid::CMD_SET_SSID ssid is too long [%d].}",
            ssid_param.uc_ssid_len);
        return -OAL_EINVAL;
    }
    if (memcpy_s(ssid_param.ac_ssid, WLAN_SSID_MAX_LEN, command + skip, ssid_param.uc_ssid_len) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_android_priv_set_ssid::memcpy_s failed!");
        return -OAL_EFAIL;
    }
    ret = hmac_config_set_ssid(mac_vap, OAL_SIZEOF(ssid_param), (uint8_t *)&ssid_param);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_android_priv_set_ssid::hmac_config_set_ssid return err code [%d]!}\r\n", ret);
    }
    return ret;
}
#endif

/* wal����netbuff������Ϣ���䣬ʧ�ܻ��ͷ�netbuff */
OAL_STATIC int32_t wal_send_netbuf_msg_adapt(oal_net_device_stru *net_dev, oal_netbuf_stru *netbuf,
    uint16_t len, uint8_t vap_id, wlan_cfgid_enum_uint16 cfgid)
{
    wal_msg_write_stru write_msg = {0};
    int32_t            ret;

    /* ��д msg ��Ϣͷ */
    WAL_WRITE_MSG_HDR_INIT(&write_msg, cfgid, len);
    /* �������netbuf�׵�ַ��д��msg��Ϣ���� */
    if (memcpy_s(write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, (uint8_t *)&netbuf, len) != EOK) {
        OAM_ERROR_LOG0(vap_id, 0, "{wal_send_netbuf_msg_adapt:: memcpy_s fail!}");
        oal_netbuf_free(netbuf);
        return OAL_FAIL;
    }
    /* ������Ϣ */
    ret = wal_send_cfg_event(net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + len,
                             (uint8_t *)&write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_ERROR_LOG2(vap_id, OAM_SF_ANY,
            "{wal_send_netbuf_msg_adapt::wal_send_cfg_event failed, cfgid[%d],error no[%d]!}", cfgid, ret);
        oal_netbuf_free(netbuf);
    }
    return ret;
}

#ifdef _PRE_WLAN_FEATURE_IP_FILTER

int32_t wal_send_assigned_filter_event(oal_net_device_stru *net_dev, wlan_cfgid_enum_uint16 cfgid,
    uint8_t *param, uint16_t len)
{
    int32_t ret;
    wal_msg_write_stru st_write_msg = {0};
    sub_type_load_stru *assigned_filter = NULL;
    uint16_t ptk_len;

    /* ��д���¼���ͷ */
    assigned_filter = (sub_type_load_stru *)&(st_write_msg.auc_value);
    assigned_filter->en_sub_type = MAC_SUB_ASSIGNED_FILTER;
    assigned_filter->us_buf_len = len;
    /* ��дICMP���������Ϣ */
    ret = memcpy_s(assigned_filter->auc_buf, WLAN_SUB_EVENT_MAX_LEN, param, len);
    if (ret != EOK) {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "wal_config_send_event::memcpy fail! cfgid [%d]", cfgid);
        return OAL_FAIL;
    }

    /* ����ȥ�ĳ���Ϊʹ�ó��ȼ��Ϸ�װ�İ�ͷ�� sizeof(sub_type_load_stru) - WLAN_SUB_EVENT_MAX_LEN */
    ptk_len = len + offsetof(sub_type_load_stru, auc_buf); //lint !e78 !e734 !e516
    /* ��д msg ��Ϣͷ */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, cfgid, ptk_len);

    /* ������Ϣ */
    ret = wal_send_cfg_event(net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + ptk_len,
                             (uint8_t *)&st_write_msg,
                             OAL_FALSE,
                             NULL);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_ERROR_LOG2(0, OAM_SF_CFG,
            "{wal_config_send_event:: wal_send_cfg_event failed, error no[%d],cfg_id [%d]!}", ret, cfgid);
        return ret;
    }

    return OAL_SUCC;
}

#ifdef CONFIG_DOZE_FILTER
/* ����supplicant�·�����򿪻�ر�IP_FILTER���� */
OAL_STATIC int32_t wal_android_priv_set_ip_filter(uint8_t *command, wal_android_wifi_priv_cmd_stru *priv_cmd)
{
    int32_t on;

    /* ��ʽ:FILTER 1 or FILTER 0(����+�ո�+����) */
    if (priv_cmd->l_total_len < (CMD_FILTER_SWITCH_LEN + 1 + 1)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY,
            "{wal_android_priv_set_ip_filter:: CMD_FILTER_SWITCH length is too short! at least need [%d]!}",
            (CMD_FILTER_SWITCH_LEN + 2));
        return -OAL_EFAIL;
    }
    on = oal_atoi(command + CMD_FILTER_SWITCH_LEN + 1);
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_android_priv_set_ip_filter::CMD_FILTER_SWITCH %d.}", on);
    /* �����ں˽ӿڵ��� gWlanFilterOps.set_filter_enable */
    return hw_set_net_filter_enable(on);
}
#endif

/* ��������: IP_FILTER���ܵ��Խӿ� */
uint32_t wal_hipriv_set_ip_filter(oal_net_device_stru *net_dev, const int8_t *param)
{
    int32_t                 items_cnt;
    int32_t                 items_idx;
    int32_t                 enable;
    uint32_t                ret;
    uint32_t                offset = 0;
    int8_t                  cmd[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    int8_t                  cmd_param[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    wal_hw_wifi_filter_item items[MAX_HIPRIV_IP_FILTER_BTABLE_SIZE] = {{0}};

    OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_ip_filter::cmd enter!}");
    /* ȡ�������� */
    ret = wal_get_cmd_one_arg(param, cmd, OAL_SIZEOF(cmd), &offset);
    if (ret != OAL_SUCC) {
        return ret;
    }
    /* ����� */
    if (oal_strncmp(cmd, CMD_CLEAR_RX_FILTERS, OAL_STRLEN(CMD_CLEAR_RX_FILTERS)) == 0) {
        return (uint32_t)wal_clear_ip_filter();
    }
    param += offset;
    ret = wal_get_cmd_one_arg(param, cmd_param, OAL_SIZEOF(cmd_param), &offset);
    if (ret != OAL_SUCC) {
        return ret;
    }
    if (oal_strncmp(cmd, CMD_SET_RX_FILTER_ENABLE, OAL_STRLEN(CMD_SET_RX_FILTER_ENABLE)) == 0) {
        /* ʹ��/�رչ��� */
        enable = oal_atoi(cmd_param);
        ret = (uint32_t)wal_set_ip_filter_enable(enable);
    } else if (oal_strncmp(cmd, CMD_ADD_RX_FILTER_ITEMS, OAL_STRLEN(CMD_ADD_RX_FILTER_ITEMS)) == 0) {
        /* ���º�����,��ȡ������Ŀ�� */
        items_cnt = oal_atoi(cmd_param);
        items_cnt = OAL_MIN(MAX_HIPRIV_IP_FILTER_BTABLE_SIZE, items_cnt);
        /* ��ȡ������Ŀ */
        for (items_idx = 0; items_idx < items_cnt; items_idx++) {
            /* ��ȡprotocolX */
            param += offset;
            ret = wal_get_cmd_one_arg(param, cmd_param, OAL_SIZEOF(cmd_param), &offset);
            if (ret != OAL_SUCC) {
                return ret;
            }
            items[items_idx].protocol = (uint8_t)oal_atoi(cmd_param);
            /* ��ȡportX */
            param += offset;
            ret = wal_get_cmd_one_arg(param, cmd_param, OAL_SIZEOF(cmd_param), &offset);
            if (ret != OAL_SUCC) {
                return ret;
            }
            items[items_idx].port = (uint16_t)oal_atoi(cmd_param);
        }
        ret = (uint32_t)wal_add_ip_filter_items(items, items_cnt);
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_ip_filter::cmd_one_arg no support!}");
        ret = OAL_FAIL;
    }
    return ret;
}


OAL_STATIC uint32_t wal_hipriv_set_assigned_filter_switch(oal_net_device_stru *net_dev, const int8_t *param)
{
    uint32_t ret;
    uint16_t len;
    uint32_t offset = 0;
    int8_t name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    mac_assigned_filter_cmd_stru assigned_filter_cmd;

    /* ׼���������� */
    len = OAL_SIZEOF(mac_assigned_filter_cmd_stru);
    memset_s((uint8_t *)&assigned_filter_cmd, len, 0, len);

    /* ��ȡfilter id */
    ret = wal_get_cmd_one_arg(param, name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &offset);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_assigned_filter_switch::1th parm err_code [%d]!}\r\n", ret);
        return ret;
    }

    if (oal_atoi(name) < 0 || oal_atoi(name) > WLAN_RX_FILTER_CNT) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_assigned_filter_switch::icmp filter id not match.}");
        return OAL_FAIL;
    }

    assigned_filter_cmd.filter_id = (uint8_t)oal_atoi(name);

    /* ƫ�ƣ�ȡ��һ������ */
    param = param + offset;

    /* ��ȡ���� */
    ret = wal_get_cmd_one_arg(param, name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &offset);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_assigned_filter_switch::2th para err_code [%d]!}\r\n", ret);
        return ret;
    }

    assigned_filter_cmd.enable = ((uint8_t)oal_atoi(name) > 0) ? OAL_TRUE : OAL_FALSE;

    OAM_WARNING_LOG2(0, OAM_SF_ANY, "{wal_hipriv_set_assigned_filter_switch::filter id [%d] on/off[%d].}",
                     assigned_filter_cmd.filter_id, assigned_filter_cmd.enable);

    // ���¼���wal�㴦��
    if (wal_send_assigned_filter_event(net_dev, WLAN_CFGID_ASSIGNED_FILTER,
        (uint8_t *)&assigned_filter_cmd, len) != OAL_SUCC) {
        return OAL_FAIL;
    }

    return OAL_SUCC;
}

/* ��������: ��ȡIP������ع�����Ϣ */
int32_t wal_get_ip_filter_info(mac_vap_stru **mac_vap_ptr, oal_net_device_stru **net_dev_ptr)
{
    mac_vap_stru        *mac_vap = OAL_PTR_NULL;
    oal_net_device_stru *net_dev = OAL_PTR_NULL;
#ifdef _PRE_WLAN_FEATURE_DFR
    if (g_st_dfr_info.bit_device_reset_process_flag) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_get_ip_filter_info:: dfr_process_status[%d]!}",
            g_st_dfr_info.bit_device_reset_process_flag);
        return -OAL_EFAIL;
    }
#endif // _PRE_WLAN_FEATURE_DFR
    net_dev = oal_dev_get_by_name("wlan0");
    if (net_dev == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_get_ip_filter_info::wlan0 not exist!}");
        return -OAL_EINVAL;
    }
    /* ����oal_dev_get_by_name�󣬱������oal_dev_putʹnet_dev�����ü�����һ */
    oal_dev_put(net_dev);
    /* vapδ����ʱ���������·������� */
    mac_vap = OAL_NET_DEV_PRIV(net_dev);
    if (mac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_get_ip_filter_info::vap not created yet, ignore the cmd!}");
        return -OAL_EINVAL;
    }
    if (mac_vap->st_cap_flag.bit_ip_filter != OAL_TRUE) {
        OAM_WARNING_LOG0(mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_get_ip_filter_info::Func not enable, ignore the cmd!}");
        return -OAL_EINVAL;
    }
    *mac_vap_ptr = mac_vap;
    *net_dev_ptr = net_dev;
    return OAL_SUCC;
}

/* ����ip���˵�ʹ��״̬ */
int32_t wal_set_ip_filter_enable(int32_t on)
{
    uint32_t               netbuf_len;
    mac_vap_stru           *mac_vap = OAL_PTR_NULL;
    oal_net_device_stru    *net_dev = OAL_PTR_NULL;
    oal_netbuf_stru        *netbuf = OAL_PTR_NULL;
    mac_ip_filter_cmd_stru ip_filter_cmd = {0};
    mac_ip_filter_cmd_stru *cmd_info = OAL_PTR_NULL;

    if (on < 0) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_set_ip_filter_enable::Invalid input parameter, on/off %d!}", on);
        return -OAL_EINVAL;
    }
    /* ������ϢԤ���� */
    if ((wal_get_ip_filter_info(&mac_vap, &net_dev) != OAL_SUCC) || OAL_ANY_NULL_PTR2(net_dev, mac_vap)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_set_ip_filter_enable::wal_get_ip_filter_info fail!}");
        return -OAL_EINVAL;
    }
    /* ׼������������һ���ֽڱ������������� */
    netbuf_len = 1 + OAL_SIZEOF(mac_ip_filter_cmd_stru);
    ip_filter_cmd.cmd    = MAC_IP_FILTER_ENABLE;
    ip_filter_cmd.enable = (on > 0) ? OAL_TRUE : OAL_FALSE;
    OAM_WARNING_LOG1(mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_set_ip_filter_enable::IP_filter on/off(%d).}",
                     ip_filter_cmd.enable);
    /* ����ռ� ������˹��� */
    netbuf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, netbuf_len, OAL_NETBUF_PRIORITY_MID);
    if (netbuf == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_set_ip_filter_enable::netbuf alloc null,size %d.}", netbuf_len);
        return -OAL_EINVAL;
    }
    memset_s(((uint8_t *)OAL_NETBUF_DATA(netbuf)), netbuf_len, 0, netbuf_len);
    /* ������Ϣ������ */
    *((uint8_t *)OAL_NETBUF_DATA(netbuf)) = MAC_IP_FILTER_CMD;
    cmd_info = (mac_ip_filter_cmd_stru *)((uint8_t *)OAL_NETBUF_DATA(netbuf) + 1);
    /* ��¼���˹��� */
    if (memcpy_s((uint8_t *)cmd_info, netbuf_len - 1, (uint8_t *)(&ip_filter_cmd),
        OAL_SIZEOF(mac_ip_filter_cmd_stru)) != EOK) {
        OAM_ERROR_LOG0(mac_vap->uc_vap_id, 0, "{wal_set_ip_filter_enable:: memcpy_s fail!}");
        oal_netbuf_free(netbuf);
        return OAL_FAIL;
    }
    oal_netbuf_put(netbuf, netbuf_len);
    /* wal�����¼� */
    return wal_send_netbuf_msg_adapt(net_dev, netbuf, OAL_SIZEOF(oal_netbuf_stru*),
                                     mac_vap->uc_vap_id, WLAN_CFGID_IP_FILTER);
}


int32_t wal_set_assigned_filter_enable(int32_t filter_id, int32_t filter_switch)
{
    uint16_t len;
    oal_net_device_stru *net_dev = NULL;
    mac_vap_stru *mac_vap = NULL;
    mac_assigned_filter_cmd_stru assigned_filter_cmd;

#ifdef _PRE_WLAN_FEATURE_DFR
    if (g_st_dfr_info.bit_device_reset_process_flag) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG,
            "{wal_set_assigned_filter_enable:: dfr_process_status[%d]!}", g_st_dfr_info.bit_device_reset_process_flag);
        return -OAL_EFAIL;
    }
#endif

    if (filter_id < 0 || filter_switch < 0) {
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{wal_set_assigned_filter_enable::params error! filter_id [%d], switch [%d]}",
                         filter_id, filter_switch);
        return -OAL_EINVAL;
    }

    net_dev = oal_dev_get_by_name("wlan0");
    if (net_dev == NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{wal_set_assigned_filter_enable::wlan0 not exist!}");
        return -OAL_EINVAL;
    }
    /* ����oal_dev_get_by_name��,�������oal_dev_putʹnet_dev�����ü���-1 */
    oal_dev_put(net_dev);

    /* vapδ����ʱ,�������·������� */
    mac_vap = OAL_NET_DEV_PRIV(net_dev);
    if (mac_vap == NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{wal_set_assigned_filter_enable::vap not created yet, ignore the cmd!}");
        return -OAL_EINVAL;
    }

    if (mac_vap->st_cap_flag.bit_icmp_filter != OAL_TRUE) {
        OAM_WARNING_LOG2(mac_vap->uc_vap_id, OAM_SF_CFG,
            "{wal_set_assigned_filter_enable::vap not support, ignore the cmd! vap mode [%d], p2p mode [%d].}",
            mac_vap->en_vap_mode, mac_vap->en_p2p_mode);
        return -OAL_EINVAL;
    }

    /* ׼���������� */
    len = OAL_SIZEOF(mac_assigned_filter_cmd_stru);
    memset_s((uint8_t *)&assigned_filter_cmd, len, 0, len);
    assigned_filter_cmd.filter_id = (mac_assigned_filter_id_enum)filter_id;
    assigned_filter_cmd.enable = (filter_switch > 0) ? OAL_TRUE : OAL_FALSE;

    OAM_WARNING_LOG2(mac_vap->uc_vap_id, OAM_SF_CFG,
        "{wal_set_assigned_filter_enable::assigned_filter filter_id [%d], switch [%d].}",
        filter_id, assigned_filter_cmd.enable);

    // ���¼���wal�㴦��
    if (wal_send_assigned_filter_event(net_dev, WLAN_CFGID_ASSIGNED_FILTER,
        (uint8_t *)&assigned_filter_cmd, len) != OAL_SUCC) {
        return -OAL_EINVAL;
    }

    return OAL_SUCC;
}

/* ���ip���˵ĺ����� */
int32_t wal_add_ip_filter_items(wal_hw_wifi_filter_item *items, int32_t count)
{
    uint8_t                filter_list_num;
    uint32_t               netbuf_len;
    uint32_t               items_idx;
    oal_netbuf_stru        *netbuf = OAL_PTR_NULL;
    mac_vap_stru           *mac_vap = OAL_PTR_NULL;
    oal_net_device_stru    *net_dev = OAL_PTR_NULL;
    mac_ip_filter_cmd_stru ip_filter_cmd = {0};
    mac_ip_filter_cmd_stru *cmd_info = OAL_PTR_NULL;

    if ((items == OAL_PTR_NULL) || (count <= 0)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_add_ip_filter_items::Invalid input parameter, count %d!}", count);
        return -OAL_EINVAL;
    }
    /* ������ϢԤ���� */
    if ((wal_get_ip_filter_info(&mac_vap, &net_dev) != OAL_SUCC) || OAL_ANY_NULL_PTR2(net_dev, mac_vap)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_set_ip_filter_enable::wal_get_ip_filter_info fail!}");
        return -OAL_EINVAL;
    }
    /* ���ڱ���������С���ƣ�ȡ�����ɵĹ�����Ŀ����Сֵ */
    filter_list_num = (uint8_t)OAL_MIN((MAC_MAX_IP_FILTER_BTABLE_SIZE / OAL_SIZEOF(mac_ip_filter_item_stru)),
        (uint32_t)count);
    if (filter_list_num < count) {
        OAM_WARNING_LOG2(mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_add_ip_filter_items::table(%d) is too small to store %d items!}", filter_list_num, count);
    }
    OAM_WARNING_LOG1(mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_add_ip_filter_items::Start updating btable, items_cnt(%d).}",
                     filter_list_num);
    /* ѡ�������¼��ռ�Ĵ�С:1���ֽڱ������¼����� */
    netbuf_len = 1 + (filter_list_num * OAL_SIZEOF(mac_ip_filter_item_stru)) + OAL_SIZEOF(mac_ip_filter_cmd_stru);
    /* ׼���������� */
    ip_filter_cmd.cmd        = MAC_IP_FILTER_UPDATE_BTABLE;
    ip_filter_cmd.item_count = filter_list_num;
    /* ����ռ� ������˹��� */
    netbuf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, netbuf_len, OAL_NETBUF_PRIORITY_MID);
    if (netbuf == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_add_ip_filter_items::alloc fail,size %d.}", netbuf_len);
        return -OAL_EINVAL;
    }
    memset_s(((uint8_t *)OAL_NETBUF_DATA(netbuf)), netbuf_len, 0, netbuf_len);
    /* �������������� */
    *((uint8_t *)OAL_NETBUF_DATA(netbuf)) = MAC_IP_FILTER_CMD;
    cmd_info = (mac_ip_filter_cmd_stru *)((uint8_t *)OAL_NETBUF_DATA(netbuf) + 1);
    /* ��¼���˹��� */
    if (memcpy_s((uint8_t *)cmd_info, netbuf_len - 1, (uint8_t *)(&ip_filter_cmd),
        OAL_SIZEOF(mac_ip_filter_cmd_stru)) != EOK) {
        OAM_ERROR_LOG0(mac_vap->uc_vap_id, 0, "{wal_add_ip_filter_items:: memcpy_s fail!}");
        oal_netbuf_free(netbuf);
        return OAL_FAIL;
    }
    oal_netbuf_put(netbuf, netbuf_len);
    for (items_idx = 0; items_idx < ip_filter_cmd.item_count; items_idx++) {
        cmd_info->filter_items_items[items_idx].protocol = (uint8_t)items[items_idx].protocol;
        cmd_info->filter_items_items[items_idx].port     = (uint16_t)items[items_idx].port;
    }
    /* wal�����¼� */
    return wal_send_netbuf_msg_adapt(net_dev, netbuf, OAL_SIZEOF(oal_netbuf_stru*),
                                     mac_vap->uc_vap_id, WLAN_CFGID_IP_FILTER);
}

/* ���ip���˵ĺ����� */
int32_t wal_clear_ip_filter()
{
    uint32_t               netbuf_len;
    oal_netbuf_stru        *netbuf = OAL_PTR_NULL;
    mac_vap_stru           *mac_vap = OAL_PTR_NULL;
    oal_net_device_stru    *net_dev = OAL_PTR_NULL;
    mac_ip_filter_cmd_stru ip_filter_cmd = {0};
    mac_ip_filter_cmd_stru *cmd_info = OAL_PTR_NULL;

    /* ������ϢԤ���� */
    if ((wal_get_ip_filter_info(&mac_vap, &net_dev) != OAL_SUCC) || OAL_ANY_NULL_PTR2(net_dev, mac_vap)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_clear_ip_filter::wal_get_ip_filter_info fail!}");
        return -OAL_EINVAL;
    }
    /* ѡ�������¼��ռ�Ĵ�С,һ���ֽڱ������������� */
    netbuf_len = 1 + OAL_SIZEOF(mac_ip_filter_cmd_stru);
    OAM_WARNING_LOG0(mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_clear_ip_filter::Now start clearing the list.}");
    /* ��������� */
    ip_filter_cmd.cmd = MAC_IP_FILTER_CLEAR;
    /* ����ռ� ������˹��� */
    netbuf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, netbuf_len, OAL_NETBUF_PRIORITY_MID);
    if (netbuf == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_clear_ip_filter::netbuf alloc null,size %d.}", netbuf_len);
        return -OAL_EINVAL;
    }
    memset_s(((uint8_t *)OAL_NETBUF_DATA(netbuf)), netbuf_len, 0, netbuf_len);
    *((uint8_t *)OAL_NETBUF_DATA(netbuf)) = MAC_IP_FILTER_CMD;
    cmd_info = (mac_ip_filter_cmd_stru *)((uint8_t *)OAL_NETBUF_DATA(netbuf) + 1);
    /* ��¼���˹��� */
    if (memcpy_s((uint8_t *)cmd_info, netbuf_len - 1, (uint8_t *)(&ip_filter_cmd),
        OAL_SIZEOF(mac_ip_filter_cmd_stru)) != EOK) {
        OAM_ERROR_LOG0(mac_vap->uc_vap_id, 0, "{wal_clear_ip_filter:: memcpy_s fail!}");
        oal_netbuf_free(netbuf);
        return OAL_FAIL;
    }
    oal_netbuf_put(netbuf, netbuf_len);
    /* wal�����¼� */
    return wal_send_netbuf_msg_adapt(net_dev, netbuf, OAL_SIZEOF(oal_netbuf_stru*),
                                     mac_vap->uc_vap_id, WLAN_CFGID_IP_FILTER);
}

int32_t wal_register_ip_filter(wal_hw_wlan_filter_ops *ip_filter_ops)
{
#ifdef CONFIG_DOZE_FILTER
    if (ip_filter_ops == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_register_ip_filter::pg_st_ip_filter_ops is null !}");
        return -OAL_EINVAL;
    }
    hw_register_wlan_filter(ip_filter_ops);
#else
    OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_register_ip_filter:: Not support CONFIG_DOZE_FILTER!}");
#endif
    return OAL_SUCC;
}

int32_t wal_unregister_ip_filter()
{
#ifdef CONFIG_DOZE_FILTER
    hw_unregister_wlan_filter();
#else
    OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_unregister_ip_filter:: Not support CONFIG_DOZE_FILTER!}");
#endif
    return OAL_SUCC;
}

#else
int32_t wal_set_ip_filter_enable(int32_t on)
{
    OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_set_ip_filter_enable::Ip_filter not support!}");
    return -OAL_EFAIL;
}

int32_t wal_set_assigned_filter_enable(int32_t filter_id, int32_t filter_switch)
{
    OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_set_assigned_filter_enable::Assigned_filter not support!}");
    return -OAL_EFAIL;
}

int32_t wal_add_ip_filter_items(wal_hw_wifi_filter_item *items, int32_t count)
{
    OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_add_ip_filter_items::Ip_filter not support!}");
    return -OAL_EFAIL;
}

int32_t wal_clear_ip_filter()
{
    OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_clear_ip_filter::Ip_filter not support!}");
    return -OAL_EFAIL;
}
#endif // _PRE_WLAN_FEATURE_IP_FILTER

OAL_STATIC int32_t wal_set_auth_rsp_time(oal_net_device_stru *net_dev, int32_t value)
{
    wal_msg_write_stru write_msg = {0};
    int32_t            ret;

    if (value < 0) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_set_auth_rsp_time::Invalid input parameter, value/off %d!}", value);
        return -OAL_EINVAL;
    }

    /* �����¼��ڴ� */
    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_SET_AUTH_RSP_TIME, OAL_SIZEOF(uint16_t));
    *((uint16_t *)(write_msg.auc_value)) = (uint16_t)value;

    ret = wal_send_cfg_event(net_dev, WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(uint16_t),
                             (uint8_t *)&write_msg, OAL_FALSE, NULL);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_set_auth_rsp_time:: return err_code [%d]!}", ret);
        return (oal_uint32)ret;
    }
    return OAL_SUCC;
}

OAL_STATIC int32_t wal_enable_forbit_open_auth(oal_net_device_stru *net_dev, int32_t value)
{
    wal_msg_write_stru write_msg = {0};
    int32_t            ret;

    if (value < 0) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_enable_forbit_open_auth::Invalid input parameter, value/off %d!}", value);
        return -OAL_EINVAL;
    }

    /* �����¼��ڴ� */
    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_FORBIT_OPEN_AUTH, OAL_SIZEOF(uint16_t));
	write_msg.auc_value[0] = ((value > 0) ? 1 : 0);

    ret = wal_send_cfg_event(net_dev, WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(uint16_t),
                             (uint8_t *)&write_msg, OAL_FALSE, NULL);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_enable_forbit_open_auth:: return err_code [%d]!}", ret);
        return (oal_uint32)ret;
    }
    return OAL_SUCC;
}

uint32_t wal_hipriv_set_str_cmd(oal_net_device_stru *net_dev, const int8_t *param)
{
    int32_t                 value;
    uint32_t                ret;
    uint32_t                offset = 0;
    int8_t                  cmd[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    int8_t                  cmd_param[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    cmd_str_config_enum     cmd_str_type = CMD_STR_BUTT;

    OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_str_cmd!}");
    /* ȡ�������� */
    ret = wal_get_cmd_one_arg(param, cmd, OAL_SIZEOF(cmd), &offset);
    if (ret != OAL_SUCC) {
        return ret;
    }
    /* sh hipriv.sh ""wlan0 set_str set_auth_rsp_time 1500/0" */
    if (oal_strncmp(cmd, CMD_SET_AUTH_RSP_TIME, OAL_STRLEN(CMD_SET_AUTH_RSP_TIME)) == 0) {
        cmd_str_type = CMD_STR_AUTH_RSP_TIME;
    }
    /* sh hipriv.sh ""wlan0 set_str forbit_open_auth 1/0" */
    if (oal_strncmp(cmd, CMD_SET_FORBIT_OPEN_AUTH, OAL_STRLEN(CMD_SET_FORBIT_OPEN_AUTH)) == 0) {
        cmd_str_type = CMD_STR_FORBIT_OPEN_AUTH;
    }

    param += offset;
    ret = wal_get_cmd_one_arg(param, cmd_param, OAL_SIZEOF(cmd_param), &offset);
    if (ret != OAL_SUCC) {
        return ret;
    }
    value = oal_atoi(cmd_param);

    switch (cmd_str_type) {
        case CMD_STR_AUTH_RSP_TIME:
            ret = (uint32_t)wal_set_auth_rsp_time(net_dev, value);
            break;
        case CMD_STR_FORBIT_OPEN_AUTH:
            ret = (uint32_t)wal_enable_forbit_open_auth(net_dev, value);
            break;
        default:
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_str_cmd unnone!}");
            ret = OAL_FAIL;
            break;
    }
    return ret;
}

OAL_STATIC uint32_t wal_hipriv_rx_filter_frag(oal_net_device_stru *net_dev, const int8_t *param)
{
    wal_msg_write_stru write_msg = { 0 };
    sub_type_load_stru *rx_filter_frag_param = NULL;
    uint32_t param_ret;
    uint8_t filter_frag;
    uint16_t len;
    uint32_t offset;
    int8_t name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    int32_t ret;

    /*
     * OAM eventģ��Ŀ��ص�����: hipriv "wlan0 rx_filter_frag 0 | 1"
     * �˴���������"1"��"0"����name
     */
    param_ret = wal_get_cmd_one_arg(param, name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &offset);
    if (param_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_rx_filter_frag::wal_get_cmd_one_arg return code[%d]}", param_ret);
        return param_ret;
    }

    filter_frag = (uint8_t)oal_atoi(name);
    rx_filter_frag_param = (sub_type_load_stru *)(write_msg.auc_value);
    rx_filter_frag_param->en_sub_type = MAC_SUB_WLAN_CFGID_RX_FILTER_FRAG;
    rx_filter_frag_param->us_buf_len  = sizeof(uint8_t);
    rx_filter_frag_param->auc_buf[0]  = filter_frag;
    len = sizeof(sub_type_load_stru) - WLAN_SUB_EVENT_MAX_LEN + rx_filter_frag_param->us_buf_len;

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_RX_FILTER_FRAG, len);
    ret = wal_send_cfg_event(net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + len,
                             (uint8_t *)&write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_rx_filter_frag::return err code [%d]!}\r\n", ret);
        return (uint32_t)ret;
    }

    return OAL_SUCC;
}
/*lint -e19*/
oal_module_symbol(wal_hipriv_proc_write);
oal_module_symbol(wal_hipriv_get_mac_addr);
/*lint +e19*/

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

