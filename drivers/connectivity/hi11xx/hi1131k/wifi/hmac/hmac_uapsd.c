

// 1 头文件包含
#include "mac_vap.h"
#include "mac_frame.h"
#include "hmac_mgmt_ap.h"
#include "hmac_encap_frame_ap.h"
#include "hmac_mgmt_bss_comm.h"
#include "hmac_rx_data.h"
#include "hmac_uapsd.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_UAPSD_C

// 2 函数实现
uint8_t hmac_get_max_sp_len(uint8_t max_sp)
{
    uint8_t max_sp_len;

    switch (max_sp) {
        case 1:
            max_sp_len = 2;
            break;
        case 2:
            max_sp_len = 4;
            break;
        case 3:
            max_sp_len = 6;
            break;
        default:
            max_sp_len = HMAC_UAPSD_SEND_ALL;
            break;
    }
    return max_sp_len;
}


oal_void hmac_uapsd_update_user_para (oal_uint8 *puc_mac_hdr, oal_uint8 uc_sub_type, oal_uint32 ul_msg_len,
    hmac_user_stru *pst_hmac_user)
{
#ifdef _PRE_WLAN_FEATURE_UAPSD
    oal_uint32                 ul_idx, ul_ret;
    oal_uint8                  uc_max_sp;
    oal_uint8                  uc_found_wmm = OAL_FALSE;
    oal_uint8                  uc_uapsd_flag = 0;
    mac_user_uapsd_status_stru st_uapsd_status;
    mac_vap_stru               *pst_mac_vap = OAL_PTR_NULL;
    oal_uint16                 us_len, us_len_total;
    oal_uint8                  *puc_param = OAL_PTR_NULL;
    uint8_t                    *wmm_ie = OAL_PTR_NULL;

    ul_idx = MAC_CAP_INFO_LEN + MAC_LIS_INTERVAL_IE_LEN;
    if (uc_sub_type == WLAN_FC0_SUBTYPE_REASSOC_REQ) {
        ul_idx += WLAN_MAC_ADDR_LEN;
    }

    if (ul_idx < ul_msg_len) {
        wmm_ie = mac_get_wmm_ie(puc_mac_hdr + ul_idx, (uint16_t)(ul_msg_len - ul_idx));
        if (wmm_ie != OAL_PTR_NULL) {
            uc_found_wmm = OAL_TRUE;
        }
    }

    /* 不存在WMM IE,直接返回 */
    if (OAL_ALL_INI_VALUE1(uc_found_wmm, wmm_ie)) {
        OAM_WARNING_LOG1(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_PWR, "Could not find WMM IE in assoc req,user_id[%d]\n", pst_hmac_user->st_user_base_info.us_assoc_id);
        return;
    }

    memset_s(&st_uapsd_status, OAL_SIZEOF(mac_user_uapsd_status_stru), 0, OAL_SIZEOF(mac_user_uapsd_status_stru));
    st_uapsd_status.uc_qos_info = wmm_ie[HMAC_UAPSD_WME_LEN];

    /* 8为WMM IE长度 */
    if ((wmm_ie[HMAC_UAPSD_WME_LEN] & BIT0) == BIT0) {
        st_uapsd_status.uc_ac_trigger_ena[WLAN_WME_AC_VO] = 1;
        st_uapsd_status.uc_ac_delievy_ena[WLAN_WME_AC_VO] = 1;
        uc_uapsd_flag |= MAC_USR_UAPSD_EN;
    }
    if ((wmm_ie[HMAC_UAPSD_WME_LEN] & BIT1) == BIT1) {
        st_uapsd_status.uc_ac_trigger_ena[WLAN_WME_AC_VI] = 1;
        st_uapsd_status.uc_ac_delievy_ena[WLAN_WME_AC_VI] = 1;
        uc_uapsd_flag |= MAC_USR_UAPSD_EN;
    }
    if ((wmm_ie[HMAC_UAPSD_WME_LEN] & BIT2) == BIT2) {
        st_uapsd_status.uc_ac_trigger_ena[WLAN_WME_AC_BK] = 1;
        st_uapsd_status.uc_ac_delievy_ena[WLAN_WME_AC_BK] = 1;
        uc_uapsd_flag |= MAC_USR_UAPSD_EN;
    }
    if ((wmm_ie[HMAC_UAPSD_WME_LEN] & BIT3) == BIT3) {
        st_uapsd_status.uc_ac_trigger_ena[WLAN_WME_AC_BE] = 1;
        st_uapsd_status.uc_ac_delievy_ena[WLAN_WME_AC_BE] = 1;
        uc_uapsd_flag |= MAC_USR_UAPSD_EN;
    }

    /* 设置max SP长度 */
    uc_max_sp = (wmm_ie[HMAC_UAPSD_WME_LEN] >> 5) & 0x3;
    st_uapsd_status.uc_max_sp_len = hmac_get_max_sp_len(uc_max_sp);

    /* Send uapsd_flag & uapsd_status syn to dmac */
    pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_hmac_user->st_user_base_info.uc_vap_id);
    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hmac_uapsd_update_user_para::get mac_vap [%d] null.}", pst_hmac_user->st_user_base_info.uc_vap_id);
        return;
    }
    us_len = OAL_SIZEOF(oal_uint16);
    us_len_total = OAL_SIZEOF(oal_uint16) + OAL_SIZEOF(oal_uint8) + OAL_SIZEOF(mac_user_uapsd_status_stru);
    puc_param = (oal_uint8 *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, us_len_total, OAL_TRUE);
    if (OAL_UNLIKELY(puc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_uapsd_update_user_para::puc_param null.}");
        return;
    }
    if (memcpy_s(puc_param, us_len_total, &pst_hmac_user->st_user_base_info.us_assoc_id, us_len) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "hmac_uapsd_update_user_para::memcpy_s fail!");
    }
    if (memcpy_s(puc_param + us_len, us_len_total - us_len, &uc_uapsd_flag, OAL_SIZEOF(uint8_t)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "hmac_uapsd_update_user_para::memcpy_s fail!");
    }
    us_len += OAL_SIZEOF(oal_uint8);
    if (memcpy_s(puc_param + us_len, us_len_total - us_len, &st_uapsd_status,
        OAL_SIZEOF(mac_user_uapsd_status_stru)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "hmac_uapsd_update_user_para::memcpy_s fail!");
    }
    us_len += OAL_SIZEOF(mac_user_uapsd_status_stru);
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_UAPSD_UPDATE, us_len, puc_param);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_uapsd_update_user_para::hmac_config_send_event fail.  erro code is %u}", ul_ret);
    }
    OAL_MEM_FREE(puc_param, OAL_TRUE);
#endif
    return ;
}


#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif

