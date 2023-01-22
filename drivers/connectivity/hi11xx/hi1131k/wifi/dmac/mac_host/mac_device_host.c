

/* 1 头文件包含 */
#include "oam_ext_if.h"
#include "frw_ext_if.h"
#include "hal_ext_if.h"
#include "mac_device.h"
#include "mac_resource.h"
#include "mac_regdomain.h"
#include "mac_vap.h"
#include "securec.h"
#ifdef _PRE_WLAN_ALG_ENABLE
#include "alg_dbac.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_DEVICE_HOST_C

/* 2 全局变量定义 */
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
extern oal_uint32 band_5g_enabled;
#endif

/* 1131debug: mac_device_init 预留接口 */
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV)
oal_bool_enum_uint8 mac_device_init_adjust(mac_device_stru *, oal_uint32, oal_uint8, oal_uint8);
typedef oal_bool_enum_uint8 (*mac_device_init_adjust_type)(mac_device_stru *pst_mac_device, oal_uint32 ul_chip_ver,
                                                           oal_uint8 uc_chip_id, oal_uint8 uc_device_id);
mac_device_init_adjust_type g_fn_mac_device_init_adjust_patch_ram = mac_device_init_adjust;
#endif
mac_device_custom_cfg_stru g_mac_device_custom_cfg = {0};
/* 3 函数实现 */

