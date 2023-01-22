

#ifndef __HMAC_ROAM_ALG_H__
#define __HMAC_ROAM_ALG_H__

#include "oam_wdk.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_ROAM

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_ROAM_ALG_H

// 1 �궨��
#define ROAM_LIST_MAX                     20          /* ������� */
#define ROAM_BLACKLIST_NORMAL_AP_TIME_OUT 50000       /* ������������ʱʱ�� */
#define ROAM_BLACKLIST_REJECT_AP_TIME_OUT 100000      /* �������ܾ�ģʽ��ʱʱ�� */
#define ROAM_BLACKLIST_COUNT_LIMIT        2           /* ��������ʱ�������� */

#define ROAM_HISTORY_BSS_TIME_OUT         20000       /* ��ʷ��ѡ������ʱʱ�� */
#define ROAM_HISTORY_COUNT_LIMIT          1           /* ��ʷ��ѡ��ʱ�������� */
#define ROAM_RSSI_LEVEL                   3
#define ROAM_CONCURRENT_USER_NUMBER       10
#define ROAM_THROUGHPUT_THRESHOLD         1000

#define ROAM_RSSI_NE80_DB                 (-80)
#define ROAM_RSSI_NE75_DB                 (-75)
#define ROAM_RSSI_NE70_DB                 (-70)

#define ROAM_RSSI_DIFF_4_DB               4
#define ROAM_RSSI_DIFF_6_DB               6
#define ROAM_RSSI_DIFF_8_DB               8
#define ROAM_RSSI_DIFF_10_DB              10

#define ROAM_RSSI_CMD_TYPE                (-128)
#define ROAM_RSSI_LINKLOSS_TYPE           (-121)
#define ROAM_RSSI_MAX_TYPE                (-126)


// 2 ö�ٶ���
/* ���κ��������� */
typedef enum {
    ROAM_BLACKLIST_TYPE_NORMAL_AP = 0,
    ROAM_BLACKLIST_TYPE_REJECT_AP = 1,
    ROAM_BLACKLIST_TYPE_BUTT
} roam_blacklist_type_enum;
typedef oal_uint8 roam_blacklist_type_enum_uint8;


// 3 STRUCT����
typedef struct {
    uint32_t ul_time_stamp;           /* ��¼������ʱ��� */
    uint32_t ul_timeout;              /* ��¼��������ʱʱ�� */
    uint16_t us_count_limit;          /* ��¼��������ʱǰ�ļ���������� */
    uint16_t us_count;                /* ��������ʱ�����У���Ӹ�Bssid�Ĵ��� */
    uint8_t  auc_bssid[WLAN_MAC_ADDR_LEN];
} hmac_roam_bss_info_stru;

/* ����blacklist�ṹ�� */
typedef struct {
    hmac_roam_bss_info_stru ast_bss[ROAM_LIST_MAX];
} hmac_roam_bss_list_stru;

/* �����㷨�ṹ�� */
typedef struct {
    hmac_roam_bss_list_stru st_blacklist;          /* ���κ�����AP��ʷ��¼ */
    hmac_roam_bss_list_stru st_history;            /* ������ѡAP��ʷ��¼ */
    uint32_t                ul_max_capacity;       /* ��¼ scan �������� capacity */
    mac_bss_dscr_stru       *pst_max_capacity_bss;  /* ��¼ scan �������� capacity �� bss */
    int8_t                  c_current_rssi;        /* ��ǰ dmac ���� rssi */
    int8_t                  c_max_rssi;            /* ��¼ scan �������� rssi */
    uint8_t                 uc_another_bss_scaned; /* �Ƿ�ɨ�赽�˷ǵ�ǰ������ bss */
    uint8_t                 uc_invalid_scan_cnt;   /* ����ɨ�赽��ǰ�������״ι����� bss �Ĵ��� */
    mac_bss_dscr_stru       *pst_max_rssi_bss;      /* ��¼ scan �������� rssi �� bss */
} hmac_roam_alg_stru;

/* ����connect�ṹ�� */
typedef struct {
    roam_connect_state_enum_uint8 en_state;
    uint8_t                       auc_bssid[WLAN_MAC_ADDR_LEN];
    mac_channel_stru              st_channel;
    frw_timeout_stru              st_timer;           /* ����connectʹ�õĶ�ʱ�� */
    mac_bss_dscr_stru             *pst_bss_dscr;
} hmac_roam_connect_stru;
typedef struct {
    int8_t   c_rssi;
    uint32_t ul_capacity_kbps;
} hmac_roam_rssi_capacity_stru;

