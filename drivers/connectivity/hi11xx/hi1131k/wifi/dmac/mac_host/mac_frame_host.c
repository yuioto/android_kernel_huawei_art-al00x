

/* 1 头文件包含 */
#include "oam_ext_if.h"
#include "frw_ext_if.h"
#include "dmac_ext_if.h"
#include "mac_ie.h"
#include "mac_frame.h"
#include "mac_vap.h"
#include "mac_device.h"
#include "mac_resource.h"
#include "mac_regdomain.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_FRAME_HOST_C

/* 2 函数原型声明 */
/* 3 全局变量定义 */
/* RSNA OUI 定义 */
oal_uint8    g_auc_rsn_oui[MAC_OUI_LEN] = {0x00, 0x0F, 0xAC};

/* WPA OUI 定义 */
oal_uint8    g_auc_wpa_oui[MAC_OUI_LEN] = {0x00, 0x50, 0xF2};

/* WMM OUI定义 */
oal_uint8    g_auc_wmm_oui[MAC_OUI_LEN] = {0x00, 0x50, 0xF2};

/* WPS OUI 定义 */
oal_uint8    g_auc_wps_oui[MAC_OUI_LEN] = {0x00, 0x50, 0xF2};

/* P2P OUI 定义 */
oal_uint8    g_auc_p2p_oui[MAC_OUI_LEN] = {0x50, 0x6F, 0x9A};

/* 4 函数实现 */

OAL_STATIC oal_void  mac_report_beacon(mac_rx_ctl_stru *pst_rx_cb, oal_netbuf_stru *pst_netbuf)
{
    oal_uint32    ul_ret;

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV)
    oal_uint8     *puc_beacon_payload_addr = OAL_PTR_NULL;

    puc_beacon_payload_addr = oal_netbuf_payload(pst_netbuf);
    ul_ret = oam_report_beacon((oal_uint8 *)mac_get_rx_cb_mac_hdr(pst_rx_cb),
                               pst_rx_cb->bit_mac_header_len,
                               puc_beacon_payload_addr,
                               pst_rx_cb->us_frame_len,
                               OAM_OTA_FRAME_DIRECTION_TYPE_RX);
#else
    ul_ret = oam_report_beacon((oal_uint8 *)pst_rx_cb->pul_mac_hdr_start_addr,
                               pst_rx_cb->uc_mac_header_len,
                               (oal_uint8 *)(pst_rx_cb->pul_mac_hdr_start_addr) + pst_rx_cb->uc_mac_header_len,
                               pst_rx_cb->us_frame_len,
                               OAM_OTA_FRAME_DIRECTION_TYPE_RX);
#endif
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_WIFI_BEACON, "{mac_report_beacon::oam_report_beacon return err: 0x%x.}\r\n", ul_ret);
    }
}


OAL_STATIC oal_uint32  mac_report_80211_get_switch(
    mac_vap_stru               *pst_mac_vap,
    mac_rx_ctl_stru            *pst_rx_cb,
    oal_switch_enum_uint8      *pen_frame_switch,
    oal_switch_enum_uint8      *pen_cb_switch,
    oal_switch_enum_uint8      *pen_dscr_switch)
{
    mac_ieee80211_frame_stru         *pst_frame_hdr;
    oal_uint8                         uc_frame_type = 0;
    oal_uint16                        us_user_idx = 0xffff;
    oal_uint8                        *puc_da = OAL_PTR_NULL;
    oal_uint32                        ul_ret;

    pst_frame_hdr = (mac_ieee80211_frame_stru *)(mac_get_rx_cb_mac_hdr(pst_rx_cb));
    if (pst_frame_hdr == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{mac_report_80211_get_switch::pst_frame_hdr null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if ((pst_frame_hdr->st_frame_control.bit_type == WLAN_CONTROL) ||
        (pst_frame_hdr->st_frame_control.bit_type == WLAN_MANAGEMENT)) {
        uc_frame_type = OAM_USER_TRACK_FRAME_TYPE_MGMT;
    }

    if (pst_frame_hdr->st_frame_control.bit_type == WLAN_DATA_BASICTYPE) {
        uc_frame_type = OAM_USER_TRACK_FRAME_TYPE_DATA;
    }

    /* probe request 和 probe response太多，单独过滤一次 */
    if (pst_frame_hdr->st_frame_control.bit_type == WLAN_MANAGEMENT) {
        if (pst_frame_hdr->st_frame_control.bit_sub_type == WLAN_PROBE_REQ ||
            pst_frame_hdr->st_frame_control.bit_sub_type == WLAN_PROBE_RSP) {
            ul_ret = oam_report_80211_probe_get_switch(OAM_OTA_FRAME_DIRECTION_TYPE_RX,
                                                       pen_frame_switch,
                                                       pen_cb_switch,
                                                       pen_dscr_switch);
            if (ul_ret != OAL_SUCC) {
                OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                                 "{mac_report_80211_get_switch::oam_report_80211_probe_get_switch failed.}");
                return ul_ret;
            }
            return OAL_SUCC;
        } else if (pst_frame_hdr->st_frame_control.bit_sub_type == WLAN_DEAUTH ||
                   pst_frame_hdr->st_frame_control.bit_sub_type == WLAN_DISASOC) {
            *pen_cb_switch    = 1;
            *pen_dscr_switch  = 1;
            *pen_frame_switch = 1;
            return OAL_SUCC;
        } else if (pst_frame_hdr->st_frame_control.bit_sub_type == WLAN_ACTION ||
                   pst_frame_hdr->st_frame_control.bit_sub_type == WLAN_ACTION_NO_ACK) {
            *pen_cb_switch    = 1;
            *pen_dscr_switch  = 1;
            *pen_frame_switch = 1;
            return OAL_SUCC;
        }
    }

    mac_rx_get_da(pst_frame_hdr, &puc_da);

    if (ETHER_IS_MULTICAST(puc_da)) {
        ul_ret = oam_report_80211_mcast_get_switch(OAM_OTA_FRAME_DIRECTION_TYPE_RX,
                                                   uc_frame_type,
                                                   pen_frame_switch,
                                                   pen_cb_switch,
                                                   pen_dscr_switch);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG2(0, OAM_SF_RX,
                "{mac_report_80211_get_switch::oam_report_80211_mcast_get_switch failed! ul_ret=[%d],frame_type=[%d]}",
                ul_ret, uc_frame_type);
            return ul_ret;
        }
    } else {
        ul_ret = mac_vap_find_user_by_macaddr(pst_mac_vap,
                                              pst_frame_hdr->auc_address2,
                                              &us_user_idx);
        if (ul_ret == OAL_ERR_CODE_PTR_NULL) {
            MAC_ERR_LOG(0, "mac_vap_find_user_by_macaddr return null ptr!");
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                             "{mac_report_80211_get_switch::mac_vap_find_user_by_macaddr failed.}");
            return ul_ret;
        }

        if (ul_ret == OAL_FAIL) {
            *pen_cb_switch    = 0;
            *pen_dscr_switch  = 0;
            *pen_frame_switch = 0;

            return OAL_FAIL;
        }

        ul_ret = oam_report_80211_ucast_get_switch(OAM_OTA_FRAME_DIRECTION_TYPE_RX,
                                                   uc_frame_type,
                                                   pen_frame_switch,
                                                   pen_cb_switch,
                                                   pen_dscr_switch,
                                                   us_user_idx);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_RX,
                             "{mac_report_80211_get_switch::oam_report_80211_ucast_get_switch failed! ul_ret=[%d],"
                             "frame_type=[%d], user_idx=[%d]}", ul_ret, uc_frame_type, us_user_idx);
            OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_RX, "{oam_report_80211_ucast_get_switch::frame_switch=[%p],"
                             "cb_switch=[%p], dscr_switch=[%p]", (uintptr_t)pen_frame_switch, (uintptr_t)pen_cb_switch,
                             (uintptr_t)pen_dscr_switch);

            return ul_ret;
        }
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  mac_report_80211_get_user_macaddr(mac_rx_ctl_stru *pst_rx_cb,
                                                         oal_uint8 auc_user_macaddr[])
{
    mac_ieee80211_frame_stru *pst_frame_hdr;
    oal_uint8                *puc_da = OAL_PTR_NULL;

    pst_frame_hdr = (mac_ieee80211_frame_stru *)(mac_get_rx_cb_mac_hdr(pst_rx_cb));
    if (pst_frame_hdr == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_report_80211_get_user_macaddr::pst_frame_hdr null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    mac_rx_get_da(pst_frame_hdr, &puc_da);

    if (ETHER_IS_MULTICAST(puc_da)) {
        oal_set_mac_addr(auc_user_macaddr, BROADCAST_MACADDR);
    } else {
        oal_set_mac_addr(auc_user_macaddr, pst_frame_hdr->auc_address2);
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  mac_report_80211_frame(mac_vap_stru *pst_mac_vap,
                                              mac_rx_ctl_stru *pst_rx_cb,
                                              oal_netbuf_stru *pst_netbuf,
                                              oam_ota_type_enum_uint8 en_ota_type)
{
    oal_switch_enum_uint8           en_frame_switch = 0;
    oal_switch_enum_uint8           en_cb_switch = 0;
    oal_switch_enum_uint8           en_dscr_switch = 0;
    oal_uint32                      ul_ret;
    oal_uint8                       auc_user_macaddr[WLAN_MAC_ADDR_LEN] = {0};
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV)
    oal_uint8                      *puc_mac_payload_addr = OAL_PTR_NULL;
#endif

    /* 获取打印开关 */
    ul_ret = mac_report_80211_get_switch(pst_mac_vap,
                                         pst_rx_cb,
                                         &en_frame_switch,
                                         &en_cb_switch,
                                         &en_dscr_switch);
    if (ul_ret == OAL_ERR_CODE_PTR_NULL) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                         "{mac_report_80211_frame::mac_report_80211_get_switch failed.}");
        return ul_ret;
    }

    if (ul_ret == OAL_FAIL) {
        return ul_ret;
    }

    /* 获取发送端用户地址，用户SDT过滤,如果是组播\广播帧，则地址填为全F */
    ul_ret = mac_report_80211_get_user_macaddr(pst_rx_cb, auc_user_macaddr);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                         "{mac_report_80211_frame::mac_report_80211_get_user_macaddr failed.}");

        return ul_ret;
    }

    /* 上报接收到的帧 */
    if (en_frame_switch == OAL_SWITCH_ON) {
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV)
        puc_mac_payload_addr = oal_netbuf_payload(pst_netbuf);

        ul_ret = oam_report_80211_frame(auc_user_macaddr,
                                        (oal_uint8 *)mac_get_rx_cb_mac_hdr(pst_rx_cb),
                                        pst_rx_cb->bit_mac_header_len,
                                        puc_mac_payload_addr,
                                        pst_rx_cb->us_frame_len,
                                        OAM_OTA_FRAME_DIRECTION_TYPE_RX);
#else
        ul_ret = oam_report_80211_frame(auc_user_macaddr,
                                        (oal_uint8 *)pst_rx_cb->pul_mac_hdr_start_addr,
                                        pst_rx_cb->uc_mac_header_len,
                                        (oal_uint8 *)(pst_rx_cb->pul_mac_hdr_start_addr) + pst_rx_cb->uc_mac_header_len,
                                        pst_rx_cb->us_frame_len,
                                        OAM_OTA_FRAME_DIRECTION_TYPE_RX);
#endif
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_RX,
                             "{mac_report_80211_frame::oam_report_80211_frame return err: 0x%x.}\r\n", ul_ret);
        }
    }

    /* 上报接收帧的CB字段 */
    if (en_cb_switch == OAL_SWITCH_ON) {
        ul_ret = oam_report_netbuf_cb(auc_user_macaddr, (oal_uint8 *)pst_rx_cb, en_ota_type);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_RX,
                             "{mac_report_80211_frame::oam_report_netbuf_cb return err: 0x%x.}\r\n", ul_ret);
        }
    }
    return OAL_SUCC;
}


oal_uint32  mac_rx_report_80211_frame(oal_uint8 *pst_vap,
                                      oal_uint8 *pst_rx_cb,
                                      oal_netbuf_stru *pst_netbuf,
                                      oam_ota_type_enum_uint8 en_ota_type)
{
    oal_uint8            uc_sub_type;
    mac_vap_stru        *pst_mac_vap = OAL_PTR_NULL;
    mac_rx_ctl_stru     *pst_mac_rx_cb = OAL_PTR_NULL;

    if (pst_rx_cb == OAL_PTR_NULL || pst_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{mac_rx_report_80211_frame::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap = (mac_vap_stru *)pst_vap;
    pst_mac_rx_cb = (mac_rx_ctl_stru *)pst_rx_cb;

    uc_sub_type = mac_get_frame_sub_type((oal_uint8 *)mac_get_rx_cb_mac_hdr(pst_mac_rx_cb));
    if (uc_sub_type == WLAN_FC0_SUBTYPE_BEACON) {
        mac_report_beacon(pst_mac_rx_cb, pst_netbuf);
    } else {
        mac_report_80211_frame(pst_mac_vap, pst_mac_rx_cb, pst_netbuf, en_ota_type);
    }

    return OAL_SUCC;
}
#ifdef _PRE_WLAN_FEATURE_P2P
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

oal_uint8 *mac_find_p2p_attribute(oal_uint8 uc_eid, oal_uint8 *puc_ies, oal_int32 l_len)
{
    oal_int32 l_ie_len = 0;

    if (puc_ies == OAL_PTR_NULL) {
        return OAL_PTR_NULL;
    }

    /* 查找P2P IE，如果不是直接找下一个 */
    while (l_len > MAC_P2P_ATTRIBUTE_HDR_LEN && puc_ies[0] != uc_eid) {
        l_ie_len = (oal_int32)((puc_ies[2] << 8) + puc_ies[1]);
        l_len   -= l_ie_len + MAC_P2P_ATTRIBUTE_HDR_LEN;
        puc_ies += l_ie_len + MAC_P2P_ATTRIBUTE_HDR_LEN; //lint !e679
    }

    /* 查找到P2P IE，剩余长度不匹配直接返回空指针 */
    l_ie_len = (oal_int32)((puc_ies[2] << 8) + puc_ies[1]);
    if ((l_len < MAC_P2P_ATTRIBUTE_HDR_LEN) || (l_len < (MAC_P2P_ATTRIBUTE_HDR_LEN + l_ie_len))) {
        return OAL_PTR_NULL;
    }

    return puc_ies;
}
#endif // #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
#endif

oal_uint8 *mac_find_ie(oal_uint8 uc_eid, oal_uint8 *puc_ies, oal_int32 l_len)
{
    if (puc_ies == OAL_PTR_NULL) {
        return OAL_PTR_NULL;
    }

    while (l_len > MAC_IE_HDR_LEN && puc_ies[0] != uc_eid) {
        l_len   -= puc_ies[1] + MAC_IE_HDR_LEN;
        puc_ies += puc_ies[1] + MAC_IE_HDR_LEN;
    }

    if ((l_len < MAC_IE_HDR_LEN) || (l_len < (MAC_IE_HDR_LEN + puc_ies[1]))) {
        return OAL_PTR_NULL;
    }

    return puc_ies;
}


oal_uint8 *mac_find_vendor_ie(oal_uint32    ul_oui,
                              oal_uint8     uc_oui_type,
                              oal_uint8    *puc_ies,
                              oal_int32     l_len)
{
    struct mac_ieee80211_vendor_ie *pst_ie = OAL_PTR_NULL;
    oal_uint8 *puc_pos = OAL_PTR_NULL;
    oal_uint8 *puc_end = OAL_PTR_NULL;
    oal_uint32 ul_ie_oui;

    if (puc_ies == OAL_PTR_NULL) {
        return OAL_PTR_NULL;
    }

    puc_pos = puc_ies;
    puc_end = puc_ies + l_len;
    while (puc_pos < puc_end) {
        puc_pos = mac_find_ie(MAC_EID_VENDOR, puc_pos, (oal_int32)(puc_end - puc_pos));
        if (puc_pos == OAL_PTR_NULL) {
            return OAL_PTR_NULL;
        }

        pst_ie = (struct mac_ieee80211_vendor_ie *)puc_pos;

        if (pst_ie->uc_len >= (sizeof(*pst_ie) - MAC_IE_HDR_LEN)) {
            ul_ie_oui = (pst_ie->auc_oui[0] << 16) | (pst_ie->auc_oui[1] << 8) | pst_ie->auc_oui[2];
            if ((ul_ie_oui == ul_oui) && (pst_ie->uc_oui_type == uc_oui_type)) {
                return puc_pos;
            }
        }
        puc_pos += 2 + pst_ie->uc_len;
    }
    return OAL_PTR_NULL;
}


oal_void  mac_set_beacon_interval_field(oal_void *pst_vap, oal_uint8 *puc_buffer)
{
    oal_uint16   *pus_bcn_int;
    oal_uint32    ul_bcn_int;
    mac_vap_stru *pst_mac_vap = (mac_vap_stru *)pst_vap;

    /*
     *           |Beacon interval|
     *   Octets:        2
     */
    pus_bcn_int = (oal_uint16 *)puc_buffer;

    ul_bcn_int = pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.ul_dot11BeaconPeriod;

    *pus_bcn_int = (oal_uint16)OAL_BYTEORDER_TO_LE32(ul_bcn_int);
}


oal_void  mac_set_cap_info_ap(oal_void *pst_vap, oal_uint8 *puc_cap_info)
{
    mac_cap_info_stru  *pst_cap_info = (mac_cap_info_stru *)puc_cap_info;
    mac_vap_stru       *pst_mac_vap  = (mac_vap_stru *)pst_vap;

    wlan_mib_ieee802dot11_stru *pst_mib = pst_mac_vap->pst_mib_info;
    /*
     * -------------------------------------------------------------------
     * |B0 |B1  |B2        |B3    |B4     |B5      |B6  |B7     |B8      |
     * -------------------------------------------------------------------
     * |ESS|IBSS|CFPollable|CFPReq|Privacy|Preamble|PBCC|Agility|SpecMgmt|
     * -------------------------------------------------------------------
     * |B9 |B10      |B11 |B12     |B13      |B14        |B15            |
     * -------------------------------------------------------------------
     * |QoS|ShortSlot|APSD|RM      |DSSS-OFDM|Delayed BA |Immediate BA   |
     * -------------------------------------------------------------------
     */
    /* 初始清零 */
    puc_cap_info[0] = 0;
    puc_cap_info[1] = 0;

    if (pst_mib->st_wlan_mib_sta_config.en_dot11DesiredBSSType == WLAN_MIB_DESIRED_BSSTYPE_INDEPENDENT) {
        pst_cap_info->bit_ibss = 1;
    } else if (pst_mib->st_wlan_mib_sta_config.en_dot11DesiredBSSType == WLAN_MIB_DESIRED_BSSTYPE_INFRA) {
        pst_cap_info->bit_ess = 1;
    }

    /* The Privacy bit is set if WEP is enabled */
    pst_cap_info->bit_privacy = pst_mib->st_wlan_mib_privacy.en_dot11PrivacyInvoked;

    /* preamble */
    pst_cap_info->bit_short_preamble = mac_mib_get_ShortPreambleOptionImplemented(pst_mac_vap);

    /* packet binary convolutional code (PBCC) modulation */
    pst_cap_info->bit_pbcc = pst_mib->st_phy_hrdsss.en_dot11PBCCOptionImplemented;

    /* Channel Agility */
    pst_cap_info->bit_channel_agility = pst_mib->st_phy_hrdsss.en_dot11ChannelAgilityPresent;

    /* Spectrum Management */
    pst_cap_info->bit_spectrum_mgmt = pst_mib->st_wlan_mib_sta_config.en_dot11SpectrumManagementRequired;

    /* QoS subfield */
    pst_cap_info->bit_qos = pst_mib->st_wlan_mib_sta_config.en_dot11QosOptionImplemented;

    /* short slot */
    pst_cap_info->bit_short_slot_time = pst_mib->st_phy_erp.en_dot11ShortSlotTimeOptionActivated &
        pst_mib->st_phy_erp.en_dot11ShortSlotTimeOptionImplemented;

    /* APSD */
    pst_cap_info->bit_apsd = pst_mib->st_wlan_mib_sta_config.en_dot11APSDOptionImplemented;

    /* Radio Measurement */
    pst_cap_info->bit_radio_measurement = pst_mib->st_wlan_mib_sta_config.en_dot11RadioMeasurementActivated;

    /* DSSS-OFDM */
    pst_cap_info->bit_dsss_ofdm = pst_mib->st_phy_erp.en_dot11DSSSOFDMOptionActivated;

    /* Delayed BA */
    pst_cap_info->bit_delayed_block_ack = pst_mib->st_wlan_mib_sta_config.en_dot11DelayedBlockAckOptionImplemented;

    /* Immediate Block Ack 参考STA及AP标杆，此能力一直为0,实际通过addba协商。此处修改为标杆一致。mib值不修改 */
    pst_cap_info->bit_immediate_block_ack = 0;
}

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

oal_void  mac_set_cap_info_sta(oal_void *pst_vap, oal_uint8 *puc_cap_info)
{
    mac_cap_info_stru   *pst_cap_info = (mac_cap_info_stru *)puc_cap_info;
    mac_vap_stru        *pst_mac_vap  = (mac_vap_stru *)pst_vap;
    int32_t ret;

    /*
     *    -------------------------------------------------------------------
     *    |B0 |B1  |B2        |B3    |B4     |B5      |B6  |B7     |B8      |
     *    -------------------------------------------------------------------
     *    |ESS|IBSS|CFPollable|CFPReq|Privacy|Preamble|PBCC|Agility|SpecMgmt|
     *    -------------------------------------------------------------------
     *    |B9 |B10      |B11 |B12     |B13      |B14        |B15            |
     *    -------------------------------------------------------------------
     *    |QoS|ShortSlot|APSD|RM      |DSSS-OFDM|Delayed BA |Immediate BA   |
     *    -------------------------------------------------------------------
     */
    /* 学习对端的能力信息 */
    ret = memcpy_s(puc_cap_info, OAL_SIZEOF(mac_cap_info_stru),
        (uint8_t *)(&pst_mac_vap->us_assoc_user_cap_info), sizeof(pst_mac_vap->us_assoc_user_cap_info));
    if (ret != EOK) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TX, "mac_set_cap_info_sta::memcopy_s return value %d ", ret);
        return;
    }

    /* 以下能力位不学习，保持默认值 */
    pst_cap_info->bit_ibss              = 0;
    pst_cap_info->bit_cf_pollable       = 0;
    pst_cap_info->bit_cf_poll_request   = 0;
    pst_cap_info->bit_radio_measurement =
        pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11RadioMeasurementActivated;
}
#endif // #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)


