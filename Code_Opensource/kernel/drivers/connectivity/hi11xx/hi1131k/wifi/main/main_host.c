

// 1 头文件包含
#define HISI_LOG_TAG    "[WIFI_MAIN]"
#include "main_host.h"
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "sdt_drv.h"
#elif ((_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION) || (_PRE_OS_VERSION_WIN32_RAW == _PRE_OS_VERSION))
#include "mac_resource.h"
#endif

#if (_PRE_PRODUCT_ID_HI1131C_HOST ==_PRE_PRODUCT_ID)
#include "hmac_ext_if.h"
#include "wal_ext_if.h"
#include "dmac_ext_if.h"
#include "plat_cali.h"
#include "wal_linux_ioctl.h"
#include "hmac_vap.h"
#include "oal_hcc_host_if.h"
#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT
#include "oal_kernel_file.h"
#endif
#elif (_PRE_PRODUCT_ID_HI1131C_DEV ==_PRE_PRODUCT_ID)
/* 以上头文件待回收 */
#include "oam_log.h"
#include "sdio_slave.h"
#include "channel_slave.h"
#include "oal_main.h"
#include "uart.h"
#include "uart.h"
#include "oal_hcc_slave_if.h"

#include "hal_ext_if.h"
#include "dmac_ext_if.h"
#include "dmac_alg.h"

#include "dmac_pm_sta.h"

#ifdef _PRE_WLAN_FEATURE_BTCOEX
#include "dmac_btcoex.h"
#endif

#ifdef _PRE_WLAN_ALG_ENABLE
#include "alg_ext_if.h"
#endif
#endif

#if (_PRE_PRODUCT_ID_HI1131C_HOST == _PRE_PRODUCT_ID)
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#include "hisi_customize_wifi.h"
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

#ifdef CONFIG_MMC
#include "oal_sdio_host_if.h"
#endif
#endif
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAIN_HOST_C

// 2 全局变量定义
#if (_PRE_OS_VERSION_LITEOS != _PRE_OS_VERSION)
oal_void platform_module_exit(oal_uint16 us_bitmap);
OAL_STATIC oal_void builder_module_exit(oal_uint16 us_bitmap);
#endif

// 3 函数实现

extern oal_void oam_hisi_kill(void);
#if (_PRE_OS_VERSION_LITEOS != _PRE_OS_VERSION)
OAL_STATIC oal_void builder_module_exit(oal_uint16 us_bitmap)
{
#if ((_PRE_PRODUCT_ID_HI1131C_DEV != _PRE_PRODUCT_ID) || (_PRE_OS_VERSION_WIN32_RAW == _PRE_OS_VERSION))
#if (_PRE_PRODUCT_ID_HI1131C_DEV != _PRE_PRODUCT_ID)
    if (BIT8 & us_bitmap) {
        wal_main_exit();
    }
    if (BIT7 & us_bitmap) {
        hmac_main_exit();
    }

    oam_hisi_kill();
#elif (_PRE_PRODUCT_ID_HI1131C_HOST != _PRE_PRODUCT_ID)
#ifdef _PRE_WLAN_ALG_ENABLE
        if (BIT6 & us_bitmap) {
            alg_main_exit();
        }
#endif
    if (BIT5 & us_bitmap) {
        dmac_main_exit();
    }
    if (BIT4 & us_bitmap) {
        hal_main_exit();
    }

    platform_module_exit(us_bitmap);
#endif
#endif
    return;
}
#endif

#if ((_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION) && (_PRE_PRODUCT_ID_HI1131C_HOST ==_PRE_PRODUCT_ID))

