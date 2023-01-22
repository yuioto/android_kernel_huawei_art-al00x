

#ifndef __HMAC_ROAM_ALG_H__
#define __HMAC_ROAM_ALG_H__

#ifdef _PRE_WLAN_FEATURE_11V_ENABLE
#include "hmac_11v.h"
#endif
#include "hmac_roam_connect.h"
#include "hmac_roam_main.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_ROAM_ALG_H

/* �˴�extern "C"ɾ������벻�� */
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_SAE
static inline bool is_only_support_sae(uint32_t *pul_suites)
{
    return ((*(pul_suites) == MAC_RSN_AKM_SAE) && (*((pul_suites) + 1) == 0));
}
static inline bool is_only_support_tbpeke(uint32_t *pul_suites)
{
    return ((*(pul_suites) == MAC_RSN_AKM_TBPEKE) && (*((pul_suites) + 1) == 0));
}
#endif

#define ROAM_LIST_MAX                     (4)     /* ������� */
#define ROAM_BLACKLIST_NORMAL_AP_TIME_OUT (30000) /* ������������ʱʱ�� */
#define ROAM_BLACKLIST_REJECT_AP_TIME_OUT (30000) /* �������ܾ�ģʽ��ʱʱ�� */
#define ROAM_BLACKLIST_COUNT_LIMIT        (1)     /* ��������ʱ�������� */

#define ROAM_HISTORY_BSS_TIME_OUT   (20000) /* ��ʷ��ѡ������ʱʱ�� */
#define ROAM_HISTORY_COUNT_LIMIT    (1)     /* ��ʷ��ѡ��ʱ�������� */
#define ROAM_RSSI_LEVEL             (3)
#define ROAM_CONCURRENT_USER_NUMBER (10)
#define ROAM_THROUGHPUT_THRESHOLD   (1000)

#define ROAM_RSSI_NE85_DB (-85)
#define ROAM_RSSI_NE80_DB (-80)
#define ROAM_RSSI_NE78_DB (-78)
#define ROAM_RSSI_NE75_DB (-75)
#define ROAM_RSSI_NE72_DB (-72)
#define ROAM_RSSI_NE70_DB (-70)
#define ROAM_RSSI_NE68_DB (-68)
#define ROAM_RSSI_NE65_DB (-65)
#define ROAM_RSSI_NE50_DB (-50)

#define ROAM_RSSI_DIFF_2_DB  (2)
#define ROAM_RSSI_DIFF_4_DB  (4)
#define ROAM_RSSI_DIFF_6_DB  (6)
#define ROAM_RSSI_DIFF_8_DB  (8)
#define ROAM_RSSI_DIFF_10_DB (10)
#define ROAM_RSSI_DIFF_15_DB (15)
#define ROAM_RSSI_DIFF_20_DB (20)
#define ROAM_RSSI_DIFF_30_DB (30)

#define ROAM_RSSI_CMD_TYPE      (-128)
#define ROAM_RSSI_LINKLOSS_TYPE (-121)
#define ROAM_RSSI_MAX_TYPE      (-126)

#define ROAM_NEIGHBOR_RPT_LIST_CHN_MAX_NUM (13)
#define ROAM_NEIGHBOR_RPT_LIST_BSSID_MAX_NUM (24)


/* ���κ��������� */
typedef enum {
    ROAM_BLACKLIST_TYPE_NORMAL_AP = 0,
    ROAM_BLACKLIST_TYPE_REJECT_AP = 1,
    ROAM_BLACKLIST_TYPE_BUTT
} roam_blacklist_type_enum;

typedef enum {
    ROAMING_DISABLE = 0,
    ROAMING_RESTART = 1,
    ROAMING_SCENARIO_ENTERPRISE = 2,
    ROAMING_SCENARIO_ENTERPRISE_DENSE = 3,
    ROAMING_SCENARIO_BETTER_AP = 4,
    ROAMING_SCENARIO_HOME = 5,
    ROAMING_SCENARIO_BUTT
} roam_scenario_type_enum;

typedef struct {
    uint32_t time_stamp;  /* ��¼������ʱ��� */
    uint32_t timeout;     /* ��¼��������ʱʱ�� */
    uint16_t us_count_limit; /* ��¼��������ʱǰ�ļ���������� */
    uint16_t us_count;       /* ��������ʱ�����У����Ӹ�Bssid�Ĵ��� */
    uint8_t auc_bssid[WLAN_MAC_ADDR_LEN];
} hmac_roam_bss_info_stru;

/* ����blacklist�ṹ�� */
typedef struct {
    hmac_roam_bss_info_stru ast_bss[ROAM_LIST_MAX];
} hmac_roam_bss_list_stru;