oal_void  mac_set_ssid_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len, oal_uint16 us_frm_type)
{
    oal_uint8    *puc_ssid = OAL_PTR_NULL;
    oal_uint8     uc_ssid_len;
    mac_vap_stru *pst_mac_vap = (mac_vap_stru *)pst_vap;
    int32_t ret;

    /*
     *               ----------------------------
     *               |Element ID | Length | SSID|
     *               ----------------------------
     *      Octets:  |1          | 1      | 0~32|
     *               ----------------------------
     */
    /*
     * A SSID  field  of length 0 is  used  within Probe
     * Request management frames to indicate the wildcard SSID.
     */
    /* 只有beacon会隐藏ssid */
    if ((pst_mac_vap->st_cap_flag.bit_hide_ssid) && (us_frm_type == WLAN_FC0_SUBTYPE_BEACON)) {
        /* ssid ie */
        *puc_buffer = MAC_EID_SSID;
        /* ssid len */
        *(puc_buffer + 1) = 0;
        *puc_ie_len = MAC_IE_HDR_LEN;
        return;
    }

    *puc_buffer = MAC_EID_SSID;

    puc_ssid = pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11DesiredSSID;

    uc_ssid_len = (oal_uint8)OAL_STRLEN((oal_int8 *)puc_ssid);   /* 不包含'\0' */

    *(puc_buffer + 1) = uc_ssid_len;

    ret = memcpy_s(puc_buffer + MAC_IE_HDR_LEN, WLAN_SSID_MAX_LEN, puc_ssid, uc_ssid_len);
    if (ret != EOK) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TX, "mac_set_ssid_ie::memcopy_s return value %d ", ret);
        return;
    }
    *puc_ie_len = uc_ssid_len + MAC_IE_HDR_LEN;
}


oal_void  mac_set_supported_rates_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    mac_vap_stru     *pst_mac_vap   = (mac_vap_stru *)pst_vap;
    mac_rateset_stru *pst_rates_set;
    oal_uint8         uc_nrates;
    oal_uint8         uc_idx;

    pst_rates_set = &(pst_mac_vap->st_curr_sup_rates.st_rate);

    /* STA全信道扫描时根据频段设置supported rates */
    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA && pst_mac_vap->en_protocol == WLAN_VHT_MODE) {
        if (pst_mac_vap->st_channel.en_band < WLAN_BAND_BUTT) {
            pst_rates_set = &(pst_mac_vap->ast_sta_sup_rates_ie[pst_mac_vap->st_channel.en_band].st_rate);
        }
    }

    /*
     *                   ---------------------------------------
     *                   |Element ID | Length | Supported Rates|
     *                   ---------------------------------------
     *        Octets:    |1          | 1      | 1~8            |
     *                   ---------------------------------------
     * The Information field is encoded as 1 to 8 octets, where each octet describes a single Supported
     * Rate or BSS membership selector.
     */
    puc_buffer[0] = MAC_EID_RATES;

    uc_nrates = pst_rates_set->uc_rs_nrates;

    if (uc_nrates > MAC_MAX_SUPRATES) {
        uc_nrates = MAC_MAX_SUPRATES;
    }

    for (uc_idx = 0; uc_idx < uc_nrates; uc_idx++) {
        puc_buffer[MAC_IE_HDR_LEN + uc_idx] = pst_rates_set->ast_rs_rates[uc_idx].uc_mac_rate;
    }

    puc_buffer[1] = uc_nrates;

    *puc_ie_len = MAC_IE_HDR_LEN + uc_nrates;
}


oal_void mac_set_dsss_params(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    mac_vap_stru    *pst_mac_vap     = (mac_vap_stru *)pst_vap;
    oal_uint8        uc_chan_num;
    mac_device_stru *pst_mac_device;

    /*
     *                   ----------------------------------------
     *                   | Element ID  | Length |Current Channel|
     *                   ----------------------------------------
     *         Octets:   | 1           | 1      | 1             |
     *                   ----------------------------------------
     * The DSSS Parameter Set element contains information to allow channel number identification for STAs.
     */

    
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        *puc_ie_len = 0;
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TX,
                       "mac_set_dsss_params::device_id[%d] ERROR in vap_stru!", pst_mac_vap->uc_device_id);
        return;
    }

    uc_chan_num = pst_mac_vap->st_channel.uc_chan_number;
#if IS_DEVICE
    if ((IS_STA(pst_mac_vap)) && (pst_mac_device->en_curr_scan_state == MAC_SCAN_STATE_RUNNING)) {
        uc_chan_num = pst_mac_device->st_scan_params.ast_channel_list[pst_mac_device->uc_scan_chan_idx].uc_chan_number;
    }
#endif
    puc_buffer[0] = MAC_EID_DSPARMS;
    puc_buffer[1] = MAC_DSPARMS_LEN;
    puc_buffer[2] = uc_chan_num;

    *puc_ie_len = MAC_IE_HDR_LEN + MAC_DSPARMS_LEN;
}

#ifdef _PRE_WLAN_FEATURE_11D


oal_void mac_set_country_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    mac_vap_stru                *pst_mac_vap = (mac_vap_stru *)pst_vap;
    mac_regdomain_info_stru     *pst_rd_info = OAL_PTR_NULL;
    oal_uint8                    uc_band;
    oal_uint8                    uc_index;
    oal_uint32                   ul_ret;
    oal_uint8                    uc_len = 0;

    if (pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11MultiDomainCapabilityActivated != OAL_TRUE &&
        pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11SpectrumManagementRequired != OAL_TRUE &&
        pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11RadioMeasurementActivated != OAL_TRUE) {
        /* 没有使能管制域ie */
        *puc_ie_len = 0;

        return;
    }

    /*
     *                         |....These three fields are repeated...|
     * -------------------------------------------------------------------------------
     * |EID | Len | CountryString | First Channel |Number of |Maximum    | Pad       |
     * |    |     |               |   Number/     |Channels/ |Transmit   |(if needed)|
     * |    |     |               |   Operating   | Operating|Power Level|           |
     * |    |     |               |   Extension   | Class    |/Coverage  |           |
     * |    |     |               |   Identifier  |          |Class      |           |
     * -------------------------------------------------------------------------------
     * |1   |1    |3              |1              |1         |1          |0 or 1     |
     * -------------------------------------------------------------------------------
     */
    /* 读取管制域信息 */
    mac_get_regdomain_info(&pst_rd_info);

    /* 获取当前工作频段 */
    uc_band = pst_mac_vap->st_channel.en_band;

    /* 填写EID, 长度最后填 */
    puc_buffer[0] = MAC_EID_COUNTRY;

    /* 初始化填写buffer的位置 */
    uc_index = MAC_IE_HDR_LEN;

    /* 国家码 */
    puc_buffer[uc_index++] = (oal_uint8)(pst_rd_info->ac_country[0]);
    puc_buffer[uc_index++] = (oal_uint8)(pst_rd_info->ac_country[1]);
    puc_buffer[uc_index++] = ' ';     /* 0表示室内室外规定相同 */

    if (uc_band == WLAN_BAND_2G) {
        ul_ret = mac_set_country_ie_2g(pst_rd_info, &(puc_buffer[uc_index]), &uc_len);
    }
#ifdef _PRE_WLAN_FEATURE_5G
    else if (uc_band == WLAN_BAND_5G) {
        ul_ret = mac_set_country_ie_5g(pst_rd_info, &(puc_buffer[uc_index]), &uc_len);
    }
#endif /* _PRE_WLAN_FEATURE_5G */
    else {
        ul_ret = OAL_FAIL;
    }

    if (ul_ret != OAL_SUCC) {
        *puc_ie_len = 0;
        return;
    }

    if (uc_len == 0) {
        /* 无管制域内容 */
        *puc_ie_len = 0;
        return;
    }

    uc_index += uc_len;

    /* 如果总长度为奇数，则补1字节pad */
    if ((uc_index & BIT0) == 1) {
        puc_buffer[uc_index] = 0;
        uc_index += 1;
    }

    /* 设置信息元素长度 */
    puc_buffer[1] = uc_index - MAC_IE_HDR_LEN;
    *puc_ie_len = uc_index;
}
#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

oal_void mac_set_11ntxbf_vendor_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    mac_vap_stru                        *pst_mac_vap = (mac_vap_stru *)pst_vap;
    mac_11ntxbf_vendor_ie_stru          *pst_vendor_ie = OAL_PTR_NULL;
    if (pst_mac_vap->st_cap_flag.bit_11ntxbf != OAL_TRUE) {
        *puc_ie_len = 0;
        return;
    }

    pst_vendor_ie = (mac_11ntxbf_vendor_ie_stru *)puc_buffer;
    pst_vendor_ie->uc_id = MAC_EID_VENDOR;
    pst_vendor_ie->uc_len = sizeof(mac_11ntxbf_vendor_ie_stru) - MAC_IE_HDR_LEN;
    /* 此值为CCB决策 */
    pst_vendor_ie->uc_ouitype = MAC_EID_11NTXBF;

    /*lint -e572*/ /*lint -e778*/
    pst_vendor_ie->auc_oui[0] = (oal_uint8)((MAC_HUAWEI_VENDER_IE >> 16) & 0xff);
    pst_vendor_ie->auc_oui[1] = (oal_uint8)((MAC_HUAWEI_VENDER_IE >> 8) & 0xff);
    pst_vendor_ie->auc_oui[2] = (oal_uint8)((MAC_HUAWEI_VENDER_IE) & 0xff);
    /*lint +e572*/ /*lint +e778*/

    memset_s(&pst_vendor_ie->st_11ntxbf, OAL_SIZEOF(mac_11ntxbf_info_stru), 0, OAL_SIZEOF(mac_11ntxbf_info_stru));
    pst_vendor_ie->st_11ntxbf.bit_11ntxbf = pst_mac_vap->st_cap_flag.bit_11ntxbf;
    *puc_ie_len = OAL_SIZEOF(mac_11ntxbf_vendor_ie_stru);
}
#endif // #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

#ifdef _PRE_WLAN_FEATURE_HISTREAM

oal_void mac_set_histream_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    mac_vap_stru                        *pst_mac_vap = (mac_vap_stru *)pst_vap;
    /* HISTREAM Information Element Format
     * --------------------------------------------------------------------
     * | 1            |    1     | 2*PORT NUM   | 1        | 2*PORT NUM   |
     * --------------------------------------------------------------------
     * | DEVICE TYPE  | PORT NUM | UDPPORT LIST | PORT NUM | TCPPORT LIST |
     */

    puc_buffer[0] = MAC_EID_VENDOR;
    puc_buffer[1] = 7;

    /*lint -e572*/ /*lint -e778*/
    puc_buffer[2] = (oal_uint8)((MAC_HUAWEI_VENDER_IE >> 16) & 0xff);
    puc_buffer[3] = (oal_uint8)((MAC_HUAWEI_VENDER_IE >> 8) & 0xff);
    puc_buffer[4] = (oal_uint8)((MAC_HUAWEI_VENDER_IE) & 0xff);
    puc_buffer[5] = MAC_HISI_HISTREAM_IE;
    /*lint +e572*/ /*lint +e778*/
    puc_buffer[6] = pst_mac_vap->st_histream_info.en_hidevice_type;
    puc_buffer[7] = 0;
    puc_buffer[8] = 0;

    *puc_ie_len = 9;
}
#endif // _PRE_WLAN_FEATURE_HISTREAM


oal_void mac_set_vendor_hisi_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    struct mac_ieee80211_vendor_ie *pst_vendor_ie;

    pst_vendor_ie = (struct mac_ieee80211_vendor_ie *)puc_buffer;
    pst_vendor_ie->uc_element_id = MAC_EID_VENDOR;
    pst_vendor_ie->uc_len = sizeof(*pst_vendor_ie) - MAC_IE_HDR_LEN;

    /* 此值参照a公司所写 */
    pst_vendor_ie->uc_oui_type = MAC_EID_VHT_TYPE;
    /*lint -e572*/ /*lint -e778*/
    pst_vendor_ie->auc_oui[0] = (oal_uint8)((MAC_HUAWEI_VENDER_IE >> 16) & 0xff);
    pst_vendor_ie->auc_oui[1] = (oal_uint8)((MAC_HUAWEI_VENDER_IE >> 8) & 0xff);
    pst_vendor_ie->auc_oui[2] = (oal_uint8)((MAC_HUAWEI_VENDER_IE) & 0xff);
    /*lint +e572*/ /*lint +e778*/
    *puc_ie_len = OAL_SIZEOF(*pst_vendor_ie);
}


oal_void mac_set_pwrconstraint_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    mac_vap_stru *pst_mac_vap = (mac_vap_stru *)pst_vap;

    /*
     *              -------------------------------------------
     *              |ElementID | Length | LocalPowerConstraint|
     *              -------------------------------------------
     *  Octets:     |1         | 1      | 1                   |
     *              -------------------------------------------
     * 向工作站描述其所允许的最大传输功率，此信息元素记录规定最大值
     * 减去实际使用时的最大值
     */
    if (pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11SpectrumManagementRequired == OAL_FALSE) {
        *puc_ie_len = 0;
        return;
    }

    *puc_buffer       = MAC_EID_PWRCNSTR;
    *(puc_buffer + 1) = MAC_PWR_CONSTRAINT_LEN;

    /* Note that this field is always set to 0 currently. Ideally
       this field can be updated by having an algorithm to decide transmit
       power to be used in the BSS by the AP. */
    *(puc_buffer + MAC_IE_HDR_LEN) = 0;

    *puc_ie_len = MAC_IE_HDR_LEN + MAC_PWR_CONSTRAINT_LEN;
}


oal_void mac_set_quiet_ie(
    void      *pst_vap,    oal_uint8 *puc_buffer, oal_uint8  uc_qcount,
    oal_uint8  uc_qperiod, oal_uint16 us_qduration,    oal_uint16 us_qoffset,
    oal_uint8 *puc_ie_len)
{
    /* 管制域相关 tbd, 需要11h特性进一步分析此ie的设置 */
    mac_quiet_ie_stru *pst_quiet = OAL_PTR_NULL;
    mac_vap_stru      *pst_mac_vap = (mac_vap_stru *)pst_vap;

    if ((pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11SpectrumManagementRequired != OAL_TRUE) &&
        (pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11RadioMeasurementActivated != OAL_TRUE)) {
        *puc_ie_len = 0;

        return;
    }

    /*
     * -----------------------------------------------------------------------------
     * |ElementID | Length | QuietCount | QuietPeriod | QuietDuration | QuietOffset|
     * -----------------------------------------------------------------------------
     * |1         | 1      | 1          | 1           | 2             | 2          |
     * -----------------------------------------------------------------------------
     */
    if (us_qduration == 0 || uc_qcount == 0) {
        *puc_ie_len = 0;

        return;
    }

    *puc_buffer = MAC_EID_QUIET;

    *(puc_buffer + 1) = MAC_QUIET_IE_LEN;

    pst_quiet = (mac_quiet_ie_stru *)(puc_buffer + MAC_IE_HDR_LEN);

    pst_quiet->quiet_count    = uc_qcount;
    pst_quiet->quiet_period   = uc_qperiod;
    pst_quiet->quiet_duration = OAL_BYTEORDER_TO_LE16(us_qduration);
    pst_quiet->quiet_offset   = OAL_BYTEORDER_TO_LE16(us_qoffset);

    *puc_ie_len = MAC_IE_HDR_LEN + MAC_QUIET_IE_LEN;
}


oal_void mac_set_tpc_report_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    mac_vap_stru *pst_mac_vap = (mac_vap_stru *)pst_vap;

    /*
     *           -------------------------------------------------
     *           |ElementID  |Length  |TransmitPower  |LinkMargin|
     *           -------------------------------------------------
     *  Octets:  |1          |1       |1              |1         |
     *           -------------------------------------------------
     * TransimitPower, 此帧的传送功率，以dBm为单位
     */
    if (pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11SpectrumManagementRequired == OAL_FALSE &&
        pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11RadioMeasurementActivated == OAL_FALSE) {
        *puc_ie_len = 0;

        return;
    }

    *puc_buffer       = MAC_EID_TPCREP;
    *(puc_buffer + 1) = MAC_TPCREP_IE_LEN;
    *(puc_buffer + 2) = pst_mac_vap->uc_tx_power;
    *(puc_buffer + 3) = 0; /* 此字段管理帧中不用 */

    *puc_ie_len = MAC_IE_HDR_LEN + MAC_TPCREP_IE_LEN;
}


