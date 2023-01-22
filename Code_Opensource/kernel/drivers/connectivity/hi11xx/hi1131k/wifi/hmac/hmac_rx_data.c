

// 1 ͷ�ļ�����
#include "oal_profiling.h"
#include "mac_frame.h"
#include "mac_data.h"
#include "hmac_rx_data.h"
#include "dmac_ext_if.h"
#include "hmac_vap.h"
#include "hmac_ext_if.h"
#include "oam_ext_if.h"
#include "oal_ext_if.h"
#include "oal_net.h"
#include "hmac_frag.h"
#include "hmac_11i.h"
#include "mac_vap.h"
#include "securec.h"
#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
#include "hmac_custom_security.h"
#endif
#ifdef _PRE_WLAN_FEATURE_MCAST
#include "hmac_m2u.h"
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
#include "hmac_proxy_arp.h"
#endif
#include "hmac_blockack.h"
#include "hmac_tcp_opt.h"

#ifdef _PRE_WLAN_FEATURE_WAPI
#include "hmac_wapi.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_RX_DATA_C


// 2 ����ʵ��

#ifdef _PRE_WLAN_DFT_DUMP_FRAME
oal_void hmac_rx_report_eth_frame(mac_vap_stru *pst_mac_vap, oal_netbuf_stru *pst_netbuf)
{
    oal_uint16            us_user_idx = 0;
    mac_ether_header_stru *pst_ether_hdr = OAL_PTR_NULL;
    oal_uint32            ul_ret;
    oal_uint8             auc_user_macaddr[WLAN_MAC_ADDR_LEN] = {0};
    oal_switch_enum_uint8 en_eth_switch = 0;

    if (OAL_UNLIKELY(pst_netbuf == OAL_PTR_NULL)) {
        return;
    }

    /* ��skb��dataָ��ָ����̫����֡ͷ */
    oal_netbuf_push(pst_netbuf, ETHER_HDR_LEN);

    /* ��ȡĿ���û���Դ��id */
    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_netbuf);
        if (OAL_UNLIKELY(pst_ether_hdr == OAL_PTR_NULL)) {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_RX, "{hmac_rx_report_eth_frame::pst_ether_hdr null.}");
            oal_netbuf_pull(pst_netbuf, ETHER_HDR_LEN);
            return;
        }

        ul_ret = mac_vap_find_user_by_macaddr(pst_mac_vap, pst_ether_hdr->auc_ether_shost, &us_user_idx);
        if (ul_ret == OAL_ERR_CODE_PTR_NULL) {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_RX, "{hmac_rx_report_eth_frame::ul_ret null.}");
            oal_netbuf_pull(pst_netbuf, ETHER_HDR_LEN);
            return;
        }

        if (ul_ret == OAL_FAIL) {
            oal_netbuf_pull(pst_netbuf, ETHER_HDR_LEN);
            return;
        }

        oal_set_mac_addr(auc_user_macaddr, pst_ether_hdr->auc_ether_shost);
    } else if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
                if (pst_mac_vap->us_user_nums == 0) {
                    oal_netbuf_pull(pst_netbuf, ETHER_HDR_LEN);
                    /* OAL_SUCC , return */
                    return;
                }

        us_user_idx = pst_mac_vap->uc_assoc_vap_id;
        oal_set_mac_addr(auc_user_macaddr, pst_mac_vap->auc_bssid);
    }

    ul_ret = oam_report_eth_frame_get_switch(us_user_idx, OAM_OTA_FRAME_DIRECTION_TYPE_RX, &en_eth_switch);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_RX,
            "{hmac_rx_report_eth_frame::oam_report_eth_frame_get_switch failed[%d].}", ul_ret);
        oal_netbuf_pull(pst_netbuf, ETHER_HDR_LEN);
        return;
    }

    if (en_eth_switch == OAL_SWITCH_ON) {
        /* ��Ҫ������̫����֡�ϱ� */
        ul_ret = oam_report_eth_frame(auc_user_macaddr,
                                      oal_netbuf_data(pst_netbuf),
                                      (oal_uint16)OAL_NETBUF_LEN(pst_netbuf),
                                      OAM_OTA_FRAME_DIRECTION_TYPE_RX);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_RX,
                "{hmac_rx_report_eth_frame::oam_report_eth_frame return err: 0x%x.}\r\n", ul_ret);
        }
    }

    oal_netbuf_pull(pst_netbuf, ETHER_HDR_LEN);
}
#endif


OAL_STATIC OAL_INLINE uint32_t hmac_rx_frame_80211_to_eth(oal_netbuf_stru *pst_netbuf,
                                                          const unsigned char *puc_da,
                                                          const unsigned char *puc_sa)
{
    mac_ether_header_stru *pst_ether_hdr = OAL_PTR_NULL;
    mac_llc_snap_stru *pst_snap;
    oal_uint16 us_ether_type;

    pst_snap = (mac_llc_snap_stru *)oal_netbuf_data(pst_netbuf);
    us_ether_type = pst_snap->us_ether_type;

    /* ��payload��ǰ����6���ֽڣ����Ϻ���8���ֽڵ�snapͷ�ռ䣬������̫��ͷ��14�ֽڿռ� */
    oal_netbuf_push(pst_netbuf, HMAC_RX_DATA_ETHER_OFFSET_LENGTH);
    if (OAL_UNLIKELY(OAL_NETBUF_LEN(pst_netbuf) < sizeof(mac_ether_header_stru))) {
        OAM_WARNING_LOG1(0, OAM_SF_RX, "hmac_rx_frame_80211_to_eth::"
            "No room for eth hdr, netbuf len[%d]", OAL_NETBUF_LEN(pst_netbuf));
        return OAL_FAIL;
    }
    pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_netbuf);

    pst_ether_hdr->us_ether_type = us_ether_type;
    oal_set_mac_addr(pst_ether_hdr->auc_ether_shost, puc_sa);
    oal_set_mac_addr(pst_ether_hdr->auc_ether_dhost, puc_da);
    return OAL_SUCC;
}


oal_void  hmac_rx_free_netbuf(oal_netbuf_stru *pst_netbuf, oal_uint16 us_nums)
{
    oal_netbuf_stru *pst_netbuf_temp = OAL_PTR_NULL;
    oal_uint16 us_netbuf_num;

    if (OAL_UNLIKELY(pst_netbuf == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_rx_free_netbuf::pst_netbuf null.}\r\n");
        return;
    }

    for (us_netbuf_num = us_nums; us_netbuf_num > 0; us_netbuf_num--) {
        pst_netbuf_temp = OAL_NETBUF_NEXT(pst_netbuf);

        /* ����netbuf��Ӧ��user���ü��� */
        oal_netbuf_free(pst_netbuf);

        pst_netbuf = pst_netbuf_temp;

        if (pst_netbuf == OAL_PTR_NULL) {
            if (OAL_UNLIKELY(us_netbuf_num != 1)) {
                OAM_ERROR_LOG2(0, OAM_SF_RX,
                    "{hmac_rx_free_netbuf::pst_netbuf list broken, us_netbuf_num[%d]us_nums[%d].}",
                    us_netbuf_num, us_nums);
                return;
            }

            break;
        }
    }
}


oal_void hmac_rx_free_netbuf_list(oal_netbuf_head_stru *pst_netbuf_hdr, oal_uint16 uc_num_buf)
{
    oal_netbuf_stru *pst_netbuf = OAL_PTR_NULL;
    oal_uint16 us_idx;

    if (OAL_UNLIKELY(pst_netbuf_hdr == OAL_PTR_NULL)) {
        OAM_INFO_LOG0(0, OAM_SF_RX, "{hmac_rx_free_netbuf_list::pst_netbuf null.}");
        return;
    }

    OAM_INFO_LOG1(0, OAM_SF_RX, "{hmac_rx_free_netbuf_list::free [%d].}", uc_num_buf);

    for (us_idx = uc_num_buf; us_idx > 0; us_idx--) {
        pst_netbuf = oal_netbuf_delist(pst_netbuf_hdr);
        if (pst_netbuf != OAL_PTR_NULL) {
            OAM_INFO_LOG0(0, OAM_SF_RX, "{hmac_rx_free_netbuf_list::pst_netbuf null.}");
            oal_netbuf_free(pst_netbuf);
        }
    }
}


OAL_STATIC oal_uint32 hmac_rx_transmit_to_wlan(frw_event_hdr_stru *pst_event_hdr, oal_netbuf_head_stru *pst_netbuf_head)
{
    oal_netbuf_stru *pst_netbuf   = OAL_PTR_NULL; /* ��netbuf����ȡ������ָ��netbuf��ָ�� */
    oal_uint32      ul_netbuf_num;
    oal_uint32      ul_ret;
    oal_netbuf_stru *pst_buf_tmp  = OAL_PTR_NULL; /* �ݴ�netbufָ�룬����whileѭ�� */
    mac_tx_ctl_stru *pst_tx_ctl   = OAL_PTR_NULL;
    mac_vap_stru    *pst_mac_vap  = OAL_PTR_NULL;

    if (OAL_UNLIKELY((pst_event_hdr == OAL_PTR_NULL) || (pst_netbuf_head == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_rx_transmit_to_wlan::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ��ͷ��net buffer */
    pst_netbuf = oal_netbuf_peek(pst_netbuf_head);

    /* ��ȡmac vap �ṹ */
    ul_ret = hmac_tx_get_mac_vap(pst_event_hdr->uc_vap_id, &pst_mac_vap);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        ul_netbuf_num = oal_netbuf_list_len(pst_netbuf_head);
        hmac_rx_free_netbuf(pst_netbuf, (oal_uint16)ul_netbuf_num);
        OAM_WARNING_LOG3(pst_event_hdr->uc_vap_id, OAM_SF_RX,
            "{hmac_rx_transmit_to_wlan::find vap [%d] failed[%d], free [%d] netbuffer.}", pst_event_hdr->uc_vap_id,
            ul_ret, ul_netbuf_num);
        return ul_ret;
    }

    /* ѭ������ÿһ��netbuf��������̫��֡�ķ�ʽ���� */
    while (pst_netbuf != OAL_PTR_NULL) {
        pst_buf_tmp = OAL_NETBUF_NEXT(pst_netbuf);

        OAL_NETBUF_NEXT(pst_netbuf) = OAL_PTR_NULL;
        OAL_NETBUF_PREV(pst_netbuf) = OAL_PTR_NULL;

        pst_tx_ctl = (mac_tx_ctl_stru *)OAL_NETBUF_CB(pst_netbuf);
        memset_s(pst_tx_ctl, sizeof(mac_tx_ctl_stru), 0, sizeof(mac_tx_ctl_stru));

        pst_tx_ctl->en_event_type = FRW_EVENT_TYPE_WLAN_DTX;
        pst_tx_ctl->uc_event_sub_type = DMAC_TX_WLAN_DTX;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        /* set the queue map id when wlan to wlan */
        oal_skb_set_queue_mapping(pst_netbuf, WLAN_NORMAL_QUEUE);
#endif

        ul_ret = hmac_tx_lan_to_wlan(pst_mac_vap, pst_netbuf);
        /* ����ʧ�ܣ��Լ������Լ��ͷ�netbuff�ڴ� */
        if (ul_ret != OAL_SUCC) {
            hmac_free_netbuf_list(pst_netbuf);
        }

        pst_netbuf = pst_buf_tmp;
    }

    return OAL_SUCC;
}

OAL_STATIC void hmac_rx_init_amsdu_state(oal_netbuf_stru *netbuf, dmac_msdu_proc_state_stru *msdu_state)
{
    mac_rx_ctl_stru *rx_ctrl = NULL;
    uint32_t        need_pull_len;

    if ((msdu_state->uc_procd_netbuf_nums == 0) && (msdu_state->uc_procd_msdu_in_netbuf == 0)) {
        msdu_state->pst_curr_netbuf = netbuf;

        /* AMSDUʱ���׸�netbuf���а���802.11ͷ����Ӧ��payload��Ҫƫ�� */
        rx_ctrl = (mac_rx_ctl_stru *)oal_netbuf_cb(msdu_state->pst_curr_netbuf);

        msdu_state->puc_curr_netbuf_data   = (uint8_t*)mac_get_rx_cb_mac_hdr(rx_ctrl) + rx_ctrl->uc_mac_header_len;
        msdu_state->uc_netbuf_nums_in_mpdu = rx_ctrl->bit_buff_nums;
        msdu_state->uc_msdu_nums_in_netbuf = rx_ctrl->uc_msdu_in_buffer;
        msdu_state->us_submsdu_offset      = 0;
        msdu_state->uc_flag = rx_ctrl->bit_is_first_buffer;

        /* ʹnetbuf ָ��amsdu ֡ͷ */
        need_pull_len = (uint32_t)(msdu_state->puc_curr_netbuf_data - oal_netbuf_payload(netbuf));
        oal_netbuf_pull(msdu_state->pst_curr_netbuf, need_pull_len);
    }
}


OAL_STATIC uint32_t hmac_rx_amsdu_check_frame(dmac_msdu_proc_state_stru *msdu_state, uint16_t submsdu_len)
{
    mac_llc_snap_stru *llc_snap = NULL;

    if ((msdu_state->pst_curr_netbuf == NULL) || (msdu_state->puc_curr_netbuf_data == NULL)) {
        return OAL_FAIL;
    }
    if (msdu_state->us_submsdu_offset + MAC_SUBMSDU_HEADER_LEN + submsdu_len >
            OAL_NETBUF_LEN(msdu_state->pst_curr_netbuf)) {
        OAM_WARNING_LOG3(0, OAM_SF_RX, "hmac_rx_amsdu_check_frame_len:submsdu_len=%d,offset=%d,netbuf_len=%d",
            submsdu_len, msdu_state->us_submsdu_offset, OAL_NETBUF_LEN(msdu_state->pst_curr_netbuf));
        return OAL_FAIL;
    }
    llc_snap = (mac_llc_snap_stru *)(msdu_state->puc_curr_netbuf_data +
                                     msdu_state->us_submsdu_offset + MAC_SUBMSDU_HEADER_LEN);
    if ((llc_snap->uc_llc_dsap != 0xAA) || (llc_snap->uc_llc_ssap != 0xAA) ||
        (llc_snap->uc_control  != 0x03)) {
        OAM_WARNING_LOG3(0, OAM_SF_RX, "hmac_rx_amsdu_check_frame_len:dsap=0x%02x,ssap=0x%02x,control=0x%02x",
            llc_snap->uc_llc_dsap, llc_snap->uc_llc_ssap, llc_snap->uc_control);
        return OAL_FAIL;
    }
    return OAL_SUCC;
}

OAL_STATIC void hmac_rx_free_amsdu_netbuf(oal_netbuf_stru *netbuf)
{
    oal_netbuf_stru *netbuf_next = NULL;
    while (netbuf != NULL) {
        netbuf_next = oal_get_netbuf_next(netbuf);
        oal_netbuf_free(netbuf);
        netbuf = netbuf_next;
    }
}

OAL_STATIC uint32_t hmac_rx_amsdu_is_first_sub_msdu_valid(dmac_msdu_proc_state_stru *msdu_state, uint8_t *dst_addr)
{
    uint8_t mac_addr_snap_header[WLAN_MAC_ADDR_LEN] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00};
    if (!msdu_state->uc_flag) {
        return OAL_SUCC;
    }
    if (oal_memcmp(dst_addr, mac_addr_snap_header, WLAN_MAC_ADDR_LEN) == 0) {
        return OAL_FAIL;
    }
    return OAL_SUCC;
}

/* msdu numsΪ1ʱ�����ϱ����� */
OAL_STATIC uint32_t hmac_rx_get_amsdu_num_eq1(uint8_t *submsdu_hdr, uint16_t submsdu_len,
    dmac_msdu_stru *msdu, dmac_msdu_proc_state_stru *msdu_state, mac_msdu_proc_status_enum_uint8 *proc_state)
{
    oal_set_mac_addr(msdu->auc_sa, (submsdu_hdr + MAC_SUBMSDU_SA_OFFSET));
    oal_set_mac_addr(msdu->auc_da, (submsdu_hdr + MAC_SUBMSDU_DA_OFFSET));

    if (hmac_rx_amsdu_is_first_sub_msdu_valid(msdu_state, msdu->auc_da) != OAL_SUCC) {
        *proc_state = MAC_PROC_ERROR;
        return OAL_FAIL;
    }
    msdu_state->uc_flag = OAL_FALSE;

    /* ָ��amsdu֡�� */
    oal_netbuf_pull(msdu_state->pst_curr_netbuf, MAC_SUBMSDU_HEADER_LEN);
    if (submsdu_len > OAL_NETBUF_LEN(msdu_state->pst_curr_netbuf)) {
        *proc_state = MAC_PROC_ERROR;
        OAM_WARNING_LOG2(0, OAM_SF_RX, "{hmac_rx_parse_amsdu::us_submsdu_len %d is not valid netbuf len=%d.}",
            submsdu_len, OAL_NETBUF_LEN(msdu_state->pst_curr_netbuf));
        hmac_rx_free_amsdu_netbuf(msdu_state->pst_curr_netbuf);
        return OAL_FAIL;
    }

    oal_netbuf_trim(msdu_state->pst_curr_netbuf, OAL_NETBUF_LEN(msdu_state->pst_curr_netbuf));
    oal_netbuf_put(msdu_state->pst_curr_netbuf, submsdu_len);

    /* ֱ��ʹ�ø�netbuf�ϱ����ں� ��ȥһ��netbuf����Ϳ��� */
    msdu->pst_netbuf = msdu_state->pst_curr_netbuf;
    return OAL_SUCC;
}

/* msdu nums����1ʱ�����ϱ����� */
OAL_STATIC uint32_t hmac_rx_get_amsdu_num_gt1(mac_msdu_proc_status_enum_uint8 *proc_state, uint16_t submsdu_len,
    uint8_t *submsdu_pad_length, uint8_t *submsdu_hdr, dmac_msdu_stru *msdu, dmac_msdu_proc_state_stru *msdu_state)
{
    if (hmac_rx_amsdu_is_first_sub_msdu_valid(msdu_state, msdu->auc_da) != OAL_SUCC) {
        *proc_state = MAC_PROC_ERROR;
        return OAL_FAIL;
    }
    msdu_state->uc_flag = OAL_FALSE;

    /* ��ȡsubmsdu�������Ϣ */
    if (hmac_rx_amsdu_check_frame(msdu_state, submsdu_len) != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_RX, "{hmac_rx_get_amsdu_num_gt1::hmac_rx_amsdu_check_frame failed}");
        OAM_STAT_VAP_INCR(0, rx_no_buff_dropped, 1);
        hmac_rx_free_amsdu_netbuf(msdu_state->pst_curr_netbuf);
        return OAL_FAIL;
    }
    mac_get_submsdu_pad_len(MAC_SUBMSDU_HEADER_LEN + submsdu_len, submsdu_pad_length);
    oal_set_mac_addr(msdu->auc_sa, (submsdu_hdr + MAC_SUBMSDU_SA_OFFSET));
    oal_set_mac_addr(msdu->auc_da, (submsdu_hdr + MAC_SUBMSDU_DA_OFFSET));

#ifdef _PRE_LWIP_ZERO_COPY
/* ����pbuf */
    msdu->pst_netbuf = oal_pbuf_netbuf_alloc(MAC_SUBMSDU_HEADER_LEN + submsdu_len + *submsdu_pad_length);
#else
/* ��Ե�ǰ��netbuf�������µ�subnetbuf�������ö�Ӧ��netbuf����Ϣ����ֵ����Ӧ��msdu */
    msdu->pst_netbuf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF,
        (MAC_SUBMSDU_HEADER_LEN + submsdu_len + *submsdu_pad_length), OAL_NETBUF_PRIORITY_MID);
