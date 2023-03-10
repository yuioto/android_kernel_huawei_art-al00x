

// 1 头文件包含
#include "oal_profiling.h"
#include "oal_net.h"
#include "frw_ext_if.h"
#include "hmac_tx_data.h"
#include "hmac_tx_amsdu.h"
#include "mac_frame.h"
#include "mac_data.h"
#include "hmac_frag.h"
#include "hmac_11i.h"
#include "securec.h"
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
#include "hmac_ext_if.h"
#endif
#ifdef _PRE_WLAN_FEATURE_MCAST /* 组播转单播 */
#include "hmac_m2u.h"
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
#include "hmac_proxy_arp.h"
#endif

#ifdef _PRE_WLAN_FEATURE_WAPI
#include "hmac_wapi.h"
#endif

#ifdef _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN
#include "hmac_traffic_classify.h"
#endif

#include "hmac_crypto_tkip.h"
#include "hmac_device.h"
#include "hmac_resource.h"

#include "hmac_tcp_opt.h"

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#include "hisi_customize_wifi.h"
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_TX_DATA_C


// 2 全局变量定义
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
static oal_uint16 us_noqos_frag_seqnum = 0; /* 保存非qos分片帧seqnum */
#endif

// 3 函数实现
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
oal_uint32 hmac_tx_data(hmac_vap_stru *pst_hmac_vap, oal_netbuf_stru *pst_netbuf);
#endif


OAL_STATIC OAL_INLINE oal_bool_enum_uint8 hmac_tx_is_dhcp(mac_ether_header_stru *pst_ether_hdr)
{
    mac_ip_header_stru *puc_ip_hdr;

    puc_ip_hdr = (mac_ip_header_stru *)(pst_ether_hdr + 1);

    return mac_is_dhcp_port(puc_ip_hdr);
}


OAL_STATIC oal_void hmac_tx_report_dhcp_and_arp(mac_vap_stru *pst_mac_vap, mac_ether_header_stru *pst_ether_hdr,
    oal_uint16 us_ether_len)
{
    oal_bool_enum_uint8 en_flg = OAL_FALSE;

    switch (OAL_HOST2NET_SHORT(pst_ether_hdr->us_ether_type)) {
        case ETHER_TYPE_ARP:
            en_flg = OAL_TRUE;
        break;

        case ETHER_TYPE_IP:
            en_flg = hmac_tx_is_dhcp(pst_ether_hdr);
        break;

        default:
            en_flg = OAL_FALSE;
        break;
    }

    if (en_flg && oam_report_dhcp_arp_get_switch()) {
        if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
            oam_report_eth_frame(pst_ether_hdr->auc_ether_dhost, (oal_uint8 *)pst_ether_hdr, us_ether_len,
                OAM_OTA_FRAME_DIRECTION_TYPE_TX);
        } else if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
            oam_report_eth_frame(pst_mac_vap->auc_bssid, (oal_uint8 *)pst_ether_hdr, us_ether_len,
                OAM_OTA_FRAME_DIRECTION_TYPE_TX);
        }
    }
}


oal_uint32 hmac_tx_report_eth_frame(mac_vap_stru *pst_mac_vap, oal_netbuf_stru *pst_netbuf)
{
    oal_uint16            us_user_idx = 0;
    mac_ether_header_stru *pst_ether_hdr = OAL_PTR_NULL;
    oal_uint32            ul_ret;
    oal_uint8             auc_user_macaddr[WLAN_MAC_ADDR_LEN] = {0};
    oal_switch_enum_uint8 en_eth_switch = 0;
#ifdef _PRE_WLAN_DFT_STAT
    hmac_vap_stru         *pst_hmac_vap;
#endif

    if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL) || OAL_UNLIKELY(pst_netbuf == OAL_PTR_NULL)) {
        OAM_ERROR_LOG2(0, OAM_SF_TX, "{hmac_tx_report_eth_frame::input null %p %p}", (uintptr_t)pst_mac_vap,
            (uintptr_t)pst_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }
#ifdef _PRE_WLAN_DFT_STAT
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_UNLIKELY(pst_hmac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(0, OAM_SF_TX, "{hmac_tx_report_eth_frame::mac_res_get_hmac_vap fail. vap_id = %u}",
            pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

#endif

    /* 统计以太网下来的数据包统计 */
    HMAC_VAP_DFT_STATS_PKT_INCR(pst_hmac_vap->st_query_stats.ul_rx_pkt_to_lan, 1);
    HMAC_VAP_DFT_STATS_PKT_INCR(pst_hmac_vap->st_query_stats.ul_rx_bytes_to_lan, OAL_NETBUF_LEN(pst_netbuf));
    OAM_STAT_VAP_INCR(pst_mac_vap->uc_vap_id, tx_pkt_num_from_lan, 1);
    OAM_STAT_VAP_INCR(pst_mac_vap->uc_vap_id, tx_bytes_from_lan, OAL_NETBUF_LEN(pst_netbuf));

    /* 获取目的用户资源池id和用户MAC地址，用于过滤 */
    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_netbuf);
        if (OAL_UNLIKELY(pst_ether_hdr == OAL_PTR_NULL)) {
            OAM_ERROR_LOG0(0, OAM_SF_TX, "{hmac_tx_report_eth_frame::ether_hdr is null!\r\n");
            return OAL_ERR_CODE_PTR_NULL;
        }

        ul_ret = mac_vap_find_user_by_macaddr(pst_mac_vap, pst_ether_hdr->auc_ether_dhost, &us_user_idx);
        if (ul_ret == OAL_ERR_CODE_PTR_NULL) {
            OAM_ERROR_LOG1(0, OAM_SF_TX, "{hmac_tx_report_eth_frame::find user return ptr null!!\r\n", ul_ret);
            return ul_ret;
        }

        if (ul_ret == OAL_FAIL) {
            /* 如果找不到用户，该帧可能是dhcp或者arp request，需要上报 */
            hmac_tx_report_dhcp_and_arp(pst_mac_vap, pst_ether_hdr, (oal_uint16)OAL_NETBUF_LEN(pst_netbuf));
            return OAL_SUCC;
        }

        oal_set_mac_addr(auc_user_macaddr, pst_ether_hdr->auc_ether_dhost);
    }

    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        if (pst_mac_vap->us_user_nums == 0) {
            return OAL_SUCC;
        }
        pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_netbuf);
        if (OAL_UNLIKELY(pst_ether_hdr == OAL_PTR_NULL)) {
            OAM_ERROR_LOG0(0, OAM_SF_TX, "{hmac_tx_report_eth_frame::ether_hdr is null!\r\n");
            return OAL_ERR_CODE_PTR_NULL;
        }
        /* 如果找不到用户，该帧可能是dhcp或者arp request，需要上报 */
        hmac_tx_report_dhcp_and_arp(pst_mac_vap, pst_ether_hdr, (oal_uint16)OAL_NETBUF_LEN(pst_netbuf));
        us_user_idx = pst_mac_vap->uc_assoc_vap_id;
        oal_set_mac_addr(auc_user_macaddr, pst_mac_vap->auc_bssid);
    }

    /* 检查打印以太网帧的开关 */
    ul_ret = oam_report_eth_frame_get_switch(us_user_idx, OAM_OTA_FRAME_DIRECTION_TYPE_TX, &en_eth_switch);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_TX, "{hmac_tx_report_eth_frame::get tx eth frame switch fail!\r\n");
        return ul_ret;
    }

    if (en_eth_switch == OAL_SWITCH_ON) {
        /* 将以太网下来的帧上报 */
        ul_ret = oam_report_eth_frame(auc_user_macaddr,
                                      oal_netbuf_data(pst_netbuf),
                                      (oal_uint16)OAL_NETBUF_LEN(pst_netbuf),
                                      OAM_OTA_FRAME_DIRECTION_TYPE_TX);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_TX, "{hmac_tx_report_eth_frame::oam_report_eth_frame return err: 0x%x.}\r\n",
                ul_ret);
        }
    }

    return OAL_SUCC;
}


oal_uint16 hmac_free_netbuf_list(oal_netbuf_stru *pst_buf)
{
    oal_netbuf_stru *pst_buf_tmp = OAL_PTR_NULL;
    mac_tx_ctl_stru *pst_tx_cb = OAL_PTR_NULL;
    oal_uint16      us_buf_num = 0;

    if (pst_buf != OAL_PTR_NULL) {
        pst_tx_cb = (mac_tx_ctl_stru *)OAL_NETBUF_CB(pst_buf);

        while (pst_buf != OAL_PTR_NULL) {
            pst_buf_tmp = oal_netbuf_list_next(pst_buf);
            us_buf_num++;

            pst_tx_cb = (mac_tx_ctl_stru *)OAL_NETBUF_CB(pst_buf);
            
            if ((oal_netbuf_headroom(pst_buf) <  MAC_80211_QOS_HTC_4ADDR_FRAME_LEN) &&
                (pst_tx_cb->pst_frame_header != OAL_PTR_NULL)) {
                OAL_MEM_FREE(pst_tx_cb->pst_frame_header, OAL_TRUE);
                pst_tx_cb->pst_frame_header = OAL_PTR_NULL;
            }

            oal_netbuf_free(pst_buf);

            pst_buf = pst_buf_tmp;
        }
    } else {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_free_netbuf_list::pst_buf is null}");
    }

    return us_buf_num;
}

#ifdef _PRE_WLAN_FEATURE_HS20

oal_void hmac_tx_set_qos_map(oal_netbuf_stru *pst_buf, oal_uint8 *puc_tid)
{
    mac_ether_header_stru *pst_ether_header;
    mac_ip_header_stru    *pst_ip;
    oal_uint8             uc_dscp;
    mac_tx_ctl_stru       *pst_tx_ctl;
    hmac_vap_stru         *pst_hmac_vap = OAL_PTR_NULL;
    oal_uint8             uc_idx;

    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_buf);
    pst_hmac_vap     = (hmac_vap_stru *)mac_res_get_hmac_vap(MAC_GET_CB_TX_VAP_INDEX(pst_tx_ctl));

    /* 获取以太网头 */
    pst_ether_header = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);
    /* 参数合法性检查 */
    if ((pst_hmac_vap == OAL_PTR_NULL) || (pst_ether_header == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_HS20, "{hmac_tx_set_qos_map::The input parameter of \
            QoS_Map_Configure_frame_with_QoSMap_Set_element is OAL_PTR_NULL.}");
        return;
    }

    /* 从IP TOS字段寻找DSCP优先级 */
    /* ---------------------------------
     * tos位定义
     * ---------------------------------
     * |    bit7~bit2      | bit1~bit0 |
     * |    DSCP优先级     | 保留      |
     * ---------------------------------
     */
    /* 偏移一个以太网头，取ip头 */
    pst_ip = (mac_ip_header_stru *)(pst_ether_header + 1);
    uc_dscp = pst_ip->uc_tos >> WLAN_DSCP_PRI_SHIFT;
    OAM_INFO_LOG2(0, OAM_SF_HS20, "{hmac_tx_set_qos_map::tos = %d, uc_dscp=%d.}", pst_ip->uc_tos, uc_dscp);

    if ((pst_hmac_vap->st_cfg_qos_map_param.uc_num_dscp_except > 0) &&
        (pst_hmac_vap->st_cfg_qos_map_param.uc_num_dscp_except <= MAX_DSCP_EXCEPT) &&
        (pst_hmac_vap->st_cfg_qos_map_param.uc_valid)) {
        for (uc_idx = 0; uc_idx < pst_hmac_vap->st_cfg_qos_map_param.uc_num_dscp_except; uc_idx++) {
            if (uc_dscp == pst_hmac_vap->st_cfg_qos_map_param.auc_dscp_exception[uc_idx]) {
               *puc_tid  = pst_hmac_vap->st_cfg_qos_map_param.auc_dscp_exception_up[uc_idx];
                pst_tx_ctl->bit_is_vipframe = OAL_TRUE;
                pst_tx_ctl->bit_is_needretry = OAL_TRUE;
                pst_hmac_vap->st_cfg_qos_map_param.uc_valid = 0;
                return;
            }
        }
    }

    for (uc_idx = 0; uc_idx < MAX_QOS_UP_RANGE; uc_idx++) {
        if ((uc_dscp < pst_hmac_vap->st_cfg_qos_map_param.auc_up_high[uc_idx]) &&
            (uc_dscp > pst_hmac_vap->st_cfg_qos_map_param.auc_up_low[uc_idx])) {
            *puc_tid = uc_idx;
            pst_tx_ctl->bit_is_vipframe = OAL_TRUE;
            pst_tx_ctl->bit_is_needretry = OAL_TRUE;
            pst_hmac_vap->st_cfg_qos_map_param.uc_valid = 0;
            return;
        } else {
            *puc_tid = 0;
        }
    }
    pst_hmac_vap->st_cfg_qos_map_param.uc_valid = 0;
    return;
}
#endif // _PRE_WLAN_FEATURE_HS20

#ifdef _PRE_WLAN_FEATURE_CLASSIFY