oal_void mac_set_erp_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    mac_vap_stru         *pst_mac_vap    = (mac_vap_stru *)pst_vap;
    mac_erp_params_stru  *pst_erp_params = OAL_PTR_NULL;

    /*
     * |EID  |Len  |NonERP_Present|Use_Protection|Barker_Preamble_Mode|Reserved|
     * --------------------------------------------------------------------------
     * |B0-B7|B0-B7|B0            |B1            |B2                  |B3-B7   |
     */
    if (
#ifdef _PRE_WLAN_FEATURE_5G
        pst_mac_vap->st_channel.en_band == WLAN_BAND_5G ||
#endif /* _PRE_WLAN_FEATURE_5G */
        pst_mac_vap->en_protocol == WLAN_LEGACY_11B_MODE) {
        *puc_ie_len = 0;

        return;     /* 5G频段和11b协议模式 没有erp信息 */
    }

    *puc_buffer       = MAC_EID_ERP;
    *(puc_buffer + 1) = MAC_ERP_IE_LEN;
    *(puc_buffer + 2) = 0;  /* 初始清0 */

    pst_erp_params = (mac_erp_params_stru *)(puc_buffer + MAC_IE_HDR_LEN);

    /* 如果存在non erp站点与ap关联， 或者obss中存在non erp站点 */
    if ((pst_mac_vap->st_protection.uc_sta_non_erp_num != 0) ||
        (pst_mac_vap->st_protection.bit_obss_non_erp_present == OAL_TRUE)) {
        pst_erp_params->bit_non_erp = 1;
    }

    /* 如果ap已经启用erp保护 */
    if (pst_mac_vap->st_protection.en_protection_mode == WLAN_PROT_ERP) {
        pst_erp_params->bit_use_protection = 1;
    }

    /* 如果存在不支持short preamble的站点与ap关联， 或者ap自身不支持short preamble */
    if ((pst_mac_vap->st_protection.uc_sta_no_short_preamble_num != 0) ||
        (mac_mib_get_ShortPreambleOptionImplemented(pst_mac_vap) == OAL_FALSE)) {
        pst_erp_params->bit_preamble_mode = 1;
    }

    *puc_ie_len = MAC_IE_HDR_LEN + MAC_ERP_IE_LEN;
}


OAL_STATIC oal_void  mac_sort_pcip(oal_uint8 uc_mode, oal_uint8 *puc_pcip, oal_uint8 uc_pw_count)
{
    oal_uint8   uc_loop;
    oal_uint8   uc_temp;

    if (uc_pw_count == 1) {
        return;
    }

    switch (uc_mode) {
        case DMAC_RSNA_802_11I:
        {
            /* If the default value is not CCMP, swap */
            if (puc_pcip[0] != WLAN_80211_CIPHER_SUITE_CCMP) {
                for (uc_loop = 1; uc_loop < uc_pw_count; uc_loop++) {
                    if (puc_pcip[uc_loop] == WLAN_80211_CIPHER_SUITE_CCMP) {
                        break;
                    }
                }

                /* CCMP swap */
                if (uc_loop != uc_pw_count) {
                    uc_temp             = puc_pcip[0];
                    puc_pcip[0]         = WLAN_80211_CIPHER_SUITE_CCMP;
                    puc_pcip[uc_loop]   = uc_temp;
                }
            }

            break;
        }
        case DMAC_WPA_802_11I:
        default: // 在1101 代码中，default 处理分支和WPA 处理分支是一样的处理流程。duankaiyong
        {
            /* If the default value is not CCMP, swap */
            if (puc_pcip[0] != WLAN_80211_CIPHER_SUITE_CCMP) {
                for (uc_loop = 1; uc_loop < uc_pw_count; uc_loop++) {
                    if (puc_pcip[uc_loop] == WLAN_80211_CIPHER_SUITE_CCMP) {
                        break;
                    }
                }

                /* CCMP swap */
                if (uc_loop != uc_pw_count) {
                    uc_temp             = puc_pcip[0];
                    puc_pcip[0]         = WLAN_80211_CIPHER_SUITE_CCMP;
                    puc_pcip[uc_loop]   = uc_temp;
                }
            }

            break;
        } // 在1101 代码中，default 处理分支和WPA 处理分支是一样的处理流程。duankaiyong
    }
}


OAL_STATIC oal_void mac_set_rsn_ie_authenticator(
    oal_void  *pst_vap,
    oal_uint8 *puc_buffer,
    oal_uint8  uc_mode,
    oal_uint8 *puc_ie_len)
{
    mac_vap_stru     *pst_mac_vap = (mac_vap_stru *)pst_vap;
    mac_rsn_cap_stru *pst_rsn_cap = OAL_PTR_NULL;
    oal_uint8        *puc_oui = OAL_PTR_NULL;
    oal_uint8         uc_index;
    oal_uint8         uc_pairwise_suites;
    oal_uint8         uc_authentication_suites;
    oal_uint8         uc_loop = 0;
    oal_uint8         auc_pcip[WLAN_PAIRWISE_CIPHER_SUITES] = {0};
    int32_t ret;

    if (pst_mac_vap->pst_mib_info->st_wlan_mib_privacy.en_dot11RSNAActivated != OAL_TRUE) {
        *puc_ie_len = 0;
        MAC_INFO_VAR(pst_mac_vap->uc_vap_id, "mac_set_rsn_ie_authenticator: en_dot11RSNAActivated = OAL_FALSE");
        return;
    }

    /* 获取实现的pairwise cipher suites个数 */
    if (uc_mode == DMAC_RSNA_802_11I) {
        mac_mib_get_wpa2_pairwise_cipher_suite(pst_mac_vap, &uc_pairwise_suites);

        /* 从MIB 中获取成对密钥套件，设置单播密钥数组 */
        mac_mib_get_wpa2_pairwise_cipher_suite_value(pst_mac_vap, auc_pcip);
    } else if (uc_mode == DMAC_WPA_802_11I) {
        mac_mib_get_wpa_pairwise_cipher_suite(pst_mac_vap, &uc_pairwise_suites);

        /* 从MIB 中获取成对密钥套件，设置单播密钥数组 */
        mac_mib_get_wpa_pairwise_cipher_suite_value(pst_mac_vap, auc_pcip);
    } else {
        *puc_ie_len = 0;
        MAC_ERR_VAR(pst_mac_vap->uc_vap_id, "mac_set_rsn_ie_authenticator: uc_mode = %d", uc_mode);
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{mac_set_rsn_ie_authenticator::uc_mode=%d.}", uc_mode);

        return;
    }

    /* 获取实现的authentication suites个数 */
    mac_mib_get_authentication_suite(pst_mac_vap, &uc_authentication_suites);

    if (uc_authentication_suites == 0) {
        *puc_ie_len = 0;
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                       "{mac_set_rsn_ie_authenticator::uc_authentication_suites=0.}");

        return;
    }

    /*
     *                  RSN Element Format
     * |Element ID | Length | Version | Group Cipher Suite | Pairwise Cipher
     * ---------------------------------------------------------------------
     * | 1         | 1      | 2       |      4             |     2
     * ---------------------------------------------------------------------
     * ---------------------------------------------------------------------
     * Suite | Pairwise Cipher Suite List | AKM Suite Count | AKM Suite List
     * ---------------------------------------------------------------------
     *       | 4-m                        |     2          | 4-n
     * ---------------------------------------------------------------------
     * ---------------------------------------------------------------------
     * |RSN Capabilities|PMKID Count|PMKID List|Group Management Cipher Suite
     * ---------------------------------------------------------------------
     * |    2           |    2      |16 -s     |         4                 |
     * ---------------------------------------------------------------------
     */
    uc_index = MAC_IE_HDR_LEN;

    /* 设置RSN ie的EID */
    puc_oui       = g_auc_rsn_oui;
    puc_buffer[0] = MAC_EID_RSN;

    if (uc_mode == DMAC_WPA_802_11I) {
        puc_buffer[0] = MAC_EID_WPA;

        puc_oui = g_auc_wpa_oui;

        ret = memcpy_s(puc_buffer + uc_index, MAC_OUI_LEN, puc_oui, MAC_OUI_LEN);
        if (ret != EOK) {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TX,
                "mac_set_rsn_ie_authenticator:one::memcopy_s return value %d ", ret);
            return;
        }
        uc_index += MAC_OUI_LEN;

        puc_buffer[uc_index++] = MAC_OUITYPE_WPA; /* 填充WPA 的OUI 类型 */
    }

    /* 设置version字段 */
    puc_buffer[uc_index++] = MAC_RSN_IE_VERSION;
    puc_buffer[uc_index++] = 0;

    /* 设置Group Cipher Suite */
    /*                  Group Cipher Suite
     * ---------------------------------------------------------------------
     *                  | OUI | Suite type |
     * ---------------------------------------------------------------------
     *          Octets: |  3  |     1      |
     */
    ret = memcpy_s(&puc_buffer[uc_index], MAC_OUI_LEN, puc_oui, MAC_OUI_LEN);
    if (ret != EOK) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TX,
            "mac_set_rsn_ie_authenticator:two::memcopy_s return value %d ", ret);
        return;
    }
    uc_index += MAC_OUI_LEN;

    puc_buffer[uc_index++] = pst_mac_vap->pst_mib_info->st_wlan_mib_rsna_cfg.uc_dot11RSNAConfigGroupCipher;

    /* 设置成对加密套件 */
    if (uc_pairwise_suites == 0) {
        /* 单播密钥数为0， 则单播密钥采用组播密钥 */
        puc_buffer[uc_index++] = 1;
        puc_buffer[uc_index++] = 0;

        ret = memcpy_s(&puc_buffer[uc_index], MAC_OUI_LEN, puc_oui, MAC_OUI_LEN);
        if (ret != EOK) {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TX,
                "mac_set_rsn_ie_authenticator:three::memcopy_s return value %d ", ret);
            return;
        }
        uc_index += MAC_OUI_LEN;
        puc_buffer[uc_index++] = WLAN_80211_CIPHER_SUITE_GROUP_CIPHER; /* 协议 Table 8-99.Use group cipher suite */
    } else {
        puc_buffer[uc_index++] = uc_pairwise_suites;
        puc_buffer[uc_index++] = 0;

        /* 对成对密钥进行排序，加密能力高的成对密钥加密套件排序靠前 */
        mac_sort_pcip(uc_mode, auc_pcip, uc_pairwise_suites);

        for (uc_loop = 0; uc_loop < uc_pairwise_suites; uc_loop++) {
            ret = memcpy_s(&puc_buffer[uc_index], MAC_OUI_LEN, puc_oui, MAC_OUI_LEN);
            if (ret != EOK) {
                OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TX,
                    "mac_set_rsn_ie_authenticator:four::memcopy_s return value %d ", ret);
                return;
            }
            uc_index += MAC_OUI_LEN;

            puc_buffer[uc_index++] = auc_pcip[uc_loop];
        }
    }

    /* 设置认证套件数 */
    puc_buffer[uc_index++] = uc_authentication_suites;
    puc_buffer[uc_index++] = 0;

    /* 根据MIB 值，设置认证套件内容 */
    for (uc_loop = 0; uc_loop < WLAN_AUTHENTICATION_SUITES; uc_loop++) {
        if (pst_mac_vap->pst_mib_info->ast_wlan_mib_rsna_cfg_auth_suite[uc_loop].en_dot11RSNAConfigAuthenticationSuiteActivated == OAL_TRUE) {
            ret = memcpy_s(&puc_buffer[uc_index], MAC_OUI_LEN, puc_oui, MAC_OUI_LEN);
            if (ret != EOK) {
                OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TX,
                    "mac_set_rsn_ie_authenticator:five::memcopy_s return value %d ", ret);
                return;
            }
            uc_index += MAC_OUI_LEN;
            puc_buffer[uc_index++] = pst_mac_vap->pst_mib_info->ast_wlan_mib_rsna_cfg_auth_suite[uc_loop].uc_dot11RSNAConfigAuthenticationSuiteImplemented;
        }
    }

    /* 设置 RSN Capabilities字段 */
    /* *********************************************************************** */
    /* --------------------------------------------------------------------- */
    /* | B15 - B6  |  B5 - B4      | B3 - B2     |       B1    |     B0    | */
    /* --------------------------------------------------------------------- */
    /* | Reserved  |  GTSKA Replay | PTSKA Replay| No Pairwise | Pre - Auth| */
    /* |           |    Counter    |   Counter   |             |           | */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /* *********************************************************************** */
    if (uc_mode == DMAC_RSNA_802_11I) {
        /* 设置RSN Capabilities 值，包括Pre_Auth, no_pairwise,  */
        /* Replay counters (PTKSA and GTKSA)                    */
        /* WPA 不需要填写RSN Capabilities 字段                  */
        pst_rsn_cap = (mac_rsn_cap_stru *)(puc_buffer + uc_index);
        memset_s(pst_rsn_cap, OAL_SIZEOF(mac_rsn_cap_stru), 0, OAL_SIZEOF(mac_rsn_cap_stru));
        uc_index += MAC_RSN_CAP_LEN;

        pst_rsn_cap->bit_mfpr        = pst_mac_vap->pst_mib_info->st_wlan_mib_privacy.en_dot11RSNAMFPR;
        pst_rsn_cap->bit_mfpc        = pst_mac_vap->pst_mib_info->st_wlan_mib_privacy.en_dot11RSNAMFPC;
        pst_rsn_cap->bit_pre_auth    =
            pst_mac_vap->pst_mib_info->st_wlan_mib_privacy.en_dot11RSNAPreauthenticationActivated;
        pst_rsn_cap->bit_no_pairwise = 0;
        pst_rsn_cap->bit_ptska_relay_counter =
            pst_mac_vap->pst_mib_info->st_wlan_mib_rsna_cfg.ul_dot11RSNAConfigNumberOfPTKSAReplayCountersImplemented;
        pst_rsn_cap->bit_gtska_relay_counter =
            pst_mac_vap->pst_mib_info->st_wlan_mib_rsna_cfg.ul_dot11RSNAConfigNumberOfGTKSAReplayCountersImplemented;

        if (pst_rsn_cap->bit_mfpc == 1) {
            puc_buffer[uc_index++] = 0x00;
            puc_buffer[uc_index++] = 0x00;
            puc_buffer[uc_index++] = 0x00;
            puc_buffer[uc_index++] = 0x0f;
            puc_buffer[uc_index++] = 0xac;
            puc_buffer[uc_index++] = WLAN_80211_CIPHER_SUITE_BIP;
        }
    }

    /* 设置RSN element的长度 */
    puc_buffer[1] = uc_index - MAC_IE_HDR_LEN;

    *puc_ie_len = uc_index;
}
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

