


// 1 头文件包含
#include "oam_ext_if.h"
#include "dmac_ext_if.h"
#include "hmac_user.h"
#include "hmac_main.h"
#include "hmac_tx_amsdu.h"
#include "hmac_protection.h"
#include "hmac_smps.h"
#include "hmac_ext_if.h"
#include "hmac_config.h"
#include "hmac_mgmt_ap.h"
#include "hmac_chan_mgmt.h"
#include "hmac_rx_filter.h"
#include "securec.h"
#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
#include "hmac_proxy_arp.h"
#endif

#ifdef _PRE_WLAN_FEATURE_WAPI
#include "hmac_wapi.h"
#endif

#ifdef _PRE_WLAN_FEATURE_MCAST
#include "hmac_m2u.h"
#endif

#ifdef _PRE_WLAN_FEATURE_ROAM
#include "hmac_roam_main.h"
#endif // _PRE_WLAN_FEATURE_ROAM

#include "hmac_blockack.h"

#ifdef _PRE_WLAN_FEATURE_1131K_BTCOEX
#include "hmac_scan.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_USER_C

// 2 函数实现

hmac_user_stru* mac_res_get_hmac_user_alloc(oal_uint16 us_idx)
{
    hmac_user_stru *pst_hmac_user;

    pst_hmac_user = (hmac_user_stru*)_mac_res_get_hmac_user(us_idx);
    if (pst_hmac_user == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(0, OAM_SF_UM, "{mac_res_get_hmac_user_init::pst_hmac_user null,user_idx=%d.}", us_idx);
        return OAL_PTR_NULL;
    }

    /* 重复申请异常,避免影响业务，暂时打印error但正常申请 */
    if (pst_hmac_user->st_user_base_info.uc_is_user_alloced == MAC_USER_ALLOCED) {
        OAM_WARNING_LOG1(0, OAM_SF_UM, "{mac_res_get_hmac_user_init::[E]user has been alloced,user_idx=%d.}", us_idx);
    }

    return pst_hmac_user;
}


hmac_user_stru* mac_res_get_hmac_user(oal_uint16 us_idx)
{
    hmac_user_stru *pst_hmac_user;

    pst_hmac_user = (hmac_user_stru*)_mac_res_get_hmac_user(us_idx);
    if (pst_hmac_user == OAL_PTR_NULL) {
        return OAL_PTR_NULL;
    }

    /* 异常: 用户资源已被释放, user idx0 为组播user */
    if ((pst_hmac_user->st_user_base_info.uc_is_user_alloced != MAC_USER_ALLOCED) && (us_idx != 0)) {
        OAM_WARNING_LOG1(0, OAM_SF_UM, "{mac_res_get_hmac_user::[E]user has been freed,user_idx=%d.}", us_idx);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        oal_mem_print_funcname(oal_get_func_return_address());
#endif
    }

    return pst_hmac_user;
}



