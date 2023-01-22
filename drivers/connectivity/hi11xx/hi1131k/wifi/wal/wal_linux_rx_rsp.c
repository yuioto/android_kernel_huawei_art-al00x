

#include "oal_ext_if.h"
#include "wlan_types.h"
#include "frw_ext_if.h"
#include "mac_device.h"
#include "mac_resource.h"
#include "mac_vap.h"
#include "mac_regdomain.h"
#include "hmac_ext_if.h"
#include "wal_ext_if.h"
#include "wal_main.h"
#include "wal_config.h"
#include "wal_linux_scan.h"
#include "wal_linux_cfg80211.h"
#include "wal_linux_ioctl.h"
#include "wal_linux_flowctl.h"
#include "oal_cfg80211.h"
#include "oal_net.h"
#include "hmac_resource.h"
#include "hmac_device.h"
#include "hmac_scan.h"
#include "securec.h"
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#include "hmac_user.h"
#endif
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "plat_pm_wlan.h"
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "hmac_config.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_LINUX_RX_RSP_C

#ifdef _PRE_WLAN_FEATURE_DFR
extern wal_dfr_info_stru g_st_dfr_info;
#endif // _PRE_WLAN_FEATURE_DFR

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
extern oal_uint32 hwal_send_others_bss_data(oal_netbuf_stru *pst_netbuf);
#endif


oal_uint32 wal_scan_comp_proc_sta(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru     *pst_event = OAL_PTR_NULL;
    hmac_scan_rsp_stru *pst_scan_rsp = OAL_PTR_NULL;
    hmac_device_stru   *pst_hmac_device = OAL_PTR_NULL;
    hmac_vap_stru      *pst_hmac_vap = OAL_PTR_NULL;
    hmac_bss_mgmt_stru *pst_bss_mgmt = OAL_PTR_NULL;
    hmac_scan_stru     *pst_scan_mgmt = OAL_PTR_NULL;
    oal_wiphy_stru     *pst_wiphy = OAL_PTR_NULL;

    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{wal_scan_comp_proc_sta::pst_event_mem is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    /* ��ȡhmac vap�ṹ�� */
    pst_hmac_vap = mac_res_get_hmac_vap(pst_event->st_event_hdr.uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_event->st_event_hdr.uc_vap_id, OAM_SF_SCAN,
            "{wal_scan_comp_proc_sta::pst_hmac_vap is NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ɾ���ȴ�ɨ�賬ʱ��ʱ�� */
    if (pst_hmac_vap->st_scan_timeout.en_is_registerd == OAL_TRUE) {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_vap->st_scan_timeout));
    }

    /* ��ȡhmac device ָ�� */
    pst_hmac_device = hmac_res_get_mac_dev(pst_event->st_event_hdr.uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(pst_event->st_event_hdr.uc_vap_id, OAM_SF_SCAN,
            "{wal_scan_comp_proc_sta::pst_hmac_device[%d] is null!}", pst_event->st_event_hdr.uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_scan_mgmt = &(pst_hmac_device->st_scan_mgmt);
    pst_wiphy = pst_hmac_device->pst_device_base_info->pst_wiphy;

    /* ��ȡɨ�����Ĺ���ṹ��ַ */
    pst_bss_mgmt = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.st_bss_mgmt);

    /* ��ȡ�����ϱ���ɨ�����ṹ��ָ�� */
    pst_scan_rsp = (hmac_scan_rsp_stru *)pst_event->auc_event_data;

    /* ���ɨ�践�ؽ���ķǳɹ�����ӡά����Ϣ */
    if (pst_scan_rsp->uc_result_code != HMAC_MGMT_SUCCESS) {
        OAM_WARNING_LOG1(pst_event->st_event_hdr.uc_vap_id, OAM_SF_SCAN,
            "{wal_scan_comp_proc_sta::scan not succ, err_code[%d]!}", pst_scan_rsp->uc_result_code);
    }

    /* �ϱ�����ɨ�赽��bss, ����ɨ�����ɹ����ͳһ�ϱ�ɨ�������м����ϱ����� */
    wal_inform_all_bss(pst_wiphy, pst_bss_mgmt, pst_event->st_event_hdr.uc_vap_id);

    /* �����ں��·���ɨ��request��Դ���� */
    oal_spin_lock(&(pst_scan_mgmt->st_scan_request_spinlock));

    /* û��δ�ͷŵ�ɨ����Դ��ֱ�ӷ��� */
    if ((pst_scan_mgmt->pst_request == OAL_PTR_NULL) && (pst_scan_mgmt->pst_sched_scan_req == OAL_PTR_NULL)) {
        /* ֪ͨ���ںˣ��ͷ���Դ����� */
        oal_spin_unlock(&(pst_scan_mgmt->st_scan_request_spinlock));
        return OAL_SUCC;
    }

    /* �ϲ��·�����ͨɨ����ж�Ӧ���� */
    if (pst_scan_mgmt->pst_request != OAL_PTR_NULL) {
        /* ֪ͨ kernel scan �Ѿ����� */
        oal_cfg80211_scan_done(pst_scan_mgmt->pst_request, 0);

        pst_scan_mgmt->pst_request = OAL_PTR_NULL;
        pst_scan_mgmt->en_complete = OAL_TRUE;

        /* �ñ������Ż�ʱ��֤OAL_WAIT_QUEUE_WAKE_UP�����ִ�� */
        OAL_SMP_MB();
        OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&pst_scan_mgmt->st_wait_queue);
    } else if (pst_scan_mgmt->pst_sched_scan_req != OAL_PTR_NULL) {
        /* �ϱ�����ɨ���� */
        oal_cfg80211_sched_scan_result(pst_wiphy);

        pst_scan_mgmt->pst_sched_scan_req = OAL_PTR_NULL;
        pst_scan_mgmt->en_sched_scan_complete = OAL_TRUE;
    }

    /* ֪ͨ���ںˣ��ͷ���Դ����� */
    oal_spin_unlock(&(pst_scan_mgmt->st_scan_request_spinlock));

    return OAL_SUCC;
}
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)

