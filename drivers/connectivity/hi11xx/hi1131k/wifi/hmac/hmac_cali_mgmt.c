

#if(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "hmac_cali_mgmt.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CALI_MGMT_C

extern oal_uint32 wlan_pm_close(oal_void);
/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
/*****************************************************************************
  3 函数实现
*****************************************************************************/
oal_void hmac_add_bound(oal_uint32 *pul_number, oal_uint32 ul_bound)
{
    *pul_number = *pul_number + 1;

    if (*pul_number > (ul_bound - 1)) {
        *pul_number -= ul_bound;
    }
}

/*lint -e571*/
/*lint -e801*/
#ifdef _PRE_WLAN_FEATURE_5G
OAL_STATIC oal_int32 hmac_print_cail_result(oal_uint8 uc_cali_chn_idx,
    oal_int8 *pc_print_buff, oal_uint32 ul_remainder_len, oal_cali_param_stru *pst_cali_data)
{
    oal_int8 *pc_string;

    pc_string = (uc_cali_chn_idx < OAL_5G_20M_CHANNEL_NUM)
        ? "5G 20M cali data index:%d, rx_dc_comp:0x%x, s_digital_rxdc_cmp_i:0x%x, us_digital_rxdc_cmp_q:0x%x \n"
        "5G 20M tx_power upc_ppa_cmp:0x%x, upc_mx_cmp:0x%x, atx_pwr_cmp:0x%x,  ppf:0x%x \n"
        "5G 20M tx_dc i:%u,  q:%u, tx_iq p:%u  e:%u \n"
        : "5G 80M cali data index:%d, rx_dc_comp:0x%x, s_digital_rxdc_cmp_i:0x%x, us_digital_rxdc_cmp_q:0x%x \n"
        "5G 80M tx_power upc_ppa_cmp:0x%x, upc_mx_cmp:0x%x, atx_pwr_cmp:0x%x,  ppf:0x%x \n"
        "5G 80M tx_dc i:%u,  q:%u, tx_iq p:%u  e:%u \n" ;
    return snprintf_s(pc_print_buff, ul_remainder_len, ul_remainder_len - 1, pc_string,
        uc_cali_chn_idx,
        pst_cali_data->ast_5Gcali_param[uc_cali_chn_idx].g_st_cali_rx_dc_cmp_5G.us_analog_rxdc_cmp,
        pst_cali_data->ast_5Gcali_param[uc_cali_chn_idx].g_st_cali_rx_dc_cmp_5G.us_digital_rxdc_cmp_i,
        pst_cali_data->ast_5Gcali_param[uc_cali_chn_idx].g_st_cali_rx_dc_cmp_5G.us_digital_rxdc_cmp_q,
        pst_cali_data->ast_5Gcali_param[uc_cali_chn_idx].g_st_cali_tx_power_cmp_5G.upc_ppa_cmp,
        pst_cali_data->ast_5Gcali_param[uc_cali_chn_idx].g_st_cali_tx_power_cmp_5G.upc_mx_cmp,
        (oal_uint8)pst_cali_data->ast_5Gcali_param[uc_cali_chn_idx].g_st_cali_tx_power_cmp_5G.ac_atx_pwr_cmp,
        pst_cali_data->ast_5Gcali_param[uc_cali_chn_idx].g_st_ppf_cmp_val.uc_ppf_val,
        pst_cali_data->ast_5Gcali_param[uc_cali_chn_idx].g_st_txdc_cmp_val.us_txdc_cmp_i,
        pst_cali_data->ast_5Gcali_param[uc_cali_chn_idx].g_st_txdc_cmp_val.us_txdc_cmp_q,
        pst_cali_data->ast_5Gcali_param[uc_cali_chn_idx].g_st_txiq_cmp_val_5G.us_txiq_cmp_p,
        pst_cali_data->ast_5Gcali_param[uc_cali_chn_idx].g_st_txiq_cmp_val_5G.us_txiq_cmp_e);
}
#endif

