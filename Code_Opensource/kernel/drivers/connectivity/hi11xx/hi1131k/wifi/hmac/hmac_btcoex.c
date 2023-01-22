

#ifndef __HMAC_BTCOEX_C__
#define __HMAC_BTCOEX_C__

#ifdef _PRE_WLAN_FEATURE_1131K_BTCOEX
/* 1 头文件包含 */
#include "hmac_vap.h"
#include "hmac_ext_if.h"
#include "mac_data.h"
#include "hmac_resource.h"
#include "hmac_btcoex.h"
#include "hmac_fsm.h"
#include "hmac_mgmt_sta.h"
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
#include "hmac_auto_adjust_freq.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_BTCOEX_C

/* 3 函数实现 */
/*
 * 功能描述  : 每个tid删除BA
 * 修改历史  :
 * 1.日    期  : 2020年2月24日
 *   修改内容  : 新生成函数
 */
OAL_STATIC uint32_t hmac_btcoex_delba_foreach_tid(mac_vap_stru *mac_vap,
                                                  mac_user_stru *mac_user,
                                                  mac_cfg_delba_req_param_stru *mac_delba_param)
{
    uint32_t ret;

    oal_set_mac_addr(mac_delba_param->auc_mac_addr, mac_user->auc_user_mac_addr);

    for (mac_delba_param->uc_tidno = 0; mac_delba_param->uc_tidno < WLAN_TID_MAX_NUM; mac_delba_param->uc_tidno++) {
        ret = hmac_config_delba_req(mac_vap, 0, (uint8_t *)mac_delba_param);
        if (ret != OAL_SUCC) {
            OAM_WARNING_LOG2(mac_vap->uc_vap_id, OAM_SF_COEX, "{hmac_btcoex_delba_foreach_tid::ret: %d, tid: %d}",
                             ret, mac_delba_param->uc_tidno);
            return ret;
        }
    }

    return ret;
}

/*
 * 功能描述  : hmac删除BA
 * 修改历史  :
 * 1.日    期  : 2020年2月24日
 *   修改内容  : 新生成函数
 */