OAL_STATIC OAL_INLINE oal_void hmac_tx_classify_lan_to_wlan(oal_netbuf_stru *pst_buf, oal_uint8 *puc_tid)
{
    mac_ether_header_stru *pst_ether_header = OAL_PTR_NULL;
    mac_ip_header_stru    *pst_ip = OAL_PTR_NULL;
    oal_vlan_ethhdr_stru  *pst_vlan_ethhdr = OAL_PTR_NULL;
    oal_uint32            ul_ipv6_hdr;
    oal_uint32            ul_pri;
    oal_uint16            us_vlan_tci;
    oal_uint8             uc_tid = 0;
    mac_tx_ctl_stru       *pst_tx_ctl = OAL_PTR_NULL;
#if defined(_PRE_WLAN_FEATURE_HS20) || defined(_PRE_WLAN_FEATURE_EDCA_OPT_AP) || defined(_PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN)
    hmac_vap_stru         *pst_hmac_vap = OAL_PTR_NULL;
#endif

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
    hmac_user_stru        *pst_hmac_user = OAL_PTR_NULL;
#endif
#ifdef _PRE_WLAN_FEATURE_SCHEDULE
    mac_tcp_header_stru   *pst_tcp = OAL_PTR_NULL;
#endif

    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_buf);
#if defined(_PRE_WLAN_FEATURE_HS20) || defined(_PRE_WLAN_FEATURE_EDCA_OPT_AP) || defined(_PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN)
    pst_hmac_vap   = (hmac_vap_stru *)mac_res_get_hmac_vap(MAC_GET_CB_TX_VAP_INDEX(pst_tx_ctl));
    if  (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::mac_res_get_hmac_vap fail.vap_index[%u]}",
            MAC_GET_CB_TX_VAP_INDEX(pst_tx_ctl));
        return;
    }
#endif
    /* 获取以太网头 */
    pst_ether_header = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);

    switch (pst_ether_header->us_ether_type) {
        /*lint -e778*/ /* 屏蔽Info -- Constant expression evaluates to 0 in operation '&' */
        case OAL_HOST2NET_SHORT(ETHER_TYPE_IP):
            OAM_INFO_LOG0(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::ETHER_TYPE_IP.}");

#ifdef _PRE_WLAN_FEATURE_HS20
            if (pst_hmac_vap->st_cfg_qos_map_param.uc_valid) {
                hmac_tx_set_qos_map(pst_buf, &uc_tid);
               *puc_tid = uc_tid;
                return;
            }
#endif // _PRE_WLAN_FEATURE_HS20

            /* 从IP TOS字段寻找优先级
             * ----------------------------------------------------------------------
             *    tos位定义
             * ----------------------------------------------------------------------
             * | bit7~bit5 | bit4 |  bit3  |  bit2  |   bit1   | bit0 |
             * | 包优先级  | 时延 | 吞吐量 | 可靠性 | 传输成本 | 保留 |
             * ----------------------------------------------------------------------
             */
            pst_ip = (mac_ip_header_stru *)(pst_ether_header + 1);      /* 偏移一个以太网头，取ip头 */

            uc_tid = pst_ip->uc_tos >> WLAN_IP_PRI_SHIFT;

#ifdef _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN
            if (pst_hmac_vap->uc_tx_traffic_classify_flag == OAL_SWITCH_ON) {
                if (uc_tid != 0) {
                    break;
                }
                hmac_tx_traffic_classify(pst_tx_ctl, pst_ip, &uc_tid);
            }
#endif  /* _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN */

            OAM_INFO_LOG2(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::tos = %d, uc_tid=%d.}", pst_ip->uc_tos, uc_tid);
            /* 如果是DHCP帧，则进入VO队列发送 */
            if (mac_is_dhcp_port(pst_ip) == OAL_TRUE) {
                uc_tid = WLAN_DATA_VIP_TID;
                pst_tx_ctl->bit_is_vipframe  = OAL_TRUE;
                pst_tx_ctl->bit_is_needretry = OAL_TRUE;
            }
#ifdef _PRE_WLAN_FEATURE_SCHEDULE
            /* 对于chariot信令报文进行特殊处理，防止断流 */
            else if (pst_ip->uc_protocol == MAC_TCP_PROTOCAL) {
                pst_tcp = (mac_tcp_header_stru *)(pst_ip + 1);

                if ((OAL_NTOH_16(pst_tcp->us_dport) == MAC_CHARIOT_NETIF_PORT) ||
                    (OAL_NTOH_16(pst_tcp->us_sport) == MAC_CHARIOT_NETIF_PORT)) {
                    OAM_INFO_LOG0(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::chariot netif tcp pkt.}");
                    uc_tid = WLAN_DATA_VIP_TID;
                    pst_tx_ctl->bit_is_vipframe  = OAL_TRUE;
                    pst_tx_ctl->bit_is_needretry = OAL_TRUE;
                }
            }
#endif

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
            pst_hmac_user    = (hmac_user_stru *)mac_res_get_hmac_user(MAC_GET_CB_TX_USER_IDX(pst_tx_ctl));
            if (pst_hmac_user == OAL_PTR_NULL) {
                OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_edca_opt_rx_pkts_stat::null param,pst_hmac_user[%d].}",
                    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl));
                break;
            }

            if ((pst_hmac_vap->uc_edca_opt_flag_ap == OAL_TRUE) &&
                (pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_AP)) {
                /* mips优化:解决开启业务统计性能差10M问题 */
                if (((pst_ip->uc_protocol == MAC_UDP_PROTOCAL) &&
                    (pst_hmac_user->aaul_txrx_data_stat[WLAN_WME_TID_TO_AC(uc_tid)][WLAN_TX_UDP_DATA] <
                    (HMAC_EDCA_OPT_PKT_NUM + 10))) ||
                    ((pst_ip->uc_protocol == MAC_TCP_PROTOCAL) &&
                    (pst_hmac_user->aaul_txrx_data_stat[WLAN_WME_TID_TO_AC(uc_tid)][WLAN_TX_TCP_DATA] <
                    (HMAC_EDCA_OPT_PKT_NUM + 10)))) {
                    hmac_edca_opt_tx_pkts_stat(pst_tx_ctl, uc_tid, pst_ip);
                }
            }
#endif
            break;

        case OAL_HOST2NET_SHORT(ETHER_TYPE_IPV6):
            OAM_INFO_LOG0(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::ETHER_TYPE_IPV6.}");
            /* 从IPv6 traffic class字段获取优先级
             * -----------------------------------------------------------------------
             *    IPv6包头 前32为定义
             * -----------------------------------------------------------------------
             * | 版本号 | traffic class   | 流量标识 |
             * | 4bit   | 8bit(同ipv4 tos)|  20bit   |
             * -----------------------------------------------------------------------
             */
            ul_ipv6_hdr = *((oal_uint32 *)(pst_ether_header + 1));  /* 偏移一个以太网头，取ip头 */

            ul_pri = (OAL_NET2HOST_LONG(ul_ipv6_hdr) & WLAN_IPV6_PRIORITY_MASK) >> WLAN_IPV6_PRIORITY_SHIFT; //lint !e60 !e64 !e507

            uc_tid = (oal_uint8)(ul_pri >> WLAN_IP_PRI_SHIFT);
            OAM_INFO_LOG1(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::uc_tid=%d.}", uc_tid);

            /* 如果是ND帧，则进入VO队列发送 */
            if (mac_is_nd((oal_ipv6hdr_stru *)(pst_ether_header + 1)) == OAL_TRUE) {
                uc_tid = WLAN_DATA_VIP_TID;
                pst_tx_ctl->bit_is_vipframe  = OAL_TRUE;
                pst_tx_ctl->bit_is_needretry = OAL_TRUE;
            } else if (mac_is_dhcp6((oal_ipv6hdr_stru *)(pst_ether_header + 1)) == OAL_TRUE) {
                /* 如果是DHCPV6帧，则进入VO队列发送 */
                OAM_INFO_LOG0(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::ETHER_TYPE_DHCP6.}");
                uc_tid = WLAN_DATA_VIP_TID;
                pst_tx_ctl->bit_is_vipframe  = OAL_TRUE;
                pst_tx_ctl->bit_is_needretry = OAL_TRUE;
            }

            break;

        case OAL_HOST2NET_SHORT(ETHER_TYPE_PAE):
            /* 如果是EAPOL帧，则进入VO队列发送 */
            OAM_INFO_LOG0(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::ETHER_TYPE_PAE.}");
            uc_tid = WLAN_DATA_VIP_TID;
            pst_tx_ctl->bit_is_vipframe  = OAL_TRUE;
            pst_tx_ctl->bit_is_needretry = OAL_TRUE;

#ifdef _PRE_DEBUG_MODE
            pst_tx_ctl->en_is_eapol = OAL_TRUE;
            pst_tx_ctl->us_eapol_ts = (oal_uint16)OAL_TIME_GET_STAMP_MS();
            OAM_INFO_LOG0(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::tx EAPOL.}");
#endif

            /* 如果是4 次握手设置单播密钥，则设置tx cb 中bit_is_eapol_key_ptk 置一，dmac 发送不加密 */
            if (mac_is_eapol_key_ptk((mac_eapol_header_stru *)(pst_ether_header + 1)) == OAL_TRUE) {
                pst_tx_ctl->bit_is_eapol_key_ptk = OAL_TRUE;
            }

            OAM_INFO_LOG1(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::uc_tid=%d.}", uc_tid);
            break;

        /* TDLS帧处理，建链保护，入高优先级TID队列 */
        case OAL_HOST2NET_SHORT(ETHER_TYPE_TDLS):
            OAM_INFO_LOG0(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::ETHER_TYPE_TDLS.}");
            uc_tid = WLAN_DATA_VIP_TID;
            OAM_INFO_LOG1(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::uc_tid=%d.}", uc_tid);
            break;

		/* PPPOE帧处理，建链保护(发现阶段, 会话阶段)，入高优先级TID队列 */
        case OAL_HOST2NET_SHORT(ETHER_TYPE_PPP_DISC):
        case OAL_HOST2NET_SHORT(ETHER_TYPE_PPP_SES):
            OAM_INFO_LOG0(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::ETHER_TYPE_PPP_DISC, ETHER_TYPE_PPP_SES.}");
            uc_tid = WLAN_DATA_VIP_TID;
            pst_tx_ctl->bit_is_vipframe  = OAL_TRUE;
            pst_tx_ctl->bit_is_needretry = OAL_TRUE;

            OAM_INFO_LOG1(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::uc_tid=%d.}", uc_tid);
            break;

#ifdef _PRE_WLAN_FEATURE_WAPI
        case OAL_HOST2NET_SHORT(ETHER_TYPE_WAI):
            OAM_INFO_LOG0(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::ETHER_TYPE_WAI.}");
            uc_tid = WLAN_DATA_VIP_TID;
            pst_tx_ctl->bit_is_vipframe  = OAL_TRUE;
            pst_tx_ctl->bit_is_needretry = OAL_TRUE;
            break;
#endif

        case OAL_HOST2NET_SHORT(ETHER_TYPE_VLAN):
            OAM_INFO_LOG0(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::ETHER_TYPE_VLAN.}");
            /* 获取vlan tag的优先级 */
            pst_vlan_ethhdr = (oal_vlan_ethhdr_stru *)oal_netbuf_data(pst_buf);

            /* -------------------------------------------------------------------
             *    802.1Q(VLAN) TCI(tag control information)位定义
             * -------------------------------------------------------------------
             * |Priority | DEI  | Vlan Identifier |
             * | 3bit    | 1bit |      12bit      |
             * ------------------------------------------------------------------
             */
            us_vlan_tci = OAL_NET2HOST_SHORT(pst_vlan_ethhdr->h_vlan_TCI);

            uc_tid = us_vlan_tci >> OAL_VLAN_PRIO_SHIFT;    /* 右移13位，提取高3位优先级 */
            OAM_INFO_LOG1(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::uc_tid=%d.}", uc_tid);

            break;

        case OAL_HOST2NET_SHORT(ETHER_TYPE_ARP):
            /* 如果是ARP帧，则进入VO队列发送 */
            OAM_INFO_LOG0(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::ETHER_TYPE_ARP.}");
            uc_tid = WLAN_DATA_VIP_TID;
            pst_tx_ctl->bit_is_vipframe  = OAL_TRUE;
            OAM_INFO_LOG1(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::uc_tid=%d.}", uc_tid);
            break;

        /*lint +e778*/
        default:
            OAM_INFO_LOG1(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::default us_ether_type[%d].}",
                pst_ether_header->us_ether_type);
            break;
    }

    /* 出参赋值 */
    *puc_tid = uc_tid;
}


OAL_STATIC OAL_INLINE oal_void hmac_tx_update_tid(oal_bool_enum_uint8 en_wmm, oal_uint8 *puc_tid)
{
    if (OAL_LIKELY(en_wmm == OAL_TRUE)) { /* wmm使能 */
        *puc_tid = (*puc_tid < WLAN_TIDNO_BUTT) ? WLAN_TOS_TO_TID(*puc_tid) : WLAN_TIDNO_BCAST;
    } else { /* wmm不使能 */
        *puc_tid = MAC_WMM_SWITCH_TID;
    }
}

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

oal_uint8 hmac_tx_wmm_acm(oal_bool_enum_uint8  en_wmm, hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_tid)
{
    oal_uint8 uc_ac;
    oal_uint8 uc_ac_new;

    if ((pst_hmac_vap == OAL_PTR_NULL) || (puc_tid == OAL_PTR_NULL)) {
        return OAL_FALSE;
    }

    if (en_wmm == OAL_FALSE) {
        return OAL_FALSE;
    }

    uc_ac = WLAN_WME_TID_TO_AC(*puc_tid);
    uc_ac_new = uc_ac;
    while ((uc_ac_new != WLAN_WME_AC_BK) &&
        (pst_hmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_qap_edac[uc_ac_new].en_dot11QAPEDCATableMandatory ==
        OAL_TRUE)) {
        switch (uc_ac_new) {
            case WLAN_WME_AC_VO:
                uc_ac_new = WLAN_WME_AC_VI;
                break;

            case WLAN_WME_AC_VI:
                uc_ac_new = WLAN_WME_AC_BE;
                break;

            default:
                uc_ac_new = WLAN_WME_AC_BK;
                break;
        }
    }

    if (uc_ac_new != uc_ac) {
        *puc_tid = WLAN_WME_AC_TO_TID(uc_ac_new);
    }

    return OAL_TRUE;
}
#endif /* (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST) */