OAL_STATIC oal_uint32 host_test_get_chip_msg(oal_void)
{
    oal_uint32             ul_return;
    mac_chip_stru         *pst_chip = OAL_PTR_NULL;
    frw_event_mem_stru    *pst_event_mem;
    frw_event_stru        *pst_event = OAL_PTR_NULL;             /* 事件结构体 */
    oal_uint32             ul_dev_id;
    oal_netbuf_stru       *pst_netbuf = OAL_PTR_NULL;
    dmac_tx_event_stru    *pst_ctx_event = OAL_PTR_NULL;
    oal_uint8             *pst_mac_rates_11g = OAL_PTR_NULL;

    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_tx_event_stru));
    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAL_IO_PRINT("host_test_get_chip_msg: hmac_init_event_process FRW_EVENT_ALLOC result = OAL_PTR_NULL.\n");
        return OAL_FAIL;
    }

    /* 申请netbuf内存 */
    pst_netbuf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if (OAL_UNLIKELY(pst_netbuf == OAL_PTR_NULL)) {
        OAL_IO_PRINT("host_test_get_chip_msg: hmac_init_event_process OAL_MEM_NETBUF_ALLOC result = OAL_PTR_NULL.\n");
        return OAL_FAIL;
    }

    pst_event                 = (frw_event_stru *)pst_event_mem->puc_data;
    pst_ctx_event             = (dmac_tx_event_stru *)pst_event->auc_event_data;
    pst_ctx_event->pst_netbuf = pst_netbuf;
    pst_mac_rates_11g = (oal_uint8*)oal_netbuf_data(pst_ctx_event->pst_netbuf);
    pst_chip = (mac_chip_stru *)(pst_mac_rates_11g + sizeof(mac_data_rate_stru) * MAC_DATARATES_PHY_80211G_NUM);

    ul_dev_id = (oal_uint32)oal_queue_dequeue(&(g_st_mac_res.st_dev_res.st_queue));
    /* 0为无效值 */
    if (ul_dev_id == 0) {
        OAL_IO_PRINT("host_test_get_chip_msg:oal_queue_dequeue return 0!");
        FRW_EVENT_FREE(pst_event_mem);
        return OAL_FAIL;
    }
    pst_chip->auc_device_id[0] = (oal_uint8)(ul_dev_id - 1);

    /* 根据ul_chip_ver，通过hal_chip_init_by_version函数获得 */
    pst_chip->uc_device_nums = 1;
    pst_chip->uc_chip_id = 0;
    pst_chip->en_chip_state = OAL_TRUE;

    /* 由hal_chip_get_version函数得到,1102 02需要SOC提供寄存器后实现 */
    pst_chip->ul_chip_ver = WLAN_CHIP_VERSION_HI1151V100H;

    pst_chip->pst_chip_stru = OAL_PTR_NULL;

    ul_return = hmac_init_event_process(pst_event_mem);
    if (OAL_UNLIKELY(ul_return != OAL_SUCC)) {
        OAL_IO_PRINT("host_test_get_chip_msg: hmac_init_event_process  ul_return != OAL_SUCC\n");
        FRW_EVENT_FREE(pst_event_mem);
        oal_netbuf_free(pst_netbuf);
        return OAL_FAIL;
    }

    return OAL_SUCC;
}
#endif

#if  (defined(HI1102_EDA))

OAL_STATIC oal_uint32 device_test_create_cfg_vap(oal_void)
{
    oal_uint32          ul_return;
    frw_event_mem_stru *pst_event_mem;
    frw_event_stru     *pst_event;

    pst_event_mem = FRW_EVENT_ALLOC(0);
    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAL_IO_PRINT("device_test_create_cfg_vap: hmac_init_event_process FRW_EVENT_ALLOC result = OAL_PTR_NULL.\n");
        return OAL_FAIL;
    }

    ul_return = dmac_init_event_process(pst_event_mem);
    if (ul_return != OAL_SUCC) {
        OAL_IO_PRINT("device_test_create_cfg_vap: dmac_init_event_process result = fale.\n");
        FRW_EVENT_FREE(pst_event_mem);
        return OAL_FAIL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    pst_event->st_event_hdr.uc_device_id = 0;

    ul_return = dmac_cfg_vap_init_event(pst_event_mem);
    if (ul_return != OAL_SUCC) {
        FRW_EVENT_FREE(pst_event_mem);
        return ul_return;
    }

    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}
#endif

#if (_PRE_PRODUCT_ID_HI1131C_DEV ==_PRE_PRODUCT_ID)
#ifdef _PRE_1131C_DEV_FUNCTION_NOT_USED