extern oal_uint32 band_5g_enabled;
oal_void hmac_dump_cali_result(oal_void)
{
    oal_cali_param_stru *pst_cali_data;
    oal_uint8            uc_cali_chn_idx;
#ifdef _PRE_WLAN_FEATURE_5G
    oal_uint32           ul_string_len;
    oal_int32            l_string_tmp_len;
    oal_int8            *pc_print_buff;
#endif
    pst_cali_data = (oal_cali_param_stru *)get_cali_data_buf_addr();

    OAM_WARNING_LOG4(0, OAM_SF_CFG, "rc code RC:0x%x, R:0x%x, C:0x%x, check_hw_status:0x%x",
                     pst_cali_data->st_bfgn_cali_data.g_uc_rc_cmp_code,
                     pst_cali_data->st_bfgn_cali_data.g_uc_r_cmp_code,
                     pst_cali_data->st_bfgn_cali_data.g_uc_c_cmp_code,
                     pst_cali_data->ul_check_hw_status);
    for (uc_cali_chn_idx = 0; uc_cali_chn_idx < OAL_2G_CHANNEL_NUM; uc_cali_chn_idx++) {
        OAM_WARNING_LOG4(0, OAM_SF_CFG,
            "2G cali data index:%u, rx_dc_comp: 0x%x, us_digital_rxdc_cmp_i:0x%x, us_digital_rxdc_cmp_q:0x%x",
            uc_cali_chn_idx,
            pst_cali_data->ast_2Gcali_param[uc_cali_chn_idx].g_st_cali_rx_dc_cmp_2G.us_analog_rxdc_cmp,
            pst_cali_data->ast_2Gcali_param[uc_cali_chn_idx].g_st_cali_rx_dc_cmp_2G.us_digital_rxdc_cmp_i,
            pst_cali_data->ast_2Gcali_param[uc_cali_chn_idx].g_st_cali_rx_dc_cmp_2G.us_digital_rxdc_cmp_q);

        OAM_WARNING_LOG4(0, OAM_SF_CFG,
            "2G cali data index:%u, upc_ppa_cmp:0x%x, ac_atx_pwr_cmp:0x%x, dtx_pwr_cmp:0x%x",
            uc_cali_chn_idx,
            pst_cali_data->ast_2Gcali_param[uc_cali_chn_idx].g_st_cali_tx_power_cmp_2G.upc_ppa_cmp,
            (oal_uint8)pst_cali_data->ast_2Gcali_param[uc_cali_chn_idx].g_st_cali_tx_power_cmp_2G.ac_atx_pwr_cmp,
            (oal_uint8)pst_cali_data->ast_2Gcali_param[uc_cali_chn_idx].g_st_cali_tx_power_cmp_2G.dtx_pwr_cmp);

        OAM_WARNING_LOG4(0, OAM_SF_CFG, " 2G tx_dc i:%u q:%u, tx_iq p:%u  e:%u",
                         pst_cali_data->ast_2Gcali_param[uc_cali_chn_idx].g_st_txdc_cmp_val.us_txdc_cmp_i,
                         pst_cali_data->ast_2Gcali_param[uc_cali_chn_idx].g_st_txdc_cmp_val.us_txdc_cmp_q,
                         pst_cali_data->ast_2Gcali_param[uc_cali_chn_idx].g_st_txiq_cmp_val_2G.us_txiq_cmp_p,
                         pst_cali_data->ast_2Gcali_param[uc_cali_chn_idx].g_st_txiq_cmp_val_2G.us_txiq_cmp_e);
    }

#ifdef _PRE_WLAN_FEATURE_5G
    if (band_5g_enabled == OAL_FALSE) {
        return;
    }

    pc_print_buff = (oal_int8 *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAM_REPORT_MAX_STRING_LEN, OAL_TRUE);
    if (pc_print_buff == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_dump_cali_result::pc_print_buff null.}", OAM_REPORT_MAX_STRING_LEN);
        return;
    }

    memset_s(pc_print_buff, OAM_REPORT_MAX_STRING_LEN, 0, OAM_REPORT_MAX_STRING_LEN);
    ul_string_len    = 0;

    for (uc_cali_chn_idx = 0; uc_cali_chn_idx < OAL_5G_20M_CHANNEL_NUM; uc_cali_chn_idx++) {
        l_string_tmp_len = hmac_print_cail_result(uc_cali_chn_idx, pc_print_buff + ul_string_len,
            (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1), pst_cali_data);
        if (l_string_tmp_len < 0) {
            goto sprint_fail;
        }
        ul_string_len += (oal_uint32)l_string_tmp_len;
    }
    pc_print_buff[OAM_REPORT_MAX_STRING_LEN - 1] = '\0';
    oam_print(pc_print_buff);

    /* 上述日志量超过OAM_REPORT_MAX_STRING_LEN，分多次oam_print */
    memset_s(pc_print_buff, OAM_REPORT_MAX_STRING_LEN, 0, OAM_REPORT_MAX_STRING_LEN);
    ul_string_len    = 0;

    for (uc_cali_chn_idx = OAL_5G_20M_CHANNEL_NUM; uc_cali_chn_idx < OAL_5G_CHANNEL_NUM; uc_cali_chn_idx++) {
        l_string_tmp_len = hmac_print_cail_result(uc_cali_chn_idx, pc_print_buff + ul_string_len,
            (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1), pst_cali_data);
        if (l_string_tmp_len < 0) {
            goto sprint_fail;
        }
        ul_string_len += (oal_uint32)l_string_tmp_len;
    }
    pc_print_buff[OAM_REPORT_MAX_STRING_LEN - 1] = '\0';
    oam_print(pc_print_buff);

    /* 上述日志量超过OAM_REPORT_MAX_STRING_LEN，分多次oam_print */
    memset_s(pc_print_buff, OAM_REPORT_MAX_STRING_LEN, 0, OAM_REPORT_MAX_STRING_LEN);
    ul_string_len    = 0;

