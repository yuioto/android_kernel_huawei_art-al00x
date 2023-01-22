

// 1 头文件包含
#include "oal_ext_if.h"
#include "oal_profiling.h"
#include "oam_ext_if.h"
#include "frw_ext_if.h"
#include "wlan_types.h"
#include "mac_vap.h"
#include "mac_resource.h"
#include "mac_ie.h"

#include "hmac_ext_if.h"
#include "hmac_chan_mgmt.h"

#include "wal_main.h"
#include "wal_config.h"
#include "wal_regdb.h"
#include "wal_linux_scan.h"
#include "wal_linux_atcmdsrv.h"
#include "wal_linux_bridge.h"
#include "wal_linux_flowctl.h"
#include "wal_linux_event.h"
#include "plat_cali.h"
#include "oal_hcc_host_if.h"
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#endif
#include "oal_util.h"
#include "hmac_config.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_LINUX_ATCMDSRV_C

// 2 结构体定义
#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST))
typedef enum {
    CHECK_LTE_GPIO_INIT            = 0,    /* 初始化 */
    CHECK_LTE_GPIO_LOW             = 1,    /* 设置为低电平 */
    CHECK_LTE_GPIO_HIGH            = 2,    /* 设置为高电平 */
    CHECK_LTE_GPIO_RESUME          = 3,    /* 恢复寄存器设置 */
    CHECK_LTE_GPIO_BUTT
} check_lte_gpio_step;

typedef struct {
    oal_uint8                     uc_mode;          /* 模式 */
    oal_uint8                     uc_band;          /* 频段 */
} wal_atcmdsrv_mode_stru;

typedef struct {
    oal_uint32                   ul_datarate;          /* at命令配置的速率值 */
    oal_int8                    *pc_datarate;          /* 速率字符串 */
} wal_atcmdsrv_datarate_stru;

typedef oal_int32 (*wal_atcmd_ioctl_set_func)(oal_net_device_stru *pst_net_dev,
                                              wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd);
typedef struct {
    int32_t                  cmd_id;
    wal_atcmd_ioctl_set_func pfunc;
    int32_t                  copy_flag;
} wal_atcmd_ioctl_set_stru;

OAL_CONST wal_atcmdsrv_mode_stru g_atcmdsrv_mode_table[] = {
    { WLAN_LEGACY_11B_MODE, WLAN_BAND_2G },       /* 11b, 2.4G */
    { WLAN_LEGACY_11G_MODE, WLAN_BAND_2G },       /* 旧的11g only已废弃, 2.4G, OFDM */
    { WLAN_MIXED_ONE_11G_MODE, WLAN_BAND_2G },    /* 11bg, 2.4G */
    { WLAN_MIXED_TWO_11G_MODE, WLAN_BAND_2G },    /* 11g only, 2.4G */
    { WLAN_VHT_MODE, WLAN_BAND_2G },              /* 11ac */
    { WLAN_HT_11G_MODE, WLAN_BAND_2G },           /* 11ng,不包括11b */
    { WLAN_HT_ONLY_MODE_2G, WLAN_BAND_2G },       /* 11nonlg 2Gmode */
    { WLAN_PROTOCOL_BUTT, WLAN_BAND_2G },

#ifdef _PRE_WLAN_FEATURE_5G
    { WLAN_LEGACY_11A_MODE, WLAN_BAND_5G },       /* 11a, 5G, OFDM */
    { WLAN_HT_MODE, WLAN_BAND_5G },               /* 11n(11bgn或者11an，根据频段判断) */
    { WLAN_HT_ONLY_MODE, WLAN_BAND_5G },          /* 11n only 5Gmode,只有带HT的设备才可以接入 */
    { WLAN_VHT_ONLY_MODE, WLAN_BAND_5G },         /* 11ac only mode 只有带VHT的设备才可以接入 */
#endif /* _PRE_WLAN_FEATURE_5G */
};

OAL_STATIC OAL_CONST wal_atcmdsrv_datarate_stru past_atcmdsrv_non_ht_rate_table[] = {
    { 1, " 1 " },
    { 2, " 2 " },
    { 5, " 5.5 " },
    { 6, " 6 " },
    { 7, " 7 " },
    { 8, " 8 " },
    { 9, " 9 " },
    { 11, " 11 " },
    { 12, " 12 " },
    { 18, " 18 " },
    { 24, " 24 " },
    { 36, " 36 " },
    { 48, " 48 " },
    { 54, " 54 " },
};

oal_uint64 ul_chipcheck_total_time;

OAL_STATIC int32_t wal_atcmsrv_ioctl_set_freq(oal_net_device_stru *net_dev,
                                              wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd);
OAL_STATIC int32_t wal_atcmsrv_ioctl_set_mode(oal_net_device_stru *net_dev,
                                              wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd);
OAL_STATIC int32_t wal_atcmsrv_ioctl_set_datarate(oal_net_device_stru *net_dev,
                                                  wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd);
OAL_STATIC int32_t wal_atcmsrv_ioctl_set_bandwidth(oal_net_device_stru *net_dev,
                                                   wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd);
OAL_STATIC int32_t wal_atcmsrv_ioctl_set_txpower(oal_net_device_stru *net_dev,
                                                 wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd);
OAL_STATIC int32_t wal_atcmsrv_ioctl_set_always_tx(oal_net_device_stru *net_dev,
                                                   wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd);
OAL_STATIC int32_t wal_atcmsrv_ioctl_set_always_rx(oal_net_device_stru *net_dev,
                                                   wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd);
