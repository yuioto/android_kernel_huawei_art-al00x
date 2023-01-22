

#include "oal_net.h"
#include "oal_cfg80211.h"
#include "oam_wdk.h"
#include "securec.h"
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#include "hwal_event.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAL_CFG80211_C

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)||(_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
static oal_kobj_uevent_env_stru env;
#endif
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
/*
 * ��������  : ���볧���Զ��巵������
 * �������  : oal_wiphy_stru * wiphy: wiphy �ṹ
 *             uint32_t         len  : ���볤��
 */
oal_netbuf_stru *oal_cfg80211_vendor_cmd_alloc_reply_skb(oal_wiphy_stru *wiphy, uint32_t len)
{
#if (KERNEL_VERSION(2, 6, 34) <= LINUX_VERSION_CODE)
    return cfg80211_vendor_cmd_alloc_reply_skb(wiphy, len);
#else
    return OAL_PTR_NULL;
#endif
}

/*
 * ��������  : �����Զ��������ϱ�
 * �������  : oal_netbuf_stru *skb_buff: ��������
 */
int32_t oal_cfg80211_vendor_cmd_reply(oal_netbuf_stru *skb_buff)
{
#if (KERNEL_VERSION(2, 6, 34) <= LINUX_VERSION_CODE)
    return cfg80211_vendor_cmd_reply(skb_buff);
#else
    return OAL_SUCC;
#endif
}
#endif

/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/
oal_void oal_cfg80211_ready_on_channel(
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 44))\
            || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    oal_wireless_dev_stru       *pst_wdev,
#else
    oal_net_device_stru         *pst_net_dev,
#endif
    oal_uint64                  ull_cookie,
    oal_ieee80211_channel_stru *pst_chan,
    oal_uint32                  ul_duration,
    oal_gfp_enum_uint8          en_gfp)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 44))
    cfg80211_ready_on_channel(pst_wdev, ull_cookie, pst_chan, ul_duration, en_gfp);
#else
    enum nl80211_channel_type   en_channel_type;
    en_channel_type = NL80211_CHAN_HT20;
    cfg80211_ready_on_channel(pst_net_dev, ull_cookie, pst_chan, en_channel_type, ul_duration, en_gfp);
#endif
#endif
}

oal_void oal_cfg80211_remain_on_channel_expired(oal_wireless_dev_stru        *pst_wdev,
                                                oal_uint64                   ull_cookie,
                                                oal_ieee80211_channel_stru  *pst_listen_channel,
                                                oal_gfp_enum_uint8           en_gfp)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 44))
    cfg80211_remain_on_channel_expired(pst_wdev,
                                       ull_cookie,
                                       pst_listen_channel,
                                       GFP_ATOMIC);
#else
    enum nl80211_channel_type   en_channel_type;
    oal_net_device_stru         *pst_net_dev;

    en_channel_type = NL80211_CHAN_HT20;
    pst_net_dev = pst_wdev->netdev;

    cfg80211_remain_on_channel_expired(pst_net_dev,
                                       ull_cookie,
                                       pst_listen_channel,
                                       en_channel_type,
                                       0);
#endif
#endif
}
oal_void oal_cfg80211_mgmt_tx_status(
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 44)) \
            || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION) \
                || (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
    struct wireless_dev *wdev,
#else
    oal_net_device_stru *pst_net_dev,
#endif
    oal_uint64        cookie,
    OAL_CONST oal_uint8 *buf,
    size_t               len,
    oal_bool_enum_uint8  ack,
    oal_gfp_enum_uint8   gfp)

{
#ifndef _PRE_HI1131K_LINUX_UT
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 44)) \
        || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION) \
            || (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
    cfg80211_mgmt_tx_status(wdev, cookie, buf, len, ack, gfp);
#else // linux vs 3.4.5
    cfg80211_mgmt_tx_status(pst_net_dev, cookie, buf, len, ack, gfp);
#endif
#endif
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)||(_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#else
OAL_STATIC oal_int32 oal_cfg80211_calculate_bitrate(oal_rate_info_stru *pst_rate)
{
    oal_int32  l_modulation;
    oal_int32  l_streams;
    oal_int32  l_bitrate;

    if ((pst_rate->flags & RATE_INFO_FLAGS_MCS) == 0) {
        return pst_rate->legacy;
    }

    /* the formula below does only work for MCS values smaller than 32 */
    if (pst_rate->mcs >= 32) {
        return -OAL_ERR_CODE_CFG80211_MCS_EXCEED;
    }
    l_modulation = pst_rate->mcs & 7;
    l_streams = (pst_rate->mcs >> 3) + 1;

    l_bitrate = (pst_rate->flags & RATE_INFO_FLAGS_40_MHZ_WIDTH) ? 13500000 : 6500000;

    if (l_modulation < 4) {
        l_bitrate *= (l_modulation + 1);
    } else if (l_modulation == 4) {
        l_bitrate *= (l_modulation + 2);
    } else {
        l_bitrate *= (l_modulation + 3);
    }
    l_bitrate *= l_streams;

    if (pst_rate->flags & RATE_INFO_FLAGS_SHORT_GI) {
        l_bitrate = (l_bitrate / 9) * 10;
    }
    /* do NOT round down here */
    return (l_bitrate + 50000) / 100000;
}

OAL_STATIC oal_int32  oal_nl80211_send_station(
    oal_netbuf_stru *pst_buf, oal_uint32 ul_pid, oal_uint32 ul_seq,
    oal_int32 l_flags, oal_net_device_stru *pst_net_dev,
    const oal_uint8 *puc_mac_addr, oal_station_info_stru *pst_station_info)
{
    oal_nlattr_stru   *pst_sinfoattr = OAL_PTR_NULL;
    oal_nlattr_stru   *pst_txrate    = OAL_PTR_NULL;
    oal_void          *p_hdr         = OAL_PTR_NULL;
    oal_int32          us_bitrate;

    /* Add generic netlink header to netlink message, returns pointer to user specific header */
    p_hdr = oal_genlmsg_put(pst_buf, ul_pid, ul_seq, &nl80211_fam, l_flags, PRIV_NL80211_CMD_NEW_STATION);
    if (p_hdr == OAL_PTR_NULL) {
        return OAL_ERR_CODE_CFG80211_SKB_MEM_FAIL;
    }

    OAL_NLA_PUT_U32(pst_buf, PRIV_NL80211_ATTR_IFINDEX, pst_net_dev->ifindex);
    OAL_NLA_PUT(pst_buf, PRIV_NL80211_ATTR_MAC, OAL_ETH_ALEN_SIZE, puc_mac_addr);

    /*
        linux-2.6.30�ں�station_infoû��generation��Ա
    */
    pst_sinfoattr = oal_nla_nest_start(pst_buf, PRIV_NL80211_ATTR_STA_INFO);
    if (pst_sinfoattr == OAL_PTR_NULL) {
        oal_genlmsg_cancel(pst_buf, p_hdr);
        return OAL_ERR_CODE_CFG80211_EMSGSIZE;
    }
    if (pst_station_info->filled & STATION_INFO_INACTIVE_TIME) {
        OAL_NLA_PUT_U32(pst_buf, PRIV_NL80211_STA_INFO_INACTIVE_TIME, pst_station_info->inactive_time);
    }
    if (pst_station_info->filled & STATION_INFO_RX_BYTES) {
        OAL_NLA_PUT_U32(pst_buf, PRIV_NL80211_STA_INFO_RX_BYTES, pst_station_info->rx_bytes);
    }
    if (pst_station_info->filled & STATION_INFO_TX_BYTES) {
        OAL_NLA_PUT_U32(pst_buf, PRIV_NL80211_STA_INFO_TX_BYTES, pst_station_info->tx_bytes);
    }
    if (pst_station_info->filled & STATION_INFO_LLID) {
        OAL_NLA_PUT_U16(pst_buf, PRIV_NL80211_STA_INFO_LLID, pst_station_info->llid);
    }
    if (pst_station_info->filled & STATION_INFO_PLID) {
        OAL_NLA_PUT_U16(pst_buf, PRIV_NL80211_STA_INFO_PLID, pst_station_info->plid);
    }
    if (pst_station_info->filled & STATION_INFO_PLINK_STATE) {
        OAL_NLA_PUT_U8(pst_buf, PRIV_NL80211_STA_INFO_PLINK_STATE, pst_station_info->plink_state);
    }
    if (pst_station_info->filled & STATION_INFO_SIGNAL) {
        OAL_NLA_PUT_U8(pst_buf, PRIV_NL80211_STA_INFO_SIGNAL, pst_station_info->signal);
    }
    if (pst_station_info->filled & STATION_INFO_TX_BITRATE) {
        pst_txrate = oal_nla_nest_start(pst_buf, PRIV_NL80211_STA_INFO_TX_BITRATE);
        if (pst_txrate == OAL_PTR_NULL) {
            oal_genlmsg_cancel(pst_buf, p_hdr);
            return OAL_ERR_CODE_CFG80211_EMSGSIZE;
        }

        /* cfg80211_calculate_bitrate will return negative for mcs >= 32 */
        us_bitrate = oal_cfg80211_calculate_bitrate(&pst_station_info->txrate);
        if (us_bitrate > 0)
            OAL_NLA_PUT_U16(pst_buf, PRIV_NL80211_RATE_INFO_BITRATE, us_bitrate);
        if (pst_station_info->txrate.flags & RATE_INFO_FLAGS_MCS)
            OAL_NLA_PUT_U8(pst_buf, PRIV_NL80211_RATE_INFO_MCS, pst_station_info->txrate.mcs);
        if (pst_station_info->txrate.flags & RATE_INFO_FLAGS_40_MHZ_WIDTH)
            OAL_NLA_PUT_FLAG(pst_buf, PRIV_NL80211_RATE_INFO_40_MHZ_WIDTH);
        if (pst_station_info->txrate.flags & RATE_INFO_FLAGS_SHORT_GI)
            OAL_NLA_PUT_FLAG(pst_buf, PRIV_NL80211_RATE_INFO_SHORT_GI);

        oal_nla_nest_end(pst_buf, pst_txrate);
    }
    if (pst_station_info->filled & STATION_INFO_RX_PACKETS) {
        OAL_NLA_PUT_U32(pst_buf, PRIV_NL80211_STA_INFO_RX_PACKETS, pst_station_info->rx_packets);
    }
    if (pst_station_info->filled & STATION_INFO_TX_PACKETS) {
        OAL_NLA_PUT_U32(pst_buf, PRIV_NL80211_STA_INFO_TX_PACKETS, pst_station_info->tx_packets);
    }

    oal_nla_nest_end(pst_buf, pst_sinfoattr);

    if (oal_genlmsg_end(pst_buf, p_hdr) < 0) {
        return OAL_ERR_CODE_CFG80211_ENOBUFS;
    }

    return OAL_SUCC;

nla_put_failure:
    oal_genlmsg_cancel(pst_buf, p_hdr);
    return OAL_ERR_CODE_CFG80211_EMSGSIZE;
}

OAL_STATIC oal_uint32 oal_nl80211_send_connect_result(
    oal_netbuf_stru             *pst_buf,
    oal_net_device_stru         *pst_net_device,
    const oal_uint8             *puc_bssid,
    const oal_uint8             *puc_req_ie,
    oal_uint32             ul_req_ie_len,
    const oal_uint8             *puc_resp_ie,
    oal_uint32             ul_resp_ie_len,
    oal_uint16             us_status,
    oal_gfp_enum_uint8           en_gfp)
{
    oal_void         *p_hdr = OAL_PTR_NULL;
    oal_int32         ul_let;

    p_hdr = oal_genlmsg_put(pst_buf, 0, 0, &nl80211_fam, 0, PRIV_NL80211_CMD_CONNECT);
    if (p_hdr == OAL_PTR_NULL) {
        oal_nlmsg_free(pst_buf);
        return OAL_ERR_CODE_CFG80211_SKB_MEM_FAIL;
    }

    /*
        ��������ϲ�û�н��������Բ��ϱ�
    */
    OAL_NLA_PUT_U32(pst_buf, PRIV_NL80211_ATTR_IFINDEX, pst_net_device->ifindex);
    if (puc_bssid != OAL_PTR_NULL) {
        OAL_NLA_PUT(pst_buf, PRIV_NL80211_ATTR_MAC, OAL_ETH_ALEN_SIZE, puc_bssid);
    }
    OAL_NLA_PUT_U16(pst_buf, PRIV_NL80211_ATTR_STATUS_CODE, us_status);
    if (puc_req_ie != OAL_PTR_NULL) {
        OAL_NLA_PUT(pst_buf, PRIV_NL80211_ATTR_REQ_IE, ul_req_ie_len, puc_req_ie);
    }
    if (puc_resp_ie != OAL_PTR_NULL) {
        OAL_NLA_PUT(pst_buf, PRIV_NL80211_ATTR_RESP_IE, ul_resp_ie_len, puc_resp_ie);
    }

    if (oal_genlmsg_end(pst_buf, p_hdr) < 0) {
        oal_nlmsg_free(pst_buf);
        return OAL_ERR_CODE_CFG80211_ENOBUFS;
    }

    ul_let = oal_genlmsg_multicast(pst_buf, 0, nl80211_mlme_mcgrp.id, en_gfp);
    if (ul_let < 0) {
        /* ���������hostapd��wpa_supplicant�Ļ������Ҳ��ʧ�ܣ����ﱨfail��Ӱ��ʹ�ã�ȥ������ */
        return OAL_FAIL;
    }

    return OAL_SUCC;

nla_put_failure:
    oal_genlmsg_cancel(pst_buf, p_hdr);
    oal_nlmsg_free(pst_buf);
    return OAL_ERR_CODE_CFG80211_EMSGSIZE;
}

OAL_STATIC oal_int32 oal_nl80211_send_disconnected(
    oal_net_device_stru *pst_net_device,
    oal_uint16           us_reason,
    const oal_uint8     *puc_ie,
    oal_uint32           ul_ie_len,
    oal_bool_enum_uint8  from_ap,
    oal_gfp_enum_uint8   en_gfp)
{
    oal_netbuf_stru   *pst_msg = OAL_PTR_NULL;
    oal_void          *p_hdr  = OAL_PTR_NULL;
    oal_int32          ul_let;

    pst_msg = oal_nlmsg_new(OAL_NLMSG_GOODSIZE, en_gfp);
    if (pst_msg == OAL_PTR_NULL) {
        return OAL_ERR_CODE_CFG80211_ENOBUFS;
    }

    p_hdr = oal_genlmsg_put(pst_msg, 0, 0, &nl80211_fam, 0, PRIV_NL80211_CMD_DISCONNECT);
    if (p_hdr == OAL_PTR_NULL) {
        oal_nlmsg_free(pst_msg);
        return OAL_ERR_CODE_CFG80211_SKB_MEM_FAIL;
    }

    /*
        ��������ϲ�û�н��������Բ��ϱ�
    */
    OAL_NLA_PUT_U32(pst_msg, PRIV_NL80211_ATTR_IFINDEX, pst_net_device->ifindex);
    if (from_ap && us_reason)
        OAL_NLA_PUT_U16(pst_msg, PRIV_NL80211_ATTR_REASON_CODE, us_reason);
    if (from_ap)
        OAL_NLA_PUT_FLAG(pst_msg, PRIV_NL80211_ATTR_DISCONNECTED_BY_AP);
    if (puc_ie == OAL_PTR_NULL)
        OAL_NLA_PUT(pst_msg, PRIV_NL80211_ATTR_IE, ul_ie_len, puc_ie);

    if (oal_genlmsg_end(pst_msg, p_hdr) < 0) {
        oal_nlmsg_free(pst_msg);
        return OAL_ERR_CODE_CFG80211_ENOBUFS;
    }

    ul_let = oal_genlmsg_multicast(pst_msg, 0, nl80211_mlme_mcgrp.id, en_gfp);
    if (ul_let < 0) {
        return OAL_FAIL;
    }

    return OAL_SUCC;

nla_put_failure:
    oal_genlmsg_cancel(pst_msg, p_hdr);
    oal_nlmsg_free(pst_msg);
    return OAL_ERR_CODE_CFG80211_EMSGSIZE;
}
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,35)) && (_PRE_OS_VERSION_LITEOS != _PRE_OS_VERSION)
OAL_STATIC oal_uint32  oal_nl80211_send_mgmt(
    oal_cfg80211_registered_device_stru *pst_rdev,
    oal_net_device_stru *pst_netdev,
    oal_int32 l_freq, const oal_uint8 *puc_buf,
    oal_uint32 ul_len, oal_gfp_enum_uint8 en_gfp)
{
    oal_netbuf_stru   *pst_msg = OAL_PTR_NULL;
    oal_void          *p_hdr   = OAL_PTR_NULL;
    oal_int32          l_let;

    pst_msg = oal_nlmsg_new(OAL_NLMSG_DEFAULT_SIZE, en_gfp);
    if (pst_msg == OAL_PTR_NULL) {
        return OAL_ERR_CODE_CFG80211_ENOBUFS;
    }

    p_hdr = oal_genlmsg_put(pst_msg, 0, 0, &nl80211_fam, 0, PRIV_NL80211_CMD_ACTION);
    if (p_hdr == OAL_PTR_NULL) {
        oal_nlmsg_free(pst_msg);
        return OAL_ERR_CODE_CFG80211_SKB_MEM_FAIL;
    }

    OAL_NLA_PUT_U32(pst_msg, PRIV_NL80211_ATTR_WIPHY, pst_rdev->wiphy_idx);
    OAL_NLA_PUT_U32(pst_msg, PRIV_NL80211_ATTR_IFINDEX, pst_netdev->ifindex);
    OAL_NLA_PUT_U32(pst_msg, PRIV_NL80211_ATTR_WIPHY_FREQ, l_freq);
    OAL_NLA_PUT(pst_msg, PRIV_NL80211_ATTR_FRAME, ul_len, puc_buf);

    if (oal_genlmsg_end(pst_msg, p_hdr) < 0) {
        oal_nlmsg_free(pst_msg);
        return OAL_ERR_CODE_CFG80211_ENOBUFS;
    }

    l_let = oal_genlmsg_multicast(pst_msg, 0, nl80211_mlme_mcgrp.id, en_gfp);
    if (l_let < 0) {
        return OAL_FAIL;
    }

    return OAL_SUCC;

nla_put_failure:
    oal_genlmsg_cancel(pst_msg, p_hdr);
    oal_nlmsg_free(pst_msg);

    return OAL_ERR_CODE_CFG80211_EMSGSIZE;
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,35)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
oal_void  oal_cfg80211_sched_scan_result(oal_wiphy_stru *pst_wiphy)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0))
    cfg80211_sched_scan_results(pst_wiphy, 0);
    return;
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 44))
    cfg80211_sched_scan_results(pst_wiphy);
    return;