oal_uint32 hmac_user_alloc(oal_uint16 *pus_user_idx, oal_uint8 ul_is_multi_user)
{
    hmac_user_stru *pst_hmac_user = OAL_PTR_NULL;
    oal_uint32      ul_rslt;
    oal_uint16      us_user_idx_temp;

    if (OAL_UNLIKELY(pus_user_idx == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_UM, "{hmac_user_alloc::pus_user_idx null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 申请hmac user内存 */
    /*lint -e413*/
    if (ul_is_multi_user == OAL_TRUE) {
        us_user_idx_temp = 0;
        /*lint -e78*/
        ul_rslt = mac_res_alloc_hmac_user_ex(&us_user_idx_temp, OAL_OFFSET_OF(hmac_user_stru, st_user_base_info));
    } else {
        ul_rslt = mac_res_alloc_hmac_user(&us_user_idx_temp, OAL_OFFSET_OF(hmac_user_stru, st_user_base_info));
        /*lint +e78*/
    }

    if (ul_rslt != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_UM, "{hmac_user_alloc::mac_res_alloc_hmac_user failed[%d].}", ul_rslt);
        return ul_rslt;
    }
    /*lint +e413*/
    pst_hmac_user = mac_res_get_hmac_user_alloc(us_user_idx_temp);
    if (pst_hmac_user == OAL_PTR_NULL) {
        mac_res_free_mac_user(us_user_idx_temp);
        OAM_ERROR_LOG1(0, OAM_SF_UM, "{hmac_user_alloc::pst_hmac_user null,user_idx=%d.}", us_user_idx_temp);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 初始清0 */
    memset_s(pst_hmac_user, OAL_SIZEOF(hmac_user_stru), 0, OAL_SIZEOF(hmac_user_stru));
    /* 标记user资源已被alloc */
    pst_hmac_user->st_user_base_info.uc_is_user_alloced = MAC_USER_ALLOCED;

    *pus_user_idx = us_user_idx_temp;

    return OAL_SUCC;
}


oal_uint32 hmac_user_free(oal_uint16 us_idx)
{
    hmac_user_stru *pst_hmac_user;
    oal_uint32      ul_ret;

    pst_hmac_user = mac_res_get_hmac_user(us_idx);
    if (pst_hmac_user == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(0, OAM_SF_UM, "{hmac_user_free::pst_hmac_user null,user_idx=%d.}", us_idx);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 重复释放异常, 继续释放不返回 */
    if (pst_hmac_user->st_user_base_info.uc_is_user_alloced == MAC_USER_FREED) {
        OAM_WARNING_LOG1(0, OAM_SF_UM, "{hmac_user_free::[E]user has been freed,user_idx=%d.}", us_idx);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        oal_mem_print_funcname(oal_get_func_return_address());
#endif
    }

    ul_ret = mac_res_free_mac_user(us_idx);
    if (ul_ret == OAL_SUCC) {
        /* 清除alloc标志 */
        pst_hmac_user->st_user_base_info.uc_is_user_alloced = MAC_USER_FREED;
    }
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_user_free::user_idx=%d.}", us_idx);
    return ul_ret;
}


oal_uint32 hmac_user_init(hmac_user_stru *pst_hmac_user)
{
    oal_uint8        uc_tid_loop;
    hmac_ba_tx_stru *pst_tx_ba = OAL_PTR_NULL;

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
    oal_uint8        uc_ac_idx;
    oal_uint8        uc_data_idx;
#endif

    /* 初始化tid信息 */
    for (uc_tid_loop = 0; uc_tid_loop < WLAN_TID_MAX_NUM; uc_tid_loop++) {
        pst_hmac_user->ast_tid_info[uc_tid_loop].uc_tid_no      = (oal_uint8)uc_tid_loop;

        pst_hmac_user->ast_tid_info[uc_tid_loop].us_hmac_user_idx = pst_hmac_user->st_user_base_info.us_assoc_id;

        /* 初始化ba rx操作句柄 */
        pst_hmac_user->ast_tid_info[uc_tid_loop].pst_ba_rx_info = OAL_PTR_NULL;

        /* 清定时器 */
        memset_s(&pst_hmac_user->ast_tid_info[uc_tid_loop].st_ba_timer, OAL_SIZEOF(frw_timeout_stru), 0, OAL_SIZEOF(frw_timeout_stru));
        memset_s(&pst_hmac_user->ast_tid_info[uc_tid_loop].st_ba_tx_info, OAL_SIZEOF(hmac_ba_tx_stru), 0, OAL_SIZEOF(hmac_ba_tx_stru));

        /* 初始化ba tx操作句柄 */
        oal_spin_lock_init(&(pst_hmac_user->ast_tid_info[uc_tid_loop].st_ba_tx_info.ba_status_lock));
        pst_hmac_user->ast_tid_info[uc_tid_loop].st_ba_tx_info.en_ba_status     = DMAC_BA_INIT;
        pst_hmac_user->ast_tid_info[uc_tid_loop].st_ba_tx_info.uc_addba_attemps = 0;
        pst_hmac_user->ast_tid_info[uc_tid_loop].st_ba_tx_info.uc_dialog_token  = 0;
        pst_hmac_user->ast_tid_info[uc_tid_loop].st_ba_tx_info.en_ba_switch     = OAL_TRUE;
        pst_hmac_user->auc_ba_flag[uc_tid_loop] = 0;

        /* addba req超时处理函数入参填写 */
        pst_tx_ba = &pst_hmac_user->ast_tid_info[uc_tid_loop].st_ba_tx_info;
        pst_tx_ba->st_alarm_data.pst_ba = (oal_void *)pst_tx_ba;
        pst_tx_ba->st_alarm_data.uc_tid = uc_tid_loop;
        pst_tx_ba->st_alarm_data.us_mac_user_idx = pst_hmac_user->st_user_base_info.us_assoc_id;
        pst_tx_ba->st_alarm_data.uc_vap_id = pst_hmac_user->st_user_base_info.uc_vap_id;

        /* 初始化用户关联请求帧参数 */
        pst_hmac_user->puc_assoc_req_ie_buff = OAL_PTR_NULL;
        pst_hmac_user->ul_assoc_req_ie_len   = 0;
    }

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
    for (uc_ac_idx = 0; uc_ac_idx < WLAN_WME_AC_BUTT; uc_ac_idx++) {
        for (uc_data_idx = 0; uc_data_idx < WLAN_TXRX_DATA_BUTT; uc_data_idx++) {
            pst_hmac_user->aaul_txrx_data_stat[uc_ac_idx][uc_data_idx] = 0;
        }
    }
#endif

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    pst_hmac_user->ul_receive_ncw_cnt          = 0;
#endif

    pst_hmac_user->pst_defrag_netbuf = OAL_PTR_NULL;
    pst_hmac_user->en_user_bw_limit  = OAL_FALSE;
#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
    pst_hmac_user->st_sa_query_info.ul_sa_query_count      = 0;
    pst_hmac_user->st_sa_query_info.ul_sa_query_start_time = 0;
    pst_hmac_user->st_sa_query_info.us_sa_query_trans_id   = 0;
    memset_s(&pst_hmac_user->st_sa_query_info.st_sa_query_interval_timer, OAL_SIZEOF(frw_timeout_stru), 0, OAL_SIZEOF(frw_timeout_stru));
#endif
    memset_s(&pst_hmac_user->st_defrag_timer, OAL_SIZEOF(frw_timeout_stru), 0, OAL_SIZEOF(frw_timeout_stru));
    pst_hmac_user->ul_rx_pkt_drop = 0;

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
    /* 清除usr统计信息 */
    oam_stats_clear_user_stat_info(pst_hmac_user->st_user_base_info.us_assoc_id);
#endif

    return OAL_SUCC;
}


OAL_STATIC uint32_t hmac_user_sync_space_streanm(mac_user_stru *mac_user)
{
    mac_vap_stru *mac_vap = NULL;
    mac_user_nss_stru user_nss;
    uint32_t ret;

    mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(mac_user->uc_vap_id);
    if (mac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(mac_user->uc_vap_id, OAM_SF_CFG,
            "hmac_user_set_avail_num_space_stream::mac vap(idx=%d) is null!", mac_user->uc_vap_id);
        return OAL_FAIL;
    }
    user_nss.uc_avail_num_spatial_stream = mac_user->uc_avail_num_spatial_stream;
    user_nss.uc_num_spatial_stream       = mac_user->uc_num_spatial_stream;
    user_nss.us_user_idx = mac_user->us_assoc_id;

    ret = hmac_config_send_event(mac_vap, WLAN_CFGID_NSS,
                                 OAL_SIZEOF(mac_user_nss_stru),
                                 (oal_uint8 *)(&user_nss));
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(mac_user->uc_vap_id, OAM_SF_CFG,
            "{hmac_user_set_avail_num_space_stream::hmac_config_send_event failed[%d].}", ret);
    }
    return ret;
}


OAL_STATIC uint32_t hmac_user_judgement_num_space_stream(mac_user_stru *mac_user,
                                                         uint8_t *avail_num_spatial_stream)
{
    mac_user_ht_hdl_stru *mac_ht_hdl  = NULL;
    mac_vht_hdl_stru *mac_vht_hdl = NULL;

    mac_ht_hdl  = &(mac_user->st_ht_hdl);
    mac_vht_hdl = &(mac_user->st_vht_hdl);

    if (mac_vht_hdl->en_vht_capable == OAL_TRUE) {
        if (mac_vht_hdl->st_rx_max_mcs_map.us_max_mcs_4ss != 3) {
            *avail_num_spatial_stream = WLAN_FOUR_NSS;
        } else if (mac_vht_hdl->st_rx_max_mcs_map.us_max_mcs_3ss != 3) {
            *avail_num_spatial_stream = WLAN_TRIPLE_NSS;
        } else if (mac_vht_hdl->st_rx_max_mcs_map.us_max_mcs_2ss != 3) {
            *avail_num_spatial_stream = WLAN_DOUBLE_NSS;
        } else if (mac_vht_hdl->st_rx_max_mcs_map.us_max_mcs_1ss != 3) {
            *avail_num_spatial_stream = WLAN_SINGLE_NSS;
        } else {
            MAC_WARNING_LOG(0, "mac_user_set_avail_num_space_stream: get vht nss error");
            OAM_WARNING_LOG0(mac_user->uc_vap_id, OAM_SF_ANY, "{mac_user_set_avail_num_space_stream:invalid vht nss.}");
            return OAL_FAIL;
        }
    } else if (mac_ht_hdl->en_ht_capable == OAL_TRUE) {
        if (mac_ht_hdl->uc_rx_mcs_bitmask[3] > 0) {
            *avail_num_spatial_stream = WLAN_FOUR_NSS;
        } else if (mac_ht_hdl->uc_rx_mcs_bitmask[2] > 0) {
            *avail_num_spatial_stream = WLAN_TRIPLE_NSS;
        } else if (mac_ht_hdl->uc_rx_mcs_bitmask[1] > 0) {
            *avail_num_spatial_stream = WLAN_DOUBLE_NSS;
        } else if (mac_ht_hdl->uc_rx_mcs_bitmask[0] > 0) {
            *avail_num_spatial_stream = WLAN_SINGLE_NSS;
        } else {
            MAC_WARNING_LOG(0, "mac_user_set_avail_num_space_stream: get ht nss error");
            OAM_WARNING_LOG0(mac_user->uc_vap_id, OAM_SF_ANY, "{mac_user_set_avail_num_space_stream::invalid ht nss.}");
            return OAL_FAIL;
        }
    } else {
        *avail_num_spatial_stream = WLAN_SINGLE_NSS;
    }
    return OAL_SUCC;
}



oal_uint32 hmac_user_set_avail_num_space_stream(mac_user_stru *mac_user, wlan_nss_enum_uint8 en_vap_nss)
{
    uint8_t avail_num_spatial_stream = 0;
    uint32_t ret;

     /* AP(STA)为legacy设备，只支持1根天线，不需要再判断天线个数 */
    /* 获取HT和VHT结构体指针 */
    ret = hmac_user_judgement_num_space_stream(mac_user, &avail_num_spatial_stream);

    /* 赋值给用户结构体变量 */
    mac_user_set_num_spatial_stream(mac_user, avail_num_spatial_stream);
    mac_user_set_avail_num_spatial_stream(mac_user, OAL_MIN(avail_num_spatial_stream, en_vap_nss));

    ret = hmac_user_sync_space_streanm(mac_user);

    return ret;
}

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)