oal_void platform_module_exit(oal_uint16 us_bitmap)
{
    if (BIT3 & us_bitmap) {
        frw_main_exit();
    }

    if (BIT1 & us_bitmap) {
        oam_main_exit();
    }

    if (BIT0 & us_bitmap) {
        oal_main_exit();
    }

    return;
}
#endif // _PRE_1131C_DEV_FUNCTION_NOT_USED


oal_int32 platform_module_init(oal_void)
{
    oal_int32 l_return;
    oal_uint16 us_bitmap;

    l_return = oal_main_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("platform_module_init: oal_main_init return error code: %d\r\n", l_return);
        return l_return;
    }
#if (!defined(_PRE_EDA))
    l_return = oam_main_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("platform_module_init: oam_main_init return error code: %d\r\n", l_return);
        us_bitmap = BIT0;
        builder_module_exit(us_bitmap);
        return l_return;
    }
#endif
    l_return = frw_main_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("platform_module_init: frw_main_init return error code: %d\r\n", l_return);
        us_bitmap = BIT0 | BIT1 | BIT2;
        builder_module_exit(us_bitmap);
        return l_return;
    }

    /* 启动完成后，输出打印 */
    OAL_IO_PRINT("platform_module_init:: platform_main_init finish!\r\n");

    return OAL_SUCC;
}


OAL_STATIC oal_int32 device_module_init(oal_void)
{
    oal_int32  l_return;
    oal_uint16 us_bitmap;

    l_return = hal_main_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("device_module_init: hal_main_init return error code: %d", l_return);
        return l_return;
    }

    l_return = dmac_main_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("device_module_init: dmac_main_init return error code: %d", l_return);
        us_bitmap = BIT4;
        builder_module_exit(us_bitmap);
        return l_return;
    }

#if (!defined(_PRE_EDA))
#if defined(_PRE_WLAN_ALG_ENABLE) || defined(_PRE_WLAN_CHIP_TEST_ALG)
    l_return = alg_main_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("device_module_init: alg_main_init return error code : %d", l_return);
        us_bitmap = BIT4 | BIT5;
        builder_module_exit(us_bitmap);
        return l_return;
    }
#endif
#endif

    /* 启动完成后，输出打印 */
    OAL_IO_PRINT("device_module_init:: device_module_init finish!\r\n");

    return OAL_SUCC;
}
#endif
#if (_PRE_PRODUCT_ID_HI1131C_HOST ==_PRE_PRODUCT_ID)
#if (_PRE_OS_VERSION_LITEOS != _PRE_OS_VERSION)
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE

extern oal_uint32 band_5g_enabled;
oal_uint32 hwifi_config_host_global_dts_param(oal_void)
{
    /* 获取5g开关 */
    band_5g_enabled = !!hwifi_get_init_value(CUS_TAG_DTS, WLAN_CFG_DTS_BAND_5G_ENABLE);

#ifndef _PRE_WLAN_FEATURE_5G
    band_5g_enabled = OAL_FALSE;
#endif /* _PRE_WLAN_FEATURE_5G */

    return OAL_SUCC;
}
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */
#endif /* #if (_PRE_OS_VERSION_LITEOS != _PRE_OS_VERSION) */

extern      oal_int32 wlan_pm_open(oal_void);
extern oal_uint32 wlan_pm_close(oal_void);
extern int isAsic(void);


oal_int32 host_module_init(oal_void)
{
    oal_int32 l_return;
    oal_net_device_stru *net_dev = NULL;
#if (_PRE_OS_VERSION_LITEOS != _PRE_OS_VERSION)
    oal_uint16 us_bitmap;

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    hwifi_config_init(CUS_TAG_DTS);
    hwifi_config_host_global_dts_param();
    hwifi_config_init(CUS_TAG_NV);
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */
#endif
    clear_cali_data_buff();

    l_return = hmac_main_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("host_module_init: hmac_main_init return error code: %d", l_return);
        return l_return;
    }

    l_return = wal_main_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("host_module_init: wal_main_init return error code: %d", l_return);
#if (_PRE_OS_VERSION_LITEOS != _PRE_OS_VERSION)
        us_bitmap = BIT7;
        builder_module_exit(us_bitmap);