#ifdef _PRE_WLAN_NEW_IQ
    for (uc_cali_chn_idx = 0; uc_cali_chn_idx < OAL_5G_20M_CHANNEL_NUM; uc_cali_chn_idx++) {
        l_string_tmp_len = snprintf_s(pc_print_buff + ul_string_len, (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1),
                                      (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1) - 1,
                                      "uc_cali_chn_idx:%d \n"
                                      "RX_IQ udelay1:0x%x, udelay2:0x%x, alpha:0x%x, beta:0x%x \n",
                                      uc_cali_chn_idx,
                                      pst_cali_data->ast_new_rxiq_cmp_val_5G[uc_cali_chn_idx].ul_rxiq_cmp_u1,
                                      pst_cali_data->ast_new_rxiq_cmp_val_5G[uc_cali_chn_idx].ul_rxiq_cmp_u2,
                                      pst_cali_data->ast_new_rxiq_cmp_val_5G[uc_cali_chn_idx].ul_rxiq_cmp_alpha,
                                      pst_cali_data->ast_new_rxiq_cmp_val_5G[uc_cali_chn_idx].ul_rxiq_cmp_beta);
        if (l_string_tmp_len < 0) {
            goto sprint_fail;
        }
        ul_string_len += (oal_uint32)l_string_tmp_len;
    }
    pc_print_buff[OAM_REPORT_MAX_STRING_LEN - 1] = '\0';
    oam_print(pc_print_buff);
#endif

    OAL_MEM_FREE(pc_print_buff, OAL_TRUE);
    return;

sprint_fail:
    OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_dump_cali_result:: snprintf_s return error!}");
    pc_print_buff[OAM_REPORT_MAX_STRING_LEN - 1] = '\0';
    oam_print(pc_print_buff);
    OAL_MEM_FREE(pc_print_buff, OAL_TRUE);

    return;
