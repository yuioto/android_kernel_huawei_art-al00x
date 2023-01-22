

//  1 头文件包含
#include "wal_app.h"
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include <net/genetlink.h>
#endif
#include "wal_linux_ioctl.h"
#include "wal_config.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

// 2 宏定义
#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_APP_C

#define MSG_LEN(pst_msg) (((wal_app_msg *)(pst_msg))->st_msg_hdr.ul_data_len + OAL_SIZEOF(wal_app_msg_hdr))
#define WAL_GENLINK_NAME "APP_MSG"

// 2 函数声明
#ifdef _PRE_WLAN_FEATURE_HILINK
oal_int32 wal_hilink_set_monitor(const oal_uint8 *puc_netdev_name, wal_monitor_info_stru *pst_monitor_info);
oal_int32 wal_hilink_set_channel(const oal_uint8 *puc_netdev_name, mac_cfg_channel_param_stru *pst_channel_info);
#endif
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_int32 wal_genetlink_recv_msg(struct sk_buff* pst_skb2, struct genl_info* pst_info);
oal_int32 wal_hilink_recv_msg(oal_uint8 *puc_data);
#endif

// 2 全局变量定义
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_int32 g_aul_app_pid[WAL_OPS_BUTT] = { 0 }; // 记录应用程序的ID

// 定义属性:包含字符串结束标志  参见net/netlink.h
static struct nla_policy wal_genl_policy[WAL_ATTR_BUTT] = {
    [WAL_ATTR_MSG] = {.type = NLA_BINARY},
};

// 定义options
static struct genl_ops wal_genl_ops[] = {
    {
        .cmd    = WAL_OPS_HISILINK,
        .flags  = 0,
        .policy = wal_genl_policy,
        .doit   = wal_genetlink_recv_msg,
        .dumpit = NULL,
    },
    {
        .cmd    = WAL_OPS_WPA,
        .flags  = 0,
        .policy = wal_genl_policy,
        .doit   = wal_genetlink_recv_msg,
        .dumpit = NULL,
    },
};

// 定义family
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
static struct genl_family wal_genl_family = {
    .hdrsize = 0,
    .name = WAL_GENLINK_NAME,
    .version = 1,
    .module     = THIS_MODULE,
    .maxattr = WAL_ATTR_BUTT,
    .ops = wal_genl_ops,
    .n_ops = ARRAY_SIZE(wal_genl_ops),
};
#else
static struct genl_family wal_genl_family = {
    .id = GENL_ID_GENERATE,
    .hdrsize = 0,
    .name = WAL_GENLINK_NAME,
    .version = 1,
    .maxattr = WAL_ATTR_BUTT,
};
#endif
#endif

// 3 函数实现
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)

oal_int32 wal_genetlink_init(void)
{
    oal_int32 l_ret;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
    l_ret = genl_register_family(&wal_genl_family);
#elif(LINUX_VERSION_CODE < KERNEL_VERSION(3, 18, 0))
    l_ret = genl_register_family_with_ops(&wal_genl_family, wal_genl_ops, ARRAY_SIZE(wal_genl_ops));
#else
    l_ret = genl_register_family_with_ops(&wal_genl_family, wal_genl_ops);
#endif
    if (l_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_APP_MSG, "wal_genetlink_init:genl_register_family_with_ops fail,l_ret=%d", l_ret);
        genl_unregister_family(&wal_genl_family);
        return -OAL_FAIL;
    }
    return OAL_SUCC;
}


void wal_genetlink_exit(void)
{
    oal_int32 l_ret;
    l_ret = genl_unregister_family(&wal_genl_family);
    if (l_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_APP_MSG, "wal_genetlink_exit:genl_unregister_ops fail,l_ret=%d", l_ret);
    }
}