#endif
        return l_return;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION))
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    hwifi_config_init_once();
#endif
    // wifi校准上电
    l_return = wlan_pm_open();
    if (l_return == OAL_SUCC) {
        // 校准上电OK之后下发配置VAP(校准时间长，防止未校准完成的时候，第二次上电，导致配置VAP没有下发)
        net_dev = oal_dev_get_by_name("wlan0");
        l_return = wal_cfg_vap_h2d_event(net_dev);
        if (l_return != OAL_SUCC) {
            OAL_IO_PRINT("host_module_init wal_cfg_vap_h2d_event FAIL %d", l_return);
        }
        OAL_IO_PRINT("host_module_init wal_cfg_vap_h2d_event succ");
    }

#endif

    /* 启动完成后，输出打印 */
    OAL_IO_PRINT("host_main_init finish!\n");

    return OAL_SUCC;
}
#endif

#if (_PRE_PRODUCT_ID_HI1131C_DEV ==_PRE_PRODUCT_ID)

oal_int32  hi1102_device_main_init(oal_void)
{
    oal_int32 l_return;
    oal_uint16 us_bitmap;
    frw_event_mem_stru *pst_event_mem = OAL_PTR_NULL;

    l_return = platform_module_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("host_bottom_main_init: platform_module_init return error code: %d\r\n", l_return);
        return l_return;
    }

    l_return = device_module_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("host_bottom_main_init: device_module_init return error code: %d\r\n", l_return);
        us_bitmap = BIT0 | BIT1 | BIT2 | BIT3;
        builder_module_exit(us_bitmap);
        return l_return;
    }

#ifndef WIN32
    /* 1102 需要在device初始化成功后同步速率级 */
    pst_event_mem = FRW_EVENT_ALLOC(0);
    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAL_IO_PRINT("hi1102_device_main_init: dmac_init_event_process FRW_EVENT_ALLOC result = OAL_PTR_NULL.\n");
        return OAL_FAIL;
    }

    l_return = dmac_init_event_process(pst_event_mem);
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("hi1102_device_main_init: dmac_init_event_process result = fale.\n");
        FRW_EVENT_FREE(pst_event_mem);
        return OAL_FAIL;
    }

    FRW_EVENT_FREE(pst_event_mem);
#endif

#if (!defined(_PRE_EDA))
    /* device_ready:调用HCC接口通知Hmac,Dmac已经完成初始化 TBD */
    chanslv_sendmsg_sync(D2H_MSG_WLAN_READY);
#endif
    /* 启动完成后，输出打印 */
    OAL_IO_PRINT("Hi1102_device_main_init:: Hi1102_device_main_init finish!\r\n");

    return OAL_SUCC;
}


typedef oal_uint8 (*dmac_psm_save_ps_state_type)(oal_void);
dmac_psm_save_ps_state_type g_fn_dmac_psm_save_ps_state_type_ram = dmac_psm_save_ps_state;
typedef oal_uint8 (*dmac_psm_recover_no_powerdown_type)(oal_void);
dmac_psm_recover_no_powerdown_type g_fn_dmac_psm_recover_no_powerdown_type_ram = dmac_psm_recover_no_powerdown;
typedef oal_void (*dmac_psm_recover_start_dma_type)(oal_uint8*, oal_uint8*, oal_uint8*);
dmac_psm_recover_start_dma_type g_fn_dmac_psm_recover_start_dma_type_ram = dmac_psm_recover_start_dma;
typedef oal_uint8 (*dmac_psm_recover_powerdown_type)(oal_uint8, oal_uint8, oal_uint8);
dmac_psm_recover_powerdown_type g_fn_dmac_psm_recover_powerdown_type_ram = dmac_psm_recover_powerdown;
typedef oal_uint8 (*dmac_psm_cbb_stopwork_type)(oal_void);
dmac_psm_cbb_stopwork_type g_fn_dmac_psm_cbb_stopwork_type_ram = dmac_psm_cbb_stopwork;
typedef oal_void (*dmac_psm_save_register_type)(oal_void);
dmac_psm_save_register_type g_fn_dmac_psm_save_register_type_ram = dmac_psm_save_register;
typedef oal_void (*dmac_psm_restore_register_type)(oal_void);
dmac_psm_restore_register_type g_fn_dmac_psm_restore_register_type_ram = dmac_psm_restore_register;

