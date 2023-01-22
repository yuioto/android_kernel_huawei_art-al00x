
#ifndef __OAL_LINUX_CFG80211_H__
#define __OAL_LINUX_CFG80211_H__

#include <net/genetlink.h>
#include <net/cfg80211.h>
#include <linux/nl80211.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/* hostapd和supplicant事件上报需要用到宏 */
#define OAL_NLMSG_GOODSIZE      NLMSG_GOODSIZE
#define OAL_ETH_ALEN_SIZE            ETH_ALEN
#define OAL_NLMSG_DEFAULT_SIZE  NLMSG_DEFAULT_SIZE
#define OAL_IEEE80211_MIN_ACTION_SIZE IEEE80211_MIN_ACTION_SIZE

#define OAL_NLA_PUT_U32(skb, attrtype, value)      NLA_PUT_U32(skb, attrtype, value)
#define OAL_NLA_PUT(skb, attrtype, attrlen, data)  NLA_PUT(skb, attrtype, attrlen, data)
#define OAL_NLA_PUT_U16(skb, attrtype, value)      NLA_PUT_U16(skb, attrtype, value)
#define OAL_NLA_PUT_U8(skb, attrtype, value)       NLA_PUT_U8(skb, attrtype, value)
#define OAL_NLA_PUT_FLAG(skb, attrtype)            NLA_PUT_FLAG(skb, attrtype)

typedef enum  rate_info_flags  oal_rate_info_flags;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0))
struct cfg80211_external_auth_params {
    enum nl80211_external_auth_action action;
    uint8_t                           bssid[OAL_ETH_ALEN_SIZE];
    oal_cfg80211_ssid_stru            ssid;
    unsigned int                      key_mgmt_suite;
    uint16_t                          status;
};
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)) */
oal_netbuf_stru *oal_cfg80211_vendor_cmd_alloc_reply_skb(oal_wiphy_stru *wiphy, uint32_t len);
int32_t oal_cfg80211_vendor_cmd_reply(oal_netbuf_stru *skb_buff);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_cfg80211.h */