OAL_STATIC int32_t wal_atcmsrv_ioctl_set_hw_addr(oal_net_device_stru *net_dev,
                                                 wal_atcmdsrv_wifi_priv_cmd_stru *priv_cmd);
OAL_STATIC int32_t wal_atcmsrv_ioctl_set_country(oal_net_device_stru *net_dev,
                                                 wal_atcmdsrv_wifi_priv_cmd_stru *priv_cmd);
OAL_STATIC int32_t wal_atcmsrv_ioctl_get_rx_rssi(oal_net_device_stru *net_dev,
                                                 wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd);
OAL_STATIC int32_t wal_atcmsrv_ioctl_get_rx_pckg_ex(oal_net_device_stru *net_dev,
                                                    wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd);
OAL_STATIC int32_t wal_atcmsrv_ioctl_set_pm_switch(oal_net_device_stru *net_dev,
                                                   wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd);
OAL_STATIC int32_t wal_atcmsrv_ioctl_memory_check(oal_net_device_stru *net_dev,
                                                  wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd);
OAL_STATIC int32_t wal_atcmsrv_ioctl_get_memory_check_result(oal_net_device_stru *net_dev,
                                                             wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd);
OAL_STATIC int32_t wal_atcmsrv_ioctl_sdio_loop(oal_net_device_stru *net_dev,
                                               wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd);
OAL_STATIC int32_t wal_atcmsrv_ioctl_get_dieid(oal_net_device_stru *net_dev,
                                               wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd);
OAL_STATIC int32_t wal_atcmsrv_ioctl_get_dbb_num(oal_net_device_stru *net_dev,
                                                 wal_atcmdsrv_wifi_priv_cmd_stru *priv_cmd);

OAL_STATIC wal_atcmd_ioctl_set_stru g_ast_wal_atcmd_table[] = {
    { WAL_ATCMDSRV_IOCTL_CMD_WI_FREQ_SET,        wal_atcmsrv_ioctl_set_freq,                  0 },
    { WAL_ATCMDSRV_IOCTL_CMD_WI_POWER_SET,       wal_atcmsrv_ioctl_set_txpower,               0 },
    { WAL_ATCMDSRV_IOCTL_CMD_MODE_SET,           wal_atcmsrv_ioctl_set_mode,                  0 },
    { WAL_ATCMDSRV_IOCTL_CMD_DATARATE_SET,       wal_atcmsrv_ioctl_set_datarate,              0 },
    { WAL_ATCMDSRV_IOCTL_CMD_BAND_SET,           wal_atcmsrv_ioctl_set_bandwidth,             0 },
    { WAL_ATCMDSRV_IOCTL_CMD_ALWAYS_TX_SET,      wal_atcmsrv_ioctl_set_always_tx,             0 },
    { WAL_ATCMDSRV_IOCTL_CMD_DBB_GET,            wal_atcmsrv_ioctl_get_dbb_num,               1 },
    { WAL_ATCMDSRV_IOCTL_CMD_HW_STATUS_GET,      NULL,                                        0 },
    { WAL_ATCMDSRV_IOCTL_CMD_ALWAYS_RX_SET,      wal_atcmsrv_ioctl_set_always_rx,             0 },
    { WAL_ATCMDSRV_IOCTL_CMD_HW_ADDR_SET,        wal_atcmsrv_ioctl_set_hw_addr,               1 },
    { WAL_ATCMDSRV_IOCTL_CMD_RX_PKCG_GET,        wal_atcmsrv_ioctl_get_rx_pckg_ex,            1 },
    { WAL_ATCMDSRV_IOCTL_CMD_PM_SWITCH_SET,      wal_atcmsrv_ioctl_set_pm_switch,             0 },
    { WAL_ATCMDSRV_IOCTL_CMD_RX_RSSI_GET,        wal_atcmsrv_ioctl_get_rx_rssi,               1 },
    { WAL_ATCMDSRV_IOCTL_CMD_CHIPCHECK_SET,      wal_atcmsrv_ioctl_memory_check,              0 },
    { WAL_ATCMDSRV_IOCTL_CMD_CHIPCHECK_RESULT,   wal_atcmsrv_ioctl_get_memory_check_result,   1 },
    { WAL_ATCMDSRV_IOCTL_CMD_CHIPCHECK_TIME,     NULL,                                        0 },
    { WAL_ATCMDSRV_IOCTL_CMD_UART_LOOP_SET,      NULL,                                        0 },
    { WAL_ATCMDSRV_IOCTL_CMD_SDIO_LOOP_SET,      wal_atcmsrv_ioctl_sdio_loop,                 0 },
    { WAL_ATCMDSRV_IOCTL_CMD_RD_CALDATA,         NULL,                                        0 },
    { WAL_ATCMDSRV_IOCTL_CMD_SET_CALDATA,        NULL,                                        0 },
    { WAL_ATCMDSRV_IOCTL_CMD_GET_EFUSE_RESULT,   NULL,                                        0 },
    { WAL_ATCMDSRV_IOCTL_CMD_SET_ANT,            NULL,                                        0 },
    { WAL_ATCMDSRV_IOCTL_CMD_DIEID_INFORM,       wal_atcmsrv_ioctl_get_dieid,                 1 },
    { WAL_ATCMDSRV_IOCTL_CMD_SET_COUNTRY,        wal_atcmsrv_ioctl_set_country,               0 },
    { WAL_ATCMDSRV_IOCTL_CMD_GET_UPCCODE,        NULL,                                        0 },
    { WAL_ATCMDSRV_IOCTL_CMD_SET_CONN_POWER,     NULL,                                        0 },
    { WAL_ATCMDSRV_IOCTL_CMD_SET_BSS_EXPIRE_AGE, NULL,                                        0 },
    { WAL_ATCMDSRV_IOCTL_CMD_GET_CONN_INFO,      NULL,                                        0 },
};