#else
    /* 51��֧�֣�do nothing */
    return;
#endif // (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0))
#endif // (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
}
#else
oal_void  oal_cfg80211_sched_scan_result(oal_wiphy_stru *pst_wiphy)
{
    /* 51��֧�֣�do nothing */
    return;
}
#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
oal_void oal_kobject_uevent_env_sta_join(oal_net_device_stru *pst_net_device, const oal_uint8 *puc_mac_addr)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    memset_s(&env, sizeof(env), 0, sizeof(env));
    /* Android�ϲ���ҪSTA_JOIN��mac��ַ���м������Ч�����Ǳ�����4������ */
    add_uevent_var(&env, "SOFTAP=STA_JOIN wlan0 wlan0 %02x:%02x:%02x:%02x:%02x:%02x",
                   puc_mac_addr[0],
                   puc_mac_addr[1],
                   puc_mac_addr[2],
                   puc_mac_addr[3],
                   puc_mac_addr[4],
                   puc_mac_addr[5]);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,1,0))
    kobject_uevent_env(&(pst_net_device->dev.kobj), KOBJ_CHANGE, env.envp);
#else
    kobject_uevent_env(&(pst_net_device->dev.kobj), KOBJ_CHANGE, (char**)&env);
#endif
#endif /* #if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) */
}