oal_uint32  mac_device_exit(mac_device_stru *pst_device)
{
    if (OAL_UNLIKELY(pst_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_device_exit::pst_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_device->uc_vap_num = 0;
    pst_device->uc_sta_num = 0;
    pst_device->st_p2p_info.uc_p2p_device_num   = 0;
    pst_device->st_p2p_info.uc_p2p_goclient_num = 0;

    mac_res_free_dev(pst_device->uc_device_id);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    mac_device_set_state(pst_device, OAL_FALSE);
#endif

    return OAL_SUCC;
}


oal_uint32  mac_chip_exit(mac_board_stru *pst_board, mac_chip_stru *pst_chip)
{
    if (OAL_UNLIKELY(pst_chip == OAL_PTR_NULL || pst_board == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_chip_init::param null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 放入Device自身结构释放 */
    pst_chip->uc_device_nums = 0;

    /* destroy流程最后将状态置为FALSE */
    pst_chip->en_chip_state  = OAL_FALSE;

    return OAL_SUCC;
}


oal_uint32  mac_board_exit(mac_board_stru *pst_board)
{
    if (OAL_UNLIKELY(pst_board == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_board_exit::pst_board null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }
    return OAL_SUCC;
}


wlan_bw_cap_enum_uint8 mac_device_max_band(oal_void)
{
#if ((_PRE_WLAN_CHIP_ASIC != _PRE_WLAN_CHIP_VERSION))
    return WLAN_BW_CAP_40M;
#else
    return WLAN_BW_CAP_80M;
#endif
}


oal_uint32  mac_device_init(mac_device_stru *pst_mac_device, oal_uint32 ul_chip_ver, oal_uint8 uc_chip_id,
                            oal_uint8 uc_device_id)
{
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    oal_uint32 ul_loop;
#endif

    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_device_init::pst_mac_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 初始化device的索引 */
    pst_mac_device->uc_chip_id   = uc_chip_id;
    pst_mac_device->uc_device_id = uc_device_id;

    /* 初始化device级别的一些参数 */
    pst_mac_device->en_max_bandwidth = WLAN_BAND_WIDTH_BUTT;
    pst_mac_device->en_max_band      = WLAN_BAND_BUTT;
    pst_mac_device->uc_max_channel   = 0;
    pst_mac_device->ul_beacon_interval = WLAN_BEACON_INTVAL_DEFAULT;

    pst_mac_device->uc_tx_chain  = 0xf;
    pst_mac_device->uc_rx_chain  = 0xf;

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    pst_mac_device->st_cap_flag.bit_proxysta = 0; /* 默认关闭proxy sta 特性 */
#endif

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    pst_mac_device->st_bss_id_list.us_num_networks = 0;
#endif

#ifdef _PRE_WLAN_FEATURE_SMPS
    pst_mac_device->en_smps = OAL_FALSE;
    pst_mac_device->uc_dev_smps_mode = WLAN_MIB_MIMO_POWER_SAVE_MIMO;
    pst_mac_device->uc_no_smps_user_cnt = 0;
#endif

    pst_mac_device->en_device_state = OAL_TRUE;

#ifdef _PRE_WALN_FEATURE_LUT_RESET
    pst_mac_device->en_reset_switch = OAL_TRUE;
#else
    pst_mac_device->en_reset_switch = OAL_FALSE;
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

    /* 根据初始化通道数，设置支持的空间流数 */
    if ((g_l_rf_channel_num == WITP_RF_CHANNEL_NUMS) && (g_l_rf_single_tran == WITP_RF_CHANNEL_ZERO)) {
        pst_mac_device->en_nss_num  = WLAN_DOUBLE_NSS;

        /* 发送通道为双通道，通道0 & 通道1 */
        pst_mac_device->uc_tx_chain = WITP_TX_CHAIN_DOUBLE;
    } else {
        pst_mac_device->en_nss_num = WLAN_SINGLE_NSS;

        if (g_l_rf_single_tran == WITP_RF_CHANNEL_ZERO) {
            /* 发送通道为双通道，通道0 */
            pst_mac_device->uc_tx_chain =  WITP_TX_CHAIN_ZERO;
        } else if (g_l_rf_single_tran == WITP_RF_CHANNEL_ONE) {
            /* 发送通道为双通道，通道1 */
            pst_mac_device->uc_tx_chain =  WITP_TX_CHAIN_ONE;
        }
    }
#else
    pst_mac_device->uc_tx_chain =  WITP_TX_CHAIN_ZERO;
#endif

    /* 默认关闭wmm,wmm超时计数器设为0 */
    pst_mac_device->en_wmm = OAL_TRUE;

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    /* 初始化device 结构下Proxy STA 相关信息 */
    for (ul_loop = 0; ul_loop < MAC_VAP_PROXY_STA_HASH_MAX_VALUE; ul_loop++) {
        oal_dlist_init_head(&(pst_mac_device->st_device_proxysta.ast_proxysta_hash[ul_loop]));
    }

    /* 配置Device工作在Repeater模式 */
    pst_mac_device->en_dev_work_mode = MAC_REPEATER_MODE;
#endif

    /* 根据芯片版本初始化device能力信息 */
    switch (ul_chip_ver) {
        case WLAN_CHIP_VERSION_HI1151V100H:
            pst_mac_device->en_protocol_cap  = WLAN_PROTOCOL_CAP_VHT;
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
            pst_mac_device->en_bandwidth_cap = (band_5g_enabled) ? mac_device_max_band() : WLAN_BW_CAP_40M;
#ifdef _PRE_WLAN_FEATURE_5G
            pst_mac_device->en_band_cap      = (band_5g_enabled) ? WLAN_BAND_CAP_2G_5G : WLAN_BAND_CAP_2G;
#else
            pst_mac_device->en_band_cap      = WLAN_BAND_CAP_2G;
#endif /* _PRE_WLAN_FEATURE_5G */
#else
            pst_mac_device->en_bandwidth_cap = mac_device_max_band();
#ifdef _PRE_WLAN_FEATURE_5G
            pst_mac_device->en_band_cap      = WLAN_BAND_CAP_2G_5G;
#else
            pst_mac_device->en_band_cap      = WLAN_BAND_CAP_2G;
#endif /* _PRE_WLAN_FEATURE_5G */
#endif
            break;

        case WLAN_CHIP_VERSION_HI1151V100L:
            pst_mac_device->en_protocol_cap  = WLAN_PROTOCOL_CAP_VHT;
            pst_mac_device->en_bandwidth_cap = WLAN_BW_CAP_40M;
            pst_mac_device->en_band_cap      = WLAN_BAND_CAP_2G;

            break;

        default:
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{mac_device_init::ul_chip_ver is not supportted[0x%x].}", ul_chip_ver);
            return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }
    pst_mac_device->bit_ldpc_coding  = OAL_FALSE;   // 1131debug  1131c不支持
#ifdef _PRE_WLAN_FEATURE_TXBF
    pst_mac_device->bit_tx_stbc      =  OAL_FALSE;
    pst_mac_device->bit_su_bfmer     =  OAL_FALSE;
    pst_mac_device->bit_su_bfmee     = OAL_TRUE;
    pst_mac_device->bit_rx_stbc      = 1;                       /* 支持2个空间流 */

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
    pst_mac_device->bit_mu_bfmee     = OAL_TRUE;
#else
    pst_mac_device->bit_mu_bfmee     = OAL_FALSE;
#endif

#else
    pst_mac_device->bit_tx_stbc      = OAL_FALSE;
    pst_mac_device->bit_su_bfmer     = OAL_FALSE;
    pst_mac_device->bit_su_bfmee     = OAL_FALSE;
    pst_mac_device->bit_mu_bfmee     = OAL_FALSE;
    pst_mac_device->bit_rx_stbc      = 1;
#endif

    /* 初始化vap num统计信息 */
    pst_mac_device->uc_vap_num = 0;
    pst_mac_device->uc_sta_num = 0;
#ifdef _PRE_WLAN_FEATURE_P2P
    pst_mac_device->st_p2p_info.uc_p2p_device_num   = 0;
    pst_mac_device->st_p2p_info.uc_p2p_goclient_num = 0;
    pst_mac_device->st_p2p_info.pst_primary_net_device = OAL_PTR_NULL; /* 初始化主net_device 为空指针 */
#endif

    /* 初始化默认管制域 */
    mac_init_regdomain();

    /* 初始化信道列表 */
    mac_init_channel_list();

    /* 初始化复位状态 */
    MAC_DEV_RESET_IN_PROGRESS(pst_mac_device, OAL_FALSE);
    pst_mac_device->us_device_reset_num = 0;

    /* 默认关闭DBAC特性 */
#ifdef _PRE_WLAN_FEATURE_DBAC
    pst_mac_device->en_dbac_enabled = OAL_TRUE;
#endif

#ifdef _PRE_SUPPORT_ACS
    memset_s(&pst_mac_device->st_acs_switch, OAL_SIZEOF(pst_mac_device->st_acs_switch), 0,
             OAL_SIZEOF(pst_mac_device->st_acs_switch));
#endif

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    mac_set_2040bss_switch(pst_mac_device, OAL_FALSE);
#endif
    pst_mac_device->uc_in_suspend       = OAL_FALSE;
    pst_mac_device->uc_arpoffload_switch   = OAL_FALSE;
    pst_mac_device->uc_wapi = OAL_FALSE;

    /* AGC绑定通道默认为自适应   */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    pst_mac_device->uc_lock_channel = 0x02;
#endif
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV)
    pst_mac_device->uc_scan_count    = 0;
#endif

    /* 1131debug: mac_device_init 接口预留 */
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV)
    if (g_fn_mac_device_init_adjust_patch_ram != NULL) {
        if (g_fn_mac_device_init_adjust_patch_ram(pst_mac_device, ul_chip_ver, uc_chip_id, uc_device_id) == OAL_TRUE) {
            return OAL_SUCC;
        }
    }
#endif
    return OAL_SUCC;
}

oal_uint32  mac_chip_init(mac_chip_stru *pst_chip, oal_uint8 uc_chip_id)
{
    return OAL_SUCC;
}

oal_uint32  mac_board_init(mac_board_stru *pst_board)
{
    return OAL_SUCC;
}


oal_uint32  mac_device_find_up_vap(mac_device_stru *pst_mac_device, mac_vap_stru **ppst_mac_vap)
{
    oal_uint8       uc_vap_idx;
    mac_vap_stru   *pst_mac_vap = OAL_PTR_NULL;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++) {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL)) {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN, "vap is null! vap id is %d", pst_mac_device->auc_vap_id[uc_vap_idx]);

            *ppst_mac_vap = OAL_PTR_NULL;

            return OAL_ERR_CODE_PTR_NULL;
        }

        if (pst_mac_vap->en_vap_state == MAC_VAP_STATE_UP || pst_mac_vap->en_vap_state == MAC_VAP_STATE_PAUSE ||
            (pst_mac_vap->en_vap_state == MAC_VAP_STATE_STA_LISTEN && pst_mac_vap->us_user_nums > 0)) {
            *ppst_mac_vap = pst_mac_vap;

            return OAL_SUCC;
        }
    }

    *ppst_mac_vap = OAL_PTR_NULL;

    return OAL_FAIL;
}