OAL_STATIC void hmac_stop_sa_query_timer(hmac_user_stru *hmac_user)
{
    frw_timeout_stru *sa_query_interval_timer;

    sa_query_interval_timer = &(hmac_user->st_sa_query_info.st_sa_query_interval_timer);
    if (sa_query_interval_timer->en_is_registerd != OAL_FALSE) {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(sa_query_interval_timer);
    }
}
#endif


#ifdef _PRE_WLAN_FEATURE_WAPI
hmac_wapi_stru *hmac_user_get_wapi_ptr(mac_vap_stru *pst_mac_vap, oal_bool_enum_uint8 en_pairwise,
    oal_uint16 us_pairwise_idx)
{
    hmac_user_stru *pst_hmac_user;
    oal_uint16 us_user_index;

    if (en_pairwise == OAL_TRUE) {
        us_user_index = us_pairwise_idx;
    } else {
        us_user_index = pst_mac_vap->us_multi_user_idx;
    }

    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(us_user_index);
    if (pst_hmac_user == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_user_get_wapi_ptr::pst_hmac_user null.}");
        return OAL_PTR_NULL;
    }

    return &pst_hmac_user->st_wapi;
}
#endif

OAL_STATIC mac_ap_type_enum_uint8 hmac_compability_ap_type_identify(mac_vap_stru *mac_vap, uint8_t *mac_addr)
{
    uint8_t ap_type = 0;
#ifdef _PRE_WLAN_FEATURE_1131K_BTCOEX
    mac_bss_dscr_stru *bss_dscr = NULL;
    hmac_vap_stru *hmac_vap = NULL;
#endif
    if (MAC_IS_GOLDEN_AP(mac_addr)) {
        ap_type |= MAC_AP_TYPE_GOLDENAP;
    }

#ifdef _PRE_WLAN_FEATURE_1131K_BTCOEX
    bss_dscr = (mac_bss_dscr_stru *)hmac_scan_get_scanned_bss_by_bssid(mac_vap, mac_addr);
    if (bss_dscr == NULL) {
        return ap_type;
    }
    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    if (hmac_vap == NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_COEX, "{hmac_compability_ap_type_identify::pst_hmac_vap null.}");
        return ap_type;
    }

    if (bss_dscr->chip_oui_info.btcoex_blacklist_chip_oui == OAL_TRUE) {
        if (MAC_IS_ASUS_AP(mac_addr) && (bss_dscr->chip_oui_info.chip_oui == WLAN_AP_CHIP_OUI_BCM)) {
            /* TUF1900P AP有兼容性问题，频繁删建BA，会造成AP短时间停止响应删建BA并停止收发报文，所以对于这个AP共存场景不删BA */
            ap_type |= MAC_AP_TYPE_BTCOEX_BA_NO_DELETE;
            hmac_btcoex_compability_ap_no_delba_proc(hmac_vap, mac_addr);
        }
        if (MAC_IS_JCG_AP(mac_addr)) {
            OAM_WARNING_LOG0(mac_vap->uc_vap_id, OAM_SF_COEX,
                             "hmac_compability_ap_type_identify::btcoex disable CTS blacklist");
            ap_type |= MAC_AP_TYPE_BTCOEX_DISABLE_CTS;
        }
    }