OAL_STATIC OAL_INLINE oal_void  hmac_tx_classify(hmac_vap_stru *pst_hmac_vap, mac_user_stru *pst_user,
    oal_netbuf_stru *pst_buf)
{
    oal_uint8       uc_tid = 0;
    mac_tx_ctl_stru *pst_tx_ctl = OAL_PTR_NULL;
    mac_device_stru *pst_mac_dev = OAL_PTR_NULL;

    hmac_tx_classify_lan_to_wlan(pst_buf, &uc_tid);

    /* 非QoS站点，直接返回 */
    if (OAL_UNLIKELY(pst_user->st_cap_info.bit_qos != OAL_TRUE)) {
        OAM_INFO_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
            "{hmac_tx_classify::user is a none QoS station.}");
        return;
    }

    pst_mac_dev = mac_res_get_dev(pst_user->uc_device_id);
    if (OAL_UNLIKELY(pst_mac_dev == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{hmac_tx_classify::pst_mac_dev null.}");
        return;
    }

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
    hmac_tx_wmm_acm(pst_mac_dev->en_wmm, pst_hmac_vap, &uc_tid);
#endif

    
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_buf);
    if ((pst_tx_ctl->bit_is_vipframe != OAL_TRUE) || (pst_mac_dev->en_wmm == OAL_FALSE)) {
        hmac_tx_update_tid(pst_mac_dev->en_wmm, &uc_tid);
    }

    /* 如果使能了vap流等级，则采用设置的vap流等级 */
    if (pst_mac_dev->en_vap_classify == OAL_TRUE) {
        uc_tid = pst_hmac_vap->uc_classify_tid;
    }

    /* 设置ac和tid到cb字段 */
    pst_tx_ctl->uc_tid  = uc_tid;
    pst_tx_ctl->uc_ac   = WLAN_WME_TID_TO_AC(uc_tid);

    OAM_INFO_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{hmac_tx_classify::uc_ac=%d uc_tid=%d.}",
        pst_tx_ctl->uc_ac, pst_tx_ctl->uc_tid);

    return;
}
#endif


OAL_STATIC OAL_INLINE oal_uint32 hmac_tx_filter_security(hmac_vap_stru *pst_hmac_vap,
                                                         oal_netbuf_stru *pst_buf,
                                                         hmac_user_stru *pst_hmac_user)
{
    mac_ether_header_stru *pst_ether_header = OAL_PTR_NULL;
    mac_user_stru         *pst_mac_user     = OAL_PTR_NULL;
    mac_vap_stru          *pst_mac_vap      = OAL_PTR_NULL;
    oal_uint32            ul_ret           = OAL_SUCC;
    oal_uint16            us_ether_type;

    pst_mac_vap  = &(pst_hmac_vap->st_vap_base_info);
    pst_mac_user = &(pst_hmac_user->st_user_base_info);

    if (pst_mac_vap->pst_mib_info->st_wlan_mib_privacy.en_dot11RSNAActivated == OAL_TRUE) { /* 判断是否使能WPA/WPA2 */
        if (pst_mac_user->en_port_valid != OAL_TRUE) { /* 判断端口是否打开 */
            /* 获取以太网头 */
            pst_ether_header = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);
            /* 发送数据时，针对非EAPOL 的数据帧做过滤 */
            if (oal_byteorder_host_to_net_uint16(ETHER_TYPE_PAE) != pst_ether_header->us_ether_type) {
                us_ether_type = oal_byteorder_host_to_net_uint16(pst_ether_header->us_ether_type);
                OAM_WARNING_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
                    "{hmac_tx_filter_security::TYPE 0x%04x, 0x%04x.}", us_ether_type, ETHER_TYPE_PAE);
                ul_ret = OAL_FAIL;
            }
        }
    }

    return ul_ret;
}


oal_void hmac_tx_ba_setup(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_user, oal_uint8 uc_tidno)
{
    mac_action_mgmt_args_stru st_action_args;   /* 用于填写ACTION帧的参数 */
    oal_bool_enum_uint8       en_ampdu_support;

    /* 建立BA会话，是否需要判断VAP的AMPDU的支持情况，因为需要实现建立BA会话时，一定发AMPDU */
    en_ampdu_support = hmac_user_xht_support(pst_user);
    if (en_ampdu_support) {
        /*
         * 建立BA会话时，st_action_args结构各个成员意义如下
         * (1)uc_category:action的类别
         * (2)uc_action:BA action下的类别
         * (3)ul_arg1:BA会话对应的TID
         * (4)ul_arg2:BUFFER SIZE大小
         * (5)ul_arg3:BA会话的确认策略
         * (6)ul_arg4:TIMEOUT时间
         */
        st_action_args.uc_category = MAC_ACTION_CATEGORY_BA;
        st_action_args.uc_action   = MAC_BA_ACTION_ADDBA_REQ;
        st_action_args.ul_arg1     = uc_tidno;                     /* 该数据帧对应的TID号 */
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
        /* ADDBA_REQ中，buffer_size的默认大小 */
        st_action_args.ul_arg2     = (oal_uint32)g_st_wlan_customize.ul_ampdu_tx_max_num;
        OAM_WARNING_LOG1(0, OAM_SF_TX, "hisi_customize_wifi::[ba buffer size:%d]", st_action_args.ul_arg2);
#else
        st_action_args.ul_arg2     = WLAN_AMPDU_TX_MAX_BUF_SIZE;       /* ADDBA_REQ中，buffer_size的默认大小 */
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

        st_action_args.ul_arg3     = MAC_BA_POLICY_IMMEDIATE;      /* BA会话的确认策略 */
        st_action_args.ul_arg4     = 0;                            /* BA会话的超时时间设置为0 */

        /* 建立BA会话 */
        hmac_mgmt_tx_action(pst_hmac_vap, pst_user, &st_action_args);
    } else {
        if (pst_user->ast_tid_info[uc_tidno].st_ba_tx_info.en_ba_status != DMAC_BA_INIT) {
            st_action_args.uc_category = MAC_ACTION_CATEGORY_BA;
            st_action_args.uc_action   = MAC_BA_ACTION_DELBA;
            st_action_args.ul_arg1     = uc_tidno;                                         /* 该数据帧对应的TID号 */
            st_action_args.ul_arg2     = MAC_ORIGINATOR_DELBA;                             /* 发送端删除ba */
            st_action_args.ul_arg3     = MAC_UNSPEC_REASON;                                /* BA会话删除原因 */
            st_action_args.puc_arg5    = pst_user->st_user_base_info.auc_user_mac_addr;    /* 用户mac地址 */

            /* 删除BA会话 */
            hmac_mgmt_tx_action(pst_hmac_vap, pst_user, &st_action_args);
        }
    }
}


oal_uint32 hmac_tx_ucast_process(hmac_vap_stru *pst_hmac_vap, oal_netbuf_stru *pst_buf, hmac_user_stru *pst_user,
    mac_tx_ctl_stru *pst_tx_ctl)
{
    oal_uint32 ul_ret = HMAC_TX_PASS;

   /* 安全过滤 */
#if defined(_PRE_WLAN_FEATURE_WPA) || defined(_PRE_WLAN_FEATURE_WPA2)
    if (OAL_UNLIKELY(hmac_tx_filter_security(pst_hmac_vap, pst_buf, pst_user) != OAL_SUCC)) {
        OAM_STAT_VAP_INCR(pst_hmac_vap->st_vap_base_info.uc_vap_id, tx_security_check_faild, 1);
        return HMAC_TX_DROP_SECURITY_FILTER;
    }
#endif

    /* 以太网业务识别 */
#ifdef _PRE_WLAN_FEATURE_CLASSIFY
    hmac_tx_classify(pst_hmac_vap, &(pst_user->st_user_base_info), pst_buf);
#endif

    OAL_MIPS_TX_STATISTIC(HMAC_PROFILING_FUNC_TRAFFIC_CLASSIFY);
    /* 如果是EAPOL、DHCP帧，则不允许主动建立BA会话 */
    if (pst_tx_ctl->bit_is_vipframe == OAL_FALSE) {
#ifdef _PRE_WLAN_FEATURE_AMPDU
        if (hmac_tid_need_ba_session(pst_hmac_vap, pst_user, pst_tx_ctl->uc_tid, pst_buf) == OAL_TRUE) {
            /* 自动触发建立BA会话，设置AMPDU聚合参数信息在DMAC模块的处理addba rsp帧的时刻后面 */
            hmac_tx_ba_setup(pst_hmac_vap, pst_user, pst_tx_ctl->uc_tid);
        }
#endif

        OAL_MIPS_TX_STATISTIC(HMAC_PROFILING_FUNC_SETUP_BA);
#ifdef _PRE_WLAN_FEATURE_AMSDU
        ul_ret = hmac_amsdu_notify(pst_hmac_vap, pst_user, pst_buf);
        if (OAL_UNLIKELY(ul_ret != HMAC_TX_PASS)) {
            return ul_ret;
        }
#endif
        OAL_MIPS_TX_STATISTIC(HMAC_PROFILING_FUNC_AMSDU);
    }

    return ul_ret;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32 hmac_nonqos_frame_set_sn(hmac_vap_stru *pst_hmac_vap, mac_tx_ctl_stru *pst_tx_ctl)
{
    pst_tx_ctl->pst_frame_header->bit_seq_num = (us_noqos_frag_seqnum++) & 0x0fff;
    pst_tx_ctl->en_seq_ctrl_bypass = OAL_TRUE;
    return OAL_SUCC;
}

OAL_STATIC oal_bool_enum_uint8 hmac_tx_is_need_frag(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user,
    oal_netbuf_stru *pst_netbuf, mac_tx_ctl_stru *pst_tx_ctl)
{
    oal_uint32 ul_threshold;
    oal_uint32 uc_last_frag;
    /* 判断报文是否需要进行分片
     * 1、长度大于门限
     * 2、是legac协议模式
     * 3、不是广播帧
     * 4、不是聚合帧
     * 6、DHCP帧不进行分片
     */
    if (pst_tx_ctl->bit_is_vipframe == OAL_TRUE) {
        return OAL_FALSE;
    }

    ul_threshold = pst_hmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_operation.ul_dot11FragmentationThreshold;
    ul_threshold = (ul_threshold & (~(BIT0 | BIT1))) + 2;
    /* 规避1151硬件bug,调整分片门限：TKIP加密时，当最后一个分片的payload长度小于等于8时，无法进行加密 */
    if (pst_hmac_user->st_user_base_info.st_key_info.en_cipher_type == WLAN_80211_CIPHER_SUITE_TKIP) {
        uc_last_frag = (OAL_NETBUF_LEN(pst_netbuf) + 8) %
            (ul_threshold - pst_tx_ctl->uc_frame_header_length - (WEP_IV_FIELD_SIZE + EXT_IV_FIELD_SIZE));
        if ((uc_last_frag > 0) && (uc_last_frag <= 8)) {
            ul_threshold = ul_threshold + 8;
        }
    }

    return (oal_bool_enum_uint8)(((OAL_NETBUF_LEN(pst_netbuf) + pst_tx_ctl->uc_frame_header_length) > ul_threshold) &&
        (!pst_tx_ctl->en_ismcast) && (!pst_tx_ctl->en_is_amsdu) &&
        (pst_hmac_user->st_user_base_info.en_cur_protocol_mode < WLAN_HT_MODE ||
        pst_hmac_vap->st_vap_base_info.en_protocol < WLAN_HT_MODE) &&
        (hmac_vap_ba_is_setup(pst_hmac_user, pst_tx_ctl->uc_tid) == OAL_FALSE));
}


#else


OAL_STATIC oal_bool_enum_uint8 hmac_tx_is_need_frag(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user,
    oal_netbuf_stru *pst_netbuf, mac_tx_ctl_stru *pst_tx_ctl)
{
    oal_uint32 ul_threshold;
    /* 判断报文是否需要进行分片
     * 1、长度大于门限
     * 2、是legac协议模式
     * 3、不是广播帧
     * 4、不是聚合帧
     * 6、DHCP帧不进行分片
     */
    if (pst_tx_ctl->bit_is_vipframe == OAL_TRUE) {
        return OAL_FALSE;
    }
    ul_threshold = pst_hmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_operation.ul_dot11FragmentationThreshold;
    ul_threshold = (ul_threshold & (~(BIT0 | BIT1))) + 2;

    return (oal_bool_enum_uint8)(ul_threshold < (OAL_NETBUF_LEN(pst_netbuf)) &&
        (!pst_tx_ctl->en_ismcast) && (!pst_tx_ctl->en_is_amsdu) &&
        (pst_hmac_user->st_user_base_info.en_avail_protocol_mode < WLAN_HT_MODE ||
        pst_hmac_vap->st_vap_base_info.en_protocol < WLAN_HT_MODE) &&
        (hmac_vap_ba_is_setup(pst_hmac_user, pst_tx_ctl->uc_tid) == OAL_FALSE));
}

#endif


oal_uint32 hmac_tx_encap(hmac_vap_stru *pst_vap, hmac_user_stru *pst_user, oal_netbuf_stru *pst_buf)
{
    mac_ieee80211_qos_htc_frame_addr4_stru *pst_hdr = OAL_PTR_NULL;           /* 802.11头 */
    mac_ether_header_stru                  *pst_ether_hdr = OAL_PTR_NULL;
    oal_uint32                             ul_qos = HMAC_TX_BSS_NOQOS;
    mac_tx_ctl_stru                        *pst_tx_ctl = OAL_PTR_NULL;
    oal_uint16                             us_ether_type = 0;
    oal_uint8                              auc_saddr[ETHER_ADDR_LEN];   /* 原地址指针 */
    oal_uint8                              auc_daddr[ETHER_ADDR_LEN];   /* 目的地址指针 */
    oal_uint32                             ul_ret;
    mac_llc_snap_stru                      *pst_snap_hdr = OAL_PTR_NULL;

#ifdef _PRE_DEBUG_MODE
    if (OAL_UNLIKELY((pst_vap == OAL_PTR_NULL) || (pst_buf == OAL_PTR_NULL))) {
        OAM_ERROR_LOG2(0, OAM_SF_TX, "{hmac_tx_encap::param null,%d %d.}", pst_vap, pst_buf);
        return OAL_ERR_CODE_PTR_NULL;
    }
#endif

    /* 获取CB */
    pst_tx_ctl = (mac_tx_ctl_stru *)(oal_netbuf_cb(pst_buf));

    /* 如果skb中data指针前预留的空间大于802.11 mac head len，则不需要格外申请内存存放802.11头 */
    if (oal_netbuf_headroom(pst_buf) >=  MAC_80211_QOS_HTC_4ADDR_FRAME_LEN) {
        pst_hdr = (mac_ieee80211_qos_htc_frame_addr4_stru *)(OAL_NETBUF_HEADER(pst_buf) -
            MAC_80211_QOS_HTC_4ADDR_FRAME_LEN);
        pst_tx_ctl->bit_80211_mac_head_type = 1;  /* 指示mac头部在skb中 */
    } else {
#ifdef _PRE_LWIP_ZERO_COPY_DEBUG
        OAM_ERROR_LOG0(0, 0, "[hmac_tx_encap] headroom not enough");
#endif

        /* 申请最大的80211头 */
        pst_hdr = (mac_ieee80211_qos_htc_frame_addr4_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_SHARED_DATA_PKT,
                                                                          MAC_80211_QOS_HTC_4ADDR_FRAME_LEN,
                                                                          OAL_FALSE);
        if (OAL_UNLIKELY(pst_hdr == OAL_PTR_NULL)) {
            OAM_ERROR_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{hmac_tx_encap::pst_hdr null.}");
            return OAL_ERR_CODE_PTR_NULL;
        }

        pst_tx_ctl->bit_80211_mac_head_type = 0;  /* 指示mac头部不在skb中，申请了额外内存存放的 */
    }

    /* 获取以太网头, 原地址，目的地址, 以太网类型 */
    pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);
    oal_set_mac_addr((oal_uint8 *)auc_daddr, pst_ether_hdr->auc_ether_dhost);
    oal_set_mac_addr((oal_uint8 *)auc_saddr, pst_ether_hdr->auc_ether_shost);

    /* 非amsdu帧 */
    if (pst_tx_ctl->en_is_amsdu == OAL_FALSE) {
        us_ether_type = pst_ether_hdr->us_ether_type;
    } else {
       /* 如果是AMSDU的第一个子帧，需要从snap头中获取以太网类型 */
       /* 如果是以太网帧，可以直接从以太网头中获取 */
        pst_snap_hdr  = (mac_llc_snap_stru *)((oal_uint8 *)pst_ether_hdr + ETHER_HDR_LEN);
        us_ether_type = pst_snap_hdr->us_ether_type;
    }

    /* 非组播帧，获取用户的QOS能力位信息 */
    if (pst_tx_ctl->en_ismcast == OAL_FALSE) {
        /* 根据用户结构体的cap_info，判断是否是QOS站点 */
        ul_qos                    = pst_user->st_user_base_info.st_cap_info.bit_qos;
        pst_tx_ctl->en_is_qosdata = pst_user->st_user_base_info.st_cap_info.bit_qos;
    }

    /* 设置帧控制 */
    hmac_tx_set_frame_ctrl(ul_qos, pst_tx_ctl, pst_hdr);

    /* 设置地址 */
    ul_ret = hmac_tx_set_addresses(pst_vap, pst_user, pst_tx_ctl, auc_saddr, sizeof(auc_saddr), auc_daddr,
        sizeof(auc_daddr), pst_hdr, us_ether_type);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        if (pst_tx_ctl->bit_80211_mac_head_type == 0) {
            OAL_MEM_FREE(pst_hdr, OAL_TRUE);
        }
        OAM_ERROR_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
            "{hmac_tx_encap::hmac_tx_set_addresses failed[%d].}", ul_ret);
        return ul_ret;
    }

    /* 对于LAN to WLAN的非AMSDU聚合帧，填充LLC SNAP头 */
    if (pst_tx_ctl->en_is_amsdu == OAL_FALSE) {
#ifdef _PRE_WLAN_CHIP_TEST
        /* al tx */
        if (pst_vap->st_vap_base_info.bit_al_tx_flag != OAL_SWITCH_ON) {
#endif
            mac_set_snap(pst_buf, us_ether_type, (ETHER_HDR_LEN - SNAP_LLC_FRAME_LEN));
#ifdef _PRE_WLAN_CHIP_TEST
        }
#endif
        /* 更新frame长度 */
        pst_tx_ctl->us_mpdu_len = (oal_uint16) oal_netbuf_get_len(pst_buf);

        /* 非amsdu聚合帧，记录mpdu字节数，不包括snap */
        pst_tx_ctl->us_mpdu_bytes = pst_tx_ctl->us_mpdu_len - SNAP_LLC_FRAME_LEN;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* mac头部在skb中时，netbuf的data指针指向mac头。 */
    /* 但是mac_set_snap函数中已经将data指针指向了llc头。因此这里要重新push到mac头。 */
    if (pst_tx_ctl->bit_80211_mac_head_type == 1) {
        oal_netbuf_push(pst_buf, MAC_80211_QOS_HTC_4ADDR_FRAME_LEN);
    }
#endif

    /* 挂接802.11头 */
    pst_tx_ctl->pst_frame_header = (mac_ieee80211_frame_stru *)pst_hdr;

    /* 分片处理 */
    if (hmac_tx_is_need_frag(pst_vap, pst_user, pst_buf, pst_tx_ctl) == OAL_TRUE) {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
        hmac_nonqos_frame_set_sn(pst_vap, pst_tx_ctl);
#endif
        ul_ret = hmac_frag_process_proc(pst_vap, pst_user, pst_buf, pst_tx_ctl);
    }

    return ul_ret;
}