/* ��bss���ݽṹ�� */
typedef struct {
    uint8_t                    auc_bssid[WLAN_MAC_ADDR_LEN];
    uint16_t                   us_sta_aid;
#ifdef _PRE_WLAN_FEATURE_TXBF
    mac_vap_txbf_add_stru      st_txbf_add_cap;
#endif
    mac_cap_flag_stru          st_cap_flag;
    mac_channel_stru           st_channel;
    wlan_mib_ieee802dot11_stru st_mib_info;
    mac_user_cap_info_stru     st_cap_info;
    mac_key_mgmt_stru          st_key_info;
    mac_user_tx_param_stru     st_user_tx_info;    /* TX��ز��� */
    mac_rate_stru              st_op_rates;
    mac_user_ht_hdl_stru       st_ht_hdl;
    mac_vht_hdl_stru           st_vht_hdl;
    wlan_bw_cap_enum_uint8     en_bandwidth_cap;
    wlan_bw_cap_enum_uint8     en_avail_bandwidth;
    wlan_bw_cap_enum_uint8     en_cur_bandwidth;
    wlan_protocol_enum_uint8   en_protocol_mode;
    wlan_protocol_enum_uint8   en_avail_protocol_mode;
    wlan_protocol_enum_uint8   en_cur_protocol_mode;
    uint8_t                    uc_num_spatial_stream;
    uint8_t                    uc_avail_num_spatial_stream;
    uint8_t                    uc_cur_num_spatial_stream;
    uint8_t                    uc_avail_bf_num_spatial_stream;
    uint16_t                   us_cap_info;        /* �ɵ�bss������λ��Ϣ */
} hmac_roam_old_bss_stru;

/* �������ṹ�� */
typedef struct {
    uint8_t                        uc_enable;           /* ����ʹ�ܿ��� */
    roam_main_state_enum_uint8     en_main_state;       /* ������״̬ */
    uint8_t                        uc_rssi_ignore;      /* ����rssi�������� */
    uint8_t                        auc_resv[1];
    hmac_vap_stru                  *pst_hmac_vap;        /* ���ζ�Ӧ��vap */
    hmac_user_stru                 *pst_hmac_user;       /* ���ζ�Ӧ��BSS user */
    hmac_roam_old_bss_stru         *pst_old_bss;         /* ����֮ǰ�����bss�����Ϣ */
    mac_scan_req_stru              st_scan_params;      /* ����ɨ����� */
    hmac_roam_config_stru          st_config;           /* �������������Ϣ */
    hmac_roam_connect_stru         st_connect;          /* ����connect��Ϣ */
    hmac_roam_alg_stru             st_alg;              /* �����㷨��Ϣ */
    hmac_roam_static_stru          st_static;           /* ����ͳ����Ϣ */
    frw_timeout_stru               st_timer;            /* ����ʹ�õĶ�ʱ�� */
    wpas_connect_state_enum_uint32 ul_connected_state;  /* �ⲿ������״̬���� */
    uint32_t                       ul_ip_addr_obtained; /* IP��ַ�Ƿ��ȡ */
} hmac_roam_info_stru;
typedef uint32_t  (*hmac_roam_fsm_func)(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);


// 4 ��������
uint32_t hmac_roam_alg_add_blacklist(hmac_roam_info_stru *pst_roam_info, uint8_t *puc_bssid,
    roam_blacklist_type_enum_uint8 list_type);
uint32_t hmac_roam_alg_add_history(hmac_roam_info_stru *pst_roam_info, uint8_t *puc_bssid);
uint32_t hmac_roam_alg_bss_check(hmac_roam_info_stru *pst_roam_info, mac_bss_dscr_stru *pst_bss_dscr);
uint32_t hmac_roam_alg_scan_channel_init(hmac_roam_info_stru *pst_roam_info, mac_scan_req_stru *pst_scan_params);
void hmac_roam_alg_init(hmac_roam_info_stru *pst_roam_info, int8_t c_current_rssi);
mac_bss_dscr_stru *hmac_roam_alg_select_bss(hmac_roam_info_stru *pst_roam_info);
oal_bool_enum_uint8 hmac_roam_alg_find_in_blacklist(hmac_roam_info_stru *pst_roam_info, uint8_t *puc_bssid);
oal_bool_enum_uint8 hmac_roam_alg_find_in_history(hmac_roam_info_stru *pst_roam_info, uint8_t *puc_bssid);
oal_bool_enum_uint8 hmac_roam_alg_need_to_stop_roam_trigger(hmac_roam_info_stru *pst_roam_info);
uint32_t hmac_roam_alg_bss_in_ess(hmac_roam_info_stru *pst_roam_info, mac_bss_dscr_stru *pst_bss_dscr);
#endif // _PRE_WLAN_FEATURE_ROAM

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of hmac_roam_alg.h */
