

#include "wal_main.h"
#include "oal_ext_if.h"
#include "oam_ext_if.h"
#include "hmac_ext_if.h"

#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT
#include "oal_kernel_file.h"
#endif

#ifdef _PRE_WLAN_FEATURE_IP_FILTER
#ifdef CONFIG_DOZE_FILTER
#include <huawei_platform/power/wifi_filter/wifi_filter.h>
#endif
#endif /* _PRE_WLAN_FEATURE_IP_FILTER */
#include "wal_config.h"
#include "wal_linux_ioctl.h"
#include "wal_linux_cfg80211.h"
#include "wal_linux_flowctl.h"
#include "wal_config_acs.h"
#include "securec.h"

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
#include "wal_proxysta.h"
#endif

#ifdef _PRE_WLAN_FEATURE_HILINK
#include "wal_linux_rx_rsp.h"
#endif

#include "wal_dfx.h"

#include "hmac_vap.h"
#include "wal_app.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_MAIN_C

/* HOST CRX子表 */
OAL_STATIC frw_event_sub_table_item_stru g_ast_wal_host_crx_table[WAL_HOST_CRX_SUBTYPE_BUTT];

/* HOST CTX字表 */
OAL_STATIC frw_event_sub_table_item_stru g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_BUTT];

/* HOST DRX子表 */
/* OAL_STATIC frw_event_sub_table_item_stru g_ast_wal_host_drx_table[WAL_HOST_DRX_SUBTYPE_BUTT]; */

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
/* WAL_TX_THREAD  */
wal_txdata_thread_stru g_st_wal_txdata_thread;
#endif

/* wal对外钩子函数 */
oam_wal_func_hook_stru g_st_wal_drv_func_hook;

oal_wakelock_stru g_st_wal_wakelock;

oal_uint32 frw_event_fsm_unregister(oal_void);
oal_void frw_timer_delete_all_timer(oal_void);
oal_void frw_timer_exit(oal_void);

oal_uint32 mac_res_exit(void);
oal_uint32 hmac_res_exit(mac_board_stru *pst_hmac_board);

#ifdef _PRE_WLAN_FEATURE_IP_FILTER
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
wal_hw_wlan_filter_ops g_ip_filter_ops = {
    .set_filter_enable   = wal_set_ip_filter_enable,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
    .set_filter_enable_ex = wal_set_assigned_filter_enable,
#endif
    .add_filter_items    = wal_add_ip_filter_items,
    .clear_filters       = wal_clear_ip_filter,
    .get_filter_pkg_stat = NULL,
};
#else
wal_hw_wlan_filter_ops g_ip_filter_ops;
#endif
#endif /* _PRE_WLAN_FEATURE_IP_FILTER */
#ifdef _PRE_WLAN_FEATURE_SAE
oal_workqueue_stru *g_sae_ext_auth_workqueue = OAL_PTR_NULL;
#endif


oal_uint32 wal_event_fsm_init(oal_void)
{
    g_ast_wal_host_crx_table[WAL_HOST_CRX_SUBTYPE_CFG].p_func = wal_config_process_pkt;
    frw_event_table_register(FRW_EVENT_TYPE_HOST_CRX, FRW_EVENT_PIPELINE_STAGE_0, g_ast_wal_host_crx_table);

    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_SCAN_COMP_STA].p_func = wal_scan_comp_proc_sta;
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_ASOC_COMP_STA].p_func = wal_asoc_comp_proc_sta;
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_DISASOC_COMP_STA].p_func = wal_disasoc_comp_proc_sta;
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_STA_CONNECT_AP].p_func = wal_connect_new_sta_proc_ap;
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_STA_DISCONNECT_AP].p_func = wal_disconnect_sta_proc_ap;
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_MIC_FAILURE].p_func = wal_mic_failure_proc;
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_RX_MGMT].p_func = wal_send_mgmt_to_host;
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_LISTEN_EXPIRED].p_func = wal_p2p_listen_timeout;

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_ADD_PROXY_STA_VAP].p_func = wal_proxysta_add_vap;
#endif

#ifdef _PRE_SUPPORT_ACS
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_ACS_RESPONSE].p_func = wal_acs_response_event_handler;
#endif