OAL_STATIC oal_uint32 hmac_tx_lan_mpdu_process_sta(hmac_vap_stru *pst_vap, oal_netbuf_stru *pst_buf,
    mac_tx_ctl_stru *pst_tx_ctl)
{
    hmac_user_stru        *pst_user = OAL_PTR_NULL; /* 目标STA结构体 */
    mac_ether_header_stru *pst_ether_hdr = OAL_PTR_NULL; /* 以太网头 */
    oal_uint32            ul_ret;
    oal_uint16            us_user_idx;
    oal_uint8             *puc_ether_payload = OAL_PTR_NULL;

    pst_ether_hdr  = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);
    pst_tx_ctl->uc_tx_vap_index = pst_vap->st_vap_base_info.uc_vap_id;

    us_user_idx = pst_vap->st_vap_base_info.uc_assoc_vap_id;

    pst_user = (hmac_user_stru *)mac_res_get_hmac_user(us_user_idx);
    if (pst_user == OAL_PTR_NULL) {
        OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
        return HMAC_TX_DROP_USER_NULL;
    }

    if (oal_byteorder_host_to_net_uint16(ETHER_TYPE_ARP) == pst_ether_hdr->us_ether_type) {
        pst_ether_hdr++;
        puc_ether_payload = (oal_uint8 *)pst_ether_hdr;
        /* The source MAC address is modified only if the packet is an   */
        /* ARP Request or a Response. The appropriate bytes are checked. */
        /* Type field (2 bytes): ARP Request (1) or an ARP Response (2)  */
        if ((puc_ether_payload[6] == 0x00) &&
          (puc_ether_payload[7] == 0x02 || puc_ether_payload[7] == 0x01)) {
            /* Set Address2 field in the WLAN Header with source address */
            oal_set_mac_addr(puc_ether_payload + 8,
                pst_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);
        }
    }

    pst_tx_ctl->us_tx_user_idx = us_user_idx;

    ul_ret = hmac_tx_ucast_process(pst_vap, pst_buf, pst_user, pst_tx_ctl);
    if (OAL_UNLIKELY(ul_ret != HMAC_TX_PASS)) {
        return ul_ret;
    }

    /* 封装802.11头 */
    ul_ret = hmac_tx_encap(pst_vap, pst_user, pst_buf);
    if (OAL_UNLIKELY((ul_ret != OAL_SUCC))) {
        OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
            "{hmac_tx_lan_mpdu_process_sta::hmac_tx_encap failed[%d].}", ul_ret);
        OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
        return HMAC_TX_DROP_80211_ENCAP_FAIL;
    }

    OAL_MIPS_TX_STATISTIC(HMAC_PROFILING_FUNC_ENCAP_HEAD);
    return HMAC_TX_PASS;
}

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 hmac_tx_proxyarp_is_en(hmac_vap_stru *pst_vap)
{
    return (pst_vap->st_vap_base_info.pst_vap_proxyarp != OAL_PTR_NULL) &&
           (pst_vap->st_vap_base_info.pst_vap_proxyarp->en_is_proxyarp == OAL_TRUE);
}
#endif


OAL_STATIC OAL_INLINE oal_uint32  hmac_tx_lan_mpdu_process_ap(hmac_vap_stru *pst_vap, oal_netbuf_stru *pst_buf,
    mac_tx_ctl_stru *pst_tx_ctl)
{
    hmac_user_stru        *pst_user = OAL_PTR_NULL; /* 目标STA结构体 */
    mac_ether_header_stru *pst_ether_hdr = OAL_PTR_NULL; /* 以太网头 */
    oal_uint8             *puc_addr = OAL_PTR_NULL; /* 目的地址 */
    oal_uint32            ul_ret;
    oal_uint16            us_user_idx;

    /* 判断是组播或单播,对于lan to wlan的单播帧，返回以太网地址 */
    pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);
    puc_addr      = pst_ether_hdr->auc_ether_dhost;

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
    /* 确认proxy arp 是否使能 */
    if (hmac_tx_proxyarp_is_en(pst_vap) == OAL_TRUE) {
        if (hmac_proxy_arp_proc(pst_vap, pst_buf) == OAL_TRUE) {
            return HMAC_TX_DROP_PROXY_ARP;
        }
    }
#endif

    /* 单播数据帧 */
#ifdef _PRE_WLAN_CHIP_TEST
    if (OAL_LIKELY(!ETHER_IS_MULTICAST(puc_addr)) && pst_vap->st_vap_base_info.bit_al_tx_flag != OAL_SWITCH_ON)
#else
    if (OAL_LIKELY(!ETHER_IS_MULTICAST(puc_addr)))
#endif
    {
        ul_ret = mac_vap_find_user_by_macaddr(&(pst_vap->st_vap_base_info), puc_addr, &us_user_idx);
        if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
            OAM_WARNING_LOG4(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
                "{hmac_tx_lan_mpdu_process_ap::hmac_tx_find_user failed %2x:%2x:%2x:%2x}", puc_addr[2], puc_addr[3],
                puc_addr[4], puc_addr[5]);
            OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
            return HMAC_TX_DROP_USER_UNKNOWN;
        }

        /* 转成HMAC的USER结构体 */
        pst_user = (hmac_user_stru *)mac_res_get_hmac_user(us_user_idx);
        if (OAL_UNLIKELY(pst_user == OAL_PTR_NULL)) {
            OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
            return HMAC_TX_DROP_USER_NULL;
        }

        /* 用户状态判断 */
        if (OAL_UNLIKELY(pst_user->st_user_base_info.en_user_asoc_state != MAC_USER_STATE_ASSOC)) {
            OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
            return HMAC_TX_DROP_USER_INACTIVE;
        }

        /* 目标user指针 */
        pst_tx_ctl->us_tx_user_idx = us_user_idx;

        ul_ret = hmac_tx_ucast_process(pst_vap, pst_buf, pst_user, pst_tx_ctl);
        if (OAL_UNLIKELY(ul_ret != HMAC_TX_PASS)) {
            return ul_ret;
        }
    } else { /* 组播 or 广播 */
        /* 设置组播标识位 */
        pst_tx_ctl->en_ismcast = OAL_TRUE;

        /* 更新ACK策略 */
        pst_tx_ctl->en_ack_policy = WLAN_TX_NO_ACK;

        /* 获取组播用户 */
        pst_user = mac_res_get_hmac_user(pst_vap->st_vap_base_info.us_multi_user_idx);
        if (OAL_UNLIKELY(pst_user == OAL_PTR_NULL)) {
            OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
                "{hmac_tx_lan_mpdu_process_ap::get multi user failed[%d].}",
                pst_vap->st_vap_base_info.us_multi_user_idx);
            OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
            return HMAC_TX_DROP_MUSER_NULL;
        }

        pst_tx_ctl->us_tx_user_idx = pst_vap->st_vap_base_info.us_multi_user_idx;
        pst_tx_ctl->uc_tid  = WLAN_TIDNO_BCAST;

        pst_tx_ctl->uc_ac   = WLAN_WME_TID_TO_AC(pst_tx_ctl->uc_tid);