oal_void device_main_init(oal_void)
{
    /* init */
    extern volatile oal_uint32 g_ulGpioIntCount;
    oal_int32 l_return;
    l_return = hi1102_device_main_init();
    PRINT("device_main_init exit..."NEWLINE);
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("device_main_function: Hi1102_device_main_init return error code: %d", l_return);
        /* 初始化失败不退出主程序，等待重启 */
        for (; ;) {
        }
    }
    OAL_IO_PRINT("device_main_function: hi1102_device_main_init succ!!\r\n");

#if (SUB_SYSTEM == SUB_SYS_WIFI)
    if (g_pf_PM_WLAN_IsrRegister != NULL) {
        g_pf_PM_WLAN_IsrRegister();
    } else {
        OAM_ERROR_LOG0(0, 0, "g_pf_PM_WLAN_IsrRegister Empty");
        return;
    }

    if (g_pf_PM_WLAN_FuncRegister != NULL) {
        g_pf_PM_WLAN_FuncRegister(device_psm_main_function,     /* 1 */
                                  dmac_psm_check_hw_txq_state,  /* 2 */
                                  dmac_psm_check_txrx_state,    /* 3 */
                                  dmac_psm_clean_state,         /* 4 */
                                  dmac_psm_save_start_dma,      /* 5 */
                                  g_fn_dmac_psm_save_ps_state_type_ram,       /* 6 */
                                  g_fn_dmac_psm_recover_no_powerdown_type_ram, /* 7 */
                                  g_fn_dmac_psm_recover_start_dma_type_ram,   /* 8 */
                                  g_fn_dmac_psm_recover_powerdown_type_ram,   /* 9 */
                                  g_fn_dmac_psm_cbb_stopwork_type_ram,        /* 10 */
                                  dmac_psm_rf_awake,            /* 11 */
                                  dmac_psm_rf_sleep,            /* 12 */
                                  dmac_psm_is_fake_queues_empty, /* 13 */
                                  g_fn_dmac_psm_save_register_type_ram,       /* 14 */
                                  g_fn_dmac_psm_restore_register_type_ram,    /* 15 */
                                  dmac_wow_host_sleep_callback);    /* 16 */
    } else {
        OAM_ERROR_LOG0(0, 0, "g_pf_PM_WLAN_FuncRegister Empty");
        return;
    }
#endif
    g_ulGpioIntCount = 0;
}


typedef oal_void (*device_main_function_type)(oal_void);
device_main_function_type g_fn_device_main_function_type_ram = device_main_function;
oal_uint8 device_psm_main_function(oal_void)
{
#ifdef _PRE_WLAN_FEATURE_BTCOEX
    mac_device_stru         *pst_mac_device;
    hal_to_dmac_device_stru *pst_hal_device;
#endif
    g_fn_device_main_function_type_ram();

#ifdef _PRE_WLAN_FEATURE_BTCOEX
    pst_mac_device = mac_res_get_dev(0);
    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{device_psm_main_function::pst_device[id:0] is NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_hal_device = pst_mac_device->pst_device_stru;
    hal_btcoex_process_bt_status(pst_hal_device);
#endif

    return OAL_SUCC;
}

#elif (_PRE_PRODUCT_ID_HI1131C_HOST ==_PRE_PRODUCT_ID)

oal_int32 hi1102_host_main_init(oal_void)
{
    oal_int32 l_return;

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
    hcc_flowctl_get_device_mode_register(hmac_flowctl_check_device_is_sta_mode);
    hcc_flowctl_operate_subq_register(hmac_vap_net_start_subqueue, hmac_vap_net_stop_subqueue);
#else
    hcc_tx_flow_ctrl_cb_register(hmac_vap_net_stopall, hmac_vap_net_startall);
#endif

    l_return = host_module_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("Hi1102_host_main_init: host_module_init return error code: %d", l_return);
        return l_return;
    }

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
    wal_hipriv_register_inetaddr_notifier();
    wal_hipriv_register_inet6addr_notifier();
#endif

    /* 启动完成后，输出打印 */
    OAL_IO_PRINT("Hi113x_host_main_init finish!\n");

    return OAL_SUCC;
}


