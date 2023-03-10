

// 1 头文件包含
#include "hmac_user.h"
#include "hmac_main.h"
#include "hmac_vap.h"
#include "hmac_protection.h"
#include "mac_vap.h"
#include "mac_ie.h"
#include "hmac_config.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_PROTECTION_C

// 2 函数实现
OAL_STATIC oal_uint32 hmac_protection_set_mode(
    mac_vap_stru *pst_mac_vap,
    wlan_prot_mode_enum_uint8 en_prot_mode);


OAL_STATIC OAL_INLINE oal_switch_enum_uint8 hmac_protection_is_autoprot_enabled(mac_vap_stru *pst_mac_vap)
{
    return pst_mac_vap->st_protection.bit_auto_protection;
}


oal_uint32 hmac_protection_set_autoprot(mac_vap_stru *pst_mac_vap, oal_switch_enum_uint8 en_mode)
{
    oal_uint32      ul_ret = OAL_SUCC;
    hmac_user_stru *pst_hmac_user = OAL_PTR_NULL;

    if (pst_mac_vap == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (en_mode == OAL_SWITCH_OFF) {
        pst_mac_vap->st_protection.bit_auto_protection = OAL_SWITCH_OFF;
        ul_ret = hmac_protection_set_mode(pst_mac_vap, WLAN_PROT_NO);
    } else {
        pst_mac_vap->st_protection.bit_auto_protection = OAL_SWITCH_ON;
        /* VAP 为 AP情况下 */
        if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
            ul_ret = hmac_protection_update_mode_ap(pst_mac_vap);
        } else if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) { /* VAP 为 STA情况下 */
            pst_hmac_user = mac_res_get_hmac_user(pst_mac_vap->uc_assoc_vap_id); /* user保存的是AP的信息 */
            if (pst_hmac_user == OAL_PTR_NULL) {
                return OAL_ERR_CODE_PTR_NULL;
            }

            ul_ret = hmac_protection_update_mode_sta(pst_mac_vap, pst_hmac_user);
        }
    }

    return ul_ret;
}


OAL_STATIC oal_uint32 hmac_protection_set_rtscts_mechanism(mac_vap_stru *pst_mac_vap, oal_switch_enum_uint8 en_flag,
    wlan_prot_mode_enum_uint8 en_prot_mode)
{
    oal_uint32 ul_ret;
    mac_cfg_rts_tx_param_stru st_rts_tx_param;

    /* 只有启用erp保护时候，RTS[0~2]速率才设为5.5Mpbs(11b), 其余时候都为24Mpbs(leagcy ofdm) */
    if ((en_prot_mode == WLAN_PROT_ERP) && (en_flag == OAL_SWITCH_ON)) {
        st_rts_tx_param.en_band = WLAN_BAND_2G;

        /* RTS[0~2]设为5.5Mbps, RTS[3]设为1Mbps */
        st_rts_tx_param.auc_protocol_mode[0] = WLAN_11B_PHY_PROTOCOL_MODE;
        st_rts_tx_param.auc_rate[0] = WLAN_LONG_11b_5_HALF_M_BPS;
        st_rts_tx_param.auc_protocol_mode[1] = WLAN_11B_PHY_PROTOCOL_MODE;
        st_rts_tx_param.auc_rate[1] = WLAN_LONG_11b_5_HALF_M_BPS;
        st_rts_tx_param.auc_protocol_mode[2] = WLAN_11B_PHY_PROTOCOL_MODE;
        st_rts_tx_param.auc_rate[2] = WLAN_LONG_11b_5_HALF_M_BPS;
        st_rts_tx_param.auc_protocol_mode[2] = WLAN_11B_PHY_PROTOCOL_MODE;
        st_rts_tx_param.auc_rate[2] = WLAN_LONG_11b_5_HALF_M_BPS;
        st_rts_tx_param.auc_protocol_mode[3] = WLAN_11B_PHY_PROTOCOL_MODE;
        st_rts_tx_param.auc_rate[3] = WLAN_LONG_11b_1_M_BPS;
    } else {
        st_rts_tx_param.en_band = pst_mac_vap->st_channel.en_band;

        /* RTS[0~2]设为24Mbps */
        st_rts_tx_param.auc_protocol_mode[0] = WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE;
        st_rts_tx_param.auc_rate[0] = WLAN_LEGACY_OFDM_24M_BPS;
        st_rts_tx_param.auc_protocol_mode[1] = WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE;
        st_rts_tx_param.auc_rate[1] = WLAN_LEGACY_OFDM_24M_BPS;
        st_rts_tx_param.auc_protocol_mode[2] = WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE;
        st_rts_tx_param.auc_rate[2] = WLAN_LEGACY_OFDM_24M_BPS;

        /* 2G的RTS[3]设为1Mbps */
        if (st_rts_tx_param.en_band == WLAN_BAND_2G) {
            st_rts_tx_param.auc_protocol_mode[3] = WLAN_11B_PHY_PROTOCOL_MODE;
            st_rts_tx_param.auc_rate[3] = WLAN_LONG_11b_1_M_BPS;
        } else { /* 5G的RTS[3]设为24Mbps */
            st_rts_tx_param.auc_protocol_mode[3] = WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE;
            st_rts_tx_param.auc_rate[3] = WLAN_LEGACY_OFDM_24M_BPS;
        }
    }

    ul_ret = hmac_config_set_rts_param(pst_mac_vap, OAL_SIZEOF(mac_cfg_rts_tx_param_stru),
        (oal_uint8*)(&st_rts_tx_param));

    /* 数据帧/管理帧发送时候，需要根据bit_rts_cts_protect_mode值填写发送描述符中的RTS/CTS enable位 */
    pst_mac_vap->st_protection.bit_rts_cts_protect_mode = en_flag;

    return ul_ret;
}


