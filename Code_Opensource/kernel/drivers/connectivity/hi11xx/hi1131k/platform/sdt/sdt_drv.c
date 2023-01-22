
/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_mem.h"
#include "sdt_drv.h"
#include "wal_ext_if.h"
#include "oam_ext_if.h"
#include "securec.h"
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#include "los_event.h"
#include "los_task.h"
#include "oam_misc.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_SDT_DRV_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
/* OAM日志送sdt侧处理全局变量 */
sdt_drv_mng_stru           g_st_sdt_drv_mng_entry;
oam_sdt_func_hook_stru     g_st_sdt_drv_func_hook;

/*****************************************************************************
  3 函数实现
*****************************************************************************/
OAL_STATIC oal_uint32  sdt_drv_netlink_send(oal_netbuf_stru *pst_netbuf, oal_uint32  ul_len);

sdt_drv_mng_stru *sdt_drv_get_mng_entry(oal_void)
{
    return &g_st_sdt_drv_mng_entry;
}


oal_void sdt_drv_set_mng_entry_usepid(oal_uint32  ulpid)
{
    oal_ulong    ui_irq_save;

    oal_spin_lock_irq_save(&g_st_sdt_drv_mng_entry.st_spin_lock, &ui_irq_save);

    g_st_sdt_drv_mng_entry.ul_usepid = ulpid;

    oal_spin_unlock_irq_restore(&g_st_sdt_drv_mng_entry.st_spin_lock, &ui_irq_save);
}


OAL_STATIC OAL_INLINE oal_void  sdt_drv_netbuf_add_to_list(oal_netbuf_stru *pst_netbuf)
{
    oal_ulong    ui_irq_save;

    oal_spin_lock_irq_save(&g_st_sdt_drv_mng_entry.st_spin_lock, &ui_irq_save);

    oal_netbuf_add_to_list_tail(pst_netbuf, &g_st_sdt_drv_mng_entry.rx_wifi_dbg_seq);

    oal_spin_unlock_irq_restore(&g_st_sdt_drv_mng_entry.st_spin_lock, &ui_irq_save);
}


oal_netbuf_stru* sdt_drv_netbuf_delist(oal_void)
{
    oal_ulong                ui_irq_save;
    oal_netbuf_stru        *pst_netbuf = OAL_PTR_NULL;

    oal_spin_lock_irq_save(&g_st_sdt_drv_mng_entry.st_spin_lock, &ui_irq_save);

    pst_netbuf = oal_netbuf_delist(&g_st_sdt_drv_mng_entry.rx_wifi_dbg_seq);

    oal_spin_unlock_irq_restore(&g_st_sdt_drv_mng_entry.st_spin_lock, &ui_irq_save);

    return pst_netbuf;
}


OAL_STATIC OAL_INLINE oal_int32 sdt_drv_check_isdevlog(oal_netbuf_stru *pst_netbuf)
{
    oal_uint8               *puc_pkt_tail;
    sdt_drv_pkt_hdr_stru    *pst_pkt_hdr;
    pst_pkt_hdr = (sdt_drv_pkt_hdr_stru *)oal_netbuf_data(pst_netbuf);
    puc_pkt_tail = (oal_uint8 *)pst_pkt_hdr + OAL_NETBUF_LEN(pst_netbuf);
    OAL_IO_PRINT("devlog {%s}\n", oal_netbuf_data(pst_netbuf));
    if (*puc_pkt_tail == SDT_DRV_PKT_END_FLG ||
        pst_pkt_hdr->uc_data_start_flg == SDT_DRV_PKT_START_FLG) {
        OAL_IO_PRINT("check out is device log\n");
        return OAL_SUCC;
    }

    return -OAL_EFAIL;
}