oal_void oal_kobject_uevent_env_sta_leave(oal_net_device_stru *pst_net_device,
                                          const oal_uint8 *puc_mac_addr)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    memset_s(&env, sizeof(env), 0, sizeof(env));
    /* Android�ϲ���ҪSTA_LEAVE��mac��ַ���м������Ч�����Ǳ�����4������ */
    add_uevent_var(&env, "SOFTAP=STA_LEAVE wlan0 wlan0 %02x:%02x:%02x:%02x:%02x:%02x",
                   puc_mac_addr[0],
                   puc_mac_addr[1],
                   puc_mac_addr[2],
                   puc_mac_addr[3],
                   puc_mac_addr[4],
                   puc_mac_addr[5]);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,1,0))
    kobject_uevent_env(&(pst_net_device->dev.kobj), KOBJ_CHANGE, env.envp);
#else
    kobject_uevent_env(&(pst_net_device->dev.kobj), KOBJ_CHANGE, (char**)&env);
#endif
#endif /* #if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) */
}
#endif

oal_void  oal_cfg80211_put_bss(oal_wiphy_stru *pst_wiphy, oal_cfg80211_bss_stru *pst_cfg80211_bss)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,44))
    /* cfg80211_put_bss(struct wiphy *wiphy, struct cfg80211_bss *pub) */
    cfg80211_put_bss(pst_wiphy, pst_cfg80211_bss);
#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    /* liteos has no cfg80211_put_bss */
#else
    return cfg80211_put_bss(pst_cfg80211_bss);
#endif
}

/*
 * ��������  : ����bssid ��ssid �����ں˱����bss ��Ϣ
 */
oal_cfg80211_bss_stru *oal_cfg80211_get_bss(oal_wiphy_stru *wiphy,
                                            oal_ieee80211_channel_stru *channel,
                                            uint8_t *bssid,
                                            uint8_t *ssid,
                                            uint32_t ssid_len)
{
#if (KERNEL_VERSION(4, 1, 0) <= LINUX_VERSION_CODE)
    return cfg80211_get_bss(wiphy, channel, bssid, ssid, ssid_len,
                            IEEE80211_BSS_TYPE_ANY, IEEE80211_PRIVACY_ANY);
#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    /* liteos has no oal_cfg80211_get_bss */
#else
    return cfg80211_get_bss(wiphy, channel, bssid, ssid, ssid_len,
                            WLAN_CAPABILITY_ESS, WLAN_CAPABILITY_ESS);
#endif
}

/*
 * ��������  : ��kernelɾ��bss entry��
 */
