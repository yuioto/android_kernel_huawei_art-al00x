
#ifdef _PRE_WLAN_FEATURE_APF
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#define OUI_VENDOR 0x001A11
#define OUI_HISI   0x001018

#include "wal_linux_cfgvendor.h"
#include "oal_cfg80211.h"
#include "oam_ext_if.h"
#include "mac_device.h"
#include "wal_config.h"
#include "wal_linux_ioctl.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_LINUX_CFGVENDOR_C

/*
 * 功能描述 : 获取apf能力
 * 修改历史 :
 * 1.日    期 : 2020年3月9日
 *   修改内容 : 新增函数
 */
OAL_STATIC int32_t wal_cfgvendor_apf_get_capabilities(oal_wiphy_stru *wiphy,
    oal_wireless_dev_stru *wdev, OAL_CONST void *data, int32_t len)
{
    oal_netbuf_stru *skb = OAL_PTR_NULL;
    int32_t ret;
    int32_t mem_needed;

    mem_needed = VENDOR_REPLY_OVERHEAD + (ATTRIBUTE_U32_LEN * 2);

    skb = oal_cfg80211_vendor_cmd_alloc_reply_skb(wiphy, mem_needed);
    if (OAL_UNLIKELY(skb == OAL_PTR_NULL)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_apf_get_capabilities::skb alloc failed, len %d", mem_needed);
        return -OAL_ENOMEM;
    }

    oal_nla_put_u32(skb, APF_ATTRIBUTE_VERSION, APF_VERSION);
    oal_nla_put_u32(skb, APF_ATTRIBUTE_MAX_LEN, APF_PROGRAM_MAX_LEN);

    ret = oal_cfg80211_vendor_cmd_reply(skb);
    if (OAL_UNLIKELY(ret)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_apf_get_capabilities::Vendor Command reply failed, ret:%d.", ret);
    }
    return ret;
}

/*
 * 功能描述 : 设置apf filter
 * 修改历史 :
 * 1.日    期 : 2020年3月9日
 *   修改内容 : 新增函数
 */
OAL_STATIC int32_t wal_cfgvendor_apf_set_filter(oal_wiphy_stru *wiphy,
    oal_wireless_dev_stru *wdev, OAL_CONST void *data, int32_t len)
{
    OAL_CONST oal_nlattr_stru *iter;
    int32_t tmp, type;
    mac_apf_filter_cmd_stru apf_filter_cmd;
    wal_msg_write_stru write_msg;
    wal_msg_stru *rsp_msg = OAL_PTR_NULL;
    uint32_t program_len;

    memset_s(&apf_filter_cmd, sizeof(mac_apf_filter_cmd_stru), 0, sizeof(mac_apf_filter_cmd_stru));

    // assumption: length attribute must come first
    OAL_NLA_FOR_EACH_ATTR(iter, data, len, tmp) {
        type = oal_nla_type(iter);
        switch (type) {
            case APF_ATTRIBUTE_PROGRAM_LEN:
                program_len = oal_nla_get_u32(iter);
                if (OAL_UNLIKELY(!program_len || program_len > APF_PROGRAM_MAX_LEN)) {
                    OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_apf_set_filter::program_len[%d] invalid", program_len);
                    return -OAL_EINVAL;
                }
                apf_filter_cmd.program_len = (uint16_t)program_len;
                break;
            case APF_ATTRIBUTE_PROGRAM:
                if (OAL_UNLIKELY(!apf_filter_cmd.program_len)) {
                    OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_cfgvendor_apf_set_filter::program len not set");
                    return -OAL_EINVAL;
                }

                apf_filter_cmd.program = (uint8_t *)oal_nla_data(iter);
                apf_filter_cmd.en_cmd_type = MAC_APF_SET_FILTER_CMD;

                // 抛事件到wal层处理
                WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_SET_APF_FILTER, sizeof(apf_filter_cmd));
                if (memcpy_s(write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN,
                    &apf_filter_cmd, sizeof(apf_filter_cmd)) != EOK) {
                    OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_cfgvendor_apf_set_filter::memcpy fail!");
                    return -OAL_EFAIL;
                }

                // 消息发送，需要将发送该函数设置为同步，否则hmac处理时会使用已释放的内存
                if (wal_send_cfg_event(wdev->netdev, WAL_MSG_TYPE_WRITE,
                                       WAL_MSG_WRITE_MSG_HDR_LENGTH + sizeof(apf_filter_cmd),
                                       (uint8_t *)&write_msg, OAL_TRUE, &rsp_msg) != OAL_SUCC) {
                    OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_cfgvendor_apf_set_filter::wal_send_cfg_event fail!}");
                    return -OAL_EFAIL;
                }
                if (wal_check_and_release_msg_resp(rsp_msg) != OAL_SUCC) {
                    OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_cfgvendor_apf_set_filter::check response msg error!}");
                    return -OAL_EFAIL;
                }
                break;
            default:
                OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_apf_set_filter::no such attribute %d", type);
                return -OAL_EINVAL;
        }
    }

    return OAL_SUCC;
}

OAL_STATIC OAL_CONST oal_wiphy_vendor_command_stru wal_vendor_cmds[] = {
    {
        {
            .vendor_id = OUI_VENDOR,
            .subcmd = APF_SUBCMD_GET_CAPABILITIES
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit  = wal_cfgvendor_apf_get_capabilities
    }, {
        {
            .vendor_id = OUI_VENDOR,
            .subcmd = APF_SUBCMD_SET_FILTER
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit  = wal_cfgvendor_apf_set_filter
    }
};

void wal_cfgvendor_init(oal_wiphy_stru *wiphy)
{
    wiphy->vendor_commands = wal_vendor_cmds;
    wiphy->n_vendor_commands = OAL_ARRAY_SIZE(wal_vendor_cmds);
}

void wal_cfgvendor_deinit(oal_wiphy_stru *wiphy)
{
    wiphy->vendor_commands = NULL;
    wiphy->n_vendor_commands = 0;
}
#endif // (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#endif // _PRE_WLAN_FEATURE_APF