OAL_STATIC uint32_t wal_update_bss_when_assoc_comp(frw_event_stru         *event,
                                                   oal_connet_result_stru connet_result,
                                                   hmac_asoc_rsp_stru     *asoc_rsp)
{
    hmac_device_stru   *hmac_device = NULL;
    oal_wiphy_stru     *wiphy       = NULL;
    hmac_bss_mgmt_stru *bss_mgmt    = NULL;

    if (connet_result.us_status_code == MAC_SUCCESSFUL_STATUSCODE) {
        hmac_device = hmac_res_get_mac_dev(event->st_event_hdr.uc_device_id);
        if (OAL_ANY_NULL_PTR3(hmac_device, hmac_device->pst_device_base_info,
                              hmac_device->pst_device_base_info->pst_wiphy)) {
            OAM_ERROR_LOG0(event->st_event_hdr.uc_vap_id, OAM_SF_ASSOC,
                           "{wal_update_bss_when_assoc_comp::get ptr is null!}");
            return OAL_ERR_CODE_PTR_NULL;
        }
        wiphy = hmac_device->pst_device_base_info->pst_wiphy;
        bss_mgmt = &(hmac_device->st_scan_mgmt.st_scan_record_mgmt.st_bss_mgmt);

        wal_update_bss(wiphy, bss_mgmt, connet_result.auc_bssid);
    }
    return OAL_SUCC;
}
#endif

OAL_STATIC uint32_t wal_asoc_comp_report_result(oal_net_device_stru *net_device, frw_event_stru *update_assoc_event,
    hmac_asoc_rsp_stru *asoc_rsp)
{
    oal_connet_result_stru  connet_result;
    uint32_t                ret;
    memset_s(&connet_result, OAL_SIZEOF(oal_connet_result_stru), 0, OAL_SIZEOF(oal_connet_result_stru));

    /* ׼���ϱ��ں˵Ĺ�������ṹ�� */
    if (memcpy_s(connet_result.auc_bssid, WLAN_MAC_ADDR_LEN, asoc_rsp->auc_addr_ap, WLAN_MAC_ADDR_LEN) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_asoc_comp_proc_sta::memcpy_s failed!");
        return OAL_FAIL;
    }
    connet_result.puc_req_ie     = asoc_rsp->puc_asoc_req_ie_buff;
    connet_result.ul_req_ie_len  = asoc_rsp->ul_asoc_req_ie_len;
    connet_result.puc_rsp_ie     = asoc_rsp->puc_asoc_rsp_ie_buff;
    connet_result.ul_rsp_ie_len  = asoc_rsp->ul_asoc_rsp_ie_len;
    connet_result.us_status_code = asoc_rsp->en_status_code;

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    
    if (wal_update_bss_when_assoc_comp(update_assoc_event, connet_result, asoc_rsp) != OAL_SUCC) {
        return OAL_FAIL;
    }
#endif
    /* �����ں˽ӿڣ��ϱ�������� */
    ret = oal_cfg80211_connect_result(net_device, connet_result.auc_bssid, connet_result.puc_req_ie,
        connet_result.ul_req_ie_len, connet_result.puc_rsp_ie, connet_result.ul_rsp_ie_len,
        connet_result.us_status_code, GFP_ATOMIC);
    if (ret != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_ASSOC, "{wal_asoc_comp_proc_sta::oal_cfg80211_connect_result fail[%d]!}", ret);
    }

    OAM_WARNING_LOG1(0, OAM_SF_ASSOC, "{wal_asoc_comp_proc_sta status_code[%d] OK!}", connet_result.us_status_code);
    return OAL_SUCC;
}