OAL_STATIC OAL_INLINE oal_void hmac_protection_set_lsig_txop_mechanism(mac_vap_stru *pst_mac_vap,
    oal_switch_enum_uint8 en_flag)
{
    /* 数据帧/管理帧发送时候，需要根据bit_lsig_txop_protect_mode值填写发送描述符中的L-SIG TXOP enable位 */
    pst_mac_vap->st_protection.bit_lsig_txop_protect_mode = en_flag;
}


OAL_STATIC OAL_INLINE oal_uint32 hmac_protection_set_erp_protection(mac_vap_stru *pst_mac_vap,
    oal_switch_enum_uint8 en_flag)
{
    oal_uint32 ul_ret;
    /* 1151只支持RTS-CTS机制来保护， 不支持Self-To-CTS机制 */
    ul_ret = hmac_protection_set_rtscts_mechanism(pst_mac_vap, en_flag, WLAN_PROT_ERP);

    return ul_ret;
}


OAL_STATIC oal_bool_enum hmac_protection_lsigtxop_check(mac_vap_stru *pst_mac_vap)
{
    mac_user_stru  *pst_mac_user = OAL_PTR_NULL;

    /* 如果不是11n站点，则不支持lsigtxop保护 */
    if ((pst_mac_vap->en_protocol != WLAN_HT_MODE) && (pst_mac_vap->en_protocol != WLAN_HT_ONLY_MODE) &&
        (pst_mac_vap->en_protocol != WLAN_HT_11G_MODE)) {
        return OAL_FALSE;
    }

    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        pst_mac_user = (mac_user_stru *)mac_res_get_mac_user(pst_mac_vap->uc_assoc_vap_id); /* user保存的是AP的信息 */
        if (pst_mac_user == OAL_PTR_NULL) {
            return OAL_FALSE;
        }
    }
    /*lint -e644*/
    /* BSS 中所有站点都支持Lsig txop protection, 则使用Lsig txop protection机制，开销小, AP和STA采用不同的判断 */
    if (((pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) &&
        (mac_mib_get_LsigTxopFullProtectionActivated(pst_mac_vap) == OAL_TRUE)) ||
        ((pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) && (pst_mac_user != OAL_PTR_NULL) &&
        (pst_mac_user->st_ht_hdl.bit_lsig_txop_protection_full_support == OAL_TRUE))) {
        return OAL_TRUE;
    } else {
        return OAL_FALSE;
    }
    /*lint +e644*/
}