#ifdef _PRE_WLAN_FEATURE_FLOWCTL
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_FLOWCTL_BACKP].p_func = wal_flowctl_backp_event_handler;
#endif

    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_INIT].p_func = wal_cfg80211_init_evt_handle;
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_MGMT_TX_STATUS].p_func = wal_cfg80211_mgmt_tx_status;
#ifdef _PRE_WLAN_FEATURE_ROAM
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_ROAM_COMP_STA].p_func = wal_roam_comp_proc_sta;
#endif // _PRE_WLAN_FEATURE_ROAM
#ifdef _PRE_WLAN_FEATURE_11R
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_FT_EVENT_STA].p_func = wal_ft_event_proc_sta;
#endif // _PRE_WLAN_FEATURE_11R
#ifdef _PRE_WLAN_FEATURE_HILINK
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_OTHER_BSS_DATA].p_func = wal_send_other_bss_data_to_host;
#endif
#ifdef _PRE_WLAN_FEATURE_SAE
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_EXT_AUTH_REQ].p_func = wal_report_external_auth_req;
#endif

    frw_event_table_register(FRW_EVENT_TYPE_HOST_CTX, FRW_EVENT_PIPELINE_STAGE_0, g_ast_wal_host_ctx_table);

    return OAL_SUCC;
}


oal_uint32 wal_event_fsm_exit(oal_void)
{
    OAL_IO_PRINT("debug2");

    memset_s(g_ast_wal_host_crx_table, OAL_SIZEOF(g_ast_wal_host_crx_table), 0, OAL_SIZEOF(g_ast_wal_host_crx_table));

    memset_s(g_ast_wal_host_ctx_table, OAL_SIZEOF(g_ast_wal_host_ctx_table), 0, OAL_SIZEOF(g_ast_wal_host_ctx_table));

    return OAL_SUCC;
}

#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT
/* debug sysfs */
OAL_STATIC oal_kobject *g_conn_syfs_wal_object = NULL;

oal_int32 wal_wakelock_info_print(char *buf, oal_int32 buf_len)
{
    oal_int32 ret = 0;

#ifdef CONFIG_PRINTK
    if (g_st_wal_wakelock.locked_addr) {
        ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "wakelocked by:%pf\n",
            (oal_void *)(uintptr_t)g_st_wal_wakelock.locked_addr);
    }
#endif

    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "hold %lu locks\n", g_st_wal_wakelock.lock_count);

    return ret;
}

OAL_STATIC ssize_t wal_get_wakelock_info(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int ret = 0;
    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    ret += wal_wakelock_info_print(buf, PAGE_SIZE - ret);

    return ret;
}

extern oal_int32 wal_atcmsrv_ioctl_get_rx_pckg(oal_net_device_stru *pst_net_dev, oal_int32 *pl_rx_pckg_succ_num);
OAL_STATIC ssize_t wal_get_packet_statistics_wlan0_info(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    ssize_t              ret = 0;
    oal_net_device_stru* pst_net_dev = OAL_PTR_NULL;
    mac_vap_stru*        pst_vap = OAL_PTR_NULL;
    hmac_vap_stru*       pst_hmac_vap = OAL_PTR_NULL;
    oal_int32            l_rx_pckg_succ_num;
    oal_int32            l_ret;

    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    pst_net_dev = oal_dev_get_by_name("wlan0");
    if (pst_net_dev == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY,
            "{wal_packet_statistics_wlan0_info_print::oal_dev_get_by_name return null ptr!}\r\n");
        return ret;
    }
    oal_dev_put(pst_net_dev); /* 调用oal_dev_get_by_name后，必须调用oal_dev_put使net_dev的引用计数减一 */
    /* 获取VAP结构体 */
    pst_vap = (mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev);
    /* 如果VAP结构体不存在，返回0 */
    if (OAL_UNLIKELY(pst_vap == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_packet_statistics_wlan0_info_print::pst_vap = NULL!}\r\n");
        return ret;
    }
    /* 非STA直接返回 */
    if (pst_vap->en_vap_mode != WLAN_VAP_MODE_BSS_STA) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_packet_statistics_wlan0_info_print::vap_mode:%d.}\r\n",
            pst_vap->en_vap_mode);
        return ret;
    }
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_CFG, "{wal_packet_statistics_wlan0_info_print::pst_hmac_vap null.}");
        return ret;
    }

    l_ret = wal_atcmsrv_ioctl_get_rx_pckg(pst_net_dev, &l_rx_pckg_succ_num);
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,
            "{wal_packet_statistics_wlan0_info_print::wal_atcmsrv_ioctl_get_rx_pckg failed, failed err:%d!}\r\n",
            l_ret);
        return ret;
    }

    ret += snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "rx_packet=%d\r\n", l_rx_pckg_succ_num);

    return ret;
}