oal_int32 wal_genetlink_recv_msg(struct sk_buff* pst_skb2, struct genl_info* pst_info)
{
    struct nlattr     *pst_na = OAL_PTR_NULL;
    oal_uint8         *puc_data = OAL_PTR_NULL;
    oal_int32          l_ret;

    if (OAL_ANY_NULL_PTR2(pst_skb2, pst_info)) {
        OAM_ERROR_LOG0(0, OAM_SF_APP_MSG, "wal_recv_genetlink_msg:param null}");
        return -OAL_FAIL;
    }
    switch (pst_info->genlhdr->cmd) {
        case WAL_OPS_HISILINK:
#if(LINUX_VERSION_CODE <= KERNEL_VERSION(3, 10, 44))
            g_aul_app_pid[WAL_OPS_HISILINK] = pst_info->snd_pid;
#else
            g_aul_app_pid[WAL_OPS_HISILINK] = pst_info->snd_portid;
#endif
            pst_na = pst_info->attrs[WAL_ATTR_MSG];
            if (pst_na == OAL_PTR_NULL) {
                OAM_ERROR_LOG0(0, OAM_SF_APP_MSG, "wal_recv_genetlink_msg:pst_na is null");
                return -OAL_FAIL;
            }
            puc_data = nla_data(pst_na);
            if (puc_data == OAL_PTR_NULL) {
                OAM_ERROR_LOG0(0, OAM_SF_APP_MSG, "wal_recv_genetlink_msg:puc_data is null");
                return -OAL_FAIL;
            }

            l_ret = wal_hilink_recv_msg(puc_data);
            if (l_ret != OAL_SUCC) {
                OAM_ERROR_LOG1(0, OAM_SF_APP_MSG, "wal_recv_genetlink_msg:l_ret=%d", l_ret);
                return -OAL_FAIL;
            }
            break;
        case WAL_OPS_WPA:
#if(LINUX_VERSION_CODE <= KERNEL_VERSION(3, 10, 44))
            g_aul_app_pid[WAL_OPS_WPA] = pst_info->snd_pid;
#else
            g_aul_app_pid[WAL_OPS_WPA] = pst_info->snd_portid;
#endif
            /* something to do */
            break;
        default:
            OAM_ERROR_LOG1(0, OAM_SF_APP_MSG, "wal_recv_genetlink_msg:cmd[%d] unused", pst_info->genlhdr->cmd);
            return -OAL_FAIL;
    }
    return OAL_SUCC;
}


oal_int32 wal_genetlink_send_msg(oal_uint32 ul_pid, oal_uint16 us_attrtype, oal_uint8 uc_cmd_type,
                                 const oal_uint8 *puc_msg, oal_uint32 ul_len)
{
    oal_int32       l_ret;
    struct sk_buff *pst_skb = OAL_PTR_NULL;
    void           *pst_msg_hdr = OAL_PTR_NULL;

    pst_skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
    if (pst_skb == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_APP_MSG, "wal_send_genetlink_msg:pst_skb is null");
        return OAL_ERR_CODE_PTR_NULL;
    }
    // 开始构建消息
    genlmsg_put(pst_skb, 0, 1, &wal_genl_family, 0, uc_cmd_type);

    // 填充具体要传的数据
    l_ret = nla_put(pst_skb, us_attrtype, ul_len, puc_msg);
    if (l_ret != 0) {
        OAM_ERROR_LOG0(0, OAM_SF_APP_MSG, "wal_send_genetlink_msg:put string fail");
        kfree_skb(pst_skb);
        return -OAL_FAIL;
    }
    pst_msg_hdr = genlmsg_data(nlmsg_data(nlmsg_hdr(pst_skb)));
    // 消息构建完成
    genlmsg_end(pst_skb, pst_msg_hdr);
    // 单播发送给用户空间的某个进程
    l_ret = genlmsg_unicast(&init_net, pst_skb, ul_pid);
    if (l_ret < 0) {
        OAM_WARNING_LOG0(0, OAM_SF_APP_MSG, "wal_send_genetlink_msg:unicast fail");
        kfree_skb(pst_skb);
        return OAL_SUCC;
    }
    kfree_skb(pst_skb);
    return OAL_SUCC;
}


oal_int32 wal_hilink_recv_msg(oal_uint8 *puc_data)
{
    wal_app_msg_hdr            *pst_msg_hdr = OAL_PTR_NULL;
    wal_monitor_info_stru      *pst_monitor_info = OAL_PTR_NULL;
    mac_cfg_channel_param_stru *pst_channel_info = OAL_PTR_NULL;
    int                         l_ret;

    if (puc_data == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_APP_MSG, "puc_data is null");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_msg_hdr = (wal_app_msg_hdr*)puc_data;
    switch (pst_msg_hdr->uc_msg_type) {
        case WAL_CMD_SET_MONITOR:
            pst_monitor_info = (wal_monitor_info_stru *)(puc_data + OAL_SIZEOF(wal_app_msg_hdr));
            l_ret = wal_hilink_set_monitor(pst_msg_hdr->auc_netdev_name, pst_monitor_info);
            if (l_ret != OAL_SUCC) {
                OAM_ERROR_LOG0(0, OAM_SF_APP_MSG, "wal_hilink_recv_msg: WAL_CMD_SET_MONITOR is error");
                return -OAL_FAIL;
            }
            break;
        case WAL_CMD_SET_CHANNEL:
            pst_channel_info = (mac_cfg_channel_param_stru *)(puc_data + OAL_SIZEOF(wal_app_msg_hdr));
            l_ret = wal_hilink_set_channel(pst_msg_hdr->auc_netdev_name, pst_channel_info);
            if (l_ret != OAL_SUCC) {
                OAM_ERROR_LOG0(0, OAM_SF_APP_MSG, "wal_hilink_recv_msg: WAL_CMD_SET_CHANNEL is error");
                return -OAL_FAIL;
            }
            break;
        default:
            OAM_ERROR_LOG1(0, OAM_SF_APP_MSG, "wal_recv_msg:msg type[%d] is error", pst_msg_hdr->uc_msg_type);
            return -OAL_FAIL;
    }
    return OAL_SUCC;
}