#endif
    if (msdu->pst_netbuf == NULL) {
        OAM_ERROR_LOG2(0, OAM_SF_RX, "{hmac_rx_get_amsdu_num_gt1::netbuf null, submsdu_len[%d], submsdu_pad_len[%d].}",
            submsdu_len, *submsdu_pad_length);
        OAM_ERROR_LOG4(0, OAM_SF_RX,
            "{hmac_rx_get_amsdu_num_gt1::submsdu_offset[%d], msdu_nums[%d], procd_msdu[%d], netbuf_nums[%d] in mpdu.}",
            msdu_state->us_submsdu_offset, msdu_state->uc_msdu_nums_in_netbuf, msdu_state->uc_procd_msdu_in_netbuf,
            msdu_state->uc_netbuf_nums_in_mpdu);
        OAM_ERROR_LOG2(0, OAM_SF_RX, "{hmac_rx_get_amsdu_num_gt1::procd_netbuf_nums[%d], procd_msdu_nums_in_mpdu[%d].}",
            msdu_state->uc_procd_netbuf_nums, msdu_state->uc_procd_msdu_nums_in_mpdu);

        OAM_STAT_VAP_INCR(0, rx_no_buff_dropped, 1);
        hmac_rx_free_amsdu_netbuf(msdu_state->pst_curr_netbuf);
        return OAL_FAIL;
    }

    OAL_MEM_NETBUF_TRACE(msdu->pst_netbuf, OAL_TRUE);

    /* ���ÿһ����msdu���޸�netbuf��end��data��tail��lenָ�� */
    oal_netbuf_put(msdu->pst_netbuf, submsdu_len + HMAC_RX_DATA_ETHER_OFFSET_LENGTH);
    oal_netbuf_pull(msdu->pst_netbuf, HMAC_RX_DATA_ETHER_OFFSET_LENGTH);
    if (memcpy_s(msdu->pst_netbuf->data, submsdu_len + *submsdu_pad_length,
        (submsdu_hdr + MAC_SUBMSDU_HEADER_LEN), submsdu_len) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_rx_get_amsdu_num_gt1::memcpy_s failed}");
        hmac_rx_free_amsdu_netbuf(msdu_state->pst_curr_netbuf);
        oal_netbuf_free(msdu->pst_netbuf);
        msdu->pst_netbuf = NULL;
        return OAL_FAIL;
    }
    return OAL_SUCC;
}

OAL_STATIC void hmac_check_and_free_netbuff(oal_netbuf_stru *pst_netbuf, oal_bool_enum_uint8 need_free_netbuf)
{
    if (need_free_netbuf) {
        oal_netbuf_free(pst_netbuf);
    }
}

/*
 * ��������  : ������ÿһ��AMSDU�е�MSDU
 * �������  : ָ��MPDU�ĵ�һ��netbuf��ָ��
 * �������  : (1)ָ��ǰҪת����MSDU��ָ��
 *             (2)���ڼ�¼����ǰ��MPDU��MSDU����Ϣ
 *             (3)��ǰMPDU�Ĵ���״̬:��ʶ��MPDU�Ƿ������
 * �� �� ֵ  : �ɹ�����ʧ��ԭ��
 */