typedef struct {
    int8_t c_trigger_rssi; /* ��ͥ������ѡ����rssi */
    int8_t c_5g_rssi_th;   /* ��ͥ����5g�������� */
    int8_t c_5g_hold_th;   /* ��ͥ����5g������������ */
} hmac_roam_homenetwork_thr_stru;

#define MAX_RECORD_BSSID_NUM 2

typedef struct {
    uint8_t auc_bssid[MAX_RECORD_BSSID_NUM][WLAN_MAC_ADDR_LEN]; /* ��¼�����bssid */
    mac_channel_stru st_channel[MAX_RECORD_BSSID_NUM];            /* ��¼������Ϣ */
    uint8_t uc_bssid_num;                                       /* ��¼bssid num�ĸ��� */
} hmac_roam_record_stru;

/* ���μ�ͥ����ṹ�� */
typedef struct {
    int8_t c_trigger_rssi;     /* ��ͥ������ѡ����rssi */
    int8_t c_5g_rssi_th;       /* ��ͥ����5g�������� */
    int8_t c_5g_hold_th;       /* ��ͥ����5g������������ */
    uint8_t uc_max_retry_cnt;  /* ÿ��rssi��ഥ���������� */
    uint8_t uc_roam_fail_cnt;  /* ����ʧ�ܴ��� */
    uint8_t uc_is_homenetwork; /* �Ƿ��Ǽ�ͥ���糡�� */
    uint8_t auc_resv[2]; /* 2ʵ���ֽڶ��� */
    hmac_roam_record_stru st_2g_bssid; /* ��¼ɨ�赽��2g��bssid */
    hmac_roam_record_stru st_5g_bssid; /* ��¼ɨ�赽��5g��bssid */
    uint32_t scan_timestamp_5g;   /* ��¼ɨ��5g rssiʱ�����30sɨ��һ�� */
} hmac_roam_homenetwork_stru;

/* ��¼ɨ�赽���ŵ���Ϣ */
#define MAX_RECORD_CHAN_NUM    3

typedef struct {
    mac_channel_stru channel[MAX_RECORD_CHAN_NUM];
    uint8_t chan_num;
    uint8_t resv[BYTE_OFFSET_3];
} hmac_roam_record_chan_stru;

typedef struct {
    hmac_roam_record_chan_stru chan_2g;
    hmac_roam_record_chan_stru chan_5g;
} hmac_roam_scan_stru;

/* �����㷨�ṹ�� */
typedef struct {
    hmac_roam_bss_list_stru st_blacklist;    /* ���κ�����AP��ʷ��¼ */
    hmac_roam_bss_list_stru st_history;      /* ������ѡAP��ʷ��¼ */
    uint32_t max_capacity;              /* ��¼ scan �������� capacity */
    mac_bss_dscr_stru *pst_max_capacity_bss; /* ��¼ scan �������� capacity �� bss */
    int8_t c_current_rssi;                 /* ��ǰ dmac ���� rssi */
    int8_t c_max_rssi;                     /* ��¼ scan �������� rssi */
    uint8_t uc_another_bss_scaned;         /* �Ƿ�ɨ�赽�˷ǵ�ǰ������ bss */
    uint8_t uc_invalid_scan_cnt;           /* ����ɨ�赽��ǰ�������״ι����� bss �Ĵ��� */
    uint8_t uc_candidate_bss_num;          /* ��ͬSSID��BSS Num����Ŀ������good, weak, and roaming */
    uint8_t uc_candidate_good_rssi_num;    /* ��ͬSSIDǿ�ź�ǿ�ȵ�BSS Num */
    uint8_t uc_candidate_weak_rssi_num;    /* ��ͬSSID���ź�ǿ�ȵ�BSS Num */
    uint8_t uc_scan_period;                /* �Ӹ��ܳ�������ܳ����л���ɨ������ */
    uint8_t uc_better_rssi_scan_period;    /* �и����ź�ǿ��AP������ɨ����� */
    uint8_t uc_better_rssi_null_period;
    uint8_t uc_roam_fail_cnt;
    uint8_t uc_rsv[1];
    hmac_roam_homenetwork_stru st_home_network; /* ��ͥ����ṹ����Ϣ */
    mac_bss_dscr_stru *pst_max_rssi_bss;        /* ��¼ scan �������� rssi �� bss */
} hmac_roam_alg_stru;

