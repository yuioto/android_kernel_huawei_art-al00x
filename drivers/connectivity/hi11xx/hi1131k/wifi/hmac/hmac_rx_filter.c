


// 1 头文件包含
#include "wlan_types.h"
#include "mac_device.h"
#include "mac_resource.h"
#include "dmac_ext_if.h"
#include "hmac_rx_filter.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_RX_FILTER_C

// 2 全局变量定义
oal_bool_enum_uint8 g_en_rx_filter_enable;
oal_uint32 g_ast_single_vap_rx_filter[WLAN_VAP_MODE_BUTT][MAC_VAP_STATE_BUTT];
oal_uint32 g_ast_multi_staup_sta_rx_filter[MAC_VAP_STA_STATE_BUTT];
oal_uint32 g_ast_multi_staup_ap_rx_filter[MAC_VAP_AP_STATE_BUTT];
oal_uint32 g_ast_multi_apup_sta_rx_filter[MAC_VAP_STA_STATE_BUTT];


// 3 函数实现

oal_bool_enum_uint8 hmac_find_is_sta_up(mac_device_stru *pst_mac_device)
{
    mac_vap_stru *pst_vap = OAL_PTR_NULL;
    oal_uint8 uc_vap_idx;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++) {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (pst_vap == OAL_PTR_NULL) {
            OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{hmac_find_is_sta_up::pst_mac_vap null,vap_idx=%d.}",
                pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if ((pst_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) && (pst_vap->en_vap_state == MAC_VAP_STATE_UP)) {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}


oal_bool_enum_uint8 hmac_find_is_ap_up(mac_device_stru *pst_mac_device)
{
    mac_vap_stru *pst_vap = OAL_PTR_NULL;
    oal_uint8 uc_vap_idx;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++) {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (pst_vap == OAL_PTR_NULL) {
            OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{hmac_find_is_ap_up::pst_mac_vap null,vap_idx=%d.}",
                pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if ((pst_vap->en_vap_state != MAC_VAP_STATE_INIT) && (pst_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP)) {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}


oal_uint32 hmac_calc_up_ap_num(mac_device_stru *pst_mac_device)
{
    mac_vap_stru *pst_vap = OAL_PTR_NULL;
    oal_uint8 uc_vap_idx;
    oal_uint8 ul_up_ap_num = 0;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++) {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (pst_vap == OAL_PTR_NULL) {
            OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{hmac_calc_up_ap_num::pst_mac_vap null,vap_idx=%d.}",
                pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if ((pst_vap->en_vap_state != MAC_VAP_STATE_INIT) && (pst_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP)) {
            ul_up_ap_num++;
        } else if ((pst_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) && (pst_vap->en_vap_state == MAC_VAP_STATE_UP)) {
            ul_up_ap_num++;
        }
    }

    return ul_up_ap_num;
}


oal_uint32 hmac_find_up_vap(mac_device_stru *pst_mac_device, mac_vap_stru **ppst_mac_vap)
{
    oal_uint32 ul_ret;
    mac_vap_stru *pst_vap_up = OAL_PTR_NULL;     /* 处在UP状态的VAP */

    /* find up VAP */
    ul_ret = mac_device_find_up_vap(pst_mac_device, &pst_vap_up);
    if ((ul_ret == OAL_SUCC) && (pst_vap_up != OAL_PTR_NULL)) {
        *ppst_mac_vap = pst_vap_up;
        /* find up AP */
        ul_ret = mac_device_find_up_ap(pst_mac_device, &pst_vap_up);
        if ((ul_ret == OAL_SUCC) && (pst_vap_up != OAL_PTR_NULL)) {
            *ppst_mac_vap = pst_vap_up;
        }

        return OAL_SUCC;
    } else {
        *ppst_mac_vap = OAL_PTR_NULL;
        return OAL_FALSE;
    }
}


oal_uint32 hmac_set_rx_filter_value(mac_vap_stru *pst_mac_vap)
{
#ifndef _PRE_WLAN_PHY_PERFORMANCE
    mac_device_stru *pst_mac_device = OAL_PTR_NULL;
    frw_event_mem_stru *pst_event_mem = OAL_PTR_NULL;
    frw_event_stru *pst_event = OAL_PTR_NULL;
    oal_uint32 ul_ret;
    oal_uint32 ul_rx_filter_val;

    if (pst_mac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_set_rx_filter_value::pst_mac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_set_rx_filter_value::pst_mac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#ifdef _PRE_WLAN_HW_TEST
    /* 若为常收，直接返回 */
    if (pst_mac_device->pst_device_stru->bit_al_rx_flag == HAL_ALWAYS_RX_RESERVED) {
        return OAL_SUCC;
    }
#endif

    if (g_en_rx_filter_enable == OAL_FALSE) {
        OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_set_rx_filter_value::g_en_rx_filter_enable is false}");
        return OAL_SUCC;
    }

    if (pst_mac_vap->en_vap_state == MAC_VAP_STATE_PAUSE) {
        return OAL_SUCC;
    }

    if (hmac_find_is_ap_up(pst_mac_device)) { /* 多VAP模式，AP已经UP */
        if (OAL_VALUE_EQ_ANY2(pst_mac_vap->en_vap_state, MAC_VAP_STATE_INIT, MAC_VAP_STATE_STA_FAKE_UP)) {
            return OAL_SUCC;
        }

        if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
            if (hmac_find_is_sta_up(pst_mac_device) && pst_mac_vap->en_vap_state < MAC_VAP_AP_STATE_BUTT) {
                ul_rx_filter_val = g_ast_multi_staup_ap_rx_filter[pst_mac_vap->en_vap_state];
            } else {
                ul_rx_filter_val = g_ast_single_vap_rx_filter[pst_mac_vap->en_vap_mode][pst_mac_vap->en_vap_state];
            }
        } else {
            /* 多VAP模式，STA配置(在STA已经up和没有STA up的情况下，配置AP模式都使用该配置) */
            ul_rx_filter_val = g_ast_multi_apup_sta_rx_filter[pst_mac_vap->en_vap_state];
        }
    } else if (hmac_find_is_sta_up(pst_mac_device)) { /* 多VAP模式，STA已经UP */
        /* 多VAP模式，STA配置(在STA已经up和没有STA up的情况下，配置AP模式都使用该配置) */
        if ((pst_mac_vap->en_vap_state == MAC_VAP_STATE_INIT) ||
            (pst_mac_vap->en_vap_state == MAC_VAP_STATE_STA_FAKE_UP)) {
            return OAL_SUCC;
        }
        /* STA已经UP的状态下，STA的配置 */
        if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
            ul_rx_filter_val = g_ast_multi_staup_sta_rx_filter[pst_mac_vap->en_vap_state];
        } else if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
            ul_rx_filter_val = g_ast_multi_staup_ap_rx_filter[pst_mac_vap->en_vap_state];
        } else {
            return OAL_SUCC;
        }
    } else {
        /* 没有任何设备处于UP状态，即单VAP存在 */
        ul_rx_filter_val = g_ast_single_vap_rx_filter[pst_mac_vap->en_vap_mode][pst_mac_vap->en_vap_state];
    }

     /* 抛事件到DMAC, 申请事件内存 */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(oal_uint32));
    if (pst_event_mem == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_set_rx_filter_value::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 填写事件 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_SET_RX_FILTER,
                       OAL_SIZEOF(oal_uint32),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_mac_vap->uc_chip_id,
                       pst_mac_vap->uc_device_id,
                       pst_mac_vap->uc_vap_id);

    /* 拷贝参数 */
    if (memcpy_s(pst_event->auc_event_data, OAL_SIZEOF(uint32_t), (void *)&ul_rx_filter_val,
        OAL_SIZEOF(uint32_t)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_set_rx_filter_value::memcpy_s failed.}");
        FRW_EVENT_FREE(pst_event_mem);
        return OAL_FAIL;
    }

    /* 分发事件 */
    ul_ret = frw_event_dispatch_event(pst_event_mem);
    if (ul_ret != OAL_SUCC) {
        FRW_EVENT_FREE(pst_event_mem);
        return OAL_FAIL;
    }

    FRW_EVENT_FREE(pst_event_mem);

#endif

    return OAL_SUCC;
}