void oal_cfg80211_unlink_bss(oal_wiphy_stru *wiphy, oal_cfg80211_bss_stru *cfg80211_bss)
{
#if (KERNEL_VERSION(4, 1, 0) <= LINUX_VERSION_CODE)
    cfg80211_unlink_bss(wiphy, cfg80211_bss);
#endif
}

oal_cfg80211_bss_stru *oal_cfg80211_inform_bss_frame(
    oal_wiphy_stru              *pst_wiphy,
    oal_ieee80211_channel_stru  *pst_ieee80211_channel,
    oal_ieee80211_mgmt_stru     *pst_mgmt,
    oal_uint32                   ul_len,
    oal_int32                    l_signal,
    oal_gfp_enum_uint8           en_ftp)
{
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    /* liteos has no cfg80211_put_bss */
    cfg80211_inform_bss_frame(pst_wiphy, pst_ieee80211_channel, pst_mgmt, ul_len, l_signal, en_ftp);
    return OAL_PTR_NULL;

#else
    return cfg80211_inform_bss_frame(pst_wiphy, pst_ieee80211_channel, pst_mgmt, ul_len, l_signal, en_ftp);

#endif
}

oal_void  oal_cfg80211_scan_done(oal_cfg80211_scan_request_stru *pst_cfg80211_scan_request, oal_int8 c_aborted)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,8,0))
    struct cfg80211_scan_info info = {0};
    info.aborted = c_aborted;
    cfg80211_scan_done(pst_cfg80211_scan_request, &info);
#else
    cfg80211_scan_done(pst_cfg80211_scan_request, c_aborted);
#endif
}

oal_uint32  oal_cfg80211_connect_result(
    oal_net_device_stru         *pst_net_device,
    const oal_uint8             *puc_bssid,
    const oal_uint8             *puc_req_ie,
    oal_uint32             ul_req_ie_len,
    const oal_uint8             *puc_resp_ie,
    oal_uint32             ul_resp_ie_len,
    oal_uint16             us_status,
    oal_gfp_enum_uint8           en_gfp)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)

    cfg80211_connect_result(pst_net_device, puc_bssid, puc_req_ie, ul_req_ie_len,
                            puc_resp_ie, ul_resp_ie_len, us_status, en_gfp);

    return OAL_SUCC;
#else
    oal_netbuf_stru         *pst_msg  = OAL_PTR_NULL;
    oal_wireless_dev_stru   *pst_wdev = OAL_PTR_NULL;

    /* ����һ���µ�netlink��Ϣ */
    pst_msg = oal_nlmsg_new(OAL_NLMSG_GOODSIZE, en_gfp);
    if (pst_msg == OAL_PTR_NULL) {
        return OAL_ERR_CODE_CFG80211_ENOBUFS;
    }

    pst_wdev = pst_net_device->ieee80211_ptr;
    if (pst_wdev == OAL_PTR_NULL) {
        oal_nlmsg_free(pst_msg);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_wdev->iftype != NL80211_IFTYPE_STATION) {
        oal_nlmsg_free(pst_msg);
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    /* ���������hostapd��wpa_supplicant�Ļ������Ҳ��ʧ�ܣ����ﱨfail��Ӱ��ʹ�ã�ȥ������
        �ɹ��Ļ�����ӡSUCC, ���ɹ��Ļ�������ӡ */
    return oal_nl80211_send_connect_result(pst_msg, pst_net_device, puc_bssid, puc_req_ie, ul_req_ie_len,
                                           puc_resp_ie, ul_resp_ie_len, us_status, en_gfp);
#endif
}

oal_uint32  oal_cfg80211_disconnected(
    oal_net_device_stru        *pst_net_device,
    oal_uint16                  us_reason,
    oal_uint8                  *puc_ie,
    oal_uint32                  ul_ie_len,
    oal_gfp_enum_uint8          en_gfp)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,0))
    cfg80211_disconnected(pst_net_device, us_reason, puc_ie, ul_ie_len, 0, en_gfp);
#else
    cfg80211_disconnected(pst_net_device, us_reason, puc_ie, ul_ie_len, en_gfp);
#endif
    return OAL_SUCC;
#else
    oal_wireless_dev_stru   *pst_wdev = OAL_PTR_NULL;

    pst_wdev = pst_net_device->ieee80211_ptr;
    if (pst_wdev == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_wdev->iftype != NL80211_IFTYPE_STATION) {
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    /*
     * If this disconnect was due to a disassoc, we
     * we might still have an auth BSS around. For
     * the userspace SME that's currently expected,
     * but for the kernel SME (nl80211 CONNECT or
     * wireless extensions) we want to clear up all
     * state.
     */
    return oal_nl80211_send_disconnected(pst_net_device, us_reason, puc_ie,
                                         ul_ie_len, OAL_TRUE, en_gfp);
#endif
}

oal_uint32  oal_cfg80211_roamed(
    oal_net_device_stru         *pst_net_device,
    struct ieee80211_channel    *pst_channel,
    const oal_uint8             *puc_bssid,
    const oal_uint8             *puc_req_ie,
    oal_uint32             ul_req_ie_len,
    const oal_uint8             *puc_resp_ie,
    oal_uint32             ul_resp_ie_len,
    oal_gfp_enum_uint8           en_gfp)
{
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    return OAL_SUCC;
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0))
    struct cfg80211_roam_info info = {0};
    info.channel = pst_channel;
    info.bssid = puc_bssid;
    info.req_ie = puc_req_ie;
    info.req_ie_len = ul_req_ie_len;
    info.resp_ie = puc_resp_ie;
    info.resp_ie_len = ul_resp_ie_len;
    cfg80211_roamed(pst_net_device, &info, en_gfp);
    return OAL_SUCC;
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34))
    cfg80211_roamed(pst_net_device, pst_channel, puc_bssid,
                    puc_req_ie, ul_req_ie_len,
                    puc_resp_ie, ul_resp_ie_len, en_gfp);

    return OAL_SUCC;