OAL_STATIC OAL_INLINE oal_void  sdt_drv_add_pkt_head(oal_netbuf_stru *pst_netbuf, oam_data_type_enum_uint8 en_type,
    oam_primid_type_enum_uint8 en_prim_id)
{
    /*************************** buffer structure ****************************/
    /**************************************/
    /*   |data_hdr | data | data_tail |   */
    /*------------------------------------*/
    /*   |  8Byte  |      |    1Byte  |   */
    /**************************************/
    /************************ data header structure **************************/
    /* ucFrameStart | ucFuncType | ucPrimId | ucReserver | usFrameLen | usSN */
    /* --------------------------------------------------------------------- */
    /*   1Byte     |    1Byte   |  1Byte   |   1Byte    |  2Bytes    |2Bytes */
    /*************************************************************************/
    oal_uint8               *puc_pkt_tail = OAL_PTR_NULL;
    sdt_drv_pkt_hdr_stru    *pst_pkt_hdr = OAL_PTR_NULL;
    oal_uint16               us_tmp_data;

    oal_netbuf_push(pst_netbuf, WLAN_SDT_SKB_HEADROOM_LEN);
    oal_netbuf_put(pst_netbuf, WLAN_SDT_SKB_TAILROOM_LEN);

    /* SDT收到的消息数目加1 */
    g_st_sdt_drv_mng_entry.us_sn_num++;

    /* 为数据头的每一个成员赋值 */
    pst_pkt_hdr = (sdt_drv_pkt_hdr_stru *)oal_netbuf_data(pst_netbuf);

    pst_pkt_hdr->uc_data_start_flg = SDT_DRV_PKT_START_FLG;
    pst_pkt_hdr->en_msg_type       = en_type;
    pst_pkt_hdr->uc_prim_id        = en_prim_id;
    pst_pkt_hdr->uc_resv[0]        = 0;

    us_tmp_data = (oal_uint16)OAL_NETBUF_LEN(pst_netbuf);
    pst_pkt_hdr->uc_data_len_low_byte  = SDT_DRV_GET_LOW_BYTE(us_tmp_data);
    pst_pkt_hdr->uc_data_len_high_byte = SDT_DRV_GET_HIGH_BYTE(us_tmp_data);

    us_tmp_data = g_st_sdt_drv_mng_entry.us_sn_num;
    pst_pkt_hdr->uc_sequence_num_low_byte   = SDT_DRV_GET_LOW_BYTE(us_tmp_data);
    pst_pkt_hdr->uc_sequence_num_high_byte  = SDT_DRV_GET_HIGH_BYTE(us_tmp_data);

    /* 为数据尾赋值0x7e */
    puc_pkt_tail = (oal_uint8 *)pst_pkt_hdr + OAL_NETBUF_LEN(pst_netbuf);
    puc_pkt_tail--;
    *puc_pkt_tail = SDT_DRV_PKT_END_FLG;
}

oal_void sdt_pkt_head_tail_add(oal_netbuf_stru *pst_netbuf, oam_data_type_enum_uint8 en_type,
    oam_primid_type_enum_uint8 en_prim_id)
{
    sdt_drv_add_pkt_head(pst_netbuf, en_type, en_prim_id);
}


OAL_STATIC OAL_INLINE oal_int32  sdt_drv_report_data2app(oal_netbuf_stru *pst_netbuf, oam_data_type_enum_uint8 en_type,
                                                         oam_primid_type_enum_uint8 en_prim)
{
    /* 由上层调用接口判断指针非空 */
    oal_int32       l_ret;

#if (_HI113X_SW_VERSION == _HI113X_SW_DEBUG)

    /* 如果是device log 则不需要加pkt 包头 */
    if (en_type != OAM_DATA_TYPE_DEVICE_LOG) {
        sdt_drv_add_pkt_head(pst_netbuf, en_type, en_prim);
    }
    sdt_drv_netbuf_add_to_list(pst_netbuf);

    /* linux版本使用workqueue， liteos版本因为liteos workqueue局限性，转而使用事件方式 */
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    l_ret = oal_queue_work(g_st_sdt_drv_mng_entry.oam_rx_workqueue, &g_st_sdt_drv_mng_entry.rx_wifi_work);
#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    l_ret = LOS_EventWrite(&oam_event, BIT0);
#endif

#elif (_HI113X_SW_VERSION == _HI113X_SW_RELEASE)
    /* RELEASE版本日志不加入日志处理链表，不上报sdt，直接free */
    oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);

