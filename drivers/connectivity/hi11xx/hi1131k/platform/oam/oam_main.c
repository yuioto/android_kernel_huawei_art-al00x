

#include "oam_main.h"
#include "oam_log.h"
#include "oam_event.h"
#include "securec.h"

#include "oam_alarm.h"
#include "oam_trace.h"
#include "oam_statistics.h"
#if (_PRE_PRODUCT_ID !=_PRE_PRODUCT_ID_HI1131C_DEV)
#include "oam_config.h"
#endif
#include "oam_linux_netlink.h"
#include "oam_ext_if.h"
#include "oam_register.h"
#include "plat_debug.h"
#include "oal_hcc_host_if.h"

#if (_PRE_OS_VERSION_WIN32 != _PRE_OS_VERSION)
#include "sdt_drv.h"
#endif

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#include "oam_misc.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define BUF_MAX_LEN 200
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
/* PS_PRINT 打印级别，适配外部可调整 */
oal_uint32 g_plat_loglevel = PLAT_LOG_WARNING;
#endif
/* OAM模块统一使用的全局操作变量上下文，包括OAM其他子模块全局上下文 */
oam_mng_ctx_stru    g_st_oam_mng_ctx;

/* 打印类型函数定义 */
OAL_STATIC oal_print_func g_pa_oam_print_type_func[OAM_OUTPUT_TYPE_BUTT] = {
    oam_print_to_console,   /* OAM_OUTPUT_TYPE_CONSOLE 控制台输出 */
    oam_print_to_file,      /* OAM_OUTPUT_TYPE_FS 写到文件系统 */
    oam_print_to_sdt,       /* OAM_OUTPUT_TYPE_SDT 输出到SDT,上报字符串不宜大于2048 */
};

