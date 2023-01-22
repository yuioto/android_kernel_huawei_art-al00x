

#ifndef __WAL_LINUX_IOCTL_H__
#define __WAL_LINUX_IOCTL_H__

// 1 ����ͷ�ļ�����
#include "oal_ext_if.h"
#include "wlan_types.h"
#include "wlan_spec.h"
#include "mac_vap.h"
#include "hmac_ext_if.h"
#include "wal_ext_if.h"
#include "wal_config.h"

#ifdef _PRE_WLAN_FEATURE_IP_FILTER
#ifdef CONFIG_DOZE_FILTER
#include <huawei_platform/power/wifi_filter/wifi_filter.h>
#endif /* CONFIG_DOZE_FILTER */
#endif /* _PRE_WLAN_FEATURE_IP_FILTER */

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_LINUX_IOCTL_H

// 2 �궨��
#define WAL_HIPRIV_CMD_MAX_LEN       (WLAN_MEM_LOCAL_SIZE2 - 4)  /* ˽�����������ַ�����󳤶ȣ���Ӧ�����ڴ��һ����С */
#define WAL_HIPRIV_CMD_NAME_MAX_LEN  80                          /* �ַ�����ÿ�����ʵ���󳤶�(ԭ20) */
#define WAL_HIPRIV_PROC_ENTRY_NAME   "hipriv"
#define WAL_SIOCDEVPRIVATE           0x89F0  /* SIOCDEVPRIVATE */
#define WAL_HIPRIV_HT_MCS_MIN        0
#define WAL_HIPRIV_HT_MCS_MAX        32
#define WAL_HIPRIV_VHT_MCS_MIN       0
#define WAL_HIPRIV_VHT_MCS_MAX       9
#define WAL_HIPRIV_NSS_MIN           1
#define WAL_HIPRIV_NSS_MAX           4
#define WAL_HIPRIV_CH_NUM            4
#define WAL_HIPRIV_BOOL_NIM          0
#define WAL_HIPRIV_BOOL_MAX          1
#define WAL_HIPRIV_FREQ_SKEW_ARG_NUM 8
#define BYTE_ALIGNED_3               3

#define WAL_HIPRIV_MS_TO_S                   1000   /* ms��s֮�䱶���� */
#define WAL_HIPRIV_KEEPALIVE_INTERVAL_MIN    5000   /* ��Ĭ���ϻ�����������ʱ�������� */
#define WAL_HIPRIV_KEEPALIVE_INTERVAL_MAX    0xffff /* timer���ʱ����������(oal_uin16) */


/* IOCTL˽����������궨�� */
#define WAL_IOCTL_PRIV_SETPARAM          (OAL_SIOCIWFIRSTPRIV + 0)
#define WAL_IOCTL_PRIV_GETPARAM          (OAL_SIOCIWFIRSTPRIV + 1)
#define WAL_IOCTL_PRIV_SET_WMM_PARAM     (OAL_SIOCIWFIRSTPRIV + 3)
#define WAL_IOCTL_PRIV_GET_WMM_PARAM     (OAL_SIOCIWFIRSTPRIV + 5)
#define WAL_IOCTL_PRIV_SET_COUNTRY       (OAL_SIOCIWFIRSTPRIV + 8)
#define WAL_IOCTL_PRIV_GET_COUNTRY       (OAL_SIOCIWFIRSTPRIV + 9)
#define WAL_IOCTL_PRIV_GET_MODE          (OAL_SIOCIWFIRSTPRIV + 17)      /* ��ȡģʽ */
#define WAL_IOCTL_PRIV_SET_MODE          (OAL_SIOCIWFIRSTPRIV + 18)      /* ����ģʽ ����Э�� Ƶ�� ���� */

#define WAL_IOCTL_PRIV_AP_GET_STA_LIST   (OAL_SIOCIWFIRSTPRIV + 21)
#define WAL_IOCTL_PRIV_AP_MAC_FLTR       (OAL_SIOCIWFIRSTPRIV + 22)
/* netd��������������ΪGET��ʽ�·���get��ʽ������������set��ż�� */
#define WAL_IOCTL_PRIV_SET_AP_CFG        (OAL_SIOCIWFIRSTPRIV + 23)
#define WAL_IOCTL_PRIV_AP_STA_DISASSOC   (OAL_SIOCIWFIRSTPRIV + 24)

