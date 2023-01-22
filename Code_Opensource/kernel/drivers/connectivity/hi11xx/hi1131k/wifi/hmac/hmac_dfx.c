

/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "oal_ext_if.h"
#include "oam_ext_if.h"
#include "hmac_dfx.h"
#include "oal_file.h"

#ifdef _PRE_WLAN_1131_CHR
#include "mac_resource.h"
#include "mac_vap.h"
#include "chr_user.h"
#include "chr_errno.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_DFX_C

/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/
OAL_STATIC oam_cfg_data_stru  g_ast_cfg_data[OAM_CFG_TYPE_BUTT] = {
    {OAM_CFG_TYPE_MAX_ASOC_USER,     "USER_SPEC",     "max_asoc_user",     31},
};

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
/* ���1102  Host���ڸñ����������ض��� */
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
oal_int32    g_l_proxysta_feature = 0;
#endif
/* �����û��������� Root APģʽ��Ϊ32��,Repeaterģʽ����15�� */
oal_uint16   g_us_wlan_assoc_user_max_num     = WLAN_ASSOC_USER_MAX_NUM_LIMIT;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
/* 3��AP + 1������vap */
oal_uint32 g_ul_wlan_vap_max_num_per_device = WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE + WLAN_CONFIG_VAP_MAX_NUM_PER_DEVICE;
#else
 /* 4��AP + 1������vap */
oal_uint32 g_ul_wlan_vap_max_num_per_device = WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE + WLAN_CONFIG_VAP_MAX_NUM_PER_DEVICE;
#endif

#else
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
extern oal_int32    g_l_proxysta_feature ;
#endif
extern oal_uint16   g_us_wlan_assoc_user_max_num;
extern oal_uint32   g_ul_wlan_vap_max_num_per_device;
#endif

/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/

oal_int32  oam_cfg_get_item_by_id(oam_cfg_type_enum_uint16  en_cfg_type)
{
    oal_uint32      ul_loop;

    for (ul_loop = 0; ul_loop < OAM_CFG_TYPE_BUTT; ul_loop++) {
        if (en_cfg_type == g_ast_cfg_data[ul_loop].en_cfg_type) {
            break;
        }
    }

    if (ul_loop == OAM_CFG_TYPE_BUTT) {
        OAL_IO_PRINT("oam_cfg_get_item_by_id::get cfg item failed!\n");
        return -OAL_FAIL;
    }

    return g_ast_cfg_data[ul_loop].l_val;
}

#if ((_PRE_PRODUCT_ID_HI1131C_DEV ==_PRE_PRODUCT_ID) || (_PRE_PRODUCT_ID_HI1131C_HOST ==_PRE_PRODUCT_ID))

oal_uint32  hmac_custom_init(oal_void)
{
    /* Ӳ������:3��STA; 2��AP */
    /* ������:
        1)AP ģʽ:  2��ap + 1������vap
        2)STA ģʽ: 3��sta + 1������vap
        3)STA+P2P����ģʽ:  1��sta + 1��P2P_dev + 1��P2P_GO/Client + 1������vap
        4)STA+Proxy STA����ģʽ:  1��sta + ?��proxy STA + 1������vap
    */
    g_us_wlan_assoc_user_max_num     = WLAN_ASSOC_USER_MAX_NUM_LIMIT;
    g_ul_wlan_vap_max_num_per_device = WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE + WLAN_CONFIG_VAP_MAX_NUM_PER_DEVICE;

    return OAL_SUCC;
}

#else

oal_uint32  hmac_custom_init(oal_void)
{
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    if (g_l_proxysta_feature) {
        g_us_wlan_assoc_user_max_num  = 15;
        g_ul_wlan_vap_max_num_per_device = WLAN_REPEATER_SERVICE_VAP_MAX_NUM_PER_DEVICE +
            WLAN_CONFIG_VAP_MAX_NUM_PER_DEVICE; /* 1��AP, 1��sta��15��Proxy STA��1������vap */
    } else
#endif
    {
        g_us_wlan_assoc_user_max_num  = WLAN_ASSOC_USER_MAX_NUM_LIMIT;
        g_ul_wlan_vap_max_num_per_device = WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE + WLAN_CONFIG_VAP_MAX_NUM_PER_DEVICE;
    }

    return OAL_SUCC;
}
#endif

oal_uint32 hmac_dfx_init(void)
{
    hmac_custom_init();
#ifdef _PRE_WLAN_1131_CHR
    chr_host_callback_register(hmac_get_chr_info_event_hander);
#endif
    return OAL_SUCC;
}

oal_uint32 hmac_dfx_exit(void)
{
#ifdef _PRE_WLAN_1131_CHR
    chr_host_callback_unregister();
#endif
    return OAL_SUCC;
}

#ifdef _PRE_WLAN_1131_CHR
/* *********************ȫ�ֱ���*************************** */
/* ȥ��������7��ԭ��(0~6),Ĭ��ֵ����Ϊ7����ʾû��ȥ�������� */
hmac_chr_disasoc_reason_stru g_hmac_chr_disasoc_reason = { 0, DMAC_DISASOC_MISC_BUTT };