#endif

    return l_ret;
}


OAL_STATIC OAL_INLINE oal_int32 sdt_drv_get_wq_len(oal_void)
{
    return (oal_int32)oal_netbuf_list_len(&g_st_sdt_drv_mng_entry.rx_wifi_dbg_seq);
}


OAL_STATIC OAL_INLINE oal_int32 sdt_drv_wal_func_hook(oal_uint8 pc_type, oal_uint8 *puc_data, oal_uint16 us_len)
{
    oal_int32 l_ret = OAL_EFAIL;

    if (pc_type >= OAM_DATA_TYPE_BUTT) {
        return OAL_EFAIL;
    }
    if (puc_data == NULL) {
        return OAL_EFAIL;
    }
    switch (pc_type) {
        case OAM_DATA_TYPE_MEM_RW:
            if (g_st_oam_wal_func_hook.p_wal_recv_mem_data_func != OAL_PTR_NULL) {
                l_ret = g_st_oam_wal_func_hook.p_wal_recv_mem_data_func(puc_data, us_len);
            }
            break;

        case OAM_DATA_TYPE_REG_RW:
            if (g_st_oam_wal_func_hook.p_wal_recv_reg_data_func != OAL_PTR_NULL) {
                l_ret = g_st_oam_wal_func_hook.p_wal_recv_reg_data_func(puc_data, us_len);
            }
            break;

        case OAM_DATA_TYPE_CFG:
            if (g_st_oam_wal_func_hook.p_wal_recv_cfg_data_func != OAL_PTR_NULL) {
                l_ret = g_st_oam_wal_func_hook.p_wal_recv_cfg_data_func(puc_data, us_len);
            }
            break;

        case OAM_DATA_TYPE_GVAR_RW:
            if (g_st_oam_wal_func_hook.p_wal_recv_global_var_func != OAL_PTR_NULL) {
                l_ret = g_st_oam_wal_func_hook.p_wal_recv_global_var_func(puc_data, us_len);
            }
            break;

        default:
            OAL_IO_PRINT("sdt_drv_send_data_to_wifi::cmd is invalid!!-->%d\n", pc_type);
            break;
    }

    return l_ret;
}


oal_int32  sdt_drv_send_data_to_wifi(const oal_uint8  *puc_param, oal_int32  l_len)
{
    oal_netbuf_stru         *pst_netbuf = OAL_PTR_NULL;
    oal_int8                *pc_buf = OAL_PTR_NULL;
    oal_long                 i_len;   /* SDIO CRC ERROR */
    oal_int32                l_ret;
    oal_uint8               *puc_data = OAL_PTR_NULL;

    if (puc_param == OAL_PTR_NULL) {
        OAL_IO_PRINT("sdt_drv_send_data_to_wifi::puc_param is null!\n");
        return -OAL_EFAIL;
    }

    if (l_len < 0) {
        OAL_IO_PRINT("sdt_drv_send_data_to_wifi::data len little then zero!\n");
        return -OAL_EFAIL;
    }
    i_len = (oal_long)l_len > 300 ? (oal_long)l_len : 300;

    /* 接收消息不用填充头，直接使用 */
    pst_netbuf = oal_mem_sdt_netbuf_alloc((oal_uint16)i_len, OAL_TRUE);
    if (pst_netbuf == OAL_PTR_NULL) {
        return -OAL_EFAIL;
    }

    pc_buf = (oal_int8 *)oal_netbuf_put(pst_netbuf, (oal_uint32)l_len);
    if (memcpy_s((oal_void *)pc_buf, (oal_uint32)l_len,
                 (const oal_void *)puc_param, (oal_uint32)l_len) != EOK) {
        OAL_IO_PRINT("sdt_drv_send_data_to_wifi: memcpy_s failed! \n");
        oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
        return -OAL_EFAIL;
    }

    i_len = pc_buf[5] * MAX_NUM;
    i_len = pc_buf[4] + i_len;
    i_len = i_len - OAM_RESERVE_SKB_LEN;

    puc_data = oal_netbuf_data(pst_netbuf);

    if (i_len < 0) {
        OAL_IO_PRINT("sdt_drv_send_data_to_wifi::need len large then zero!! \n");
#if (_PRE_OS_VERSION_RAW != _PRE_OS_VERSION)
        oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
#endif
        return -OAL_EFAIL;
    }

    l_ret = sdt_drv_wal_func_hook(pc_buf[1], &puc_data[8], (oal_uint16)i_len);

#if (_PRE_OS_VERSION_RAW != _PRE_OS_VERSION)
    oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
#endif
    return l_ret;
}


