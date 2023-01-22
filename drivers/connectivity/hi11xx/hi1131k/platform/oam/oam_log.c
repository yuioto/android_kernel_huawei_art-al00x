

#include "oam_main.h"
#include "oam_log.h"
#include "securec.h"
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    #include "driver_hisi_lib_api.h"
#endif
#ifdef CONFIG_PRINTK
    #include <linux/kernel.h>
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAM_LOG_C
#define BUF_MAX_LENGTH 150


#ifdef _PRE_DEBUG_MODE
oam_tx_complete_stat_stru   g_ast_tx_complete_stat[WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC];
#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
#ifdef CONFIG_PRINTK
static char* g_loglevel_string[OAM_LOG_LEVEL_BUTT] ;
#endif
#endif
static char* g_oammacerr_print[OAM_EXCP_TYPE_BUTT] = {
    "{Exception Statistics::OAM_HAL_MAC_ERROR_PARA_CFG_ERR.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_RXBUFF_LEN_TOO_SMALL.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_BA_ENTRY_NOT_FOUND.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_PHY_TRLR_TIME_OUT.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_PHY_RX_FIFO_OVERRUN.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_TX_DATAFLOW_BREAK.[%d]}\r\n",
    "", // 此case缺失，确认是否存在问题OAM_HAL_MAC_ERROR_RX_FSM_ST_TIMEOUT
    "{Exception Statistics::OAM_HAL_MAC_ERROR_TX_FSM_ST_TIMEOUT.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_RX_HANDLER_ST_TIMEOUT.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_TX_HANDLER_ST_TIMEOUT.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_TX_INTR_FIFO_OVERRUN.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_RX_INTR_FIFO_OVERRUN.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_HIRX_INTR_FIFO_OVERRUN.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_RX_Q_EMPTY.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_HIRX_Q_EMPTY.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_BUS_RLEN_ERR.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_BUS_RADDR_ERR.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_BUS_WLEN_ERR.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_BUS_WADDR_ERR.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_TX_ACBK_Q_OVERRUN.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_TX_ACBE_Q_OVERRUN.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_TX_ACVI_Q_OVERRUN.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_TX_ACVO_Q_OVERRUN.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_TX_HIPRI_Q_OVERRUN.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_MATRIX_CALC_TIMEOUT.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_CCA_TIMEOUT.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_DCOL_DATA_OVERLAP.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_BEACON_MISS.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_UNKOWN_28.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_UNKOWN_29.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_UNKOWN_30.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_MAC_ERROR_UNKOWN_31.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_SOC_ERROR_BUCK_OCP.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_SOC_ERROR_BUCK_SCP.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_SOC_ERROR_OCP_RFLDO1.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_SOC_ERROR_OCP_RFLDO2.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_SOC_ERROR_OCP_CLDO.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_SOC_ERROR_RF_OVER_TEMP.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_SOC_ERROR_CMU_UNLOCK.[%d]}\r\n",
    "{Exception Statistics::OAM_HAL_SOC_ERROR_PCIE_SLV_ERR.[%d]}\r\n"
};


oal_int32 OAL_PRINT2KERNEL(
    oal_uint8       uc_vap_id,
    oal_uint16      us_file_no,
    oal_uint8       clog_level,
    const oal_int8* pfunc_local_name,
    oal_uint16      us_line_no,
    void*           pfunc_addr,
    const oal_int8* fmt,
    oal_ulong p1,
    oal_ulong p2,
    oal_ulong p3,
    oal_ulong p4)
{
    /* liteos 版本无printk，DFT流程也没做，所以暂时注空 */
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    {
    }
#else
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
#ifdef CONFIG_PRINTK
    oal_int32 l_ret;
    oal_int8   pc_buf[OAM_LOG_PRINT_DATA_LENGTH];

    pc_buf[0] = '\0';

    if (clog_level == OAM_LOG_LEVEL_ERROR) {
        DECLARE_DFT_TRACE_KEY_INFO("oam error log", OAL_DFT_TRACE_OTHER);
    }

    l_ret = snprintf_s(pc_buf, OAM_LOG_PRINT_DATA_LENGTH, OAM_LOG_PRINT_DATA_LENGTH -1,
                       KERN_DEBUG"[%s][vap:%d]%s [F:%d][L:%d]\n",
                       g_loglevel_string[clog_level],
                       uc_vap_id,
                       fmt,
                       us_file_no,
                       us_line_no);
    if (l_ret < 0) {
        OAM_IO_PRINTK("l_ret < 0 \r\n");
        return l_ret;
    }

    printk(pc_buf, (oal_int32)p1, (oal_int32)p2, (oal_int32)p3, (oal_int32)p4);
#endif
#endif
#endif
    return OAL_SUCC;
}


oal_int32 OAL_PRINT_NLOGS(
    const oal_int8* pfunc_local_name,
    oal_uint16      us_file_no,
    oal_uint16      us_line_no,
    void*           pfunc_addr,
    oal_uint8       uc_vap_id,
    oal_uint8       en_feature_id,
    oal_uint8       clog_level,
    oal_int8*       fmt,
    uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    if (oam_get_log_switch(uc_vap_id, en_feature_id, clog_level) == OAL_SWITCH_ON)
        OAL_PRINT2KERNEL(uc_vap_id, us_file_no, clog_level, pfunc_local_name,
                         us_line_no, pfunc_addr, fmt, p1, p2, p3, p4);
    oam_log_print4(uc_vap_id, en_feature_id, us_file_no, us_line_no, clog_level, fmt,
                   (oal_int32)p1, (oal_int32)p2, (oal_int32)p3, (oal_int32)p4);

    return OAL_SUCC;
}


oal_uint32 oam_log_set_global_switch(oal_switch_enum_uint8 en_log_switch)
{
    if (OAL_UNLIKELY(en_log_switch >= OAL_SWITCH_BUTT)) {
        OAM_IO_PRINTK("invalid en_log_switch[%d]. \r\n", en_log_switch);
        return OAL_FAIL;
    }

    g_st_oam_mng_ctx.st_log_ctx.en_global_log_switch = en_log_switch;

    return OAL_SUCC;
}