uint32_t hmac_rx_parse_amsdu(oal_netbuf_stru *netbuf, dmac_msdu_stru *msdu,
    dmac_msdu_proc_state_stru *msdu_state, mac_msdu_proc_status_enum_uint8 *proc_state)
{
    mac_rx_ctl_stru     *rx_ctrl          = NULL; /* MPDU�Ŀ�����Ϣ */
    uint8_t             *buffer_data_addr = NULL; /* ָ��netbuf�������ָ�� */
    uint16_t            offset;                   /* submsdu�����dataָ���ƫ�� */
    uint16_t            submsdu_len       = 0;    /* submsdu�ĳ��� */
    uint8_t             submsdu_pad_len   = 0;    /* submsdu����䳤�� */
    uint8_t             *submsdu_hdr      = NULL; /* ָ��submsduͷ����ָ�� */
    oal_netbuf_stru     *pst_netbuf_prev  = NULL;
    oal_bool_enum_uint8 need_free_netbuf;

    if (OAL_UNLIKELY(netbuf == NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_rx_parse_amsdu::pst_netbuf null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �״ν���ú�������AMSDU */
    hmac_rx_init_amsdu_state(netbuf, msdu_state);

    /* ��ȡsubmsdu��ͷָ�� */
    buffer_data_addr = msdu_state->puc_curr_netbuf_data;
    offset           = msdu_state->us_submsdu_offset;
    submsdu_hdr      = buffer_data_addr + offset;

    /* 1��netbuf ֻ����һ��msdu */
    mac_get_submsdu_len(submsdu_hdr, &submsdu_len);
    if (msdu_state->uc_msdu_nums_in_netbuf == 1) {
        if (hmac_rx_get_amsdu_num_eq1(submsdu_hdr, submsdu_len, msdu, msdu_state, proc_state) != OAL_SUCC) {
            OAM_WARNING_LOG0(0, OAM_SF_RX, "{hmac_rx_parse_amsdu::hmac_rx_get_amsdu_num_eq1 fail.}");
            return OAL_FAIL;
        }
        /* ֱ��ʹ�ø�netbuf�ϱ����ں� ��ȥһ��netbuf����Ϳ��� */
        need_free_netbuf = OAL_FALSE;
    } else {
        if (hmac_rx_get_amsdu_num_gt1(proc_state, submsdu_len, &submsdu_pad_len,
            submsdu_hdr, msdu, msdu_state) != OAL_SUCC) {
            return OAL_FAIL;
        }
        need_free_netbuf = OAL_TRUE;
    }

    /* ���ӵ�ǰ�Ѵ����msdu�ĸ��� */
    msdu_state->uc_procd_msdu_in_netbuf++;

    /* ��ȡ��ǰ��netbuf�е���һ��msdu���д��� */
    if (msdu_state->uc_procd_msdu_in_netbuf < msdu_state->uc_msdu_nums_in_netbuf) {
        msdu_state->us_submsdu_offset += submsdu_len + submsdu_pad_len + MAC_SUBMSDU_HEADER_LEN;
    } else if (msdu_state->uc_procd_msdu_in_netbuf == msdu_state->uc_msdu_nums_in_netbuf) {
        msdu_state->uc_procd_netbuf_nums++;
        pst_netbuf_prev = msdu_state->pst_curr_netbuf;

        /* ��ȡ��MPDU��Ӧ����һ��netbuf������ */
        if (msdu_state->uc_procd_netbuf_nums < msdu_state->uc_netbuf_nums_in_mpdu) {
            msdu_state->pst_curr_netbuf      = OAL_NETBUF_NEXT(msdu_state->pst_curr_netbuf);
            msdu_state->puc_curr_netbuf_data = oal_netbuf_data(msdu_state->pst_curr_netbuf);

            rx_ctrl = (mac_rx_ctl_stru *)oal_netbuf_cb(msdu_state->pst_curr_netbuf);

            msdu_state->uc_msdu_nums_in_netbuf  = rx_ctrl->uc_msdu_in_buffer;
            msdu_state->us_submsdu_offset       = 0;
            msdu_state->uc_procd_msdu_in_netbuf = 0;

            /* amsdu �ڶ���netbuf len��0, ��put�����size */
            oal_netbuf_put(msdu_state->pst_curr_netbuf, WLAN_MEM_NETBUF_SIZE2);
        } else if (msdu_state->uc_procd_netbuf_nums == msdu_state->uc_netbuf_nums_in_mpdu) {
            *proc_state = MAC_PROC_LAST_MSDU;
            hmac_check_and_free_netbuff(pst_netbuf_prev, need_free_netbuf);
            return OAL_SUCC;
        } else {
            *proc_state = MAC_PROC_ERROR;
            OAM_WARNING_LOG0(0, OAM_SF_RX, "{hmac_rx_parse_amsdu::pen_proc_state is err for uc_procd_netbuf_nums > uc_netbuf_nums_in_mpdul.}");
            hmac_rx_free_amsdu_netbuf(msdu_state->pst_curr_netbuf);
            return OAL_FAIL;
        }

        hmac_check_and_free_netbuff(pst_netbuf_prev, need_free_netbuf);
    } else {
        *proc_state = MAC_PROC_ERROR;
        OAM_WARNING_LOG0(0, OAM_SF_RX, "{hmac_rx_parse_amsdu::pen_proc_state is err for uc_procd_netbuf_nums > uc_netbuf_nums_in_mpdul.}");
        hmac_rx_free_amsdu_netbuf(msdu_state->pst_curr_netbuf);
        return OAL_FAIL;
    }

    *proc_state = MAC_PROC_MORE_MSDU;

    return OAL_SUCC;
}

/* ����amsdu ���һ�� netbuf nextָ��Ϊnull */
OAL_STATIC void hmac_rx_clear_amsdu_last_netbuf_pointer(oal_netbuf_stru *netbuf, uint8_t num_buf)
{
    if (num_buf == 0) {
        netbuf->next = NULL;
        return;
    }

    while (netbuf != NULL) {
        num_buf--;
        if (num_buf == 0) {
            netbuf->next = NULL;
            break;
        }
        netbuf = oal_get_netbuf_next(netbuf);
    }
}

OAL_STATIC uint32_t hmac_rx_add_frame_to_netbuf_list(hmac_vap_stru            *vap,
                                                     oal_netbuf_head_stru     *netbuf_header,
                                                     oal_netbuf_stru          *netbuf,
                                                     mac_ieee80211_frame_stru *frame_hdr)
{
    hmac_rx_ctl_stru *rx_ctrl = NULL;  /* ָ��MPDU���ƿ���Ϣ��ָ�� */
    hmac_user_stru   *hmac_user = NULL;
    uint8_t          *tmp_addr = NULL;
    uint8_t           source_addr[WLAN_MAC_ADDR_LEN] = {0};
    uint8_t           dest_addr[WLAN_MAC_ADDR_LEN] = {0};
#if defined(_PRE_WLAN_FEATURE_WPA) || defined(_PRE_WLAN_FEATURE_WPA2)
    mac_ether_header_stru *ether_hdr = NULL;
#endif

    rx_ctrl = (hmac_rx_ctl_stru *)oal_netbuf_cb(netbuf);
    hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(rx_ctrl->st_rx_info.us_ta_user_idx);
    if (OAL_UNLIKELY(hmac_user == NULL)) {
        /* ��ӡ��net buf�����Ϣ */
        mac_rx_report_80211_frame((uint8_t *)&(vap->st_vap_base_info), (uint8_t *)&(rx_ctrl->st_rx_info),
                                  netbuf, OAM_OTA_TYPE_RX_HMAC_CB);
        return OAL_ERR_CODE_PTR_NULL;
    }

    netbuf = hmac_defrag_process(hmac_user, netbuf, rx_ctrl->st_rx_info.uc_mac_header_len);
    if (netbuf == NULL) {
        return OAL_SUCC;
    }

    rx_ctrl   = (hmac_rx_ctl_stru *)oal_netbuf_cb(netbuf);
    frame_hdr = (mac_ieee80211_frame_stru *)rx_ctrl->st_rx_info.pul_mac_hdr_start_addr;

    /* ��MACͷ�л�ȡԴ��ַ��Ŀ�ĵ�ַ */
    mac_rx_get_sa(frame_hdr, &tmp_addr);
    oal_set_mac_addr(source_addr, tmp_addr);

    mac_rx_get_da(frame_hdr, &tmp_addr);
    oal_set_mac_addr(dest_addr, tmp_addr);

    /* ��netbuf��dataָ��ָ��mac frame��payload����Ҳ����ָ����8�ֽڵ�snapͷ */
    oal_netbuf_pull(netbuf, rx_ctrl->st_rx_info.uc_mac_header_len);

    /* ��MSDUת��Ϊ��̫����ʽ��֡ */
    if (hmac_rx_frame_80211_to_eth(netbuf, dest_addr, source_addr) != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_RX, "hmac_rx_prepare_msdu_list_to_wlan: frame len is error");
        oal_netbuf_free(netbuf);
        return OAL_FAIL;
    }

    memset_s(OAL_NETBUF_CB(netbuf), OAL_NETBUF_CB_SIZE(), 0, OAL_NETBUF_CB_SIZE());

#if defined(_PRE_WLAN_FEATURE_WPA) || defined(_PRE_WLAN_FEATURE_WPA2)
    ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(netbuf);
    if (hmac_11i_ether_type_filter(vap, ether_hdr->auc_ether_shost, ether_hdr->us_ether_type) != OAL_SUCC) {
        /* ���հ�ȫ���ݹ��� */
        oam_report_eth_frame(dest_addr, (uint8_t*)ether_hdr, (uint16_t)OAL_NETBUF_LEN(netbuf),
            OAM_OTA_FRAME_DIRECTION_TYPE_RX);

        oal_netbuf_free(netbuf);
        OAM_STAT_VAP_INCR(vap->st_vap_base_info.uc_vap_id, rx_portvalid_check_fail_dropped, 1);
        return OAL_FAIL;
    } else
#endif
    {
        /* ��MSDU���뵽netbuf������� */
        oal_netbuf_add_to_list_tail(netbuf, netbuf_header);
    }
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_rx_prepare_msdu_list_to_wlan(hmac_vap_stru            *pst_vap,
                                                        oal_netbuf_head_stru     *pst_netbuf_header,
                                                        oal_netbuf_stru          *pst_netbuf,
                                                        mac_ieee80211_frame_stru *pst_frame_hdr)
{
    hmac_rx_ctl_stru                *pst_rx_ctrl      = OAL_PTR_NULL;  /* ָ��MPDU���ƿ���Ϣ��ָ�� */
    dmac_msdu_stru                  st_msdu;                          /* �������������ÿһ��MSDU */
    mac_msdu_proc_status_enum_uint8 en_process_state = MAC_PROC_BUTT; /* ����AMSDU��״̬ */
    dmac_msdu_proc_state_stru       st_msdu_state    = { 0 };         /* ��¼MPDU�Ĵ�����Ϣ */
    oal_uint32                      ul_ret;
#if defined(_PRE_WLAN_FEATURE_WPA) || defined(_PRE_WLAN_FEATURE_WPA2)
    mac_ether_header_stru           *pst_ether_hdr;
#endif

    if (OAL_UNLIKELY(pst_netbuf == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_rx_prepare_msdu_list_to_wlan::pst_netbuf null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ����MPDU-->MSDU ,��MSDU���netbuf�� */
    OAL_MEM_NETBUF_TRACE(pst_netbuf, OAL_TRUE);

    /* ��ȡ��MPDU�Ŀ�����Ϣ */
    pst_rx_ctrl = (hmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

    memset_s(&st_msdu, OAL_SIZEOF(dmac_msdu_stru), 0, OAL_SIZEOF(dmac_msdu_stru));

    /* ���һ:����AMSDU�ۺϣ����MPDU��Ӧһ��MSDU��ͬʱ��Ӧһ��NETBUF,��MSDU��ԭ
       ����̫����ʽ֡�Ժ�ֱ�Ӽ��뵽netbuf�������
    */
    if (pst_rx_ctrl->st_rx_info.bit_amsdu_enable == OAL_FALSE) {
        ul_ret = hmac_rx_add_frame_to_netbuf_list(pst_vap, pst_netbuf_header, pst_netbuf, pst_frame_hdr);
        if (ul_ret != OAL_SUCC) {
            return ul_ret;
        }
    } else {
        /* �����:AMSDU�ۺ� */
        st_msdu_state.uc_procd_netbuf_nums    = 0;
        st_msdu_state.uc_procd_msdu_in_netbuf = 0;
        /* amsdu ���һ��netbuf nextָ����Ϊ NULL ����ʱ�����ͷ�amsdu netbuf */
        hmac_rx_clear_amsdu_last_netbuf_pointer(pst_netbuf, pst_rx_ctrl->st_rx_info.bit_buff_nums);
        do {
            /* ��ȡ��һ��Ҫת����msdu */
            ul_ret = hmac_rx_parse_amsdu(pst_netbuf, &st_msdu, &st_msdu_state, &en_process_state);
            if (ul_ret != OAL_SUCC) {
                OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                    "{hmac_rx_prepare_msdu_list_to_wlan::hmac_rx_parse_amsdu failed[%d].}", ul_ret);
                return ul_ret;
            }

            /* ��MSDUת��Ϊ��̫����ʽ��֡ */
            hmac_rx_frame_80211_to_eth(st_msdu.pst_netbuf, st_msdu.auc_da, st_msdu.auc_sa);

#if defined(_PRE_WLAN_FEATURE_WPA) || defined(_PRE_WLAN_FEATURE_WPA2)
            pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(st_msdu.pst_netbuf);
            if (hmac_11i_ether_type_filter(pst_vap, pst_ether_hdr->auc_ether_shost,
                pst_ether_hdr->us_ether_type) != OAL_SUCC) {
                /* ���հ�ȫ���ݹ��� */
                oam_report_eth_frame(st_msdu.auc_da, (oal_uint8*)pst_ether_hdr, (oal_uint16)OAL_NETBUF_LEN(pst_netbuf),
                    OAM_OTA_FRAME_DIRECTION_TYPE_RX);

                oal_netbuf_free(st_msdu.pst_netbuf);
                OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, rx_portvalid_check_fail_dropped, 1);
                continue;
            } else
#endif
            {
                /* ��MSDU���뵽netbuf������� */
                oal_netbuf_add_to_list_tail(st_msdu.pst_netbuf, pst_netbuf_header);
            }
        } while (en_process_state != MAC_PROC_LAST_MSDU);
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_PROXYSTA

oal_uint16 hmac_rx_proxysta_update_checksum_addr(oal_uint16 us_osum, oal_uint8 uc_omac[6], oal_uint8 uc_nmac[6])

{
    oal_uint16 us_nsum = us_osum;
    oal_uint32 ul_sum;
    oal_uint32 ul_loop;

    if (us_osum == 0) {
        return us_nsum;
    }

    for (ul_loop = 0; ul_loop < 3; ul_loop++) {
        ul_sum = us_nsum;
        ul_sum += *(oal_uint16 *)&uc_omac[ul_loop * 2] + (~(*(oal_uint16 *)&uc_nmac[ul_loop * 2]) & 0XFFFF);
        ul_sum  = (ul_sum >> 16) + (ul_sum & 0XFFFF);
        us_nsum = (oal_uint16)((ul_sum >> 16) + ul_sum);
    }

    return us_nsum;
}


OAL_STATIC oal_uint32 hmac_rx_proxysta_arp_mat(mac_vap_stru          *pst_mac_vap,
                                               mac_ether_header_stru *pst_ether_header,
                                               mac_device_stru       *pst_mac_device,
                                               oal_uint32            ul_pkt_len)
{
    oal_eth_arphdr_stru *pst_arp          = OAL_PTR_NULL;
    oal_uint8           *puc_eth_body     = OAL_PTR_NULL;
    oal_uint8           *puc_arp_dmac     = OAL_PTR_NULL;
    mac_vap_stru        *pst_mac_temp_vap = OAL_PTR_NULL;
    oal_uint8           *puc_des_mac      = OAL_PTR_NULL;
    oal_uint8           uc_vap_idx;
    oal_bool_enum_uint8 en_is_mcast;
    oal_uint32          ul_contig_len;
    oal_bool_enum_uint8 en_is_arp_mac_changed = OAL_FALSE;

    /* ************************************************************************* */
    /*                      ARP Frame Format                                   */
    /* ----------------------------------------------------------------------- */
    /* |��̫��Ŀ�ĵ�ַ|��̫��Դ��ַ|֡����|Ӳ������|Э������|Ӳ����ַ����|     */
    /* ----------------------------------------------------------------------- */
    /* | 6 (���滻)   |6           |2     |2       |2       |1           |     */
    /* ----------------------------------------------------------------------- */
    /* |Э���ַ����|op|���Ͷ���̫����ַ|���Ͷ�IP��ַ|Ŀ����̫����ַ|Ŀ��IP��ַ */
    /* ----------------------------------------------------------------------- */
    /* | 1          |2 |6               |4           |6 (���滻)    |4         */
    /* ----------------------------------------------------------------------- */
    /*                                                                         */
    /* ************************************************************************* */
    /* �����Ϸ��Լ�� */
    if ((pst_mac_vap == OAL_PTR_NULL) || (pst_ether_header == OAL_PTR_NULL) || (pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_PROXYSTA,
                       "{hmac_rx_proxysta_arp_mat::The input parameter of hmac_rx_proxysta_arp_mat is OAL_PTR_NULL.}");
        return OAL_FAIL;
    }

    ul_contig_len = OAL_SIZEOF(mac_ether_header_stru);

    /* ��ȡ��̫��Ŀ��mac�����ݶ� */
    puc_des_mac = pst_ether_header->auc_ether_dhost;
    puc_eth_body = (oal_uint8 *)(pst_ether_header + 1);

    /* ��ȡ��̫��֡Ŀ�ĵ�ַ�Ƿ�Ϊ�ಥ��ַ */
    en_is_mcast = ETHER_IS_MULTICAST(puc_des_mac);

    /* ARP ����ַת�� */
    pst_arp = (oal_eth_arphdr_stru *)puc_eth_body;
    ul_contig_len += OAL_SIZEOF(oal_eth_arphdr_stru);
    if (ul_pkt_len < ul_contig_len) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_rx_proxysta_arp_mat::The length of buf is less than the sum of mac_ether_header_stru \
            and oal_eth_arphdr_stru.}");
        return OAL_FAIL;
    }

    /*lint -e778*/
    if ((pst_arp->uc_ar_hln == ETHER_ADDR_LEN) && (OAL_NET2HOST_SHORT(ETHER_TYPE_IP) == pst_arp->us_ar_pro)) {
        puc_arp_dmac = pst_arp->auc_ar_tha;
    } else {
        pst_arp = OAL_PTR_NULL;
    }
    /*lint +e778*/
    if (pst_arp != OAL_PTR_NULL) {
        if (en_is_mcast != OAL_TRUE) {
            switch (OAL_NET2HOST_SHORT(pst_arp->us_ar_op)) {
                case OAL_ARPOP_REQUEST:
                case OAL_ARPOP_REPLY:
                    /* �滻mac��ַΪout vap��mac��ַ */
                    oal_set_mac_addr(puc_arp_dmac, pst_mac_vap->st_vap_proxysta.auc_oma);

                    break;
                default:
                    OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                        "{hmac_rx_proxysta_arp_mat::do not replace arp addr.}");
            }
        } else {
            OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                          "{hmac_rx_proxysta_arp_mat::received arp broadcast packet, start process.}");

            /* root ap�����������й㲥����ֻ��sta0�����յ�����arp����arp_dmacΪstax���ĵ�ַ���ҵ�stax��,�滻Ϊ������oma��ַ */
            if (oal_compare_mac_addr(puc_arp_dmac, pst_mac_vap->st_vap_proxysta.auc_oma) != OAL_FALSE) {
                OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                              "{hmac_rx_proxysta_arp_mat::start into if.}");

                /* ����Device�µ�vap(STAģʽ)����Ŀ�ĵ�ַΪ��vap�Լ��ĵ�ַ��ͬʱ������oma��ַ�滻֡��Ŀ�ĵ�ַ */
                for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++) {
                    pst_mac_temp_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
                    if (OAL_UNLIKELY(pst_mac_temp_vap == OAL_PTR_NULL)) {
                        OAM_WARNING_LOG0(uc_vap_idx, OAM_SF_PROXYSTA,
                            "{hmac_rx_proxysta_arp_mat::pst_mac_vap is null ptr!}");
                        return OAL_SUCC;
                    }

                    if (oal_compare_mac_addr(puc_arp_dmac, mac_mib_get_StationID(pst_mac_temp_vap)) == OAL_FALSE) {
                        /* �滻mac��ַΪout vap��mac��ַ */
                        oal_set_mac_addr(puc_arp_dmac, pst_mac_temp_vap->st_vap_proxysta.auc_oma);

                        OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                            "{hmac_rx_proxysta_arp_mat::changed multi arp des mac with proxysta's oma mac.}");

                        en_is_arp_mac_changed = OAL_TRUE;
                        break;
                    }
                }

                /* ���arm macû�иı䣬���������arp�㲥�������軻��ַ��ֱ�ӷ��� */
                if (en_is_arp_mac_changed == OAL_FALSE) {
                    OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                                  "{hmac_rx_proxysta_arp_mat::do not changed multi arp des mac, return succ.}");
                    return OAL_SUCC;
                }
            }
        }
    }

    oal_set_mac_addr(puc_des_mac, pst_mac_vap->st_vap_proxysta.auc_oma);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_rx_proxysta_ip_mat(mac_vap_stru          *pst_mac_vap,
                                              mac_ether_header_stru *pst_ether_header,
                                              mac_device_stru       *pst_mac_device,
                                              oal_uint32            ul_pkt_len)
{
    oal_ip_header_stru   *pst_ip_header    = OAL_PTR_NULL;
    oal_udp_header_stru  *pst_udp_header   = OAL_PTR_NULL;
    oal_dhcp_packet_stru *pst_dhcp_packet  = OAL_PTR_NULL;
    mac_vap_stru         *pst_mac_temp_vap = OAL_PTR_NULL;
    oal_uint8            *puc_eth_body     = OAL_PTR_NULL;
    oal_uint8            *puc_des_mac      = OAL_PTR_NULL;
    oal_uint16           us_ip_header_len;
    oal_uint8            uc_vap_idx;
    oal_bool_enum_uint8  en_is_mcast;
    oal_uint32           ul_contig_len;

    /* �����Ϸ��Լ�� */
    if ((pst_mac_vap == OAL_PTR_NULL) || (pst_ether_header == OAL_PTR_NULL) || (pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_PROXYSTA,
            "{hmac_rx_proxysta_ip_mat::The input parameter of hmac_rx_proxysta_ip_mat is OAL_PTR_NULL.}");
        return OAL_FAIL;
    }

    OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                  "{hmac_rx_proxysta_ip_mat::start into hmac_rx_proxysta_ip_mat.}");

    ul_contig_len = OAL_SIZEOF(mac_ether_header_stru);

    /* ��ȡ��̫��Ŀ��mac�����ݶ� */
    puc_des_mac = pst_ether_header->auc_ether_dhost;
    puc_eth_body = (oal_uint8 *)(pst_ether_header + 1);

    /* ��ȡ��̫��֡Ŀ�ĵ�ַ�Ƿ�Ϊ�ಥ��ַ */
    en_is_mcast = ETHER_IS_MULTICAST(puc_des_mac);

    /* *********************************************************************** */
    /*                      DHCP Frame Format                                */
    /* --------------------------------------------------------------------- */
    /* |��̫��ͷ        |   IPͷ         | UDPͷ           |DHCP����       | */
    /* --------------------------------------------------------------------- */
    /* | 14             |20              |8                | ����          | */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /* *********************************************************************** */
    pst_ip_header = (oal_ip_header_stru *)puc_eth_body;

    /* *********************************************************************** */
    /*                    IPͷ��ʽ (oal_ip_header_stru)                      */
    /* --------------------------------------------------------------------- */
    /* | �汾 | ��ͷ���� | �������� | �ܳ���  |��ʶ  |��־  |��ƫ����     |      */
    /* --------------------------------------------------------------------- */
    /* | 4bits|  4bits   | 1        | 2       | 2    |3bits | 13bits  |      */
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* | ������ | Э��        | ͷ��У���| Դ��ַ��SrcIp��|Ŀ�ĵ�ַ��DstIp�� */
    /* --------------------------------------------------------------------- */
    /* | 1      |  1 (17ΪUDP)| 2         | 4              | 4               */
    /* --------------------------------------------------------------------- */
    /* *********************************************************************** */
    ul_contig_len += OAL_SIZEOF(oal_ip_header_stru);
    if (ul_pkt_len < ul_contig_len) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA, "{hmac_rx_proxysta_ip_mat::The length of buf is \
        less than the sum of mac_ether_header_stru and oal_ip_header_stru.}");
        return OAL_FAIL;
    }

    us_ip_header_len = pst_ip_header->us_ihl * 4;

    /* �����UDP����������DHCPЭ��ı��ĵ�ַת�� */
    if (pst_ip_header->uc_protocol == OAL_IPPROTO_UDP) {
        pst_udp_header  = (oal_udp_header_stru *)((oal_uint8 *)pst_ip_header + us_ip_header_len);

        ul_contig_len += OAL_SIZEOF(oal_udp_header_stru);
        if (ul_pkt_len < ul_contig_len) {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                           "{hmac_rx_proxysta_ip_mat::The length of buf is invalid.}");
            return OAL_FAIL;
        }

        /* *********************************************************************** */
        /*                      UDP ͷ (oal_udp_header_stru)                     */
        /* --------------------------------------------------------------------- */
        /* |Դ�˿ںţ�SrcPort��|Ŀ�Ķ˿ںţ�DstPort��| UDP����    | UDP�����  | */
        /* --------------------------------------------------------------------- */
        /* | 2                 | 2                   |2           | 2          | */
        /* --------------------------------------------------------------------- */
        /*                                                                       */
        /* *********************************************************************** */
        /*lint -e778*/
        /*lint -e572*/
        /* DHCP request UDP Client SP = 68 (bootpc), DP = 67 (bootps) */
        if (OAL_NET2HOST_SHORT(67) == pst_udp_header->source) {
            /*lint +e778*/
            /*lint +e572*/
            /* *********************************************************************** */
            /*                    DHCP ���ĸ�ʽ (oal_dhcp_packet_stru)               */
            /* --------------------------------------------------------------------- */
            /* | op | htpe | hlen | hops  |xid(����IP)  |secs(����)   |flags(��־)|  */
            /* --------------------------------------------------------------------- */
            /* | 1  | 1    | 1    | 1     | 4           | 2           | 2          | */
            /* --------------------------------------------------------------------- */
            /* --------------------------------------------------------------------- */
            /* | ciaddr(4)�ͻ�ip��ַ | yiaddr��4�����IP��ַ |siaddr(4)������IP��ַ| */
            /* --------------------------------------------------------------------- */
            /* | giaddr��4���м̴���IP��ַ      | chaddr��16���ͻ���Ӳ����ַ(���滻)| */
            /* --------------------------------------------------------------------- */
            /* |sname��64����������������|file��128�������ļ���|option����������ѡ��| */
            /* --------------------------------------------------------------------- */
            /* *********************************************************************** */
            pst_dhcp_packet = (oal_dhcp_packet_stru *)(((oal_uint8 *)pst_udp_header) + OAL_SIZEOF(oal_udp_header_stru));

            ul_contig_len += OAL_SIZEOF(oal_dhcp_packet_stru);
            if (ul_pkt_len < ul_contig_len) {
                return OAL_FAIL;
            }

            if (en_is_mcast == OAL_FALSE) {
                /* ������ */
                if (oal_compare_mac_addr(pst_dhcp_packet->chaddr, mac_mib_get_StationID(pst_mac_vap)) == OAL_FALSE) {
                    /* ��UDP���еĵ�ַ�滻Ϊ�Լ��ĵ�ַ */
                    oal_set_mac_addr(pst_dhcp_packet->chaddr, pst_mac_vap->st_vap_proxysta.auc_oma);

                    /* ���¼���udp checksum */
                    pst_udp_header->check = hmac_rx_proxysta_update_checksum_addr(pst_udp_header->check,
                        mac_mib_get_StationID(pst_mac_vap), pst_mac_vap->st_vap_proxysta.auc_oma);

                    /* ���DHCP offer��ack��Ӧ֡�ǵ���֡��Ϊ�˷�ֹ����֡���ۺϽ���Ŀ�ĵ�ַת��Ϊ�㲥��ַ */
                    oal_set_mac_addr(puc_des_mac, BROADCAST_MACADDR);
                } else {
                    return OAL_SUCC;
                }
            } else {
                /* main sta�յ��㲥DHCP����֡��dhcp�����пͻ���mac��ַ�ͽ��ն˿ڵĵ�ַ��ͬ����������е�vap(��proxysta)��
                   �ҵ���dhcp�����еĿͻ���mac��ַ��ͬ��vap mac(��proxysta)��ַ�����ͻ���mac��ַ�滻Ϊproxysta��oma��ַ��
                   ������udp check */
                if (oal_compare_mac_addr(pst_dhcp_packet->chaddr, mac_mib_get_StationID(pst_mac_vap)) != OAL_FALSE) {
                    /* ����Device�µ�vap(STAģʽ)����Ŀ�ĵ�ַΪ��vap�Լ��ĵ�ַ��ͬʱ������oma��ַ�滻֡��Ŀ�ĵ�ַ */
                    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++) {
                        pst_mac_temp_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
                        if (OAL_UNLIKELY(pst_mac_temp_vap == OAL_PTR_NULL)) {
                            OAM_WARNING_LOG0(uc_vap_idx, OAM_SF_PROXYSTA,
                                             "{hmac_rx_proxysta_ip_mat::pst_mac_vap is null ptr!}");
                            return OAL_SUCC;
                        }

                        if (oal_compare_mac_addr(pst_dhcp_packet->chaddr,
                            mac_mib_get_StationID(pst_mac_temp_vap)) == OAL_FALSE) {
                            /* �滻mac��ַΪout vap��mac��ַ */
                            oal_set_mac_addr(pst_dhcp_packet->chaddr, pst_mac_temp_vap->st_vap_proxysta.auc_oma);

                            /* ���¼���udp checksum */
                            pst_udp_header->check = hmac_rx_proxysta_update_checksum_addr(pst_udp_header->check,
                                mac_mib_get_StationID(pst_mac_temp_vap), pst_mac_temp_vap->st_vap_proxysta.auc_oma);
                            break;
                        }
                    }
                }
            }
        } else {
            /* ����DHCP������UDP����Ҳ��Ҫ������̫��Ŀ�ĵ�ַΪproxysta��vma */
            oal_set_mac_addr(puc_des_mac, pst_mac_vap->st_vap_proxysta.auc_oma);
        }
    } else {
        /* ��������IP���ĵĵ�ַת������ */
        /* ������̫��Ŀ�ĵ�ַΪproxysta��vma */
        oal_set_mac_addr(puc_des_mac, pst_mac_vap->st_vap_proxysta.auc_oma);
    }

    return OAL_SUCC;
}


