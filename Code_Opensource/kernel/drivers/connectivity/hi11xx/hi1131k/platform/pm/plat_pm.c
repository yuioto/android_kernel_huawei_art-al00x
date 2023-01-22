

/*****************************************************************************
  1 Header File Including
*****************************************************************************/
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/tty.h>
#include <linux/notifier.h>
#include "plat_board_adapt.h"
#include "plat_debug.h"
#include "plat_firmware.h"
#include "plat_pm.h"
#include "oal_sdio_host_if.h"
#include "oal_hcc_host_if.h"
#include "oal_schedule.h"
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include <linux/kobject.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/host.h>
#include <linux/gpio.h>
#include <linux/suspend.h>
#include "exception_rst.h"
#include "heartbeat_host.h"
#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#include <linux/module.h>   /* kernel module definitions */
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/wakelock.h>
#endif
#ifdef CONFIG_HUAWEI_DSM
#if (_PRE_LINUX_PLATFORM == MIAMI_C60)
#include "dsm/dsm_pub.h"
#endif
#endif
#include "oal_channel_host_if.h"

/*****************************************************************************
  2 Global Variable Definition
*****************************************************************************/
struct pm_drv_data *pm_drv_data_t = NULL;

struct pm_drv_data *pm_get_drvdata(void)
{
    return pm_drv_data_t;
}

static void pm_set_drvdata(struct pm_drv_data *data)
{
    pm_drv_data_t = data;
}

oal_int32 wlan_is_shutdown(void)
{
    struct pm_drv_data *pm_data = pm_get_drvdata();
    if (pm_data == NULL) {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    return ((pm_data->pst_wlan_pm_info->ul_wlan_power_state == POWER_STATE_SHUTDOWN) ? true : false);
}


OAL_STATIC int32_t firmware_download_main_function(uint32_t which_cfg)
{
    int32_t ret;

    PS_PRINT_INFO("firmware_download begin\n");
    ret = firmware_download(which_cfg);
    if (ret < 0) {
        PS_PRINT_ERR("firmware download fail!\n");
        DECLARE_DFT_TRACE_KEY_INFO("patch_download_fail", OAL_DFT_TRACE_FAIL);
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_FW, CHR_PLAT_DRV_ERROR_CFG_FAIL_FIRMWARE_DOWN);
        return -FAILURE;
    }

    DECLARE_DFT_TRACE_KEY_INFO("patch_download_ok", OAL_DFT_TRACE_SUCC);

    PS_PRINT_INFO("firmware_download success\n");
    return SUCCESS;
}


int firmware_download_function(oal_uint32 which_cfg)
{
    oal_int32 ret;
    unsigned long long total_time;
    oal_time_t_stru start_time, end_time, trans_time;
    static unsigned long long max_time = 0;
    static unsigned long long count = 0;
    oal_channel_stru *hi_channel = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata();
    if ((pm_data == NULL) || (pm_data->pst_wlan_pm_info->pst_channel == NULL)) {
        PS_PRINT_ERR("pm_data or pst_channel is NULL!\n");
        return -FAILURE;
    }

    hi_channel = pm_data->pst_wlan_pm_info->pst_channel;
    start_time = oal_ktime_get();

    if (which_cfg >= CFG_FILE_TOTAL) {
        PS_PRINT_ERR("cfg file index [%d] outof range\n", which_cfg);
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_FW, CHR_PLAT_DRV_ERROR_WIFI_FW_DOWN);
        return -FAILURE;
    }
    board_power_on();
    oal_channel_wake_lock(hi_channel);
    oal_channel_claim_host(hi_channel);

    wlan_pm_init_dev();
#ifndef _PRE_FEATURE_NO_GPIO
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    ret = oal_sdio_reinit();
    if (ret < 0) {
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_FW, CHR_PLAT_DRV_ERROR_FW_SDIO_INIT);
        ret = oal_sdio_func_reset();
    }
    if (ret < 0) {
        PS_PRINT_ERR("sdio reinit failed, ret:%d!\n", ret);
#ifdef CONFIG_HUAWEI_DSM
#if (_PRE_LINUX_PLATFORM == MIAMI_C60)
        hw_1131k_dsm_client_notify(SYSTEM_TYPE_PLATFORM, DSM_1131K_DOWNLOAD_FIRMWARE,
            "%s: sdio reinit failed, ret %d \n", __FUNCTION__, ret);
#endif
#endif
        goto done;
    }