oal_uint32  mac_device_find_2up_vap(
    mac_device_stru *pst_mac_device,
    mac_vap_stru   **ppst_mac_vap1,
    mac_vap_stru   **ppst_mac_vap2)
{
    mac_vap_stru                  *pst_vap = OAL_PTR_NULL;
    oal_uint8                      uc_vap_idx;
    oal_uint8                      ul_up_vap_num = 0;
    mac_vap_stru                  *past_vap[2] = {0};

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++) {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (pst_vap == OAL_PTR_NULL) {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "vap is null, vap id is %d", pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if (pst_vap->en_vap_state == MAC_VAP_STATE_UP || pst_vap->en_vap_state == MAC_VAP_STATE_PAUSE ||
            (pst_vap->en_vap_state == MAC_VAP_STATE_STA_LISTEN && pst_vap->us_user_nums > 0)) {
            past_vap[ul_up_vap_num] = pst_vap;
            ul_up_vap_num++;

            if (ul_up_vap_num >= 2) {
                break;
            }
        }
    }

    if (ul_up_vap_num < 2) {
        return OAL_FAIL;
    }

    *ppst_mac_vap1 = past_vap[0];
    *ppst_mac_vap2 = past_vap[1];

    return OAL_SUCC;
}

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

oal_uint32  mac_device_find_up_ap(mac_device_stru *pst_mac_device, mac_vap_stru **ppst_mac_vap)
{
    oal_uint8       uc_vap_idx;
    mac_vap_stru   *pst_mac_vap = OAL_PTR_NULL;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++) {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL)) {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN, "vap is null! vap id is %d", pst_mac_device->auc_vap_id[uc_vap_idx]);
            return OAL_ERR_CODE_PTR_NULL;
        }

        if ((pst_mac_vap->en_vap_state == MAC_VAP_STATE_UP || pst_mac_vap->en_vap_state == MAC_VAP_STATE_PAUSE) &&
            (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP)) {
            *ppst_mac_vap = pst_mac_vap;

            return OAL_SUCC;
        }
    }

    *ppst_mac_vap = OAL_PTR_NULL;

    return OAL_FAIL;
}
#endif // #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)