oal_int32 wal_hilink_send_msg(wal_app_msg *pst_msg)
{
    int    l_ret;
    if (pst_msg == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_APP_MSG, "wal_send_msg:puc_msg is null");
        return OAL_ERR_CODE_PTR_NULL;
    }
    switch (pst_msg->st_msg_hdr.uc_msg_type) {
        case WAL_MSG_FRAME:
            l_ret = wal_genetlink_send_msg(g_aul_app_pid[WAL_OPS_HISILINK], WAL_ATTR_MSG, WAL_OPS_HISILINK,
                                           (oal_uint8*)pst_msg, MSG_LEN(pst_msg));
            if (l_ret != OAL_SUCC) {
                OAM_ERROR_LOG0(0, OAM_SF_APP_MSG, "wal_hilink_send_msg: WAL_MSG_FRAME is error");
                return -OAL_FAIL;
            }
            break;
        default:
            OAM_ERROR_LOG1(0, OAM_SF_APP_MSG, "wal_send_msg:msg type[%d] is error", pst_msg->st_msg_hdr.uc_msg_type);
            return -OAL_FAIL;
    }
    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_HILINK

oal_int32 wal_hilink_set_monitor(const oal_uint8 *puc_netdev_name, wal_monitor_info_stru *pst_monitor_info)
{
    oal_net_device_stru           *pst_netdev = OAL_PTR_NULL;
    wal_msg_stru                  *pst_rsp_msg = OAL_PTR_NULL;
    wal_monitor_switch_enum_uint8  en_monitor_mode;
    wal_msg_write_stru             st_write_msg;
    oal_int8                       c_rssi_level;
    oal_uint8                      auc_info[2];
    oal_int32                      l_ret;
    oal_uint32                     ul_ret;

    if (OAL_ANY_NULL_PTR2(puc_netdev_name, pst_monitor_info)) {
        OAM_ERROR_LOG0(0, OAM_SF_APP_MSG, "wal_app_set_monitor::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_netdev      = oal_dev_get_by_name((oal_int8 *)puc_netdev_name);
    if (pst_netdev == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_APP_MSG, "wal_app_set_monitor::pst_netdev is null");
        return OAL_ERR_CODE_PTR_NULL;
    }

    en_monitor_mode = pst_monitor_info->en_monitor_mode;
    c_rssi_level    = pst_monitor_info->c_rssi_level;
    if (en_monitor_mode == WAL_MONITOR_SWITCH_OFF) {
        en_monitor_mode = WAL_MONITOR_SWITCH_OFF;
    } else {
        en_monitor_mode = WAL_MONITOR_SWITCH_ON;
    }

    auc_info[0] = en_monitor_mode;
    auc_info[1] = (oal_uint8)c_rssi_level;
    memset_s(&st_write_msg, sizeof(wal_msg_write_stru), 0, sizeof(wal_msg_write_stru));

    /* 抛事件到wal层处理 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_MONITOR_EN, OAL_SIZEOF(oal_uint8) * 2);
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, auc_info,
                 OAL_SIZEOF(oal_uint8) * 2) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_app_set_monitor::memcpy_s failed!");
        return -OAL_FAIL;
    }

    l_ret = wal_send_cfg_event(pst_netdev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + st_write_msg.us_len,
                               (oal_uint8 *)&st_write_msg, OAL_TRUE, &pst_rsp_msg);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_APP_MSG, "{wal_app_set_monitor::return err code %d!}\r\n", l_ret);
        return l_ret;
    }

    /* 读取返回的错误码 */
    ul_ret = wal_check_and_release_msg_resp(pst_rsp_msg);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_APP_MSG, "{wal_app_set_monitor::wal_send_cfg_event return err code: [%d].}", ul_ret);
        return -OAL_FAIL;
    }
    return OAL_SUCC;
}