/* 用于和SDT工具交互的全局变量 */
oam_sdt_func_hook_stru          g_st_oam_sdt_func_hook;
oam_wal_func_hook_stru          g_st_oam_wal_func_hook;
oam_sdt_stat_info_stru          g_st_sdt_stat_info;
oal_uint8                       g_uc_log_level = OAM_LOG_LEVEL_ERROR;
oal_uint8 g_auc_bcast_addr[WLAN_MAC_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

#ifdef _PRE_DEBUG_MODE  /* 调试特性默认开关状态 */
oal_uint32          g_aul_debug_feature_switch[OAM_DEBUG_TYPE_BUTT] = {
    OAL_SWITCH_OFF,   /* OAM_DEBUG_TYPE_ECHO_REG */
};
#endif

/* 特性列表 */
oam_software_feature_stru   gst_oam_feature_list[OAM_SOFTWARE_FEATURE_BUTT] = {
    /* 特性宏ID                  特性名缩写 */
    /* 0 */
    {OAM_SF_SCAN,               "scan"},
    {OAM_SF_AUTH,               "auth"},
    {OAM_SF_ASSOC,              "assoc"},
    {OAM_SF_FRAME_FILTER,       "ff"},
    {OAM_SF_WMM,                "wmm"},

    /* 5 */
    {OAM_SF_DFS,                "dfs"},
    {OAM_SF_NETWORK_MEASURE,    "nm"},
    {OAM_SF_ENTERPRISE_VO,      "ev"},
    {OAM_SF_HOTSPOTROAM,        "roam"},
    {OAM_SF_NETWROK_ANNOUNCE,   "11u"},

    /* 10 */
    {OAM_SF_NETWORK_MGMT,       "11k"},
    {OAM_SF_NETWORK_PWS,        "pws"},
    {OAM_SF_PROXYARP,           "proxyarp"},
    {OAM_SF_TDLS,               "tdls"},
    {OAM_SF_CALIBRATE,          "cali"},

    /* 15 */
    {OAM_SF_EQUIP_TEST,         "equip"},
    {OAM_SF_CRYPTO,             "crypto"},
    {OAM_SF_WPA,                "wpa"},
    {OAM_SF_WEP,                "wep"},
    {OAM_SF_WPS,                "wps"},

    /* 20 */
    {OAM_SF_PMF,                "pmf"},
    {OAM_SF_WAPI,               "wapi"},
    {OAM_SF_BA,                 "ba"},
    {OAM_SF_AMPDU,              "ampdu"},
    {OAM_SF_AMSDU,              "amsdu"},

    /* 25 */
    {OAM_SF_STABILITY,          "dfr"},
    {OAM_SF_TCP_OPT,            "tcp"},
    {OAM_SF_ACS,                "acs"},
    {OAM_SF_AUTORATE,           "autorate"},
    {OAM_SF_TXBF,               "txbf"},

    /* 30 */
    {OAM_SF_DYN_RECV,           "weak"},
    {OAM_SF_VIVO,               "vivo"},
    {OAM_SF_MULTI_USER,         "muser"},
    {OAM_SF_MULTI_TRAFFIC,      "mtraff"},
    {OAM_SF_ANTI_INTF,          "anti_intf"},

    /* 35 */
    {OAM_SF_EDCA,               "edca"},
    {OAM_SF_SMART_ANTENNA,      "ani"},
    {OAM_SF_TPC,                "tpc"},
    {OAM_SF_TX_CHAIN,           "txchain"},
    {OAM_SF_RSSI,               "rssi"},

    /* 40 */
    {OAM_SF_WOW,                "wow"},
    {OAM_SF_GREEN_AP,           "green"},
    {OAM_SF_PWR,                "pwr"},
    {OAM_SF_SMPS,               "smps"},
    {OAM_SF_TXOP,               "txop"},

    /* 45 */
    {OAM_SF_WIFI_BEACON,        "beacon"},
    {OAM_SF_KA_AP,              "alive"},
    {OAM_SF_MULTI_VAP,          "mvap"},
    {OAM_SF_2040,               "2040"},
    {OAM_SF_DBAC,               "dbac"},

    /* 50 */
    {OAM_SF_PROXYSTA,           "proxysta"},
    {OAM_SF_UM,                 "um"},
    {OAM_SF_P2P,                "p2p"},
    {OAM_SF_M2U,                "m2u"},
    {OAM_SF_IRQ,                "irq"},

    /* 55 */
    {OAM_SF_TX,                 "tx"},
    {OAM_SF_RX,                 "rx"},
    {OAM_SF_DUG_COEX,           "dugcoex"},
    {OAM_SF_CFG,                "cfg"},
    {OAM_SF_FRW,                "frw"},

    /* 60 */
    {OAM_SF_KEEPALIVE,          "keepalive"},
    {OAM_SF_COEX,               "coex"},
    {OAM_SF_HS20,               "hs20"},
    {OAM_SF_MWO_DET,            "mwodet"},
    {OAM_SF_CCA_OPT,            "cca"},

    {OAM_SF_ROAM,               "roam"},
    {OAM_SF_DFT,                "dft"},
    {OAM_SF_DFR,                "dfr"},
    {OAM_SF_BACKUP,             "backup"},
    {OAM_SF_ANY,                "any"},

    /* 70 */
    {OAM_SF_APP_MSG,            "app_msg"},
};

/*****************************************************************************
  3 函数实现
*****************************************************************************/

oal_uint32  oam_print(const char *pc_string)
{
    oam_output_type_enum_uint8 en_output_type;
    oal_uint32                 ul_rslt;

    ul_rslt = oam_get_output_type(&en_output_type);
    if (ul_rslt != OAL_SUCC) {
        return ul_rslt;
    }

    ul_rslt = g_pa_oam_print_type_func[en_output_type](pc_string);
    if (ul_rslt != OAL_SUCC) {
        return ul_rslt;
    }

    return OAL_SUCC;
}


oal_uint32 oam_print_to_console(const char *pc_string)
{
    if (OAL_UNLIKELY(pc_string == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_IO_PRINT("%s\r\n", pc_string);

    return OAL_SUCC;
}


oal_uint32   oam_print_to_file(const char *pc_string)
{
#ifdef _PRE_WIFI_DMT

    oal_file_stru            *f_file_ret = OAL_PTR_NULL;                                 /* 用于保存写文件后的返回值 */
    oal_file_stru            *f_event_file = OAL_PTR_NULL;
    oal_int32                 l_rslt;

    if (OAL_UNLIKELY(pc_string == OAL_PTR_NULL)) {
        OAM_IO_PRINTK("null param. \r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    f_event_file = oal_file_open(g_st_oam_mng_ctx.ac_file_path, (OAL_O_CREAT | OAL_O_APPEND), 0);
    if (OAL_UNLIKELY(f_event_file == OAL_FILE_FAIL)) {
        OAM_IO_PRINTK("open file failed. \r\n");
        return OAL_ERR_CODE_OPEN_FILE_FAIL;
    }

    f_file_ret = oal_file_write(f_event_file, pc_string, (OAL_STRLEN(pc_string) + 1));
    if (f_file_ret == OAL_FILE_FAIL) {
        l_rslt = oal_file_close(f_event_file);
        if (l_rslt != 0) {
            OAM_IO_PRINTK("close file failed. \r\n");
            return OAL_ERR_CODE_CLOSE_FILE_FAIL;
        }

        OAM_IO_PRINTK("write file failed. \r\n");
        return OAL_ERR_CODE_WRITE_FILE_FAIL;
    }

    l_rslt = oal_file_close(f_event_file);
    if (l_rslt != 0) {
        OAM_IO_PRINTK("close file failed. \r\n");
        return OAL_ERR_CODE_CLOSE_FILE_FAIL;
    }
#endif
    return OAL_SUCC;
}


oal_uint32 oam_print_to_sdt(const char *pc_string)
{
    oal_netbuf_stru                *pst_skb = OAL_PTR_NULL;
    oal_uint32                      ul_ret;
    oal_uint16                      us_strlen;

    if (OAL_UNLIKELY(pc_string == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_UNLIKELY(g_st_oam_sdt_func_hook.p_sdt_report_data_func == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 上报sdt字符串以'0'结束 */
    us_strlen = (oal_uint16)OAL_STRLEN(pc_string);
    us_strlen = (us_strlen > OAM_REPORT_MAX_STRING_LEN) ? OAM_REPORT_MAX_STRING_LEN : us_strlen;

    pst_skb = oam_alloc_data2sdt(us_strlen);
    if (pst_skb == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* COPY打印的内容 */
    memset_s(oal_netbuf_data(pst_skb), us_strlen, 0, us_strlen);
    ul_ret = (oal_uint32)memcpy_s(oal_netbuf_data(pst_skb), us_strlen, pc_string, (oal_uint32)us_strlen);
    if (ul_ret != EOK) {
        OAM_IO_PRINTK("oam_print_to_sdt: memcpy_s failed!\n");
        oal_mem_sdt_netbuf_free(pst_skb, OAL_TRUE);
        return OAL_FAIL;
    }
    /* 若走linux分支string类型的数据没有流控，走liteos分支string类型数据
    没有流控，适配的wpa_printf和HISI_PRINT 有流控，流控为OAM_RATELIMIT_TYPE_WPA_PRINTF_AND_HISI_PRINT */
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    /* 下发至sdt接收队列 */
    ul_ret = oam_report_data2sdt(pst_skb, OAM_DATA_TYPE_STRING, OAM_PRIMID_TYPE_OUTPUT_CONTENT);
#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    ul_ret = oam_report_data2sdt(pst_skb, OAM_DATA_TYPE_STRING_AND_WPA_PRINTF_AND_HISI_PRINT,
                                 OAM_PRIMID_TYPE_OUTPUT_CONTENT);
#endif

    return ul_ret;
}


#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
oal_uint32  oam_log_sdt(const oal_int8 *pc_func_name, const oal_int8 *pc_args_buf, oal_int32 buf_len)
{
    oal_int8    ac_output_data[200]; /* 用于保存写入到文件中的格式 */
    oal_int8    ac_printk_format[] = {"Tick=%lu, Func::%s, \"%s\""};
    oal_uint32  ul_tick;
    oal_uint32                 ul_rslt;
    oam_output_type_enum_uint8 en_output_type;
    oal_int32   ret;

    if (buf_len > BUF_MAX_LEN) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "oam_log_sdt::buf_len check failed %d!", buf_len);
        return OAL_FAIL;
    }
    ul_tick = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    ret = snprintf_s(ac_output_data, BUF_MAX_LEN, BUF_MAX_LEN - 1, ac_printk_format, ul_tick,
                     pc_func_name, pc_args_buf);
    if ((ret < 0) || (ret > BUF_MAX_LEN)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "oam_log_sdt::snprintf_s failed !");
        return OAL_FAIL;
    }

    ul_rslt = oam_get_output_type(&en_output_type);
    if (ul_rslt != OAL_SUCC) {
        return ul_rslt;
    }

    ul_rslt = g_pa_oam_print_type_func[en_output_type](ac_output_data);
    if (ul_rslt != OAL_SUCC) {
        return ul_rslt;
    }

    return OAL_SUCC;
}

#endif


oal_uint32 oam_upload_log_to_sdt(const oal_int8 *pc_string)
{
    oal_netbuf_stru        *pst_skb = OAL_PTR_NULL;
    oal_uint32              ul_ret;

    if (OAL_UNLIKELY(g_st_oam_sdt_func_hook.p_sdt_report_data_func == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pc_string == OAL_PTR_NULL) {
        OAL_IO_PRINT("oam_upload_log_to_sdt::pc_string is null!\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_skb = oam_alloc_data2sdt(OAL_SIZEOF(oam_log_info_stru));
    if (pst_skb == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* COPY打印的内容 */
    ul_ret = (oal_uint32)memcpy_s(oal_netbuf_data(pst_skb), OAL_SIZEOF(oam_log_info_stru),
                                  pc_string, OAL_SIZEOF(oam_log_info_stru));
    if (ul_ret != EOK) {
        OAM_IO_PRINTK("oam_upload_log_to_sdt: memcpy_s failed!\n");
        oal_mem_sdt_netbuf_free(pst_skb, OAL_TRUE);
        return OAL_FAIL;
    }
    /* 下发至sdt接收队列，若队列满则串口输出 */
    ul_ret = oam_report_data2sdt(pst_skb, OAM_DATA_TYPE_LOG, OAM_PRIMID_TYPE_OUTPUT_CONTENT);

    return ul_ret;
}

oal_uint32 oam_upload_device_log_to_sdt(const oal_uint8 *pc_string, oal_uint16 len)
{
    oal_netbuf_stru        *pst_skb = OAL_PTR_NULL;
    oal_uint32              ul_ret;

    if (pc_string == OAL_PTR_NULL) {
        OAL_IO_PRINT("oam_upload_log_to_sdt::pc_string is null!\n");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_skb = oam_alloc_data2sdt(len);
    if (pst_skb == OAL_PTR_NULL) {
        OAL_IO_PRINT("alloc netbuf stru failed!\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* COPY打印的内容 */
    ul_ret = (oal_uint32)memcpy_s(oal_netbuf_data(pst_skb), len, pc_string, len);
    if (ul_ret != EOK) {
        OAM_IO_PRINTK("oam_upload_device_log_to_sdt: memcpy_s failed!\n");
        oal_mem_sdt_netbuf_free(pst_skb, OAL_TRUE);
        return OAL_FAIL;
    }
    /* 下发至sdt接收队列，若队列满则串口输出 */
    ul_ret = oam_report_data2sdt(pst_skb, OAM_DATA_TYPE_DEVICE_LOG, OAM_PRIMID_TYPE_OUTPUT_CONTENT);

    return ul_ret;
}


oal_uint32 oam_send_device_data2sdt(const oal_uint8* pc_string, oal_uint16 len)
{
    oal_uint32 ul_ret;
    if (pc_string == NULL) {
        return OAL_EFAIL;
    }

    ul_ret = oam_upload_device_log_to_sdt(pc_string, len);

    return ul_ret;
}

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
oal_int32 oam_rx_post_action_function(oal_uint8 stype,
                                      hcc_netbuf_stru* pst_hcc_netbuf, oal_uint8 *pst_context)
{
    oal_uint8   *puc_data = OAL_PTR_NULL;
    oal_uint8   *puc_data_tmp = OAL_PTR_NULL;
    oal_uint16  us_netbuff_len;
    oal_uint16  us_len_tmpa, us_len_tmpb;
    oal_uint8   uc_tmp_i = 0;

    OAL_REFERENCE(pst_context);
    OAL_BUG_ON(pst_hcc_netbuf == NULL);

    puc_data = oal_netbuf_data(pst_hcc_netbuf->pst_netbuf);

    us_netbuff_len = pst_hcc_netbuf->len;
    puc_data_tmp = puc_data;

    /*
     * 日志netbuff由devcie log聚合而成，device log紧密聚合，
     * netbuff里没有任何非日志数据，
     * 每个netbuff device约束最大保存512字节device log
     */
    while ((us_netbuff_len != 0) && (us_netbuff_len <= 512)) {
        /*
         * 根据devcie log结构体得出device log长度
         */
        us_len_tmpa = ((*(puc_data_tmp + 4)) + ((*(puc_data_tmp + 4 + 1)) << 8));
        us_len_tmpb = us_len_tmpa;
        us_netbuff_len -= us_len_tmpa;

        /* oam netbuff中不仅包含oam log，仅需展示log日志 */
        if (((*(puc_data_tmp + 1)) == DEV_OM_MSG_TYPE_LOG) && ((*(puc_data_tmp + 8) >> 4) <= g_uc_log_level)) {
            /* device log 去除非包头20字节包尾1个字节，剩余即为纯参数长度 */
            us_len_tmpb -= (20 + 1);

            /*
             * device日志在送sdt前，OAL_IO_PRINT打印
             * 日志级别LV、文件号FI、行号LI、参数P[1-12]
             * (最多12个参数)
             */
            OAL_IO_PRINT("Dlog LV:%d, FI:%d, LI:%d",\
                         (*(puc_data_tmp + 8) >> 4),\
                         ((*(puc_data_tmp + 12)) + ((*(puc_data_tmp + 12 + 1)) << 8)),\
                         ((*(puc_data_tmp + 14)) + ((*(puc_data_tmp + 14 + 1)) << 8)));

            while ((us_len_tmpb != 0) && (uc_tmp_i < 12)) {
                OAL_IO_PRINT(", ");
                OAL_IO_PRINT("P%d:0x%08x ", (uc_tmp_i + 1), ((*(puc_data_tmp + 20 + 4 * uc_tmp_i)) + \
                                                             ((*(puc_data_tmp + 20 + 4 * uc_tmp_i + 1)) << 8) + \
                                                             ((*(puc_data_tmp + 20 + 4 * uc_tmp_i + 2)) << 16) + \
                                                             ((*(puc_data_tmp + 20 + 4 * uc_tmp_i + 3)) << 24) \
                                                            ));
                uc_tmp_i++;
                us_len_tmpb -= 4;
            }

            OAL_IO_PRINT("\n");
            uc_tmp_i = 0;
        }

        puc_data_tmp += us_len_tmpa;
    }

#if (_HI113X_SW_VERSION == _HI113X_SW_DEBUG)
    oam_send_device_data2sdt(puc_data, pst_hcc_netbuf->len);
#endif
    oal_netbuf_free(pst_hcc_netbuf->pst_netbuf);
    return OAL_SUCC;
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
int32_t chr_rx_post_action_function(uint8_t stype, hcc_netbuf_stru* hcc_netbuf, uint8_t *context)
{
    uint8_t *netbuf_data = NULL;
    OAL_REFERENCE(context);
    OAL_BUG_ON(hcc_netbuf == NULL);

    netbuf_data = oal_netbuf_data(hcc_netbuf->pst_netbuf);
    chr_dev_exception_callback(netbuf_data, (uint16_t)hcc_netbuf->len);
    oal_netbuf_free(hcc_netbuf->pst_netbuf);
    return OAL_SUCC;
}
#endif
#endif


oal_uint32  oam_get_output_type(oam_output_type_enum_uint8 *pen_output_type)
{
    if (OAL_UNLIKELY(pen_output_type == OAL_PTR_NULL)) {
        OAM_IO_PRINTK("null param \r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    *pen_output_type = g_st_oam_mng_ctx.en_output_type;

    return OAL_SUCC;
}


oal_uint32  oam_set_output_type(oam_output_type_enum_uint8 en_output_type)
{
    if (en_output_type >= OAM_OUTPUT_TYPE_BUTT) {
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    g_st_oam_mng_ctx.en_output_type = en_output_type;

    return OAL_SUCC;
}


oal_uint32  oam_set_file_path(const oal_int8 *pc_file_path, oal_uint32 ul_length)
{
    errno_t l_ret;
#ifdef _PRE_WIFI_DMT
    oal_file_stru         *f_event_file = OAL_PTR_NULL;
    oal_uint8             *puc_file_path = OAL_PTR_NULL;

    if (pc_file_path == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (ul_length > OAM_FILE_PATH_LENGTH) {
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    /* 待优化 */
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    puc_file_path = "/jffs0/";
#else
    puc_file_path = DmtStub_GetDebugFilePath();
#endif
    l_ret = memcpy_s(g_st_oam_mng_ctx.ac_file_path, OAM_FILE_PATH_LENGTH, puc_file_path, strlen(puc_file_path));
    if (l_ret != EOK) {
        OAM_IO_PRINTK("oam_set_file_path: memcpy_s failed!\n");
        return OAL_FAIL;
    }
    /* 以下操作是为了将上一次的日志文件清空 */
    f_event_file = oal_file_open(g_st_oam_mng_ctx.ac_file_path, (OAL_O_CREAT | OAL_O_RDWR), 0);
    if (f_event_file == OAL_FILE_FAIL) {
        return OAL_ERR_CODE_WRITE_FILE_FAIL;
    }

    if (oal_file_close(f_event_file) != 0) {
        return OAL_ERR_CODE_CLOSE_FILE_FAIL;
    }

#else

    if (pc_file_path == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (ul_length > OAM_FILE_PATH_LENGTH) {
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    l_ret = memcpy_s(g_st_oam_mng_ctx.ac_file_path, OAM_FILE_PATH_LENGTH, pc_file_path, ul_length);
    if (l_ret != EOK) {
        OAM_IO_PRINTK("oam_set_file_path: memcpy_s failed!\n");
        return OAL_FAIL;
    }
#endif

    return OAL_SUCC;
}

oal_void  oam_dump_buff_by_hex(const oal_uint8 *puc_buff, oal_int32 l_len, oal_int32 l_num)
{
    oal_int32       l_loop;
    if (l_num <= 0) {
        OAL_IO_PRINT("l_num invaild! \n");
        return;
    }

    for (l_loop = 0; l_loop < l_len; l_loop++) {
        OAL_IO_PRINT("%02x ", puc_buff[l_loop]);

        if ((l_loop + 1) % l_num == 0) {
            OAL_IO_PRINT("\n");
        }
    }

    OAL_IO_PRINT("\n");
}


OAL_STATIC oal_void oam_drv_func_hook_init(oal_void)
{
    /* sdt侧对外钩子函数初始化 */
    g_st_oam_sdt_func_hook.p_sdt_report_data_func       = OAL_PTR_NULL;
    g_st_oam_sdt_func_hook.p_sdt_get_wq_len_func        = OAL_PTR_NULL;

    /* wal侧对外钩子函数初始化 */
    g_st_oam_wal_func_hook.p_wal_recv_cfg_data_func     = OAL_PTR_NULL;
    g_st_oam_wal_func_hook.p_wal_recv_mem_data_func     = OAL_PTR_NULL;
    g_st_oam_wal_func_hook.p_wal_recv_reg_data_func     = OAL_PTR_NULL;
    g_st_oam_wal_func_hook.p_wal_recv_global_var_func   = OAL_PTR_NULL;
}

oal_void oam_sdt_func_fook_register(oam_sdt_func_hook_stru *pfun_st_oam_sdt_hook)
{
    g_st_oam_sdt_func_hook.p_sdt_report_data_func = pfun_st_oam_sdt_hook->p_sdt_report_data_func;
    g_st_oam_sdt_func_hook.p_sdt_get_wq_len_func  = pfun_st_oam_sdt_hook->p_sdt_get_wq_len_func;
}


oal_void oam_wal_func_fook_register(oam_wal_func_hook_stru *pfun_st_oam_wal_hook)
{
    g_st_oam_wal_func_hook.p_wal_recv_cfg_data_func     = pfun_st_oam_wal_hook->p_wal_recv_cfg_data_func;
    g_st_oam_wal_func_hook.p_wal_recv_mem_data_func     = pfun_st_oam_wal_hook->p_wal_recv_mem_data_func;
    g_st_oam_wal_func_hook.p_wal_recv_reg_data_func     = pfun_st_oam_wal_hook->p_wal_recv_reg_data_func;
    g_st_oam_wal_func_hook.p_wal_recv_global_var_func   = pfun_st_oam_wal_hook->p_wal_recv_global_var_func;
}

oal_uint32  oam_filter_data2sdt(oam_data_type_enum_uint8 en_type)
{
    if (g_st_sdt_stat_info.ul_wq_len < WLAN_SDT_MSG_FLT_HIGH_THD) {
        g_st_sdt_stat_info.en_filter_switch = OAL_FALSE;
        return OAM_FLT_PASS;
    } else if ((g_st_sdt_stat_info.ul_wq_len >= WLAN_SDT_MSG_FLT_HIGH_THD)
               && (g_st_sdt_stat_info.ul_wq_len < WLAN_SDT_MSG_QUEUE_MAX_LEN)) {
        /* 消息队列达到过滤上限，过滤非日志消息 */
        g_st_sdt_stat_info.en_filter_switch = OAL_TRUE;
        /* 就host oam log不过滤 */
        return (oal_uint32)((en_type == (oal_uint8)OAM_DATA_TYPE_LOG) ? OAM_FLT_PASS : OAM_FLT_DROP);
    }

    /* 消息队列满全部过滤 */
    return OAM_FLT_DROP;
}


oal_netbuf_stru *oam_alloc_data2sdt(oal_uint16  us_data_len)
{
    oal_netbuf_stru    *pst_netbuf = OAL_PTR_NULL;

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    pst_netbuf = oal_mem_sdt_netbuf_alloc(us_data_len + WLAN_SDT_SKB_RESERVE_LEN, OAL_TRUE);
#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    /* sdt内存暂用netbuf公用内存池，后续liteos继续优化做成与k3一致
    ，提高内存分配速度且不会内存越界 */
    pst_netbuf = oal_netbuf_alloc(us_data_len + WLAN_SDT_SKB_RESERVE_LEN, 0, 0);
#endif
    if (pst_netbuf == OAL_PTR_NULL) {
        return OAL_PTR_NULL;
    }
    oal_netbuf_reserve(pst_netbuf, WLAN_SDT_SKB_HEADROOM_LEN);

    oal_netbuf_put(pst_netbuf, us_data_len);

    return pst_netbuf;
}

oal_uint32 oam_report_data2sdt(oal_netbuf_stru *pst_netbuf,
                               oam_data_type_enum_uint8 en_type,
                               oam_primid_type_enum_uint8 en_prim)
{
    /* 判断sdt发送消息队列是否已满，若满输出至串口 */
    if (OAL_LIKELY(g_st_oam_sdt_func_hook.p_sdt_get_wq_len_func != OAL_PTR_NULL)) {
        g_st_sdt_stat_info.ul_wq_len = (oal_uint32)g_st_oam_sdt_func_hook.p_sdt_get_wq_len_func();
    }

    if (oam_filter_data2sdt(en_type) != OAM_FLT_PASS) {
        OAM_SDT_STAT_INCR(ul_filter_cnt);
        oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
        return OAL_FAIL;
    }

    if (OAL_UNLIKELY(g_st_oam_sdt_func_hook.p_sdt_report_data_func == OAL_PTR_NULL)) {
        OAL_IO_PRINT("oam_report_data2sdt p_sdt_report_data_func is NULL. \n");
        return OAL_FAIL;
    }

    g_st_oam_sdt_func_hook.p_sdt_report_data_func(pst_netbuf, en_type, en_prim);

    return OAL_SUCC;
}


oal_void oam_sdt_func_fook_unregister(oal_void)
{
    /* 函数指针赋值 */
    g_st_oam_sdt_func_hook.p_sdt_report_data_func           = OAL_PTR_NULL;
    g_st_oam_sdt_func_hook.p_sdt_get_wq_len_func            = OAL_PTR_NULL;
}


oal_void oam_wal_func_fook_unregister(oal_void)
{
    /* 函数指针赋值 */
    g_st_oam_wal_func_hook.p_wal_recv_cfg_data_func         = OAL_PTR_NULL;
    g_st_oam_wal_func_hook.p_wal_recv_global_var_func       = OAL_PTR_NULL;
    g_st_oam_wal_func_hook.p_wal_recv_mem_data_func         = OAL_PTR_NULL;
    g_st_oam_wal_func_hook.p_wal_recv_reg_data_func         = OAL_PTR_NULL;
}

#if ((_PRE_OS_VERSION_WIN32 != _PRE_OS_VERSION) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION))
oal_void  oam_hisi_kill(void)
{
    struct task_struct *oamhisi_task = NULL;
    oamhisi_task = pid_task(find_vpid((sdt_drv_get_mng_entry())->ul_usepid), PIDTYPE_PID);
    if (oamhisi_task != NULL) {
        force_sig(SIGKILL, oamhisi_task);
    } else {
        OAL_IO_PRINT("[oam_hisi_kill]do not find oam hisi process\n");
    }
}
#else
oal_void  oam_hisi_kill(void)
{
}
#endif


oal_int32  oam_main_init(oal_void)
{
    oal_uint32 ul_rslt;

    /* 初始化可维可测试FILE路径 */
    ul_rslt = oam_set_file_path(WLAN_OAM_FILE_PATH, (OAL_STRLEN(WLAN_OAM_FILE_PATH) + 1));
    if (ul_rslt != OAL_SUCC) {
        OAL_IO_PRINT("oam_main_init call oam_set_file_path fail %d\n", ul_rslt);
        OAL_WARN_ON(1); // OAL_BUG_ON 会使程序发生进入panic, 无法进入return，修改成WRN_ON 只打印调用栈，使用return正常退出
        return -OAL_EFAIL;
    }

    /* 初始化可维可测输出方式 */
    ul_rslt = oam_set_output_type(OAM_OUTPUT_TYPE_SDT);
    if (ul_rslt != OAL_SUCC) {
        OAL_IO_PRINT("oam_main_init call oam_set_output_type fail %d\n", ul_rslt);
        OAL_WARN_ON(1);
        return -OAL_EFAIL;
    }

    ul_rslt = oam_log_init();
    if (ul_rslt != OAL_SUCC) {
        OAL_IO_PRINT("oam_main_init call oam_log_init fail %d\n", ul_rslt);
        OAL_WARN_ON(1);
        return -OAL_EFAIL;
    }

    /* 完成EVENT模块的初始化操作 */
    ul_rslt = oam_event_init();
    if (ul_rslt != OAL_SUCC) {
        OAL_IO_PRINT("oam_main_init call oam_event_init fail %d\n", ul_rslt);
        OAL_WARN_ON(1);
        return -OAL_EFAIL;
    }

    /* 1131会用到oam_event，其余1151用到1102没用到的功能不必要，oam_trace.c，
    oam_statistics.c中繁复的但1102也并没用好的内存、特性统计和cpu跟踪也抛弃
    */
#ifdef _PRE_PROFILING_MODE
    /* 完成PROFILING模块的初始化操作 */
    ul_rslt = oam_profiling_init();
    if (ul_rslt != OAL_SUCC) {
        OAL_IO_PRINT("oam_main_init call oam_profiling_init fail %d\n", ul_rslt);
        OAL_WARN_ON(1);
        return -OAL_EFAIL;
    }
#endif

    /* 初始化oam模块的钩子函数 */
    oam_drv_func_hook_init();

    /* 统计模块初始化 */
    oam_statistics_init();

    /* 1151 用到的，理论上1102也该关闭，但1102该宏忘记关 */
#ifdef _PRE_WLAN_DFT_REG
    oam_reg_init();
#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
    hcc_rx_register(hcc_get_default_handler(), HCC_ACTION_TYPE_OAM, oam_rx_post_action_function, NULL);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    hcc_rx_register(hcc_get_default_handler(), HCC_ACTION_TYPE_CHR, chr_rx_post_action_function, NULL);
#endif
#endif

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    /* 从pc端dump文件到liteos处理函数初始化 */
    oam_uart_rx_sdt_cmd_init();
#endif
    return OAL_SUCC;
}


oal_void  oam_main_exit(oal_void)
{
    /* 初始化5115timer，用于代码中获取高精度时间戳 */
#ifdef _PRE_WLAN_DFT_REG
    oam_reg_exit();
#endif

    /* 卸载成功后，输出打印 */
    OAL_IO_PRINT("oam exit ok!\n");
    return ;
}

/*lint -e578*//*lint -e19*/
/*lint -e19*/
oal_module_symbol(oam_hisi_kill);
oal_module_symbol(oam_main_init);
oal_module_symbol(oam_main_exit);
oal_module_symbol(oam_send_device_data2sdt);
oal_module_symbol(oam_set_file_path);
oal_module_symbol(oam_set_output_type);
oal_module_symbol(oam_get_output_type);
oal_module_symbol(oam_print);
oal_module_symbol(g_st_oam_mng_ctx);
oal_module_symbol(oam_dump_buff_by_hex);
oal_module_symbol(g_st_oam_sdt_func_hook);
oal_module_symbol(g_st_oam_wal_func_hook);
oal_module_symbol(oam_sdt_func_fook_register);
oal_module_symbol(oam_sdt_func_fook_unregister);
oal_module_symbol(oam_wal_func_fook_register);
oal_module_symbol(oam_wal_func_fook_unregister);
oal_module_symbol(oam_report_data2sdt);
oal_module_symbol(g_st_sdt_stat_info);
oal_module_symbol(oam_alloc_data2sdt);
oal_module_symbol(gst_oam_feature_list);
oal_module_symbol(g_auc_bcast_addr);
oal_module_symbol(g_plat_loglevel);

#ifdef _PRE_DEBUG_MODE
oal_module_symbol(g_aul_debug_feature_switch);
#endif

oal_module_license("GPL");

