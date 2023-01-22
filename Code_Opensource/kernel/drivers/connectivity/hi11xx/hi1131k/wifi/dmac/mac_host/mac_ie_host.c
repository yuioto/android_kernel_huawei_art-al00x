

/* 1 头文件包含 */
#include "dmac_ext_if.h"
#include "mac_ie.h"
#include "mac_frame.h"
#include "mac_device.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_IE_HOST_C

/* 2 全局变量定义 */
/* 3 函数实现 */
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

oal_void  mac_ie_get_vht_rx_mcs_map(mac_rx_max_mcs_map_stru  *pst_mac_rx_mcs_sta,
                                    mac_rx_max_mcs_map_stru  *pst_mac_rx_mcs_ap)
{
    oal_uint16      *pus_rx_mcs_sta;

    /* 获取空间流1及空间流2的能力信息，目前1151最多支持2个空间流 */
    if ((pst_mac_rx_mcs_sta->us_max_mcs_1ss != 0x3) && (pst_mac_rx_mcs_ap->us_max_mcs_1ss != 0x3)) {
        pst_mac_rx_mcs_sta->us_max_mcs_1ss = pst_mac_rx_mcs_sta->us_max_mcs_1ss > pst_mac_rx_mcs_ap->us_max_mcs_1ss ?
        pst_mac_rx_mcs_ap->us_max_mcs_1ss : pst_mac_rx_mcs_sta->us_max_mcs_1ss;
    } else {
        pst_mac_rx_mcs_sta->us_max_mcs_1ss = 0x3;
    }

    if ((pst_mac_rx_mcs_sta->us_max_mcs_2ss != 0x3) && (pst_mac_rx_mcs_ap->us_max_mcs_2ss != 0x3)) {
        pst_mac_rx_mcs_sta->us_max_mcs_2ss = pst_mac_rx_mcs_sta->us_max_mcs_2ss > pst_mac_rx_mcs_ap->us_max_mcs_2ss ?
        pst_mac_rx_mcs_ap->us_max_mcs_2ss : pst_mac_rx_mcs_sta->us_max_mcs_2ss;
    } else {
        pst_mac_rx_mcs_sta->us_max_mcs_2ss = 0x3;
    }

    /* 限制最大的空间流数目 */
    pus_rx_mcs_sta = (oal_uint16 *)pst_mac_rx_mcs_sta;

    *pus_rx_mcs_sta = (*pus_rx_mcs_sta) | 0xFFF0;
}


oal_bool_enum_uint8 mac_ie_proc_ht_supported_channel_width(
    mac_user_stru    *pst_mac_user_sta,
    mac_vap_stru     *pst_mac_vap,
    oal_uint8         channel_width,
    oal_bool_enum     en_prev_asoc_ht)
{
    /* 不支持20/40Mhz频宽 */
    if (channel_width == 0) {
        if ((en_prev_asoc_ht == OAL_FALSE) || (pst_mac_user_sta->st_ht_hdl.bit_supported_channel_width == OAL_TRUE)) {
            pst_mac_vap->st_protection.uc_sta_20M_only_num++;
        }
        return OAL_FALSE;
    } else { /* 支持20/40Mhz频宽 */
        /* 如果STA之前已经作为不支持20/40Mhz频宽的HT站点与AP关联 */
        if ((en_prev_asoc_ht == OAL_TRUE) && (pst_mac_user_sta->st_ht_hdl.bit_supported_channel_width == OAL_FALSE)) {
            pst_mac_vap->st_protection.uc_sta_20M_only_num--;
        }

        return OAL_TRUE;
    }
}


oal_bool_enum_uint8 mac_ie_proc_ht_green_field(
    mac_user_stru    *pst_mac_user_sta,
    mac_vap_stru     *pst_mac_vap,
    oal_uint8         uc_ht_green_field,
    oal_bool_enum     en_prev_asoc_ht)
{
    /* 不支持Greenfield */
    if (uc_ht_green_field == 0) {
        if ((en_prev_asoc_ht == OAL_FALSE) || (pst_mac_user_sta->st_ht_hdl.bit_ht_green_field == OAL_TRUE)) {
            pst_mac_vap->st_protection.uc_sta_non_gf_num++;
        }
        return OAL_FALSE;
    } else { /* 支持Greenfield */
        /* 如果STA之前已经作为不支持GF的HT站点与AP关联 */
        if ((en_prev_asoc_ht == OAL_TRUE) && (pst_mac_user_sta->st_ht_hdl.bit_ht_green_field == OAL_FALSE)) {
            pst_mac_vap->st_protection.uc_sta_non_gf_num--;
        }

        return OAL_TRUE;
    }
}


