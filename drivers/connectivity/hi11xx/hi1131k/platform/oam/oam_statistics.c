

#include "oam_main.h"
#include "oam_statistics.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAM_STATISTICS_C

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
/* 统计信息全局变量 */
oam_stat_info_stru g_st_stat_info;
#endif

oal_void  oam_stats_report_irq_info_to_sdt(
    const oal_uint8 *puc_irq_info_addr,
    oal_uint16  us_irq_info_len)
{
    oal_uint32           ul_tick;
    oal_uint16           us_skb_len;        /* skb总长度 */
    oal_netbuf_stru     *pst_netbuf = OAL_PTR_NULL;
    oam_ota_stru        *pst_ota_data = OAL_PTR_NULL;

    if (OAL_UNLIKELY(g_st_oam_sdt_func_hook.p_sdt_report_data_func == OAL_PTR_NULL)) {
        return ;
    }

    if (puc_irq_info_addr == OAL_PTR_NULL) {
        OAL_IO_PRINT("oam_stats_report_irq_info_to_sdt::puc_irq_info_addr is null!\n");
        return;
    }

    /* 为上报描述符申请空间,头部预留8字节，尾部预留1字节，给sdt_drv用 */
    us_skb_len = us_irq_info_len + OAL_SIZEOF(oam_ota_hdr_stru);
    if (us_skb_len > WLAN_SDT_NETBUF_MAX_PAYLOAD) {
        us_skb_len = WLAN_SDT_NETBUF_MAX_PAYLOAD;
        us_irq_info_len = WLAN_SDT_NETBUF_MAX_PAYLOAD - OAL_SIZEOF(oam_ota_hdr_stru);
    }

    pst_netbuf = oam_alloc_data2sdt(us_skb_len);
    if (pst_netbuf == OAL_PTR_NULL) {
        return;
    }

    pst_ota_data = (oam_ota_stru *)oal_netbuf_data(pst_netbuf);

    /* 获取系统TICK值 */
    ul_tick = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    /* 填写ota消息头结构体 */
    pst_ota_data->st_ota_hdr.ul_tick     = ul_tick;
    pst_ota_data->st_ota_hdr.en_ota_type = OAM_OTA_TYPE_IRQ;
    pst_ota_data->st_ota_hdr.uc_frame_hdr_len = 0;
    pst_ota_data->st_ota_hdr.us_ota_data_len = us_irq_info_len;
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
    pst_ota_data->st_ota_hdr.auc_resv[0]    = 2;
#else
    pst_ota_data->st_ota_hdr.auc_resv[0]    = 3;
#endif

    /* 复制数据,填写ota数据 */
    if (memcpy_s((oal_void *)pst_ota_data->auc_ota_data,
                 pst_ota_data->st_ota_hdr.us_ota_data_len,
                 (const oal_void *)puc_irq_info_addr,
                 (oal_uint32)us_irq_info_len) != EOK) {
        OAL_IO_PRINT("oam_stats_report_irq_info_to_sdt: memcpy_s failed! \n");
        oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
        return;
    }

    /* 下发至sdt接收队列，若队列满则串口输出 */
    oam_report_data2sdt(pst_netbuf, OAM_DATA_TYPE_OTA, OAM_PRIMID_TYPE_OUTPUT_CONTENT);
}



oal_uint32  oam_stats_report_timer_info_to_sdt(
    oal_uint8 *puc_timer_addr,
    oal_uint8  uc_timer_len)
{
    oal_uint32 ul_ret;

    if (puc_timer_addr != NULL) {
        ul_ret = oam_ota_report(puc_timer_addr, uc_timer_len, 0, 0, OAM_OTA_TYPE_TIMER);
        return ul_ret;
    } else {
        OAL_IO_PRINT("oam_stats_report_timer_info_to_sdt::puc_timer_addr is NULL");
        return OAL_ERR_CODE_PTR_NULL;
    }
}