oal_uint32 mac_set_rsn_ie(oal_void *pst_mac_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    oal_uint8                            uc_index;
    oal_uint16                           us_rsn_capabilities;
    wlan_mib_dot11RSNAConfigEntry_stru  *pst_mib_rsna_cfg = OAL_PTR_NULL;
    wlan_mib_Dot11PrivacyEntry_stru     *pst_mib_privacy = OAL_PTR_NULL;
    int32_t ret;

    if ((pst_mac_vap == OAL_PTR_NULL) || (puc_buffer == OAL_PTR_NULL) || (puc_ie_len == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (((mac_vap_stru *)pst_mac_vap)->pst_mib_info == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* *********************************************************************** */
    /*                  RSN Element Format              */
    /* --------------------------------------------------------------------- */
    /* |Element ID | Length | Version | Group Cipher Suite | Pairwise Cipher */
    /* --------------------------------------------------------------------- */
    /* | 1         | 1      | 2       |      4             |     2           */
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* Suite | Pairwise Cipher Suite List | AKM Suite Count | AKM Suite List */
    /* --------------------------------------------------------------------- */
    /*       | 4*m                        |     2          | 4*n             */
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* |RSN Capabilities|PMKID Count|PMKID List|Group Management Cipher Suite */
    /* --------------------------------------------------------------------- */
    /* |    2           |    2      |16 -s     |         4                 | */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /* *********************************************************************** */

    *puc_ie_len = 0;    /* 初始化IE 长度为0 */

    pst_mib_rsna_cfg = &((mac_vap_stru *)pst_mac_vap)->pst_mib_info->st_wlan_mib_rsna_cfg;
    pst_mib_privacy  = &((mac_vap_stru *)pst_mac_vap)->pst_mib_info->st_wlan_mib_privacy;

    if (pst_mib_privacy->en_dot11RSNAActivated != OAL_TRUE) {
        return OAL_FALSE;
    }

    /* 添加 RSN 信息 */
    uc_index    = 0;
    puc_buffer[uc_index] = MAC_EID_RSN;
    uc_index += MAC_IE_HDR_LEN;

    /* 填充RSN 版本信息 */
    puc_buffer[uc_index++] = MAC_RSN_IE_VERSION;
    puc_buffer[uc_index++] = 0;

    /* 填充组播加密套件 */
    ret = memcpy_s(puc_buffer + uc_index, MAC_OUI_LEN, g_auc_rsn_oui, MAC_OUI_LEN);
    if (ret != EOK) {
        OAM_WARNING_LOG1(0, OAM_SF_TX, "mac_set_rsn_ie::memcopy_s return value %d ", ret);
        return OAL_FALSE;
    }
    uc_index += MAC_OUI_LEN;
    /* 填充组播 */
    puc_buffer[uc_index++] = pst_mib_rsna_cfg->uc_dot11RSNAConfigGroupCipher;

    /* 关联时， 只添加一个加密套件 */
    /* 填充单播加密套件信息 */
    puc_buffer[uc_index++] = 1;
    puc_buffer[uc_index++] = 0;
    ret = memcpy_s(puc_buffer + uc_index, MAC_OUI_LEN, g_auc_rsn_oui, MAC_OUI_LEN);
    if (ret != EOK) {
        OAM_WARNING_LOG1(0, OAM_SF_TX, "mac_set_rsn_ie::memcopy_s return value %d ", ret);
        return OAL_FALSE;
    }
    uc_index += MAC_OUI_LEN;
    puc_buffer[uc_index++] = pst_mib_rsna_cfg->uc_dot11RSNAPairwiseCipherRequested;

    /* 填充认证套件信息 */
    puc_buffer[uc_index++] = 1;
    puc_buffer[uc_index++] = 0;
    ret = memcpy_s(puc_buffer + uc_index, MAC_OUI_LEN, g_auc_rsn_oui, MAC_OUI_LEN);
    if (ret != EOK) {
        OAM_WARNING_LOG1(0, OAM_SF_TX, "mac_set_rsn_ie::memcopy_s return value %d ", ret);
        return OAL_FALSE;
    }
    uc_index += MAC_OUI_LEN;
    puc_buffer[uc_index++] = pst_mib_rsna_cfg->uc_dot11RSNAAuthenticationSuiteRequested;

    /* 填充RSN 能力信息 */
    /* *********************************************************************************** */
    /* --------------------------------------------------------------------------------- */
    /* | B15 - B8 |  B7  | B6  |  B5 - B4      | B3 - B2     |       B1    |     B0    | */
    /* --------------------------------------------------------------------------------- */
    /* | Reserved | MFPC |MFPR |  GTSKA Replay | PTSKA Replay| No Pairwise | Pre - Auth| */
    /* |          |      |     |    Counter    |   Counter   |             |           | */
    /* --------------------------------------------------------------------------------- */
    /*                                                                                   */
    /* *********************************************************************************** */
    us_rsn_capabilities = 0;

    /* 根据MIB 值，设置 PTSKA Replay counters.                  */
    us_rsn_capabilities |= ((pst_mib_rsna_cfg->ul_dot11RSNAConfigNumberOfPTKSAReplayCountersImplemented << 2) & 0x000C);

    /* 根据MIB 值，设置 PTSKA Replay counters.  */
    us_rsn_capabilities |= ((pst_mib_rsna_cfg->ul_dot11RSNAConfigNumberOfGTKSAReplayCountersImplemented << 4) & 0x0030);

    /* 根据MIB 值，设置 MFPR.  */
    us_rsn_capabilities |= ((pst_mib_privacy->en_dot11RSNAMFPR == OAL_TRUE) ? BIT6 : 0);

    /* 根据MIB 值，设置 MFPC.  */
    us_rsn_capabilities |= ((pst_mib_privacy->en_dot11RSNAMFPC == OAL_TRUE) ? BIT7 : 0);

    /* 根据MIB 值，设置pre auth.  */
    us_rsn_capabilities |= ((pst_mib_privacy->en_dot11RSNAPreauthenticationActivated == OAL_TRUE) ? BIT0 : 0);

    /* 设置 RSN Capabilities 信息 */
    puc_buffer[uc_index++] = us_rsn_capabilities & 0x00FF;
    puc_buffer[uc_index++] = (us_rsn_capabilities & 0xFF00) >> 8;

    if (mac_mib_get_dot11RSNAMFPC((mac_vap_stru *)pst_mac_vap) == OAL_TRUE) {
        puc_buffer[uc_index++] = 0x00;
        puc_buffer[uc_index++] = 0x00;
        puc_buffer[uc_index++] = 0x00;
        puc_buffer[uc_index++] = 0x0f;
        puc_buffer[uc_index++] = 0xac;
        puc_buffer[uc_index++] = WLAN_80211_CIPHER_SUITE_BIP;
    }

    /* 设置RSN IE 长度 */
    puc_buffer[1] = uc_index - MAC_IE_HDR_LEN;
    *puc_ie_len = uc_index;
    return OAL_TRUE;
}


oal_uint32 mac_set_wpa_ie(oal_void *pst_mac_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    oal_uint8                            uc_index;
    wlan_mib_dot11RSNAConfigEntry_stru  *pst_mib_rsna_cfg = OAL_PTR_NULL;
    wlan_mib_Dot11PrivacyEntry_stru     *pst_mib_privacy = OAL_PTR_NULL;
    int32_t ret = EOK;

    if ((pst_mac_vap == OAL_PTR_NULL) || (puc_buffer == OAL_PTR_NULL) || (puc_ie_len == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (((mac_vap_stru *)pst_mac_vap)->pst_mib_info == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* *********************************************************************** */
    /*                  WPA Element Format              */
    /* --------------------------------------------------------------------- */
    /* |Element ID | Length | Version | Group Cipher Suite | Pairwise Cipher */
    /* --------------------------------------------------------------------- */
    /* | 1         | 1      | 2       |      4             |     2           */
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* Suite | Pairwise Cipher Suite List | AKM Suite Count | AKM Suite List */
    /* --------------------------------------------------------------------- */
    /*       | 4*m                        |     2          | 4*n             */
    /* --------------------------------------------------------------------- */
    /* *********************************************************************** */

    *puc_ie_len = 0;    /* 初始化IE 长度为0 */

    pst_mib_rsna_cfg = &((mac_vap_stru *)pst_mac_vap)->pst_mib_info->st_wlan_mib_rsna_cfg;
    pst_mib_privacy  = &((mac_vap_stru *)pst_mac_vap)->pst_mib_info->st_wlan_mib_privacy;

    if (pst_mib_privacy->en_dot11RSNAActivated != OAL_TRUE) {
        return OAL_FALSE;
    }

    /* 添加 WPA 信息和 WPA OUI 信息 */
    uc_index = 0;
    puc_buffer[uc_index] = MAC_EID_WPA;
    uc_index += MAC_IE_HDR_LEN;
    ret += memcpy_s(puc_buffer + uc_index, MAC_OUI_LEN, g_auc_wpa_oui, MAC_OUI_LEN);
    uc_index += MAC_OUI_LEN;
    puc_buffer[uc_index++] = MAC_OUITYPE_WPA;

    /* 填充WPA 版本信息 */
    puc_buffer[uc_index++] = MAC_RSN_IE_VERSION;
    puc_buffer[uc_index++] = 0;

    /* 填充组播加密套件 */
    ret += memcpy_s(puc_buffer + uc_index, MAC_OUI_LEN, g_auc_wpa_oui, MAC_OUI_LEN);
    uc_index += MAC_OUI_LEN;
    /* 填充组播 */
    puc_buffer[uc_index++] = pst_mib_rsna_cfg->uc_dot11RSNAConfigGroupCipher;

    /* 关联时， 只添加一个加密套件 */
    /* 填充单播加密套件信息 */
    puc_buffer[uc_index++] = 1;
    puc_buffer[uc_index++] = 0;
    ret += memcpy_s(puc_buffer + uc_index, MAC_OUI_LEN, g_auc_wpa_oui, MAC_OUI_LEN);
    uc_index += MAC_OUI_LEN;
    puc_buffer[uc_index++] = pst_mib_rsna_cfg->uc_dot11RSNAPairwiseCipherRequested;

    /* 填充认证套件信息 */
    puc_buffer[uc_index++] = 1;
    puc_buffer[uc_index++] = 0;
    ret += memcpy_s(puc_buffer + uc_index, MAC_OUI_LEN, g_auc_wpa_oui, MAC_OUI_LEN);
    uc_index += MAC_OUI_LEN;
    puc_buffer[uc_index++] = pst_mib_rsna_cfg->uc_dot11RSNAAuthenticationSuiteRequested;
    if (ret != EOK) {
        OAM_WARNING_LOG1(0, OAM_SF_TX, "mac_set_rsn_ie::memcopy_s return value %d ", ret);
        return OAL_FALSE;
    }
    /* 设置RSN IE 长度 */
    puc_buffer[1] = uc_index - MAC_IE_HDR_LEN;
    *puc_ie_len = uc_index;

    return OAL_TRUE;
}
#endif // #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)


oal_void  mac_set_security_ie_authenticator(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len,
                                            oal_uint8 uc_mode)
{
    mac_vap_stru *pst_mac_vap        = (mac_vap_stru *)pst_vap;

    *puc_ie_len = 0;

    if ((uc_mode == DMAC_WPA_802_11I) && (pst_mac_vap->st_cap_flag.bit_wpa != OAL_TRUE)) {
        return;
    }

    if ((uc_mode == DMAC_RSNA_802_11I) && (pst_mac_vap->st_cap_flag.bit_wpa2 != OAL_TRUE)) {
        return;
    }

    mac_set_rsn_ie_authenticator(pst_vap, puc_buffer, uc_mode, puc_ie_len);

    return;
}

#ifdef _PRE_WLAN_FEATURE_STA_UAPSD

oal_uint8 mac_get_uapsd_config_max_sp_len(mac_vap_stru *pst_mac_vap)
{
    return pst_mac_vap->st_sta_uapsd_cfg.uc_max_sp_len;
}


oal_uint8 mac_get_uapsd_config_ac(mac_vap_stru *pst_mac_vap, oal_uint8 uc_ac)
{
    if (uc_ac < WLAN_WME_AC_BUTT) {
        return pst_mac_vap->st_sta_uapsd_cfg.uc_trigger_enabled[uc_ac];
    }

    return 0;
}


oal_void  mac_set_qos_info_wmm_sta(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_buffer)
{
    oal_uint8                   uc_qos_info = 0;
    oal_uint8                   uc_max_sp_bits;
    oal_uint8                   uc_max_sp_length;

    /* QoS Information field                                          */
    /* -------------------------------------------------------------- */
    /* | B0    | B1    | B2    | B3    | B4      | B5:B6 | B7       | */
    /* -------------------------------------------------------------- */
    /* | AC_VO | AC_VI | AC_BK | AC_BE |         | Max SP|          | */
    /* | U-APSD| U-APSD| U-APSD| U-APSD| Reserved| Length| Reserved | */
    /* | Flag  | Flag  | Flag  | Flag  |         |       |          | */
    /* -------------------------------------------------------------- */

    /* Set the UAPSD configuration information in the QoS info field if the  */
    /* BSS type is Infrastructure and the AP supports UAPSD.                 */
    if (pst_mac_vap->uc_uapsd_cap == OAL_TRUE) {
        uc_max_sp_length = mac_get_uapsd_config_max_sp_len(pst_mac_vap);
        /*lint -e734*/
        uc_qos_info |= (mac_get_uapsd_config_ac(pst_mac_vap, WLAN_WME_AC_VO) << 0);
        uc_qos_info |= (mac_get_uapsd_config_ac(pst_mac_vap, WLAN_WME_AC_VI) << 1);
        uc_qos_info |= (mac_get_uapsd_config_ac(pst_mac_vap, WLAN_WME_AC_BK) << 2);
        uc_qos_info |= (mac_get_uapsd_config_ac(pst_mac_vap, WLAN_WME_AC_BE) << 3);
        /*lint +e734*/
        if (uc_max_sp_length <= 6) {
            uc_max_sp_bits = uc_max_sp_length >> 1;

            uc_qos_info |= ((uc_max_sp_bits & 0x03) << 5);
        }
    }

    puc_buffer[0] = uc_qos_info;
}
#endif


OAL_STATIC oal_void  mac_set_qos_info_field(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_buffer)
{
    mac_qos_info_stru *pst_qos_info = (mac_qos_info_stru *)puc_buffer;

    /* QoS Information field  (AP MODE)            */
    /* ------------------------------------------- */
    /* | B0:B3               | B4:B6    | B7     | */
    /* ------------------------------------------- */
    /* | Parameter Set Count | Reserved | U-APSD | */

    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        pst_qos_info->bit_params_count = pst_mac_vap->uc_wmm_params_update_count;
        pst_qos_info->bit_uapsd        = pst_mac_vap->st_cap_flag.bit_uapsd;
        pst_qos_info->bit_resv         = 0;
    }

    /* QoS Information field  (STA MODE)           */
    /* ---------------------------------------------------------------------------------------------------------- */
    /* | B0              | B1              | B2              | B3              | B4      |B5   B6      | B7     | */
    /* ---------------------------------------------------------------------------------------------------------- */
    /* |AC_VO U-APSD Flag|AC_VI U-APSD Flag|AC_BK U-APSD Flag|AC_BE U-APSD Flag|Reserved |Max SP Length|Reserved| */
    /* ---------------------------------------------------------------------------------------------------------- */
    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
#ifdef _PRE_WLAN_FEATURE_STA_UAPSD
        mac_set_qos_info_wmm_sta(pst_mac_vap, puc_buffer);
#else
        puc_buffer[0] = 0;
        puc_buffer[0] |= 0x0;
#endif
    }
}


OAL_STATIC oal_void  mac_set_wmm_ac_params(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_buffer,
                                           wlan_wme_ac_type_enum_uint8 en_ac)
{
    mac_wmm_ac_params_stru *pst_ac_params = (mac_wmm_ac_params_stru *)puc_buffer;

    /* AC_** Parameter Record field               */
    /* ------------------------------------------ */
    /* | Byte 1    | Byte 2        | Byte 3:4   | */
    /* ------------------------------------------ */
    /* | ACI/AIFSN | ECWmin/ECWmax | TXOP Limit | */
    /* ------------------------------------------ */

    /* ACI/AIFSN Field                    */
    /* ---------------------------------- */
    /* | B0:B3 | B4  | B5:B6 | B7       | */
    /* ---------------------------------- */
    /* | AIFSN | ACM | ACI   | Reserved | */
    /* ---------------------------------- */
    /* AIFSN */
    pst_ac_params->bit_aifsn = pst_mac_vap->pst_mib_info->ast_wlan_mib_edca[en_ac].ul_dot11EDCATableAIFSN;

    /* ACM */
    pst_ac_params->bit_acm = pst_mac_vap->pst_mib_info->ast_wlan_mib_edca[en_ac].en_dot11EDCATableMandatory;

    /* ACI */
    pst_ac_params->bit_aci = pst_mac_vap->pst_mib_info->ast_wlan_mib_edca[en_ac].ul_dot11EDCATableIndex - 1;

    pst_ac_params->bit_resv = 0;

    /* ECWmin/ECWmax Field */
    /* ------------------- */
    /* | B0:B3  | B4:B7  | */
    /* ------------------- */
    /* | ECWmin | ECWmax | */
    /* ------------------- */
    /* ECWmin */
    pst_ac_params->bit_ecwmin = pst_mac_vap->pst_mib_info->ast_wlan_mib_edca[en_ac].ul_dot11EDCATableCWmin;

    /* ECWmax */
    pst_ac_params->bit_ecwmax = pst_mac_vap->pst_mib_info->ast_wlan_mib_edca[en_ac].ul_dot11EDCATableCWmax;

    /* TXOP Limit. The value saved in MIB is in usec while the value to be   */
    /* set in this element should be in multiple of 32us                     */
    pst_ac_params->us_txop =
        (oal_uint16)((pst_mac_vap->pst_mib_info->ast_wlan_mib_edca[en_ac].ul_dot11EDCATableTXOPLimit) >> 5);
}


oal_void  mac_set_wmm_params_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_bool_enum_uint8 en_is_qos,
                                oal_uint8 *puc_ie_len)
{
    oal_uint8     uc_index;
    mac_vap_stru *pst_mac_vap = (mac_vap_stru *)pst_vap;
    int32_t ret;

    if (en_is_qos == OAL_FALSE) {
        *puc_ie_len = 0;

        return;
    }

    /* WMM Parameter Element Format                                          */
    /* --------------------------------------------------------------------- */
    /* | 3Byte | 1        | 1           | 1             | 1        | 1     | */
    /* --------------------------------------------------------------------- */
    /* | OUI   | OUI Type | OUI Subtype | Version field | QoS Info | Resvd | */
    /* --------------------------------------------------------------------- */
    /* | 4              | 4              | 4              | 4              | */
    /* --------------------------------------------------------------------- */
    /* | AC_BE ParamRec | AC_BK ParamRec | AC_VI ParamRec | AC_VO ParamRec | */
    /* --------------------------------------------------------------------- */
    puc_buffer[0] = MAC_EID_WMM;
    puc_buffer[1] = MAC_WMM_PARAM_LEN;

    uc_index = MAC_IE_HDR_LEN;

    /* OUI */
    ret = memcpy_s(&puc_buffer[uc_index], MAC_OUI_LEN, g_auc_wmm_oui, MAC_OUI_LEN);
    if (ret != EOK) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TX, "mac_set_wmm_params_ie::memcopy_s return value %d ", ret);
        return;
    }
    uc_index += MAC_OUI_LEN;

    /* OUI Type */
    puc_buffer[uc_index++] = MAC_OUITYPE_WMM;

    /* OUI Subtype */
    puc_buffer[uc_index++] = MAC_OUISUBTYPE_WMM_PARAM;

    /* Version field */
    puc_buffer[uc_index++] = MAC_OUI_WMM_VERSION;

    /* QoS Information Field */
    mac_set_qos_info_field(pst_mac_vap, &puc_buffer[uc_index]);
    uc_index += MAC_QOS_INFO_LEN;

    /* Reserved */
    puc_buffer[uc_index++] = 0;

    /* Set the AC_BE, AC_BK, AC_VI, AC_VO Parameter Record fields */
    mac_set_wmm_ac_params(pst_mac_vap, &puc_buffer[uc_index], WLAN_WME_AC_BE);
    uc_index += MAC_AC_PARAM_LEN;

    mac_set_wmm_ac_params(pst_mac_vap, &puc_buffer[uc_index], WLAN_WME_AC_BK);
    uc_index += MAC_AC_PARAM_LEN;

    mac_set_wmm_ac_params(pst_mac_vap, &puc_buffer[uc_index], WLAN_WME_AC_VI);
    uc_index += MAC_AC_PARAM_LEN;

    mac_set_wmm_ac_params(pst_mac_vap, &puc_buffer[uc_index], WLAN_WME_AC_VO);

    *puc_ie_len = MAC_IE_HDR_LEN + MAC_WMM_PARAM_LEN;
}


oal_void mac_set_exsup_rates_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    mac_vap_stru     *pst_mac_vap   = (mac_vap_stru *)pst_vap;
    mac_rateset_stru *pst_rates_set;
    oal_uint8         uc_nrates;
    oal_uint8         uc_idx;

    pst_rates_set = &(pst_mac_vap->st_curr_sup_rates.st_rate);

    /* STA全信道扫描时根据频段设置supported rates */
    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA && pst_mac_vap->en_protocol == WLAN_VHT_MODE) {
        pst_rates_set = &(pst_mac_vap->ast_sta_sup_rates_ie[pst_mac_vap->st_channel.en_band].st_rate);
    }

    /***************************************************************************
                   -----------------------------------------------
                   |ElementID | Length | Extended Supported Rates|
                   -----------------------------------------------
       Octets:     |1         | 1      | 1-255                   |
                   -----------------------------------------------
    ***************************************************************************/
    if (pst_rates_set->uc_rs_nrates <= MAC_MAX_SUPRATES) {
        *puc_ie_len = 0;

        return;
    }

    puc_buffer[0] = MAC_EID_XRATES;
    uc_nrates     = pst_rates_set->uc_rs_nrates - MAC_MAX_SUPRATES;
    puc_buffer[1] = uc_nrates;

    for (uc_idx = 0; uc_idx < uc_nrates; uc_idx++) {
        puc_buffer[MAC_IE_HDR_LEN + uc_idx] = pst_rates_set->ast_rs_rates[uc_idx + MAC_MAX_SUPRATES].uc_mac_rate;
    }

    *puc_ie_len = MAC_IE_HDR_LEN + uc_nrates;
}


oal_void mac_set_bssload_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    mac_bss_load_stru  *pst_bss_load = OAL_PTR_NULL;
    mac_vap_stru       *pst_mac_vap  = (mac_vap_stru *)pst_vap;

    if (pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11QosOptionImplemented == OAL_FALSE ||
        pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11QBSSLoadImplemented == OAL_FALSE) {
        *puc_ie_len = 0;

        return;
    }

    /***************************************************************************
    ------------------------------------------------------------------------
    |EID |Len |StationCount |ChannelUtilization |AvailableAdmissionCapacity|
    ------------------------------------------------------------------------
    |1   |1   |2            |1                  |2                         |
    ------------------------------------------------------------------------
    ***************************************************************************/
    puc_buffer[0] = MAC_EID_QBSS_LOAD;
    puc_buffer[1] = MAC_BSS_LOAD_IE_LEN;

    pst_bss_load = (mac_bss_load_stru *)(puc_buffer + MAC_IE_HDR_LEN);

    pst_bss_load->us_sta_count = OAL_BYTEORDER_TO_LE16(pst_mac_vap->us_user_nums);

    pst_bss_load->uc_chan_utilization = pst_mac_vap->uc_channel_utilization;

    pst_bss_load->us_aac = 0;

    *puc_ie_len = MAC_IE_HDR_LEN + MAC_BSS_LOAD_IE_LEN;
}

#ifdef _PRE_WLAN_FEATURE_SMPS

OAL_STATIC oal_uint8 mac_calc_smps_field(oal_void *pst_vap)
{
    mac_vap_stru *pst_mac_vap = (mac_vap_stru *)pst_vap;

    wlan_mib_mimo_power_save_enum_uint8   en_smps;

    en_smps = (wlan_mib_mimo_power_save_enum_uint8)pst_mac_vap->st_cap_flag.bit_smps;

    if (en_smps == WLAN_MIB_MIMO_POWER_SAVE_STATIC) {
        return MAC_SMPS_STATIC_MODE;
    } else if (en_smps == WLAN_MIB_MIMO_POWER_SAVE_DYNAMIC) {
        return MAC_SMPS_DYNAMIC_MODE;
    }

    return MAC_SMPS_MIMO_MODE;
}
#endif