OAL_STATIC struct kobj_attribute dev_attr_wakelock = __ATTR(wakelock, S_IRUGO, wal_get_wakelock_info, NULL);
OAL_STATIC struct kobj_attribute dev_attr_packet_statistics_wlan0 =
    __ATTR(packet_statistics_wlan0, S_IRUGO, wal_get_packet_statistics_wlan0_info, NULL);

oal_int32 wal_msg_queue_info_print(char *buf, oal_int32 buf_len)
{
    oal_int32 ret = 0;

    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "message count:%u\n", wal_get_request_msg_count());

    return ret;
}

OAL_STATIC ssize_t wal_get_msg_queue_info(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int ret = 0;
    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    ret += wal_msg_queue_info_print(buf, PAGE_SIZE - ret);

    return ret;
}

OAL_STATIC struct kobj_attribute dev_attr_msg_queue = __ATTR(msg_queue, S_IRUGO, wal_get_msg_queue_info, NULL);

OAL_STATIC struct attribute *wal_sysfs_entries[] = {
        &dev_attr_wakelock.attr,
        &dev_attr_msg_queue.attr,
        &dev_attr_packet_statistics_wlan0.attr,
        NULL
};

OAL_STATIC struct attribute_group wal_attribute_group = {
    // .name = "vap",
    .attrs = wal_sysfs_entries,
};

OAL_STATIC oal_int32 wal_sysfs_entry_init(oal_void)
{
    oal_int32 ret;
    oal_kobject *pst_root_object = NULL;
    pst_root_object = oal_get_sysfs_root_object();
    if (pst_root_object == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_sysfs_entry_init::get sysfs root object failed!}");
        return -OAL_EFAIL;
    }

    g_conn_syfs_wal_object = kobject_create_and_add("wal", pst_root_object);
    if (g_conn_syfs_wal_object == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_sysfs_entry_init::create wal object failed!}");
        return -OAL_EFAIL;
    }

    ret = sysfs_create_group(g_conn_syfs_wal_object, &wal_attribute_group);
    if (ret) {
        kobject_put(g_conn_syfs_wal_object);
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_sysfs_entry_init::sysfs create group failed!}");
        return ret;
    }
    return OAL_SUCC;
}

OAL_STATIC oal_int32 wal_sysfs_entry_exit(oal_void)
{
    if (g_conn_syfs_wal_object) {
        sysfs_remove_group(g_conn_syfs_wal_object, &wal_attribute_group);
        kobject_put(g_conn_syfs_wal_object);
    }
    return OAL_SUCC;
}
#endif

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)

oal_bool_enum_uint8 wal_get_txdata_thread_enable(oal_void)
{
    return g_st_wal_txdata_thread.en_txthread_enable;
}