oal_void hi1102_host_main_exit(oal_void)
{
#if (_PRE_OS_VERSION_LITEOS != _PRE_OS_VERSION)
    oal_uint16 us_bitmap;
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
    wal_hipriv_unregister_inetaddr_notifier();
    wal_hipriv_unregister_inet6addr_notifier();
#endif

#if (_PRE_OS_VERSION_LITEOS != _PRE_OS_VERSION)
    us_bitmap =  BIT6 | BIT7 | BIT8;
    builder_module_exit(us_bitmap);
#endif

    return;
}
#endif

/*lint -e578*//*lint -e19*/
#if (_PRE_PRODUCT_ID_HI1131C_HOST == _PRE_PRODUCT_ID)
#ifndef CONFIG_HI110X_KERNEL_MODULES_BUILD_SUPPORT
#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT
#include "plat_board_adapt.h"
oal_int32 g_wifi_init_flag = 0;
oal_int32 g_wifi_init_ret;
/* built-in */
OAL_STATIC ssize_t wifi_sysfs_set_init(struct kobject *dev, struct kobj_attribute *attr, const char *buf, size_t count)
{
    char            mode[128] = {0};
    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    if ((sscanf_s(buf, "%20s", mode, sizeof(mode)) != 1)) {
        OAL_IO_PRINT("set value one param!\n");
        return -OAL_EINVAL;
    }

    if (sysfs_streq("init", mode)) {
        /* init */
        if (g_wifi_init_flag == 0) {
            g_wifi_init_ret = hi1102_host_main_init();
            g_wifi_init_flag = 1;
        } else {
            OAL_IO_PRINT("double init!\n");
        }
    } else {
        OAL_IO_PRINT("invalid input:%s\n", mode);
    }

    return count;
}

OAL_STATIC ssize_t wifi_sysfs_get_init(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int ret = 0;
    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    if (g_wifi_init_flag == 1) {
        if (g_wifi_init_ret == OAL_SUCC) {
            ret = snprintf_s(buf + ret, PAGE_SIZE - ret, PAGE_SIZE - ret - 1, "running\n");
        } else {
            ret = snprintf_s(buf + ret, PAGE_SIZE - ret, PAGE_SIZE - ret - 1, "boot failed ret=%d\n", g_wifi_init_ret);
        }
    } else {
        ret = snprintf_s(buf + ret, PAGE_SIZE - ret, PAGE_SIZE - ret - 1, "uninit\n");
    }
    if (ret < 0) {
        OAL_IO_PRINT("wifi_sysfs_get_init::snprintf_s failed!");
    }
    return ret;
}
OAL_STATIC struct kobj_attribute dev_attr_wifi =
    __ATTR(wifi, S_IRUGO | S_IWUSR, wifi_sysfs_get_init, wifi_sysfs_set_init);

OAL_STATIC struct attribute *wifi_init_sysfs_entries[] = {
    &dev_attr_wifi.attr,
    NULL
};

OAL_STATIC struct attribute_group wifi_init_attribute_group = {
    .attrs = wifi_init_sysfs_entries,
};

oal_int32 wifi_sysfs_init(oal_void)
{
    oal_int32 ret;
    oal_kobject *pst_root_boot_object = NULL;

    OAL_IO_PRINT("wifi_sysfs_init: chiptype is 113x.\n");

    pst_root_boot_object = oal_get_sysfs_root_boot_object();
    if (pst_root_boot_object == NULL) {
        OAL_IO_PRINT("[E]get root boot sysfs object failed!\n");
        return -OAL_EBUSY;
    }

    ret = sysfs_create_group(pst_root_boot_object, &wifi_init_attribute_group);
    if (ret) {
        OAL_IO_PRINT("sysfs create plat boot group fail.ret=%d\n", ret);
        ret = -OAL_ENOMEM;
        return ret;
    }

    return ret;
}

oal_void wifi_sysfs_exit(oal_void)
{
    /* need't exit,built-in */
    return;
}
oal_module_init(wifi_sysfs_init);
oal_module_exit(wifi_sysfs_exit);
#endif
#else
oal_module_init(hi1102_host_main_init);
oal_module_exit(hi1102_host_main_exit);
#endif
#endif

oal_module_license("GPL");
/*lint +e578*//*lint +e19*/

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

