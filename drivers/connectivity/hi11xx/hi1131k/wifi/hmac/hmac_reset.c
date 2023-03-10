

// 1 头文件包含
#include "mac_device.h"
#include "mac_resource.h"
#include "hmac_vap.h"
#include "oal_cfg80211.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_RESET_C

// 2 全局变量定义
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
extern oal_uint32 hmac_config_send_event(mac_vap_stru *pst_mac_vap,
                                         wlan_cfgid_enum_uint16 en_cfg_id,
                                         oal_uint16 us_len,
                                         const oal_uint8 *puc_param);

#endif
// 3 函数实现
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

oal_uint32 hmac_reset_sys_event(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_device_stru *pst_mac_dev;
    mac_reset_sys_stru *pst_reset_sys;

    pst_reset_sys = (mac_reset_sys_stru *)puc_param;

    pst_mac_dev = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_mac_dev == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_reset_sys_event::pst_mac_dev null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    switch (pst_reset_sys->en_reset_sys_type) {
        case MAC_RESET_STATUS_SYS_TYPE:
            if ((pst_mac_dev->uc_device_reset_in_progress == OAL_TRUE) && (pst_reset_sys->uc_value == OAL_FALSE)) {
                pst_mac_dev->us_device_reset_num++;
            }

            hmac_config_set_reset_state(pst_mac_vap, uc_len, puc_param);
            break;
        case MAC_RESET_SWITCH_SYS_TYPE:
            pst_mac_dev->en_reset_switch = pst_reset_sys->uc_value;
            break;
        default:
            break;
    }

    return OAL_SUCC;
}

uint8_t hmac_get_rate_info_flag(dmac_query_station_info_response_event_ex *query_station_reponse)
{
    uint8_t flag = 0;

    if (query_station_reponse == OAL_PTR_NULL) {
        return flag;
    }
    /*
     * 速率flag因内核版本而异，而DMAC依3.8.0为准给出全部结果，在此提取并转换flags
     * linux < 3.5.0
     *     RATE_INFO_FLAGS_MCS             = 1<<0,
     *     RATE_INFO_FLAGS_40_MHZ_WIDTH    = 1<<1,
     *     RATE_INFO_FLAGS_SHORT_GI        = 1<<2,
     *
     * linux >= 3.5.0 && linux < 3.8.0
     *    RATE_INFO_FLAGS_MCS             = 1<<0,
     *    RATE_INFO_FLAGS_40_MHZ_WIDTH    = 1<<1,
     *    RATE_INFO_FLAGS_SHORT_GI        = 1<<2,
     *    RATE_INFO_FLAGS_60G             = 1<<3
     *
     * linux >= 3.8.0&& linux < 4.0.0
     *    RATE_INFO_FLAGS_MCS             = BIT(0),
     *    RATE_INFO_FLAGS_VHT_MCS         = BIT(1),
     *    RATE_INFO_FLAGS_40_MHZ_WIDTH    = BIT(2),
     *    RATE_INFO_FLAGS_80_MHZ_WIDTH    = BIT(3),
     *    RATE_INFO_FLAGS_80P80_MHZ_WIDTH = BIT(4),
     *    RATE_INFO_FLAGS_160_MHZ_WIDTH   = BIT(5),
     *    RATE_INFO_FLAGS_SHORT_GI        = BIT(6),
     *    RATE_INFO_FLAGS_60G             = BIT(7),
     *
     * linux >= 4.0.0
     * rate_info_flags
     *    RATE_INFO_FLAGS_MCS             = BIT(0),
     *    RATE_INFO_FLAGS_VHT_MCS         = BIT(1),
     *    RATE_INFO_FLAGS_SHORT_GI        = BIT(2),
     *    RATE_INFO_FLAGS_60G             = BIT(3),
     * rate_info_bw
     *    RATE_INFO_BW_5
     *    RATE_INFO_BW_10
     *    RATE_INFO_BW_20
     *    RATE_INFO_BW_40
     *    RATE_INFO_BW_80
     *    RATE_INFO_BW_160
     */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0))
    flag |= ((query_station_reponse->st_txrate.flags &
        MAC_RATE_INFO_FLAGS_MCS) ? RATE_INFO_FLAGS_MCS : 0);
    flag |= ((query_station_reponse->st_txrate.flags &
        MAC_RATE_INFO_FLAGS_40_MHZ_WIDTH) ? RATE_INFO_FLAGS_40_MHZ_WIDTH : 0);
    flag |= ((query_station_reponse->st_txrate.flags &
        MAC_RATE_INFO_FLAGS_SHORT_GI) ? RATE_INFO_FLAGS_SHORT_GI : 0);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
    flag |= ((query_station_reponse->st_txrate.flags &
        MAC_RATE_INFO_FLAGS_60G) ? RATE_INFO_FLAGS_60G : 0);
#endif
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0))
    flag  = query_station_reponse->st_txrate.flags;
#else
    flag |= ((query_station_reponse->st_txrate.flags &
        MAC_RATE_INFO_FLAGS_MCS) ? RATE_INFO_FLAGS_MCS : 0);
    flag |= ((query_station_reponse->st_txrate.flags &
        MAC_RATE_INFO_FLAGS_VHT_MCS) ? RATE_INFO_FLAGS_VHT_MCS : 0);
    flag |= ((query_station_reponse->st_txrate.flags &
        MAC_RATE_INFO_FLAGS_SHORT_GI) ? RATE_INFO_FLAGS_SHORT_GI : 0);
    flag |= ((query_station_reponse->st_txrate.flags &
        MAC_RATE_INFO_FLAGS_60G) ? RATE_INFO_FLAGS_60G : 0);