oal_int32 wal_hilink_set_channel(const oal_uint8 *puc_netdev_name, mac_cfg_channel_param_stru *pst_channel_info)
{
    wal_msg_write_stru                st_write_msg;
    wal_msg_stru                     *pst_rsp_msg = OAL_PTR_NULL;
    oal_net_device_stru              *pst_netdev = OAL_PTR_NULL;
    oal_int32                         l_ret;
    oal_uint32                        ul_ret;

    if (OAL_ANY_NULL_PTR2(puc_netdev_name, pst_channel_info)) {
        OAM_ERROR_LOG0(0, OAM_SF_APP_MSG, "wal_app_set_channel:param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_netdev = oal_dev_get_by_name((oal_int8 *)puc_netdev_name);
    if (pst_netdev == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_APP_MSG, "wal_app_set_channel::pst_netdev is null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    l_ret = mac_is_channel_num_valid(WLAN_BAND_2G, pst_channel_info->uc_channel);
    if (l_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_APP_MSG, "wal_app_set_channel:channel %d is invable", pst_channel_info->uc_channel);
        return OAL_SUCC;
    }
    if (((pst_channel_info->en_bandwidth == WLAN_BAND_WIDTH_40PLUS) && (pst_channel_info->uc_channel > 9)) ||
        ((pst_channel_info->en_bandwidth == WLAN_BAND_WIDTH_40MINUS) && (pst_channel_info->uc_channel < 5))) {
        OAM_ERROR_LOG2(0, OAM_SF_APP_MSG, "wal_app_set_channel:uc_channel = %d,en_bandwidth = %d",
                       pst_channel_info->uc_channel, pst_channel_info->en_bandwidth);
        return -OAL_FAIL;
    }

    memset_s(&st_write_msg, sizeof(wal_msg_write_stru), 0, sizeof(wal_msg_write_stru));
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_CFG80211_SET_CHANNEL, OAL_SIZEOF(mac_cfg_channel_param_stru));

    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, pst_channel_info,
                 OAL_SIZEOF(mac_cfg_channel_param_stru)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_app_set_channel::memcpy_s failed!");
        return -OAL_FAIL;
    }

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_netdev, WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_channel_param_stru),
                               (oal_uint8 *)&st_write_msg, OAL_TRUE, &pst_rsp_msg);
    if (l_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_APP_MSG,
                         "{wal_app_set_channel:wal_send_cfg_event return err code: [%d]!}\r\n", l_ret);
        return -OAL_FAIL;
    }

    /* 读取返回的错误码 */
    ul_ret = wal_check_and_release_msg_resp(pst_rsp_msg);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_APP_MSG, "{wal_app_set_channel:wal_send_cfg_event return err code: [%d].}", ul_ret);
        return -OAL_FAIL;
    }
    return OAL_SUCC;
}


oal_int32 wal_hilink_upload_frame(oal_netbuf_stru *pst_netbuf)
{
    oal_uint8        *puc_payload = OAL_PTR_NULL;
    oal_uint32        ul_len;
    wal_app_msg      *pst_msg = OAL_PTR_NULL;
    oal_int32         l_ret = OAL_SUCC;

    if (pst_netbuf == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_APP_MSG, "wal_upload_frame:pst_netbuf is null");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_len      = OAL_NETBUF_LEN(pst_netbuf);
    puc_payload = OAL_NETBUF_PAYLOAD(pst_netbuf);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    /* 申请内存空间 */
    pst_msg = (wal_app_msg *)oal_memalloc(OAL_SIZEOF(wal_app_msg_hdr) + ul_len);
    if (pst_msg == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_APP_MSG, "wal_upload_frame:malloc fail");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_msg->st_msg_hdr.uc_msg_type  = WAL_MSG_FRAME;
    pst_msg->st_msg_hdr.uc_ack_flag  = 0;
    pst_msg->st_msg_hdr.ul_data_len  = ul_len;
    /* 将数据包复制后边进行传输 */
    if (memcpy_s(pst_msg->auc_data, WAL_APP_MSG_DATA_MAX_LEN, puc_payload, ul_len) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_APP_MSG, "wal_upload_frame:memcpy_s fail");
        oal_free(pst_msg);
        return OAL_FAIL;
    }

    l_ret = wal_hilink_send_msg(pst_msg);
    if (l_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_APP_MSG, "wal_upload_frame:send msg fail,l_ret=%d\n", l_ret);
    }
    oal_free(pst_msg);
#endif
    return l_ret;
}
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