#endif
    return ap_type;
}

// 清除user下的分片缓存，防止重关联或者rekey流程报文重组攻击
void hmac_user_clear_defrag_res(hmac_user_stru *hmac_user)
{
    if (hmac_user == NULL) {
        return;
    }
    OAM_WARNING_LOG2(hmac_user->st_user_base_info.uc_vap_id, OAM_SF_ASSOC,
        "{hmac_user_clear_defrag_res::timer[%d] netbuf NULL[%d] .}",
        hmac_user->st_defrag_timer.en_is_registerd, (hmac_user->pst_defrag_netbuf == NULL));

    if (hmac_user->st_defrag_timer.en_is_registerd == OAL_TRUE) {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&hmac_user->st_defrag_timer);
    }
    if (hmac_user->pst_defrag_netbuf != NULL) {
        oal_netbuf_free(hmac_user->pst_defrag_netbuf);
        hmac_user->pst_defrag_netbuf = NULL;
    }
}


oal_uint32 hmac_user_del(mac_vap_stru *pst_mac_vap, hmac_user_stru *pst_hmac_user)
{
    oal_uint16                      us_user_index;
    frw_event_mem_stru             *pst_event_mem        = OAL_PTR_NULL;
    frw_event_stru                 *pst_event            = OAL_PTR_NULL;
    dmac_ctx_del_user_stru         *pst_del_user_payload = OAL_PTR_NULL;
    hmac_vap_stru                  *pst_hmac_vap         = OAL_PTR_NULL;
    mac_device_stru                *pst_mac_device       = OAL_PTR_NULL;
    mac_user_stru                  *pst_mac_user         = OAL_PTR_NULL;
    oal_uint32                      ul_ret;
#ifdef _PRE_WLAN_FEATURE_WAPI
    hmac_user_stru                 *pst_hmac_user_multi  = OAL_PTR_NULL;
#endif
    mac_ap_type_enum_uint8          en_ap_type = MAC_AP_TYPE_BUTT;

    if (OAL_UNLIKELY((pst_mac_vap == OAL_PTR_NULL) || (pst_hmac_user == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_UM, "{hmac_user_del::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_user = (mac_user_stru*)(&pst_hmac_user->st_user_base_info);
    if (OAL_UNLIKELY(pst_mac_user == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_UM, "{hmac_user_del::pst_mac_user param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_UM,
        "{hmac_user_del::del user[%d] start,is multi user[%d], user mac:XX:XX:XX:XX:%02X:%02X}",
        pst_mac_user->us_assoc_id, pst_mac_user->en_is_multi_user, pst_mac_user->auc_user_mac_addr[4],
        pst_mac_user->auc_user_mac_addr[5]);

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    hmac_chan_update_40M_intol_user(pst_mac_vap, &(pst_hmac_user->st_user_base_info), OAL_FALSE);
#endif
    /* 删除user时候，需要更新保护机制 */
    ul_ret = hmac_protection_del_user(pst_mac_vap, &(pst_hmac_user->st_user_base_info));
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_UM, "{hmac_user_del::hmac_protection_del_user[%d]}", ul_ret);
    }

     /* 获取用户对应的索引 */
    us_user_index = pst_hmac_user->st_user_base_info.us_assoc_id;

    /* 删除hmac user 的关联请求帧空间 */
    if (pst_hmac_user->puc_assoc_req_ie_buff != OAL_PTR_NULL) {
        OAL_MEM_FREE(pst_hmac_user->puc_assoc_req_ie_buff, OAL_TRUE);
        pst_hmac_user->puc_assoc_req_ie_buff = OAL_PTR_NULL;
        pst_hmac_user->ul_assoc_req_ie_len   = 0;
    }
#ifdef _PRE_WLAN_FEATURE_SMPS
    /* 删除用户，更新SMPS能力 */
    hmac_smps_update_status(pst_mac_vap, &(pst_hmac_user->st_user_base_info), OAL_FALSE);
    mac_user_set_sm_power_save(&pst_hmac_user->st_user_base_info, 0);
#endif

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
    hmac_stop_sa_query_timer(pst_hmac_user);
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
    hmac_proxy_remove_mac(pst_mac_vap, pst_hmac_user->st_user_base_info.auc_user_mac_addr);
#endif

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_del::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_del::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#ifdef _PRE_WLAN_FEATURE_WAPI
    hmac_wapi_deinit(&pst_hmac_user->st_wapi);

     /* STA模式下，清组播wapi加密端口 */
    pst_hmac_user_multi = (hmac_user_stru *)mac_res_get_hmac_user(pst_hmac_vap->st_vap_base_info.us_multi_user_idx);
    if (pst_hmac_user_multi == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
            "{hmac_user_del::mac_res_get_hmac_user fail! user_idx[%u]}",
            pst_hmac_vap->st_vap_base_info.us_multi_user_idx);
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_wapi_reset_port(&pst_hmac_user_multi->st_wapi);

    pst_mac_device->uc_wapi = OAL_FALSE;

#endif

#ifdef _PRE_WLAN_FEATURE_MCAST
    /* 用户去关联时清空snoop链表中的该成员 */
    if (pst_hmac_vap->pst_m2u != OAL_PTR_NULL) {
        hmac_m2u_cleanup_snoopwds_node(pst_hmac_user);
    }
#endif

    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
#ifdef _PRE_WLAN_FEATURE_STA_PM
        mac_vap_set_aid(pst_mac_vap, 0);
#endif
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
        pst_hmac_user->ul_receive_ncw_cnt          = 0;
#endif
        en_ap_type = hmac_compability_ap_type_identify(pst_mac_vap, pst_mac_user->auc_user_mac_addr);
    }

    /* 抛事件到DMAC层, 删除dmac用户 */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_ctx_del_user_stru));
    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_del::pst_event_mem null.}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    pst_del_user_payload = (dmac_ctx_del_user_stru *)pst_event->auc_event_data;
    pst_del_user_payload->us_user_idx = us_user_index;
    pst_del_user_payload->en_ap_type  = en_ap_type;