#endif
int32_t                       g_bandwidth;
int32_t                       g_mode;
extern hmac_reg_info_receive_event     g_st_hmac_reg_info_receive_event;
extern oal_uint32 wal_hipriv_set_freq(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
extern oal_uint32 wal_hipriv_set_mode(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
extern oal_uint32 wal_hipriv_set_bw(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
extern oal_uint32 wal_hipriv_always_tx_1102(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
extern oal_uint32 wal_hipriv_always_rx(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
extern oal_uint32 wal_hipriv_reg_write(oal_net_device_stru *pst_net_dev, const oal_int8 *pc_param);
#ifdef _PRE_WLAN_FEATURE_STA_PM
extern uint32_t  wal_hipriv_sta_pm_on(oal_net_device_stru *pst_cfg_net_dev, const int8_t *pc_param);
#endif
// 3 函数实现
#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST))

OAL_STATIC int32_t wal_atcmsrv_ioctl_set_freq(oal_net_device_stru *net_dev,
                                              wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd)
{
    int32_t                ret;
    wal_msg_write_stru     write_msg;

    if (net_dev == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_atcmsrv_ioctl_set_freq:: net_dev is NULL");
        return OAL_FAIL;
    }

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_atcmsrv_ioctl_set_freq:freq[%d]", pst_priv_cmd->pri_data.freq);

    /* 填写消息 */
    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_CURRENT_CHANEL, OAL_SIZEOF(int32_t));
    *((int32_t *)(write_msg.auc_value)) = pst_priv_cmd->pri_data.freq;

    /* 发送消息 */
    ret = wal_send_cfg_event(net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(int32_t),
                             (uint8_t *)&write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_set_freq::return err code %d!}", ret);
        return ret;
    }

    return OAL_SUCC;
}


OAL_STATIC int32_t wal_atcmsrv_ioctl_set_mode(oal_net_device_stru *net_dev,
                                              wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd)
{
    int32_t                  ret;
    uint8_t                  prot_idx;
    mac_vap_stru            *mac_vap    = OAL_PTR_NULL;
    wal_msg_write_stru       write_msg;
    mac_cfg_mode_param_stru *mode_param = OAL_PTR_NULL;
    uint8_t                  arr_num = OAL_ARRAY_SIZE(g_atcmdsrv_mode_table);
    int32_t                  mode  = pst_priv_cmd->pri_data.mode;

    mac_vap = OAL_NET_DEV_PRIV(net_dev);
    if (mac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_set_mode::OAL_NET_DEV_PRIV, return null!}");
        return -OAL_EINVAL;
    }

    /* 获取模式对应的band */
    for (prot_idx = 0; prot_idx < arr_num; prot_idx++) {
        if (g_atcmdsrv_mode_table[prot_idx].uc_mode == (uint8_t)mode) {
            break;
        }
    }

    if (prot_idx >= arr_num) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_set_mode:err code[%u]}", prot_idx);
        return -OAL_EINVAL;
    }

    /* 抛事件到wal层处理 */
    /* 填写消息 */
    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_MODE, OAL_SIZEOF(mac_cfg_mode_param_stru));

    /* 设置模式，在配置模式的时候将带宽默认成20M */
    mode_param = (mac_cfg_mode_param_stru *)(write_msg.auc_value);
    if (mode == WLAN_HT_ONLY_MODE_2G) {
        mode_param->en_protocol = WLAN_HT_ONLY_MODE;
    } else if (mode == WLAN_VHT_ONLY_MODE_2G) {
        mode_param->en_protocol = WLAN_VHT_MODE;
    } else {
        mode_param->en_protocol = (uint8_t)mode;
    }

    mode_param->en_band      = (wlan_channel_band_enum_uint8)g_atcmdsrv_mode_table[prot_idx].uc_band;
    mode_param->en_bandwidth = WLAN_BAND_WIDTH_20M;

    /* 维测使用，后续将删除 */
    OAM_WARNING_LOG3(mac_vap->uc_vap_id, OAM_SF_CFG,
                     "{wal_atcmsrv_ioctl_set_mode::protocol[%d],band[%d],bandwidth[%d]!}\r\n",
                     mode_param->en_protocol, mode_param->en_band, mode_param->en_bandwidth);

    /* 发送消息 */
    ret = wal_send_cfg_event(net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_mode_param_stru),
                             (uint8_t *)&write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_set_mode::return err code %d!}\r\n", ret);
        return ret;
    }

    g_mode = mode_param->en_protocol;

    return OAL_SUCC;
}

static int32_t wal_atcmsrv_ioctl_set_datarate_mode(oal_net_device_stru *net_dev,
                                                   wal_atcmdsrv_wifi_priv_cmd_stru *priv_cmd)
{
    uint8_t               idx;
    uint8_t               arr_num      = OAL_ARRAY_SIZE(past_atcmdsrv_non_ht_rate_table);
    uint32_t              rate_ret;

    /* 获取速率对应的字符，方便调用设置速率的相应接口 */
    for (idx = 0; idx < arr_num; idx++) {
        if (past_atcmdsrv_non_ht_rate_table[idx].ul_datarate == (uint32_t)priv_cmd->pri_data.datarate) {
            break;
        }
    }

    if (idx >= arr_num) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "idx Overrunning!");
        return -OAL_EINVAL;
    }

    if (g_mode == WLAN_HT_ONLY_MODE) { /* 当速率设置为7时表示MCS7 */
        rate_ret = wal_hipriv_set_mcs(net_dev, past_atcmdsrv_non_ht_rate_table[idx].pc_datarate);
    } else if (g_mode == WLAN_VHT_MODE) {
        rate_ret = wal_hipriv_set_mcsac(net_dev, past_atcmdsrv_non_ht_rate_table[idx].pc_datarate);
    } else {
        rate_ret = wal_hipriv_set_rate(net_dev, past_atcmdsrv_non_ht_rate_table[idx].pc_datarate);
    }

    if (rate_ret != OAL_SUCC) {
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}