OAL_STATIC oal_uint32 hmac_protection_update_ht_protection(mac_vap_stru *pst_mac_vap)
{
    oal_uint32    ul_ret      = OAL_SUCC;
    oal_bool_enum en_lsigtxop;

    if (pst_mac_vap == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 当前vap并没有设置ht 保护，直接返回 */
    if (pst_mac_vap->st_protection.en_protection_mode != WLAN_PROT_HT) {
        return OAL_SUCC;
    }

    en_lsigtxop = hmac_protection_lsigtxop_check(pst_mac_vap);
    /* 如果可以设置lsigtxop保护， 则优先设置lsigtxop保护 */
    if (en_lsigtxop == OAL_TRUE) {
        /* 如果启用了rts cts protection机制， 则更新为lsig txop protection机制 */
        if (pst_mac_vap->st_protection.bit_rts_cts_protect_mode == OAL_SWITCH_ON) {
            ul_ret = hmac_protection_set_rtscts_mechanism(pst_mac_vap, OAL_SWITCH_OFF, WLAN_PROT_HT);
            if (ul_ret != OAL_SUCC) {
                return ul_ret;
            }

            hmac_protection_set_lsig_txop_mechanism(pst_mac_vap, OAL_SWITCH_ON);
        }
    } else { /* 其余情况需要设置ht保护方式为rts cts protection 机制 */
        /* 如果启用了rts cts protection机制， 则更新为lsig txop protection机制 */
        if (pst_mac_vap->st_protection.bit_lsig_txop_protect_mode == OAL_SWITCH_ON) {
            hmac_protection_set_lsig_txop_mechanism(pst_mac_vap, OAL_SWITCH_OFF);
            ul_ret = hmac_protection_set_rtscts_mechanism(pst_mac_vap, OAL_SWITCH_ON, WLAN_PROT_HT);
            if (ul_ret != OAL_SUCC) {
                return ul_ret;
            }
        }
    }

    return ul_ret;
}


OAL_STATIC oal_uint32 hmac_protection_set_ht_protection(mac_vap_stru *pst_mac_vap, oal_switch_enum_uint8 en_flag)
{
    oal_uint32      ul_ret = OAL_SUCC;
    oal_bool_enum   en_lsigtxop;

    en_lsigtxop = hmac_protection_lsigtxop_check(pst_mac_vap);
    /* 优先使用lsigtxop保护，开销小 */
    if (en_lsigtxop == OAL_TRUE) {
        hmac_protection_set_lsig_txop_mechanism(pst_mac_vap, en_flag);
    } else {
        ul_ret = hmac_protection_set_rtscts_mechanism(pst_mac_vap, en_flag, WLAN_PROT_HT);
        if (ul_ret != OAL_SUCC) {
            return ul_ret;
        }
    }

    return ul_ret;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

oal_bool_enum_uint8 hmac_protection_need_sync(mac_vap_stru *pst_mac_vap, mac_h2d_protection_stru *pst_h2d_prot)
{
    mac_h2d_protection_stru *pst_prot_old = OAL_PTR_NULL;
    hmac_vap_stru       *pst_hmac_vap = OAL_PTR_NULL;
    mac_protection_stru *pst_old = OAL_PTR_NULL;
    mac_protection_stru *pst_new = OAL_PTR_NULL;
    oal_uint             l_ret   = EOK;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                       "{hmac_protection_need_sync::null hmac_vap}");
        return OAL_FALSE;
    }
    pst_prot_old = &pst_hmac_vap->st_prot;

    if ((pst_prot_old->en_dot11HTProtection != pst_h2d_prot->en_dot11HTProtection) ||
        (pst_prot_old->en_dot11RIFSMode != pst_h2d_prot->en_dot11RIFSMode) ||
        (pst_prot_old->en_dot11LSIGTXOPFullProtectionActivated !=
        pst_h2d_prot->en_dot11LSIGTXOPFullProtectionActivated) ||
        (pst_prot_old->en_dot11NonGFEntitiesPresent != pst_h2d_prot->en_dot11NonGFEntitiesPresent)) {
        l_ret += memcpy_s(pst_prot_old, OAL_SIZEOF(mac_h2d_protection_stru), pst_h2d_prot,
            OAL_SIZEOF(mac_h2d_protection_stru));
        return OAL_TRUE;
    }

    pst_old = &pst_prot_old->st_protection;
    pst_new = &pst_h2d_prot->st_protection;

    if ((pst_old->en_protection_mode != pst_new->en_protection_mode) ||
        (pst_old->bit_auto_protection != pst_new->bit_auto_protection) ||
        (pst_old->bit_obss_non_erp_present != pst_new->bit_obss_non_erp_present) ||
        (pst_old->bit_obss_non_ht_present != pst_new->bit_obss_non_ht_present) ||
        (pst_old->bit_rts_cts_protect_mode != pst_new->bit_rts_cts_protect_mode) ||
        (pst_old->bit_lsig_txop_protect_mode != pst_new->bit_lsig_txop_protect_mode) ||
        (pst_old->uc_sta_non_ht_num != pst_new->uc_sta_non_ht_num)) {
        l_ret += memcpy_s(pst_prot_old, OAL_SIZEOF(mac_h2d_protection_stru), pst_h2d_prot,
            OAL_SIZEOF(mac_h2d_protection_stru));
        if (l_ret != EOK) {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_protection_need_sync::memcpy_s failed !}");
            return OAL_FALSE;
        }
        return OAL_TRUE;
    }

    return OAL_FALSE;
}