OAL_STATIC oal_void mac_set_ht_capinfo_field(oal_void *pst_vap, oal_uint8 *puc_buffer)
{
    mac_vap_stru *pst_mac_vap = (mac_vap_stru *)pst_vap;
    mac_frame_ht_cap_stru *pst_ht_capinfo = (mac_frame_ht_cap_stru *)puc_buffer;

    /*********************** HT Capabilities Info field*************************
    ----------------------------------------------------------------------------
     |-------------------------------------------------------------------|
     | LDPC   | Supp    | SM    | Green- | Short  | Short  |  Tx  |  Rx  |
     | Coding | Channel | Power | field  | GI for | GI for | STBC | STBC |
     | Cap    | Wth Set | Save  |        | 20 MHz | 40 MHz |      |      |
     |-------------------------------------------------------------------|
     |   B0   |    B1   |B2   B3|   B4   |   B5   |    B6  |  B7  |B8  B9|
     |-------------------------------------------------------------------|

     |-------------------------------------------------------------------|
     |    HT     |  Max   | DSS/CCK | Reserved | 40 MHz     | L-SIG TXOP |
     |  Delayed  | AMSDU  | Mode in |          | Intolerant | Protection |
     | Block-Ack | Length | 40MHz   |          |            | Support    |
     |-------------------------------------------------------------------|
     |    B10    |   B11  |   B12   |   B13    |    B14     |    B15     |
     |-------------------------------------------------------------------|
    ***************************************************************************/
    /* 初始清0 */
    puc_buffer[0] = 0;
    puc_buffer[1] = 0;

    pst_ht_capinfo->bit_ldpc_coding_cap = pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11LDPCCodingOptionImplemented;

    /* 设置所支持的信道宽度集"，0:仅20MHz运行; 1:20MHz与40MHz运行 */
    pst_ht_capinfo->bit_supported_channel_width = mac_mib_get_FortyMHzOperationImplemented(pst_mac_vap);
#ifdef _PRE_WLAN_FEATURE_SMPS
    pst_ht_capinfo->bit_sm_power_save = mac_calc_smps_field(pst_mac_vap);
#else
    pst_ht_capinfo->bit_sm_power_save = MAC_SMPS_MIMO_MODE;
#endif

    pst_ht_capinfo->bit_ht_green_field = pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11HTGreenfieldOptionImplemented;

    pst_ht_capinfo->bit_short_gi_20mhz = pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11ShortGIOptionInTwentyImplemented;

    pst_ht_capinfo->bit_short_gi_40mhz = mac_mib_get_ShortGIOptionInFortyImplemented(pst_mac_vap);

    pst_ht_capinfo->bit_tx_stbc = pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11TxSTBCOptionImplemented;

    pst_ht_capinfo->bit_rx_stbc =
        (pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11RxSTBCOptionImplemented == OAL_TRUE) ? 1 : 0;

    pst_ht_capinfo->bit_ht_delayed_block_ack =
        pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11DelayedBlockAckOptionImplemented;

    pst_ht_capinfo->bit_max_amsdu_length = pst_mac_vap->pst_mib_info->st_wlan_mib_ht_sta_cfg.en_dot11MaxAMSDULength;

    /* 是否在具有40MHz能力，而运行于20/40MHz模式的BSS上使用DSSS/CCK */
    if (pst_mac_vap->st_channel.en_band == WLAN_BAND_2G) {
        if ((pst_mac_vap->en_protocol == WLAN_LEGACY_11B_MODE) ||
            (pst_mac_vap->en_protocol == WLAN_MIXED_ONE_11G_MODE) ||
            (pst_mac_vap->en_protocol == WLAN_MIXED_TWO_11G_MODE) ||
            (pst_mac_vap->en_protocol == WLAN_HT_MODE)) {
            pst_ht_capinfo->bit_dsss_cck_mode_40mhz = pst_mac_vap->st_cap_flag.bit_dsss_cck_mode_40mhz;
        } else {
            pst_ht_capinfo->bit_dsss_cck_mode_40mhz = 0;
        }
    } else {
        pst_ht_capinfo->bit_dsss_cck_mode_40mhz = 0;
    }

    /* 设置"40MHz不容许"，只在2.4GHz下有效 */
    if (pst_mac_vap->st_channel.en_band == WLAN_BAND_2G) {
        pst_ht_capinfo->bit_forty_mhz_intolerant = mac_mib_get_FortyMHzIntolerant(pst_mac_vap);
    }

    pst_ht_capinfo->bit_lsig_txop_protection =
        pst_mac_vap->pst_mib_info->st_wlan_mib_ht_sta_cfg.en_dot11LsigTxopProtectionOptionImplemented;
}


OAL_STATIC oal_void mac_set_ampdu_params_field(oal_void *pst_vap, oal_uint8 *puc_buffer)
{
    mac_vap_stru *pst_mac_vap                = (mac_vap_stru *)pst_vap;
    mac_ampdu_params_stru *pst_ampdu_params  = (mac_ampdu_params_stru *)puc_buffer;

    /******************** AMPDU Parameters Field ******************************
     |-----------------------------------------------------------------------|
     | Maximum AMPDU Length Exponent | Minimum MPDU Start Spacing | Reserved |
     |-----------------------------------------------------------------------|
     | B0                         B1 | B2                      B4 | B5     B7|
     |-----------------------------------------------------------------------|
    **************************************************************************/
    /* 初始清0 */
    puc_buffer[0] = 0;

    pst_ampdu_params->bit_max_ampdu_len_exponent =
        pst_mac_vap->pst_mib_info->st_wlan_mib_ht_sta_cfg.ul_dot11MaxRxAMPDUFactor;

    pst_ampdu_params->bit_min_mpdu_start_spacing =
        pst_mac_vap->pst_mib_info->st_wlan_mib_ht_sta_cfg.ul_dot11MinimumMPDUStartSpacing;
}


OAL_STATIC oal_void mac_set_sup_mcs_set_field(oal_void *pst_vap, oal_uint8 *puc_buffer)
{
    mac_vap_stru *pst_mac_vap              = (mac_vap_stru *)pst_vap;
    mac_sup_mcs_set_stru *pst_sup_mcs_set  = (mac_sup_mcs_set_stru *)puc_buffer;
    int32_t ret;

    /************************* Supported MCS Set Field **********************
    |-------------------------------------------------------------------|
    | Rx MCS Bitmask | Reserved | Rx Highest    | Reserved |  Tx MCS    |
    |                |          | Supp Data Rate|          |Set Defined |
    |-------------------------------------------------------------------|
    | B0         B76 | B77  B79 | B80       B89 | B90  B95 |    B96     |
    |-------------------------------------------------------------------|
    | Tx Rx MCS Set  | Tx Max Number     |   Tx Unequal     | Reserved  |
    |  Not Equal     | Spat Stream Supp  | Modulation Supp  |           |
    |-------------------------------------------------------------------|
    |      B97       | B98           B99 |       B100       | B101 B127 |
    |-------------------------------------------------------------------|
    *************************************************************************/

    /* 初始清零 */
    memset_s(puc_buffer, OAL_SIZEOF(mac_sup_mcs_set_stru), 0, OAL_SIZEOF(mac_sup_mcs_set_stru));

    ret = memcpy_s(pst_sup_mcs_set->auc_rx_mcs, sizeof(pst_sup_mcs_set->auc_rx_mcs),
        pst_mac_vap->pst_mib_info->st_supported_mcsrx.auc_dot11SupportedMCSRxValue,
        WLAN_HT_MCS_BITMASK_LEN);
    if (ret != EOK) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TX, "mac_set_sup_mcs_set_field:memcopy_s return value %d", ret);
        return;
    }

    pst_sup_mcs_set->bit_rx_highest_rate = pst_mac_vap->pst_mib_info->st_phy_ht.ul_dot11HighestSupportedDataRate;

    if (pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11TxMCSSetDefined == OAL_TRUE) {
        pst_sup_mcs_set->bit_tx_mcs_set_def = 1;

        if (pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11TxRxMCSSetNotEqual == OAL_TRUE) {
            pst_sup_mcs_set->bit_tx_rx_not_equal = 1;

            pst_sup_mcs_set->bit_tx_max_stream =
                pst_mac_vap->pst_mib_info->st_phy_ht.ul_dot11TxMaximumNumberSpatialStreamsSupported;

            if (pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11TxUnequalModulationSupported == OAL_TRUE) {
                pst_sup_mcs_set->bit_tx_unequal_modu = 1;
            }
        }
    }

    /* reserve位清0 */
    pst_sup_mcs_set->bit_resv1 = 0;
    pst_sup_mcs_set->bit_resv2 = 0;
}


OAL_STATIC oal_void mac_set_ht_extcap_field(oal_void *pst_vap, oal_uint8 *puc_buffer)
{
    mac_vap_stru *pst_mac_vap      = (mac_vap_stru *)pst_vap;
    mac_ext_cap_stru *pst_ext_cap  = (mac_ext_cap_stru *)puc_buffer;

    /***************** HT Extended Capabilities Field **********************
      |-----------------------------------------------------------------|
      | PCO | PCO Trans | Reserved | MCS  |  +HTC   |  RD    | Reserved |
      |     |   Time    |          | Fdbk | Support | Resp   |          |
      |-----------------------------------------------------------------|
      | B0  | B1     B2 | B3    B7 | B8 B9|   B10   |  B11   | B12  B15 |
      |-----------------------------------------------------------------|
    ***********************************************************************/
    /* 初始清0 */
    puc_buffer[0] = 0;
    puc_buffer[1] = 0;

    if (pst_mac_vap->pst_mib_info->st_wlan_mib_ht_sta_cfg.en_dot11PCOOptionImplemented == OAL_TRUE) {
        pst_ext_cap->bit_pco = 1;

        pst_ext_cap->bit_pco_trans_time = pst_mac_vap->pst_mib_info->st_wlan_mib_ht_sta_cfg.ul_dot11TransitionTime;
    }

    pst_ext_cap->bit_mcs_fdbk = pst_mac_vap->pst_mib_info->st_wlan_mib_ht_sta_cfg.en_dot11MCSFeedbackOptionImplemented;

    pst_ext_cap->bit_htc_sup = pst_mac_vap->pst_mib_info->st_wlan_mib_ht_sta_cfg.en_dot11HTControlFieldSupported;

    pst_ext_cap->bit_rd_resp = pst_mac_vap->pst_mib_info->st_wlan_mib_ht_sta_cfg.en_dot11RDResponderOptionImplemented;
}


OAL_STATIC oal_void mac_set_txbf_cap_field(oal_void *pst_vap, oal_uint8 *puc_buffer)
{
    mac_vap_stru *pst_mac_vap        = (mac_vap_stru *)pst_vap;
    mac_txbf_cap_stru *pst_txbf_cap  = (mac_txbf_cap_stru *)puc_buffer;

    /*************** Transmit Beamforming Capability Field *********************
     |-------------------------------------------------------------------------|
     |   Implicit | Rx Stagg | Tx Stagg  | Rx NDP   | Tx NDP   | Implicit      |
     |   TxBF Rx  | Sounding | Sounding  | Capable  | Capable  | TxBF          |
     |   Capable  | Capable  | Capable   |          |          | Capable       |
     |-------------------------------------------------------------------------|
     |      B0    |     B1   |    B2     |   B3     |   B4     |    B5         |
     |-------------------------------------------------------------------------|
     |              | Explicit | Explicit Non- | Explicit      | Explicit      |
     |  Calibration | CSI TxBF | Compr Steering| Compr steering| TxBF CSI      |
     |              | Capable  | Cap.          | Cap.          | Feedback      |
     |-------------------------------------------------------------------------|
     |  B6       B7 |   B8     |       B9      |       B10     | B11  B12      |
     |-------------------------------------------------------------------------|
     | Explicit Non- | Explicit | Minimal  | CSI Num of | Non-Compr Steering   |
     | Compr BF      | Compr BF | Grouping | Beamformer | Num of Beamformer    |
     | Fdbk Cap.     | Fdbk Cap.|          | Ants Supp  | Ants Supp            |
     |-------------------------------------------------------------------------|
     | B13       B14 | B15  B16 | B17  B18 | B19    B20 | B21        B22       |
     |-------------------------------------------------------------------------|
     | Compr Steering    | CSI Max Num of     |   Channel     |                |
     | Num of Beamformer | Rows Beamformer    | Estimation    | Reserved       |
     | Ants Supp         | Supported          | Capability    |                |
     |-------------------------------------------------------------------------|
     | B23           B24 | B25            B26 | B27       B28 | B29  B31       |
     |-------------------------------------------------------------------------|
    ***************************************************************************/
    /* 初始清零 */
    puc_buffer[0] = 0;
    puc_buffer[1] = 0;
    puc_buffer[2] = 0;
    puc_buffer[3] = 0;

    /* 指示STA是否可以接收staggered sounding帧 */
    pst_txbf_cap->bit_rx_stagg_sounding =
        pst_mac_vap->pst_mib_info->st_wlan_mib_txbf_config.en_dot11ReceiveStaggerSoundingOptionImplemented;

    /* 指示STA是否可以发送staggered sounding帧. */
    pst_txbf_cap->bit_tx_stagg_sounding =
        pst_mac_vap->pst_mib_info->st_wlan_mib_txbf_config.en_dot11TransmitStaggerSoundingOptionImplemented;

    pst_txbf_cap->bit_rx_ndp = pst_mac_vap->pst_mib_info->st_wlan_mib_txbf_config.en_dot11ReceiveNDPOptionImplemented;

    pst_txbf_cap->bit_tx_ndp = pst_mac_vap->pst_mib_info->st_wlan_mib_txbf_config.en_dot11TransmitNDPOptionImplemented;

    pst_txbf_cap->bit_implicit_txbf =
        pst_mac_vap->pst_mib_info->st_wlan_mib_txbf_config.en_dot11ImplicitTransmitBeamformingOptionImplemented;

    pst_txbf_cap->bit_calibration =
        pst_mac_vap->pst_mib_info->st_wlan_mib_txbf_config.uc_dot11CalibrationOptionImplemented;

    pst_txbf_cap->bit_explicit_csi_txbf =
        pst_mac_vap->pst_mib_info->st_wlan_mib_txbf_config.en_dot11ExplicitCSITransmitBeamformingOptionImplemented;

    pst_txbf_cap->bit_explicit_noncompr_steering = pst_mac_vap->pst_mib_info->st_wlan_mib_txbf_config.en_dot11ExplicitNonCompressedBeamformingMatrixOptionImplemented;

    /* Indicates if this STA can apply transmit beamforming using compressed */
    /* beamforming feedback matrix explicit feedback in its tranmission.     */
    /* *********************************************************************** */
    /* *********************************************************************** */
    /* No MIB exists, not clear what needs to be set    B10                  */
    /* *********************************************************************** */
    /* *********************************************************************** */

    /* Indicates if this receiver can return CSI explicit feedback */
    pst_txbf_cap->bit_explicit_txbf_csi_fdbk = pst_mac_vap->pst_mib_info->st_wlan_mib_txbf_config.uc_dot11ExplicitTransmitBeamformingCSIFeedbackOptionImplemented;

    /* Indicates if this receiver can return non-compressed beamforming      */
    /* feedback matrix explicit feedback.                                    */
    pst_txbf_cap->bit_explicit_noncompr_bf_fdbk = pst_mac_vap->pst_mib_info->st_wlan_mib_txbf_config.uc_dot11ExplicitNonCompressedBeamformingFeedbackOptionImplemented;

    /* Indicates if this STA can apply transmit beamforming using explicit   */
    /* compressed beamforming feedback matrix.                               */
    pst_txbf_cap->bit_explicit_compr_bf_fdbk = pst_mac_vap->pst_mib_info->st_wlan_mib_txbf_config.uc_dot11ExplicitCompressedBeamformingFeedbackOptionImplemented;

    /* Indicates the minimal grouping used for explicit feedback reports */
    /* *********************************************************************** */
    /* *********************************************************************** */
    /*  No MIB exists, not clear what needs to be set       B17              */
    /* *********************************************************************** */
    /* *********************************************************************** */
    /* Indicates the maximum number of beamformer antennas the beamformee    */
    /* can support when CSI feedback is required.                            */
    pst_txbf_cap->bit_csi_num_bf_antssup =
        pst_mac_vap->pst_mib_info->st_wlan_mib_txbf_config.ul_dot11NumberBeamFormingCSISupportAntenna;

    /* Indicates the maximum number of beamformer antennas the beamformee    */
    /* can support when non-compressed beamforming feedback matrix is        */
    /* required                                                              */
    pst_txbf_cap->bit_noncompr_steering_num_bf_antssup = pst_mac_vap->pst_mib_info->st_wlan_mib_txbf_config.ul_dot11NumberNonCompressedBeamformingMatrixSupportAntenna;

    /* Indicates the maximum number of beamformer antennas the beamformee   */
    /* can support when compressed beamforming feedback matrix is required  */
    pst_txbf_cap->bit_compr_steering_num_bf_antssup =
        pst_mac_vap->pst_mib_info->st_wlan_mib_txbf_config.ul_dot11NumberCompressedBeamformingMatrixSupportAntenna;

    /* Indicates the maximum number of rows of CSI explicit feedback from    */
    /* beamformee that the beamformer can support when CSI feedback is       */
    /* required                                                              */
    /* *********************************************************************** */
    /* *********************************************************************** */
    /*  No MIB exists, not clear what needs to be set     B25                */
    /* *********************************************************************** */
    /* *********************************************************************** */

    /* Indicates maximum number of space time streams (columns of the MIMO   */
    /* channel matrix) for which channel dimensions can be simultaneously    */
    /* estimated. When staggered sounding is supported this limit applies    */
    /* independently to both the data portion and to the extension portion   */
    /* of the long training fields.                                          */
    /* *********************************************************************** */
    /* *********************************************************************** */
    /*      No MIB exists, not clear what needs to be set          B27       */
    /* *********************************************************************** */
    /* *********************************************************************** */
#ifdef _PRE_WLAN_FEATURE_TXBF
    pst_txbf_cap->bit_explicit_compr_Steering = pst_mac_vap->st_txbf_add_cap.bit_exp_comp_txbf_cap;
    pst_txbf_cap->bit_chan_estimation = pst_mac_vap->st_txbf_add_cap.bit_channel_est_cap;
    pst_txbf_cap->bit_minimal_grouping = pst_mac_vap->st_txbf_add_cap.bit_min_grouping;
    pst_txbf_cap->bit_csi_maxnum_rows_bf_sup = pst_mac_vap->st_txbf_add_cap.bit_csi_bfee_max_rows;
    pst_txbf_cap->bit_implicit_txbf_rx = pst_mac_vap->st_txbf_add_cap.bit_imbf_receive_cap;
#endif
}


OAL_STATIC oal_void mac_set_asel_cap_field(oal_void *pst_vap, oal_uint8 *puc_buffer)
{
    mac_vap_stru *pst_mac_vap        = (mac_vap_stru *)pst_vap;
    mac_asel_cap_stru *pst_asel_cap  = (mac_asel_cap_stru *)puc_buffer;

    /************** Antenna Selection Capability Field *************************
     |-------------------------------------------------------------------|
     |  Antenna  | Explicit CSI  | Antenna Indices | Explicit | Antenna  |
     | Selection | Fdbk based TX | Fdbk based TX   | CSI Fdbk | Indices  |
     |  Capable  | ASEL Capable  | ASEL Capable    | Capable  | Fdbk Cap.|
     |-------------------------------------------------------------------|
     |    B0     |     B1        |      B2         |    B3    |    B4    |
     |-------------------------------------------------------------------|

     |------------------------------------|
     |  RX ASEL |   Transmit   |          |
     |  Capable |   Sounding   | Reserved |
     |          | PPDU Capable |          |
     |------------------------------------|
     |    B5    |     B6       |    B7    |
     |------------------------------------|
    ***************************************************************************/
    /* 初始清0 */
    puc_buffer[0] = 0;

    /* 指示STA是否支持天线选择 */
    pst_asel_cap->bit_asel =
        pst_mac_vap->pst_mib_info->st_wlan_mib_phy_antenna.en_dot11AntennaSelectionOptionImplemented;

    /* 指示STA是否具有基于显示CSI(信道状态信息)反馈的发射天线选择能力 */
    pst_asel_cap->bit_explicit_sci_fdbk_tx_asel =
        pst_mac_vap->pst_mib_info->st_wlan_mib_phy_antenna.en_dot11TransmitExplicitCSIFeedbackASOptionImplemented;

    /* 指示STA是否具有基于天线指数反馈的发射天线选择能力 */
    pst_asel_cap->bit_antenna_indices_fdbk_tx_asel =
        pst_mac_vap->pst_mib_info->st_wlan_mib_phy_antenna.en_dot11TransmitIndicesFeedbackASOptionImplemented;

    /* 指示STA在天线选择的支持下是否能够计算CSI(信道状态信息)并提供CSI反馈 */
    pst_asel_cap->bit_explicit_csi_fdbk =
        pst_mac_vap->pst_mib_info->st_wlan_mib_phy_antenna.en_dot11ExplicitCSIFeedbackASOptionImplemented;

    /* Indicates whether or not this STA can conduct antenna indices */
    /* selection computation and feedback the results in support of  */
    /* Antenna Selection. */
    pst_asel_cap->bit_antenna_indices_fdbk =
        pst_mac_vap->pst_mib_info->st_wlan_mib_phy_antenna.en_dot11TransmitExplicitCSIFeedbackASOptionImplemented;

    /* 指示STA是否具有接收天线选择能力 */
    pst_asel_cap->bit_rx_asel =
        pst_mac_vap->pst_mib_info->st_wlan_mib_phy_antenna.en_dot11ReceiveAntennaSelectionOptionImplemented;

    /* 指示STA是否能够在每一次请求中都可以为天线选择序列发送探测PPDU */
    pst_asel_cap->bit_trans_sounding_ppdu =
        pst_mac_vap->pst_mib_info->st_wlan_mib_phy_antenna.en_dot11TransmitSoundingPPDUOptionImplemented;
}

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