oal_uint32 oam_stats_report_mempool_info_to_sdt(
    oal_uint8           uc_pool_id,
    oal_uint16          us_pool_total_cnt,
    oal_uint16          us_pool_used_cnt,
    oal_uint8           uc_subpool_id,
    oal_uint16          us_subpool_total_cnt,
    oal_uint16          us_subpool_free_cnt)
{
    oam_stats_mempool_stru        st_device_mempool_info;
    oal_netbuf_stru              *pst_netbuf = OAL_PTR_NULL;
    oam_ota_stru                 *pst_ota_data = OAL_PTR_NULL;
    oal_uint16                    us_skb_len;        /* skb总长度 */
    oal_uint32                    ul_tick;
    oal_uint32                    ul_ret;
    oal_uint16                    us_stru_len;

    if (OAL_UNLIKELY(g_st_oam_sdt_func_hook.p_sdt_report_data_func == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    memset_s(&st_device_mempool_info,  sizeof(st_device_mempool_info), 0, sizeof(st_device_mempool_info));
    /* 填写要上报给sdt的内存池信息结构体 */
    st_device_mempool_info.uc_mem_pool_id          = uc_pool_id;
    st_device_mempool_info.uc_subpool_id           = uc_subpool_id;
    st_device_mempool_info.us_mem_pool_total_cnt   = us_pool_total_cnt;
    st_device_mempool_info.us_mem_pool_used_cnt    = us_pool_used_cnt;
    st_device_mempool_info.us_subpool_total_cnt    = us_subpool_total_cnt;
    st_device_mempool_info.us_subpool_free_cnt     = us_subpool_free_cnt;

    us_stru_len = OAL_SIZEOF(oam_stats_mempool_stru);
    /* 为ota消息上报SDT申请空间,头部预留8字节，尾部预留1字节，给sdt_drv用 */
    us_skb_len = us_stru_len + OAL_SIZEOF(oam_ota_hdr_stru);
    if (us_skb_len > WLAN_SDT_NETBUF_MAX_PAYLOAD) {
        us_skb_len = WLAN_SDT_NETBUF_MAX_PAYLOAD;
        us_stru_len = us_skb_len - OAL_SIZEOF(oam_ota_hdr_stru);
    }

    pst_netbuf = oam_alloc_data2sdt(us_skb_len);
    if (pst_netbuf == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_ota_data = (oam_ota_stru *)oal_netbuf_data(pst_netbuf);

    /* 获取系统TICK值 */
    ul_tick = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    /* 填写ota消息结构体 */
    pst_ota_data->st_ota_hdr.ul_tick = ul_tick;
    pst_ota_data->st_ota_hdr.en_ota_type = OAM_OTA_TYPE_MEMPOOL;
    pst_ota_data->st_ota_hdr.uc_frame_hdr_len = 0;
    pst_ota_data->st_ota_hdr.us_ota_data_len = us_stru_len;

    /* 复制数据,填写ota数据 */
    if (memcpy_s((oal_void *)pst_ota_data->auc_ota_data, pst_ota_data->st_ota_hdr.us_ota_data_len,
                 (const oal_void *)&st_device_mempool_info,
                 (oal_uint32)us_stru_len) != EOK) {
        oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
        return OAL_FAIL;
    }
    /* 下发至sdt接收队列，若队列满则串口输出 */
    ul_ret = oam_report_data2sdt(pst_netbuf, OAM_DATA_TYPE_OTA, OAM_PRIMID_TYPE_OUTPUT_CONTENT);

    return ul_ret;
}


oal_uint32  oam_stats_report_memblock_info_to_sdt(
    const oal_uint8  *puc_origin_data,
    oal_uint8   uc_user_cnt,
    oal_uint8   uc_pool_id,
    oal_uint8   uc_subpool_id,
    oal_uint16  us_len,
    oal_uint32  ul_file_id,
    oal_uint32  ul_alloc_line_num)
{
    oam_memblock_info_stru        st_memblock_info;
    oal_uint16                    us_memblock_info_len;
    oal_netbuf_stru              *pst_netbuf = OAL_PTR_NULL;
    oam_ota_stru                 *pst_ota_data = OAL_PTR_NULL;
    oal_uint16                    us_skb_len;        /* skb总长度 */
    oal_uint32                    ul_tick;
    oal_uint32                    ul_ret;

    if (OAL_UNLIKELY(g_st_oam_sdt_func_hook.p_sdt_report_data_func == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (puc_origin_data == OAL_PTR_NULL) {
        OAL_IO_PRINT("oam_stats_report_memblock_info_to_sdt:puc_origin_data is null!\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    us_memblock_info_len = OAL_SIZEOF(oam_memblock_info_stru);

    memset_s(&st_memblock_info,  sizeof(st_memblock_info), 0, sizeof(st_memblock_info));
    /* 填写要上报给sdt的内存块信息结构体 */
    st_memblock_info.uc_pool_id         = uc_pool_id;
    st_memblock_info.uc_subpool_id      = uc_subpool_id;
    st_memblock_info.uc_user_cnt        = uc_user_cnt;
    st_memblock_info.ul_alloc_line_num  = ul_alloc_line_num;
    st_memblock_info.ul_file_id         = ul_file_id;

    /* 为ota消息上报SDT申请空间,头部预留8字节，尾部预留1字节，给sdt_drv用 */
    us_skb_len = us_memblock_info_len + us_len + OAL_SIZEOF(oam_ota_hdr_stru);
    if (us_skb_len > WLAN_SDT_NETBUF_MAX_PAYLOAD) {
        us_skb_len = WLAN_SDT_NETBUF_MAX_PAYLOAD;
        if ((us_memblock_info_len + OAL_SIZEOF(oam_ota_hdr_stru)) < us_skb_len) {
            us_len = us_skb_len - us_memblock_info_len - (oal_uint16)OAL_SIZEOF(oam_ota_hdr_stru);
        } else {
            us_memblock_info_len = us_skb_len - OAL_SIZEOF(oam_ota_hdr_stru);
            us_len = 0;
        }
    }

    pst_netbuf = oam_alloc_data2sdt(us_skb_len);
    if (pst_netbuf == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_ota_data = (oam_ota_stru *)oal_netbuf_data(pst_netbuf);

    /* 获取系统TICK值 */
    ul_tick = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    /* 填写ota消息结构体 */
    pst_ota_data->st_ota_hdr.ul_tick          = ul_tick;
    pst_ota_data->st_ota_hdr.en_ota_type      = OAM_OTA_TYPE_MEMBLOCK;
    pst_ota_data->st_ota_hdr.uc_frame_hdr_len = (oal_uint8)us_memblock_info_len;
    pst_ota_data->st_ota_hdr.us_ota_data_len  = us_memblock_info_len + us_len;

    /* 填写ota数据部分,首先复制内存块的信息结构体 */
    ul_ret = (oal_uint32)memcpy_s((oal_void *)pst_ota_data->auc_ota_data, pst_ota_data->st_ota_hdr.us_ota_data_len,
                                  (const oal_void *)&st_memblock_info, (oal_uint32)us_memblock_info_len);

    /* 复制内存块的具体内容 *//*lint -e416*/
    ul_ret += (oal_uint32)memcpy_s((oal_void *)(pst_ota_data->auc_ota_data + us_memblock_info_len),
                                   pst_ota_data->st_ota_hdr.us_ota_data_len - us_memblock_info_len,
                                   (const oal_void *)puc_origin_data, (oal_uint32)us_len);
    if (ul_ret != EOK) {
        OAL_IO_PRINT("oam_stats_report_memblock_info_to_sdt: memcpy_s failed! \n");
        oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
        return OAL_FAIL;
    }
    /*lint +e416*/
    /* 下发至sdt接收队列，若队列满则串口输出 */
    ul_ret = oam_report_data2sdt(pst_netbuf, OAM_DATA_TYPE_OTA, OAM_PRIMID_TYPE_OUTPUT_CONTENT);

    return ul_ret;
}



oal_uint32  oam_stats_report_event_queue_info_to_sdt(
    const oal_uint8   *puc_event_queue_addr,
    oal_uint16    us_event_queue_info_len)
{
    oal_uint32           ul_tick;
    oal_uint16           us_skb_len;        /* skb总长度 */
    oal_netbuf_stru     *pst_netbuf = OAL_PTR_NULL;
    oam_ota_stru        *pst_ota_data = OAL_PTR_NULL;
    oal_uint32           ul_ret;

    if (OAL_UNLIKELY(g_st_oam_sdt_func_hook.p_sdt_report_data_func == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (puc_event_queue_addr == OAL_PTR_NULL) {
        OAL_IO_PRINT("oam_stats_report_event_queue_info_to_sdt::puc_event_queue_addr is null!\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 为上报描述符申请空间,头部预留8字节，尾部预留1字节，给sdt_drv用 */
    us_skb_len = us_event_queue_info_len + OAL_SIZEOF(oam_ota_hdr_stru);
    if (us_skb_len > WLAN_SDT_NETBUF_MAX_PAYLOAD) {
        us_skb_len = WLAN_SDT_NETBUF_MAX_PAYLOAD;
        us_event_queue_info_len = WLAN_SDT_NETBUF_MAX_PAYLOAD - OAL_SIZEOF(oam_ota_hdr_stru);
    }

    pst_netbuf = oam_alloc_data2sdt(us_skb_len);
    if (pst_netbuf == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_ota_data = (oam_ota_stru *)oal_netbuf_data(pst_netbuf);

    /* 获取系统TICK值 */
    ul_tick = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    /* 填写ota消息头结构体 */
    pst_ota_data->st_ota_hdr.ul_tick     = ul_tick;
    pst_ota_data->st_ota_hdr.en_ota_type = OAM_OTA_TYPE_EVENT_QUEUE;
    pst_ota_data->st_ota_hdr.uc_frame_hdr_len = 0;
    pst_ota_data->st_ota_hdr.us_ota_data_len = us_event_queue_info_len;

    /* 复制数据,填写ota数据 */
    if (memcpy_s((oal_void *)pst_ota_data->auc_ota_data,
                 pst_ota_data->st_ota_hdr.us_ota_data_len,
                 (const oal_void *)puc_event_queue_addr,
                 (oal_uint32)us_event_queue_info_len) != EOK) {
        OAL_IO_PRINT("oam_stats_report_event_queue_info_to_sdt: memcpy_s failed! \n");
        oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
        return OAL_FAIL;
    }

    /* 下发至sdt接收队列，若队列满则串口输出 */
    ul_ret = oam_report_data2sdt(pst_netbuf, OAM_DATA_TYPE_OTA, OAM_PRIMID_TYPE_OUTPUT_CONTENT);

    return ul_ret;
}
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

oal_uint32  oam_report_vap_pkt_stat_to_sdt(oal_uint8 uc_vap_id)
{
    oal_uint32           ul_tick;
    oal_uint16           us_skb_len;        /* skb总长度 */
    oal_netbuf_stru     *pst_netbuf = OAL_PTR_NULL;
    oam_ota_stru        *pst_ota_data = OAL_PTR_NULL;
    oal_uint32           ul_ret;
    oal_uint16           us_stat_info_len;

    if (OAL_UNLIKELY(g_st_oam_sdt_func_hook.p_sdt_report_data_func == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    us_stat_info_len = OAL_SIZEOF(oam_vap_stat_info_stru);

    /* 为上报统计信息申请空间,头部预留8字节，尾部预留1字节，给sdt_drv用 */
    us_skb_len = us_stat_info_len + OAL_SIZEOF(oam_ota_hdr_stru);

    pst_netbuf = oam_alloc_data2sdt(us_skb_len);
    if (pst_netbuf == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_ota_data = (oam_ota_stru *)oal_netbuf_data(pst_netbuf);

    /* 获取系统TICK值 */
    ul_tick = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    /* 填写ota消息头结构体 */
    pst_ota_data->st_ota_hdr.ul_tick     = ul_tick;
    pst_ota_data->st_ota_hdr.en_ota_type = OAM_OTA_TYPE_VAP_STAT_INFO;
    pst_ota_data->st_ota_hdr.us_ota_data_len = us_stat_info_len;
    pst_ota_data->st_ota_hdr.uc_frame_hdr_len = 0;
    oal_set_mac_addr(pst_ota_data->st_ota_hdr.auc_user_macaddr, BROADCAST_MACADDR);

    /* 复制数据,填写ota数据 */
    if (memcpy_s((oal_void *)pst_ota_data->auc_ota_data,
                 pst_ota_data->st_ota_hdr.us_ota_data_len,
                 (const oal_void *)&g_st_stat_info.ast_vap_stat_info[uc_vap_id],
                 us_stat_info_len) != EOK) {
        OAL_IO_PRINT("oam_report_vap_pkt_stat_to_sdt: memcpy_s failed! \n");
        oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
        return OAL_FAIL;
    }
    /* 下发至sdt接收队列，若队列满则串口输出 */
    ul_ret = oam_report_data2sdt(pst_netbuf, OAM_DATA_TYPE_OTA, OAM_PRIMID_TYPE_OUTPUT_CONTENT);

    return ul_ret;
}



oal_uint32  oam_stats_report_info_to_sdt(oam_ota_type_enum_uint8 en_ota_type)
{
    oal_uint32           ul_tick;
    oal_uint16           us_skb_len;        /* skb总长度 */
    oal_netbuf_stru     *pst_netbuf = OAL_PTR_NULL;
    oam_ota_stru        *pst_ota_data = OAL_PTR_NULL;
    oal_uint32           ul_ret;
    oal_uint16           us_stat_info_len;

    if (OAL_UNLIKELY(g_st_oam_sdt_func_hook.p_sdt_report_data_func == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    switch (en_ota_type) {
        case OAM_OTA_TYPE_DEV_STAT_INFO:
            us_stat_info_len = OAL_SIZEOF(oam_device_stat_info_stru) * WLAN_DEVICE_MAX_NUM_PER_CHIP;
            break;

        case OAM_OTA_TYPE_VAP_STAT_INFO:
            us_stat_info_len = (oal_uint16)(OAL_SIZEOF(oam_vap_stat_info_stru) * WLAN_VAP_SUPPORT_MAX_NUM_LIMIT);
            break;

        default:
            us_stat_info_len = 0;
            break;
    }
    if (us_stat_info_len == 0) {
        OAL_IO_PRINT("oam_stats_report_info_to_sdt::ota_type invalid-->%d!\n", en_ota_type);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }
    /* 为上报统计信息申请空间,头部预留8字节，尾部预留1字节，给sdt_drv用 */
    us_skb_len = us_stat_info_len + OAL_SIZEOF(oam_ota_hdr_stru);
    if (us_skb_len > WLAN_SDT_NETBUF_MAX_PAYLOAD) {
        us_skb_len = WLAN_SDT_NETBUF_MAX_PAYLOAD;
        us_stat_info_len = WLAN_SDT_NETBUF_MAX_PAYLOAD - OAL_SIZEOF(oam_ota_hdr_stru);
    }
    pst_netbuf = oam_alloc_data2sdt(us_skb_len);
    if (pst_netbuf == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_ota_data = (oam_ota_stru *)oal_netbuf_data(pst_netbuf);

    /* 获取系统TICK值 */
    ul_tick = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    /* 填写ota消息头结构体 */
    pst_ota_data->st_ota_hdr.ul_tick     = ul_tick;
    pst_ota_data->st_ota_hdr.en_ota_type = en_ota_type;
    pst_ota_data->st_ota_hdr.uc_frame_hdr_len = 0;
    pst_ota_data->st_ota_hdr.us_ota_data_len = us_stat_info_len;

    /* 复制数据,填写ota数据 */
    switch (en_ota_type) {
        case OAM_OTA_TYPE_DEV_STAT_INFO:
            if (memcpy_s((oal_void *)pst_ota_data->auc_ota_data, pst_ota_data->st_ota_hdr.us_ota_data_len,
                         (const oal_void *)g_st_stat_info.ast_dev_stat_info, us_stat_info_len) != EOK) {
                OAL_IO_PRINT("oam_stats_report_info_to_sdt: memcpy_s failed! \n");
                oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
                return OAL_FAIL;
            }
            break;

        case OAM_OTA_TYPE_VAP_STAT_INFO:
            if (memcpy_s((oal_void *)pst_ota_data->auc_ota_data, pst_ota_data->st_ota_hdr.us_ota_data_len,
                         (const oal_void *)g_st_stat_info.ast_vap_stat_info, us_stat_info_len) != EOK) {
                OAL_IO_PRINT("oam_stats_report_info_to_sdt: memcpy_s failed! \n");
                oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
                return OAL_FAIL;
            }
            break;
        default:
            break;
    }

    /* 下发至sdt接收队列，若队列满则串口输出 */
    ul_ret = oam_report_data2sdt(pst_netbuf, OAM_DATA_TYPE_OTA, OAM_PRIMID_TYPE_OUTPUT_CONTENT);
    return ul_ret;
}


oal_uint32  oam_stats_report_usr_info(oal_uint16  us_usr_id)
{
    oal_uint32           ul_tick;
    oal_uint16           us_skb_len;        /* skb总长度 */
    oal_netbuf_stru     *pst_netbuf = OAL_PTR_NULL;
    oam_ota_stru        *pst_ota_data = OAL_PTR_NULL;
    oal_uint32           ul_ret;
    oal_uint16           us_stat_info_len;

    if (OAL_UNLIKELY(g_st_oam_sdt_func_hook.p_sdt_report_data_func == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (us_usr_id >= WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC * WLAN_ACTIVE_USER_MAX_NUM) {
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    us_stat_info_len = OAL_SIZEOF(oam_user_stat_info_stru);

    /* 为上报统计信息申请空间,头部预留8字节，尾部预留1字节，给sdt_drv用 */
    us_skb_len = us_stat_info_len + OAL_SIZEOF(oam_ota_hdr_stru);
    if (us_skb_len > WLAN_SDT_NETBUF_MAX_PAYLOAD) {
        us_skb_len = WLAN_SDT_NETBUF_MAX_PAYLOAD;
        us_stat_info_len = WLAN_SDT_NETBUF_MAX_PAYLOAD - OAL_SIZEOF(oam_ota_hdr_stru);
    }

    pst_netbuf = oam_alloc_data2sdt(us_skb_len);
    if (pst_netbuf == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_ota_data = (oam_ota_stru *)oal_netbuf_data(pst_netbuf);

    /* 获取系统TICK值 */
    ul_tick = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    /* 填写ota消息头结构体 */
    pst_ota_data->st_ota_hdr.ul_tick     = ul_tick;
    pst_ota_data->st_ota_hdr.en_ota_type = OAM_OTA_TYPE_USER_STAT_INFO;
    pst_ota_data->st_ota_hdr.uc_frame_hdr_len = 0;
    pst_ota_data->st_ota_hdr.us_ota_data_len = us_stat_info_len;

    if (memcpy_s((oal_void *)pst_ota_data->auc_ota_data,
                 pst_ota_data->st_ota_hdr.us_ota_data_len,
                 (const oal_void *)&g_st_stat_info.ast_user_stat_info[us_usr_id],
                 us_stat_info_len) != EOK) {
        OAL_IO_PRINT("oam_stats_report_usr_info: memcpy_s failed! \n");
        oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
        return OAL_FAIL;
    }

    /* 下发至sdt接收队列，若队列满则串口输出 */
    ul_ret = oam_report_data2sdt(pst_netbuf, OAM_DATA_TYPE_OTA, OAM_PRIMID_TYPE_OUTPUT_CONTENT);

    return ul_ret;
}



oal_void  oam_stats_clear_stat_info(oal_void)
{
    memset_s(&g_st_stat_info, OAL_SIZEOF(oam_stat_info_stru), 0, OAL_SIZEOF(oam_stat_info_stru));
}


oal_uint32  oam_stats_clear_vap_stat_info(oal_uint8   uc_vap_id)
{
    if (uc_vap_id >= WLAN_VAP_SUPPORT_MAX_NUM_LIMIT) {
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    memset_s(&g_st_stat_info.ast_vap_stat_info[uc_vap_id], OAL_SIZEOF(oam_vap_stat_info_stru),
             0, OAL_SIZEOF(oam_vap_stat_info_stru));

    return OAL_SUCC;
}



oal_uint32  oam_stats_clear_user_stat_info(oal_uint16   us_usr_id)
{
    if (us_usr_id >= WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC * WLAN_ACTIVE_USER_MAX_NUM) {
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    memset_s(&g_st_stat_info.ast_user_stat_info[us_usr_id], OAL_SIZEOF(oam_user_stat_info_stru),
             0, OAL_SIZEOF(oam_user_stat_info_stru));

    return OAL_SUCC;
}

#endif

oal_uint32  oam_statistics_init(oal_void)
{
#if ((_PRE_OS_VERSION_RAW != _PRE_OS_VERSION) && (_PRE_OS_VERSION_WIN32_RAW != _PRE_OS_VERSION))
    oal_mempool_info_to_sdt_register(oam_stats_report_mempool_info_to_sdt,
                                     oam_stats_report_memblock_info_to_sdt);
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
    memset_s(&g_st_stat_info, OAL_SIZEOF(oam_stat_info_stru), 0, OAL_SIZEOF(oam_stat_info_stru));
#endif
#endif
    return OAL_SUCC;
}

/*lint -e19*/
oal_module_symbol(oam_stats_report_irq_info_to_sdt);
oal_module_symbol(oam_stats_report_timer_info_to_sdt);
oal_module_symbol(oam_stats_report_mempool_info_to_sdt);
oal_module_symbol(oam_statistics_init);
oal_module_symbol(oam_stats_report_memblock_info_to_sdt);
oal_module_symbol(oam_stats_report_event_queue_info_to_sdt);
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
oal_module_symbol(g_st_stat_info);
oal_module_symbol(oam_stats_report_info_to_sdt);
oal_module_symbol(oam_stats_clear_stat_info);
oal_module_symbol(oam_stats_clear_user_stat_info);
oal_module_symbol(oam_stats_clear_vap_stat_info);
oal_module_symbol(oam_stats_report_usr_info);
oal_module_symbol(oam_report_vap_pkt_stat_to_sdt);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