OAL_STATIC oal_uint32 hmac_protection_sync_data(mac_vap_stru *pst_mac_vap)
{
    mac_h2d_protection_stru           st_h2d_prot;
    wlan_mib_Dot11OperationEntry_stru *pst_mib = OAL_PTR_NULL;
    oal_uint32                        ul_ret = OAL_SUCC;

    memset_s(&st_h2d_prot, OAL_SIZEOF(mac_h2d_protection_stru), 0, OAL_SIZEOF(mac_h2d_protection_stru));

    st_h2d_prot.ul_sync_mask |= H2D_SYNC_MASK_MIB;
    st_h2d_prot.ul_sync_mask |= H2D_SYNC_MASK_PROT;

    if (memcpy_s((oal_uint8*)&st_h2d_prot.st_protection, OAL_SIZEOF(mac_protection_stru),
        (oal_uint8 *)&pst_mac_vap->st_protection, OAL_SIZEOF(mac_protection_stru)) != EOK) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_protection_sync_data::memcpy_s failed !}");
        return OAL_FAIL;
    }
    pst_mib = &pst_mac_vap->pst_mib_info->st_wlan_mib_operation;
    st_h2d_prot.en_dot11HTProtection         = pst_mib->en_dot11HTProtection;
    st_h2d_prot.en_dot11RIFSMode             = pst_mib->en_dot11RIFSMode;
    st_h2d_prot.en_dot11LSIGTXOPFullProtectionActivated =
        pst_mib->en_dot11LSIGTXOPFullProtectionActivated;
    st_h2d_prot.en_dot11NonGFEntitiesPresent = pst_mib->en_dot11NonGFEntitiesPresent;

    if (hmac_protection_need_sync(pst_mac_vap, &st_h2d_prot) == OAL_TRUE) {
        ul_ret = hmac_config_set_protection(pst_mac_vap, OAL_SIZEOF(st_h2d_prot), (oal_uint8 *)&st_h2d_prot);
    }

    return ul_ret;
}
#endif


OAL_STATIC oal_uint32 hmac_protection_set_mode(mac_vap_stru *pst_mac_vap, wlan_prot_mode_enum_uint8 en_prot_mode)
{
    oal_uint32 ul_ret;

    if (en_prot_mode != pst_mac_vap->st_protection.en_protection_mode) {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_protection_sync_data::mode[%d]->mode[%d]}",
                         pst_mac_vap->st_protection.en_protection_mode, en_prot_mode);
    }

    /* 相同的保护模式已经被设置，直接返回 */
    if (en_prot_mode == pst_mac_vap->st_protection.en_protection_mode) {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        ul_ret = hmac_protection_sync_data(pst_mac_vap);
#endif
        return ul_ret;
    }

    /* 关闭之前的保护模式 */
    if (pst_mac_vap->st_protection.en_protection_mode == WLAN_PROT_ERP) {
        ul_ret = hmac_protection_set_erp_protection(pst_mac_vap, OAL_SWITCH_OFF);
        if (ul_ret != OAL_SUCC) {
            return ul_ret;
        }
    } else if (pst_mac_vap->st_protection.en_protection_mode == WLAN_PROT_HT) {
        ul_ret = hmac_protection_set_ht_protection(pst_mac_vap, OAL_SWITCH_OFF);
        if (ul_ret != OAL_SUCC) {
            return ul_ret;
        }
    } else {
        /* GF保护和无保护无需额外操作 */
    }

    pst_mac_vap->st_protection.en_protection_mode = en_prot_mode;

    /* 开启新的保护模式 */
    if (en_prot_mode == WLAN_PROT_ERP) {
        ul_ret = hmac_protection_set_erp_protection(pst_mac_vap, OAL_SWITCH_ON);
        if (ul_ret != OAL_SUCC) {
            return ul_ret;
        }
    } else if (en_prot_mode == WLAN_PROT_HT) {
        ul_ret = hmac_protection_set_ht_protection(pst_mac_vap, OAL_SWITCH_ON);
        if (ul_ret != OAL_SUCC) {
            return ul_ret;
        }
    } else {
        /* GF保护和无保护无需额外操作 */
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    ul_ret = hmac_protection_sync_data(pst_mac_vap);
#else
    /* 更新数据帧或管理帧与保护特性相关的发送参数 */
    hmac_config_update_protection_tx_param(pst_mac_vap, OAL_SIZEOF(ul_ret), (oal_uint8*)(&ul_ret)); /* 后面两个参数无作用 */
#endif

    return ul_ret;
}