oal_uint32  mac_device_find_up_sta(mac_device_stru *pst_mac_device, mac_vap_stru **ppst_mac_vap)
{
    oal_uint8       uc_vap_idx;
    mac_vap_stru   *pst_mac_vap = OAL_PTR_NULL;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++) {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL)) {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN, "vap is null! vap id is %d", pst_mac_device->auc_vap_id[uc_vap_idx]);

            *ppst_mac_vap = OAL_PTR_NULL;

            return OAL_ERR_CODE_PTR_NULL;
        }

        if ((pst_mac_vap->en_vap_state == MAC_VAP_STATE_UP || pst_mac_vap->en_vap_state == MAC_VAP_STATE_PAUSE) &&
            (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA)) {
            *ppst_mac_vap = pst_mac_vap;

            return OAL_SUCC;
        }
    }

    *ppst_mac_vap = OAL_PTR_NULL;

    return OAL_FAIL;
}

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

oal_uint32  mac_device_find_up_p2p_go(mac_device_stru *pst_mac_device, mac_vap_stru **ppst_mac_vap)
{
    oal_uint8       uc_vap_idx;
    mac_vap_stru   *pst_mac_vap = OAL_PTR_NULL;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++) {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL)) {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN, "vap is null! vap id is %d", pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if ((pst_mac_vap->en_vap_state == MAC_VAP_STATE_UP || pst_mac_vap->en_vap_state == MAC_VAP_STATE_PAUSE) &&
            (pst_mac_vap->en_p2p_mode == WLAN_P2P_GO_MODE)) {
            *ppst_mac_vap = pst_mac_vap;

            return OAL_SUCC;
        }
    }

    *ppst_mac_vap = OAL_PTR_NULL;

    return OAL_FAIL;
}
#endif // #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)


oal_uint32  mac_device_calc_up_vap_num(mac_device_stru *pst_mac_device)
{
    mac_vap_stru                  *pst_vap = OAL_PTR_NULL;
    oal_uint8                      uc_vap_idx;
    oal_uint8                      ul_up_ap_num = 0;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++) {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (pst_vap == OAL_PTR_NULL) {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "vap is null, vap id is %d",
                           pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if (pst_vap->en_vap_state == MAC_VAP_STATE_UP || pst_vap->en_vap_state == MAC_VAP_STATE_PAUSE) {
            ul_up_ap_num++;
        }
    }

    return ul_up_ap_num;
}