oal_uint32  sdt_drv_netlink_send(oal_netbuf_stru *pst_netbuf, oal_uint32  ul_len)
{
#if (_PRE_OS_VERSION_RAW != _PRE_OS_VERSION)
    oal_netbuf_stru            *pst_copy_netbuf = OAL_PTR_NULL;
    oal_nlmsghdr_stru          *pst_nlhdr = OAL_PTR_NULL;

    oal_uint32                  ul_nlmsg_len;
    oal_int32                   l_unicast_bytes;

    /* 由上层保证参数非空 */
    /* 如果没有与app建立连接，则直接返回，每500次打印一次提示信息 */
    if (g_st_sdt_drv_mng_entry.ul_usepid == 0) {
        if ((oal_atomic_read(&g_st_sdt_drv_mng_entry.ul_unconnect_cnt) % SDT_DRV_REPORT_NO_CONNECT_FREQUENCE) == 0) {
            OAL_IO_PRINT("Info:waitting app_sdt start...\r\n");
            oal_atomic_inc(&g_st_sdt_drv_mng_entry.ul_unconnect_cnt);
        }

        oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);

        return OAL_FAIL;
    }

    /* 填写netlink消息头 */
    ul_nlmsg_len = OAL_NLMSG_SPACE(ul_len);
    pst_copy_netbuf = oal_netbuf_alloc(ul_nlmsg_len, 0, WLAN_MEM_NETBUF_ALIGN);
    if (OAL_UNLIKELY(pst_copy_netbuf == OAL_PTR_NULL)) {
        oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);

        OAL_IO_PRINT("oal_netbuf_alloc failed. \r\n");
        return OAL_FAIL;
    }

    pst_nlhdr = oal_nlmsg_put(pst_copy_netbuf, 0, 0, 0, (oal_int32)ul_len, 0);
    if (memcpy_s((oal_void *)OAL_NLMSG_DATA(pst_nlhdr), ul_len,
                 (const oal_void *)oal_netbuf_data(pst_netbuf), ul_len) != EOK) {
        oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
        OAL_IO_PRINT("sdt_drv_netlink_send :memcpy_s failed. \r\n");
        return OAL_FAIL;
    }

    l_unicast_bytes = oal_netlink_unicast(g_st_sdt_drv_mng_entry.pst_nlsk, pst_copy_netbuf,
                                          g_st_sdt_drv_mng_entry.ul_usepid, OAL_MSG_DONTWAIT);

    oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);

    OAM_SDT_STAT_INCR(ul_nlk_sd_cnt);
    if (l_unicast_bytes <= 0) {
        OAM_SDT_STAT_INCR(ul_nlk_sd_fail);
        return OAL_FAIL;
    }
#endif
    return OAL_SUCC;
}