#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
#define WAL_IOCTL_PRIV_GET_BLACKLIST     (OAL_SIOCIWFIRSTPRIV + 27)
#define WAL_IOCTL_PRIV_SET_ISOLATION     (OAL_SIOCIWFIRSTPRIV + 28)
#endif
#define WAL_IOCTL_PRIV_SUBCMD_MAX_LEN    20
typedef oal_uint32 (*wal_hipriv_cmd_func)(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);

// 3 ö�ٶ���
typedef enum {
    WAL_DSCR_PARAM_FREQ_BANDWIDTH_MODE = 0, /* Ƶ��ģʽ */
    WAL_DSCR_PARAM_PA_GAIN_LEVEL,           /* pa����ȼ� */
    WAL_DSCR_PARAM_MICRO_TX_POWER_GAIN_LEVEL,
    WAL_DSCR_PARAM_SMART_ANTENNA_ENABLE,
    WAL_DSCR_PARAM_TXRTS_ANTENNA,
    WAL_DSCR_PARAM_RXCTRL_ANTENNA,
    WAL_DSCR_PARAM_CHANNAL_CODE,
    WAL_DSCR_PARAM_DATA_RATE0,
    WAL_DSCR_PARAM_DATA_RATE1,
    WAL_DSCR_PARAM_DATA_RATE2,
    WAL_DSCR_PARAM_DATA_RATE3,
    WAL_DSCR_PARAM_POWER,
    WAL_DSCR_PARAM_SHORTGI,
    WAL_DSCR_PARAM_PREAMBLE_MODE,
    WAL_DSCR_PARAM_RTSCTS,
    WAL_DSCR_PARAM_LSIGTXOP,
    WAL_DSCR_PARAM_SMOOTH,
    WAL_DSCR_PARAM_SOUNDING,
    WAL_DSCR_PARAM_TXBF,
    WAL_DSCR_PARAM_STBC,
    WAL_DSCR_PARAM_GET_ESS,
    WAL_DSCR_PARAM_DYN_BW,
    WAL_DSCR_PARAM_DYN_BW_EXIST,
    WAL_DSCR_PARAM_CH_BW_EXIST,

    WAL_DSCR_PARAM_BUTT
} wal_dscr_param_enum;
typedef oal_uint8 wal_dscr_param_enum_uint8;
typedef enum {
    CMD_STR_AUTH_RSP_TIME    = 0, /* APģʽ����auth rsp��ʱʱ�� */
    CMD_STR_FORBIT_OPEN_AUTH = 1, /* �ܾ�openģʽ���� */
    CMD_STR_BUTT
} cmd_str_config_enum;
#ifdef _PRE_WLAN_FEATURE_HILINK
typedef enum {
    WAL_HILINK_OTHER_BSS_CAST_DATA_CLOSE, /* ����BSS�����鲥����֡�ϱ��ر� */
    WAL_HILINK_OTHER_BSS_CAST_DATA_OPEN,  /* ����BSS�����鲥����֡�ϱ��� */
    WAL_HILINK_MONITOR_CLOSE,             /* monitorģʽ�ر� */
    WAL_HILINK_MONITOR_OPEN,              /* monitorģʽ�� */
    WAL_HILINK_BUTT
} wal_hilink_enum;
typedef oal_uint8 wal_hilink_enum_uint8;
#endif

/* rx ip���ݰ����˹��ܺ��ϲ�Э��(��ʽ)�ṹ�壬�ϲ�ӿ���δ��ȷ���������޸� */
#ifdef CONFIG_DOZE_FILTER
typedef hw_wifi_filter_item wal_hw_wifi_filter_item;
typedef struct hw_wlan_filter_ops wal_hw_wlan_filter_ops;
#else
typedef struct {
    unsigned short protocol; // Э������
    unsigned short port;     // Ŀ�Ķ˿ں�
    unsigned int filter_cnt; // ���˱�����
} wal_hw_wifi_filter_item;

typedef struct {
    int (*set_filter_enable)(int);
    int (*set_filter_enable_ex)(int, int);
    int (*add_filter_items)(wal_hw_wifi_filter_item *, int);
    int (*clear_filters)(void);
    int (*get_filter_pkg_stat)(wal_hw_wifi_filter_item *, int, int *);
} wal_hw_wlan_filter_ops;
#endif