#endif
#endif
    ret = firmware_download_main_function(which_cfg);
    if (ret != SUCCESS) {
        goto done;
    }
    end_time = oal_ktime_get();

    trans_time = oal_ktime_sub(end_time, start_time);
    total_time = (unsigned long long)oal_ktime_to_us(trans_time);
    if (total_time > max_time) {
        max_time = total_time;
    }

    count++;
    PS_PRINT_WARNING("download firmware, count [%llu], current time [%llu]us, max time [%llu]us\n", count, total_time, max_time);

    ret =  SUCCESS;

done:
    oal_channel_release_host(hi_channel);
    oal_channel_wake_unlock(hi_channel);
    return ret;
}

oal_int32 wifi_power_fail_process(oal_int32 error)
{
    struct pm_drv_data *pm_data = pm_get_drvdata();
    if (pm_data == NULL) {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return WIFI_POWER_FAIL;
    }

    if (error >= WIFI_POWER_ENUM_BUTT) {
        PS_PRINT_ERR("error is undefined, error=[%d]\n", error);
        return WIFI_POWER_FAIL;
    }

    PS_PRINT_INFO("wifi power fail, error=[%d]\n", error);

    switch (error) {
        /* BFGX off，wifi firmware download fail和wait boot up fail，直接返回失败，上层重试，不走DFR */
        case WIFI_POWER_BFGX_OFF_BOOT_UP_FAIL:
            if (oal_trigger_exception(OAL_TRUE) == OAL_TRUE) {
                /* exception is processing, can't power off */
                PS_PRINT_INFO("sdio exception is working\n");
                break;
            }
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
            oal_wlan_gpio_intr_enable(oal_get_sdio_default_handler(), OAL_FALSE);
#endif
            PS_PRINT_INFO("set wlan_power_state to shutdown\n");
            pm_data->pst_wlan_pm_info->ul_wlan_power_state = POWER_STATE_SHUTDOWN;
        // 复用 WIFI_POWER_BFGX_OFF_FIRMWARE_DOWNLOAD_FAIL 进行对device下点操作
        // 不用break
        case WIFI_POWER_BFGX_OFF_FIRMWARE_DOWNLOAD_FAIL: // lint !e616
            PS_PRINT_INFO("wifi power fail: pull down power on gpio\n");
            board_power_off();
            break;
        default:
            PS_PRINT_ERR("error is undefined, error=[%d]\n", error);
            break;
    }

    return WIFI_POWER_FAIL;
}

OAL_STATIC int32_t wlan_power_on_judg_exception_busy(oal_time_t_stru *start_time)
{
    *start_time = oal_ktime_get();

    PS_PRINT_INFO("wlan power on!\n");
    if (oal_exception_is_busy() == OAL_TRUE) {
        DECLARE_DFT_TRACE_KEY_INFO("open_fail_sdio_is_busy", OAL_DFT_TRACE_FAIL);
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_OPEN, CHR_PLAT_DRV_ERROR_POWER_ON_EXCP);
        return -FAILURE;
    }
    return SUCCESS;
}