/* ��������, ����4��˽�ж�������5200-5203 */
uint16_t g_hmac_chr_connect_code = 0;

hmac_chr_del_ba_info_stru g_hmac_chr_del_ba_info = { 0, 0, MAC_UNSPEC_REASON };

/* *********************��ȡȫ�ֱ�����ַ*************************** */
hmac_chr_disasoc_reason_stru *hmac_chr_disasoc_reason_get_pointer(void)
{
    return &g_hmac_chr_disasoc_reason;
}

uint16_t *hmac_chr_connect_code_get_pointer(void)
{
    return &g_hmac_chr_connect_code;
}

hmac_chr_del_ba_info_stru *hmac_chr_del_ba_info_get_pointer(void)
{
    return &g_hmac_chr_del_ba_info;
}

/* �ظ�ȫ�������ĳ�ʼֵ */
void hmac_chr_info_clean(void)
{
    g_hmac_chr_disasoc_reason.us_user_id = 0;
    g_hmac_chr_disasoc_reason.en_disasoc_reason = DMAC_DISASOC_MISC_BUTT;
    g_hmac_chr_connect_code = 0;
    g_hmac_chr_del_ba_info.uc_ba_num = 0;
    g_hmac_chr_del_ba_info.uc_del_ba_tid = 0;
    g_hmac_chr_del_ba_info.en_del_ba_reason = MAC_UNSPEC_REASON;

    return;
}
/**********************CHR���ͻ�ȡ****************************/
/* �ֽ׶�CHRֻ����STA״̬(������P2P)�����Բ�����vap_id */
/* ��� */
void hmac_chr_set_disasoc_reason(uint16_t user_id, uint16_t reason_id)
{
    hmac_chr_disasoc_reason_stru *pst_disasoc_reason = OAL_PTR_NULL;

    pst_disasoc_reason = hmac_chr_disasoc_reason_get_pointer();

    pst_disasoc_reason->us_user_id = user_id;
    pst_disasoc_reason->en_disasoc_reason = (dmac_disasoc_misc_reason_enum)reason_id;

    return;
}

/* ��ȡ */
void hmac_chr_get_disasoc_reason(hmac_chr_disasoc_reason_stru *pst_disasoc_reason)
{
    hmac_chr_disasoc_reason_stru *pst_disasoc_reason_temp = OAL_PTR_NULL;

    pst_disasoc_reason_temp = hmac_chr_disasoc_reason_get_pointer();

    pst_disasoc_reason->us_user_id = pst_disasoc_reason_temp->us_user_id;
    pst_disasoc_reason->en_disasoc_reason = pst_disasoc_reason_temp->en_disasoc_reason;

    return;
}

void hmac_chr_set_ba_session_num(uint8_t uc_ba_num)
{
    hmac_chr_del_ba_info_stru *pst_del_ba_info = OAL_PTR_NULL;

    pst_del_ba_info = hmac_chr_del_ba_info_get_pointer();
    pst_del_ba_info->uc_ba_num = uc_ba_num;
    return;
}

/* ��� */
/* ����ɾ���ۺϵ����� ����ͳ�� */
void hmac_chr_set_del_ba_info(uint8_t uc_tid, uint16_t reason_id)
{
    hmac_chr_del_ba_info_stru *pst_del_ba_info = OAL_PTR_NULL;

    pst_del_ba_info = hmac_chr_del_ba_info_get_pointer();

    pst_del_ba_info->uc_del_ba_tid = uc_tid;
    pst_del_ba_info->en_del_ba_reason = (mac_reason_code_enum)reason_id;

    return;
}

/* ��ȡ */
void hmac_chr_get_del_ba_info(mac_vap_stru *pst_mac_vap,
    hmac_chr_del_ba_info_stru *pst_del_ba_reason)
{
    hmac_chr_del_ba_info_stru *pst_del_ba_info = OAL_PTR_NULL;

    pst_del_ba_info = hmac_chr_del_ba_info_get_pointer();

    pst_del_ba_reason->uc_ba_num = pst_del_ba_info->uc_ba_num;
    pst_del_ba_reason->uc_del_ba_tid = pst_del_ba_info->uc_del_ba_tid;
    pst_del_ba_reason->en_del_ba_reason = pst_del_ba_info->en_del_ba_reason;

    return;
}

void hmac_chr_set_connect_code(uint16_t connect_code)
{
    uint16_t *pus_connect_code = OAL_PTR_NULL;

    pus_connect_code = hmac_chr_connect_code_get_pointer();
    *pus_connect_code = connect_code;
    return;
}

void hmac_chr_get_connect_code(uint16_t *pus_connect_code)
{
    pus_connect_code = hmac_chr_connect_code_get_pointer();
    return;
}