// 4 ȫ�ֱ�������
extern oal_iw_handler_def_stru g_st_iw_handler_def;
extern oal_net_device_ops_stru g_st_wal_net_dev_ops;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
extern oal_ethtool_ops_stru g_st_wal_ethtool_ops;
#endif

#ifdef _PRE_WLAN_FEATURE_DFR
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
extern oal_mutex_stru g_dfr_mutex;
#endif
#endif

/* ˽��������ڽṹ���� */
typedef struct {
    oal_int8 *pc_cmd_name;      /* �����ַ��� */
    wal_hipriv_cmd_func p_func; /* �����Ӧ������ */
} wal_hipriv_cmd_entry_stru;

/* Э��ģʽ���ַ���ӳ�� */
typedef struct {
    oal_int8 *pc_name;                              /* ģʽ���ַ��� */
    wlan_protocol_enum_uint8 en_mode;               /* Э��ģʽ */
    wlan_channel_band_enum_uint8 en_band;           /* Ƶ�� */
    wlan_channel_bandwidth_enum_uint8 en_bandwidth; /* ���� */
    oal_uint8 auc_resv[1];
} wal_ioctl_mode_map_stru;

/* �㷨�������ýṹ�� */
typedef struct {
    oal_int8 *pc_name;                  /* ���������ַ��� */
    mac_alg_cfg_enum_uint8 en_alg_cfg;  /* ���������Ӧ��ö��ֵ */
    oal_uint8 auc_resv[BYTE_ALIGNED_3]; /* �ֽڶ��� */
} wal_ioctl_alg_cfg_stru;

/* 1102 ʹ��wpa_supplicant �·����� */
typedef struct wal_android_wifi_priv_cmd {
    oal_int32 l_total_len;
    oal_int32 l_used_len;
    oal_uint8 *puc_buf;
} wal_android_wifi_priv_cmd_stru;