OAL_STATIC  oal_uint32 oam_log_set_ratelimit_switch(
    oam_ratelimit_type_enum_uint8  en_ratelimit_type,
    oal_switch_enum_uint8 en_log_switch)
{
    if (OAL_UNLIKELY(en_ratelimit_type >= OAM_RATELIMIT_TYPE_BUTT)) {
        OAM_IO_PRINTK("invalid en_ratelimit_type[%d]. \r\n", en_ratelimit_type);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    if (en_log_switch >= OAL_SWITCH_BUTT) {
        OAM_IO_PRINTK("invalid en_log_switch[%d]. \r\n", en_log_switch);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    g_st_oam_mng_ctx.st_log_ctx.st_ratelimit[en_ratelimit_type].en_ratelimit_switch = en_log_switch;

    return OAL_SUCC;
}


oal_uint32 oam_log_get_ratelimit_switch(
    oam_ratelimit_type_enum_uint8  en_ratelimit_type,
    oal_switch_enum_uint8 *pen_log_switch)
{
    if (OAL_UNLIKELY(en_ratelimit_type >= OAM_RATELIMIT_TYPE_BUTT)) {
        OAM_IO_PRINTK("invalid en_ratelimit_type[%d]. \r\n", en_ratelimit_type);
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    if (OAL_UNLIKELY(pen_log_switch == OAL_PTR_NULL)) {
        OAM_IO_PRINTK("null param. \r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    *pen_log_switch = g_st_oam_mng_ctx.st_log_ctx.st_ratelimit[en_ratelimit_type].en_ratelimit_switch;

    return OAL_SUCC;
}


OAL_STATIC  oal_uint32 oam_log_set_ratelimit_intervel(
    oam_ratelimit_type_enum_uint8  en_ratelimit_type,
    oal_uint32 ul_interval)
{
    if (OAL_UNLIKELY(en_ratelimit_type >= OAM_RATELIMIT_TYPE_BUTT)) {
        OAM_IO_PRINTK("invalid en_ratelimit_type[%d]. \r\n", en_ratelimit_type);
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    if ((ul_interval < OAM_RATELIMIT_MIN_INTERVAL)
        || (ul_interval > OAM_RATELIMIT_MAX_INTERVAL)) {
        OAM_IO_PRINTK("ul_interval[%d] must be range[%d~%d]. \r\n",
                      ul_interval, OAM_RATELIMIT_MIN_INTERVAL, OAM_RATELIMIT_MAX_INTERVAL);

        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    g_st_oam_mng_ctx.st_log_ctx.st_ratelimit[en_ratelimit_type].ul_interval = ul_interval * OAL_TIME_HZ;

    return OAL_SUCC;
}


OAL_STATIC  oal_uint32 oam_log_set_ratelimit_burst(
    oam_ratelimit_type_enum_uint8  en_ratelimit_type,
    oal_uint32 ul_burst)
{
    if (OAL_UNLIKELY(en_ratelimit_type >= OAM_RATELIMIT_TYPE_BUTT)) {
        OAM_IO_PRINTK("invalid en_ratelimit_type[%d]. \r\n", en_ratelimit_type);
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    if ((ul_burst < OAM_RATELIMIT_MIN_BURST)
        || (ul_burst > OAM_RATELIMIT_MAX_BURST)) {
        OAM_IO_PRINTK("ul_burst[%d] must be range[%d~%d]. \r\n",
                      ul_burst, OAM_RATELIMIT_MIN_BURST, OAM_RATELIMIT_MAX_BURST);

        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    g_st_oam_mng_ctx.st_log_ctx.st_ratelimit[en_ratelimit_type].ul_burst = ul_burst;

    return OAL_SUCC;
}


oal_uint32 oam_log_set_ratelimit_param(
    oam_ratelimit_type_enum_uint8  en_ratelimit_type,
    oam_ratelimit_stru *pst_printk_ratelimit)
{
    oal_uint32          ul_ret;

    if (en_ratelimit_type >= OAM_RATELIMIT_TYPE_BUTT) {
        OAM_IO_PRINTK("invalid en_ratelimit_type[%d]. \r\n", en_ratelimit_type);
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    if (OAL_UNLIKELY(pst_printk_ratelimit == OAL_PTR_NULL)) {
        OAM_IO_PRINTK("null param. \r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = oam_log_set_ratelimit_switch(en_ratelimit_type, pst_printk_ratelimit->en_ratelimit_switch);

    ul_ret += oam_log_set_ratelimit_intervel(en_ratelimit_type, pst_printk_ratelimit->ul_interval);

    ul_ret += oam_log_set_ratelimit_burst(en_ratelimit_type, pst_printk_ratelimit->ul_burst);

    return ul_ret;
}


oam_ratelimit_output_enum_uint8 oam_log_ratelimit(oam_ratelimit_type_enum_uint8 en_ratelimit_type)
{
    oal_ulong                            ui_flags;
    oam_ratelimit_stru      	       *pst_ratelimit;
    oam_ratelimit_output_enum_uint8     en_ret;

    pst_ratelimit = &g_st_oam_mng_ctx.st_log_ctx.st_ratelimit[en_ratelimit_type];

    // 判断流控开关状态
    if (pst_ratelimit->en_ratelimit_switch == OAL_SWITCH_OFF) {
        return OAM_RATELIMIT_OUTPUT;
    }

    // 若间隔为0 表明不流控
    if (pst_ratelimit->ul_interval == 0) {
        return OAM_RATELIMIT_OUTPUT;
    }

    oal_spin_lock_irq_save(&pst_ratelimit->spin_lock, &ui_flags);

    // 记录第一条日志的当前时间
    if (pst_ratelimit->ul_begin == 0) {
        pst_ratelimit->ul_begin = OAL_TIME_JIFFY;
    }

    // 起时时间+间隔在当前时间之前，表明间隔时间已经超时，需要重新计数了
    if (oal_time_is_before(pst_ratelimit->ul_begin + pst_ratelimit->ul_interval)) {
        pst_ratelimit->ul_begin   = 0;
        pst_ratelimit->ul_printed = 0;
        pst_ratelimit->ul_missed  = 0;
    }

    /* 若未超时，判断当前时间周期内已输出日志计数是否达到限制输出数 */
    /* 未达到限制的输出日志个数，继续输出 */
    if (pst_ratelimit->ul_burst && (pst_ratelimit->ul_burst > pst_ratelimit->ul_printed)) {
        pst_ratelimit->ul_printed++;
        en_ret = OAM_RATELIMIT_OUTPUT;
    } else {
        /* 达到限制的输出日志个数，不输出；待下一个周期再输出 */
        pst_ratelimit->ul_missed++;
        en_ret = OAM_RATELIMIT_NOT_OUTPUT;
    }

    oal_spin_unlock_irq_restore(&pst_ratelimit->spin_lock, &ui_flags);

    return en_ret;
}


oal_uint32 oam_log_set_vap_switch(oal_uint8 uc_vap_id,
                                  oal_switch_enum_uint8 en_log_switch)
{
    if (OAL_UNLIKELY(uc_vap_id >= WLAN_VAP_SUPPORT_MAX_NUM_LIMIT)) {
        OAM_IO_PRINTK("invalid uc_vap_id[%d]. \r\n", uc_vap_id);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    if (OAL_UNLIKELY(en_log_switch >= OAL_SWITCH_BUTT)) {
        OAM_IO_PRINTK("invalid en_log_switch[%d]. \r\n", en_log_switch);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    g_st_oam_mng_ctx.st_log_ctx.st_vap_log_info.aen_vap_log_switch[uc_vap_id] = en_log_switch;

    return OAL_SUCC;
}


oal_uint32 oam_log_set_vap_level(oal_uint8 uc_vap_id, oam_log_level_enum_uint8 en_log_level)
{
    oam_feature_enum_uint8       en_feature_idx;

    if (OAL_UNLIKELY(uc_vap_id >= WLAN_VAP_SUPPORT_MAX_NUM_LIMIT)) {
        OAM_IO_PRINTK("invalid vap id.[%d] \r\n", uc_vap_id);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    /* 设置当前VAP的日志级别 */
    g_st_oam_mng_ctx.st_log_ctx.st_vap_log_info.aen_vap_log_level[uc_vap_id] = en_log_level;

    /* 同时设置当前VAP下所有特性日志级别 */
    for (en_feature_idx = 0; en_feature_idx < OAM_SOFTWARE_FEATURE_BUTT; en_feature_idx++) {
        oam_log_set_feature_level(uc_vap_id, en_feature_idx, en_log_level);
    }

    return OAL_SUCC;
}


oal_uint32 oam_log_get_vap_level(oal_uint8 uc_vap_id, oam_log_level_enum_uint8 *pen_log_level)
{
    if (OAL_UNLIKELY(uc_vap_id >= WLAN_VAP_SUPPORT_MAX_NUM_LIMIT)) {
        OAM_IO_PRINTK("invalid vap id.[%d] \r\n", uc_vap_id);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    if (OAL_UNLIKELY(pen_log_level == OAL_PTR_NULL)) {
        OAM_IO_PRINTK("null param \r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    *pen_log_level = g_st_oam_mng_ctx.st_log_ctx.st_vap_log_info.aen_vap_log_level[uc_vap_id];

    return OAL_SUCC;
}


oal_uint32 oam_log_set_feature_level(oal_uint8 uc_vap_id,
                                     oam_feature_enum_uint8 en_feature_id,
                                     oam_log_level_enum_uint8 en_log_level)
{
    if (OAL_UNLIKELY(uc_vap_id >= WLAN_VAP_SUPPORT_MAX_NUM_LIMIT)) {
        OAM_IO_PRINTK("invalid uc_vap_id.[%d] \r\n", uc_vap_id);
        return  OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    if (OAL_UNLIKELY(en_feature_id >= OAM_SOFTWARE_FEATURE_BUTT)) {
        OAM_IO_PRINTK("invalid en_feature_id.[%d] \r\n", en_feature_id);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    if (OAL_UNLIKELY((en_log_level >= OAM_LOG_LEVEL_BUTT) || (en_log_level < OAM_LOG_LEVEL_ERROR))) {
        OAM_IO_PRINTK("invalid en_log_level.[%d] \r\n", en_log_level);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    g_st_oam_mng_ctx.st_log_ctx.st_vap_log_info.aen_feature_log_level[uc_vap_id][en_feature_id] = en_log_level;
    return OAL_SUCC;
}


oal_uint32 oam_get_feature_id(const oal_uint8 *puc_feature_name,
                              oam_feature_enum_uint8 *puc_feature_id)
{
    oam_feature_enum_uint8   en_feature_idx;

    if (OAL_UNLIKELY((puc_feature_name == OAL_PTR_NULL) || (puc_feature_id == OAL_PTR_NULL))) {
        OAM_IO_PRINTK("null param \r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    for (en_feature_idx = 0; en_feature_idx < OAM_SOFTWARE_FEATURE_BUTT; en_feature_idx++) {
        if (oal_strcmp((oal_int8 *)gst_oam_feature_list[en_feature_idx].auc_feature_name_abbr,
                       (oal_int8 *)puc_feature_name) == 0) {
            *puc_feature_id = en_feature_idx;
            return OAL_SUCC;
        }
    }

    return OAL_FAIL;
}


oal_uint32 oam_get_feature_name(oam_feature_enum_uint8     en_feature_id,
                                oal_uint8    *puc_feature_name,
                                oal_uint8     uc_size)
{
    oal_uint8       uc_feature_len;

    if (OAL_UNLIKELY(en_feature_id >= OAM_SOFTWARE_FEATURE_BUTT)) {
        OAM_IO_PRINTK("en_feature_id override. %d. \r\n", en_feature_id);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    if (OAL_UNLIKELY(puc_feature_name == OAL_PTR_NULL)) {
        OAM_IO_PRINTK("puc_feature_name is NULL. \r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_feature_len = (oal_uint8)OAL_STRLEN((oal_int8*)gst_oam_feature_list[en_feature_id].auc_feature_name_abbr);
    uc_size = (uc_size > uc_feature_len) ? uc_feature_len : uc_size;

    if (memcpy_s(puc_feature_name, OAM_FEATURE_NAME_ABBR_LEN, gst_oam_feature_list[en_feature_id].auc_feature_name_abbr,
                 uc_size) != EOK) {
        OAM_IO_PRINTK("oam_get_feature_name: memcpy_s fail.\n");
        return OAL_FAIL;
    }
    return OAL_SUCC;
}


oal_uint32 oam_show_feature_list(oal_void)
{
    oam_feature_enum_uint8              en_feature_id;

    OAL_IO_PRINT("feature_list: \r\n");
    for (en_feature_id = 0; en_feature_id < OAM_SOFTWARE_FEATURE_BUTT; en_feature_id++) {
        OAL_IO_PRINT("%s\r\n", gst_oam_feature_list[en_feature_id].auc_feature_name_abbr);
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  oam_log_format_string(
    oal_int8                        *pac_output_data,
    oal_uint16                       us_data_len,
    oal_uint8                        uc_vap_id,
    oam_feature_enum_uint8           en_feature_id,
    oal_uint16                       us_file_id,
    oal_uint16                       us_line_num,
    oam_log_level_enum_uint8         en_log_level,
    const oal_int8                  *pc_string,
    oal_uint8                        uc_param_cnt,
    oal_int32                        l_param1,
    oal_int32                        l_param2,
    oal_int32                        l_param3,
    oal_int32                        l_param4)
{
    oal_int8            *pac_print_level_tbl[] = { "OFF", "ERROR", "WARNING", "INFO" };
    oal_uint32           ul_tick;
    oal_int32           ret;
    oal_uint8            auc_feature_name[OAM_FEATURE_NAME_ABBR_LEN] = {0};
    oal_int8            *pac_print_format[] = {
        "[LOG=%s]:Tick=%lu, FileId=%d, LineNo=%d, VAP=%d, FeatureName=%s, \"%s\", \r\n",
        "[LOG=%s]:Tick=%lu, FileId=%d, LineNo=%d, VAP=%d, FeatureName=%s, \"%s\", %lu \r\n",
        "[LOG=%s]:Tick=%lu, FileId=%d, LineNo=%d, VAP=%d, FeatureName=%s, \"%s\", %lu, %lu \r\n",
        "[LOG=%s]:Tick=%lu, FileId=%d, LineNo=%d, VAP=%d, FeatureName=%s, \"%s\", %lu, %lu, %lu \r\n",
        "[LOG=%s]:Tick=%lu, FileId=%d, LineNo=%d, VAP=%d, FeatureName=%s, \"%s\", %lu, %lu, %lu, %lu \r\n"
    };

    /* 获取系统TICK值 */
    ul_tick = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    oam_get_feature_name(en_feature_id, auc_feature_name, OAL_SIZEOF(auc_feature_name));
    /* 根据参数个数,将LOG信息保存到ac_file_data中 */
    switch (uc_param_cnt) {
        case 0:
            ret = snprintf_s(pac_output_data, us_data_len, us_data_len - 1, pac_print_format[uc_param_cnt],
                             pac_print_level_tbl[en_log_level], ul_tick, us_file_id, us_line_num,
                             uc_vap_id, auc_feature_name, pc_string);
            if ((ret < 0) || (us_data_len < ret)) {
                return OAL_FAIL;
            }

            break;

        case 1:
            ret = snprintf_s(pac_output_data, us_data_len, us_data_len - 1, pac_print_format[uc_param_cnt],
                             pac_print_level_tbl[en_log_level], ul_tick, us_file_id, us_line_num, uc_vap_id,
                             auc_feature_name, pc_string, l_param1);
            if ((ret < 0) || (us_data_len < ret)) {
                return OAL_FAIL;
            }
            break;

        case 2:
            ret = snprintf_s(pac_output_data, us_data_len, us_data_len - 1, pac_print_format[uc_param_cnt],
                             pac_print_level_tbl[en_log_level], ul_tick, us_file_id, us_line_num, uc_vap_id,
                             auc_feature_name, pc_string, l_param1, l_param2);
            if ((ret < 0) || (us_data_len < ret)) {
                return OAL_FAIL;
            }
            break;

        case 3:
            ret = snprintf_s(pac_output_data, us_data_len, us_data_len - 1, pac_print_format[uc_param_cnt],
                             pac_print_level_tbl[en_log_level], ul_tick, us_file_id, us_line_num, uc_vap_id,
                             auc_feature_name, pc_string, l_param1, l_param2, l_param3);
            if ((ret < 0) || (us_data_len < ret)) {
                return OAL_FAIL;
            }
            break;

        case 4:
            ret = snprintf_s(pac_output_data, us_data_len, us_data_len - 1, pac_print_format[uc_param_cnt],
                             pac_print_level_tbl[en_log_level],
                             ul_tick, us_file_id, us_line_num, uc_vap_id, auc_feature_name, pc_string,
                             l_param1, l_param2, l_param3, l_param4);
            if ((ret < 0) || (us_data_len < ret)) {
                return OAL_FAIL;
            }
            break;

        default:
            OAM_IO_PRINTK("invalid uc_param_cnt.[%d] \r\n", uc_param_cnt);
            return OAL_FAIL;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_void  oam_set_log_info_stru(
    oam_log_info_stru               *pst_log_info,
    oal_uint8                        uc_vap_id,
    oam_feature_enum_uint8           en_feature_id,
    oal_uint16                       us_file_id,
    oal_uint16                       us_line_num,
    oam_log_level_enum_uint8         en_log_level,
    oal_int32                        l_param1,
    oal_int32                        l_param2,
    oal_int32                        l_param3,
    oal_int32                        l_param4)
{
    oal_uint32                      ul_tick;

    /* 获取系统TICK值 */
    ul_tick = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    /* 为日志结构体整数成员赋值 */
    pst_log_info->st_vap_log_level.bit_vap_id       = uc_vap_id;
    pst_log_info->st_vap_log_level.bit_log_level    = en_log_level;
    pst_log_info->us_file_id                        = us_file_id;
    pst_log_info->us_line_num                       = us_line_num;
    pst_log_info->en_feature_id                     = en_feature_id;
    pst_log_info->ul_tick                           = ul_tick;
    pst_log_info->al_param[0]                       = l_param1;
    pst_log_info->al_param[1]                       = l_param2;
    pst_log_info->al_param[2]                       = l_param3;
    pst_log_info->al_param[3]                       = l_param4;
}

/* 1151的逻辑，1102该宏内函数不执行 */
#if ((_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)||(_PRE_OS_VERSION_WIN32_RAW == _PRE_OS_VERSION)) /* UT需要部分接口进行测试 */

OAL_STATIC oal_uint32  oam_log_check_param(
    oal_uint8                           uc_vap_id,
    oam_feature_enum_uint8              en_feature_id,
    oam_log_level_enum_uint8            en_log_level)
{
    /* 判断VAP是否合理 */
    if (OAL_UNLIKELY(uc_vap_id >= WLAN_VAP_SUPPORT_MAX_NUM_LIMIT)) {
        OAM_IO_PRINTK("invalid uc_vap_id[%d]. \r\n", uc_vap_id);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    /* 判断特性ID的合理性 */
    if (OAL_UNLIKELY(en_feature_id >= OAM_SOFTWARE_FEATURE_BUTT)) {
        OAM_IO_PRINTK("invalid en_feature_id[%d]. \r\n", en_feature_id);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    /* 判断打印级别的合理性 */
    if (OAL_UNLIKELY(en_log_level >= OAM_LOG_LEVEL_BUTT)) {
        OAM_IO_PRINTK("invalid en_log_level[%d]. \r\n", en_log_level);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 oam_log_get_feature_level(oal_uint8 uc_vap_id,
                                                oam_feature_enum_uint8 en_feature_id,
                                                oam_log_level_enum_uint8 *pen_log_level)
{
    if (OAL_UNLIKELY(uc_vap_id >= WLAN_VAP_SUPPORT_MAX_NUM_LIMIT)) {
        OAM_IO_PRINTK("invalid uc_vap_id.[%d] \r\n", uc_vap_id);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    if (OAL_UNLIKELY(en_feature_id >= OAM_SOFTWARE_FEATURE_BUTT)) {
        OAM_IO_PRINTK("invalid en_feature_id.[%d] \r\n", en_feature_id);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    if (OAL_UNLIKELY(pen_log_level == OAL_PTR_NULL)) {
        OAM_IO_PRINTK("null param \r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    *pen_log_level = g_st_oam_mng_ctx.st_log_ctx.st_vap_log_info.aen_feature_log_level[uc_vap_id][en_feature_id];

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 oam_log_get_global_switch(oal_switch_enum_uint8 *pen_log_switch)
{
    if (OAL_UNLIKELY(pen_log_switch == OAL_PTR_NULL)) {
        OAM_IO_PRINTK("null param. \r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    *pen_log_switch = g_st_oam_mng_ctx.st_log_ctx.en_global_log_switch;
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 oam_log_get_vap_switch(oal_uint8 uc_vap_id,
                                             oal_switch_enum_uint8 *pen_log_switch)
{
    if (OAL_UNLIKELY(uc_vap_id >= WLAN_VAP_SUPPORT_MAX_NUM_LIMIT)) {
        OAM_IO_PRINTK("invalid uc_vap_id[%d] \r\n", uc_vap_id);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    if (OAL_UNLIKELY(pen_log_switch == OAL_PTR_NULL)) {
        OAM_IO_PRINTK("null param \r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    *pen_log_switch = g_st_oam_mng_ctx.st_log_ctx.st_vap_log_info.aen_vap_log_switch[uc_vap_id];

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32  oam_log_print_to_console(
    oal_uint8                        uc_vap_id,
    oam_feature_enum_uint8           en_feature_id,
    oal_uint16                       us_file_id,
    oal_uint16                       us_line_num,
    oam_log_level_enum_uint8         en_log_level,
    oal_int8                        *pc_string,
    oal_uint8                        uc_param_cnt,
    oal_int32                        l_param1,
    oal_int32                        l_param2,
    oal_int32                        l_param3,
    oal_int32                        l_param4)
{
    oal_int8    ac_print_buff[OAM_PRINT_FORMAT_LENGTH]; /* 用于保存写入到文件中的格式 */

    oam_log_format_string(ac_print_buff,
                          OAM_PRINT_FORMAT_LENGTH,
                          uc_vap_id,
                          en_feature_id,
                          us_file_id,
                          us_line_num,
                          en_log_level,
                          pc_string,
                          uc_param_cnt,
                          l_param1,
                          l_param2,
                          l_param3,
                          l_param4);

    oam_print_to_console(ac_print_buff);

    return OAL_SUCC;
}


oal_uint32  oam_log_print_to_file(
    oal_uint8                        uc_vap_id,
    oam_feature_enum_uint8           en_feature_id,
    oal_uint16                       us_file_id,
    oal_uint16                       us_line_num,
    oam_log_level_enum_uint8         en_log_level,
    oal_int8                        *pc_string,
    oal_uint8                        uc_param_cnt,
    oal_int32                        l_param1,
    oal_int32                        l_param2,
    oal_int32                        l_param3,
    oal_int32                        l_param4)
{
#ifdef _PRE_WIFI_DMT
    oal_int8    ac_output_data[OAM_PRINT_FORMAT_LENGTH]; /* 用于保存写入到文件中的格式 */
    oal_uint32  ul_ret;

    oam_log_format_string(ac_output_data, OAM_PRINT_FORMAT_LENGTH, uc_vap_id, en_feature_id,
                          us_file_id, us_line_num, en_log_level, pc_string, uc_param_cnt,
                          l_param1, l_param2, l_param3, l_param4);
    ul_ret = oam_print_to_file(ac_output_data);
    if (ul_ret != OAL_SUCC) {
        return ul_ret;
    }
#endif
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  oam_log_print_to_sdt(
    oal_uint8                        uc_vap_id,
    oam_feature_enum_uint8           en_feature_id,
    oal_uint16                       us_file_id,
    oal_uint16                       us_line_num,
    oam_log_level_enum_uint8         en_log_level,
    oal_int8                        *pc_string,
    oal_int32                        l_param1,
    oal_int32                        l_param2,
    oal_int32                        l_param3,
    oal_int32                        l_param4)
{
    oal_uint32                      ul_ret;
    oam_log_info_stru               st_log_info;

    memset_s(&st_log_info, OAL_SIZEOF(oam_log_info_stru), 0, OAL_SIZEOF(oam_log_info_stru));

    oam_set_log_info_stru(&st_log_info,
                          uc_vap_id,
                          en_feature_id,
                          us_file_id,
                          us_line_num,
                          en_log_level,
                          l_param1,
                          l_param2,
                          l_param3,
                          l_param4);

    /* WARNING和ERROR级别流控 */
    if ((en_log_level != OAM_LOG_LEVEL_INFO)
        && (oam_log_ratelimit(OAM_RATELIMIT_TYPE_LOG) == OAM_RATELIMIT_NOT_OUTPUT)) {
        return OAL_SUCC;
    }

    ul_ret = oam_upload_log_to_sdt((oal_int8 *)&st_log_info);
    if (ul_ret != OAL_SUCC) {
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  oam_log_print_n_param(oal_uint8                        uc_vap_id,
                                             oam_feature_enum_uint8           en_feature_id,
                                             oal_uint16                       us_file_id,
                                             oal_uint16                       us_line_num,
                                             oam_log_level_enum_uint8         en_log_level,
                                             oal_int8                        *pc_string,
                                             oal_uint8                        uc_param_cnt,
                                             oal_int32                        l_param1,
                                             oal_int32                        l_param2,
                                             oal_int32                        l_param3,
                                             oal_int32                        l_param4)
{
    oal_uint32                  ul_ret;
    oam_output_type_enum_uint8  en_output_type;

    if (OAL_UNLIKELY(pc_string == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* 若输出条件满足，判断输出方向 */
    oam_get_output_type(&en_output_type);
    switch (en_output_type) {
        /* 输出至控制台 */
        case OAM_OUTPUT_TYPE_CONSOLE:
            ul_ret = oam_log_print_to_console(uc_vap_id,
                                              en_feature_id,
                                              us_file_id,
                                              us_line_num,
                                              en_log_level,
                                              pc_string,
                                              uc_param_cnt,
                                              l_param1,
                                              l_param2,
                                              l_param3,
                                              l_param4);
            break;

        /* 输出至文件系统中 */
        case OAM_OUTPUT_TYPE_FS:
            ul_ret =  oam_log_print_to_file(uc_vap_id,
                                            en_feature_id,
                                            us_file_id,
                                            us_line_num,
                                            en_log_level,
                                            pc_string,
                                            uc_param_cnt,
                                            l_param1,
                                            l_param2,
                                            l_param3,
                                            l_param4);
            break;

        /* 输出至PC侧调测工具平台 */
        case OAM_OUTPUT_TYPE_SDT:
            ul_ret =  oam_log_print_to_sdt(uc_vap_id,
                                           en_feature_id,
                                           us_file_id,
                                           us_line_num,
                                           en_log_level,
                                           pc_string,
                                           l_param1,
                                           l_param2,
                                           l_param3,
                                           l_param4);

            break;

        /* 无效配置 */
        default:
            ul_ret = OAL_ERR_CODE_INVALID_CONFIG;
            break;
    }

    return ul_ret;
}



oal_uint32  oam_log_print0(oal_uint8                        uc_vap_id,
                           oam_feature_enum_uint8           en_feature_id,
                           oal_uint16                       us_file_id,
                           oal_uint16                       us_line_num,
                           oam_log_level_enum_uint8         en_log_level,
                           oal_int8                        *pc_string,
                           oal_int32                        l_param1,
                           oal_int32                        l_param2,
                           oal_int32                        l_param3,
                           oal_int32                        l_param4)
{
    oal_uint32 ul_ret = OAL_SUCC;
    if (oam_get_log_switch(uc_vap_id, en_feature_id, en_log_level) == OAL_SWITCH_ON) {
        ul_ret = oam_log_print_n_param(uc_vap_id, en_feature_id, us_file_id, us_line_num,
                                       en_log_level, pc_string, 0, 0, 0, 0, 0);
    }

    return ul_ret;
}


oal_uint32  oam_log_print1(oal_uint8                        uc_vap_id,
                           oam_feature_enum_uint8           en_feature_id,
                           oal_uint16                       us_file_id,
                           oal_uint16                       us_line_num,
                           oam_log_level_enum_uint8         en_log_level,
                           oal_int8                        *pc_string,
                           oal_int32                        l_param1,
                           oal_int32                        l_param2,
                           oal_int32                        l_param3,
                           oal_int32                        l_param4)
{
    oal_uint32 ul_ret = OAL_SUCC;
    if (oam_get_log_switch(uc_vap_id, en_feature_id, en_log_level) == OAL_SWITCH_ON) {
        ul_ret = oam_log_print_n_param(uc_vap_id, en_feature_id, us_file_id, us_line_num,\
                                       en_log_level, pc_string, 1, l_param1, 0, 0, 0);
    }

    return ul_ret;
}


oal_uint32  oam_log_print2(oal_uint8                        uc_vap_id,
                           oam_feature_enum_uint8           en_feature_id,
                           oal_uint16                       us_file_id,
                           oal_uint16                       us_line_num,
                           oam_log_level_enum_uint8         en_log_level,
                           oal_int8                        *pc_string,
                           oal_int32                        l_param1,
                           oal_int32                        l_param2,
                           oal_int32                        l_param3,
                           oal_int32                        l_param4)
{
    oal_uint32 ul_ret = OAL_SUCC;
    if (oam_get_log_switch(uc_vap_id, en_feature_id, en_log_level) == OAL_SWITCH_ON) {
        ul_ret = oam_log_print_n_param(uc_vap_id, en_feature_id, us_file_id, us_line_num,\
                                       en_log_level, pc_string, 2, l_param1, l_param2, 0, 0);
    }

    return ul_ret;
}



oal_uint32  oam_log_print3(oal_uint8                        uc_vap_id,
                           oam_feature_enum_uint8           en_feature_id,
                           oal_uint16                       us_file_id,
                           oal_uint16                       us_line_num,
                           oam_log_level_enum_uint8         en_log_level,
                           oal_int8                        *pc_string,
                           oal_int32                        l_param1,
                           oal_int32                        l_param2,
                           oal_int32                        l_param3,
                           oal_int32                        l_param4)
{
    oal_uint32 ul_ret = OAL_SUCC;
    if (oam_get_log_switch(uc_vap_id, en_feature_id, en_log_level) == OAL_SWITCH_ON) {
        ul_ret = oam_log_print_n_param(uc_vap_id, en_feature_id, us_file_id, us_line_num,\
                                       en_log_level, pc_string, 3, l_param1, l_param2, l_param3, 0);
    }

    return ul_ret;
}


oal_uint32  oam_log_print4(oal_uint8                        uc_vap_id,
                           oam_feature_enum_uint8           en_feature_id,
                           oal_uint16                       us_file_id,
                           oal_uint16                       us_line_num,
                           oam_log_level_enum_uint8         en_log_level,
                           oal_int8                        *pc_string,
                           oal_int32                        l_param1,
                           oal_int32                        l_param2,
                           oal_int32                        l_param3,
                           oal_int32                        l_param4)
{
    oal_uint32 ul_ret = OAL_SUCC;
    if (oam_get_log_switch(uc_vap_id, en_feature_id, en_log_level) == OAL_SWITCH_ON) {
        ul_ret = oam_log_print_n_param(uc_vap_id, en_feature_id, us_file_id, us_line_num, en_log_level,\
                                       pc_string, 4, l_param1, l_param2, l_param3, l_param4);
    }

    return ul_ret;
}


OAL_STATIC oal_uint32  oam_log_printk(
    oal_uint16                       us_file_no,
    oal_uint16                       us_line_num,
    const oal_int8                   *pc_func_name,
    const oal_int8                   *pc_args_buf)
{
    oal_int8    ac_output_data[OAM_PRINT_FORMAT_LENGTH]; /* 用于保存写入到文件中的格式 */
    oal_int8    ac_printk_format[] = {"Tick=%lu, FileId=%d, LineNo=%d, FuncName::%s, \"%s\"\r\n"};
    oal_uint32  ul_tick;
    oal_int32  ret;

    ul_tick = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    ret = snprintf_s(ac_output_data, OAM_PRINT_FORMAT_LENGTH, OAM_PRINT_FORMAT_LENGTH - 1,
                     ac_printk_format, ul_tick, us_file_no,
                     us_line_num, pc_func_name, pc_args_buf);
    if ((ret < 0) || (ret > OAM_PRINT_FORMAT_LENGTH)) {
        return OAL_FAIL;
    }

    OAL_IO_PRINT("%s\r\n", ac_output_data);

    return OAL_SUCC;
}


oal_uint32  oam_log_console_printk(
    oal_uint16 us_file_no,
    oal_uint16 us_line_num,
    const oal_int8 *pc_func_name,
    const oal_int8 *pc_fmt, ...)

{
    oal_int8                    ac_args_buf[OAM_PRINT_FORMAT_LENGTH];
    OAL_VA_LIST                 pc_args;
    oal_int32                   ret;

    if (OAL_UNLIKELY((pc_func_name == OAL_PTR_NULL) || (pc_fmt == OAL_PTR_NULL))) {
        OAL_IO_PRINT("null param. \r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 流控判断 */
    if (oam_log_ratelimit(OAM_RATELIMIT_TYPE_PRINTK) == OAM_RATELIMIT_NOT_OUTPUT) {
        return OAL_SUCC;
    }

    OAL_VA_START(pc_args, pc_fmt);
    ret = vsnprintf_s(ac_args_buf, OAM_PRINT_FORMAT_LENGTH, OAM_PRINT_FORMAT_LENGTH - 1, pc_fmt, pc_args);
    OAL_VA_END(pc_args);

    if ((ret < 0) || (ret > OAM_PRINT_FORMAT_LENGTH)) {
        return OAL_FAIL;
    }

    oam_log_printk(us_file_no, us_line_num, pc_func_name, ac_args_buf);

    return OAL_SUCC;
}

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
oal_uint32  oam_log_sdt_out(oal_uint16 us_level,
                            const oal_int8 *pc_func_name,
                            const oal_int8 *pc_fmt, ...)
{
    if (us_level < HISI_MSG_DEBUG) {
        return OAL_FAIL;
    }

    oal_int8                    ac_args_buf[150];
    OAL_VA_LIST                 pc_args;
    oal_int32                   ret;

    if (OAL_UNLIKELY((pc_func_name == OAL_PTR_NULL) || (pc_fmt == OAL_PTR_NULL))) {
        OAL_IO_PRINT("null param. \r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 1131c liteos 上，开源代码 用了wpa_printf接口，自研代码用了HISI_PRINT接口，
    接口均做了上报sdt的适配，对应的流控data_type为OAM_DATA_TYPE_STRING_AND_WPA_PRINTF_AND_HISI_PRINT，
    流控ratelimit_type为OAM_RATELIMIT_TYPE_WPA_PRINTF_AND_HISI_PRINT
    其中流控均默认关闭，支持动态hipriv命令修改流控开关及流控值
    */
    if (oam_log_ratelimit(OAM_RATELIMIT_TYPE_WPA_PRINTF_AND_HISI_PRINT) == OAM_RATELIMIT_NOT_OUTPUT) {
        return OAL_SUCC;
    }

    OAL_VA_START(pc_args, pc_fmt);
    ret = vsnprintf_s(ac_args_buf, BUF_MAX_LENGTH, BUF_MAX_LENGTH - 1, pc_fmt, pc_args);
    OAL_VA_END(pc_args);
    if ((ret < 0) || (ret > BUF_MAX_LENGTH)) {
        return OAL_FAIL;
    }
    oam_log_sdt(pc_func_name, ac_args_buf, ret);
    return OAL_SUCC;
}
#endif


oal_uint32 oam_log_ratelimit_init(oal_void)
{
    oal_uint32                          ul_ret = OAL_SUCC;
    oam_ratelimit_type_enum_uint8       en_type_idx;

    memset_s(&g_st_oam_mng_ctx.st_log_ctx.st_ratelimit,
             OAL_SIZEOF(oam_ratelimit_stru) * OAM_RATELIMIT_TYPE_BUTT, 0,
             OAL_SIZEOF(oam_ratelimit_stru) * OAM_RATELIMIT_TYPE_BUTT);

    for (en_type_idx = 0; en_type_idx < OAM_RATELIMIT_TYPE_BUTT; en_type_idx++) {
        oal_spin_lock_init(&g_st_oam_mng_ctx.st_log_ctx.st_ratelimit[en_type_idx].spin_lock);
        ul_ret += oam_log_set_ratelimit_switch(en_type_idx, OAL_SWITCH_OFF);
        ul_ret += oam_log_set_ratelimit_intervel(en_type_idx, OAM_RATELIMIT_DEFAULT_INTERVAL);
        ul_ret += oam_log_set_ratelimit_burst(en_type_idx, OAM_RATELIMIT_DEFAULT_BURST);
    }
    return ul_ret;
}

oal_void oam_log_param_init(oal_void)
{
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
#ifdef CONFIG_PRINTK
    oal_int32 i;
    for (i = 0; i < OAM_LOG_LEVEL_BUTT; i++) {
        g_loglevel_string[i] = "X";
    }
    g_loglevel_string[OAM_LOG_LEVEL_ERROR] = "E";
    g_loglevel_string[OAM_LOG_LEVEL_WARNING] = "W";
    g_loglevel_string[OAM_LOG_LEVEL_INFO] = "I";
#endif
#endif
}


oal_uint32  oam_log_init(oal_void)
{
    oal_uint8   uc_vap_idx;
    oal_uint32  ul_ret;

    oam_log_param_init();

    /* 日志全局开关默认为开 */
    ul_ret = oam_log_set_global_switch(OAL_SWITCH_ON);
    if (ul_ret != OAL_SUCC) {
        return ul_ret;
    }

    /* VAP级别日志设置 */
    for (uc_vap_idx = 0; uc_vap_idx < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; uc_vap_idx++) {
        /* 设置VAP日志开关 */
        ul_ret += oam_log_set_vap_switch(uc_vap_idx, OAL_SWITCH_ON);

        /* 设置VAP日志级别 */
        ul_ret += oam_log_set_vap_level(uc_vap_idx, OAM_LOG_DEFAULT_LEVEL);

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV)
        /* 设置feature打印级别 */
        ul_ret += oam_log_set_feature_level(uc_vap_idx, OAM_SF_WPA, OAM_LOG_LEVEL_INFO);
#endif
        if (ul_ret != OAL_SUCC) {
            return ul_ret;
        }
    }

    /* printk日志流控初始化 */
    ul_ret = oam_log_ratelimit_init();

    return ul_ret;
}


oal_uint32 oam_exception_record(oal_uint8 uc_vap_id, oam_excp_type_enum_uint8 en_excp_id)
{
    if (OAL_UNLIKELY(uc_vap_id >= WLAN_VAP_SUPPORT_MAX_NUM_LIMIT)) {
        OAM_IO_PRINTK("invalid uc_vap_id[%d] \r\n", uc_vap_id);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    if (OAL_UNLIKELY(en_excp_id >= OAM_EXCP_TYPE_BUTT)) {
        OAM_IO_PRINTK("invalid en_excp_id[%d]. \r\n", en_excp_id);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    g_st_oam_mng_ctx.st_exception_ctx.ast_excp_record_tbl[en_excp_id].ul_record_cnt++;

    /* 已刷新，可上报 */
    g_st_oam_mng_ctx.st_exception_ctx.en_status = OAM_EXCP_STATUS_REFRESHED;

    g_st_oam_mng_ctx.st_exception_ctx.ast_excp_record_tbl[en_excp_id].en_status = OAM_EXCP_STATUS_REFRESHED;

    return OAL_SUCC;
}


oal_uint32 oam_exception_stat_report(
    oal_uint8 uc_vap_id,
    oam_excp_type_enum_uint8 en_excp_id,
    oal_uint32 ul_cnt)
{
    OAM_ERROR_LOG1(uc_vap_id, OAM_SF_ANY, g_oammacerr_print[en_excp_id], ul_cnt);
    return OAL_SUCC;
}


oal_void oam_exception_stat_handler(oal_uint8 en_module_id, oal_uint8 uc_vap_idx)
{
    oam_excp_record_stru           *pst_excp_record = OAL_PTR_NULL;
    oam_excp_type_enum_uint8   en_excp_idx;

    if (en_module_id == OM_WIFI) {
        /* 当前VAP异常统计为0 */
        if (g_st_oam_mng_ctx.st_exception_ctx.en_status == OAM_EXCP_STATUS_REFRESHED) {
            pst_excp_record = g_st_oam_mng_ctx.st_exception_ctx.ast_excp_record_tbl;

            for (en_excp_idx = 0; en_excp_idx < OAM_EXCP_TYPE_BUTT; en_excp_idx++) {
                /* 记录数已刷新 */
                if (pst_excp_record[en_excp_idx].en_status == OAM_EXCP_STATUS_REFRESHED) {
                    oam_exception_stat_report(uc_vap_idx, en_excp_idx, pst_excp_record[en_excp_idx].ul_record_cnt);
                    g_st_oam_mng_ctx.st_exception_ctx.ast_excp_record_tbl[en_excp_idx].en_status = OAM_EXCP_STATUS_INIT;
                }
            }

            /* 已上报，置初始状态 */
            g_st_oam_mng_ctx.st_exception_ctx.en_status = OAM_EXCP_STATUS_INIT;
        }
    }
}

/*lint -e19*/
oal_module_symbol(OAL_PRINT2KERNEL);
oal_module_symbol(OAL_PRINT_NLOGS);
oal_module_symbol(oam_log_print0);
oal_module_symbol(oam_log_set_global_switch);
oal_module_symbol(oam_log_set_vap_switch);
oal_module_symbol(oam_log_set_vap_level);
oal_module_symbol(oam_log_set_feature_level);
oal_module_symbol(oam_log_console_printk);
oal_module_symbol(oam_log_set_ratelimit_param);
oal_module_symbol(oam_get_feature_id);
oal_module_symbol(oam_log_get_vap_level);
oal_module_symbol(oam_show_feature_list);

oal_module_symbol(oam_log_print1);
oal_module_symbol(oam_log_print2);
oal_module_symbol(oam_log_print3);
oal_module_symbol(oam_log_print4);
oal_module_symbol(oam_exception_record);
oal_module_symbol(oam_exception_stat_handler);

#ifdef _PRE_DEBUG_MODE
oal_module_symbol(g_ast_tx_complete_stat);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