OAL_STATIC int32_t wal_atcmsrv_ioctl_set_datarate(oal_net_device_stru *net_dev,
                                                  wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd)
{
    int32_t               ret;
    uint8_t               en_bw_index;
    mac_vap_stru         *mac_vap      = OAL_PTR_NULL;
    mac_cfg_tx_comp_stru *set_bw_param = OAL_PTR_NULL;
    wal_msg_write_stru    write_msg;

    mac_vap = OAL_NET_DEV_PRIV(net_dev);
    if (mac_vap == OAL_PTR_NULL) {
        return -OAL_EINVAL;
    }

    OAM_WARNING_LOG1(mac_vap->uc_vap_id, OAM_SF_ANY,
        "wal_atcmsrv_ioctl_set_datarate:datarate[%d]", pst_priv_cmd->pri_data.datarate);

    ret = wal_atcmsrv_ioctl_set_datarate_mode(net_dev, pst_priv_cmd);
    if (ret != OAL_SUCC) {
        return ret;
    }

    /* 设置长发描述符带宽 */
    /* 抛事件到wal层处理 */
    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_SET_BW, OAL_SIZEOF(mac_cfg_tx_comp_stru));

    /* 解析并设置配置命令参数 */
    set_bw_param = (mac_cfg_tx_comp_stru *)(write_msg.auc_value);

    if ((g_bandwidth >= WLAN_BAND_WIDTH_80PLUSPLUS) && (g_bandwidth <= WLAN_BAND_WIDTH_80MINUSMINUS)) {
        en_bw_index = 8;
    } else if ((g_bandwidth >= WLAN_BAND_WIDTH_40PLUS) && (g_bandwidth <= WLAN_BAND_WIDTH_40MINUS)) {
        en_bw_index = 4;
    } else {
        en_bw_index = 0;
    }

    set_bw_param->uc_param = (uint8_t)(en_bw_index);

    ret = wal_send_cfg_event(net_dev, WAL_MSG_TYPE_WRITE,
        WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_tx_comp_stru),
        (uint8_t *)&write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_bw::return err code [%d]!}\r\n", ret);
        return ret;
    }

    return OAL_SUCC;
}


OAL_STATIC int32_t wal_atcmsrv_ioctl_set_bandwidth(oal_net_device_stru *net_dev,
                                                   wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd)
{
    int32_t                  ret;
    wal_msg_write_stru       write_msg;
    mac_cfg_mode_param_stru *mode_param = OAL_PTR_NULL;
    int32_t                  bandwidth = pst_priv_cmd->pri_data.bandwidth;

    g_bandwidth = bandwidth;
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_atcmsrv_ioctl_set_bandwidth:bandwidth[%d]", bandwidth);

    /* 抛事件到wal层处理 */
    /* 填写消息 */
    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_BANDWIDTH, OAL_SIZEOF(int32_t));

    /* 设置带宽时，模式不做修改，还是按照之前的值配置 */
    mode_param = (mac_cfg_mode_param_stru *)(write_msg.auc_value);

    mode_param->en_bandwidth = (uint8_t)bandwidth;

    /* 发送消息 */
    ret = wal_send_cfg_event(net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_mode_param_stru),
                             (uint8_t *)&write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_set_mode::return err code %d!}\r\n", ret);
        return ret;
    }

    return OAL_SUCC;
}


OAL_STATIC int32_t wal_atcmsrv_ioctl_set_txpower(oal_net_device_stru *net_dev,
                                                 wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd)
{
    int32_t            ret;
    wal_msg_write_stru write_msg;
    int32_t            txpower = pst_priv_cmd->pri_data.pow;

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_atcmsrv_ioctl_set_txpower:txpower[%d]", txpower);

    /* 抛事件到wal层处理 */
    /* 填写消息 */
    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_TX_POWER, OAL_SIZEOF(int32_t));
    *((int32_t *)(write_msg.auc_value)) = txpower;

    /* 发送消息 */
    ret = wal_send_cfg_event(net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(int32_t),
                             (uint8_t *)&write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_set_txpower::return err code %d!}", ret);
        return ret;
    }

    return OAL_SUCC;
}


OAL_STATIC int32_t  wal_atcmsrv_ioctl_set_always_tx(oal_net_device_stru *net_dev,
                                                    wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd)
{
    int32_t            ret;
    int8_t             open_tx[] = "1 2 2000";
    int32_t            always_tx = pst_priv_cmd->pri_data.always_tx;

    if (net_dev == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_atcmsrv_ioctl_set_always_tx:: net_dev is NULL");
        return OAL_FAIL;
    }

    if (always_tx == CLOSE_ALWYAS_TX) {
        ret = (int32_t)wal_hipriv_always_tx_1102(net_dev, "0 0 0");
        if (OAL_UNLIKELY(ret != OAL_SUCC)) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_set_always_tx::return err code [%d]!}\r\n", ret);
        }
    } else if (always_tx == OPEN_ALWYAS_TX) {
        ret = (int32_t)wal_hipriv_always_tx_1102(net_dev, open_tx);
        if (OAL_UNLIKELY(ret != OAL_SUCC)) {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_set_always_tx::return err code [%d]!}\r\n", ret);
        }
    } else {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_set_always_tx::param error [%d]!}\r\n", always_tx);
        return always_tx;
    }
    return OAL_SUCC;
}