#if (_PRE_OS_VERSION_WIN32 != _PRE_OS_VERSION)
    /* 用户 mac地址和idx 需至少一份有效，供dmac侧查找待删除的用户 */
    memcpy_s(pst_del_user_payload->auc_user_mac_addr, WLAN_MAC_ADDR_LEN, pst_mac_user->auc_user_mac_addr,
        WLAN_MAC_ADDR_LEN);
#endif

    /* 填充事件头 */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr), FRW_EVENT_TYPE_WLAN_CTX, DMAC_WLAN_CTX_EVENT_SUB_TYPE_DEL_USER,
        OAL_SIZEOF(dmac_ctx_del_user_stru), FRW_EVENT_PIPELINE_STAGE_1, pst_mac_vap->uc_chip_id,
        pst_mac_vap->uc_device_id, pst_mac_vap->uc_vap_id);

    frw_event_dispatch_event(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

    hmac_tid_clear(pst_mac_vap, pst_hmac_user);

    if (pst_hmac_user->st_mgmt_timer.en_is_registerd == OAL_TRUE) {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_hmac_user->st_mgmt_timer);
    }

    // 删除user流程中清除user下的分片缓存
    hmac_user_clear_defrag_res(pst_hmac_user);

    /* 从vap中删除用户 */
    mac_vap_del_user(pst_mac_vap, us_user_index);

    /* 释放用户内存 */
    ul_ret = hmac_user_free(us_user_index);
    if (ul_ret == OAL_SUCC) {
        /* device下已关联user个数-- */
        pst_mac_device->uc_asoc_user_cnt--;
    } else {
        OAM_ERROR_LOG1(0, OAM_SF_UM, "{hmac_user_del::mac_res_free_mac_user fail[%d].}", ul_ret);
    }
    return OAL_SUCC;
}


oal_uint32 hmac_user_add(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_mac_addr, oal_uint16 *pus_user_index)
{
    oal_uint32                      ul_up_vap_num;
    oal_uint16                      us_user_max;
    hmac_vap_stru                  *pst_hmac_vap         = OAL_PTR_NULL;
    hmac_user_stru                 *pst_hmac_user        = OAL_PTR_NULL;
    oal_uint32                      ret;
    frw_event_mem_stru             *pst_event_mem        = OAL_PTR_NULL;
    frw_event_stru                 *pst_event            = OAL_PTR_NULL;
    dmac_ctx_add_user_stru         *pst_add_user_payload = OAL_PTR_NULL;
    oal_uint16                      us_user_idx;
    mac_device_stru                *pst_mac_device       = OAL_PTR_NULL;
    mac_cfg_80211_ucast_switch_stru st_80211_ucast_switch;
    mac_ap_type_enum_uint8          en_ap_type = MAC_AP_TYPE_BUTT;

    if (OAL_UNLIKELY(OAL_ANY_NULL_PTR3(pst_mac_vap, puc_mac_addr, pus_user_index))) {
        OAM_ERROR_LOG0(0, OAM_SF_UM, "{hmac_user_add::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_UM, "{hmac_user_add::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 在HMAC处作最大活跃用户数判断，已达32则返回错误 */
    if (pst_mac_device->uc_active_user_cnt >= WLAN_ACTIVE_USER_MAX_NUM) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::invalid uc_active_user_cnt[%d].}",
            pst_mac_device->uc_active_user_cnt);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    ul_up_vap_num   = hmac_calc_up_ap_num(pst_mac_device);
    us_user_max     = pst_hmac_vap->us_user_nums_max;
    if ((ul_up_vap_num >= 2) && (us_user_max == WLAN_ACTIVE_USER_MAX_NUM)) {
        /* AP STA共存的场景，设置host最大关联用户数为7个 */
        us_user_max--;
    }

    if (pst_hmac_vap->st_vap_base_info.us_user_nums >= us_user_max) {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_UM,
            "{hmac_user_add::invalid us_user_nums[%d], us_user_nums_max[%d].}",
            pst_hmac_vap->st_vap_base_info.us_user_nums, pst_hmac_vap->us_user_nums_max);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    /* 如果此用户已经创建，则返回失败 */
    ret = mac_vap_find_user_by_macaddr(pst_mac_vap, puc_mac_addr, &us_user_idx);
    if (ret == OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM,
            "{hmac_user_add::mac_vap_find_user_by_macaddr failed[%d].}", ret);
        return OAL_FAIL;
    }

    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
#ifdef _PRE_WLAN_FEATURE_P2P
        if (IS_P2P_CL(pst_mac_vap)) {
            if (pst_hmac_vap->st_vap_base_info.us_user_nums >= 2) { // 1131_debug
                OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM,
                    "{hmac_user_add::a STA can only associated with 2 ap.}");
                return OAL_FAIL;
            }
        } else