OAL_STATIC oal_int32 wal_txdata(oal_void *p_data)
{
    oal_netbuf_stru *pst_netbuf = OAL_PTR_NULL;
    oal_net_device_stru *pst_net_dev = OAL_PTR_NULL;

    while (OAL_LIKELY(!oal_down_interruptible(&g_st_wal_txdata_thread.st_txdata_sema))) {
        if (oal_kthread_should_stop()) {
            break;
        }

        pst_netbuf = oal_netbuf_delist(&g_st_wal_txdata_thread.st_txdata_netbuf_head);
        if (pst_netbuf != NULL) {
            pst_net_dev = (oal_net_device_stru *)pst_netbuf->dev;
            pst_net_dev->netdev_ops->ndo_start_xmit(pst_netbuf, pst_net_dev);
        }
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_txdata_thread_init(oal_void)
{
    oal_kthread_param_stru st_thread_param = { 0 };

    OAL_WAIT_QUEUE_INIT_HEAD(&g_st_wal_txdata_thread.st_txdata_wq);

    oal_netbuf_list_head_init(&g_st_wal_txdata_thread.st_txdata_netbuf_head);

    g_st_wal_txdata_thread.en_txthread_enable = OAL_TRUE;
    g_st_wal_txdata_thread.ul_pkt_loss_cnt = 0;

    oal_sema_init(&g_st_wal_txdata_thread.st_txdata_sema, 0);
    memset_s(&st_thread_param, OAL_SIZEOF(oal_kthread_param_stru), 0, OAL_SIZEOF(oal_kthread_param_stru));
    st_thread_param.l_cpuid = NOT_BIND_CPU;
    st_thread_param.l_policy = OAL_SCHED_FIFO;

    st_thread_param.l_prio = WAL_TXDATA_THERAD_PRIORITY;
    st_thread_param.ul_stacksize = 0x2000;
    g_st_wal_txdata_thread.pst_txdata_thread = oal_kthread_create("wal_txdata", wal_txdata, NULL, &st_thread_param);
    if (g_st_wal_txdata_thread.pst_txdata_thread == OAL_PTR_NULL) {
        dprintf("wal_tx thread create failed!\n");
    }

    return OAL_SUCC;
}
#endif

/* 特性相关初始化，减少代码行数 */
OAL_STATIC int32_t wal_feature_init(void)
{
#ifdef _PRE_SUPPORT_ACS
    wal_acs_init();
#endif
#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT
    (void)wal_sysfs_entry_init();
#endif
#ifdef _PRE_WLAN_FEATURE_DFR
    (void)wal_dfx_init();
#endif
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    (void)wal_set_custom_process_func(wal_custom_cali);
#endif
#ifdef _PRE_WLAN_FEATURE_IP_FILTER
    (void)wal_register_ip_filter(&g_ip_filter_ops);
#endif /* _PRE_WLAN_FEATURE_IP_FILTER */
#if defined(_PRE_WLAN_FEATURE_SAE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    g_sae_ext_auth_workqueue = OAL_CREATE_SINGLETHREAD_WORKQUEUE("sae_ext_auth_workqueue");
    if (!g_sae_ext_auth_workqueue) {
        wal_cfg80211_exit();
        frw_timer_delete_all_timer();
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_feature_init::Failed to create sae_ext_auth_workqueue!}");
        return -OAL_EFAIL;
    }
#endif
    return OAL_SUCC;
}


oal_int32 wal_main_init(oal_void)
{
    oal_uint32 ul_ret;
    frw_init_enum_uint16 en_init_state;

    oal_wake_lock_init(&g_st_wal_wakelock, "wlan_wal_lock");

    wal_msg_queue_init();

    en_init_state = frw_get_init_state();
    /* WAL模块初始化开始时，说明HMAC肯定已经初始化成功 */
    if ((en_init_state == FRW_INIT_STATE_BUTT) || (en_init_state < FRW_INIT_STATE_HMAC_CONFIG_VAP_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_main_init::en_init_state has a invalid value [%d]!}\r\n", en_init_state);

        frw_timer_delete_all_timer();
        return -OAL_EFAIL;
    }

    wal_event_fsm_init();

    /* 创建proc */
    ul_ret = wal_hipriv_create_proc(OAL_PTR_NULL);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_main_init::wal_hipriv_create_proc has a wrong return value [%d]!}\r\n",
            ul_ret);

        frw_timer_delete_all_timer();
        return -OAL_EFAIL;
    }

    /* 初始化每个device硬件设备对应的wiphy */
    if (wal_cfg80211_init() != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_main_init::wal_cfg80211_init has wrong return value [%d]!}\r\n", ul_ret);

        frw_timer_delete_all_timer();
        return -OAL_EFAIL;
    }

    /* 在host侧如果WAL初始化成功，即为全部初始化成功 */
    frw_set_init_state(FRW_INIT_STATE_ALL_SUCC);

    /* wal钩子函数初始化 */
    wal_drv_cfg_func_hook_init();

    /* wal层对外钩子函数注册至oam模块 */
    oam_wal_func_fook_register(&g_st_wal_drv_func_hook);

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    wal_txdata_thread_init();
#endif
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#if defined(_PRE_WLAN_FEATURE_HILINK)
    wal_genetlink_init();
#endif
#endif
#if defined(_PRE_WLAN_FEATURE_P2P) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    /* DTSxxxxxx 初始化cfg80211 删除网络设备工作队列 */
    g_pst_del_virtual_inf_workqueue = OAL_CREATE_SINGLETHREAD_WORKQUEUE("cfg80211_del_virtual_inf");
    if (!g_pst_del_virtual_inf_workqueue) {
        wal_cfg80211_exit();
        frw_timer_delete_all_timer();
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_main_init::Failed to create cfg80211 del virtual infterface workqueue!}");

        return -OAL_EFAIL;
    }
#endif /* _PRE_WLAN_FEATURE_P2P  _PRE_OS_VERSION_LINUX */

    return wal_feature_init();
}