oal_int32 wlan_power_on(void)
{
    oal_int32  ret;
    oal_int32  error = WIFI_POWER_SUCCESS;
    unsigned long long total_time;
    oal_time_t_stru start_time, end_time, trans_time;
    static unsigned long long max_download_time = 0;
    static unsigned long long num = 0;
    struct pm_drv_data *pm_data = pm_get_drvdata();
    if (pm_data == NULL) {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }
    if (wlan_power_on_judg_exception_busy(&start_time) != SUCCESS) {
        PS_PRINT_ERR("wlan_power_on_judg_exception_busy error!\n");
        return -FAILURE;
    }

    wlan_pm_init_device_ready(pm_data->pst_wlan_pm_info);

    if (firmware_download_function(WIFI_CFG) == WIFI_POWER_SUCCESS) {
        pm_data->pst_wlan_pm_info->ul_wlan_power_state = POWER_STATE_OPEN;
        if (oal_channel_transfer_prepare(pm_data->pst_wlan_pm_info->pst_channel) != OAL_SUCC) {
            PS_PRINT_ERR("channel transfer prepare fail\n");
            error = WIFI_POWER_BFGX_OFF_FIRMWARE_DOWNLOAD_FAIL;
            goto wifi_power_fail;
        }
    } else {
        PS_PRINT_ERR("firmware download fail\n");
        error = WIFI_POWER_BFGX_OFF_FIRMWARE_DOWNLOAD_FAIL;
        goto wifi_power_fail;
    }

    if (!wlan_pm_wait_device_ready(pm_data->pst_wlan_pm_info)) {
        DECLARE_DFT_TRACE_KEY_INFO("wlan_poweron_dev_ready_by_gpio_fail", OAL_DFT_TRACE_FAIL);
        PS_PRINT_ERR("wlan_pm_wait_device_ready timeout %d !!!!!!\n", HOST_WAIT_BOTTOM_INIT_TIMEOUT);
        error = WIFI_POWER_BFGX_OFF_BOOT_UP_FAIL;
        chr_exception(chr_wifi_drv(CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_WCPU_BOOTUP));
#ifdef CONFIG_HUAWEI_DSM
#if (_PRE_LINUX_PLATFORM == MIAMI_C60)
        hw_1131k_dsm_client_notify(SYSTEM_TYPE_PLATFORM, DSM_1131K_HALT, "%s: wlan power on dev ready by gpio fail\n",
            __FUNCTION__);
#endif
#endif
        goto wifi_power_fail;
    }

    end_time = oal_ktime_get();
    trans_time = oal_ktime_sub(end_time, start_time);
    total_time = (unsigned long long)oal_ktime_to_us(trans_time);
    if (total_time > max_download_time) {
        max_download_time = total_time;
    }

    num++;
    PS_PRINT_WARNING("power on, count [%llu], current time [%llu]us, max time [%llu]us\n",
        num, total_time, max_download_time);

    ret = WIFI_POWER_SUCCESS;
    goto wifi_power_over;

wifi_power_fail: /* 1131C-debug */
    ret = wifi_power_fail_process(error);
    chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                         CHR_PLT_DRV_EVENT_OPEN, CHR_PLAT_DRV_ERROR_WCPU_BOOTUP);
wifi_power_over:
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    board_set_host_to_dev_gpio_val_high();
#endif
    return ret;
}

