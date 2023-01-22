

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include <linux/delay.h>
#include <linux/rtc.h>

#include "exception_rst.h"
#include "plat_type.h"
#include "plat_debug.h"
#include "plat_pm.h"
#include "plat_pm_wlan.h"
#include "exception_rst.h"
#include "plat_firmware.h"
#include "oal_file.h"
#include "plat_board_adapt.h"

#include "oal_sdio.h"
#include "oal_sdio_host_if.h"
#include "oal_hcc_host_if.h"
#include "heartbeat_host.h"
#include "stack.h"
#include "oam_ext_if.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_EXCEPTION_RST_C


/*****************************************************************************
  3 全局变量定义
*****************************************************************************/
struct st_exception_info *g_pst_exception_info = NULL;
EXPORT_SYMBOL(g_pst_exception_info);

struct st_wifi_dump_mem_info g_st_pkt_b_ram = {
    .mem_addr = PKT_B_RAM_BASEADDR,
    .size     = PKT_B_RAM_LEN,
    .file_name= (uint8_t *)"pkt_b_ram",
};

struct st_wifi_dump_mem_info g_st_ram_mem = {
    .mem_addr = RAM_BASEADDR,
    .size     = RAM_LEN,
    .file_name= (uint8_t *)"ram",
};

struct st_wifi_dump_mem_info g_st_pkt_h_ram = {
    .mem_addr = PKT_H_RAM_BASEADDR,
    .size     = PKT_H_RAM_LEN,
    .file_name= (uint8_t *)"pkt_h_ram",
};


p_st_wifi_demp_mem_info g_apst_panic_dump_mem[] = {
    &g_st_ram_mem,
    &g_st_pkt_b_ram,
    &g_st_pkt_h_ram,
    NULL,
};

p_st_wifi_demp_mem_info g_apst_trans_fail_dump_mem[] = {
    &g_st_ram_mem,
    &g_st_pkt_b_ram,
    &g_st_pkt_h_ram,
    NULL,
};

struct st_wifi_dump_mem_driver g_dump_mem_driver = {
    .is_open    = 0,
    .is_working = 0,
};

oal_uint32 g_recvd_block_count = 0;

/*****************************************************************************
  2 函数实现
*****************************************************************************/
oal_int32 get_exception_info_reference(struct st_exception_info **exception_data);
void  plat_exception_reset_work(struct work_struct *work);


void plat_dfr_cfg_set(oal_ulong arg)
{
    struct st_exception_info *pst_exception_data = NULL;
    int32_t ret;

    ret = get_exception_info_reference(&pst_exception_data);
    if (pst_exception_data == NULL) {
        PS_PRINT_ERR("get exception info reference is error\n");
        return;
    }
    if (ret != EXCEPTION_SUCCESS) {
        PS_PRINT_ERR("plat_exception_reset_work::get_exception_info_reference ret error!\n");
    }

    pst_exception_data->exception_reset_enable = arg ? (PLAT_EXCEPTION_ENABLE) : (PLAT_EXCEPTION_DISABLE);

    PS_PRINT_INFO("plat dfr cfg set value = %ld\n", arg);
}


oal_int32 get_exception_info_reference(struct st_exception_info **exception_data)
{
    if (exception_data == NULL) {
        PS_PRINT_ERR("%s parm exception_data is NULL\n", __func__);
        return -EXCEPTION_FAIL;
    }

    if (g_pst_exception_info == NULL) {
        *exception_data = NULL;
        PS_PRINT_ERR("%s g_pst_exception_info is NULL\n", __func__);
        return -EXCEPTION_FAIL;
    }

    *exception_data = g_pst_exception_info;

    return EXCEPTION_SUCCESS;
}
EXPORT_SYMBOL(get_exception_info_reference);


