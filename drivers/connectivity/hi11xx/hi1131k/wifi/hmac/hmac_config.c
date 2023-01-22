


/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "oam_ext_if.h"
#include "oam_trace.h"
#include "frw_ext_if.h"
#include "mac_device.h"
#include "hmac_device.h"
#include "mac_resource.h"
#include "hmac_resource.h"
#include "mac_vap.h"
#include "mac_ie.h"
#include "oal_hcc_host_if.h"
#include "mac_regdomain.h"
#include "dmac_ext_if.h"
#include "hmac_ext_if.h"
#include "hmac_fsm.h"
#include "hmac_main.h"
#include "hmac_vap.h"
#include "hmac_tx_amsdu.h"
#include "hmac_rx_data.h"
#include "hmac_mgmt_classifier.h"
#include "hmac_config.h"
#include "hmac_chan_mgmt.h"
#include "hmac_rx_filter.h"
#include "hmac_psm_ap.h"
#ifdef _PRE_WLAN_CHIP_TEST
#include "hmac_test_main.h"
#include "alg_test_main.h"
#include "oal_schedule.h"
#endif
#include "hmac_protection.h"
#include "hmac_mgmt_bss_comm.h"
#include "hmac_encap_frame_sta.h"
#include "hmac_data_acq.h"
#include "hmac_rx_filter.h"
#include "hmac_mgmt_sta.h"
#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
#include "hmac_custom_security.h"
#endif

#ifdef _PRE_WLAN_FEATURE_MCAST
#include "hmac_m2u.h"
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
#include "hmac_proxy_arp.h"
#endif


#include "hmac_dfs.h"
#include "hmac_reset.h"
#include "hmac_scan.h"
#include "hmac_blockack.h"
#include "hmac_p2p.h"
#include "hmac_mgmt_ap.h"
#include "securec.h"
#include "oal_profiling.h"

#ifdef _PRE_WLAN_RF_110X_CALI_DPD
#include "hmac_cali_dpd.h"
#endif

#ifdef _PRE_WLAN_FEATURE_ROAM
#include "hmac_roam_main.h"
#endif // _PRE_WLAN_FEATURE_ROAM

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
#include "hmac_arp_offload.h"
#endif
#ifdef _PRE_WLAN_TCP_OPT
#include "mac_data.h"
#include "hmac_tcp_opt.h"
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || \
    (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION))
#include "plat_pm_wlan.h"
#endif
#ifdef _PRE_WLAN_DFT_STAT
#include "mac_board.h"
#endif
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
#include  "hmac_auto_adjust_freq.h"
#endif

#include "hmac_dfx.h"
#include "plat_firmware.h"

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#include "hisi_customize_wifi.h"
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */
#include "hmac_sme_sta.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CONFIG_C
#define PUC_MDE_MEMCPY_LENGTH  5

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
extern oal_int32 g_l_proxysta_feature;
#endif
#ifdef _PRE_WLAN_FEATURE_DFR
extern  wal_dfr_info_stru    g_st_dfr_info;
#endif // _PRE_WLAN_FEATURE_DFR
extern oal_uint32 band_5g_enabled;

#ifdef _PRE_WLAN_FEATURE_HILINK
oal_uint8 g_uc_data_switch       = OAL_FALSE;
oal_uint8 g_uc_management_switch = OAL_FALSE;
#endif
extern oal_bool_enum_uint8 g_ht_mcs_set_check;

/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/
#if ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)) && \
    defined(_PRE_WLAN_CHIP_TEST_ALG)
struct kobject     *g_alg_test_sys_kobject = OAL_PTR_NULL;
#endif
hmac_reg_info_receive_event     g_st_hmac_reg_info_receive_event = {0};

typedef struct {
    wlan_protocol_enum_uint8   en_protocol_mode;                /* widö�� */
    oal_uint8                  auc_resv[3];
    oal_int8                  *puc_protocol_desc;
}hmac_protocol_stru;

OAL_STATIC hmac_protocol_stru gst_protocol_mode_list[WLAN_PROTOCOL_BUTT] = {
    {WLAN_LEGACY_11A_MODE,      {0}, "11a" },
    {WLAN_LEGACY_11B_MODE,      {0}, "11b" },
    {WLAN_LEGACY_11G_MODE,      {0}, "abandon_mode" },
    {WLAN_MIXED_ONE_11G_MODE,   {0}, "11bg"},
    {WLAN_MIXED_TWO_11G_MODE,   {0}, "11g"},
    {WLAN_HT_MODE,              {0}, "11n" },
    {WLAN_VHT_MODE,             {0}, "11ac"},
    {WLAN_HT_ONLY_MODE,         {0}, "11n_only"},
    {WLAN_VHT_ONLY_MODE,        {0}, "11ac_only"},
};
#ifdef _PRE_WLAN_CHIP_TEST_ALG
#define HMAC_ALG_TEST_BUF_SIZE  128
typedef struct {
    oal_wait_queue_head_stru        st_wait_queue; /* �̵߳ȴ��ṹ��,����WAL_Linux���̵߳ȴ�(WAL_Linuc -> WAL_Config) */
    OAL_VOLATILE   oal_uint8        auc_data[HMAC_ALG_TEST_BUF_SIZE];
}alg_test_main_hmac_stru;
alg_test_main_hmac_stru g_st_alg_test_hmac;


#if ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)||(_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)) && \
    defined(_PRE_WLAN_CHIP_TEST_ALG)
OAL_STATIC oal_ssize_t  hmac_alg_test_result_proc_read(struct kobject *dev, struct kobj_attribute *attr, char *buf);

OAL_STATIC struct kobj_attribute dev_attr_alg_test_result =
    __ATTR(alg_test_result, OAL_S_IRUGO | OAL_S_IWUSR, hmac_alg_test_result_proc_read, OAL_PTR_NULL);


#endif


#endif

oal_uint32  hmac_config_set_freq(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
oal_uint32  hmac_config_set_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_SAE
OAL_STATIC void hmac_update_sae_connect_param(hmac_vap_stru *hmac_vap, uint8_t *ie_data, uint32_t ie_len);
#endif
/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/
oal_int8* hmac_config_index2string(oal_uint32 ul_index, oal_int8* pst_string[], oal_uint32 ul_max_str_nums)
{
    if (OAL_WARN_ON(ul_index >= ul_max_str_nums)) {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_index2string::wrong index nums:%u, max:%u}", ul_index,
            ul_max_str_nums);
        return (oal_int8*)"unkown";
    }
    return pst_string[ul_index];
}

oal_int8* hmac_config_protocol2string(oal_uint32 ul_protocol)
{
    oal_int8 *pac_protocol2string[] = { "11a", "11b", "error", "11bg", "11g", "11n", "11ac", "11nonly", "11aconly",
        "11ng", "error"};
    return hmac_config_index2string(ul_protocol, pac_protocol2string,
        OAL_SIZEOF(pac_protocol2string)/OAL_SIZEOF(oal_int8 *));
}

oal_int8* hmac_config_band2string(oal_uint32 ul_band)
{
    oal_int8 *pac_band2string[]     = {"2.4G", "5G", "error"};
    return hmac_config_index2string(ul_band, pac_band2string, OAL_SIZEOF(pac_band2string)/OAL_SIZEOF(oal_int8 *));
}

oal_int8* hmac_config_bw2string(oal_uint32 ul_bw)
{
    oal_int8 *pac_bw2string[]       = {"20M", "40+", "40-", "80++", "80+-", "80-+", "80--", "error"};
    return hmac_config_index2string(ul_bw, pac_bw2string, OAL_SIZEOF(pac_bw2string)/OAL_SIZEOF(oal_int8 *));
}

oal_int8* hmac_config_ciper2string(oal_uint32 ul_ciper2)
{
    oal_int8 *pac_ciper2string[]    = {"GROUP", "WEP40", "TKIP", "RSV", "CCMP", "WEP104", "BIP", "NONE"};
    return hmac_config_index2string(ul_ciper2, pac_ciper2string, OAL_SIZEOF(pac_ciper2string)/OAL_SIZEOF(oal_int8 *));
}

oal_int8* hmac_config_akm2string(oal_uint32 ul_akm2)
{
    oal_int8 *pac_akm2string[]      = {"RSV", "1X", "PSK", "FT_1X", "FT_PSK", "1X_SHA256", "PSK_SHA256", "NONE"};
    return hmac_config_index2string(ul_akm2, pac_akm2string, OAL_SIZEOF(pac_akm2string)/OAL_SIZEOF(oal_int8 *));
}

oal_int8* hmac_config_keytype2string(oal_uint32 ul_keytype)
{
    oal_int8 *pac_keytype2string[] = {"GTK", "PTK", "RX_GTK", "ERR"};
    return hmac_config_index2string(ul_keytype, pac_keytype2string,
        OAL_SIZEOF(pac_keytype2string)/OAL_SIZEOF(oal_int8 *));
}

oal_int8* hmac_config_cipher2string(oal_uint32 ul_cipher)
{
    oal_int8 *pac_cipher2string[] = {"GROUP", "WEP40", "TKIP", "NO_ENCRYP", "CCMP", "WEP104", "BIP", "GROUP_DENYD"};
    return hmac_config_index2string(ul_cipher, pac_cipher2string, OAL_SIZEOF(pac_cipher2string)/OAL_SIZEOF(oal_int8 *));
}

oal_int8* hmac_config_smps2string(oal_uint32 ul_smps)
{
    oal_int8 *pac_smps2string[] = {"Static", "Dynamic", "MIMO", "error"};
    return hmac_config_index2string(ul_smps, pac_smps2string, OAL_SIZEOF(pac_smps2string)/OAL_SIZEOF(oal_int8 *));
}

oal_int8* hmac_config_dev2string(oal_uint32 ul_dev)
{
    oal_int8 *pac_dev2string[]  = {"Close", "Open", "error"};
    return hmac_config_index2string(ul_dev, pac_dev2string, OAL_SIZEOF(pac_dev2string)/OAL_SIZEOF(oal_int8 *));
}

oal_int8* hmac_config_nss2string(oal_uint32 ul_nss)
{
    oal_int8 *pac_nss2string[] = {"Single Nss", "Double Nss", "error"};
    return hmac_config_index2string(ul_nss, pac_nss2string, OAL_SIZEOF(pac_nss2string)/OAL_SIZEOF(oal_int8 *));
}

oal_int8* hmac_config_b_w2string(oal_uint32 ul_b_w)
{
    oal_int8 *pac_bw2string[]  = {"20M", "40M", "80M", "error"};
    return hmac_config_index2string(ul_b_w, pac_bw2string, OAL_SIZEOF(pac_bw2string)/OAL_SIZEOF(oal_int8 *));
}


OAL_STATIC oal_uint32  hmac_config_alloc_event(
    mac_vap_stru                     *pst_mac_vap,
    hmac_to_dmac_syn_type_enum_uint8  en_syn_type,
    hmac_to_dmac_cfg_msg_stru       **ppst_syn_msg,
    frw_event_mem_stru              **ppst_event_mem,
    oal_uint16                        us_len)
{
    frw_event_mem_stru *pst_event_mem;
    frw_event_stru     *pst_event = OAL_PTR_NULL;

    pst_event_mem = FRW_EVENT_ALLOC(us_len + OAL_SIZEOF(hmac_to_dmac_cfg_msg_stru) - 4);
    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_alloc_event::pst_event_mem null,us_len = %d }",
            us_len);
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    /* ����¼�ͷ */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_CRX,
                       en_syn_type,
                       (us_len + OAL_SIZEOF(hmac_to_dmac_cfg_msg_stru) - 4),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_mac_vap->uc_chip_id,
                       pst_mac_vap->uc_device_id,
                       pst_mac_vap->uc_vap_id);

    /* ���θ�ֵ */
    *ppst_event_mem = pst_event_mem;
    *ppst_syn_msg   = (hmac_to_dmac_cfg_msg_stru *)pst_event->auc_event_data;

    return OAL_SUCC;
}


oal_uint32  hmac_config_send_event(mac_vap_stru           *pst_mac_vap,
                                   wlan_cfgid_enum_uint16 en_cfg_id,
                                   oal_uint16             us_len,
                                   const oal_uint8        *puc_param)
{
    oal_uint32                  ul_ret;
    frw_event_mem_stru         *pst_event_mem = OAL_PTR_NULL;
    hmac_to_dmac_cfg_msg_stru  *pst_syn_msg = OAL_PTR_NULL;

    ul_ret = hmac_config_alloc_event(pst_mac_vap, HMAC_TO_DMAC_SYN_CFG, &pst_syn_msg, &pst_event_mem, us_len);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_send_event::hmac_config_alloc_event failed[%d].}", ul_ret);
        return ul_ret;
    }

    HMAC_INIT_SYN_MSG_HDR(pst_syn_msg, en_cfg_id, us_len);

    /* ��д����ͬ����Ϣ���� */
    if ((puc_param != OAL_PTR_NULL) && (us_len)) {
        if (memcpy_s(pst_syn_msg->auc_msg_body, (oal_uint32)us_len, puc_param, (oal_uint32)us_len) != EOK) {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_send_event::memcpy_s fail");
            FRW_EVENT_FREE(pst_event_mem);
            return OAL_FAIL;
        }
    }

    /* �׳��¼� */
    ul_ret = frw_event_dispatch_event(pst_event_mem);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_send_event::frw_event_dispatch_event failed[%d].}", ul_ret);
        FRW_EVENT_FREE(pst_event_mem);
        return ul_ret;
    }

    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_config_alg_send_event(mac_vap_stru           *pst_mac_vap,
                                                  wlan_cfgid_enum_uint16 en_cfg_id,
                                                  oal_uint16             us_len,
                                                  const oal_uint8        *puc_param)
{
    oal_uint32                  ul_ret;
    frw_event_mem_stru         *pst_event_mem = OAL_PTR_NULL;
    hmac_to_dmac_cfg_msg_stru  *pst_syn_msg = OAL_PTR_NULL;

    ul_ret = hmac_config_alloc_event(pst_mac_vap, HMAC_TO_DMAC_SYN_ALG, &pst_syn_msg, &pst_event_mem, us_len);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_alg_send_event::hmac_config_alloc_event failed[%d].}", ul_ret);
        return ul_ret;
    }

    HMAC_INIT_SYN_MSG_HDR(pst_syn_msg, en_cfg_id, us_len);

    /* ��д����ͬ����Ϣ���� */
    if (memcpy_s(pst_syn_msg->auc_msg_body, us_len, puc_param, us_len) != EOK) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_alg_send_event::memcpy_s fail");
        FRW_EVENT_FREE(pst_event_mem);
        return OAL_FAIL;
    }

    /* �׳��¼� */
    frw_event_dispatch_event(pst_event_mem);

    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}


oal_uint32 hmac_config_start_vap_event(mac_vap_stru  *pst_mac_vap, oal_bool_enum_uint8 en_mgmt_rate_init_flag)
{
    oal_uint32                    ul_ret;
    mac_cfg_start_vap_param_stru  st_start_vap_param;

    /* DMAC��ʹ��netdev��Ա */
    st_start_vap_param.pst_net_dev = OAL_PTR_NULL;
    st_start_vap_param.en_mgmt_rate_init_flag = en_mgmt_rate_init_flag;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    st_start_vap_param.uc_protocol = pst_mac_vap->en_protocol;
    st_start_vap_param.uc_band     = pst_mac_vap->st_channel.en_band;
    st_start_vap_param.uc_bandwidth = pst_mac_vap->st_channel.en_bandwidth;
#endif
#ifdef _PRE_WLAN_FEATURE_P2P
    st_start_vap_param.en_p2p_mode = pst_mac_vap->en_p2p_mode;
#endif

    ul_ret = hmac_config_send_event(pst_mac_vap,
                                    WLAN_CFGID_START_VAP,
                                    OAL_SIZEOF(mac_cfg_start_vap_param_stru),
                                    (oal_uint8 *)&st_start_vap_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_start_vap_event::Start_vap failed[%d].}", ul_ret);
    }
    return ul_ret;
}


oal_uint32 hmac_set_mode_event(mac_vap_stru *pst_mac_vap)
{
    oal_uint32               ul_ret;
    mac_cfg_mode_param_stru  st_prot_param;

    /* ���ô���ģʽ��ֱ�����¼���DMAC���üĴ��� */
    st_prot_param.en_protocol  = pst_mac_vap->en_protocol;
    st_prot_param.en_band      = pst_mac_vap->st_channel.en_band;
    st_prot_param.en_bandwidth = pst_mac_vap->st_channel.en_bandwidth;

    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_MODE, OAL_SIZEOF(mac_cfg_mode_param_stru),
        (oal_uint8 *)&st_prot_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_set_mode_event::mode_set failed[%d],protocol[%d],"
            "band[%d], bandwidth[%d].}", ul_ret, pst_mac_vap->en_protocol, pst_mac_vap->st_channel.en_band,
            pst_mac_vap->st_channel.en_bandwidth);
    }
    return ul_ret;
}


oal_uint32 hmac_config_update_opmode_event(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user,
    oal_uint8 uc_mgmt_frm_type)
{
    oal_uint32              ul_relt;
    mac_user_opmode_stru    st_user_opmode;
    /* opmodeϢͬ��dmac */
    st_user_opmode.uc_avail_num_spatial_stream    = pst_mac_user->uc_avail_num_spatial_stream;
    st_user_opmode.uc_avail_bf_num_spatial_stream = pst_mac_user->uc_avail_bf_num_spatial_stream;
    st_user_opmode.en_avail_bandwidth = pst_mac_user->en_avail_bandwidth;
    st_user_opmode.en_cur_bandwidth   = pst_mac_user->en_cur_bandwidth;
    st_user_opmode.us_user_idx        = pst_mac_user->us_assoc_id;
    st_user_opmode.uc_frame_type      = uc_mgmt_frm_type;

    ul_relt = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_UPDATE_OPMODE,
                                     OAL_SIZEOF(mac_user_opmode_stru),
                                     (oal_uint8 *)(&st_user_opmode));
    if (OAL_UNLIKELY(ul_relt != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_user->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_update_opmode_event::opmode_event send failed[%d].}", ul_relt);
    }

    return ul_relt;
}


oal_uint32  hmac_config_sync_cmd_common(mac_vap_stru *pst_mac_vap, wlan_cfgid_enum_uint16 en_cfg_id, oal_uint16 us_len,
    const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, en_cfg_id, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_sync_cmd_common::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL

oal_uint32  hmac_config_get_hipkt_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_GET_HIPKT_STAT, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_alg::hmac_config_send_event failed[%d].}",
            ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_flowctl_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_flowctl_param_stru  *pst_flowctl_param  = (mac_cfg_flowctl_param_stru *)puc_param;

    /* ����host flowctl ��ز��� */
    hcc_host_set_flowctl_param(pst_flowctl_param->uc_queue_type, pst_flowctl_param->us_burst_limit,
                               pst_flowctl_param->us_low_waterline, pst_flowctl_param->us_high_waterline);

    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "hcc_host_set_flowctl_param, queue[%d]: burst limit = %d,"
        "low_waterline = %d, high_waterline =%d", pst_flowctl_param->uc_queue_type, pst_flowctl_param->us_burst_limit,
        pst_flowctl_param->us_low_waterline, pst_flowctl_param->us_high_waterline);

    return OAL_SUCC;
}


oal_uint32  hmac_config_get_flowctl_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    /* ����host flowctl ��ز��� */
    hcc_host_get_flowctl_stat();

    return OAL_SUCC;
}

#endif


OAL_STATIC OAL_INLINE oal_uint32 hmac_normal_check_legacy_vap_num(mac_device_stru *pst_mac_device,
    wlan_vap_mode_enum_uint8 en_vap_mode)
{
    /* VAP�����ж� */
    if (en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        if ((pst_mac_device->uc_sta_num == 1) && (pst_mac_device->uc_vap_num == WLAN_AP_STA_COEXIST_VAP_NUM)) {
            /* AP STA���泡����ֻ�ܴ���4��AP + 1��STA */
            OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_normal_check_legacy_vap_num::have created 1AP + 1STA,"
                "cannot create another AP.}");
            return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
        }

        if ((pst_mac_device->uc_vap_num - pst_mac_device->uc_sta_num) >= WLAN_SERVICE_AP_MAX_NUM_PER_DEVICE) {
            /* �Ѵ�����AP�����ﵽ���ֵ4 */
            OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_normal_check_legacy_vap_num::ap num exceeds the supported spec,"
                "vap_num[%u], sta_num[%u].}", pst_mac_device->uc_vap_num, pst_mac_device->uc_sta_num);
            return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
        }
    } else if (en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        if (pst_mac_device->uc_sta_num >= WLAN_SERVICE_STA_MAX_NUM_PER_DEVICE) {
            /* �Ѵ�����STA�����ﵽ���ֵ */
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_normal_check_legacy_vap_num::have created 2+ AP.can not create STA"
                "any more[%d].}", pst_mac_device->uc_sta_num);
            return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
        }
    }

    return OAL_SUCC;
}


#if (_PRE_OS_VERSION_WIN32 != _PRE_OS_VERSION)
OAL_STATIC OAL_INLINE oal_uint32  hmac_config_normal_check_vap_num(mac_device_stru *pst_mac_device,
    mac_cfg_add_vap_param_stru *pst_param)
#else
uint32_t hmac_config_normal_check_vap_num(mac_device_stru *pst_mac_device,
    mac_cfg_add_vap_param_stru *pst_param)
#endif
{
    wlan_vap_mode_enum_uint8   en_vap_mode;
#ifdef _PRE_WLAN_FEATURE_P2P
    wlan_p2p_mode_enum_uint8   en_p2p_mode;

    en_p2p_mode = pst_param->en_p2p_mode;
    if (en_p2p_mode != WLAN_LEGACY_VAP_MODE) {
        return hmac_check_p2p_vap_num(pst_mac_device, en_p2p_mode);
    }
#endif

    en_vap_mode = pst_param->en_vap_mode;
    return hmac_normal_check_legacy_vap_num(pst_mac_device, en_vap_mode);
}

#ifdef _PRE_WLAN_FEATURE_PROXYSTA


OAL_STATIC OAL_INLINE  oal_uint32 hmac_config_proxysta_check_vap_num(mac_device_stru          *pst_mac_device,
                                                                     wlan_vap_mode_enum_uint8   en_vap_mode,
                                                                     oal_bool_enum_uint8      en_is_proxysta,
                                                                     oal_bool_enum_uint8     en_is_main_proxysta)
{
    /* VAP�����ж� */
    if (en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        if (((pst_mac_device->uc_sta_num - pst_mac_device->uc_proxysta_num) == 1) &&
            ((pst_mac_device->uc_vap_num - pst_mac_device->uc_proxysta_num) == WLAN_AP_STA_COEXIST_VAP_NUM)) {
            /* AP STA���泡����ֻ�ܴ���1��AP + 1��STA */
            OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_config_check_vap_num::have created 4AP + 1STA,"
                "cannot create another AP!}");
            return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
        }

        if ((pst_mac_device->uc_vap_num - pst_mac_device->uc_sta_num) >= WLAN_SERVICE_AP_MAX_NUM_PER_DEVICE) {
            /* �Ѵ�����AP�����ﵽ���ֵ4 */
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_check_vap_num::ap num[%d] exceeds the supported spec.}",
                (pst_mac_device->uc_vap_num - pst_mac_device->uc_sta_num));
            return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
        }
    } else if (en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        if (pst_mac_device->uc_proxysta_num >= WLAN_MAX_PROXY_STA_NUM) {
            /* �Ѵ�����ProxySTA�����ﵽ���ֵ */
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_check_vap_num::sta num[%d] exceeds the supported spec.}",
                pst_mac_device->uc_sta_num);
            return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
        }

        if ((en_is_proxysta == OAL_TRUE) && (en_is_main_proxysta == OAL_TRUE)) {
            if ((pst_mac_device->uc_sta_num - pst_mac_device->uc_proxysta_num) >= WLAN_SERVICE_STA_MAX_NUM_PER_DEVICE) {
                /* �Ѵ�����STA(��ProxySTA)�����ﵽ���ֵ */
                OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_check_vap_num::sta num[%d] exceeds the supported spec.}",
                    (pst_mac_device->uc_sta_num - pst_mac_device->uc_proxysta_num));
                return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
            }
        }
    }

    return OAL_SUCC;
}

#endif


OAL_STATIC oal_uint32  hmac_config_check_vap_num(mac_device_stru *pst_mac_device, mac_cfg_add_vap_param_stru *pst_param)
{
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    if (pst_mac_device->st_cap_flag.bit_proxysta == 1) {
        return hmac_config_proxysta_check_vap_num(pst_mac_device, pst_param->en_vap_mode, pst_param->en_is_proxysta,
            pst_param->en_is_main_proxysta);
    } else
#endif
    {
        return hmac_config_normal_check_vap_num(pst_mac_device, pst_param);
    }
}
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

oal_uint32 hmac_cfg_vap_send_event(mac_device_stru *pst_device)
{
    frw_event_mem_stru   *pst_event_mem;
    frw_event_stru       *pst_event = OAL_PTR_NULL;
    oal_uint32            ul_ret;

    /* ���¼���DMAC,��DMAC�������VAP���� */
    pst_event_mem = FRW_EVENT_ALLOC(0);
    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_cfg_vap_send_event::pst_event_mem null.}");
        return OAL_PTR_NULL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    /* ��д�¼�ͷ */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_CRX,
                       HMAC_TO_DMAC_SYN_CREATE_CFG_VAP,
                       0,
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_device->uc_chip_id,
                       pst_device->uc_device_id,
                       pst_device->uc_cfg_vap_id);

    ul_ret = frw_event_dispatch_event(pst_event_mem);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_cfg_vap_send_event::frw_event_dispatch_event failed[%d].}", ul_ret);
    }

    /* �ͷ��¼� */
    FRW_EVENT_FREE(pst_event_mem);

    return ul_ret;
}
#endif


oal_uint32  hmac_config_add_vap(mac_vap_stru *pst_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_add_vap_param_stru    *pst_param = OAL_PTR_NULL;
    hmac_vap_stru                 *pst_hmac_vap = OAL_PTR_NULL;
    oal_uint32                     ul_ret;
    mac_device_stru               *pst_dev = OAL_PTR_NULL;
    oal_uint8                      uc_vap_id = 0;

    if (OAL_UNLIKELY((pst_vap == OAL_PTR_NULL) || (puc_param == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_add_vap::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_param      = (mac_cfg_add_vap_param_stru *)puc_param;

    pst_dev = mac_res_get_dev(pst_vap->uc_device_id);

#ifdef _PRE_WLAN_FEATURE_P2P
    if (pst_param->en_p2p_mode == WLAN_P2P_CL_MODE) {
        return hmac_add_p2p_cl_vap(pst_vap, us_len, puc_param);
    }
#endif

    if (OAL_UNLIKELY(pst_dev == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_add_vap::pst_dev null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* VAP�����ж� */
    ul_ret = hmac_config_check_vap_num(pst_dev, pst_param);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_add_vap::hmac_config_check_vap_num failed[%d].}",
            ul_ret);
        return ul_ret;
    }

    if (OAL_NET_DEV_PRIV(pst_param->pst_net_dev) != OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_add_vap::vap created.}");
        return OAL_SUCC;
    }

    /* ����Դ������hmac vap */
    /*lint -e413*/
    ul_ret = mac_res_alloc_hmac_vap(&uc_vap_id, OAL_OFFSET_OF(hmac_vap_stru, st_vap_base_info));//lint !e78
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_add_vap::mac_res_alloc_hmac_vap failed[%d].}",
            ul_ret);
        return ul_ret;
    }
    /*lint +e413*/
    /* ����Դ�ػ�ȡ�����뵽��hmac vap */
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_add_vap::pst_hmac_vap null.vap_id %d}", uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_param->uc_vap_id = uc_vap_id;

    /* ��ʼ��0 */
    memset_s(pst_hmac_vap, OAL_SIZEOF(hmac_vap_stru), 0, OAL_SIZEOF(hmac_vap_stru));

    /* ��ʼ��HMAC VAP */
    ul_ret = hmac_vap_init(pst_hmac_vap, pst_dev->uc_chip_id, pst_dev->uc_device_id, uc_vap_id, pst_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_add_vap::hmac_vap_init failed[%d].}", ul_ret);
        if (pst_hmac_vap->st_vap_base_info.pst_mib_info != OAL_PTR_NULL) {
            OAL_MEM_FREE(pst_hmac_vap->st_vap_base_info.pst_mib_info, OAL_TRUE);
        }

        /* �쳣�����ͷ��ڴ� */
        mac_res_free_mac_vap(uc_vap_id);
        return ul_ret;
    }

    /* ���÷��ҵ�net_deviceָ�� */
#ifdef _PRE_WLAN_FEATURE_P2P
    if (pst_param->en_p2p_mode == WLAN_P2P_DEV_MODE) {
        /* p2p0 DEV ģʽvap������pst_p2p0_net_device ��Աָ���Ӧ��net_device */
        pst_hmac_vap->pst_p2p0_net_device = pst_param->pst_net_dev;
        pst_dev->st_p2p_info.uc_p2p0_vap_idx = pst_hmac_vap->st_vap_base_info.uc_vap_id;
    }
#endif
    pst_hmac_vap->pst_net_device = pst_param->pst_net_dev;

    /* ����'\0' */
    if (memcpy_s(pst_hmac_vap->auc_name, OAL_IF_NAME_SIZE, pst_param->pst_net_dev->name, OAL_IF_NAME_SIZE) != EOK) {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_add_vap::memcpy_s fail!");
        OAL_MEM_FREE(pst_hmac_vap->st_vap_base_info.pst_mib_info, OAL_TRUE);
        mac_res_free_mac_vap(uc_vap_id);
        return OAL_FAIL;
    }

    /* �����뵽��mac_vap�ռ�ҵ�net_device ml_privָ����ȥ */
    OAL_NET_DEV_PRIV(pst_param->pst_net_dev) = &pst_hmac_vap->st_vap_base_info;

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    /* ����proxy sta�ı�־λ */
    if (pst_dev->st_cap_flag.bit_proxysta == OAL_TRUE) {
        pst_hmac_vap->st_vap_base_info.st_vap_proxysta.en_is_proxysta       = pst_param->en_is_proxysta;
        pst_hmac_vap->st_vap_base_info.st_vap_proxysta.en_is_main_proxysta  = pst_param->en_is_main_proxysta;

        if ((pst_hmac_vap->st_vap_base_info.st_vap_proxysta.en_is_proxysta == OAL_TRUE) &&
            (pst_hmac_vap->st_vap_base_info.st_vap_proxysta.en_is_main_proxysta == OAL_FALSE)) {
            /* Proxysta �������鲥�û� */
            OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CFG, "{hmac_config_add_vap::"
                "proxysta don't add multicast user.}");
        } else {
            /* ����hmac�鲥�û� */
            hmac_user_add_multi_user(&(pst_hmac_vap->st_vap_base_info), &pst_param->us_muti_user_id);
            mac_vap_set_multi_user_idx(&(pst_hmac_vap->st_vap_base_info), pst_param->us_muti_user_id);
        }
    } else {
        /* ����hmac�鲥�û� */
        hmac_user_add_multi_user(&(pst_hmac_vap->st_vap_base_info), &pst_param->us_muti_user_id);
        mac_vap_set_multi_user_idx(&(pst_hmac_vap->st_vap_base_info), pst_param->us_muti_user_id);
    }
#else
    /* ����hmac�鲥�û� */
    hmac_user_add_multi_user(&(pst_hmac_vap->st_vap_base_info), &pst_param->us_muti_user_id);
    mac_vap_set_multi_user_idx(&(pst_hmac_vap->st_vap_base_info), pst_param->us_muti_user_id);
#endif
    mac_device_set_vap_id(pst_dev, &(pst_hmac_vap->st_vap_base_info), uc_vap_id, pst_param->en_vap_mode,
        pst_param->en_p2p_mode, OAL_TRUE);

    switch (pst_param->en_vap_mode) {
        case WLAN_VAP_MODE_BSS_AP:
#ifdef _PRE_WLAN_FEATURE_UAPSD
            pst_param->bit_uapsd_enable = pst_hmac_vap->st_vap_base_info.st_cap_flag.bit_uapsd;
#endif
            break;
        case WLAN_VAP_MODE_BSS_STA:

            break;

        case WLAN_VAP_MODE_WDS:

            break;

        default:
            return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(&pst_hmac_vap->st_vap_base_info,
                                    WLAN_CFGID_ADD_VAP,
                                    us_len,
                                    puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        /* �˴�����������Ҫ��Ӧmac_device_set_vap_id�������˲��� */
        mac_device_set_vap_id(pst_dev, &(pst_hmac_vap->st_vap_base_info), uc_vap_id, pst_param->en_vap_mode,
            pst_param->en_p2p_mode, OAL_FALSE);

        hmac_user_del_multi_user(&(pst_hmac_vap->st_vap_base_info));

        /* �쳣�����ͷ��ڴ� */
        OAL_MEM_FREE(pst_hmac_vap->st_vap_base_info.pst_mib_info, OAL_TRUE);

        mac_res_free_mac_vap(uc_vap_id);

        OAL_NET_DEV_PRIV(pst_param->pst_net_dev) = OAL_PTR_NULL;

        OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_add_vap::hmac_config_alloc_event failed[%d].}",
            ul_ret);
        return ul_ret;
    }

    if (IS_P2P_GO(&pst_hmac_vap->st_vap_base_info)) {
        hmac_config_set_max_user(&pst_hmac_vap->st_vap_base_info, 0, WLAN_P2P_GO_ASSOC_USER_MAX_NUM_SPEC);
    }

    /* ����֡���� */
    hmac_set_rx_filter_value(&pst_hmac_vap->st_vap_base_info);

    OAM_WARNING_LOG4(uc_vap_id, OAM_SF_ANY, "{hmac_config_add_vap::SUCCESS!!vap_mode[%d], p2p_mode[%d]},"
        "multi user idx[%d] idx[%d]", pst_param->en_vap_mode, pst_param->en_p2p_mode, pst_vap->us_multi_user_idx,
        pst_param->us_muti_user_id);

    return OAL_SUCC;
}


oal_uint32  hmac_config_del_vap(mac_vap_stru *pst_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    hmac_vap_stru                 *pst_hmac_vap = OAL_PTR_NULL;
    oal_uint32                     ul_ret;
    mac_device_stru               *pst_device = OAL_PTR_NULL;
    mac_cfg_del_vap_param_stru    *pst_del_vap_param = OAL_PTR_NULL;

    if (OAL_UNLIKELY((pst_vap == OAL_PTR_NULL) || (puc_param == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_del_vap::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_del_vap_param = (mac_cfg_del_vap_param_stru *)puc_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_del_vap::mac_res_get_hmac_vap failed.}");
        return OAL_FAIL;
    }

#if defined(_PRE_WLAN_FEATURE_BTCOEX) || defined(_PRE_WLAN_FEATURE_1131K_BTCOEX)
    /* ����arp̽��timer */
    if (pst_hmac_vap->hmac_vap_btcoex.hmac_btcoex_arp_req_process.delba_opt_timer.en_is_registerd == OAL_TRUE) {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_vap->hmac_vap_btcoex.hmac_btcoex_arp_req_process.delba_opt_timer));
    }
#endif
#ifdef _PRE_WLAN_FEATURE_SAE
    oal_cancel_work_sync(&(pst_hmac_vap->sae_report_ext_auth_worker));
#endif
#ifdef _PRE_WLAN_FEATURE_P2P
    if (pst_vap->en_p2p_mode == WLAN_P2P_CL_MODE) {
        return hmac_del_p2p_cl_vap(pst_vap, us_len, puc_param);
    }
#endif
#ifdef _PRE_WLAN_FEATURE_DFR
    /* can't return when dfr process! */
    if ((pst_vap->en_vap_state != MAC_VAP_STATE_INIT) && (g_st_dfr_info.bit_device_reset_process_flag != OAL_TRUE))
#else
    if (pst_vap->en_vap_state != MAC_VAP_STATE_INIT)
#endif
    {
        OAM_WARNING_LOG2(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_del_vap::deleting vap failed."
            "vap state is not INIT, en_vap_state=%d,en_vap_mode=%d}", pst_vap->en_vap_state, pst_vap->en_vap_mode);
        return OAL_FAIL;
    }

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
    if (pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        pst_hmac_vap->uc_edca_opt_flag_ap   = 0;
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_vap->st_edca_opt_timer));
    } else if (pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        pst_hmac_vap->uc_edca_opt_flag_sta = 0;
    }
#endif

    /* ���������VAP, ȥע������vap��Ӧ��net_device, �ͷţ����� */
    if (pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_CONFIG) {
        /* ��ע��netdevice֮ǰ�Ƚ�ָ�븳Ϊ�� */
        oal_net_device_stru   *pst_net_device = pst_hmac_vap->pst_net_device;
        pst_hmac_vap->pst_net_device = OAL_PTR_NULL;
        OAL_SMP_MB();
        oal_net_unregister_netdev(pst_net_device);

        mac_res_free_mac_vap(pst_hmac_vap->st_vap_base_info.uc_vap_id);
        return  OAL_SUCC;
    }

    /* ҵ��vap net_device����WAL�ͷţ��˴���Ϊnull */
#ifdef _PRE_WLAN_FEATURE_P2P
    if (pst_del_vap_param->en_p2p_mode == WLAN_P2P_DEV_MODE) {
        /* ���p2p0,��Ҫɾ��hmac �ж�Ӧ��p2p0 netdevice ָ�� */
        pst_hmac_vap->pst_p2p0_net_device = OAL_PTR_NULL;
    }
#endif
    pst_hmac_vap->pst_net_device = OAL_PTR_NULL;

    /* �鲥ת������detach */
#ifdef _PRE_WLAN_FEATURE_MCAST
    hmac_m2u_detach(pst_hmac_vap);
#endif

    if (pst_hmac_vap->puc_asoc_req_ie_buff != OAL_PTR_NULL) {
        OAL_MEM_FREE(pst_hmac_vap->puc_asoc_req_ie_buff, OAL_TRUE);
        pst_hmac_vap->puc_asoc_req_ie_buff = OAL_PTR_NULL;
    }

    /* ҵ��vap��ɾ������device��ȥ�� */
    pst_device     = mac_res_get_dev(pst_vap->uc_device_id);
    if (pst_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_del_vap::mac_res_get_dev failed.}");
        return OAL_FAIL;
    }

    /* �������е�timer */
    if (pst_hmac_vap->st_mgmt_timer.en_is_registerd == OAL_TRUE) {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_vap->st_mgmt_timer));
        pst_hmac_vap->st_mgmt_timer.en_is_registerd = OAL_FALSE;
    }
    if (pst_hmac_vap->st_scan_timeout.en_is_registerd == OAL_TRUE) {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_vap->st_scan_timeout));
        pst_hmac_vap->st_scan_timeout.en_is_registerd = OAL_FALSE;
    }
    if (pst_hmac_vap->st_40M_recovery_timer.en_is_registerd == OAL_TRUE) {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_vap->st_40M_recovery_timer));
        pst_hmac_vap->st_40M_recovery_timer.en_is_registerd = OAL_FALSE;
    }
#ifdef _PRE_WLAN_FEATURE_STA_PM
    if (pst_hmac_vap->st_ps_sw_timer.en_is_registerd == OAL_TRUE) {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_vap->st_ps_sw_timer));
        pst_hmac_vap->st_ps_sw_timer.en_is_registerd = OAL_FALSE;
    }
#endif
    /* ɾ��vapʱɾ��TCP ACK�Ķ��� */
#ifdef _PRE_WLAN_TCP_OPT
    hmac_tcp_opt_deinit_list(pst_hmac_vap);
#endif
    /* �ͷ��鲥�û� */
    hmac_user_del_multi_user(pst_vap);

    mac_vap_exit(&(pst_hmac_vap->st_vap_base_info));

    /* ���1��vapɾ��ʱ�����device��������Ϣ */
    if (pst_device->uc_vap_num == 0) {
        pst_device->en_40MHz_intol_bit_recd = OAL_FALSE;
    }

    /* ����ԭ����Ϊ���䣬���������ֲ𡣲��ΪHmac�ı��� */
#ifdef _PRE_WLAN_FEATURE_P2P
    if (pst_device->uc_vap_num == 0) {
#if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1131C_HOST)
        /* 1102 wlan0�����豸һֱ���� */
        pst_device->st_p2p_info.pst_primary_net_device = OAL_PTR_NULL;
#endif
    }
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
    hmac_proxy_exit(pst_vap);
#endif

#ifdef _PRE_WLAN_FEATURE_ROAM
    hmac_roam_exit(pst_hmac_vap);
#endif // _PRE_WLAN_FEATURE_ROAM

    mac_res_free_mac_vap(pst_hmac_vap->st_vap_base_info.uc_vap_id);

    /***************************************************************************
                          ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_vap, WLAN_CFGID_DEL_VAP, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) { // ���˳�����֤Devce�ҵ�������¿����µ硣
        OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_send_event failed[%d].}", ul_ret);
    }

    OAM_WARNING_LOG3(pst_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_del_vap::Del succ.vap_mode[%d], p2p_mode[%d],"
        "multi user idx[%d]}", pst_vap->en_vap_mode, pst_del_vap_param->en_p2p_mode, pst_vap->us_multi_user_idx);

#ifdef _PRE_WLAN_FEATURE_DFR
    if (g_st_dfr_info.bit_device_reset_process_flag) {
        return OAL_SUCC;
    }
#endif // _PRE_WLAN_FEATURE_DFR

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || \
    (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION))
    // ���WIFI sta��wlan0 stop���µ�
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "uc_vap_num = %d", pst_device->uc_vap_num);
    if (pst_device->uc_vap_num == 0) {
        if (pst_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
            hmac_device_stru              *pst_hmac_device;
            pst_hmac_device = hmac_res_get_mac_dev(pst_vap->uc_device_id);
            if (OAL_LIKELY(pst_hmac_device != OAL_PTR_NULL)) {
                hmac_scan_clean_scan_record(&(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt));
            } else {
                OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_config_del_vap::pst_hmac_device=%d null!}", pst_vap->uc_device_id);
            }
        }
        hmac_config_host_dev_exit(pst_vap);
        wlan_pm_close();
    }
#endif

    return ul_ret;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)


oal_uint32 hmac_config_def_chan(mac_vap_stru *pst_mac_vap)
{
    oal_uint8                     uc_channel;
    mac_cfg_mode_param_stru       st_param;

    if (((pst_mac_vap->st_channel.en_band == WLAN_BAND_BUTT) ||
        (pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_BUTT) ||
        (pst_mac_vap->en_protocol == WLAN_PROTOCOL_BUTT)) && (!IS_P2P_GO(pst_mac_vap))) {
        st_param.en_band = WLAN_BAND_2G;
        st_param.en_bandwidth = WLAN_BAND_WIDTH_20M;
        st_param.en_protocol = WLAN_HT_MODE;
        hmac_config_set_mode(pst_mac_vap, OAL_SIZEOF(st_param), (oal_uint8*)&st_param);
    }

    if ((pst_mac_vap->st_channel.uc_chan_number == 0) && (!IS_P2P_GO(pst_mac_vap))) {
        pst_mac_vap->st_channel.uc_chan_number = 6;
        uc_channel = pst_mac_vap->st_channel.uc_chan_number;
        hmac_config_set_freq(pst_mac_vap, OAL_SIZEOF(oal_uint32), &uc_channel);
    }

    return OAL_SUCC;
}
#endif


oal_uint32  hmac_config_start_vap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint8                    *puc_ssid = OAL_PTR_NULL;
    oal_uint32                    ul_ret;
    mac_device_stru              *pst_mac_device = OAL_PTR_NULL;
    hmac_vap_stru                *pst_hmac_vap = OAL_PTR_NULL;
    mac_cfg_start_vap_param_stru *pst_start_vap_param = (mac_cfg_start_vap_param_stru *)puc_param;

    if (OAL_UNLIKELY((pst_mac_vap == OAL_PTR_NULL) || (puc_param == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_start_vap::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#ifdef _PRE_WLAN_FEATURE_DAQ
    if (g_uc_data_acq_used == OAL_TRUE) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap::DAQ has been enabled."
            "Please reset the board.}");
        return OAL_FAIL;
    }
#endif
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap::pst_mac_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_mac_vap->en_vap_state == MAC_VAP_STATE_BUTT) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap::the vap has been deleted.}");

        return OAL_FAIL;
    }

    if ((pst_mac_vap->en_vap_state == MAC_VAP_STATE_UP) ||
        (pst_mac_vap->en_vap_state == MAC_VAP_STATE_AP_WAIT_START) ||
        (pst_mac_vap->en_vap_state == MAC_VAP_STATE_STA_FAKE_UP)) { /* ����Ѿ���up״̬���򷵻سɹ� */
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap::state=%d, duplicate start again}",
            pst_mac_vap->en_vap_state);
        return OAL_SUCC;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        puc_ssid = pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11DesiredSSID;
        /* P2P GO ������δ����ssid ��Ϣ������Ϊup ״̬����Ҫ���ssid ���� */
        if (OAL_STRLEN((oal_int8 *)puc_ssid) == 0 && (!IS_P2P_GO(pst_mac_vap))) {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap::ssid length=0.}");
            return OAL_FAIL;        /* û����SSID��������VAP */
        }

        /* ����AP��״̬��Ϊ WAIT_START */
        hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_AP_WAIT_START);

        if (IS_LEGACY_VAP(&(pst_hmac_vap->st_vap_base_info))) {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
            hmac_config_def_chan(pst_mac_vap);
#else
#if defined(_PRE_SUPPORT_ACS) || defined(_PRE_WLAN_FEATURE_DFS) || defined(_PRE_WLAN_FEATURE_20_40_80_COEXIST)
            if (hmac_init_scan_try(pst_mac_device, pst_mac_vap, MAC_TRY_INIT_SCAN_VAP_UP) == OAL_SUCC) {
                return OAL_SUCC;
            }
#endif
#endif
        } /* ���� en_status ���� MAC_CHNL_AV_CHK_NOT_REQ(������) ���� MAC_CHNL_AV_CHK_COMPLETE(������) */

        /* ���Э�� Ƶ�� �����Ƿ����� */
        if (((pst_mac_vap->st_channel.en_band == WLAN_BAND_BUTT) ||
            (pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_BUTT) ||
            (pst_mac_vap->en_protocol == WLAN_PROTOCOL_BUTT))) {
            if (IS_P2P_GO(pst_mac_vap)) {
                /* wpa_supplicant ��������vap up�� ��ʱ��δ��vap �����ŵ��������Э��ģʽ��Ϣ��
                   wpa_supplicant ��cfg80211_start_ap �ӿ�����GO �ŵ��������Э��ģʽ��Ϣ��
                   �ʴ˴����û�������ŵ��������Э��ģʽ��ֱ�ӷ��سɹ���������ʧ�ܡ� */
                hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_INIT);
                OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap::set band bandwidth"
                    "protocol first.band[%d], bw[%d], protocol[%d]}",
                                 pst_mac_vap->st_channel.en_band,
                                 pst_mac_vap->st_channel.en_bandwidth,
                                 pst_mac_vap->en_protocol);
                return OAL_SUCC;
            } else {
                hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_INIT);
                OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                    "{hmac_config_start_vap::set band bandwidthprotocol first.}");
                return OAL_FAIL;
            }
        }

        /* ����ŵ����Ƿ����� */
        if ((pst_mac_vap->st_channel.uc_chan_number == 0) && (!IS_P2P_GO(pst_mac_vap))) {
            hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_INIT);
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap::set channel number first.}");
            return OAL_FAIL;
        }

        /* ����bssid */
        mac_vap_set_bssid(pst_mac_vap, pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);

        /* �����Ż�����ͬƵ���µ�������һ�� */
        if (pst_mac_vap->st_channel.en_band == WLAN_BAND_2G) {
            mac_mib_set_ShortPreambleOptionImplemented(pst_mac_vap, WLAN_LEGACY_11B_MIB_SHORT_PREAMBLE);
            mac_mib_set_SpectrumManagementRequired(pst_mac_vap, OAL_FALSE);
        } else {
            mac_mib_set_ShortPreambleOptionImplemented(pst_mac_vap, WLAN_LEGACY_11B_MIB_LONG_PREAMBLE);
            mac_mib_set_SpectrumManagementRequired(pst_mac_vap, OAL_TRUE);
        }
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        /* ����AP��״̬��Ϊ UP */
        hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_UP);
#else
#if defined(_PRE_SUPPORT_ACS) || defined(_PRE_WLAN_FEATURE_DFS) || defined(_PRE_WLAN_FEATURE_20_40_80_COEXIST)
        
        if (hmac_device_in_init_scan(pst_mac_device) == OAL_FALSE)
#endif
        {
            /* ����AP��״̬��Ϊ UP */
            hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_UP);
        }
#endif
    } else if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
#ifdef _PRE_WLAN_FEATURE_P2P
        /* p2p0��p2p-p2p0 ��VAP �ṹ������p2p cl�����޸�vap ״̬ */
        if (pst_start_vap_param->en_p2p_mode != WLAN_P2P_CL_MODE)
#endif
        {
            hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_STA_FAKE_UP);
        }
    } else {
        /* ������֧ �ݲ�֧�� ������ */
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap::Do not surport other mode[%d].}",
            pst_mac_vap->en_vap_mode);
    }
    mac_vap_init_rates(pst_mac_vap);
    ul_ret = hmac_config_start_vap_event(pst_mac_vap, pst_start_vap_param->en_mgmt_rate_init_flag);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_start_vap::hmac_config_send_event failed[%d].}", ul_ret);
        return ul_ret;
    }

#ifdef _PRE_WLAN_FEATURE_DFR
    /* �쳣�ָ�����Ҫ���߶Զ�"��ȥ����"����Ϣ */
    if (g_st_dfr_info.bit_user_disconnect_flag == OAL_TRUE) {
        g_st_dfr_info.bit_user_disconnect_flag = OAL_FALSE;
        hmac_mgmt_send_disassoc_frame(pst_mac_vap, BROADCAST_MACADDR, MAC_UNSPEC_REASON, OAL_FALSE);
    }
#endif // _PRE_WLAN_FEATURE_DFR
    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap::host start vap ok."
        "now post event to dmac. vap mode[%d], p2p mode[%d]}", pst_mac_vap->en_vap_mode, pst_mac_vap->en_p2p_mode);

    return OAL_SUCC;
}


oal_uint32  hmac_config_sta_update_rates(mac_vap_stru *pst_mac_vap, mac_cfg_mode_param_stru *pst_cfg_mode,
    mac_bss_dscr_stru *pst_bss_dscr)
{
    oal_uint32                    ul_ret;
    mac_device_stru              *pst_mac_device;
    hmac_vap_stru                *pst_hmac_vap = OAL_PTR_NULL;

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_sta_update_rates::pst_mac_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_mac_vap->en_vap_state == MAC_VAP_STATE_BUTT) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_sta_update_rates::vap has been deleted.}");

        return OAL_FAIL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_sta_update_rates::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_cfg_mode->en_protocol >= WLAN_HT_MODE) {
        pst_hmac_vap->en_tx_aggr_on   = OAL_TRUE;
        pst_hmac_vap->en_amsdu_active = OAL_TRUE;
    } else {
        pst_hmac_vap->en_tx_aggr_on   = OAL_FALSE;
        pst_hmac_vap->en_amsdu_active = OAL_FALSE;
    }

    mac_vap_init_by_protocol(pst_mac_vap, pst_cfg_mode->en_protocol);
    pst_mac_vap->st_channel.en_band = pst_cfg_mode->en_band;
    pst_mac_vap->st_channel.en_bandwidth = pst_cfg_mode->en_bandwidth;
#ifdef _PRE_WIFI_DMT
    printf("hmac_config_sta_update_rates\n");
    mac_vap_init_rates(pst_mac_vap);
#else
    mac_sta_init_bss_rates(pst_mac_vap, (oal_void *)pst_bss_dscr);
#endif

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_start_vap_event(pst_mac_vap, OAL_FALSE);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_sta_update_rates::hmac_config_send_event failed[%d].}", ul_ret);
        mac_vap_init_by_protocol(pst_mac_vap, pst_hmac_vap->st_preset_para.en_protocol);
        pst_mac_vap->st_channel.en_band      = pst_hmac_vap->st_preset_para.en_band;
        pst_mac_vap->st_channel.en_bandwidth = pst_hmac_vap->st_preset_para.en_bandwidth;
        return ul_ret;
    }

    return OAL_SUCC;
}


#if defined(_PRE_SUPPORT_ACS) || defined(_PRE_WLAN_FEATURE_DFS) || defined(_PRE_WLAN_FEATURE_20_40_80_COEXIST)


oal_uint8 hmac_calc_up_and_wait_vap(hmac_device_stru *pst_hmac_dev)
{
    mac_vap_stru                  *pst_vap = OAL_PTR_NULL;
    oal_uint8                      uc_vap_idx;
    oal_uint8                      ul_up_ap_num = 0;
    mac_device_stru               *pst_mac_device = OAL_PTR_NULL;

    if (pst_hmac_dev->pst_device_base_info == OAL_PTR_NULL) {
        return 0;
    }

    pst_mac_device = pst_hmac_dev->pst_device_base_info;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++) {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (pst_vap == OAL_PTR_NULL) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "vap is null, vap id is %d", pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if (pst_vap->en_vap_state == MAC_VAP_STATE_UP || pst_vap->en_vap_state == MAC_VAP_STATE_AP_WAIT_START) {
            ul_up_ap_num++;
        }
    }

    return ul_up_ap_num;
}
#endif

oal_uint32 hmac_config_down_vap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_device_stru                *pst_mac_device = OAL_PTR_NULL;
    hmac_vap_stru                  *pst_hmac_vap = OAL_PTR_NULL;
    mac_cfg_down_vap_param_stru    *pst_param = OAL_PTR_NULL;
    oal_uint32                      ul_ret;
    oal_dlist_head_stru            *pst_entry = OAL_PTR_NULL;
    oal_dlist_head_stru            *pst_user_list_head = OAL_PTR_NULL;
    mac_user_stru                  *pst_user_tmp = OAL_PTR_NULL;
    hmac_user_stru                 *pst_hmac_user_tmp = OAL_PTR_NULL;
    oal_bool_enum_uint8             en_is_protected;
    mac_user_stru                  *pst_multi_user = OAL_PTR_NULL;

    if (OAL_UNLIKELY((pst_mac_vap == OAL_PTR_NULL) || (puc_param == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_down_vap::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_param      = (mac_cfg_down_vap_param_stru *)puc_param;

    /* ���vap�Ѿ���down��״̬��ֱ�ӷ��� */
    if (pst_mac_vap->en_vap_state == MAC_VAP_STATE_INIT) {
        /* ����net_device��flags��־ */
        if ((pst_param->pst_net_dev != OAL_PTR_NULL) &&
            (OAL_NETDEVICE_FLAGS(pst_param->pst_net_dev) & OAL_IFF_RUNNING)) {
            OAL_NETDEVICE_FLAGS(pst_param->pst_net_dev) &= (~OAL_IFF_RUNNING);
        }

        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_down_vap::vap already down.}");
        return OAL_SUCC;
    }

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_down_vap::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_down_vap::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_multi_user = mac_res_get_mac_user(pst_mac_vap->us_multi_user_idx);
    if (pst_multi_user == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_down_vap::multi_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �����������滥�⣬��Ҫ�������� */
    /* ����net_device��flags��־ */
    OAL_NETDEVICE_FLAGS(pst_param->pst_net_dev) &= (~OAL_IFF_RUNNING);

    /* ����vap�������û�, ɾ���û� */
    pst_user_list_head = &(pst_mac_vap->st_mac_user_list_head);
    for (pst_entry = pst_user_list_head->pst_next; pst_entry != pst_user_list_head;) {
        pst_user_tmp      = OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_dlist);
        pst_hmac_user_tmp = mac_res_get_hmac_user(pst_user_tmp->us_assoc_id);

        /* ָ��˫��������һ�� */
        pst_entry = pst_entry->pst_next;

        /* ����֡�����Ƿ��� */
        en_is_protected = pst_user_tmp->st_cap_info.bit_pmf_active;

        /* ��ȥ����֡ */
        hmac_mgmt_send_disassoc_frame(pst_mac_vap, pst_user_tmp->auc_user_mac_addr, MAC_DISAS_LV_SS, en_is_protected);
        /* ɾ���û� */
        hmac_user_del(pst_mac_vap, pst_hmac_user_tmp);
    }

    /* VAP��user����Ӧ��Ϊ�� */
    if (oal_dlist_is_empty(&pst_mac_vap->st_mac_user_list_head) == OAL_FALSE) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_down_vap::st_mac_user_list_head is't empty.}");
        return OAL_FAIL;
    }

    /* ��ʼ���鲥�û��İ�ȫ��Ϣ */
    mac_user_init_key(pst_multi_user);
    pst_multi_user->st_user_tx_info.st_security.en_cipher_key_type = WLAN_KEY_TYPE_TX_GTK;

    /* staģʽʱ ��desired ssid MIB���ÿգ����������Э���־ */
    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        pst_hmac_vap->bit_sta_protocol_cfg = OAL_SWITCH_OFF;
        if (pst_mac_vap->pst_mib_info != OAL_PTR_NULL) {
            memset_s(pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11DesiredSSID, WLAN_SSID_MAX_LEN, 0,
                WLAN_SSID_MAX_LEN);
        } else {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_down_vap::mib pointer is NULL!!}");
        }
        OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_down_vap::sta protocol cfg clear}");
    } else if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
#ifdef _PRE_WLAN_FEATURE_DFS
        /* ȡ�� CAC ��ʱ�� */
        hmac_dfs_cac_stop(pst_mac_device);
        hmac_dfs_off_cac_stop(pst_mac_device, pst_mac_vap);
#endif
#if defined(_PRE_SUPPORT_ACS) || defined(_PRE_WLAN_FEATURE_DFS) || defined(_PRE_WLAN_FEATURE_20_40_80_COEXIST)
        {
            hmac_device_stru *pst_hmac_dev = hmac_res_get_mac_dev(pst_mac_device->uc_device_id);

            if (pst_hmac_dev && hmac_calc_up_and_wait_vap(pst_hmac_dev) <= 1) {
                hmac_init_scan_cancel_timer(pst_hmac_dev);
                pst_hmac_dev->en_in_init_scan = OAL_FALSE;
            }
        }
#endif
    }

    /***************************************************************************
                         ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_DOWN_VAP, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_down_vap::hmac_config_send_event failed[%d].}", ul_ret);

        return ul_ret;
    }

#ifdef _PRE_WLAN_FEATURE_P2P
    if (pst_param->en_p2p_mode == WLAN_P2P_CL_MODE) {
        mac_vap_state_change(pst_mac_vap, MAC_VAP_STATE_STA_SCAN_COMP);
    } else
#endif
    {
        mac_vap_state_change(pst_mac_vap, MAC_VAP_STATE_INIT);
    }
    pst_hmac_vap->en_auth_mode = WLAN_WITP_AUTH_OPEN_SYSTEM;

#ifdef _PRE_WLAN_FEATURE_DFS
    hmac_dfs_radar_wait(pst_mac_device, pst_mac_vap->st_channel.uc_idx);
#endif

    hmac_set_rx_filter_value(pst_mac_vap);

    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_down_vap:: SUCC! Now remaining %d vaps"
        "in device[%d].}", pst_mac_device->uc_vap_num, pst_mac_device->uc_device_id);
    return OAL_SUCC;
}


#ifdef _PRE_WLAN_FEATURE_PM
oal_uint32 hmac_config_wifi_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_int32                   l_value;
    oal_uint32                  ul_ret;
    mac_device_stru*            pst_mac_device;
    oal_uint8                   uc_vap_idx;
    mac_vap_stru               *pst_service_vap;
    hmac_vap_stru              *pst_hmac_vap;
    mac_cfg_down_vap_param_stru   st_down_vap;
    mac_cfg_start_vap_param_stru  st_start_vap;

    l_value = *((oal_int32 *)puc_param);

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_config_wifi_enable::pst_mac_device null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* ����device������vap����vap up/down������PM�����¼� */
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++) {
        pst_service_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (pst_service_vap == OAL_PTR_NULL) {
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_wifi_enable::pst_mac_vap null, vap id=%d.",
                pst_mac_device->auc_vap_id[uc_vap_idx]);
            return OAL_ERR_CODE_PTR_NULL;
        }
        pst_hmac_vap = mac_res_get_hmac_vap(pst_service_vap->uc_vap_id);
        if (l_value == OAL_TRUE) {
            /* enable */
            /* ����vap upǰ֪ͨPM����оƬ */
            ul_ret = hmac_config_send_event(pst_service_vap,
                                            WLAN_CFGID_WIFI_EN,
                                            us_len,
                                            puc_param);
            if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
                OAM_WARNING_LOG1(pst_service_vap->uc_vap_id, OAM_SF_CFG,
                    "{hmac_config_wifi_enable::hmac_config_send_event failed[%d].}", ul_ret);
                return ul_ret;
            }

            st_start_vap.pst_net_dev = pst_hmac_vap->pst_net_device;
            st_start_vap.en_mgmt_rate_init_flag = OAL_TRUE;
            ul_ret = hmac_config_start_vap(&pst_hmac_vap->st_vap_base_info,
                                           OAL_SIZEOF(mac_cfg_start_vap_param_stru),
                                           (oal_uint8 *)&st_start_vap);
            if (ul_ret != OAL_SUCC) {
                OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CFG,
                    "{hmac_config_wifi_enable::hmac_config_start_vap failed[%d].}", ul_ret);
                return ul_ret;
            }
            oal_net_device_open(pst_hmac_vap->pst_net_device);
        } else {
            /* disable */
            st_down_vap.pst_net_dev = pst_hmac_vap->pst_net_device;
            ul_ret = hmac_config_down_vap(&pst_hmac_vap->st_vap_base_info,
                                          OAL_SIZEOF(mac_cfg_down_vap_param_stru),
                                          (oal_uint8 *)&st_down_vap);
            if (ul_ret != OAL_SUCC) {
                OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CFG,
                    "{hmac_config_wifi_enable::hmac_config_down_vap failed[%d].}", ul_ret);
                return ul_ret;
            }

            oal_net_device_close(pst_hmac_vap->pst_net_device);

            /* vap down����֪ͨPM˯��оƬ */
            ul_ret = hmac_config_send_event(pst_service_vap,
                                            WLAN_CFGID_WIFI_EN,
                                            us_len,
                                            puc_param);
            if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
                OAM_WARNING_LOG1(pst_service_vap->uc_vap_id, OAM_SF_CFG,
                    "{hmac_config_wifi_enable::hmac_config_send_event failed[%d].}", ul_ret);
                return ul_ret;
            }
        }
    }

    /* ���û��ҵ��VAP����������VAP */
    if (pst_mac_device->uc_vap_num == 0) {
        ul_ret = hmac_config_send_event(pst_mac_vap,
                                        WLAN_CFGID_WIFI_EN,
                                        us_len,
                                        puc_param);
        if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                "{hmac_config_wifi_enable::hmac_config_send_event failed[%d].}", ul_ret);
            return ul_ret;
        }
    }

    return ul_ret;
}
#endif


oal_uint32  hmac_config_set_bss_type(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    /* ����mibֵ */
    mac_mib_set_bss_type(pst_mac_vap, (oal_uint8)us_len, (uint8_t*)puc_param);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_HMAC == _PRE_MULTI_CORE_MODE)
    return hmac_config_send_event(pst_mac_vap, WLAN_CFGID_BSS_TYPE, us_len, puc_param);
#else
    return OAL_SUCC;
#endif
}


oal_uint32  hmac_config_get_bss_type(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    /* ��ȡmibֵ */
    return mac_mib_get_bss_type(pst_mac_vap, (oal_uint8 *)pus_len, puc_param);
}


oal_uint32  hmac_config_get_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    mac_cfg_mode_param_stru   *pst_prot_param;

    pst_prot_param = (mac_cfg_mode_param_stru *)puc_param;

    pst_prot_param->en_protocol  = pst_mac_vap->en_protocol;
    pst_prot_param->en_band      = pst_mac_vap->st_channel.en_band;
    pst_prot_param->en_bandwidth = pst_mac_vap->st_channel.en_bandwidth;

    *pus_len = OAL_SIZEOF(mac_cfg_mode_param_stru);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_config_check_mode_param(mac_device_stru *pst_mac_device,
    mac_cfg_mode_param_stru *pst_prot_param)
{
    /* ����device�����Բ������м�� */
    switch (pst_prot_param->en_protocol) {
        case WLAN_LEGACY_11A_MODE:
        case WLAN_LEGACY_11B_MODE:
        case WLAN_LEGACY_11G_MODE:
        case WLAN_MIXED_ONE_11G_MODE:
        case WLAN_MIXED_TWO_11G_MODE:
            break;

        case WLAN_HT_MODE:
        case WLAN_HT_ONLY_MODE:
        case WLAN_HT_11G_MODE:
            if (pst_mac_device->en_protocol_cap < WLAN_PROTOCOL_CAP_HT) {
                /* ����11nЭ�飬��device��֧��HTģʽ */
                OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_check_mode_param::not support HT mode, en_protocol=%d"
                    "en_protocol_cap=%d.}", pst_prot_param->en_protocol, pst_mac_device->en_protocol_cap);
                return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
            }
            break;

        case WLAN_VHT_MODE:
        case WLAN_VHT_ONLY_MODE:
            if (pst_mac_device->en_protocol_cap < WLAN_PROTOCOL_CAP_VHT) {
                /* ����11acЭ�飬��device��֧��VHTģʽ */
                OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_check_mode_param::not support VHT mode, en_protocol=%d"
                    "en_protocol_cap=%d.}", pst_prot_param->en_protocol, pst_mac_device->en_protocol_cap);
                return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
            }
            break;

        default:
            OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_config_check_mode_param::mode param does not in the list.}");
            break;
    }

    if ((pst_prot_param->en_bandwidth > WLAN_BAND_WIDTH_40MINUS) &&
        (pst_mac_device->en_bandwidth_cap < WLAN_BW_CAP_80M)) {
        /* ����80M������device������֧��80M�����ش����� */
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_check_mode_param::not support 80MHz bandwidth, en_protocol=%d"
            "en_protocol_cap=%d.}", pst_prot_param->en_bandwidth, pst_mac_device->en_bandwidth_cap);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

#ifdef _PRE_WLAN_FEATURE_5G
    if ((pst_prot_param->en_band == WLAN_BAND_5G) && (pst_mac_device->en_band_cap == WLAN_BAND_CAP_2G)) {
        /* ����5GƵ������device��֧��5G */
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_check_mode_param::not support 5GHz band, en_protocol=%d"
            "en_protocol_cap=%d.}", pst_prot_param->en_band, pst_mac_device->en_band_cap);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    } else if ((pst_prot_param->en_band == WLAN_BAND_2G) && (pst_mac_device->en_band_cap == WLAN_BAND_CAP_5G)) {
        /* ����2GƵ������device��֧��2G */
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_check_mode_param::not support 2GHz band, en_protocol=%d"
            "en_protocol_cap=%d.}", pst_prot_param->en_band, pst_mac_device->en_band_cap);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }
#else
    if ((pst_prot_param->en_band != WLAN_BAND_2G) || (pst_mac_device->en_band_cap != WLAN_BAND_CAP_2G)) {
        /* ����2GƵ������device��֧��2G */
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_check_mode_param::not support 5GHz band, en_protocol=%d"
            "en_protocol_cap=%d.}", pst_prot_param->en_band, pst_mac_device->en_band_cap);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }
#endif /* _PRE_WLAN_FEATURE_5G */

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_config_set_mode_check_bandwith(
    wlan_channel_bandwidth_enum_uint8 en_bw_device,
    wlan_channel_bandwidth_enum_uint8 en_bw_config)
{
    /* Ҫ���ô�����20M */
    if (en_bw_config == WLAN_BAND_WIDTH_20M) {
        return OAL_SUCC;
    }

    /* Ҫ���ô������״����ô�����ͬ */
    if (en_bw_device == en_bw_config) {
        return OAL_SUCC;
    }

    switch (en_bw_device) {
        case WLAN_BAND_WIDTH_80PLUSPLUS:
        case WLAN_BAND_WIDTH_80PLUSMINUS:
            if (en_bw_config == WLAN_BAND_WIDTH_40PLUS) {
                return OAL_SUCC;
            }
            break;

        case WLAN_BAND_WIDTH_80MINUSPLUS:
        case WLAN_BAND_WIDTH_80MINUSMINUS:
            if (en_bw_config == WLAN_BAND_WIDTH_40MINUS) {
                return OAL_SUCC;
            }
            break;

        default:
            break;
    }

    return OAL_FAIL;
}


oal_uint32  hmac_config_set_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_mode_param_stru    *pst_prot_param = OAL_PTR_NULL;
    hmac_vap_stru              *pst_hmac_vap = OAL_PTR_NULL;
    oal_uint32                  ul_ret;
    mac_device_stru            *pst_mac_device;

    /* ��ȡdevice */
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_mode::pst_mac_device null.}");
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }

    /* ����ģʽʱ��device�±���������һ��vap */
    if (pst_mac_device->uc_vap_num == 0) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_mode::no vap in device.}");
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_mode::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_prot_param = (mac_cfg_mode_param_stru *)puc_param;

    /* ������ò����Ƿ���device������ */
    ul_ret = hmac_config_check_mode_param(pst_mac_device, pst_prot_param);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_mode::hmac_config_check_mode_param failed[%d].}", ul_ret);
        return ul_ret;
    }

    /* device�Ѿ�����ʱ����ҪУ����Ƶ�Ρ������Ƿ�һ�� */
    if ((pst_mac_device->en_max_bandwidth != WLAN_BAND_WIDTH_BUTT) && (!MAC_DBAC_ENABLE(pst_mac_device))) {
        if (pst_mac_device->en_max_band != pst_prot_param->en_band) {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_mode::previous"
                "vap band[%d] mismatch with [%d].}", pst_mac_device->en_max_band,  pst_prot_param->en_band);
            return OAL_FAIL;
        }

        ul_ret = hmac_config_set_mode_check_bandwith(pst_mac_device->en_max_bandwidth, pst_prot_param->en_bandwidth);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                "{hmac_config_set_mode::hmac_config_set_mode_check_bandwith failed[%d],"
                "previous vap bandwidth[%d], current[%d].}", ul_ret, pst_mac_device->en_max_bandwidth,
                pst_prot_param->en_bandwidth);
            return ul_ret;
        }
    }

    if (pst_prot_param->en_protocol >= WLAN_HT_MODE) {
        pst_hmac_vap->en_tx_aggr_on   = OAL_TRUE;
        pst_hmac_vap->en_amsdu_active = OAL_TRUE;
    } else {
        pst_hmac_vap->en_tx_aggr_on   = OAL_FALSE;
        pst_hmac_vap->en_amsdu_active = OAL_FALSE;
    }

    /* ����STAЭ�����ñ�־λ */
    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        pst_hmac_vap->bit_sta_protocol_cfg        = OAL_SWITCH_ON;
        pst_hmac_vap->st_preset_para.en_protocol  = pst_prot_param->en_protocol;
        pst_hmac_vap->st_preset_para.en_bandwidth = pst_prot_param->en_bandwidth;
        pst_hmac_vap->st_preset_para.en_band      = pst_prot_param->en_band;
    }

    /* ��¼Э��ģʽ, band, bandwidth��mac_vap�� */
    pst_mac_vap->en_protocol                              = pst_prot_param->en_protocol;
    pst_mac_vap->st_channel.en_band                       = pst_prot_param->en_band;
    pst_mac_vap->st_channel.en_bandwidth                  = pst_prot_param->en_bandwidth;
    pst_mac_vap->st_ch_switch_info.en_user_pref_bandwidth = pst_prot_param->en_bandwidth;

#ifdef _PRE_WLAN_FEATURE_11AC2G
    if ((pst_mac_vap->en_protocol == WLAN_VHT_MODE) && (pst_mac_vap->st_channel.en_band == WLAN_BAND_2G)) {
        mac_vap_set_11ac2g(pst_mac_vap, OAL_TRUE);
    } else {
        mac_vap_set_11ac2g(pst_mac_vap, OAL_FALSE);
    }
#endif
#ifdef _PRE_WLAN_FEATURE_TXBF
    if ((pst_prot_param->en_protocol >= WLAN_HT_MODE)
        && (pst_mac_device->bit_su_bfmee == OAL_TRUE)) {
        pst_mac_vap->st_cap_flag.bit_11ntxbf = OAL_TRUE;
    } else {
        pst_mac_vap->st_cap_flag.bit_11ntxbf = OAL_FALSE;
    }
#endif
    OAM_INFO_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                  "{hmac_config_set_mode::protocol=%d, band=%d, bandwidth=%d.}",
                  pst_prot_param->en_protocol, pst_mac_vap->st_channel.en_band, pst_mac_vap->st_channel.en_bandwidth);

    /* ����Э�����vap���� */
    mac_vap_init_by_protocol(pst_mac_vap, pst_prot_param->en_protocol);

    /* ���ݴ�����Ϣ����Mib */
    mac_vap_change_mib_by_bandwidth(pst_mac_vap, pst_prot_param->en_bandwidth);

    /* ����device��Ƶ�μ���������Ϣ */
    if (pst_mac_device->en_max_bandwidth == WLAN_BAND_WIDTH_BUTT) {
        pst_mac_device->en_max_bandwidth = pst_prot_param->en_bandwidth;
        pst_mac_device->en_max_band      = pst_prot_param->en_band;
    }

    /***************************************************************************
     ���¼���DMAC��, ���üĴ���
    ***************************************************************************/
    ul_ret = hmac_set_mode_event(pst_mac_vap);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_mode::hmac_config_send_event failed[%d].}", ul_ret);
        return ul_ret;
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_mac_addr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
#ifdef _PRE_WLAN_FEATURE_P2P
    mac_cfg_staion_id_param_stru    *pst_station_id_param;
    wlan_p2p_mode_enum_uint8         en_p2p_mode;
#endif
    oal_uint32                       ul_ret;

    if (pst_mac_vap->pst_mib_info == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_set_mac_addr::vap->mib_info is NULL !}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#ifdef _PRE_WLAN_FEATURE_P2P
    /* P2P ����MAC ��ַmib ֵ��Ҫ����P2P DEV ��P2P_CL/P2P_GO,P2P_DEV MAC ��ַ���õ�p2p0 MIB �� */
    pst_station_id_param = (mac_cfg_staion_id_param_stru *)puc_param;
    en_p2p_mode          = pst_station_id_param->en_p2p_mode;
    if (en_p2p_mode == WLAN_P2P_DEV_MODE) {
        /* �����p2p0 device��������MAC ��ַ��auc_p2p0_dot11StationID ��Ա�� */
        oal_set_mac_addr(pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_p2p0_dot11StationID,
                         pst_station_id_param->auc_station_id);
    } else
#endif
    {
        /* ����mibֵ, Station_ID */
        mac_mib_set_station_id(pst_mac_vap, (oal_uint8)us_len, (uint8_t*)puc_param);
    }

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_STATION_ID, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_mac_addr::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_concurrent(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_int32                   l_value;

    l_value = *((oal_int32 *)puc_param);

    mac_res_set_max_asoc_user((oal_uint16)l_value);

    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_concurrent::l_value=%d.}", l_value);

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_HMAC == _PRE_MULTI_CORE_MODE)
    return hmac_config_send_event(pst_mac_vap, WLAN_CFGID_CONCURRENT, us_len, puc_param);
#else
    return OAL_SUCC;
#endif
}


oal_uint32  hmac_config_get_concurrent(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    oal_int32  *pl_value;

    pl_value  = (oal_int32 *)puc_param;
    *pl_value = mac_res_get_max_asoc_user();
    *pus_len  = OAL_SIZEOF(oal_int32);

    return OAL_SUCC;
}


oal_uint32  hmac_config_get_ssid(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    /* ��ȡmibֵ */
    return mac_mib_get_ssid(pst_mac_vap, (oal_uint8 *)pus_len, puc_param);
}


oal_uint32  hmac_config_set_ssid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    /* ����mibֵ */
    mac_mib_set_ssid(pst_mac_vap, (oal_uint8)us_len, (uint8_t*)puc_param);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) /* hi1102-cb set at both side (HMAC to DMAC) */
    return hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SSID, us_len, puc_param);
#else
    return OAL_SUCC;
#endif
}


oal_uint32  hmac_config_set_shpreamble(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32 ul_ret;

    /* ����mibֵ */
    mac_mib_set_shpreamble(pst_mac_vap, (oal_uint8)us_len, (uint8_t*)puc_param);

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SHORT_PREAMBLE, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_concurrent::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_get_shpreamble(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    /* ��mibֵ */
    return mac_mib_get_shpreamble(pst_mac_vap, (oal_uint8 *)pus_len, puc_param);
}


oal_uint32  hmac_config_set_shortgi20(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_int32           l_value;
    oal_uint32          ul_ret;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    shortgi_cfg_stru    shortgi_cfg;

    shortgi_cfg.uc_shortgi_type = SHORTGI_20_CFG_ENUM;
#endif
    l_value = *((oal_int32 *)puc_param);

    if (l_value != 0) {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        shortgi_cfg.uc_enable = OAL_TRUE;
#endif
        pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11ShortGIOptionInTwentyImplemented = OAL_TRUE;
    } else {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        shortgi_cfg.uc_enable = OAL_FALSE;
#endif
        pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11ShortGIOptionInTwentyImplemented = OAL_FALSE;
    }

    /* hi1102-cb : Need to send to Dmac via sdio */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* �����¼������¼� WLAN_CFGID_SHORTGI ͨ���¼ӵĽӿں���ȡ���ؼ����ݴ���skb��ͨ��sdio���� */
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SHORTGI, SHORTGI_CFG_STRU_LEN, (oal_uint8 *)&shortgi_cfg);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_shortgi20::hmac_config_send_event failed[%u].}", ul_ret);
    }
#endif
    return ul_ret;
}


oal_uint32  hmac_config_set_shortgi40(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_int32       l_value;
    oal_uint32      ul_ret;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    shortgi_cfg_stru    shortgi_cfg;

    shortgi_cfg.uc_shortgi_type = SHORTGI_40_CFG_ENUM;
#endif
    l_value = *((oal_int32 *)puc_param);

    if (l_value != 0) {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        shortgi_cfg.uc_enable = OAL_TRUE;
#endif
        mac_mib_set_ShortGIOptionInFortyImplemented(pst_mac_vap, OAL_TRUE);
    } else {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        shortgi_cfg.uc_enable = OAL_FALSE;
#endif
        mac_mib_set_ShortGIOptionInFortyImplemented(pst_mac_vap, OAL_FALSE);
    }

    /* hi1102-cb : Need to send to Dmac via sdio */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* �����¼������¼� WLAN_CFGID_SHORTGI ͨ���¼ӵĽӿں���ȡ���ؼ����ݴ���skb��ͨ��sdio���� */
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SHORTGI, SHORTGI_CFG_STRU_LEN, (oal_uint8 *)&shortgi_cfg);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_shortgi40::hmac_config_send_event failed[%u].}", ul_ret);
    }
#endif
    return ul_ret;
}


oal_uint32  hmac_config_set_shortgi80(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_int32       l_value;
    oal_uint32      ul_ret;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    shortgi_cfg_stru    shortgi_cfg;

    shortgi_cfg.uc_shortgi_type = SHORTGI_40_CFG_ENUM;
#endif

    l_value = *((oal_int32 *)puc_param);

    if (l_value != 0) {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        shortgi_cfg.uc_enable = OAL_TRUE;
#endif
        pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.en_dot11VHTShortGIOptionIn80Implemented = OAL_TRUE;
    } else {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        shortgi_cfg.uc_enable = OAL_FALSE;
#endif
        pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.en_dot11VHTShortGIOptionIn80Implemented = OAL_FALSE;
    }

    /* hi1102-cb : Need to send to Dmac via sdio */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* �����¼������¼� WLAN_CFGID_SHORTGI ͨ���¼ӵĽӿں���ȡ���ؼ����ݴ���skb��ͨ��sdio���� */
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SHORTGI, SHORTGI_CFG_STRU_LEN, (oal_uint8 *)&shortgi_cfg);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_shortgi80::hmac_config_send_event failed[%u].}", ul_ret);
    }
#endif
    return ul_ret;
}


oal_uint32  hmac_config_get_shortgi20(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    oal_int32       l_value;

    l_value = pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11ShortGIOptionInTwentyImplemented;

    *((oal_int32 *)puc_param) = l_value;

    *pus_len = OAL_SIZEOF(l_value);

    return OAL_SUCC;
}


oal_uint32  hmac_config_get_shortgi40(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    oal_int32       l_value;

    l_value = (oal_int32)mac_mib_get_ShortGIOptionInFortyImplemented(pst_mac_vap);

    *((oal_int32 *)puc_param) = l_value;

    *pus_len = OAL_SIZEOF(l_value);

    return OAL_SUCC;
}


oal_uint32  hmac_config_get_shortgi80(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    oal_int32       l_value;

    l_value = pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.en_dot11VHTShortGIOptionIn80Implemented;

    *((oal_int32 *)puc_param) = l_value;

    *pus_len = OAL_SIZEOF(l_value);

    return OAL_SUCC;
}
#ifdef _PRE_WLAN_FEATURE_MONITOR

oal_uint32  hmac_config_set_addr_filter(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru               *pst_hmac_vap;
    oal_uint32                   ul_ret;
    oal_int32                    l_value;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_addr_filter::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    l_value = *((oal_int32 *)puc_param);

    pst_hmac_vap->en_addr_filter = (oal_uint8)l_value;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_ADDR_FILTER, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_addr_filter::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif

oal_uint32  hmac_config_get_addr_filter(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    hmac_vap_stru               *pst_hmac_vap;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_addr_filter::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    *((oal_int32 *)puc_param) = pst_hmac_vap->en_addr_filter;
    *pus_len = OAL_SIZEOF(oal_int32);

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_prot_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_int32                   l_value;
    oal_uint32                  ul_ret;

    l_value = *((oal_int32 *)puc_param);

    if (OAL_UNLIKELY(l_value >= WLAN_PROT_BUTT)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_prot_mode::invalid l_value[%d].}", l_value);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pst_mac_vap->st_protection.en_protection_mode = (oal_uint8)l_value;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_PROT_MODE, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_prot_mode::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_get_prot_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    *((oal_int32 *)puc_param) = pst_mac_vap->st_protection.en_protection_mode;
    *pus_len = OAL_SIZEOF(oal_int32);

    return OAL_SUCC;
}


uint32_t hmac_config_set_auth_mode(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param)
{
    uint8_t       value;
    hmac_vap_stru *hmac_vap;

    value = *((uint8_t *)param);

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    if (hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_auth_mode::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    hmac_vap->en_auth_mode = WLAN_WITP_AUTH_OPEN_SYSTEM;
    if (value & BIT1) {
        hmac_vap->en_auth_mode = WLAN_WITP_AUTH_SHARED_KEY;
    }
    /* ֧��OPEN��SHARE KEY */
    if ((value & BIT0) && (value & BIT1)) {
        hmac_vap->en_auth_mode = WLAN_WITP_ALG_AUTH_BUTT;
    }
    if (value == WLAN_WITP_AUTH_SAE) {
        hmac_vap->en_auth_mode = WLAN_WITP_AUTH_SAE;
    }

    OAM_INFO_LOG1(mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_auth_mode::set auth mode[%d] succ.}",
        hmac_vap->en_auth_mode);
    return OAL_SUCC;
}


oal_uint32  hmac_config_get_auth_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    hmac_vap_stru              *pst_hmac_vap;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_auth_mode::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    *((oal_int32 *)puc_param) = pst_hmac_vap->en_auth_mode;
    *pus_len = OAL_SIZEOF(oal_int32);

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_max_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint32 ul_max_user)
{
    hmac_vap_stru            *pst_hmac_vap;
    oal_uint16                us_user_nums_max;
#if (_PRE_OS_VERSION_WIN32 != _PRE_OS_VERSION)
    oal_uint32                ul_soft_ver;
#endif
   /*
    * �˺���������vap���������û���������device�����õ����û�����ʽ���ԣ�ͬʱû�п���˫оƬ��ʵ�֡�
    * device�¹����û��������ܵ�������û������ƣ����ﲻ��Ҫȥ����device���û���������,
    * �ع麯������ֻ������vap����û����ĳ���
    */
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_max_user::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
    /* ����vap����û���Ϊ8 */
    us_user_nums_max = WLAN_ASSOC_USER_MAX_NUM_SPEC;
#else
    ul_soft_ver = get_device_soft_version();
    if (ul_soft_ver >= SOFT_VER_BUTT) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "device soft version is invalid!\n");
        return OAL_FAIL;
    } else if (ul_soft_ver == SOFT_VER_CO1) {
        /* ����vap����û���Ϊ4 */
        us_user_nums_max = WLAN_ASSOC_USER_MAX_NUM_SPEC_C01;
    } else {
        /* ����vap����û���Ϊ8 */
        us_user_nums_max = WLAN_ASSOC_USER_MAX_NUM_SPEC;
    }
#endif

    if (ul_max_user >= us_user_nums_max) {
        pst_hmac_vap->us_user_nums_max = us_user_nums_max;
    } else {
        pst_hmac_vap->us_user_nums_max = (oal_uint16)ul_max_user;
    }

    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_max_user::vap[%d] us_user_nums_max[%d].}",
        pst_mac_vap->uc_vap_id, pst_hmac_vap->us_user_nums_max);

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_bintval(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    mac_device_stru             *pst_mac_device;
    oal_uint8                   uc_vap_idx;
    mac_vap_stru*               pst_vap = OAL_PTR_NULL;

    pst_mac_device              = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
            "{hmac_config_set_bintval::mac_res_get_dev fail.device_id = %u}", pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ����device�µ�ֵ */
    mac_device_set_beacon_interval(pst_mac_device, *((oal_uint32 *)puc_param));

    /* ����device������vap */
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++) {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (pst_vap == OAL_PTR_NULL) {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_config_set_bintval::pst_mac_vap(%d) null.}",
                           pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        /* ֻ��AP VAP��Ҫbeacon interval */
        if (pst_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
            /* ����mibֵ */
            mac_mib_set_beacon_period(pst_vap, (oal_uint8)us_len, (uint8_t*)puc_param);
        }
    }

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_BEACON_INTERVAL, us_len,  puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_bintval::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_get_bintval(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    /* ��ȡmibֵ */
    return mac_mib_get_beacon_period(pst_mac_vap, (oal_uint8 *)pus_len, puc_param);
}


oal_uint32  hmac_config_set_dtimperiod(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /* ����mibֵ */
    mac_mib_set_dtim_period(pst_mac_vap, (oal_uint8)us_len, (uint8_t*)puc_param);

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_DTIM_PERIOD, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_bintval::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_get_dtimperiod(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    /* ��ȡmibֵ */
    return mac_mib_get_dtim_period(pst_mac_vap, (oal_uint8 *)pus_len, puc_param);
}


oal_uint32  hmac_config_set_nobeacon(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    hmac_vap_stru               *pst_hmac_vap;
    oal_uint32                   ul_ret;
    oal_int32                    l_value;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_nobeacon::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    l_value = *((oal_int32 *)puc_param);
    pst_hmac_vap->en_no_beacon = (oal_uint8)l_value;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_NO_BEACON, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_nobeacon::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_get_nobeacon(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    hmac_vap_stru               *pst_hmac_vap;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_nobeacon::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    *((oal_int32 *)puc_param) = pst_hmac_vap->en_no_beacon;
    *pus_len = OAL_SIZEOF(oal_int32);

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_txchain(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_int32                   l_value;
    oal_uint32                  ul_ret;
    mac_device_stru            *pst_mac_device;

    /* ��ȡdevice */
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_txchain::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    l_value = *((oal_int32 *)puc_param);

    mac_device_set_txchain(pst_mac_device, (oal_uint8)l_value);

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_TX_CHAIN, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_txchain::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_get_txchain(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    mac_device_stru  *pst_mac_device;

    /* ��ȡdevice */
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_txchain::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    *((oal_int32 *)puc_param) = pst_mac_device->uc_tx_chain;
    *pus_len = OAL_SIZEOF(oal_int32);

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_rxchain(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_int32                   l_value;
    oal_uint32                  ul_ret;
    mac_device_stru            *pst_mac_device;

    /* ��ȡdevice */
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_rxchain::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    l_value = *((oal_int32 *)puc_param);

    mac_device_set_rxchain(pst_mac_device, (oal_uint8)l_value);

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_RX_CHAIN, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_rxchain::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_get_rxchain(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    mac_device_stru  *pst_mac_device;

    /* ��ȡdevice */
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_rxchain::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    *((oal_int32 *)puc_param) = pst_mac_device->uc_rx_chain;
    *pus_len = OAL_SIZEOF(oal_int32);

    return OAL_SUCC;
}


uint32_t hmac_config_set_txpower(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param)
{
    uint32_t value;
    uint32_t ret = OAL_SUCC;

    value = (*((int32_t *)param) <= 0) ? 0 : (*((int32_t *)param));
    if (value == 0) {
        return ret;
    }

    mac_vap_set_tx_power(mac_vap, (uint8_t)value);

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ret = hmac_config_send_event(mac_vap, WLAN_CFGID_TX_POWER, len, param);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_txpower::hmac_config_send_event failed[%d].}", ret);
    }
    return ret;
}


oal_uint32  hmac_config_get_txpower(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    *((oal_int32 *)puc_param) = pst_mac_vap->uc_tx_power;
    *pus_len = OAL_SIZEOF(oal_int32);

    return OAL_SUCC;
}


extern oal_uint8 g_channel_14_enable;
oal_uint32  hmac_config_set_freq(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                           ul_ret;
    oal_uint8                            uc_channel = *puc_param;
    mac_device_stru                     *pst_mac_device;
    mac_cfg_channel_param_stru          l_channel_param;

    /* ��ȡdevice */
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_freq::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#ifdef _PRE_WLAN_HW_TEST
    if (pst_mac_device->pst_device_stru->bit_al_rx_flag == HAL_ALWAYS_RX_ENABLE) {
        /* 2.4G֧��11ac */
        if ((uc_channel <= 14) && (pst_mac_vap->en_protocol) == WLAN_VHT_MODE) {
            pst_mac_vap->st_channel.en_band = WLAN_BAND_2G;
        }
    }
#endif

    ul_ret = mac_is_channel_num_valid(pst_mac_vap->st_channel.en_band, uc_channel);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_freq::mac_is_channel_num_valid[%d]"
            "failed[%d].}", uc_channel, ul_ret);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

#ifdef _PRE_WLAN_FEATURE_11D
    /* �ŵ�14���⴦��ֻ��11bЭ��ģʽ����Ч */
    if ((uc_channel == 14) && (pst_mac_vap->en_protocol != WLAN_LEGACY_11B_MODE)) {
        if (g_channel_14_enable == 0) {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                "{hmac_config_set_freq::chan-14 only available in 11b, curr protocol=%d.}", pst_mac_vap->en_protocol);
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }
#endif
    pst_mac_vap->st_channel.uc_chan_number = uc_channel;
    ul_ret = mac_get_channel_idx_from_num(pst_mac_vap->st_channel.en_band, uc_channel,
        &(pst_mac_vap->st_channel.uc_idx));
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_freq::mac_get_channel_idx_from_num fail."
            "band[%u] channel[%u]!}", pst_mac_vap->st_channel.en_band, pst_mac_vap->st_channel.uc_idx);
        return ul_ret;
    }

    /* ��DBACʱ���״������ŵ�ʱ���õ�Ӳ�� */
    if (pst_mac_device->uc_vap_num == 1 || pst_mac_device->uc_max_channel == 0) {
        mac_device_get_channel(pst_mac_device, &l_channel_param);
        l_channel_param.uc_channel = uc_channel;
        mac_device_set_channel(pst_mac_device, &l_channel_param);

        /***************************************************************************
            ���¼���DMAC��, ͬ��DMAC����
        ***************************************************************************/
        ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_CURRENT_CHANEL, us_len, puc_param);
        if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                "{hmac_config_set_freq::hmac_config_send_event failed[%d].}", ul_ret);
            return ul_ret;
        }
    }
#ifdef _PRE_WLAN_FEATURE_DBAC
    else if (mac_is_dbac_enabled(pst_mac_device)) {
        /***************************************************************************
           ���¼���DMAC��, ͬ��DMAC����
        ***************************************************************************/
        ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_CURRENT_CHANEL, us_len, puc_param);
        if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                "{hmac_config_set_freq::hmac_config_send_event failed[%d].}", ul_ret);
            return ul_ret;
        }

        OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_freq::do not check channel while DBAC enabled.}");
    }
#endif
    else {
        if (pst_mac_device->uc_max_channel != uc_channel) {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_freq::previous vap"
                "channel number=%d mismatch [%d].}", pst_mac_device->uc_max_channel, uc_channel);

            return OAL_FAIL;
        }
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_get_freq(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    *((oal_uint32 *)puc_param) = pst_mac_vap->st_channel.uc_chan_number;

    *pus_len = OAL_SIZEOF(oal_uint32);

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_wmm_params(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    oal_uint32                  ul_ac;
    oal_uint32                  ul_value;
    wlan_cfgid_enum_uint16      en_cfg_id;
    hmac_config_wmm_para_stru  *pst_cfg_stru;

    pst_cfg_stru = (hmac_config_wmm_para_stru *)puc_param;
    en_cfg_id    = (oal_uint16)pst_cfg_stru->ul_cfg_id;
    ul_ac        = pst_cfg_stru->ul_ac;
    ul_value     = pst_cfg_stru->ul_value;

    ul_ret = OAL_SUCC;

    if (ul_ac >= WLAN_WME_AC_BUTT) {
        OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_wmm_params::invalid param,en_cfg_id=%d,"
            "ul_ac=%d, ul_value=%d.}", en_cfg_id, ul_ac, ul_value);
        return OAL_FAIL;
    }

    switch (en_cfg_id) { /* ����sub-ioctl id��дWID */
        case WLAN_CFGID_EDCA_TABLE_CWMIN:
            if ((ul_value > WLAN_QEDCA_TABLE_CWMIN_MAX) || (ul_value < WLAN_QEDCA_TABLE_CWMIN_MIN)) {
                ul_ret = OAL_FAIL;
            }
            pst_mac_vap->pst_mib_info->ast_wlan_mib_edca[ul_ac].ul_dot11EDCATableCWmin = ul_value;

            break;

        case WLAN_CFGID_EDCA_TABLE_CWMAX:
            if ((ul_value > WLAN_QEDCA_TABLE_CWMAX_MAX) || (ul_value < WLAN_QEDCA_TABLE_CWMAX_MIN)) {
                ul_ret = OAL_FAIL;
            }

            pst_mac_vap->pst_mib_info->ast_wlan_mib_edca[ul_ac].ul_dot11EDCATableCWmax = ul_value;

            break;

        case WLAN_CFGID_EDCA_TABLE_AIFSN:
            if ((ul_value < WLAN_QEDCA_TABLE_AIFSN_MIN) || (ul_value > WLAN_QEDCA_TABLE_AIFSN_MAX)) {
                ul_ret = OAL_FAIL;
            }

            pst_mac_vap->pst_mib_info->ast_wlan_mib_edca[ul_ac].ul_dot11EDCATableAIFSN = ul_value;

            break;

        case WLAN_CFGID_EDCA_TABLE_TXOP_LIMIT:
            if (ul_value > WLAN_QEDCA_TABLE_TXOP_LIMIT_MAX) {
                ul_ret = OAL_FAIL;
            }

            pst_mac_vap->pst_mib_info->ast_wlan_mib_edca[ul_ac].ul_dot11EDCATableTXOPLimit = ul_value;

            break;

        case WLAN_CFGID_EDCA_TABLE_MSDU_LIFETIME:
            if (ul_value > WLAN_QEDCA_TABLE_MSDU_LIFETIME_MAX) {
                ul_ret = OAL_FAIL;
            }
            pst_mac_vap->pst_mib_info->ast_wlan_mib_edca[ul_ac].ul_dot11EDCATableMSDULifetime = ul_value;

            break;

        case WLAN_CFGID_EDCA_TABLE_MANDATORY:
            if ((ul_value != OAL_TRUE) &&  (ul_value != OAL_FALSE)) {
                ul_ret = OAL_FAIL;
            }
            pst_mac_vap->pst_mib_info->ast_wlan_mib_edca[ul_ac].en_dot11EDCATableMandatory = (oal_uint8)ul_value;

            break;

        case WLAN_CFGID_QEDCA_TABLE_CWMIN:
            if ((ul_value > WLAN_QEDCA_TABLE_CWMIN_MAX) || (ul_value < WLAN_QEDCA_TABLE_CWMIN_MIN)) {
                ul_ret = OAL_FAIL;
            }
            pst_mac_vap->pst_mib_info->st_wlan_mib_qap_edac[ul_ac].ul_dot11QAPEDCATableCWmin = ul_value;
            break;

        case WLAN_CFGID_QEDCA_TABLE_CWMAX:
            if ((ul_value > WLAN_QEDCA_TABLE_CWMAX_MAX) || (ul_value < WLAN_QEDCA_TABLE_CWMAX_MIN)) {
                ul_ret = OAL_FAIL;
            }

            pst_mac_vap->pst_mib_info->st_wlan_mib_qap_edac[ul_ac].ul_dot11QAPEDCATableCWmax = ul_value;

            break;

        case WLAN_CFGID_QEDCA_TABLE_AIFSN:
            if ((ul_value < WLAN_QEDCA_TABLE_AIFSN_MIN) || (ul_value > WLAN_QEDCA_TABLE_AIFSN_MAX)) {
                ul_ret = OAL_FAIL;
            }

            pst_mac_vap->pst_mib_info->st_wlan_mib_qap_edac[ul_ac].ul_dot11QAPEDCATableAIFSN = ul_value;

            break;

        case WLAN_CFGID_QEDCA_TABLE_TXOP_LIMIT:
            if (ul_value > WLAN_QEDCA_TABLE_TXOP_LIMIT_MAX) {
                ul_ret = OAL_FAIL;
            }

            pst_mac_vap->pst_mib_info->st_wlan_mib_qap_edac[ul_ac].ul_dot11QAPEDCATableTXOPLimit = ul_value;

            break;

        case WLAN_CFGID_QEDCA_TABLE_MSDU_LIFETIME:
            if (ul_value > WLAN_QEDCA_TABLE_MSDU_LIFETIME_MAX) {
                ul_ret = OAL_FAIL;
            }
            pst_mac_vap->pst_mib_info->st_wlan_mib_qap_edac[ul_ac].ul_dot11QAPEDCATableMSDULifetime = ul_value;
            break;

        case WLAN_CFGID_QEDCA_TABLE_MANDATORY:
            if ((ul_value != OAL_TRUE) &&  (ul_value != OAL_FALSE)) {
                ul_ret = OAL_FAIL;
            }

            pst_mac_vap->pst_mib_info->st_wlan_mib_qap_edac[ul_ac].en_dot11QAPEDCATableMandatory = (oal_uint8)ul_value;

            break;

        default:
            ul_ret = OAL_FAIL;
            break;
    }

    if (ul_ret == OAL_FAIL) {
        return ul_ret;
    }

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, en_cfg_id, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_wmm_params::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_get_wmm_params(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ac;
    oal_uint32                  ul_value;
    wlan_cfgid_enum_uint16      en_cfg_id;
    hmac_config_wmm_para_stru  *pst_cfg_stru;

    ul_value       = 0xFFFFFFFF;
    pst_cfg_stru = (hmac_config_wmm_para_stru *)puc_param;
    en_cfg_id    = (oal_uint16)pst_cfg_stru->ul_cfg_id;
    ul_ac        = pst_cfg_stru->ul_ac;

    if (ul_ac >= WLAN_WME_AC_BUTT) {
        OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_wmm_params::invalid param,en_cfg_id=%d,"
            "ul_ac=%d, ul_value=%d.}", en_cfg_id, ul_ac, ul_value);
        return ul_value;
    }

    switch (en_cfg_id) { /* ����sub-ioctl id��дWID */
        case WLAN_CFGID_EDCA_TABLE_CWMIN:
            ul_value = pst_mac_vap->pst_mib_info->ast_wlan_mib_edca[ul_ac].ul_dot11EDCATableCWmin;
            break;

        case WLAN_CFGID_EDCA_TABLE_CWMAX:
            ul_value = pst_mac_vap->pst_mib_info->ast_wlan_mib_edca[ul_ac].ul_dot11EDCATableCWmax;
            break;

        case WLAN_CFGID_EDCA_TABLE_AIFSN:
            ul_value = pst_mac_vap->pst_mib_info->ast_wlan_mib_edca[ul_ac].ul_dot11EDCATableAIFSN;
            break;

        case WLAN_CFGID_EDCA_TABLE_TXOP_LIMIT:
            ul_value = pst_mac_vap->pst_mib_info->ast_wlan_mib_edca[ul_ac].ul_dot11EDCATableTXOPLimit;
            break;

        case WLAN_CFGID_EDCA_TABLE_MSDU_LIFETIME:
            ul_value = pst_mac_vap->pst_mib_info->ast_wlan_mib_edca[ul_ac].ul_dot11EDCATableMSDULifetime;
            break;

        case WLAN_CFGID_EDCA_TABLE_MANDATORY:
            ul_value = pst_mac_vap->pst_mib_info->ast_wlan_mib_edca[ul_ac].en_dot11EDCATableMandatory;
            break;

        case WLAN_CFGID_QEDCA_TABLE_CWMIN:
            ul_value = pst_mac_vap->pst_mib_info->st_wlan_mib_qap_edac[ul_ac].ul_dot11QAPEDCATableCWmin;
            break;

        case WLAN_CFGID_QEDCA_TABLE_CWMAX:
            ul_value = pst_mac_vap->pst_mib_info->st_wlan_mib_qap_edac[ul_ac].ul_dot11QAPEDCATableCWmax;
            break;

        case WLAN_CFGID_QEDCA_TABLE_AIFSN:
            ul_value = pst_mac_vap->pst_mib_info->st_wlan_mib_qap_edac[ul_ac].ul_dot11QAPEDCATableAIFSN;
            break;

        case WLAN_CFGID_QEDCA_TABLE_TXOP_LIMIT:
            ul_value = pst_mac_vap->pst_mib_info->st_wlan_mib_qap_edac[ul_ac].ul_dot11QAPEDCATableTXOPLimit;
            break;

        case WLAN_CFGID_QEDCA_TABLE_MSDU_LIFETIME:
            ul_value = pst_mac_vap->pst_mib_info->st_wlan_mib_qap_edac[ul_ac].ul_dot11QAPEDCATableMSDULifetime;
            break;

        case WLAN_CFGID_QEDCA_TABLE_MANDATORY:
            ul_value = pst_mac_vap->pst_mib_info->st_wlan_mib_qap_edac[ul_ac].en_dot11QAPEDCATableMandatory;
            break;

        default:
            break;
    }

    return ul_value;
}

#ifdef _PRE_WLAN_FEATURE_SMPS

oal_uint32  hmac_config_set_smps_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_device_stru                      *pst_mac_device = OAL_PTR_NULL;
    oal_dlist_head_stru                  *pst_entry = OAL_PTR_NULL;
    oal_dlist_head_stru                  *pst_user_list_head = OAL_PTR_NULL;
    mac_user_stru                        *pst_user_tmp = OAL_PTR_NULL;
    hmac_user_stru                       *pst_hmac_user_tmp = OAL_PTR_NULL;
    mac_vap_stru                         *pst_mac_vap_tmp = OAL_PTR_NULL;
    hmac_config_wmm_para_stru            *pst_smps_mode;
    oal_uint8                             uc_vap_idx;
    oal_uint8                             uc_smps_mode;
    oal_bool_enum_uint8                   en_is_protected;
    oal_uint32                            ul_ret;
    mac_cfg_smps_mode_stru                st_smps_mode;

    pst_smps_mode = (hmac_config_wmm_para_stru *)puc_param;

    uc_smps_mode = (oal_uint8)pst_smps_mode->ul_ac;

    /* ���VAP ��֧��HT����,ֱ�ӷ��سɹ�! */
    if (pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11HighThroughputOptionImplemented != OAL_TRUE) {
        return OAL_SUCC;
    }

    /* ������ò�����ΧΪ1��2��3������ֵ��ֱ�ӷ��� */
    if ((uc_smps_mode >= WLAN_MIB_MIMO_POWER_SAVE_BUTT) || (uc_smps_mode == 0)) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_smps_mode::invalid smps_mode[%d].}", uc_smps_mode);
        return OAL_FAIL;
    }

    /* ��ȡ��ǰSMPSģʽ����δ�ı���ֱ�ӷ��� */
    if (uc_smps_mode == mac_mib_get_smps(pst_mac_vap)) {
        OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_smps_mode::unchanged smps_mode[%d].}", uc_smps_mode);
        return OAL_SUCC;
    }

    /* ��ȡdevice */
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_smps_mode::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (hmac_calc_up_ap_num(pst_mac_device) <= 1) {
        pst_mac_device->uc_dev_smps_mode = uc_smps_mode;
    } else {
        if (uc_smps_mode != pst_mac_device->uc_dev_smps_mode) {
            OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_smps_mode::current smps_mode[%d] !="
                "configed_smps_mode[%d]}", pst_mac_device->uc_dev_smps_mode, uc_smps_mode);
        }

        return OAL_SUCC;
    }

    /* ����device������vap������vap �µ��ŵ��� */
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++) {
        pst_mac_vap_tmp = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (pst_mac_vap_tmp == OAL_PTR_NULL) {
            continue;
        }

        if (pst_mac_vap_tmp->pst_mib_info->st_wlan_mib_sta_config.en_dot11HighThroughputOptionImplemented != OAL_TRUE) {
            continue;
        }
        /* �ŵ�����ֻ���APģʽ����APģʽ������ */
        if (pst_mac_vap_tmp->en_vap_mode != WLAN_VAP_MODE_BSS_AP) {
            /* STA��ʱ������ */
            /* ����STA��SMPS������������SM Power Save Frame֡ */
            continue;
        }

        /* ����mib�� */
        pst_mac_vap_tmp->pst_mib_info->st_wlan_mib_ht_sta_cfg.en_dot11MIMOPowerSave = uc_smps_mode;
        mac_vap_set_smps(pst_mac_vap_tmp, uc_smps_mode);

        /* ����VAP������SMPS���� */
        pst_mac_device->en_smps = OAL_FALSE;

        /* ����vap�������û�, ɾ���û� */
        pst_user_list_head = &(pst_mac_vap_tmp->st_mac_user_list_head);
        for (pst_entry = pst_user_list_head->pst_next; pst_entry != pst_user_list_head;) {
            pst_user_tmp      = OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_dlist);
            pst_hmac_user_tmp = mac_res_get_hmac_user(pst_user_tmp->us_assoc_id);

            /* ָ��˫��������һ�� */
            pst_entry = pst_entry->pst_next;

            /* ����֡�����Ƿ��� */
            en_is_protected = pst_user_tmp->st_cap_info.bit_pmf_active;

            /* ��ȥ����֡ */
            hmac_mgmt_send_disassoc_frame(pst_mac_vap_tmp, pst_user_tmp->auc_user_mac_addr, MAC_UNSPEC_REASON,
                en_is_protected);

            /* ɾ���û� */
            hmac_user_del(pst_mac_vap_tmp, pst_hmac_user_tmp);
        }

        /* VAP��userͷָ�벻Ӧ��Ϊ�� */
        if (oal_dlist_is_empty(&pst_mac_vap_tmp->st_mac_user_list_head) == OAL_FALSE) {
            OAM_ERROR_LOG0(pst_mac_vap_tmp->uc_vap_id, OAM_SF_CFG,
                "{hmac_config_set_smps_mode::st_mac_user_list_head is not empty.}");
        }
    }

    st_smps_mode.uc_smps_mode = (oal_uint8)pst_mac_vap->st_cap_flag.bit_smps;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_SMPS, OAL_SIZEOF(mac_cfg_smps_mode_stru),
        (oal_uint8 *)&st_smps_mode);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_smps_mode::hmac_config_send_event failed[%d].}", ul_ret);
        return ul_ret;
    }
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_smps_mode::set smpsmode[%d] ok}", uc_smps_mode);

    return OAL_SUCC;
}


oal_uint32  hmac_config_get_smps_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    ul_ret = mac_vap_get_smps_mode(pst_mac_vap, pus_len, puc_param);
    return ul_ret;
}


oal_uint32  hmac_config_get_smps_en(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    ul_ret = mac_vap_get_smps_en(pst_mac_vap, pus_len, puc_param);
    return ul_ret;
}

#endif

#ifdef _PRE_WLAN_FEATURE_UAPSD

oal_uint32  hmac_config_set_uapsden(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /* ����mibֵ */
    mac_vap_set_uapsd_en(pst_mac_vap, *puc_param);
    g_uc_uapsd_cap = *puc_param;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_UAPSD_EN, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_uapsden::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_get_uapsden(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    *puc_param = mac_vap_get_uapsd_en(pst_mac_vap);
    *pus_len   = OAL_SIZEOF(oal_uint8);

    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_DFT_STAT

oal_uint32  hmac_config_set_phy_stat_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_PHY_STAT_EN, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_phy_stat_en::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_dbb_env_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_DBB_ENV_PARAM, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_dbb_env_param::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_usr_queue_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return OAL_SUCC;
}


oal_uint32  hmac_config_report_vap_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_VAP_STAT, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_report_vap_stat::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_report_all_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_ALL_STAT, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_report_all_stat::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


#endif
#ifdef _PRE_DEBUG_MODE

#endif


oal_uint32  hmac_config_reset_hw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_RESET_HW, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_reset_hw::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

oal_uint32  hmac_config_set_reset_state(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret = OAL_SUCC;
    mac_reset_sys_stru  *pst_reset_sys;
    hmac_device_stru    *pst_hmac_device;

    pst_reset_sys = (mac_reset_sys_stru *)puc_param;

    pst_hmac_device = hmac_res_get_mac_dev(pst_mac_vap->uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hmac_config_set_reset_state::pst_hmac_device[%d] is null.}",
            pst_mac_vap->uc_device_id);
        return OAL_PTR_NULL;
    }
    mac_device_set_dfr_reset(pst_hmac_device->pst_device_base_info, pst_reset_sys->uc_value);

    /* �����ģ���ǰ����DMAC TO HMAC SYNC������������������¼����� */
    return ul_ret;
}


oal_uint32  hmac_config_dump_rx_dscr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_DUMP_RX_DSCR, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_dump_rx_dscr::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_dump_tx_dscr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_DUMP_TX_DSCR, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_dump_tx_dscr::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

OAL_STATIC oal_uint32  hmac_config_set_channel_check_param(mac_device_stru *pst_mac_device,
    mac_cfg_channel_param_stru *pst_prot_param)
{
    /* ����device�����Բ������м�� */
    if ((pst_prot_param->en_bandwidth > WLAN_BAND_WIDTH_40MINUS) &&
        (pst_mac_device->en_bandwidth_cap < WLAN_BW_CAP_80M)) {
        /* ����80M������device������֧��80M�����ش����� */
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_set_channel_check_param::not support 80MHz bandwidth,"
            "en_protocol=%d en_protocol_cap=%d.}", pst_prot_param->en_bandwidth, pst_mac_device->en_bandwidth_cap);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }
#ifdef _PRE_WLAN_FEATURE_5G
    if ((pst_prot_param->en_band == WLAN_BAND_5G) && (pst_mac_device->en_band_cap == WLAN_BAND_CAP_2G)) {
        /* ����5GƵ������device��֧��5G */
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_set_channel_check_param::not support 5GHz band,"
            "en_protocol=%d en_protocol_cap=%d.}", pst_prot_param->en_band, pst_mac_device->en_band_cap);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    } else if ((pst_prot_param->en_band == WLAN_BAND_2G) && (pst_mac_device->en_band_cap == WLAN_BAND_CAP_5G)) {
        /* ����2GƵ������device��֧��2G */
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_set_channel_check_param::not support 2GHz band,"
            "en_protocol=%d en_protocol_cap=%d.}", pst_prot_param->en_band, pst_mac_device->en_band_cap);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }
#else
    if ((pst_prot_param->en_band != WLAN_BAND_2G) || (pst_mac_device->en_band_cap != WLAN_BAND_CAP_2G)) {
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_set_channel_check_param::not support 5GHz band,"
            "en_protocol=%d en_protocol_cap=%d.}", pst_prot_param->en_band, pst_mac_device->en_band_cap);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }
#endif /* _PRE_WLAN_FEATURE_5G */
    return OAL_SUCC;
}


oal_uint32 hmac_config_set_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_channel_param_stru     *pst_channel_param;
    mac_device_stru                *pst_mac_device;
    mac_vap_stru                   *pst_mac_vap_tmp = OAL_PTR_NULL;
    oal_uint32                      ul_ret;
    oal_bool_enum_uint8             en_set_reg = OAL_FALSE;
    oal_uint8                       uc_vap_idx;
    oal_uint32                      ul_up_vap_cnt;

    pst_channel_param = (mac_cfg_channel_param_stru *)puc_param;

    /* ��ȡdevice */
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hmac_config_set_channel::pst_mac_device null,divice_id=%d.}",
            pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* ������ò����Ƿ���device������ */
    ul_ret = hmac_config_set_channel_check_param(pst_mac_device, pst_channel_param);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_channel::hmac_config_set_channel_check_param failed[%d].}", ul_ret);
        return ul_ret;
    }

    ul_up_vap_cnt = hmac_calc_up_ap_num(pst_mac_device);
    /* ACSʹ��ʱ������ʱ��ʱ���ò�ͬ���ŵ� */
#ifdef _PRE_SUPPORT_ACS
    if ((ul_up_vap_cnt > 1) && (mac_get_acs_switch(pst_mac_device) >= MAC_ACS_SW_INIT)
#ifdef _PRE_WLAN_FEATURE_DBAC
        && !mac_is_dbac_enabled(pst_mac_device)
#endif
        ) {
        mac_device_get_channel(pst_mac_device, pst_channel_param);
        OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_channel::forcechan band=%d ch=%d bw=%d}",
            pst_channel_param->en_band, pst_channel_param->uc_channel, pst_channel_param->en_bandwidth);
    }
#endif

    /* ����û��VAP up������£�����Ӳ��Ƶ��������Ĵ��� */
    if (ul_up_vap_cnt <= 1) {
        /* ��¼�״����õĴ���ֵ */
        mac_device_set_channel(pst_mac_device, pst_channel_param);

        /***************************************************************************
         ���¼���DMAC��, ���üĴ���  �ñ�־λ
        ***************************************************************************/
        en_set_reg = OAL_TRUE;
    }
#ifdef _PRE_WLAN_FEATURE_DBAC
    else if (mac_is_dbac_enabled(pst_mac_device) == OAL_TRUE) {
        /* ����DBAC�������ŵ��ж� */
        /* �ŵ�����ֻ���APģʽ����APģʽ������ */
    }
#endif /* _PRE_WLAN_FEATURE_DBAC */
    else {
        /* �ŵ����ǵ�ǰ�ŵ� */
        if (pst_mac_device->uc_max_channel != pst_channel_param->uc_channel) {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_channel::previous"
                "vap channel number=%d mismatch [%d].}", pst_mac_device->uc_max_channel, pst_channel_param->uc_channel);

            return OAL_FAIL;
        }

        /* �����ܳ��������õĴ��� */
        ul_ret = hmac_config_set_mode_check_bandwith(pst_mac_device->en_max_bandwidth, pst_channel_param->en_bandwidth);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_channel::"
                "hmac_config_set_mode_check_bandwith failed[%d], previous vap bandwidth[%d, current[%d].}", ul_ret,
                pst_mac_device->en_max_bandwidth, pst_channel_param->en_bandwidth);
            return OAL_FAIL;
        }
    }
#ifdef _PRE_WLAN_FEATURE_DBAC
    if (mac_is_dbac_enabled(pst_mac_device) == OAL_TRUE) {
        pst_mac_vap->st_channel.uc_chan_number  = pst_channel_param->uc_channel;
        pst_mac_vap->st_channel.en_band         = pst_channel_param->en_band;
        pst_mac_vap->st_channel.en_bandwidth    = pst_channel_param->en_bandwidth;
        ul_ret = mac_get_channel_idx_from_num(pst_channel_param->en_band,
                                              pst_channel_param->uc_channel, &(pst_mac_vap->st_channel.uc_idx));
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                "{hmac_config_set_channel::mac_get_channel_idx_from_num failed[%d], band[%d], channel[%d].}", ul_ret,
                pst_channel_param->en_band, pst_channel_param->uc_channel);
            return OAL_FAIL;
        }

        /* ���ݴ�����Ϣ����Mib */
        mac_vap_change_mib_by_bandwidth(pst_mac_vap, pst_channel_param->en_bandwidth);

        en_set_reg = OAL_TRUE;
    } else
#endif /* _PRE_WLAN_FEATURE_DBAC */
    {
        for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++) {
            pst_mac_vap_tmp = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
            if (pst_mac_vap_tmp == OAL_PTR_NULL) {
                continue;
            }
            pst_mac_vap_tmp->st_channel.uc_chan_number  = pst_channel_param->uc_channel;
            pst_mac_vap_tmp->st_channel.en_band         = pst_channel_param->en_band;
            pst_mac_vap_tmp->st_channel.en_bandwidth    = pst_channel_param->en_bandwidth;
            ul_ret = mac_get_channel_idx_from_num(pst_channel_param->en_band, pst_channel_param->uc_channel,
                &(pst_mac_vap->st_channel.uc_idx));
            if (ul_ret != OAL_SUCC) {
                OAM_WARNING_LOG3(pst_mac_vap_tmp->uc_vap_id, OAM_SF_CFG,
                    "{hmac_config_set_channel::mac_get_channel_idx_from_num failed[%d], band[%d], channel[%d].}",
                    ul_ret, pst_channel_param->en_band, pst_channel_param->uc_channel);
                continue;
            }

            /* ���ݴ�����Ϣ����Mib */
            mac_vap_change_mib_by_bandwidth(pst_mac_vap_tmp, pst_channel_param->en_bandwidth);
        }
    }
    /***************************************************************************
     ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    if (en_set_reg == OAL_TRUE) {
        ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_CFG80211_SET_CHANNEL, us_len, puc_param);
        if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                "{hmac_config_set_channel::hmac_config_send_event failed[%d].}", ul_ret);
            return ul_ret;
        }
    }

#if defined(_PRE_SUPPORT_ACS) || defined(_PRE_WLAN_FEATURE_DFS) || defined(_PRE_WLAN_FEATURE_20_40_80_COEXIST)
    if (hmac_init_scan_try(pst_mac_device, pst_mac_vap, MAC_TRY_INIT_SCAN_SET_CHANNEL) == OAL_SUCC) {
        return OAL_SUCC;
    }
#endif

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_config_set_beacon_check_param(mac_device_stru *pst_mac_device,
    mac_beacon_param_stru *pst_prot_param)
{
    /* ����device�����Բ������м�� */
    switch (pst_prot_param->en_protocol) {
        case WLAN_LEGACY_11A_MODE:
        case WLAN_LEGACY_11B_MODE:
        case WLAN_LEGACY_11G_MODE:
        case WLAN_MIXED_ONE_11G_MODE:
        case WLAN_MIXED_TWO_11G_MODE:
            break;

        case WLAN_HT_MODE:
        case WLAN_HT_ONLY_MODE:
        case WLAN_HT_11G_MODE:
            if (pst_mac_device->en_protocol_cap < WLAN_PROTOCOL_CAP_HT) {
                /* ����11nЭ�飬��device��֧��HTģʽ */
                OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_set_beacon_check_param::not support HT mode,"
                    "en_protocol=%d protocol_cap=%d.}", pst_prot_param->en_protocol, pst_mac_device->en_protocol_cap);
                return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
            }
            break;

        case WLAN_VHT_MODE:
        case WLAN_VHT_ONLY_MODE:
            if (pst_mac_device->en_protocol_cap < WLAN_PROTOCOL_CAP_VHT) {
                /* ����11acЭ�飬��device��֧��VHTģʽ */
                OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_set_beacon_check_param::not support VHT mode,"
                    "en_protocol=%d protocol_cap=%d.}", pst_prot_param->en_protocol, pst_mac_device->en_protocol_cap);
                return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
            }
            break;

        default:
            OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_config_set_beacon_check_param::mode param does not in the list.}");
            break;
    }

    return OAL_SUCC;
}


oal_uint32 hmac_config_set_beacon(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_beacon_param_stru          *beacon_param = NULL;
    mac_device_stru                *pst_mac_device;
    hmac_vap_stru                  *pst_hmac_vap = OAL_PTR_NULL;
    oal_uint32                      ul_ret;
#ifdef _PRE_WLAN_FEATURE_SMPS
    hmac_config_wmm_para_stru       st_smps_mode;
#endif
    /* ��ȡdevice */
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL) || (puc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_beacon::null param}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_beacon::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    beacon_param = (mac_beacon_param_stru*)puc_param;

    /* ���Э�����ò����Ƿ���device������ */
    ul_ret = hmac_config_set_beacon_check_param(pst_mac_device, beacon_param);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_beacon::hmac_config_add_beacon_check_param failed[%d].}", ul_ret);
        return ul_ret;
    }

    if (beacon_param->en_protocol >= WLAN_HT_MODE) {
        pst_hmac_vap->en_tx_aggr_on   = OAL_TRUE;
        pst_hmac_vap->en_amsdu_active = OAL_TRUE;
    } else {
        pst_hmac_vap->en_tx_aggr_on   = OAL_FALSE;
        pst_hmac_vap->en_amsdu_active = OAL_FALSE;
    }

    /* ����Э��ģʽ */
    if ((beacon_param->en_privacy == OAL_TRUE) && (beacon_param->uc_crypto_mode & (WLAN_WPA_BIT | WLAN_WPA2_BIT))) {
        pst_hmac_vap->en_auth_mode        = WLAN_WITP_AUTH_OPEN_SYSTEM; /* ǿ������VAP ��֤��ʽΪOPEN */
    }
    mac_vap_set_hide_ssid(pst_mac_vap, beacon_param->uc_hidden_ssid);

    /* 1102�������ں�start ap��change beacon�ӿڸ��ô˽ӿڣ���ͬ����change beaconʱ����������beacon����
       ��dtim���ڣ���ˣ�change beaconʱ��interval��dtim period����Ϊȫ�㣬��ʱ��Ӧ�ñ����õ�mib�� */
    /* ����VAP beacon interval�� dtim_period */
    if ((beacon_param->l_dtim_period != 0) || (beacon_param->l_interval != 0)) {
        pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.ul_dot11DTIMPeriod   =
            (oal_uint32)beacon_param->l_dtim_period;
        pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.ul_dot11BeaconPeriod =
            (oal_uint32)beacon_param->l_interval;
    }

    /* ����short gi */
    pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11ShortGIOptionInTwentyImplemented = beacon_param->en_shortgi_20;
    mac_mib_set_ShortGIOptionInFortyImplemented(pst_mac_vap, beacon_param->en_shortgi_40);

    pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.en_dot11VHTShortGIOptionIn80Implemented =
        beacon_param->en_shortgi_80;

    if (beacon_param->en_operation_type == MAC_ADD_BEACON) {
        mac_vap_add_beacon(pst_mac_vap, beacon_param);
    } else {
        mac_vap_set_beacon(pst_mac_vap, beacon_param);
    }

    mac_vap_init_by_protocol(pst_mac_vap, beacon_param->en_protocol);

    mac_vap_init_rates(pst_mac_vap);

    pst_mac_vap->bit_del_group_mgmt_cs = beacon_param->bit_del_group_mgmt_cs;

#ifdef _PRE_WLAN_FEATURE_SMPS
    st_smps_mode.ul_ac = (oal_uint32)(beacon_param->uc_smps_mode == (oal_uint8)MAC_SMPS_MIMO_MODE) ?
        beacon_param->uc_smps_mode : (beacon_param->uc_smps_mode + 1);
    hmac_config_set_smps_mode(pst_mac_vap, OAL_SIZEOF(hmac_config_wmm_para_stru), (oal_uint8 *)&st_smps_mode);
#endif
    /***************************************************************************
     ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_CFG80211_CONFIG_BEACON, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_beacon::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}
/*lint -e801*/

oal_uint32  hmac_config_vap_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_device_stru             *pst_mac_device = OAL_PTR_NULL;
    hmac_vap_stru               *pst_hmac_vap = OAL_PTR_NULL;
    oal_uint32                   ul_ret;
    wlan_protocol_enum_uint8     en_disp_protocol;
    mac_user_stru               *pst_multi_user = OAL_PTR_NULL;
    mac_user_stru               *pst_mac_user = OAL_PTR_NULL;
    oal_uint8                    uc_group_suit;
    oal_uint8                    uc_pairwise_suit[2];
    oal_uint8                    auc_akm_suit[2];
    oal_uint8                    uc_loop;

    oal_int8            *pc_print_buff = OAL_PTR_NULL;
    oal_uint32           ul_string_len;
    oal_int32            l_string_tmp_len;

    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_CONFIG) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_vap_info::the config vap! can't get info.}");
        return OAL_FAIL;
    }

    pst_mac_device = (mac_device_stru *)mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_vap_info::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pc_print_buff = (oal_int8 *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAM_REPORT_MAX_STRING_LEN, OAL_TRUE);
    if (pc_print_buff == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_vap_info::pc_print_buff null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    memset_s(pc_print_buff, OAM_REPORT_MAX_STRING_LEN, 0, OAM_REPORT_MAX_STRING_LEN);
    ul_string_len    = 0;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_vap_info::pst_hmac_vap null.}");
        OAL_MEM_FREE(pc_print_buff, OAL_TRUE);
        return OAL_ERR_CODE_PTR_NULL;
    }

    l_string_tmp_len = snprintf_s(pc_print_buff + ul_string_len, (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1),
                                  (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1) - 1,
                                  "vap id: %d  device id: %d  chip id: %d\n" "vap state: %d\n"
                                  "vap mode: %d   P2P mode:%d\n" "ssid: %.32s\n" "hide_ssid :%d\n",
                                  pst_mac_vap->uc_vap_id, pst_mac_vap->uc_device_id, pst_mac_vap->uc_chip_id,
                                  pst_mac_vap->en_vap_state, pst_mac_vap->en_vap_mode,
                                  pst_mac_vap->en_p2p_mode,
                                  pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11DesiredSSID,
                                  pst_mac_vap->st_cap_flag.bit_hide_ssid);
    if (l_string_tmp_len < 0) {
        goto sprint_fail;
    }
    ul_string_len += (oal_uint32)l_string_tmp_len;

    /* AP/STAЭ��ģʽ��ʾ */
    if ((pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) &&
        ((pst_mac_user = mac_res_get_mac_user(pst_mac_vap->uc_assoc_vap_id)) != NULL)) {
        en_disp_protocol = pst_mac_user->en_cur_protocol_mode;
        l_string_tmp_len = snprintf_s(pc_print_buff + ul_string_len, (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1),
                                      (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1) - 1,
                                      "protocol: sta|ap[%s|%s]\n",
                                      hmac_config_protocol2string(pst_mac_vap->en_protocol),
                                      hmac_config_protocol2string(en_disp_protocol));
    } else {
        en_disp_protocol = pst_mac_vap->en_protocol;
        l_string_tmp_len = snprintf_s(pc_print_buff + ul_string_len, (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1),
                                      (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1) - 1,
                                      "protocol: %s\n", hmac_config_protocol2string(en_disp_protocol));
    }
    if (l_string_tmp_len < 0) {
        goto sprint_fail;
    }
    ul_string_len += (oal_uint32)l_string_tmp_len;

    l_string_tmp_len = snprintf_s(pc_print_buff + ul_string_len, (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1),
        (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1) - 1,
        "band: %s  bandwidth: %s\n"
        "channel number:%d \n"
        "associated user number:%d/%d \n"
        "Beacon interval:%d \n"
        "vap feature info:\n"
        "amsdu  wme  msdu_defrag   uapsd   psm  wpa   wpa2   wps  keepalive\n"
        "%d      %d       %d       %d      %d    %d    %d     %d     %d\n"
        "vap cap info:\n"
        "shpreamble  shslottime  nobeacon  shortgi\n"
        "%d           %d          %d         %d \n"
        "rx chain: 0x%x, tx_chain: 0x%x\n"
        "tx power: %d \n"
        "protect mode: %d, auth mode: %d\n"
        "erp aging cnt: %d, ht aging cnt: %d\n"
        "auto_protection: %d\nobss_non_erp_present: %d\nobss_non_ht_present: %d\n"
        "rts_cts_protect_mode: %d\ntxop_protect_mode: %d\n"
        "no_short_slot_num: %d\nno_short_preamble_num: %d\nnon_erp_num: %d\n"
        "non_ht_num: %d\nnon_gf_num: %d\n20M_only_num: %d\n"
        "no_40dsss_cck_num: %d\nno_lsig_txop_num: %d\n",
        hmac_config_band2string(pst_mac_vap->st_channel.en_band),
        hmac_config_bw2string(pst_mac_vap->en_protocol <= WLAN_MIXED_TWO_11G_MODE ?
        WLAN_BAND_WIDTH_20M : pst_mac_vap->st_channel.en_bandwidth),
        pst_mac_vap->st_channel.uc_chan_number,
        pst_mac_vap->us_user_nums, pst_hmac_vap->us_user_nums_max,
        pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.ul_dot11BeaconPeriod,
        pst_hmac_vap->en_amsdu_active, pst_hmac_vap->en_wme_active,
        pst_hmac_vap->en_msdu_defrag_active,
        pst_mac_vap->st_cap_flag.bit_uapsd,
        pst_hmac_vap->en_psm_active,
        pst_mac_vap->st_cap_flag.bit_wpa,
        pst_mac_vap->st_cap_flag.bit_wpa2,
        pst_hmac_vap->en_wps_active,
        pst_mac_vap->st_cap_flag.bit_keepalive,
        mac_mib_get_ShortPreambleOptionImplemented(pst_mac_vap),
        pst_mac_vap->pst_mib_info->st_phy_erp.en_dot11ShortSlotTimeOptionImplemented,
        pst_hmac_vap->en_no_beacon,
        pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11ShortGIOptionInTwentyImplemented,
        pst_mac_device->uc_rx_chain, pst_mac_device->uc_tx_chain,
        pst_mac_vap->uc_tx_power,
        pst_mac_vap->st_protection.en_protection_mode,
        pst_hmac_vap->en_auth_mode,
        pst_mac_vap->st_protection.uc_obss_non_erp_aging_cnt,
        pst_mac_vap->st_protection.uc_obss_non_ht_aging_cnt,
        pst_mac_vap->st_protection.bit_auto_protection,
        pst_mac_vap->st_protection.bit_obss_non_erp_present,
        pst_mac_vap->st_protection.bit_obss_non_ht_present,
        pst_mac_vap->st_protection.bit_rts_cts_protect_mode,
        pst_mac_vap->st_protection.bit_lsig_txop_protect_mode,
        pst_mac_vap->st_protection.uc_sta_no_short_slot_num,
        pst_mac_vap->st_protection.uc_sta_no_short_preamble_num,
        pst_mac_vap->st_protection.uc_sta_non_erp_num,
        pst_mac_vap->st_protection.uc_sta_non_ht_num,
        pst_mac_vap->st_protection.uc_sta_non_gf_num,
        pst_mac_vap->st_protection.uc_sta_20M_only_num,
        pst_mac_vap->st_protection.uc_sta_no_40dsss_cck_num,
        pst_mac_vap->st_protection.uc_sta_no_lsig_txop_num);
    if (l_string_tmp_len < 0) {
        goto sprint_fail;
    }

    pc_print_buff[OAM_REPORT_MAX_STRING_LEN - 1] = '\0';
    oam_print(pc_print_buff);

    /* ������־������OAM_REPORT_MAX_STRING_LEN���ֶ��oam_print */
    memset_s(pc_print_buff, OAM_REPORT_MAX_STRING_LEN, 0, OAM_REPORT_MAX_STRING_LEN);
    ul_string_len    = 0;

    /* WPA/WPA2 ���ܲ��� */
    if (pst_mac_vap->pst_mib_info->st_wlan_mib_privacy.en_dot11PrivacyInvoked == OAL_TRUE) {
        pst_multi_user = mac_res_get_mac_user(pst_mac_vap->us_multi_user_idx);
        if (pst_multi_user == OAL_PTR_NULL) {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_vap_info::pst_multi_user null.}");
            OAL_MEM_FREE(pc_print_buff, OAL_TRUE);
            return OAL_ERR_CODE_PTR_NULL;
        }

        if (pst_mac_vap->pst_mib_info->st_wlan_mib_privacy.en_dot11RSNAActivated == OAL_TRUE) {
            if (pst_mac_vap->st_cap_flag.bit_wpa == 1) {
                uc_group_suit =
                    OAL_MIN(pst_mac_vap->pst_mib_info->st_wlan_mib_rsna_cfg.uc_dot11RSNAConfigGroupCipher, 7);
                uc_pairwise_suit[0] = OAL_MIN(pst_mac_vap->pst_mib_info->ast_wlan_mib_rsna_cfg_wpa_pairwise_cipher[0].uc_dot11RSNAConfigPairwiseCipherImplemented, 7);
                uc_pairwise_suit[1] = OAL_MIN(pst_mac_vap->pst_mib_info->ast_wlan_mib_rsna_cfg_wpa_pairwise_cipher[1].uc_dot11RSNAConfigPairwiseCipherImplemented, 7);
                auc_akm_suit[0] = OAL_MIN(pst_mac_vap->pst_mib_info->ast_wlan_mib_rsna_cfg_auth_suite[0].uc_dot11RSNAConfigAuthenticationSuiteImplemented, 7);
                auc_akm_suit[1] = OAL_MIN(pst_mac_vap->pst_mib_info->ast_wlan_mib_rsna_cfg_auth_suite[1].uc_dot11RSNAConfigAuthenticationSuiteImplemented, 7);

                l_string_tmp_len = snprintf_s(pc_print_buff + ul_string_len,
                    (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1),
                    (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1) - 1,
                    "Privacy Invoked: \nRSNA-WPA \n "
                    "GRUOP     WPA PAIREWISE0[Actived]    WPA PAIRWISE1[Actived]     AUTH1[Active]     AUTH2[Active]\n "
                    "%s        %s[%s]          %s[%s]             %s[%s]             %s[%s]\n",
                    hmac_config_ciper2string(uc_group_suit),
                    hmac_config_ciper2string(uc_pairwise_suit[0]),
                    pst_mac_vap->pst_mib_info->ast_wlan_mib_rsna_cfg_wpa_pairwise_cipher[0].en_dot11RSNAConfigPairwiseCipherActivated == OAL_TRUE ? "Actived" : "Inactived",
                    hmac_config_ciper2string(uc_pairwise_suit[1]),
                    pst_mac_vap->pst_mib_info->ast_wlan_mib_rsna_cfg_wpa_pairwise_cipher[1].en_dot11RSNAConfigPairwiseCipherActivated == OAL_TRUE ? "Actived" : "Inactived",
                    hmac_config_akm2string(auc_akm_suit[0]),
                    pst_mac_vap->pst_mib_info->ast_wlan_mib_rsna_cfg_auth_suite[0].en_dot11RSNAConfigAuthenticationSuiteActivated == OAL_TRUE ? "Actived" : "Inactived",
                    hmac_config_akm2string(auc_akm_suit[1]),
                    pst_mac_vap->pst_mib_info->ast_wlan_mib_rsna_cfg_auth_suite[1].en_dot11RSNAConfigAuthenticationSuiteActivated == OAL_TRUE ? "Actived" : "Inactived");
                if (l_string_tmp_len < 0) {
                    goto sprint_fail;
                }
                ul_string_len += (oal_uint32)l_string_tmp_len;
            }

            if (pst_mac_vap->st_cap_flag.bit_wpa2 == 1) {
                uc_group_suit =
                    OAL_MIN(pst_mac_vap->pst_mib_info->st_wlan_mib_rsna_cfg.uc_dot11RSNAConfigGroupCipher, 7);
                uc_pairwise_suit[0] = OAL_MIN(pst_mac_vap->pst_mib_info->ast_wlan_mib_rsna_cfg_wpa2_pairwise_cipher[0].uc_dot11RSNAConfigPairwiseCipherImplemented, 7);
                uc_pairwise_suit[1] = OAL_MIN(pst_mac_vap->pst_mib_info->ast_wlan_mib_rsna_cfg_wpa2_pairwise_cipher[1].uc_dot11RSNAConfigPairwiseCipherImplemented, 7);
                auc_akm_suit[0] = OAL_MIN(pst_mac_vap->pst_mib_info->ast_wlan_mib_rsna_cfg_auth_suite[0].uc_dot11RSNAConfigAuthenticationSuiteImplemented, 7);
                auc_akm_suit[1] = OAL_MIN(pst_mac_vap->pst_mib_info->ast_wlan_mib_rsna_cfg_auth_suite[1].uc_dot11RSNAConfigAuthenticationSuiteImplemented, 7);

                l_string_tmp_len = snprintf_s(pc_print_buff + ul_string_len,
                    (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1),
                    (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1) - 1,
                    "Privacy Invoked: \nRSNA-WPA2 \n"
                    "GRUOP     WPA2 PAIREWISE0[Actived]    WPA2 PAIRWISE1[Actived]     AUTH1[Active]    AUTH2[Active]\n"
                    "%s        %s[%s]          %s[%s]             %s[%s]             %s[%s]\n",
                    hmac_config_ciper2string(uc_group_suit),
                    hmac_config_ciper2string(uc_pairwise_suit[0]),
                    pst_mac_vap->pst_mib_info->ast_wlan_mib_rsna_cfg_wpa2_pairwise_cipher[0].en_dot11RSNAConfigPairwiseCipherActivated == OAL_TRUE ? "Actived" : "Inactived",
                    hmac_config_ciper2string(uc_pairwise_suit[1]),
                    pst_mac_vap->pst_mib_info->ast_wlan_mib_rsna_cfg_wpa2_pairwise_cipher[1].en_dot11RSNAConfigPairwiseCipherActivated == OAL_TRUE ? "Actived" : "Inactived",
                    hmac_config_akm2string(auc_akm_suit[0]),
                    pst_mac_vap->pst_mib_info->ast_wlan_mib_rsna_cfg_auth_suite[0].en_dot11RSNAConfigAuthenticationSuiteActivated == OAL_TRUE ? "Actived" : "Inactived",
                    hmac_config_akm2string(auc_akm_suit[1]),
                    pst_mac_vap->pst_mib_info->ast_wlan_mib_rsna_cfg_auth_suite[1].en_dot11RSNAConfigAuthenticationSuiteActivated == OAL_TRUE ? "Actived" : "Inactived");
                if (l_string_tmp_len < 0) {
                    goto sprint_fail;
                }
                ul_string_len += (oal_uint32)l_string_tmp_len;
            }
        }

        l_string_tmp_len = snprintf_s(pc_print_buff + ul_string_len, (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1),
                                      (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1) - 1,
                                      "MULTI_USER: cipher_type:%s, key_type:%d \n",
                                      hmac_config_ciper2string(pst_multi_user->st_key_info.en_cipher_type),
                                      pst_multi_user->st_user_tx_info.st_security.en_cipher_key_type);
        if (l_string_tmp_len < 0) {
            goto sprint_fail;
        }
        ul_string_len += (oal_uint32)l_string_tmp_len;
    } else {
        l_string_tmp_len = snprintf_s(pc_print_buff + ul_string_len, (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1),
                                      (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1) - 1, "Privacy NOT Invoked\n");
        if (l_string_tmp_len < 0) {
            goto sprint_fail;
        }
        ul_string_len += (oal_uint32)l_string_tmp_len;
    }

    /* APP IE ��Ϣ */
    for (uc_loop = 0; uc_loop < OAL_APP_IE_NUM; uc_loop++) {
        l_string_tmp_len = snprintf_s(pc_print_buff + ul_string_len, (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1),
                                      (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1) - 1,
                                      "APP IE:type= %d, addr = %p, len = %d, max_len = %d\n",
                                      uc_loop, pst_mac_vap->ast_app_ie[uc_loop].puc_ie,
                                      pst_mac_vap->ast_app_ie[uc_loop].ul_ie_len,
                                      pst_mac_vap->ast_app_ie[uc_loop].ul_ie_max_len);
        if (l_string_tmp_len < 0) {
            goto sprint_fail;
        }
        ul_string_len += (oal_uint32)l_string_tmp_len;
    }

    pc_print_buff[OAM_REPORT_MAX_STRING_LEN - 1] = '\0';
    oam_print(pc_print_buff);
    OAL_MEM_FREE(pc_print_buff, OAL_TRUE);

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_VAP_INFO, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_vap_info::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;

sprint_fail:

    OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_config_vap_info:: snprintf_s return error!}");
    pc_print_buff[OAM_REPORT_MAX_STRING_LEN - 1] = '\0';
    oam_print(pc_print_buff);
    OAL_MEM_FREE(pc_print_buff, OAL_TRUE);

    return OAL_FAIL;
}
/*lint +e801*/

oal_uint32  hmac_config_event_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_int32                   l_value;
    oal_uint32                  ul_ret;
    oal_uint8                   uc_loop_vap_id;

    l_value = *((oal_int32 *)puc_param);

    /* ����OAM eventģ��Ŀ��� */
    for (uc_loop_vap_id = 0; uc_loop_vap_id < WLAN_VAP_SUPPOTR_MAX_NUM_SPEC; uc_loop_vap_id++) {
        ul_ret = oam_event_set_switch(uc_loop_vap_id, (oal_switch_enum_uint8)l_value);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(uc_loop_vap_id, OAM_SF_CFG,
                "{hmac_config_event_switch::oam_event_set_switch failed[%d].}", ul_ret);
            return ul_ret;
        }
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_eth_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_eth_switch_param_stru  *pst_eth_switch_param;
    oal_uint16                      us_user_idx = 0;
    oal_uint32                      ul_ret;

    pst_eth_switch_param = (mac_cfg_eth_switch_param_stru *)puc_param;

    ul_ret = mac_vap_find_user_by_macaddr(pst_mac_vap,
                                          pst_eth_switch_param->auc_user_macaddr,
                                          &us_user_idx);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_eth_switch::mac_vap_find_user_by_macaddr failed[%d].}", ul_ret);
        return ul_ret;
    }

    ul_ret = oam_report_eth_frame_set_switch(us_user_idx,
                                             pst_eth_switch_param->en_switch,
                                             pst_eth_switch_param->en_frame_direction);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_eth_switch::oam_report_eth_frame_set_switch failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_80211_ucast_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_80211_ucast_switch_stru *pst_80211_switch_param;
    oal_uint16                            us_user_idx = 0;
    oal_uint32                            ul_ret;

    pst_80211_switch_param = (mac_cfg_80211_ucast_switch_stru *)puc_param;

    /* �㲥��ַ�����������û��ĵ���֡���� */
    if (ETHER_IS_BROADCAST(pst_80211_switch_param->auc_user_macaddr)) {
        for (us_user_idx = 0; us_user_idx < WLAN_ACTIVE_USER_MAX_NUM + WLAN_MAX_MULTI_USER_NUM_SPEC; us_user_idx++) {
            oam_report_80211_ucast_set_switch(pst_80211_switch_param->en_frame_direction,
                                              pst_80211_switch_param->en_frame_type,
                                              pst_80211_switch_param->en_frame_switch,
                                              pst_80211_switch_param->en_cb_switch,
                                              pst_80211_switch_param->en_dscr_switch,
                                              us_user_idx);
        }
    } else {
        ul_ret = mac_vap_find_user_by_macaddr(pst_mac_vap,
                                              pst_80211_switch_param->auc_user_macaddr,
                                              &us_user_idx);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_80211_ucast_switch::"
                "mac_vap_find_user_by_macaddr[%02X:XX:XX:%02X:%02X:%02X]failed !!}",
                pst_80211_switch_param->auc_user_macaddr[0], pst_80211_switch_param->auc_user_macaddr[3],
                pst_80211_switch_param->auc_user_macaddr[4], pst_80211_switch_param->auc_user_macaddr[5]);
            return ul_ret;
        }

        ul_ret = oam_report_80211_ucast_set_switch(pst_80211_switch_param->en_frame_direction,
                                                   pst_80211_switch_param->en_frame_type,
                                                   pst_80211_switch_param->en_frame_switch,
                                                   pst_80211_switch_param->en_cb_switch,
                                                   pst_80211_switch_param->en_dscr_switch,
                                                   us_user_idx);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_80211_ucast_switch::Set switch of"
                "report_ucast failed[%d]!!frame_switch[%d], cb_switch[%d], dscr_switch[%d].}", ul_ret,
                pst_80211_switch_param->en_frame_switch, pst_80211_switch_param->en_cb_switch,
                pst_80211_switch_param->en_dscr_switch);
            return ul_ret;
        }
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_80211_UCAST_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_80211_ucast_switch::hmac_config_send_event fail[%d].", ul_ret);
    }
#endif /* DMAC_OFFLOAD */

    return OAL_SUCC;
}

#ifdef _PRE_DEBUG_MODE_USER_TRACK

oal_uint32  hmac_config_report_thrput_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_USR_THRPUT_STAT, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_report_thrput_stat::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#endif

#ifdef _PRE_WLAN_FEATURE_TXOPPS


oal_uint32  hmac_config_set_txop_ps_machw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_TXOP_PS_MACHW, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TXOP,
                         "{hmac_config_set_txop_ps_machw::send event return err code [%d].}", ul_ret);
    }

    return ul_ret;
}

#endif

#ifdef _PRE_WLAN_FEATURE_BTCOEX

oal_uint32  hmac_config_print_btcoex_status(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_BTCOEX_STATUS_PRINT, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX,
                         "{hmac_config_print_btcoex_status::send event return err code [%d].}", ul_ret);
    }

    return ul_ret;
}
#endif

#ifdef _PRE_WLAN_FEATURE_LTECOEX

oal_uint32  hmac_config_ltecoex_mode_set(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_LTECOEX_MODE_SET, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX,
                         "{hmac_config_ltecoex_mode_set::send event return err code [%d].}", ul_ret);
    }

    return ul_ret;
}
#endif


oal_uint32  hmac_config_80211_mcast_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_80211_mcast_switch_stru *pst_80211_switch_param;
    oal_uint32                       ul_ret;

    pst_80211_switch_param = (mac_cfg_80211_mcast_switch_stru *)puc_param;

    ul_ret = oam_report_80211_mcast_set_switch(pst_80211_switch_param->en_frame_direction,
                                               pst_80211_switch_param->en_frame_type,
                                               pst_80211_switch_param->en_frame_switch,
                                               pst_80211_switch_param->en_cb_switch,
                                               pst_80211_switch_param->en_dscr_switch);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_80211_mcast_switch::oam_report_80211_mcast_set_switch failed[%d].}", ul_ret);
        return ul_ret;
    }
    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_80211_MCAST_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_80211_mcast_switch::hmac_config_send_event fail[%d].", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_probe_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_probe_switch_stru       *pst_probe_switch;
    oal_uint32                       ul_ret;

    pst_probe_switch = (mac_cfg_probe_switch_stru *)puc_param;

    ul_ret = oam_report_80211_probe_set_switch(pst_probe_switch->en_frame_direction,
                                               pst_probe_switch->en_frame_switch,
                                               pst_probe_switch->en_cb_switch,
                                               pst_probe_switch->en_dscr_switch);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_probe_switch::oam_report_80211_probe_set_switch failed[%d].}", ul_ret);
        return ul_ret;
    }

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_PROBE_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_probe_switch::hmac_config_send_event fail[%d].", ul_ret);
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_rssi_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_RSSI_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_rssi_switch::hmac_config_send_event fail[%d].", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

oal_uint32  hmac_config_report_vap_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_REPORT_VAP_INFO, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_report_vap_info::hmac_config_send_event fail[%d].", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_wfa_cfg_aifsn(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_WFA_CFG_AIFSN, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_wfa_cfg_aifsn::hmac_config_send_event fail[%d].", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_wfa_cfg_cw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_WFA_CFG_CW, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_wfa_cfg_cw::hmac_config_send_event fail[%d].", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}

oal_uint32  hmac_config_lte_gpio_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_CHECK_LTE_GPIO, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_lte_gpio_mode::hmac_config_send_event fail[%d].", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}

#endif


oal_uint32  hmac_config_get_mpdu_num(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_GET_MPDU_NUM, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_send_event::hmac_config_send_event fail[%d].", ul_ret);
    }

    return ul_ret;
}

#ifdef _PRE_WLAN_RF_110X_CALI_DPD
#define  DPD_CALI_LUT_LENGTH 128
oal_uint32  hmac_config_start_dpd(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                               ul_ret;

    /***************************************************************************
       ���¼���DMAC��, ͬ��DMAC����
     ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_START_DPD, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_start_dpd::hmac_config_send_event fail[%d].", ul_ret);
    }

    return ul_ret;
}

oal_uint32  hmac_dpd_data_processed_send(mac_vap_stru *pst_mac_vap, oal_void *p_param)
{
    frw_event_mem_stru       *pst_event_mem;
    frw_event_stru           *pst_event;
    dmac_tx_event_stru       *pst_dpd_event;
    oal_netbuf_stru          *pst_netbuf_dpd_data;
    dpd_cali_data_stru       *pst_dpd_cali_data;

    if (OAL_UNLIKELY((pst_mac_vap == OAL_PTR_NULL) || (p_param == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_CALIBRATE, "{hmac_dpd_data_processed_send::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_tx_event_stru));
    if (pst_event_mem == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CALIBRATE, "{hmac_scan_proc_scan_req_event::pst_event_memnull.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_netbuf_dpd_data = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_LARGE_NETBUF_SIZE, OAL_NETBUF_PRIORITY_MID);
    if (pst_netbuf_dpd_data == OAL_PTR_NULL) {
        FRW_EVENT_FREE(pst_event_mem);
        OAM_ERROR_LOG0(0, OAM_SF_CALIBRATE, "{hmac_dpd_data_processed_send::pst_netbuf_scan_result null.}");

        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_DPD_DATA_PROCESSED,
                       OAL_SIZEOF(dmac_tx_event_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_mac_vap->uc_chip_id,
                       pst_mac_vap->uc_device_id,
                       pst_mac_vap->uc_vap_id);

    memset_s(oal_netbuf_cb(pst_netbuf_dpd_data), OAL_TX_CB_LEN, 0, OAL_TX_CB_LEN);

    pst_dpd_cali_data = (dpd_cali_data_stru *)(OAL_NETBUF_DATA(pst_netbuf_dpd_data));
    if (memcpy_s(pst_dpd_cali_data, OAL_SIZEOF(dpd_cali_data_stru), p_param, OAL_SIZEOF(dpd_cali_data_stru)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_CALIBRATE, "{hmac_dpd_data_processed_send::memcpy_s fail!}");
        oal_netbuf_free(pst_netbuf_dpd_data);
        FRW_EVENT_FREE(pst_event_mem);
        return OAL_FAIL;
    }
    pst_dpd_event               = (dmac_tx_event_stru *)pst_event->auc_event_data;
    pst_dpd_event->pst_netbuf   = pst_netbuf_dpd_data;
    pst_dpd_event->us_frame_len = OAL_SIZEOF(dpd_cali_data_stru);

    frw_event_dispatch_event(pst_event_mem);

    oal_netbuf_free(pst_netbuf_dpd_data);
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}

oal_uint32 hmac_dpd_cali_data_recv(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru             *pst_event;
    oal_netbuf_stru            *pst_dpd_data_netbuf;
    dpd_cali_data_stru         *pst_dpd_cali_data_read, dpd_cali_data_calc;
    mac_vap_stru               *pst_vap;
    dmac_tx_event_stru         *pst_dtx_event;

    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_CALIBRATE, "{hmac_dpd_cali_data_recv::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event           = (frw_event_stru *)pst_event_mem->puc_data;
    pst_dtx_event       = (dmac_tx_event_stru *)pst_event->auc_event_data;
    pst_dpd_data_netbuf = pst_dtx_event->pst_netbuf;

    pst_vap = mac_res_get_mac_vap(pst_event->st_event_hdr.uc_vap_id);

    pst_dpd_cali_data_read = (dpd_cali_data_stru *)OAL_NETBUF_DATA(pst_dpd_data_netbuf);

    OAM_WARNING_LOG0(0, OAM_SF_CALIBRATE, "{hmac_dpd_cali_data_recv DPD cali data}\r\n");
    oal_print_hex_dump((oal_uint8 *)pst_dpd_cali_data_read->us_dpd_data, DPD_CALI_LUT_LENGTH, 32, "  ");

    if (hmac_rf_cali_dpd_corr_calc(pst_dpd_cali_data_read->us_dpd_data, dpd_cali_data_calc.us_dpd_data) != OAL_SUCC) {
        dpd_cali_data_calc.us_data_len = 0;
    } else {
        dpd_cali_data_calc.us_data_len = DPD_CALI_LUT_LENGTH;
    }

    hmac_dpd_data_processed_send(pst_vap, &dpd_cali_data_calc);

    oal_netbuf_free(pst_dpd_data_netbuf);

    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_CHIP_TEST

oal_uint32 hmac_config_beacon_offload_test(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;

    OAL_IO_PRINT("hmac_config_beacon_offload_test: host_sleep=%d\n", *puc_param);

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_BEACON_OFFLOAD_TEST, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_beacon_offload_test::hmac_config_send_event fail[%d].", ul_ret);
    }

    return ul_ret;
}
#endif


oal_uint32 hmac_config_ota_beacon_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint8                                uc_vap_id_loop;
    oal_uint32                               ul_ret;
    oal_int32                                l_value;

    l_value = *((oal_int32 *)puc_param);

    for (uc_vap_id_loop = 0; uc_vap_id_loop < WLAN_VAP_SUPPOTR_MAX_NUM_SPEC; uc_vap_id_loop++) {
        ul_ret = oam_ota_set_beacon_switch(uc_vap_id_loop,
                                           (oam_sdt_print_beacon_rxdscr_type_enum_uint8)l_value);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG0(uc_vap_id_loop, OAM_SF_ANY,
                "{hmac_config_ota_beacon_switch::ota beacon switch set failed!}\r\n");
            return ul_ret;
        }
    }

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_OTA_BEACON_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_ota_beacon_switch::hmac_config_send_event fail[%d].", ul_ret);
    }

    return ul_ret;
}

oal_uint32 hmac_config_ota_rx_dscr_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint8                                uc_vap_id_loop;
    oal_uint32                               ul_ret;
    oal_int32                                l_value;

    l_value = *((oal_int32 *)puc_param);

    for (uc_vap_id_loop = 0; uc_vap_id_loop < WLAN_VAP_SUPPOTR_MAX_NUM_SPEC; uc_vap_id_loop++) {
        ul_ret = oam_ota_set_rx_dscr_switch(uc_vap_id_loop,
                                            (oal_switch_enum_uint8)l_value);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG0(uc_vap_id_loop, OAM_SF_ANY,
                "{hmac_config_ota_rx_dscr_switch::ota rx_dscr switch set failed!}\r\n");
            return ul_ret;
        }
    }

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_OTA_RX_DSCR_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_ota_rx_dscr_switch::hmac_config_send_event fail[%d].", ul_ret);
    }

    return ul_ret;
}


oal_uint32 hmac_config_set_all_ota(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                 ul_ret;
    oal_switch_enum_uint8      en_switch;

    en_switch = *((oal_switch_enum_uint8 *)puc_param);
    oam_report_set_all_switch(en_switch);

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_ALL_OTA, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_set_all_ota::hmac_config_send_event fail[%d].", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_oam_output(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_int32                   l_value;
    oal_uint32                  ul_ret;

    l_value = *((oal_int32 *)puc_param);

    /* ����OAM logģ��Ŀ��� */
    ul_ret = oam_set_output_type((oam_output_type_enum_uint8)l_value);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_oam_output::oam_set_output_type failed[%d].}", ul_ret);
        return ul_ret;
    }

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_OAM_OUTPUT_TYPE, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_oam_output::hmac_config_send_event fail[%d].", ul_ret);
    }

    return OAL_SUCC;
}


oal_uint32 hmac_config_set_dhcp_arp_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_switch_enum_uint8      en_switch;

    en_switch = *((oal_switch_enum_uint8 *)puc_param);
    oam_report_dhcp_arp_set_switch(en_switch);

    return OAL_SUCC;
}


oal_uint32 hmac_config_set_random_mac_addr_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len,
                                                const oal_uint8 *puc_param)
{
    hmac_device_stru          *pst_hmac_device = OAL_PTR_NULL;
    oal_bool_enum_uint8        en_random_mac_addr_scan_switch;

    en_random_mac_addr_scan_switch = *((oal_bool_enum_uint8 *)puc_param);

    /* ��ȡhmac device�ṹ�� */
    pst_hmac_device = hmac_res_get_mac_dev(pst_mac_vap->uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_config_set_random_mac_addr_scan::pst_hmac_device is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    g_st_wlan_customize.uc_random_mac_addr_scan = en_random_mac_addr_scan_switch;
#else
    pst_hmac_device->st_scan_mgmt.en_is_random_mac_addr_scan = en_random_mac_addr_scan_switch;
#endif

    return OAL_SUCC;
}


oal_uint32  hmac_config_ampdu_start(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_ampdu_start_param_stru *pst_ampdu_start;
    hmac_user_stru                 *pst_hmac_user = OAL_PTR_NULL;
    hmac_vap_stru                  *pst_hmac_vap;
    mac_action_mgmt_args_stru       st_action_args;
    oal_uint8                       ul_ret;

    pst_ampdu_start = (mac_cfg_ampdu_start_param_stru *)puc_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_ampdu_start::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ�û���Ӧ������ */
    pst_hmac_user = mac_vap_get_hmac_user_by_addr(pst_mac_vap, pst_ampdu_start->auc_mac_addr);
    if (pst_hmac_user == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_ampdu_start::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_tid_need_ba_session(pst_hmac_vap, pst_hmac_user, pst_ampdu_start->uc_tidno, OAL_PTR_NULL);
    if (ul_ret == OAL_TRUE) {
        /*
            ����BA�Ựʱ��st_action_args�ṹ������Ա��������
            (1)uc_category:action�����
            (2)uc_action:BA action�µ����
            (3)ul_arg1:BA�Ự��Ӧ��TID
            (4)ul_arg2:BUFFER SIZE��С
            (5)ul_arg3:BA�Ự��ȷ�ϲ���
            (6)ul_arg4:TIMEOUTʱ��
        */
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_ampdu_start::uc_tidno=%d.}",
            pst_ampdu_start->uc_tidno);
        st_action_args.uc_category = MAC_ACTION_CATEGORY_BA;
        st_action_args.uc_action   = MAC_BA_ACTION_ADDBA_REQ;
        st_action_args.ul_arg1     = pst_ampdu_start->uc_tidno;    /* ������֡��Ӧ��TID�� */
        st_action_args.ul_arg2     = WLAN_AMPDU_TX_MAX_BUF_SIZE;    /* ADDBA_REQ�У�buffer_size��Ĭ�ϴ�С */
        st_action_args.ul_arg3     = MAC_BA_POLICY_IMMEDIATE;      /* BA�Ự��ȷ�ϲ��� */
        st_action_args.ul_arg4     = 0;                            /* BA�Ự�ĳ�ʱʱ������Ϊ0 */

        /* ����BA�Ự */
        hmac_mgmt_tx_action(pst_hmac_vap,  pst_hmac_user, &st_action_args);
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_ampdu_end(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_ampdu_end_param_stru   *pst_ampdu_end;
    hmac_user_stru                 *pst_hmac_user = OAL_PTR_NULL;
    hmac_vap_stru                  *pst_hmac_vap;
    mac_priv_req_args_stru          st_req_arg;

    pst_ampdu_end = (mac_cfg_ampdu_start_param_stru *)puc_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_ampdu_end::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ�û���Ӧ������ */
    pst_hmac_user = mac_vap_get_hmac_user_by_addr(pst_mac_vap, pst_ampdu_end->auc_mac_addr);
    if (pst_hmac_user == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_ampdu_end::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ����AMPDU��ʼʱ��st_req_arg�ṹ������Ա�������� */
    st_req_arg.uc_type  = MAC_A_MPDU_END;
    st_req_arg.uc_arg1  = pst_ampdu_end->uc_tidno;      /* ������֡��Ӧ��TID�� */

    /* ����BA�Ự */
    hmac_mgmt_tx_priv_req(pst_hmac_vap,  pst_hmac_user, &st_req_arg);

    return OAL_SUCC;
}


oal_uint32  hmac_config_amsdu_ampdu_switch(mac_vap_stru  *pst_mac_vap,
                                           oal_uint16    us_len,
                                           const uint8_t *puc_param)
{
    hmac_vap_stru              *pst_hmac_vap;
    oal_int32                   l_value;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_amsdu_ampdu_switch::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    l_value = *((oal_int32 *)puc_param);

    pst_hmac_vap->en_amsdu_ampdu_active = (oal_uint8)l_value;

    return OAL_SUCC;
}


oal_uint32  hmac_config_auto_ba_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    hmac_vap_stru              *pst_hmac_vap;
    oal_int32                   l_value;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_auto_ba_switch::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    l_value = *((oal_int32 *)puc_param);

    /* �����Զ�����BA�Ự�Ŀ��أ�0����رգ�1������ */
    if (l_value == 0) {
        pst_hmac_vap->en_addba_mode = HMAC_ADDBA_MODE_MANUAL;
    } else {
        pst_hmac_vap->en_addba_mode = HMAC_ADDBA_MODE_AUTO;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_profiling_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
#ifdef _PRE_PROFILING_MODE
    oal_int32                   l_value;

    l_value = *((oal_int32 *)puc_param);

    /* �����Զ�����BA�Ự�Ŀ��أ�0����رգ�1������ */
    if (l_value == 0) {
        oam_profiling_set_switch(OAM_PROFILING_TX, OAM_PROFILING_SWITCH_OFF);
        oam_profiling_set_switch(OAM_PROFILING_RX, OAM_PROFILING_SWITCH_OFF);
    } else {
        oam_profiling_set_switch(OAM_PROFILING_TX, OAM_PROFILING_SWITCH_ON);
        oam_profiling_set_switch(OAM_PROFILING_RX, OAM_PROFILING_SWITCH_ON);
    }
#endif
    return OAL_SUCC;
}


oal_uint32  hmac_config_addba_req(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_addba_req_param_stru   *pst_addba_req;
    hmac_user_stru                 *pst_hmac_user = OAL_PTR_NULL;
    hmac_vap_stru                  *pst_hmac_vap;
    mac_action_mgmt_args_stru       st_action_args;   /* ������дACTION֡�Ĳ��� */
    oal_bool_enum_uint8             en_ampdu_support;

    pst_addba_req = (mac_cfg_addba_req_param_stru *)puc_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_addba_req::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ�û���Ӧ������ */
    pst_hmac_user = mac_vap_get_hmac_user_by_addr(pst_mac_vap, pst_addba_req->auc_mac_addr);
    if (pst_hmac_user == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_addba_req::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ����BA�Ự���Ƿ���Ҫ�ж�VAP��AMPDU��֧���������Ϊ��Ҫʵ�ֽ���BA�Ựʱ��һ����AMPDU */
    en_ampdu_support = hmac_user_xht_support(pst_hmac_user);
    /* �ֶ�����ba�Ự������������������ */
    if (en_ampdu_support) {
        /*
            ����BA�Ựʱ��st_action_args(ADDBA_REQ)�ṹ������Ա��������
            (1)uc_category:action�����
            (2)uc_action:BA action�µ����
            (3)ul_arg1:BA�Ự��Ӧ��TID
            (4)ul_arg2:BUFFER SIZE��С
            (5)ul_arg3:BA�Ự��ȷ�ϲ���
            (6)ul_arg4:TIMEOUTʱ��
        */
        st_action_args.uc_category = MAC_ACTION_CATEGORY_BA;
        st_action_args.uc_action   = MAC_BA_ACTION_ADDBA_REQ;
        st_action_args.ul_arg1     = pst_addba_req->uc_tidno;       /* ������֡��Ӧ��TID�� */
        st_action_args.ul_arg2     = pst_addba_req->us_buff_size;   /* ADDBA_REQ�У�buffer_size��Ĭ�ϴ�С */
        st_action_args.ul_arg3     = pst_addba_req->en_ba_policy;   /* BA�Ự��ȷ�ϲ��� */
        st_action_args.ul_arg4     = pst_addba_req->us_timeout;     /* BA�Ự�ĳ�ʱʱ������Ϊ0 */

        /* ����BA�Ự */
        hmac_mgmt_tx_action(pst_hmac_vap,  pst_hmac_user, &st_action_args);
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_delba_req(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_delba_req_param_stru   *pst_delba_req;
    hmac_user_stru                 *pst_hmac_user;
    hmac_vap_stru                  *pst_hmac_vap;
    mac_action_mgmt_args_stru       st_action_args;   /* ������дACTION֡�Ĳ��� */
    hmac_tid_stru                  *pst_hmac_tid = OAL_PTR_NULL;

    pst_delba_req = (mac_cfg_delba_req_param_stru *)puc_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    /* ��ȡ�û���Ӧ������ */
    pst_hmac_user = mac_vap_get_hmac_user_by_addr(pst_mac_vap, pst_delba_req->auc_mac_addr);
    if (pst_hmac_user == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_delba_req::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_tid = &(pst_hmac_user->ast_tid_info[pst_delba_req->uc_tidno]);

    /* �鿴�Ự�Ƿ���� */
    if (pst_delba_req->en_direction == MAC_RECIPIENT_DELBA) {
        if (pst_hmac_tid->pst_ba_rx_info == OAL_PTR_NULL) {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_delba_req::the rx hdl is not exist.}");
            return OAL_SUCC;
        }
    } else {
        if (pst_hmac_tid->st_ba_tx_info.en_ba_status == DMAC_BA_INIT) {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_delba_req::the tx hdl is not exist.}");
            return OAL_SUCC;
        }
    }

    /*
        ����BA�Ựʱ��st_action_args(DELBA_REQ)�ṹ������Ա��������
        (1)uc_category:action�����
        (2)uc_action:BA action�µ����
        (3)ul_arg1:BA�Ự��Ӧ��TID
        (4)ul_arg2:ɾ��ba�Ự�ķ����
        (5)ul_arg3:ɾ��ba�Ự��ԭ��
        (6)ul_arg5:ba�Ự��Ӧ���û�
    */
    st_action_args.uc_category = MAC_ACTION_CATEGORY_BA;
    st_action_args.uc_action   = MAC_BA_ACTION_DELBA;
    st_action_args.ul_arg1     = pst_delba_req->uc_tidno;       /* ������֡��Ӧ��TID�� */
    st_action_args.ul_arg2     = pst_delba_req->en_direction;   /* ADDBA_REQ�У�buffer_size��Ĭ�ϴ�С */
    st_action_args.ul_arg3     = MAC_UNSPEC_REASON; /* BA�Ự��ȷ�ϲ��� */
    st_action_args.puc_arg5    = pst_delba_req->auc_mac_addr;   /* ba�Ự��Ӧ��user */

    /* ����BA�Ự */
    hmac_mgmt_tx_action(pst_hmac_vap,  pst_hmac_user, &st_action_args);

    return OAL_SUCC;
}


oal_uint32  hmac_config_amsdu_start(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_amsdu_start_param_stru  *pst_amsdu_param;
    hmac_user_stru                  *pst_hmac_user;
    oal_uint8                        uc_tid_index;

    pst_amsdu_param = (mac_cfg_amsdu_start_param_stru *)puc_param;

    /* ��ȡ�û���Ӧ������ */
    pst_hmac_user = mac_vap_get_hmac_user_by_addr(pst_mac_vap, pst_amsdu_param->auc_mac_addr);
    if (pst_hmac_user == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_amsdu_start::pst_hamc_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    for (uc_tid_index = 0; uc_tid_index < WLAN_WME_MAX_TID_NUM; uc_tid_index++) {
        hmac_amsdu_set_maxnum(&pst_hmac_user->ast_hmac_amsdu[uc_tid_index], pst_amsdu_param->uc_amsdu_max_num);
        hmac_amsdu_set_maxsize(&pst_hmac_user->ast_hmac_amsdu[uc_tid_index],
            pst_hmac_user, pst_amsdu_param->us_amsdu_max_size);
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_user_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    hmac_user_stru                  *pst_hmac_user;
    oal_uint32                       ul_ret;
    oal_uint8                        uc_tid_index;
    oam_output_type_enum_uint8       en_output_type       = OAM_OUTPUT_TYPE_BUTT;
    mac_cfg_user_info_param_stru    *pst_hmac_event;

    pst_hmac_event = (mac_cfg_user_info_param_stru *)puc_param;
    pst_hmac_user  = (hmac_user_stru *)mac_res_get_hmac_user(pst_hmac_event->us_user_idx);
    if (pst_hmac_user == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_user_info::pst_hmac_user null.}");
        return OAL_FAIL;
    }

    oam_get_output_type(&en_output_type);
    if (en_output_type != OAM_OUTPUT_TYPE_SDT) {
        /* ��־��sdt�ϱ�ʱ���߽��÷�֧�����к�ʱ�Ĵ�ӡӰ��wifi��hostapd�Ĺ���ʱ�� ���¹������� */
        OAL_IO_PRINT("en_user_asoc_state :  %d \n", pst_hmac_user->st_user_base_info.en_user_asoc_state);
        OAL_IO_PRINT("uc_is_wds :           %d \n", pst_hmac_user->uc_is_wds);
        OAL_IO_PRINT("us_amsdu_maxsize :    %d \n", pst_hmac_user->us_amsdu_maxsize);
        OAL_IO_PRINT("11ac2g :              %d \n", pst_hmac_user->st_hmac_cap_info.bit_11ac2g);
        OAL_IO_PRINT("\n");

        for (uc_tid_index = 0; uc_tid_index < 8; uc_tid_index++) {
            OAL_IO_PRINT("tid               %d \n", uc_tid_index);
            OAL_IO_PRINT("uc_amsdu_maxnum : %d \n", pst_hmac_user->ast_hmac_amsdu[uc_tid_index].uc_amsdu_maxnum);
            OAL_IO_PRINT("us_amsdu_maxsize :%d \n", pst_hmac_user->ast_hmac_amsdu[uc_tid_index].us_amsdu_maxsize);
            OAL_IO_PRINT("us_amsdu_size :   %d \n", pst_hmac_user->ast_hmac_amsdu[uc_tid_index].us_amsdu_size);
            OAL_IO_PRINT("uc_msdu_num :     %d \n", pst_hmac_user->ast_hmac_amsdu[uc_tid_index].uc_msdu_num);
            OAL_IO_PRINT("\n");
        }

        OAL_IO_PRINT("us_user_hash_idx :    %d \n", pst_hmac_user->st_user_base_info.us_user_hash_idx);
        OAL_IO_PRINT("us_assoc_id :         %d \n", pst_hmac_user->st_user_base_info.us_assoc_id);
        OAL_IO_PRINT("uc_vap_id :           %d \n", pst_hmac_user->st_user_base_info.uc_vap_id);
        OAL_IO_PRINT("uc_device_id :        %d \n", pst_hmac_user->st_user_base_info.uc_device_id);
        OAL_IO_PRINT("uc_chip_id :          %d \n", pst_hmac_user->st_user_base_info.uc_chip_id);
        OAL_IO_PRINT("uc_amsdu_supported :  %d \n", pst_hmac_user->uc_amsdu_supported);
        OAL_IO_PRINT("uc_htc_support :      %d \n", pst_hmac_user->st_user_base_info.st_ht_hdl.uc_htc_support);
        OAL_IO_PRINT("en_ht_support :       %d \n", pst_hmac_user->st_user_base_info.st_ht_hdl.en_ht_capable);
        OAL_IO_PRINT("short gi 20 40 80:    %d %d %d \n", pst_hmac_user->st_user_base_info.st_ht_hdl.bit_short_gi_20mhz,
                     pst_hmac_user->st_user_base_info.st_ht_hdl.bit_short_gi_40mhz,
                     pst_hmac_user->st_user_base_info.st_vht_hdl.bit_short_gi_80mhz);
        OAL_IO_PRINT("\n");

        OAL_IO_PRINT("Privacy info : \r\n");
        OAL_IO_PRINT("    port_valid   :                     %d \r\n",
                     pst_hmac_user->st_user_base_info.en_port_valid);
        OAL_IO_PRINT("    user_tx_info.security.cipher_key_type:      %s \r\n"
            "    user_tx_info.security.cipher_protocol_type: %s \r\n",
            hmac_config_keytype2string(pst_hmac_user->st_user_base_info.st_user_tx_info.st_security.en_cipher_key_type),
            hmac_config_cipher2string(pst_hmac_user->st_user_base_info.st_key_info.en_cipher_type));

        if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
            OAL_IO_PRINT("    STA:cipher_type :                           %s \r\n",
                         hmac_config_cipher2string(pst_hmac_user->st_user_base_info.st_key_info.en_cipher_type));
        }
        OAL_IO_PRINT("\n");
    } else {
        oam_ota_report((oal_uint8 *)pst_hmac_user,
                       (oal_uint16)(OAL_SIZEOF(hmac_user_stru) - OAL_SIZEOF(mac_user_stru)),
                       0, 0, OAM_OTA_TYPE_HMAC_USER);
    }

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_USER_INFO, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_user_info::hmac_config_send_event failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_kick_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_kick_user_param_stru        *pst_kick_user_param = OAL_PTR_NULL;
    oal_uint16                          us_user_idx;
    oal_uint32                          ul_ret;
    hmac_user_stru                      *pst_hmac_user = OAL_PTR_NULL;
    oal_bool_enum_uint8                 en_is_protected;
    oal_dlist_head_stru                 *pst_entry = OAL_PTR_NULL;
    oal_dlist_head_stru                 *pst_user_list_head = OAL_PTR_NULL;
    mac_user_stru                       *pst_user_tmp = OAL_PTR_NULL;
    hmac_user_stru                      *pst_hmac_user_tmp = OAL_PTR_NULL;
    hmac_vap_stru                       *pst_hmac_vap = OAL_PTR_NULL;

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_kick_user::null param}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_CONFIG) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_kick_user::en_vap_mode is WLAN_VAP_MODE_CONFIG.}");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pst_kick_user_param = (mac_cfg_kick_user_param_stru *)puc_param;
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_UNLIKELY(pst_hmac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hmac_config_kick_user::null param,pst_hmac_vap[%d].}", pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_kick_user::"
        "kick user mac[%02X:XX:XX:XX:%02X:%02X] reacon code[%d]}", pst_kick_user_param->auc_mac_addr[0],
        pst_kick_user_param->auc_mac_addr[4], pst_kick_user_param->auc_mac_addr[5],
        pst_kick_user_param->us_reason_code);

    /* �ߵ�ȫ��user */
    if (oal_is_broadcast_ether_addr(pst_kick_user_param->auc_mac_addr)) {
        hmac_mgmt_send_disassoc_frame(pst_mac_vap, pst_kick_user_param->auc_mac_addr,
            pst_kick_user_param->us_reason_code, OAL_FALSE);

        /* ����vap�������û�, ɾ���û� */
        pst_user_list_head = &(pst_mac_vap->st_mac_user_list_head);
        for (pst_entry = pst_user_list_head->pst_next; pst_entry != pst_user_list_head;) {
            pst_user_tmp      = OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_dlist);
            pst_hmac_user_tmp = mac_res_get_hmac_user(pst_user_tmp->us_assoc_id);
            if (OAL_UNLIKELY(pst_hmac_user_tmp == OAL_PTR_NULL)) {
                OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hmac_config_kick_user::null param,pst_hmac_user_tmp[%d].}",
                    pst_user_tmp->us_assoc_id);
                continue;
            }

            /* ָ��˫��������һ�� */
            pst_entry = pst_entry->pst_next;

            /* �޸� state & ɾ�� user */
            hmac_handle_disconnect_rsp(pst_hmac_vap, pst_hmac_user_tmp, HMAC_REPORT_DISASSOC);

            /* ɾ���û� */
            hmac_user_del(pst_mac_vap, pst_hmac_user_tmp);
        }

        /* VAP��userͷָ�벻Ӧ��Ϊ�� */
        if (oal_dlist_is_empty(&pst_mac_vap->st_mac_user_list_head) == OAL_FALSE) {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                "{hmac_config_kick_user::st_mac_user_list_head is not empty.}");
        }
        return OAL_SUCC;
    }

    ul_ret = mac_vap_find_user_by_macaddr(pst_mac_vap, pst_kick_user_param->auc_mac_addr, &us_user_idx);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_kick_user::mac_vap_find_user_by_macaddr failed[%d].}", ul_ret);
        if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
            hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_STA_FAKE_UP);
        }
        return ul_ret;
    }

    pst_hmac_user = mac_res_get_hmac_user(us_user_idx);
    if (pst_hmac_user == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_kick_user::pst_hmac_user null,us_user_idx:%d}", us_user_idx);
        if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
            hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_STA_FAKE_UP);
        }
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_hmac_user->st_user_base_info.en_user_asoc_state != MAC_USER_STATE_ASSOC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_kick_user::the user is unassociated,us_user_idx:%d}", us_user_idx);
    }

    en_is_protected = pst_hmac_user->st_user_base_info.st_cap_info.bit_pmf_active;

    /* ��ȥ��֤֡ */
    hmac_mgmt_send_disassoc_frame(pst_mac_vap, pst_hmac_user->st_user_base_info.auc_user_mac_addr,
        pst_kick_user_param->us_reason_code, en_is_protected);

    /* �޸� state & ɾ�� user */
    hmac_handle_disconnect_rsp(pst_hmac_vap, pst_hmac_user, HMAC_REPORT_DISASSOC);

    /* ɾ���û� */
    hmac_user_del(pst_mac_vap, pst_hmac_user);

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_PROXYSTA

oal_uint32  hmac_config_set_oma(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_set_oma_param_stru     *pst_set_oma_param = OAL_PTR_NULL;
    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_set_oma::null param,pst_mac_vap=%d puc_param=%d.}", pst_mac_vap,
            puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_set_oma_param = (mac_cfg_set_oma_param_stru *)puc_param;

    /* ����proxy sta��oma��ַ */
    oal_set_mac_addr(&(pst_mac_vap->st_vap_proxysta.auc_oma[0]), &(pst_set_oma_param->auc_mac_addr[0]));

    /* ���¼���wal�㣬���Proxy STA vap */
    hmac_vap_add_proxysta_vap(pst_mac_vap, (oal_uint8 *)pst_set_oma_param);

    return OAL_SUCC;
}


oal_uint32  hmac_config_proxysta_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32    ul_value;
    mac_device_stru *pst_mac_device = OAL_PTR_NULL;

    ul_value = *((oal_uint32 *)puc_param);

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_proxysta_switch::mac_res_get_dev null.");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ����proxystaģ��ʹ�ܿ��� */
    pst_mac_device->st_cap_flag.bit_proxysta = ul_value;

    /* ֻ����DMT��ʱ��ʹ�� */
#ifdef _PRE_WIFI_DMT
    if (pst_mac_device->st_cap_flag.bit_proxysta == OAL_TRUE) {
        g_l_proxysta_feature = OAL_TRUE;
        g_us_wlan_assoc_user_max_num  = 15;
        g_ul_wlan_vap_max_num_per_device = WLAN_REPEATER_SERVICE_VAP_MAX_NUM_PER_DEVICE +
            WLAN_CONFIG_VAP_MAX_NUM_PER_DEVICE; /* 1��AP, 1��sta��15��Proxy STA��1������vap */
        printf("hmac_config_proxysta_switch:enabled proxysta \r\n");
    } else {
        g_l_proxysta_feature = OAL_FALSE;
        g_us_wlan_assoc_user_max_num  = 32;
        g_ul_wlan_vap_max_num_per_device = WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE +
            WLAN_CONFIG_VAP_MAX_NUM_PER_DEVICE; /* 4��AP, 1������vap */
        printf("hmac_config_proxysta_switch:disabled proxysta \r\n");
    }
    /*
        �������ƻ�ʱ����������ͬ������
        ���1151���֣���Ҫ����Hal�ĵ���
        ����1102���֣���Ҫ�����˼�ͬ��
        ����HAL Device�ı�������
    */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* ������ͬ������ */
#else
    dmac_custom_sync(pst_mac_vap);
#endif

#endif
    return OAL_SUCC;
}

#endif


oal_uint32  hmac_config_set_dscr_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_DSCR, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_dscr_param::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_log_level(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_LOG_LEVEL, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_log_level::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_rate(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_RATE, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_rate::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_mcs(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_MCS, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_mcs::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_mcsac(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_MCSAC, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_mcsac::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_nss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_NSS, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_nss::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_rfch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_RFCH, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_rfch::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_bw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_BW, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_bw::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_always_tx(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                        ul_ret;
    hmac_vap_stru                    *pst_hmac_vap = OAL_PTR_NULL;
    mac_cfg_tx_comp_stru             *pst_set_bcast_param = OAL_PTR_NULL;
    OAL_STATIC oal_uint8              uc_enable_times = 0;

    pst_set_bcast_param = (mac_cfg_tx_comp_stru *)puc_param;
    if (pst_set_bcast_param == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_always_tx::pst_set_bcast_param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_always_tx::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_set_bcast_param->uc_param != OAL_SWITCH_OFF) {
#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
        pst_hmac_vap->bit_init_flag   = (uc_enable_times++ == 0) ? OAL_TRUE : OAL_FALSE;
        pst_hmac_vap->bit_ack_policy  = pst_set_bcast_param->en_ack_policy;
        mac_vap_set_al_tx_payload_flag(pst_mac_vap, pst_set_bcast_param->en_payload_flag);
#endif
    } else {
#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
        /* �״����б�־���ڽ��ó���ʱ�ָ�ԭֵ��ʹ��ʱ������ */
        pst_hmac_vap->bit_init_flag  = OAL_TRUE;
        mac_vap_set_al_tx_first_run(pst_mac_vap, OAL_FALSE);
#endif
        uc_enable_times = 0;
    }

    /* ʹ�ܳ��� */
    mac_vap_set_al_tx_flag(pst_mac_vap, OAL_SWITCH_OFF);
    if (pst_set_bcast_param->uc_param == OAL_SWITCH_ON) {
        mac_vap_set_al_tx_flag(pst_mac_vap, OAL_SWITCH_ON);
        pst_mac_vap->st_cap_flag.bit_keepalive = OAL_FALSE;
    }

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_ALWAYS_TX, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_always_tx::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_always_tx_1102(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                       ul_ret;
    mac_cfg_tx_comp_stru            *pst_event_set_bcast;

    /* ʹ�ܳ��� */
    pst_event_set_bcast = (mac_cfg_tx_comp_stru *)puc_param;
    mac_vap_set_al_tx_flag(pst_mac_vap, OAL_SWITCH_OFF);
    if (pst_event_set_bcast->uc_param == OAL_SWITCH_ON) {
        mac_vap_set_al_tx_flag(pst_mac_vap, OAL_SWITCH_ON);
        pst_mac_vap->st_cap_flag.bit_keepalive = OAL_FALSE;

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
        // ������ʱ�رյ͹���
        wlan_pm_disable();
        g_wlan_pm_switch = OAL_FALSE;
#endif
    } else {
        mac_vap_set_al_tx_first_run(pst_mac_vap, OAL_FALSE);
    }
    mac_vap_set_al_tx_payload_flag(pst_mac_vap, pst_event_set_bcast->en_payload_flag);

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_ALWAYS_TX_1102, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_always_tx_1102::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_always_rx(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32     ul_ret;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    oal_uint8      uc_al_rx_flag = *(oal_uint8 *)puc_param;

    if (uc_al_rx_flag == OAL_SWITCH_ON) {
        // ���մ�ʱ�رյ͹���
        wlan_pm_disable();
        g_wlan_pm_switch = OAL_FALSE;
    }
#endif

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_ALWAYS_RX, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_always_rx::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#ifdef _PRE_DEBUG_MODE

oal_uint32 hmac_config_dync_txpower(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_DYNC_TXPOWER, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_dync_txpower::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif


oal_uint32  hmac_config_get_thruput(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_GET_THRUPUT, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_get_thruput::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_freq_skew(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_FREQ_SKEW, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_freq_skew::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_adjust_ppm(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_ADJUST_PPM, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_adjust_ppm::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_list_ap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    if (pst_mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_STA) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_list_ap::invalid vap mode[%d].}", pst_mac_vap->en_vap_mode);
        return OAL_FAIL;
    }

    /* ��ӡɨ�赽��bss��Ϣ */
    hmac_scan_print_scanned_bss_info(pst_mac_vap->uc_device_id);

    return OAL_SUCC;
}


oal_uint32  hmac_config_list_sta(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint8                   uc_user_idx;
    mac_user_stru              *pst_mac_user = OAL_PTR_NULL;
    mac_res_user_hash_stru     *pst_res_hash = OAL_PTR_NULL;
    oal_dlist_head_stru        *pst_head = OAL_PTR_NULL;
    wlan_protocol_enum_uint8    en_protocol_mode;
    oal_int8                    ac_tmp_buff[256]    = {0};
    oal_int32                   l_remainder_len;
    oal_int8                   *pc_print_buff = OAL_PTR_NULL;

    /* AP�����Ϣ���ܴ�ӡ�����Ϣ */
    if (pst_mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_AP) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_list_sta::invalid en_vap_mode[%d].", pst_mac_vap->en_vap_mode);
        return OAL_FAIL;
    }

    pc_print_buff = (oal_int8 *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAM_REPORT_MAX_STRING_LEN, OAL_TRUE);
    if (pc_print_buff == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    memset_s(pc_print_buff, OAM_REPORT_MAX_STRING_LEN, 0, OAM_REPORT_MAX_STRING_LEN);

    if (snprintf_s(pc_print_buff, OAM_REPORT_MAX_STRING_LEN, OAM_REPORT_MAX_STRING_LEN - 1, "Total user num is %d\n",
                   pst_mac_vap->us_user_nums) < EOK) {
        OAM_ERROR_LOG0(0, 0, "{hmac_config_list_sta::snprintf_s fail.}");
    }
    if (strcat_s(pc_print_buff, OAM_REPORT_MAX_STRING_LEN, "User assoc id      ADDR      Protocol Type \n") != EOK) {
        OAM_ERROR_LOG0(0, 0, "{hmac_config_list_sta::strcat_s fail.}");
    }
    l_remainder_len = (oal_int32)(OAM_REPORT_MAX_STRING_LEN - OAL_STRLEN(pc_print_buff));
    /* AP���USER��Ϣ */
    for (uc_user_idx = 0; uc_user_idx < MAC_VAP_USER_HASH_MAX_VALUE; uc_user_idx++) {
        pst_head = pst_mac_vap->ast_user_hash[uc_user_idx].pst_next;

        while ((&pst_mac_vap->ast_user_hash[uc_user_idx]) != pst_head) {
            pst_res_hash = (mac_res_user_hash_stru *)pst_head;
            /* �ҵ���Ӧ�û� */
            pst_mac_user = mac_res_get_mac_user(pst_res_hash->us_user_idx);
            if (pst_mac_user == OAL_PTR_NULL) {
                OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_list_sta::pst_mac_user null.}"); pst_head = pst_res_hash->st_entry.pst_next;
                continue;
            }
            /* user�ṹ���µ�Э��ģʽ������a��g����Ҫ����Ƶ������ */
            en_protocol_mode = pst_mac_user->en_protocol_mode;
            if (en_protocol_mode >= WLAN_PROTOCOL_BUTT) {
                OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_list_sta:: protocol_mode wrong.[%d]}", en_protocol_mode);
                pst_head = pst_res_hash->st_entry.pst_next;
                continue;
            }
#ifdef _PRE_WLAN_FEATURE_5G
            if ((en_protocol_mode == WLAN_LEGACY_11G_MODE)  && (pst_mac_vap->st_channel.en_band == WLAN_BAND_5G)) {
                en_protocol_mode = WLAN_LEGACY_11A_MODE;
            }
#endif /* _PRE_WLAN_FEATURE_5G */
            if (snprintf_s(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff), OAL_SIZEOF(ac_tmp_buff) - 1,
                           "     %d       %02X:XX:XX:%02X:%02X:%02X       %s \n", pst_mac_user->us_assoc_id,
                           pst_mac_user->auc_user_mac_addr[0], pst_mac_user->auc_user_mac_addr[3],
                           pst_mac_user->auc_user_mac_addr[4], pst_mac_user->auc_user_mac_addr[5],
                           gst_protocol_mode_list[en_protocol_mode].puc_protocol_desc) < EOK) {
                OAM_ERROR_LOG0(0, 0, "{hmac_config_list_sta::snprintf_s fail.}");
            }
            if (strncat_s(pc_print_buff, OAM_REPORT_MAX_STRING_LEN, ac_tmp_buff, l_remainder_len - 1) != EOK) {
                OAM_ERROR_LOG0(0, 0, "{hmac_config_list_sta::strcat_s fail.}");
            }
            memset_s(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff), 0, OAL_SIZEOF(ac_tmp_buff));
            l_remainder_len = (oal_int32)(OAM_REPORT_MAX_STRING_LEN - OAL_STRLEN(pc_print_buff));

            pst_head = pst_res_hash->st_entry.pst_next;
        }
    }

    oam_print(pc_print_buff);
    OAL_MEM_FREE(pc_print_buff, OAL_TRUE);
    return OAL_SUCC;
}


oal_uint32  hmac_config_get_sta_list(mac_vap_stru *pst_mac_vap, oal_uint16 *us_len, oal_uint8 *puc_param)
{
    oal_uint8                   uc_user_idx;
    mac_user_stru              *pst_mac_user = OAL_PTR_NULL;
    mac_res_user_hash_stru     *pst_res_hash = OAL_PTR_NULL;
    oal_dlist_head_stru        *pst_head = OAL_PTR_NULL;
    oal_int8                    ac_tmp_buff[256]    = {0};
    oal_int32                   l_remainder_len;
    oal_int8                   *pc_sta_list_buff = OAL_PTR_NULL;
    oal_netbuf_stru*            pst_netbuf = OAL_PTR_NULL;
    oal_uint32                  ul_netbuf_len;

    /* �¼�����ָ��ֵ���˴��쳣����ǰ������ΪNULL */
    *(oal_ulong*)puc_param = (oal_ulong)OAL_PTR_NULL;

    /* AP�����Ϣ���ܴ�ӡ�����Ϣ */
    if (pst_mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_AP) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_sta_list::invalid en_vap_mode[%d].}",
                         pst_mac_vap->en_vap_mode);
        return OAL_FAIL;
    }

    pc_sta_list_buff = (oal_int8 *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAM_REPORT_MAX_STRING_LEN, OAL_TRUE);
    if (pc_sta_list_buff == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_get_sta_list, OAL_MEM_ALLOC failed.\n");
        return OAL_ERR_CODE_PTR_NULL;
    }
    memset_s(pc_sta_list_buff, OAM_REPORT_MAX_STRING_LEN, 0, OAM_REPORT_MAX_STRING_LEN);
    l_remainder_len = (oal_int32)(OAM_REPORT_MAX_STRING_LEN - OAL_STRLEN(pc_sta_list_buff));

    /* AP���USER��Ϣ */
    for (uc_user_idx = 0; uc_user_idx < MAC_VAP_USER_HASH_MAX_VALUE; uc_user_idx++) {
        pst_head = pst_mac_vap->ast_user_hash[uc_user_idx].pst_next;

        while ((pst_head != OAL_PTR_NULL) && ((&pst_mac_vap->ast_user_hash[uc_user_idx]) != pst_head)) {
            pst_res_hash = (mac_res_user_hash_stru *)pst_head;

            /* �ҵ���Ӧ�û� */
            pst_mac_user = mac_res_get_mac_user(pst_res_hash->us_user_idx);
            if (pst_mac_user == OAL_PTR_NULL) {
                OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_sta_list::pst_mac_user null.}");
                pst_head = pst_res_hash->st_entry.pst_next;
                continue;
            }

            /* ����û�����״̬ */
            if (pst_mac_user->en_user_asoc_state != MAC_USER_STATE_ASSOC) {
                pst_head = pst_res_hash->st_entry.pst_next;
                continue;
            }

            if (snprintf_s(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff), OAL_SIZEOF(ac_tmp_buff) - 1,
                           "%02X:%02X:%02X:%02X:%02X:%02X\n",
                           pst_mac_user->auc_user_mac_addr[0], pst_mac_user->auc_user_mac_addr[1],
                           pst_mac_user->auc_user_mac_addr[2], pst_mac_user->auc_user_mac_addr[3],
                           pst_mac_user->auc_user_mac_addr[4], pst_mac_user->auc_user_mac_addr[5]) < EOK) {
                OAL_MEM_FREE(pc_sta_list_buff, OAL_TRUE);
                OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_list_sta::snprintf_s fail.}");
                return OAL_FAIL;
            }

            OAL_IO_PRINT("hmac_config_get_sta_list,STA:%02X:XX:XX:%02X:%02X:%02X\n",
                         pst_mac_user->auc_user_mac_addr[0], pst_mac_user->auc_user_mac_addr[3],
                         pst_mac_user->auc_user_mac_addr[4], pst_mac_user->auc_user_mac_addr[5]);

            if (strncat_s(pc_sta_list_buff, OAM_REPORT_MAX_STRING_LEN, ac_tmp_buff, l_remainder_len - 1) != EOK) {
                OAM_ERROR_LOG0(0, OAM_SF_CFG, "hmac_config_get_sta_list::strncat_s error!");
                OAL_MEM_FREE(pc_sta_list_buff, OAL_TRUE);
                return OAL_FAIL;
            }
            memset_s(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff), 0, OAL_SIZEOF(ac_tmp_buff));
            l_remainder_len = (oal_int32)(OAM_REPORT_MAX_STRING_LEN - OAL_STRLEN(pc_sta_list_buff));
            pst_head = pst_res_hash->st_entry.pst_next;
        }
    }

    ul_netbuf_len = OAL_STRLEN(pc_sta_list_buff);
    pst_netbuf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, ul_netbuf_len, OAL_NETBUF_PRIORITY_MID);
    if (pst_netbuf != OAL_PTR_NULL) {
        if (memcpy_s(oal_netbuf_put(pst_netbuf, ul_netbuf_len), ul_netbuf_len, pc_sta_list_buff,
            ul_netbuf_len) != EOK) {
            OAM_ERROR_LOG0(0, OAM_SF_CFG, "hmac_config_get_sta_list::memcpy_s fail!");
            OAL_MEM_FREE(pc_sta_list_buff, OAL_TRUE);
            oal_netbuf_free(pst_netbuf);
            return OAL_FAIL;
        }
    } else {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "hmac_config_get_sta_list::Alloc netbuf(size %d)NULL in normal_netbuf_pool!", ul_netbuf_len);
    }

    *(oal_ulong*)puc_param = (oal_ulong)(uintptr_t)pst_netbuf;

    /* �¼�����ָ�룬�˴���¼ָ�볤�� */
    *us_len = (oal_uint16)OAL_SIZEOF(oal_netbuf_stru*);

    OAL_MEM_FREE(pc_sta_list_buff, OAL_TRUE);
    return OAL_SUCC;
}


oal_uint32  hmac_config_dump_ba_bitmap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_CONFIG) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_dump_ba_bitmap::config vap do not have ba bitmap.}");
        return OAL_FAIL;
    }

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_DUMP_BA_BITMAP, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_dump_ba_bitmap::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_dump_all_rx_dscr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_DUMP_ALL_RX_DSCR, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_dump_all_rx_dscr::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_vap_pkt_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return OAL_SUCC;
}


oal_uint32  hmac_config_set_country(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_country_stru      *pst_country_param;
    mac_regdomain_info_stru   *pst_mac_regdom;
    mac_device_stru           *pst_mac_device = OAL_PTR_NULL;
    oal_uint32                 ul_ret;

    oal_uint8                uc_rc_num;
    oal_uint32               ul_size;

    pst_country_param = (mac_cfg_country_stru *)puc_param;
    pst_mac_regdom    = (mac_regdomain_info_stru *)pst_country_param->p_mac_regdom;

    mac_regdomain_set_country(us_len, (uint8_t*)puc_param);

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL)) {
        OAL_MEM_FREE(pst_mac_regdom, OAL_TRUE);
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_country::pst_mac_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

#ifdef _PRE_WLAN_FEATURE_DFS
    hmac_dfs_channel_list_init(pst_mac_device);
#endif

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    /* ��ȡ������ĸ��� */
    uc_rc_num = pst_mac_regdom->uc_regclass_num;

    /* ������������ */
    ul_size = (oal_uint32)(OAL_SIZEOF(mac_regclass_info_stru) * uc_rc_num + MAC_RD_INFO_LEN);

    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_COUNTRY, (oal_uint16)ul_size, (oal_uint8 *)pst_mac_regdom);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAL_MEM_FREE(pst_mac_regdom, OAL_TRUE);
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_country::hmac_config_send_event failed[%d].}", ul_ret);

        return ul_ret;
    }

    /* WAL�����ڴ��������˴��ͷ� */
    OAL_MEM_FREE(pst_mac_regdom, OAL_TRUE);

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_amsdu_tx_on(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
#ifdef _PRE_WLAN_FEATURE_AMSDU
    mac_cfg_ampdu_tx_on_param_stru *pst_ampdu_tx_on_param;
    hmac_vap_stru                  *pst_hmac_vap;

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_config_set_amsdu_tx_on:: parma null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_amsdu_tx_on::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_ampdu_tx_on_param = (mac_cfg_ampdu_tx_on_param_stru *)puc_param;

    pst_hmac_vap->en_amsdu_active = pst_ampdu_tx_on_param->uc_aggr_tx_on;
#endif

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_ampdu_tx_on(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_ampdu_tx_on_param_stru     *pst_ampdu_tx_on_param = OAL_PTR_NULL;
    hmac_vap_stru                      *pst_hmac_vap = OAL_PTR_NULL;

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_set_ampdu_tx_on:: param null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_ampdu_tx_on::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_ampdu_tx_on_param = (mac_cfg_ampdu_tx_on_param_stru *)puc_param;

    pst_hmac_vap->en_ampdu_tx_on_switch = pst_ampdu_tx_on_param->uc_aggr_tx_on;
    OAM_INFO_LOG1(0, OAM_SF_CFG, "{hmac_config_set_ampdu_tx_on::en_tx_aggr_on[%d] null!}", pst_hmac_vap->en_tx_aggr_on);

    return OAL_SUCC;
}



oal_uint32  hmac_config_hide_ssid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint8             uc_hide_ssid;
    oal_uint32            ul_ret;

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_config_hide_ssid::param null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_hide_ssid = *(oal_uint8 *)puc_param;
    mac_vap_set_hide_ssid(pst_mac_vap, uc_hide_ssid);
    OAM_ERROR_LOG1(0, OAM_SF_CFG, "{dmac_config_hide_ssid::mac_vap_set_hide_ssid [%d].}", uc_hide_ssid);

    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_HIDE_SSID, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_hide_ssid::hmac_config_send_event failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_DFS // 1131_debug

oal_uint32  hmac_config_set_country_for_dfs(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                 ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_COUNTRY_FOR_DFS, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_country_for_dfs::hmac_config_send_event failed[%d].}", ul_ret);

        return ul_ret;
    }

    return OAL_SUCC;
}
#endif


oal_uint32  hmac_config_set_regdomain_pwr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_regdomain_max_pwr_stru *pst_cfg;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_uint32                      ul_ret;
#endif
    pst_cfg = (mac_cfg_regdomain_max_pwr_stru *)puc_param;

    mac_regdomain_set_max_power(pst_cfg->uc_pwr, pst_cfg->en_exceed_reg);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_REGDOMAIN_PWR, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_regdomain_pwr::hmac_config_send_event failed[%d].}", ul_ret);

        return ul_ret;
    }

#endif
    return OAL_SUCC;
}

oal_uint32  hmac_config_reduce_sar(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;
    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_REDUCE_SAR, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "hmac_config_reduce_sar::hmac_config_send_event failed, error no[%d]!", ul_ret);
        return ul_ret;
    }
    return OAL_SUCC;
}

oal_uint32  hmac_config_get_country(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    mac_regdomain_info_stru  *regdomain_info = NULL;
    mac_cfg_get_country_stru *param          = NULL;

    param = (mac_cfg_get_country_stru *)puc_param;

    mac_get_regdomain_info(&regdomain_info);
    param->ac_country[0] = regdomain_info->ac_country[0];
    param->ac_country[1] = regdomain_info->ac_country[1];
    param->ac_country[2] = regdomain_info->ac_country[2];
    *pus_len = WLAN_COUNTRY_STR_LEN;
#endif
    OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_get_country");

    return OAL_SUCC;
}
OAL_STATIC void hmac_get_security_param_from_conn(mac_cfg80211_connect_security_stru *conn_sec,
    mac_cfg80211_connect_param_stru *connect_param)
{
    int32_t ret;

    conn_sec->uc_wep_key_len   = connect_param->uc_wep_key_len;
    conn_sec->en_auth_type     = connect_param->en_auth_type;
    conn_sec->en_privacy       = connect_param->en_privacy;
    conn_sec->st_crypto        = connect_param->st_crypto;
    conn_sec->uc_wep_key_index = connect_param->uc_wep_key_index;
    conn_sec->en_mgmt_proteced = connect_param->en_mfp;
    if (conn_sec->uc_wep_key_len > WLAN_WEP104_KEY_LEN) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY,
            "{hmac_config_connect:: wep_key_len[%d] > WLAN_WEP104_KEY_LEN!}", conn_sec->uc_wep_key_len);
        conn_sec->uc_wep_key_len = WLAN_WEP104_KEY_LEN;
    }
    ret = memcpy_s(conn_sec->auc_wep_key, WLAN_WEP104_KEY_LEN, connect_param->puc_wep_key,
        conn_sec->uc_wep_key_len);
    if (ret != EOK) {
        OAM_WARNING_LOG1(0, OAM_SF_WPA, "hmac_get_security_param_from_conn::memcpy_s fail! l_ret[%d]", ret);
    }

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
    conn_sec->en_pmf_cap = mac_get_pmf_cap(connect_param->puc_ie, connect_param->ul_ie_len);
#endif
    conn_sec->en_wps_enable = OAL_FALSE;
    if (mac_find_vendor_ie(MAC_WLAN_OUI_MICROSOFT, MAC_WLAN_OUI_TYPE_MICROSOFT_WPS, connect_param->puc_ie,
        (int32_t)(connect_param->ul_ie_len))) {
        conn_sec->en_wps_enable = OAL_TRUE;
    }
}

oal_uint32  hmac_config_connect(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                          ul_ret;
    hmac_vap_stru                      *pst_hmac_vap = OAL_PTR_NULL;
    mac_bss_dscr_stru                  *pst_bss_dscr = OAL_PTR_NULL;
    mac_cfg80211_connect_param_stru    *pst_connect_param = OAL_PTR_NULL;
#ifdef _PRE_WLAN_FEATURE_11R
    oal_uint8                          *puc_mde;
#endif
    oal_app_ie_stru                     st_app_ie;
    mac_cfg80211_connect_security_stru  st_conn_sec = {0};
    hmac_scanned_bss_info              *pst_scanned_bss_info = OAL_PTR_NULL;
    hmac_device_stru                   *pst_hmac_device = OAL_PTR_NULL;
    hmac_bss_mgmt_stru                 *pst_bss_mgmt = OAL_PTR_NULL;          /* ����ɨ���bss����Ľṹ�� */
#ifdef _PRE_WLAN_FEATURE_WAPI
    mac_device_stru                    *pst_mac_device = OAL_PTR_NULL;
#endif
    hmac_user_stru                     *pst_hmac_user = OAL_PTR_NULL;
    oal_uint32                          l_ret         = EOK;

    if (puc_param == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_config_connect:: connect failed, null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_mac_vap == NULL || us_len != OAL_SIZEOF(mac_cfg80211_connect_param_stru)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_config_connect:: connect failed,unexpected param len ! [%x]!}", us_len);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ���ж�����VAP ��״̬�Ƿ�����VAP �������� */
    /* �������VAP �������򷵻��豸æ״̬ */
    pst_connect_param = (mac_cfg80211_connect_param_stru *)puc_param;

    /* �����ں��·��Ĺ�����������ֵ������ص�mib ֵ */
    /* �����·���join,��ȡ����ȫ��ص����� */
    hmac_get_security_param_from_conn(&st_conn_sec, pst_connect_param);

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_connect::connect failed,"
            "pst_hmac_vap null.uc_vap_id[%d]}", pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (((pst_mac_vap->en_vap_state >= MAC_VAP_STATE_STA_JOIN_COMP) &&
        (pst_mac_vap->en_vap_state <= MAC_VAP_STATE_STA_WAIT_ASOC))
#ifdef _PRE_WLAN_FEATURE_ROAM
        || (pst_mac_vap->en_vap_state == MAC_VAP_STATE_ROAMING)
#endif  /* _PRE_WLAN_FEATURE_ROAM */
        || (pst_mac_vap->en_vap_state == MAC_VAP_STATE_UP)) {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,
            "hmac_config_connect:vap is connecting or connected.vap_state[%d]", pst_mac_vap->en_vap_state);
        /* ��ȡ�û�ָ�� */
        pst_hmac_user = mac_res_get_hmac_user((oal_uint16)pst_hmac_vap->st_vap_base_info.uc_assoc_vap_id);
        if (pst_hmac_user != NULL) {
            /* ����ȥ��֤֡��AP */
            hmac_mgmt_send_deauth_frame(pst_mac_vap, pst_hmac_user->st_user_base_info.auc_user_mac_addr,
                MAC_AUTH_NOT_VALID, OAL_FALSE);

            /* ɾ����Ӧ�û� */
            hmac_user_del(pst_mac_vap, pst_hmac_user);
        } else {
            OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,
                             "hmac_config_connect:pst_hmac_user NULL.");
        }

        /* ����״̬ΪFAKE UP */
        hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_STA_FAKE_UP);

        /* ͬ��DMAC״̬ */
        hmac_send_connect_result_to_dmac_sta(pst_hmac_vap, OAL_FAIL);
    }
    pst_hmac_vap->en_auth_mode = st_conn_sec.en_auth_type;

    /* ��ȡhmac device �ṹ */
    pst_hmac_device = hmac_res_get_mac_dev(pst_mac_vap->uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_scan_find_scanned_bss_by_bssid::pst_hmac_device is null.}");
        return OAL_PTR_NULL;
    }

    /* ��ȡ����ɨ���bss����Ľṹ�� */
    pst_bss_mgmt = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.st_bss_mgmt);
    /* ������ɾ����ǰ���� */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));
    pst_scanned_bss_info = hmac_scan_find_scanned_bss_by_bssid(pst_bss_mgmt, pst_connect_param->puc_bssid);
    if (pst_scanned_bss_info == OAL_PTR_NULL) {
        OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_connect::find the bss failed by bssid:%02X:XX:XX:%02X:%02X:%02X}",
            pst_connect_param->puc_bssid[0], pst_connect_param->puc_bssid[3],
            pst_connect_param->puc_bssid[4], pst_connect_param->puc_bssid[5]);

        /* ���� */
        oal_spin_unlock(&(pst_bss_mgmt->st_lock));
        return OAL_FAIL;
    }

    if (oal_memcmp(pst_connect_param->puc_ssid, pst_scanned_bss_info->st_bss_dscr_info.ac_ssid,
        (oal_uint32)pst_connect_param->uc_ssid_len)) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_connect::find the bss failed by ssid.}");
        /* ���� */
        oal_spin_unlock(&(pst_bss_mgmt->st_lock));
        return OAL_FAIL;
    }

    pst_bss_dscr = &(pst_scanned_bss_info->st_bss_dscr_info);

    if (oal_memcmp(pst_mac_vap->auc_bssid, pst_connect_param->puc_bssid, OAL_MAC_ADDR_LEN)) {
        pst_hmac_vap->bit_reassoc_flag = OAL_FALSE;
    } else {
        pst_hmac_vap->bit_reassoc_flag = OAL_TRUE;
    }
    /* ����ع������������� */
    pst_hmac_vap->bit_reassoc_flag = OAL_FALSE;
#ifdef _PRE_WLAN_FEATURE_WAPI
    pst_bss_dscr->uc_wapi = pst_connect_param->uc_wapi;
    if (pst_bss_dscr->uc_wapi) {
        pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
        if (pst_mac_device == OAL_PTR_NULL) {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_connect::connect failed,"
                "pst_mac_device null! uc_device_id[%d]}", pst_mac_vap->uc_device_id);
            oal_spin_unlock(&(pst_bss_mgmt->st_lock));
            return OAL_ERR_CODE_MAC_DEVICE_NULL;
        }

        if (mac_device_is_p2p_connected(pst_mac_device) == OAL_SUCC) {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wapi connect failed for p2p have been connected!}");
            oal_spin_unlock(&(pst_bss_mgmt->st_lock));

            return OAL_FAIL;
        }
    }
#endif

    /* ���� */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    /* ����P2P/WPS IE ��Ϣ�� vap �ṹ���� */
    if (IS_LEGACY_VAP(pst_mac_vap)) {
        hmac_config_del_p2p_ie(pst_connect_param->puc_ie, &(pst_connect_param->ul_ie_len));
    }
    if (pst_connect_param->ul_ie_len > WLAN_WPS_IE_MAX_SIZE) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_connect::connect ie is too big to save.[%d]!}",
            pst_connect_param->ul_ie_len);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    st_app_ie.ul_ie_len      = pst_connect_param->ul_ie_len;
    l_ret += memcpy_s(st_app_ie.auc_ie, WLAN_WPS_IE_MAX_SIZE, pst_connect_param->puc_ie, st_app_ie.ul_ie_len);
    st_app_ie.en_app_ie_type = OAL_APP_ASSOC_REQ_IE;
    hmac_config_set_app_ie_to_vap(pst_mac_vap, &st_app_ie, st_app_ie.en_app_ie_type);

    pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.ul_dot11DTIMPeriod = pst_bss_dscr->uc_dtim_period;

    /* ���ù����û���������Ϣ */
    pst_mac_vap->us_assoc_user_cap_info = pst_bss_dscr->us_cap_info;
    pst_mac_vap->bit_ap_11ntxbf         = (pst_bss_dscr->en_11ntxbf == OAL_TRUE) ? 1 : 0;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* ����ѡ����ȵ�rssi��ͬ����dmac����tpc�㷨��������tpc */
    st_conn_sec.c_rssi = pst_scanned_bss_info->st_bss_dscr_info.c_rssi;
#endif /* _PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE */

    ul_ret = mac_vap_init_privacy(pst_mac_vap, &st_conn_sec);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_connect::mac_11i_init_privacy failed[%d]!}",
            ul_ret);
        return ul_ret;
    }

    pst_hmac_vap->en_wps_active = st_conn_sec.en_wps_enable;

#ifdef _PRE_WLAN_FEATURE_11R
    puc_mde = mac_find_ie(MAC_EID_MOBILITY_DOMAIN, pst_bss_dscr->auc_mgmt_buff + MAC_80211_FRAME_LEN + MAC_SSID_OFFSET,
        pst_bss_dscr->ul_mgmt_len - MAC_80211_FRAME_LEN - MAC_SSID_OFFSET);
    if (puc_mde != OAL_PTR_NULL) {
        l_ret += memcpy_s(st_conn_sec.auc_mde, sizeof(st_conn_sec.auc_mde), puc_mde, puc_mde[1] + MAC_IE_HDR_LEN);
    }
    ul_ret = mac_mib_init_ft_cfg(pst_mac_vap, puc_mde);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                       "{hmac_config_connect::mac_mib_init_ft_cfg fail[%d]!}", ul_ret);
        return ul_ret;
    }
#endif // _PRE_WLAN_FEATURE_11R
    if (l_ret != EOK) {
        OAM_WARNING_LOG1(0, OAM_SF_WPA, "hmac_config_connect::memcpy_s fail! l_ret[%d]", l_ret);
    }
#ifdef _PRE_WLAN_FEATURE_SAE
    hmac_update_sae_connect_param(pst_hmac_vap, st_app_ie.auc_ie, st_app_ie.ul_ie_len);
#endif /* _PRE_WLAN_FEATURE_SAE */
    ul_ret = hmac_check_capability_mac_phy_supplicant(pst_mac_vap, pst_bss_dscr);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_connect::check mac and phy"
            "capability fail[%d]!}", ul_ret);
    }

    /***************************************************************************
    ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_CONNECT_REQ, OAL_SIZEOF(st_conn_sec),
        (oal_uint8 *)&st_conn_sec);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                       "{hmac_config_connect::hmac_config_send_event failed[%d].}", ul_ret);
        return ul_ret;
    }

    return hmac_sta_initiate_join(pst_mac_vap, pst_bss_dscr);
}


#ifdef _PRE_WLAN_FEATURE_11D

oal_uint32  hmac_config_set_rd_by_ie_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_bool_enum_uint8       *pc_param;
    hmac_vap_stru   *pst_hmac_vap;

    pc_param = (oal_bool_enum_uint8 *)puc_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(0);
    if (OAL_UNLIKELY(pst_hmac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_BA, "{hmac_config_set_rd_by_ie_switch::pst_mac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_hmac_vap->en_updata_rd_by_ie_switch = *pc_param;

    return OAL_SUCC;
}
#endif

oal_uint32  hmac_config_get_tid(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    mac_device_stru          *pst_mac_dev;
    mac_cfg_get_tid_stru     *pst_tid;

    pst_tid = (mac_cfg_get_tid_stru *)puc_param;
    pst_mac_dev = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_mac_dev == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_tid->en_tid = pst_mac_dev->en_tid;
    *pus_len = OAL_SIZEOF(pst_tid->en_tid);

    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_tid::en_tid=%d.}", pst_tid->en_tid);
    return OAL_SUCC;
}


oal_uint32  hmac_config_list_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint8                uc_chan_num;
    oal_uint8                uc_chan_idx;
    oal_uint32               ul_ret;

    if (pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_list_channel::null param}");
        return OAL_FAIL;
    }

    for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_2_BUTT; uc_chan_idx++) {
        ul_ret = mac_is_channel_idx_valid(MAC_RC_START_FREQ_2, uc_chan_idx);
        if (ul_ret == OAL_SUCC) {
            mac_get_channel_num_from_idx(MAC_RC_START_FREQ_2, uc_chan_idx, &uc_chan_num);

            /* ���2G�ŵ��� */
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_list_channel::2gCHA.NO=%d}", uc_chan_num);
        }
    }

#ifdef _PRE_WLAN_FEATURE_5G
    for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_5_BUTT / 2; uc_chan_idx++) {
        ul_ret = mac_is_channel_idx_valid(MAC_RC_START_FREQ_5, uc_chan_idx);
        if (ul_ret == OAL_SUCC) {
            mac_get_channel_num_from_idx(MAC_RC_START_FREQ_5, uc_chan_idx, &uc_chan_num);

#ifdef _PRE_WLAN_FEATURE_DFS
            /* ���5G 36~120�ŵ��ϵ�DFS�״��� */
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_list_channel::5gCHA.NO=%d,"
                "DFS_REQUIRED[%c]}\n", uc_chan_num, ((mac_is_ch_in_radar_band(MAC_RC_START_FREQ_5, uc_chan_idx) ==
                OAL_TRUE) ? 'Y' : 'N'));
#endif
        }
    }

    for (uc_chan_idx = MAC_CHANNEL_FREQ_5_BUTT / 2; uc_chan_idx < MAC_CHANNEL_FREQ_5_BUTT; uc_chan_idx++) {
        ul_ret = mac_is_channel_idx_valid(MAC_RC_START_FREQ_5, uc_chan_idx);
        if (ul_ret == OAL_SUCC) {
            mac_get_channel_num_from_idx(MAC_RC_START_FREQ_5, uc_chan_idx, &uc_chan_num);
#ifdef _PRE_WLAN_FEATURE_DFS
            /* ���5G 124~196�ŵ��ϵ�DFS�״��� */
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_list_channel::5gCHA.NO=%d,"
                "DFS_REQUIRED[%c]}\n", uc_chan_num, ((mac_is_ch_in_radar_band(MAC_RC_START_FREQ_5, uc_chan_idx) ==
                OAL_TRUE) ? 'Y' : 'N'));
#endif
        }
    }
#endif /* _PRE_WLAN_FEATURE_5G */

    return OAL_SUCC;
}

OAL_STATIC void hmac_config_remove_operating_class_ie(mac_vap_stru *mac_vap, oal_app_ie_stru *app_ie)
{
    uint8_t   *temp_ie = NULL;
    uint32_t  remain_len;

    temp_ie = mac_find_ie(MAC_EID_OPERATING_CLASS, app_ie->auc_ie, (uint32_t)app_ie->ul_ie_len);
    if ((temp_ie != NULL) &&
        (!mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11ExtendedChannelSwitchActivated)) {
        app_ie->ul_ie_len -= (uint32_t)(temp_ie[1] + MAC_IE_HDR_LEN);
        remain_len = app_ie->ul_ie_len - (uint32_t)(temp_ie - app_ie->auc_ie);
        if (memmove_s(temp_ie, remain_len, temp_ie + (uint32_t)(temp_ie[1] + MAC_IE_HDR_LEN), remain_len) != EOK) {
            OAM_WARNING_LOG0(0, OAM_SF_CFG,
                             "hmac_config_remove_operating_class_ie::remove MAC_EID_OPERATING_CLASS fail!");
        }
    }
    return;
}


oal_uint32 hmac_config_set_app_ie_to_vap(mac_vap_stru *pst_mac_vap,
    oal_app_ie_stru *pst_app_ie, en_app_ie_type_uint8 en_type)
{
    oal_uint32           ul_ret;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_app_ie_stru      st_tmp_app_ie;
#endif
    oal_uint8                      *puc_ie = OAL_PTR_NULL;
    oal_uint32                      remain_len;

    if ((pst_mac_vap == OAL_PTR_NULL) || (pst_app_ie == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_set_app_ie_to_vap::scan failed, set ie null ptr}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �Ƴ��������ظ�MAC_EID_EXT_CAPS */
    puc_ie = mac_find_ie(MAC_EID_EXT_CAPS, pst_app_ie->auc_ie, (oal_int32)pst_app_ie->ul_ie_len);
    if (puc_ie != OAL_PTR_NULL) {
        pst_app_ie->ul_ie_len  -= (oal_uint32)(puc_ie[1] + MAC_IE_HDR_LEN);
        remain_len              = pst_app_ie->ul_ie_len - (oal_uint32)(puc_ie - pst_app_ie->auc_ie);
        if (memmove_s(puc_ie, remain_len, puc_ie + (oal_uint32)(puc_ie[1] + MAC_IE_HDR_LEN), remain_len) < 0) {
            OAM_ERROR_LOG0(0, OAM_SF_CFG, "hmac_config_set_app_ie_to_vap memmove puc_ie failed.");
            return OAL_ERR_CODE_PTR_NULL;
        }
    }

    hmac_config_remove_operating_class_ie(pst_mac_vap, pst_app_ie);

    ul_ret = mac_vap_save_app_ie(pst_mac_vap, pst_app_ie, en_type);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_app_ie_to_vap::mac_vap_save_app_ie"
            "failed[%d], en_type[%d], len[%d].}", ul_ret, en_type, pst_app_ie->ul_ie_len);
        return ul_ret;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    if (pst_app_ie->en_app_ie_type >= OAL_APP_ASSOC_REQ_IE) {
        /* ֻ��OAL_APP_BEACON_IE��OAL_APP_PROBE_REQ_IE��OAL_APP_PROBE_RSP_IE ����Ҫ���浽device */
        return OAL_SUCC;
    }
    st_tmp_app_ie.en_app_ie_type    = pst_app_ie->en_app_ie_type;
    st_tmp_app_ie.ul_ie_len         = pst_app_ie->ul_ie_len;

    /* ���·���ie���ͺͳ��ȱ��浽auc_buffer �У��������¼��·���DMAC */
    if (memcpy_s(st_tmp_app_ie.auc_ie, WLAN_WPS_IE_MAX_SIZE, pst_app_ie->auc_ie, pst_app_ie->ul_ie_len) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "hmac_config_set_app_ie_to_vap::memcpy_s fail!");
        return OAL_FAIL;
    }
    /***************************************************************************
     ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_WPS_P2P_IE, OAL_SIZEOF(oal_app_ie_stru),
        (oal_uint8 *)&st_tmp_app_ie);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_set_app_ie_to_vap::hmac_config_send_event fail[%d],vap id[%d].}",
            ul_ret, pst_mac_vap->uc_vap_id);
    }
#endif

    return ul_ret;
}


oal_uint32 hmac_config_set_wps_p2p_ie(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_app_ie_stru                *pst_wps_p2p_ie;
    hmac_vap_stru                  *pst_hmac_vap = OAL_PTR_NULL;
    oal_uint32                      ul_ret;

    pst_wps_p2p_ie = (oal_app_ie_stru *)puc_param;

    /* ����WPS/P2P ��Ϣ */
    ul_ret = hmac_config_set_app_ie_to_vap(pst_mac_vap, pst_wps_p2p_ie, pst_wps_p2p_ie->en_app_ie_type);
    if (ul_ret != OAL_SUCC) {
        return ul_ret;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_wps_p2p_ie::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ���beacon ��Ϣ���Ƿ���WPS ��ϢԪ�� */
    if (pst_wps_p2p_ie->en_app_ie_type == OAL_APP_BEACON_IE) {
        if (pst_wps_p2p_ie->ul_ie_len != 0 &&
            mac_find_vendor_ie(MAC_WLAN_OUI_MICROSOFT, MAC_WLAN_OUI_TYPE_MICROSOFT_WPS, pst_wps_p2p_ie->auc_ie,
                (oal_int32)(pst_wps_p2p_ie->ul_ie_len)) != OAL_PTR_NULL) {
            /* ����WPS ����ʹ�� */
            pst_hmac_vap->en_wps_active = OAL_TRUE;
            OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_wps_p2p_ie::set wps enable.}");
        } else {
            pst_hmac_vap->en_wps_active = OAL_FALSE;
        }
    }

    return ul_ret;
}


oal_uint32 hmac_config_set_wps_ie(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    hmac_vap_stru                  *pst_hmac_vap = OAL_PTR_NULL;
    oal_app_ie_stru                *pst_wps_ie;
    oal_uint8                      *puc_ie = OAL_PTR_NULL;
    oal_uint32                      ul_ret;

    pst_wps_ie = (oal_app_ie_stru *)puc_param;

    /* ����WPS ��Ϣ */
    ul_ret = hmac_config_set_app_ie_to_vap(pst_mac_vap, pst_wps_ie, pst_wps_ie->en_app_ie_type);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                         "{hmac_config_set_wps_ie::ul_ret=[%d].}",
                         ul_ret);
        return ul_ret;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_wps_ie::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ���beacon ��Ϣ���Ƿ���WPS ��ϢԪ�� */
    if ((pst_wps_ie->en_app_ie_type == OAL_APP_BEACON_IE) && (pst_wps_ie->ul_ie_len != 0)) {
        puc_ie = mac_find_vendor_ie(MAC_WLAN_OUI_MICROSOFT, MAC_WLAN_OUI_TYPE_MICROSOFT_WPS, pst_wps_ie->auc_ie,
            (oal_int32)(pst_wps_ie->ul_ie_len));
        if (puc_ie != OAL_PTR_NULL) {
            /* ����WPS ����ʹ�� */
            pst_hmac_vap->en_wps_active = OAL_TRUE;
            OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_wps_ie::set wps enable.}");
        }
    } else if ((pst_wps_ie->ul_ie_len == 0) && (pst_wps_ie->en_app_ie_type == OAL_APP_BEACON_IE)) {
        pst_hmac_vap->en_wps_active = OAL_FALSE;
        OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_wps_ie::set wps disable.}");
    }

    return ul_ret;
}


oal_uint32  hmac_config_pause_tid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_PAUSE_TID, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_pause_tid::hmac_config_send_event fail=%d.}",
            ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_dump_timer(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    frw_timer_dump_timer(pst_mac_vap->ul_core_id);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_DUMP_TIEMR, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_dump_timer::hmac_config_send_event failed[%d].}", ul_ret);
    }

#endif
    return ul_ret;
}



oal_uint32  hmac_config_set_user_vip(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_USER_VIP, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_user_vip::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_vap_host(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_VAP_HOST, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_vap_host::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_reg_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_REG_INFO, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_reg_info::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_dbb_scaling_amend(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                        ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_DBB_SCALING_AMEND, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_dbb_scaling_amend::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST))


oal_uint32  hmac_config_sdio_flowctrl(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32    ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SDIO_FLOWCTRL, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_sdio_flowctrl::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif


#ifdef _PRE_WLAN_FEATURE_HILINK

oal_uint32  hmac_config_set_monitor_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32    ul_ret;
    OAM_WARNING_LOG0(0, OAM_SF_ANY, "hmac_config_set_monitor_switch::in");
    if ((puc_param[0] == HMAC_MONITOR_SWITCH_MCAST_DATA) || (puc_param[0] == HMAC_MONITOR_SWITCH_UCAST_DATA)) {
        g_uc_data_switch = OAL_TRUE;
    } else if ((puc_param[0] == HMAC_MONITOR_SWITCH_MCAST_MANAGEMENT) ||
        (puc_param[0] == HMAC_MONITOR_SWITCH_UCAST_MANAGEMENT)) {
        g_uc_management_switch = OAL_TRUE;
    } else {
        g_uc_data_switch       = OAL_FALSE;
        g_uc_management_switch = OAL_FALSE;
    }
    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_MONITOR_EN, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_monitor_switch::hmac_config_send_event failed[%d].}", ul_ret);
    }
    return ul_ret;
}
#endif


oal_uint32 hmac_config_mips_cycle_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;

    /* ���¼���DMAC�㣬ͬ��DMAC���� */
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_MIPS_CYCLE_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_mips_cycle_switch::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_send_bar(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SEND_BAR, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_send_bar::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_reg_write(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_REG_WRITE, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_reg_write::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_alg_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    /***************************************************************************
        ���¼���ALG��, ͬ��ALG����
    ***************************************************************************/
#ifdef _PRE_WLAN_FEATURE_TXBF
    /* ͬ������txbf��mibֵ */
    mac_ioctl_alg_param_stru            *pst_alg_param;
    pst_alg_param = (mac_ioctl_alg_param_stru *)puc_param;

    if (pst_alg_param->en_alg_cfg == MAC_ALG_CFG_TXBF_TXBFEE_ENABLE) {
        pst_mac_vap->pst_mib_info->st_wlan_mib_txbf_config.en_dot11ReceiveStaggerSoundingOptionImplemented =
            pst_alg_param->ul_value;
        pst_mac_vap->pst_mib_info->st_wlan_mib_vht_txbf_config.en_dot11VHTSUBeamformeeOptionImplemented =
            pst_alg_param->ul_value;
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV)
        pst_mac_vap->pst_mib_info->st_wlan_mib_vht_txbf_config.en_dot11VHTMUBeamformeeOptionImplemented =
            pst_alg_param->ul_value;
#endif
        pst_mac_vap->pst_mib_info->st_wlan_mib_vht_txbf_config.ul_dot11VHTBeamformeeNTxSupport =
            pst_alg_param->ul_value;
        if (pst_mac_vap->pst_mib_info->st_wlan_mib_txbf_config.en_dot11ReceiveStaggerSoundingOptionImplemented ==
            OAL_TRUE) {
            pst_mac_vap->pst_mib_info->st_wlan_mib_txbf_config.uc_dot11ExplicitCompressedBeamformingFeedbackOptionImplemented = 1;
            pst_mac_vap->pst_mib_info->st_wlan_mib_txbf_config.ul_dot11NumberCompressedBeamformingMatrixSupportAntenna =
            1;
        } else {
            pst_mac_vap->pst_mib_info->st_wlan_mib_txbf_config.uc_dot11ExplicitCompressedBeamformingFeedbackOptionImplemented = 0;
            pst_mac_vap->pst_mib_info->st_wlan_mib_txbf_config.ul_dot11NumberCompressedBeamformingMatrixSupportAntenna =
            0;
        }
    } else if (pst_alg_param->en_alg_cfg == MAC_ALG_CFG_TXBF_RXSTBC_ENABLE) {
        pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11RxSTBCOptionImplemented = pst_alg_param->ul_value;
        pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.en_dot11VHTRxSTBCOptionImplemented = pst_alg_param->ul_value;
    } else if (pst_alg_param->en_alg_cfg == MAC_ALG_CFG_TXBF_11N_BFEE_ENABLE) {
        pst_mac_vap->st_cap_flag.bit_11ntxbf = pst_alg_param->ul_value;
    }

#endif

    return hmac_config_alg_send_event(pst_mac_vap, WLAN_CFGID_ALG_PARAM, us_len, puc_param);
}

#ifdef _PRE_WLAN_FEATURE_DFS


oal_uint32  hmac_config_dfs_radartool(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_device_stru       *pst_mac_device;
    oal_int8              *pc_token;
    oal_int8              *pc_end;
    oal_int8              *pc_ctx;
    oal_int8              *pc_sep = " ";
    oal_bool_enum_uint8    en_val;
    oal_uint32             ul_val;
    oal_uint32             ul_ret;
    oal_uint32             ul_copy_len;
    oal_int8               auc_param[WLAN_MEM_LOCAL_SIZE2];

    ul_copy_len = OAL_STRLEN((oal_int8 *)puc_param);
    if (ul_copy_len > WLAN_MEM_LOCAL_SIZE2 - 1) {
        ul_copy_len = WLAN_MEM_LOCAL_SIZE2 - 1;
    }
    if (memcpy_s(auc_param, WLAN_MEM_LOCAL_SIZE2, puc_param, ul_copy_len) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_DFS, "hmac_config_dfs_radartool::memcpy_s fail!");
        return OAL_FAIL;
    }
    auc_param[ul_copy_len] = '\0';

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ�������� */
    pc_token = oal_strtok((oal_int8 *)auc_param, pc_sep, &pc_ctx);
    if (OAL_UNLIKELY(pc_token == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (oal_strcmp(pc_token, "dfsenable") == 0) {
        /* ��ȡDFSʹ�ܿ��� */
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(pc_token == OAL_PTR_NULL)) {
            return OAL_ERR_CODE_PTR_NULL;
        }

        en_val = (oal_bool_enum_uint8)oal_strtol(pc_token, &pc_end, 10);

        mac_dfs_set_dfs_enable(pst_mac_device, en_val);
    } else if (oal_strcmp(pc_token, "cacenable") == 0) {
        /* ��ȡCAC���ʹ�ܿ��� */
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(pc_token == OAL_PTR_NULL)) {
            return OAL_ERR_CODE_PTR_NULL;
        }

        en_val = (oal_bool_enum_uint8)oal_strtol(pc_token, &pc_end, 10);

        mac_dfs_set_cac_enable(pst_mac_device, en_val);
    } else if (oal_strcmp(pc_token, "shownol") == 0) {
    } else if (oal_strcmp(pc_token, "cac") == 0) {
        /* ��ȡƵ�� */
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(pc_token == OAL_PTR_NULL)) {
            return OAL_ERR_CODE_PTR_NULL;
        }

        if (oal_strcmp(pc_token, "weather") == 0) {
            en_val = OAL_TRUE;
        } else if (oal_strcmp(pc_token, "nonweather") == 0) {
            en_val = OAL_FALSE;
        } else {
            return OAL_ERR_CODE_INVALID_CONFIG;
        }

        /* ��ȡCAC���ʱ�� */
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(pc_token == OAL_PTR_NULL)) {
            return OAL_ERR_CODE_PTR_NULL;
        }

        ul_val = (oal_uint32)oal_strtol(pc_token, &pc_end, 10);
        if ((ul_val > HMAC_DFS_ONE_MIN_IN_MS) && (ul_val % HMAC_DFS_ONE_MIN_IN_MS != 0)) {
            OAM_WARNING_LOG0(0, OAM_SF_DFS, "hmac_config_dfs_radartool::cac time should be a multiple of 1min.");
        }

        hmac_dfs_set_cac_time(pst_mac_device, ul_val, en_val);

        return OAL_SUCC;
    } else if (oal_strcmp(pc_token, "dfsdebug") == 0) {
        /* ��ȡdebug level */
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(pc_token == OAL_PTR_NULL)) {
            return OAL_ERR_CODE_PTR_NULL;
        }

        ul_val = (oal_uint32)oal_strtol(pc_token, &pc_end, 16);

        mac_dfs_set_debug_level(pst_mac_device, (oal_uint8)ul_val);
    } else if (oal_strcmp(pc_token, "offchanenable") == 0) {
        /* ��ȡOFF-CHAN CAC���ʹ�ܿ��� */
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(pc_token == OAL_PTR_NULL)) {
            return OAL_ERR_CODE_PTR_NULL;
        }

        en_val = (oal_bool_enum_uint8)oal_strtol(pc_token, &pc_end, 10);
        OAL_IO_PRINT("en_val = %u.\n", en_val);

        mac_dfs_set_offchan_cac_enable(pst_mac_device, en_val);
    } else if (oal_strcmp(pc_token, "offchannum") == 0) {
        /* ��ȡOFF-CHAN CAC����ŵ� */
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(pc_token == OAL_PTR_NULL)) {
            return OAL_ERR_CODE_PTR_NULL;
        }

        ul_val = (oal_bool_enum_uint8)oal_strtol(pc_token, &pc_end, 10);

        mac_dfs_set_offchan_number(pst_mac_device, ul_val);
    } else if (oal_strcmp(pc_token, "operntime") == 0) {
        /* ��ȡOFF-CHAN CAC��⹤���ŵ�פ��ʱ�� */
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(pc_token == OAL_PTR_NULL)) {
            return OAL_ERR_CODE_PTR_NULL;
        }

        ul_val = (oal_bool_enum_uint8)oal_strtol(pc_token, &pc_end, 10);

        hmac_dfs_set_opern_chan_time(pst_mac_device, ul_val);

        return OAL_SUCC;
    } else if (oal_strcmp(pc_token, "offchantime") == 0) {
        /* ��ȡOFF-CHAN CAC���OFF�ŵ�פ��ʱ�� */
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(pc_token == OAL_PTR_NULL)) {
            return OAL_ERR_CODE_PTR_NULL;
        }

        ul_val = (oal_bool_enum_uint8)oal_strtol(pc_token, &pc_end, 10);

        hmac_dfs_set_off_chan_time(pst_mac_device, ul_val);

        return OAL_SUCC;
    } else if (oal_strcmp(pc_token, "radarfilter") == 0) {
    } else if (oal_strcmp(pc_token, "ctsdura") == 0) {
    } else if (oal_strcmp(pc_token, "offcactime") == 0) {
        /* ��ȡOFF-CHAN CAC���ʱ�� */
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(pc_token == OAL_PTR_NULL)) {
            return OAL_ERR_CODE_PTR_NULL;
        }

        ul_val = (oal_uint32)oal_strtol(pc_token, &pc_end, 10);
        if (ul_val % HMAC_DFS_ONE_MIN_IN_MS != 0) {
            OAM_WARNING_LOG0(0, OAM_SF_DFS, "hmac_config_dfs_radartool::off cac time should be a multiple of 1min.");
        }

        pst_mac_device->st_dfs.st_dfs_info.ul_off_chan_cac_outof_5600_to_5650_time_ms = ul_val;

        return OAL_SUCC;
    } else if (oal_strcmp(pc_token, "enabletimer") == 0) {
    } else if (oal_strcmp(pc_token, "set_chan") == 0) {
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(pc_token == OAL_PTR_NULL)) {
            return OAL_ERR_CODE_PTR_NULL;
        }
        ul_val = (oal_uint32)oal_strtol(pc_token, &pc_end, 10);
        pst_mac_vap->st_ch_switch_info.uc_ch_switch_cnt = WLAN_CHAN_SWITCH_DEFAULT_CNT;
        hmac_dfs_set_channel(pst_mac_vap, (oal_uint8)ul_val);
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DFS, "{hmac_config_dfs_radartool::set channel to %d.}", ul_val);
        return OAL_SUCC;
    } else {
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_RADARTOOL, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_always_tx::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return OAL_SUCC;
}
#endif
#ifdef _PRE_SUPPORT_ACS
extern oal_uint32 hmac_acs_process_scan(mac_device_stru *pst_mac_dev, mac_vap_stru *pst_mac_vap,
    mac_scan_op_enum_uint8 en_op);

oal_uint32  hmac_config_acs(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_device_stru       *pst_mac_device;
    hmac_device_stru      *pst_hmac_device;
    oal_int8              *pc_token;
    oal_int8              *pc_end;
    oal_int8              *pc_ctx;
    oal_int8              *pc_sep = " ";
    oal_bool_enum_uint8    en_val;
    oal_uint32             ul_ret;
    oal_int8               auc_param[WLAN_MEM_LOCAL_SIZE2];
    oal_uint32             ul_len = OAL_STRLEN((oal_int8 *)puc_param);
    if (ul_len >= OAL_SIZEOF(auc_param)-1) {
        return OAL_FAIL;
    }

    if (memcpy_s(auc_param, WLAN_MEM_LOCAL_SIZE2, (oal_void *)puc_param, ul_len) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ACS, "hmac_config_acs::memcpy fail!");
        return OAL_FAIL;
    }
    auc_param[ul_len] = 0;

    pst_hmac_device = hmac_res_get_mac_dev(pst_mac_vap->uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL || pst_hmac_device->pst_device_base_info == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = pst_hmac_device->pst_device_base_info;

    /* ��ȡ�������� */
    pc_token = oal_strtok((oal_int8 *)auc_param, pc_sep, &pc_ctx);
    if (OAL_UNLIKELY(pc_token == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (oal_strcmp(pc_token, "sw") == 0) {
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(pc_token == OAL_PTR_NULL)) {
            OAL_IO_PRINT("error : hmac_config_acs\n");
            return OAL_ERR_CODE_PTR_NULL;
        }

        en_val = (oal_bool_enum_uint8)oal_strtol(pc_token, &pc_end, 10);
        if (en_val > MAC_ACS_SW_BUTT) {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ACS,
                "{hmac_config_acs::invalid switch=%d, force to NONE}", en_val);
            en_val = MAC_ACS_SW_NONE;
        }
        mac_set_acs_switch(pst_mac_device, en_val);
        OAL_IO_PRINT("set acs sw=%d\n", en_val);
    } else if (oal_strcmp(pc_token, "fscan") == 0) {
        OAL_IO_PRINT("fg scan\n");
        hmac_init_scan_process(pst_hmac_device, pst_mac_vap, MAC_SCAN_OP_FG_SCAN_ONLY);
    } else if (oal_strcmp(pc_token, "bscan") == 0) {
        OAL_IO_PRINT("bg scan\n");
        hmac_init_scan_process(pst_hmac_device, pst_mac_vap, MAC_SCAN_OP_BG_SCAN_ONLY);
    } else if (oal_strcmp(pc_token, "idle_scan") == 0) {
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (pc_token != OAL_PTR_NULL) {
            pst_hmac_device->en_rescan_idle = oal_strtol(pc_token, &pc_end, 10) ? OAL_TRUE : OAL_FALSE;
        }
    } else if (oal_strcmp(pc_token, "start_priv") == 0) {
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(pc_token == OAL_PTR_NULL)) {
            OAL_IO_PRINT("error : hmac_config_acs\n");
            return OAL_ERR_CODE_PTR_NULL;
        }

        en_val = (oal_bool_enum_uint8)oal_strtol(pc_token, &pc_end, 10);
        pst_hmac_device->en_start_via_priv = en_val ? OAL_TRUE : OAL_FALSE;
        OAL_IO_PRINT("set start priv=%d in hmac\n", pst_hmac_device->en_start_via_priv);
        return OAL_SUCC;
    } else {
        OAL_IO_PRINT("unknown acs cmd=[%s]\n", pc_token);
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_ACS_CONFIG, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_acs::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return OAL_SUCC;
}

#endif

oal_uint32  hmac_config_beacon_chain_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
    ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_BEACON_CHAIN_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_beacon_chain_switch::hmac_config_send_event failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_2040_channel_switch_prohibited(mac_vap_stru *pst_mac_vap, oal_uint16 us_len,
    const oal_uint8 *puc_param)
{
    mac_device_stru   *pst_device;
    oal_uint8          uc_vap_idx;
    hmac_vap_stru     *pst_hmac_vap = OAL_PTR_NULL;

    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_2040_channel_switch_prohibited::pst_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_device->uc_vap_num == 0) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_2040_channel_switch_prohibited::uc_vap_num=0.}");
        return OAL_FAIL;
    }

    /* ����device������vap������20/40�ŵ��л���ֹ���� */
    for (uc_vap_idx = 0; uc_vap_idx < pst_device->uc_vap_num; uc_vap_idx++) {
        pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_device->auc_vap_id[uc_vap_idx]);
        if (pst_hmac_vap == OAL_PTR_NULL) {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                "{hmac_config_2040_channel_switch_prohibited::pst_vap null.}");
            continue;
        }

        pst_hmac_vap->en_2040_switch_prohibited = *((oal_uint8 *)puc_param);
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_FortyMHzIntolerant(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    if ((*puc_param != 0) && (*puc_param != 1)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_FortyMHzIntolerant::invalid param[%d].",
            *puc_param);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    mac_mib_set_FortyMHzIntolerant(pst_mac_vap, (oal_bool_enum_uint8)(*puc_param));

    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_FortyMHzIntolerant::end func,puc_param=%d.}",
        *puc_param);
    return OAL_SUCC;
}


oal_uint32  hmac_config_set_2040_coext_support(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    if ((*puc_param != 0) && (*puc_param != 1)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_2040_coext_support::invalid param[%d].",
            *puc_param);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ���������VAP, ֱ�ӷ��� */
    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_CONFIG) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_2040_coext_support::this is config vap! can't set.}");
        return OAL_FAIL;
    }

    mac_mib_set_2040BSSCoexistenceManagementSupport(pst_mac_vap, (oal_bool_enum_uint8)(*puc_param));

    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_2040_coext_support::end func,puc_param=%d.}",
        *puc_param);
    return OAL_SUCC;
}


oal_uint32  hmac_config_rx_fcs_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    mac_cfg_rx_fcs_info_stru   *pst_rx_fcs_info;

    pst_rx_fcs_info = (mac_cfg_rx_fcs_info_stru *)puc_param;

    if (pst_rx_fcs_info->ul_print_info == 0) { /* ��ӡ���� */
        /* ��ӡ����֡������Ŀ */
        OAL_IO_PRINT("total frame num is: [0x%x] \n", g_ast_rx_fcs_statistic[pst_mac_vap->uc_vap_id].ul_total_num);

        /* ��ӡ���͸��Լ���FCS��ȷ��ͳ����Ŀ */
        OAL_IO_PRINT("self fcs correct: [0x%x] \n", g_ast_rx_fcs_statistic[pst_mac_vap->uc_vap_id].ul_self_fcs_correct);

        /* ��ӡ���Ƿ��͸��Լ���FCS��ȷ��ͳ����Ŀ */
        OAL_IO_PRINT("other fcs correct:[0x%x]\n", g_ast_rx_fcs_statistic[pst_mac_vap->uc_vap_id].ul_other_fcs_correct);

        /* ��ӡFCS�����ͳ����Ŀ */
        OAL_IO_PRINT("total fcs error: [0x%x] \n", g_ast_rx_fcs_statistic[pst_mac_vap->uc_vap_id].ul_total_fcs_error);
    } else if (pst_rx_fcs_info->ul_print_info == 1) { /* ��ӡ����֡������Ŀ */
        OAL_IO_PRINT("total frame num is: [0x%x] \n", g_ast_rx_fcs_statistic[pst_mac_vap->uc_vap_id].ul_total_num);
    } else if (pst_rx_fcs_info->ul_print_info == 2) { /* ��ӡ���͸��Լ���FCS��ȷ��ͳ����Ŀ */
        OAL_IO_PRINT("self fcs correct: [0x%x] \n", g_ast_rx_fcs_statistic[pst_mac_vap->uc_vap_id].ul_self_fcs_correct);
    } else if (pst_rx_fcs_info->ul_print_info == 3) { /* ��ӡ���Ƿ��͸��Լ���FCS��ȷ��ͳ����Ŀ */
        OAL_IO_PRINT("other fcs correct:[0x%x]\n", g_ast_rx_fcs_statistic[pst_mac_vap->uc_vap_id].ul_other_fcs_correct);
    } else if (pst_rx_fcs_info->ul_print_info == 4) { /* ��ӡFCS�����ͳ����Ŀ */
        OAL_IO_PRINT("total fcs error: [0x%x] \n", g_ast_rx_fcs_statistic[pst_mac_vap->uc_vap_id].ul_total_fcs_error);
    } else {
        OAL_IO_PRINT("hmac_config_rx_fcs_info:ul_print_info id wrong!, id is:[%d] \n", pst_rx_fcs_info->ul_print_info);
    }

    /* <1>���ԭʼ���ݣ�<0>�����ԭʼ���� */
    if (pst_rx_fcs_info->ul_data_op == 1) {
        g_ast_rx_fcs_statistic[pst_mac_vap->uc_vap_id].ul_total_num = 0;
        g_ast_rx_fcs_statistic[pst_mac_vap->uc_vap_id].ul_self_fcs_correct  = 0;
        g_ast_rx_fcs_statistic[pst_mac_vap->uc_vap_id].ul_other_fcs_correct = 0;
        g_ast_rx_fcs_statistic[pst_mac_vap->uc_vap_id].ul_total_fcs_error   = 0;

        OAL_IO_PRINT("the data has been reset! \n");
    } else if (pst_rx_fcs_info->ul_data_op != 0) {
        OAL_IO_PRINT("hmac_config_rx_fcs_info: ul_data_op id is wrong!, id is:[%d] \n", pst_rx_fcs_info->ul_data_op);
    }
#else
    oal_uint32          ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_RX_FCS_INFO, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_rx_fcs_info::hmac_config_send_event failed[%d].}", ul_ret);
        return ul_ret;
    }
#endif

    return OAL_SUCC;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

oal_uint32  hmac_config_resume_rx_intr_fifo(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_RESUME_RX_INTR_FIFO, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_resume_rx_intr_fifo::hmac_config_send_event failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}
#endif


#ifdef _PRE_WLAN_PERFORM_STAT


oal_uint32  hmac_config_pfm_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_PFM_STAT, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_pfm_stat::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_pfm_display(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_PFM_DISPLAY, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_pfm_display::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#endif

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP

oal_uint32  hmac_config_set_edca_opt_switch_sta(mac_vap_stru *pst_mac_vap, oal_uint16 us_len,
    const oal_uint8 *puc_param)
{
    oal_uint8        uc_flag;
    oal_uint32       ul_ret;
    hmac_vap_stru   *pst_hmac_vap    = OAL_PTR_NULL;

    /* ��ȡhmac_vap */
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_set_edca_opt_switch_sta, mac_res_get_hmac_vap fail.vap_id = %u",
            pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ���ò��� */
    uc_flag = *puc_param;

    /* ����û�и��ģ�����Ҫ�������� */
    if (uc_flag == pst_hmac_vap->uc_edca_opt_flag_sta) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_set_edca_opt_switch_sta, change nothing to flag:%d",
            pst_hmac_vap->uc_edca_opt_flag_sta);
        return OAL_SUCC;
    }

    /* ���ò���������������ֹͣedca����������ʱ�� */
    pst_hmac_vap->uc_edca_opt_flag_sta = uc_flag;

    if (pst_hmac_vap->uc_edca_opt_flag_sta == 0) {
        ul_ret = mac_vap_init_wme_param(pst_mac_vap);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "hmac_config_set_edca_opt_switch_sta: mac_vap_init_wme_param failed");
            return ul_ret;
        }

        OAM_WARNING_LOG0(0, OAM_SF_ANY, "mac_vap_init_wme_param succ");
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "hmac_edca_opt_adj_param_sta succ");
    }

    /* ����EDCA��ص�MAC�Ĵ��� */
    ul_ret = hmac_sta_up_update_edca_params_machw(pst_hmac_vap, MAC_WMM_SET_PARAM_TYPE_UPDATE_EDCA);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY,
            "hmac_config_set_edca_opt_switch_sta:hmac_sta_up_update_edca_params_machw failed");
        return ul_ret;
    }

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_set_edca_opt_switch_sta,config sucess, %d",
        pst_hmac_vap->uc_edca_opt_flag_sta);

    return OAL_SUCC;
}



oal_uint32  hmac_config_set_edca_opt_weight_sta(mac_vap_stru *pst_mac_vap, oal_uint16 us_len,
    const oal_uint8 *puc_param)
{
    oal_uint8        uc_weight;
    hmac_vap_stru   *pst_hmac_vap   = OAL_PTR_NULL;

    /* ��ȡhmac_vap */
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_set_edca_opt_weight_sta, mac_res_get_hmac_vap fail.vap_id = %u",
            pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_weight = *puc_param;

    /* �ж�edcaȨ���Ƿ��е��� */
    if (uc_weight == pst_hmac_vap->uc_edca_opt_weight_sta) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_set_edca_opt_weight_sta, change nothing to cycle:%d",
            pst_hmac_vap->uc_edca_opt_weight_sta);
        return OAL_SUCC;
    }

    /* ����Ȩ�� */
    pst_hmac_vap->uc_edca_opt_weight_sta = uc_weight;
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_set_edca_opt_weight_sta succ, wieight = %d",
        pst_hmac_vap->uc_edca_opt_weight_sta);

    return OAL_SUCC;
}



oal_uint32  hmac_config_set_edca_opt_switch_ap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint8        uc_flag;
    hmac_vap_stru   *pst_hmac_vap    = OAL_PTR_NULL;

    /* ��ȡhmac_vap */
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_set_edca_opt_switch_ap, mac_res_get_hmac_vap fail.vap_id = %u",
            pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ���ò��� */
    uc_flag = *puc_param;

    /* ����û�и��ģ�����Ҫ�������� */
    if (uc_flag == pst_hmac_vap->uc_edca_opt_flag_ap) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_hipriv_set_edca_opt_switch_ap, change nothing to flag:%d",
            pst_hmac_vap->uc_edca_opt_flag_ap);
        return OAL_SUCC;
    }

    /* ���ò���������������ֹͣedca����������ʱ�� */
    if (uc_flag == 1) {
        pst_hmac_vap->uc_edca_opt_flag_ap = 1;
        FRW_TIMER_RESTART_TIMER(&(pst_hmac_vap->st_edca_opt_timer), pst_hmac_vap->ul_edca_opt_time_ms, OAL_TRUE);
    } else {
        pst_hmac_vap->uc_edca_opt_flag_ap = 0;
        FRW_TIMER_STOP_TIMER(&(pst_hmac_vap->st_edca_opt_timer));
    }

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_set_edca_opt_switch_ap succ, flag = %d",
        pst_hmac_vap->uc_edca_opt_flag_ap);

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_edca_opt_cycle_ap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32       ul_cycle_ms;
    hmac_vap_stru   *pst_hmac_vap    = OAL_PTR_NULL;

    /* ��ȡhmac_vap */
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_set_edca_opt_cycle_ap, mac_res_get_hmac_vap fail.vap_id = %u",
            pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_cycle_ms = *((oal_uint32 *)puc_param);

    /* �ж�edca���������Ƿ��и��� */
    if (ul_cycle_ms == pst_hmac_vap->ul_edca_opt_time_ms) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_set_edca_opt_cycle_ap, change nothing to cycle:%d",
            pst_hmac_vap->ul_edca_opt_time_ms);
        return OAL_SUCC;
    }

    /* ���edca������ʱ���������У�����Ҫ��ֹͣ���ٸ����µĲ���restart */
    if (pst_hmac_vap->uc_edca_opt_flag_ap == 1) {
        pst_hmac_vap->ul_edca_opt_time_ms = ul_cycle_ms;
        FRW_TIMER_STOP_TIMER(&(pst_hmac_vap->st_edca_opt_timer));
        FRW_TIMER_RESTART_TIMER(&(pst_hmac_vap->st_edca_opt_timer), pst_hmac_vap->ul_edca_opt_time_ms, OAL_TRUE);
    } else { /* �����²������� */
        pst_hmac_vap->ul_edca_opt_time_ms = ul_cycle_ms;
    }

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_set_edca_opt_cycle_ap succ, cycle = %d",
        pst_hmac_vap->ul_edca_opt_time_ms);

    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_FEATURE_STA_PM

oal_uint32  hmac_config_set_pm_by_module(mac_vap_stru *pst_mac_vap, mac_pm_ctrl_type_enum pm_ctrl_type,
    mac_pm_switch_enum pm_enable)
{
    oal_uint32              ul_ret;
    mac_cfg_ps_open_stru    st_ps_open  = {0};

    if (pm_enable >= MAC_STA_PM_SWITCH_BUTT    ||
        pm_ctrl_type >= MAC_STA_PM_CTRL_TYPE_BUTT ||
        pst_mac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hmac_config_set_pm_by_module, PARAM ERROR!");
        return OAL_FAIL;
    }

    st_ps_open.uc_pm_enable      = pm_enable;
    st_ps_open.uc_pm_ctrl_type   = pm_ctrl_type;

#ifdef _PRE_WLAN_FEATURE_STA_PM
    ul_ret = hmac_config_set_sta_pm_on(pst_mac_vap, OAL_SIZEOF(mac_cfg_ps_open_stru), (oal_uint8 *)&st_ps_open);
#endif

    OAM_WARNING_LOG3(0, OAM_SF_PWR, "hmac_config_set_pm_by_module, pm_module = %d, pm_enable = %d, cfg ret = %d ",
                     pm_ctrl_type, pm_enable, ul_ret);

    return ul_ret;
}
#endif


oal_uint32  hmac_config_alg(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                   ul_ret;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    mac_user_stru               *pst_user = OAL_PTR_NULL;
    oal_int8                    *pac_argv[DMAC_ALG_CONFIG_MAX_ARG + 1] = {0};
    mac_ioctl_alg_config_stru   *pst_alg_config;
    oal_uint8                    uc_idx;
    oal_uint32                   ul_bw_limit_kbps;
    oal_dlist_head_stru         *pst_list_pos = OAL_PTR_NULL;
    uint8_t                     *tmp_param = (uint8_t*)puc_param;

    pst_alg_config = (mac_ioctl_alg_config_stru *)tmp_param;

    for (uc_idx = OAL_SIZEOF(mac_ioctl_alg_config_stru); uc_idx < us_len; uc_idx++) {
        if (tmp_param[uc_idx] == ' ') {
            tmp_param[uc_idx] = 0;
        }
    }

    for (uc_idx = 0; uc_idx < pst_alg_config->uc_argc; uc_idx++) {
        pac_argv[uc_idx] = (oal_int8 *)tmp_param + OAL_SIZEOF(mac_ioctl_alg_config_stru) + pst_alg_config->auc_argv_offset[uc_idx];
    }

    /* ���Ϊ�û����٣�����Ҫͬ��hmac_vap��״̬��Ϣ */
    if ((oal_strcmp(pac_argv[0], "sch") == 0) && (oal_strcmp(pac_argv[1], "usr_bw") == 0)) {
        pst_user = mac_vap_get_user_by_addr(pst_mac_vap, (oal_uint8*)(pac_argv[2]));
        if (pst_user == OAL_PTR_NULL) {
            OAM_ERROR_LOG0(0, OAM_SF_MULTI_TRAFFIC, "{alg_schedule_config_user_bw_limit: get pst_user failed}");
            return OAL_ERR_CODE_PTR_NULL;
        }

        ul_bw_limit_kbps = (oal_uint32)oal_atoi(pac_argv[3]);

        pst_mac_vap->bit_has_user_bw_limit = OAL_FALSE;
        for ((pst_list_pos) = (pst_mac_vap)->st_mac_user_list_head.pst_next, (pst_user) = OAL_DLIST_GET_ENTRY((pst_list_pos), mac_user_stru, st_user_dlist);
            (pst_list_pos) != &((pst_mac_vap)->st_mac_user_list_head);
            (pst_list_pos) = (pst_list_pos)->pst_next, (pst_user) = OAL_DLIST_GET_ENTRY((pst_list_pos), mac_user_stru, st_user_dlist)) {
            /* �������ֵ��Ϊ0,��ʾ��user�ѱ����٣������vap��״̬ */
            if ((pst_user != OAL_PTR_NULL) && (ul_bw_limit_kbps != 0)) {
                pst_mac_vap->bit_has_user_bw_limit = OAL_TRUE;
                break;
            }
        }
    }
#else
#ifdef _PRE_WLAN_FEATURE_DBAC
    {
        mac_device_stru *pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);

        if (pst_mac_device != OAL_PTR_NULL && mac_is_dbac_enabled(pst_mac_device) &&
            !oal_strcmp((const oal_int8 *)tmp_param + OAL_SIZEOF(mac_ioctl_alg_config_stru), " dbac start")) {
            if (hmac_init_scan_try(pst_mac_device, pst_mac_vap, MAC_TRY_INIT_SCAN_START_DBAC) == OAL_SUCC) {
                return OAL_SUCC;
            }
        }
    }
#endif

#endif

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_ALG, us_len, tmp_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_alg::hmac_config_send_event failed[%d].}",
            ul_ret);
    }

    return ul_ret;
}

#ifdef _PRE_WLAN_CHIP_TEST

oal_uint32  hmac_config_lpm_tx_data(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_lpm_tx_data_stru    *pst_lpm_tx_data;
    hmac_vap_stru               *pst_hmac_vap;
    oal_uint16                  us_send_num;
    oal_netbuf_stru             *pst_buf;
    mac_ether_header_stru       *pst_ether_header;
    mac_ip_header_stru          *pst_ip;
    oal_uint8                   uc_tid;
    oal_uint32                   ul_ret;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_alg::pst_hmac_vap null.}");
        return OAL_FAIL;
    }

    pst_lpm_tx_data = (mac_cfg_lpm_tx_data_stru *)puc_param;

    for (us_send_num = 0; us_send_num < pst_lpm_tx_data->us_num; us_send_num++) {
        /* ����SKB */
        pst_buf = oal_netbuf_alloc(pst_lpm_tx_data->us_len, 0, 4);
        oal_netbuf_put(pst_buf, pst_lpm_tx_data->us_len);

        oal_set_mac_addr(&pst_buf->data[0], pst_lpm_tx_data->auc_da);
        oal_set_mac_addr(&pst_buf->data[6], pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);

        pst_ether_header = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);
        /*lint -e778*/
        pst_ether_header->us_ether_type = OAL_HOST2NET_SHORT(ETHER_TYPE_IP);
        /*lint +e778*/
        pst_ip = (mac_ip_header_stru *)(pst_ether_header + 1);      /* ƫ��һ����̫��ͷ��ȡipͷ */

        uc_tid = WLAN_WME_AC_TO_TID(pst_lpm_tx_data->uc_ac);

        pst_ip->uc_version_ihl = 0x45;
        pst_ip->uc_tos = (oal_uint8)(uc_tid << WLAN_IP_PRI_SHIFT);
        pst_ip->us_tot_len = oal_byteorder_host_to_net_uint16(pst_lpm_tx_data->us_len - 34);
        pst_ip->us_id = oal_byteorder_host_to_net_uint16(0x4000);
        pst_ip->us_frag_off = 0;
        pst_ip->uc_ttl = 128;
        pst_ip->uc_protocol = 0x06;
        pst_ip->us_check = 0;
        pst_ip->ul_saddr = oal_byteorder_host_to_net_uint32(0x010101c4);
        pst_ip->ul_daddr = oal_byteorder_host_to_net_uint32(0x010101c2);

        pst_buf->next = OAL_PTR_NULL;
        pst_buf->prev = OAL_PTR_NULL;
        memset_s(oal_netbuf_cb(pst_buf), OAL_NETBUF_CB_SIZE(), 0, OAL_NETBUF_CB_SIZE());

        ul_ret = hmac_tx_lan_to_wlan(&pst_hmac_vap->st_vap_base_info, pst_buf);
        /* ����ʧ�ܣ�Ҫ�ͷ��ں������netbuff�ڴ�� */
        if (ul_ret != OAL_SUCC) {
            hmac_free_netbuf_list(pst_buf);
        }
    }

    return OAL_SUCC;
}

#endif


oal_uint32  hmac_config_set_rts_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_RTS_PARAM, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_rts_param::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_update_protection_tx_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len,
    const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_UPDTAE_PROT_TX_PARAM, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_update_protection_tx_param::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_protection(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    mac_dump_protection(pst_mac_vap, puc_param);

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_PROTECTION, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                         "{hmac_config_set_protection::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#ifdef _PRE_WLAN_CHIP_TEST

oal_uint32  hmac_config_set_coex(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /* оƬ��֤���ܣ��ݲ����浽device���� */
    /***************************************************************************
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_COEX, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_coex::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_dfx(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_DFX_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_dfx::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

oal_uint32  hmac_config_set_obss_scan_param(mac_vap_stru *pst_mac_vap)
{
    oal_uint32 ul_ret;

    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_OBSS_MIB,
                                    OAL_SIZEOF(wlan_mib_Dot11OperationEntry_stru),
                                    (oal_uint8*)&pst_mac_vap->pst_mib_info->st_wlan_mib_operation);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                         "{hmac_config_set_obss_scan_param::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif


oal_uint32  hmac_config_set_mib(mac_vap_stru *pst_mac_vap, wlan_cfgid_enum_uint16 en_cfg_id, oal_uint16 us_len,
    const oal_uint8 *puc_param)
{
    oal_uint32 ul_ret;

    /* ���������VAP, ֱ�ӷ��� */
    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_CONFIG) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_mib::this is config vap! can't set.}");
        return OAL_FAIL;
    }

    mac_config_set_mib(pst_mac_vap, us_len, (uint8_t*)puc_param);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_MIB, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_mib::hmac_config_send_event failed[%d].}", ul_ret);
    }
#endif

    return ul_ret;
}


OAL_STATIC uint32_t hmac_config_get_mib_default_case(mac_vap_stru *mac_vap, uint32_t *mib_value, uint32_t mib_idx)
{
    switch (mib_idx) {
        case WLAN_MIB_INDEX_OBSSSCAN_TRANSITION_DELAY_FACTOR:
            *mib_value =
                (uint32_t)mac_vap->pst_mib_info->st_wlan_mib_operation.ul_dot11BSSWidthChannelTransitionDelayFactor;
            break;
        case WLAN_MIB_INDEX_OBSSSCAN_PASSIVE_DWELL:
            *mib_value = (uint32_t)mac_vap->pst_mib_info->st_wlan_mib_operation.ul_dot11OBSSScanPassiveDwell;
            break;
        case WLAN_MIB_INDEX_OBSSSCAN_ACTIVE_DWELL:
            *mib_value = (uint32_t)mac_vap->pst_mib_info->st_wlan_mib_operation.ul_dot11OBSSScanActiveDwell;
            break;
        case WLAN_MIB_INDEX_OBSSSCAN_PASSIVE_TOTAL_PER_CHANNEL:
            *mib_value =
                (uint32_t)mac_vap->pst_mib_info->st_wlan_mib_operation.ul_dot11OBSSScanPassiveTotalPerChannel;
            break;
        case WLAN_MIB_INDEX_OBSSSCAN_ACTIVE_TOTAL_PER_CHANNEL:
            *mib_value =
                (uint32_t)mac_vap->pst_mib_info->st_wlan_mib_operation.ul_dot11OBSSScanActiveTotalPerChannel;
            break;
        case WLAN_MIB_INDEX_OBSSSCAN_ACTIVITY_THRESHOLD:
            *mib_value =
                (uint32_t)mac_vap->pst_mib_info->st_wlan_mib_operation.ul_dot11OBSSScanActivityThreshold;
            break;
        default:
            return OAL_FAIL;
    }
    return OAL_SUCC;
}


uint32_t hmac_config_get_mib(mac_vap_stru *pst_mac_vap, wlan_cfgid_enum_uint16 en_cfg_id, uint16_t us_len,
                             const uint8_t *puc_param)
{
    uint32_t mib_idx;
    uint32_t mib_value;

    mib_idx = *((uint32_t *)puc_param);
    switch (mib_idx) {
        case WLAN_MIB_INDEX_LSIG_TXOP_PROTECTION_OPTION_IMPLEMENTED:
            mib_value =
                (uint32_t)pst_mac_vap->pst_mib_info->st_wlan_mib_ht_sta_cfg.en_dot11LsigTxopProtectionOptionImplemented;
            break;
        case WLAN_MIB_INDEX_HT_GREENFIELD_OPTION_IMPLEMENTED:
            mib_value = (uint32_t)pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11HTGreenfieldOptionImplemented;
            break;
        case WLAN_MIB_INDEX_SPEC_MGMT_IMPLEMENT:
            mib_value =
                (uint32_t)pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11SpectrumManagementImplemented;
            break;
        case WLAN_MIB_INDEX_FORTY_MHZ_OPERN_IMPLEMENT:
            mib_value = (uint32_t)mac_mib_get_FortyMHzOperationImplemented(pst_mac_vap);
            break;
        case WLAN_MIB_INDEX_2040_COEXT_MGMT_SUPPORT:
            mib_value =
                (uint32_t)pst_mac_vap->pst_mib_info->st_wlan_mib_operation.en_dot112040BSSCoexistenceManagementSupport;
            break;
        case WLAN_MIB_INDEX_FORTY_MHZ_INTOL:
            mib_value = (uint32_t)pst_mac_vap->pst_mib_info->st_wlan_mib_operation.en_dot11FortyMHzIntolerant;
            break;
        case WLAN_MIB_INDEX_VHT_CHAN_WIDTH_OPTION_IMPLEMENT:
            mib_value =
                (uint32_t)pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.uc_dot11VHTChannelWidthOptionImplemented;
            break;
        case WLAN_MIB_INDEX_OBSSSCAN_TRIGGER_INTERVAL:
            mib_value =
                (uint32_t)pst_mac_vap->pst_mib_info->st_wlan_mib_operation.ul_dot11BSSWidthTriggerScanInterval;
            break;
        default:
            if (hmac_config_get_mib_default_case(pst_mac_vap, &mib_value, mib_idx) != OAL_SUCC) {
                OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_mib::invalid ul_mib_idx[%d].}",
                    mib_idx);
                return OAL_FAIL;
            }
    }
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_mib::mib vaule=%d.}", mib_value);

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_thruput_bypass(mac_vap_stru *pst_mac_vap, wlan_cfgid_enum_uint16 en_cfg_id,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                       ul_ret;
    mac_cfg_set_thruput_bypass_stru *pst_set_thruput_bypass = (mac_cfg_set_thruput_bypass_stru *)puc_param;

    /* ���������VAP, ֱ�ӷ��� */
    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_CONFIG) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_thruput_bypass::config vap can't set.}");
        return OAL_FAIL;
    }

    OAL_SET_THRUPUT_BYPASS_ENABLE(pst_set_thruput_bypass->uc_bypass_type, pst_set_thruput_bypass->uc_value);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    if (pst_set_thruput_bypass->uc_bypass_type == OAL_TX_SDIO_SLAVE_BYPASS && pst_set_thruput_bypass->uc_value == 0) {
        hcc_msg_slave_thruput_bypass();
    }
#endif
    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_thruput_bypass::bypass type:%u, value:%u.}",
                     pst_set_thruput_bypass->uc_bypass_type, pst_set_thruput_bypass->uc_value);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_THRUPUT_BYPASS, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_thruput_bypass::hmac_config_send_event failed[%d].}", ul_ret);
    }
#endif

    return ul_ret;
}


oal_uint32  hmac_config_set_auto_protection(mac_vap_stru *pst_mac_vap, oal_uint8 uc_auto_protection_flag)
{
    return hmac_protection_set_autoprot(pst_mac_vap, uc_auto_protection_flag);
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)


oal_uint32  hmac_config_vap_state_syn(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��VAP����״̬��DMAC
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_VAP_STATE_SYN, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_vap_state_syn::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


#ifdef _PRE_WLAN_FEATURE_STA_PM

oal_uint32  hmac_set_ipaddr_timeout(void   *puc_para)
{
    oal_uint32          ul_ret;
    hmac_vap_stru       *pst_hmac_vap = (hmac_vap_stru *)puc_para;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    wlan_pm_set_timeout(WLAN_SLEEP_DEFAULT_CHECK_CNT);
#endif

    /* δ����dhcp�ɹ�,��ʱ���͹��� */
    ul_ret = hmac_config_set_pm_by_module(&pst_hmac_vap->st_vap_base_info, MAC_STA_PM_CTRL_TYPE_HOST,
        MAC_STA_PM_SWITCH_ON);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR,
            "{hmac_set_ipaddr_timeout::hmac_config_set_pm_by_module failed[%d].}", ul_ret);
    }

    return OAL_SUCC;
}

#endif
#endif


oal_uint32  hmac_config_user_asoc_state_syn(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    oal_uint32                     ul_ret;
    mac_h2d_user_asoc_state_stru   st_h2d_user_asoc_state_stru;

    st_h2d_user_asoc_state_stru.us_user_idx   = pst_mac_user->us_assoc_id;
    st_h2d_user_asoc_state_stru.en_asoc_state = pst_mac_user->en_user_asoc_state;

    /***************************************************************************
        ���¼���DMAC��, ͬ��user����״̬��device��
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_USER_ASOC_STATE_SYN,
        OAL_SIZEOF(mac_h2d_user_asoc_state_stru), (oal_uint8 *)(&st_h2d_user_asoc_state_stru));
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_user_asoc_state_syn::hmac_config_send_event"
            "failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32 hmac_config_user_cap_syn(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    oal_uint32                  ul_ret;
    mac_h2d_usr_cap_stru        st_mac_h2d_usr_cap;

    st_mac_h2d_usr_cap.us_user_idx = pst_mac_user->us_assoc_id;
    memcpy_s((oal_uint8 *)(&st_mac_h2d_usr_cap.st_user_cap_info), OAL_SIZEOF(mac_user_cap_info_stru),
             (oal_uint8 *)(&pst_mac_user->st_cap_info), OAL_SIZEOF(mac_user_cap_info_stru));

    /***************************************************************************
        ���¼���DMAC��, ͬ��VAP����״̬��DMAC
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_USER_CAP_SYN, OAL_SIZEOF(mac_h2d_usr_cap_stru),
        (oal_uint8 *)(&st_mac_h2d_usr_cap));
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_user_cap_syn::hmac_config_sta_vap_info_syn failed[%d].}", ul_ret);
    }

    return ul_ret;
}



oal_uint32  hmac_config_user_rate_info_syn(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    oal_uint32                  ul_ret;
    mac_h2d_usr_rate_info_stru  st_mac_h2d_usr_rate_info;

    st_mac_h2d_usr_rate_info.us_user_idx   = pst_mac_user->us_assoc_id;
    st_mac_h2d_usr_rate_info.en_protocol_mode = pst_mac_user->en_protocol_mode;

    /* legacy���ʼ���Ϣ��ͬ����dmac */
    st_mac_h2d_usr_rate_info.uc_avail_rs_nrates = pst_mac_user->st_avail_op_rates.uc_rs_nrates;
    memcpy_s(st_mac_h2d_usr_rate_info.auc_avail_rs_rates, WLAN_RATE_MAXSIZE,
             pst_mac_user->st_avail_op_rates.auc_rs_rates, WLAN_RATE_MAXSIZE);

    /* ht���ʼ���Ϣ��ͬ����dmac */
    mac_user_get_ht_hdl(pst_mac_user, &st_mac_h2d_usr_rate_info.st_ht_hdl);

    /* vht���ʼ���Ϣ��ͬ����dmac */
    mac_user_get_vht_hdl(pst_mac_user, &st_mac_h2d_usr_rate_info.st_vht_hdl);

    /***************************************************************************
        ���¼���DMAC��, ͬ��user����״̬��device��
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_USER_RATE_SYN, sizeof(mac_h2d_usr_rate_info_stru),
        (oal_uint8 *)(&st_mac_h2d_usr_rate_info));
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_user_rate_info_syn::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_user_info_syn(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    oal_uint32                  ul_ret;
    mac_h2d_usr_info_stru       st_mac_h2d_usr_info;

    st_mac_h2d_usr_info.en_avail_bandwidth = pst_mac_user->en_avail_bandwidth;
    st_mac_h2d_usr_info.en_cur_bandwidth   = pst_mac_user->en_cur_bandwidth;
    st_mac_h2d_usr_info.us_user_idx        = pst_mac_user->us_assoc_id;
    st_mac_h2d_usr_info.en_user_pmf        = pst_mac_user->st_cap_info.bit_pmf_active;
    st_mac_h2d_usr_info.uc_arg1            = pst_mac_user->st_ht_hdl.uc_max_rx_ampdu_factor;
    st_mac_h2d_usr_info.uc_arg2            = pst_mac_user->st_ht_hdl.uc_min_mpdu_start_spacing;
    st_mac_h2d_usr_info.en_user_asoc_state = pst_mac_user->en_user_asoc_state;

    /* Э��ģʽ��Ϣͬ����dmac */
    st_mac_h2d_usr_info.en_avail_protocol_mode  = pst_mac_user->en_avail_protocol_mode;

    st_mac_h2d_usr_info.en_cur_protocol_mode    = pst_mac_user->en_cur_protocol_mode;
    st_mac_h2d_usr_info.en_protocol_mode        = pst_mac_user->en_protocol_mode;
    st_mac_h2d_usr_info.en_bandwidth_cap        = pst_mac_user->en_bandwidth_cap;
    st_mac_h2d_usr_info.uc_drop_ncw             = pst_mac_user->uc_drop_ncw;

#ifdef _PRE_WLAN_FEATURE_HISTREAM
    if (g_uc_histream_switch == HISTREAM_SWITCH_ON) {
        if (memcpy_s(&(st_mac_h2d_usr_info.st_histream_info), OAL_SIZEOF(mac_histream_stru),
                     &(pst_mac_user->st_histream_info), OAL_SIZEOF(mac_histream_stru)) != EOK) {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_user_info_syn::memcpy_s fail!");
            return OAL_FAIL;
        }
    }
#endif // _PRE_WLAN_FEATURE_HISTREAM

    /***************************************************************************
        ���¼���DMAC��, ͬ��VAP����״̬��DMAC
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_USR_INFO_SYN, OAL_SIZEOF(st_mac_h2d_usr_info),
        (oal_uint8 *)(&st_mac_h2d_usr_info));
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_user_info_syn::hmac_config_send_event"
            "failed[%d], user_id[%d].}", ul_ret, pst_mac_user->us_assoc_id);
    }

    return ul_ret;
}


oal_uint32  hmac_config_sta_vap_info_syn(mac_vap_stru *pst_mac_vap)
{
    oal_uint32                  ul_ret;
    mac_h2d_vap_info_stru       st_mac_h2d_vap_info;

    st_mac_h2d_vap_info.us_sta_aid = pst_mac_vap->us_sta_aid;
    st_mac_h2d_vap_info.uc_uapsd_cap = pst_mac_vap->uc_uapsd_cap;
    /***************************************************************************
        ���¼���DMAC��, ͬ��VAP����״̬��DMAC
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_STA_VAP_INFO_SYN, OAL_SIZEOF(mac_h2d_vap_info_stru),
        (oal_uint8 *)(&st_mac_h2d_vap_info));
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_sta_vap_info_syn::hmac_config_sta_vap_info_syn failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32 hmac_init_user_security_port(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    oal_uint32                      ul_ret;
    mac_cfg80211_init_port_stru     st_init_port;

    /* ��ʼ����֤�˿���Ϣ */
    mac_vap_init_user_security_port(pst_mac_vap, pst_mac_user);

    memcpy_s(st_init_port.auc_mac_addr, OAL_MAC_ADDR_LEN, pst_mac_user->auc_user_mac_addr, OAL_MAC_ADDR_LEN);
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_INIT_SECURTIY_PORT, OAL_SIZEOF(st_init_port),
        (oal_uint8 *)&st_init_port);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
            "{hmac_config_user_security_port::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32 hmac_user_set_asoc_state(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user,
    mac_user_asoc_state_enum_uint8 en_value)
{
    oal_uint32 ul_ret;

    mac_user_set_asoc_state(pst_mac_user, en_value);

    /* dmac offload�ܹ��£�ͬ��user����״̬��Ϣ��dmac */
    ul_ret = hmac_config_user_asoc_state_syn(pst_mac_vap, pst_mac_user);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                       "{hmac_user_set_asoc_state::hmac_config_user_asoc_state_syn failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_send_2040_coext(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_set_2040_coexist_stru *pst_2040_coexist;
    oal_netbuf_stru   *pst_netbuf = OAL_PTR_NULL;
    mac_tx_ctl_stru   *pst_tx_ctl = OAL_PTR_NULL;
    oal_uint32         ul_ret;
    oal_uint16         us_frame_len;

    pst_2040_coexist = (mac_cfg_set_2040_coexist_stru*)puc_param;
    OAM_ERROR_LOG2(0, 0, "hmac_config_send_2040_coext::coinfo=%d chan=%d",
                   pst_2040_coexist->ul_coext_info, pst_2040_coexist->ul_channel_report);

    /* �������֡�ڴ� */
    pst_netbuf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if (pst_netbuf == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_send_2040_coext::pst_netbuf null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_NETBUF_PREV(pst_netbuf) = OAL_PTR_NULL;
    OAL_NETBUF_NEXT(pst_netbuf) = OAL_PTR_NULL;

    /* ��װ20/40 �������֡ */
    us_frame_len = hmac_encap_2040_coext_mgmt(pst_mac_vap, OAL_NETBUF_HEADER(pst_netbuf),
        (oal_uint8)pst_2040_coexist->ul_coext_info, pst_2040_coexist->ul_channel_report);

    oal_netbuf_put(pst_netbuf, us_frame_len);

    /* ��дnetbuf��cb�ֶΣ������͹���֡�ͷ�����ɽӿ�ʹ�� */
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

    memset_s(pst_tx_ctl, OAL_NETBUF_CB_SIZE(), 0, OAL_NETBUF_CB_SIZE());
    pst_tx_ctl->us_mpdu_len = us_frame_len;
    pst_tx_ctl->us_tx_user_idx = 0xFFFF;
    pst_tx_ctl->uc_ac = WLAN_WME_AC_MGMT;

    /* ���¼���DMAC���͹���֡ */
    ul_ret = hmac_tx_mgmt_send_event(pst_mac_vap, pst_netbuf, us_frame_len);
    if (ul_ret != OAL_SUCC) {
        oal_netbuf_free(pst_netbuf);

        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_send_2040_coext::hmac_tx_mgmt_send_event failed.}", ul_ret);
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_2040_coext_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_device_stru       *pst_mac_device;
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    mac_ap_ch_info_stru   *pst_ch_list = OAL_PTR_NULL;
    oal_uint32             ul_idx;
#endif
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_2040_coext_info::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    for (ul_idx = 0; ul_idx < MAC_MAX_SUPP_CHANNEL; ul_idx++) {
        pst_ch_list = &(pst_mac_device->st_ap_channel_list[ul_idx]);
        OAM_ERROR_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                       "{hmac_config_2040_coext_info::chan_idx=%d num_networks=%d, chan_type=%d.}",
                       ul_idx, pst_ch_list->us_num_networks, pst_ch_list->en_ch_type);
    }
#endif

    return OAL_SUCC;
}


oal_uint32  hmac_config_get_version(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_GET_VERSION, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_get_version::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_get_fem_pa_status(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_CHECK_FEM_PA, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_get_fem_pa_status::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


#ifdef _PRE_DEBUG_MODE
oal_uint32  hmac_config_get_all_reg_value(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_GET_ALL_REG_VALUE, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_get_all_reg_value:hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif
#ifdef _PRE_WLAN_FEATURE_DAQ

oal_uint32  hmac_config_data_acq(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                   ul_ret;

    /* �����ʹ�����ݲɼ�����Down����VAP */
    if (puc_param[0] == '2') {
        hmac_data_acq_down_vap(pst_mac_vap);
    }

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_DATA_ACQ, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_data_acq::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif
#ifdef _PRE_WLAN_FEATURE_SMPS
#ifdef _PRE_DEBUG_MODE

oal_uint32  hmac_config_get_smps_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_device_stru             *pst_mac_device;
    oal_uint32                   uc_user_idx;
    mac_res_user_hash_stru      *pst_res_hash = OAL_PTR_NULL;
    oal_dlist_head_stru         *pst_head = OAL_PTR_NULL;
    mac_user_stru               *pst_user_tmp = OAL_PTR_NULL;
    oal_int8                     ac_tmp_buff[512]   = {0};
    oal_int32                    l_remainder_len;
    oal_int8                     uc_vap_index;
    oal_int8                    *pc_print_buff = OAL_PTR_NULL;

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_smps_info::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pc_print_buff = (oal_int8 *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAM_REPORT_MAX_STRING_LEN, OAL_TRUE);
    if (pc_print_buff == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    memset_s(pc_print_buff, OAM_REPORT_MAX_STRING_LEN, 0, OAM_REPORT_MAX_STRING_LEN);

    if (snprintf_s(pc_print_buff, OAM_REPORT_MAX_STRING_LEN, OAM_REPORT_MAX_STRING_LEN - 1,
                   "The device smps is %s, and nosmps num is %d.\r\n",
                   hmac_config_dev2string(pst_mac_device->en_smps), pst_mac_device->uc_no_smps_user_cnt) < EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_get_smps_info::snprintf_s fail.}");
    }
    l_remainder_len = (oal_int32)(OAM_REPORT_MAX_STRING_LEN - OAL_STRLEN(pc_print_buff));

    for (uc_vap_index = 0; uc_vap_index < pst_mac_device->uc_vap_num; uc_vap_index++) {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_index]);
        if (pst_mac_vap == OAL_PTR_NULL) {
            OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_get_smps_info::pst_mac_vap null.}");
            continue;
        }

        if (snprintf_s(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff), OAL_SIZEOF(ac_tmp_buff) - 1,
                       "VAP Index[%d] SMPS CAP IS [%s]; SMPS MODE IS [%s]!\r\n",
                       pst_mac_device->auc_vap_id[uc_vap_index],
                       hmac_config_smps2string(mac_mib_get_smps(pst_mac_vap) - 1),
                       hmac_config_smps2string(pst_mac_vap->st_cap_flag.bit_smps - 1)) < EOK) {
            OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_get_smps_info::snprintf_s fail.}");
        }

        if (strncat_s(pc_print_buff, OAM_REPORT_MAX_STRING_LEN, ac_tmp_buff, l_remainder_len - 1) != EOK) {
            OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_get_smps_info::strncat_s fail.}");
        }
        memset_s(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff), 0, OAL_SIZEOF(ac_tmp_buff));
        l_remainder_len = (oal_int32)(OAM_REPORT_MAX_STRING_LEN - OAL_STRLEN(pc_print_buff));

        /* AP���USER��Ϣ */
        for (uc_user_idx = 0; uc_user_idx < MAC_VAP_USER_HASH_MAX_VALUE; uc_user_idx++) {
            pst_head = pst_mac_vap->ast_user_hash[uc_user_idx].pst_next;

            while ((&pst_mac_vap->ast_user_hash[uc_user_idx]) != pst_head) {
                pst_res_hash = (mac_res_user_hash_stru *)pst_head;

                pst_user_tmp = mac_res_get_mac_user(pst_res_hash->us_user_idx); /* �ҵ���Ӧ�û� */
                if (pst_user_tmp == OAL_PTR_NULL) {
                    OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_config_get_smps_info::pst_mac_user null.}");
                    pst_head = pst_res_hash->st_entry.pst_next;
                    continue;
                }

                if (snprintf_s(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff), OAL_SIZEOF(ac_tmp_buff) - 1,
                               "[%d] USER MACADDR [%02X:%02X:%02X:%02X:%02X:%02X] SMPS CAP is [%s]! \r\n",
                               uc_user_idx, pst_user_tmp->auc_user_mac_addr[0],
                               pst_user_tmp->auc_user_mac_addr[1], pst_user_tmp->auc_user_mac_addr[2],
                               pst_user_tmp->auc_user_mac_addr[3], pst_user_tmp->auc_user_mac_addr[4],
                               pst_user_tmp->auc_user_mac_addr[5],
                               hmac_config_smps2string(pst_user_tmp->st_ht_hdl.bit_sm_power_save - 1)) < EOK) {
                    OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_get_smps_info::snprintf_s fail.}");
                }
                if (strncat_s(pc_print_buff, OAM_REPORT_MAX_STRING_LEN, ac_tmp_buff, l_remainder_len - 1) != EOK) {
                    OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_get_smps_info::strncat_s fail.}");
                }
                memset_s(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff), 0, OAL_SIZEOF(ac_tmp_buff));
                l_remainder_len = (oal_int32)(OAM_REPORT_MAX_STRING_LEN - OAL_STRLEN(pc_print_buff));

                pst_head = pst_res_hash->st_entry.pst_next;
            }
        }
    }

    oam_print(pc_print_buff);
    OAL_MEM_FREE(pc_print_buff, OAL_TRUE);
    return OAL_SUCC;
}
#endif
#endif


#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY

/* zhongwen ͬ����Ҫ���ӣ����� */
oal_uint32  hmac_config_set_opmode_notify(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint8       uc_value;

    uc_value     = *puc_param;

    if (uc_value >= OAL_BUTT) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_opmode_notify::uc_value is limit! value = [%d].}\r\n", uc_value);
        return OAL_FAIL;
    }

    if (mac_mib_get_VHTOptionImplemented(pst_mac_vap) == OAL_TRUE) {
        mac_mib_set_OperatingModeNotificationImplemented(pst_mac_vap, (oal_bool_enum_uint8)uc_value);
    } else {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_opmode_notify::pst_mac_vap is not 11ac."
            "en_protocol = [%d].}\r\n", pst_mac_vap->en_protocol);
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_get_user_rssbw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_add_user_param_stru    *pst_user;
    hmac_vap_stru                  *pst_hmac_vap;
    hmac_user_stru                 *pst_hmac_user;
    oal_int8                        ac_tmp_buff[200];

    pst_user = (mac_cfg_add_user_param_stru *)puc_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_config_get_user_rssbw::pst_hmac_vap null.}");
        return OAL_FAIL;
    }

    pst_hmac_user = mac_vap_get_hmac_user_by_addr(pst_mac_vap, pst_user->auc_mac_addr);
    if (pst_hmac_user == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_get_user_rssbw::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (snprintf_s(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff), OAL_SIZEOF(ac_tmp_buff) - 1,
                   "%02X:XX:XX:%02X:%02X:%02X\r\nnss_cap: %s, avail_nss: %s.\n\ruser bw_cap: %s, avail_bw: %s.",
                   pst_user->auc_mac_addr[0], pst_user->auc_mac_addr[3],
                   pst_user->auc_mac_addr[4], pst_user->auc_mac_addr[5],
                   hmac_config_nss2string(pst_hmac_user->st_user_base_info.uc_num_spatial_stream),
                   hmac_config_nss2string(pst_hmac_user->st_user_base_info.uc_avail_num_spatial_stream),
                   hmac_config_b_w2string(pst_hmac_user->st_user_base_info.en_bandwidth_cap),
                   hmac_config_b_w2string(pst_hmac_user->st_user_base_info.en_avail_bandwidth)) < EOK) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_config_get_user_rssbw::snprintf_s failed!.}");
        return OAL_FAIL;
    }
    oam_print(ac_tmp_buff);

    return OAL_SUCC;
}

#endif


oal_uint32  hmac_config_set_vap_nss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint8            uc_value;
    oal_uint32           ul_ret;

    uc_value = *puc_param;

    if ((uc_value > (WLAN_FOUR_NSS + 1)) || (uc_value < (WLAN_SINGLE_NSS + 1))) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_vap_nss::uc_value is limit!value = [%d].}",
            uc_value);
        return OAL_FAIL;
    }

    if (pst_mac_vap->en_vap_state != MAC_VAP_STATE_INIT) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_vap_nss::vap is up,please down vap first."
            "VAP STATE = [%d].}\r\n", pst_mac_vap->en_vap_state);
        return OAL_FAIL;
    }

    if (pst_mac_vap->en_protocol == WLAN_PROTOCOL_BUTT) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{mac_vap_init_rx_nss_by_protocol::please set mode first!}");
        return OAL_FAIL;
    }

    mac_vap_init_rx_nss_by_protocol(pst_mac_vap);
    mac_vap_set_rx_nss(pst_mac_vap, OAL_MIN(pst_mac_vap->en_vap_rx_nss, (uc_value - 1)));

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_VAP_NSS, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_vap_nss::hmac_config_send_event failed[%d].}", ul_ret);
        return ul_ret;
    }
    return OAL_SUCC;
}

#ifdef _PRE_DEBUG_MODE

oal_uint32  hmac_config_rx_filter_val(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_cfg_rx_filter_stru       *pst_rx_filter_val;

    pst_rx_filter_val = (hmac_cfg_rx_filter_stru *)puc_param;

    if (pst_rx_filter_val->uc_write_read == 0) {
        hmac_set_status_rx_filter_val(pst_rx_filter_val->uc_dev_mode, pst_rx_filter_val->uc_vap_mode,
            pst_rx_filter_val->uc_vap_status, pst_rx_filter_val->ul_val);
    } else {
        hmac_get_status_rx_filter_val(pst_rx_filter_val->uc_dev_mode, pst_rx_filter_val->uc_vap_mode,
            pst_rx_filter_val->uc_vap_status);
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_rx_filter_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8            uc_value;

    uc_value = *puc_param;

    if (uc_value > 1) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_vap_nss::uc_value is limit! value = [%d].}\r\n", uc_value);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    hmac_set_rx_filter_en(uc_value);

    return OAL_SUCC;
}

oal_uint32  hmac_config_get_rx_filter_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_get_rx_filter_en();
    return OAL_SUCC;
}


oal_uint32  hmac_config_report_ampdu_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_REPORT_AMPDU_STAT, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_report_ampdu_stat::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#endif


oal_uint32  hmac_config_set_ampdu_aggr_num(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32    ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_AGGR_NUM, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_ampdu_aggr_num::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

oal_uint32  hmac_config_set_ampdu_mmss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_ampdu_mmss_stru   *pst_ampdu_mmss_ctrl;

    pst_ampdu_mmss_ctrl = (mac_cfg_ampdu_mmss_stru *)puc_param;

    pst_mac_vap->pst_mib_info->st_wlan_mib_ht_sta_cfg.ul_dot11MinimumMPDUStartSpacing =
        pst_ampdu_mmss_ctrl->uc_mmss_val;

    OAL_IO_PRINT("hmac_config_set_ampdu_mmss: mmss[%d] \n", pst_ampdu_mmss_ctrl->uc_mmss_val);

    return OAL_SUCC;
}
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

oal_uint32  hmac_config_freq_adjust(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_FREQ_ADJUST, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_freq_adjust::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif


oal_uint32  hmac_config_set_stbc_cap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_bool_enum_uint8   uc_value;

    uc_value = *puc_param;

    if (OAL_UNLIKELY(pst_mac_vap->pst_mib_info == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_stbc_cap::pst_mac_vap->pst_mib_info null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (uc_value == 1) {
        pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11TxSTBCOptionImplemented                = OAL_TRUE;
        pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11RxSTBCOptionImplemented                = OAL_TRUE;
        pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11TxSTBCOptionActivated                  = OAL_TRUE;
        pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.en_dot11VHTTxSTBCOptionImplemented   = OAL_TRUE;
        pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.en_dot11VHTRxSTBCOptionImplemented   = OAL_TRUE;
    } else if (uc_value == 0) {
        pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11TxSTBCOptionImplemented                = OAL_FALSE;
        pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11RxSTBCOptionImplemented                = OAL_FALSE;
        pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11TxSTBCOptionActivated                  = OAL_FALSE;
        pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.en_dot11VHTTxSTBCOptionImplemented   = OAL_FALSE;
        pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.en_dot11VHTRxSTBCOptionImplemented   = OAL_FALSE;
    } else {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_stbc_cap::stbc_value is limit! value ="
            "[%d].}", uc_value);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) /* hi1102-cb set at both side (HMAC to DMAC) */
    return hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_STBC_CAP, us_len, puc_param);
#else
    return OAL_SUCC;
#endif
}



oal_uint32 hmac_config_send_sub_event(
    mac_vap_stru                     *pst_mac_vap,
    oal_uint16                       us_len,
    const oal_uint8                  *puc_param)
{
    if (pst_mac_vap == NULL || puc_param == NULL) {
        OAM_ERROR_LOG0(0, 0, "hmac_config_send_sub_event:Param NULL");
        return OAL_FAIL;
    }

    return hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_LDPC_CAP, us_len, puc_param);
}

oal_uint32  hmac_config_set_ldpc_cap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint8   uc_value;
    sub_type_load_stru  st_ldpc_cap = {0};

    uc_value = (oal_bool_enum_uint8)(*puc_param);

    if (OAL_UNLIKELY(pst_mac_vap->pst_mib_info == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_ldpc_cap::pst_mac_vap->pst_mib_info null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (uc_value == 1) {
        pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11LDPCCodingOptionImplemented                = OAL_TRUE;
        pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11LDPCCodingOptionActivated                  = OAL_TRUE;
        pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.en_dot11VHTLDPCCodingOptionImplemented   = OAL_TRUE;
    } else if (uc_value == 0) {
        pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11LDPCCodingOptionImplemented                = OAL_FALSE;
        pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11LDPCCodingOptionActivated                  = OAL_FALSE;
        pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.en_dot11VHTLDPCCodingOptionImplemented   = OAL_FALSE;
    } else {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_ldpc_cap::ldpc_value is limit! value ="
            "[%d].}", uc_value);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    /*
     *   LDPC�¼�������
     *   ����TLV��ʽ
     *   device��ͨ���ж�TLVֵ
     *   ������Ӧ�Ĳ���
     *   ��֤����ԭ�й��ܲ���
     */
    st_ldpc_cap.en_sub_type = MAC_SUB_LDPC_EVENT;
    st_ldpc_cap.us_buf_len = us_len;
    if (memcpy_s(st_ldpc_cap.auc_buf, WLAN_SUB_EVENT_MAX_LEN, puc_param, us_len) != EOK) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_set_ldpc_cap::memcpy_s fail!");
        return OAL_FAIL;
    }
    us_len += 4;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) /* hi1102-cb set at both side (HMAC to DMAC) */
    return hmac_config_send_sub_event(pst_mac_vap, us_len, (oal_uint8 *)&st_ldpc_cap);
#else
    return OAL_SUCC;
#endif
}
#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY


oal_uint32  hmac_config_update_blacklist(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_kick_user_param_stru   *pst_kick_user_param;
    hmac_user_stru                 *pst_hmac_user;

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{pst_mac_vap/puc_param is null ptr}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_kick_user_param = (mac_cfg_kick_user_param_stru *)puc_param;

    pst_hmac_user = mac_vap_get_hmac_user_by_addr(pst_mac_vap, pst_kick_user_param->auc_mac_addr);
    if (pst_hmac_user == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{pst_hmac_user is null ptr.}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_hmac_user->st_user_base_info.en_user_asoc_state != MAC_USER_STATE_ASSOC) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_kick_user: the user is unassociated.}");
        return OAL_FAIL;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_blacklist_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    oal_uint32                 *pul_cfg_mode;

    pul_cfg_mode = (oal_uint32 *)puc_param;
    ul_ret = hmac_blacklist_set_mode(pst_mac_vap, (oal_uint8) * pul_cfg_mode);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                       "{hmac_blacklist_set_mode fail: ret=%d; mode=%d}\r\n", ul_ret, *pul_cfg_mode);
        return ul_ret;
    }
    return OAL_SUCC;
}

oal_uint32  hmac_config_blacklist_add(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    ul_ret = hmac_blacklist_add(pst_mac_vap, puc_param, 0);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                         "{hmac_blacklist_add fail: ret=%d;}\r\n", ul_ret);
        return ul_ret;
    }
    return OAL_SUCC;
}


oal_uint32  hmac_config_blacklist_add_only(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    ul_ret = hmac_blacklist_add_only(pst_mac_vap, puc_param, 0);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                       "{hmac_config_blacklist_add_only fail: ret=%d;}\r\n", ul_ret);
        return ul_ret;
    }
    return OAL_SUCC;
}



oal_uint32  hmac_config_blacklist_del(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    ul_ret = hmac_blacklist_del(pst_mac_vap, puc_param);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                       "{hmac_config_blacklist_del::blacklist_del fail: ret=%d;}\r\n", ul_ret);

        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_show_blacklist(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    hmac_show_blacklist_info(pst_mac_vap);
    return OAL_SUCC;
}


oal_uint32  hmac_config_show_isolation(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    hmac_show_isolation_info(pst_mac_vap);
    return OAL_SUCC;
}


oal_uint32  hmac_config_autoblacklist_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;
    oal_uint8   uc_enable;

    uc_enable = *puc_param;

    ul_ret = hmac_autoblacklist_enable(pst_mac_vap, uc_enable);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                       "{hmac_autoblacklist_enable fail: ret=%d; cfg=%d}\r\n", ul_ret, *puc_param);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_autoblacklist_aging(mac_vap_stru *pst_mac_vap, oal_uint16 us_len,
    const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    oal_uint32                 *pul_cfg_aging_time;

    pul_cfg_aging_time = (oal_uint32 *)puc_param;
    ul_ret = hmac_autoblacklist_set_aging(pst_mac_vap, *pul_cfg_aging_time);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                       "{hmac_autoblacklist_set_aging fail: ret=%d; cfg=%d}\r\n", ul_ret, *pul_cfg_aging_time);
        return ul_ret;
    }
    return OAL_SUCC;
}


oal_uint32  hmac_config_set_autoblacklist_threshold(mac_vap_stru *pst_mac_vap, oal_uint16 us_len,
    const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    oal_uint32                 *pul_cfg_threshold;

    pul_cfg_threshold = (oal_uint32 *)puc_param;
    ul_ret = hmac_autoblacklist_set_threshold(pst_mac_vap, *pul_cfg_threshold);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                       "{hmac_autoblacklist_set_threshold fail: ret=%d; cfg=%d}\r\n", ul_ret, *pul_cfg_threshold);
        return ul_ret;
    }
    return OAL_SUCC;
}


oal_uint32  hmac_config_set_autoblacklist_reset_time(mac_vap_stru *pst_mac_vap, oal_uint16 us_len,
    const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    oal_uint32                 *pul_cfg_reset_time;

    pul_cfg_reset_time = (oal_uint32 *)puc_param;
    ul_ret = hmac_autoblacklist_set_reset_time(pst_mac_vap, *pul_cfg_reset_time);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                       "{hmac_autoblacklist_set_reset_time fail: ret=%d; cfg=%d}\r\n", ul_ret, *pul_cfg_reset_time);
        return ul_ret;
    }
    return OAL_SUCC;
}


oal_uint32  hmac_config_set_isolation_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    oal_uint32                 *pul_cfg_mode;

    pul_cfg_mode = (oal_uint32 *)puc_param;
    ul_ret = hmac_isolation_set_mode(pst_mac_vap, (oal_uint8) * pul_cfg_mode);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                       "{hmac_isolation_set_mode fail: ret=%d; cfg=%d}\r\n", ul_ret, *pul_cfg_mode);
        return ul_ret;
    }
    return OAL_SUCC;
}


oal_uint32  hmac_config_set_isolation_type(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    oal_uint32                 *pul_cfg_type;

    pul_cfg_type = (oal_uint32 *)puc_param;
    ul_ret = hmac_isolation_set_type(pst_mac_vap, (oal_uint8) * pul_cfg_type);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                       "{hmac_isolation_set_type fail: ret=%d; cfg=%d}\r\n", ul_ret, *pul_cfg_type);
        return ul_ret;
    }
    return OAL_SUCC;
}


oal_uint32  hmac_config_set_isolation_forword(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    oal_uint32                 *pul_cfg_forword;

    pul_cfg_forword = (oal_uint32 *)puc_param;
    ul_ret = hmac_isolation_set_forward(pst_mac_vap, (oal_uint8) * pul_cfg_forword);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                       "{hmac_isolation_set_forward fail: ret=%d; cfg=%d}\r\n", ul_ret, *pul_cfg_forword);
        return ul_ret;
    }
    return OAL_SUCC;
}


oal_uint32  hmac_config_set_isolation_clear(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    ul_ret = hmac_isolation_clear_counter(pst_mac_vap);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                       "{hmac_isolation_clear_counter fail: ret=%d; cfg=%d}\r\n", ul_ret);
        return ul_ret;
    }
    return OAL_SUCC;
}
#endif  /* _PRE_WLAN_FEATURE_CUSTOM_SECURITY */


oal_uint32  hmac_config_scan_abort(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    hmac_vap_stru           *pst_hmac_vap = OAL_PTR_NULL;
    hmac_device_stru        *pst_hmac_device = OAL_PTR_NULL;
    oal_uint32               ul_ret;

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                     "{hmac_config_scan_abort::vap_id[%d] scan abort.}", pst_mac_vap->uc_vap_id);

    pst_hmac_device = hmac_res_get_mac_dev(pst_mac_vap->uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                         "{hmac_config_scan_abort::pst_hmac_device is null, dev_id[%d].}",
                         pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                         "{hmac_config_scan_abort::pst_hmac_vap is null, vap_id[%d].}",
                         pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }

    if ((pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) &&
        (pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.en_vap_last_state != MAC_VAP_STATE_BUTT)) {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                         "{hmac_config_scan_abort::en_vap_last_state:%d}",
                         pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.en_vap_last_state);
        hmac_fsm_change_state(pst_hmac_vap, pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.en_vap_last_state);
        pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.en_vap_last_state = MAC_VAP_STATE_BUTT;
    }
    /* ���ɨ�����ϱ��Ļص������������ϱ� */
    if (pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.uc_vap_id == pst_mac_vap->uc_vap_id) {
        pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.p_fn_cb = OAL_PTR_NULL;
        pst_hmac_device->st_scan_mgmt.en_is_scanning = OAL_FALSE;
    }

    /***************************************************************************
                         ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap,
                                    WLAN_CFGID_SCAN_ABORT,
                                    us_len,
                                    puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
            "{hmac_config_scan_abort::hmac_config_send_event failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32 hmac_config_remain_on_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_remain_on_channel_param_stru   *pst_remain_on_channel = OAL_PTR_NULL;
    hmac_device_stru                   *pst_hmac_device = OAL_PTR_NULL;
    mac_device_stru                    *pst_mac_device = OAL_PTR_NULL;
    hmac_vap_stru                      *pst_hmac_vap = OAL_PTR_NULL;
    oal_uint32                          ul_ret;

    /* 1.1 �ж���� */
    if (pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_P2P, "{hmac_config_remain_on_channel null ptr");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_remain_on_channel = (mac_remain_on_channel_param_stru *)puc_param;
    pst_mac_device        = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_config_remain_on_channel::pst_mac_device null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡhmac device */
    pst_hmac_device = hmac_res_get_mac_dev(pst_mac_vap->uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
            "{hmac_config_remain_on_channel::pst_hmac_device[%d] null.}",
                         pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }

    /* 1.2 ����Ƿ��ܽ������״̬ */
    ul_ret = hmac_p2p_check_can_enter_state(pst_mac_vap, HMAC_FSM_INPUT_LISTEN_REQ);
    if (ul_ret != OAL_SUCC) {
        /* ���ܽ������״̬�������豸æ */
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_P2P,
                         "{hmac_config_remain_on_channel fail,device busy: ul_ret=%d}\r\n", ul_ret);
        return OAL_ERR_CODE_CONFIG_BUSY;
    }
    /* �жϵ�ǰɨ���Ƿ�����ִ�� */
    if (pst_hmac_device->st_scan_mgmt.en_is_scanning == OAL_TRUE) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
            "{hmac_config_remain_on_channel::the scan request is rejected.}");
        return OAL_ERR_CODE_CONFIG_BUSY;
    }

#ifdef _PRE_WLAN_FEATURE_ROAM
    /* �жϵ�ǰ�Ƿ�����ִ������ */
    if (pst_mac_vap->en_vap_state == MAC_VAP_STATE_ROAMING) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
            "{hmac_config_remain_on_channel:: roam reject new scan.}");
        return OAL_ERR_CODE_CONFIG_BUSY;
    }
#endif // _PRE_WLAN_FEATURE_ROAM

    /* 1.3 ��ȡhome �ŵ����ŵ����͡�����������ŵ�Ϊ0����ʾû���豸����up ״̬����������Ҫ�������ŵ� */
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_P2P,
            "{hmac_config_remain_on_channel::mac_res_get_hmac_vap null.vap_id = %d}", pst_mac_vap->uc_vap_id);
        return OAL_PTR_NULL;
    }

    /* �����ں��·��ļ����ŵ���Ϣ�����ڼ�����ʱ��ȡ������ʱ���� */
    pst_mac_device->st_p2p_info.st_listen_channel                  = pst_remain_on_channel->st_listen_channel;

    /* ����p2p0�� p2p cl ����һ��VAP �ṹ�����ڽ������ʱ����Ҫ����֮ǰ��״̬�����ڼ�������ʱ���� */
    if (pst_mac_vap->en_vap_state != MAC_VAP_STATE_STA_LISTEN) {
        pst_mac_device->st_p2p_info.en_last_vap_state           = pst_mac_vap->en_vap_state;
    }
    pst_remain_on_channel->en_last_vap_state = pst_mac_device->st_p2p_info.en_last_vap_state;

    OAM_INFO_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_P2P,
                  "{hmac_config_remain_on_channel :: listen_channel=%d, current_channel=%d, last_state=%d}\r\n",
                  pst_remain_on_channel->uc_listen_channel,
                  pst_mac_vap->st_channel.uc_chan_number,
                  pst_mac_device->st_p2p_info.en_last_vap_state);

    /* 3.1 �޸�VAP ״̬Ϊ���� */
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_P2P,
                       "{hmac_config_remain_on_channel fail!pst_hmac_vap is null}\r\n");
        return OAL_PTR_NULL;
    }

    /* ״̬������:  hmac_p2p_remain_on_channel */
    ul_ret = hmac_fsm_call_func_sta(pst_hmac_vap, HMAC_FSM_INPUT_LISTEN_REQ, (oal_void *)(pst_remain_on_channel));
    if (ul_ret != OAL_SUCC) {
        /* DMAC �����л��ŵ�ʧ�� */
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_P2P, "{hmac_config_remain_on_channel fail: ul_ret=%d}",
            ul_ret);
        return OAL_ERR_CODE_CONFIG_BUSY;
    }

    OAM_INFO_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_P2P, "{hmac_config_remain_on_channel succ: l_channel=%d,"
        "ul_duration=%d, band=%d}", pst_remain_on_channel->uc_listen_channel, pst_remain_on_channel->ul_listen_duration,
        pst_remain_on_channel->en_band);
    return OAL_SUCC;
}


oal_uint32 hmac_config_cancel_remain_on_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len,
    const oal_uint8 *puc_param)
{
    hmac_vap_stru               *pst_hmac_vap;

    pst_hmac_vap    = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_P2P,
            "hmac_config_cancel_remain_on_channel::mac_res_get_hmac_vap fail.vap_id = %u", pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_mac_vap->en_vap_state == MAC_VAP_STATE_STA_LISTEN) {
#ifdef _PRE_WLAN_FEATURE_P2P
        hmac_p2p_listen_timeout(pst_hmac_vap, pst_mac_vap);
#endif
    } else {
        hmac_p2p_send_listen_expired_to_host(pst_hmac_vap);
    }
    return OAL_SUCC;
}


oal_uint32  hmac_config_vap_classify_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_val;
    mac_device_stru    *pst_mac_device;
    oal_int8            ac_string[OAM_PRINT_FORMAT_LENGTH];

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(0, OAM_SF_P2P, "hmac_config_vap_classify_en::mac_res_get_dev fail.device_id = %u",
            pst_mac_vap->uc_device_id);

        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_val = *((oal_uint32 *)puc_param);

    if (ul_val == 0xff) {
        /* ��ӡ��ǰ��ֵ */
        if (snprintf_s(ac_string, sizeof(ac_string), sizeof(ac_string) - 1, "device classify en is %d\n",
                       pst_mac_device->en_vap_classify) < EOK) {
            OAM_ERROR_LOG0(0, OAM_SF_P2P, "hmac_config_vap_classify_en::snprintf_s failed !");
            return OAL_FAIL;
        }

        oam_print(ac_string);

        return OAL_SUCC;
    }

    if (ul_val == 0) {
        pst_mac_device->en_vap_classify = OAL_FALSE;
    } else {
        pst_mac_device->en_vap_classify = OAL_TRUE;
    }

    return OAL_SUCC;
}
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

oal_uint32  hmac_config_query_station_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_QUERY_STATION_STATS, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_phy_stat_info::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif


oal_uint32  hmac_config_vap_classify_tid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_val;
    hmac_vap_stru      *pst_hmac_vap;
    oal_int8            ac_string[OAM_PRINT_FORMAT_LENGTH];

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "hmac_config_vap_classify_tid::mac_res_get_hmac_vap fail.vap_id = %u", pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_val = *((oal_uint32 *)puc_param);

    if (ul_val == 0xff) {
        /* ��ӡ��ǰ��ֵ */
        if (snprintf_s(ac_string, sizeof(ac_string), sizeof(ac_string) - 1, "vap classify tid is %d\n",
                       pst_hmac_vap->uc_classify_tid) < EOK) {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                           "hmac_config_vap_classify_tid::snprintf_s failed !");
            return OAL_FAIL;
        }

        oam_print(ac_string);

        return OAL_SUCC;
    }

    if (ul_val >= WLAN_TIDNO_BUTT) {
        /* ��ӡ��ǰ��ֵ */
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "val is invalid:%d, vap classify tid is %d", ul_val,
            pst_hmac_vap->uc_classify_tid);
        return OAL_SUCC;
    }

    pst_hmac_vap->uc_classify_tid = (oal_uint8)ul_val;

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_BTCOEX

OAL_STATIC OAL_INLINE oal_uint32 hmac_btcoex_delba_foreach_tid(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user,
    mac_cfg_delba_req_param_stru *pst_mac_cfg_delba_param)
{
    oal_uint32 ul_ret;

    oal_set_mac_addr(pst_mac_cfg_delba_param->auc_mac_addr, pst_mac_user->auc_user_mac_addr);

    for (pst_mac_cfg_delba_param->uc_tidno = 0; pst_mac_cfg_delba_param->uc_tidno < WLAN_TID_MAX_NUM;
        pst_mac_cfg_delba_param->uc_tidno++) {
        ul_ret = hmac_config_delba_req(pst_mac_vap, 0, (oal_uint8 *)pst_mac_cfg_delba_param);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{hmac_btcoex_delba_foreach_tid::ul_ret: %d,tid: %d}",
                ul_ret, pst_mac_cfg_delba_param->uc_tidno);
            return ul_ret;
        }
    }
    return ul_ret;
}


oal_uint32 hmac_btcoex_delba_foreach_user(mac_vap_stru *pst_mac_vap)
{
    oal_uint32 ul_ret;
    mac_cfg_delba_req_param_stru st_mac_cfg_delba_param;
    mac_user_stru *pst_mac_user          = OAL_PTR_NULL;
    oal_uint8 uc_user_idx;
    oal_dlist_head_stru *pst_entry       = OAL_PTR_NULL;
    mac_res_user_hash_stru *pst_res_hash = OAL_PTR_NULL;

    st_mac_cfg_delba_param.en_direction = MAC_RECIPIENT_DELBA;

    for (uc_user_idx = 0; uc_user_idx < MAC_VAP_USER_HASH_MAX_VALUE; uc_user_idx++) {
        OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_mac_vap->ast_user_hash[uc_user_idx])) {
            pst_res_hash = (mac_res_user_hash_stru *)pst_entry;

            pst_mac_user = mac_res_get_mac_user(pst_res_hash->us_user_idx);
            if (pst_mac_user == OAL_PTR_NULL) {
                OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX,
                    "{hmac_btcoex_delba_foreach_user::pst_mac_user null, user_idx: %d.}", pst_res_hash->us_user_idx);
                pst_entry = pst_res_hash->st_entry.pst_next;
                continue;
            }
            ul_ret = hmac_btcoex_delba_foreach_tid(pst_mac_vap, pst_mac_user, &st_mac_cfg_delba_param);
            if (ul_ret != OAL_SUCC) {
                return ul_ret;
            }
        }
    }

    return ul_ret;
}


oal_uint32 hmac_btcoex_rx_delba_trigger(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_device_stru *pst_hmac_device;
    dmac_to_hmac_btcoex_rx_delba_trigger_event_stru *pst_dmac_to_hmac_btcoex_rx_delba = OAL_PTR_NULL;
    oal_uint32 ul_ret;

    pst_hmac_device = hmac_res_get_mac_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(pst_hmac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_assoc_vap_id, OAM_SF_COEX, "{hmac_btcoex_syn::pst_hmac_device is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_to_hmac_btcoex_rx_delba = (dmac_to_hmac_btcoex_rx_delba_trigger_event_stru *)puc_param;

    pst_hmac_device->st_dmac_to_hmac_btcoex_rx_delba.uc_need_delba = pst_dmac_to_hmac_btcoex_rx_delba->uc_need_delba;
    pst_hmac_device->st_dmac_to_hmac_btcoex_rx_delba.uc_ba_size = pst_dmac_to_hmac_btcoex_rx_delba->uc_ba_size;
    if (pst_hmac_device->st_dmac_to_hmac_btcoex_rx_delba.uc_need_delba) {
        ul_ret = hmac_btcoex_delba_foreach_user(pst_mac_vap);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX,
                "{hmac_btcoex_syn:delba send failed:ul_ret: %d}", ul_ret);
            return ul_ret;
        }
    }
    return OAL_SUCC;
}
#endif

oal_uint32  hmac_atcmdsrv_fem_pa_response(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                           *pst_hmac_vap;
    dmac_atcmdsrv_atcmd_response_event      *pst_atcmdsrv_fem_pa_response_event = OAL_PTR_NULL;
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_atcmdsrv_fem_pa_response::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);
    pst_atcmdsrv_fem_pa_response_event = (dmac_atcmdsrv_atcmd_response_event *)(puc_param);
    if (pst_atcmdsrv_fem_pa_response_event->uc_event_id == OAL_ATCMDSRV_FEM_PA_INFO_EVENT) {
        pst_hmac_vap->st_atcmdsrv_get_status.ul_check_fem_pa_status = pst_atcmdsrv_fem_pa_response_event->ul_event_para;
    }
    /* ����wal_sdt_recv_reg_cmd�ȴ��Ľ��� */
    pst_hmac_vap->st_atcmdsrv_get_status.uc_check_fem_pa_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

    return OAL_SUCC;
}

oal_uint32  hmac_atcmdsrv_dbb_num_response(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                           *pst_hmac_vap;
    dmac_atcmdsrv_atcmd_response_event       *pst_atcmdsrv_dbb_num_response_event = OAL_PTR_NULL;
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_atcmdsrv_dbb_num_response::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);
    pst_atcmdsrv_dbb_num_response_event = (dmac_atcmdsrv_atcmd_response_event *)(puc_param);
    if (pst_atcmdsrv_dbb_num_response_event->uc_event_id == OAL_ATCMDSRV_DBB_NUM_INFO_EVENT) {
        pst_hmac_vap->st_atcmdsrv_get_status.ul_dbb_num = pst_atcmdsrv_dbb_num_response_event->ul_event_para;
    }
    /* ����wal_sdt_recv_reg_cmd�ȴ��Ľ��� */
    pst_hmac_vap->st_atcmdsrv_get_status.uc_get_dbb_completed_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

    return OAL_SUCC;
}

oal_uint32  hmac_atcmdsrv_get_rx_pkcg(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                           *pst_hmac_vap;
    dmac_atcmdsrv_atcmd_response_event      *pst_atcmdsrv_get_rx_pkcg_event = OAL_PTR_NULL;
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_atcmdsrv_get_rx_pkcg::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);
    pst_atcmdsrv_get_rx_pkcg_event = (dmac_atcmdsrv_atcmd_response_event *)(puc_param);
    if (pst_atcmdsrv_get_rx_pkcg_event->uc_event_id == OAL_ATCMDSRV_GET_RX_PKCG) {
        pst_hmac_vap->st_atcmdsrv_get_status.ul_rx_pkct_succ_num = pst_atcmdsrv_get_rx_pkcg_event->ul_event_para;
        pst_hmac_vap->st_atcmdsrv_get_status.s_rx_rssi = pst_atcmdsrv_get_rx_pkcg_event->s_always_rx_rssi;
    }
    /* ����wal_sdt_recv_reg_cmd�ȴ��Ľ��� */
    pst_hmac_vap->st_atcmdsrv_get_status.uc_get_rx_pkct_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

    return OAL_SUCC;
}

oal_uint32  hmac_wifitest_get_reg_info(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_reg_info_receive_event             *pst_dmac_reg_info_response_event = OAL_PTR_NULL;
    hmac_vap_stru                           *pst_hmac_vap;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_atcmdsrv_get_rx_pkcg::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);
    pst_dmac_reg_info_response_event = (hmac_reg_info_receive_event *)puc_param;
    g_st_hmac_reg_info_receive_event.ul_reg_info_num = pst_dmac_reg_info_response_event->ul_reg_info_num;
    if (g_st_hmac_reg_info_receive_event.ul_reg_info_num > REG_INFO_MAX_NUM) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "hmac_wifitest_get_reg_info:: reg numb is out of range[%d]",
            g_st_hmac_reg_info_receive_event.ul_reg_info_num);
        return OAL_FAIL;
    }
    while (pst_dmac_reg_info_response_event->ul_reg_info_num) {
        g_st_hmac_reg_info_receive_event.ul_val[pst_dmac_reg_info_response_event->ul_reg_info_num - 1] =
        pst_dmac_reg_info_response_event->ul_val[pst_dmac_reg_info_response_event->ul_reg_info_num - 1];
        pst_dmac_reg_info_response_event->ul_reg_info_num--;
    }

    g_st_hmac_reg_info_receive_event.ul_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

    return OAL_SUCC;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

oal_uint32  hmac_atcmdsrv_lte_gpio_check(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                           *pst_hmac_vap;
    dmac_atcmdsrv_atcmd_response_event      *pst_atcmdsrv_lte_gpio_check_event = OAL_PTR_NULL;
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_atcmdsrv_lte_gpio_check::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_atcmdsrv_lte_gpio_check_event = (dmac_atcmdsrv_atcmd_response_event *)(puc_param);
    if (pst_atcmdsrv_lte_gpio_check_event->uc_event_id == OAL_ATCMDSRV_LTE_GPIO_CHECK) {
        /* ����wal_sdt_recv_reg_cmd�ȴ��Ľ��� */
        pst_hmac_vap->st_atcmdsrv_get_status.uc_lte_gpio_check_flag = OAL_TRUE;
        OAL_WAIT_QUEUE_WAKE_UP(&(pst_hmac_vap->query_wait_q));
    }

    return OAL_SUCC;
}
#endif
OAL_STATIC OAL_CONST hmac_config_syn_stru g_ast_hmac_config_syn[] = {
    /* ͬ��ID                    ����2���ֽ�            �������� */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    {WLAN_CFGID_RESET_HW_OPERATE,       {0, 0}, hmac_reset_sys_event},
    {WLAN_CFGID_QUERY_STATION_STATS,    {0, 0}, hmac_proc_query_response_event},
    {WLAN_CFGID_THRUPUT_INFO,           {0, 0}, hmac_get_thruput_info},
#if defined(_PRE_WLAN_FEATURE_BTCOEX) || defined(_PRE_WLAN_FEATURE_1131K_BTCOEX)
    {WLAN_CFGID_BTCOEX_RX_DELBA_TRIGGER, {0, 0}, hmac_btcoex_rx_delba_trigger},
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
    {WLAN_CFGID_BTCOEX_AUTO_FREQ_ENABLE, {0, 0}, hmac_btcoex_rx_auto_freq_enable},
#endif
    {WLAN_CFGID_PSM_GET_TXRX_INFO,       {0, 0}, hmac_psm_chr_get_chip_info},
    {WLAN_CFGID_PSM_GET_BEACON_INFO,     {0, 0}, hmac_psm_get_beacon_info},
#endif
#endif
    {WLAN_CFGID_CHECK_FEM_PA,           {0, 0}, hmac_atcmdsrv_fem_pa_response},
    {WLAN_CFGID_GET_VERSION,            {0, 0}, hmac_atcmdsrv_dbb_num_response},
    {WLAN_CFGID_RX_FCS_INFO,            {0, 0}, hmac_atcmdsrv_get_rx_pkcg},
    {WLAN_CFGID_REG_INFO,               {0, 0}, hmac_wifitest_get_reg_info},
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    {WLAN_CFGID_CHECK_LTE_GPIO,              {0, 0}, hmac_atcmdsrv_lte_gpio_check},
#endif
    //    {WLAN_CFGID_ADD_VAP,                   {0, 0},         hmac_add_vap_sysnc},
    {WLAN_CFGID_CFG80211_MGMT_TX_STATUS,   {0, 0},         hmac_mgmt_tx_event_status},
    {WLAN_CFGID_BUTT,                      {0, 0},         OAL_PTR_NULL},
};


oal_uint32  hmac_event_config_syn(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru             *pst_event = OAL_PTR_NULL;
    frw_event_hdr_stru         *pst_event_hdr = OAL_PTR_NULL;
    dmac_to_hmac_cfg_msg_stru  *pst_dmac2hmac_msg = OAL_PTR_NULL;
    mac_vap_stru               *pst_mac_vap = OAL_PTR_NULL;
    mac_device_stru            *pst_mac_device = OAL_PTR_NULL;
    oal_uint32                  ul_ret;
    oal_uint16                  us_cfgid;

    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_event_config_syn::pst_event_mem null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ�¼� */
    pst_event         = (frw_event_stru *)pst_event_mem->puc_data;
    pst_event_hdr     = &(pst_event->st_event_hdr);
    pst_dmac2hmac_msg = (dmac_to_hmac_cfg_msg_stru *)pst_event->auc_event_data;

    OAM_INFO_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_CFG,
        "{hmac_event_config_syn::a dmac config syn event occur, cfg_id=%d.}", pst_dmac2hmac_msg->en_syn_id);
    /* ��ȡdmac vap */
    pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_event_hdr->uc_vap_id);
    if (pst_mac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_CFG, "{hmac_event_config_syn::pst_mac_vap null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡmac device */
    pst_mac_device = (mac_device_stru *)mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_CFG, "{hmac_event_config_syn::pst_mac_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ���cfg id��Ӧ�Ĳ������� */
    for (us_cfgid = 0; g_ast_hmac_config_syn[us_cfgid].en_cfgid != WLAN_CFGID_BUTT; us_cfgid++) {
        if (g_ast_hmac_config_syn[us_cfgid].en_cfgid == pst_dmac2hmac_msg->en_syn_id) {
            break;
        }
    }

    /* �쳣�����cfgid��g_ast_dmac_config_syn�в����� */
    if (g_ast_hmac_config_syn[us_cfgid].en_cfgid == WLAN_CFGID_BUTT) {
        OAM_WARNING_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_CFG, "{hmac_event_config_syn::invalid en_cfgid[%d].",
            pst_dmac2hmac_msg->en_syn_id);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ִ�в������� */
    ul_ret = g_ast_hmac_config_syn[us_cfgid].p_set_func(pst_mac_vap, (oal_uint8)(pst_dmac2hmac_msg->us_len),
        (oal_uint8 *)pst_dmac2hmac_msg->auc_msg_body);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG2(pst_event_hdr->uc_vap_id, OAM_SF_CFG,
            "{hmac_event_config_syn::p_set_func failed, ul_ret=%d en_syn_id=%d.", ul_ret, pst_dmac2hmac_msg->en_syn_id);
        return ul_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_DEBUG_MODE


oal_uint32  hmac_config_scan_test(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_scan_test(pst_mac_vap, us_len, puc_param);

    return OAL_SUCC;
}
#endif


oal_uint32  hmac_config_bgscan_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_bgscan_enable(pst_mac_vap, us_len, puc_param);
}


oal_uint32  hmac_config_mcs_set_check_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_bool_enum_uint8 en_mcs_set_check_enable;

    if (OAL_UNLIKELY(puc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_mcs_set_check_enable::puc_param null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }
    en_mcs_set_check_enable = *((oal_bool_enum_uint8 *)puc_param);  /* 11n���ʼ����ʹ��λ */

    if (en_mcs_set_check_enable) {
        g_ht_mcs_set_check = OAL_TRUE;
    } else {
        g_ht_mcs_set_check = OAL_FALSE;
    }

    OAM_WARNING_LOG1(0, OAM_SF_SCAN, "hmac_config_mcs_set_check_enable: g_ht_mcs_set_check_flag = %d.",
        g_ht_mcs_set_check);

    return OAL_SUCC;
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && defined(_PRE_WLAN_CHIP_TEST_ALG)

OAL_STATIC oal_ssize_t hmac_alg_test_result_proc_read(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
#define ALG_READ_STR  "test read"
    oal_uint32                 ul_ret;
    mac_ioctl_alg_config_stru *pst_config;
    hmac_vap_stru             *pst_vap = mac_res_get_hmac_vap(1);
    oal_uint8                  auc_param[128];
    oal_uint16                 us_config_len = (oal_uint16)(OAL_SIZEOF(mac_ioctl_alg_config_stru) +
                                                                       OAL_STRLEN(ALG_READ_STR) + 1);

    pst_config = (mac_ioctl_alg_config_stru *)auc_param;
    if (pst_vap == OAL_PTR_NULL) {
        return 0;
    }
    memset_s(pst_config, 128, 0, 128);

    pst_config->uc_argc = 2;
    pst_config->auc_argv_offset[0] = 0;
    pst_config->auc_argv_offset[1] = 5;
    snprintf_s((oal_int8 *)(auc_param + OAL_SIZEOF(mac_ioctl_alg_config_stru)),
               128 - OAL_SIZEOF(mac_ioctl_alg_config_stru),
               (128 - OAL_SIZEOF(mac_ioctl_alg_config_stru)) - 1, ALG_READ_STR);

    ul_ret = hmac_config_alg(&pst_vap->st_vap_base_info, us_config_len, auc_param);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_DBAC,
            "{hmac_alg_test_result_proc_read:: failed[%d].}", ul_ret);
        return 0;
    }

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
    /* wait dmac result */
    OAL_INTERRUPTIBLE_SLEEP_ON(&g_st_alg_test_hmac.st_wait_queue);
#endif

    /* DMAC���صĽ���У�ָʾ����Ϊ��֤ACT�������У�����һ��E */
    if (g_st_alg_test_hmac.auc_data[0] == 0xFF) {
        g_st_alg_test_hmac.auc_data[0] = 1;
        g_st_alg_test_hmac.auc_data[1] = 'E';
    }

    if (memcpy_s(buf, g_st_alg_test_hmac.auc_data[0], (void *)(g_st_alg_test_hmac.auc_data + 1),
                 g_st_alg_test_hmac.auc_data[0]) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_DBAC, "hmac_alg_test_result_proc_read::memcpy fail!");
        return 0;
    }

    OAL_IO_PRINT("DEBUG:: sysfs return len: %d\r\n", g_st_alg_test_hmac.auc_data[0]);

    return g_st_alg_test_hmac.auc_data[0];
}

oal_uint32  hmac_alg_test_result_process(frw_event_mem_stru  *pst_event_mem)
{
    frw_event_stru      *pst_event;
    hmac_vap_stru       *pst_hmac_vap;

    pst_event = (frw_event_stru  *)pst_event_mem->puc_data;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_event->st_event_hdr.uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_event->st_event_hdr.uc_vap_id, OAM_SF_ANY,
            "{hmac_alg_test_result_process::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (memcpy_s((oal_void *)g_st_alg_test_hmac.auc_data, HMAC_ALG_TEST_BUF_SIZE,
                 (const oal_void *)pst_event->auc_event_data, HMAC_ALG_TEST_BUF_SIZE) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hmac_alg_test_result_process::memcpy_s fail!");
        return OAL_FAIL;
    }

    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&g_st_alg_test_hmac.st_wait_queue);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_alg_test_result_create_proc(oal_void *p_proc_arg)
{
    /* hi1102-cb add sys for 51/02 */
    oal_int32           l_ret = OAL_SUCC;

    if (g_alg_test_sys_kobject == OAL_PTR_NULL) {
        g_alg_test_sys_kobject = kobject_create_and_add("alg", OAL_PTR_NULL);
        l_ret = sysfs_create_file(g_alg_test_sys_kobject, &dev_attr_alg_test_result.attr);
    }

    return l_ret;
}


OAL_STATIC oal_uint32 hmac_alg_test_result_delete_proc(void)
{
    if (g_alg_test_sys_kobject != OAL_PTR_NULL) {
        sysfs_remove_file(g_alg_test_sys_kobject, &dev_attr_alg_test_result.attr);
        kobject_del(g_alg_test_sys_kobject);
        g_alg_test_sys_kobject = OAL_PTR_NULL;
    }

    return OAL_SUCC;
}


oal_int32  hmac_alg_test_main_common_init(oal_void)
{
    oal_uint32  ul_ret;

    memset_s(&g_st_alg_test_hmac, OAL_SIZEOF(g_st_alg_test_hmac), 0,
             OAL_SIZEOF(g_st_alg_test_hmac));

    ul_ret = hmac_alg_test_result_create_proc(OAL_PTR_NULL);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{alg_test_main_common_init::hmac_alg_test_result_create_proc fail}");
        return (oal_int32)ul_ret;
    }

    OAL_WAIT_QUEUE_INIT_HEAD(&g_st_alg_test_hmac.st_wait_queue);

    return 0;
}

oal_int32  hmac_alg_test_main_common_exit(oal_void)
{
    oal_uint32  ul_ret;

    ul_ret = hmac_alg_test_result_delete_proc();
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{alg_test_main_common_exit::alg_test_result_delete_proc fail}");
        return -1;
    }
    return 0;
}
#endif

#ifdef _PRE_WLAN_FEATURE_STA_UAPSD

oal_uint32  hmac_config_set_uapsd_para(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_uapsd_sta_stru *pst_uapsd_param;
    oal_uint32              ul_ret;
    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "{hmac_config_set_uapsd_para:: pst_mac_vap/puc_param is null ptr!}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_uapsd_param = (mac_cfg_uapsd_sta_stru *)puc_param;

    /* uc_max_sp_len */
    if (pst_uapsd_param->uc_max_sp_len > 6) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_PWR, "{hmac_config_set_uapsd_para::uc_max_sp_len[%d] > 6!}",
            pst_uapsd_param->uc_max_sp_len);
        return OAL_FAIL;
    }

#ifdef _PRE_WLAN_FEATURE_STA_PM
    mac_vap_set_uapsd_para(pst_mac_vap, pst_uapsd_param);
#endif

    /***************************************************************************
        ���¼���DMAC��, ͬ��VAP����״̬��DMAC
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_UAPSD_PARA, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_uapsd_para::hmac_config_send_event failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}
#endif
#ifdef _PRE_WLAN_FEATURE_STA_PM

oal_uint32 hmac_config_set_sta_pm_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_SET_PS_MODE, us_len, puc_param);
}

oal_uint32  hmac_config_set_sta_pm_on(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                   ul_ret;
    mac_cfg_ps_mode_param_stru   st_ps_mode_param;
    hmac_vap_stru                *pst_hmac_vap;
    mac_cfg_ps_open_stru         *pst_sta_pm_open = (mac_cfg_ps_open_stru *)puc_param;

    pst_hmac_vap    = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PWR, "{hmac_config_set_sta_pm_on::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �л����ֶ�����Ϊpspollģʽ */
    if (pst_sta_pm_open->uc_pm_enable == MAC_STA_PM_MANUAL_MODE_ON) {
        pst_hmac_vap->uc_cfg_sta_pm_manual = OAL_TRUE;
        /* �ر��ֶ�����pspollģʽ,�ص�fastpsģʽ */
    } else if (pst_sta_pm_open->uc_pm_enable == MAC_STA_PM_MANUAL_MODE_OFF) {
        pst_hmac_vap->uc_cfg_sta_pm_manual = 0xFF;
    }

    pst_sta_pm_open->uc_pm_enable =
        (pst_sta_pm_open->uc_pm_enable > MAC_STA_PM_SWITCH_OFF) ? MAC_STA_PM_SWITCH_ON : MAC_STA_PM_SWITCH_OFF;

    st_ps_mode_param.uc_vap_ps_mode = pst_sta_pm_open->uc_pm_enable ?
                                      ((pst_hmac_vap->uc_cfg_sta_pm_manual != 0xFF) ? MIN_PSPOLL_PS : MIN_FAST_PS)
                                      : NO_POWERSAVE;

    /* ���·����õ͹���ģʽ */
    ul_ret = hmac_config_set_sta_pm_mode(pst_mac_vap, OAL_SIZEOF(st_ps_mode_param), (oal_uint8 *)&st_ps_mode_param);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_PWR, "sta_pm sta_pm mode[%d]fail", ul_ret);
        return ul_ret;
    }

    /* ���·��򿪵͹��� */
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_SET_STA_PM_ON, us_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_CHIP_TEST

oal_uint32 hmac_test_send_action(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_param)
{
    mac_cfg_send_action_param_stru  *pst_action_param;
    oal_uint32                       ul_ret;
    hmac_user_stru                  *pst_hmac_user   = OAL_PTR_NULL;
#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
    oal_bool_enum_uint8              en_is_protected = OAL_FALSE;
#endif

    if (pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL) {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "hmac_test_send_action:: pointer is null: pst_mac_vap[%d],puc_param[%d]",
            pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_action_param = (mac_cfg_send_action_param_stru *)puc_param;
    if (ETHER_IS_MULTICAST(pst_action_param->auc_mac_da) != OAL_TRUE) {
        pst_hmac_user = mac_vap_get_hmac_user_by_addr(pst_mac_vap, pst_action_param->auc_mac_da);
        if (pst_hmac_user == OAL_PTR_NULL) {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_test_send_action::pst_hmac_user null.}");
            return OAL_ERR_CODE_PTR_NULL;
        }

        if (pst_hmac_user->st_user_base_info.en_user_asoc_state != MAC_USER_STATE_ASSOC) {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_test_send_action::the user is unassociated.}");
            return OAL_FAIL;
        }
#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
        en_is_protected = pst_hmac_user->st_user_base_info.st_cap_info.bit_pmf_active;
#endif
    }

    /* Category */
    switch (pst_action_param->uc_category) {
        case MAC_ACTION_CATEGORY_SA_QUERY:
#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
            {
                if (ETHER_IS_MULTICAST(pst_action_param->auc_mac_da) == OAL_TRUE) {
                    break;
                }
                OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                    "hmac_test_send_action:: now start to send SA Query Request!");
                ul_ret = hmac_start_sa_query(pst_mac_vap, pst_hmac_user, en_is_protected);
                if (ul_ret != OAL_SUCC) {
                    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                        "{hmac_start_sa_query::hmac_start_sa_query failed[%d].}", ul_ret);
                    return ul_ret;
                }
            }
#endif
            break;
        default:
            break;
    }
    return OAL_SUCC;
}



oal_uint32  hmac_config_send_pspoll(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SEND_PSPOLL, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_send_pspoll::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_send_nulldata(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SEND_NULLDATA, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_send_nulldata::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_clear_all_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_CLEAR_ALL_STAT, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_clear_all_stat::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#endif /* #ifdef _PRE_WLAN_CHIP_TEST */

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

oal_uint32 hmac_get_thruput_info(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    dmac_thruput_info_sync_stru  *pst_thruput_info;

    pst_thruput_info = (dmac_thruput_info_sync_stru *)puc_param;

    if (pst_thruput_info != OAL_PTR_NULL) {
        OAL_IO_PRINT("interval cycles: %u \n",          pst_thruput_info->ul_cycles);
        OAL_IO_PRINT("sw tx succ num: %u \n",           pst_thruput_info->ul_sw_tx_succ_num);
        OAL_IO_PRINT("sw tx fail num: %u \n",           pst_thruput_info->ul_sw_tx_fail_num);
        OAL_IO_PRINT("sw rx ampdu succ num: %u \n",     pst_thruput_info->ul_sw_rx_ampdu_succ_num);
        OAL_IO_PRINT("sw rx mpdu succ num: %u \n",      pst_thruput_info->ul_sw_rx_mpdu_succ_num);
        OAL_IO_PRINT("sw rx fail num: %u \n",           pst_thruput_info->ul_sw_rx_ppdu_fail_num);
        OAL_IO_PRINT("hw rx ampdu fcs fail num: %u \n", pst_thruput_info->ul_hw_rx_ampdu_fcs_fail_num);
        OAL_IO_PRINT("hw rx mpdu fcs fail num: %u \n",  pst_thruput_info->ul_hw_rx_mpdu_fcs_fail_num);
        return OAL_SUCC;
    } else {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_get_thruput_info::pst_thruput_info null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
}
#endif

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)


oal_uint32 hmac_enable_pmf(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_param)
{
    oal_bool_enum_uint8        en_pmf_active;
    wlan_pmf_cap_status_uint8 *puc_pmf_cap;
    oal_dlist_head_stru       *pst_entry;
    oal_dlist_head_stru       *pst_user_list_head;
    mac_user_stru             *pst_user_tmp;

    OAL_IO_PRINT("hmac_enable_pmf: func start!");
    if (pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hmac_enable_pmf:: pointer is null");
        return OAL_ERR_CODE_PTR_NULL;
    }

    puc_pmf_cap = (wlan_pmf_cap_status_uint8 *)puc_param;

    switch (*puc_pmf_cap) {
        case MAC_PMF_DISABLED:
        {
            mac_mib_set_dot11RSNAMFPC(pst_mac_vap, OAL_FALSE);
            mac_mib_set_dot11RSNAMFPR(pst_mac_vap, OAL_FALSE);
            mac_mib_set_dot11RSNAActivated(pst_mac_vap, OAL_FALSE);
            en_pmf_active = OAL_FALSE;
        }
        break;
        case MAC_PMF_ENABLED:
        {
            mac_mib_set_dot11RSNAMFPC(pst_mac_vap, OAL_TRUE);
            mac_mib_set_dot11RSNAMFPR(pst_mac_vap, OAL_FALSE);
            mac_mib_set_dot11RSNAActivated(pst_mac_vap, OAL_TRUE);
            return OAL_SUCC;
        }
        case MAC_PME_REQUIRED:
        {
            mac_mib_set_dot11RSNAMFPC(pst_mac_vap, OAL_TRUE);
            mac_mib_set_dot11RSNAMFPR(pst_mac_vap, OAL_TRUE);
            mac_mib_set_dot11RSNAActivated(pst_mac_vap, OAL_TRUE);
            en_pmf_active = OAL_TRUE;
        }
        break;
        default:
        {
            OAL_IO_PRINT("hmac_enable_pmf: commend error!");
            return OAL_FALSE;
        }
    }

    if (pst_mac_vap->en_vap_state == MAC_VAP_STATE_UP) {
        pst_user_list_head = &(pst_mac_vap->st_mac_user_list_head);

        for (pst_entry = pst_user_list_head->pst_next; pst_entry != pst_user_list_head;) {
            pst_user_tmp = OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_dlist);

            /* ָ��˫��������һ���ڵ� */
            pst_entry = pst_entry->pst_next;
            if (pst_user_tmp == OAL_PTR_NULL) {
                OAM_ERROR_LOG0(0, OAM_SF_ANY, "hmac_enable_pmf:: pst_user_tmp is null");
                return OAL_ERR_CODE_PTR_NULL;
            }
            mac_user_set_pmf_active(pst_user_tmp, en_pmf_active);
        }
    }

    OAL_IO_PRINT("hmac_enable_pmf: func end!");

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_HS20

oal_uint32  hmac_config_set_qos_map(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8                      uc_idx;
    hmac_cfg_qos_map_param_stru   *pst_qos_map;
    hmac_vap_stru  *pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    if (pst_hmac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL) {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "hmac_config_set_qos_map:: pointer is null: pst_hmac_vap[%d],puc_param[%d]",
            pst_hmac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_qos_map = (hmac_cfg_qos_map_param_stru *)puc_param;
    OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_qos_map:uc_num_dscp_except=%d, uc_valid=%d}",
                  pst_qos_map->uc_num_dscp_except,
                  pst_qos_map->uc_valid);

    /* �ж�QOS MAP SET��ʹ�ܿ����Ƿ�� */
    if (!pst_qos_map->uc_valid) {
        return OAL_FAIL;
    }

    /* ����·���QoS Map Set�����е�DSCP Exception fields �Ƿ񳬹������Ŀ21 */
    if (pst_qos_map->uc_num_dscp_except > MAX_DSCP_EXCEPT) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_qos_map:: input exceeds maximum:"
            "pst_qos_map->num_dscp_except[%d]", pst_qos_map->uc_num_dscp_except);
        return OAL_FAIL;
    }
    /* �ж�DSCP Exception fields�Ƿ�Ϊ�� */
    if ((pst_qos_map->uc_num_dscp_except != 0)) {
        pst_hmac_vap->st_cfg_qos_map_param.uc_num_dscp_except = pst_qos_map->uc_num_dscp_except;
        for (uc_idx = 0; uc_idx < pst_qos_map->uc_num_dscp_except; uc_idx++) {
            pst_hmac_vap->st_cfg_qos_map_param.auc_dscp_exception[uc_idx] = pst_qos_map->auc_dscp_exception[uc_idx];
            pst_hmac_vap->st_cfg_qos_map_param.auc_dscp_exception_up[uc_idx] =
                pst_qos_map->auc_dscp_exception_up[uc_idx];
        }
    }

    /* ����DSCP Exception format�е�User Priority��HIGHT��LOW VALUEֵ */
    for (uc_idx = 0; uc_idx < MAX_QOS_UP_RANGE; uc_idx++) {
        pst_hmac_vap->st_cfg_qos_map_param.auc_up_high[uc_idx] = pst_qos_map->auc_up_high[uc_idx];
        pst_hmac_vap->st_cfg_qos_map_param.auc_up_low[uc_idx] = pst_qos_map->auc_up_low[uc_idx];
    }
    return OAL_SUCC;
}
#endif // _PRE_WLAN_FEATURE_HS20

#ifdef _PRE_WLAN_FEATURE_P2P

oal_uint32  hmac_config_set_p2p_ps_ops(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    mac_cfg_p2p_ops_param_stru *pst_p2p_ops;
    pst_p2p_ops = (mac_cfg_p2p_ops_param_stru *)puc_param;
    OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_p2p_ps_ops:ctrl:%d, ct_window:%d\r\n}",
                  pst_p2p_ops->en_ops_ctrl,
                  pst_p2p_ops->uc_ct_window);

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_P2P_PS_OPS, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_p2p_ps_ops::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_p2p_ps_noa(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    mac_cfg_p2p_noa_param_stru  *pst_p2p_noa;
    pst_p2p_noa = (mac_cfg_p2p_noa_param_stru *)puc_param;
    OAM_INFO_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                  "{hmac_config_set_p2p_ps_noa:start_time:%d, duration:%d, interval:%d, count:%d}",
                  pst_p2p_noa->ul_start_time,
                  pst_p2p_noa->ul_duration,
                  pst_p2p_noa->ul_interval,
                  pst_p2p_noa->uc_count);
    /* ms to us */
    pst_p2p_noa->ul_start_time *= 1000;
    pst_p2p_noa->ul_duration   *= 1000;
    pst_p2p_noa->ul_interval   *= 1000;
    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_P2P_PS_NOA, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_p2p_ps_noa::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_p2p_ps_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                    ul_ret;
    mac_cfg_p2p_stat_param_stru  *pst_p2p_stat;
    pst_p2p_stat = (mac_cfg_p2p_stat_param_stru *)puc_param;
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "(hmac_config_set_p2p_ps_stat::ctrl %d\r\n}",
                  pst_p2p_stat->uc_p2p_statistics_ctrl);

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_P2P_PS_STAT, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_p2p_ps_stat::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif

#ifdef _PRE_WLAN_PROFLING_MIPS

oal_uint32 hmac_config_set_mips(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                 ul_ret;

    oal_mips_type_param_stru      *pst_mips_type_param;

    pst_mips_type_param = (oal_mips_type_param_stru *)puc_param;

    switch (pst_mips_type_param->l_mips_type) {
        case OAL_MIPS_TX:
        {
            if (pst_mips_type_param->l_switch == OAL_SWITCH_ON) {
                if (g_mips_tx_statistic.en_switch == OAL_SWITCH_OFF) {
                    g_mips_tx_statistic.en_switch = OAL_SWITCH_ON;
                    g_mips_tx_statistic.uc_flag |= BIT0;
                }
            } else if (pst_mips_type_param->l_switch == OAL_SWITCH_OFF) {
                if (g_mips_tx_statistic.en_switch == OAL_SWITCH_ON) {
                    g_mips_tx_statistic.en_switch = OAL_SWITCH_OFF;
                }
            }
        }
        break;

        case OAL_MIPS_RX:
        {
            if (pst_mips_type_param->l_switch == OAL_SWITCH_ON) {
                if (g_mips_rx_statistic.en_switch == OAL_SWITCH_OFF) {
                    g_mips_rx_statistic.en_switch = OAL_SWITCH_ON;
                }
            } else if (pst_mips_type_param->l_switch == OAL_SWITCH_OFF) {
                if (g_mips_rx_statistic.en_switch == OAL_SWITCH_ON) {
                    g_mips_rx_statistic.en_switch = OAL_SWITCH_OFF;
                }
            }
        }
        break;

        default:
        {
            OAL_IO_PRINT("hmac_config_set_mips: mips type is wrong!\r\n");
        }
    }

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_MIPS, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_set_mips::hmac_config_send_event fail[%d].", ul_ret);
    }

    return ul_ret;
}


oal_uint32 hmac_config_show_mips(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                 ul_ret;
    oal_int32 l_mips_type;

    l_mips_type = *((oal_int32 *)puc_param);

    switch (l_mips_type) {
        case OAL_MIPS_TX:
        {
            oal_profiling_tx_mips_show();
        }
        break;

        case OAL_MIPS_RX:
        {
            oal_profiling_rx_mips_show();
        }
        break;

        default:
        {
            OAL_IO_PRINT("hmac_config_show_mips: mips type is wrong!\r\n");
        }
    }

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SHOW_MIPS, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_show_mips::hmac_config_send_event fail[%d].", ul_ret);
    }

    return ul_ret;
}
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD

oal_uint32 hmac_config_enable_arp_offload(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                 ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ����DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_ENABLE_ARP_OFFLOAD, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_enable_arp_offload::hmac_config_send_event fail[%d].", ul_ret);
    }

    return ul_ret;
}


oal_uint32 hmac_config_set_ip_addr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                 ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ����DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_IP_ADDR, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_set_ip_addr::hmac_config_send_event fail[%d].", ul_ret);
    }

    return ul_ret;
}


oal_uint32 hmac_config_show_arpoffload_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                 ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ����DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SHOW_ARPOFFLOAD_INFO, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_show_arpoffload_info::hmac_config_send_event fail[%d].", ul_ret);
    }

    return ul_ret;
}
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

oal_uint32   hmac_config_cfg_vap_h2d(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32            ul_ret;
    mac_device_stru      *pst_dev = OAL_PTR_NULL;

    if (OAL_UNLIKELY((pst_mac_vap == OAL_PTR_NULL) || (puc_param == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_add_vap::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dev = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(pst_dev == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_config_cfg_vap_h2d::mac_res_get_dev fail. vap_id[%u]}",
            pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /***************************************************************************
    ���¼���DMAC��, ����dmac cfg vap
    ***************************************************************************/
    ul_ret = hmac_cfg_vap_send_event(pst_dev);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_cfg_vap_send_event::hmac_config_send_event fail[%d].", ul_ret);
    }

    return ul_ret;
}
#endif
#ifdef _PRE_WLAN_TCP_OPT

oal_uint32  hmac_config_get_tcp_ack_stream_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len,
    const oal_uint8 *puc_param)
{
    hmac_vap_stru    *pst_hmac_vap;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                       "{hmac_config_get_tcp_ack_stream_info fail: pst_hmac_vap is null}\r\n");
        return OAL_FAIL;
    }

    hmac_tcp_opt_ack_show_count(pst_hmac_vap);
    return OAL_SUCC;
}


oal_uint32  hmac_config_tx_tcp_ack_opt_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_val;
    hmac_device_stru    *pst_hmac_device;

    pst_hmac_device = hmac_res_get_mac_dev(pst_mac_vap->uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                       "{hmac_config_tx_tcp_ack_opt_enable fail: pst_hmac_device is null}\r\n");
        return OAL_FAIL;
    }

    ul_val = *((oal_uint32 *)puc_param);

    if (ul_val == 0) {
        pst_hmac_device->sys_tcp_tx_ack_opt_enable = OAL_FALSE;
    } else {
        pst_hmac_device->sys_tcp_tx_ack_opt_enable = OAL_TRUE;
    }
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_config_tx_tcp_ack_opt_enable:sys_tcp_tx_ack_opt_enable = %d}\r\n",
                     pst_hmac_device->sys_tcp_tx_ack_opt_enable);
    return OAL_SUCC;
}
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ

oal_uint32  hmac_set_device_freq(oal_uint8 uc_device_freq_type)
{
    oal_uint32                  ul_ret;
    config_device_freq_h2d_stru       st_device_freq_type;
    mac_vap_stru           *pst_mac_vap;

    pst_mac_vap  = mac_res_get_mac_vap(0);
    if (pst_mac_vap != OAL_PTR_NULL) {
        st_device_freq_type.uc_device_freq_type = uc_device_freq_type;

        /***************************************************************************
            ���¼���DMAC��, ͬ��VAP����״̬��DMAC
        ***************************************************************************/
        ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_DEVICE_FREQ,
            OAL_SIZEOF(config_device_freq_h2d_stru), (oal_uint8 *)(&st_device_freq_type));
        if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_set_device_freq::hmac_set_device_freq failed[%d].}", ul_ret);
        }
    } else {
        ul_ret = OAL_ERR_CODE_PTR_NULL;
    }

    return ul_ret;
}
#endif

oal_uint32  hmac_config_rx_tcp_ack_opt_enable(mac_vap_stru  *pst_mac_vap,
                                              oal_uint16    us_len,
                                              const uint8_t *puc_param)
{
    oal_uint32          ul_val;
    hmac_device_stru    *pst_hmac_device;

    pst_hmac_device = hmac_res_get_mac_dev(pst_mac_vap->uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                       "{hmac_config_rx_tcp_ack_opt_enable fail: pst_hmac_device is null}\r\n");
        return OAL_FAIL;
    }

    ul_val = *((oal_uint32 *)puc_param);

    if (ul_val == 0) {
        pst_hmac_device->sys_tcp_rx_ack_opt_enable = OAL_FALSE;
    } else {
        pst_hmac_device->sys_tcp_rx_ack_opt_enable = OAL_TRUE;
    }
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_config_rx_tcp_ack_opt_enable:sys_tcp_tx_ack_opt_enable = %d}\r\n",
                     pst_hmac_device->sys_tcp_rx_ack_opt_enable);
    return OAL_SUCC;
}

oal_uint32  hmac_config_tx_tcp_ack_limit(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const uint8_t *puc_param)
{
    oal_uint32          ul_val;
    hmac_vap_stru *pst_hmac_vap;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                       "{hmac_config_tx_tcp_ack_limit fail: pst_hmac_vap is null}\r\n");
        return OAL_FAIL;
    }

    ul_val = *((oal_uint32 *)puc_param);

    if (ul_val >= DEFAULT_TX_TCP_ACK_THRESHOLD) {
        pst_hmac_vap->st_hamc_tcp_ack[HCC_TX].filter_info.ul_ack_limit = DEFAULT_TX_TCP_ACK_THRESHOLD;
    } else {
        pst_hmac_vap->st_hamc_tcp_ack[HCC_TX].filter_info.ul_ack_limit = ul_val;
    }
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_config_tx_tcp_ack_limit:ul_ack_limit = %ld}\r\n",
                     pst_hmac_vap->st_hamc_tcp_ack[HCC_TX].filter_info.ul_ack_limit);
    return OAL_SUCC;
}

oal_uint32  hmac_config_rx_tcp_ack_limit(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32          ul_val;
    hmac_vap_stru *pst_hmac_vap;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                       "{hmac_config_rx_tcp_ack_limit fail: pst_hmac_vap is null}\r\n");
        return OAL_FAIL;
    }

    ul_val = *((oal_uint32 *)puc_param);

    if (ul_val >= DEFAULT_RX_TCP_ACK_THRESHOLD) {
        pst_hmac_vap->st_hamc_tcp_ack[HCC_RX].filter_info.ul_ack_limit = DEFAULT_RX_TCP_ACK_THRESHOLD;
    } else {
        pst_hmac_vap->st_hamc_tcp_ack[HCC_RX].filter_info.ul_ack_limit = ul_val;
    }
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_config_rx_tcp_ack_limit:ul_ack_limit = %ld}\r\n",
                     pst_hmac_vap->st_hamc_tcp_ack[HCC_RX].filter_info.ul_ack_limit);
    return OAL_SUCC;
}

#endif

#if ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 35)) || \
    (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION))
#ifdef _PRE_WLAN_DFT_STAT

oal_uint32  hmac_config_set_performance_log_switch (mac_vap_stru *pst_mac_vap, wlan_cfgid_enum_uint16 en_cfg_id,
    oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;
    oal_uint8                       uc_loop_index;
    mac_cfg_set_performance_log_switch_stru *pst_set_performance_log_switch =
        (mac_cfg_set_performance_log_switch_stru *)puc_param;

    /* ���������VAP, ֱ�ӷ��� */
    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_CONFIG) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_performance_log_switch::this is config vap! can't set.}");
        return OAL_FAIL;
    }
    if (pst_set_performance_log_switch->uc_performance_log_switch_type >= DFX_PERFORMANCE_LOG_BUTT) {
        for (uc_loop_index = 0; uc_loop_index < DFX_PERFORMANCE_LOG_BUTT; uc_loop_index++) {
            DFX_SET_PERFORMANCE_LOG_SWITCH_ENABLE(uc_loop_index, pst_set_performance_log_switch->uc_value);
        }
    } else {
        DFX_SET_PERFORMANCE_LOG_SWITCH_ENABLE(pst_set_performance_log_switch->uc_performance_log_switch_type,
            pst_set_performance_log_switch->uc_value);
    }

    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
        "{hmac_config_set_performance_log_switch::log_switch type:%u, value:%u.}",
        pst_set_performance_log_switch->uc_performance_log_switch_type, pst_set_performance_log_switch->uc_value);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_PERFORMANCE_LOG_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_performance_log_switch::hmac_config_send_event failed[%d].}", ul_ret);
    }
#endif

    return ul_ret;
}
#endif
#endif
#ifdef _PRE_WLAN_FEATURE_P2P

oal_uint32  hmac_find_p2p_listen_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8 *puc_p2p_ie = OAL_PTR_NULL;
    oal_uint8 *puc_listen_channel_ie = OAL_PTR_NULL;

    /* ����P2P IE��Ϣ */
    puc_p2p_ie = mac_find_vendor_ie(MAC_WLAN_OUI_WFA, MAC_WLAN_OUI_TYPE_WFA_P2P, puc_param, (oal_int32)us_len);
    if (puc_p2p_ie == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_find_p2p_listen_channel::p2p ie is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ����У�� */
    if (puc_p2p_ie[1] < MAC_P2P_MIN_IE_LEN) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_find_p2p_listen_channel::invalid p2p ie len[%d].}", puc_p2p_ie[1]);
        return OAL_FAIL;
    }

    /* ����P2P Listen channel��Ϣ */
    puc_listen_channel_ie = mac_find_p2p_attribute(MAC_P2P_ATTRIBUTE_LISTEN_CHAN, puc_p2p_ie + 6, (puc_p2p_ie[1] - 4));
    if (puc_listen_channel_ie == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_find_p2p_listen_channel::p2p listen channel ie is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* listen channel����У�飬��� */
    if ((oal_int32)((puc_listen_channel_ie[2] << 8) + puc_listen_channel_ie[1]) != MAC_P2P_LISTEN_CHN_ATTR_LEN) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_find_p2p_listen_channel::invalid p2p listen channel ie len[%d].}",
            (oal_int32)((puc_listen_channel_ie[2] << 8) + puc_listen_channel_ie[1]));
        return OAL_FAIL;
    }

    /* ��ȡP2P Listen channel��Ϣ */
    pst_mac_vap->uc_p2p_listen_channel = puc_listen_channel_ie[7];
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_find_p2p_listen_channel::END CHANNEL[%d].}",
                  pst_mac_vap->uc_p2p_listen_channel);

    return OAL_SUCC;
}
#endif

oal_void hmac_config_del_p2p_ie(oal_uint8 *puc_ie, oal_uint32 *pul_ie_len)
{
    oal_uint8       *puc_p2p_ie = OAL_PTR_NULL;
    oal_uint32       ul_p2p_ie_len;
    oal_uint8       *puc_ie_end = OAL_PTR_NULL;
    oal_uint8       *puc_p2p_ie_end = OAL_PTR_NULL;

    if ((puc_ie == OAL_PTR_NULL) || (pul_ie_len == OAL_PTR_NULL) || (*pul_ie_len == 0)) {
        return;
    }

    puc_p2p_ie = mac_find_vendor_ie(MAC_WLAN_OUI_WFA, MAC_WLAN_OUI_TYPE_WFA_P2P, puc_ie, (oal_int32)(*pul_ie_len));
    if ((puc_p2p_ie == OAL_PTR_NULL) || (puc_p2p_ie[1] < MAC_P2P_MIN_IE_LEN)) {
        return;
    }

    ul_p2p_ie_len = puc_p2p_ie[1] + MAC_IE_HDR_LEN;

    /* ��p2p ie ��������ݿ�����p2p ie ����λ�� */
    puc_ie_end     = (puc_ie + *pul_ie_len);
    puc_p2p_ie_end = (puc_p2p_ie + ul_p2p_ie_len);

    if (puc_ie_end >= puc_p2p_ie_end) {
        if (memmove_s(puc_p2p_ie, *pul_ie_len, puc_p2p_ie_end, (oal_uint32)(puc_ie_end - puc_p2p_ie_end)) != EOK) {
            OAM_ERROR_LOG0(0, OAM_SF_CFG, "hmac_config_del_p2p_ie::memmove fail!");
            return;
        }
        *pul_ie_len -= ul_p2p_ie_len;
    }
    return;
}
#ifdef _PRE_WLAN_FEATURE_ROAM

oal_uint32 hmac_config_roam_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru     *pst_hmac_vap;
    oal_uint8          uc_enable;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_roam_enable::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_enable = (*puc_param == 0) ? 0 : 1;

    return hmac_roam_enable(pst_hmac_vap, uc_enable);
}


oal_uint32 hmac_config_roam_band(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru     *pst_hmac_vap;
    oal_uint8          uc_band;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_roam_band::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_band = *puc_param;

    return hmac_roam_band(pst_hmac_vap, uc_band);
}


oal_uint32 hmac_config_roam_org(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru     *pst_hmac_vap;
    oal_uint8          uc_scan_orthogonal;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_roam_band::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_scan_orthogonal = *puc_param;

    return hmac_roam_org(pst_hmac_vap, uc_scan_orthogonal);
}



oal_uint32 hmac_config_roam_start(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru     *pst_hmac_vap;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_roam_enable::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    return hmac_roam_start(pst_hmac_vap);
}


oal_uint32 hmac_config_roam_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru     *pst_hmac_vap;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_roam_enable::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    return hmac_roam_show(pst_hmac_vap);
}
#endif // _PRE_WLAN_FEATURE_ROAM

#ifdef _PRE_WLAN_FEATURE_11R


oal_uint32 hmac_config_set_ft_ies(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                      *pst_hmac_vap;
    mac_cfg80211_ft_ies_stru           *pst_mac_ft_ies;
    oal_app_ie_stru                     st_ft_ie;
    oal_uint32                          ul_ret;
    oal_uint16                          us_md_id;

    if ((pst_mac_vap == OAL_PTR_NULL) || (puc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_set_ft_ies::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_ft_ies::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_ft_ies = (mac_cfg80211_ft_ies_stru *)puc_param;
    ul_ret = mac_mib_get_md_id(pst_mac_vap, &us_md_id);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_ft_ies::get_md_id fail[%d].}", ul_ret);
        return ul_ret;
    }

    if (us_md_id != pst_mac_ft_ies->us_mdid) {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                       "{hmac_config_set_ft_ies::UNEXPECTED mdid[%d/%d].}", pst_mac_ft_ies->us_mdid, us_md_id);
        return OAL_FAIL;
    }

    st_ft_ie.en_app_ie_type   = OAL_APP_FT_IE;
    st_ft_ie.ul_ie_len        = pst_mac_ft_ies->us_len;
    if (memcpy_s(st_ft_ie.auc_ie, WLAN_WPS_IE_MAX_SIZE, pst_mac_ft_ies->auc_ie, pst_mac_ft_ies->us_len) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "hmac_config_set_ft_ies::memcpy_s fail!");
        return OAL_FAIL;
    }

    ul_ret = hmac_config_set_app_ie_to_vap(pst_mac_vap, &st_ft_ie, OAL_APP_FT_IE);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_ft_ies::set_app_ie FAIL[%d].}", ul_ret);
        return ul_ret;
    }

    OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_ft_ies::set_app_ie OK LEN[%d].}",
        pst_mac_ft_ies->us_len);

    hmac_roam_reassoc(pst_hmac_vap);

    return OAL_SUCC;
}
#endif // _PRE_WLAN_FEATURE_11R

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST

oal_uint32 hmac_config_enable_2040bss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_device_stru       *pst_mac_device = OAL_PTR_NULL;
    oal_bool_enum_uint8    en_2040bss_switch;

    if (OAL_UNLIKELY((pst_mac_vap == OAL_PTR_NULL) || (puc_param == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_enable_2040bss::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_enable_2040bss:: pst_mac_device is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    en_2040bss_switch = (*puc_param == 0) ? OAL_FALSE : OAL_TRUE;
    mac_set_2040bss_switch(pst_mac_device, en_2040bss_switch);

    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_enable_2040bss:: set 2040bss switch[%d]}",
        en_2040bss_switch);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_2040BSS_ENABLE, us_len, puc_param);
#else
    return OAL_SUCC;
#endif
}
#endif /* _PRE_WLAN_FEATURE_20_40_80_COEXIST */

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
oal_uint32 hmac_config_set_auto_freq_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_set_auto_freq_stru* pst_set_auto_freq = (mac_cfg_set_auto_freq_stru *)puc_param;

    if (pst_set_auto_freq->uc_cmd_type == CMD_SET_AUTO_FREQ_ENDABLE) {
        if (pst_set_auto_freq->uc_value == FREQ_LOCK_ENABLE) {
            hmac_set_auto_freq_mod(FREQ_LOCK_ENABLE);
        } else {
            hmac_set_auto_freq_mod(FREQ_LOCK_DISABLE);
            hmac_set_device_freq(FREQ_HIGHEST_DBG_480MHZ);
        }
    } else if (pst_set_auto_freq->uc_cmd_type == CMD_SET_CPU_FREQ_VALUE) {
        if (pst_set_auto_freq->uc_value == SCALING_MAX_FREQ) {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
            hmac_set_cpu_freq_raw(SCALING_MAX_FREQ, 1805000);
#endif
        } else {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
            hmac_set_cpu_freq_raw(SCALING_MIN_FREQ, 807000);
#endif
        }
    } else if (pst_set_auto_freq->uc_cmd_type == CMD_SET_DDR_FREQ_VALUE) {
        if (pst_set_auto_freq->uc_value == SCALING_MAX_FREQ) {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
            hmac_set_ddr_freq_raw(SCALING_MAX_FREQ, 1805000);
#endif
        } else {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
            hmac_set_ddr_freq_raw(SCALING_MIN_FREQ, 807000);
#endif
        }
    } else if (pst_set_auto_freq->uc_cmd_type == CMD_SET_DEVICE_FREQ_VALUE) {
        if (pst_set_auto_freq->uc_value == FREQ_IDLE) {
            hmac_set_device_freq(FREQ_IDLE);
        } else if (pst_set_auto_freq->uc_value == FREQ_MIDIUM) {
            hmac_set_device_freq(FREQ_MIDIUM);
        } else if (pst_set_auto_freq->uc_value == FREQ_HIGHER) {
            hmac_set_device_freq(FREQ_HIGHER);
        } else if (pst_set_auto_freq->uc_value == FREQ_HIGHEST) {
            hmac_set_device_freq(FREQ_HIGHEST);
        } else {
            hmac_set_device_freq(FREQ_HIGHEST_DBG_480MHZ);
        }
    } else if (pst_set_auto_freq->uc_cmd_type == CMD_SET_AUTO_FREQ_DEBUG_PRINT) {
        if (pst_set_auto_freq->uc_value == OAL_TRUE) {
            hmac_set_auto_freq_debug_print(OAL_TRUE);
        } else {
            hmac_set_auto_freq_debug_print(OAL_FALSE);
        }
    } else if (pst_set_auto_freq->uc_cmd_type == CMD_SET_AUTO_BYPASS_DEVICE_AUTO_FREQ) {
        if (pst_set_auto_freq->uc_value == OAL_TRUE) {
            hmac_set_auto_freq_bypass_device_auto_freq(OAL_TRUE);
            hmac_set_device_freq(FREQ_HIGHEST_DBG_480MHZ);
        } else {
            hmac_set_auto_freq_bypass_device_auto_freq(OAL_FALSE);
        }
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_set_device_freq:parameter error!}\r\n");
    }
    OAM_WARNING_LOG2(0, OAM_SF_ANY, "{hmac_config_set_auto_freq_enable:set_auto_freq_enable:uc_cmd_type = %d,"
        "uc_value = %d}\r\n", pst_set_auto_freq->uc_cmd_type, pst_set_auto_freq->uc_value);
    return OAL_SUCC;
}

#endif

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1131C_HOST)

OAL_STATIC oal_uint32 hmac_get_max_lauch_cap(mac_regclass_info_stru *pst_regclass_info, oal_int32 cfg_id,
    oal_uint8 uc_parm, oal_bool_enum_uint8 en_edge_flag)
{
    oal_uint8 uc_max_tx_pw;

    /* ���ƻ���ȡֵ��λΪ0.1dBm��ͳһʹ��dBm��λ��� */
    /* ������TPC�Լ����ƻ�������������� */
    uc_max_tx_pw = OAL_MIN(OAL_MIN(pst_regclass_info->uc_max_reg_tx_pwr, pst_regclass_info->uc_max_tx_pwr),
        uc_parm / 10);

    /* ���ݶ��ƻ����ȡ�ߴ������߿ڷ��书�� */
    return (en_edge_flag == OAL_TRUE && cfg_id) ?
        OAL_MIN(uc_max_tx_pw, hwifi_get_init_value(CUS_TAG_DTS, cfg_id) / 10) : uc_max_tx_pw;
}

OAL_STATIC oal_int32 hmac_config_lauch_cap_show_snprintf_one(oal_int8 *ac_tmp_buff, oal_uint32 buff_len,
    mac_regclass_info_stru *pst_regclass_info, oal_uint8  *pst_nc_params, oal_bool_enum_uint8 en_edge_flag)
{
    return snprintf_s(ac_tmp_buff, buff_len, buff_len - 1, "11b[1_2M:%d 5.5_11M:%d]\n"
        "11g[6_9M:%d 12_18M:%d 24_36M:%d 48M:%d 54M:%d]\n" "HT20_VHT20[MCS 0:%d 1_2:%d\n" "3_4:%d 5:%d 6:%d 7:%d]\n"
        "VHT20[MCS8:%d]\n" "HT40_VHT40[MCS 0:%d 1_2:%d\n" "3_4:%d 5:%d 6:%d 7:%d]\n" "VHT40[MCS8:%d MCS9:%d]\n"
        "HT40[MCS32:%d]\n",
        hmac_get_max_lauch_cap(pst_regclass_info, 0, *pst_nc_params, en_edge_flag),
        hmac_get_max_lauch_cap(pst_regclass_info, 0, *(pst_nc_params + 2), en_edge_flag),
        hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11G_TXPWR,
            *(pst_nc_params + 4), en_edge_flag),
        hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11G_TXPWR,
            *(pst_nc_params + 6), en_edge_flag),
        hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11G_TXPWR,
            *(pst_nc_params + 8), en_edge_flag),
        hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11G_TXPWR,
            *(pst_nc_params + 10), en_edge_flag),
        hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11G_TXPWR,
            *(pst_nc_params + 12), en_edge_flag),
        hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT20_TXPWR,
            *(pst_nc_params + 14), en_edge_flag),
        hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT20_TXPWR,
            *(pst_nc_params + 16), en_edge_flag),
        hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT20_TXPWR,
            *(pst_nc_params + 18), en_edge_flag),
        hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT20_TXPWR,
            *(pst_nc_params + 20), en_edge_flag),
        hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT20_TXPWR,
            *(pst_nc_params + 22), en_edge_flag),
        hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT20_TXPWR,
            *(pst_nc_params + 24), en_edge_flag),
        hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT20_TXPWR,
            *(pst_nc_params + 26), en_edge_flag),
        hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT40_TXPWR,
            *(pst_nc_params + 28), en_edge_flag),
        hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT40_TXPWR,
            *(pst_nc_params + 30), en_edge_flag),
        hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT40_TXPWR,
            *(pst_nc_params + 32), en_edge_flag),
        hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT40_TXPWR,
            *(pst_nc_params + 34), en_edge_flag),
        hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT40_TXPWR,
            *(pst_nc_params + 36), en_edge_flag),
        hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT40_TXPWR,
            *(pst_nc_params + 38), en_edge_flag),
        hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT40_TXPWR,
            *(pst_nc_params + 40), en_edge_flag),
        hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT40_TXPWR,
            *(pst_nc_params + 42), en_edge_flag),
        hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT40_TXPWR,
            *(pst_nc_params + 44), en_edge_flag));
}


OAL_STATIC oal_uint32 hmac_config_lauch_cap_show(mac_regclass_info_stru *pst_regclass_info,
    oal_uint8 channel_freq, oal_bool_enum_uint8 en_edge_flag)
{
    oal_int8    ac_tmp_buff[280] = {0};
    oal_uint8  *pst_nc_params;
    int32_t    ret;

    /* ��ȡ���ƻ���֧�ֹ��� */
    pst_nc_params  = hwifi_get_nvram_params();
    if ((pst_regclass_info == OAL_PTR_NULL) || (pst_nc_params == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_lauch_cap_print::null param");
        return OAL_FAIL;
    }

    /* ���ݹ����������ǰ֧������Ƶ���ģʽ�������ʵ����߿ڷ��书���Լ��ߴ������߿ڷ��书�� */
    switch (channel_freq) {
        case MAC_RC_START_FREQ_2:
            ret = hmac_config_lauch_cap_show_snprintf_one(ac_tmp_buff, sizeof(ac_tmp_buff),
                mac_regclass_info_stru *pst_regclass_info, oal_uint8  *pst_nc_params, oal_bool_enum_uint8 en_edge_flag);
            break;
#ifdef _PRE_WLAN_FEATURE_5G
        /*
         * ���ڶ��ƻ���������g_auc_nv_params[NUM_OF_NV_PARAMS]�ӵ�46λ��ʼ���5G������ֵ
         * �޸�ʱ����Ҫ��Ӧ�޸Ĵ˴���ʼֵ
         */
        case MAC_RC_START_FREQ_5:
            ret = snprintf_s(ac_tmp_buff, sizeof(ac_tmp_buff), sizeof(ac_tmp_buff) - 1, "11a[6_9M:%d 12_18M:%d 24_36M:%d 48M:%d 54M:%d]\n"
                "HT20_VHT20[MCS0:%d 1_2:%d 3_4:%d 5:%d 6:%d 7:%d]\n"
                "VHT20[MCS8:%d]\n" "HT40_VHT40[MCS 0:%d 1_2:%d 3_4:%d\n"
                "5:%d 6:%d 7:%d]\n" "VHT40[MCS8_9:%d]\n" "HT40[MCS32:%d]\n"
                "VHT80[MCS 0:%d 1_2:%d 3_4:%d\n" "5:%d 6:%d 7:%d 8_9:%d]\n",
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_11A_HT20_VHT20_TXPWR,
                    *pst_nc_params + 46, en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_11A_HT20_VHT20_TXPWR,
                    *(pst_nc_params + 48), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_11A_HT20_VHT20_TXPWR,
                    *(pst_nc_params + 50), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_11A_HT20_VHT20_TXPWR,
                    *(pst_nc_params + 52), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_11A_HT20_VHT20_TXPWR,
                    *(pst_nc_params + 54), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_11A_HT20_VHT20_TXPWR,
                    *(pst_nc_params + 56), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_11A_HT20_VHT20_TXPWR,
                    *(pst_nc_params + 58), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_11A_HT20_VHT20_TXPWR,
                    *(pst_nc_params + 60), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_11A_HT20_VHT20_TXPWR,
                    *(pst_nc_params + 62), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_11A_HT20_VHT20_TXPWR,
                    *(pst_nc_params + 64), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_11A_HT20_VHT20_TXPWR,
                    *(pst_nc_params + 66), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_11A_HT20_VHT20_TXPWR,
                    *(pst_nc_params + 68), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_HT40_VHT40_TXPWR,
                    *(pst_nc_params + 70), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_HT40_VHT40_TXPWR,
                    *(pst_nc_params + 72), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_HT40_VHT40_TXPWR,
                    *(pst_nc_params + 74), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_HT40_VHT40_TXPWR,
                    *(pst_nc_params + 76), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_HT40_VHT40_TXPWR,
                    *(pst_nc_params + 78), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_HT40_VHT40_TXPWR,
                    *(pst_nc_params + 80), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_HT40_VHT40_TXPWR, *(pst_nc_params + 82), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_HT40_VHT40_TXPWR, *(pst_nc_params + 84), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_VHT80_TXPWR, *(pst_nc_params + 86), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_VHT80_TXPWR, *(pst_nc_params + 88), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_VHT80_TXPWR, *(pst_nc_params + 90), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_VHT80_TXPWR, *(pst_nc_params + 82), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_VHT80_TXPWR, *(pst_nc_params + 84), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_VHT80_TXPWR, *(pst_nc_params + 84), en_edge_flag),
                hmac_get_max_lauch_cap(pst_regclass_info, WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_VHT80_TXPWR, *(pst_nc_params + 86), en_edge_flag));
            break;
#endif /* _PRE_WLAN_FEATURE_5G */
        default:
            OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hmac_config_lauch_cap_print:channel_freq type[%d] invalid!}", channel_freq);
            return OAL_FAIL;
    }
    if (ret < EOK) {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "hmac_config_lauch_cap_print::snprintf_s failed! case[%d]", channel_freq);
        return OAL_FAIL;
    }
    oam_print(ac_tmp_buff);
    return OAL_SUCC;
}


oal_uint32  hmac_config_get_lauch_cap(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, const oal_uint8 *puc_param)
{
    oal_uint8                uc_chan_num;
    oal_uint8                uc_chan_num_2gup_idx                = 0;
    oal_uint8                uc_chan_num_2gdown_idx              = 0;
#ifdef _PRE_WLAN_FEATURE_5G
    oal_uint8                uc_chan_num_5gup_idx                = 0;
    oal_uint8                uc_chan_num_5gdown_idx              = 0;
#endif /* _PRE_WLAN_FEATURE_5G */

    oal_uint8                uc_chan_idx;
    oal_uint32               ul_ret;
    mac_regclass_info_stru  *pst_regclass_info                   = OAL_PTR_NULL;

    /* ��μ�� */
    if (pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL) {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_get_lauch_cap::null param,pst_mac_vap=%d puc_param=%d.}",
            pst_mac_vap, puc_param);
        return OAL_FAIL;
    }

    /* ����2.4G�ŵ��ߴ� */
    for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_2_BUTT / 2; uc_chan_idx++) {
        if ((mac_is_channel_idx_valid(MAC_RC_START_FREQ_2, uc_chan_idx)) == OAL_SUCC) {
            uc_chan_num_2gup_idx = uc_chan_idx;
            break;
        }
    }
    for (uc_chan_idx = MAC_CHANNEL_FREQ_2_BUTT / 2; uc_chan_idx < MAC_CHANNEL_FREQ_2_BUTT; uc_chan_idx++) {
        if ((mac_is_channel_idx_valid(MAC_RC_START_FREQ_2, uc_chan_idx)) == OAL_SUCC) {
            uc_chan_num_2gdown_idx = uc_chan_idx;
        }
    }

#ifdef _PRE_WLAN_FEATURE_5G
    /* ����5G�ŵ��ߴ� */
    for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_5_BUTT / 2; uc_chan_idx++) {
        if ((mac_is_channel_idx_valid(MAC_RC_START_FREQ_5, uc_chan_idx)) == OAL_SUCC) {
            uc_chan_num_5gup_idx = uc_chan_idx;
            break;
        }
    }
    for (uc_chan_idx = MAC_CHANNEL_FREQ_5_BUTT / 2; uc_chan_idx < MAC_CHANNEL_FREQ_5_BUTT; uc_chan_idx++) {
        if ((mac_is_channel_idx_valid(MAC_RC_START_FREQ_5, uc_chan_idx)) == OAL_SUCC) {
            uc_chan_num_5gdown_idx = uc_chan_idx;
        }
    }
#endif /* _PRE_WLAN_FEATURE_5G */

    /* ���2.4G�ŵ���֧�ֵ����߿ڷ��书���Լ��ߴ������߿ڷ��书�� */
    for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_2_BUTT; uc_chan_idx++) {
        if ((mac_is_channel_idx_valid(MAC_RC_START_FREQ_2, uc_chan_idx)) == OAL_SUCC) {
            mac_get_channel_num_from_idx(MAC_RC_START_FREQ_2, uc_chan_idx, &uc_chan_num);
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_lauch_cap::24g channel=%d}\n",
                uc_chan_num);
            pst_regclass_info = mac_get_channel_num_rc_info(MAC_RC_START_FREQ_2, uc_chan_num);
            ul_ret = hmac_config_lauch_cap_show(pst_regclass_info, MAC_RC_START_FREQ_2,
                ((uc_chan_idx == uc_chan_num_2gup_idx) || (uc_chan_idx == uc_chan_num_2gdown_idx)) ?
                OAL_TRUE : OAL_FALSE);
            if (ul_ret != OAL_SUCC) {
                return OAL_FAIL;
            }
        }
    }
#ifdef _PRE_WLAN_FEATURE_5G
    /* ���5G�ŵ���֧�ֵ����߿ڷ��书���Լ��ߴ������߿ڷ��书��  */
    for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_5_BUTT; uc_chan_idx++) {
        if ((mac_is_channel_idx_valid(MAC_RC_START_FREQ_5, uc_chan_idx)) == OAL_SUCC) {
            mac_get_channel_num_from_idx(MAC_RC_START_FREQ_5, uc_chan_idx, &uc_chan_num);
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_lauch_cap::5g channel=%d}\n",
                uc_chan_num);
            pst_regclass_info = mac_get_channel_num_rc_info(MAC_RC_START_FREQ_5, uc_chan_num);
            ul_ret = hmac_config_lauch_cap_show(pst_regclass_info, MAC_RC_START_FREQ_5,
                ((uc_chan_idx == uc_chan_num_5gup_idx) || (uc_chan_idx == uc_chan_num_5gdown_idx)) ?
                OAL_TRUE : OAL_FALSE);
            if (ul_ret != OAL_SUCC) {
                return OAL_FAIL;
            }
        }
    }
#endif /* _PRE_WLAN_FEATURE_5G */

    return OAL_SUCC;
}
#endif


oal_uint32  hmac_config_set_linkloss_threshold(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32    ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_LINKLOSS_THRESHOLD, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_linkloss_threshold::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

oal_uint32  hmac_config_set_all_log_level(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32      ul_ret = 0;
    oal_uint8       uc_vap_idx;
    oal_uint8       uc_level;

    if (pst_mac_vap == OAL_PTR_NULL || puc_param == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_set_all_log_level:: pointer is null");
        return OAL_ERR_CODE_PTR_NULL;
    }
    uc_level = (oal_uint8)(*puc_param);

    for (uc_vap_idx = 0; uc_vap_idx < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; uc_vap_idx++) {
        ul_ret += oam_log_set_vap_level(uc_vap_idx, uc_level);
        if (ul_ret != OAL_SUCC) {
            return ul_ret;
        }
    }

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_ALL_LOG_LEVEL, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_all_log_level::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

oal_uint32  hmac_config_set_d2h_hcc_assemble_cnt(mac_vap_stru *pst_mac_vap, oal_uint16 us_len,
    const oal_uint8 *puc_param)
{
    oal_uint32      ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_D2H_HCC_ASSEMBLE_CNT, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_d2h_hcc_assemble_cnt::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_chn_est_ctrl(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32      ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_CHN_EST_CTRL, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_chn_est_ctrl::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

oal_uint32  hmac_config_set_power_ref(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32      ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_POWER_REF, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_power_ref::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

oal_uint32  hmac_config_set_pm_cfg_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32      ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_PM_CFG_PARAM, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_pm_cfg_param::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

oal_uint32  hmac_config_set_cus_rf(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32      ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_CUS_RF, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_cus_rf::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

oal_uint32  hmac_config_set_cus_dts_cali(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32      ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_CUS_DTS_CALI, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_cus_dts_cali::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

oal_uint32  hmac_config_set_cus_nvram_params(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32      ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_CUS_NVRAM_PARAM, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_cus_nvram_params::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

oal_uint32  hmac_config_dev_customize_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint32      ul_ret;

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SHOW_DEV_CUSTOMIZE_INFOS, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_dev_customize_info::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

#ifdef _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN

oal_uint32  hmac_config_set_tx_classify_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_uint8        uc_flag;
    hmac_vap_stru   *pst_hmac_vap    = OAL_PTR_NULL;

    /* ��ȡhmac_vap */
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_config_set_tx_classify_switch::mac_res_get_hmac_vap fail.vap_id[%u]}",
            pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ���ò��� */
    uc_flag = *puc_param;

    /* ����û�и��ģ�����Ҫ�������� */
    if (uc_flag == pst_hmac_vap->uc_tx_traffic_classify_flag) {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
            "hmac_config_set_tx_classify_switch::change nothing to flag:%d", pst_hmac_vap->uc_tx_traffic_classify_flag);
        return OAL_SUCC;
    }

    /* ���ò������� */
    pst_hmac_vap->uc_tx_traffic_classify_flag = uc_flag;

    if (pst_hmac_vap->uc_tx_traffic_classify_flag == OAL_SWITCH_OFF) {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
            "hmac_config_set_tx_classify_switch::flag = OAL_SWITCH_OFF(0)");
        return OAL_SUCC;
    } else if (pst_hmac_vap->uc_tx_traffic_classify_flag == OAL_SWITCH_ON) {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
            "hmac_config_set_tx_classify_switch::flag = OAL_SWITCH_ON(1)");
        return OAL_SUCC;
    }

    return OAL_FAIL;
}
#endif  /* _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN */


oal_uint32 hmac_config_vendor_cmd_get_channel_list(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    oal_uint8  uc_chan_idx;
    oal_uint8  uc_chan_num;
    oal_uint8  uc_chan_number;
    oal_uint8 *puc_channel_list = OAL_PTR_NULL;
    mac_vendor_cmd_channel_list_stru *pst_channel_list = OAL_PTR_NULL;
    oal_uint32 ul_ret;

    if (pus_len == OAL_PTR_NULL || puc_param == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_config_vendor_cmd_get_channel_list::len or param is NULL.");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_channel_list = (mac_vendor_cmd_channel_list_stru *)puc_param;
    *pus_len = OAL_SIZEOF(mac_vendor_cmd_channel_list_stru);

    /* ��ȡ2G �ŵ��б� */
    uc_chan_num = 0;
    puc_channel_list   = pst_channel_list->auc_channel_list_2g;

    for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_2_BUTT; uc_chan_idx++) {
        ul_ret = mac_is_channel_idx_valid(MAC_RC_START_FREQ_2, uc_chan_idx);
        if (ul_ret == OAL_SUCC) {
            mac_get_channel_num_from_idx(MAC_RC_START_FREQ_2, uc_chan_idx, &uc_chan_number);
            puc_channel_list[uc_chan_num++] = uc_chan_number;
        }
    }
    pst_channel_list->uc_channel_num_2g = uc_chan_num;

#ifdef _PRE_WLAN_FEATURE_5G
    /* ��鶨�ƻ�5g�����Ƿ�ʹ�� */
    if (band_5g_enabled == OAL_FALSE) {
        pst_channel_list->uc_channel_num_5g = 0;
        return OAL_SUCC;
    }

    /* ��ȡ5G �ŵ��б� */
    uc_chan_num = 0;
    puc_channel_list   = pst_channel_list->auc_channel_list_5g;

    for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_5_BUTT; uc_chan_idx++) {
        ul_ret = mac_is_channel_idx_valid(MAC_RC_START_FREQ_5, uc_chan_idx);
        if (ul_ret == OAL_SUCC) {
            mac_get_channel_num_from_idx(MAC_RC_START_FREQ_5, uc_chan_idx, &uc_chan_number);
            puc_channel_list[uc_chan_num++] = uc_chan_number;
        }
    }
    pst_channel_list->uc_channel_num_5g = uc_chan_num;
#endif

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_IP_FILTER
/* rx ip���ݰ����˹��ܵ���ز������ýӿ� */
uint32_t hmac_config_update_ip_filter(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param)
{
    uint32_t           ret;
    dmac_tx_event_stru *tx_event = OAL_PTR_NULL;
    frw_event_mem_stru *event_mem = OAL_PTR_NULL;
    oal_netbuf_stru    *netbuf = OAL_PTR_NULL;
    frw_event_stru     *hmac_to_dmac_ctx_event = OAL_PTR_NULL;

    if (OAL_ANY_NULL_PTR2(mac_vap, param)) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_update_ip_filter::null param");
        return OAL_ERR_CODE_PTR_NULL;
    }
    netbuf = *((oal_netbuf_stru **)param);
    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
    ***************************************************************************/
    event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_tx_event_stru));
    if (event_mem == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_update_ip_filter::pst_event_mem null.}");
        oal_netbuf_free(netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }
    hmac_to_dmac_ctx_event = (frw_event_stru *)event_mem->puc_data;
    FRW_EVENT_HDR_INIT(&(hmac_to_dmac_ctx_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_TX_HOST_DATA_FRAME_FILTER,
                       OAL_SIZEOF(dmac_tx_event_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       mac_vap->uc_chip_id,
                       mac_vap->uc_device_id,
                       mac_vap->uc_vap_id);
    tx_event = (dmac_tx_event_stru *)(hmac_to_dmac_ctx_event->auc_event_data);
    tx_event->pst_netbuf = netbuf;
    tx_event->us_frame_len = OAL_NETBUF_LEN(netbuf);
    ret = frw_event_dispatch_event(event_mem);
    if (ret != OAL_SUCC) {
        OAM_ERROR_LOG1(mac_vap->uc_vap_id, OAM_SF_CFG,
                       "{hmac_config_update_ip_filter::frw_event_dispatch_event failed[%d].}", ret);
    }
    oal_netbuf_free(netbuf);
    FRW_EVENT_FREE(event_mem);
    return ret;
}

uint32_t hmac_config_assigned_filter(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param)
{
    uint32_t ret;

    if (OAL_UNLIKELY(mac_vap == NULL || param == NULL)) {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_assigned_filter::null param,pst_mac_vap=%x puc_param=%x.}",
                       (uintptr_t)mac_vap, (uintptr_t)param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ���¼���DMAC��, ͬ��DMAC���� */
    ret = hmac_config_send_sub_event(mac_vap, len, param);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_assigned_filter:h2d fail[%d].", ret);
        return ret;
    }

    return OAL_SUCC;
}
#endif // _PRE_WLAN_FEATURE_IP_FILTER

#ifdef _PRE_WLAN_FEATURE_APF
/*
 * ��������  : ��ӡapf program��sdt
 * �޸���ʷ  :
 * 1.��   ��:2020��3��9��
 *   �޸�����:�����ɺ���
 */
OAL_STATIC void hmac_print_apf_program(const uint8_t* program, uint32_t program_len)
{
    uint32_t index;
    uint32_t str_len;
    int32_t  str_tmp_len;
    int8_t   *print_buff = OAL_PTR_NULL;

    if (program == OAL_PTR_NULL || program_len == 0) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_print_paf_program::puc_program is null.}");
        return;
    }

    print_buff = (int8_t *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAM_REPORT_MAX_STRING_LEN, OAL_TRUE);
    if (print_buff == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_print_apf_program::print_buff null.}");
        return;
    }
    memset_s(print_buff, OAM_REPORT_MAX_STRING_LEN, 0, OAM_REPORT_MAX_STRING_LEN);
    str_len = 0;
    str_tmp_len = snprintf_s(print_buff + str_len,
                             (OAM_REPORT_MAX_STRING_LEN - str_len) - 1,
                             ((OAM_REPORT_MAX_STRING_LEN - str_len) - 1) - 1,
                             "Id           :200\n"
                             "Program len  :%d\n",
                             program_len);
    if (str_tmp_len < 0) {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "hmac_print_apf_program:sprintf return error[%d]", str_tmp_len);
        OAL_MEM_FREE(print_buff, OAL_TRUE);
        return;
    }
    str_len += str_tmp_len;

    for (index = 0; index < program_len; index++) {
        str_tmp_len = snprintf_s(print_buff + str_len,
                                 ((OAM_REPORT_MAX_STRING_LEN - str_len) - 1),
                                 ((OAM_REPORT_MAX_STRING_LEN - str_len) - 1) - 1,
                                 "%02x",
                                 program[index]);
        if (str_tmp_len < 0) {
            break;
        }
        str_len += str_tmp_len;
    }

    print_buff[OAM_REPORT_MAX_STRING_LEN - 1] = '\0';
    oam_print(print_buff);
    OAL_MEM_FREE(print_buff, OAL_TRUE);
}

/*
 * ��������  : hmac�·�apf�¼���dmac
 * �޸���ʷ  :
 * 1.��    ��: 2020��3��9��
 *   �޸�����: �����ɺ���
 */
OAL_STATIC uint32_t hmac_dispatch_apf_event(OAL_CONST mac_vap_stru *mac_vap, oal_netbuf_stru *cmd_netbuf)
{
    frw_event_mem_stru  *event_mem              = OAL_PTR_NULL;
    frw_event_stru      *hmac_to_dmac_ctx_event = OAL_PTR_NULL;
    dmac_tx_event_stru  *tx_event               = OAL_PTR_NULL;
    uint32_t            ret;

    if (OAL_UNLIKELY(OAL_ANY_NULL_PTR2(mac_vap, cmd_netbuf))) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_dispatch_apf_event::param is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    event_mem = FRW_EVENT_ALLOC(sizeof(dmac_tx_event_stru));
    if (event_mem == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_apf_program::pst_event_mem null.}");
        oal_netbuf_free(cmd_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_to_dmac_ctx_event = (frw_event_stru *)event_mem->puc_data;
    FRW_EVENT_HDR_INIT(&(hmac_to_dmac_ctx_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_TX_HOST_DATA_FRAME_FILTER,
                       sizeof(dmac_tx_event_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       mac_vap->uc_chip_id,
                       mac_vap->uc_device_id,
                       mac_vap->uc_vap_id);

    tx_event = (dmac_tx_event_stru *)(hmac_to_dmac_ctx_event->auc_event_data);
    tx_event->pst_netbuf   = cmd_netbuf;
    tx_event->us_frame_len = OAL_NETBUF_LEN(cmd_netbuf);

    ret = frw_event_dispatch_event(event_mem);
    if (ret != OAL_SUCC) {
        OAM_ERROR_LOG1(mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_apf_program::dispatch event error[%d].}", ret);
    }

    oal_netbuf_free(cmd_netbuf);
    FRW_EVENT_FREE(event_mem);
    return ret;
}


/*
 * ��������  : ����apf�·����ϱ�program������
 * �޸���ʷ  :
 * 1.��    ��: 2020��3��9��
 *   �޸�����: �����ɺ���
 */
uint32_t hmac_config_apf_filter_cmd(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param)
{
    oal_netbuf_stru               *cmd_netbuf     = OAL_PTR_NULL;
    mac_apf_filter_cmd_stru       *apf_filter_cmd = OAL_PTR_NULL;
    int32_t                       ret;

    if (OAL_UNLIKELY(OAL_ANY_NULL_PTR2(mac_vap, param))) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_apf_filter_cmd::param is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    apf_filter_cmd = (mac_apf_filter_cmd_stru*)param;

    /* ����netbuf */
    cmd_netbuf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, sizeof(uint8_t) + apf_filter_cmd->program_len,
                                      OAL_NETBUF_PRIORITY_MID);
    if (cmd_netbuf == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_set_apf_program::netbuf alloc null}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /* ��������ṹ�嵽netbuf */
    ret = memcpy_s(OAL_NETBUF_DATA(cmd_netbuf), (sizeof(uint8_t) + apf_filter_cmd->program_len),
                   &apf_filter_cmd->en_cmd_type, sizeof(uint8_t));
    oal_netbuf_put(cmd_netbuf, sizeof(uint8_t));

    if (apf_filter_cmd->en_cmd_type == MAC_APF_SET_FILTER_CMD) {
        /* program���ݿ�����netbuf */
        ret += memcpy_s(OAL_NETBUF_DATA(cmd_netbuf) + sizeof(uint8_t), apf_filter_cmd->program_len,
                        apf_filter_cmd->program, apf_filter_cmd->program_len);
        oal_netbuf_put(cmd_netbuf, apf_filter_cmd->program_len);
        /* ��ӡ��sdt */
        hmac_print_apf_program(apf_filter_cmd->program, apf_filter_cmd->program_len);
    }

    if (ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "hmac_config_apf_filter_cmd::memcpy fail!");
        oal_netbuf_free(cmd_netbuf);
        return OAL_FAIL;
    }

    // ���¼���DMAC��, ͬ��DMAC����
    return hmac_dispatch_apf_event(mac_vap, cmd_netbuf);
}

/*
 * ��������  : apf program�ϱ��¼�����
 * �޸���ʷ  :
 * 1.��    ��: 2020��3��9��
 *   �޸�����: �����ɺ���
 */
uint32_t hmac_apf_program_report_event(frw_event_mem_stru *event_mem)
{
    frw_event_stru             *event        = OAL_PTR_NULL;
    frw_event_hdr_stru         *event_hdr    = OAL_PTR_NULL;
    dmac_apf_report_event_stru *report_event = OAL_PTR_NULL;
    oal_netbuf_stru            *netbuf       = OAL_PTR_NULL;
    uint8_t                    *program      = OAL_PTR_NULL;

    /* ��ȡ�¼�ͷ���¼��ṹ��ָ�� */
    event        = frw_get_event_stru(event_mem);
    event_hdr    = &(event->st_event_hdr);
    report_event = (dmac_apf_report_event_stru*)event->auc_event_data;

    netbuf = (oal_netbuf_stru*)report_event->program;
    if (netbuf == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "hmac_apf_program_report_event:netbuf is null");
        return OAL_FAIL;
    }
    program = OAL_NETBUF_DATA(netbuf);
    hmac_print_apf_program(OAL_NETBUF_DATA(netbuf), OAL_NETBUF_LEN(netbuf));

    oal_netbuf_free(netbuf);
    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_SAE
/* ��������  : �������ʧ�ܽ�� */
void hmac_handle_connect_failed_result(hmac_vap_stru *hmac_vap, uint16_t status)
{
    hmac_user_stru *hmac_user = OAL_PTR_NULL;

    OAM_WARNING_LOG1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,
                     "{hmac_handle_connect_failed_result::connect fail[%d]}", status);

#ifdef _PRE_WLAN_1131_CHR
        hmac_chr_connect_fail_query_and_report(hmac_vap, status);
#endif

    /* �ϱ�����ʧ�ܵ�wpa_supplicant */
    hmac_report_connect_failed_result(hmac_vap, status);

    /* ��ȡ�û�ָ�� */
    hmac_user = mac_res_get_hmac_user(hmac_vap->st_vap_base_info.uc_assoc_vap_id);
    if (hmac_user != OAL_PTR_NULL) {
        /* ����ȥ��֤֡��AP */
        hmac_mgmt_send_deauth_frame(&hmac_vap->st_vap_base_info,
                                    hmac_user->st_user_base_info.auc_user_mac_addr, status, OAL_FALSE);

        /* ɾ����Ӧ�û� */
        hmac_user_del(&hmac_vap->st_vap_base_info, hmac_user);
    } else {
        /* ����״̬ΪFAKE UP */
        hmac_fsm_change_state(hmac_vap, MAC_VAP_STATE_STA_FAKE_UP);
        OAM_WARNING_LOG1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH, "asoc:hmac_user[%d] NULL.",
                         hmac_vap->st_vap_base_info.uc_assoc_vap_id);
    }

    /* ͬ��DMAC״̬ */
    hmac_send_connect_result_to_dmac_sta(hmac_vap, OAL_FAIL);
}


OAL_STATIC uint32_t hmac_config_external_auth_param_check(mac_vap_stru *mac_vap,
    hmac_external_auth_req_stru *ext_auth)
{
    uint8_t desired_ssid_len;
    if (OAL_ANY_NULL_PTR2(mac_vap, ext_auth)) {
        OAM_ERROR_LOG0(0, OAM_SF_SAE, "{hmac_config_external_auth_param_check::null ptr");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��legacy sta��ֱ�ӷ��� */
    if (!IS_LEGACY_STA(mac_vap)) {
        OAM_WARNING_LOG2(mac_vap->uc_vap_id, OAM_SF_SAE,
                         "{hmac_config_external_auth_param_check::wrong vap. vap_mode %d, p2p_mode %d}",
                         mac_vap->en_vap_mode, mac_vap->en_p2p_mode);
        return OAL_FAIL;
    }

    /* �ǹ�����bssid��ֱ�ӷ��� */
    if (oal_memcmp(mac_vap->auc_bssid, ext_auth->bssid, WLAN_MAC_ADDR_LEN) != 0) {
        OAM_WARNING_LOG3(mac_vap->uc_vap_id, OAM_SF_SAE,
                         "{hmac_config_external_auth_param_check::wrong bssid %02X:XX:XX:XX:%02X:%02X}",
                         ext_auth->bssid[0], ext_auth->bssid[4], ext_auth->bssid[5]);
        return OAL_FAIL;
    }

    /* �ǹ�����SSID��ֱ�ӷ��� */
    desired_ssid_len = OAL_STRLEN((int8_t *)mac_mib_get_DesiredSSID(mac_vap));
    if ((ext_auth->ssid.uc_ssid_len != desired_ssid_len) ||
        oal_memcmp(mac_mib_get_DesiredSSID(mac_vap), ext_auth->ssid.auc_ssid, ext_auth->ssid.uc_ssid_len) != 0) {
        OAM_WARNING_LOG2(mac_vap->uc_vap_id, OAM_SF_SAE,
                         "{hmac_config_external_auth_param_check::DesiredSSID len [%d], ext_auth ssid len [%d]}",
                         desired_ssid_len, ext_auth->ssid.uc_ssid_len);
        return OAL_FAIL;
    }
    return OAL_SUCC;
}

/* ��������  : ִ��SAE external auth���� */
uint32_t hmac_config_external_auth(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param)
{
    uint32_t                     ret;
    hmac_external_auth_req_stru  *ext_auth = OAL_PTR_NULL;
    hmac_vap_stru                *hmac_vap = OAL_PTR_NULL;
    hmac_user_stru               *hmac_user = OAL_PTR_NULL;
    mac_cfg_kick_user_param_stru kick_user_param = {{0}};
    hmac_auth_rsp_stru           auth_rsp = {{0}};

    ext_auth = (hmac_external_auth_req_stru *)param;

    ret = hmac_config_external_auth_param_check(mac_vap, ext_auth);
    if (ret != OAL_SUCC) {
        return ret;
    }

    hmac_vap = mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    if (hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(mac_vap->uc_vap_id, OAM_SF_SAE, "{hmac_config_external_auth::hmac_vap null, vap_idx:%d}",
            mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ�û�ָ�� */
    hmac_user = mac_res_get_hmac_user(hmac_vap->st_vap_base_info.uc_assoc_vap_id);
    if (hmac_user == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SAE,
                         "hmac_config_external_auth::hmac_user[%d] NULL.",
                         hmac_vap->st_vap_base_info.uc_assoc_vap_id);
        return OAL_FAIL;
    }

    /* ext_auth ״̬Ϊʧ������£��Ͽ����� */
    if (ext_auth->status == MAC_UNSPEC_FAIL) {
        /* ����SAE�������ĳ���, ��Ҫ���ϱ�status = 1��connect�¼� */
        hmac_handle_connect_failed_result(hmac_vap, MAC_UNSPEC_FAIL);
        return OAL_SUCC;
    } else if (ext_auth->status != MAC_SUCCESSFUL_STATUSCODE) {
        kick_user_param.us_reason_code = ext_auth->status;
        memcpy_s(kick_user_param.auc_mac_addr, WLAN_MAC_ADDR_LEN, mac_vap->auc_bssid, WLAN_MAC_ADDR_LEN);
        hmac_config_kick_user(mac_vap, OAL_SIZEOF(kick_user_param), (uint8_t *)(&kick_user_param));
        return OAL_SUCC;
    }
    /* ext_auth ״̬Ϊ�ɹ���ִ��SAE���� */
    /* ȡ����ʱ�� */
    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&hmac_vap->st_mgmt_timer);

    /* ��״̬����ΪAUTH_COMP */
    hmac_fsm_change_state(hmac_vap, MAC_VAP_STATE_STA_AUTH_COMP);
    auth_rsp.us_status_code = HMAC_MGMT_SUCCESS;

    /* �ϱ�SME��֤�ɹ���ִ�й��� */
    hmac_send_rsp_to_sme_sta(hmac_vap, HMAC_SME_AUTH_RSP, (uint8_t *)&auth_rsp);

    return OAL_SUCC;
}

/*
 * ��������  : �ж�wpa_s�·�����rsn ie �Ƿ����PMKID.
 *             ���AUTH_TYPE = SAE��ʽ�������ҹ�������IE��RSN IE����PMKID����ʹ��SAE������
 *             ���AUTH_TYPE = SAE��ʽ�������ҹ�������IE��RSN IE������PMKID����ʹ��SAE������
 */
OAL_STATIC oal_bool_enum_uint8 sae_connect_with_pmkid(uint8_t *rsn_ie, uint8_t rsn_ie_len)
{
     /*
      * RSN Element Format
      * ---------------------------------------------------------------------
      * |Element ID | Length | Version | Group Cipher Suite | Pairwise Suite
      * --------------------------------------------------------------------
      * |     1     |    1   |    2    |      0 or 4        |     0 or 2
      * ---------------------------------------------------------------------
      * ---------------------------------------------------------------------
      *  Count| Pairwise Cipher Suite List | AKM Suite Count | AKM Suite List
      * ---------------------------------------------------------------------
      *       |       0 or 4*m             |     0 or 2      |   0 or 4*n
      * ---------------------------------------------------------------------
      * ---------------------------------------------------------------------
      * |RSN Capabilities|PMKID Count|PMKID List|Group Management Cipher Suite
      * ---------------------------------------------------------------------
      * |      0 or 2    |   0 or 2  | 0 or 16 *s  |          0 or 4        |
      * ---------------------------------------------------------------------
      */

    /*
      * ����wpa_s�·�������rsn_ie�����鲥�����׼������������׼�����Կ�����׼���RSN capability��
      * ���Բ��ÿ���û�������׼��ĳ���
      */
    struct hmac_rsn_connect_pmkid_stru {
        uint8_t  eid;
        uint8_t  ie_len;
        uint16_t rsn_ver;
        uint32_t group_cipher;
        uint16_t pairwise_cipher_cnt;
        uint32_t pairwise_cipher;
        uint16_t akm_cnt;
        uint32_t akm;
        uint16_t rsn_cap;
        uint16_t pmkid_cnt;
        uint8_t  apmkid[WLAN_PMKID_LEN];
    };

    /* RSN���ȹ��̣�������PMKID */
    if (rsn_ie_len < OAL_SIZEOF(struct hmac_rsn_connect_pmkid_stru)) {
        return OAL_FALSE;
    }

    return OAL_TRUE;
}

/*
 * ��������  : ����ʱ���ж��Ƿ�ΪSAE����
 *             ���AUTH_TYPE = SAE��ʽ�������ҹ�������IE��RSN IE����PMKID����ʹ��SAE������
 *             ���AUTH_TYPE = SAE��ʽ�������ҹ�������IE��RSN IE������PMKID����ʹ��SAE������
 */
OAL_STATIC void hmac_update_sae_connect_param(hmac_vap_stru *hmac_vap, uint8_t *ie_data, uint32_t ie_len)
{
    uint8_t *rsn_ie = OAL_PTR_NULL;

    hmac_vap->bit_sae_connect_with_pmkid = OAL_FALSE;

    if (hmac_vap->en_auth_mode != WLAN_WITP_AUTH_SAE) {
        return;
    }

    /* wpa_s�·�SAE������ȻЯ��RSN IE */
    rsn_ie = mac_find_ie(MAC_EID_RSN, ie_data, ie_len);
    if (rsn_ie == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SAE,
                       "hmac_update_sae_connect_param:SAE connect without RSN ie", ie_len);
        return;
    }

    /*
     * ��RSN IE�в���Я��PMKID���ݣ�������sae_connect_with_pmkid = true;
     * δЯ��PMKID���ݣ�����sae_connect_with_pmkid = false
     */
    hmac_vap->bit_sae_connect_with_pmkid = sae_connect_with_pmkid(rsn_ie, rsn_ie[1]);

    OAM_WARNING_LOG1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SAE,
                     "hmac_update_sae_connect_param::use sae connect. with PMKID [%d]",
                     hmac_vap->bit_sae_connect_with_pmkid);
}
#endif /* _PRE_WLAN_FEATURE_SAE */

uint32_t hmac_config_set_auth_rsp_time(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param)
{
    if (OAL_ANY_NULL_PTR2(mac_vap, param)) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_set_auth_rsp_time::param is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    g_mac_device_custom_cfg.cmd_auth_rsp_timeout = *(uint16_t *)param;
    OAM_WARNING_LOG1(0, OAM_SF_CFG, "hmac_config_set_auth_rsp_time::auth_rsp_timeout[%d]",
                     g_mac_device_custom_cfg.cmd_auth_rsp_timeout);
    return OAL_SUCC;
}
uint32_t hmac_config_forbit_open_auth(mac_vap_stru *mac_vap, uint16_t len, const uint8_t *param)
{
    if (OAL_ANY_NULL_PTR2(mac_vap, param)) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_forbit_open_auth::param is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    g_mac_device_custom_cfg.forbit_open_auth = ((*param == 1) ? 1 : 0);
    OAM_WARNING_LOG1(0, OAM_SF_CFG, "hmac_config_forbit_open_auth::forbit_open_auth[%d]",
                     g_mac_device_custom_cfg.forbit_open_auth);
    return OAL_SUCC;
}

/*lint -e578*//*lint -e19*/
#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
oal_module_symbol(hmac_config_set_blacklist_mode);
oal_module_symbol(hmac_config_blacklist_add);
oal_module_symbol(hmac_config_blacklist_add_only);
oal_module_symbol(hmac_config_blacklist_del);
oal_module_symbol(hmac_config_show_blacklist);
oal_module_symbol(hmac_config_show_isolation);
oal_module_symbol(hmac_config_autoblacklist_enable);
oal_module_symbol(hmac_config_set_autoblacklist_aging);
oal_module_symbol(hmac_config_set_autoblacklist_threshold);
oal_module_symbol(hmac_config_set_autoblacklist_reset_time);
oal_module_symbol(hmac_config_set_isolation_mode);
oal_module_symbol(hmac_config_set_isolation_type);
oal_module_symbol(hmac_config_set_isolation_forword);
oal_module_symbol(hmac_config_set_isolation_clear);
#endif /* _PRE_WLAN_FEATURE_CUSTOM_SECURITY */
oal_module_symbol(hmac_config_get_version);
oal_module_symbol(hmac_config_send_2040_coext);
oal_module_symbol(hmac_config_2040_coext_info);
oal_module_symbol(hmac_config_get_mib);
oal_module_symbol(hmac_config_set_mib);
oal_module_symbol(hmac_config_set_auto_protection);
oal_module_symbol(hmac_config_rssi_switch);
oal_module_symbol(hmac_config_set_random_mac_addr_scan);
oal_module_symbol(hmac_config_start_vap);
oal_module_symbol(hmac_config_add_vap);
oal_module_symbol(hmac_config_del_vap);
oal_module_symbol(hmac_config_down_vap);
oal_module_symbol(hmac_config_set_mac_addr);
oal_module_symbol(hmac_config_set_mode);
oal_module_symbol(hmac_config_get_mode);
oal_module_symbol(hmac_config_set_concurrent);
oal_module_symbol(hmac_config_get_concurrent);
oal_module_symbol(hmac_config_set_bss_type);
oal_module_symbol(hmac_config_get_bss_type);
oal_module_symbol(hmac_config_set_ssid);
oal_module_symbol(hmac_config_get_ssid);
oal_module_symbol(hmac_config_set_shpreamble);
oal_module_symbol(hmac_config_get_shpreamble);
oal_module_symbol(hmac_config_set_shortgi20);
oal_module_symbol(hmac_config_set_shortgi40);
oal_module_symbol(hmac_config_set_shortgi80);
oal_module_symbol(hmac_config_get_shortgi20);
oal_module_symbol(hmac_config_get_shortgi40);
oal_module_symbol(hmac_config_get_shortgi80);
#ifdef _PRE_WLAN_FEATURE_MONITOR
oal_module_symbol(hmac_config_set_addr_filter);
#endif
oal_module_symbol(hmac_config_get_addr_filter);
oal_module_symbol(hmac_config_set_prot_mode);
oal_module_symbol(hmac_config_get_prot_mode);
oal_module_symbol(hmac_config_set_auth_mode);
oal_module_symbol(hmac_config_get_auth_mode);
oal_module_symbol(hmac_config_set_max_user);
oal_module_symbol(hmac_config_set_bintval);
oal_module_symbol(hmac_config_get_bintval);
oal_module_symbol(hmac_config_set_nobeacon);
oal_module_symbol(hmac_config_get_nobeacon);
oal_module_symbol(hmac_config_set_txchain);
oal_module_symbol(hmac_config_get_txchain);
oal_module_symbol(hmac_config_set_rxchain);
oal_module_symbol(hmac_config_get_rxchain);
oal_module_symbol(hmac_config_set_txpower);
oal_module_symbol(hmac_config_get_txpower);
oal_module_symbol(hmac_config_set_freq);
oal_module_symbol(hmac_config_get_freq);
oal_module_symbol(hmac_config_get_wmm_params);
oal_module_symbol(hmac_config_set_wmm_params);
oal_module_symbol(hmac_config_vap_info);
oal_module_symbol(hmac_config_user_info);
oal_module_symbol(hmac_config_add_user);
oal_module_symbol(hmac_config_del_user);
oal_module_symbol(hmac_config_addba_req);
oal_module_symbol(hmac_config_set_dscr_param);
oal_module_symbol(hmac_config_set_rate);
oal_module_symbol(hmac_config_log_level);
oal_module_symbol(hmac_config_set_mcs);
oal_module_symbol(hmac_config_set_mcsac);
oal_module_symbol(hmac_config_set_nss);
oal_module_symbol(hmac_config_set_rfch);
oal_module_symbol(hmac_config_set_bw);
oal_module_symbol(hmac_config_always_tx);
oal_module_symbol(hmac_config_always_rx);
#ifdef _PRE_DEBUG_MODE
oal_module_symbol(hmac_config_dync_txpower);
#endif
oal_module_symbol(hmac_config_connect);
oal_module_symbol(hmac_config_get_thruput);
oal_module_symbol(hmac_config_set_freq_skew);
oal_module_symbol(hmac_config_adjust_ppm);
oal_module_symbol(hmac_config_delba_req);
oal_module_symbol(hmac_config_ampdu_end);
oal_module_symbol(hmac_config_ampdu_start);
oal_module_symbol(hmac_config_event_switch);
oal_module_symbol(hmac_config_profiling_switch);
oal_module_symbol(hmac_config_amsdu_start);
oal_module_symbol(hmac_config_auto_ba_switch);
oal_module_symbol(hmac_config_list_sta);
oal_module_symbol(hmac_config_get_sta_list);
oal_module_symbol(hmac_config_list_ap);
oal_module_symbol(hmac_config_send_bar);
oal_module_symbol(hmac_config_pause_tid);
oal_module_symbol(hmac_config_dump_timer);
oal_module_symbol(hmac_config_set_user_vip);
oal_module_symbol(hmac_config_set_vap_host);
oal_module_symbol(hmac_config_set_dtimperiod);
oal_module_symbol(hmac_config_get_dtimperiod);
oal_module_symbol(hmac_config_alg_param);
oal_module_symbol(hmac_config_hide_ssid);
oal_module_symbol(hmac_config_set_amsdu_tx_on);
oal_module_symbol(hmac_config_set_ampdu_tx_on);
oal_module_symbol(hmac_config_get_country);
oal_module_symbol(hmac_config_set_country);
#ifdef _PRE_WLAN_FEATURE_DFS    // 1131_debug
oal_module_symbol(hmac_config_set_country_for_dfs);
#endif
oal_module_symbol(hmac_config_amsdu_ampdu_switch);
oal_module_symbol(hmac_config_reset_hw);
oal_module_symbol(hmac_config_dump_rx_dscr);
oal_module_symbol(hmac_config_dump_tx_dscr);
oal_module_symbol(hmac_config_set_channel);
oal_module_symbol(hmac_config_set_beacon);
oal_module_symbol(hmac_config_set_app_ie_to_vap);
oal_module_symbol(hmac_config_set_wps_p2p_ie);
oal_module_symbol(hmac_config_set_wps_ie);
oal_module_symbol(hmac_config_list_channel);
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1131C_HOST)
oal_module_symbol(hmac_config_get_lauch_cap);
#endif
#endif
oal_module_symbol(hmac_config_set_regdomain_pwr);
oal_module_symbol(hmac_config_reduce_sar);
oal_module_symbol(hmac_config_reg_write);
#ifdef _PRE_WLAN_FEATURE_11D
oal_module_symbol(hmac_config_set_rd_by_ie_switch);
#endif
#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST))
oal_module_symbol(hmac_config_sdio_flowctrl);
#endif
oal_module_symbol(hmac_config_mips_cycle_switch);
oal_module_symbol(hmac_config_reg_info);
oal_module_symbol(hmac_config_dbb_scaling_amend);
oal_module_symbol(hmac_config_dump_all_rx_dscr);
oal_module_symbol(hmac_config_alg);
oal_module_symbol(hmac_config_send_event);
oal_module_symbol(hmac_config_sync_cmd_common);
oal_module_symbol(hmac_config_beacon_chain_switch);
oal_module_symbol(hmac_config_2040_channel_switch_prohibited);
oal_module_symbol(hmac_config_set_FortyMHzIntolerant);
oal_module_symbol(hmac_config_set_2040_coext_support);
oal_module_symbol(hmac_config_rx_fcs_info);
oal_module_symbol(hmac_config_get_tid);
oal_module_symbol(hmac_config_dump_ba_bitmap);
oal_module_symbol(hmac_config_eth_switch);
oal_module_symbol(hmac_config_80211_ucast_switch);

oal_module_symbol(hmac_config_80211_mcast_switch);
oal_module_symbol(hmac_config_probe_switch);
oal_module_symbol(hmac_config_get_mpdu_num);
oal_module_symbol(hmac_config_set_thruput_bypass);
#ifdef _PRE_WLAN_CHIP_TEST
#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
oal_module_symbol(hmac_enable_pmf);
#endif
oal_module_symbol(hmac_test_send_action);
oal_module_symbol(hmac_config_send_pspoll);
oal_module_symbol(hmac_config_send_nulldata);
oal_module_symbol(hmac_config_beacon_offload_test);
#endif
oal_module_symbol(hmac_config_ota_beacon_switch);
oal_module_symbol(hmac_config_ota_rx_dscr_switch);
oal_module_symbol(hmac_config_set_all_ota);
oal_module_symbol(hmac_config_oam_output);
oal_module_symbol(hmac_config_set_dhcp_arp_switch);
oal_module_symbol(hmac_config_vap_pkt_stat);
#ifdef _PRE_WLAN_RF_110X_CALI_DPD
oal_module_symbol(hmac_config_start_dpd);
#endif

#ifdef _PRE_DEBUG_MODE_USER_TRACK
oal_module_symbol(hmac_config_report_thrput_stat);
#endif

#ifdef _PRE_WLAN_FEATURE_DAQ
oal_module_symbol(hmac_config_data_acq);
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
oal_module_symbol(hmac_config_resume_rx_intr_fifo);
#endif

#ifdef _PRE_WLAN_FEATURE_SMPS
#ifdef _PRE_DEBUG_MODE
oal_module_symbol(hmac_config_get_smps_info);
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_SMPS
oal_module_symbol(hmac_config_get_smps_mode);
oal_module_symbol(hmac_config_set_smps_mode);
oal_module_symbol(hmac_config_get_smps_en);
#endif

#ifdef _PRE_WLAN_CHIP_TEST
oal_module_symbol(hmac_config_lpm_tx_data);
oal_module_symbol(hmac_config_set_coex);
oal_module_symbol(hmac_config_set_dfx);
oal_module_symbol(hmac_config_clear_all_stat);
#endif

#ifdef _PRE_WLAN_PERFORM_STAT
oal_module_symbol(hmac_config_pfm_stat);
oal_module_symbol(hmac_config_pfm_display);
#endif

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
oal_module_symbol(hmac_config_proxysta_switch);
oal_module_symbol(hmac_config_set_oma);
#endif

#ifdef _PRE_WLAN_FEATURE_DFS
oal_module_symbol(hmac_config_dfs_radartool);
#endif   /* end of _PRE_WLAN_FEATURE_DFS */

#ifdef _PRE_SUPPORT_ACS
oal_module_symbol(hmac_config_acs);
#endif


#ifdef _PRE_WLAN_FEATURE_UAPSD
oal_module_symbol(hmac_config_get_uapsden);
oal_module_symbol(hmac_config_set_uapsden);
#endif
#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
oal_module_symbol(hmac_config_set_opmode_notify);
oal_module_symbol(hmac_config_get_user_rssbw);

#endif

#ifdef _PRE_WLAN_FEATURE_TXOPPS
oal_module_symbol(hmac_config_set_txop_ps_machw);
#endif
#ifdef _PRE_WLAN_FEATURE_BTCOEX
oal_module_symbol(hmac_config_print_btcoex_status);
#endif
#ifdef _PRE_WLAN_FEATURE_LTECOEX
oal_module_symbol(hmac_config_ltecoex_mode_set);
#endif
oal_module_symbol(hmac_config_set_vap_nss);


#ifdef _PRE_DEBUG_MODE
oal_module_symbol(hmac_config_rx_filter_val);
oal_module_symbol(hmac_config_set_rx_filter_en);
oal_module_symbol(hmac_config_get_rx_filter_en);
#endif


#ifdef _PRE_DEBUG_MODE
oal_module_symbol(hmac_config_get_all_reg_value);
oal_module_symbol(hmac_config_report_ampdu_stat);
oal_module_symbol(hmac_config_scan_test);

#endif
oal_module_symbol(hmac_config_bgscan_enable);
oal_module_symbol(hmac_config_set_ampdu_aggr_num);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
oal_module_symbol(hmac_config_set_ampdu_mmss);
#endif
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
oal_module_symbol(hmac_config_freq_adjust);
#endif
oal_module_symbol(hmac_config_set_stbc_cap);
oal_module_symbol(hmac_config_set_ldpc_cap);

#ifdef _PRE_WLAN_DFT_STAT
oal_module_symbol(hmac_config_set_phy_stat_en);
oal_module_symbol(hmac_config_dbb_env_param);
oal_module_symbol(hmac_config_usr_queue_stat);
oal_module_symbol(hmac_config_report_vap_stat);
oal_module_symbol(hmac_config_report_all_stat);
#endif

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
oal_module_symbol(hmac_config_set_edca_opt_cycle_ap);
oal_module_symbol(hmac_config_set_edca_opt_switch_ap);
oal_module_symbol(hmac_config_set_edca_opt_weight_sta);
oal_module_symbol(hmac_config_set_edca_opt_switch_sta);
#endif

#ifdef _PRE_WLAN_FEATURE_PM
oal_module_symbol(hmac_config_wifi_enable);
#endif
oal_module_symbol(hmac_config_remain_on_channel);
oal_module_symbol(hmac_config_cancel_remain_on_channel);

oal_module_symbol(hmac_config_vap_classify_en);
oal_module_symbol(hmac_config_vap_classify_tid);
oal_module_symbol(hmac_config_always_tx_1102);
oal_module_symbol(hmac_config_scan_abort);

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && defined(_PRE_WLAN_CHIP_TEST_ALG)
oal_module_symbol(hmac_alg_test_result_process);
oal_module_symbol(hmac_alg_test_main_common_init);
oal_module_symbol(hmac_alg_test_main_common_exit);
#endif

#ifdef _PRE_WLAN_FEATURE_HS20
oal_module_symbol(hmac_config_set_qos_map);
#endif

#ifdef _PRE_WLAN_FEATURE_P2P
oal_module_symbol(hmac_config_set_p2p_ps_ops);
oal_module_symbol(hmac_config_set_p2p_ps_noa);
oal_module_symbol(hmac_config_set_p2p_ps_stat);
#endif

#ifdef _PRE_WLAN_PROFLING_MIPS
oal_module_symbol(hmac_config_set_mips);
oal_module_symbol(hmac_config_show_mips);
#endif


#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
oal_module_symbol(hmac_config_enable_arp_offload);
oal_module_symbol(hmac_config_set_ip_addr);
oal_module_symbol(hmac_config_show_arpoffload_info);
#endif
oal_module_symbol(hmac_config_get_fem_pa_status);
#ifdef _PRE_WLAN_FEATURE_ROAM
oal_module_symbol(hmac_config_roam_enable);
oal_module_symbol(hmac_config_roam_start);
oal_module_symbol(hmac_config_roam_band);
oal_module_symbol(hmac_config_roam_org);
oal_module_symbol(hmac_config_roam_info);
#endif // _PRE_WLAN_FEATURE_ROAM

#ifdef _PRE_WLAN_FEATURE_STA_PM
oal_module_symbol(hmac_config_set_pm_by_module);
#endif // _PRE_WLAN_FEATURE_STA_PM

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
oal_module_symbol(hmac_config_enable_2040bss);
#endif

#ifdef _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN
oal_module_symbol(hmac_config_set_tx_classify_switch);
#endif  /* _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN */

/*lint +e578*//*lint +e19*/
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