uint32_t hmac_btcoex_delba_from_user(mac_vap_stru *mac_vap, hmac_user_stru *hmac_user)
{
    uint32_t                     ret;
    hmac_vap_stru                *hmac_vap = OAL_PTR_NULL;
    mac_cfg_delba_req_param_stru mac_cfg_delba_param;

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    if (OAL_UNLIKELY(hmac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(mac_vap->uc_vap_id, OAM_SF_COEX, "{hmac_btcoex_delba_from_user::hmac_vap is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    mac_cfg_delba_param.en_direction = MAC_RECIPIENT_DELBA;

    if (hmac_vap->hmac_vap_btcoex.hmac_btcoex_addba_req.delba_enable == OAL_FALSE) {
        OAM_WARNING_LOG0(0, OAM_SF_COEX, "{hmac_btcoex_delba_from_user::DO NOT DELBA.}");
        return OAL_FAIL;
    }

    ret = hmac_btcoex_delba_foreach_tid(mac_vap, &(hmac_user->st_user_base_info), &mac_cfg_delba_param);

    return ret;
}

/*
 * 功能描述  : hmac处理dmac抛出的删除BA事件
 * 修改历史  :
 * 1.日    期  : 2020年2月24日
 *   修改内容  : 新生成函数
 */
uint32_t hmac_btcoex_rx_delba_trigger(mac_vap_stru *mac_vap, uint8_t len, uint8_t *param)
{
    hmac_vap_stru                                   *hmac_vap = OAL_PTR_NULL;
    hmac_user_stru                                  *hmac_user = OAL_PTR_NULL;
    dmac_to_hmac_btcoex_rx_delba_trigger_event_stru *dmac_to_hmac_btcoex_rx_delba = OAL_PTR_NULL;
    uint32_t                                        ret;

    if (OAL_UNLIKELY(param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(mac_vap->uc_vap_id, OAM_SF_COEX, "{hmac_btcoex_rx_delba_trigger::param is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    dmac_to_hmac_btcoex_rx_delba = (dmac_to_hmac_btcoex_rx_delba_trigger_event_stru *)param;

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    if (OAL_UNLIKELY(hmac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(mac_vap->uc_vap_id, OAM_SF_COEX, "{hmac_btcoex_rx_delba_trigger::hmac_vap is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_user = mac_res_get_hmac_user(dmac_to_hmac_btcoex_rx_delba->user_id);
    if (OAL_UNLIKELY(hmac_user == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(mac_vap->uc_vap_id, OAM_SF_COEX,
                       "{hmac_btcoex_rx_delba_trigger::hmac_user is null! user_id is %d.}",
                       dmac_to_hmac_btcoex_rx_delba->user_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_vap->hmac_vap_btcoex.ba_size = dmac_to_hmac_btcoex_rx_delba->ba_size;
    OAM_WARNING_LOG2(mac_vap->uc_vap_id, OAM_SF_COEX, "{hmac_btcoex_rx_delba_trigger:delba size: %d, need_delba %d}",
                     dmac_to_hmac_btcoex_rx_delba->ba_size, dmac_to_hmac_btcoex_rx_delba->need_delba);

    if (dmac_to_hmac_btcoex_rx_delba->need_delba) {
        ret = hmac_btcoex_delba_from_user(mac_vap, hmac_user);
        if (ret != OAL_SUCC) {
            OAM_WARNING_LOG1(mac_vap->uc_vap_id, OAM_SF_COEX,
                             "{hmac_btcoex_rx_delba_trigger:delba send failed:ret: %d}", ret);
            return ret;
        }
    }

    return OAL_SUCC;
}

/*
 * 功能描述  : hmac记录异常兼容性AP的地址
 * 修改历史  :
 * 1.日    期  : 2020年2月24日
 *   修改内容  : 新生成函数
 */
uint32_t hmac_btcoex_check_exception_in_list(hmac_vap_stru *hmac_vap, uint8_t *addr)
{
    hmac_btcoex_delba_exception_stru *btcoex_exception = OAL_PTR_NULL;
    uint8_t                          index;
    hmac_device_stru                 *hmac_device = OAL_PTR_NULL;

    hmac_device = hmac_res_get_mac_dev(hmac_vap->st_vap_base_info.uc_device_id);
    if (hmac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_COEX,
                       "{hmac_btcoex_check_exception_in_list::hmac_device null}");
        return OAL_FALSE;
    }

    for (index = 0; index < MAX_BTCOEX_BSS_IN_BL; index++) {
        btcoex_exception = &(hmac_device->hmac_device_btcoex.hmac_btcoex_delba_exception[index]);

        if ((btcoex_exception->used != 0) &&
            (oal_compare_mac_addr(btcoex_exception->user_mac_addr, addr) == 0)) {
            OAM_WARNING_LOG4(0, OAM_SF_COEX,
                "{hmac_btcoex_check_exception_in_list::Find in blacklist, addr->%02x:%02x:XX:XX:%02x:%02x.}",
                addr[0], addr[1], addr[4], addr[5]);
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}

/*
 * 功能描述  : hmac记录异常兼容性AP的地址
 * 修改历史  :
 * 1.日    期  : 2020年2月24日
 *   修改内容  : 新生成函数
 */
OAL_STATIC void hmac_btcoex_add_exception_to_list(hmac_vap_stru *hmac_vap, uint8_t *mac_addr)
{
    hmac_btcoex_delba_exception_stru *btcoex_exception = OAL_PTR_NULL;
    hmac_device_btcoex_stru          *hmac_device_btcoex = OAL_PTR_NULL;
    hmac_device_stru                 *hmac_device = OAL_PTR_NULL;

    hmac_device = hmac_res_get_mac_dev(hmac_vap->st_vap_base_info.uc_device_id);
    if (hmac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_COEX,
                       "{hmac_btcoex_add_exception_to_list::hmac_device null}");
        return;
    }

    hmac_device_btcoex = &(hmac_device->hmac_device_btcoex);

    if (hmac_device_btcoex->exception_bss_index >= MAX_BTCOEX_BSS_IN_BL) {
        OAM_WARNING_LOG1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_COEX,
                         "{hmac_btcoex_add_exception_to_list::already reach max num:%d.}", MAX_BTCOEX_BSS_IN_BL);
        hmac_device_btcoex->exception_bss_index = 0;
    }
    btcoex_exception = &(hmac_device_btcoex->hmac_btcoex_delba_exception[hmac_device_btcoex->exception_bss_index]);
    oal_set_mac_addr(btcoex_exception->user_mac_addr, mac_addr);
    btcoex_exception->type = 0;
    btcoex_exception->used = 1;

    hmac_device_btcoex->exception_bss_index++;
}

/*
 * 功能描述  : hmac检查AP建立的BA seq是否有误
 * 修改历史  :
 * 1.日    期  : 2020年2月24日
 *   修改内容  : 新生成函数
 */
void hmac_btcoex_check_rx_same_baw_start_from_addba_req(hmac_vap_stru *hmac_vap,
                                                        hmac_user_stru *hmac_user,
                                                        mac_ieee80211_frame_stru *frame_hdr,
                                                        uint8_t *action)
{
    hmac_vap_btcoex_stru       *hmac_vap_btcoex = OAL_PTR_NULL;
    hmac_btcoex_addba_req_stru *hmac_btcoex_addba_req = OAL_PTR_NULL;
    uint16_t                   baw_start;
    uint8_t                    tid;

    hmac_vap_btcoex = &(hmac_vap->hmac_vap_btcoex);

    hmac_btcoex_addba_req = &(hmac_vap_btcoex->hmac_btcoex_addba_req);

    /* 两次收到addba req的start num一样且不是重传帧，认为对端移窗卡死  */
    if (frame_hdr->st_frame_control.bit_retry == OAL_TRUE &&
        frame_hdr->bit_seq_num == hmac_btcoex_addba_req->last_seq_num) {
        OAM_WARNING_LOG0(0, OAM_SF_COEX,
                         "{hmac_btcoex_check_rx_same_baw_start_from_addba_req::retry addba req.}");
        return;
    }

    /* **************************************************************** */
    /*       ADDBA Request Frame - Frame Body                          */
    /* --------------------------------------------------------------- */
    /* | Category | Action | Dialog | Parameters | Timeout | SSN     | */
    /* --------------------------------------------------------------- */
    /* | 1        | 1      | 1      | 2          | 2       | 2       | */
    /* --------------------------------------------------------------- */
    /*                                                                 */
    /* **************************************************************** */
    baw_start = (action[7] >> 4) | (action[8] << 4);
    tid = (action[3] & 0x3C) >> 2;
    if (tid != 0) {
        OAM_WARNING_LOG1(0, OAM_SF_COEX,
                         "{hmac_btcoex_check_rx_same_baw_start_from_addba_req::tid != 0, tid = %d.}", tid);
        return;
    }

    if ((baw_start != 0) && (baw_start == hmac_btcoex_addba_req->last_baw_start) &&
        (hmac_vap_btcoex->rx_no_pkt_count > 2)) {
        OAM_WARNING_LOG1(0, OAM_SF_COEX,
            "{hmac_btcoex_check_rx_same_baw_start_from_addba_req::baw_start:%d, delba will forbidden.}", baw_start);
        hmac_btcoex_addba_req->delba_enable = OAL_FALSE;

        if (hmac_btcoex_check_exception_in_list(hmac_vap, frame_hdr->auc_address2) == OAL_FALSE) {
            OAM_WARNING_LOG0(0, OAM_SF_COEX,
                             "{hmac_btcoex_check_rx_same_baw_start_from_addba_req::write down to file.}");
            hmac_btcoex_add_exception_to_list(hmac_vap, frame_hdr->auc_address2);
        }

        /* 发送去认证帧到AP */
        hmac_mgmt_send_disassoc_frame(&hmac_vap->st_vap_base_info,
                                      frame_hdr->auc_address2,
                                      MAC_UNSPEC_REASON,
                                      hmac_user->st_user_base_info.st_cap_info.bit_pmf_active);

        /* 删除对应用户 */
        hmac_user_del(&hmac_vap->st_vap_base_info, hmac_user);
        /* 设置状态为FAKE UP */
        hmac_fsm_change_state(hmac_vap, MAC_VAP_STATE_STA_FAKE_UP);
        hmac_sta_handle_disassoc_rsp(hmac_vap, MAC_UNSPEC_REASON, DMAC_DISASOC_MISC_KICKUSER);
    }

    hmac_btcoex_addba_req->last_baw_start = baw_start;
    hmac_btcoex_addba_req->last_seq_num = frame_hdr->bit_seq_num;
}

/*
 * 功能描述  : 加入ap 共存黑名单
 * 修改历史  :
 * 1.日    期  : 2020年8月17日
 *   修改内容  : 新生成函数
 */
void hmac_btcoex_compability_ap_no_delba_proc(hmac_vap_stru *hmac_vap, uint8_t *addr)
{
    if (hmac_btcoex_check_exception_in_list(hmac_vap, addr) == OAL_FALSE) {
        OAM_WARNING_LOG0(0, OAM_SF_COEX,
                         "{hmac_btcoex_compability_ap_no_delba_proc::add ap to btcoex blacklist.}");
        hmac_btcoex_add_exception_to_list(hmac_vap, addr);
    }
}

/*
 * 功能描述  : 检查是否使能删除BA
 * 修改历史  :
 * 1.日    期  : 2020年2月24日
 *   修改内容  : 新生成函数
 */
void hmac_btcoex_delba_enable_check(hmac_vap_stru *hmac_sta, uint8_t *addr)
{
    hmac_sta->hmac_vap_btcoex.hmac_btcoex_addba_req.delba_enable = OAL_TRUE;

    if (hmac_btcoex_check_exception_in_list(hmac_sta, addr) == OAL_TRUE) {
        OAM_WARNING_LOG0(hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_COEX,
                         "{hmac_btcoex_delba_enable_check::mac_addr in blacklist.}");
        hmac_sta->hmac_vap_btcoex.hmac_btcoex_addba_req.delba_enable = OAL_FALSE;
    }
}

/*
 * 功能描述  : arp超时处理
 * 修改历史  :
 * 1.日    期  : 2020年2月24日
 *   修改内容  : 新生成函数
 */
OAL_STATIC uint32_t hmac_delba_send_timeout(void *arg)
{
    hmac_vap_stru                    *hmac_vap = OAL_PTR_NULL;
    hmac_user_stru                   *hmac_user = OAL_PTR_NULL;
    mac_vap_stru                     *mac_vap = OAL_PTR_NULL;
    hmac_vap_btcoex_stru             *hmac_vap_btcoex = OAL_PTR_NULL;
    hmac_btcoex_arp_req_process_stru *hmac_btcoex_arp_req_process = OAL_PTR_NULL;
    uint32_t                         val;

    hmac_vap = (hmac_vap_stru *)arg;
    if (hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{hmac_delba_send_timeout::hmac_vap is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    mac_vap = &(hmac_vap->st_vap_base_info);

    hmac_user = mac_res_get_hmac_user(mac_vap->uc_assoc_vap_id);
    if (OAL_UNLIKELY(hmac_user == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{hmac_delba_send_timeout::hmac_user is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_vap_btcoex = &(hmac_vap->hmac_vap_btcoex);

    hmac_btcoex_arp_req_process = &(hmac_vap_btcoex->hmac_btcoex_arp_req_process);

    val = oal_atomic_read(&(hmac_btcoex_arp_req_process->rx_unicast_pkt_to_lan));
    if (val == 0) {
        OAM_WARNING_LOG0(mac_vap->uc_vap_id, OAM_SF_COEX, "{hmac_delba_send_timeout:rx_pkt:0}");

        hmac_vap_btcoex->rx_no_pkt_count++;
        if (hmac_vap_btcoex->rx_no_pkt_count > 2) {
            hmac_btcoex_delba_from_user(mac_vap, hmac_user);
        }
    } else {
        hmac_vap_btcoex->rx_no_pkt_count = 0;
    }

    oal_atomic_set(&hmac_btcoex_arp_req_process->rx_unicast_pkt_to_lan, 0);

    return OAL_SUCC;
}

/*
 * 功能描述  : arp流程启动定时器
 * 修改历史  :
 * 1.日    期  : 2020年2月24日
 *   修改内容  : 新生成函数
 */
void hmac_btcoex_arp_fail_delba(oal_netbuf_stru *netbuf, hmac_vap_stru *hmac_vap)
{
    uint8_t                          data_type;
    mac_vap_stru                     *mac_vap = OAL_PTR_NULL;
    hmac_btcoex_arp_req_process_stru *hmac_btcoex_arp_req_process = OAL_PTR_NULL;
    hmac_vap_btcoex_stru             *hmac_vap_btcoex = OAL_PTR_NULL;
    mac_ether_header_stru            *mac_ether_hdr = OAL_PTR_NULL;

    mac_vap = &(hmac_vap->st_vap_base_info);
    if (mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_STA) {
        return;
    }
    if ((hmac_vap->hmac_vap_btcoex.ba_size <= 0) || (hmac_vap->hmac_vap_btcoex.ba_size >= WLAN_AMPDU_RX_BA_LUT_WSIZE)) {
        return;
    }

    /*
     *                      ARP Frame Format
     * |以太网目的地址|以太网源地址|帧类型|硬件类型|协议类型|硬件地址长度|
     * | 6            |6(待替换)   |2     |2       |2       |1           |
     * |协议地址长度|op|发送端以太网地址|发送端IP地址|目的以太网地址|目的IP地址
     * | 1          |2 |6(待替换)       |4           |6             |4
     */
    mac_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(netbuf);

    hmac_vap_btcoex = &(hmac_vap->hmac_vap_btcoex);

    /* 参数外面已经做检查，里面没必要再做检查了 */
    data_type = mac_get_data_type_from_8023((uint8_t *)mac_ether_hdr, MAC_NETBUFF_PAYLOAD_ETH);

    hmac_btcoex_arp_req_process = &(hmac_vap_btcoex->hmac_btcoex_arp_req_process);

    /* 发送方向创建定时器 */
    if ((data_type == MAC_DATA_ARP_REQ) &&
        (hmac_btcoex_arp_req_process->delba_opt_timer.en_is_registerd == OAL_FALSE)) {
        /* 每次重启定时器之前清零,保证统计的时间 */
        oal_atomic_set(&(hmac_btcoex_arp_req_process->rx_unicast_pkt_to_lan), 0);

        FRW_TIMER_CREATE_TIMER(&(hmac_btcoex_arp_req_process->delba_opt_timer),
                               hmac_delba_send_timeout,
                               300,
                               hmac_vap,
                               OAL_FALSE,
                               OAM_MODULE_ID_HMAC,
                               mac_vap->ul_core_id);
    }
}

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
/*
 * 功能描述  : hmac处理dmac抛出的自动调频使能事件
 * 修改历史  :
 * 1.日    期  : 2020年7月9日
 *   修改内容  : 新生成函数
 */
uint32_t hmac_btcoex_rx_auto_freq_enable(mac_vap_stru *mac_vap, uint8_t len, uint8_t *param)
{
    dmac_to_hmac_btcoex_auto_freq_stru *dmac_to_hmac_auto_freq_enable = OAL_PTR_NULL;

    if (OAL_UNLIKELY(param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(mac_vap->uc_vap_id, OAM_SF_COEX, "{hmac_btcoex_rx_auto_freq_enable::param is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    dmac_to_hmac_auto_freq_enable = (dmac_to_hmac_btcoex_auto_freq_stru *)param;

    OAM_WARNING_LOG1(mac_vap->uc_vap_id, OAM_SF_COEX, "hmac_btcoex_rx_auto_freq_enable::set auto freq [%d]",
                     dmac_to_hmac_auto_freq_enable->btcoex_auto_freq_enable);

    if (dmac_to_hmac_auto_freq_enable->btcoex_auto_freq_enable == OAL_TRUE) {
        hmac_set_auto_freq_mod(FREQ_LOCK_ENABLE);
    } else {
        hmac_set_auto_freq_mod(FREQ_LOCK_DISABLE);
    }

    return OAL_SUCC;
}
#endif

OAL_STATIC uint32_t hmac_config_psm_query_rsp_proc(hmac_psm_flt_stat_query_stru *hmac_psm_query,
                                                   dmac_to_hmac_psm_txrx_pkts_stru *psm_chip_info)
{
    mac_psm_query_stat_stru *psm_stat = &hmac_psm_query->psm_stat;

    // 返回的内容中包含4个信息
    psm_stat->query_item = 4;
    psm_stat->val[0] = psm_chip_info->arpoffload_drop_frame_cnt;
    psm_stat->val[1] = psm_chip_info->arpoffload_send_rsp_cnt;
    psm_stat->val[2] = psm_chip_info->apf_flt_drop_cnt;
    psm_stat->val[3] = psm_chip_info->icmp_flt_drop_cnt;

    return OAL_SUCC;
}

uint32_t hmac_psm_chr_get_chip_info(mac_vap_stru *mac_vap, uint8_t len, uint8_t *param)
{
    dmac_to_hmac_psm_txrx_pkts_stru *psm_chip_info = OAL_PTR_NULL;
    hmac_device_stru *hmac_device = OAL_PTR_NULL;
    hmac_psm_flt_stat_query_stru *hmac_psm_query = OAL_PTR_NULL;

    if (param == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(mac_vap->uc_vap_id, OAM_SF_COEX, "{hmac_psm_chr_get_chip_info::param is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    hmac_device = hmac_res_get_mac_dev(mac_vap->uc_device_id);
    if (hmac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "hmac_config_query_psm_rsp: hmac_device is null ptr");
        return OAL_ERR_CODE_PTR_NULL;
    }

    psm_chip_info = (dmac_to_hmac_psm_txrx_pkts_stru *)param;
    hmac_psm_query = &hmac_device->psm_flt_stat_query;
    OAM_WARNING_LOG4(0, 0, "hmac_psm_chr_get_chip_info::ao drop [%u] ao send rsp [%u] apf drop [%u] icmp flt [%u]",
        psm_chip_info->arpoffload_drop_frame_cnt, psm_chip_info->arpoffload_send_rsp_cnt,
        psm_chip_info->apf_flt_drop_cnt, psm_chip_info->icmp_flt_drop_cnt);

    if (hmac_config_psm_query_rsp_proc(hmac_psm_query, psm_chip_info) == OAL_SUCC) {
        /* 设置wait条件为true */
        hmac_psm_query->complete_flag = OAL_TRUE;
        OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(hmac_device->psm_flt_stat_query.wait_queue));
    }

    return OAL_SUCC;
}

OAL_STATIC uint32_t hmac_config_psm_beacon_query_rsp_proc(hmac_psm_beacon_query_stru *hmac_psm_query,
                                                          dmac_to_hmac_psm_beacon_pkts_stru *psm_beacon_info)
{
    mac_psm_query_stat_stru *psm_stat = &hmac_psm_query->psm_stat;

    // 返回的内容中包含4个信息
    psm_stat->query_item = 4;
    psm_stat->val[0] = psm_beacon_info->pm_msg_psm_beacon_cnt;
    psm_stat->val[1] = psm_beacon_info->pm_msg_tbtt_cnt;
    psm_stat->val[2] = psm_beacon_info->psm_err_tim_set_cnt;
    psm_stat->val[3] = psm_beacon_info->pm_msg_tim_awake;

    return OAL_SUCC;
}

uint32_t hmac_psm_get_beacon_info(mac_vap_stru *mac_vap, uint8_t len, uint8_t *param)
{
    dmac_to_hmac_psm_beacon_pkts_stru *psm_beacon_info = OAL_PTR_NULL;
    hmac_device_stru *hmac_device = OAL_PTR_NULL;
    hmac_psm_beacon_query_stru *hmac_psm_query = OAL_PTR_NULL;

    if (param == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(mac_vap->uc_vap_id, OAM_SF_COEX, "{hmac_psm_get_beacon_info::param is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    hmac_device = hmac_res_get_mac_dev(mac_vap->uc_device_id);
    if (hmac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "hmac_config_query_psm_rsp: hmac_device is null ptr");
        return OAL_ERR_CODE_PTR_NULL;
    }

    psm_beacon_info = (dmac_to_hmac_psm_beacon_pkts_stru *)param;
    hmac_psm_query = &hmac_device->psm_beacon_query;
    OAM_WARNING_LOG4(0, 0, "hmac_psm_get_beacon_info::beacon [%u] tbtt [%u] beacon tim err [%u] beacon tim [%u]",
        psm_beacon_info->pm_msg_psm_beacon_cnt, psm_beacon_info->pm_msg_tbtt_cnt,
        psm_beacon_info->psm_err_tim_set_cnt, psm_beacon_info->pm_msg_tim_awake);

    if (hmac_config_psm_beacon_query_rsp_proc(hmac_psm_query, psm_beacon_info) == OAL_SUCC) {
        /* 设置wait条件为true */
        hmac_psm_query->complete_flag = OAL_TRUE;
        OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(hmac_device->psm_beacon_query.wait_queue));
    }

    return OAL_SUCC;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
#endif