OAL_STATIC int32_t wal_atcmsrv_ioctl_set_always_rx(oal_net_device_stru *net_dev,
                                                   wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd)
{
    int32_t                        ret;
    wal_msg_write_stru             write_msg;
    uint8_t                        always_rx = (uint8_t)pst_priv_cmd->pri_data.always_rx;

    if (net_dev == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_atcmsrv_ioctl_set_always_rx:: net_dev is NULL");
        return OAL_FAIL;
    }

    if (always_rx > HAL_ALWAYS_RX_RESERVED) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_set_always_rx::input should be 0 or 1.}");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }
    *(uint8_t *)(write_msg.auc_value) = always_rx;

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_atcmsrv_ioctl_set_always_rx:: %d", always_rx);
    /* 抛事件到wal层处理 */
    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_SET_ALWAYS_RX, OAL_SIZEOF(uint8_t));
    ret = wal_send_cfg_event(net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(uint8_t),
                             (uint8_t *)&write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_always_rx::return err code [%d]!}\r\n", ret);
        return (uint32_t)ret;
    }

    return OAL_SUCC;
}


OAL_STATIC int32_t wal_atcmsrv_ioctl_set_hw_addr(oal_net_device_stru *net_dev,
                                                 wal_atcmdsrv_wifi_priv_cmd_stru *priv_cmd)
{
    int32_t ret;
    mac_cfg_staion_id_param_stru *mac_cfg_para = NULL;
    wal_msg_write_stru write_msg;
    uint8_t *hw_addr = (uint8_t *)priv_cmd->pri_data.hw_mac;

    // 抛事件到wal层处理
    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_STATION_ID, OAL_SIZEOF(mac_cfg_staion_id_param_stru));

    /* 设置配置命令参数 */
    mac_cfg_para = (mac_cfg_staion_id_param_stru *)(write_msg.auc_value);
    /* 这两个参数在02已经没有意义 */
    mac_cfg_para->en_p2p_mode = WLAN_LEGACY_VAP_MODE;
    oal_set_mac_addr(mac_cfg_para->auc_station_id, hw_addr);

    ret = wal_send_cfg_event(net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_staion_id_param_stru),
                             (uint8_t *)&write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_set_hw_addr_etc::return err code %d!}\r\n", ret);
        return ret;
    }
    return OAL_SUCC;
}


OAL_STATIC int32_t wal_atcmsrv_ioctl_set_country(oal_net_device_stru *net_dev,
                                                 wal_atcmdsrv_wifi_priv_cmd_stru *priv_cmd)
{
#ifdef _PRE_WLAN_FEATURE_11D
    int32_t ret;
    int8_t *countrycode = priv_cmd->pri_data.countrycode;

#ifdef _PRE_WLAN_FEATURE_DFS
    ret = wal_regdomain_update_for_dfs(net_dev, countrycode);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
                         "{wal_atcmsrv_ioctl_set_country::regdomain_update_for_dfs return err code %d!}\r\n",
                         ret);
        return ret;
    }
#endif
    ret = wal_regdomain_update(net_dev, countrycode);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
                         "{wal_atcmsrv_ioctl_set_country::regdomain_update return err code %d!}\r\n",
                         ret);
        return ret;
    }
#endif
    return OAL_SUCC;
}

/*
 * 功能描述 : 查询接收RSSI值
 * 修改历史 :
 * 1.修改内容 : 新增函数
 */