oal_uint32 hmac_rx_proxysta_mat(oal_netbuf_stru *pst_buf, oal_net_device_stru *pst_dev)
{
    mac_vap_stru          *pst_mac_vap;
    mac_ether_header_stru *pst_ether_header;
    oal_uint16            us_ether_type;
    oal_uint8             *puc_scr_mac;
    oal_uint8             *puc_des_mac;
    oal_bool_enum_uint8   en_is_mcast;
    oal_uint32            ul_contig_len = OAL_SIZEOF(mac_ether_header_stru);
    oal_uint32            ul_pkt_len;
    oal_uint8             uc_vap_idx;
    mac_device_stru       *pst_mac_device;
    mac_vap_stru          *pst_mac_temp_vap;
    oal_uint32            ul_ret;

    ul_pkt_len = OAL_NETBUF_LEN(pst_buf) + ETHER_HDR_LEN;

    /* ��ȡVAP�ṹ�� */
    pst_mac_vap = (mac_vap_stru *)OAL_NET_DEV_PRIV(pst_dev);
    /* �������sta������Ҫ��ַת�� */
    if (pst_mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_STA) {
        return OAL_SUCC;
    }

    /* ��ȡdeviceָ��ṹ�� */
    pst_mac_device = (mac_device_stru *)mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_PROXYSTA, "{hmac_rx_proxysta_mat::get pst_mac_device is a null ptr!}");
        return OAL_FAIL ;
    }

    if (ul_pkt_len < ul_contig_len) {
        return OAL_FAIL;
    }

    /* ��skb��dataָ��ָ����̫����֡ͷ */
    oal_netbuf_push(pst_buf, ETHER_HDR_LEN);

    pst_ether_header = (mac_ether_header_stru *)OAL_NETBUF_HEADER(pst_buf);

    /* ��ԭskb��dataָ�� */
    oal_netbuf_pull(pst_buf, ETHER_HDR_LEN);

    us_ether_type = pst_ether_header->us_ether_type;
    puc_scr_mac   = pst_ether_header->auc_ether_shost;
    puc_des_mac   = pst_ether_header->auc_ether_dhost;

    en_is_mcast   = ETHER_IS_MULTICAST(puc_des_mac);

    if (OAL_HOST2NET_SHORT(ETHER_TYPE_PAE) == us_ether_type) {
        /* ��ע���������� �����Ź��Ӻ����д��� */
        pst_buf->mark   = OAL_PROXYSTA_MARK_ROUTE;
        return OAL_SUCC;
    }

    /* main sta��proxysta��root ap���͹㲥����root ap�Ὣ�յ��Ĺ㲥������������������û�ת���˰���
       ��ˣ�main sta���յ�root ap�����͵Ĺ㲥�������յ���Щ�㲥����main sta���ж��Ƿ�Ϊ��device�ϵ�
       sta���͵Ĺ㲥��������ǽ�����Ϊ�����������ź��䶪�����������κδ��� */
    if ((en_is_mcast == OAL_TRUE) && (pst_mac_vap->st_vap_proxysta.en_is_proxysta == OAL_TRUE)) {
        /* ����device�µ�vap,����ǣ��Ա����Ŵ��� */
        for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++) {
            pst_mac_temp_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
            if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL)) {
                OAM_WARNING_LOG0(uc_vap_idx, OAM_SF_PROXYSTA, "{hmac_rx_proxysta_mat::get pst_mac_vap is null ptr!}");
                return OAL_FAIL;
            }

            if (oal_compare_mac_addr(puc_scr_mac, mac_mib_get_StationID(pst_mac_temp_vap)) == OAL_FALSE) {
                pst_buf->mark = OAL_PROXYSTA_MARK_DROPPED;
                return OAL_SUCC;
            }
        }
    }

    /* ARP ����ַת�� */
    if (OAL_HOST2NET_SHORT(ETHER_TYPE_ARP) == us_ether_type) {
        ul_ret = hmac_rx_proxysta_arp_mat(pst_mac_vap,
                                          pst_ether_header,
                                          pst_mac_device,
                                          ul_pkt_len);
        if (ul_ret != OAL_SUCC) {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                           "{hmac_rx_proxysta_mat::hmac_rx_proxysta_arp_mat returns %d.}", ul_ret);
        }
        return ul_ret;
    }

    /*lint -e778*/
    /* IP����ַת�� */
    if (OAL_HOST2NET_SHORT(ETHER_TYPE_IP) == us_ether_type) {
        /*lint +e778*/
        ul_ret = hmac_rx_proxysta_ip_mat(pst_mac_vap,
                                         pst_ether_header,
                                         pst_mac_device,
                                         ul_pkt_len);
        if (ul_ret != OAL_SUCC) {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                           "{hmac_rx_proxysta_mat::hmac_rx_proxysta_ip_mat returns %d.}", ul_ret);
        }
        return ul_ret;
    }

    /* icmpv6 ����ַת�� */
    if (OAL_HOST2NET_SHORT(ETHER_TYPE_IPV6) == us_ether_type) {
    }

    oal_set_mac_addr(puc_des_mac, pst_mac_vap->st_vap_proxysta.auc_oma);

    return OAL_SUCC;
}

#endif
#ifdef _PRE_WLAN_FEATURE_PKT_MEM_OPT
OAL_STATIC void hmac_pkt_mem_opt_stat_reset(hmac_device_stru *pst_hmac_device, oal_bool_enum_uint8 en_dscr_opt_state)
{
    frw_event_mem_stru    *pst_event_mem;
    frw_event_stru        *pst_event;
    hmac_rx_dscr_opt_stru *pst_dscr_opt = &pst_hmac_device->st_rx_dscr_opt;

    if (OAL_UNLIKELY(pst_hmac_device->pst_device_base_info == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_pkt_mem_opt_stat_reset::pst_device_base_info null!}");
        return;
    }

    OAM_WARNING_LOG2(0, OAM_SF_ANY, "{hmac_rx_dscr_opt_stat_reset::new_state[%d], pkt_num[%d]}", en_dscr_opt_state,
        pst_dscr_opt->ul_rx_pkt_num);
    pst_dscr_opt->en_dscr_opt_state = en_dscr_opt_state;
    pst_dscr_opt->ul_rx_pkt_num     = 0;

    // ���¼���dmacģ��,��ͳ����Ϣ����dmac
    pst_event_mem = FRW_EVENT_ALLOC(0);
    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_rx_dscr_opt_timeout_fn::pst_event_mem null.}");
        return;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    /* ��д�¼�ͷ */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_DSCR_OPT,
                       0,
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_hmac_device->pst_device_base_info->uc_chip_id,
                       pst_hmac_device->pst_device_base_info->uc_device_id,
                       0);

    /* �������� */
    pst_event->auc_event_data[0] = pst_dscr_opt->en_dscr_opt_state;

    /* �ַ��¼� */
    frw_event_dispatch_event(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);
}