#endif
        {
            if (pst_hmac_vap->st_vap_base_info.us_user_nums >= 1) { // 1131_debug
                OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM,
                    "{hmac_user_add::a STA can only associated with one ap.}");
                return OAL_FAIL;
            }

            en_ap_type = hmac_compability_ap_type_identify(pst_mac_vap, puc_mac_addr);
        }
    }

    /* 申请hmac用户内存，并初始清0 */
    ret = hmac_user_alloc(&us_user_idx, OAL_FALSE);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::hmac_user_alloc failed[%d].}", ret);
        return ret;
    }

    /* 单播用户不能使用userid为0，需重新申请一个。将userid作为aid分配给对端，处理psm时会出错 */
    if (us_user_idx == 0) {
        hmac_user_free(us_user_idx);
        ret = hmac_user_alloc(&us_user_idx, OAL_FALSE);
        if ((ret != OAL_SUCC) || (us_user_idx == 0)) {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::hmac_user_alloc failed ret[%d] us_user_idx[%d].}", ret, us_user_idx);
            return ret;
        }
    }

    *pus_user_index = us_user_idx;  /* 出参赋值 */

    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(us_user_idx);
    if (pst_hmac_user == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 初始化mac_user_stru */
    mac_user_init(&(pst_hmac_user->st_user_base_info), us_user_idx, puc_mac_addr, pst_mac_vap->uc_chip_id,
        pst_mac_vap->uc_device_id, pst_mac_vap->uc_vap_id);

#ifdef _PRE_WLAN_FEATURE_WAPI
    /* 初始化单播wapi对象 */
    hmac_wapi_init(&pst_hmac_user->st_wapi, OAL_TRUE);
    pst_mac_device->uc_wapi = OAL_FALSE;
#endif
    /* 设置amsdu域 */
    hmac_amsdu_init_user(pst_hmac_user);

    /* 抛事件到DMAC层, 创建dmac用户 */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_ctx_add_user_stru));
    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        /* 异常处理，释放内存，device下关联用户数还没有++，这里不需要判断返回值做--操作 */
        hmac_user_free(us_user_idx);

        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::pst_event_mem null.}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    pst_add_user_payload = (dmac_ctx_add_user_stru *)pst_event->auc_event_data;
    pst_add_user_payload->us_user_idx = us_user_idx;
    pst_add_user_payload->en_ap_type  = en_ap_type;
    oal_set_mac_addr(pst_add_user_payload->auc_user_mac_addr, puc_mac_addr);

    /* 填充事件头 */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr), FRW_EVENT_TYPE_WLAN_CTX, DMAC_WLAN_CTX_EVENT_SUB_TYPE_ADD_USER,
        OAL_SIZEOF(dmac_ctx_add_user_stru), FRW_EVENT_PIPELINE_STAGE_1, pst_mac_vap->uc_chip_id,
        pst_mac_vap->uc_device_id, pst_mac_vap->uc_vap_id);

    ret = frw_event_dispatch_event(pst_event_mem);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        /* 异常处理，释放内存，device下关联用户数还没有++，这里不需要判断返回值做--操作 */
        hmac_user_free(us_user_idx);
        FRW_EVENT_FREE(pst_event_mem);

        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::frw_event_dispatch_event failed[%d].}", ret);
        return ret;
    }

    FRW_EVENT_FREE(pst_event_mem);

    /* 添加用户到MAC VAP */
    ret = mac_vap_add_assoc_user(pst_mac_vap, us_user_idx);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::mac_vap_add_assoc_user failed[%d].}", ret);

        /* 异常处理，释放内存，device下关联用户数还没有++，这里不需要判断返回值做--操作 */
        hmac_user_free(us_user_idx);
        return OAL_FAIL;
    }

    /* 初始话hmac user部分信息 */
    hmac_user_init(pst_hmac_user);

    pst_mac_device->uc_asoc_user_cnt++;

    /* 打开80211单播管理帧开关，观察关联过程，关联成功了就关闭 */
    st_80211_ucast_switch.en_frame_direction = OAM_OTA_FRAME_DIRECTION_TYPE_TX;
    st_80211_ucast_switch.en_frame_type = OAM_USER_TRACK_FRAME_TYPE_MGMT;
    st_80211_ucast_switch.en_frame_switch = OAL_SWITCH_ON;
    st_80211_ucast_switch.en_cb_switch = OAL_SWITCH_ON;
    st_80211_ucast_switch.en_dscr_switch = OAL_SWITCH_ON;
    if (memcpy_s(st_80211_ucast_switch.auc_user_macaddr, WLAN_MAC_ADDR_LEN, (const void *)puc_mac_addr,
        OAL_SIZEOF(st_80211_ucast_switch.auc_user_macaddr)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hmac_user_add::memcpy_s failed!");
        hmac_user_free(us_user_idx);
        return OAL_FAIL;
    }
    hmac_config_80211_ucast_switch(pst_mac_vap, OAL_SIZEOF(st_80211_ucast_switch), (oal_uint8 *)&st_80211_ucast_switch);

    st_80211_ucast_switch.en_frame_direction = OAM_OTA_FRAME_DIRECTION_TYPE_RX;
    st_80211_ucast_switch.en_frame_type = OAM_USER_TRACK_FRAME_TYPE_MGMT;
    st_80211_ucast_switch.en_frame_switch = OAL_SWITCH_ON;
    st_80211_ucast_switch.en_cb_switch = OAL_SWITCH_ON;
    st_80211_ucast_switch.en_dscr_switch = OAL_SWITCH_ON;
    hmac_config_80211_ucast_switch(pst_mac_vap, OAL_SIZEOF(st_80211_ucast_switch), (oal_uint8 *)&st_80211_ucast_switch);

    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::user[%d] mac:%02X:XX:XX:XX:%02X:%02X}",
        us_user_idx, puc_mac_addr[0], puc_mac_addr[4], puc_mac_addr[5]);

    return OAL_SUCC;
}