oal_void  sdt_drv_netlink_recv(oal_netbuf_stru  *pst_netbuf)
{
    oal_nlmsghdr_stru              *pst_nlhdr = OAL_PTR_NULL;
    sdt_drv_netlink_msg_hdr_stru    st_msg_hdr = {0};
    oal_uint32                      ul_len;

    if (pst_netbuf == OAL_PTR_NULL) {
        OAL_IO_PRINT("sdt_drv_netlink_recv::pst_netbuf is null!\n");
        return;
    }
    memset_s(g_st_sdt_drv_mng_entry.puc_data, DATA_BUF_LEN, 0, DATA_BUF_LEN);

    if (OAL_NETBUF_LEN(pst_netbuf) >= OAL_NLMSG_SPACE(0)) {
        pst_nlhdr = oal_nlmsg_hdr((OAL_CONST oal_netbuf_stru *)pst_netbuf);
        /* 对报文长度进行检查 */
        if (!OAL_NLMSG_OK(pst_nlhdr, OAL_NETBUF_LEN(pst_netbuf))) {
            OAL_IO_PRINT("[ERROR]invaild netlink buff data packge data len = :%u,skb_buff data len = %u\n",
                         pst_nlhdr->nlmsg_len, OAL_NETBUF_LEN(pst_netbuf));
            return;
        }
        ul_len   = OAL_NLMSG_PAYLOAD(pst_nlhdr, 0);
        /* 后续需要拷贝OAL_SIZEOF(st_msg_hdr)故判断之 */
        if (ul_len <= DATA_BUF_LEN && ul_len >= (oal_uint32)OAL_SIZEOF(st_msg_hdr)) {
            if (memcpy_s((oal_void *)g_st_sdt_drv_mng_entry.puc_data, DATA_BUF_LEN,
                         (const oal_void *)OAL_NLMSG_DATA(pst_nlhdr), ul_len) != EOK) {
                OAL_IO_PRINT("sdt_drv_netlink_send :memcpy_s failed. \r\n");
                return;
            }
        } else {
            /* overflow */
            OAL_IO_PRINT("[ERROR]invaild netlink buff len:%u,max len:%u\n", ul_len, DATA_BUF_LEN);
            return;
        }

        if (memcpy_s((oal_void *)&st_msg_hdr, (oal_uint32)OAL_SIZEOF(st_msg_hdr),
                     (const oal_void *)g_st_sdt_drv_mng_entry.puc_data, (oal_uint32)OAL_SIZEOF(st_msg_hdr)) != EOK) {
            OAL_IO_PRINT("sdt_drv_netlink_recv::memcpy_s failed!\n");
            return;
        }

        if (st_msg_hdr.ul_cmd == NETLINK_MSG_HELLO) {
            g_st_sdt_drv_mng_entry.ul_usepid = pst_nlhdr->nlmsg_pid;   /* pid of sending process */
            OAL_IO_PRINT("%s pid is-->%d \n", OAL_FUNC_NAME, g_st_sdt_drv_mng_entry.ul_usepid);
        } else {
#if defined(PLATFORM_DEBUG_ENABLE)
            sdt_drv_send_data_to_wifi(&g_st_sdt_drv_mng_entry.puc_data[OAL_SIZEOF(st_msg_hdr)],
                                      ul_len - (oal_uint32)OAL_SIZEOF(st_msg_hdr));
#else
            OAL_IO_PRINT("user mode not accept msg except hello from sdt!\n");
#endif
        }
    }
}


oal_int32  sdt_drv_netlink_create(oal_void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    g_st_sdt_drv_mng_entry.pst_nlsk = oal_netlink_kernel_create(&OAL_INIT_NET, NETLINK_TEST,
                                                                0, sdt_drv_netlink_recv,
                                                                OAL_PTR_NULL, OAL_THIS_MODULE);
    if (g_st_sdt_drv_mng_entry.pst_nlsk == OAL_PTR_NULL) {
        OAL_IO_PRINT("sdt_drv_netlink_create return fail!\n");
        return -OAL_EFAIL;
    }

#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)

#endif

    return OAL_SUCC;
}


oal_void  sdt_drv_push_wifi_log_work(oal_work_stru *work)
{
    oal_netbuf_stru  *pst_netbuf;
    int32_t ret;

    pst_netbuf = sdt_drv_netbuf_delist();

    while (pst_netbuf != OAL_PTR_NULL) {
        ret = sdt_drv_netlink_send(pst_netbuf, OAL_NETBUF_LEN(pst_netbuf));
        if (ret != OAL_SUCC) {
            OAL_IO_PRINT("sdt_drv_push_wifi_log_work::sdt_drv_netlink_send return fail!\n");
        }
        pst_netbuf = sdt_drv_netbuf_delist();

#if (!defined(CONFIG_PREEMPT) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION))
        cond_resched();
#endif
    }
    return;
}