OAL_STATIC int32_t wal_atcmsrv_ioctl_get_rx_rssi(oal_net_device_stru *net_dev,
                                                 wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd)
{
    int32_t                         ret;
    int32_t                         leftime;
    wal_msg_write_stru              write_msg;
    mac_vap_stru                   *mac_vap     = OAL_PTR_NULL;
    hmac_vap_stru                  *hmac_vap    = OAL_PTR_NULL;
    mac_cfg_rx_fcs_info_stru       *rx_fcs_info = OAL_PTR_NULL;
    hmac_atcmdsrv_get_stats_stru   *atcmdsrv    = OAL_PTR_NULL;
    int32_t                        *rx_rssi     = &pst_priv_cmd->pri_data.rx_rssi;

    mac_vap = OAL_NET_DEV_PRIV(net_dev);
    if (mac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_rx_rssi::OAL_NET_DEV_PRIV, return null!}");
        return -OAL_EINVAL;
    }

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    if (hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_rx_rssi::get_hmac_vap failed!}");
        return OAL_FAIL;
    }

    /* 抛事件到wal层处理 */
    atcmdsrv = &(hmac_vap->st_atcmdsrv_get_status);
    atcmdsrv->uc_get_rx_pkct_flag = OAL_FALSE;
    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_RX_FCS_INFO, OAL_SIZEOF(mac_cfg_rx_fcs_info_stru));

    /* 设置配置命令参数 */
    rx_fcs_info = (mac_cfg_rx_fcs_info_stru *)(write_msg.auc_value);
    /* 这两个参数在02已经没有意义 */
    rx_fcs_info->ul_data_op = 0;
    rx_fcs_info->ul_print_info = 0;

    ret = wal_send_cfg_event(net_dev, WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_rx_fcs_info_stru),
                             (uint8_t *)&write_msg, OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_rx_rssi::return err code %d!}\r\n", ret);
        return ret;
    }

    /* 阻塞等待dmac上报 */
    leftime = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(hmac_vap->query_wait_q,
        (uint32_t)(atcmdsrv->uc_get_rx_pkct_flag == OAL_TRUE), WAL_ATCMDSRB_GET_RX_PCKT);
    if (leftime == 0) {
        /* 超时还没有上报扫描结束 */
        OAM_WARNING_LOG1(mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_atcmsrv_ioctl_get_rx_rssi::dbb_num wait for %ld ms timeout!}",
            ((WAL_ATCMDSRB_DBB_NUM_TIME * 1000) / OAL_TIME_HZ));
        return -OAL_EINVAL;
    } else if (leftime < 0) {
        /* 定时器内部错误 */
        OAM_WARNING_LOG1(mac_vap->uc_vap_id, OAM_SF_ANY,
            "{wal_atcmsrv_ioctl_get_rx_rssi::dbb_num wait for %ld ms error!}",
            ((WAL_ATCMDSRB_DBB_NUM_TIME * 1000) / OAL_TIME_HZ));
        return -OAL_EINVAL;
    } else {
        /* 正常结束 */
        *rx_rssi = (oal_int)hmac_vap->st_atcmdsrv_get_status.s_rx_rssi;
        OAM_WARNING_LOG1(mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_rx_rssi::rx_rssi:%d}", *rx_rssi);
        return OAL_SUCC;
    }
}

// ate dieid
OAL_STATIC int32_t  wal_atcmsrv_ioctl_get_dieid(oal_net_device_stru *net_dev,
                                                wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd)
{
    uint8_t *res = NULL;
    uint32_t dieid_len = 0;

    if (net_dev == NULL || pst_priv_cmd == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_sdio_loop::param is null!}");
        return -OAL_EINVAL;
    }
    res = get_die_id_info_handler(&dieid_len);
    if (memcpy_s(pst_priv_cmd->pri_data.die_id, WAL_ATCMDSRV_DIE_ID_LENGTH * sizeof(uint16_t),
        res, dieid_len) != EOK) {
        return -OAL_EINVAL;
    }
    return OAL_SUCC;
}

// ate sdio loop test
OAL_STATIC int32_t  wal_atcmsrv_ioctl_sdio_loop(oal_net_device_stru *net_dev,
                                                wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd)
{
    int32_t ret;
    int8_t  param = 0;
    if (net_dev == NULL || pst_priv_cmd == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_sdio_loop::param is null!}");
        return -OAL_EINVAL;
    }
    ret = conn_test_sdio_loop(&param);
    return ret;
}

// ate memory check
OAL_STATIC int32_t  wal_atcmsrv_ioctl_memory_check(oal_net_device_stru *net_dev,
                                                   wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd)
{
    int32_t ret;
    if (net_dev == NULL || pst_priv_cmd == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_memory_check::param is null!}");
        return -OAL_EINVAL;
    }
    ret = device_mem_check();
    return ret;
}

// ate get memory check result
OAL_STATIC int32_t  wal_atcmsrv_ioctl_get_memory_check_result(oal_net_device_stru *net_dev,
                                                              wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd)
{
    if (net_dev == NULL || pst_priv_cmd == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_memory_check_result::param is null!}");
        return -OAL_EINVAL;
    }
    pst_priv_cmd->pri_data.chipcheck_result = device_mem_check_get_result();
    return OAL_SUCC;
}


int32_t  wal_atcmsrv_ioctl_get_rx_pckg(oal_net_device_stru *net_dev, int32_t *rx_pckg_succ_num)
{
    int32_t                     ret;
    int32_t                     leftime;
    wal_msg_write_stru          write_msg;
    mac_vap_stru               *mac_vap     = OAL_PTR_NULL;
    hmac_vap_stru              *hmac_vap    = OAL_PTR_NULL;
    mac_cfg_rx_fcs_info_stru   *rx_fcs_info = OAL_PTR_NULL;
    uint8_t                     vap_id;

    mac_vap = OAL_NET_DEV_PRIV(net_dev);
    if (mac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_rx_pckg::OAL_NET_DEV_PRIV, return null!}");
        return -OAL_EINVAL;
    }
    vap_id = mac_vap->uc_vap_id;

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(vap_id);
    if (hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(vap_id, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_rx_pckg::mac_res_get_hmac_vap failed!}");
        return OAL_FAIL;
    }

    /* 抛事件到wal层处理 */
    hmac_vap->st_atcmdsrv_get_status.uc_get_rx_pkct_flag = OAL_FALSE;
    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_RX_FCS_INFO, OAL_SIZEOF(mac_cfg_rx_fcs_info_stru));

    /* 设置配置命令参数 */
    rx_fcs_info = (mac_cfg_rx_fcs_info_stru *)(write_msg.auc_value);
    /* 这两个参数在02已经没有意义 */
    rx_fcs_info->ul_data_op    = 0;
    rx_fcs_info->ul_print_info = 0;

    ret = wal_send_cfg_event(net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_rx_fcs_info_stru),
                             (uint8_t *)&write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_rx_pckg::return err code %d!}\r\n", ret);
        return ret;
    }

    /* 阻塞等待dmac上报 */
    leftime = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(hmac_vap->query_wait_q,
        (uint32_t)(hmac_vap->st_atcmdsrv_get_status.uc_get_rx_pkct_flag == OAL_TRUE), WAL_ATCMDSRB_GET_RX_PCKT);
    if (leftime == 0) {
        /* 超时还没有上报扫描结束 */
        OAM_WARNING_LOG1(vap_id, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_rx_pckg::dbb_num wait for %ld ms timeout!}",
                         ((WAL_ATCMDSRB_DBB_NUM_TIME * 1000) / OAL_TIME_HZ));
        return -OAL_EINVAL;
    } else if (leftime < 0) {
        /* 定时器内部错误 */
        OAM_WARNING_LOG1(vap_id, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_rx_pckg::dbb_num wait for %ld ms error}",
            ((WAL_ATCMDSRB_DBB_NUM_TIME * 1000) / OAL_TIME_HZ));
        return -OAL_EINVAL;
    } else {
        /* 正常结束 */
        OAM_INFO_LOG1(vap_id, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_rx_pckg::dbb_num wait for %ld ms error!}",
            ((WAL_ATCMDSRB_DBB_NUM_TIME * 1000) / OAL_TIME_HZ));
        *rx_pckg_succ_num = (oal_long)hmac_vap->st_atcmdsrv_get_status.ul_rx_pkct_succ_num;
        OAM_WARNING_LOG1(vap_id, OAM_SF_ANY, "rx package [%d]", *rx_pckg_succ_num);
        return OAL_SUCC;
    }
}