oal_void hmac_rx_filter_init_single_vap(oal_void)
{
    oal_uint32 ul_state;
    oal_uint32 ul_vap_mode;

    for (ul_vap_mode = WLAN_VAP_MODE_CONFIG; ul_vap_mode < WLAN_VAP_MODE_BUTT; ul_vap_mode++) {
        for (ul_state = MAC_VAP_STATE_INIT; ul_state < MAC_VAP_STATE_BUTT; ul_state++) {
            g_ast_single_vap_rx_filter[ul_vap_mode][ul_state] = (1<<21);
        }
    }

    /*            WLAN_VAP_MODE_CONFIG, 配置模式                */
    /* +---------------------------+--------------------------+ */
    /* | FSM State                 | RX FILTER VALUE          | */
    /* +---------------------------+--------------------------+ */
    /* | All states                | 0x37BDEEFA               | */
    /* +---------------------------+--------------------------+ */
    for (ul_state = MAC_VAP_STATE_INIT; ul_state < MAC_VAP_STATE_BUTT; ul_state++) {
        g_ast_single_vap_rx_filter[WLAN_VAP_MODE_CONFIG][ul_state] = 0x37B9FEFA;
    }

    /* WLAN_VAP_MODE_BSS_STA          BSS STA模式                  */
    /* +----------------------------------+--------------------------+ */
    /* | FSM State                        | RX FILTER VALUE          | */
    /* +----------------------------------+--------------------------+ */
    /* | MAC_VAP_STATE_INIT               | 0x37BDEEFA               | */
    /* | MAC_VAP_STATE_UP                 | 0x37BDEADA               | */
    /* | MAC_VAP_STATE_STA_FAKE_UP        | 0x37BDEEFA               | */
    /* | MAC_VAP_STATE_STA_WAIT_SCAN      | 0x37BDCEEA               | */
    /* | MAC_VAP_STATE_STA_SCAN_COMP      | 0x37BDEEDA               | */
    /* | MAC_VAP_STATE_STA_WAIT_JOIN      | 0x37BDEEDA               | */
    /* | MAC_VAP_STATE_STA_JOIN_COMP      | 0x37BDEEDA               | */
    /* | MAC_VAP_STATE_STA_WAIT_AUTH_SEQ2 | 0x37BDEEDA               | */
    /* | MAC_VAP_STATE_STA_WAIT_AUTH_SEQ4 | 0x37BDEEDA               | */
    /* | MAC_VAP_STATE_STA_AUTH_COMP      | 0x37BDEEDA               | */
    /* | MAC_VAP_STATE_STA_WAIT_ASOC      | 0x37BDEEDA               | */
    /* | MAC_VAP_STATE_STA_OBSS_SCAN      | 0x37BDCEEA               | */
    /* | MAC_VAP_STATE_STA_BG_SCAN        | 0x37BDCEEA               | */
    /* | MAC_VAP_STATE_STA_LISTEN         | 0x33A9EECA               | */
    /* +----------------------------------+--------------------------+ */
    for (ul_state = MAC_VAP_STATE_INIT; ul_state < MAC_VAP_STATE_BUTT; ul_state++) {
        g_ast_single_vap_rx_filter[WLAN_VAP_MODE_BSS_STA][ul_state] = (1<<21);
    }

    g_ast_single_vap_rx_filter[WLAN_VAP_MODE_BSS_STA][MAC_VAP_STATE_INIT] = 0xF7B9FEFA;
    g_ast_single_vap_rx_filter[WLAN_VAP_MODE_BSS_STA][MAC_VAP_STATE_STA_FAKE_UP] = 0xF7B9FEFA;
    g_ast_single_vap_rx_filter[WLAN_VAP_MODE_BSS_STA][MAC_VAP_STATE_UP] = 0x73B9FADA;
    g_ast_single_vap_rx_filter[WLAN_VAP_MODE_BSS_STA][MAC_VAP_STATE_PAUSE] = 0x73B9FADA;

    for (ul_state = MAC_VAP_STATE_STA_SCAN_COMP; ul_state <= MAC_VAP_STATE_STA_WAIT_ASOC; ul_state++) {
        g_ast_single_vap_rx_filter[WLAN_VAP_MODE_BSS_STA][ul_state] = 0x73B9FADA;
    }

    g_ast_single_vap_rx_filter[WLAN_VAP_MODE_BSS_STA][MAC_VAP_STATE_STA_WAIT_SCAN] = 0x37B9DECA;
    g_ast_single_vap_rx_filter[WLAN_VAP_MODE_BSS_STA][MAC_VAP_STATE_STA_OBSS_SCAN] = 0x37B9DEEA;
    g_ast_single_vap_rx_filter[WLAN_VAP_MODE_BSS_STA][MAC_VAP_STATE_STA_BG_SCAN] = 0x37B9DEEA;
    g_ast_single_vap_rx_filter[WLAN_VAP_MODE_BSS_STA][MAC_VAP_STATE_STA_LISTEN] = 0x33A9FECA;

#ifdef _PRE_WLAN_FEATURE_ROAM
    g_ast_single_vap_rx_filter[WLAN_VAP_MODE_BSS_STA][MAC_VAP_STATE_ROAMING] = 0x73B9FADA;
#endif

    /* WLAN_VAP_MODE_BSS_AP                BSS AP模式                  */
    /* +----------------------------------+--------------------------+ */
    /* | FSM State                        | RX FILTER VALUE          | */
    /* +----------------------------------+--------------------------+ */
    /* | MAC_VAP_STATE_INIT               | 0xF7B9EEFA               | */
    /* | MAC_VAP_STATE_UP                 | 0x73B9EAEA               | */
    /* | MAC_VAP_STATE_PAUSE              | 0x73B9EAEA               | */
    /* | MAC_VAP_STATE_AP_WAIT_START      | 0x73B9EAEA               | */
    /* +----------------------------------+--------------------------+ */
    for (ul_state = MAC_VAP_STATE_INIT; ul_state < MAC_VAP_STATE_BUTT; ul_state++) {
        g_ast_single_vap_rx_filter[WLAN_VAP_MODE_BSS_AP][ul_state] = (1<<21);
    }

    g_ast_single_vap_rx_filter[WLAN_VAP_MODE_BSS_AP][MAC_VAP_STATE_INIT] = 0xF7B9FEFA;
    g_ast_single_vap_rx_filter[WLAN_VAP_MODE_BSS_AP][MAC_VAP_STATE_UP] = 0x73B9FAEA;
    g_ast_single_vap_rx_filter[WLAN_VAP_MODE_BSS_AP][MAC_VAP_STATE_PAUSE] = 0x73B9FAEA;
    g_ast_single_vap_rx_filter[WLAN_VAP_MODE_BSS_AP][MAC_VAP_STATE_AP_WAIT_START] = 0x73B9FAEA;

    /* WLAN_VAP_MODE_MONITOER                侦听模式                  */
    /* +----------------------------------+--------------------------+ */
    /* | FSM State                        | RX FILTER VALUE          | */
    /* +----------------------------------+--------------------------+ */
    /* | all status                       | 0x1                      | */
    /* +----------------------------------+--------------------------+ */
    for (ul_state = MAC_VAP_STATE_INIT; ul_state < MAC_VAP_STATE_BUTT; ul_state++) {
        g_ast_single_vap_rx_filter[WLAN_VAP_MODE_MONITOER][ul_state] = 0x1 | (1<<21);
    }
}