oal_uint32 hmac_protection_update_mib_ap(mac_vap_stru *pst_mac_vap)
{
    oal_uint32          ul_ret;
    mac_protection_stru *pst_protection = OAL_PTR_NULL;
    oal_bool_enum_uint8 en_lsig_txop_full_protection_activated;
    oal_bool_enum_uint8 en_non_gf_entities_present;
    oal_bool_enum_uint8 en_rifs_mode;
    oal_bool_enum_uint8 en_ht_protection;

    if (pst_mac_vap == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_protection = &(pst_mac_vap->st_protection);

    /* 更新vap的en_dot11NonGFEntitiesPresent字段 */
    en_non_gf_entities_present = (pst_protection->uc_sta_non_gf_num != 0) ? OAL_TRUE : OAL_FALSE;
    mac_mib_set_NonGFEntitiesPresent(pst_mac_vap, en_non_gf_entities_present);

    /* 更新vap的en_dot11LSIGTXOPFullProtectionActivated字段 */
    en_lsig_txop_full_protection_activated = (pst_protection->uc_sta_no_lsig_txop_num == 0) ? OAL_TRUE : OAL_FALSE;

    mac_mib_set_LsigTxopFullProtectionActivated(pst_mac_vap, en_lsig_txop_full_protection_activated);

    ul_ret = hmac_protection_update_ht_protection(pst_mac_vap);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{hmac_protection_update_mib_ap::hmac_protection_update_ht_protection fail.err code %u}", ul_ret);
        return ul_ret;
    }

    /* 更新vap的en_dot11HTProtection和en_dot11RIFSMode字段 */
    if (pst_protection->uc_sta_non_ht_num != 0) {
        en_ht_protection = WLAN_MIB_HT_NON_HT_MIXED;
        en_rifs_mode     = OAL_FALSE;
    } else if (pst_protection->bit_obss_non_ht_present == OAL_TRUE) {
        en_ht_protection = WLAN_MIB_HT_NONMEMBER_PROTECTION;
        en_rifs_mode     = OAL_FALSE;
    } else if ((pst_mac_vap->st_channel.en_bandwidth != WLAN_BAND_WIDTH_20M) &&
        (pst_protection->uc_sta_20M_only_num != 0)) {
        en_ht_protection = WLAN_MIB_HT_20MHZ_PROTECTION;
        en_rifs_mode     = OAL_TRUE;
    } else {
        en_ht_protection = WLAN_MIB_HT_NO_PROTECTION;
        en_rifs_mode     = OAL_TRUE;
    }

    mac_mib_set_HtProtection(pst_mac_vap, en_ht_protection);
    mac_mib_set_RifsMode(pst_mac_vap, en_rifs_mode);

    ul_ret = hmac_protection_update_mode_ap(pst_mac_vap);
    return ul_ret;
}