/* ����connect�ṹ�� */
typedef struct {
    roam_connect_state_enum en_state;
    mac_status_code_enum_uint16 en_status_code; /* auth/assoc rsp frame status code */
    uint8_t uc_roam_main_fsm_state; // only for chr record main statemachine
    uint8_t uc_auth_num;
    uint8_t uc_assoc_num;
    uint8_t uc_ft_num;
    uint8_t uc_ft_force_air; /* 0:����ʹ��ds��1:dsʧ�ܣ���ֹʹ��ds */
    uint8_t uc_ft_failed;    /* 0:ds����δʧ�ܣ�1:ds����ʧ�� */
    frw_timeout_stru st_timer; /* ����connectʹ�õĶ�ʱ�� */
    mac_bss_dscr_stru *pst_bss_dscr;
} hmac_roam_connect_stru;
typedef struct {
    int8_t c_rssi;
    uint32_t capacity_kbps;
} hmac_roam_rssi_capacity_stru;

/* ��bss���ݽṹ�� */
typedef struct {
    uint8_t auc_bssid[WLAN_MAC_ADDR_LEN];
    uint16_t us_sta_aid;
#ifdef _PRE_WLAN_FEATURE_TXBF
    mac_vap_txbf_add_stru st_txbf_add_cap;
#endif
    mac_cap_flag_stru st_cap_flag;
    mac_channel_stru st_channel;
    wlan_mib_ieee802dot11_stru st_mib_info;
    mac_user_cap_info_stru st_cap_info;
    mac_key_mgmt_stru st_key_info;
    mac_user_tx_param_stru st_user_tx_info; /* TX��ز��� */
    hmac_rate_stru st_op_rates;
    mac_user_ht_hdl_stru st_ht_hdl;
    mac_vht_hdl_stru st_vht_hdl;
    wlan_bw_cap_enum_uint8 en_bandwidth_cap;
    wlan_bw_cap_enum_uint8 en_avail_bandwidth;
    wlan_bw_cap_enum_uint8 en_cur_bandwidth;
    wlan_protocol_enum_uint8 en_protocol_mode;
    wlan_protocol_enum_uint8 en_avail_protocol_mode;
    wlan_protocol_enum_uint8 en_cur_protocol_mode;
    wlan_nss_enum_uint8 en_user_num_spatial_stream;
    wlan_nss_enum_uint8 en_avail_num_spatial_stream;
    wlan_nss_enum_uint8 en_cur_num_spatial_stream;
    wlan_nss_enum_uint8 en_avail_bf_num_spatial_stream;
    uint16_t us_cap_info; /* �ɵ�bss������λ��Ϣ */
    mac_ap_type_enum_uint16 en_ap_type;
    mac_sap_mode_enum_uint8 sap_mode;
    uint8_t reservd[NUM_1_BYTES];
} hmac_roam_old_bss_stru;

/* �������ṹ�� */
typedef struct {
    uint8_t uc_enable;                       /* ����ʹ�ܿ��� */
    roam_trigger_enum en_roam_trigger;   /* ���δ���Դ */
    roam_main_state_enum en_main_state;  /* ������״̬ */
    roam_scenario_enum uc_rssi_type;     /* ����rssi���ʹ������� */
    uint8_t uc_invalid_scan_cnt;             /* ��Чɨ��ͳ�� */
    oal_bool_enum_uint8 en_current_bss_ignore; /* �Ƿ�֧�����λ��Լ�(֧�����������ع���) */
    uint8_t auc_target_bssid[WLAN_MAC_ADDR_LEN];
#ifdef _PRE_WLAN_FEATURE_11V_ENABLE
    oal_work_stru hmac_11v_roam_work;        /* 11v 2�ε��ŵ�+1��ȫ�ŵ�ɨ�� �������� */
    hmac_bsst_rsp_info_stru st_bsst_rsp_info;
#endif
    hmac_vap_stru *pst_hmac_vap;                       /* ���ζ�Ӧ��vap */
    hmac_user_stru *pst_hmac_user;                     /* ���ζ�Ӧ��BSS user */
    hmac_roam_old_bss_stru st_old_bss;                 /* ����֮ǰ�����bss�����Ϣ */
    mac_scan_req_h2d_stru st_scan_h2d_params;          /* ����ɨ����� */
    hmac_roam_config_stru st_config;                   /* �������������Ϣ */
    hmac_roam_connect_stru st_connect;                 /* ����connect��Ϣ */
    hmac_roam_alg_stru st_alg;                         /* �����㷨��Ϣ */
    hmac_roam_static_stru st_static;                   /* ����ͳ����Ϣ */
    hmac_roam_scan_stru scan_info;                     /* ����ɨ����Ϣ */
    frw_timeout_stru st_timer;                         /* ����ʹ�õĶ�ʱ�� */
    hmac_wpas_connect_state_enum_uint32 connected_state; /* �ⲿ������״̬���� */

    uint8_t auc_neighbor_rpt_bssid[ROAM_NEIGHBOR_RPT_LIST_BSSID_MAX_NUM][WLAN_MAC_ADDR_LEN];
    uint8_t uc_neighbor_rpt_bssid_num;
    uint8_t uc_roaming_scenario;
    oal_bool_enum_uint8 is_roaming_trigged_by_cmd; /* �Ƿ����������ǿ������  0:���� 1:�� */

    uint8_t stay_on_current_bss;                    /* ��Ҫפ����ǰbss���ܾ����� */
    mac_bss_dscr_stru save_bss_dscr_info;           /* ������ǰѡ���bss��Ϣ */
} hmac_roam_info_stru;
typedef uint32_t (*hmac_roam_fsm_func)(hmac_roam_info_stru *pst_roam_info, void *p_param);