oal_void hmac_pkt_mem_opt_cfg(oal_uint32 ul_cfg_tpye, oal_uint32 ul_cfg_value)
{
    hmac_device_stru      *pst_hmac_device = (hmac_device_stru*)hmac_res_get_mac_dev(0);
    hmac_rx_dscr_opt_stru *pst_dscr_opt;

    if (ul_cfg_tpye > 2) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_rx_dscr_opt_cfg::invalid cfg tpye.}");
        return;
    }
    if (pst_hmac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_rx_dscr_opt_cfg::hmac device is null.}");
        return;
    }

    OAM_WARNING_LOG2(0, OAM_SF_ANY, "{hmac_rx_dscr_opt_cfg::cfg type[%d], cfg value[%d].}", ul_cfg_tpye, ul_cfg_value);
    pst_dscr_opt = &pst_hmac_device->st_rx_dscr_opt;
    if (ul_cfg_tpye == 0) {
        pst_dscr_opt->en_dscr_opt_enable = (oal_uint8)ul_cfg_value;
        if (pst_dscr_opt->en_dscr_opt_enable == OAL_FALSE && pst_dscr_opt->en_dscr_opt_state == OAL_TRUE) {
            hmac_pkt_mem_opt_stat_reset(pst_hmac_device, OAL_FALSE);
        }
    }
    if (ul_cfg_tpye == 1) {
        pst_dscr_opt->ul_rx_pkt_opt_limit = ul_cfg_value;
    }
    if (ul_cfg_tpye == 2) {
        pst_dscr_opt->ul_rx_pkt_reset_limit = ul_cfg_value;
    }
}

oal_uint32  hmac_pkt_mem_opt_timeout_fn(oal_void *p_arg)
{
    hmac_device_stru      *pst_hmac_device;
    hmac_rx_dscr_opt_stru *pst_dscr_opt;

    if (OAL_UNLIKELY(p_arg == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_rx_dscr_opt_timeout_fn::p_arg is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_device = (hmac_device_stru *)p_arg;
    pst_dscr_opt    = &pst_hmac_device->st_rx_dscr_opt;

    if (pst_dscr_opt->en_dscr_opt_enable != OAL_TRUE) {
        return OAL_SUCC;
    }

    OAM_INFO_LOG2(0, OAM_SF_ANY, "{hmac_rx_dscr_opt_timeout_fn::state[%d], pkt_num[%d]}",
        pst_dscr_opt->en_dscr_opt_state, pst_dscr_opt->ul_rx_pkt_num);

    /* rx_dscrδ����״̬ʱ, ��⵽RXҵ��,���������� */
    if (pst_dscr_opt->en_dscr_opt_state == OAL_FALSE &&
        pst_dscr_opt->ul_rx_pkt_num > pst_dscr_opt->ul_rx_pkt_opt_limit) {
        hmac_pkt_mem_opt_stat_reset(pst_hmac_device, OAL_TRUE);
        /* rx_dscr�ѵ���״̬ʱ, δ��⵽RXҵ��,������������,��֤TX���� */
    } else if(pst_dscr_opt->en_dscr_opt_state == OAL_TRUE &&
        pst_dscr_opt->ul_rx_pkt_num < pst_dscr_opt->ul_rx_pkt_reset_limit) {
        hmac_pkt_mem_opt_stat_reset(pst_hmac_device, OAL_FALSE);
    } else {
        pst_dscr_opt->ul_rx_pkt_num  = 0;
    }

    return OAL_SUCC;
}

oal_void hmac_pkt_mem_opt_init(hmac_device_stru *pst_hmac_device)
{
    pst_hmac_device->st_rx_dscr_opt.en_dscr_opt_state     = OAL_FALSE;
    pst_hmac_device->st_rx_dscr_opt.ul_rx_pkt_num         = 0;
    pst_hmac_device->st_rx_dscr_opt.ul_rx_pkt_opt_limit   = WLAN_PKT_MEM_PKT_OPT_LIMIT;
    pst_hmac_device->st_rx_dscr_opt.ul_rx_pkt_reset_limit = WLAN_PKT_MEM_PKT_RESET_LIMIT;
    /* ������Ч����ʱ�ر� */
    pst_hmac_device->st_rx_dscr_opt.en_dscr_opt_enable    = OAL_FALSE;

    FRW_TIMER_CREATE_TIMER(&(pst_hmac_device->st_rx_dscr_opt.st_rx_dscr_opt_timer),
        hmac_pkt_mem_opt_timeout_fn, WLAN_PKT_MEM_OPT_TIME_MS, pst_hmac_device, OAL_TRUE, OAM_MODULE_ID_HMAC, 0);
}

oal_void hmac_pkt_mem_opt_exit(hmac_device_stru *pst_hmac_device)
{
    if (pst_hmac_device->st_rx_dscr_opt.st_rx_dscr_opt_timer.en_is_registerd == OAL_TRUE) {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_device->st_rx_dscr_opt.st_rx_dscr_opt_timer));
    }
}


OAL_STATIC oal_void  hmac_pkt_mem_opt_rx_pkts_stat(hmac_vap_stru *pst_vap, oal_ip_header_stru *pst_ip)
{
    hmac_device_stru *pst_hmac_device = (hmac_device_stru*)hmac_res_get_mac_dev(pst_vap->st_vap_base_info.uc_device_id);

    if (pst_hmac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_pkt_mem_opt_rx_pkts_stat::hmac_res_get_mac_dev fail.device_id :%d}",
            pst_vap->st_vap_base_info.uc_device_id);
        return;
    }
    /* ����IP_LEN С�� HMAC_RX_DSCR_OPT_MIN_PKT_LEN�ı��� */
    if (OAL_NET2HOST_SHORT(pst_ip->us_tot_len) < WLAN_PKT_MEM_OPT_MIN_PKT_LEN) {
        return;
    }

    if ((pst_ip->uc_protocol == MAC_UDP_PROTOCAL) || (pst_ip->uc_protocol == MAC_TCP_PROTOCAL)) {
        pst_hmac_device->st_rx_dscr_opt.ul_rx_pkt_num++;
    } else {
        OAM_INFO_LOG0(0, OAM_SF_RX, "{hmac_rx_dscr_opt_rx_pkts_stat: neither UDP nor TCP ");
    }
}
#endif

OAL_STATIC oal_void hmac_rx_transmit_msdu_to_lan(hmac_vap_stru *pst_vap, dmac_msdu_stru *pst_msdu)
{
    oal_net_device_stru   *pst_device    = OAL_PTR_NULL;
    oal_netbuf_stru       *pst_netbuf    = OAL_PTR_NULL;
    mac_ether_header_stru *pst_ether_hdr = OAL_PTR_NULL;
#ifdef _PRE_WLAN_FEATURE_MCAST
    hmac_user_stru        *pst_hmac_user = OAL_PTR_NULL;
    oal_uint16            us_user_idx = 0xffff;
#endif

#if defined(_PRE_WLAN_FEATURE_WPA) || defined(_PRE_WLAN_FEATURE_WPA2)
    oal_uint8             *puc_mac_addr   = OAL_PTR_NULL;
#endif
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    oal_uint32            ul_proxymat_ret;
    mac_device_stru       *pst_mac_device = OAL_PTR_NULL;
    mac_vap_stru          *pst_mac_vap_proxysta = &(pst_vap->st_vap_base_info);
#endif

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
    mac_vap_stru          *pst_mac_vap       = &(pst_vap->st_vap_base_info);
    hmac_user_stru        *pst_hmac_user_st  = OAL_PTR_NULL;
    mac_ip_header_stru    *pst_ip            = OAL_PTR_NULL;
    oal_uint16            us_assoc_id       = 0xffff;
#endif

    /* ��ȡnetbuf����netbuf��dataָ���Ѿ�ָ��payload�� */
    pst_netbuf = pst_msdu->pst_netbuf;

    OAL_NETBUF_PREV(pst_netbuf) = OAL_PTR_NULL;
    OAL_NETBUF_NEXT(pst_netbuf) = OAL_PTR_NULL;

    if (hmac_rx_frame_80211_to_eth(pst_netbuf, pst_msdu->auc_da, pst_msdu->auc_sa) != OAL_SUCC) {
        OAM_WARNING_LOG0(0, OAM_SF_RX, "hmac_rx_transmit_msdu_to_lan: frame len is error.");
        oal_netbuf_free(pst_netbuf);
        return;
    }

    pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_netbuf);
    if (OAL_UNLIKELY(pst_ether_hdr == OAL_PTR_NULL)) {
        oal_netbuf_free(pst_netbuf);
        OAM_ERROR_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
            "{hmac_rx_transmit_msdu_to_lan::pst_ether_hdr null.}");
        return;
    }

#if defined(_PRE_WLAN_FEATURE_WPA) || defined(_PRE_WLAN_FEATURE_WPA2)
    puc_mac_addr = pst_msdu->auc_ta;

    if (hmac_11i_ether_type_filter(pst_vap, puc_mac_addr, pst_ether_hdr->us_ether_type) != OAL_SUCC) {
        /* ���հ�ȫ���ݹ��� */
        oam_report_eth_frame(puc_mac_addr, (oal_uint8*)pst_ether_hdr, (oal_uint16)OAL_NETBUF_LEN(pst_netbuf),
            OAM_OTA_FRAME_DIRECTION_TYPE_RX);

        oal_netbuf_free(pst_netbuf);
        OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, rx_portvalid_check_fail_dropped, 1);
        return;
    }
#endif

#ifdef _PRE_WLAN_FEATURE_MCAST
    if (pst_vap->pst_m2u != OAL_PTR_NULL) {
        if (pst_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
            if (mac_vap_find_user_by_macaddr(&(pst_vap->st_vap_base_info), pst_ether_hdr->auc_ether_shost,
                &us_user_idx) != OAL_SUCC) {
                OAM_ERROR_LOG4(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U,
                    "{hmac_rx_transmit_msdu_to_lan::mac_vap_find_user_by_macaddr [%x].[%x].[%x].[%x]failed}",
                    (oal_uint32)(pst_ether_hdr->auc_ether_shost[2]), (oal_uint32)(pst_ether_hdr->auc_ether_shost[3]),
                    (oal_uint32)(pst_ether_hdr->auc_ether_shost[4]), (oal_uint32)(pst_ether_hdr->auc_ether_shost[5]));
                oal_netbuf_free(pst_netbuf);
                return;
            }
        } else if (pst_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
            us_user_idx = pst_vap->st_vap_base_info.uc_assoc_vap_id;
        }
        pst_hmac_user = mac_res_get_hmac_user(us_user_idx);
        hmac_m2u_snoop_inspecting(pst_vap, pst_hmac_user, pst_netbuf);
    }
#endif

    /* ��ȡnet device hmac������ʱ����Ҫ��¼netdeviceָ�� */
    pst_device = pst_vap->pst_net_device;

    /* ��protocolģʽ��ֵ */
    /* �˴�linux�и��Ӳ�����liteos���˼򻯣�protocolΪ0����pull 14�ֽڣ�������õ�����󽻸�ϵͳ֮ǰpush��ȥ�� */
    OAL_NETBUF_PROTOCOL(pst_netbuf) = oal_eth_type_trans(pst_netbuf, pst_device);

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    pst_mac_device = mac_res_get_dev(pst_vap->st_vap_base_info.uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        oal_netbuf_free(pst_netbuf);
        OAM_ERROR_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                       "{hmac_rx_transmit_msdu_to_lan::mac_res_get_dev is null!}");
        return;
    }

    if (pst_mac_device->st_cap_flag.bit_proxysta == OAL_TRUE) {
        if (pst_mac_vap_proxysta->st_vap_proxysta.en_is_proxysta == OAL_TRUE) {
            /* ���հ���ַת�� */
            ul_proxymat_ret = hmac_rx_proxysta_mat(pst_netbuf, pst_device);
            if (ul_proxymat_ret != OAL_SUCC) {
                oal_netbuf_free(pst_netbuf);
                OAM_ERROR_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                    "{hmac_rx_transmit_msdu_to_lan::hmac_rx_proxysta_mat fail, return value is %d.}", ul_proxymat_ret);
                return;
            }
        }
    }
#endif

#if defined(_PRE_WLAN_FEATURE_BTCOEX) || defined(_PRE_WLAN_FEATURE_1131K_BTCOEX)
    if (ETHER_IS_MULTICAST(pst_msdu->auc_da) == OAL_FALSE) {
        oal_atomic_inc(&(pst_vap->hmac_vap_btcoex.hmac_btcoex_arp_req_process.rx_unicast_pkt_to_lan));
    }
#endif

    /* ��Ϣͳ����֡�ϱ����� */
    /* ����ͳ����Ϣ */
    HMAC_VAP_DFT_STATS_PKT_INCR(pst_vap->st_query_stats.ul_rx_pkt_to_lan, 1);
    HMAC_VAP_DFT_STATS_PKT_INCR(pst_vap->st_query_stats.ul_rx_bytes_to_lan, OAL_NETBUF_LEN(pst_netbuf));
    OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, rx_pkt_to_lan, 1); /* ���ӷ���LAN��֡����Ŀ */
    /* ���ӷ���LAN���ֽ��� */
    OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, rx_bytes_to_lan, OAL_NETBUF_LEN(pst_netbuf));

#ifdef _PRE_WLAN_DFT_DUMP_FRAME
    hmac_rx_report_eth_frame(&pst_vap->st_vap_base_info, pst_netbuf);
#endif

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
    if ((pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) && (pst_vap->uc_edca_opt_flag_ap == OAL_TRUE)) {
        /*lint -e778*/
        if (OAL_HOST2NET_SHORT(ETHER_TYPE_IP) == pst_ether_hdr->us_ether_type) {
            if (mac_vap_find_user_by_macaddr(pst_mac_vap, pst_ether_hdr->auc_ether_shost, &us_assoc_id) != OAL_SUCC) {
                OAM_WARNING_LOG4(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U,
                    "{hmac_rx_transmit_msdu_to_lan::find_user_by_macaddr[%02x:XX:XX:%02x:%02x:%02x]failed}",
                    (oal_uint32)(pst_ether_hdr->auc_ether_shost[0]), (oal_uint32)(pst_ether_hdr->auc_ether_shost[3]),
                    (oal_uint32)(pst_ether_hdr->auc_ether_shost[4]), (oal_uint32)(pst_ether_hdr->auc_ether_shost[5]));
                oal_netbuf_free(pst_netbuf);
                return;
            }
            pst_hmac_user_st = (hmac_user_stru *)mac_res_get_hmac_user(us_assoc_id);
            if (pst_hmac_user_st == OAL_PTR_NULL) {
                oal_netbuf_free(pst_netbuf);
                OAM_ERROR_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                    "{hmac_rx_transmit_msdu_to_lan::mac_res_get_hmac_user fail. assoc_id: %u}", us_assoc_id);
                return;
            }

            pst_ip = (mac_ip_header_stru *)(pst_ether_hdr + 1);

            /* mips�Ż�:�������ҵ��ͳ�����ܲ�10M���� */
            if (((pst_ip->uc_protocol == MAC_UDP_PROTOCAL) &&
                (pst_hmac_user_st->aaul_txrx_data_stat[WLAN_WME_AC_BE][WLAN_RX_UDP_DATA] <
                (HMAC_EDCA_OPT_PKT_NUM + 10))) || ((pst_ip->uc_protocol == MAC_TCP_PROTOCAL) &&
                (pst_hmac_user_st->aaul_txrx_data_stat[WLAN_WME_AC_BE][WLAN_RX_TCP_DATA] <
                (HMAC_EDCA_OPT_PKT_NUM + 10)))) {
                hmac_edca_opt_rx_pkts_stat(us_assoc_id, WLAN_TIDNO_BEST_EFFORT, pst_ip);
            }
        }
        /*lint +e778*/
    }
#endif

    OAL_MEM_NETBUF_TRACE(pst_netbuf, OAL_TRUE);
    memset_s(OAL_NETBUF_CB(pst_netbuf), OAL_NETBUF_CB_SIZE(), 0, OAL_NETBUF_CB_SIZE());