#else
    return OAL_ERR_CODE_CONFIG_UNSUPPORT;
#endif
}

oal_uint32  oal_cfg80211_ft_event(oal_net_device_stru *pst_net_device, oal_cfg80211_ft_event_stru  *pst_ft_event)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 44))
    cfg80211_ft_event(pst_net_device, pst_ft_event);
#endif
    return OAL_SUCC;
#endif
    return OAL_ERR_CODE_CONFIG_UNSUPPORT;//lint !e527
#else
    return OAL_ERR_CODE_CONFIG_UNSUPPORT;
#endif
}

oal_uint32  oal_cfg80211_new_sta(
    oal_net_device_stru     *pst_net_device,
    const oal_uint8         *puc_mac_addr,
    oal_station_info_stru   *pst_station_info,
    oal_gfp_enum_uint8      en_gfp)
{
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
    oal_kobject_uevent_env_sta_join(pst_net_device, puc_mac_addr);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    cfg80211_new_sta(pst_net_device, puc_mac_addr, pst_station_info, en_gfp);

    return OAL_SUCC;
#else
    oal_netbuf_stru   *pst_msg = OAL_PTR_NULL;
    oal_int32          l_let;

    /* ����һ���µ�netlink��Ϣ */
    pst_msg = oal_nlmsg_new(OAL_NLMSG_GOODSIZE, en_gfp);
    if (pst_msg == OAL_PTR_NULL) {
        return OAL_ERR_CODE_CFG80211_ENOBUFS;
    }

    l_let = oal_nl80211_send_station(pst_msg, 0, 0, 0, pst_net_device, puc_mac_addr, pst_station_info);
    if (l_let != OAL_SUCC) {
        oal_nlmsg_free(pst_msg);
        return l_let;
    }

    /* ���÷�װ���ں�netlink�㲥���ͺ��������ͳɹ�����0��ʧ��Ϊ��ֵ */
    l_let = oal_genlmsg_multicast(pst_msg, 0, nl80211_mlme_mcgrp.id, en_gfp);
    if (l_let < 0) {
        return OAL_FAIL;
    }

    return OAL_SUCC;
#endif
}

oal_void  oal_cfg80211_mic_failure(
    oal_net_device_stru     *pst_net_device,
    const oal_uint8         *puc_mac_addr,
    enum nl80211_key_type    key_type,
    oal_int32                key_id,
    const oal_uint8         *puc_tsc,
    oal_gfp_enum_uint8       en_gfp)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    cfg80211_michael_mic_failure(pst_net_device, puc_mac_addr, key_type, key_id, puc_tsc, en_gfp);
#endif
#else
    oal_wireless_dev_stru               *pst_wdev  = OAL_PTR_NULL;
    oal_cfg80211_registered_device_stru *pst_rdev  = OAL_PTR_NULL;
    oal_netbuf_stru                     *pst_msg   = OAL_PTR_NULL;
    oal_void                            *p_hdr     = OAL_PTR_NULL;
    oal_int32                            l_let;

    pst_wdev = pst_net_device->ieee80211_ptr;
    if (pst_wdev == OAL_PTR_NULL) {
        return;
    }

    if (pst_wdev->wiphy == OAL_PTR_NULL) {
        return;
    }
    pst_rdev = oal_wiphy_to_dev(pst_wdev->wiphy);

    pst_msg = oal_nlmsg_new(OAL_NLMSG_DEFAULT_SIZE, en_gfp);
    if (pst_msg == OAL_PTR_NULL) {
        return;
    }

    p_hdr = oal_genlmsg_put(pst_msg, 0, 0, &nl80211_fam, 0, PRIV_NL80211_CMD_MICHAEL_MIC_FAILURE);
    if (p_hdr == OAL_PTR_NULL) {
        oal_nlmsg_free(pst_msg);
        return;
    }

    /* rdev��Ӧ�ں�core.h�е�cfg80211_registered_device�ṹ�壬����������ϲ�û�д��� */
    OAL_NLA_PUT_U32(pst_msg, PRIV_NL80211_ATTR_WIPHY, pst_rdev->wiphy_idx);
    OAL_NLA_PUT_U32(pst_msg, PRIV_NL80211_ATTR_IFINDEX, pst_net_device->ifindex);

    if (puc_mac_addr != OAL_PTR_NULL) {
        OAL_NLA_PUT(pst_msg, PRIV_NL80211_ATTR_MAC, OAL_ETH_ALEN_SIZE, puc_mac_addr);
    }
    OAL_NLA_PUT_U32(pst_msg, PRIV_NL80211_ATTR_KEY_TYPE, key_type);
    OAL_NLA_PUT_U8(pst_msg, PRIV_NL80211_ATTR_KEY_IDX, key_id);
    if (puc_tsc != OAL_PTR_NULL) {
        OAL_NLA_PUT(pst_msg, PRIV_NL80211_ATTR_KEY_SEQ, 6, puc_tsc);
    }

    if (oal_genlmsg_end(pst_msg, p_hdr) < 0) {
        oal_nlmsg_free(pst_msg);
        return;
    }

    l_let = oal_genlmsg_multicast(pst_msg, 0, nl80211_mlme_mcgrp.id, en_gfp);
    if (l_let < 0) {
        return;
    }
    return;
nla_put_failure:
    oal_genlmsg_cancel(pst_msg, p_hdr);
    oal_nlmsg_free(pst_msg);
    return;
#endif
}