oal_int32 plat_exception_handler(oal_uint32 exception_type)
{
    oal_int32  ret;
    struct st_exception_info *pst_exception_data = NULL;

    stop_heartbeat();

    if (exception_type >= EXCEPTION_TYPE_BUTT) {
        PS_PRINT_ERR("para exception_type %u is error!\n", exception_type);
        return -EXCEPTION_FAIL;
    }

    ret = get_exception_info_reference(&pst_exception_data);
    if (pst_exception_data == NULL) {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -EXCEPTION_FAIL;
    }
    if (ret != EXCEPTION_SUCCESS) {
        PS_PRINT_ERR("plat_exception_handler::get_exception_info_reference ret error\n");
    }

    if (pst_exception_data->exception_reset_enable != PLAT_EXCEPTION_ENABLE) {
        PS_PRINT_INFO("plat exception reset not enable!");
        return EXCEPTION_SUCCESS;
    }

    pst_exception_data->wifi_exception_cnt += 1;
    ret = wifi_system_reset();
    if (ret != EXCEPTION_SUCCESS) {
        PS_PRINT_ERR("plat execption recovery fail! exception_type = [%u]\n",
                     pst_exception_data->excetion_type);
    } else {
        PS_PRINT_INFO("plat execption recovery success!\n");
    }

    return ret;
}

EXPORT_SYMBOL(plat_exception_handler);


oal_int32 wifi_system_reset(void)
{
    struct pm_drv_data *pm_data = pm_get_drvdata();

    if (pm_data == NULL) {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -EXCEPTION_FAIL;
    }
    /* 清除pm相关变量 */
    wlan_pm_info_clean();
    /* 重新上电，firmware重新加载 */
    hcc_disable(hcc_get_default_handler(), OAL_FALSE);

    wlan_pm_init_device_ready(pm_data->pst_wlan_pm_info);

    PS_PRINT_INFO("wifi system reset, board power on\n");

    if (firmware_download_function(WIFI_CFG) != EXCEPTION_SUCCESS) {
        PS_PRINT_ERR("hi110x system power reset failed!\n");
        return -EXCEPTION_FAIL;
    }

    if (oal_channel_transfer_prepare(oal_get_channel_default_handler()) != OAL_SUCC) {
        PS_PRINT_ERR("channel transfer prepare fail\n");
        return -EXCEPTION_FAIL;
    }

    if (wlan_pm_wait_device_ready(pm_data->pst_wlan_pm_info) == 0) {
        PS_PRINT_ERR("wlan_pm_wait_device_ready timeout %d !!!!!!", HOST_WAIT_BOTTOM_INIT_TIMEOUT);
        chr_exception(chr_wifi_drv(CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_WCPU_BOOTUP));
        return -EXCEPTION_FAIL;
    }

    hcc_enable(hcc_get_default_handler(), OAL_FALSE);

    return EXCEPTION_SUCCESS;
}


void wifi_exception_dev_panic_info_get(oal_file_stru *fp, p_st_wifi_demp_mem_info pst_mem_dump_info)
{
    if (&g_st_ram_mem == pst_mem_dump_info) {
        oal_uint32 ul_tempdata;
        oal_uint8  *pucDataBuf = oal_kzalloc(PLAT_EXCEPTION_DEV_PANIC_PC_LR_LEN, OAL_GFP_KERNEL);
        if (pucDataBuf == NULL) {
            OAL_IO_PRINT("[exception_dbg]mem alloc fail\r\n");
            return;
        }

        if (oal_file_lseek(fp, PLAT_EXCEPTION_DEV_PANIC_LR_ADDR, OAL_SEEK_SET) != PLAT_EXCEPTION_DEV_PANIC_LR_ADDR) {
            OAL_IO_PRINT("[exception_dbg]file seek fail\r\n");
            oal_free(pucDataBuf);
            return;
        }

        if (oal_file_read(fp, (int8_t *)pucDataBuf, PLAT_EXCEPTION_DEV_PANIC_PC_LR_LEN) !=
            PLAT_EXCEPTION_DEV_PANIC_PC_LR_LEN) {
            OAL_IO_PRINT("[exception_dbg]file rd fail\r\n");
            oal_free(pucDataBuf);
            return;
        }

        ul_tempdata = (*(pucDataBuf));
        ul_tempdata |= ((*(pucDataBuf + 0x1))<<8);
        ul_tempdata |= ((*(pucDataBuf + 0x2))<<16);
        ul_tempdata |= ((*(pucDataBuf + 0x3))<<24);
        OAL_IO_PRINT("[exception_dbg]panic lr::%p\r\n", (void *)(uintptr_t)ul_tempdata);
        OAM_ERROR_LOG1(0, OAM_SF_DFT, "[E]WiFi Device Panic LR::%p\r\n", (uintptr_t)ul_tempdata);
        ul_tempdata = (*(pucDataBuf + 0x4));
        ul_tempdata |= ((*(pucDataBuf + 0x5))<<8);
        ul_tempdata |= ((*(pucDataBuf + 0x6))<<16);
        ul_tempdata |= ((*(pucDataBuf + 0x7))<<24);
        OAL_IO_PRINT("[exception_dbg]panic pc::%p\r\n", (void *)(uintptr_t)ul_tempdata);
        OAM_ERROR_LOG1(0, OAM_SF_DFT, "[E]WiFi Device Panic PC::%p\r\n", (uintptr_t)ul_tempdata);
        oal_free(pucDataBuf);
    }
}