#ifdef _PRE_WLAN_FEATURE_PKT_MEM_OPT
    hmac_pkt_mem_opt_rx_pkts_stat(pst_vap, (oal_ip_header_stru*)(pst_ether_hdr + 1));
#endif
    OAL_MIPS_RX_STATISTIC(HMAC_PROFILING_FUNC_RX_NETBUF_FOR_KERNEL);

    /* ��skbת������ */
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    pst_netbuf->dev = pst_device;

    /* ��skb��dataָ��ָ����̫����֡ͷ */
    // ����ǰ��pull��14�ֽڣ�����ط�Ҫpush��ȥ��
    oal_netbuf_push(pst_netbuf, ETHER_HDR_LEN);
#endif /* */

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if (hmac_get_rxthread_enable() == OAL_TRUE) {
        hmac_rxdata_netbuf_enqueue(pst_netbuf);

        hmac_rxdata_sched();
    } else
#endif
    {
        oal_netif_rx_ni(pst_netbuf);
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,11,0))
    /* 4.11�����ں˰汾net_device�ṹ����û��last_rx�ֶ� */
#else
    /* ��λnet_dev->jiffies���� */
    OAL_NETDEVICE_LAST_RX(pst_device) = OAL_TIME_JIFFY;
#endif
}


oal_void hmac_rx_lan_frame_classify(hmac_vap_stru *pst_vap, oal_netbuf_stru *pst_netbuf,
    mac_ieee80211_frame_stru *pst_frame_hdr)
{
    hmac_rx_ctl_stru                *pst_rx_ctrl     = OAL_PTR_NULL;    /* ָ��MPDU���ƿ���Ϣ��ָ�� */
    dmac_msdu_stru                  st_msdu;                            /* �������������ÿһ��MSDU */
    mac_msdu_proc_status_enum_uint8 en_process_state;   /* ����AMSDU��״̬ */
    dmac_msdu_proc_state_stru       st_msdu_state    = { 0 };             /* ��¼MPDU�Ĵ�����Ϣ */
    oal_uint8                       *puc_addr        = OAL_PTR_NULL;
    oal_uint32                      ul_ret;
    hmac_user_stru                  *pst_hmac_user   = OAL_PTR_NULL;
    oal_uint8                       uc_datatype;
#ifdef _PRE_WLAN_FEATURE_WAPI
    hmac_wapi_stru                  *pst_wapi        = OAL_PTR_NULL;
#endif
    if (OAL_UNLIKELY((pst_vap == OAL_PTR_NULL) || (pst_netbuf == OAL_PTR_NULL) || (pst_frame_hdr == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_rx_lan_frame_classify::params null.}");
        return;
    }
    /* ��ȡ��MPDU�Ŀ�����Ϣ */
    pst_rx_ctrl = (hmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

    memset_s(&st_msdu, OAL_SIZEOF(dmac_msdu_stru), 0, OAL_SIZEOF(dmac_msdu_stru));

    mac_get_transmit_addr(pst_frame_hdr, &puc_addr);

    oal_set_mac_addr(st_msdu.auc_ta, puc_addr);

    pst_hmac_user = (hmac_user_stru*)mac_res_get_hmac_user(pst_rx_ctrl->st_rx_info.us_ta_user_idx);
    if (OAL_UNLIKELY(pst_hmac_user == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                       "{hmac_rx_lan_frame_classify::pst_hmac_user null, user_idx=%d.}",
                       pst_rx_ctrl->st_rx_info.us_ta_user_idx);

        /* ��ӡ��net buf�����Ϣ */
        OAM_ERROR_LOG4(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
            "{hmac_rx_lan_frame_classify::info in cb, vap id=%d mac_hdr_len=%d, us_frame_len=%d \
            mac_hdr_start_addr=0x%08p.}", pst_rx_ctrl->st_rx_info.bit_vap_id, pst_rx_ctrl->st_rx_info.uc_mac_header_len,
            pst_rx_ctrl->st_rx_info.us_frame_len, (uintptr_t)pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr);
        OAM_ERROR_LOG2(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                       "{hmac_rx_lan_frame_classify::net_buf ptr addr=%p, cb ptr addr=%p.}",
                       (uintptr_t)pst_netbuf, (uintptr_t)pst_rx_ctrl);
#ifdef _PRE_WLAN_DFT_DUMP_FRAME
        mac_rx_report_80211_frame((oal_uint8 *)&(pst_vap->st_vap_base_info),
                                  (oal_uint8 *)&(pst_rx_ctrl->st_rx_info),
                                  pst_netbuf,
                                  OAM_OTA_TYPE_RX_HMAC_CB);
#endif

        return;
    }

    hmac_ba_update_rx_bitmap(pst_hmac_user, pst_frame_hdr);

    /* ���һ:����AMSDU�ۺϣ����MPDU��Ӧһ��MSDU��ͬʱ��Ӧһ��NETBUF */
    if (pst_rx_ctrl->st_rx_info.bit_amsdu_enable == OAL_FALSE) {
#ifdef _PRE_WLAN_FEATURE_WAPI
        /*lint -e730*/
        pst_wapi = hmac_user_get_wapi_ptr(&pst_vap->st_vap_base_info,
                                          !ETHER_IS_MULTICAST(pst_frame_hdr->auc_address1),
                                          pst_hmac_user->st_user_base_info.us_assoc_id);
        /*lint +e730*/
        if (pst_wapi == OAL_PTR_NULL) {
            OAM_WARNING_LOG0(0, OAM_SF_WPA, "{hmac_rx_lan_frame_classify:: get pst_wapi Err!.}");
            HMAC_USER_STATS_PKT_INCR(pst_hmac_user->ul_rx_pkt_drop, 1);
            return ;
        }

        if ((WAPI_IS_PORT_VALID(pst_wapi) == OAL_TRUE)
                && (pst_wapi->wapi_netbuff_rxhandle != OAL_PTR_NULL)) {
            pst_netbuf = pst_wapi->wapi_netbuff_rxhandle(pst_wapi, pst_netbuf);
            if (pst_netbuf == OAL_PTR_NULL) {
                OAM_WARNING_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                    "{hmac_rx_lan_frame_classify:: wapi decrypt FAIL!}");
                HMAC_USER_STATS_PKT_INCR(pst_hmac_user->ul_rx_pkt_drop, 1);
                return ;
            }

            /* ���»�ȡ��MPDU�Ŀ�����Ϣ */
            pst_rx_ctrl = (hmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
        }
#endif /* #ifdef _PRE_WLAN_FEATURE_WAPI */

        pst_netbuf = hmac_defrag_process(pst_hmac_user, pst_netbuf, pst_rx_ctrl->st_rx_info.uc_mac_header_len);
        if (pst_netbuf == OAL_PTR_NULL) {
            return;
        }

        /* ���»�ȡ��MPDU�Ŀ�����Ϣ */
        pst_rx_ctrl = (hmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
        pst_frame_hdr = (mac_ieee80211_frame_stru *)pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr;

        /* ��ӡ���ؼ�֡(dhcp)��Ϣ */
        uc_datatype = mac_get_data_type_from_80211(pst_netbuf, pst_rx_ctrl->st_rx_info.uc_mac_header_len);
        if ((uc_datatype <= MAC_DATA_VIP) && (uc_datatype != MAC_DATA_ARP_REQ)) {
            OAM_WARNING_LOG4(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                "{hmac_rx_lan_frame_classify::user[%d], datatype==%u, len==%u, rx_drop_cnt==%u}\
                [0:dhcp 1:arp_req 2:arp_rsp 3:eapol]", pst_rx_ctrl->st_rx_info.us_ta_user_idx, uc_datatype,
                pst_rx_ctrl->st_rx_info.us_frame_len, pst_hmac_user->ul_rx_pkt_drop);
        }

        /* �Ե�ǰ��msdu���и�ֵ */
        st_msdu.pst_netbuf    = pst_netbuf;

        /* ��netbuf��dataָ��ָ��mac frame��payload�� */
        oal_netbuf_pull(pst_netbuf, pst_rx_ctrl->st_rx_info.uc_mac_header_len);

        /* ��ȡԴ��ַ��Ŀ�ĵ�ַ */
        mac_rx_get_sa(pst_frame_hdr, &puc_addr);
        oal_set_mac_addr(st_msdu.auc_sa, puc_addr);

        mac_rx_get_da(pst_frame_hdr, &puc_addr);
        oal_set_mac_addr(st_msdu.auc_da, puc_addr);

        OAL_MIPS_RX_STATISTIC(HMAC_PROFILING_FUNC_RX_PREPARE_MSDU_INFO);

        /* ��MSDUת����LAN */
        hmac_rx_transmit_msdu_to_lan(pst_vap, &st_msdu);
    } else { /* �����:AMSDU�ۺ� */
        st_msdu_state.uc_procd_netbuf_nums    = 0;
        st_msdu_state.uc_procd_msdu_in_netbuf = 0;
        /* amsdu ���һ��netbuf nextָ����Ϊ NULL ����ʱ�����ͷ�amsdu netbuf */
        hmac_rx_clear_amsdu_last_netbuf_pointer(pst_netbuf, pst_rx_ctrl->st_rx_info.bit_buff_nums);

        do {
            /* ��ȡ��һ��Ҫת����msdu */
            ul_ret = hmac_rx_parse_amsdu(pst_netbuf, &st_msdu, &st_msdu_state, &en_process_state);
            if (ul_ret != OAL_SUCC) {
                OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                                 "{hmac_rx_lan_frame_classify::hmac_rx_parse_amsdu failed[%d].}", ul_ret);
                return;
            }

            OAL_MIPS_RX_STATISTIC(HMAC_PROFILING_FUNC_RX_PREPARE_MSDU_INFO);

            /* ��ÿһ��MSDUת����LAN */
            hmac_rx_transmit_msdu_to_lan(pst_vap, &st_msdu);
        } while (en_process_state != MAC_PROC_LAST_MSDU);
    }
}


oal_uint32 hmac_rx_copy_netbuff(oal_netbuf_stru  **ppst_dest_netbuf, oal_netbuf_stru  *pst_src_netbuf,
    oal_uint8 uc_vap_id, mac_ieee80211_frame_stru **ppul_mac_hdr_start_addr)
{
    hmac_rx_ctl_stru  *pst_rx_ctrl = OAL_PTR_NULL;
    oal_int32          l_ret       = EOK;

    if (pst_src_netbuf == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    *ppst_dest_netbuf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if (OAL_UNLIKELY(*ppst_dest_netbuf == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(uc_vap_id, OAM_SF_RX, "{hmac_rx_copy_netbuff::pst_netbuf_copy null.}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /* ��Ϣ���� */
    l_ret += memcpy_s(oal_netbuf_cb(*ppst_dest_netbuf), OAL_SIZEOF(hmac_rx_ctl_stru),
                      oal_netbuf_cb(pst_src_netbuf), OAL_SIZEOF(hmac_rx_ctl_stru)); // modify src bug
    l_ret += memcpy_s(oal_netbuf_data(*ppst_dest_netbuf), OAL_NETBUF_LEN(pst_src_netbuf),
                      oal_netbuf_data(pst_src_netbuf), OAL_NETBUF_LEN(pst_src_netbuf));
    if (l_ret != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "hmac_rx_copy_netbuff::memcpy_s fail!");
        oal_netbuf_free(*ppst_dest_netbuf);
        return OAL_FAIL;
    }
    /* ����netbuf���ȡ�TAILָ�� */
    oal_netbuf_put(*ppst_dest_netbuf, oal_netbuf_get_len(pst_src_netbuf));

    /* ����MAC֡ͷ��ָ��copy�󣬶�Ӧ��mac header��ͷ�Ѿ������仯) */
    pst_rx_ctrl = (hmac_rx_ctl_stru *)oal_netbuf_cb(*ppst_dest_netbuf);
    pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr = (oal_uint32 *)oal_netbuf_data(*ppst_dest_netbuf);
    *ppul_mac_hdr_start_addr = (mac_ieee80211_frame_stru *)oal_netbuf_data(*ppst_dest_netbuf);

    return OAL_SUCC;
}


oal_void  hmac_rx_process_data_filter(oal_netbuf_head_stru *pst_netbuf_header, oal_netbuf_stru *pst_temp_netbuf,
    oal_uint16 us_netbuf_num)
{
    oal_netbuf_stru          *pst_netbuf     = OAL_PTR_NULL;
    hmac_rx_ctl_stru         *pst_rx_ctrl    = OAL_PTR_NULL;
#ifdef _PRE_WLAN_FEATURE_AMPDU
    hmac_user_stru           *pst_hmac_user  = OAL_PTR_NULL;
    mac_ieee80211_frame_stru *pst_frame_hdr  = OAL_PTR_NULL;
#endif
    oal_uint8                uc_buf_nums;
    mac_vap_stru             *pst_vap        = OAL_PTR_NULL;
    oal_uint32               ul_ret = OAL_SUCC;
    oal_bool_enum_uint8      en_is_ba_buf;
    oal_uint8                uc_netbuf_num;

    while (us_netbuf_num != 0) {
        en_is_ba_buf = OAL_FALSE;
        pst_netbuf  = pst_temp_netbuf;
        if (pst_netbuf == OAL_PTR_NULL) {
            OAM_WARNING_LOG1(0, OAM_SF_RX, "{hmac_rx_process_data_filter::us_netbuf_num = %d}", us_netbuf_num);
            break;
        }

        pst_rx_ctrl   = (hmac_rx_ctl_stru*)oal_netbuf_cb(pst_netbuf);
#ifdef _PRE_WLAN_FEATURE_AMPDU
        // make sure ta user idx is exist
        pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(MAC_GET_RX_CB_TA_USER_IDX(&(pst_rx_ctrl->st_rx_info)));
        pst_frame_hdr = (mac_ieee80211_frame_stru *)pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr;
#endif
        uc_buf_nums   = pst_rx_ctrl->st_rx_info.bit_buff_nums;

        /* ��ȡ��һ��Ҫ�����MPDU */
        oal_netbuf_get_appointed_netbuf(pst_netbuf, uc_buf_nums, &pst_temp_netbuf);
        us_netbuf_num = OAL_SUB(us_netbuf_num, uc_buf_nums);

        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_rx_ctrl->st_rx_info.uc_mac_vap_id);
        if (OAL_UNLIKELY(pst_vap == OAL_PTR_NULL)) {
            hmac_rx_free_netbuf_list(pst_netbuf_header, uc_buf_nums);
            OAM_WARNING_LOG0(pst_rx_ctrl->st_rx_info.bit_vap_id, OAM_SF_RX,
                "{hmac_rx_process_data_filter::pst_vap null.}");
            continue;
        }

        if (pst_vap->uc_vap_id == 0 || pst_vap->uc_vap_id  > WLAN_VAP_MAX_NUM_PER_DEVICE_LIMIT) {
            OAM_ERROR_LOG1(0, OAM_SF_RX, "{hmac_rx_process_data_filter::Invalid vap_id.vap_id[%u]}",
                pst_vap->uc_vap_id);
            hmac_rx_free_netbuf_list(pst_netbuf_header, uc_buf_nums);
            continue;
        }
#ifdef _PRE_WLAN_FEATURE_AMPDU
        if (pst_rx_ctrl->st_rx_info.bit_amsdu_enable == OAL_FALSE) {
            ul_ret = hmac_ba_filter_serv(pst_vap, pst_hmac_user, pst_rx_ctrl, pst_frame_hdr, pst_netbuf_header,
                &en_is_ba_buf);
            OAL_MIPS_RX_STATISTIC(HMAC_PROFILING_FUNC_RX_REORDER_FILTER);
        } else {
            ul_ret = OAL_SUCC;
        }
#endif

        if (ul_ret != OAL_SUCC) {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#endif /* 1151��ʱע�͵�����ӡ */
            hmac_rx_free_netbuf_list(pst_netbuf_header, uc_buf_nums);
            continue;
        }

        if (en_is_ba_buf == OAL_TRUE) {
            continue;
        }

        /* �����buff��reorder���У������¹ҵ�����β������ */
        for (uc_netbuf_num = 0; uc_netbuf_num < uc_buf_nums; uc_netbuf_num++) {
            pst_netbuf = oal_netbuf_delist(pst_netbuf_header);
            if (OAL_LIKELY(pst_netbuf != OAL_PTR_NULL)) {
                oal_netbuf_add_to_list_tail(pst_netbuf, pst_netbuf_header);
            } else {
                OAM_WARNING_LOG0(pst_rx_ctrl->st_rx_info.bit_vap_id, OAM_SF_RX,
                    "{hmac_rx_process_data_filter::no buff error.}");
            }
        }
        OAL_MIPS_RX_STATISTIC(HMAC_PROFILING_FUNC_RX_NON_REORDER_BACK);
    }
}

#ifdef _PRE_WLAN_TCP_OPT
OAL_STATIC oal_bool_enum_uint8 hmac_transfer_rx_handler(hmac_device_stru *pst_hmac_device, hmac_vap_stru *hmac_vap,
    oal_netbuf_stru *netbuf)
{
#ifndef WIN32
    hmac_rx_ctl_stru *pst_rx_ctrl = OAL_PTR_NULL; /* ָ��MPDU���ƿ���Ϣ��ָ�� */
    oal_netbuf_stru* pst_mac_llc_snap_netbuf;

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
    if (pst_hmac_device->sys_tcp_rx_ack_opt_enable == OAL_TRUE) {
        pst_rx_ctrl = (hmac_rx_ctl_stru *)oal_netbuf_cb(netbuf);
        pst_mac_llc_snap_netbuf = (oal_netbuf_stru*)(netbuf->data + pst_rx_ctrl->st_rx_info.uc_mac_header_len);
#ifdef _PRE_WLAN_TCP_OPT_DEBUG
        OAM_WARNING_LOG1(0, OAM_SF_TX,
            "{hmac_transfer_rx_handler::uc_mac_header_len = %d}\r\n", pst_rx_ctrl->st_rx_info.uc_mac_header_len);
#endif
        if (hmac_judge_rx_netbuf_classify(pst_mac_llc_snap_netbuf) == OAL_TRUE) {
#ifdef _PRE_WLAN_TCP_OPT_DEBUG
            OAM_WARNING_LOG0(0, OAM_SF_TX, "{hmac_transfer_rx_handler::netbuf is tcp ack.}\r\n");
#endif
            oal_spin_lock_bh(&hmac_vap->st_hamc_tcp_ack[HCC_RX].data_queue_lock[HMAC_TCP_ACK_QUEUE]);
            oal_netbuf_list_tail(&hmac_vap->st_hamc_tcp_ack[HCC_RX].data_queue[HMAC_TCP_ACK_QUEUE], netbuf);
            pst_hmac_device->en_need_notify = OAL_TRUE;
            oal_spin_unlock_bh(&hmac_vap->st_hamc_tcp_ack[HCC_RX].data_queue_lock[HMAC_TCP_ACK_QUEUE]);
            hmac_sched_transfer();
            return OAL_TRUE;
        }
    }
#endif
#endif
    return OAL_FALSE;
}

#endif


oal_uint32 hmac_rx_lan_frame(oal_netbuf_head_stru *pst_netbuf_header)
{
    oal_uint32               ul_netbuf_num;
    oal_netbuf_stru          *pst_temp_netbuf = OAL_PTR_NULL;
    oal_netbuf_stru          *pst_netbuf      = OAL_PTR_NULL;
    oal_uint8                uc_buf_nums;
    hmac_rx_ctl_stru         *pst_rx_ctrl     = OAL_PTR_NULL;
    mac_ieee80211_frame_stru *pst_frame_hdr   = OAL_PTR_NULL;
    hmac_vap_stru            *pst_vap         = OAL_PTR_NULL;

    ul_netbuf_num   = oal_netbuf_get_buf_num(pst_netbuf_header);
    pst_temp_netbuf = oal_netbuf_peek(pst_netbuf_header);

    while (ul_netbuf_num != 0) {
        pst_netbuf = pst_temp_netbuf;
        if (pst_netbuf == NULL) {
            break;
        }

        pst_rx_ctrl   = (hmac_rx_ctl_stru*)oal_netbuf_cb(pst_netbuf);
        pst_frame_hdr = (mac_ieee80211_frame_stru *)pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr;
        uc_buf_nums = pst_rx_ctrl->st_rx_info.bit_buff_nums;

        ul_netbuf_num = OAL_SUB(ul_netbuf_num, uc_buf_nums);
        oal_netbuf_get_appointed_netbuf(pst_netbuf, uc_buf_nums, &pst_temp_netbuf);

        pst_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_rx_ctrl->st_rx_info.uc_mac_vap_id);
        if (pst_vap == OAL_PTR_NULL) {
            OAM_ERROR_LOG1(0, OAM_SF_RX, "{hmac_rx_lan_frame::mac_res_get_hmac_vap null. vap_id:%u}",
                pst_rx_ctrl->st_rx_info.uc_mac_vap_id);
            continue;
        }
        pst_rx_ctrl->st_rx_info.us_da_user_idx = pst_vap->st_vap_base_info.uc_assoc_vap_id;

        hmac_rx_lan_frame_classify(pst_vap, pst_netbuf, pst_frame_hdr);
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_HILINK
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION) || (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
extern oal_uint32 hwal_send_others_bss_data(oal_netbuf_stru *pst_netbuf);
#else
extern oal_int32 wal_hilink_upload_frame(oal_netbuf_stru *pst_netbuf);
#endif

oal_uint32 hmac_rx_process_others_bss_data(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru           *pst_event         = OAL_PTR_NULL;
    frw_event_hdr_stru       *pst_event_hdr     = OAL_PTR_NULL;
    oal_netbuf_stru          *pst_netbuf        = OAL_PTR_NULL;
    oal_netbuf_stru          *pst_temp_netbuf   = OAL_PTR_NULL;
    dmac_wlan_drx_event_stru *pst_wlan_rx_event = OAL_PTR_NULL;
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION) || (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
    uint32_t                 ret = OAL_SUCC;
#else
    int32_t                  ret = OAL_SUCC;
#endif
    oal_uint16               us_netbuf_num;
    if (pst_event_mem == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* ��ȡ�¼�ͷ���¼��ṹ��ָ�� */
    pst_event           = (frw_event_stru *)pst_event_mem->puc_data;
    pst_event_hdr       = &(pst_event->st_event_hdr);
    pst_wlan_rx_event   = (dmac_wlan_drx_event_stru *)(pst_event->auc_event_data);
    us_netbuf_num       = pst_wlan_rx_event->us_netbuf_num;
    pst_netbuf          = pst_wlan_rx_event->pst_netbuf;
    if (pst_netbuf == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    while (us_netbuf_num != 0) {
        pst_temp_netbuf = pst_netbuf;
        if (pst_temp_netbuf == OAL_PTR_NULL) {
            return OAL_ERR_CODE_PTR_NULL;
        }
        pst_netbuf = OAL_NETBUF_NEXT(pst_temp_netbuf);
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION) || (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
#ifndef _PRE_HI1131K_LINUX_UT
        /* ���¼���wal�����������¼��������������¼�ʧ��,ֱ�ӵ��ÿ������Ч�� */
        ret = hwal_send_others_bss_data(pst_temp_netbuf);
#endif
#else
        ret = wal_hilink_upload_frame(pst_temp_netbuf);
#endif
        if (ret != OAL_SUCC) {
            OAM_ERROR_LOG1(0, OAM_SF_RX,
                "{hmac_rx_process_others_bss_data::hwal_send_others_bss_data|wal_hilink_upload_frame ret=%d.}", ret);
            return OAL_FAIL;
        }
        us_netbuf_num--;
    }
    return OAL_SUCC;
}

extern oal_uint8 g_uc_data_switch;
#endif


oal_uint32 hmac_rx_process_data_ap(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru           *pst_event         = OAL_PTR_NULL;
    frw_event_hdr_stru       *pst_event_hdr     = OAL_PTR_NULL;
    dmac_wlan_drx_event_stru *pst_wlan_rx_event = OAL_PTR_NULL;
    oal_netbuf_stru          *pst_netbuf        = OAL_PTR_NULL; /* ���ڱ��浱ǰ�����MPDU�ĵ�һ��netbufָ�� */
    oal_netbuf_stru          *pst_temp_netbuf   = OAL_PTR_NULL; /* ������ʱ������һ����Ҫ�����netbufָ�� */
    oal_uint16               us_netbuf_num;                    /* netbuf����ĸ��� */
    oal_netbuf_head_stru     st_netbuf_header;                 /* �洢�ϱ������������� */
    hmac_vap_stru            *pst_hmac_vap      = OAL_PTR_NULL;
#ifdef _PRE_WLAN_TCP_OPT
    oal_netbuf_head_stru     st_temp_header;
    hmac_device_stru         *pst_hmac_device   = OAL_PTR_NULL;
#endif
#ifdef _PRE_WLAN_FEATURE_HILINK
    oal_uint32               ul_ret;
#endif

    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_rx_process_data_ap::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_PROFILING_RX_STATISTIC(OAM_PROFILING_FUNC_RX_HMAC_START);

    /* ��ȡ�¼�ͷ���¼��ṹ��ָ�� */
    pst_event         = (frw_event_stru *)pst_event_mem->puc_data;
    pst_event_hdr     = &(pst_event->st_event_hdr);
    pst_wlan_rx_event = (dmac_wlan_drx_event_stru *)(pst_event->auc_event_data);
    pst_temp_netbuf   = pst_wlan_rx_event->pst_netbuf;
    us_netbuf_num     = pst_wlan_rx_event->us_netbuf_num;
#ifdef _PRE_WLAN_FEATURE_HILINK
    if (g_uc_data_switch == OAL_TRUE) {
        /* �����ϱ�������BSS�鲥���ݰ� */
        ul_ret = hmac_rx_process_others_bss_data(pst_event_mem);
        if (ul_ret != OAL_SUCC) {
            OAM_ERROR_LOG1(0, OAM_SF_RX, "hmac_rx_process_data_ap::process other bss data failed! ul_ret=%d", ul_ret);
        }
        /* monitorģʽ�ϱ��Ĺ㲥���ݰ����鲥�����ڸô�ͳһ�ͷ� */
        hmac_rx_free_netbuf(pst_temp_netbuf, us_netbuf_num);
        return ul_ret;
    }
#endif

    OAM_PROFILING_RX_STATISTIC(OAM_PROFILING_FUNC_RX_HMAC_BASE_INFO);

    if (OAL_UNLIKELY(pst_temp_netbuf == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(0, OAM_SF_RX, "{hmac_rx_process_data_ap::us_netbuf_num = %d.}", us_netbuf_num);
        return OAL_SUCC; /* ������¼���������Ϊ�˷�ֹ51��UT�ҵ� ���� true */
    }
#ifdef _PRE_WLAN_TCP_OPT
    pst_hmac_device = hmac_res_get_mac_dev(pst_event_hdr->uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_rx_process_data_ap::pst_hmac_device null.}");
        hmac_rx_free_netbuf(pst_temp_netbuf, us_netbuf_num);
        return OAL_ERR_CODE_PTR_NULL;
    }
#endif
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_event_hdr->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_rx_process_data_ap::pst_hmac_vap null.}");
        hmac_rx_free_netbuf(pst_temp_netbuf, us_netbuf_num);
        return OAL_ERR_CODE_PTR_NULL;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* ��ʱ���mib_info ָ��Ϊ�յ����⣬If mib info is null ptr,release the netbuf */
    if (pst_hmac_vap->st_vap_base_info.pst_mib_info == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_rx_process_data_ap::pst_mib_info null.}");
        hmac_rx_free_netbuf(pst_temp_netbuf, us_netbuf_num);
        return OAL_SUCC;
    }
#endif

    /* ������netbuffȫ�������� */
    oal_netbuf_list_head_init(&st_netbuf_header);
    while (us_netbuf_num != 0) {
        pst_netbuf = pst_temp_netbuf;
        if (pst_netbuf == OAL_PTR_NULL) {
            break;
        }

        pst_temp_netbuf = OAL_NETBUF_NEXT(pst_netbuf);
        oal_netbuf_add_to_list_tail(pst_netbuf, &st_netbuf_header);
        us_netbuf_num--;
    }

    if (us_netbuf_num != 0) {
        OAM_ERROR_LOG2(0, OAM_SF_RX, "{hmac_rx_process_data_ap::us_netbuf_num[%d], event_buf_num[%d].}",
                       us_netbuf_num, pst_wlan_rx_event->us_netbuf_num);
    }

    /* ��Dmac�ϱ���֡����reorder���й���һ�� */
    hmac_rx_process_data_filter(&st_netbuf_header, pst_wlan_rx_event->pst_netbuf, pst_wlan_rx_event->us_netbuf_num);

#ifdef _PRE_WLAN_TCP_OPT
    oal_netbuf_head_init(&st_temp_header);

    while (!!(pst_temp_netbuf = oal_netbuf_delist(&st_netbuf_header))) {
        if (hmac_transfer_rx_handler(pst_hmac_device, pst_hmac_vap, pst_temp_netbuf) == OAL_FALSE) {
            oal_netbuf_list_tail(&st_temp_header, pst_temp_netbuf);
        }
    }
    /*lint -e522*/
    OAL_WARN_ON(!oal_netbuf_list_empty(&st_netbuf_header)); //lint !e730
    /*lint +e522*/
    oal_netbuf_splice_init(&st_temp_header, &st_netbuf_header);
#endif

    hmac_rx_process_data_ap_tcp_ack_opt(pst_hmac_vap, &st_netbuf_header);
    return OAL_SUCC;
}


oal_void hmac_rx_process_data_ap_tcp_ack_opt(hmac_vap_stru *pst_vap, oal_netbuf_head_stru *pst_netbuf_header)
{
    frw_event_hdr_stru        st_event_hdr;
    mac_ieee80211_frame_stru  *pst_frame_hdr      = OAL_PTR_NULL; /* ����mac֡��ָ�� */
    mac_ieee80211_frame_stru  *pst_copy_frame_hdr = OAL_PTR_NULL; /* ����mac֡��ָ�� */
    oal_uint8                 *puc_da             = OAL_PTR_NULL; /* �����û�Ŀ�ĵ�ַ��ָ�� */
    hmac_user_stru            *pst_hmac_da_user   = OAL_PTR_NULL;
    oal_uint32                ul_rslt;
    oal_uint16                us_user_dix;
    hmac_rx_ctl_stru          *pst_rx_ctrl        = OAL_PTR_NULL; /* ÿһ��MPDU�Ŀ�����Ϣ */
    oal_uint16                us_netbuf_num;                      /* netbuf����ĸ��� */
    oal_uint8                 uc_buf_nums;                        /* ÿ��mpduռ��buf�ĸ��� */
    oal_netbuf_stru           *pst_netbuf         = OAL_PTR_NULL; /* ���ڱ��浱ǰ�����MPDU�ĵ�һ��netbufָ�� */
    oal_netbuf_stru           *pst_temp_netbuf    = OAL_PTR_NULL; /* ������ʱ������һ����Ҫ�����netbufָ�� */
    oal_netbuf_stru           *pst_netbuf_copy    = OAL_PTR_NULL; /* ���ڱ����鲥֡copy */
    oal_netbuf_head_stru      st_w2w_netbuf_hdr;                  /* ����wlan to wlan��netbuf�����ͷ */
#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
    cs_isolation_forward_enum en_forward;
#endif

    /* ѭ���յ���ÿһ��MPDU�����������:
        1���鲥֡ʱ������WLAN TO WLAN��WLAN TO LAN�ӿ�
        2������������ʵ�����������WLAN TO LAN�ӿڻ���WLAN TO WLAN�ӿ� */
    oal_netbuf_list_head_init(&st_w2w_netbuf_hdr);
    pst_temp_netbuf = oal_netbuf_peek(pst_netbuf_header);
    us_netbuf_num = (oal_uint16)oal_netbuf_get_buf_num(pst_netbuf_header);
    st_event_hdr.uc_chip_id = pst_vap->st_vap_base_info.uc_chip_id;
    st_event_hdr.uc_device_id = pst_vap->st_vap_base_info.uc_device_id;
    st_event_hdr.uc_vap_id = pst_vap->st_vap_base_info.uc_vap_id;

    while (us_netbuf_num != 0) {
        pst_netbuf  = pst_temp_netbuf;
        if (pst_netbuf == OAL_PTR_NULL) {
            break;
        }

        pst_rx_ctrl   = (hmac_rx_ctl_stru*)oal_netbuf_cb(pst_netbuf);

        /* ��ȡ֡ͷ��Ϣ */
        pst_frame_hdr = (mac_ieee80211_frame_stru *)pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr;

        /* ��ȡ��ǰMPDUռ�õ�netbuf��Ŀ */
        uc_buf_nums   = pst_rx_ctrl->st_rx_info.bit_buff_nums;

        /* ��ȡ��һ��Ҫ�����MPDU */
        oal_netbuf_get_appointed_netbuf(pst_netbuf, uc_buf_nums, &pst_temp_netbuf);
        us_netbuf_num = OAL_SUB(us_netbuf_num, uc_buf_nums);

        pst_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_rx_ctrl->st_rx_info.uc_mac_vap_id);
        if (OAL_UNLIKELY(pst_vap == OAL_PTR_NULL)) {
            OAM_WARNING_LOG0(pst_rx_ctrl->st_rx_info.bit_vap_id, OAM_SF_RX, "{hmac_rx_process_data_ap::pst_vap null.}");
            hmac_rx_free_netbuf(pst_netbuf, (oal_uint16)uc_buf_nums);
            continue;
        }

        /* ��ȡ���ն˵�ַ  */
        mac_rx_get_da(pst_frame_hdr, &puc_da);

        /* Ŀ�ĵ�ַΪ�鲥��ַʱ������WLAN_TO_WLAN��WLAN_TO_LAN��ת�� */
        if (ETHER_IS_MULTICAST(puc_da)) {
            OAM_INFO_LOG0(st_event_hdr.uc_vap_id, OAM_SF_RX,
                "{hmac_rx_lan_frame_classify::the frame is a group frame.}");
            OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, rx_mcast_cnt, 1);

            if (hmac_rx_copy_netbuff(&pst_netbuf_copy, pst_netbuf,
                pst_rx_ctrl->st_rx_info.uc_mac_vap_id, &pst_copy_frame_hdr) != OAL_SUCC) {
                OAM_WARNING_LOG0(st_event_hdr.uc_vap_id, OAM_SF_RX,
                    "{hmac_rx_process_data_ap::send mcast pkt to air fail.}");

                OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, rx_no_buff_dropped, 1);
                continue;
            }

            hmac_rx_lan_frame_classify(pst_vap, pst_netbuf, pst_frame_hdr); // �ϱ������

#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
            pst_rx_ctrl   = (hmac_rx_ctl_stru*)oal_netbuf_cb(pst_netbuf_copy);

            /* ��ȡ֡ͷ��Ϣ */
            pst_frame_hdr = (mac_ieee80211_frame_stru *)pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr;
            mac_rx_get_da(pst_frame_hdr, &puc_da);

            en_forward = hmac_isolation_filter(&pst_vap->st_vap_base_info, puc_da);
            if (en_forward == CS_ISOLATION_FORWORD_DROP) {
                /* �ͷŵ�ǰ�����MPDUռ�õ�netbuf. 2014.7.29 cause memory leak bug fixed */
                hmac_rx_free_netbuf(pst_netbuf_copy, (oal_uint16)uc_buf_nums);
                continue;
            }
#endif

            /* ��MPDU�����ɵ���MSDU�������е�MSDU���һ��netbuf�� */
            hmac_rx_prepare_msdu_list_to_wlan(pst_vap, &st_w2w_netbuf_hdr, pst_netbuf_copy, pst_copy_frame_hdr);
            continue;
        }

#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
        en_forward = hmac_isolation_filter(&pst_vap->st_vap_base_info, puc_da);
        if (en_forward == CS_ISOLATION_FORWORD_DROP) {
            /* �ͷŵ�ǰ�����MPDUռ�õ�netbuf. 2014.7.29 cause memory leak bug fixed */
            hmac_rx_free_netbuf(pst_netbuf, (oal_uint16)uc_buf_nums);
            /* return OAL_SUCC; bug fixed */
            continue;
        }
#endif

        /* ��ȡĿ�ĵ�ַ��Ӧ���û�ָ�� */
        ul_rslt = mac_vap_find_user_by_macaddr(&pst_vap->st_vap_base_info, puc_da, &us_user_dix);
        if (ul_rslt == OAL_ERR_CODE_PTR_NULL) { /* �����û�ʧ�� */
            /* �ͷŵ�ǰ�����MPDUռ�õ�netbuf */
            hmac_rx_free_netbuf(pst_netbuf, (oal_uint16)uc_buf_nums);

            OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, rx_da_check_dropped, 1);
            continue;
        }

        /* û���ҵ���Ӧ���û� */
        if (ul_rslt != OAL_SUCC) {
            OAM_INFO_LOG0(st_event_hdr.uc_vap_id, OAM_SF_RX,
                "{hmac_rx_lan_frame_classify::the frame is a unique frame.}");
            /* Ŀ���û�����AP���û����У�����wlan_to_lanת���ӿ� */
            hmac_rx_lan_frame_classify(pst_vap, pst_netbuf, pst_frame_hdr);
            continue;
        }

        /* Ŀ���û�����AP���û����У�����WLAN_TO_WLANת�� */
        pst_hmac_da_user = (hmac_user_stru *)mac_res_get_hmac_user(us_user_dix);
        if (pst_hmac_da_user == OAL_PTR_NULL) {
            OAM_WARNING_LOG0(st_event_hdr.uc_vap_id, OAM_SF_RX, "{hmac_rx_lan_frame_classify::pst_hmac_da_user null.}");
            OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, rx_da_check_dropped, 1);

            hmac_rx_free_netbuf(pst_netbuf, (oal_uint16)uc_buf_nums);
            continue;
        }

        if (pst_hmac_da_user->st_user_base_info.en_user_asoc_state != MAC_USER_STATE_ASSOC) {
            OAM_WARNING_LOG0(st_event_hdr.uc_vap_id, OAM_SF_RX,
                "{hmac_rx_lan_frame_classify::the station is not associated with ap.}");
            OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, rx_da_check_dropped, 1);

            hmac_rx_free_netbuf(pst_netbuf, (oal_uint16)uc_buf_nums);
            hmac_mgmt_send_deauth_frame(&pst_vap->st_vap_base_info, puc_da, MAC_NOT_AUTHED, OAL_FALSE);

            continue;
        }

        /* ��Ŀ�ĵ�ַ����Դ������ֵ�ŵ�cb�ֶ��У�user��asoc id���ڹ�����ʱ�򱻸�ֵ */
        pst_rx_ctrl->st_rx_info.us_da_user_idx = pst_hmac_da_user->st_user_base_info.us_assoc_id;

        /* ��MPDU�����ɵ���MSDU�������е�MSDU���һ��netbuf�� */
        hmac_rx_prepare_msdu_list_to_wlan(pst_vap, &st_w2w_netbuf_hdr, pst_netbuf, pst_frame_hdr);
    }

    OAM_PROFILING_RX_STATISTIC(OAM_PROFILING_FUNC_RX_HMAC_TO_LAN);

    /*  ��MSDU�������������̴��� */
    if (oal_netbuf_list_empty(&st_w2w_netbuf_hdr) == OAL_FALSE && oal_netbuf_tail(&st_w2w_netbuf_hdr) != OAL_PTR_NULL &&
        oal_netbuf_peek(&st_w2w_netbuf_hdr) != OAL_PTR_NULL) {
        OAL_NETBUF_NEXT((oal_netbuf_tail(&st_w2w_netbuf_hdr))) = OAL_PTR_NULL;
        OAL_NETBUF_PREV((oal_netbuf_peek(&st_w2w_netbuf_hdr))) = OAL_PTR_NULL;

        hmac_rx_transmit_to_wlan(&st_event_hdr, &st_w2w_netbuf_hdr);
    }
    OAM_PROFILING_RX_STATISTIC(OAM_PROFILING_FUNC_RX_HMAC_TO_WLAN);

    OAM_PROFILING_RX_STATISTIC(OAM_PROFILING_FUNC_RX_HMAC_END);
}