#ifdef _PRE_WINDOWS_SUPPORT
typedef struct {
    hmac_roam_info_stru roam_info;
    mac_bss_dscr_stru bss_dscr;
    uint8_t scan_timeout; // ����ɨ�賬ʱ��ǣ�TRUE��ʶ��ʱ��FALSE��ʶδ��ʱ��Ĭ��δ��ʱ
    uint8_t rsv[NUM_3_BYTES];
} hmac_roam_info_context;
#endif

uint32_t hmac_roam_alg_add_blacklist(hmac_roam_info_stru *pst_roam_info, uint8_t *puc_bssid,
                                     roam_blacklist_type_enum list_type);
uint32_t hmac_roam_alg_add_history(hmac_roam_info_stru *pst_roam_info, uint8_t *puc_bssid);
uint32_t hmac_roam_alg_bss_check(hmac_roam_info_stru *pst_roam_info,
                                 mac_bss_dscr_stru *pst_bss_dscr);
#if defined(_PRE_WLAN_FEATURE_11K) || defined(_PRE_WLAN_FEATURE_11V_ENABLE)
void hmac_roam_neighbor_report_add_chan(mac_scan_req_stru *pst_scan_params,
    uint8_t uc_channum, wlan_channel_band_enum_uint8 en_band, uint8_t uc_channel_idx);
void hmac_roam_neighbor_report_add_bssid(hmac_roam_info_stru *pst_roam_info, uint8_t *puc_bssid);
#endif
void hmac_roam_alg_init(hmac_roam_info_stru *pst_roam_info, int8_t c_current_rssi);
void hmac_roam_alg_init_rssi(hmac_vap_stru *pst_hmac_vap, hmac_roam_info_stru *pst_roam_info);
mac_bss_dscr_stru *hmac_roam_alg_select_bss(hmac_roam_info_stru *pst_roam_info);
oal_bool_enum_uint8 hmac_roam_alg_find_in_blacklist(hmac_roam_info_stru *pst_roam_info,
                                                    uint8_t *puc_bssid);
oal_bool_enum_uint8 hmac_roam_alg_find_in_history(hmac_roam_info_stru *pst_roam_info,
                                                  uint8_t *puc_bssid);
oal_bool_enum_uint8 hmac_roam_alg_need_to_stop_roam_trigger(hmac_roam_info_stru *pst_roam_info);
uint32_t hmac_roam_alg_bss_in_ess(hmac_roam_info_stru *pst_roam_info,
                                  mac_bss_dscr_stru *pst_bss_dscr);
#ifdef _PRE_WLAN_FEATURE_11V_ENABLE
oal_bool_enum_uint8 hmac_roam_alg_check_bsst_bssid_in_scan_list(hmac_roam_info_stru *pst_roam_info,
                                                                uint8_t *puc_mac_addr);
#endif
void hmac_chr_roam_info_report(hmac_roam_info_stru *pst_roam_info, uint8_t result);
uint32_t hmac_roam_main_fsm_action(hmac_roam_info_stru *roam_info,
    roam_main_fsm_event_type_enum event, void *param);
uint32_t hmac_roam_main_check_state(hmac_roam_info_stru *roam_info,
                                    mac_vap_state_enum_uint8 vap_state,
                                    roam_main_state_enum main_state);
void hmac_roam_main_change_state(hmac_roam_info_stru *roam_info, roam_main_state_enum state);
#ifdef _PRE_WLAN_FEATURE_11K
uint32_t hmac_roam_send_neighbor_req(hmac_roam_info_stru *roam_info, uint8_t  *cur_ssid);
#endif
void hmac_roam_main_start_timer(hmac_roam_info_stru *roam_info, uint32_t timeout);
uint32_t hmac_roam_handle_scan_result(hmac_roam_info_stru *roam_info, void *param);
void hmac_roam_main_clear(hmac_roam_info_stru *roam_info);
uint32_t hmac_roam_resume_pm(hmac_roam_info_stru *roam_info);
void hmac_roam_resume_security_port(hmac_roam_info_stru *roam_info);
uint32_t hmac_roam_scan_timeout(hmac_roam_info_stru *roam_info, void *param);
void hmac_roam_main_del_timer(hmac_roam_info_stru *roam_info);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of hmac_roam_alg.h */