oal_void hmac_rx_filter_init_multi_vap(oal_void)
{
    oal_uint32 ul_state;

    for (ul_state = MAC_VAP_STATE_INIT; ul_state < MAC_VAP_STA_STATE_BUTT; ul_state++) {
        g_ast_multi_staup_sta_rx_filter[ul_state] = (1<<21);
        g_ast_multi_apup_sta_rx_filter[ul_state] = (1<<21);
    }
    for (ul_state = MAC_VAP_STATE_INIT; ul_state < MAC_VAP_AP_STATE_BUTT; ul_state++) {
        g_ast_multi_staup_ap_rx_filter[ul_state] = (1<<21);
    }

#ifndef _PRE_WLAN_FEATURE_PROXYSTA
    /*  多STA模式    WLAN_VAP_MODE_BSS_STA          BSS STA模式        */
    /* +----------------------------------+--------------------------+ */
    /* | FSM State                        | RX FILTER VALUE          | */
    /* +----------------------------------+--------------------------+ */
    /* | MAC_VAP_STATE_STA_WAIT_SCAN      | 0x33B9CACA               | */
    /* | MAC_VAP_STATE_STA_OBSS_SCAN      | 0x33B9CACA               | */
    /* | MAC_VAP_STATE_STA_BG_SCAN        | 0x33B9CACA               | */
    /* | MAC_VAP_STATE_STA_LISTEN         | 0x33A9EACA               | */
    /* | ALL OTHER STATE                  | 0x73B9EADA               | */
    /* +----------------------------------+--------------------------+ */
    for (ul_state = MAC_VAP_STATE_INIT; ul_state < MAC_VAP_STA_STATE_BUTT; ul_state++) {
        g_ast_multi_staup_sta_rx_filter[ul_state] = 0x73B9FADA;
    }

    g_ast_multi_staup_sta_rx_filter[MAC_VAP_STATE_STA_WAIT_SCAN] = 0x33B9DACA;
    g_ast_multi_staup_sta_rx_filter[MAC_VAP_STATE_STA_OBSS_SCAN] = 0x33B9DACA;
    g_ast_multi_staup_sta_rx_filter[MAC_VAP_STATE_STA_BG_SCAN] = 0x33B9DACA;
    g_ast_multi_staup_sta_rx_filter[MAC_VAP_STATE_STA_LISTEN] = 0x33A9FACA;

    /* 多AP UP时,STA配置场景:  WLAN_VAP_MODE_BSS_STA   BSS STA模式     */
    /* +----------------------------------+--------------------------+ */
    /* | FSM State                        | RX FILTER VALUE          | */
    /* +----------------------------------+--------------------------+ */
    /* | MAC_VAP_STATE_INIT               | 保持原有值不配置         | */
    /* | MAC_VAP_STATE_UP                 | 0x73B9EACA               | */
    /* | MAC_VAP_STATE_STA_FAKE_UP        | 保持原有值不配置         | */
    /* | MAC_VAP_STATE_STA_WAIT_SCAN      | 0x33B9CACA               | */
    /* | MAC_VAP_STATE_STA_SCAN_COMP      | 0x73B9EACA               | */
    /* | MAC_VAP_STATE_STA_WAIT_JOIN      | 0x73B9EACA               | */
    /* | MAC_VAP_STATE_STA_JOIN_COMP      | 0x73B9EACA               | */
    /* | MAC_VAP_STATE_STA_WAIT_AUTH_SEQ2 | 0x73B9EACA               | */
    /* | MAC_VAP_STATE_STA_WAIT_AUTH_SEQ4 | 0x73B9EACA               | */
    /* | MAC_VAP_STATE_STA_AUTH_COMP      | 0x73B9EACA               | */
    /* | MAC_VAP_STATE_STA_WAIT_ASOC      | 0x73B9EACA               | */
    /* | MAC_VAP_STATE_STA_OBSS_SCAN      | 0x33B9CACA               | */
    /* | MAC_VAP_STATE_STA_BG_SCAN        | 0x33B9CACA               | */
    /* | MAC_VAP_STATE_STA_LISTEN         | 0x33A9EACA               | */
    /* +----------------------------------+--------------------------+ */
    for (ul_state = MAC_VAP_STATE_INIT; ul_state < MAC_VAP_STA_STATE_BUTT; ul_state++) {
        g_ast_multi_apup_sta_rx_filter[ul_state] = (1<<21);
    }
    g_ast_multi_apup_sta_rx_filter[MAC_VAP_STATE_STA_WAIT_SCAN] = 0x33B9DACA;
    g_ast_multi_apup_sta_rx_filter[MAC_VAP_STATE_STA_OBSS_SCAN] = 0x33B9DACA;
    g_ast_multi_apup_sta_rx_filter[MAC_VAP_STATE_STA_BG_SCAN]   = 0x33B9DACA;
    g_ast_multi_apup_sta_rx_filter[MAC_VAP_STATE_STA_LISTEN]    = 0x33A9FACA;
    g_ast_multi_apup_sta_rx_filter[MAC_VAP_STATE_UP]            = 0x73B9FACA;
#ifdef _PRE_WLAN_FEATURE_ROAM
    g_ast_multi_apup_sta_rx_filter[MAC_VAP_STATE_ROAMING]       = 0x73B9FACA;
#endif
    for (ul_state = MAC_VAP_STATE_STA_SCAN_COMP; ul_state <= MAC_VAP_STATE_STA_WAIT_ASOC; ul_state++) {
        g_ast_multi_apup_sta_rx_filter[ul_state] = 0x73B9FACA;
    }

    /* 多STA UP时,AP配置场景:  WLAN_VAP_MODE_BSS_AP    BSS AP模式      */
    /* +----------------------------------+--------------------------+ */
    /* | FSM State                        | RX FILTER VALUE          | */
    /* +----------------------------------+--------------------------+ */
    /* | MAC_VAP_STATE_INIT               | 保持原有值不配置         | */
    /* | MAC_VAP_STATE_UP                 | 0x73B9EACA               | */
    /* | MAC_VAP_STATE_PAUSE              | 0x73B9EACA               | */
    /* | MAC_VAP_STATE_AP_WAIT_START      | 0x73B9EACA               | */
    /* +----------------------------------+--------------------------+ */
    for (ul_state = MAC_VAP_STATE_INIT; ul_state < MAC_VAP_AP_STATE_BUTT; ul_state++) {
        g_ast_multi_staup_ap_rx_filter[ul_state] = 0x73B9FACA;
    }

    g_ast_multi_staup_ap_rx_filter[MAC_VAP_STATE_INIT] = (1<<21);

#else
    /* Proxy ARP模式下不能过滤重复帧 */
    /*  多STA模式    WLAN_VAP_MODE_BSS_STA          BSS STA模式        */
    /* +----------------------------------+--------------------------+ */
    /* 多AP UP时,STA配置场景:  WLAN_VAP_MODE_BSS_STA   BSS STA模式     */
    /* +----------------------------------+--------------------------+ */
    /* | FSM State                        | RX FILTER VALUE          | */
    /* +----------------------------------+--------------------------+ */
    /* | ALL STATE                        | 0x33ADEACA               | */
    /* +----------------------------------+--------------------------+ */
    for (ul_state = MAC_VAP_STATE_INIT; ul_state < MAC_VAP_STA_STATE_BUTT; ul_state++) {
        g_ast_multi_staup_sta_rx_filter[ul_state] = 0x33ADFACA;
        g_ast_multi_apup_sta_rx_filter[ul_state] = 0x33ADFACA;
    }

    /* WLAN_VAP_MODE_BSS_STA               BSS AP模式                  */
    /* +----------------------------------+--------------------------+ */
    /* | FSM State                        | RX FILTER VALUE          | */
    /* +----------------------------------+--------------------------+ */
    /* | MAC_VAP_STATE_INIT               | 0x37BDEADA               | */
    /* | MAC_VAP_STATE_UP                 | 0x37BDEACA               | */
    /* | MAC_VAP_STATE_PAUSE              | 0                        | */
    /* | MAC_VAP_STATE_AP_WAIT_START      | 0x37BDEADA               | */
    /* | MAC_VAP_STATE_AP_PAUSE           | 0                        | */
    /* +----------------------------------+--------------------------+ */
    for (ul_state = MAC_VAP_STATE_INIT; ul_state < MAC_VAP_AP_STATE_BUTT; ul_state++) {
        g_ast_multi_staup_ap_rx_filter[ul_state] = (1<<21);
    }

    g_ast_multi_staup_ap_rx_filter[MAC_VAP_STATE_INIT] = 0x33B9FADA;
    g_ast_multi_staup_ap_rx_filter[MAC_VAP_STATE_UP] = 0x33B9FACA;
    g_ast_multi_staup_ap_rx_filter[MAC_VAP_STATE_AP_WAIT_START] = 0x33B9FACA;

#endif
}