oal_uint32  oal_cfg80211_del_sta(oal_net_device_stru *pst_net_device,
                                 const oal_uint8      *puc_mac_addr,
                                 oal_gfp_enum_uint8    en_gfp)
{
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
    oal_kobject_uevent_env_sta_leave(pst_net_device, puc_mac_addr);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,35)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    cfg80211_del_sta(pst_net_device, puc_mac_addr, en_gfp);

    return OAL_SUCC;
#else

    /*
        yaorui  �ں�2.6.34��û���ں�3.x�汾�� cfg80211_del_sta�������
        linux-2.6.30��linux-2.6.34ʵ�ַ�ʽ����ͳһ��Ŀǰ34�ں��Դ�׮����Ȼ��Ҫ�޸�5115�ں˴���
    */
    oal_netbuf_stru  *pst_msg = OAL_PTR_NULL;
    oal_void         *p_hdr   = OAL_PTR_NULL;
    oal_int32         l_let;

    pst_msg = oal_nlmsg_new(OAL_NLMSG_DEFAULT_SIZE, en_gfp);
    if (pst_msg == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    p_hdr = oal_genlmsg_put(pst_msg, 0, 0, &nl80211_fam, 0, PRIV_NL80211_CMD_DEL_STATION);
    if (p_hdr == OAL_PTR_NULL) {
        oal_nlmsg_free(pst_msg);
        return OAL_ERR_CODE_CFG80211_SKB_MEM_FAIL;
    }

    if (oal_nla_put_u32(pst_msg, PRIV_NL80211_ATTR_IFINDEX, pst_net_device->ifindex) ||
       oal_nla_put(pst_msg, PRIV_NL80211_ATTR_MAC, OAL_ETH_ALEN_SIZE, puc_mac_addr)) {
        oal_genlmsg_cancel(pst_msg, p_hdr);
        oal_nlmsg_free(pst_msg);
        return OAL_ERR_CODE_CFG80211_EMSGSIZE;
    }

    if (oal_genlmsg_end(pst_msg, p_hdr) < 0) {
        oal_nlmsg_free(pst_msg);
        return OAL_ERR_CODE_CFG80211_ENOBUFS;
    }

    /*
        liuux-2.6.30��liuux-2.6.34�ں˶��Ǵ����������,���ܴﵽҪ��
        linux-2.6.34�ں˽��ŵ���genlmsg_multicast_netns(&init_net......)
        linux-2.6.30�ں˽��ŵ���nlmsg_multicast(genl_sock......)
    */
    l_let = oal_genlmsg_multicast(pst_msg, 0, nl80211_mlme_mcgrp.id, en_gfp);
    if (l_let < 0) {
        return OAL_FAIL;
    }
#endif

    return OAL_SUCC;//lint !e527
}

oal_uint32 oal_cfg80211_rx_mgmt(oal_net_device_stru *pst_dev,
                                oal_int32               l_freq,
                                const oal_uint8        *puc_buf,
                                oal_uint32              ul_len,
                                oal_gfp_enum_uint8      en_gfp)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,35)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    oal_wireless_dev_stru   *pst_wdev = OAL_PTR_NULL;
    oal_uint32               ul_ret;
    oal_bool_enum_uint8      uc_ret;
    pst_wdev = pst_dev->ieee80211_ptr;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,4,35)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    uc_ret   = cfg80211_rx_mgmt(pst_wdev, l_freq, 0, puc_buf, ul_len, en_gfp);
#else
    uc_ret   = cfg80211_rx_mgmt(pst_dev, l_freq, 0, puc_buf, ul_len, en_gfp);
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    uc_ret = OAL_TRUE;
#endif
    (uc_ret == OAL_TRUE) ? (ul_ret = OAL_SUCC) : (ul_ret = OAL_FAIL);
    return ul_ret;
#else
    oal_wireless_dev_stru               *pst_wdev = OAL_PTR_NULL;
    oal_cfg80211_registered_device_stru *pst_rdev = OAL_PTR_NULL;
    const oal_uint8                     *action_data = OAL_PTR_NULL;

    pst_wdev = pst_dev->ieee80211_ptr;
    if (pst_wdev == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_rdev = oal_wiphy_to_dev(pst_wdev->wiphy);

    /* action data starts with category */
    action_data = puc_buf + OAL_IEEE80211_MIN_ACTION_SIZE - 1;

    return oal_nl80211_send_mgmt(pst_rdev, pst_dev, l_freq, puc_buf, ul_len, en_gfp);
#endif
}

uint32_t oal_cfg80211_rx_exception(oal_net_device_stru *netdev,
    uint8_t *data, uint32_t data_len)
{
#if(_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    cfg80211_rx_exception(netdev, data, data_len);
#else
    dev_netlink_send (data, data_len);
#endif
    OAL_IO_PRINT("DFR OAL send[%s] over\n", data);
    return OAL_SUCC;
}

int32_t oal_cfg80211_external_auth_request(oal_net_device_stru *pst_netdev,
    struct cfg80211_external_auth_params *pst_params, oal_gfp_enum_uint8 en_gfp)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
    return cfg80211_external_auth_request(pst_netdev, pst_params, en_gfp);
#else
    return -OAL_EFAIL;
#endif
}
#elif (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
oal_void  oal_cfg80211_put_bss(oal_wiphy_stru *pst_wiphy, oal_cfg80211_bss_stru *pst_cfg80211_bss)
{
}
oal_cfg80211_bss_stru *oal_cfg80211_get_bss(oal_wiphy_stru *wiphy,
                                            oal_ieee80211_channel_stru *channel,
                                            uint8_t *bssid,
                                            uint8_t *ssid,
                                            uint32_t ssid_len)
{
    return (oal_cfg80211_bss_stru *)NULL;
}

void oal_cfg80211_unlink_bss(oal_wiphy_stru *wiphy, oal_cfg80211_bss_stru *cfg80211_bss)
{
}