oal_void mac_set_timeout_interval_ie(oal_void *pst_vap,
                                     oal_uint8 *puc_buffer,
                                     oal_uint8 *puc_ie_len,
                                     oal_uint32 ul_type,
                                     oal_uint32 ul_timeout)
{
#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
    mac_Timeout_Interval_type_enum en_tie_type;

    en_tie_type = (mac_Timeout_Interval_type_enum)ul_type;
    *puc_ie_len = 0;

    /* 判断是否需要设置timeout_interval IE */
    if (en_tie_type >= MAC_TIE_BUTT) {
        return;
    }

    /* Timeout Interval Parameter Element Format
    -----------------------------------------------------------------------
    |ElementID | Length | Timeout Interval Type| Timeout Interval Value  |
    -----------------------------------------------------------------------
    |1         | 1      | 1                    |  4                      |
    -----------------------------------------------------------------------
    */
    puc_buffer[0] = MAC_EID_TIMEOUT_INTERVAL;
    puc_buffer[1] = MAC_TIMEOUT_INTERVAL_INFO_LEN;
    puc_buffer[2] = en_tie_type;

    /* 设置Timeout Interval Value */
    puc_buffer[3] = ul_timeout & 0x000000FF;
    puc_buffer[4] = (ul_timeout & 0x0000FF00)>>8;
    puc_buffer[5] = (ul_timeout & 0x00FF0000)>>16;
    puc_buffer[6] = (ul_timeout & 0xFF000000)>>24;

    *puc_ie_len = MAC_IE_HDR_LEN + MAC_TIMEOUT_INTERVAL_INFO_LEN;
#else
    *puc_ie_len = 0;
#endif
    return;
}
#endif // #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)


oal_void mac_set_ht_capabilities_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    mac_vap_stru *pst_mac_vap        = (mac_vap_stru *)pst_vap;

    if (pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11HighThroughputOptionImplemented != OAL_TRUE) {
        *puc_ie_len = 0;
        return;
    }

    /***************************************************************************
    -------------------------------------------------------------------------
    |EID |Length |HT Capa. Info |A-MPDU Parameters |Supported MCS Set|
    -------------------------------------------------------------------------
    |1   |1      |2             |1                 |16               |
    -------------------------------------------------------------------------
    |HT Extended Cap. |Transmit Beamforming Cap. |ASEL Cap.          |
    -------------------------------------------------------------------------
    |2                |4                         |1                  |
    -------------------------------------------------------------------------
    ***************************************************************************/
    *puc_buffer       = MAC_EID_HT_CAP;
    *(puc_buffer + 1) = MAC_HT_CAP_LEN;

    puc_buffer += MAC_IE_HDR_LEN;

    /* 填充ht capabilities information域信息 */
    mac_set_ht_capinfo_field(pst_vap, puc_buffer);
    puc_buffer += MAC_HT_CAPINFO_LEN;

    /* 填充A-MPDU parameters域信息 */
    mac_set_ampdu_params_field(pst_vap, puc_buffer);
    puc_buffer += MAC_HT_AMPDU_PARAMS_LEN;

    /* 填充supported MCS set域信息 */
    mac_set_sup_mcs_set_field(pst_vap, puc_buffer);
    puc_buffer += MAC_HT_SUP_MCS_SET_LEN;

    /* 填充ht extended capabilities域信息 */
    mac_set_ht_extcap_field(pst_vap, puc_buffer);
    puc_buffer += MAC_HT_EXT_CAP_LEN;

    /* 填充 transmit beamforming capabilities域信息 */
    mac_set_txbf_cap_field(pst_vap, puc_buffer);
    puc_buffer += MAC_HT_TXBF_CAP_LEN;

    /* 填充asel(antenna selection) capabilities域信息 */
    mac_set_asel_cap_field(pst_vap, puc_buffer);

    *puc_ie_len = MAC_IE_HDR_LEN + MAC_HT_CAP_LEN;
}


oal_void mac_set_ht_opern_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    mac_vap_stru        *pst_mac_vap     = (mac_vap_stru *)pst_vap;
    mac_ht_opern_stru   *pst_ht_opern = OAL_PTR_NULL;
    oal_uint8           uc_obss_non_ht = 0;

    if (mac_mib_get_HighThroughputOptionImplemented(pst_mac_vap) != OAL_TRUE) {
        *puc_ie_len = 0;
        return;
    }

    /***************************************************************************
      ----------------------------------------------------------------------
      |EID |Length |PrimaryChannel |HT Operation Information |Basic MCS Set|
      ----------------------------------------------------------------------
      |1   |1      |1              |5                        |16           |
      ----------------------------------------------------------------------
    ***************************************************************************/

    /************************ HT Information Field ****************************
     |--------------------------------------------------------------------|
     | Primary | Seconday  | STA Ch | RIFS |           reserved           |
     | Channel | Ch Offset | Width  | Mode |                              |
     |--------------------------------------------------------------------|
     |    1    | B0     B1 |   B2   |  B3  |    B4                     B7 |
     |--------------------------------------------------------------------|

     |----------------------------------------------------------------|
     |     HT     | Non-GF STAs | resv      | OBSS Non-HT  | Reserved |
     | Protection |   Present   |           | STAs Present |          |
     |----------------------------------------------------------------|
     | B0     B1  |     B2      |    B3     |     B4       | B5   B15 |
     |----------------------------------------------------------------|

     |-------------------------------------------------------------|
     | Reserved |  Dual  |  Dual CTS  | Seconday | LSIG TXOP Protn |
     |          | Beacon | Protection |  Beacon  | Full Support    |
     |-------------------------------------------------------------|
     | B0    B5 |   B6   |     B7     |     B8   |       B9        |
     |-------------------------------------------------------------|

     |---------------------------------------|
     |  PCO   |  PCO  | Reserved | Basic MCS |
     | Active | Phase |          |    Set    |
     |---------------------------------------|
     |  B10   |  B11  | B12  B15 |    16     |
     |---------------------------------------|
    **************************************************************************/

    *puc_buffer = MAC_EID_HT_OPERATION;

    *(puc_buffer + 1) = MAC_HT_OPERN_LEN;
    pst_ht_opern = (mac_ht_opern_stru *)(puc_buffer + MAC_IE_HDR_LEN);

    /* 主信道编号 */
    pst_ht_opern->uc_primary_channel = pst_mac_vap->st_channel.uc_chan_number;

#if 1
    /* 设置"次信道偏移量" */
    if ((pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40PLUS) ||
        (pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_80PLUSPLUS) ||
        (pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_80PLUSMINUS)) {
        pst_ht_opern->bit_secondary_chan_offset = MAC_SCA;
    } else if ((pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40MINUS) ||
               (pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_80MINUSPLUS) ||
               (pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_80MINUSMINUS)) {
        pst_ht_opern->bit_secondary_chan_offset = MAC_SCB;
    } else {
        pst_ht_opern->bit_secondary_chan_offset = MAC_SCN;
    }
#else
    pst_ht_opern->bit_secondary_chan_offset = MAC_SCN;
#endif

    /* 设置"STA信道宽度"，当BSS运行信道宽度 >= 40MHz时，需要将此field设置为1 */
    pst_ht_opern->bit_sta_chan_width = (pst_mac_vap->st_channel.en_bandwidth > WLAN_BAND_WIDTH_20M) ? 1 : 0;

    /* 指示基本服务集里是否允许使用减小的帧间距 */
    pst_ht_opern->bit_rifs_mode = mac_mib_get_RifsMode(pst_mac_vap);

    /* B4-B7保留 */
    pst_ht_opern->bit_resv1 = 0;

    /* 指示ht传输的保护要求 */
    pst_ht_opern->bit_HT_protection = mac_mib_get_HtProtection(pst_mac_vap);

    /* Non-GF STAs */
    pst_ht_opern->bit_nongf_sta_present = mac_mib_get_NonGFEntitiesPresent(pst_mac_vap);

    /* B3 resv */
    pst_ht_opern->bit_resv2 = 0;

    /* B4  obss_nonht_sta_present */
    if ((pst_mac_vap->st_protection.bit_obss_non_ht_present != 0) ||
        (pst_mac_vap->st_protection.uc_sta_non_ht_num != 0)) {
        uc_obss_non_ht = 1;
    }
    pst_ht_opern->bit_obss_nonht_sta_present = uc_obss_non_ht;

    /* B5-B15 保留 */
    pst_ht_opern->bit_resv3 = 0;
    pst_ht_opern->bit_resv4 = 0;

    /* B0-B5 保留 */
    pst_ht_opern->bit_resv5 = 0;

    /* B6  dual_beacon */
    pst_ht_opern->bit_dual_beacon = 0;

    /* Dual CTS protection */
    pst_ht_opern->bit_dual_cts_protection = pst_mac_vap->pst_mib_info->st_wlan_mib_operation.en_dot11DualCTSProtection;

    /* secondary_beacon: Set to 0 in a primary beacon */
    pst_ht_opern->bit_secondary_beacon = 0;

    /* BSS support L-SIG TXOP Protection */
    pst_ht_opern->bit_lsig_txop_protection_full_support = mac_mib_get_LsigTxopFullProtectionActivated(pst_mac_vap);

    /* PCO active */
    pst_ht_opern->bit_pco_active = pst_mac_vap->pst_mib_info->st_wlan_mib_operation.en_dot11PCOActivated;

    /* PCO phase */
    pst_ht_opern->bit_pco_phase = 0;

    /* B12-B15  保留 */
    pst_ht_opern->bit_resv6 = 0;

    /* Basic MCS Set: set all bit zero,Indicates the MCS values that are supported by all HT STAs in the BSS. */
    memset_s(pst_ht_opern->auc_basic_mcs_set, MAC_HT_BASIC_MCS_SET_LEN, 0, MAC_HT_BASIC_MCS_SET_LEN);

    *puc_ie_len = MAC_IE_HDR_LEN + MAC_HT_OPERN_LEN;
}


oal_void mac_set_obss_scan_params(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    mac_vap_stru                *pst_mac_vap = (mac_vap_stru *)pst_vap;
    mac_obss_scan_params_stru   *pst_obss_scan = OAL_PTR_NULL;
    oal_uint32                  ul_ret;

    if (mac_mib_get_HighThroughputOptionImplemented(pst_mac_vap) != OAL_TRUE) {
        *puc_ie_len = 0;
        return;
    }

    if ((pst_mac_vap->st_channel.en_band != WLAN_BAND_2G) ||
        (mac_mib_get_FortyMHzOperationImplemented(pst_mac_vap) != OAL_TRUE)) {
        *puc_ie_len = 0;
        return;
    }

    /***************************************************************************
     |ElementID |Length |OBSS    |OBSS   |BSS Channel   |OBSS Scan  |OBSS Scan   |
     |          |       |Scan    |Scan   |Width Trigger |Passive    |Active Total|
     |          |       |Passive |Active |Scan Interval |Total Per  |Per         |
     |          |       |Dwell   |Dwell  |              |Channel    |Channel     |
     ----------------------------------------------------------------------------
     |1         |1      |2       |2      |2             |2          |2           |
     ----------------------------------------------------------------------------
     |BSS Width   |OBSS Scan|
     |Channel     |Activity |
     |Transition  |Threshold|
     |Delay Factor|         |
     ------------------------
     |2           |2        |
    ***************************************************************************/
    puc_buffer[0] = MAC_EID_OBSS_SCAN;
    puc_buffer[1] = MAC_OBSS_SCAN_IE_LEN;

    pst_obss_scan = (mac_obss_scan_params_stru *)(puc_buffer + MAC_IE_HDR_LEN);

    ul_ret = mac_mib_get_OBSSScanPassiveDwell(pst_mac_vap);
    pst_obss_scan->us_passive_dwell = (oal_uint16)(OAL_BYTEORDER_TO_LE32(ul_ret));

    ul_ret = mac_mib_get_OBSSScanActiveDwell(pst_mac_vap);
    pst_obss_scan->us_active_dwell  = (oal_uint16)(OAL_BYTEORDER_TO_LE32(ul_ret));

    ul_ret = mac_mib_get_BSSWidthTriggerScanInterval(pst_mac_vap);
    pst_obss_scan->us_scan_interval = (oal_uint16)(OAL_BYTEORDER_TO_LE32(ul_ret));

    ul_ret = mac_mib_get_OBSSScanPassiveTotalPerChannel(pst_mac_vap);
    pst_obss_scan->us_passive_total_per_chan  = (oal_uint16)(OAL_BYTEORDER_TO_LE32(ul_ret));

    ul_ret = mac_mib_get_OBSSScanActiveTotalPerChannel(pst_mac_vap);
    pst_obss_scan->us_active_total_per_chan   = (oal_uint16)(OAL_BYTEORDER_TO_LE32(ul_ret));

    ul_ret = mac_mib_get_BSSWidthChannelTransitionDelayFactor(pst_mac_vap);
    pst_obss_scan->us_transition_delay_factor = (oal_uint16)(OAL_BYTEORDER_TO_LE32(ul_ret));

    ul_ret = mac_mib_get_OBSSScanActivityThreshold(pst_mac_vap);
    pst_obss_scan->us_scan_activity_thresh    = (oal_uint16)(OAL_BYTEORDER_TO_LE32(ul_ret));

    *puc_ie_len = MAC_IE_HDR_LEN + MAC_OBSS_SCAN_IE_LEN;
}


oal_void mac_set_ext_capabilities_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    mac_vap_stru            *pst_mac_vap = (mac_vap_stru *)pst_vap;
    mac_ext_cap_ie_stru     *pst_ext_cap = OAL_PTR_NULL;

    if (mac_mib_get_HighThroughputOptionImplemented(pst_mac_vap) == OAL_FALSE) {
        *puc_ie_len = 0;
        return;
    }

    /***************************************************************************
                         ----------------------------------
                         |Element ID |Length |Capabilities|
                         ----------------------------------
          Octets:        |1          |1      |n           |
                         ----------------------------------
    ------------------------------------------------------------------------------------------
    |  B0       | B1 | B2             | B3   | B4   |  B5  |  B6    |  B7   | ...|  B38    |   B39      |
    ----------------------------------------------------------------------------
    |20/40 coex |resv|extended channel| resv | PSMP | resv | S-PSMP | Event |    |TDLS Pro-  TDLS Channel
                                                                                             Switching
    |mgmt supp  |    |switching       |      |      |      |        |       | ...| hibited | Prohibited |
    --------------------------------------------------------------------------------------------
    ***************************************************************************/
    puc_buffer[0] = MAC_EID_EXT_CAPS;
    puc_buffer[1] = MAC_XCAPS_EX_LEN;

    /* 初始清零 */
    memset_s(puc_buffer + MAC_IE_HDR_LEN, OAL_SIZEOF(mac_ext_cap_ie_stru), 0, OAL_SIZEOF(mac_ext_cap_ie_stru));

    pst_ext_cap = (mac_ext_cap_ie_stru *)(puc_buffer + MAC_IE_HDR_LEN);

    /* 设置20/40 BSS Coexistence Management Support fieid */
    if ((mac_mib_get_2040BSSCoexistenceManagementSupport(pst_mac_vap) == OAL_TRUE) &&
        (pst_mac_vap->st_channel.en_band == WLAN_BAND_2G)) {
        pst_ext_cap->bit_2040_coexistence_mgmt = 1;
    }

    /* 设置TDLS prohibited */
    pst_ext_cap->bit_tdls_prhibited =  pst_mac_vap->st_cap_flag.bit_tdls_prohibited;

    /* 设置TDLS channel switch prohibited */
    pst_ext_cap->bit_tdls_channel_switch_prhibited = pst_mac_vap->st_cap_flag.bit_tdls_channel_switch_prohibited;

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
    /* 如果是11ac 站点 设置OPMODE NOTIFY标志 */
    if (mac_mib_get_VHTOptionImplemented(pst_mac_vap) == OAL_TRUE) {
        pst_ext_cap->bit_operating_mode_notification = mac_mib_get_OperatingModeNotificationImplemented(pst_mac_vap);
    }
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
    if ((pst_mac_vap->pst_vap_proxyarp != OAL_PTR_NULL) &&
        (pst_mac_vap->pst_vap_proxyarp->en_is_proxyarp == OAL_TRUE) &&
        (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP)) {
        pst_ext_cap->bit_proxyarp = 1;
    } else {
        pst_ext_cap->bit_proxyarp = 0;
    }
#endif

    *puc_ie_len = MAC_IE_HDR_LEN + MAC_XCAPS_EX_LEN;
}