oal_void hmac_rx_filter_init(oal_void)
{
    g_en_rx_filter_enable = OAL_TRUE;

    /* Normal模式帧过滤值初始化 */
    hmac_rx_filter_init_single_vap();
    hmac_rx_filter_init_multi_vap();
}


oal_void hmac_rx_filter_exit(oal_void)
{
    g_en_rx_filter_enable = OAL_FALSE;
}

#ifdef _PRE_DEBUG_MODE

oal_void hmac_set_rx_filter_en(oal_uint8 uc_rx_filter_en)
{
    g_en_rx_filter_enable = uc_rx_filter_en;
}


oal_void hmac_get_rx_filter_en(oal_void)
{
    oal_int8 ac_tmp_buff[200];
    oal_int8 *pac_string[] = {"close", "open", "error"};

    if (snprintf_s(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff), OAL_SIZEOF(ac_tmp_buff)-1, "rx filer function is : %s.\n",
        pac_string[g_en_rx_filter_enable]) < 0) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hmac_get_rx_filter_en snprintf_s failed.\n");
    }
    oam_print(ac_tmp_buff);
}


oal_uint32 hmac_set_status_rx_filter_val(oal_ulong uc_dev_mode,
                                         oal_ulong uc_vap_mode,
                                         oal_ulong uc_vap_status,
                                         oal_uint32 uc_val)
{
    switch (uc_dev_mode) {
        case 0:
            g_ast_single_vap_rx_filter[uc_vap_mode][uc_vap_status] = uc_val;
            break;

        case 1:
            g_ast_multi_staup_sta_rx_filter[uc_vap_status] = uc_val;
            break;

        default:
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_set_status_rx_filter_val::uc_dev_mode is exceed.[%d]}", uc_dev_mode);
            return OAL_FAIL;
    }

    return OAL_SUCC;
}