oal_void sdt_drv_func_hook_init(oal_void)
{
    g_st_sdt_drv_func_hook.p_sdt_report_data_func = sdt_drv_report_data2app;
    g_st_sdt_drv_func_hook.p_sdt_get_wq_len_func  = sdt_drv_get_wq_len;
}


oal_int32  sdt_drv_main_init(oal_void)
{
    oal_int32   l_nl_return_val;

    memset_s((void *)&g_st_sdt_drv_mng_entry, OAL_SIZEOF(g_st_sdt_drv_mng_entry), 0,
             OAL_SIZEOF(g_st_sdt_drv_mng_entry));

    g_st_sdt_drv_mng_entry.puc_data = oal_memalloc(DATA_BUF_LEN);
    if (g_st_sdt_drv_mng_entry.puc_data == OAL_PTR_NULL) {
        OAL_IO_PRINT("alloc g_st_sdt_drv_mng_entry.puc_data fail!\n");
        return -OAL_EFAIL;
    }

    memset_s(g_st_sdt_drv_mng_entry.puc_data, DATA_BUF_LEN, 0, DATA_BUF_LEN);

    l_nl_return_val = sdt_drv_netlink_create();
    if (l_nl_return_val < 0) {
        OAL_IO_PRINT("sdt_drv_main_init::create netlink returns fail! l_nl_return_val--> \
                      %d\n", l_nl_return_val);
        return -l_nl_return_val;
    }

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    g_st_sdt_drv_mng_entry.oam_rx_workqueue = oal_create_singlethread_workqueue("oam_rx_queue");
    OAL_INIT_WORK(&g_st_sdt_drv_mng_entry.rx_wifi_work, sdt_drv_push_wifi_log_work);
#elif ((_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION) && (_HI113X_SW_VERSION == _HI113X_SW_DEBUG))
    oam_task_init();
#endif

    oal_spin_lock_init(&g_st_sdt_drv_mng_entry.st_spin_lock);
    oal_netbuf_list_head_init(&g_st_sdt_drv_mng_entry.rx_wifi_dbg_seq);

    /* sdt模块钩子函数初始化 */
    sdt_drv_func_hook_init();

    /* 将sdt钩子函数注册至oam模块 */
    oam_sdt_func_fook_register(&g_st_sdt_drv_func_hook);

    /* sdt正常加载之后将输出方式置为OAM_OUTPUT_TYPE_SDT */
    if (oam_set_output_type(OAM_OUTPUT_TYPE_SDT) != OAL_SUCC) {
        OAL_IO_PRINT("oam set output type fail!");
        return -OAL_EFAIL;
    }
    return OAL_SUCC;
}


oal_void  sdt_drv_main_exit(oal_void)
{
    oam_sdt_func_fook_unregister();

    if (g_st_sdt_drv_mng_entry.pst_nlsk != OAL_PTR_NULL) {
        oal_netlink_kernel_release(g_st_sdt_drv_mng_entry.pst_nlsk);
    }

    if (g_st_sdt_drv_mng_entry.puc_data != OAL_PTR_NULL) {
        oal_free(g_st_sdt_drv_mng_entry.puc_data);
    }

    if (g_st_sdt_drv_mng_entry.oam_rx_workqueue != OAL_PTR_NULL) {
        oal_destroy_workqueue(g_st_sdt_drv_mng_entry.oam_rx_workqueue);
    }
    oal_netbuf_queue_purge(&g_st_sdt_drv_mng_entry.rx_wifi_dbg_seq);

    /* 卸载成功后，输出打印 */
    OAL_IO_PRINT("sdt exit ok!\n");

    return;
}

/*lint -e578*//*lint -e19*/
oal_module_symbol(sdt_drv_main_init);
oal_module_symbol(sdt_drv_main_exit);
oal_module_license("GPL");

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