oal_int32 wifi_exception_mem_dump(p_st_wifi_demp_mem_info *pst_mem_dump_info)
{
    oal_int32 ret;
    struct st_exception_info *pst_exception_data = NULL;
    oal_channel_stru *hi_channel = oal_get_channel_default_handler();

    ret = get_exception_info_reference(&pst_exception_data);
    if (pst_exception_data == NULL) {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -EXCEPTION_FAIL;
    }
    if (ret != EXCEPTION_SUCCESS) {
        PS_PRINT_ERR("wifi_exception_mem_dump::get_exception_info_reference ret error\n");
    }

    if (hi_channel == NULL) {
        PS_PRINT_ERR("channel is NULL!\n");
        return -EXCEPTION_FAIL;
    }

#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    if (pst_exception_data->excetion_type == HOST_EXCP) {
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
        ret = oal_sdio_func_reset();
#else
        ret = oal_sdio_reinit();
#endif
    } else {
        ret = oal_sdio_reinit();
    }
#endif
    if (ret < 0) {
        PS_PRINT_ERR("wifi mem dump:sdio reinit failed, ret=[%d]\n", ret);
        return -EXCEPTION_FAIL;
    }

    oal_channel_claim_host(hi_channel);
    device_mem_dump_etc(pst_mem_dump_info);
    oal_disable_channel_state(hi_channel, OAL_SDIO_ALL);
    oal_channel_release_host(hi_channel);

    return EXCEPTION_SUCCESS;
}

EXPORT_SYMBOL(wifi_exception_mem_dump);

void oal_wakeup_exception(void)
{
    oal_ulong flags;

    DECLARE_DFT_TRACE_KEY_INFO("exception", OAL_DFT_TRACE_EXCEP);
    if (g_pst_exception_info == NULL) {
        OAL_IO_PRINT("[E]%s, g_pst_exception_info is null\n", __FUNCTION__);
        return;
    }

    oal_spin_lock_irq_save(&(g_pst_exception_info->excp_lock), &flags);
    if (work_busy(&(g_pst_exception_info->excp_worker))) {
        oal_spin_unlock_irq_restore(&g_pst_exception_info->excp_lock, &flags);
        return;
    }

    hcc_change_state_exception();

    g_pst_exception_info->excetion_type = TRANS_FAIL;
    schedule_work(&(g_pst_exception_info->excp_worker));
    oal_spin_unlock_irq_restore(&(g_pst_exception_info->excp_lock), &flags);
}


void oal_exception_submit(oal_int32 excep_type)
{
    oal_ulong flags;

    DECLARE_DFT_TRACE_KEY_INFO("sdio_exception", OAL_DFT_TRACE_EXCEP);
    if (g_pst_exception_info == NULL) {
        OAL_IO_PRINT("[E]%s, g_pst_exception_info is null\n", __FUNCTION__);
        return;
    }

    if (wlan_is_shutdown()) {
        PS_PRINT_INFO("device has power off!\r\n");
        return;
    }

    if (wlan_pm_state_get() == HOST_ALLOW_TO_SLEEP) {
        PS_PRINT_INFO("device has sleep!\r\n");
        return;
    }
    wlan_pm_set_pm_sts_exception();
    oal_spin_lock_irq_save(&(g_pst_exception_info->excp_lock), &flags);
    if (work_busy(&(g_pst_exception_info->excp_worker))) {
        OAL_IO_PRINT("excep %d block, exception %d is working\n", excep_type, g_pst_exception_info->excetion_type);
        oal_spin_unlock_irq_restore(&g_pst_exception_info->excp_lock, &flags);
        return;
    }

    hcc_change_state_exception();

    g_pst_exception_info->excetion_type = excep_type;
    schedule_work(&(g_pst_exception_info->excp_worker));
    oal_spin_unlock_irq_restore(&(g_pst_exception_info->excp_lock), &flags);
}
EXPORT_SYMBOL(oal_exception_submit);