oal_uint32 hmac_get_status_rx_filter_val(oal_ulong uc_dev_mode, oal_ulong uc_vap_mode, oal_ulong uc_vap_status)
{
    oal_uint32 uc_val;
    oal_int8 ac_tmp_buff[200];
    oal_int8 *pac_vapmode2string[] = {"CONFIG", "STA", "AP", "WDS", "MONITOER", "TEST", "error"};
    oal_int8 *pac_vapstatus2string[] = {"INIT", "UP", "PAUSE", "WAIT_START", "AP_PAUSE",
                                        "FAKE_UP", "WAIT_SCAN", "SCAN_COMP", "WAIT_JOIN", "JOIN_COMP",
                                        "WAIT_AUTH_SEQ2", "WAIT_AUTH_SEQ4", "AUTH_COMP", "STA_WAIT_ASOC",
                                        "STA_OBSS_SCAN", "STA_BG_SCAN", "error"};
    switch (uc_dev_mode) {
        case 0:
            uc_val = g_ast_single_vap_rx_filter[uc_vap_mode][uc_vap_status];
            break;

        case 1:
            uc_val = g_ast_multi_staup_sta_rx_filter[uc_vap_status];
            break;

        default:
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_get_status_rx_filter_val::uc_dev_mode is exceed.[%d]}", uc_dev_mode);
            return OAL_FAIL;
    }

    if (snprintf_s(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff), OAL_SIZEOF(ac_tmp_buff)-1,
        "The %s when it is %s status. \r\nThe rx filter value is 0x%08x.\r\n",
        pac_vapmode2string[uc_vap_mode], pac_vapstatus2string[uc_vap_status], uc_val) < 0) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hmac_get_status_rx_filter_val snprintf_s failed.\n");
        return OAL_FAIL;
    }

    oam_print(ac_tmp_buff);

    return OAL_SUCC;
}
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