void hmac_chr_get_vap_info(mac_vap_stru *pst_mac_vap, hmac_chr_vap_info_stru *pst_vap_info)
{
    mac_user_stru *pst_mac_user = OAL_PTR_NULL;
    mac_device_stru *pst_mac_device;

    pst_mac_device = mac_res_get_dev(0);

    pst_vap_info->uc_vap_state = pst_mac_vap->en_vap_state;
    pst_vap_info->uc_vap_num = pst_mac_device->uc_vap_num;
    pst_vap_info->uc_vap_rx_nss = pst_mac_vap->en_vap_rx_nss;
    pst_vap_info->uc_protocol = pst_mac_vap->en_protocol;

    /* sta ������AP������ */
    pst_mac_user = mac_res_get_mac_user(pst_mac_vap->us_multi_user_idx);
    if (pst_mac_user != OAL_PTR_NULL) {
        pst_vap_info->uc_ap_spatial_stream_num = pst_mac_user->uc_num_spatial_stream;
        pst_vap_info->bit_ap_qos = pst_mac_user->st_cap_info.bit_qos;
        pst_vap_info->uc_ap_protocol_mode = pst_mac_user->en_protocol_mode;
    }

    
    pst_vap_info->bit_ampdu_active = OAL_TRUE;
    pst_vap_info->bit_amsdu_active = OAL_TRUE;
    pst_vap_info->bit_sta_11ntxbf = pst_mac_vap->st_cap_flag.bit_11ntxbf;
    pst_vap_info->bit_is_dbac_running = mac_is_dbac_running(pst_mac_device);

    return;
}

uint32_t hmac_chr_get_chip_info(uint32_t chr_event_id)
{
    uint8_t uc_vap_index;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    mac_device_stru *pst_mac_device = OAL_PTR_NULL;
    hmac_chr_info st_hmac_chr_info;

    pst_mac_device = mac_res_get_dev(0);
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ACS, "{hmac_chr_get_chip_info::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_mac_device->uc_vap_num == 0) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    for (uc_vap_index = 0; uc_vap_index < pst_mac_device->uc_vap_num; uc_vap_index++) {
        pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_index]);
        if (pst_mac_vap == OAL_PTR_NULL) {
            return OAL_ERR_CODE_PTR_NULL;
        }

        if (IS_LEGACY_STA(pst_mac_vap)) {
            memset_s(&st_hmac_chr_info, OAL_SIZEOF(hmac_chr_info), 0, OAL_SIZEOF(hmac_chr_info));

            hmac_chr_get_disasoc_reason(&st_hmac_chr_info.st_disasoc_reason);
            hmac_chr_get_del_ba_info(pst_mac_vap, &st_hmac_chr_info.st_del_ba_info);
            hmac_chr_get_connect_code(&st_hmac_chr_info.us_connect_code);
            hmac_chr_get_vap_info(pst_mac_vap, &st_hmac_chr_info.st_vap_info);

            chr_exception_p(chr_event_id, (uint8_t *)(&st_hmac_chr_info), OAL_SIZEOF(hmac_chr_info));
        }
    }

    /* ���ȫ�ֱ�������ʷֵ */
    hmac_chr_info_clean();

    return OAL_SUCC;
}
uint32_t hmac_get_chr_info_event_hander(uint32_t chr_event_id)
{
    uint32_t ul_ret;

    switch (chr_event_id) {
        case CHR_WIFI_DISCONNECT_QUERY_EVENTID:
        case CHR_WIFI_CONNECT_FAIL_QUERY_EVENTID:
        case CHR_WIFI_WEB_FAIL_QUERY_EVENTID:
        case CHR_WIFI_WEB_SLOW_QUERY_EVENTID:
            ul_ret = hmac_chr_get_chip_info(chr_event_id);
            if (ul_ret != OAL_SUCC) {
                OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_chr_get_chip_info::hmac_chr_get_web_fail_slow_info fail.}");
                return ul_ret;
            }
            break;
        default:
            break;
    }

    return OAL_SUCC;
}

void hmac_chr_connect_fail_query_and_report(hmac_vap_stru *pst_hmac_vap,
    mac_status_code_enum_uint16 connet_code)
{
    mac_chr_connect_fail_report_stru st_chr_connect_fail_report = { 0 };

    if (IS_LEGACY_STA(&pst_hmac_vap->st_vap_base_info)) {
        /* ������ѯ */
        hmac_chr_set_connect_code(connet_code);
        /* �����ϱ� */
#ifdef CONFIG_HW_GET_EXT_SIG
        st_chr_connect_fail_report.ul_noise = pst_hmac_vap->station_info.noise;
        st_chr_connect_fail_report.ul_chload = pst_hmac_vap->station_info.chload;
#endif
        st_chr_connect_fail_report.c_signal = pst_hmac_vap->station_info.signal;
        st_chr_connect_fail_report.us_err_code = connet_code;
        chr_exception_p(CHR_WIFI_CONNECT_FAIL_REPORT_EVENTID, (uint8_t *)(&st_chr_connect_fail_report),
                        OAL_SIZEOF(mac_chr_connect_fail_report_stru));
    }

    return;
}
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