oal_bool_enum_uint8 mac_ie_proc_lsig_txop_protection_support(
    mac_user_stru    *pst_mac_user_sta,
    mac_vap_stru     *pst_mac_vap,
    oal_uint8         txop_support,
    oal_bool_enum     en_prev_asoc_ht)
{
    /* 不支持L-sig txop protection */
    if (txop_support == 0) {
        if ((en_prev_asoc_ht == OAL_FALSE) || (pst_mac_user_sta->st_ht_hdl.bit_lsig_txop_protection == OAL_TRUE)) {
            pst_mac_vap->st_protection.uc_sta_no_lsig_txop_num++;
        }

        return OAL_FALSE;
    } else { /* 支持L-sig txop protection */
        /* 如果STA之前已经作为不支持Lsig txop protection的HT站点与AP关联 */
        if ((en_prev_asoc_ht == OAL_TRUE) && (pst_mac_user_sta->st_ht_hdl.bit_lsig_txop_protection == OAL_FALSE)) {
            pst_mac_vap->st_protection.uc_sta_no_lsig_txop_num--;
        }

        return OAL_TRUE;
    }
}


oal_uint32  mac_ie_proc_ht_sta(
    mac_vap_stru            *pst_mac_sta,
    oal_uint8                *puc_payload,
    oal_uint16               *pus_index,
    mac_user_stru           *pst_mac_user_ap,
    oal_uint16               *pus_ht_cap_info,
    oal_uint16               *pus_amsdu_maxsize)
{
    oal_uint8                           uc_mcs_bmp_index;
    oal_uint8                           uc_smps;
    oal_uint16                          us_offset;
    mac_user_ht_hdl_stru               *pst_ht_hdl = OAL_PTR_NULL;
    mac_user_ht_hdl_stru                st_ht_hdl;
    mac_user_stru                      *pst_mac_user = OAL_PTR_NULL;
    oal_uint16        us_tmp_info_elem;
    oal_uint16        us_tmp_txbf_low;
    oal_uint32        ul_tmp_txbf_elem;

    if ((pst_mac_sta == OAL_PTR_NULL) || (puc_payload == OAL_PTR_NULL) ||
        (pus_index == OAL_PTR_NULL) || (pst_mac_user_ap == OAL_PTR_NULL) || (pus_ht_cap_info == OAL_PTR_NULL) ||
        (pus_amsdu_maxsize == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_ie_proc_ht_sta::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    us_offset       = *pus_index;
    pst_mac_user    = pst_mac_user_ap;
    pst_ht_hdl      = &st_ht_hdl;
    mac_user_get_ht_hdl(pst_mac_user, pst_ht_hdl);

    /* 带有 HT Capability Element 的 AP，标示它具有HT capable. */
    pst_ht_hdl->en_ht_capable = OAL_TRUE;

    us_offset += MAC_IE_HDR_LEN;

    /********************************************/
    /*     解析 HT Capabilities Info Field      */
    /********************************************/
    *pus_ht_cap_info = OAL_MAKE_WORD16(puc_payload[us_offset], puc_payload[us_offset + 1]);

    /* 检查STA所支持的LDPC编码能力 B0，0:不支持，1:支持 */
    pst_ht_hdl->bit_ldpc_coding_cap = (*pus_ht_cap_info & BIT0);

    /* 提取AP所支持的带宽能力  */
    pst_ht_hdl->bit_supported_channel_width = ((*pus_ht_cap_info & BIT1) >> 1);

    /* 检查空间复用节能模式 B2~B3 */
    uc_smps = (*pus_ht_cap_info & (BIT2 | BIT3));
    pst_ht_hdl->bit_sm_power_save = mac_ie_proc_sm_power_save_field(pst_mac_user, uc_smps);

    /* 提取AP支持Greenfield情况 */
    pst_ht_hdl->bit_ht_green_field = ((*pus_ht_cap_info & BIT4) >> 4);

    /* 提取AP支持20MHz Short-GI情况 */
    pst_ht_hdl->bit_short_gi_20mhz = ((*pus_ht_cap_info & BIT5) >> 5);

    /* 提取AP支持40MHz Short-GI情况 */
    pst_ht_hdl->bit_short_gi_40mhz = ((*pus_ht_cap_info & BIT6) >> 6);

    /* 提取AP支持STBC PPDU情况 */
    pst_ht_hdl->bit_rx_stbc = (oal_uint8)((*pus_ht_cap_info & 0x30) >> 4);

    /* 提取AP支持最大A-MSDU长度情况 */
    if ((*pus_ht_cap_info & BIT11) == 0) {
        *pus_amsdu_maxsize = WLAN_MIB_MAX_AMSDU_LENGTH_SHORT;
    } else {
        *pus_amsdu_maxsize = WLAN_MIB_MAX_AMSDU_LENGTH_LONG;
    }

    /* 提取AP 40M上DSSS/CCK的支持情况 */
    pst_ht_hdl->bit_dsss_cck_mode_40mhz = ((*pus_ht_cap_info & BIT12) >> 12);

    /* 提取AP L-SIG TXOP 保护的支持情况 */
    pst_ht_hdl->bit_lsig_txop_protection = ((*pus_ht_cap_info & BIT15) >> 15);

    us_offset += MAC_HT_CAPINFO_LEN;

    /********************************************/
    /*     解析 A-MPDU Parameters Field         */
    /********************************************/

    /* 提取 Maximum Rx A-MPDU factor (B1 - B0) */
    pst_ht_hdl->uc_max_rx_ampdu_factor = (puc_payload[us_offset] & 0x03);

    /* 提取 Minmum Rx A-MPDU factor (B3 - B2) */
    pst_ht_hdl->uc_min_mpdu_start_spacing = (puc_payload[us_offset] >> 2) & 0x07;

    us_offset += MAC_HT_AMPDU_PARAMS_LEN;

    /********************************************/
    /*     解析 Supported MCS Set Field         */
    /********************************************/
    for (uc_mcs_bmp_index = 0; uc_mcs_bmp_index < WLAN_HT_MCS_BITMASK_LEN; uc_mcs_bmp_index++) {
        pst_ht_hdl->uc_rx_mcs_bitmask[uc_mcs_bmp_index] =
            (pst_mac_sta->pst_mib_info->st_supported_mcstx.auc_dot11SupportedMCSTxValue[uc_mcs_bmp_index]) &
            (*(oal_uint8 *)(puc_payload + us_offset + uc_mcs_bmp_index));
    }

    pst_ht_hdl->uc_rx_mcs_bitmask[WLAN_HT_MCS_BITMASK_LEN - 1] &= 0x1F;

    us_offset += MAC_HT_SUP_MCS_SET_LEN;

    /********************************************/
    /* 解析 HT Extended Capabilities Info Field */
    /********************************************/
    *pus_ht_cap_info = OAL_MAKE_WORD16(puc_payload[us_offset], puc_payload[us_offset + 1]);

    /* 提取 HTC support Information */
    if ((*pus_ht_cap_info & BIT10) != 0) {
        pst_ht_hdl->uc_htc_support = 1;
    }
    us_offset += MAC_HT_EXT_CAP_LEN;

    /********************************************/
    /*  解析 Tx Beamforming Field               */
    /********************************************/
    us_tmp_info_elem = OAL_MAKE_WORD16(puc_payload[us_offset], puc_payload[us_offset + 1]);
    us_tmp_txbf_low = OAL_MAKE_WORD16(puc_payload[us_offset + 2], puc_payload[us_offset + 3]);
    ul_tmp_txbf_elem = OAL_MAKE_WORD32(us_tmp_info_elem, us_tmp_txbf_low);
    pst_ht_hdl->bit_imbf_receive_cap                = (ul_tmp_txbf_elem & BIT0);
    pst_ht_hdl->bit_receive_staggered_sounding_cap  = ((ul_tmp_txbf_elem & BIT1) >> 1);
    pst_ht_hdl->bit_transmit_staggered_sounding_cap = ((ul_tmp_txbf_elem & BIT2) >> 2);
    pst_ht_hdl->bit_receive_ndp_cap                 = ((ul_tmp_txbf_elem & BIT3) >> 3);
    pst_ht_hdl->bit_transmit_ndp_cap                = ((ul_tmp_txbf_elem & BIT4) >> 4);
    pst_ht_hdl->bit_imbf_cap                        = ((ul_tmp_txbf_elem & BIT5) >> 5);
    pst_ht_hdl->bit_calibration                     = ((ul_tmp_txbf_elem & 0x000000C0) >> 6);
    pst_ht_hdl->bit_exp_csi_txbf_cap                = ((ul_tmp_txbf_elem & BIT8) >> 8);
    pst_ht_hdl->bit_exp_noncomp_txbf_cap            = ((ul_tmp_txbf_elem & BIT9) >> 9);
    pst_ht_hdl->bit_exp_comp_txbf_cap               = ((ul_tmp_txbf_elem & BIT10) >> 10);
    pst_ht_hdl->bit_exp_csi_feedback                = ((ul_tmp_txbf_elem & 0x00001800) >> 11);
    pst_ht_hdl->bit_exp_noncomp_feedback            = ((ul_tmp_txbf_elem & 0x00006000) >> 13);
    pst_ht_hdl->bit_exp_comp_feedback               = ((ul_tmp_txbf_elem & 0x0001C000) >> 15);
    pst_ht_hdl->bit_min_grouping                    = ((ul_tmp_txbf_elem & 0x00060000) >> 17);
    pst_ht_hdl->bit_csi_bfer_ant_number             = ((ul_tmp_txbf_elem & 0x001C0000) >> 19);
    pst_ht_hdl->bit_noncomp_bfer_ant_number         = ((ul_tmp_txbf_elem & 0x00600000) >> 21);
    pst_ht_hdl->bit_comp_bfer_ant_number            = ((ul_tmp_txbf_elem & 0x01C00000) >> 23);
    pst_ht_hdl->bit_csi_bfee_max_rows               = ((ul_tmp_txbf_elem & 0x06000000) >> 25);
    pst_ht_hdl->bit_channel_est_cap                 = ((ul_tmp_txbf_elem & 0x18000000) >> 27);

    mac_user_set_ht_hdl(pst_mac_user, pst_ht_hdl);

    return OAL_SUCC;
}


oal_bool_enum_uint8 mac_ie_check_p2p_action(oal_uint8 *puc_payload)
{
    /* 找到WFA OUI */
    if ((oal_memcmp(puc_payload, g_auc_p2p_oui, MAC_OUI_LEN) == 0) &&
        (puc_payload[MAC_OUI_LEN] == MAC_OUITYPE_P2P)) {
        /*  找到WFA P2P v1.0 oui type */
        return OAL_TRUE;
    }

    return OAL_FALSE;
}


wlan_mib_mimo_power_save_enum_uint8 mac_ie_proc_sm_power_save_field(mac_user_stru *pst_mac_user, oal_uint8 uc_smps)
{
    if (uc_smps == MAC_SMPS_STATIC_MODE) {
        return WLAN_MIB_MIMO_POWER_SAVE_STATIC;
    } else if (uc_smps == MAC_SMPS_DYNAMIC_MODE) {
        return WLAN_MIB_MIMO_POWER_SAVE_DYNAMIC;
    } else {
        return WLAN_MIB_MIMO_POWER_SAVE_MIMO;
    }
}


oal_uint32  mac_ie_proc_ext_cap_ie(mac_user_stru *pst_mac_user, oal_uint8 *puc_payload)
{
    /* 目前无实际操作任务，此函数注空 */
    return OAL_SUCC;
}
#endif //#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)


oal_uint8  mac_ie_get_chan_num(oal_uint8 *puc_frame_body, oal_uint16 us_frame_len, oal_uint16 us_offset,
                               oal_uint8 uc_curr_chan)
{
    oal_uint8   uc_chan_num = 0;
    oal_uint8  *puc_ie_start_addr;

    /* 在DSSS Param set ie中解析chan num */
    puc_ie_start_addr = mac_find_ie(MAC_EID_DSPARMS, puc_frame_body + us_offset, us_frame_len - us_offset);
    if ((puc_ie_start_addr != OAL_PTR_NULL) && (puc_ie_start_addr[1] == MAC_DSPARMS_LEN)) {
        uc_chan_num = puc_ie_start_addr[2];
        if (mac_is_channel_num_valid(mac_get_band_by_channel_num(uc_chan_num), uc_chan_num) == OAL_SUCC) {
            return  uc_chan_num;
        }
    }

    /* 在HT operation ie中解析 chan num */
    puc_ie_start_addr = mac_find_ie(MAC_EID_HT_OPERATION, puc_frame_body + us_offset, us_frame_len - us_offset);
    if ((puc_ie_start_addr != OAL_PTR_NULL) && (puc_ie_start_addr[1] >= 1)) {
        uc_chan_num = puc_ie_start_addr[2];
        if (mac_is_channel_num_valid(mac_get_band_by_channel_num(uc_chan_num), uc_chan_num) == OAL_SUCC) {
            return uc_chan_num;
        }
    }

    uc_chan_num = uc_curr_chan;
    return uc_chan_num;
}

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY

oal_uint32  mac_check_is_assoc_frame(oal_uint8 uc_mgmt_frm_type)
{
    if ((uc_mgmt_frm_type == WLAN_FC0_SUBTYPE_ASSOC_RSP) ||
        (uc_mgmt_frm_type == WLAN_FC0_SUBTYPE_REASSOC_REQ) ||
        (uc_mgmt_frm_type == WLAN_FC0_SUBTYPE_REASSOC_RSP) ||
        (uc_mgmt_frm_type == WLAN_FC0_SUBTYPE_ASSOC_REQ)) {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}
#endif


oal_uint32 mac_set_second_channel_offset_ie(wlan_channel_bandwidth_enum_uint8 en_bw,
                                            oal_uint8 *pauc_buffer,
                                            oal_uint8 *puc_output_len)
{
    if ((pauc_buffer == OAL_PTR_NULL) || (puc_output_len == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{mac_set_second_channel_offset_ie::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 默认输出为空 */
    *pauc_buffer    = '\0';
    *puc_output_len = 0;

    /* 11n 设置Secondary Channel Offset Element */
    /* -------------------------------------------------------------- */
    /* |Ele. ID |Length |Secondary channel offset |                   */
    /* -------------------------------------------------------------- */
    /* |1       |1      |1                        |                   */
    /*                                                                */
    pauc_buffer[0] = 62;
    pauc_buffer[1] = 1;

    switch (en_bw) {
        case WLAN_BAND_WIDTH_20M:
            pauc_buffer[2] = 0;  /* no secondary channel */
            break;

        case WLAN_BAND_WIDTH_40PLUS:
        case WLAN_BAND_WIDTH_80PLUSPLUS:
        case WLAN_BAND_WIDTH_80PLUSMINUS:
            pauc_buffer[2] = 1;  /* secondary 20M channel above */
            break;

        case WLAN_BAND_WIDTH_40MINUS:
        case WLAN_BAND_WIDTH_80MINUSPLUS:
        case WLAN_BAND_WIDTH_80MINUSMINUS:
            pauc_buffer[2] = 3;  /* secondary 20M channel below */
            break;

        default:
            OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{mac_set_second_channel_offset_ie::invalid bandwidth[%d].}", en_bw);

            return OAL_FAIL;
    }

    *puc_output_len = 3;

    return OAL_SUCC;
}


oal_uint32  mac_set_11ac_wideband_ie(oal_uint8 uc_channel,
                                     wlan_channel_bandwidth_enum_uint8 en_bw,
                                     oal_uint8 *pauc_buffer,
                                     oal_uint8 *puc_output_len)
{
    if ((pauc_buffer == OAL_PTR_NULL) || (puc_output_len == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{mac_set_11ac_wideband_ie::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 默认输出为空 */
    *pauc_buffer     = '\0';
    *puc_output_len  = 0;

    /* 11ac 设置Wide Bandwidth Channel Switch Element                 */
    /* -------------------------------------------------------------- */
    /* |ID |Length |New Ch width |Center Freq seg1 |Center Freq seg2  */
    /* -------------------------------------------------------------- */
    /* |1  |1      |1            |1                |1                 */
    /*                                                                */
    pauc_buffer[0] = 194;
    pauc_buffer[1] = 3;
    switch (en_bw) {
        case WLAN_BAND_WIDTH_20M:
        case WLAN_BAND_WIDTH_40PLUS:
        case WLAN_BAND_WIDTH_40MINUS:
            pauc_buffer[2] = 0;
            pauc_buffer[3] = 0;
            break;

        case WLAN_BAND_WIDTH_80PLUSPLUS:
            pauc_buffer[2] = 1;
            pauc_buffer[3] = uc_channel + 6;
            break;

        case WLAN_BAND_WIDTH_80PLUSMINUS:
            pauc_buffer[2] = 1;
            pauc_buffer[3] = uc_channel - 2;
            break;

        case WLAN_BAND_WIDTH_80MINUSPLUS:
            pauc_buffer[2] = 1;
            pauc_buffer[3] = uc_channel + 2;
            break;

        case WLAN_BAND_WIDTH_80MINUSMINUS:
            pauc_buffer[2] = 1;
            pauc_buffer[3] = uc_channel - 6;
            break;

        default:
            OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{mac_set_11ac_wideband_ie::invalid bandwidth[%d].}", en_bw);
            return OAL_FAIL;
    }

    pauc_buffer[4] = 0; /* reserved. Not support 80M + 80M */
    *puc_output_len = 5;

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

oal_uint32  mac_ie_proc_chwidth_field(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user, oal_uint8 uc_chwidth)
{
    wlan_bw_cap_enum_uint8      en_bwcap_vap = 0; /* vap自身带宽能力 */
    wlan_bw_cap_enum_uint8      en_bwcap_user; /* user之前的带宽信息 */

    if (OAL_UNLIKELY((pst_mac_user == OAL_PTR_NULL) || (pst_mac_vap == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_2040, "{mac_ie_proc_opmode_field::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    en_bwcap_user = pst_mac_user->en_avail_bandwidth;

    mac_vap_get_bandwidth_cap(pst_mac_vap, &en_bwcap_vap);
    en_bwcap_vap = OAL_MIN(en_bwcap_vap, (wlan_bw_cap_enum_uint8)uc_chwidth);
    mac_user_set_bandwidth_info(pst_mac_user, en_bwcap_vap, en_bwcap_vap);

    return OAL_SUCC;
}
#endif //#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
#endif


oal_uint32  mac_config_set_mib(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_set_mib_stru   *pst_set_mib;
    oal_uint32              ul_ret = OAL_SUCC;

    pst_set_mib = (mac_cfg_set_mib_stru *)puc_param;

    switch (pst_set_mib->ul_mib_idx) {
        case WLAN_MIB_INDEX_LSIG_TXOP_PROTECTION_OPTION_IMPLEMENTED:
            pst_mac_vap->pst_mib_info->st_wlan_mib_ht_sta_cfg.en_dot11LsigTxopProtectionOptionImplemented =
                (oal_uint8)(pst_set_mib->ul_mib_value);
            break;

        case WLAN_MIB_INDEX_HT_GREENFIELD_OPTION_IMPLEMENTED:
            pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11HTGreenfieldOptionImplemented =
                (oal_uint8)(pst_set_mib->ul_mib_value);
            break;

        case WLAN_MIB_INDEX_SPEC_MGMT_IMPLEMENT:
            pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11SpectrumManagementImplemented =
                (oal_bool_enum_uint8)(pst_set_mib->ul_mib_value);
            break;

        case WLAN_MIB_INDEX_FORTY_MHZ_OPERN_IMPLEMENT:
            mac_mib_set_FortyMHzOperationImplemented(pst_mac_vap, (oal_bool_enum_uint8)(pst_set_mib->ul_mib_value));
            break;

        case WLAN_MIB_INDEX_2040_COEXT_MGMT_SUPPORT:
            pst_mac_vap->pst_mib_info->st_wlan_mib_operation.en_dot112040BSSCoexistenceManagementSupport =
                (oal_bool_enum_uint8)(pst_set_mib->ul_mib_value);
            break;

        case WLAN_MIB_INDEX_FORTY_MHZ_INTOL:
            pst_mac_vap->pst_mib_info->st_wlan_mib_operation.en_dot11FortyMHzIntolerant =
                (oal_bool_enum_uint8)(pst_set_mib->ul_mib_value);
            break;

        case WLAN_MIB_INDEX_VHT_CHAN_WIDTH_OPTION_IMPLEMENT:
            pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.uc_dot11VHTChannelWidthOptionImplemented =
                (oal_uint8)(pst_set_mib->ul_mib_value);
            break;

        case WLAN_MIB_INDEX_MINIMUM_MPDU_STARTING_SPACING:
            pst_mac_vap->pst_mib_info->st_wlan_mib_ht_sta_cfg.ul_dot11MinimumMPDUStartSpacing =
                (oal_uint8)(pst_set_mib->ul_mib_value);
            break;

        case WLAN_MIB_INDEX_OBSSSCAN_TRIGGER_INTERVAL:
            pst_mac_vap->pst_mib_info->st_wlan_mib_operation.ul_dot11BSSWidthTriggerScanInterval =
                pst_set_mib->ul_mib_value;
            break;

        case WLAN_MIB_INDEX_OBSSSCAN_TRANSITION_DELAY_FACTOR:
            pst_mac_vap->pst_mib_info->st_wlan_mib_operation.ul_dot11BSSWidthChannelTransitionDelayFactor =
                pst_set_mib->ul_mib_value;
            break;

        case WLAN_MIB_INDEX_OBSSSCAN_PASSIVE_DWELL:
            pst_mac_vap->pst_mib_info->st_wlan_mib_operation.ul_dot11OBSSScanPassiveDwell = pst_set_mib->ul_mib_value;
            break;

        case WLAN_MIB_INDEX_OBSSSCAN_ACTIVE_DWELL:
            pst_mac_vap->pst_mib_info->st_wlan_mib_operation.ul_dot11OBSSScanActiveDwell = pst_set_mib->ul_mib_value;
            break;

        case WLAN_MIB_INDEX_OBSSSCAN_PASSIVE_TOTAL_PER_CHANNEL:
            pst_mac_vap->pst_mib_info->st_wlan_mib_operation.ul_dot11OBSSScanPassiveTotalPerChannel =
                pst_set_mib->ul_mib_value;
            break;

        case WLAN_MIB_INDEX_OBSSSCAN_ACTIVE_TOTAL_PER_CHANNEL:
            pst_mac_vap->pst_mib_info->st_wlan_mib_operation.ul_dot11OBSSScanActiveTotalPerChannel =
                pst_set_mib->ul_mib_value;
            break;

        case WLAN_MIB_INDEX_OBSSSCAN_ACTIVITY_THRESHOLD:
            pst_mac_vap->pst_mib_info->st_wlan_mib_operation.ul_dot11OBSSScanActivityThreshold =
                pst_set_mib->ul_mib_value;
            break;

        default :
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_mib::invalid ul_mib_idx[%d].}",
                pst_set_mib->ul_mib_idx);
            break;
    }

    return ul_ret;
}

/*lint -e19*/
oal_module_symbol(mac_ie_proc_sm_power_save_field);
oal_module_symbol(mac_ie_proc_ht_green_field);
oal_module_symbol(mac_ie_get_chan_num);
oal_module_symbol(mac_ie_proc_ht_supported_channel_width);
oal_module_symbol(mac_ie_proc_lsig_txop_protection_support);
oal_module_symbol(mac_ie_proc_ext_cap_ie);
oal_module_symbol(mac_set_second_channel_offset_ie);
oal_module_symbol(mac_set_11ac_wideband_ie);
oal_module_symbol(mac_config_set_mib);
oal_module_symbol(mac_ie_proc_ht_sta);
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
oal_module_symbol(mac_ie_proc_chwidth_field);
#endif
oal_module_symbol(mac_ie_get_vht_rx_mcs_map);
/*lint +e19*/

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