oal_uint32 hmac_rx_process_data_sta(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru           *pst_event         = OAL_PTR_NULL;
    frw_event_hdr_stru       *pst_event_hdr     = OAL_PTR_NULL;
    dmac_wlan_drx_event_stru *pst_wlan_rx_event = OAL_PTR_NULL;
    oal_netbuf_stru          *pst_netbuf        = OAL_PTR_NULL; /* ������ʱ������һ����Ҫ�����netbufָ�� */
    oal_uint16               us_netbuf_num;                  /* netbuf����ĸ��� */
    oal_netbuf_head_stru     st_netbuf_header;               /* �洢�ϱ������������� */
    oal_netbuf_stru          *pst_temp_netbuf   = OAL_PTR_NULL;
    hmac_vap_stru            *pst_hmac_vap      = OAL_PTR_NULL;
#ifdef _PRE_WLAN_TCP_OPT
    oal_netbuf_head_stru     st_temp_header;
    hmac_device_stru         *pst_hmac_device   = OAL_PTR_NULL;
#endif

    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_rx_process_data_sta::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_PROFILING_RX_STATISTIC(OAM_PROFILING_FUNC_RX_HMAC_START);
    OAL_MIPS_RX_STATISTIC(HMAC_PROFILING_FUNC_RX_DATA_START);

    /* ��ȡ�¼�ͷ���¼��ṹ��ָ�� */
    pst_event         = (frw_event_stru *)pst_event_mem->puc_data;
    pst_event_hdr     = &(pst_event->st_event_hdr);
    pst_wlan_rx_event = (dmac_wlan_drx_event_stru *)(pst_event->auc_event_data);
    pst_temp_netbuf   = pst_wlan_rx_event->pst_netbuf;
    us_netbuf_num     = pst_wlan_rx_event->us_netbuf_num;

    OAM_PROFILING_RX_STATISTIC(OAM_PROFILING_FUNC_RX_HMAC_BASE_INFO);

#ifdef _PRE_WLAN_TCP_OPT
    pst_hmac_device = hmac_res_get_mac_dev(pst_event_hdr->uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_rx_process_data_sta::pst_hmac_device null.}");
        hmac_rx_free_netbuf(pst_temp_netbuf, us_netbuf_num);
        return OAL_ERR_CODE_PTR_NULL;
    }