oal_void  mac_set_vht_capinfo_field(oal_void *pst_vap, oal_uint8 *puc_buffer)
{
    mac_vap_stru           *pst_mac_vap     = (mac_vap_stru *)pst_vap;
    mac_vht_cap_info_stru  *pst_vht_capinfo = (mac_vht_cap_info_stru *)puc_buffer;

    /* VHT 能力信息域
     * -----------------------------------------------------------------------
     * Max    | Supp    | RX   | Short GI| Short  | Tx   |  Rx  |  SU
     * MPDU   | Channel | LDPC | for 80  | GI for | STBC | STBC | Beamformer
     * Length | Wth Set |      |         | 160MHz |      |      | Capable
     * -----------------------------------------------------------------------
     * B0 B1  | B2 B3   | B4   |   B5    |   B6   |  B7  |B8 B10|   B11
     * -----------------------------------------------------------------------
     * SU         | Compressed   | Num of    | MU        | MU        | VHT
     * Beamformee | Steering num | Sounding  | Beamformer| Beamformee| TXOP
     * Capable    | of bf ant sup| Dimensions| Capable   | Capable   | PS
     * -----------------------------------------------------------------------
     *    B12     | B13      B15 | B16    B18|   B19     |    B20    | B21
     * -----------------------------------------------------------------------
     * +HTC   | Max AMPDU| VHT Link  | Rx ANT     | Tx ANT     |   Resv
     * VHT    | Length   | Adaptation| Pattern    | Pattern    |
     * Capable| Exponent | Capable   | Consistency| Consistency|
     * -----------------------------------------------------------------------
     * B22    | B23  B25 | B26   B27 |   B28      |   B29      |  B30 B31
     * -----------------------------------------------------------------------
     */
    pst_vht_capinfo->bit_max_mpdu_length = pst_mac_vap->pst_mib_info->st_wlan_mib_vht_sta_config.ul_dot11MaxMPDULength;

    /* 设置"所支持的信道宽度集"，0:80MHz运行; 1:160MHz; 2:80+80MHz */
    pst_vht_capinfo->bit_supported_channel_width = mac_mib_get_VHTChannelWidthOptionImplemented(pst_mac_vap);

    pst_vht_capinfo->bit_rx_ldpc =
        pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.en_dot11VHTLDPCCodingOptionImplemented;
    pst_vht_capinfo->bit_short_gi_80mhz =
        pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.en_dot11VHTShortGIOptionIn80Implemented;
    pst_vht_capinfo->bit_short_gi_160mhz =
        pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.en_dot11VHTShortGIOptionIn160and80p80Implemented;
    pst_vht_capinfo->bit_tx_stbc = pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.en_dot11VHTTxSTBCOptionImplemented;
    pst_vht_capinfo->bit_rx_stbc =
        (pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.en_dot11VHTRxSTBCOptionImplemented == OAL_TRUE) ? 1 : 0;
    pst_vht_capinfo->bit_su_beamformer_cap =
        pst_mac_vap->pst_mib_info->st_wlan_mib_vht_txbf_config.en_dot11VHTSUBeamformerOptionImplemented;
    pst_vht_capinfo->bit_su_beamformee_cap =
        pst_mac_vap->pst_mib_info->st_wlan_mib_vht_txbf_config.en_dot11VHTSUBeamformeeOptionImplemented;
    pst_vht_capinfo->bit_num_sounding_dim =
        pst_mac_vap->pst_mib_info->st_wlan_mib_vht_txbf_config.ul_dot11VHTNumberSoundingDimensions;
    pst_vht_capinfo->bit_mu_beamformer_cap =
        pst_mac_vap->pst_mib_info->st_wlan_mib_vht_txbf_config.en_dot11VHTMUBeamformerOptionImplemented;
    pst_vht_capinfo->bit_mu_beamformee_cap =
        pst_mac_vap->pst_mib_info->st_wlan_mib_vht_txbf_config.en_dot11VHTMUBeamformeeOptionImplemented;
    pst_vht_capinfo->bit_vht_txop_ps =
        pst_mac_vap->pst_mib_info->st_wlan_mib_vht_sta_config.en_dot11VHTTXOPPowerSaveOptionImplemented;
    pst_vht_capinfo->bit_htc_vht_capable =
        pst_mac_vap->pst_mib_info->st_wlan_mib_vht_sta_config.en_dot11VHTControlFieldSupported;
    pst_vht_capinfo->bit_max_ampdu_len_exp =
        pst_mac_vap->pst_mib_info->st_wlan_mib_vht_sta_config.ul_dot11VHTMaxRxAMPDUFactor;
#ifdef _PRE_WLAN_FEATURE_TXBF
    pst_vht_capinfo->bit_num_bf_ant_supported =
        pst_mac_vap->pst_mib_info->st_wlan_mib_vht_txbf_config.ul_dot11VHTBeamformeeNTxSupport;
#else
    pst_vht_capinfo->bit_num_bf_ant_supported   = 0;
#endif
    pst_vht_capinfo->bit_vht_link_adaptation    = 0;
    pst_vht_capinfo->bit_rx_ant_pattern         = 0;   /* 在该关联中不改变天线模式，设为1,；改变则设为0 */
    pst_vht_capinfo->bit_tx_ant_pattern         = 0;   /* 在该关联中不改变天线模式，设为1,；改变则设为0 */

    /* resv位清0 */
    pst_vht_capinfo->bit_resv = 0;
}


oal_void  mac_set_vht_supported_mcsset_field(oal_void *pst_vap, oal_uint8 *puc_buffer)
{
    mac_vap_stru              *pst_mac_vap    = (mac_vap_stru *)pst_vap;
    mac_vht_sup_mcs_set_stru  *pst_vht_mcsset = (mac_vht_sup_mcs_set_stru *)puc_buffer;

    /* VHT 支持的MCS集
     * -----------------------------------------------------------------------
     * Rx MCS Map | Rx Highest Supported | Resv    | Tx MCS Map  |
     *            | Long gi Data Rate    |         |             |
     * -----------------------------------------------------------------------
     * B0     B15 | B16              B28 | B29 B31 | B32     B47 |
     * -----------------------------------------------------------------------
     * Tx Highest Supported |  Resv   |
     * Long gi Data Rate    |         |
     * -----------------------------------------------------------------------
     *  B48             B60 | B61 B63 |
     * -----------------------------------------------------------------------
     */
    pst_vht_mcsset->bit_rx_mcs_map      = pst_mac_vap->pst_mib_info->st_wlan_mib_vht_sta_config.us_dot11VHTRxMCSMap;
    pst_vht_mcsset->bit_rx_highest_rate =
        pst_mac_vap->pst_mib_info->st_wlan_mib_vht_sta_config.ul_dot11VHTRxHighestDataRateSupported;
    pst_vht_mcsset->bit_tx_mcs_map      = pst_mac_vap->pst_mib_info->st_wlan_mib_vht_sta_config.us_dot11VHTTxMCSMap;
    pst_vht_mcsset->bit_tx_highest_rate =
        pst_mac_vap->pst_mib_info->st_wlan_mib_vht_sta_config.ul_dot11VHTTxHighestDataRateSupported;

    /* resv清0 */
    pst_vht_mcsset->bit_resv  = 0;
    pst_vht_mcsset->bit_resv2 = 0;
}


oal_void  mac_set_vht_capabilities_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    mac_vap_stru  *pst_mac_vap = (mac_vap_stru *)pst_vap;
    if ((pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11VHTOptionImplemented != OAL_TRUE)
#ifdef _PRE_WLAN_FEATURE_11AC2G
        || ((pst_mac_vap->st_cap_flag.bit_11ac2g == OAL_FALSE) && (pst_mac_vap->st_channel.en_band == WLAN_BAND_2G))
#endif /* _PRE_WLAN_FEATURE_11AC2G */
        ) {
        *puc_ie_len = 0;
        return;
    }

    /*
     * |EID |Length |VHT Capa. Info |VHT Supported MCS Set|
     * -------------------------------------------------------------------------
     * |1   |1      | 4             | 8                   |
     * -------------------------------------------------------------------------
     */
    puc_buffer[0] = MAC_EID_VHT_CAP;
    puc_buffer[1] = MAC_VHT_CAP_IE_LEN;

    puc_buffer += MAC_IE_HDR_LEN;

    mac_set_vht_capinfo_field(pst_vap, puc_buffer);

    puc_buffer += MAC_VHT_CAP_INFO_FIELD_LEN;

    mac_set_vht_supported_mcsset_field(pst_vap, puc_buffer);

    *puc_ie_len = MAC_IE_HDR_LEN + MAC_VHT_CAP_IE_LEN;
}


oal_void  mac_set_vht_opern_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    mac_vap_stru        *pst_mac_vap = (mac_vap_stru *)pst_vap;
    mac_vht_opern_stru  *pst_vht_opern = OAL_PTR_NULL;

    if ((pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11VHTOptionImplemented != OAL_TRUE)
#ifdef _PRE_WLAN_FEATURE_11AC2G
        || ((pst_mac_vap->st_cap_flag.bit_11ac2g == OAL_FALSE) && (pst_mac_vap->st_channel.en_band == WLAN_BAND_2G))
#endif /* _PRE_WLAN_FEATURE_11AC2G */
        ) {
        *puc_ie_len = 0;
        return;
    }

    /* VHT Operation element
     *        |EID |Length |VHT Opern Info |VHT Basic MCS Set|
     * -------------------------------------------------------------------------
     * Octes:  |1   |1      | 3             | 2               |
     * -------------------------------------------------------------------------
     */
    puc_buffer[0] = MAC_EID_VHT_OPERN;
    puc_buffer[1] = MAC_VHT_INFO_IE_LEN;

    puc_buffer += MAC_IE_HDR_LEN;

    /* VHT Opern Info
     *         | Channel Width | Channel Center | Channel Center |
     *         |               | Freq Seg0      | Freq Seg1      |
     * -------------------------------------------------------------------------
     * Octes:  |       1       |       1        |       1        |
     * -------------------------------------------------------------------------
     */
    pst_vht_opern = (mac_vht_opern_stru *)puc_buffer;

    /* uc_channel_width的取值，0 -- 20/40M, 1 -- 80M, 2 -- 160M */
    if (pst_mac_vap->st_channel.en_bandwidth >= WLAN_BAND_WIDTH_80PLUSPLUS) {
        pst_vht_opern->uc_channel_width = 1;
    } else {
        pst_vht_opern->uc_channel_width = 0;
    }

    switch (pst_mac_vap->st_channel.en_bandwidth) {
        case WLAN_BAND_WIDTH_80PLUSPLUS:
            /*
             * | 主20 | 从20 | 从40       |
             *               |中心频率相对于主20偏6个信道
             */
            pst_vht_opern->uc_channel_center_freq_seg0 = pst_mac_vap->st_channel.uc_chan_number + 6;
            break;

        case WLAN_BAND_WIDTH_80PLUSMINUS:
            /*
             * 从40        | 主20 | 从20 |
             *             |中心频率相对于主20偏-2个信道
             */
            pst_vht_opern->uc_channel_center_freq_seg0 = pst_mac_vap->st_channel.uc_chan_number - 2;
            break;

        case WLAN_BAND_WIDTH_80MINUSPLUS:
            /*
             * 从20 | 主20 | 从40       |
             *             |中心频率相对于主20偏2个信道
             */
            pst_vht_opern->uc_channel_center_freq_seg0 = pst_mac_vap->st_channel.uc_chan_number + 2;
            break;

        case WLAN_BAND_WIDTH_80MINUSMINUS:
            /*
             * 从40        | 从20 | 主20 |
             *             |中心频率相对于主20偏-6个信道
             */
            pst_vht_opern->uc_channel_center_freq_seg0 = pst_mac_vap->st_channel.uc_chan_number - 6;
            break;

        case WLAN_BAND_WIDTH_40MINUS:
            /*
             * 从20 | 主20 |
             * 中心频率相对于主20偏-2个信道
             */
            pst_vht_opern->uc_channel_center_freq_seg0 = pst_mac_vap->st_channel.uc_chan_number - 2;
            break;

        case WLAN_BAND_WIDTH_40PLUS:
            /*
             * 主20 | 从20 |
             * 中心频率相对于主20偏+2个信道
             */
            pst_vht_opern->uc_channel_center_freq_seg0 = pst_mac_vap->st_channel.uc_chan_number + 2;
            break;

        case WLAN_BAND_WIDTH_20M:
            /* 中心频率就是主信道频率   */
            pst_vht_opern->uc_channel_center_freq_seg0 = pst_mac_vap->st_channel.uc_chan_number;
            break;

        default:
            break;
    }

    pst_vht_opern->uc_channel_center_freq_seg1 = 0;
    pst_vht_opern->us_basic_mcs_set = pst_mac_vap->pst_mib_info->st_wlan_mib_vht_sta_config.us_dot11VHTRxMCSMap;

    *puc_ie_len = MAC_IE_HDR_LEN + MAC_VHT_INFO_IE_LEN;
}


oal_uint32  mac_set_csa_ie(oal_uint8 uc_channel, oal_uint8 uc_csa_cnt, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    if (OAL_UNLIKELY((puc_buffer == OAL_PTR_NULL) || (puc_ie_len == OAL_PTR_NULL))) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*  Channel Switch Announcement Information Element Format
     * ---------------------------------------------------------------------
     * | Element ID | Length | Chnl Switch Mode | New Chnl | Ch Switch Cnt |
     * ---------------------------------------------------------------------
     * | 1          | 1      | 1                | 1        | 1             |
     */

    /* 设置Channel Switch Announcement Element */
    puc_buffer[0] = MAC_EID_CHANSWITCHANN;
    puc_buffer[1] = MAC_CHANSWITCHANN_LEN;
    puc_buffer[2] = 1;                       /* ask all associated STAs to stop transmission */
    puc_buffer[3] = uc_channel;
    puc_buffer[4] = uc_csa_cnt;

    *puc_ie_len = MAC_IE_HDR_LEN + MAC_CHANSWITCHANN_LEN;

    return OAL_SUCC;
}


oal_uint8*  mac_get_ssid(oal_uint8 *puc_beacon_body, oal_int32 l_frame_body_len, oal_uint8 *puc_ssid_len)
{
    const oal_uint8   *puc_ssid_ie = OAL_PTR_NULL;
    oal_uint16         us_offset =  MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;

    /*
     * Beacon Frame - Frame Body
     * ---------------------------------------------------------------------
     * |Timestamp |BeaconInt |CapInfo |SSID |SupRates |DSParSet |TIM elm   |
     * ---------------------------------------------------------------------
     * |8         |2         |2       |2-34 |3-10     |3        |4-256     |
     * ---------------------------------------------------------------------
     *
     */

    /*
     *               ----------------------------
     *               |Element ID | Length | SSID|
     *               ----------------------------
     *      Octets:  |1          | 1      | 0~32|
     *               ----------------------------
     */
    /* ssid的长度初始赋值为0 */
    *puc_ssid_len = 0;

    /* 检测beacon帧或者probe rsp帧的长度的合法性 */
    if (l_frame_body_len <= us_offset) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{mac_get_ssid:: the length of beacon/probe rsp frame body is invalid.}");
        return OAL_PTR_NULL;
    }

    /* 查找ssid的ie */
    puc_ssid_ie = mac_find_ie(MAC_EID_SSID, (puc_beacon_body + us_offset), (oal_int32)(l_frame_body_len - us_offset));
    if ((puc_ssid_ie != OAL_PTR_NULL) && (puc_ssid_ie[1] < WLAN_SSID_MAX_LEN)) {
        /* 获取ssid ie的长度 */
        *puc_ssid_len = puc_ssid_ie[1];

        return (oal_uint8 *)(puc_ssid_ie + MAC_IE_HDR_LEN);
    }

    OAM_WARNING_LOG0(0, OAM_SF_ANY, "{mac_get_ssid:: ssid ie isn't found.}");
    return OAL_PTR_NULL;
}

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

oal_uint16  mac_get_beacon_period(oal_uint8 *puc_beacon_body)
{
    /*
     * Beacon Frame - Frame Body
     * ---------------------------------------------------------------------
     * |Timestamp |BeaconInt |CapInfo |SSID |SupRates |DSParSet |TIM elm   |
     * ---------------------------------------------------------------------
     * |8         |2         |2       |2-34 |3-10     |3        |4-256     |
     * ---------------------------------------------------------------------
     */
    return *((oal_uint16 *)(puc_beacon_body + MAC_TIME_STAMP_LEN));
}


oal_uint8  mac_get_dtim_period(oal_uint8 *puc_frame_body, oal_uint16 us_frame_body_len)
{
    oal_uint8   *puc_ie;

    oal_uint16   us_offset;

    us_offset = MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;

    puc_ie = mac_find_ie(MAC_EID_TIM, puc_frame_body + us_offset, us_frame_body_len - us_offset);
    if ((puc_ie != OAL_PTR_NULL) && (puc_ie[1] >= MAC_MIN_TIM_LEN)) {
        return puc_ie[3];
    }

    return 0;
}


oal_uint8  mac_get_dtim_cnt(oal_uint8 *puc_frame_body, oal_uint16 us_frame_body_len)
{
    oal_uint8   *puc_ie;

    oal_uint16   us_offset;

    us_offset = MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;

    puc_ie = mac_find_ie(MAC_EID_TIM, puc_frame_body + us_offset, us_frame_body_len - us_offset);
    if ((puc_ie != OAL_PTR_NULL) && (puc_ie[1] >= MAC_MIN_TIM_LEN)) {
        return puc_ie[2];
    }

    return 0;
}
#endif // #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)


oal_bool_enum_uint8  mac_is_wmm_ie(oal_uint8 *puc_ie)
{
    /* ---------------------------------------------------------------------
     * WMM Information/Parameter Element Format
     * ---------------------------------------------------------------------
     * | OUI | OUIType | OUISubtype | Version | QoSInfo | OUISubtype based |
     * ---------------------------------------------------------------------
     * |3    | 1       | 1          | 1       | 1       | ---------------- |
     */
    if ((puc_ie[0] == MAC_EID_WMM) && (puc_ie[1] >= MAC_WMM_IE_LEN) && (puc_ie[2] == MAC_WMM_OUI_BYTE_ONE) &&
        (puc_ie[3] == MAC_WMM_OUI_BYTE_TWO) &&
        (puc_ie[4] == MAC_WMM_OUI_BYTE_THREE) && /* OUI */
        (puc_ie[5] == MAC_OUITYPE_WMM) && /* OUI Type     */
        ((puc_ie[6] == MAC_OUISUBTYPE_WMM_INFO) || (puc_ie[6] == MAC_OUISUBTYPE_WMM_PARAM)) && /* OUI Sub Type */
        (puc_ie[7] == MAC_OUI_WMM_VERSION)) { /* Version field */
        return OAL_TRUE;
    }

    return OAL_FALSE;
}
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

oal_uint8* mac_get_wmm_ie(oal_uint8 *puc_beacon_body, oal_uint16 us_frame_len)
{
    uint8_t *wmmie_data = OAL_PTR_NULL;

    wmmie_data = mac_find_vendor_ie(MAC_WLAN_OUI_MICROSOFT, MAC_WLAN_OUI_TYPE_MICROSOFT_WMM,
                                    puc_beacon_body, us_frame_len);
    if (wmmie_data == OAL_PTR_NULL) {
        return OAL_PTR_NULL;
    }

    return mac_is_wmm_ie(wmmie_data) ? wmmie_data : OAL_PTR_NULL;
}


oal_uint8* mac_get_obss_scan_ie(oal_uint8 *puc_beacon_body, oal_uint16 us_frame_len, oal_uint16 us_offset)
{
    oal_uint16 us_index;

    us_index = us_offset;

    /* 寻找ht opern信息元素 */
    while (us_index < us_frame_len) {
        if (puc_beacon_body[us_index] == MAC_EID_OBSS_SCAN) {
            return(&puc_beacon_body[us_index]);
        } else {
            us_index += (MAC_IE_HDR_LEN + puc_beacon_body[us_index + 1]);
        }
    }

    return OAL_PTR_NULL;
}


oal_uint16 mac_get_rsn_capability(const oal_uint8 *puc_rsn_ie)
{
    oal_uint16  us_pairwise_count;
    oal_uint16  us_akm_count;
    oal_uint16  us_rsn_capability;
    oal_uint16  us_index = 0;

    if (puc_rsn_ie == OAL_PTR_NULL) {
        return 0;
    }

    /*
     *                  RSN Element Format
     * ---------------------------------------------------------------------
     * |Element ID | Length | Version | Group Cipher Suite | Pairwise Cipher
     * ---------------------------------------------------------------------
     * |     1     |    1   |    2    |         4          |       2
     * ---------------------------------------------------------------------
     * ---------------------------------------------------------------------
     * Suite Count| Pairwise Cipher Suite List | AKM Suite Count | AKM Suite List
     * ---------------------------------------------------------------------
     *            |         4*m                |     2           |   4*n
     * ---------------------------------------------------------------------
     * ---------------------------------------------------------------------
     * |RSN Capabilities|PMKID Count|PMKID List|Group Management Cipher Suite
     * ---------------------------------------------------------------------
     * |        2       |    2      |   16 *s  |               4           |
     * ---------------------------------------------------------------------
     */
    if (puc_rsn_ie[1] < MAC_MIN_RSN_LEN) {
        OAM_WARNING_LOG1(0, OAM_SF_WPA, "{hmac_get_rsn_capability::invalid rsn ie len[%d].}", puc_rsn_ie[1]);
        return 0;
    }

    us_index += 8;
    us_pairwise_count = OAL_MAKE_WORD16(puc_rsn_ie[us_index], puc_rsn_ie[us_index + 1]);
    if (us_pairwise_count > MAC_PAIRWISE_CIPHER_SUITES_NUM) {
        OAM_WARNING_LOG1(0, OAM_SF_WPA, "{hmac_get_rsn_capability::invalid us_pairwise_count[%d].}", us_pairwise_count);
        return 0;
    }

    us_index += 2 + 4 * (oal_uint8)us_pairwise_count;

    us_akm_count = OAL_MAKE_WORD16(puc_rsn_ie[us_index], puc_rsn_ie[us_index + 1]);
    if (us_akm_count > WLAN_AUTHENTICATION_SUITES) {
        OAM_WARNING_LOG1(0, OAM_SF_WPA, "{hmac_get_rsn_capability::invalid us_akm_count[%d].}", us_akm_count);
        return 0;
    }

    us_index += 2 + 4 * (oal_uint8)us_akm_count;

    if (MAC_IE_REAMIN_LEN_IS_ENOUGH(2, us_index, puc_rsn_ie[1], 2) == OAL_FALSE) {
        return 0;
    }

    us_rsn_capability = OAL_MAKE_WORD16(puc_rsn_ie[us_index], puc_rsn_ie[us_index + 1]);
    return us_rsn_capability;
}