oal_uint32  mac_device_is_p2p_connected(mac_device_stru *pst_mac_device)
{
    oal_uint8       uc_vap_idx;
    mac_vap_stru   *pst_mac_vap = OAL_PTR_NULL;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++) {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL)) {
            OAM_WARNING_LOG1(0, OAM_SF_P2P, "vap is null! vap id is %d", pst_mac_device->auc_vap_id[uc_vap_idx]);
            return OAL_ERR_CODE_PTR_NULL;
        }
        if ((IS_P2P_GO(pst_mac_vap) || IS_P2P_CL(pst_mac_vap)) &&
            (pst_mac_vap->us_user_nums > 0)) {
            return OAL_SUCC;
        }
    }
    return OAL_FAIL;
}


oal_void mac_device_set_vap_id(mac_device_stru *pst_mac_device, mac_vap_stru *pst_mac_vap, oal_uint8 uc_vap_idx,
    wlan_vap_mode_enum_uint8 en_vap_mode, wlan_p2p_mode_enum_uint8 en_p2p_mode, oal_uint8 is_add_vap)
{
#ifdef _PRE_WLAN_FEATURE_P2P
    oal_uint8                       uc_vap_tmp_idx;
    mac_vap_stru                   *pst_tmp_vap;
#endif

    if (is_add_vap) {
        /* ?offload???,????HMAC????? */
        pst_mac_device->auc_vap_id[pst_mac_device->uc_vap_num++] = uc_vap_idx;

        /* device?sta???1 */
        if (en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
            pst_mac_device->uc_sta_num++;

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
            if (pst_mac_device->st_cap_flag.bit_proxysta == 1) {
                if ((pst_mac_vap->st_vap_proxysta.en_is_proxysta == OAL_TRUE) &&
                    (pst_mac_vap->st_vap_proxysta.en_is_main_proxysta == OAL_FALSE)) {
                    pst_mac_device->uc_proxysta_num++;
                }
            }
#endif
            /* ???uc_assoc_vap_id??????ap??? */
            pst_mac_vap->uc_assoc_vap_id = 0xff;
        }

#ifdef _PRE_WLAN_FEATURE_P2P
        pst_mac_vap->en_p2p_mode = en_p2p_mode;
        mac_inc_p2p_num(pst_mac_vap);
        if (IS_P2P_GO(pst_mac_vap)) {
            for (uc_vap_tmp_idx = 0; uc_vap_tmp_idx < pst_mac_device->uc_vap_num; uc_vap_tmp_idx++) {
                pst_tmp_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_tmp_idx]);
                if (pst_tmp_vap == OAL_PTR_NULL) {
                    OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{dmac_config_add_vap::pst_mac_vap null,vap_idx=%d.}",
                                   pst_mac_device->auc_vap_id[uc_vap_tmp_idx]);
                    continue;
                }

                if ((pst_tmp_vap->en_vap_state == MAC_VAP_STATE_UP) && (pst_tmp_vap != pst_mac_vap)) {
                    pst_mac_vap->st_channel.en_band        = pst_tmp_vap->st_channel.en_band;
                    pst_mac_vap->st_channel.en_bandwidth   = pst_tmp_vap->st_channel.en_bandwidth;
                    pst_mac_vap->st_channel.uc_chan_number = pst_tmp_vap->st_channel.uc_chan_number;
                    pst_mac_vap->st_channel.uc_idx         = pst_tmp_vap->st_channel.uc_idx;
                    break;
                }
            }
        }
#endif
    } else {
        /* ?offload???,????HMAC????? */
        pst_mac_device->auc_vap_id[pst_mac_device->uc_vap_num--] = 0;

        /* device?sta???1 */
        if (en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
            pst_mac_device->uc_sta_num--;

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
            if (pst_mac_device->st_cap_flag.bit_proxysta == 1) {
                if ((pst_mac_vap->st_vap_proxysta.en_is_proxysta == OAL_TRUE) &&
                    (pst_mac_vap->st_vap_proxysta.en_is_main_proxysta == OAL_FALSE)) {
                    pst_mac_device->uc_proxysta_num--;
                }
            }
#endif
            /* ???uc_assoc_vap_id??????ap??? */
            pst_mac_vap->uc_assoc_vap_id = 0xff;
        }

#ifdef _PRE_WLAN_FEATURE_P2P
        pst_mac_vap->en_p2p_mode = en_p2p_mode;
        mac_dec_p2p_num(pst_mac_vap);
#endif
    }
}