oal_cfg80211_bss_stru *oal_cfg80211_inform_bss_frame(
    oal_wiphy_stru              *pst_wiphy,
    oal_ieee80211_channel_stru  *pst_ieee80211_channel,
    oal_ieee80211_mgmt_stru     *pst_mgmt,
    oal_uint32                   ul_len,
    oal_int32                    l_signal,
    oal_gfp_enum_uint8           en_ftp)
{
    oal_cfg80211_bss_stru  *pst_cfg80211_bss;

    pst_cfg80211_bss = OAL_PTR_NULL;

    return pst_cfg80211_bss;
}

oal_void  oal_cfg80211_scan_done(oal_cfg80211_scan_request_stru *pst_cfg80211_scan_req, oal_int8 c_aborted)
{
}

oal_void  oal_cfg80211_sched_scan_result(oal_wiphy_stru *pst_wiphy)
{
    return;
}

oal_uint32  oal_cfg80211_connect_result(
    oal_net_device_stru         *pst_net_device,
    const oal_uint8             *puc_bssid,
    const oal_uint8             *puc_req_ie,
    oal_uint32             ul_req_ie_len,
    const oal_uint8             *puc_resp_ie,
    oal_uint32             ul_resp_ie_len,
    oal_uint16             us_status,
    oal_gfp_enum_uint8     en_gfp)
{
    return OAL_SUCC;
}

oal_uint32  oal_cfg80211_roamed(
    oal_net_device_stru         *pst_net_device,
    struct ieee80211_channel    *pst_channel,
    const oal_uint8             *puc_bssid,
    const oal_uint8             *puc_req_ie,
    oal_uint32             ul_req_ie_len,
    const oal_uint8             *puc_resp_ie,
    oal_uint32             ul_resp_ie_len,
    oal_gfp_enum_uint8           en_gfp)
{
    return OAL_SUCC;
}

oal_uint32  oal_cfg80211_ft_event(oal_net_device_stru *pst_net_device, oal_cfg80211_ft_event_stru  *pst_ft_event)
{
    return OAL_SUCC;
}

oal_uint32  oal_cfg80211_disconnected(
    oal_net_device_stru        *pst_net_device,
    oal_uint16                  us_reason,
    oal_uint8                   *puc_ie,
    oal_uint32                  ul_ie_len,
    oal_gfp_enum_uint8          en_gfp)
{
    return OAL_SUCC;
}

oal_uint32  oal_cfg80211_new_sta(
    oal_net_device_stru     *pst_net_device,
    const oal_uint8         *puc_mac_addr,
    oal_station_info_stru   *pst_station_info,
    oal_gfp_enum_uint8      en_gfp)
{
    return OAL_SUCC;
}

oal_void  oal_cfg80211_mic_failure(
    oal_net_device_stru     *pst_net_device,
    const oal_uint8         *puc_mac_addr,
    enum nl80211_key_type    key_type,
    oal_int32                key_id,
    const oal_uint8         *tsc,
    oal_gfp_enum_uint8       en_gfp)
{
    /* do nothing */
}

oal_uint32  oal_cfg80211_del_sta(oal_net_device_stru *pst_net_device,
                                 const oal_uint8      *puc_mac_addr,
                                 oal_gfp_enum_uint8    en_gfp)
{
    return OAL_SUCC;
}

oal_uint32 oal_cfg80211_rx_mgmt(oal_net_device_stru   *pst_dev,
                                oal_int32              l_freq,
                                const oal_uint8       *puc_buf,
                                oal_uint32             ul_len,
                                oal_gfp_enum_uint8     en_gfp)
{
    return OAL_SUCC;
}

uint32_t oal_cfg80211_rx_exception(oal_net_device_stru *netdev,
    uint8_t *data, uint32_t data_len)
{
    OAL_IO_PRINT("DFR report excp to APP!!");
    return OAL_SUCC;
}

/* �ϱ��ں�external_auth �¼� */
int32_t oal_cfg80211_external_auth_request(oal_net_device_stru *netdev,
    struct cfg80211_external_auth_params *params, oal_gfp_enum_uint8 gfp)
{
    return OAL_SUCC;
}
#endif

/*lint -e578*//*lint -e19*/
oal_module_symbol(oal_cfg80211_vendor_cmd_alloc_reply_skb);
oal_module_symbol(oal_cfg80211_vendor_cmd_reply);
oal_module_symbol(oal_cfg80211_put_bss);
oal_module_symbol(oal_cfg80211_get_bss);
oal_module_symbol(oal_cfg80211_unlink_bss);
oal_module_symbol(oal_cfg80211_inform_bss_frame);
oal_module_symbol(oal_cfg80211_scan_done);
oal_module_symbol(oal_cfg80211_sched_scan_result);
oal_module_symbol(oal_cfg80211_connect_result);
oal_module_symbol(oal_cfg80211_roamed);
oal_module_symbol(oal_cfg80211_ft_event);
oal_module_symbol(oal_cfg80211_disconnected);
oal_module_symbol(oal_cfg80211_new_sta);
oal_module_symbol(oal_cfg80211_mic_failure);
oal_module_symbol(oal_cfg80211_del_sta);
oal_module_symbol(oal_cfg80211_rx_mgmt);
oal_module_symbol(oal_cfg80211_mgmt_tx_status);
oal_module_symbol(oal_cfg80211_ready_on_channel);
oal_module_symbol(oal_cfg80211_remain_on_channel_expired);
oal_module_symbol(oal_cfg80211_external_auth_request);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)||(_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
oal_module_symbol(oal_kobject_uevent_env_sta_join);
oal_module_symbol(oal_kobject_uevent_env_sta_leave);
#endif
#endif

oal_module_symbol(oal_cfg80211_rx_exception);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