#endif

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_event_hdr->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_rx_process_data_sta::pst_hmac_vap null.}");
        hmac_rx_free_netbuf(pst_temp_netbuf, us_netbuf_num);
        return OAL_ERR_CODE_PTR_NULL;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* If mib info is null ptr,release the netbuf */
    if (pst_hmac_vap->st_vap_base_info.pst_mib_info == NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_rx_process_data_sta::pst_mib_info null.}");
        hmac_rx_free_netbuf(pst_temp_netbuf, us_netbuf_num);
        return OAL_SUCC;
    }
#endif

    /* ������netbuffȫ�������� */
    oal_netbuf_list_head_init(&st_netbuf_header);
    while (us_netbuf_num != 0) {
        pst_netbuf = pst_temp_netbuf;
        if (pst_netbuf == OAL_PTR_NULL) {
            break;
        }

        pst_temp_netbuf = OAL_NETBUF_NEXT(pst_netbuf);
        oal_netbuf_add_to_list_tail(pst_netbuf, &st_netbuf_header);
        us_netbuf_num--;
    }

    if (us_netbuf_num != 0) {
        OAM_ERROR_LOG2(0, OAM_SF_RX, "{hmac_rx_process_data_sta::us_netbuf_num[%d], event_buf_num[%d].}",
                       us_netbuf_num, pst_wlan_rx_event->us_netbuf_num);
    }

    OAL_MIPS_RX_STATISTIC(HMAC_PROFILING_FUNC_RX_GET_NETBUF_LIST);

    hmac_rx_process_data_filter(&st_netbuf_header, pst_wlan_rx_event->pst_netbuf, pst_wlan_rx_event->us_netbuf_num);

#ifdef _PRE_WLAN_TCP_OPT
    oal_netbuf_head_init(&st_temp_header);
    while (!!(pst_temp_netbuf = oal_netbuf_delist(&st_netbuf_header))) {
        if (hmac_transfer_rx_handler(pst_hmac_device, pst_hmac_vap, pst_temp_netbuf) == OAL_FALSE) {
            oal_netbuf_list_tail(&st_temp_header, pst_temp_netbuf);
        }
    }
    /*lint -e522 -e730*/
    OAL_WARN_ON(!oal_netbuf_list_empty(&st_netbuf_header));
    /*lint +e522 +e730*/
    oal_netbuf_splice_init(&st_temp_header, &st_netbuf_header);
#endif
    OAL_MIPS_RX_STATISTIC(HMAC_PROFILING_FUNC_RX_TCP_ACK_OPT);

    hmac_rx_process_data_sta_tcp_ack_opt(pst_hmac_vap, &st_netbuf_header);
    return OAL_SUCC;
}


oal_uint32 hmac_rx_process_data_sta_tcp_ack_opt(hmac_vap_stru *pst_vap, oal_netbuf_head_stru *pst_netbuf_header)
{
    /* ����Ҫ�ϱ���֡��һ���Ӵ��� */
    hmac_rx_lan_frame(pst_netbuf_header);

    OAM_PROFILING_RX_STATISTIC(OAM_PROFILING_FUNC_RX_HMAC_END);
    OAL_MIPS_RX_STATISTIC(HMAC_PROFILING_FUNC_RX_HMAC_END);
#ifdef _PRE_WLAN_PROFLING_MIPS
    oal_profiling_stop_rx_save();
#endif
    return OAL_SUCC;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