#endif
    return flag;
}

oal_uint32 hmac_proc_query_response_event(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_vap_stru *pst_hmac_vap = OAL_PTR_NULL;
    dmac_query_station_info_response_event_ex *pst_query_station_reponse_event = OAL_PTR_NULL;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_query_response::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_query_station_reponse_event = (dmac_query_station_info_response_event_ex *)(puc_param);
    if (pst_query_station_reponse_event->query_event == OAL_QUERY_STATION_INFO_EVENT) {
        pst_hmac_vap->station_info.signal = (oal_int8)pst_query_station_reponse_event->ul_signal;
        pst_hmac_vap->station_info.rx_packets = pst_query_station_reponse_event->ul_rx_packets;
        pst_hmac_vap->station_info.tx_packets = pst_query_station_reponse_event->ul_tx_packets;
        pst_hmac_vap->station_info.rx_bytes = pst_query_station_reponse_event->ul_rx_bytes;
        pst_hmac_vap->station_info.tx_bytes = pst_query_station_reponse_event->ul_tx_bytes;
        pst_hmac_vap->station_info.tx_retries = pst_query_station_reponse_event->ul_tx_retries;
        pst_hmac_vap->station_info.rx_dropped_misc = pst_hmac_vap->station_info.rx_dropped_misc +
            pst_query_station_reponse_event->ul_rx_dropped_misc;
        pst_hmac_vap->station_info.tx_failed = pst_query_station_reponse_event->ul_tx_failed;
        pst_hmac_vap->station_info.txrate.mcs = pst_query_station_reponse_event->st_txrate.mcs;
        pst_hmac_vap->station_info.txrate.legacy = pst_query_station_reponse_event->st_txrate.legacy;
        pst_hmac_vap->station_info.txrate.nss = pst_query_station_reponse_event->st_txrate.nss;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
        pst_hmac_vap->station_info.txrate.bw =
            pst_query_station_reponse_event->st_txrate.flags & MAC_RATE_INFO_FLAGS_40_MHZ_WIDTH ? RATE_INFO_BW_40 :
            (pst_query_station_reponse_event->st_txrate.flags & MAC_RATE_INFO_FLAGS_80_MHZ_WIDTH ? RATE_INFO_BW_80 :
            (pst_query_station_reponse_event->st_txrate.flags & MAC_RATE_INFO_FLAGS_160_MHZ_WIDTH ? RATE_INFO_BW_160 :
            RATE_INFO_BW_20));
#endif
        pst_hmac_vap->station_info.txrate.flags = hmac_get_rate_info_flag(pst_query_station_reponse_event);
        pst_hmac_vap->center_freq = oal_ieee80211_channel_to_frequency(pst_mac_vap->st_channel.uc_chan_number,
            (enum ieee80211_band)pst_mac_vap->st_channel.en_band);
        pst_hmac_vap->s_free_power = pst_query_station_reponse_event->s_free_power;
        pst_hmac_vap->station_info_extend.chload = pst_query_station_reponse_event->station_info_extend.chload;
    }
    /* 唤醒wal_sdt_recv_reg_cmd等待的进程 */
    pst_hmac_vap->station_info_query_completed_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

    return OAL_SUCC;
}
#endif


oal_uint32 hmac_config_reset_operate(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    mac_device_stru *pst_mac_dev = OAL_PTR_NULL;
    mac_reset_sys_stru st_reset_sys;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_uint32 ul_ret;
#endif
    oal_int8 *pc_token = OAL_PTR_NULL;
    oal_int8 *pc_end = OAL_PTR_NULL;
    oal_int8 *pc_ctx = OAL_PTR_NULL;
    oal_int8 *pc_sep = " ";

    pst_mac_dev = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_mac_dev == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_config_reset_operate::pst_mac_dev null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取复位信息 */
    pc_token = oal_strtok((oal_int8 *)puc_param, pc_sep, &pc_ctx);
    if (pc_token == NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_reset_operate::pc_token null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    st_reset_sys.en_reset_sys_type = (mac_reset_sys_type_enum_uint8)oal_strtol(pc_token, &pc_end, 16);

    if (st_reset_sys.en_reset_sys_type == MAC_RESET_SWITCH_SET_TYPE) {
        /* 获取Channel List */
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (pc_token == NULL) {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_reset_operate::pc_token null.}");
            return OAL_ERR_CODE_PTR_NULL;
        }

        st_reset_sys.uc_value = (oal_uint8)oal_strtol(pc_token, &pc_end, 16);
        pst_mac_dev->en_reset_switch = st_reset_sys.uc_value;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_RESET_HW_OPERATE, us_len, (oal_uint8 *)&st_reset_sys);
        if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_reset_operate::hmac_config_send_event failed[%d].}", ul_ret);
        }

        return ul_ret;
#endif
    } else if (st_reset_sys.en_reset_sys_type == MAC_RESET_SWITCH_GET_TYPE) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_reset_operate::the reset switch is %d.}", pst_mac_dev->en_reset_switch);
    } else if (st_reset_sys.en_reset_sys_type == MAC_RESET_STATUS_GET_TYPE) {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_reset_operate::the reset status is %d, the reset num is %d.}",
            pst_mac_dev->uc_device_reset_in_progress, pst_mac_dev->us_device_reset_num);
    } else {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_reset_operate::the reset tpye is %d, out of limit.}", st_reset_sys.en_reset_sys_type);
    }

    return OAL_SUCC;
}

/*lint -e19 */
oal_module_symbol(hmac_config_reset_operate);
/*lint +e19 */


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