oal_uint32 wal_asoc_comp_proc_sta(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru         *pst_event = OAL_PTR_NULL;
    oal_net_device_stru    *pst_net_device = OAL_PTR_NULL;
    hmac_asoc_rsp_stru     *pst_asoc_rsp = OAL_PTR_NULL;
    oal_uint32              ul_ret;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    mac_cfg_ps_open_stru    st_ps_open  = {0};
    mac_vap_stru           *pst_mac_vap = OAL_PTR_NULL;
#endif

    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{wal_asoc_comp_proc_sta::pst_event_mem is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    pst_asoc_rsp = (hmac_asoc_rsp_stru *)pst_event->auc_event_data;

    /* ��ȡnet_device */
    pst_net_device = hmac_vap_get_net_device(pst_event->st_event_hdr.uc_vap_id);
    if (pst_net_device == OAL_PTR_NULL) {
        oal_free(pst_asoc_rsp->puc_asoc_rsp_ie_buff);
        pst_asoc_rsp->puc_asoc_rsp_ie_buff = OAL_PTR_NULL;
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{wal_asoc_comp_proc_sta::get net device ptr is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (wal_asoc_comp_report_result(pst_net_device, pst_event, pst_asoc_rsp) != OAL_SUCC) {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "wal_asoc_comp_proc_sta report result failed.");
        oal_free(pst_asoc_rsp->puc_asoc_rsp_ie_buff);
        pst_asoc_rsp->puc_asoc_rsp_ie_buff = OAL_PTR_NULL;
        return OAL_FAIL;
    }

    /* �ͷŹ�������֡�ڴ� */
    OAL_MEM_FREE(pst_asoc_rsp->puc_asoc_req_ie_buff, OAL_TRUE);
    oal_free(pst_asoc_rsp->puc_asoc_rsp_ie_buff);
    pst_asoc_rsp->puc_asoc_rsp_ie_buff = OAL_PTR_NULL;

#ifdef _PRE_WLAN_FEATURE_11D
    /* ��������ɹ���sta����AP�Ĺ����������Լ��Ĺ����� */
    if (pst_asoc_rsp->en_result_code == HMAC_MGMT_SUCCESS) {
        wal_regdomain_update_sta(pst_event->st_event_hdr.uc_vap_id);
    }
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if (pst_asoc_rsp->en_result_code == HMAC_MGMT_SUCCESS) {
        st_ps_open.uc_pm_ctrl_type = MAC_STA_PM_CTRL_TYPE_HOST;
        st_ps_open.uc_pm_enable    = MAC_STA_PM_SWITCH_OFF;
        pst_mac_vap                = mac_res_get_mac_vap(pst_event->st_event_hdr.uc_vap_id);
        if (pst_mac_vap == OAL_PTR_NULL) {
            OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{wal_asoc_comp_proc_sta::get mac vap ptr is null!}");
            return OAL_ERR_CODE_PTR_NULL;
        }
        hmac_config_set_sta_pm_on(pst_mac_vap, OAL_SIZEOF(mac_cfg_ps_open_stru), (oal_uint8 *)&st_ps_open);
    }
#endif

    ul_ret = hmac_vap_set_asoc_req_ie_ptr_null(pst_event->st_event_hdr.uc_vap_id);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{wal_asoc_comp_proc_sta::hmac_vap_set_asoc_req_ie_ptr_null fail!}");
        return ul_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_ROAM

oal_uint32 wal_roam_comp_proc_sta(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru           *pst_event;
    oal_net_device_stru      *pst_net_device;
    mac_device_stru          *pst_mac_device;
    hmac_roam_rsp_stru       *pst_roam_rsp;
    struct ieee80211_channel *pst_channel;
    enum ieee80211_band       en_band = IEEE80211_NUM_BANDS;
    oal_uint32                ul_ret;
    oal_long                  l_freq;

    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{wal_roam_comp_proc_sta::pst_event_mem is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    pst_roam_rsp = (hmac_roam_rsp_stru *)pst_event->auc_event_data;

    /* ��ȡnet_device */
    pst_net_device = hmac_vap_get_net_device(pst_event->st_event_hdr.uc_vap_id);
    if (pst_net_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_event->st_event_hdr.uc_vap_id, OAM_SF_ROAM,
            "{wal_asoc_comp_proc_sta::get net device ptr is null!}\r\n");
        oal_free(pst_roam_rsp->puc_asoc_rsp_ie_buff);
        pst_roam_rsp->puc_asoc_rsp_ie_buff = OAL_PTR_NULL;
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* ��ȡdevice id ָ�� */
    pst_mac_device = mac_res_get_dev(pst_event->st_event_hdr.uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_event->st_event_hdr.uc_vap_id, OAM_SF_SCAN,
            "{wal_asoc_comp_proc_sta::pst_mac_device is null ptr!}");
        oal_free(pst_roam_rsp->puc_asoc_rsp_ie_buff);
        pst_roam_rsp->puc_asoc_rsp_ie_buff = OAL_PTR_NULL;
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_roam_rsp->st_channel.en_band >= WLAN_BAND_BUTT) {
        OAM_ERROR_LOG1(pst_event->st_event_hdr.uc_vap_id, OAM_SF_ROAM,
            "{wal_asoc_comp_proc_sta::unexpected band[%d]!}\r\n", pst_roam_rsp->st_channel.en_band);
        oal_free(pst_roam_rsp->puc_asoc_rsp_ie_buff);
        pst_roam_rsp->puc_asoc_rsp_ie_buff = OAL_PTR_NULL;
        return OAL_FAIL;
    }
    if (pst_roam_rsp->st_channel.en_band == WLAN_BAND_2G) {
        en_band = IEEE80211_BAND_2GHZ;
    }
#ifdef _PRE_WLAN_FEATURE_5G
    if (pst_roam_rsp->st_channel.en_band == WLAN_BAND_5G) {
        en_band = IEEE80211_BAND_5GHZ;
    }
#endif /* _PRE_WLAN_FEATURE_5G */
    /* for test, flush 192.168.1.1 arp */

    l_freq = oal_ieee80211_channel_to_frequency(pst_roam_rsp->st_channel.uc_chan_number, en_band);

    pst_channel = (struct ieee80211_channel *)oal_ieee80211_get_channel(pst_mac_device->pst_wiphy, l_freq);

    /* �����ں˽ӿڣ��ϱ�������� */
    ul_ret = oal_cfg80211_roamed(pst_net_device,
                                 pst_channel,
                                 pst_roam_rsp->auc_bssid,
                                 pst_roam_rsp->puc_asoc_req_ie_buff,
                                 pst_roam_rsp->ul_asoc_req_ie_len,
                                 pst_roam_rsp->puc_asoc_rsp_ie_buff,
                                 pst_roam_rsp->ul_asoc_rsp_ie_len,
                                 GFP_ATOMIC);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(pst_event->st_event_hdr.uc_vap_id, OAM_SF_ASSOC,
            "{wal_roam_comp_proc_sta::oal_cfg80211_roamed fail[%d]!}", ul_ret);
    }
    OAM_WARNING_LOG4(pst_event->st_event_hdr.uc_vap_id, OAM_SF_ASSOC,
        "{wal_roam_comp_proc_sta::oal_cfg80211_roamed OK asoc_req_ie[%p] len[%d] asoc_rsp_ie[%p] len[%d]!}",
        pst_roam_rsp->puc_asoc_req_ie_buff, pst_roam_rsp->ul_asoc_req_ie_len, pst_roam_rsp->puc_asoc_rsp_ie_buff,
        pst_roam_rsp->ul_asoc_rsp_ie_len);

    oal_free(pst_roam_rsp->puc_asoc_rsp_ie_buff);
    pst_roam_rsp->puc_asoc_rsp_ie_buff = OAL_PTR_NULL;

    /* �ͷŹ�������֡�ڴ� */
    OAL_MEM_FREE(pst_roam_rsp->puc_asoc_req_ie_buff, OAL_TRUE);

    ul_ret = hmac_vap_set_asoc_req_ie_ptr_null(pst_event->st_event_hdr.uc_vap_id);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG0(pst_event->st_event_hdr.uc_vap_id, OAM_SF_ASSOC,
            "{wal_roam_comp_proc_sta::hmac_vap_set_asoc_req_ie_ptr_null fail!}\r\n");
        return ul_ret;
    }

    return OAL_SUCC;
}
#endif // _PRE_WLAN_FEATURE_ROAM
#ifdef _PRE_WLAN_FEATURE_11R

oal_uint32 wal_ft_event_proc_sta(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru             *pst_event;
    oal_net_device_stru        *pst_net_device;
    hmac_roam_ft_stru          *pst_ft_event;
    oal_cfg80211_ft_event_stru  st_cfg_ft_event;
    oal_uint32                  ul_ret;

    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{wal_ft_event_proc_sta::pst_event_mem is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    pst_ft_event = (hmac_roam_ft_stru *)pst_event->auc_event_data;

    /* ��ȡnet_device */
    pst_net_device = hmac_vap_get_net_device(pst_event->st_event_hdr.uc_vap_id);
    if (pst_net_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_event->st_event_hdr.uc_vap_id, OAM_SF_ROAM,
            "{wal_ft_event_proc_sta::get net device ptr is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    st_cfg_ft_event.ies = pst_ft_event->puc_ft_ie_buff;
    st_cfg_ft_event.ies_len = pst_ft_event->us_ft_ie_len;
    st_cfg_ft_event.target_ap = pst_ft_event->auc_bssid;
    st_cfg_ft_event.ric_ies = OAL_PTR_NULL;
    st_cfg_ft_event.ric_ies_len = 0;

    /* �����ں˽ӿڣ��ϱ�������� */
    ul_ret = oal_cfg80211_ft_event(pst_net_device, &st_cfg_ft_event);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(pst_event->st_event_hdr.uc_vap_id, OAM_SF_ASSOC,
            "{wal_ft_event_proc_sta::oal_cfg80211_ft_event fail[%d]!}\r\n", ul_ret);
    }

    return OAL_SUCC;
}
#endif // _PRE_WLAN_FEATURE_11R

oal_uint32 wal_disasoc_comp_proc_sta(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru              *pst_event = OAL_PTR_NULL;
    oal_disconnect_result_stru   st_disconnect_result;
    oal_net_device_stru         *pst_net_device = OAL_PTR_NULL;
    oal_uint32                  *pul_reason_code = OAL_PTR_NULL;
    oal_uint16                   us_disass_reason_code;
    oal_uint16                   us_dmac_reason_code;
    oal_uint32                   ul_ret;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    mac_cfg_ps_open_stru         st_ps_open  = {0};
    mac_vap_stru                *pst_mac_vap = OAL_PTR_NULL;
#endif

    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{wal_disasoc_comp_proc_sta::pst_event_mem is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    /* ��ȡnet_device */
    pst_net_device = hmac_vap_get_net_device(pst_event->st_event_hdr.uc_vap_id);
    if (pst_net_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_event->st_event_hdr.uc_vap_id, OAM_SF_ASSOC,
            "{wal_disasoc_comp_proc_sta::get net device ptr is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡȥ����ԭ����ָ�� */
    pul_reason_code = (oal_uint32 *)pst_event->auc_event_data;
    us_disass_reason_code = (*pul_reason_code) & 0x0000ffff;
    us_dmac_reason_code = ((*pul_reason_code) >> 16) & 0x0000ffff;

    memset_s(&st_disconnect_result, OAL_SIZEOF(oal_disconnect_result_stru), 0, OAL_SIZEOF(oal_disconnect_result_stru));

    /* ׼���ϱ��ں˵Ĺ�������ṹ�� */
    st_disconnect_result.us_reason_code = us_disass_reason_code;

    /* �����ں˽ӿڣ��ϱ�ȥ������� */
    ul_ret = oal_cfg80211_disconnected(pst_net_device, st_disconnect_result.us_reason_code,
                                       st_disconnect_result.pus_disconn_ie, st_disconnect_result.us_disconn_ie_len,
                                       GFP_ATOMIC);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(pst_event->st_event_hdr.uc_vap_id, OAM_SF_ASSOC,
            "{wal_disasoc_comp_proc_sta::oal_cfg80211_disconnected fail[%d]!}\r\n", ul_ret);
        return ul_ret;
    }

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    st_ps_open.uc_pm_ctrl_type = MAC_STA_PM_CTRL_TYPE_HOST;
    st_ps_open.uc_pm_enable    = MAC_STA_PM_SWITCH_ON;
    pst_mac_vap                = mac_res_get_mac_vap(pst_event->st_event_hdr.uc_vap_id);
    if (pst_mac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_event->st_event_hdr.uc_vap_id, OAM_SF_ASSOC,
            "{wal_disasoc_comp_proc_sta::get mac vap ptr is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }
    hmac_config_set_sta_pm_on(pst_mac_vap, OAL_SIZEOF(mac_cfg_ps_open_stru), (oal_uint8 *)&st_ps_open);
#endif

    OAM_WARNING_LOG2(pst_event->st_event_hdr.uc_vap_id, OAM_SF_ASSOC,
        "{wal_disasoc_comp_proc_sta reason_code[%d], dmac_reason_code[%d]OK!}\r\n",
        us_disass_reason_code, us_dmac_reason_code);

    return OAL_SUCC;
}


oal_uint32 wal_connect_new_sta_proc_ap(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru             *pst_event = OAL_PTR_NULL;
    oal_net_device_stru        *pst_net_device = OAL_PTR_NULL;
    oal_uint8                   user_addr[WLAN_MAC_ADDR_LEN] = {0};
    oal_station_info_stru       st_station_info;
    oal_uint32                  ul_ret;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    hmac_asoc_user_req_ie_stru *pst_asoc_user_req_info = OAL_PTR_NULL;
#endif
    oal_uint32                  l_ret = EOK;
    uint8_t                     vap_id;

    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{wal_connect_new_sta_proc_ap::pst_event_mem is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    vap_id = pst_event->st_event_hdr.uc_vap_id;

    /* ��ȡnet_device */
    pst_net_device = hmac_vap_get_net_device(vap_id);
    if (pst_net_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(vap_id, OAM_SF_ASSOC, "{wal_connect_new_sta_proc_ap::get net device ptr is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    memset_s(&st_station_info, OAL_SIZEOF(oal_station_info_stru), 0, OAL_SIZEOF(oal_station_info_stru));

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* ���ں˱������˹�������֡��ie��Ϣ */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
    /* Linux 4.0 �汾����ҪSTATION_INFO_ASSOC_REQ_IES ��ʶ */
#else
    st_station_info.filled |= STATION_INFO_ASSOC_REQ_IES;
#endif

    pst_asoc_user_req_info = (hmac_asoc_user_req_ie_stru *)(pst_event->auc_event_data);
    st_station_info.assoc_req_ies = pst_asoc_user_req_info->puc_assoc_req_ie_buff;
    if (st_station_info.assoc_req_ies == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(vap_id, OAM_SF_ASSOC, "{wal_connect_new_sta_proc_ap::asoc ie is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    st_station_info.assoc_req_ies_len = pst_asoc_user_req_info->ul_assoc_req_ie_len;

    /* ��ȡ����user mac addr */
    l_ret += memcpy_s(user_addr, WLAN_MAC_ADDR_LEN, pst_asoc_user_req_info->auc_user_mac_addr, WLAN_MAC_ADDR_LEN);
#else
    /* ��ȡ����user mac addr */
    l_ret += memcpy_s(user_addr, WLAN_MAC_ADDR_LEN, (oal_uint8 *)pst_event->auc_event_data, WLAN_MAC_ADDR_LEN);
#endif
    if (l_ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_connect_new_sta_proc_ap::memcpy_s failed!");
        return OAL_FAIL;
    }
    /* �����ں˽ӿڣ��ϱ�STA������� */
    ul_ret = oal_cfg80211_new_sta(pst_net_device, user_addr, &st_station_info, GFP_ATOMIC);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(vap_id, OAM_SF_ASSOC, "{wal_connect_new_sta_proc_ap::oal_cfg80211_new_sta fail[%d]!}", ul_ret);
        return ul_ret;
    }

    OAM_WARNING_LOG4(vap_id, OAM_SF_ASSOC, "{wal_connect_new_sta_proc_ap mac[%02X:XX:XX:%02X:%02X:%02X] OK!}",
        user_addr[0], user_addr[3], user_addr[4], user_addr[5]);

    return OAL_SUCC;
}


oal_uint32 wal_disconnect_sta_proc_ap(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru      *pst_event = OAL_PTR_NULL;
    oal_net_device_stru *pst_net_device = OAL_PTR_NULL;
    oal_uint8            auc_disconn_user_addr[WLAN_MAC_ADDR_LEN] = {0};
    oal_uint32           ul_ret;

    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{wal_disconnect_sta_proc_ap::pst_event_mem is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    /* ��ȡnet_device */
    pst_net_device = hmac_vap_get_net_device(pst_event->st_event_hdr.uc_vap_id);
    if (pst_net_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_event->st_event_hdr.uc_vap_id, OAM_SF_ASSOC,
            "{wal_disconnect_sta_proc_ap::get net device ptr is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡȥ����user mac addr */
    if (memcpy_s(auc_disconn_user_addr, WLAN_MAC_ADDR_LEN, (oal_uint8 *)pst_event->auc_event_data,
        WLAN_MAC_ADDR_LEN) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_disconnect_sta_proc_ap::memcpy_s failed!");
        return OAL_FAIL;
    }

    /* �����ں˽ӿڣ��ϱ�STAȥ������� */
    ul_ret = oal_cfg80211_del_sta(pst_net_device, auc_disconn_user_addr, GFP_ATOMIC);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(pst_event->st_event_hdr.uc_vap_id, OAM_SF_ASSOC,
            "{wal_disconnect_sta_proc_ap::oal_cfg80211_del_sta fail[%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    OAM_WARNING_LOG3(pst_event->st_event_hdr.uc_vap_id, OAM_SF_ASSOC,
        "{wal_disconnect_sta_proc_ap mac[%x %x %x] OK!}\r\n", auc_disconn_user_addr[3], auc_disconn_user_addr[4],
        auc_disconn_user_addr[5]);

    return OAL_SUCC;
}


oal_uint32 wal_mic_failure_proc(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru      *pst_event = OAL_PTR_NULL;
    oal_net_device_stru *pst_net_device = OAL_PTR_NULL;
    hmac_mic_event_stru *pst_mic_event = OAL_PTR_NULL;

    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_CRYPTO, "{wal_mic_failure_proc::pst_event_mem is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    pst_mic_event = (hmac_mic_event_stru *)(pst_event->auc_event_data);

    /* ��ȡnet_device */
    pst_net_device = hmac_vap_get_net_device(pst_event->st_event_hdr.uc_vap_id);
    if (pst_net_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_event->st_event_hdr.uc_vap_id, OAM_SF_CRYPTO,
            "{wal_mic_failure_proc::get net device ptr is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �����ں˽ӿڣ��ϱ�mic���� */
    oal_cfg80211_mic_failure(pst_net_device, pst_mic_event->auc_user_mac, pst_mic_event->en_key_type,
        pst_mic_event->l_key_id, NULL, GFP_ATOMIC);

    OAM_WARNING_LOG3(pst_event->st_event_hdr.uc_vap_id, OAM_SF_CRYPTO, "{wal_mic_failure_proc::mac[%x %x %x] OK!}\r\n",
        pst_mic_event->auc_user_mac[3], pst_mic_event->auc_user_mac[4], pst_mic_event->auc_user_mac[5]);

    return OAL_SUCC;
}


oal_uint32 wal_send_mgmt_to_host(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru          *pst_event = OAL_PTR_NULL;
    oal_net_device_stru     *pst_net_device = OAL_PTR_NULL;
    oal_int32                l_freq;
    oal_uint8               *puc_buf = OAL_PTR_NULL;
    oal_uint16               us_len;
    oal_uint32               ul_ret;
    hmac_rx_mgmt_event_stru *pst_mgmt_frame = OAL_PTR_NULL;
    oal_ieee80211_mgmt      *pst_ieee80211_mgmt = OAL_PTR_NULL;

    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_send_mgmt_to_host::pst_event_mem is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    pst_mgmt_frame = (hmac_rx_mgmt_event_stru *)(pst_event->auc_event_data);

    /* ��ȡnet_device */
    pst_net_device = oal_dev_get_by_name(pst_mgmt_frame->ac_name);
    if (pst_net_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_event->st_event_hdr.uc_vap_id, OAM_SF_ANY,
            "{wal_send_mgmt_to_host::get net device ptr is null!}\r\n");
        oal_free(pst_mgmt_frame->puc_buf);
        return OAL_ERR_CODE_PTR_NULL;
    }
    oal_dev_put(pst_net_device);

    puc_buf = pst_mgmt_frame->puc_buf;
    us_len = pst_mgmt_frame->us_len;
    l_freq = pst_mgmt_frame->l_freq;

    pst_ieee80211_mgmt = (oal_ieee80211_mgmt *)puc_buf;
    /* �����ں˽ӿڣ��ϱ����յ�����֡ */
    ul_ret = oal_cfg80211_rx_mgmt(pst_net_device, l_freq, puc_buf, us_len, GFP_ATOMIC);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG2(pst_event->st_event_hdr.uc_vap_id, OAM_SF_ANY,
            "{wal_send_mgmt_to_host::fc[0x%04x], if_type[%d]!}\r\n", pst_ieee80211_mgmt->frame_control,
            pst_net_device->ieee80211_ptr->iftype);
        OAM_WARNING_LOG3(pst_event->st_event_hdr.uc_vap_id, OAM_SF_ANY,
            "{wal_send_mgmt_to_host::oal_cfg80211_rx_mgmt fail[%d]!len[%d], freq[%d]}\r\n", ul_ret, us_len, l_freq);
        oal_free(puc_buf);
        return OAL_FAIL;
    }

    OAM_INFO_LOG3(pst_event->st_event_hdr.uc_vap_id, OAM_SF_ANY,
        "{wal_send_mgmt_to_host::freq = %d, len = %d, TYPE[%04X] OK!}\r\n", l_freq, us_len,
        pst_ieee80211_mgmt->frame_control);
    oal_free(puc_buf);
    return OAL_SUCC;
}


oal_uint32 wal_p2p_listen_timeout(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru               *pst_event = NULL;
    oal_wireless_dev_stru        *pst_wdev = NULL;
    oal_uint64                    ull_cookie;
    oal_ieee80211_channel_stru    st_listen_channel;
    hmac_p2p_listen_expired_stru *p2p_listen_expired = NULL;
    mac_device_stru              *pst_mac_device = NULL;

    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_P2P, "{wal_p2p_listen_timeout::pst_event_mem is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    /* ��ȡmac_device_stru */
    pst_mac_device = mac_res_get_dev(pst_event->st_event_hdr.uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_event->st_event_hdr.uc_vap_id, OAM_SF_PROXYSTA,
            "{wal_p2p_listen_timeout::get mac_device ptr is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    p2p_listen_expired  = (hmac_p2p_listen_expired_stru *)(pst_event->auc_event_data);
    pst_wdev                = p2p_listen_expired->pst_wdev;
    ull_cookie              = pst_mac_device->st_p2p_info.ull_last_roc_id;
    st_listen_channel       = p2p_listen_expired->st_listen_channel;
    oal_cfg80211_remain_on_channel_expired(pst_wdev,
                                           ull_cookie,
                                           &st_listen_channel,
                                           GFP_ATOMIC);
    OAM_INFO_LOG1(0, OAM_SF_P2P, "{wal_p2p_listen_timeout::END!cookie [%x]}\r\n", ull_cookie);
    return OAL_SUCC;
}
#ifdef _PRE_WLAN_FEATURE_HILINK

oal_uint32 wal_send_other_bss_data_to_host(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru  *pst_event  = OAL_PTR_NULL;
    oal_netbuf_stru *pst_netbuf = OAL_PTR_NULL;

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    mac_ieee80211_frame_stru *pst_frame_hdr = OAL_PTR_NULL;
    oal_uint32 ul_ret;
#endif

    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_CRYPTO, "{wal_send_other_bss_data_to_host::pst_event_mem is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    pst_netbuf = (oal_netbuf_stru *)(uintptr_t)(*((oal_ulong *)(pst_event->auc_event_data)));
    if (pst_netbuf == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    ul_ret = hwal_send_others_bss_data(pst_netbuf);
    if (ul_ret != OAL_SUCC) {
        oal_netbuf_free(pst_netbuf);
    }
#endif

    FRW_EVENT_FREE(pst_event_mem);
    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_SAE
/* ��������  : HMAC�ϱ�external auth request���� */
uint32_t wal_report_external_auth_req(frw_event_mem_stru *event_mem)
{
    frw_event_stru                *event = NULL;
    oal_net_device_stru           *net_device = NULL;
    hmac_external_auth_req_stru   *hmac_ext_auth_req = NULL;
    struct cfg80211_external_auth_params nl80211_external_auth_req;
    int32_t                       ret;

    if (OAL_UNLIKELY(event_mem == NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_SAE, "{wal_report_external_auth_req::event_mem is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    event = (frw_event_stru *)frw_get_event_stru(event_mem);

    /* ��ȡnet_device */
    net_device = (oal_net_device_stru *)hmac_vap_get_net_device(event->st_event_hdr.uc_vap_id);
    if (net_device == NULL) {
        OAM_ERROR_LOG1(event->st_event_hdr.uc_vap_id, OAM_SF_SAE,
                       "{wal_report_external_auth_req::get net device ptr is null! vap_id %d}",
                       event->st_event_hdr.uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_ext_auth_req = (hmac_external_auth_req_stru *)(event->auc_event_data);

    nl80211_external_auth_req.action         = hmac_ext_auth_req->action;
    nl80211_external_auth_req.key_mgmt_suite = hmac_ext_auth_req->key_mgmt_suite;
    nl80211_external_auth_req.status         = hmac_ext_auth_req->status;
    nl80211_external_auth_req.ssid.ssid_len  =
        OAL_MIN(hmac_ext_auth_req->ssid.uc_ssid_len, OAL_IEEE80211_MAX_SSID_LEN);
    if (memcpy_s(nl80211_external_auth_req.ssid.ssid, IEEE80211_MAX_SSID_LEN, hmac_ext_auth_req->ssid.auc_ssid,
        nl80211_external_auth_req.ssid.ssid_len) != EOK) {
        OAM_ERROR_LOG0(event->st_event_hdr.uc_vap_id, OAM_SF_SAE, "{wal_report_external_auth_req::memcpy_s fail!}");
        return OAL_FAIL;
    }

    memcpy_s(nl80211_external_auth_req.bssid, WLAN_MAC_ADDR_LEN, hmac_ext_auth_req->bssid, WLAN_MAC_ADDR_LEN);

    ret = (uint32_t)oal_cfg80211_external_auth_request(net_device, &nl80211_external_auth_req, GFP_ATOMIC);
    return ret;
}
#endif /* _PRE_WLAN_FEATURE_SAE */


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