oal_uint32 hmac_config_add_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_add_user_param_stru    *pst_add_user   = OAL_PTR_NULL;
    oal_uint16                      us_user_index;
    hmac_vap_stru                  *pst_hmac_vap   = OAL_PTR_NULL;
    hmac_user_stru                 *pst_hmac_user  = OAL_PTR_NULL;
    oal_uint32                      ul_ret;
    mac_user_ht_hdl_stru            st_ht_hdl;
    oal_uint32                      ul_rslt;
    mac_device_stru                *pst_mac_device = OAL_PTR_NULL;

    pst_add_user = (mac_cfg_add_user_param_stru *)puc_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_UM, "{hmac_config_add_user::pst_hmac_vap null.}");
        return OAL_FAIL;
    }

    ul_ret = hmac_user_add(pst_mac_vap, pst_add_user->auc_mac_addr, &us_user_index);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_config_add_user::hmac_user_add failed.}", ul_ret);
        return ul_ret;
    }

    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(us_user_index);
    if (pst_hmac_user == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_config_add_user::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* 设置qos域，后续如有需要可以通过配置命令参数配置 */
    mac_user_set_qos(&pst_hmac_user->st_user_base_info, OAL_TRUE);

    /* 设置HT域 */
    mac_user_get_ht_hdl(&pst_hmac_user->st_user_base_info, &st_ht_hdl);
    st_ht_hdl.en_ht_capable = pst_add_user->en_ht_cap;

    if (pst_add_user->en_ht_cap == OAL_TRUE) {
        pst_hmac_user->st_user_base_info.en_cur_protocol_mode                = WLAN_HT_MODE;
        pst_hmac_user->st_user_base_info.en_avail_protocol_mode              = WLAN_HT_MODE;
    }

    /* 设置HT相关的信息:应该在关联的时候赋值 这个值配置的合理性有待考究 2012->page:786 */
    st_ht_hdl.uc_min_mpdu_start_spacing = 6;
    st_ht_hdl.uc_max_rx_ampdu_factor    = 3;
    mac_user_set_ht_hdl(&pst_hmac_user->st_user_base_info, &st_ht_hdl);

    mac_user_set_asoc_state(&pst_hmac_user->st_user_base_info, MAC_USER_STATE_ASSOC);

    /* 设置amsdu域 */
    hmac_amsdu_init_user(pst_hmac_user);

    /* 抛事件到DMAC层, 同步DMAC数据 */
    /* 重新设置DMAC需要的参数 */
    pst_add_user->us_user_idx = us_user_index;

    ul_ret = hmac_config_send_event(&pst_hmac_vap->st_vap_base_info,
                                    WLAN_CFGID_ADD_USER,
                                    us_len,
                                    puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        /* 异常处理，释放内存 */
        ul_rslt = hmac_user_free(us_user_index);
        if (ul_rslt == OAL_SUCC) {
            pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
            if (pst_mac_device == OAL_PTR_NULL) {
                OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_config_add_user::pst_mac_device null.}");
                return OAL_ERR_CODE_PTR_NULL;
            }

            /* hmac_add_user成功时device下关联用户数已经++, 这里的device下已关联user个数要-- */
            pst_mac_device->uc_asoc_user_cnt--;
        }

        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM,
            "{hmac_config_add_user::hmac_config_send_event failed[%d].}", ul_ret);
        return ul_ret;
    }

    /* 打桩添加用户信息不全，不需要通知算法 */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    if (IS_LEGACY_VAP(pst_mac_vap)) {
        mac_vap_state_change(pst_mac_vap, MAC_VAP_STATE_UP);
    }
#endif

    return OAL_SUCC;
}


oal_uint32 hmac_config_del_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_cfg_del_user_param_stru    *pst_del_user  = OAL_PTR_NULL;
    hmac_user_stru                 *pst_hmac_user = OAL_PTR_NULL;
    hmac_vap_stru                  *pst_hmac_vap  = OAL_PTR_NULL;
    oal_uint16                      us_user_index;
    oal_uint32                      ul_ret;

    pst_del_user = (mac_cfg_add_user_param_stru *)puc_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_config_del_user::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取用户对应的索引 */
    ul_ret = mac_vap_find_user_by_macaddr(pst_mac_vap, pst_del_user->auc_mac_addr, &us_user_index);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM,
            "{hmac_config_del_user::mac_vap_find_user_by_macaddr failed[%d].}", ul_ret);
        return ul_ret;
    }

    /* 获取hmac用户 */
    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(us_user_index);

    ul_ret = hmac_user_del(pst_mac_vap, pst_hmac_user);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM,
            "{hmac_config_del_user::hmac_user_del failed[%d].}", ul_ret);
        return ul_ret;
    }

    /* 重新设置DMAC需要的参数 */
    pst_del_user->us_user_idx = us_user_index;

    /* 抛事件到DMAC层, 同步DMAC数据 */
    ul_ret = hmac_config_send_event(&pst_hmac_vap->st_vap_base_info, WLAN_CFGID_DEL_USER, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        /* 异常处理，释放内存 */
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM,
            "{hmac_config_del_user::hmac_config_send_event failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32 hmac_user_add_multi_user(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_user_index)
{
    oal_uint32      ul_ret;
    oal_uint16      us_user_index;
    mac_user_stru  *pst_mac_user  = OAL_PTR_NULL;
#ifdef _PRE_WLAN_FEATURE_WAPI
    hmac_user_stru *pst_hmac_user = OAL_PTR_NULL;
#endif

    ul_ret = hmac_user_alloc(&us_user_index, OAL_TRUE);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add_multi_user::hmac_user_alloc failed[%d].}",
            ul_ret);
        return ul_ret;
    }

    /* 初始化组播用户基本信息 */
    pst_mac_user = (mac_user_stru *)mac_res_get_mac_user(us_user_index);
    if (pst_mac_user == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add_multi_user::pst_mac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    mac_user_init(pst_mac_user, us_user_index, OAL_PTR_NULL, pst_mac_vap->uc_chip_id,
        pst_mac_vap->uc_device_id, pst_mac_vap->uc_vap_id);

    *pus_user_index = us_user_index;

#ifdef _PRE_WLAN_FEATURE_WAPI
    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(us_user_index);
    if (pst_hmac_user == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_user_add_multi_user::get hmac_user fail.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 初始化wapi对象 */
    hmac_wapi_init(&pst_hmac_user->st_wapi, OAL_FALSE);
#endif

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_user_add_multi_user, user index[%d].}", us_user_index);

    return OAL_SUCC;
}