#endif
}
/*lint +e801*/
/*lint +e571*/
oal_uint32  hmac_save_cali_event(frw_event_mem_stru *pst_event_mem)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    frw_event_stru               *frw_event = OAL_PTR_NULL;
    hal_cali_hal2hmac_event_stru *cali_save_event = OAL_PTR_NULL;
    oal_netbuf_stru              *netbuf = OAL_PTR_NULL;
    uint8_t                      *cali_param = OAL_PTR_NULL;
    host_cali_stru               *cali_data = OAL_PTR_NULL;

    if (pst_event_mem == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_save_cali_event:: pst_event_mem null.}");
        return OAL_FAIL;
    }

    frw_event              = (frw_event_stru *)pst_event_mem->puc_data;
    cali_save_event        = (hal_cali_hal2hmac_event_stru *)frw_event->auc_event_data;
    netbuf                 = cali_save_event->pst_netbuf;
    cali_param             = (uint8_t *)OAL_NETBUF_DATA(netbuf);
    cali_data              = (host_cali_stru *)get_cali_data_buf_addr();
    cali_data->cali_len    = OAL_NETBUF_LEN(netbuf);

    OAM_WARNING_LOG1(0, OAM_SF_CFG, "hmac_save_cali_event %d", OAL_NETBUF_LEN(netbuf));

    if (memcpy_s(cali_data->cali_data, CALI_MAX_LEN, cali_param, cali_data->cali_len) != EOK) {
        OAM_ERROR_LOG2(0, OAM_SF_CALIBRATE, "hmac_save_cali_event::memcpy fail! %d %d",
                       CALI_MAX_LEN, cali_data->cali_len);
        oal_netbuf_free(netbuf);
        return OAL_FAIL;
    }
    cali_data->saved_flag = OAL_TRUE;
    oal_netbuf_free(netbuf);
    hmac_dump_cali_result();
    if (g_uc_netdev_is_open == OAL_FALSE) {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_save_cali_event: wlan_pm_close,g_uc_netdev_is_open = FALSE!.}");
        wlan_pm_close();
        return OAL_SUCC;
    }
#endif
    return OAL_SUCC;
}

oal_uint32 hmac_send_cali_data(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    frw_event_mem_stru *event_mem = OAL_PTR_NULL;
    frw_event_stru     *frw_event = OAL_PTR_NULL;
    dmac_tx_event_stru *dmac_tx_event = OAL_PTR_NULL;
    oal_netbuf_stru    *netbuf_cali_data = OAL_PTR_NULL;
    uint8_t            *cali_data = OAL_PTR_NULL;

    if (pst_mac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_CALIBRATE, "{hmac_dpd_data_processed_send::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_tx_event_stru));
    if (event_mem == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CALIBRATE,
            "{hmac_scan_proc_scan_req_event::pst_event_mem null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_IO_PRINT("{hmac_send_cali_data.start %d %d}\r\n", DMAC_WLAN_CTX_EVENT_SUB_TYPE_CALI_HMAC2DMAC, us_len);
    netbuf_cali_data = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_LARGE_NETBUF_SIZE, OAL_NETBUF_PRIORITY_MID);
    if (netbuf_cali_data == OAL_PTR_NULL) {
        FRW_EVENT_FREE(event_mem);
        OAM_ERROR_LOG0(0, OAM_SF_CALIBRATE, "{hmac_dpd_data_processed_send::pst_netbuf_scan_result null.}");

        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    frw_event = (frw_event_stru *)event_mem->puc_data;

    FRW_EVENT_HDR_INIT(&(frw_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_CALI_HMAC2DMAC,
                       OAL_SIZEOF(dmac_tx_event_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_mac_vap->uc_chip_id,
                       pst_mac_vap->uc_device_id,
                       pst_mac_vap->uc_vap_id);

    memset_s(oal_netbuf_cb(netbuf_cali_data), OAL_TX_CB_LEN, 0, OAL_TX_CB_LEN);
    cali_data = (oal_uint8 *)(OAL_NETBUF_DATA(netbuf_cali_data));
    if (memcpy_s(cali_data, WLAN_LARGE_NETBUF_SIZE, puc_param, us_len) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_CALIBRATE, "hmac_send_cali_data::memcpy fail!");
        oal_netbuf_free(netbuf_cali_data);
        FRW_EVENT_FREE(event_mem);
        return OAL_FAIL;
    }

    dmac_tx_event               = (dmac_tx_event_stru *)frw_event->auc_event_data;
    dmac_tx_event->pst_netbuf   = netbuf_cali_data;
    dmac_tx_event->us_frame_len = us_len;
    frw_event_dispatch_event(event_mem);

    oal_netbuf_free(netbuf_cali_data);
    FRW_EVENT_FREE(event_mem);

    return OAL_SUCC;
}


#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