OAL_STATIC int32_t  wal_atcmsrv_ioctl_get_rx_pckg_ex(oal_net_device_stru *net_dev,
                                                     wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd)
{
    int32_t ret;
    ret = wal_atcmsrv_ioctl_get_rx_pckg(net_dev, &pst_priv_cmd->pri_data.rx_pkcg);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_rx_pckg_ex::return err code %d!}\r\n", ret);
    }
    return ret;
}


OAL_STATIC int32_t wal_atcmsrv_ioctl_set_pm_switch(oal_net_device_stru *net_dev,
                                                   wal_atcmdsrv_wifi_priv_cmd_stru *pst_priv_cmd)
{
    int32_t              ret;
    int8_t               sta_pm_on[5] = " 0 ";
    wal_msg_write_stru   write_msg;
    int32_t              pm_switch    = pst_priv_cmd->pri_data.pm_switch;

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_atcmsrv_ioctl_set_pm_switch:l_pm_switch[%d]", pm_switch);

    *(uint8_t *)(write_msg.auc_value) = (uint8_t)pm_switch;

    /* 抛事件到wal层处理 */
    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_SET_PM_SWITCH, OAL_SIZEOF(int32_t));

    ret = wal_send_cfg_event(net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(int32_t),
                             (uint8_t *)&write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_set_pm_switch::return err code [%d]!}\r\n", ret);
        return ret;
    }
#ifdef _PRE_WLAN_FEATURE_STA_PM
    ret = wal_hipriv_sta_pm_on(net_dev, sta_pm_on);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_set_pm_switch::wal_hipriv_sta_pm_on fail[%d]!}", ret);
        return ret;
    }
#endif

    return OAL_SUCC;
}



OAL_STATIC void wal_atcmdsrv_ioctl_convert_dbb_num(uint32_t dbb_num, uint8_t *dbb_number)
{
    uint8_t temp;

    /* MAC H/w version register format                  */
    /* ------------------------------------------------ */
    /* | 31 - 24 | 23 - 16 | 15 - 12 | 11 - 0 | */
    /* ------------------------------------------------ */
    /* | BN      | Y1      | Y2      |   Y3   | */
    /* ------------------------------------------------ */
    /* Format the version as BN.Y1.Y2.Y3 with all values in hex i.e. the  */
    /* version string would be XX.XX.X.XXX.                                 */
    /* For e.g. 0225020A saved in the version register would translate to */
    /* the configuration interface version number 02.25.0.20A             */
    temp = (dbb_num & 0xF0000000) >> BIT_OFFSET_28;
    dbb_number[0] = WAL_ATCMDSRV_GET_HEX_CHAR(temp);
    temp = (dbb_num & 0x0F000000) >> BIT_OFFSET_24;
    dbb_number[1] = WAL_ATCMDSRV_GET_HEX_CHAR(temp);

    dbb_number[2] = '.';

    temp = (dbb_num & 0x00F00000) >> BIT_OFFSET_20;
    dbb_number[3] = WAL_ATCMDSRV_GET_HEX_CHAR(temp);
    temp = (dbb_num & 0x000F0000) >> BIT_OFFSET_16;
    dbb_number[4] = WAL_ATCMDSRV_GET_HEX_CHAR(temp);
    dbb_number[5] = '.';

    temp = (dbb_num & 0x0000F000) >> BIT_OFFSET_12;
    dbb_number[6] = WAL_ATCMDSRV_GET_HEX_CHAR(temp);
    dbb_number[7] = '.';

    temp = (dbb_num & 0x00000F00) >> BIT_OFFSET_8;
    dbb_number[8] = WAL_ATCMDSRV_GET_HEX_CHAR(temp);
    temp = (dbb_num & 0x000000F0) >> BIT_OFFSET_4;
    dbb_number[9] = WAL_ATCMDSRV_GET_HEX_CHAR(temp);
    temp = (dbb_num & 0x0000000F) >> BIT_OFFSET_0;
    dbb_number[10] = WAL_ATCMDSRV_GET_HEX_CHAR(temp);

    return;
}