extern oal_uint32 wal_hipriv_set_rate(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
extern oal_uint32 wal_hipriv_set_mcs(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
extern oal_uint32 wal_hipriv_set_mcsac(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
extern oal_uint32 wal_hipriv_vap_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);

extern oal_uint32 wal_hipriv_create_proc(oal_void *p_proc_arg);
extern oal_int32 wal_netdev_stop(oal_net_device_stru *pst_net_dev);
extern oal_int32 wal_netdev_open(oal_net_device_stru *pst_net_dev);
extern oal_uint32 wal_hipriv_del_vap(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
extern oal_uint32 wal_hipriv_remove_proc(oal_void);
#ifdef _PRE_WLAN_FEATURE_IP_FILTER
extern uint32_t wal_hipriv_set_ip_filter(oal_net_device_stru *net_dev, const int8_t *param);
#endif // _PRE_WLAN_FEATURE_IP_FILTER
extern oal_uint32  wal_alloc_cfg_event(oal_net_device_stru *pst_net_dev, frw_event_mem_stru **ppst_event_mem,
                                       const oal_void *pst_resp_addr, wal_msg_stru **ppst_cfg_msg, oal_uint16 us_len);
extern oal_int32   wal_send_cfg_event(oal_net_device_stru     *pst_net_dev,
                                      wal_msg_type_enum_uint8  en_msg_type,
                                      oal_uint16               us_len,
                                      const oal_uint8         *puc_param,
                                      oal_bool_enum_uint8      en_need_rsp,
                                      wal_msg_stru           **ppst_rsp_msg);

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
extern oal_int32 wal_start_vap(oal_net_device_stru *pst_net_dev);
extern oal_int32 wal_stop_vap(oal_net_device_stru *pst_net_dev);
extern oal_int32 wal_netdev_stop_ap(oal_net_device_stru *pst_net_dev);
extern oal_int32 wal_init_wlan_vap(oal_net_device_stru *pst_net_dev);
extern oal_int32 wal_deinit_wlan_vap(oal_net_device_stru *pst_net_dev);
extern oal_int32 wal_init_wlan_netdev(oal_wiphy_stru *pst_wiphy, const char *dev_name);
extern oal_int32 wal_setup_ap(oal_net_device_stru *pst_net_dev);
extern oal_int32 wal_host_dev_init(oal_net_device_stru *pst_net_dev);
extern oal_uint32 wal_set_ota_test(oal_net_device_stru *pst_netdev);
#endif

#ifdef _PRE_WLAN_FEATURE_11D
extern oal_uint32 wal_regdomain_update_sta(oal_uint8 uc_vap_id);
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
oal_int32 wal_set_random_mac_to_mib(oal_net_device_stru *pst_net_dev);
#endif

wlan_p2p_mode_enum_uint8 wal_wireless_iftype_to_mac_p2p_mode(enum nl80211_iftype en_iftype);

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
extern oal_uint32 wal_hipriv_arp_offload_enable(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif
extern oal_int32 wal_cfg_vap_h2d_event(oal_net_device_stru *pst_net_dev);
#ifdef _PRE_WLAN_FEATURE_ROAM
extern oal_uint32 wal_hipriv_roam_enable(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
extern oal_uint32 wal_hipriv_roam_band(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
extern oal_uint32 wal_hipriv_roam_org(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
extern oal_uint32 wal_hipriv_roam_start(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
extern oal_uint32 wal_hipriv_roam_info(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#endif // _PRE_WLAN_FEATURE_ROAM
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE

extern oal_void hwifi_config_init_dbb_main(oal_net_device_stru *pst_cfg_net_dev);
#if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1131C_HOST)
/* 1131ֻ����ini�ļ���ʽ,������dts��nv��ʽ���ж��ƻ��������� */
extern oal_void hwifi_config_init_nvram_main(oal_net_device_stru *pst_cfg_net_dev);
extern oal_uint32 hwifi_config_init_dts_main(oal_net_device_stru *pst_cfg_net_dev);
#endif
extern int32_t wal_set_custom_process_func(custom_cali_func fun);
extern oal_uint32 wal_custom_cali(oal_void);
extern oal_void hwifi_config_init_force(oal_void);
extern void hwifi_config_init_once(void);
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

#ifndef WIN32
oal_int32 wal_recover_ini_main_gloabal(void);
#endif

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
extern oal_ssize_t wal_hipriv_cmd_sys_write(struct kobject *dev, struct kobj_attribute *attr, const char *pc_buffer,
    oal_size_t count);
#endif
extern oal_int32 wal_host_dev_exit(oal_net_device_stru *pst_net_dev);
extern oal_int32 wal_init_max_sta_num(oal_net_device_stru *pst_net_dev, oal_uint32 ul_max_num);
extern oal_int32 wal_send_cfg_event(oal_net_device_stru *pst_net_dev, wal_msg_type_enum_uint8 en_msg_type,
    oal_uint16 us_len, const oal_uint8 *puc_param, oal_bool_enum_uint8 en_need_rsp, wal_msg_stru **ppst_rsp_msg);
extern oal_int32 wal_set_pm_on(oal_net_device_stru *pst_cfg_net_dev, const oal_void *p_buf);
extern void wal_set_random_mac_addr(uint8_t *primary_mac_addr);

#ifndef WIN32
extern int32_t device_mem_check(void);
extern oal_int32 wal_update_macaddr_to_netdev_and_lwip(oal_uint8 *puc_macaddr);
extern oal_uint32 wal_set_country(oal_uint8 *pc_param);
extern oal_int32 wal_rx_fcs_info(void);
extern oal_uint32 wal_set_always_tx(const oal_int8 *pc_param);
extern oal_uint32 wal_set_always_rx(const oal_int8 *pc_param);
extern oal_int32 wal_set_wlan_pm_state(oal_uint32 ul_pm_state);

extern int32_t wal_set_ip_filter_enable(int32_t on);
extern int32_t wal_set_assigned_filter_enable(int32_t filter_id, int32_t filter_switch);
extern int32_t wal_add_ip_filter_items(wal_hw_wifi_filter_item *items, int32_t count);
extern int32_t wal_clear_ip_filter(void);
#ifdef _PRE_WLAN_FEATURE_IP_FILTER
extern int32_t wal_register_ip_filter(wal_hw_wlan_filter_ops *ip_filter_ops);
extern int32_t wal_unregister_ip_filter(void);
#endif /* _PRE_WLAN_FEATURE_IP_FILTER */

#ifdef _PRE_WLAN_FEATURE_11D

#ifdef _PRE_WLAN_FEATURE_DFS
extern int32_t wal_regdomain_update_for_dfs(oal_net_device_stru *pst_net_dev, int8_t *pc_country);
#endif
extern int32_t wal_regdomain_update(oal_net_device_stru *pst_net_dev, const int8_t *pc_country);

#endif

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif /* end of wal_linux_ioctl.h */