oal_void mac_device_set_dfr_reset(mac_device_stru *pst_mac_device, oal_uint8 uc_device_reset_in_progress)
{
    pst_mac_device->uc_device_reset_in_progress = uc_device_reset_in_progress;
}

oal_void  mac_device_set_state(mac_device_stru *pst_mac_device, oal_uint8 en_device_state)
{
    pst_mac_device->en_device_state = en_device_state;
}

oal_void  mac_device_set_channel(mac_device_stru *pst_mac_device, mac_cfg_channel_param_stru *pst_channel_param)
{
    pst_mac_device->uc_max_channel = pst_channel_param->uc_channel;
    pst_mac_device->en_max_band = pst_channel_param->en_band;
    pst_mac_device->en_max_bandwidth = pst_channel_param->en_bandwidth;
}

oal_void  mac_device_get_channel(mac_device_stru *pst_mac_device, mac_cfg_channel_param_stru *pst_channel_param)
{
    pst_channel_param->uc_channel = pst_mac_device->uc_max_channel;
    pst_channel_param->en_band = pst_mac_device->en_max_band;
    pst_channel_param->en_bandwidth = pst_mac_device->en_max_bandwidth;
}


oal_void  mac_device_set_txchain(mac_device_stru *pst_mac_device, oal_uint8 uc_tx_chain)
{
    pst_mac_device->uc_tx_chain = uc_tx_chain;
}

oal_void  mac_device_set_rxchain(mac_device_stru *pst_mac_device, oal_uint8 uc_rx_chain)
{
    pst_mac_device->uc_rx_chain = uc_rx_chain;
}

oal_void  mac_device_set_beacon_interval(mac_device_stru *pst_mac_device, oal_uint32 ul_beacon_interval)
{
    pst_mac_device->ul_beacon_interval = ul_beacon_interval;
}

oal_void  mac_device_inc_active_user(mac_device_stru *pst_mac_device)
{
    /* ?????+1 */
    pst_mac_device->uc_active_user_cnt++;
}

oal_void  mac_device_dec_active_user(mac_device_stru *pst_mac_device)
{
    if (pst_mac_device->uc_active_user_cnt) {
        pst_mac_device->uc_active_user_cnt--;
    }
}


oal_void* mac_device_get_all_rates(mac_device_stru *pst_dev)
{
    return (oal_void *)pst_dev->st_mac_rates_11g;
}


/*lint -e19*/
oal_module_symbol(mac_device_set_vap_id);
oal_module_symbol(mac_device_set_dfr_reset);
oal_module_symbol(mac_device_set_state);
oal_module_symbol(mac_device_get_channel);
oal_module_symbol(mac_device_set_channel);
oal_module_symbol(mac_device_set_txchain);
oal_module_symbol(mac_device_set_rxchain);
oal_module_symbol(mac_device_set_beacon_interval);
oal_module_symbol(mac_device_inc_active_user);
oal_module_symbol(mac_device_dec_active_user);
oal_module_symbol(mac_device_init);
oal_module_symbol(mac_chip_init);
oal_module_symbol(mac_board_init);
oal_module_symbol(mac_device_exit);
oal_module_symbol(mac_chip_exit);
oal_module_symbol(mac_board_exit);
oal_module_symbol(mac_device_find_up_vap);
oal_module_symbol(mac_device_find_up_ap);
oal_module_symbol(mac_device_calc_up_vap_num);
oal_module_symbol(mac_device_is_p2p_connected);
oal_module_symbol(mac_device_find_2up_vap);
oal_module_symbol(mac_device_find_up_p2p_go);
/*lint +e19*/
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV)
#pragma arm section rwdata = "patch", code = "patch", zidata = "patch", rodata = "patch"
oal_bool_enum_uint8 mac_device_init_adjust(mac_device_stru *pst_mac_device, oal_uint32 ul_chip_ver,
                                           oal_uint8 uc_chip_id, oal_uint8 uc_device_id)
{
    return OAL_FALSE;
}
#pragma arm section rodata, code, rwdata, zidata
#endif
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