#ifdef _PRE_WLAN_FEATURE_MCAST /* 组播转单播 */
            if ((ETHER_IS_IPV4_MULTICAST(puc_addr)) && (!ETHER_IS_BROADCAST(puc_addr)) &&
                (pst_vap->pst_m2u != OAL_PTR_NULL)) {
                ul_ret = hmac_m2u_snoop_convert(pst_vap, pst_buf);
                if (ul_ret != HMAC_TX_PASS) {
                    return ul_ret;
                }
            }
#endif
    }

    /* 封装802.11头 */
    ul_ret = hmac_tx_encap(pst_vap, pst_user, pst_buf);
    if (OAL_UNLIKELY((ul_ret != OAL_SUCC))) {
        OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
            "{hmac_tx_lan_mpdu_process_ap::hmac_tx_encap failed[%d].}", ul_ret);
        OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
        return HMAC_TX_DROP_80211_ENCAP_FAIL;
    }

    return HMAC_TX_PASS;
}

oal_uint32 hmac_tx_lan_to_wlan_no_tcp_opt(mac_vap_stru *pst_vap, oal_netbuf_stru *pst_buf)
{
    frw_event_stru        *pst_event          = OAL_PTR_NULL;       /* 事件结构体 */
    frw_event_mem_stru    *pst_event_mem      = OAL_PTR_NULL;
    hmac_vap_stru         *pst_hmac_vap       = OAL_PTR_NULL;       /* VAP结构体 */
    mac_tx_ctl_stru       *pst_tx_ctl         = OAL_PTR_NULL;       /* SKB CB */
    oal_uint32             ul_ret = HMAC_TX_PASS;
    dmac_tx_event_stru    *pst_dtx_stru       = OAL_PTR_NULL;
    oal_uint8              uc_data_type;
#ifdef _PRE_WLAN_FEATURE_WAPI
    hmac_user_stru              *pst_user     = OAL_PTR_NULL;
    hmac_wapi_stru              *pst_wapi     = OAL_PTR_NULL;
    mac_ieee80211_frame_stru    *pst_mac_hdr  = OAL_PTR_NULL;
#endif

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_vap->uc_vap_id);
    if (OAL_UNLIKELY(pst_hmac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_TX, "{hmac_tx_lan_to_wlan::pst_hmac_vap null.}");
        OAM_STAT_VAP_INCR(pst_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* VAP模式判断 */
    if (OAL_UNLIKELY(pst_vap->en_vap_mode != WLAN_VAP_MODE_BSS_AP && pst_vap->en_vap_mode != WLAN_VAP_MODE_BSS_STA)) {
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_TX, "{hmac_tx_lan_to_wlan::en_vap_mode=%d.}", pst_vap->en_vap_mode);
        OAM_STAT_VAP_INCR(pst_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    /* 如果关联用户数量为0，则丢弃报文 */
    if (OAL_UNLIKELY(pst_hmac_vap->st_vap_base_info.us_user_nums == 0)) {
        OAM_STAT_VAP_INCR(pst_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);
        return OAL_FAIL;
    }
#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
    /* 发数据，只发一次，避免反复申请tx描述符地址 */
    if (pst_vap->bit_al_tx_flag == OAL_SWITCH_ON) {
        if (pst_vap->bit_first_run != OAL_FALSE) {
            return OAL_SUCC;
        }
        mac_vap_set_al_tx_first_run(pst_vap, OAL_TRUE);
    }
#endif

    /* 初始化CB tx rx字段 , CB字段在前面已经被清零， 在这里不需要重复对某些字段赋零值 */
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_buf);
    pst_tx_ctl->uc_mpdu_num      = 1;
    pst_tx_ctl->uc_netbuf_num    = 1;
    pst_tx_ctl->en_frame_type    = WLAN_DATA_BASICTYPE;
    pst_tx_ctl->en_is_probe_data = DMAC_USER_ALG_NON_PROBE;
    pst_tx_ctl->en_ack_policy    = WLAN_TX_NORMAL_ACK;
#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
    pst_tx_ctl->en_ack_policy    = pst_hmac_vap->bit_ack_policy;
#endif
    pst_tx_ctl->uc_tx_vap_index  = pst_vap->uc_vap_id;
    pst_tx_ctl->us_tx_user_idx   = 0xffff;
    pst_tx_ctl->uc_ac            = WLAN_WME_AC_BE;                  /* 初始化入BE队列 */

    /* 由于LAN TO WLAN和WLAN TO WLAN的netbuf都走这个函数，为了区分，需要先判断
     * 到底是哪里来的netbuf然后再对CB的事件类型字段赋值
     */
    if (pst_tx_ctl->en_event_type != FRW_EVENT_TYPE_WLAN_DTX) {
        pst_tx_ctl->en_event_type             = FRW_EVENT_TYPE_HOST_DRX;
        pst_tx_ctl->uc_event_sub_type         = DMAC_TX_HOST_DRX;
    }

    /* 此处数据可能从内核而来，也有可能由dev报上来再通过空口转出去，注意一下 */
    uc_data_type =  mac_get_data_type_from_8023((oal_uint8 *)oal_netbuf_data(pst_buf), MAC_NETBUFF_PAYLOAD_ETH);
    /* 维测，输出一个关键帧打印 */
    if ((uc_data_type == MAC_DATA_DHCP) ||
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_HMAC == _PRE_MULTI_CORE_MODE)
    (uc_data_type == MAC_DATA_ARP_REQ) ||
    (uc_data_type == MAC_DATA_ARP_RSP) ||
#endif
    (uc_data_type == MAC_DATA_EAPOL)) {
        pst_tx_ctl->bit_is_vipframe  = OAL_TRUE;
        OAM_WARNING_LOG2(pst_vap->uc_vap_id, OAM_SF_WPA,
            "{hmac_tx_lan_to_wlan_no_tcp_opt::send datatype==%u, len==%u}[0:dhcp 1:arp_req 2:arp_rsp 3:eapol]",
            uc_data_type, OAL_NETBUF_LEN(pst_buf));
    }

    if (pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        /*  处理当前 MPDU */
        if (pst_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11QosOptionImplemented == OAL_FALSE) {
            pst_tx_ctl->uc_ac = WLAN_WME_AC_VO;            /* AP模式 关WMM 入VO队列 */
            pst_tx_ctl->uc_tid =  WLAN_WME_AC_TO_TID(pst_tx_ctl->uc_ac);
        }

        ul_ret = hmac_tx_lan_mpdu_process_ap(pst_hmac_vap, pst_buf, pst_tx_ctl);
    } else if (pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        /* 处理当前MPDU */
        pst_tx_ctl->uc_ac = WLAN_WME_AC_VO;                  /* STA模式 非qos帧入VO队列 */
        pst_tx_ctl->uc_tid =  WLAN_WME_AC_TO_TID(pst_tx_ctl->uc_ac);

        OAL_MIPS_TX_STATISTIC(HMAC_PROFILING_FUNC_CB_INIT);
        ul_ret = hmac_tx_lan_mpdu_process_sta(pst_hmac_vap, pst_buf, pst_tx_ctl);
#ifdef _PRE_WLAN_FEATURE_WAPI
        if (ul_ret == HMAC_TX_PASS) {
            pst_user = (hmac_user_stru *)mac_res_get_hmac_user(pst_vap->uc_assoc_vap_id);
            if (pst_user == OAL_PTR_NULL) {
                OAM_STAT_VAP_INCR(pst_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);
                OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_tx_lan_to_wlan_no_tcp_opt::usrid==%u no usr!}",
                    pst_vap->uc_assoc_vap_id);
                return HMAC_TX_DROP_USER_NULL;
            }

            /* 获取wapi对象 组播/单播 */
            pst_mac_hdr = ((mac_tx_ctl_stru *)OAL_NETBUF_CB(pst_buf))->pst_frame_header;
/*lint -e730*/
            pst_wapi = hmac_user_get_wapi_ptr(pst_vap, !ETHER_IS_MULTICAST(pst_mac_hdr->auc_address1),
                pst_user->st_user_base_info.us_assoc_id);
/*lint +e730*/
            if (pst_wapi != OAL_PTR_NULL && (WAPI_IS_PORT_VALID(pst_wapi) == OAL_TRUE) &&
                (pst_wapi->wapi_netbuff_txhandle != OAL_PTR_NULL)) {
                pst_buf = pst_wapi->wapi_netbuff_txhandle(pst_wapi, pst_buf);
                /* 由于wapi可能修改netbuff，此处需要重新获取一下cb */
                pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_buf);
            }
        }

#endif /* #ifdef _PRE_WLAN_FEATURE_WAPI */
    }

    if (OAL_LIKELY(ul_ret == HMAC_TX_PASS)) {
        /* 抛事件，传给DMAC */
        pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_tx_event_stru));
        if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
            OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_TX, "{hmac_tx_lan_to_wlan::FRW_EVENT_ALLOC failed.}");
            return OAL_ERR_CODE_ALLOC_MEM_FAIL;
        }

        pst_event = (frw_event_stru *)pst_event_mem->puc_data;

        /* 填写事件头 */
        FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                           FRW_EVENT_TYPE_HOST_DRX,
                           DMAC_TX_HOST_DRX,
                           OAL_SIZEOF(dmac_tx_event_stru),
                           FRW_EVENT_PIPELINE_STAGE_1,
                           pst_vap->uc_chip_id,
                           pst_vap->uc_device_id,
                           pst_vap->uc_vap_id);

        pst_dtx_stru               = (dmac_tx_event_stru *)pst_event->auc_event_data;
        pst_dtx_stru->pst_netbuf   = pst_buf;
        pst_dtx_stru->us_frame_len = pst_tx_ctl->us_mpdu_len;

        /* 调度事件 */
        ul_ret = frw_event_dispatch_event(pst_event_mem);
        if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
            OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_TX,
                "{hmac_tx_lan_to_wlan::frw_event_dispatch_event failed[%d].}", ul_ret);
            OAM_STAT_VAP_INCR(pst_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);
        }

         /* 释放事件 */
        FRW_EVENT_FREE(pst_event_mem);

        OAL_MIPS_TX_STATISTIC(HMAC_PROFILING_FUNC_TX_EVENT_TO_DMAC);
#ifdef _PRE_WLAN_PROFLING_MIPS
        oal_profiling_stop_tx_save();
#endif
    } else if (OAL_UNLIKELY(ul_ret == HMAC_TX_BUFF)) {
        ul_ret = OAL_SUCC;
    } else if (ul_ret == HMAC_TX_DONE) {
        ul_ret = OAL_SUCC;
    } else if (ul_ret == HMAC_TX_DROP_MTOU_FAIL) {
        /* 组播报文没有对应的STA可以转成单播，所以丢弃，属正常行为 */
        OAM_INFO_LOG1(pst_vap->uc_vap_id, OAM_SF_TX, "{hmac_tx_lan_to_wlan_no_tcp_opt::HMAC_TX_DROP.reason[%d]!}",
            ul_ret);
    } else {
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_TX, "{hmac_tx_lan_to_wlan_no_tcp_opt::HMAC_TX_DROP.reason[%d]!}",
            ul_ret);
    }

    return ul_ret;
}

#ifdef _PRE_WLAN_TCP_OPT
OAL_STATIC  oal_uint32 hmac_transfer_tx_handler(hmac_device_stru *hmac_device, hmac_vap_stru *hmac_vap,
    oal_netbuf_stru *netbuf)
{
    oal_uint32 ul_ret;
#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
    if (oal_netbuf_select_queue(netbuf) == WLAN_TCP_ACK_QUEUE) {
#ifdef _PRE_WLAN_TCP_OPT_DEBUG
        OAM_WARNING_LOG0(0, OAM_SF_TX, "{hmac_transfer_tx_handler::netbuf is tcp ack.}\r\n");
#endif
        oal_spin_lock_bh(&hmac_vap->st_hamc_tcp_ack[HCC_TX].data_queue_lock[HMAC_TCP_ACK_QUEUE]);
        oal_netbuf_list_tail(&hmac_vap->st_hamc_tcp_ack[HCC_TX].data_queue[HMAC_TCP_ACK_QUEUE], netbuf);
        ul_ret = OAL_SUCC;
        /* 单纯TCP ACK等待调度, 特殊报文马上发送 */
        if (hmac_judge_tx_netbuf_is_tcp_ack((oal_ether_header_stru *)oal_netbuf_data(netbuf))) {
            hmac_device->en_need_notify = OAL_TRUE;
            oal_spin_unlock_bh(&hmac_vap->st_hamc_tcp_ack[HCC_TX].data_queue_lock[HMAC_TCP_ACK_QUEUE]);
            hmac_sched_transfer();
        } else {
            oal_spin_unlock_bh(&hmac_vap->st_hamc_tcp_ack[HCC_TX].data_queue_lock[HMAC_TCP_ACK_QUEUE]);
            hmac_tcp_ack_process();
        }
    } else {
        ul_ret = hmac_tx_lan_to_wlan_no_tcp_opt(&(hmac_vap->st_vap_base_info), netbuf);
    }
#endif
    return ul_ret;
}
#endif