oal_uint32 hmac_protection_update_mode_ap(mac_vap_stru *pst_mac_vap)
{
    oal_uint32                ul_ret;
    wlan_prot_mode_enum_uint8 en_protection_mode;
    mac_protection_stru       *pst_protection = OAL_PTR_NULL;

    if (pst_mac_vap == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_protection = &(pst_mac_vap->st_protection);

    /* 如果保护机制不启用， 直接返回 */
    if (hmac_protection_is_autoprot_enabled(pst_mac_vap) == OAL_SWITCH_OFF) {
        return OAL_SUCC;
    }

    /* 在2G频段下，如果有non erp站点与AP关联， 或者OBSS中存在non erp站点， 设置为erp保护 */
    if ((pst_mac_vap->st_channel.en_band == WLAN_BAND_2G) && ((pst_protection->uc_sta_non_erp_num != 0) ||
        (pst_protection->bit_obss_non_erp_present == OAL_TRUE))) {
        en_protection_mode = WLAN_PROT_ERP;
    } else if ((pst_protection->uc_sta_non_ht_num != 0) || (pst_protection->bit_obss_non_ht_present == OAL_TRUE)) {
        /* 如果有non ht站点与AP关联， 或者OBSS中存在non ht站点， 设置为ht保护 */
        en_protection_mode = WLAN_PROT_HT;
    } else if (pst_protection->uc_sta_non_gf_num != 0) { /* 如果有non gf站点与AP关联， 设置为gf保护 */
        en_protection_mode = WLAN_PROT_GF;
    } else { /* 剩下的情况不做保护 */
        en_protection_mode = WLAN_PROT_NO;
    }

    /* 设置具体保护模式 */
    ul_ret = hmac_protection_set_mode(pst_mac_vap, en_protection_mode);

    return ul_ret;
}


oal_uint32 hmac_protection_update_mode_sta(mac_vap_stru *pst_mac_vap_sta, hmac_user_stru *pst_hmac_user)
{
    oal_uint32                ul_ret;
    wlan_prot_mode_enum_uint8 en_protection_mode;
    mac_user_stru             *pst_mac_user = &pst_hmac_user->st_user_base_info;

    if ((pst_mac_vap_sta == OAL_PTR_NULL) || (pst_mac_user == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 如果保护机制不启用， 直接返回 */
    if (hmac_protection_is_autoprot_enabled(pst_mac_vap_sta) == OAL_SWITCH_OFF) {
        return OAL_SUCC;
    }

    /* 在2G频段下，如果AP发送的beacon帧ERP ie中Use Protection bit置为1，则将保护级别设置为ERP保护 */
    if ((pst_mac_vap_sta->st_channel.en_band == WLAN_BAND_2G) &&
        (pst_hmac_user->st_hmac_cap_info.bit_erp_use_protect == OAL_TRUE)) {
        en_protection_mode = WLAN_PROT_ERP;
    } else if ((pst_mac_user->st_ht_hdl.bit_HT_protection == WLAN_MIB_HT_NON_HT_MIXED) ||
               (pst_mac_user->st_ht_hdl.bit_HT_protection == WLAN_MIB_HT_NONMEMBER_PROTECTION)) {
        /* 如果AP发送的beacon帧ht operation ie中ht protection字段为mixed或non-member，则将保护级别设置为HT保护 */
        en_protection_mode = WLAN_PROT_HT;
    } else if (pst_mac_user->st_ht_hdl.bit_nongf_sta_present == OAL_TRUE) {
        /* 如果AP发送的beacon帧ht operation ie中non-gf sta present字段为1，则将保护级别设置为GF保护 */
        en_protection_mode = WLAN_PROT_GF;
    } else {
        /* 剩下的情况不做保护 */
        en_protection_mode = WLAN_PROT_NO;
    }

    /* 设置具体保护模式 */
    ul_ret = hmac_protection_set_mode(pst_mac_vap_sta, en_protection_mode);

    return ul_ret;
}


oal_uint32 hmac_protection_obss_aging_ap(mac_vap_stru *pst_mac_vap)
{
    oal_uint32    ul_ret               = OAL_SUCC;
    oal_bool_enum em_update_protection = OAL_FALSE; /* 指示是否需要更新vap的protection */

    if (pst_mac_vap == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 更新ERP老化计数 */
    if (pst_mac_vap->st_protection.bit_obss_non_erp_present == OAL_TRUE) {
        pst_mac_vap->st_protection.uc_obss_non_erp_aging_cnt++;
        if (pst_mac_vap->st_protection.uc_obss_non_erp_aging_cnt >= WLAN_PROTECTION_NON_ERP_AGING_THRESHOLD) {
            pst_mac_vap->st_protection.bit_obss_non_erp_present = OAL_FALSE;
            em_update_protection = OAL_TRUE;
            pst_mac_vap->st_protection.uc_obss_non_erp_aging_cnt = 0;
        }
    }

    /* 更新HT老化计数 */
    if (pst_mac_vap->st_protection.bit_obss_non_ht_present == OAL_TRUE) {
        pst_mac_vap->st_protection.uc_obss_non_ht_aging_cnt++;

        if (pst_mac_vap->st_protection.uc_obss_non_ht_aging_cnt >= WLAN_PROTECTION_NON_HT_AGING_THRESHOLD) {
            pst_mac_vap->st_protection.bit_obss_non_ht_present = OAL_FALSE;
            em_update_protection = OAL_TRUE;
            pst_mac_vap->st_protection.uc_obss_non_ht_aging_cnt = 0;
        }
    }

    /* 需要更新保护模式 */
    if (em_update_protection == OAL_TRUE) {
        ul_ret = hmac_protection_update_mib_ap(pst_mac_vap);
    }

    return ul_ret;
}


OAL_STATIC oal_uint32 hmac_protection_del_user_stat_legacy_ap(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    mac_protection_stru    *pst_protection = &(pst_mac_vap->st_protection);
    hmac_user_stru         *pst_hmac_user;

    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(pst_mac_user->us_assoc_id);
    if (pst_hmac_user == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "hmac_protection_del_user_stat_legacy_ap::Get Hmac_user(idx=%d) NULL POINT!", pst_mac_user->us_assoc_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 如果去关联的站点不支持ERP */
    if ((pst_hmac_user->st_hmac_cap_info.bit_erp == OAL_FALSE) &&
        (pst_hmac_user->st_user_stats_flag.bit_no_erp_stats_flag == OAL_TRUE) &&
        (pst_protection->uc_sta_non_erp_num != 0)) {
        pst_protection->uc_sta_non_erp_num--;
    }

    /* 如果去关联的站点不支持short preamble */
    if ((pst_hmac_user->st_hmac_cap_info.bit_short_preamble == OAL_FALSE) &&
        (pst_hmac_user->st_user_stats_flag.bit_no_short_preamble_stats_flag == OAL_TRUE) &&
        (pst_protection->uc_sta_no_short_preamble_num != 0)) {
        pst_protection->uc_sta_no_short_preamble_num--;
    }

    /* 如果去关联的站点不支持short slot */
    if ((pst_hmac_user->st_hmac_cap_info.bit_short_slot_time == OAL_FALSE) &&
        (pst_hmac_user->st_user_stats_flag.bit_no_short_slot_stats_flag == OAL_TRUE) &&
        (pst_protection->uc_sta_no_short_slot_num != 0)) {
        pst_protection->uc_sta_no_short_slot_num--;
    }

    pst_hmac_user->st_user_stats_flag.bit_no_short_slot_stats_flag     = OAL_FALSE;
    pst_hmac_user->st_user_stats_flag.bit_no_short_preamble_stats_flag = OAL_FALSE;
    pst_hmac_user->st_user_stats_flag.bit_no_erp_stats_flag            = OAL_FALSE;

    return OAL_SUCC;
}



OAL_STATIC oal_uint32 hmac_protection_del_user_stat_ht_ap(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    mac_user_ht_hdl_stru   *pst_ht_hdl     = &(pst_mac_user->st_ht_hdl);
    mac_protection_stru    *pst_protection = &(pst_mac_vap->st_protection);
    hmac_user_stru         *pst_hmac_user;

    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(pst_mac_user->us_assoc_id);
    if (pst_hmac_user == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "hmac_protection_del_user_stat_ht_ap::Get Hmac_user(idx=%d) NULL POINT!", pst_mac_user->us_assoc_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 如果去关联的站点不支持HT */
    if ((pst_ht_hdl->en_ht_capable == OAL_FALSE) &&
        (pst_hmac_user->st_user_stats_flag.bit_no_ht_stats_flag == OAL_TRUE) &&
        (pst_protection->uc_sta_non_ht_num != 0)) {
        pst_protection->uc_sta_non_ht_num--;
    } else { /* 支持HT */
        /* 如果去关联的站点不支持20/40Mhz频宽 */
        if ((pst_ht_hdl->bit_supported_channel_width == OAL_FALSE) &&
            (pst_hmac_user->st_user_stats_flag.bit_20M_only_stats_flag == OAL_TRUE) &&
            (pst_protection->uc_sta_20M_only_num != 0)) {
            pst_protection->uc_sta_20M_only_num--;
        }

        /* 如果去关联的站点不支持GF */
        if ((pst_ht_hdl->bit_ht_green_field == OAL_FALSE) &&
            (pst_hmac_user->st_user_stats_flag.bit_no_gf_stats_flag == OAL_TRUE) &&
            (pst_protection->uc_sta_non_gf_num != 0)) {
            pst_protection->uc_sta_non_gf_num--;
        }

        /* 如果去关联的站点不支持L-SIG TXOP Protection */
        if ((pst_ht_hdl->bit_lsig_txop_protection == OAL_FALSE) &&
            (pst_hmac_user->st_user_stats_flag.bit_no_lsig_txop_stats_flag == OAL_TRUE) &&
            (pst_protection->uc_sta_no_lsig_txop_num != 0)) {
            pst_protection->uc_sta_no_lsig_txop_num--;
        }

        /* 如果去关联的站点不支持40Mhz cck */
        if ((pst_ht_hdl->bit_dsss_cck_mode_40mhz == OAL_FALSE) &&
            (pst_ht_hdl->bit_supported_channel_width == OAL_TRUE) &&
            (pst_hmac_user->st_user_stats_flag.bit_no_40dsss_stats_flag == OAL_TRUE) &&
            (pst_protection->uc_sta_no_40dsss_cck_num != 0)) {
            pst_protection->uc_sta_no_40dsss_cck_num--;
        }
    }

    pst_hmac_user->st_user_stats_flag.bit_no_ht_stats_flag = OAL_FALSE;
    pst_hmac_user->st_user_stats_flag.bit_no_gf_stats_flag = OAL_FALSE;
    pst_hmac_user->st_user_stats_flag.bit_20M_only_stats_flag = OAL_FALSE;
    pst_hmac_user->st_user_stats_flag.bit_no_40dsss_stats_flag = OAL_FALSE;
    pst_hmac_user->st_user_stats_flag.bit_no_lsig_txop_stats_flag = OAL_FALSE;

    return OAL_SUCC;
}



OAL_STATIC oal_void hmac_protection_del_user_stat_ap(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    hmac_protection_del_user_stat_legacy_ap(pst_mac_vap, pst_mac_user);
    hmac_protection_del_user_stat_ht_ap(pst_mac_vap, pst_mac_user);
}


oal_uint32 hmac_protection_del_user(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    oal_uint32 ul_ret = OAL_SUCC;

    if ((pst_mac_vap == OAL_PTR_NULL) || (pst_mac_user == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* AP 更新VAP结构体统计量，更新保护机制 */
    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        /* 删除保护模式相关user统计 */
        hmac_protection_del_user_stat_ap(pst_mac_vap, pst_mac_user);

        /* 更新AP中保护相关mib量 */
        ul_ret = hmac_protection_update_mib_ap(pst_mac_vap);
        if (ul_ret != OAL_SUCC) {
            return ul_ret;
        }
    } else if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        /* 恢复STA为无保护状态 */
        ul_ret = hmac_protection_set_mode(pst_mac_vap, WLAN_PROT_NO);
    }

    return ul_ret;
}


oal_uint32 hmac_protection_obss_update_timer(void *p_arg)
{
    mac_device_stru *pst_mac_device = OAL_PTR_NULL;
    oal_uint8       uc_vap_idx;
    mac_vap_stru    *pst_mac_vap = OAL_PTR_NULL;

    if (p_arg == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_protection_obss_update_timer::p_arg null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = (mac_device_stru *)p_arg;

    /* 遍历device下对应VAP, 定时更新OBSS 保护模式 */
    /* 业务vap从1开始 */
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++) {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL)) {
            OAM_WARNING_LOG0(uc_vap_idx, OAM_SF_ANY, "{hmac_protection_obss_update_timer::pst_mac_vap null.}");
            return OAL_ERR_CODE_PTR_NULL;
        }

        /* OBSS老化只针对AP模式，非AP模式则跳出 */
        if (pst_mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_AP) {
            continue;
        }

        hmac_protection_obss_aging_ap(pst_mac_vap);
    }

    return OAL_SUCC;
}


oal_uint32 hmac_protection_start_timer(mac_vap_stru *mac_vap)
{
    mac_device_stru *mac_device = OAL_PTR_NULL;

    mac_device = mac_res_get_dev(mac_vap->uc_device_id);
    if (mac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_protection_start_timer::mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 启动OBSS保护老化定时器 定时器已开启，则不用再开启 */
    if (mac_device->st_obss_aging_timer.en_is_registerd == OAL_FALSE) {
        FRW_TIMER_CREATE_TIMER(&(mac_device->st_obss_aging_timer),
                               hmac_protection_obss_update_timer,
                               WLAN_USER_AGING_TRIGGER_TIME,                    /* 5000ms触发一次 */
                               mac_device,
                               OAL_TRUE,
                               OAM_MODULE_ID_HMAC,
                               mac_device->ul_core_id);
    }

    return OAL_SUCC;
}

uint32_t hmac_protection_stop_timer(mac_vap_stru *mac_vap)
{
    mac_device_stru *mac_device = OAL_PTR_NULL;

    mac_device = mac_res_get_dev(mac_vap->uc_device_id);
    if (mac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_protection_start_timer::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (mac_device->st_obss_aging_timer.en_is_registerd == OAL_TRUE) {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(mac_device->st_obss_aging_timer));
    }
    return OAL_SUCC;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