oal_void mac_set_power_cap_ie(oal_uint8 *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    mac_vap_stru            *pst_mac_vap        = (mac_vap_stru *)pst_vap;
    mac_regclass_info_stru  *pst_regclass_info = OAL_PTR_NULL;

    /*
     *       ------------------------------------------------------------------------------------
     *       |ElementID | Length | MinimumTransmitPowerCapability| MaximumTransmitPowerCapability|
     *       ------------------------------------------------------------------------------------
     * Octets: |1         | 1      | 1                             | 1                           |
     *       -------------------------------------------------------------------------------------
     */

    *puc_buffer       = MAC_EID_PWRCAP;
    *(puc_buffer + 1) = MAC_PWR_CAP_LEN;

    /* 成功获取管制域信息则根据国家码和TPC设置最大和最小发射功率，否则默认为0 */
    pst_regclass_info =
        mac_get_channel_num_rc_info(pst_mac_vap->st_channel.en_band, pst_mac_vap->st_channel.uc_chan_number);
    if (pst_regclass_info != OAL_PTR_NULL) {
        *(puc_buffer + 2) = (oal_uint8)((pst_mac_vap->st_channel.en_band == WLAN_BAND_2G) ? 4 : 3);
        *(puc_buffer + 3) = OAL_MIN(pst_regclass_info->uc_max_reg_tx_pwr, pst_regclass_info->uc_max_tx_pwr);
    } else {
        *(puc_buffer + 2) = 0;
        *(puc_buffer + 3) = 0;
    }
    *puc_ie_len = MAC_IE_HDR_LEN + MAC_PWR_CAP_LEN;
}


oal_void mac_set_supported_channel_ie(oal_uint8 *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    oal_uint8            uc_channel_max_num;
    oal_uint8            uc_channel_idx;
    oal_uint8            us_channel_ie_len = 0;
    oal_uint8           *puc_ie_len_buffer = 0;
    mac_vap_stru        *pst_mac_vap       = (mac_vap_stru *)pst_vap;
    oal_uint8            uc_channel_idx_cnt = 0;

    if (pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11SpectrumManagementRequired == OAL_FALSE ||
        pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11ExtendedChannelSwitchActivated == OAL_TRUE) {
        *puc_ie_len = 0;
        return;
    }

    /*
     *       长度不定，信道号与信道数成对出现
     *       ------------------------------------------------------------------------------------
     *       |ElementID | Length | Fisrt Channel Number| Number of Channels|
     *       ------------------------------------------------------------------------------------
     * Octets: |1         | 1      | 1                   | 1                 |
     *       -------------------------------------------------------------------------------------
     */
    /* 根据支持的频段获取最大信道个数 */
    if (pst_mac_vap->st_channel.en_band == WLAN_BAND_2G) {
        uc_channel_max_num = (oal_uint8)MAC_CHANNEL_FREQ_2_BUTT;
    }
#ifdef _PRE_WLAN_FEATURE_5G
    else if (pst_mac_vap->st_channel.en_band == WLAN_BAND_5G) {
        uc_channel_max_num = (oal_uint8)MAC_CHANNEL_FREQ_5_BUTT;
    }
#endif /* _PRE_WLAN_FEATURE_5G */
    else {
        *puc_ie_len = 0;
        return;
    }

    *puc_buffer = MAC_EID_SUPPCHAN;
    puc_buffer++;
    puc_ie_len_buffer = puc_buffer;

    /* 填写信道信息 */
    for (uc_channel_idx = 0; uc_channel_idx < uc_channel_max_num; uc_channel_idx++) {
        /* 修改管制域结构体后，需要增加该是否支持信号的判断 */
        if (mac_is_channel_idx_valid(pst_mac_vap->st_channel.en_band, uc_channel_idx) == OAL_SUCC) {
            uc_channel_idx_cnt++;
            /* uc_channel_idx_cnt为1的时候表示是第一个可用信道，需要写到Fisrt Channel Number */
            if (uc_channel_idx_cnt == 1) {
                puc_buffer++;

                mac_get_channel_num_from_idx(pst_mac_vap->st_channel.en_band, uc_channel_idx, puc_buffer);
            } else if ((uc_channel_max_num - 1) == uc_channel_idx) {
                /* 将Number of Channels写入帧体中 */
                puc_buffer++;
                *puc_buffer = uc_channel_idx_cnt;

                us_channel_ie_len += 2;
            }
        } else {
            /* uc_channel_idx_cnt不为0的时候表示之前有可用信道，需要将可用信道的长度写到帧体中 */
            if (uc_channel_idx_cnt != 0) {
                /* 将Number of Channels写入帧体中 */
                puc_buffer++;
                *puc_buffer = uc_channel_idx_cnt;

                us_channel_ie_len += 2;
            }
            /* 将Number of Channels统计清零 */
            uc_channel_idx_cnt = 0;
        }
    }

    *puc_ie_len_buffer = us_channel_ie_len;
    *puc_ie_len = us_channel_ie_len + MAC_IE_HDR_LEN;
}


oal_void mac_set_wmm_ie_sta(oal_uint8 *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    oal_uint8            uc_index;
    mac_vap_stru        *pst_mac_vap  = (mac_vap_stru *)pst_vap;
    int32_t ret;

    /* WMM Information Element Format
     * -------------------------------------------------------------
     * | 3     | 1        | 1           | 1             | 1        |
     * -------------------------------------------------------------
     * | OUI   | OUI Type | OUI Subtype | Version field | QoS Info |
     */

    /* 判断STA是否支持WMM */
    if (pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11QosOptionImplemented != OAL_TRUE) {
        *puc_ie_len = 0;
        return;
    }

    puc_buffer[0]        = MAC_EID_WMM;
    puc_buffer[1]        = MAC_WMM_INFO_LEN;

    uc_index             = MAC_IE_HDR_LEN;

    /* OUI */
    ret = memcpy_s(&puc_buffer[uc_index], MAC_OUI_LEN, g_auc_wmm_oui, MAC_OUI_LEN);
    if (ret != EOK) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TX, "mac_set_wmm_ie_sta::memcopy_s return value %d", ret);
        return;
    }
    uc_index += MAC_OUI_LEN;

    /* OUI Type */
    puc_buffer[uc_index++] = MAC_OUITYPE_WMM;

    /* OUI Subtype */
    puc_buffer[uc_index++] = MAC_OUISUBTYPE_WMM_INFO;

    /* Version field */
    puc_buffer[uc_index++] = MAC_OUI_WMM_VERSION;

    /* QoS Information Field */
    mac_set_qos_info_field(pst_mac_vap, &puc_buffer[uc_index]);
    uc_index += MAC_QOS_INFO_LEN;

    /* Reserved */
    puc_buffer[uc_index++] = 0;

    *puc_ie_len = MAC_IE_HDR_LEN + MAC_WMM_INFO_LEN;
}


oal_void mac_set_listen_interval_ie(oal_uint8 *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    puc_buffer[0] = 0x03;
    puc_buffer[1] = 0x00;
    *puc_ie_len   = MAC_LIS_INTERVAL_IE_LEN;
}


oal_void mac_set_status_code_ie(oal_uint8 *puc_buffer, mac_status_code_enum_uint16 en_status_code)
{
    puc_buffer[0] = (oal_uint8)(en_status_code & 0x00FF);
    puc_buffer[1] = (oal_uint8)((en_status_code & 0xFF00) >> 8);
}


oal_void mac_set_aid_ie(oal_uint8 *puc_buffer, oal_uint16 uc_aid)
{
    /* The 2 MSB bits of Association ID is set to 1 as required by the standard. */
    uc_aid |= 0xC000;
    puc_buffer[0] = (uc_aid & 0x00FF);
    puc_buffer[1] = (uc_aid & 0xFF00) >> 8;
}


oal_uint8  mac_get_bss_type(oal_uint16 us_cap_info)
{
    mac_cap_info_stru *pst_cap_info = (mac_cap_info_stru *)&us_cap_info;

    if (pst_cap_info->bit_ess != 0) {
        return (oal_uint8)WLAN_MIB_DESIRED_BSSTYPE_INFRA;
    }

    if (pst_cap_info->bit_ibss != 0) {
        return (oal_uint8)WLAN_MIB_DESIRED_BSSTYPE_INDEPENDENT;
    }

    return (oal_uint8)WLAN_MIB_DESIRED_BSSTYPE_ANY;
}


oal_uint32  mac_check_mac_privacy(oal_uint16 us_cap_info, oal_uint8 *pst_vap)
{
    mac_vap_stru       *pst_mac_vap = OAL_PTR_NULL;
    mac_cap_info_stru  *pst_cap_info = (mac_cap_info_stru *)&us_cap_info;

    if (pst_vap == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap = (mac_vap_stru *)pst_vap;

    if (pst_mac_vap->pst_mib_info->st_wlan_mib_privacy.en_dot11PrivacyInvoked == OAL_TRUE) {
        /* 该VAP有Privacy invoked但其他VAP没有 */
        if (pst_cap_info->bit_privacy == 0) {
            return (oal_uint32)OAL_FALSE;
        }
    }
    /* 考虑兼容性，当vap不支持加密时，不检查用户的能力 */

    return (oal_uint32)OAL_TRUE;
}
#endif // #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)


oal_void mac_add_app_ie(oal_void *pst_vap,
                        oal_uint8 *puc_buffer,
                        oal_uint16 *pus_ie_len,
                        en_app_ie_type_uint8 en_type)
{
    mac_vap_stru    *pst_mac_vap;
    oal_uint8       *puc_app_ie;
    oal_uint32       ul_app_ie_len;
    int32_t ret;

    pst_mac_vap   = (mac_vap_stru *)pst_vap;
    puc_app_ie    = pst_mac_vap->ast_app_ie[en_type].puc_ie;
    ul_app_ie_len = pst_mac_vap->ast_app_ie[en_type].ul_ie_len;

    if (ul_app_ie_len == 0) {
        *pus_ie_len = 0;
        return;
    } else {
        ret = memcpy_s(puc_buffer, ul_app_ie_len, puc_app_ie, ul_app_ie_len);
        if (ret != EOK) {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TX, "mac_add_app_ie::memcopy_s return value %d", ret);
            return;
        }
        *pus_ie_len = (oal_uint16)ul_app_ie_len;
    }

    return;
}


oal_void mac_add_wps_ie(oal_void *pst_vap,
                        oal_uint8 *puc_buffer,
                        oal_uint16 *pus_ie_len,
                        en_app_ie_type_uint8 en_type)
{
    mac_vap_stru    *pst_mac_vap;
    oal_uint8       *puc_app_ie;
    oal_uint8       *puc_wps_ie = OAL_PTR_NULL;
    oal_uint32       ul_app_ie_len;
    int32_t ret;

    pst_mac_vap   = (mac_vap_stru *)pst_vap;
    puc_app_ie    = pst_mac_vap->ast_app_ie[en_type].puc_ie;
    ul_app_ie_len = pst_mac_vap->ast_app_ie[en_type].ul_ie_len;

    if (ul_app_ie_len == 0) {
        *pus_ie_len = 0;
        return;
    }

    puc_wps_ie = mac_find_vendor_ie(MAC_WLAN_OUI_MICROSOFT, MAC_WLAN_OUI_TYPE_MICROSOFT_WPS, puc_app_ie,
                                    (oal_int32)ul_app_ie_len);
    if ((puc_wps_ie == OAL_PTR_NULL) || (puc_wps_ie[1] < MAC_MIN_WPS_IE_LEN)) {
        *pus_ie_len = 0;
        return;
    }

    /* 将WPS ie 信息拷贝到buffer 中 */
    ret = memcpy_s(puc_buffer, puc_wps_ie[1] + MAC_IE_HDR_LEN, puc_wps_ie, puc_wps_ie[1] + MAC_IE_HDR_LEN);
    if (ret != EOK) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TX, "mac_add_wps_ie::memcopy_s return value %d", ret);
        return;
    }
    *pus_ie_len = puc_wps_ie[1] + MAC_IE_HDR_LEN;

    return;
}

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

wlan_pmf_cap_status_uint8 mac_get_pmf_cap(oal_uint8 *puc_ie, oal_uint32 ul_ie_len)
{
    oal_uint8  *puc_rsn_ie;
    oal_uint16  us_rsn_cap;

    if (OAL_UNLIKELY(puc_ie == OAL_PTR_NULL)) {
        return MAC_PMF_DISABLED;
    }

    /* 查找RSN信息元素,如果没有RSN信息元素,则按照不支持处理 */
    puc_rsn_ie = mac_find_ie(MAC_EID_RSN, puc_ie, (oal_int32)(ul_ie_len));
    if (puc_rsn_ie == OAL_PTR_NULL) {
        return MAC_PMF_DISABLED;
    }

    /* 根据RSN信息元素, 判断RSN能力是否匹配 */
    us_rsn_cap = mac_get_rsn_capability(puc_rsn_ie);
    if ((us_rsn_cap & BIT6) && (us_rsn_cap & BIT7)) {
        return MAC_PME_REQUIRED;
    }

    if (us_rsn_cap & BIT7) {
        return MAC_PMF_ENABLED;
    }
    return MAC_PMF_DISABLED;
}
#endif // #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
#endif
#ifdef _PRE_WLAN_FEATURE_11R

oal_void mac_set_md_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    mac_vap_stru *pst_mac_vap = (mac_vap_stru *)pst_vap;

    if (pst_mac_vap->pst_mib_info->st_wlan_mib_fast_bss_trans_cfg.en_dot11FastBSSTransitionActivated != OAL_TRUE) {
        *puc_ie_len = 0;
        return;
    }

    /*
     *           ---------------------------------------------------
     *           |Element ID | Length | MDID| FT Capbility & Policy|
     *           ---------------------------------------------------
     *  Octets:  |1          | 1      | 2   |  1                   |
     *           ---------------------------------------------------
     */

    *puc_buffer       = MAC_EID_MOBILITY_DOMAIN;
    *(puc_buffer + 1) = 3;
    *(puc_buffer + 2) = pst_mac_vap->pst_mib_info->st_wlan_mib_fast_bss_trans_cfg.auc_dot11FTMobilityDomainID[0];
    *(puc_buffer + 3) = pst_mac_vap->pst_mib_info->st_wlan_mib_fast_bss_trans_cfg.auc_dot11FTMobilityDomainID[1];
    *(puc_buffer + 4) = 0;

    /*
     *        ------------------------------------------
     *        |FT over DS| RRP Capability | Reserved   |
     *        ------------------------------------------
     * Bits:  |1         | 1              | 6          |
     *        ------------------------------------------
     */
    if (pst_mac_vap->pst_mib_info->st_wlan_mib_fast_bss_trans_cfg.en_dot11FTOverDSActivated == OAL_TRUE) {
        *(puc_buffer + 4) += 1;
    }
    if (pst_mac_vap->pst_mib_info->st_wlan_mib_fast_bss_trans_cfg.en_dot11FTResourceRequestSupported == OAL_TRUE) {
        *(puc_buffer + 4) += 2;
    }
    *puc_ie_len = 5;
}


oal_void mac_set_ft_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint16 *pus_ie_len)
{
    mac_vap_stru *pst_mac_vap = (mac_vap_stru *)pst_vap;

    if (pst_mac_vap->pst_mib_info->st_wlan_mib_fast_bss_trans_cfg.en_dot11FastBSSTransitionActivated != OAL_TRUE) {
        *pus_ie_len = 0;
        return;
    }

    /*
     *         ------------------------------------------------------------------------------
     *         |Element ID | Length | MIC Control | MIC | ANonce | SNonce | Optional Params |
     *         ------------------------------------------------------------------------------
     * Octets: |1          | 1      | 2           |  16 |   32   |   32   |   variable      |
     *         ------------------------------------------------------------------------------
     */

    *puc_buffer       = MAC_EID_FT;

    *pus_ie_len = 84;
}

#endif // _PRE_WLAN_FEATURE_11R

uint32_t mac_vap_set_cb_tx_user_idx(mac_vap_stru *mac_vap, mac_tx_ctl_stru *tx_ctl, const unsigned char *data)
{
    uint16_t user_idx = MAC_INVALID_USER_ID;
    uint32_t ret;

    ret = mac_vap_find_user_by_macaddr(mac_vap, data, &user_idx);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG4(mac_vap->uc_vap_id, OAM_SF_ANY,
            "{mac_vap_set_cb_tx_user_idx:: cannot find user_idx from xx:xx:xx:%x:%x:%x, set TX_USER_IDX %d.}",
            data[3], data[4], data[5], MAC_INVALID_USER_ID);
        MAC_GET_CB_TX_USER_IDX(tx_ctl) = MAC_INVALID_USER_ID;
        return ret;
    }
    OAM_WARNING_LOG1(mac_vap->uc_vap_id, OAM_SF_ANY, "{mac_vap_set_cb_tx_user_idx:: set TX_USER_IDX %d.}", user_idx);
    MAC_GET_CB_TX_USER_IDX(tx_ctl) = user_idx;
    return OAL_SUCC;
}

/*lint -e19*/
oal_module_symbol(mac_find_ie);
oal_module_symbol(mac_find_vendor_ie);
oal_module_symbol(mac_set_aid_ie);
oal_module_symbol(mac_set_supported_rates_ie);
oal_module_symbol(mac_set_ext_capabilities_ie);
oal_module_symbol(mac_set_ssid_ie);
oal_module_symbol(mac_set_exsup_rates_ie);
oal_module_symbol(mac_set_cap_info_ap);
oal_module_symbol(mac_set_cap_info_sta);
oal_module_symbol(mac_set_wmm_params_ie);
oal_module_symbol(mac_set_wmm_ie_sta);
oal_module_symbol(mac_set_ht_capabilities_ie);
oal_module_symbol(mac_set_obss_scan_params);
oal_module_symbol(mac_set_ht_opern_ie);
oal_module_symbol(mac_set_listen_interval_ie);
oal_module_symbol(mac_set_status_code_ie);
oal_module_symbol(mac_set_power_cap_ie);
oal_module_symbol(mac_set_supported_channel_ie);
oal_module_symbol(mac_set_rsn_ie);
oal_module_symbol(mac_set_wpa_ie);
oal_module_symbol(mac_set_vht_capabilities_ie);
oal_module_symbol(mac_set_vht_opern_ie);
oal_module_symbol(mac_get_dtim_period);
oal_module_symbol(mac_get_dtim_cnt);
oal_module_symbol(mac_is_wmm_ie);
oal_module_symbol(mac_check_mac_privacy);
oal_module_symbol(mac_get_obss_scan_ie);
oal_module_symbol(g_auc_rsn_oui);
oal_module_symbol(g_auc_wpa_oui);
oal_module_symbol(mac_set_timeout_interval_ie);
oal_module_symbol(mac_add_app_ie);
oal_module_symbol(mac_rx_report_80211_frame);
oal_module_symbol(mac_set_vendor_hisi_ie);
oal_module_symbol(mac_set_11ntxbf_vendor_ie);
oal_module_symbol(mac_get_wmm_ie);
oal_module_symbol(mac_get_rsn_capability);

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
oal_module_symbol(mac_get_pmf_cap);
#endif
/*lint +e19*/

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