OAL_STATIC int32_t wal_atcmsrv_ioctl_get_dbb_num(oal_net_device_stru *net_dev,
                                                 wal_atcmdsrv_wifi_priv_cmd_stru *priv_cmd)
{
    wal_msg_write_stru write_msg = {0};
    int32_t ret;
    int32_t leftime;
    int8_t *dbb_num = priv_cmd->pri_data.dbb;
    mac_vap_stru *mac_vap = NULL;
    hmac_vap_stru *hmac_vap = NULL;

    mac_vap = OAL_NET_DEV_PRIV(net_dev);
    if (mac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_dbb_num::OAL_NET_DEV_PRIV, return null!}");
        return -OAL_EINVAL;
    }

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    if (hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_dbb_num::mac_res_get_hmac_vap failed!}");
        return OAL_FAIL;
    }
    // 抛事件到wal层处理
    hmac_vap->st_atcmdsrv_get_status.uc_get_dbb_completed_flag = OAL_FALSE;
    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_GET_VERSION, 0);

    ret = wal_send_cfg_event(net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH, (uint8_t *)&write_msg,
        OAL_FALSE, OAL_PTR_NULL);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_dbb_num::wal_send_cfg_event return errCode[%d]!}", ret);
        return ret;
    }
    /* 阻塞等待dmac上报 */
    /*lint -e730*/
    leftime = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(hmac_vap->query_wait_q,
        (uint32_t)(hmac_vap->st_atcmdsrv_get_status.uc_get_dbb_completed_flag == OAL_TRUE),
        WAL_ATCMDSRB_DBB_NUM_TIME);
    /*lint +e730*/
    if (leftime == 0) {
        /* 超时还没有上报扫描结束 */
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_dbb_num::dbb_num wait for %ld ms timeout!}",
            ((WAL_ATCMDSRB_DBB_NUM_TIME * SEC2MSEC) / OAL_TIME_HZ));
        return -OAL_EINVAL;
    } else if (leftime < 0) {
        /* 定时器内部错误 */
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_atcmsrv_ioctl_get_dbb_num::dbb_num wait for %ld ms error!}",
            ((WAL_ATCMDSRB_DBB_NUM_TIME * SEC2MSEC) / OAL_TIME_HZ));
        return -OAL_EINVAL;
    } else {
        /* 正常结束  */
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_atcmsrv_ioctl_get_dbb_num:ul_dbb_num[0x%x].",
            hmac_vap->st_atcmdsrv_get_status.ul_dbb_num);
        if (hmac_vap->st_atcmdsrv_get_status.ul_dbb_num != WAL_ATCMDSRV_MAC_DBB_NUM) {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_atcmsrv_ioctl_get_dbb_num:ul_dbb_num[0x%x],not match 0x3115040c",
                hmac_vap->st_atcmdsrv_get_status.ul_dbb_num);
            return -OAL_EINVAL;
        }
        wal_atcmdsrv_ioctl_convert_dbb_num(hmac_vap->st_atcmdsrv_get_status.ul_dbb_num, (uint8_t *)dbb_num);
        return OAL_SUCC;
    }
}


int32_t wal_atcmdsrv_wifi_priv_cmd(oal_net_device_stru *net_dev, oal_ifreq_stru *ifr, int32_t cmd)
{
    int32_t                          ret;
    uint32_t                         arrSize = (sizeof(g_ast_wal_atcmd_table) / sizeof(g_ast_wal_atcmd_table[0]));
    wal_atcmdsrv_wifi_priv_cmd_stru  priv_cmd = {0};

    if ((ifr->ifr_data == OAL_PTR_NULL) || (net_dev == OAL_PTR_NULL)) {
        return -OAL_EINVAL;
    }
    /* 将用户态数据拷贝到内核态 */
    if (oal_copy_from_user(&priv_cmd, ifr->ifr_data, sizeof(wal_atcmdsrv_wifi_priv_cmd_stru))) {
        return -OAL_EINVAL;
    }

    /* compare with common_service verify code(80211) */
    if (priv_cmd.l_verify != WAL_ATCMDSRV_IOCTL_VERIFY_CODE_1131) {
        OAM_WARNING_LOG2(0, OAM_SF_ANY,
            "wal_atcmdsrv_wifi_priv_cmd:ioctl verify failed, verify code is:%d(not equal %d)",
            priv_cmd.l_verify, WAL_ATCMDSRV_IOCTL_VERIFY_CODE_1131);
        return -OAL_EINVAL;
    }

    if ((uint32_t)priv_cmd.l_cmd >= arrSize) {
        OAM_ERROR_LOG1(0, 0, "wal_atcmdsrv_wifi_priv_cmd_etc::err cmd[%d]", priv_cmd.l_cmd);
        return -OAL_EINVAL;
    }

    if ((g_ast_wal_atcmd_table[priv_cmd.l_cmd].pfunc != NULL)
        && (g_ast_wal_atcmd_table[priv_cmd.l_cmd].cmd_id == priv_cmd.l_cmd)) {
        ret = g_ast_wal_atcmd_table[priv_cmd.l_cmd].pfunc(net_dev, &priv_cmd);
        if (g_ast_wal_atcmd_table[priv_cmd.l_cmd].copy_flag) {
            oal_copy_to_user(ifr->ifr_data, &priv_cmd, sizeof(wal_atcmdsrv_wifi_priv_cmd_stru));
        }
        return ret;
    }
    OAM_ERROR_LOG1(0, 0, "wal_atcmdsrv_wifi_priv_cmd::pfunc not register, cmd_idx is %d", priv_cmd.l_cmd);
    return -OAL_EINVAL;
}
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