oal_void wal_destroy_all_vap(oal_void)
{
#if (_PRE_TEST_MODE_UT != _PRE_TEST_MODE)

    oal_uint8            uc_vap_id;
    oal_net_device_stru *pst_net_dev;
    oal_int8             ac_param[10] = {0};
    OAL_IO_PRINT("wal_destroy_all_vap start");

    for (uc_vap_id = WLAN_SERVICE_VAP_START_ID_PER_BOARD; uc_vap_id < WLAN_VAP_SUPPOTR_MAX_NUM_SPEC; uc_vap_id++) {
        pst_net_dev = hmac_vap_get_net_device(uc_vap_id);
        if (pst_net_dev != OAL_PTR_NULL) {
            oal_net_close_dev(pst_net_dev);

            wal_hipriv_del_vap(pst_net_dev, ac_param);
            frw_event_process_all_event(0);
        }
    }

    frw_timer_exit();
    frw_timer_delete_all_timer();
    pst_net_dev = oal_dev_get_by_name("wlan0");
    if (pst_net_dev != OAL_PTR_NULL) {
        OAL_IO_PRINT("wal_destroy_all_vap:remove wlan0\n");
        OAL_MEM_FREE(OAL_NETDEVICE_WDEV(pst_net_dev), OAL_TRUE);
        oal_dev_put(pst_net_dev);
        oal_net_unregister_netdev(pst_net_dev);
    }
    pst_net_dev = oal_dev_get_by_name("p2p0");
    if (pst_net_dev != OAL_PTR_NULL) {
        OAL_IO_PRINT("wal_destroy_all_vap:remove p2p0\n");
        OAL_MEM_FREE(OAL_NETDEVICE_WDEV(pst_net_dev), OAL_TRUE);
        oal_dev_put(pst_net_dev);
        oal_net_unregister_netdev(pst_net_dev);
    }
    frw_event_process_all_event(0);
    mac_res_exit();
    hmac_res_exit(&g_st_hmac_board); /* 释放hmac res资源 */
#endif

    return;
}


oal_void wal_main_exit(oal_void)
{
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    wal_set_custom_process_func(OAL_PTR_NULL);
#endif

#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT
    /* debug sysfs */
    wal_sysfs_entry_exit();
#endif
    /* down掉所有的vap */
    wal_destroy_all_vap();

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
    wal_cfg80211_exit();
#endif
    wal_event_fsm_exit();

    /* 删除proc */
    wal_hipriv_remove_proc();

    /* 卸载成功时，将初始化状态置为HMAC初始化成功 */
    frw_set_init_state(FRW_INIT_STATE_HMAC_CONFIG_VAP_SUCC);

    /* 去注册钩子函数 */
    oam_wal_func_fook_unregister();
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#if (_PRE_WLAN_FEATURE_HILINK)
    wal_genetlink_exit();
#endif
#endif

#if defined(_PRE_WLAN_FEATURE_P2P) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    /* DTSxxxxxx 删除cfg80211 删除网络设备工作队列 */
    if (g_pst_del_virtual_inf_workqueue) {
        oal_destroy_workqueue(g_pst_del_virtual_inf_workqueue);
        g_pst_del_virtual_inf_workqueue = OAL_PTR_NULL;
    }
#endif /* _PRE_WLAN_FEATURE_P2P  _PRE_OS_VERSION_LINUX */

#ifdef _PRE_SUPPORT_ACS
    wal_acs_exit();
#endif
#ifdef _PRE_WLAN_FEATURE_DFR
    wal_dfx_exit();
#endif // #ifdef _PRE_WLAN_FEATURE_DFR

    oal_wake_lock_exit(&g_st_wal_wakelock);
    frw_timer_clean_timer(OAM_MODULE_ID_WAL);
#ifdef _PRE_WLAN_FEATURE_IP_FILTER
    wal_unregister_ip_filter();
#endif /* _PRE_WLAN_FEATURE_IP_FILTER */
#if defined(_PRE_WLAN_FEATURE_SAE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if (g_sae_ext_auth_workqueue) {
        oal_destroy_workqueue(g_sae_ext_auth_workqueue);
        g_sae_ext_auth_workqueue = OAL_PTR_NULL;
    }
#endif /* _PRE_WLAN_FEATURE_SAE */
}

/*lint -e578*/ /*lint -e19*/
oal_module_symbol(wal_main_init);
oal_module_symbol(wal_main_exit);

oal_module_license("GPL");
/*lint +e578*/ /*lint +e19*/

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