oal_int32 oal_exception_is_busy(void)
{
    if (OAL_UNLIKELY(g_pst_exception_info == NULL)) {
        return OAL_FALSE;
    }

    if (work_busy(&(g_pst_exception_info->excp_worker))) {
        /* sdio mem dump is processing, can't power off or submit repeat */
        return OAL_TRUE;
    }

    return OAL_FALSE;
}
EXPORT_SYMBOL(oal_exception_is_busy);

oal_int32 oal_trigger_exception(oal_int32 is_sync)
{
    oal_ulong timeout_jiffies;
    if (oal_exception_is_busy() == OAL_TRUE) {
        return OAL_TRUE;
    }
    OAL_IO_PRINT("oal_trigger_exception start\n");
    /* trigger device panic */
    if (oal_channel_send_msg(oal_get_channel_default_handler(), H2D_MSG_TEST)) {
        OAL_IO_PRINT("send sdio panic message failed!\n");
        return OAL_FALSE;
    }

    if (is_sync != OAL_TRUE) {
        OAL_IO_PRINT("sdio exception is doing...\n");
        return OAL_TRUE;
    }

    /* wait device panic */
    timeout_jiffies = OAL_TIME_JIFFY + OAL_MSECS_TO_JIFFIES(2000);
    for (; ;) {
        if (oal_exception_is_busy() == OAL_TRUE) {
            break;
        }

        if (oal_time_after(OAL_TIME_JIFFY, timeout_jiffies)) {
            OAL_IO_PRINT("wait panic message timeout!\n");
            return OAL_FALSE;
        }

        oal_msleep(OAL_JIFFIES_TO_MSECS(1));
    }

    OAL_IO_PRINT("trigger sdio exception manually sucuess\n");
    return OAL_TRUE;
}
EXPORT_SYMBOL(oal_trigger_exception);


/* Try to dump device mem, controlled by flag sdio_dump_mem_flag */
void oal_try_to_dump_device_mem(oal_int32 is_sync)
{
    int32_t ret;
    if ((g_pst_exception_info->dump_mem_flag) == NOT_DUMP_MEM) {
        OAL_IO_PRINT("sdio_dump_mem_flag is NOT_DUMP_MEM\r\n");
        return;
    }

    OAL_IO_PRINT("Try to dump device mem!\n");
    ret = oal_trigger_exception(is_sync);
    if (ret != OAL_TRUE) {
        OAL_IO_PRINT("oal_try_to_dump_device_mem::oal_trigger_exception error!\n");
    }
}
EXPORT_SYMBOL(oal_try_to_dump_device_mem);


oal_int32 oal_device_panic_callback(void *data)
{
    OAL_IO_PRINT("oal_device_panic\n");
    oal_exception_submit(DEVICE_PANIC);
    return OAL_SUCC;
}

oal_void oal_dev2host_gpio_hold_time_check_sleep(void)
{
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    usleep(20);
#else
    usleep_range(10, 20);
#endif
    return;
}

/*
 * 检查DEVICE WAKEUP HOST gpio 是否拉高。
 * this func is not check whether the device is panic,
 * it is give a long time for device reset CPU
 */