oal_uint32 hmac_tx_wlan_to_wlan_ap(oal_mem_stru *pst_event_mem)
{
    frw_event_stru  *pst_event    = OAL_PTR_NULL;      /* 事件结构体 */
    mac_vap_stru    *pst_mac_vap  = OAL_PTR_NULL;
    oal_netbuf_stru *pst_buf      = OAL_PTR_NULL;      /* 从netbuf链上取下来的指向netbuf的指针 */
    oal_netbuf_stru *pst_buf_tmp  = OAL_PTR_NULL;      /* 暂存netbuf指针，用于while循环 */
    mac_tx_ctl_stru *pst_tx_ctl   = OAL_PTR_NULL;
    oal_uint32      ul_ret;

    /* 入参判断 */
    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_TX, "{hmac_tx_wlan_to_wlan_ap::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* 获取事件 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    if (OAL_UNLIKELY(pst_event == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_TX, "{hmac_tx_wlan_to_wlan_ap::pst_event null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取PAYLOAD中的netbuf链 */
    pst_buf = (oal_netbuf_stru *)(*((uintptr_t*)(pst_event->auc_event_data)));

    ul_ret = hmac_tx_get_mac_vap(pst_event->st_event_hdr.uc_vap_id, &pst_mac_vap);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(pst_event->st_event_hdr.uc_vap_id, OAM_SF_TX,
            "{hmac_tx_wlan_to_wlan_ap::hmac_tx_get_mac_vap failed[%d].}", ul_ret);
        hmac_free_netbuf_list(pst_buf);
        return ul_ret;
    }

    /* 循环处理每一个netbuf，按照以太网帧的方式处理 */
    while (pst_buf != OAL_PTR_NULL) {
        pst_buf_tmp = OAL_NETBUF_NEXT(pst_buf);

        OAL_NETBUF_NEXT(pst_buf) = OAL_PTR_NULL;
        OAL_NETBUF_PREV(pst_buf) = OAL_PTR_NULL;

        
        pst_tx_ctl = (mac_tx_ctl_stru *)OAL_NETBUF_CB(pst_buf);
        memset_s(pst_tx_ctl, sizeof(mac_tx_ctl_stru), 0, sizeof(mac_tx_ctl_stru));

        pst_tx_ctl->en_event_type = FRW_EVENT_TYPE_WLAN_DTX;
        pst_tx_ctl->uc_event_sub_type = DMAC_TX_WLAN_DTX;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        /* set the queue map id when wlan to wlan */
        oal_skb_set_queue_mapping(pst_buf, WLAN_NORMAL_QUEUE);
#endif

        ul_ret = hmac_tx_lan_to_wlan(pst_mac_vap, pst_buf);
        /* 调用失败，自己调用自己释放netbuff内存 */
        if (ul_ret != OAL_SUCC) {
            hmac_free_netbuf_list(pst_buf);
        }

        pst_buf = pst_buf_tmp;
    }

    return OAL_SUCC;
}
#ifdef _PRE_WLAN_FEATURE_BTCOEX

oal_uint32 hmac_delba_send_timeout(oal_void *p_arg)
{
    hmac_vap_stru *pst_hmac_vap;
    mac_vap_stru  *pst_vap;
    oal_uint32 ui_val;

    pst_hmac_vap = (hmac_vap_stru *)p_arg;
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{hmac_delba_send_timeout::p_arg is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_vap = &(pst_hmac_vap->st_vap_base_info);
    ui_val = oal_atomic_read(&pst_hmac_vap->ul_rx_unicast_pkt_to_lan);
    if (ui_val < 2) {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_COEX,
            "{hmac_delba_send_timeout:delba send rx_pkt_below.this rx_pkt:%d}", ui_val);
        hmac_btcoex_delba_foreach_user(pst_vap);
    }

    oal_atomic_set(&pst_hmac_vap->ul_rx_unicast_pkt_to_lan, 0);

    return OAL_SUCC;
}


oal_void hmac_btcoex_arp_fail_delba(oal_netbuf_stru *pst_netbuf, hmac_vap_stru *pst_hmac_vap)
{
    oal_uint8 uc_data_type;

    /* ************************************************************************* */
    /*                      ARP Frame Format                                   */
    /* ----------------------------------------------------------------------- */
    /* |以太网目的地址|以太网源地址|帧类型|硬件类型|协议类型|硬件地址长度|     */
    /* ----------------------------------------------------------------------- */
    /* | 6            |6(待替换)   |2     |2       |2       |1           |     */
    /* ----------------------------------------------------------------------- */
    /* |协议地址长度|op|发送端以太网地址|发送端IP地址|目的以太网地址|目的IP地址 */
    /* ----------------------------------------------------------------------- */
    /* | 1          |2 |6(待替换)       |4           |6             |4         */
    /* ----------------------------------------------------------------------- */
    /*                                                                          */
    /* ************************************************************************* */
    /* 参数外面已经做检查，里面没必要再做检查了 */
    uc_data_type =  mac_get_data_type_from_8023((oal_uint8 *)oal_netbuf_data(pst_netbuf), MAC_NETBUFF_PAYLOAD_ETH);
    /* 发送方向创建定时器 */
    if ((uc_data_type == MAC_DATA_ARP_REQ) && (pst_hmac_vap->st_delba_opt_timer.en_is_registerd == OAL_FALSE)) {
        /* 每次重启定时器之前清零,保证统计的时间 */
        oal_atomic_set(&pst_hmac_vap->ul_rx_unicast_pkt_to_lan, 0);

        FRW_TIMER_CREATE_TIMER(&(pst_hmac_vap->st_delba_opt_timer),
                               hmac_delba_send_timeout,
                               5000,
                               pst_hmac_vap,
                               OAL_FALSE,
                               OAM_MODULE_ID_HMAC,
                               pst_hmac_vap->st_vap_base_info.ul_core_id);
    }
}
#endif
#ifdef _PRE_WLAN_FEATURE_PROXYSTA


OAL_STATIC oal_uint16 hmac_tx_proxysta_checksum(oal_uint16 us_protocol,
                                                oal_uint16 us_len,
                                                oal_uint8 auc_src_addr[],
                                                oal_uint8 auc_dest_addr[],
                                                oal_uint16 us_addrleninbytes,
                                                oal_uint8 *puc_buff)
{
    oal_uint16 us_pad = 0;
    oal_uint16 us_word16;
    oal_uint32 ul_sum = 0;
    oal_long   l_loop;

    if (us_len & 1) {
        us_len -= 1;
        us_pad  = 1;
    }

    for (l_loop = 0; l_loop < us_len; l_loop = l_loop + 2) {
        us_word16 = puc_buff[l_loop];
        us_word16 = (oal_uint16)((us_word16 << 8) + puc_buff[l_loop + 1]);
        ul_sum = ul_sum + (oal_uint32)us_word16;
    }

    if (us_pad) {
        us_word16 = puc_buff[us_len];
        us_word16 <<= 8;
        ul_sum = ul_sum + (oal_uint32)us_word16;
    }

    for (l_loop = 0; l_loop < us_addrleninbytes; l_loop = l_loop + 2) {
        us_word16 = auc_src_addr[l_loop];
        us_word16 = (oal_uint16)((us_word16 << 8) + auc_src_addr[l_loop + 1]);
        ul_sum = ul_sum + (oal_uint32)us_word16;
    }

    for (l_loop = 0; l_loop < us_addrleninbytes; l_loop = l_loop + 2) {
        us_word16 = auc_dest_addr[l_loop];
        us_word16 = (oal_uint16)((us_word16 << 8) + auc_dest_addr[l_loop + 1]);
        ul_sum = ul_sum + (oal_uint32)us_word16;
    }

    ul_sum = ul_sum + (oal_uint32)us_protocol + (oal_uint32)(us_len + us_pad);

    while (ul_sum >> 16) {
        ul_sum = (ul_sum & 0xFFFF) + (ul_sum >> 16);
    }

    ul_sum = ~ul_sum;

    return ((oal_uint16) ul_sum);
}