oal_int32 wlan_power_off(void)
{
    struct pm_drv_data *pm_data = pm_get_drvdata();

    if (pm_data == NULL) {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    PS_PRINT_INFO("wlan power off!\n");
    stop_heartbeat();
    hcc_disable(hcc_get_default_handler(), OAL_TRUE);

    pm_data->pst_wlan_pm_info->ul_wlan_power_state = POWER_STATE_SHUTDOWN;
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    oal_wlan_gpio_intr_enable(oal_get_sdio_default_handler(), OAL_FALSE);
    oal_disable_channel_state(oal_get_sdio_default_handler(), OAL_SDIO_ALL);
#endif

    board_power_off();
    DECLARE_DFT_TRACE_KEY_INFO("wlan_poweroff_by_gpio", OAL_DFT_TRACE_SUCC);

    return SUCCESS;
}

/* return 1 for wifi power on,0 for off. */
oal_int32 hi110x_get_wifi_power_stat(oal_void)
{
    struct pm_drv_data *pm_data = pm_get_drvdata();
    if (pm_data == NULL) {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return 0;
    }
    return (pm_data->pst_wlan_pm_info->ul_wlan_power_state != POWER_STATE_SHUTDOWN);
}
EXPORT_SYMBOL(hi110x_get_wifi_power_stat);

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
/*
  * Function: suspend_notify
  * Description: suspend notify call back
  * Ruturn: 0 -- success
 * */
static int pf_suspend_notify(struct notifier_block *notify_block,
                             unsigned long mode, void *unused)
{
    struct pm_drv_data *pm_data = pm_get_drvdata();
    if (pm_data == NULL) {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return IRQ_NONE;
    }

    switch (mode) {
        case PM_POST_SUSPEND:
            PS_PRINT_INFO("host resume OK!\n");
            break;
        case PM_SUSPEND_PREPARE:
            PS_PRINT_INFO("host suspend now!\n");
            break;
        default:
            break;
    }
    return 0;
}

static struct notifier_block pf_suspend_notifier = {
    .notifier_call = pf_suspend_notify,
    .priority = INT_MIN,
};

OAL_STATIC int low_power_remove(void)
{
    int ret = 0;
    struct pm_drv_data *pm_data = pm_get_drvdata();
    if (pm_data == NULL) {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    unregister_pm_notifier(&pf_suspend_notifier);

    wlan_pm_exit();

    /* free platform driver data struct */
    oal_free(pm_data);

    pm_data = NULL;

    pm_set_drvdata(NULL);

    return ret;
}

OAL_STATIC int low_power_probe(void)
{
    int ret;
    struct pm_drv_data  *pm_data = NULL;

    pm_data = oal_memalloc(sizeof(struct pm_drv_data));
    if (pm_data == NULL) {
        PS_PRINT_ERR("no mem to allocate pm_data\n");
        goto PMDATA_MALLOC_FAIL;
    }

    pm_data->pst_wlan_pm_info = wlan_pm_init();
    if (pm_data->pst_wlan_pm_info == 0) {
        PS_PRINT_ERR("no mem to allocate wlan_pm_info\n");
        goto WLAN_INIT_FAIL;
    }

    pm_data->board = get_board_info();

    /* init mutex */
    OAL_MUTEX_INIT(&pm_data->host_mutex);

    /* set driver data */
    pm_set_drvdata(pm_data);

    /* register host pm */
    ret = register_pm_notifier(&pf_suspend_notifier);
    if (ret < 0) {
        PS_PRINT_ERR("%s : register_pm_notifier failed!\n", __func__);
    }

    return OAL_SUCC;

WLAN_INIT_FAIL:
    oal_free(pm_data);
PMDATA_MALLOC_FAIL:
    return -ENOMEM;
}

int low_power_init(void)
{
    int ret;

    ret = low_power_probe();
    if (ret != SUCCESS) {
        PS_PRINT_ERR("low_power_init: low_power_probe fail\n");
    }

    PS_PRINT_INFO("low_power_init: success\n");
    return ret;
}

void  low_power_exit(void)
{
    low_power_remove();
}

#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
void wake_lock_destroy(struct wake_lock *lock);
OAL_STATIC int low_power_remove(void)
{
    int ret = 0;
    struct pm_drv_data *pm_data = pm_get_drvdata();
    if (pm_data == NULL) {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    wlan_pm_exit();

    /* remove kobject */
    /* delete timer */
    del_timer_sync(&pm_data->bfg_timer);
    del_timer_sync(&pm_data->dev_ack_timer);

    /* destory wake lock */
    wake_lock_destroy(&pm_data->bfg_wake_lock);

    /* free platform driver data struct */
    kfree(pm_data);

    pm_data = NULL;

    pm_set_drvdata(NULL);

    return ret;
}

OAL_STATIC int low_power_probe(void)
{
    int ret;
    struct pm_drv_data  *pm_data = NULL;
    struct workqueue_struct *host_wkup_dev_workq = NULL;

    pm_data = zalloc(sizeof(struct pm_drv_data));
    if (pm_data == NULL) {
        PS_PRINT_ERR("no mem to allocate pm_data\n");
        goto PMDATA_MALLOC_FAIL;
    }

    pm_data->pst_wlan_pm_info = wlan_pm_init();
    if (pm_data->pst_wlan_pm_info == 0) {
        PS_PRINT_ERR("no mem to allocate wlan_pm_info\n");
        goto WLAN_INIT_FAIL;
    }
    pm_set_drvdata(pm_data);
    OAL_MUTEX_INIT(&pm_data->host_mutex);

    return OAL_SUCC;

WLAN_INIT_FAIL:
    kfree(pm_data);
PMDATA_MALLOC_FAIL:
    return -ENOMEM;
}

int low_power_init(void)
{
    int32_t ret;

    ret = low_power_probe();
    if (ret != SUCCESS) {
        PS_PRINT_ERR("low_power_init: low_power_probe fail\n");
    }

    PS_PRINT_INFO("low_power_init: success\n");
    return ret;
}

void  low_power_exit(void)
{
    low_power_remove();
}
#endif

#ifdef CONFIG_HUAWEI_DSM
#if (_PRE_LINUX_PLATFORM == MIAMI_C60)
OAL_DEFINE_SPINLOCK(g_dsm_lock);
static struct dsm_dev dsm_wifi = {
    .name = "dsm_wifi",
    .device_name = NULL,
    .ic_name = NULL,
    .module_name = NULL,
    .fops = NULL,
    .buff_size = DMD_EVENT_BUFF_SIZE,
};

struct dsm_client *hw_1131k_dsm_wifi_client = NULL;
void hw_1131k_register_dsm_client(void)
{
    if (hw_1131k_dsm_wifi_client == NULL) {
        hw_1131k_dsm_wifi_client = dsm_register_client(&dsm_wifi);
    }
}

void hw_1131k_unregister_dsm_client(void)
{
    if (hw_1131k_dsm_wifi_client != NULL) {
        dsm_unregister_client(hw_1131k_dsm_wifi_client, &dsm_wifi);
        hw_1131k_dsm_wifi_client = NULL;
    }
}
#define LOG_BUF_SIZE 512
int32_t g_last_dsm_id = 0;
/*
 * 功能描述  : DMD事件上报
 * 修改历史  :
 *   修改内容: 新增函数
 */
void hw_1131k_dsm_client_notify(int32_t sub_sys, int32_t dsm_id, const int8_t *fmt, ...)
{
    oal_ulong flags;
    int8_t buf[LOG_BUF_SIZE] = {0};
    struct dsm_client *dsm_client_buf = NULL;
    va_list ap;
    int32_t ret = 0;

    switch (sub_sys) {
        case SYSTEM_TYPE_WIFI:
        case SYSTEM_TYPE_PLATFORM:
            dsm_client_buf = hw_1131k_dsm_wifi_client;
            break;
        case SYSTEM_TYPE_BT:
        default:
            PS_PRINT_ERR("hw_1131k_dsm_client_notify::unsupport dsm sub_type[%d]", sub_sys);
            break;
    }

    if (dsm_client_buf != NULL) {
        if (fmt != NULL) {
            va_start(ap, fmt);
            ret = vsnprintf_s(buf, sizeof(buf), sizeof(buf) - 1, fmt, ap);
            va_end(ap);
            if (ret < 0) {
                PS_PRINT_ERR("vsnprintf_s fail, line[%d]\n", __LINE__);
                return;
            }
        } else {
            PS_PRINT_ERR("dsm_client_buf is null, line[%d]\n", __LINE__);
            return;
        }
    }

    oal_spin_lock_irq_save(&g_dsm_lock, &flags);
    if (!dsm_client_ocuppy(dsm_client_buf)) {
        dsm_client_record(dsm_client_buf, buf);
        dsm_client_notify(dsm_client_buf, dsm_id);
        g_last_dsm_id = dsm_id;
        OAL_IO_PRINT("[I]wifi dsm_client_notify success,dsm_id=%d[%s]\n", dsm_id, buf);
    } else {
        OAL_IO_PRINT("[E]wifi dsm_client_notify failed,last_dsm_id=%d dsm_id=%d\n", g_last_dsm_id, dsm_id);

        // retry dmd record
        dsm_client_unocuppy(dsm_client_buf);
        if (!dsm_client_ocuppy(dsm_client_buf)) {
            dsm_client_record(dsm_client_buf, buf);
            dsm_client_notify(dsm_client_buf, dsm_id);
            OAL_IO_PRINT("[I]wifi dsm notify success, dsm_id=%d[%s]\n", dsm_id, buf);
        } else {
            OAL_IO_PRINT("[E]wifi dsm client ocuppy, dsm notify failed, dsm_id=%d\n", dsm_id);
        }
    }
    oal_spin_unlock_irq_restore(&g_dsm_lock, &flags);
}
EXPORT_SYMBOL(hw_1131k_dsm_client_notify);
#endif
#endif