oal_int32 oal_dev2host_gpio_hold_time_check(oal_uint32 switch_timeout, oal_uint32 hold_time)
{
    oal_ulong timeout;
    oal_uint32 gpio_value;
    oal_int32 state = GPIO_HOLD_STATE_INIT; /* 0 init, 1 gpio to high */
    oal_time_t_stru time_start;
    oal_time_t_stru time_stop;
    oal_uint64  trans_us;

    if (!switch_timeout) {
        switch_timeout = 200;
    }

    timeout = OAL_TIME_JIFFY + OAL_MSECS_TO_JIFFIES(switch_timeout);
    time_start = oal_ktime_get();
    for (; ;) {
        gpio_value = board_get_wlan_wkup_gpio_val();
        if (state == GPIO_HOLD_STATE_INIT) {
            if (gpio_value != GPIO_LOWLEVEL) {
                time_stop = oal_ktime_get();
                trans_us = (oal_uint64)oal_ktime_to_us(oal_ktime_sub(time_stop, time_start));
                OAL_IO_PRINT("device reset sdio ip cost %llu us\n", trans_us);
                timeout = OAL_TIME_JIFFY + OAL_MSECS_TO_JIFFIES(hold_time);
                state = GPIO_HOLD_STATE_HIGH;
                continue;
            }
            if (oal_time_after(OAL_TIME_JIFFY, timeout)) {
                OAL_IO_PRINT("[E]wait wakeup gpio to high timeout [%u] ms\n", switch_timeout);
                return OAL_FALSE;
            }
            oal_dev2host_gpio_hold_time_check_sleep();
            continue;
        } else if (state == GPIO_HOLD_STATE_HIGH) {
            if (gpio_value == GPIO_LOWLEVEL) {
                OAL_IO_PRINT("[E]gpio pull down again!\n");
                return OAL_FALSE;
            }
            if (oal_time_after(OAL_TIME_JIFFY, timeout)) {
                return OAL_TRUE;
            } else {
                oal_dev2host_gpio_hold_time_check_sleep();
                continue;
            }
        } else {
            OAL_IO_PRINT("[E]error state=%d\n", state);
            return OAL_FALSE;
        }
    }
}

oal_int32 check_except_type(oal_int32  excp_type)
{
    if (excp_type == DEVICE_PANIC) {
        DECLARE_DFT_TRACE_KEY_INFO("wifi_device_panic", OAL_DFT_TRACE_EXCEP);
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "[E]WiFi Device Panic");
        return OAL_SUCC;
    } else if(excp_type == TRANS_FAIL) {
        DECLARE_DFT_TRACE_KEY_INFO("wifi_device_collapse", OAL_DFT_TRACE_EXCEP);
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "[E]wifi_device_collapse");
        return OAL_SUCC;
    } else {
        PS_PRINT_ERR("excp_type error, excp_type = %d\r\n", excp_type);
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "[E]excp_type error");
        return -EINVAL;
    }
}

/* 如果是panic类型，则需要确定gpio时间确保device复位成功 */
void check_panic_except(oal_int32  *excp_type)
{
    if (*excp_type != DEVICE_PANIC) {
        return;
    }

#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    if (!oal_get_hisdio_intr_mode()) {
        /* sdio mode */
        oal_msleep(200);
        return;
    }
#endif

    /* gpio mode */
    if (oal_dev2host_gpio_hold_time_check(500, 200) != OAL_TRUE) {
        *excp_type = TRANS_FAIL;
    }
}

void check_trans_fail_except(oal_int32  *excp_type)
{
    if (*excp_type != TRANS_FAIL) {
        return;
    }
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    oal_msleep(5000);
#endif
    *excp_type = DEVICE_PANIC;
}

oal_int32 exception_mem_dump(oal_int32  excp_type)
{
    oal_int32 ret;

    if (excp_type == TRANS_FAIL) {
        OAL_IO_PRINT("wifi_exception_mem_dump,excep_type is TRANS_FAIL, line = %d\n", __LINE__);
        wlan_rst();
        ret = wifi_exception_mem_dump(g_apst_trans_fail_dump_mem);
    } else if (excp_type == DEVICE_PANIC) {
        OAL_IO_PRINT("wifi_exception_mem_dump,excep_type is DEVICE_PANIC, line = %d\n", __LINE__);
        ret = wifi_exception_mem_dump(g_apst_panic_dump_mem);
    } else if (excp_type == HOST_EXCP) {
        OAL_IO_PRINT("wifi_exception_mem_dump,excep_type is HOST_EXCP, line = %d\n", __LINE__);
        board_power_on();
        ret = wifi_exception_mem_dump(g_apst_trans_fail_dump_mem);
    } else {
        ret = -EINVAL;
    }

    return ret;
}

void  oal_exception_mem_dump_handler(struct pm_drv_data *pm_data, oal_channel_stru* hi_channel)
{
    oal_int32 ret;

    // first time, try panic
    oal_channel_wake_lock(hi_channel);
    ret = exception_mem_dump(g_pst_exception_info->excetion_type);
    oal_channel_wake_unlock(hi_channel);
    if (ret < 0) {
        // second time, try trans fail
        OAL_IO_PRINT("exception_mem_dump fail, exception trans to TRANS_FAIL, line = %d\n", __LINE__);
        g_pst_exception_info->excetion_type = TRANS_FAIL;
        oal_channel_wake_lock(hi_channel);
        ret = exception_mem_dump(g_pst_exception_info->excetion_type);
        oal_channel_wake_unlock(hi_channel);
    }

#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    if (ret < 0) {
        // third time, reset sdio host
        OAL_IO_PRINT("exception_mem_dump fail, exception trans to TRANS_FAIL, line = %d\n", __LINE__);
        g_pst_exception_info->excetion_type = HOST_EXCP;
        oal_channel_wake_lock(hi_channel);
        ret = exception_mem_dump(g_pst_exception_info->excetion_type);
        oal_channel_wake_unlock(hi_channel);
    }
#endif

    if (ret < 0) {
        OAL_IO_PRINT("exception_mem_dump fail, line = %d\n", __LINE__);
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "Panic File Save Failed!");

        oal_channel_release_host(hi_channel);
        oal_disable_channel_state(hi_channel, OAL_SDIO_ALL);
        return;
    } else {
        OAL_IO_PRINT("exception_mem_dump ok,exception is %d, line = %d\n", g_pst_exception_info->excetion_type,
                     __LINE__);
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "Panic File Save OK!");
    }

    oal_disable_channel_state(hi_channel, OAL_SDIO_ALL);
    if (g_pst_exception_info->wifi_dfr_func == NULL) {
        PS_PRINT_ERR("g_pst_exception_info->wifi_dfr_fun is NULL\r\n");
        return;
    }
    (g_pst_exception_info->wifi_dfr_func)();
}

void  oal_exception_handler(oal_work_stru *work)
{
    struct pm_drv_data *pm_data = pm_get_drvdata();
    oal_channel_stru* hi_channel = NULL;

    if ((pm_data == NULL) || (pm_data->pst_wlan_pm_info == NULL)) {
        OAL_IO_PRINT("[E]pm_paramater is null\n");
        return;
    }
    hi_channel = pm_data->pst_wlan_pm_info->pst_channel;

    OAL_IO_PRINT("oal_sdio_exception_handler(), line = %d\n", __LINE__);

    /* 打印host侧pm统计信息 */
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    wlan_pm_dump_host_gpio_state();
#endif
    wlan_pm_dump_host_info();

    if ((hi_channel == NULL) || (g_pst_exception_info == NULL)) {
        OAL_IO_PRINT("[E]paramater is null\n");
        return;
    }

    if (check_except_type(g_pst_exception_info->excetion_type) < 0) {
        return;
    }

    oal_mutex_lock(&pm_data->host_mutex);

    oal_disable_channel_state(hi_channel, OAL_SDIO_ALL);
    oal_wlan_gpio_intr_enable(hi_channel, OAL_FALSE);

#ifdef _PRE_OAL_FEATURE_KEY_PROCESS_TRACE
    oal_dft_print_all_key_info();
#endif
#ifdef CONFIG_PRINTK
    hwifi_panic_log_dump(KERN_DEBUG);
#endif

    check_panic_except((int32_t *)&(g_pst_exception_info->excetion_type));
    check_trans_fail_except((int32_t *)&(g_pst_exception_info->excetion_type));

    oal_exception_mem_dump_handler(pm_data, hi_channel);
    oal_mutex_unlock(&pm_data->host_mutex);
}


oal_int32 plat_exception_reset_init(void)
{
    int32_t ret;
    if (g_pst_exception_info != NULL) {
        PS_PRINT_ERR("g_pst_exception_info is not null\r\n");
        return -EXCEPTION_FAIL;
    }

    g_pst_exception_info = (struct st_exception_info *)oal_kzalloc(sizeof(struct st_exception_info), OAL_GFP_KERNEL);
    if (g_pst_exception_info == NULL) {
        PS_PRINT_ERR("kzalloc p_exception_data is failed!\n");
        goto fail;
    }

    g_pst_exception_info->excetion_type = EXCEPTION_TYPE_BUTT;

    g_pst_exception_info->exception_reset_enable   = PLAT_EXCEPTION_ENABLE;
    g_pst_exception_info->wifi_exception_cnt       = 0;

    oal_spin_lock_init(&g_pst_exception_info->excp_lock);
    OAL_INIT_WORK(&g_pst_exception_info->excp_worker, oal_exception_handler);

    g_pst_exception_info->wifi_dfr_func = NULL;

    /* usb低功耗场景下通道不下电，为节省功耗，关闭heartbeat */
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    if (heart_beat_init() < 0) {
        goto fail;
    }
#endif
    PS_PRINT_INFO("plat exception reset init success\n");

    plat_exception_dump_file_rotate_init();

    return EXCEPTION_SUCCESS;

fail:
    ret = plat_exception_reset_exit();
    if (ret != EXCEPTION_SUCCESS) {
        PS_PRINT_ERR("plat_exception_reset_init::plat_exception_reset_exit error\n");
    }
    return EXCEPTION_FAIL;
}

EXPORT_SYMBOL_GPL(plat_exception_reset_init);


oal_int32 plat_exception_reset_exit(void)
{
    if (g_pst_exception_info == NULL) {
        PS_PRINT_ERR("g_pst_exception_info is NULL!\n");
        return -EXCEPTION_FAIL;
    }

    if (g_pst_exception_info != NULL) {
        oal_cancel_work_sync(&g_pst_exception_info->excp_worker);
        kfree(g_pst_exception_info);
        g_pst_exception_info = NULL;
    }

    heart_beat_release();

    PS_PRINT_INFO("plat exception reset exit success\n");

    return EXCEPTION_SUCCESS;
}

EXPORT_SYMBOL_GPL(plat_exception_reset_exit);

#ifndef WIN32
struct st_wifi_dump_mem_driver *plat_get_dump_mem_driver(void)
{
    return &g_dump_mem_driver;
}

void plat_exception_dump_file_rotate_init(void)
{
    OAL_WAIT_QUEUE_INIT_HEAD(&g_dump_mem_driver.dump_type_wait);
    skb_queue_head_init(&g_dump_mem_driver.dump_type_queue);
    skb_queue_head_init(&g_dump_mem_driver.quenue);
    PS_PRINT_INFO("plat exception dump file rotate init success\n");
}

void plat_exception_dump_quenue_clear(void)
{
    struct sk_buff *skb = NULL;
    struct st_wifi_dump_mem_driver *p_dumpmem_driver = NULL;

    p_dumpmem_driver = plat_get_dump_mem_driver();
    while ((skb = skb_dequeue(&p_dumpmem_driver->quenue)) != NULL) {
        kfree_skb(skb);
    }
}

void plat_exception_dump_finish(void)
{
    struct st_wifi_dump_mem_driver *p_dumpmem_driver = NULL;

    p_dumpmem_driver = plat_get_dump_mem_driver();
    p_dumpmem_driver->is_working = 0;
}