OAL_STATIC oal_uint32 hmac_tx_proxysta_arp_mat(mac_vap_stru *pst_mac_vap,
                                               oal_uint8 *puc_eth_body,
                                               oal_uint32 ul_pkt_len,
                                               oal_uint32 ul_contig_len)
{
    oal_eth_arphdr_stru *pst_arp      = OAL_PTR_NULL;
    oal_uint8           *puc_arp_smac = OAL_PTR_NULL;

    /* ************************************************************************* */
    /*                      ARP Frame Format                                   */
    /* ----------------------------------------------------------------------- */
    /* |以太网目的地址|以太网源地址|帧类型|硬件类型|协议类型|硬件地址长度|     */
    /* ----------------------------------------------------------------------- */
    /* | 6            |6(待替换)   |2     |2       |2       |1           |     */
    /* ----------------------------------------------------------------------- */
    /* |协议地址长度|op|发送端以太网地址|发送端IP地址|目的以太网地址|目的IP地址 */
    /* ----------------------------------------------------------------------- */
    /* | 1          |2 |6(待替换)       |4           |6             |4         */
    /* ----------------------------------------------------------------------- */
    /*                                                                          */
    /* ************************************************************************* */
    /* 参数合法性检查 */
    if ((pst_mac_vap == OAL_PTR_NULL) || (puc_eth_body == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_PROXYSTA,
            "{hmac_tx_proxysta_arp_mat::The input parameter of hmac_tx_proxysta_arp_mat is OAL_PTR_NULL.}");
        return OAL_FAIL;
    }

    pst_arp = (oal_eth_arphdr_stru *)puc_eth_body;
    ul_contig_len += (oal_int32)OAL_SIZEOF(oal_eth_arphdr_stru);
    if (ul_pkt_len < ul_contig_len) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_tx_proxysta_arp_mat::The length of buf is less than the sum of mac_ether_header_stru and \
            oal_eth_arphdr_stru.}");
        return OAL_FAIL;
    }

    /*lint -e778*/
    if ((pst_arp->uc_ar_hln == ETHER_ADDR_LEN) && (OAL_HOST2NET_SHORT(ETHER_TYPE_IP) == pst_arp->us_ar_pro)) {
        puc_arp_smac = pst_arp->auc_ar_sha;
    /*lint +e778*/
    } else {
        pst_arp = OAL_PTR_NULL;
    }

    if (pst_arp != OAL_PTR_NULL) {
        switch (OAL_NET2HOST_SHORT(pst_arp->us_ar_op)) {
            case OAL_ARPOP_REQUEST:
            case OAL_ARPOP_REPLY:

            // 替换mac地址为out vap的mac地址
            oal_set_mac_addr(puc_arp_smac,  mac_mib_get_StationID(pst_mac_vap));
            break;
            default:
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                             "{hmac_tx_proxysta_arp_mat::do not replace arp addr.}");
            break;
        }
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_tx_proxysta_ip_mat(mac_vap_stru           *pst_mac_vap,
                                              mac_ether_header_stru  *pst_ether_header,
                                              oal_uint8              *puc_eth_body,
                                              oal_uint32             ul_pkt_len,
                                              oal_uint32             ul_contig_len)
{
    oal_ip_header_stru   *pst_ip_header   = OAL_PTR_NULL;
    oal_udp_header_stru  *pst_udp_header  = OAL_PTR_NULL;
    oal_dhcp_packet_stru *pst_dhcp_packet = OAL_PTR_NULL;
    oal_uint8            *puc_scr_mac     = OAL_PTR_NULL;
    oal_uint16            us_ip_header_len;

    /* *********************************************************************** */
    /*                      DHCP Frame Format                                */
    /* --------------------------------------------------------------------- */
    /* |以太网头        |   IP头         | UDP头           |DHCP报文       | */
    /* --------------------------------------------------------------------- */
    /* | 14             |20              |8                | 不定          | */
    /* --------------------------------------------------------------------- */
    /*                                                                        */
    /* *********************************************************************** */
    /* 参数合法性检查 */
    if ((pst_mac_vap == OAL_PTR_NULL) || (pst_ether_header == OAL_PTR_NULL) || (puc_eth_body == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_PROXYSTA,
                       "{hmac_tx_proxysta_ip_mat::The input parameter of hmac_tx_proxysta_dhcp_mat is OAL_PTR_NULL.}");
        return OAL_FAIL;
    }

    pst_ip_header =  (oal_ip_header_stru *)puc_eth_body;

    /* *********************************************************************** */
    /*                    IP头格式 (oal_ip_header_stru)                      */
    /* --------------------------------------------------------------------- */
    /* | 版本 | 报头长度 | 服务类型 | 总长度  |标识  |标志  |段偏移量    |   */
    /* --------------------------------------------------------------------- */
    /* | 4bits|  4bits   | 1        | 2       | 2    |3bits | 13bits  |      */
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* | 生存期 | 协议        | 头部校验和| 源地址（SrcIp）|目的地址（DstIp） */
    /* --------------------------------------------------------------------- */
    /* | 1      |  1 (17为UDP)| 2         | 4              | 4               */
    /* --------------------------------------------------------------------- */
    /* *********************************************************************** */
    ul_contig_len += OAL_SIZEOF(oal_ip_header_stru);
    if (ul_pkt_len < ul_contig_len) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_tx_proxysta_ip_mat::The length of buf is less than the sum of mac_ether_header_stru \
            and oal_ip_header_stru.}");
        return OAL_FAIL;
    }

    us_ip_header_len = pst_ip_header->us_ihl * 4;

    /* 如果是UDP */
    if (pst_ip_header->uc_protocol == OAL_IPPROTO_UDP) {
        pst_udp_header = (oal_udp_header_stru *)((oal_uint8 *)pst_ip_header + us_ip_header_len);

        ul_contig_len += OAL_SIZEOF(oal_udp_header_stru);
        if (ul_pkt_len < ul_contig_len) {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                           "{hmac_tx_proxysta_ip_mat::The length of buf is invalid.}");
            return OAL_FAIL;
        }

        /* *********************************************************************** */
        /*                      UDP 头 (oal_udp_header_stru)                     */
        /* --------------------------------------------------------------------- */
        /* |源端口号（SrcPort）|目的端口号（DstPort）| UDP长度    | UDP检验和  | */
        /* --------------------------------------------------------------------- */
        /* | 2                 | 2                   |2           | 2          | */
        /* --------------------------------------------------------------------- */
        /*                                                                        */
        /* *********************************************************************** */
        /* DHCP request UDP Client SP = 68 (bootpc), DP = 67 (bootps) */
        /*lint -e778*/
        /*lint -e572*/
        if (OAL_HOST2NET_SHORT(67) == pst_udp_header->dest) {
        /*lint +e778*/
        /*lint +e572*/
            /* *********************************************************************** */
            /*                    DHCP 报文格式 (oal_dhcp_packet_stru)               */
            /* --------------------------------------------------------------------- */
            /* | op | htpe | hlen | hops  |xid(事务IP)  |secs(秒数)   |flags(标志)|  */
            /* --------------------------------------------------------------------- */
            /* | 1  | 1    | 1    | 1     | 4           | 2           | 2          | */
            /* --------------------------------------------------------------------- */
            /* --------------------------------------------------------------------- */
            /* | ciaddr(4)客户ip地址 | yiaddr（4）你的IP地址 |siaddr(4)服务器IP地址| */
            /* --------------------------------------------------------------------- */
            /* | giaddr（4）中继代理IP地址      | chaddr（16）客户机硬件地址(待替换)| */
            /* --------------------------------------------------------------------- */
            /* |sname（64）服务器的主机名|file（128）启动文件名|option（不定长）选项| */
            /* --------------------------------------------------------------------- */
            /* *********************************************************************** */
            pst_dhcp_packet = (oal_dhcp_packet_stru *)(((oal_uint8 *)pst_udp_header) + OAL_SIZEOF(oal_udp_header_stru));

            ul_contig_len += OAL_SIZEOF(oal_dhcp_packet_stru);
            if (ul_pkt_len < ul_contig_len) {
                OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                    "{hmac_tx_proxysta_ip_mat::The length of buf is invalid.}");
                return OAL_FAIL;
            }

            puc_scr_mac = pst_ether_header->auc_ether_shost;

            if (oal_compare_mac_addr(pst_dhcp_packet->chaddr, pst_ether_header->auc_ether_shost) == OAL_FALSE) {
                /* 把UDP包中的地址替换为自己的地址 */
                oal_set_mac_addr(pst_dhcp_packet->chaddr, mac_mib_get_StationID(pst_mac_vap));

                /* 由于包内容已经被改写，所以要重新写UDP 帧的checksum */
                pst_udp_header->check = hmac_rx_proxysta_update_checksum_addr(pst_udp_header->check,
                                                                              puc_scr_mac,
                                                                              mac_mib_get_StationID(pst_mac_vap));
            }
        }
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_tx_proxysta_icmpv6_mat(mac_vap_stru *pst_mac_vap,
                                                  oal_uint8 *puc_eth_body,
                                                  oal_uint32 ul_pkt_len,
                                                  oal_uint32 ul_contig_len)
{
    oal_ipv6hdr_stru          *pst_ipv6hdr = OAL_PTR_NULL;
    oal_icmp6hdr_stru         *pst_icmp6hdr = OAL_PTR_NULL;
    oal_eth_icmp6_lladdr_stru *pst_eth_icmp6_lladdr = OAL_PTR_NULL;
    oal_uint16                 us_icmp6len;
    oal_bool_enum_uint8        en_packet_changed = OAL_TRUE;
    oal_uint16                 us_check_sum;

    /* 参数合法性检查 */
    if ((pst_mac_vap == OAL_PTR_NULL) || (puc_eth_body == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_PROXYSTA,
            "{hmac_tx_proxysta_icmpv6_mat::The input parameter of hmac_tx_proxysta_icmpv6_mat is OAL_PTR_NULL.}");
        return OAL_FAIL;
    }

    pst_ipv6hdr = (oal_ipv6hdr_stru *)puc_eth_body;

    ul_contig_len += OAL_SIZEOF(oal_ipv6hdr_stru);
    if (ul_pkt_len < ul_contig_len) {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_tx_proxysta_icmpv6_mat::The length of buf is less than the sum of mac_ether_header_stru \
            and oal_ipv6hdr_stru.  ul_pkt_len[%d], ul_contig_len[%d]}", ul_pkt_len, ul_contig_len);
        return OAL_FAIL;
    }

    if (pst_ipv6hdr->nexthdr == OAL_IPPROTO_ICMPV6) {
        pst_icmp6hdr = (oal_icmp6hdr_stru *)(pst_ipv6hdr + 1);
        us_icmp6len  = pst_ipv6hdr->payload_len;

        ul_contig_len += OAL_SIZEOF(oal_icmp6hdr_stru);
        if (ul_pkt_len < ul_contig_len) {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                           "{hmac_tx_proxysta_icmpv6_mat::The length of buf is invalid.}");
            return OAL_FAIL;
        }

        /*
         * It seems that we only have to modify IPv6 packets being
         * sent by a Proxy STA. Both the solicitation and advertisement
         * packets have the STA's OMA. Flip that to the VMA.
         */
        switch (pst_icmp6hdr->icmp6_type) {
            case OAL_NDISC_NEIGHBOUR_SOLICITATION:
            case OAL_NDISC_NEIGHBOUR_ADVERTISEMENT:
            {
                ul_contig_len += OAL_IPV6_MAC_ADDR_LEN;
                /* 不存在IcmpV6 option,源地址不存在，不需要转换 */
                if (ul_pkt_len == ul_contig_len) {
                    OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                                     "{hmac_tx_proxysta_icmpv6_mat::No source addr.}");
                    return OAL_SUCC;
                }

                /* 存在源地址，进行转换 */
                ul_contig_len += OAL_SIZEOF(oal_eth_icmp6_lladdr_stru);
                if (ul_pkt_len < ul_contig_len) {
                    OAM_ERROR_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                        "{hmac_tx_proxysta_icmpv6_mat ABNORMAL::The length of buf is invalid ul_pkt_len[%d], \
                        ul_contig_len[%d] icmp6_type[%d].}", ul_pkt_len, ul_contig_len, pst_icmp6hdr->icmp6_type);
                    return OAL_FAIL;
                }

                pst_eth_icmp6_lladdr = (oal_eth_icmp6_lladdr_stru *)((oal_uint8 *)(pst_icmp6hdr + 1) + 16);

                /* 替换地址 */
                oal_set_mac_addr(pst_eth_icmp6_lladdr->uc_addr, mac_mib_get_StationID(pst_mac_vap));

                us_check_sum = hmac_tx_proxysta_checksum((oal_uint16)OAL_IPPROTO_ICMPV6, us_icmp6len,
                    pst_ipv6hdr->saddr.s6_addr, pst_ipv6hdr->daddr.s6_addr, 16, (oal_uint8 *)pst_icmp6hdr);

                pst_icmp6hdr->icmp6_cksum = OAL_HOST2NET_SHORT(us_check_sum);
                break;
            }
            case OAL_NDISC_ROUTER_SOLICITATION:
            {
                ul_contig_len += OAL_SIZEOF(oal_eth_icmp6_lladdr_stru);
                if (ul_pkt_len < ul_contig_len) {
                    OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                                   "{hmac_tx_proxysta_icmpv6_mat::The length of buf is invalid.}");
                    return OAL_FAIL;
                }

                /* replace the HW address with the VMA */
                pst_eth_icmp6_lladdr = (oal_eth_icmp6_lladdr_stru *)((oal_uint8 *)(pst_icmp6hdr + 1));
                break;
            }
            default:
                en_packet_changed = OAL_FALSE;
                break;
        }

        if ((en_packet_changed == OAL_TRUE) && (pst_eth_icmp6_lladdr != OAL_PTR_NULL)) {
            oal_set_mac_addr(pst_eth_icmp6_lladdr->uc_addr, pst_mac_vap->auc_bssid);
            us_check_sum = hmac_tx_proxysta_checksum((oal_uint16)OAL_IPPROTO_ICMPV6, us_icmp6len,
                pst_ipv6hdr->saddr.s6_addr, pst_ipv6hdr->daddr.s6_addr, 16, (oal_uint8 *)pst_icmp6hdr);

            pst_icmp6hdr->icmp6_cksum = OAL_HOST2NET_SHORT(us_check_sum);
        }
    }

    return OAL_SUCC;
}


oal_uint32 hmac_tx_proxysta_mat(oal_netbuf_stru *pst_buf, mac_vap_stru *pst_mac_vap)
{
    mac_ether_header_stru *pst_ether_header;
    oal_uint8             *puc_eth_body;
    oal_uint16            us_ether_type;
    oal_uint8             *puc_scr_mac;
    oal_uint32            ul_contig_len = OAL_SIZEOF(mac_ether_header_stru);
    oal_uint32            ul_pkt_len;
    oal_uint32            ul_ret;

    /* 参数合法性检查 */
    if (OAL_UNLIKELY((pst_buf == OAL_PTR_NULL) || (pst_mac_vap == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_PROXYSTA,
                       "{hmac_tx_proxysta_mat::The input parameter of hmac_tx_proxysta_arp_mat is OAL_PTR_NULL.}");
        return OAL_FAIL;
    }

    /* 参数合法性检查后，获取pst_buf的长度 */
    ul_pkt_len = OAL_NETBUF_LEN(pst_buf);

    /* 如果不是sta，不需要地址转换 */
    if (pst_mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_STA) {
        return OAL_SUCC;
    }

    if (ul_pkt_len < ul_contig_len) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                       "{hmac_tx_proxysta_mat::The length of buf is invalid.}");
        return OAL_FAIL;
    }

    pst_ether_header = (mac_ether_header_stru *)OAL_NETBUF_HEADER(pst_buf);

    puc_eth_body = (oal_uint8 *)(pst_ether_header + 1);

    us_ether_type = pst_ether_header->us_ether_type;
    puc_scr_mac   = pst_ether_header->auc_ether_shost;

    if (OAL_HOST2NET_SHORT(ETHER_TYPE_PAE) == us_ether_type) {
        return OAL_SUCC;
    }

    /* ARP包地址转换 */
    if (OAL_HOST2NET_SHORT(ETHER_TYPE_ARP)  == us_ether_type) {
        ul_ret = hmac_tx_proxysta_arp_mat(pst_mac_vap, puc_eth_body, ul_pkt_len, ul_contig_len);
        if (ul_ret != OAL_SUCC) {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                           "{hmac_tx_proxysta_mat::hmac_tx_proxysta_arp_mat return value is %d.}", ul_ret);
            return ul_ret;
        }
    }

/*lint -e778*/
    /* DHCP 包地址转换 */
    if (OAL_HOST2NET_SHORT(ETHER_TYPE_IP) == us_ether_type) {
/*lint +e778*/
        ul_ret = hmac_tx_proxysta_ip_mat(pst_mac_vap,
                                         pst_ether_header,
                                         puc_eth_body,
                                         ul_pkt_len,
                                         ul_contig_len);
        if (ul_ret != OAL_SUCC) {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                           "{hmac_tx_proxysta_mat::hmac_tx_proxysta_dhcp_mat return value is %d.}", ul_ret);
            return ul_ret;
        }
    }

    /* icmpv6 包地址转换 */
    if (OAL_HOST2NET_SHORT(ETHER_TYPE_IPV6) == us_ether_type) {
        ul_ret = hmac_tx_proxysta_icmpv6_mat(pst_mac_vap, puc_eth_body, ul_pkt_len, ul_contig_len);
        if (ul_ret != OAL_SUCC) {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                           "{hmac_tx_proxysta_mat::hmac_tx_proxysta_icmpv6_mat return value is %d.}", ul_ret);
            return ul_ret;
        }
    }

    oal_set_mac_addr(puc_scr_mac, mac_mib_get_StationID(pst_mac_vap));

    return OAL_SUCC;
}

#endif


oal_uint32 hmac_tx_lan_to_wlan(mac_vap_stru *pst_vap, oal_netbuf_stru *pst_buf)
{
    oal_uint32 ul_ret;
#ifdef _PRE_WLAN_TCP_OPT
    hmac_device_stru *pst_hmac_device = OAL_PTR_NULL;
    hmac_vap_stru    *pst_hmac_vap    = OAL_PTR_NULL;     /* VAP结构体 */

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_vap->uc_vap_id);
    if (OAL_UNLIKELY(pst_hmac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_TX, "{hmac_tx_lan_to_wlan_tcp_opt::pst_dmac_vap null.}\r\n");
        return OAL_FAIL;
    }
    pst_hmac_device = hmac_res_get_mac_dev(pst_vap->uc_device_id);
    if (OAL_UNLIKELY(pst_hmac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
            "{hmac_tx_lan_to_wlan_tcp_opt::pst_hmac_device null.}\r\n");
        return OAL_FAIL;
    }
    if (pst_hmac_device->sys_tcp_tx_ack_opt_enable == OAL_TRUE) {
        ul_ret = hmac_transfer_tx_handler(pst_hmac_device, pst_hmac_vap, pst_buf);
    } else
#endif
    {
        ul_ret = hmac_tx_lan_to_wlan_no_tcp_opt(pst_vap, pst_buf);
    }
    return ul_ret;
}


oal_bool_enum_uint8 hmac_bridge_vap_should_drop(oal_netbuf_stru *pst_buf, mac_vap_stru *pst_vap)
{
    oal_uint8 uc_data_type;

    /* P2P_CL和P2P_DEV共VAP,入网过程中触发p2p_listen,VAP状态切换为LISTEN,
     * 在LISTEN状态不丢dhcp、eapol帧，防止入网失败
     */
    if (pst_vap->en_vap_state == MAC_VAP_STATE_STA_LISTEN) {
        uc_data_type =  mac_get_data_type_from_8023((oal_uint8 *)oal_netbuf_payload(pst_buf), MAC_NETBUFF_PAYLOAD_ETH);
        if ((uc_data_type == MAC_DATA_EAPOL) || (uc_data_type == MAC_DATA_DHCP)) {
            OAM_WARNING_LOG2(pst_vap->uc_vap_id,
                             OAM_SF_TX,
                             "{hmac_bridge_vap_should_drop::donot drop [%d]frame[EAPOL:3,DHCP:0]. vap state[%d].}",
                             uc_data_type,
                             pst_vap->en_vap_state);
            return OAL_FALSE;
        }
    }
    return OAL_TRUE;
}