oal_uint32 hmac_user_del_multi_user(mac_vap_stru *pst_mac_vap)
{
#ifdef _PRE_WLAN_FEATURE_WAPI
    hmac_user_stru      *pst_hmac_user;
#endif

#ifdef _PRE_WLAN_FEATURE_WAPI
    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(pst_mac_vap->us_multi_user_idx);
    if (pst_hmac_user == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_user_add_multi_user::get hmac_user fail.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_wapi_deinit(&pst_hmac_user->st_wapi);
#endif

    hmac_user_free(pst_mac_vap->us_multi_user_idx);

    return OAL_SUCC;
}


#ifdef _PRE_WLAN_FEATURE_WAPI
oal_uint8 hmac_user_is_wapi_connected(oal_uint8 uc_device_id)
{
    oal_uint8               uc_vap_idx;
    hmac_user_stru         *pst_hmac_user_multi;
    mac_device_stru        *pst_mac_device;
    mac_vap_stru           *pst_mac_vap;

    pst_mac_device = mac_res_get_dev(uc_device_id);
    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(0, OAM_SF_UM, "{hmac_user_is_wapi_connected::pst_mac_device null.id %u}", uc_device_id);
        return OAL_FALSE;
    }

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++) {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL)) {
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "vap is null! vap id is %d", pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if (!IS_STA(pst_mac_vap)) {
            continue;
        }

        pst_hmac_user_multi = (hmac_user_stru *)mac_res_get_hmac_user(pst_mac_vap->us_multi_user_idx);
        if ((pst_hmac_user_multi != OAL_PTR_NULL) && (pst_hmac_user_multi->st_wapi.uc_port_valid == OAL_TRUE)) {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}
#endif /* #ifdef _PRE_WLAN_FEATURE_WAPI */


oal_uint32 hmac_user_add_notify_alg(mac_vap_stru *pst_mac_vap, oal_uint16 us_user_idx)
{
    frw_event_mem_stru             *pst_event_mem        = OAL_PTR_NULL;
    frw_event_stru                 *pst_event            = OAL_PTR_NULL;
    dmac_ctx_add_user_stru         *pst_add_user_payload = OAL_PTR_NULL;
    oal_uint32                      ul_ret;
    hmac_user_stru                 *pst_hmac_user        = OAL_PTR_NULL;

    /* 抛事件给Dmac，在dmac层挂用户算法钩子 */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_ctx_add_user_stru));
    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_user_add_notify_alg::pst_event_mem null.}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    pst_add_user_payload = (dmac_ctx_add_user_stru *)pst_event->auc_event_data;
    pst_add_user_payload->us_user_idx = us_user_idx;
    pst_add_user_payload->us_sta_aid = pst_mac_vap->us_sta_aid;
    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(us_user_idx);
    if (OAL_UNLIKELY(pst_hmac_user == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hmac_user_add_notify_alg::null param,pst_hmac_user[%d].}", us_user_idx);
        FRW_EVENT_FREE(pst_event_mem);
        return OAL_ERR_CODE_PTR_NULL;
    }

    mac_user_get_vht_hdl(&pst_hmac_user->st_user_base_info, &pst_add_user_payload->st_vht_hdl);
    mac_user_get_ht_hdl(&pst_hmac_user->st_user_base_info, &pst_add_user_payload->st_ht_hdl);
    /* 填充事件头 */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_NOTIFY_ALG_ADD_USER,
                       OAL_SIZEOF(dmac_ctx_add_user_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_mac_vap->uc_chip_id,
                       pst_mac_vap->uc_device_id,
                       pst_mac_vap->uc_vap_id);

    ul_ret = frw_event_dispatch_event(pst_event_mem);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        /* 异常处理，释放内存 */
        FRW_EVENT_FREE(pst_event_mem);

        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_user_add_notify_alg::frw_event_dispatch_event failed[%d].}", ul_ret);
        return ul_ret;
    }
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}


hmac_user_stru *mac_vap_get_hmac_user_by_addr(mac_vap_stru *pst_mac_vap, const oal_uint8 *puc_mac_addr)
{
    oal_uint32              ul_ret;
    oal_uint16              us_user_idx   = 0xffff;
    hmac_user_stru         *pst_hmac_user = OAL_PTR_NULL;

    /* 根据mac addr找sta索引 */
    ul_ret = mac_vap_find_user_by_macaddr(pst_mac_vap, puc_mac_addr, &us_user_idx);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{mac_vap_get_hmac_user_by_addr::find_user_by_macaddr failed[%d].}", ul_ret);
        if (puc_mac_addr != OAL_PTR_NULL) {
            OAM_WARNING_LOG3(0, OAM_SF_ANY, "{mac_vap_get_hmac_user_by_addr:: mac_addr[%02x XX XX XX %02x %02x]!.}",
                puc_mac_addr[0], puc_mac_addr[4], puc_mac_addr[5]);
        }
        return OAL_PTR_NULL;
    }

    /* 根据sta索引找到user内存区域 */
    pst_hmac_user = mac_res_get_hmac_user(us_user_idx);
    if (pst_hmac_user == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_vap_get_hmac_user_by_addr::user ptr null.}");
    }
    return pst_hmac_user;
}

/*lint -e19*/
oal_module_symbol(hmac_user_alloc);
oal_module_symbol(hmac_user_init);
oal_module_symbol(hmac_config_kick_user);
oal_module_symbol(mac_vap_get_hmac_user_by_addr);
/*lint +e19*/

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