int32_t plat_exception_dump_enquenue(uint8_t *buf_ptr, uint32_t count)
{
    struct sk_buff *skb = NULL;
    struct st_wifi_dump_mem_driver *p_dumpmem_driver = NULL;

    p_dumpmem_driver = plat_get_dump_mem_driver();
    PS_PRINT_DBG("[send] len:%d\n", count);
    if (!p_dumpmem_driver->is_working) {
        PS_PRINT_ERR("excp_memdump_queue_etc not allow\n");
        return -EINVAL;
    }
    if (buf_ptr == NULL) {
        PS_PRINT_ERR("buf_ptr is NULL\n");
        return -EINVAL;
    }

    skb = alloc_skb(count, oal_in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
    if (skb == NULL) {
        PS_PRINT_ERR("can't allocate mem for new debug skb, len=%d\n", count);
        return -EINVAL;
    }

    if (memcpy_s(skb_tail_pointer(skb), count, buf_ptr, count) != EOK) {
        PS_PRINT_ERR("memcpy_s failed\n");
        kfree_skb(skb);
        skb = NULL;
        return -EINVAL;
    }
    skb_put(skb, count);
    skb_queue_tail(&p_dumpmem_driver->quenue, skb);
    PS_PRINT_DBG("[excp_memdump_queue_etc]qlen:%d,count:%d\n", p_dumpmem_driver->quenue.qlen, count);
    return 0;
}

/*
 * Prototype    : plat_excp_send_rotate_cmd_to_app_etc
 * Description  : driver send rotate cmd to app for rotate file
 */
int32_t plat_excp_send_rotate_cmd_to_app(uint32_t which_dump)
{
    struct sk_buff *skb = NULL;
    struct st_wifi_dump_mem_driver *p_dumpmem_driver = NULL;

    p_dumpmem_driver = plat_get_dump_mem_driver();
    if (which_dump >= CMD_DUMP_BUFF) {
        PS_PRINT_WARNING("which dump:%d error\n", which_dump);
        return -EINVAL;
    }
    if (skb_queue_len(&p_dumpmem_driver->dump_type_queue) > PLAT_EXCP_DUMP_ROTATE_QUEUE_MAX_LEN) {
        PS_PRINT_WARNING("too many dump type in queue,dispose type:%d", which_dump);
        return -EINVAL;
    }

    skb = alloc_skb(sizeof(which_dump), GFP_KERNEL);
    if (skb == NULL) {
        PS_PRINT_ERR("alloc errno skbuff failed! len=%d, errno=%x\n",
            (int32_t)sizeof(which_dump), which_dump);
        return -EINVAL;
    }
    skb_put(skb, sizeof(which_dump));
    *(uint32_t *)skb->data = which_dump;
    skb_queue_tail(&p_dumpmem_driver->dump_type_queue, skb);
    PS_PRINT_INFO("save rotate cmd [%d] in queue\n", which_dump);

    wake_up_interruptible(&p_dumpmem_driver->dump_type_wait);

    return 0;
}

int32_t plat_exception_dump_notice(void)
{
    struct st_wifi_dump_mem_driver *p_dumpmem_driver = NULL;
    PS_PRINT_FUNCTION_NAME;

    p_dumpmem_driver = plat_get_dump_mem_driver();
    if (p_dumpmem_driver->is_working) {
        PS_PRINT_ERR("is doing memdump\n");
        return -1;
    }
    plat_exception_dump_quenue_clear();
    plat_excp_send_rotate_cmd_to_app(CMD_READM_WIFI_SDIO);
    p_dumpmem_driver->is_working = 1;
    return 0;
}



void wifi_exception_dev_panic_info_get_etc(uint8_t *DataBuf,
                                           uint32_t pos,
                                           p_st_wifi_demp_mem_info pst_mem_dump_info)
{
    uint8_t     *pucDataBuf = NULL;
    uint32_t    ul_tempdata;
    if (&g_st_ram_mem != pst_mem_dump_info) {
        PS_PRINT_INFO("wifi_exception_dev_panic_info_get_etc not ram.../\n");
        return;
    }

    pucDataBuf = DataBuf + pos;
    ul_tempdata = *(uint32_t *)pucDataBuf;
    OAM_ERROR_LOG1(0, OAM_SF_DFT, "[E]WiFi Device Panic LR::%p\r\n", (uintptr_t)ul_tempdata);
    pucDataBuf =  pucDataBuf + sizeof(uint32_t);
    ul_tempdata = *(uint32_t *)pucDataBuf;
    OAM_ERROR_LOG1(0, OAM_SF_DFT, "[E]WiFi Device Panic PC::%p\r\n", (uintptr_t)ul_tempdata);
}
#endif