#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
oal_uint8 g_tx_debug = 0;

oal_uint32 hmac_tx_data(hmac_vap_stru *pst_hmac_vap, oal_netbuf_stru *pst_netbuf)
{
    mac_vap_stru    *pst_vap = &(pst_hmac_vap->st_vap_base_info);
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    mac_device_stru *pst_mac_device;
#endif

    if (g_tx_debug) {
        OAL_IO_PRINT("hmac_tx_data start\n");
    }

#ifdef _PRE_WLAN_CHIP_TEST
    if (pst_vap->bit_al_tx_flag == OAL_SWITCH_ON) {
        OAM_INFO_LOG0(0, OAM_SF_TX, "{hmac_tx_data::the vap alway tx!}\r\n");
        oal_netbuf_free(pst_netbuf);
        return OAL_NETDEV_TX_OK;
    }
#endif

    /* 将以太网过来的帧上报SDT */
    hmac_tx_report_eth_frame(pst_vap, pst_netbuf);
    /* 判断VAP的状态，如果没有UP，则丢弃报文 */
    if (OAL_UNLIKELY(!((pst_vap->en_vap_state == MAC_VAP_STATE_UP) ||
        (pst_vap->en_vap_state == MAC_VAP_STATE_PAUSE)))) {
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_TX,
            "{hmac_tx_data::vap state[%d] != MAC_VAP_STATE_{UP|PAUSE}!}\r\n", pst_vap->en_vap_state);

        oal_netbuf_free(pst_netbuf);

        OAM_STAT_VAP_INCR(pst_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);

        return OAL_NETDEV_TX_OK;
    }

    OAL_NETBUF_NEXT(pst_netbuf) = OAL_PTR_NULL;
    OAL_NETBUF_PREV(pst_netbuf) = OAL_PTR_NULL;

    memset_s(OAL_NETBUF_CB(pst_netbuf), OAL_NETBUF_CB_SIZE(), 0, OAL_NETBUF_CB_SIZE());

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    pst_mac_device = mac_res_get_dev(pst_vap->uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_PROXYSTA, "{hmac_tx_data::mac_res_get_dev is null!}");
        oal_netbuf_free(pst_netbuf);

        return OAL_NETDEV_TX_OK;
    }

    if (pst_mac_device->st_cap_flag.bit_proxysta == OAL_TRUE) {
        /* ARP、DHCP、ICMPv6等数据包的地址转换 (只有proxy STA的发送的包才需要地址转换) */
        if ((pst_vap->st_vap_proxysta.en_is_proxysta == OAL_TRUE) &&
            (pst_vap->st_vap_proxysta.en_is_main_proxysta == OAL_FALSE)) {
            if (hmac_tx_proxysta_mat(pst_netbuf, pst_vap) != OAL_SUCC) {
                OAM_ERROR_LOG0(0, OAM_SF_PROXYSTA, "{hmac_tx_data::hmac_tx_proxysta_mat fail.}");
                oal_netbuf_free(pst_netbuf);

                return OAL_NETDEV_TX_OK;
            }
        }
    }

#endif
    /* 调用失败，要释放内核申请的netbuff内存池 */
    if (OAL_UNLIKELY(hmac_tx_lan_to_wlan(pst_vap, pst_netbuf) != OAL_SUCC)) {
        hmac_free_netbuf_list(pst_netbuf);
    }

    return OAL_NETDEV_TX_OK;
}

oal_uint32 hmac_tx_post_event(mac_vap_stru *pst_mac_vap)
{
    oal_uint32         ul_ret;
    frw_event_stru     *pst_event;
    frw_event_mem_stru *pst_event_mem;
    hmac_vap_stru      *pst_hmac_vap;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    /* tx_event_num减1后等于0表示eth to wlan数据事件为空，可以申请事件。同时这个方案能确保每次只有一个事件入队并处理，防止存在两个以上事件申请入队 */
    if (oal_atomic_dec_and_test(&(pst_hmac_vap->ul_tx_event_num))) {
        /* 申请事件内存 */
        pst_event_mem = FRW_EVENT_ALLOC(0);
        if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
            oal_atomic_inc(&(pst_hmac_vap->ul_tx_event_num)); /* 事件申请失败，tx_event_num要加回去，恢复成默认值1 */
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_TX, "{hmac_tx_post_event::pst_event_mem null.}");
            OAL_IO_PRINT("Hmac_tx_post_event fail to alloc event mem\n");
            return OAL_ERR_CODE_PTR_NULL;
        }

        pst_event = (frw_event_stru *)pst_event_mem->puc_data;

        /* 填写事件 */
        FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                           FRW_EVENT_TYPE_HOST_DRX,
                           HMAC_TX_HOST_DRX,
                           0,
                           FRW_EVENT_PIPELINE_STAGE_0,
                           pst_mac_vap->uc_chip_id,
                           pst_mac_vap->uc_device_id,
                           pst_mac_vap->uc_vap_id);

        /* 抛事入队列 */
        ul_ret = frw_event_post_event(pst_event_mem, pst_mac_vap->ul_core_id);
        if (ul_ret != OAL_SUCC) {
            oal_atomic_inc(&(pst_hmac_vap->ul_tx_event_num)); /* 事件入队失败，tx_event_num要加回去，恢复成默认值1 */
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TX,
                "{hmac_tx_post_event::frw_event_dispatch_event failed[%d].}", ul_ret);
            OAL_IO_PRINT("{hmac_tx_post_event::frw_event_dispatch_event failed}\n");
        }

        /* 释放事件内存 */
        FRW_EVENT_FREE(pst_event_mem);
    } else {
        /* 事件队列中已经有事件要处理(tx_event_num值为0)，这个时候预先减去的值要加回去，tx_event_num要恢复成0 */
        oal_atomic_inc(&(pst_hmac_vap->ul_tx_event_num));

        if (g_tx_debug) {
            /* 增加维测信息，把tx_event_num的值打印出来，如果此时的值不为0，就属于异常(不申请事件，但是值又不为0) */
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TX, "{hmac_tx_post_event::tx_event_num value is [%d].}",
                (oal_int32)oal_atomic_read(&(pst_hmac_vap->ul_tx_event_num)));
            OAL_IO_PRINT("do not post tx event, data in queue len %d,out queue len %d\n",
                         OAL_NETBUF_LIST_NUM(&pst_hmac_vap->st_tx_queue_head[pst_hmac_vap->uc_in_queue_id]),
                         OAL_NETBUF_LIST_NUM(&pst_hmac_vap->st_tx_queue_head[pst_hmac_vap->uc_out_queue_id]));
        }
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint8 hmac_vap_user_is_bw_limit(mac_vap_stru *pst_vap, oal_netbuf_stru *pst_netbuf)
{
    mac_ether_header_stru *pst_ether_hdr; /* 以太网头 */
    oal_uint16            us_user_idx;
    hmac_user_stru        *pst_hmac_user;
    oal_uint32            ul_ret;

    pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_netbuf);
    ul_ret = mac_vap_find_user_by_macaddr(pst_vap, pst_ether_hdr->auc_ether_dhost, &us_user_idx);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        return OAL_FALSE;
    }
    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(us_user_idx);
    if (pst_hmac_user == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    return (pst_vap->bit_vap_bw_limit || pst_hmac_user->en_user_bw_limit);
}

oal_uint32 hmac_tx_event_process(oal_mem_stru *pst_event_mem)
{
    frw_event_stru  *pst_event;
    hmac_vap_stru   *pst_hmac_vap;
    oal_uint32      ul_work = 0;
    oal_uint32      ul_reschedule = OAL_TRUE;
    oal_netbuf_stru *pst_netbuf;

    /* 入参判断 */
    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_TX, "{hmac_tx_event_process::pst_event_mem null.}");
        OAL_BUG_ON(1);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取事件 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    if (OAL_UNLIKELY(pst_event == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_TX, "{hmac_tx_event_process::pst_event null.}");
        OAL_BUG_ON(1);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_event->st_event_hdr.uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_TX, "{hmac_tx_event_process::pst_hmac_vap null.}");
        OAL_BUG_ON(1);
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (g_tx_debug)
        OAL_IO_PRINT("hmac_tx_event_process start\n");

    /* 事件申请时在hmac_tx_post_event处理时已经减为了0，这里要加回去，恢复成1 */
    oal_atomic_inc(&(pst_hmac_vap->ul_tx_event_num));

    if (g_tx_debug)
        OAL_IO_PRINT("oal_atomic_dec OK\n");

    oal_spin_lock_bh(&pst_hmac_vap->st_smp_lock);
    if (OAL_NETBUF_LIST_NUM(&pst_hmac_vap->st_tx_queue_head[pst_hmac_vap->uc_out_queue_id]) == 0) {
        /* 如果当前out queue空了, 才切换out_queue_id */
        pst_hmac_vap->uc_in_queue_id = pst_hmac_vap->uc_out_queue_id;
        pst_hmac_vap->uc_out_queue_id = (pst_hmac_vap->uc_out_queue_id + 1) & 1;
    }
    oal_spin_unlock_bh(&pst_hmac_vap->st_smp_lock);

    do {
        oal_spin_lock_bh(&pst_hmac_vap->st_smp_lock);
        pst_netbuf = OAL_NETBUF_DEQUEUE(&pst_hmac_vap->st_tx_queue_head[pst_hmac_vap->uc_out_queue_id]);
        oal_spin_unlock_bh(&pst_hmac_vap->st_smp_lock);

        if (!pst_netbuf) {
            if (g_tx_debug)
                OAL_IO_PRINT("OAL_NETBUF_DEQUEUE OK\n");
            ul_reschedule = OAL_FALSE;
            break;
        }

        if (hmac_vap_user_is_bw_limit(&(pst_hmac_vap->st_vap_base_info), pst_netbuf) == OAL_TRUE) {
            /* 增加维测信息，出现用户和vap限速之后，也会造成用户关连不上，ping不通问题 */
            OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
                "{hmac_tx_event_process::vap id[%d] hmac_vap_user_is_bw_limit.}",
                pst_hmac_vap->st_vap_base_info.uc_vap_id);
            oal_spin_lock_bh(&pst_hmac_vap->st_smp_lock);
            OAL_NETBUF_QUEUE_TAIL(&pst_hmac_vap->st_tx_queue_head[pst_hmac_vap->uc_out_queue_id], pst_netbuf);
            oal_spin_unlock_bh(&pst_hmac_vap->st_smp_lock);
            continue;
        }

        hmac_tx_data(pst_hmac_vap, pst_netbuf);
    } while (++ul_work < pst_hmac_vap->ul_tx_quata); // && jiffies == ul_start_time

    if (ul_reschedule == OAL_TRUE) {
        hmac_tx_post_event(&(pst_hmac_vap->st_vap_base_info));
    }

    return OAL_SUCC;
}
#endif


oal_void hmac_tx_ba_cnt_vary(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user, oal_uint8 uc_tidno,
    oal_netbuf_stru *pst_buf)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_uint32 ul_current_timestamp;
    oal_uint32 ul_runtime;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    if (oal_netbuf_is_tcp_ack((oal_ip_header_stru *)(oal_netbuf_data(pst_buf) + ETHER_HDR_LEN)) == OAL_TRUE) {
        pst_hmac_user->auc_ba_flag[uc_tidno]++;
        return ;
    }
#endif

    if (pst_hmac_user->auc_ba_flag[uc_tidno] == 0) {
        pst_hmac_user->auc_ba_flag[uc_tidno]++;
        pst_hmac_user->aul_last_timestamp[uc_tidno] = (oal_uint32)OAL_TIME_GET_STAMP_MS();

        return ;
    }

    ul_current_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();
    ul_runtime = (oal_uint32)OAL_TIME_GET_RUNTIME(ul_current_timestamp,
                                                  pst_hmac_user->aul_last_timestamp[uc_tidno]); //lint !e573
    if ((oal_netbuf_get_len(pst_buf) <= WLAN_MSDU_MAX_LEN && ul_runtime > WLAN_BA_CNT_INTERVAL) ||
         (oal_netbuf_get_len(pst_buf) > WLAN_MSDU_MAX_LEN && ul_runtime < WLAN_BA_CNT_INTERVAL)) {
        pst_hmac_user->auc_ba_flag[uc_tidno]++;
    } else {
        pst_hmac_user->auc_ba_flag[uc_tidno] = 0;
    }

    pst_hmac_user->aul_last_timestamp[uc_tidno] = (oal_uint32)OAL_TIME_GET_STAMP_MS();
#else
    pst_hmac_user->auc_ba_flag[uc_tidno]++;
#endif
}


/*lint -e19*/
oal_module_symbol(hmac_tx_wlan_to_wlan_ap);
oal_module_symbol(hmac_tx_lan_to_wlan);
oal_module_symbol(hmac_free_netbuf_list);

oal_module_symbol(hmac_tx_report_eth_frame);

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
oal_module_symbol(hmac_tx_proxysta_mat);
#endif

#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
oal_module_symbol(g_tx_debug);
oal_module_symbol(hmac_tx_post_event);
#endif
/*lint +e19*/

#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif

