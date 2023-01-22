
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
/*****************************************************************************
  1 Header File Including
*****************************************************************************/
#include <linux/module.h>   /* kernel module definitions */
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/wakelock.h>
#include "securec.h"
#include "oal_sdio.h"
#include "oal_sdio_comm.h"
#include "oal_sdio_host_if.h"

#include "plat_pm_wlan.h"
#include "plat_pm.h"

#include "oal_hcc_host_if.h"
#include "oam_ext_if.h"
#include "plat_debug.h"
#include "exception_rst.h"
#include "oal_interrupt.h"

#ifdef __cpluscplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_PLAT_PM_WLAN_C

/*****************************************************************************
     2 Global Variable Definition
*****************************************************************************/
static uint8_t g_heartbeat_cnt; // The variable type must be the same as that of the device. (heartbeat_cnt)
struct wlan_pm_s* gpst_wlan_pm_info = OAL_PTR_NULL;

wlan_pm_callback_stru     gst_wlan_pm_callback = {
    .wlan_pm_wakeup_dev  = wlan_pm_wakeup_dev,
    .wlan_pm_state_get   = wlan_pm_state_get,
    .wlan_pm_wakeup_host = wlan_pm_wakeup_host,
    .wlan_pm_feed_wdg    = wlan_pm_feed_wdg,
    .wlan_pm_wakeup_dev_ack    = wlan_pm_wakeup_dev_ack,

};
oal_bool_enum g_wlan_pm_switch = 0; /* 1131C-debug */

EXPORT_SYMBOL_GPL(g_wlan_pm_switch);

oal_uint32 auto_freq_enable = 1; /* 1131C-debug */
module_param(auto_freq_enable, uint, S_IRUGO | S_IWUSR);


oal_int32 wlan_power_on(void);
oal_int32 wlan_power_off(void);
irqreturn_t wlan_gpio_irq(oal_int32 irq, oal_void *dev_id);

oal_ulong wlan_pm_waitting_for_resume(oal_void);

void wlan_pm_wakeup_work(oal_work_stru *pst_worker);
void wlan_pm_sleep_work(oal_work_stru *pst_worker);
void wlan_pm_freq_adjust_work(oal_work_stru *pst_worker);

void wlan_pm_wdg_timeout(unsigned long data);

oal_int32 wlan_pm_wakeup_done_callback(void *data);
oal_int32 wlan_pm_close_done_callback(void *data);
oal_int32 wlan_pm_open_bcpu_done_callback(void *data);
oal_int32 wlan_pm_close_bcpu_done_callback(void *data);

oal_int32 wlan_pm_stop_wdg(struct wlan_pm_s *pst_wlan_pm_info);
oal_long wlan_pm_work_submit(struct wlan_pm_s    *pst_wlan_pm, oal_work_stru* pst_worker);
void wlan_pm_info_clean(void);


struct wlan_pm_s*  wlan_pm_get_drv(oal_void)
{
    return gpst_wlan_pm_info;
}

EXPORT_SYMBOL_GPL(wlan_pm_get_drv);


oal_int32  wlan_pm_set_device_ready(oal_void* data)
{
    struct wlan_pm_s    *pst_wlan_pm = (struct wlan_pm_s*)data;
    OAM_WARNING_LOG0(0, OAM_SF_PWR, "#### wlan_pm_set_device_ready\n");

    OAL_COMPLETE(&pst_wlan_pm->st_device_ready);
    return OAL_SUCC;
}


oal_void wlan_pm_init_device_ready(struct wlan_pm_s    *pst_wlan_pm)
{
    OAL_INIT_COMPLETION(&pst_wlan_pm->st_device_ready);
}


oal_uint32 wlan_pm_wait_device_ready(struct wlan_pm_s    *pst_wlan_pm)
{
    oal_uint32 rv;
    /* 等待host下半部初始化完成 */
    rv = oal_wait_for_completion_timeout(
        &pst_wlan_pm->st_device_ready, (oal_uint32)OAL_MSECS_TO_JIFFIES(HOST_WAIT_BOTTOM_INIT_TIMEOUT));
    if (rv != 0) {
        start_heartbeat();
    }
    return rv;
}


oal_int32 wlan_pm_heartbeat_callback(void *data)
{
    g_heartbeat_cnt += 1;
    PS_PRINT_WARNING("[pm] g_heartbeat_cnt :%d\r\n", g_heartbeat_cnt);
    return SUCCESS;
}


oal_ulong  wlan_pm_exit(oal_void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();

    if (pst_wlan_pm == OAL_PTR_NULL) {
        return OAL_SUCC;
    }

    wlan_pm_stop_wdg(pst_wlan_pm);
    oal_channel_message_unregister(pst_wlan_pm->pst_channel, D2H_MSG_WAKEUP_SUCC);
    oal_channel_message_unregister(pst_wlan_pm->pst_channel, D2H_MSG_WLAN_READY);
    oal_channel_message_unregister(pst_wlan_pm->pst_channel, D2H_MSG_ALLOW_SLEEP);
    oal_channel_message_unregister(pst_wlan_pm->pst_channel, D2H_MSG_DISALLOW_SLEEP);
    oal_channel_message_unregister(pst_wlan_pm->pst_channel, D2H_MSG_POWEROFF_ACK);
    oal_channel_message_unregister(pst_wlan_pm->pst_channel, D2H_MSG_OPEN_BCPU_ACK);
    oal_channel_message_unregister(pst_wlan_pm->pst_channel, D2H_MSG_CLOSE_BCPU_ACK);
    kfree(pst_wlan_pm);

    gpst_wlan_pm_info = OAL_PTR_NULL;

    OAL_IO_PRINT("[plat_pm]wlan_pm_exit ok!");

    return OAL_SUCC;
}


oal_uint32 wlan_pm_is_poweron(oal_void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();

    if (pst_wlan_pm == OAL_PTR_NULL) {
        return OAL_FALSE;
    }

    if (pst_wlan_pm->ul_wlan_power_state == POWER_STATE_OPEN) {
        return OAL_TRUE;
    } else {
        return OAL_FALSE;
    }
}
EXPORT_SYMBOL_GPL(wlan_pm_is_poweron);


oal_long wlan_pm_work_submit(struct wlan_pm_s    *pst_wlan_pm, oal_work_stru* pst_worker)
{
    oal_long  i_ret    = 0;

    if (work_busy(pst_worker)) {
        /* If comm worker is processing,
            we need't submit again */
        i_ret = -OAL_EBUSY;
        goto done;
    } else {
        queue_work(pst_wlan_pm->pst_pm_wq, pst_worker);
    }
done:
    return i_ret;
}

void wlan_pm_freq_adjust_work(oal_work_stru *pst_worker)
{
    hcc_tx_transfer_lock(hcc_get_default_handler());

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
    if (g_pst_alg_process_func.p_auto_freq_adjust_to_level_func != OAL_PTR_NULL) {
        g_pst_alg_process_func.p_auto_freq_adjust_to_level_func();
    } else {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "wlan_pm_freq_adjust_work:NO p_auto_freq_adjust_to_level_func registered");
    }
#endif

    hcc_tx_transfer_unlock(hcc_get_default_handler());
}

struct wifi_srv_callback_handler* wlan_pm_get_wifi_srv_handler(oal_void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();

    if (pst_wlan_pm == OAL_PTR_NULL) {
        return OAL_FALSE;
    }

    return &pst_wlan_pm->st_wifi_srv_handler;
}

oal_ulong wlan_pm_adjust_feq(void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();

    if (!auto_freq_enable) {
        return OAL_SUCC;
    }

    OAL_BUG_ON(!pst_wlan_pm);

    if (wlan_pm_work_submit(pst_wlan_pm, &pst_wlan_pm->st_freq_adjust_work) != 0) {
        OAM_WARNING_LOG0(0, OAM_SF_PWR, "wlan_pm_adjust_feq submit work fail !\n");
    }

    return OAL_SUCC;
}

#elif (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)

/*****************************************************************************
  1 Header File Including
*****************************************************************************/
#include <linux/module.h>   /* kernel module definitions */
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include <linux/kobject.h>
#include <linux/irq.h>
#include "oal_wakelock.h"
#include <linux/mmc/sdio.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/host.h>
#include <linux/gpio.h>
#include "oal_sdio.h"
#include "oal_sdio_comm.h"
#include "oal_sdio_host_if.h"

#include "heartbeat_host.h"
#include "plat_pm_wlan.h"
#include "plat_pm.h"

#include "oal_interrupt.h"
#include "oal_hcc_host_if.h"
#include "oam_ext_if.h"
#include "plat_debug.h"
#include "plat_cali.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_PLAT_PM_WLAN_C

#ifdef __cpluscplus
#if __cplusplus
extern "C" {
#endif
#endif
/*****************************************************************************
  2 Global Variable Definition
*****************************************************************************/
static uint8_t g_heartbeat_cnt; // The variable type must be the same as that of the device. (heartbeat_cnt)
struct wlan_pm_s* gpst_wlan_pm_info = OAL_PTR_NULL;

wlan_pm_callback_stru     gst_wlan_pm_callback = {
    .wlan_pm_wakeup_dev  = wlan_pm_wakeup_dev,
    .wlan_pm_state_get   = wlan_pm_state_get,
    .wlan_pm_wakeup_host = wlan_pm_wakeup_host,
    .wlan_pm_feed_wdg    = wlan_pm_feed_wdg,
    .wlan_pm_wakeup_dev_ack    = wlan_pm_wakeup_dev_ack,

};
oal_bool_enum g_wlan_pm_switch = OAL_FALSE; /* 1131C-debug */

EXPORT_SYMBOL_GPL(g_wlan_pm_switch);

oal_uint32 auto_freq_enable = 1; /* 1131C-debug */
module_param(auto_freq_enable, uint, S_IRUGO | S_IWUSR);

oal_int32 wlan_power_on(void);
oal_int32 wlan_power_off(void);
irqreturn_t wlan_gpio_irq(oal_int32 irq, oal_void *dev_id);

void wlan_pm_wakeup_work(oal_work_stru *pst_worker);
void wlan_pm_sleep_work(oal_work_stru *pst_worker);
void wlan_pm_freq_adjust_work(oal_work_stru *pst_worker);

void wlan_pm_wdg_timeout(unsigned long data);

oal_int32 wlan_pm_wakeup_done_callback(void *data);
oal_int32 wlan_pm_close_done_callback(void *data);
oal_int32 wlan_pm_open_bcpu_done_callback(void *data);
oal_int32 wlan_pm_close_bcpu_done_callback(void *data);
oal_int32 wlan_pm_halt_bcpu_done_callback(void *data);


oal_int32 wlan_pm_stop_wdg(struct wlan_pm_s *pst_wlan_pm_info);
oal_long wlan_pm_work_submit(struct wlan_pm_s    *pst_wlan_pm, oal_work_stru* pst_worker);
void wlan_pm_info_clean(void);


struct wlan_pm_s*  wlan_pm_get_drv(oal_void)
{
    return gpst_wlan_pm_info;
}

EXPORT_SYMBOL_GPL(wlan_pm_get_drv);


oal_int32  wlan_pm_set_device_ready(oal_void* data)
{
    struct wlan_pm_s    *pst_wlan_pm = (struct wlan_pm_s*)data;
    OAM_WARNING_LOG0(0, OAM_SF_PWR, "#### wlan_pm_set_device_ready\n");

    OAL_COMPLETE(&pst_wlan_pm->st_device_ready);
    return OAL_SUCC;
}


oal_void wlan_pm_init_device_ready(struct wlan_pm_s    *pst_wlan_pm)
{
    OAL_INIT_COMPLETION(&pst_wlan_pm->st_device_ready);
}


oal_uint32 wlan_pm_wait_device_ready(struct wlan_pm_s    *pst_wlan_pm)
{
    oal_uint32 rv;
    /* 等待host下半部初始化完成 */
    rv = oal_wait_for_completion_timeout(
        &pst_wlan_pm->st_device_ready, (oal_uint32)OAL_MSECS_TO_JIFFIES(HOST_WAIT_BOTTOM_INIT_TIMEOUT));
    if (rv != 0) {
        start_heartbeat();
    }
    return rv;
}


oal_int32 wlan_pm_heartbeat_callback(void *data)
{
    g_heartbeat_cnt += 1;
    PS_PRINT_WARNING("[pm] g_heartbeat_cnt :%d\r\n", g_heartbeat_cnt);
    return SUCCESS;
}


oal_ulong  wlan_pm_exit(oal_void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();

    if (pst_wlan_pm == OAL_PTR_NULL) {
        return OAL_SUCC;
    }

    wlan_pm_stop_wdg(pst_wlan_pm);
    oal_channel_message_unregister(pst_wlan_pm->pst_channel, D2H_MSG_WAKEUP_SUCC);
    oal_channel_message_unregister(pst_wlan_pm->pst_channel, D2H_MSG_WLAN_READY);
    oal_channel_message_unregister(pst_wlan_pm->pst_channel, D2H_MSG_ALLOW_SLEEP);
    oal_channel_message_unregister(pst_wlan_pm->pst_channel, D2H_MSG_DISALLOW_SLEEP);
    oal_channel_message_unregister(pst_wlan_pm->pst_channel, D2H_MSG_POWEROFF_ACK);
    oal_channel_message_unregister(pst_wlan_pm->pst_channel, D2H_MSG_OPEN_BCPU_ACK);
    oal_channel_message_unregister(pst_wlan_pm->pst_channel, D2H_MSG_CLOSE_BCPU_ACK);

#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    {
        BOARD_INFO *pst_board = get_board_info();
        oal_int32 wlan_irq;

        if (pst_board == OAL_PTR_NULL) {
            OAL_IO_PRINT("failed to get board info!\n");
            oal_free(pst_wlan_pm);
            return OAL_PTR_NULL;
        }
        wlan_irq = pst_board->wlan_wakeup_irq;
        if (wlan_irq != INVALID_IRQ) {
            oal_free_irq(wlan_irq, pst_wlan_pm->pst_channel);
            OAM_INFO_LOG0(0, OAM_SF_PWR, "success to unregister wakeup gpio intr!\n");
        }
    }
#endif

    oal_destroy_workqueue(pst_wlan_pm->pst_pm_wq);

    kfree(pst_wlan_pm);

    gpst_wlan_pm_info = OAL_PTR_NULL;

    OAL_IO_PRINT("[plat_pm]wlan_pm_exit ok!");

    return OAL_SUCC;
}


oal_uint32 wlan_pm_is_poweron(oal_void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();

    if (pst_wlan_pm == OAL_PTR_NULL) {
        return OAL_FALSE;
    }

    if (pst_wlan_pm->ul_wlan_power_state == POWER_STATE_OPEN) {
        return OAL_TRUE;
    } else {
        return OAL_FALSE;
    }
}
EXPORT_SYMBOL_GPL(wlan_pm_is_poweron);


struct wifi_srv_callback_handler* wlan_pm_get_wifi_srv_handler(oal_void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();

    if (pst_wlan_pm == OAL_PTR_NULL) {
        return OAL_PTR_NULL;
    }

    return &pst_wlan_pm->st_wifi_srv_handler;
}
EXPORT_SYMBOL_GPL(wlan_pm_get_wifi_srv_handler);


#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)


oal_ulong  wlan_pm_open_bcpu(oal_void)
{
#define RETRY_TIMES (3)
    oal_uint32           i;
    oal_int32            ret;
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();
    oal_uint32           ul_ret;

    if (pst_wlan_pm == OAL_PTR_NULL) {
        return OAL_FAIL;
    }

    /* 通过sdio配置命令，解复位BCPU */
    OAM_WARNING_LOG0(0, OAM_SF_PWR, "sdio open BCPU");

    hcc_tx_transfer_lock(hcc_get_default_handler());

    for (i = 0; i < RETRY_TIMES; i++) {
        ret = wlan_pm_wakeup_dev();
        if (ret == OAL_SUCC) {
            break;
        }
    }

    if (ret != OAL_SUCC) {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "wlan_pm_wakeup_dev fail!");
        hcc_tx_transfer_unlock(hcc_get_default_handler());
        return OAL_FAIL;
    }

    OAM_WARNING_LOG1(0, OAM_SF_PWR, "wlan_pm_wakeup_dev succ, retry times [%d]", i);

    OAL_INIT_COMPLETION(&pst_wlan_pm->st_open_bcpu_done);

    ul_ret = oal_channel_send_msg(pst_wlan_pm->pst_channel, H2D_MSG_RESET_BCPU);
    if (ul_ret == OAL_SUCC) {
        /* 等待device执行命令 */
        oal_up(&pst_wlan_pm->pst_channel->gpio_rx_sema);
        ul_ret =  oal_wait_for_completion_timeout(
            &pst_wlan_pm->st_open_bcpu_done, (oal_uint32)OAL_MSECS_TO_JIFFIES(WLAN_OPEN_BCPU_WAIT_TIMEOUT));
        if (ul_ret == 0) {
            OAM_ERROR_LOG0(0, OAM_SF_PWR, "wlan_pm_open_bcpu wait device ACK timeout !");
            hcc_tx_transfer_unlock(hcc_get_default_handler());
            return OAL_FAIL;
        }

        hcc_tx_transfer_unlock(hcc_get_default_handler());
        return  OAL_SUCC;
    } else {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "fail to send H2D_MSG_RESET_BCPU");
        hcc_tx_transfer_unlock(hcc_get_default_handler());
        return  OAL_FAIL;
    }
}
#endif


oal_long wlan_pm_work_submit(struct wlan_pm_s    *pst_wlan_pm, oal_work_stru* pst_worker)
{
    oal_long  i_ret    = 0;

    if (oal_work_is_busy(pst_worker)) {
        /* If comm worker is processing,
            we need't submit again */
        i_ret = -OAL_EBUSY;
        goto done;
    } else {
        OAM_INFO_LOG1(0, OAM_SF_PWR, "WiFi %pF Worker Submit\n", (uintptr_t)pst_worker->func);
        if (oal_queue_work(pst_wlan_pm->pst_pm_wq, pst_worker) == false) {
            i_ret = -OAL_EFAIL;
        }
    }
done:
    return i_ret;
}


oal_int32 wlan_pm_close_done_callback(void *data)
{
    struct wlan_pm_s *pst_wlan_pm = (struct wlan_pm_s *)data;

    OAM_WARNING_LOG0(0, OAM_SF_PWR, "wlan_pm_close_done_callback !");

    /* 关闭RX通道，防止SDIO RX thread继续访问SDIO */
    oal_disable_channel_state(pst_wlan_pm->pst_channel, OAL_SDIO_RX);

    pst_wlan_pm->ul_close_done_callback++;
    OAL_COMPLETE(&pst_wlan_pm->st_close_done);

    OAM_WARNING_LOG0(0, OAM_SF_PWR, "complete H2D_MSG_PM_WLAN_OFF done!");

    return SUCCESS;
}


oal_int32 wlan_pm_open_bcpu_done_callback(void *data)
{
    struct wlan_pm_s *pst_wlan_pm = (struct wlan_pm_s *)data;

    OAM_WARNING_LOG0(0, OAM_SF_PWR, "wlan_pm_open_bcpu_done_callback !");

    pst_wlan_pm->ul_open_bcpu_done_callback++;
    OAL_COMPLETE(&pst_wlan_pm->st_open_bcpu_done);

    OAM_WARNING_LOG0(0, OAM_SF_PWR, "complete H2D_MSG_RESET_BCPU done!");

    return SUCCESS;
}


oal_int32 wlan_pm_close_bcpu_done_callback(void *data)
{
    struct wlan_pm_s *pst_wlan_pm = (struct wlan_pm_s *)data;

    OAM_WARNING_LOG0(0, OAM_SF_PWR, "wlan_pm_close_bcpu_done_callback !");

    pst_wlan_pm->ul_close_bcpu_done_callback++;
    OAL_COMPLETE(&pst_wlan_pm->st_close_bcpu_done);

    OAM_WARNING_LOG0(0, OAM_SF_PWR, "complete H2D_MSG_PM_BCPU_OFF done!");

    return SUCCESS;
}


oal_int32 wlan_pm_halt_bcpu_done_callback(void *data)
{
    struct wlan_pm_s *pst_wlan_pm = (struct wlan_pm_s *)data;

    OAM_WARNING_LOG0(0, OAM_SF_PWR, "wlan_pm_halt_bcpu_done_callback !");

    OAL_COMPLETE(&pst_wlan_pm->st_halt_bcpu_done);

    OAM_WARNING_LOG0(0, OAM_SF_PWR, "complete wlan_pm_halt_bcpu_done_callback done!");

    return SUCCESS;
}

void wlan_pm_freq_adjust_work(oal_work_stru *pst_worker)
{
    hcc_tx_transfer_lock(hcc_get_default_handler());

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
    if (g_pst_alg_process_func.p_auto_freq_adjust_to_level_func != OAL_PTR_NULL) {
        g_pst_alg_process_func.p_auto_freq_adjust_to_level_func();
    } else {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "wlan_pm_freq_adjust_work:NO p_auto_freq_adjust_to_level_func registered");
    }
#endif

    hcc_tx_transfer_unlock(hcc_get_default_handler());
}


oal_ulong wlan_pm_adjust_feq(void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();

    if (!auto_freq_enable) {
        return OAL_SUCC;
    }

    OAL_BUG_ON(!pst_wlan_pm);

    if (wlan_pm_work_submit(pst_wlan_pm, &pst_wlan_pm->st_freq_adjust_work) != 0) {
        OAM_WARNING_LOG0(0, OAM_SF_PWR, "wlan_pm_adjust_feq submit work fail !\n");
    }

    return OAL_SUCC;
}
EXPORT_SYMBOL_GPL(wlan_pm_adjust_feq);


oal_int32 wlan_pm_poweroff_cmd(oal_void)
{
    oal_int32            ret;
    oal_int32           l_val;
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();

    OAM_WARNING_LOG0(0, OAM_SF_PWR, "Send H2D_MSG_PM_WLAN_OFF cmd");

    hcc_tx_transfer_lock(hcc_get_default_handler());

    if (wlan_pm_wakeup_dev() != OAL_SUCC) {
        hcc_tx_transfer_unlock(hcc_get_default_handler());
        return OAL_FAIL;
    }

    ret =  oal_channel_send_msg(pst_wlan_pm->pst_channel, H2D_MSG_PM_WLAN_OFF);
    if (ret == OAL_SUCC) {
        /* 等待device执行命令 */
        msleep(20);
        l_val = board_get_wlan_wkup_gpio_val();
        if (l_val == 0) {
            OAM_ERROR_LOG1(0, OAM_SF_PWR, "wlan_pm_poweroff_cmd  wait device ACK timeout && GPIO_LEVEL[%d] !", l_val);

            hcc_tx_transfer_unlock(hcc_get_default_handler());

            return OAL_FAIL;
        }
    } else {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "fail to send H2D_MSG_PM_WLAN_OFF");
        hcc_tx_transfer_unlock(hcc_get_default_handler());
        return  OAL_FAIL;
    }

    hcc_tx_transfer_unlock(hcc_get_default_handler());

    return OAL_SUCC;
}


oal_int32 wlan_pm_shutdown_bcpu_cmd(oal_void)
{
#define RETRY_TIMES (3)
    oal_uint32           i;
    oal_int32            ret;
    oal_uint32           ul_ret;
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();

    OAM_WARNING_LOG0(0, OAM_SF_PWR, "Send H2D_MSG_PM_BCPU_OFF cmd");

    hcc_tx_transfer_lock(hcc_get_default_handler());

    for (i = 0; i < RETRY_TIMES; i++) {
        ret = wlan_pm_wakeup_dev();
        if (ret == OAL_SUCC) {
            break;
        }
    }

    if (ret != OAL_SUCC) {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "wlan_pm_wakeup_dev fail!");
        hcc_tx_transfer_unlock(hcc_get_default_handler());
        return OAL_FAIL;
    }

    OAM_WARNING_LOG1(0, OAM_SF_PWR, "wlan_pm_wakeup_dev succ, retry times [%d]", i);

    OAL_INIT_COMPLETION(&pst_wlan_pm->st_close_bcpu_done);

    ret =  oal_channel_send_msg(pst_wlan_pm->pst_channel, H2D_MSG_PM_BCPU_OFF);
    if (ret == OAL_SUCC) {
        /* 等待device执行命令 */
        ul_ret = oal_wait_for_completion_timeout(
            &pst_wlan_pm->st_close_bcpu_done, (oal_uint32)OAL_MSECS_TO_JIFFIES(WLAN_POWEROFF_ACK_WAIT_TIMEOUT));
        if (ul_ret == 0) {
            OAM_ERROR_LOG0(0, OAM_SF_PWR, "wlan_pm_shutdown_bcpu_cmd wait device ACK timeout !");
            hcc_tx_transfer_unlock(hcc_get_default_handler());
            return OAL_FAIL;
        }
    } else {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "fail to send H2D_MSG_PM_BCPU_OFF");
        hcc_tx_transfer_unlock(hcc_get_default_handler());
        return  OAL_FAIL;
    }

    hcc_tx_transfer_unlock(hcc_get_default_handler());

    return OAL_SUCC;
}


oal_int32 wlan_pm_host_info_print(struct wlan_pm_s *pst_wlan_pm, char* buf, oal_int32 buf_len)
{
    oal_int32 ret = 0;

    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "----------wlan_pm_host_info_print begin-----------\n");
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "power on:%ld, enable:%ld,g_wlan_pm_switch:%d\n",
                      pst_wlan_pm->ul_wlan_power_state, pst_wlan_pm->ul_wlan_pm_enable, g_wlan_pm_switch);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "dev state:%ld, sleep stage:%d\n", pst_wlan_pm->ul_wlan_dev_state, pst_wlan_pm->ul_slpack);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "open:%d,close:%d\n", pst_wlan_pm->ul_open_cnt, pst_wlan_pm->ul_close_cnt);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "sdio suspend:%d,sdio resume:%d\n",
                      pst_wlan_pm->pst_channel->ul_sdio_suspend, pst_wlan_pm->pst_channel->ul_sdio_resume);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "gpio_intr[no.%lu]:%llu\n",
                      pst_wlan_pm->pst_channel->ul_wlan_irq, pst_wlan_pm->pst_channel->gpio_int_count);

#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "data_intr:%llu\n", pst_wlan_pm->pst_channel->data_int_count);
#endif

    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "wakeup_intr:%llu\n", pst_wlan_pm->pst_channel->wakeup_int_count);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "D2H_MSG_WAKEUP_SUCC:%d\n", pst_wlan_pm->pst_channel->msg[D2H_MSG_WAKEUP_SUCC].count);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "D2H_MSG_ALLOW_SLEEP:%d\n", pst_wlan_pm->pst_channel->msg[D2H_MSG_ALLOW_SLEEP].count);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "D2H_MSG_DISALLOW_SLEEP:%d\n", pst_wlan_pm->pst_channel->msg[D2H_MSG_DISALLOW_SLEEP].count);

    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "wakeup_dev_wait_ack:%d\n", oal_atomic_read(&g_wakeup_dev_wait_ack));
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "wakeup_succ:%d\n", pst_wlan_pm->ul_wakeup_succ);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "wakeup_dev_ack:%d\n", pst_wlan_pm->ul_wakeup_dev_ack);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "wakeup_done_callback:%d\n", pst_wlan_pm->ul_wakeup_done_callback);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "wakeup_succ_work_submit:%d\n", pst_wlan_pm->ul_wakeup_succ_work_submit);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "wakeup_fail_wait_sdio:%d\n", pst_wlan_pm->ul_wakeup_fail_wait_sdio);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "wakeup_fail_timeout:%d\n", pst_wlan_pm->ul_wakeup_fail_timeout);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "wakeup_fail_set_reg:%d\n", pst_wlan_pm->ul_wakeup_fail_set_reg);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "wakeup_fail_submit_work:%d\n", pst_wlan_pm->ul_wakeup_fail_submit_work);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "sleep_succ:%d\n", pst_wlan_pm->ul_sleep_succ);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "sleep feed wdg:%d\n", pst_wlan_pm->ul_sleep_feed_wdg_cnt);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "sleep_fail_request:%d\n", pst_wlan_pm->ul_sleep_fail_request);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "sleep_fail_set_reg:%d\n", pst_wlan_pm->ul_sleep_fail_set_reg);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "sleep_fail_wait_timeout:%d\n", pst_wlan_pm->ul_sleep_fail_wait_timeout);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "sleep_fail_forbid:%d\n", pst_wlan_pm->ul_sleep_fail_forbid);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "sleep_work_submit:%d\n", pst_wlan_pm->ul_sleep_work_submit);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "wklock_cnt:%lu\n \n", pst_wlan_pm->pst_channel->st_sdio_wakelock.lock_count);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                      "----------wlan_pm_host_info_print end-----------\n");
    if (ret < EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "wlan_pm_host_info_print::snprintf_s failed!");
        return ret;
    }
    return ret;
}

void wlan_pm_dump_device_info(void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();

    oal_channel_send_msg(pst_wlan_pm->pst_channel, H2D_MSG_PM_DEBUG);
}

#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
oal_void wlan_pm_debug_sleep(void)
{
    struct wlan_pm_s   *pst_wlan_pm     = wlan_pm_get_drv();

    oal_sdio_sleep_dev(pst_wlan_pm->pst_channel);

    pst_wlan_pm->ul_wlan_dev_state = HOST_ALLOW_TO_SLEEP;

    return  ;
}

oal_void wlan_pm_debug_wakeup(void)
{
    struct wlan_pm_s   *pst_wlan_pm     = wlan_pm_get_drv();

    oal_sdio_wakeup_dev(pst_wlan_pm->pst_channel);

    pst_wlan_pm->ul_wlan_dev_state = HOST_DISALLOW_TO_SLEEP;

    return  ;
}
#endif

oal_void wlan_pm_debug_wake_lock(void)
{
    struct wlan_pm_s   *pst_wlan_pm     = wlan_pm_get_drv();

    oal_channel_wake_lock(pst_wlan_pm->pst_channel);
    OAL_IO_PRINT("wlan_pm_debug_wake_lock:wklock_cnt = %lu\r\n", pst_wlan_pm->pst_channel->st_sdio_wakelock.lock_count);

    return  ;
}

oal_void wlan_pm_debug_wake_unlock(void)
{
    struct wlan_pm_s   *pst_wlan_pm     = wlan_pm_get_drv();

    oal_channel_wake_unlock(pst_wlan_pm->pst_channel);
    OAL_IO_PRINT(
        "wlan_pm_debug_wake_unlock:wklock_cnt = %lu\r\n", pst_wlan_pm->pst_channel->st_sdio_wakelock.lock_count);

    return  ;
}

void wlan_pm_dump_host_gpio_state(void)
{
    int32_t  state;
    int32_t  *iocg_ctrl_reg = NULL;
    uint32_t reg;
    BOARD_INFO *pst_board = get_board_info();
    state = gpio_get_value(pst_board->wlan_host_to_device);
    OAL_IO_PRINT("gpio host wakeup dev state: %d, gpio_num:%d\n", state, pst_board->wlan_host_to_device);
    state = gpio_get_value(pst_board->power_on_enable);
    OAL_IO_PRINT("gpio power_on state: %d, gpio_num:%d\n", state, pst_board->power_on_enable);
    state = gpio_get_value(pst_board->wlan_en);
    OAL_IO_PRINT("gpio wlan_rst state: %d, gpio_num:%d\n", state, pst_board->wlan_en);
    // sdio
    iocg_ctrl_reg = (int32_t *)oal_ioremap(IOCFG_BASE_GPIO_FOR_SD_CLK, GPIO_SDIO_REMAP_SIZE);
    reg = oal_readl(iocg_ctrl_reg + GPIO_SDIO_OFFSET * (GPIO_SDIO_CLK - GPIO_CURRENT_NUMBER_FOR_SD_CLK));
    OAL_IO_PRINT("GPIO_SDIO_CLK state: 0x%x\n", reg & GPIO_MOD_MASK);
    reg = oal_readl(iocg_ctrl_reg + GPIO_SDIO_OFFSET * (GPIO_SDIO_CMD - GPIO_CURRENT_NUMBER_FOR_SD_CLK));
    OAL_IO_PRINT("GPIO_SDIO_CMD state: 0x%x\n", reg & GPIO_MOD_MASK);
    reg = oal_readl(iocg_ctrl_reg + GPIO_SDIO_OFFSET * (GPIO_SDIO_DAT0 - GPIO_CURRENT_NUMBER_FOR_SD_CLK));
    OAL_IO_PRINT("GPIO_SDIO_DAT0 state: 0x%x\n", reg & GPIO_MOD_MASK);
    reg = oal_readl(iocg_ctrl_reg + GPIO_SDIO_OFFSET * (GPIO_SDIO_DAT1 - GPIO_CURRENT_NUMBER_FOR_SD_CLK));
    OAL_IO_PRINT("GPIO_SDIO_DAT1 state: 0x%x\n", reg & GPIO_MOD_MASK);
    reg = oal_readl(iocg_ctrl_reg + GPIO_SDIO_OFFSET * (GPIO_SDIO_DAT2 - GPIO_CURRENT_NUMBER_FOR_SD_CLK));
    OAL_IO_PRINT("GPIO_SDIO_DAT2 state: 0x%x\n", reg & GPIO_MOD_MASK);
    reg = oal_readl(iocg_ctrl_reg + GPIO_SDIO_OFFSET * (GPIO_SDIO_DAT3 - GPIO_CURRENT_NUMBER_FOR_SD_CLK));
    OAL_IO_PRINT("GPIO_SDIO_DAT3 state: 0x%x\n", reg & GPIO_MOD_MASK);
    oal_iounmap(iocg_ctrl_reg);
}

#endif

oal_void wlan_pm_state_set(struct wlan_pm_s  *pst_wlan_pm, oal_ulong ul_state);


oal_uint32 wlan_pm_enable(oal_void)
{
#ifndef _PRE_FEATURE_NO_GPIO
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();

    if (!g_wlan_pm_switch) {
        return OAL_SUCC;
    }

    if (pst_wlan_pm->ul_wlan_pm_enable == OAL_TRUE) {
        OAM_WARNING_LOG0(0, OAM_SF_PWR, "wlan_pm_enable already enabled!");
        return OAL_SUCC;
    }

    pst_wlan_pm->ul_wlan_pm_enable = OAL_TRUE;

    OAM_INFO_LOG0(0, OAM_SF_PWR, "wlan_pm_enable OAL_SUCC!");
#endif
    return OAL_SUCC;
}
EXPORT_SYMBOL_GPL(wlan_pm_enable);


oal_uint32 wlan_pm_disable_check_wakeup(oal_int32 flag)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();

    hcc_tx_transfer_lock(hcc_get_default_handler());

    if (pst_wlan_pm->ul_wlan_pm_enable == OAL_FALSE) {
        OAM_WARNING_LOG0(0, OAM_SF_PWR, "wlan_pm_disable already disabled!");
        hcc_tx_transfer_unlock(hcc_get_default_handler());
        return OAL_SUCC;
    }

    if (flag == OAL_TRUE) {
        if (wlan_pm_wakeup_dev() != OAL_SUCC) {
            OAM_WARNING_LOG0(0, OAM_SF_PWR, "pm wake up dev fail!");
        }
    }

    pst_wlan_pm->ul_wlan_pm_enable = OAL_FALSE;

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
    if (g_pst_alg_process_func.p_auto_freq_set_lock_mod_func != OAL_PTR_NULL) {
        g_pst_alg_process_func.p_auto_freq_set_lock_mod_func(OAL_FALSE);
    } else {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "wlan_pm_wdg_timeout:NO p_auto_freq_set_lock_mod_func registered");
    }
#endif

    hcc_tx_transfer_unlock(hcc_get_default_handler());

    oal_cancel_work_sync(&pst_wlan_pm->st_wakeup_work);
    oal_cancel_work_sync(&pst_wlan_pm->st_sleep_work);

    OAM_INFO_LOG0(0, OAM_SF_PWR, "wlan_pm_disable OAL_SUCC!");

    return OAL_SUCC;
}
EXPORT_SYMBOL_GPL(wlan_pm_disable_check_wakeup);

oal_uint32 wlan_pm_disable(oal_void)
{
    return wlan_pm_disable_check_wakeup(OAL_TRUE);
}
EXPORT_SYMBOL_GPL(wlan_pm_disable);


/*
 * 函 数 名  : wlan_pm_idle_sleep_vote
 * 功能描述  : wlan投票是否允许kirin进入32k idle模式
 * 输入参数  : TRUE:允许，FALSE:不允许
 * 返 回 值  : 初始化返回值，成功或失败原因
 */
void wlan_pm_idle_sleep_vote(uint8_t uc_allow)
{
#if ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && (_PRE_LINUX_PLATFORM == MIAMI_C60))
#if defined(CONFIG_HISI_IDLE_SLEEP)
    if (uc_allow == WLAN_ALLOW_IDLESLEEP) {
        hisi_idle_sleep_vote(ID_WIFI, 0);
    } else {
        hisi_idle_sleep_vote(ID_WIFI, 1);
    }
#elif defined(CONFIG_LPCPU_IDLE_SLEEP)
    if (uc_allow == WLAN_ALLOW_IDLESLEEP) {
        lpcpu_idle_sleep_vote(ID_WIFI, 0);
    } else {
        lpcpu_idle_sleep_vote(ID_WIFI, 1);
    }
#endif
#else
    (void)uc_allow;
#endif
}


int32_t host_power_on_cali(void)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION))
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    if (g_uc_netdev_is_open != OAL_TRUE) {
        if (g_pst_custom_process_func.p_custom_cali_func == OAL_PTR_NULL) {
            OAM_ERROR_LOG0(0, OAM_SF_PWR, "wlan_pm_open::NO g_pst_custom_process_func registered");
            wlan_pm_idle_sleep_vote(WLAN_ALLOW_IDLESLEEP);
            chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV,
                                 CHR_WIFI_DRV_EVENT_PLAT, CHR_WIFI_DRV_ERROR_POWER_ON_NO_CUSTOM_CALL);
            return OAL_FAIL;
        }
        if (g_pst_custom_process_func.p_custom_cali_func() != OAL_SUCC) {
            OAM_ERROR_LOG0(0, OAM_SF_PWR, "wlan_pm_open::custom_cali failed !!!");
        }
    }
#endif
#endif
    return OAL_SUCC;
}

OAL_STATIC oal_int32 wlan_pm_open_check(struct wlan_pm_s *pst_wlan_pm, struct pm_drv_data *pm_data)
{
    if ((pm_data == NULL) || (pst_wlan_pm == OAL_PTR_NULL)) {
        OAM_ERROR_LOG2(0, OAM_SF_PWR, "wlan_pm_open::pm_data[%p] or pst_wlan_pm[%p] is NULL!",
                       (uintptr_t)pm_data, (uintptr_t)pst_wlan_pm);
        return OAL_FAIL;
    }
    if (pst_wlan_pm->ul_wlan_power_state == POWER_STATE_OPEN) {
        oal_mutex_unlock(&pm_data->host_mutex);
        OAM_WARNING_LOG0(0, OAM_SF_PWR, "wlan_pm_open::aleady opened");
#ifdef _PRE_FEATURE_NO_GPIO
        return OAL_SUCC;
#else
        return OAL_ERR_CODE_ALREADY_OPEN;
#endif
    }
    return OAL_SUCC;
}


oal_int32 wlan_pm_open(oal_void)
{
    oal_int32           ret;
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();
    struct pm_drv_data  *pm_data     = pm_get_drvdata();

    PS_PRINT_INFO("wlan_pm_open caller\r\n");

    ret = wlan_pm_open_check(pst_wlan_pm, pm_data);
    if (ret != OAL_SUCC) {
        return ret;
    }

    oal_mutex_lock(&pm_data->host_mutex);
    if (!pst_wlan_pm->pst_channel->st_sdio_wakelock.lock_count) {
        /* make sure open only lock once */
        oal_channel_wake_lock(pst_wlan_pm->pst_channel);
        wlan_pm_idle_sleep_vote(WLAN_DISALLOW_IDLESLEEP);
        OAM_WARNING_LOG0(0, OAM_SF_PWR, "wlan_pm_idle_sleep_vote DISALLOW::hisi_idle_sleep_vote ID_WIFI 1!");
    }
    OAM_INFO_LOG1(
        0, OAM_SF_PWR, "wlan_pm_open::get wakelock %lu!", pst_wlan_pm->pst_channel->st_sdio_wakelock.lock_count);

    pst_wlan_pm->ul_open_cnt++;

    if (wlan_power_on() != OAL_SUCC) {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "wlan_pm_open::wlan_power_on fail!");
        pst_wlan_pm->ul_wlan_power_state = POWER_STATE_SHUTDOWN;
        oal_channel_wake_unlock(pst_wlan_pm->pst_channel);
        wlan_pm_idle_sleep_vote(WLAN_ALLOW_IDLESLEEP);
        oal_mutex_unlock(&pm_data->host_mutex);
        DECLARE_DFT_TRACE_KEY_INFO("wlan_power_on_fail", OAL_DFT_TRACE_FAIL);
        return OAL_FAIL;
    }

    OAM_WARNING_LOG1(0, OAM_SF_PWR, "wlan_pm_open::wlan_pm_open OAL_SUCC!! g_wlan_pm_switch = %d", g_wlan_pm_switch);
    DECLARE_DFT_TRACE_KEY_INFO("wlan_open_succ", OAL_DFT_TRACE_SUCC);

    OAM_WARNING_LOG0(0, OAM_SF_PWR, "hcc_enable\n");
    hcc_enable(hcc_get_default_handler(), OAL_TRUE);

    if (host_power_on_cali() != OAL_SUCC) {
        wlan_pm_idle_sleep_vote(WLAN_ALLOW_IDLESLEEP);
        oal_mutex_unlock(&pm_data->host_mutex);
        return OAL_FAIL;
    }

    PS_PRINT_INFO("wlan_pm_open OAL_SUCC!!\n");

    wlan_pm_enable();

    /* 将timeout值恢复为默认值，并启动定时器 */
    wlan_pm_set_timeout(WLAN_SLEEP_DEFAULT_CHECK_CNT);

    oal_mutex_unlock(&pm_data->host_mutex);
    return OAL_SUCC;
}


EXPORT_SYMBOL_GPL(wlan_pm_open);


int isAsic(void);
#ifdef _PRE_FEATURE_NO_GPIO
oal_uint32 wlan_pm_close(oal_void)
{
    OAL_IO_PRINT("[plat_pm]wifi need always on,do not close!!\n");
    return OAL_SUCC;
}
#else
oal_uint32 wlan_pm_close(oal_void)
{
    struct wlan_pm_s *pst_wlan_pm = wlan_pm_get_drv();
    struct pm_drv_data *pm_data = pm_get_drvdata();

    if (pm_data == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "wlan_pm_close,pm_data is NULL!");
        return OAL_FAIL;
    }

    if (pst_wlan_pm == OAL_PTR_NULL) {
        return OAL_FAIL;
    }

    if (!pst_wlan_pm->ul_apmode_allow_pm_flag) {
        OAM_WARNING_LOG0(0, OAM_SF_PWR, "wlan_pm_close,AP mode,do not shutdown power.");
        return OAL_SUCC;
    }

    oal_mutex_lock(&pm_data->host_mutex);
    pst_wlan_pm->ul_close_cnt++;

    if (pst_wlan_pm->ul_wlan_power_state == POWER_STATE_SHUTDOWN) {
        oal_mutex_unlock(&pm_data->host_mutex);
        return OAL_ERR_CODE_ALREADY_CLOSE;
    }

    wlan_pm_disable();

    wlan_pm_stop_wdg(pst_wlan_pm);
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    wlan_pm_info_clean();

    /* mask rx sdio data interrupt */
    oal_sdio_func1_int_mask(pst_wlan_pm->pst_channel, HISDIO_FUNC1_INT_DREADY);

    if (wlan_power_off() != OAL_SUCC) {
        OAL_IO_PRINT("wlan_power_off FAIL!\n");

        OAM_ERROR_LOG0(0, OAM_SF_PWR, "wlan_power_off FAIL!\n");
        oal_mutex_unlock(&pm_data->host_mutex);
        DECLARE_DFT_TRACE_KEY_INFO("wlan_power_off_fail", OAL_DFT_TRACE_FAIL);
        return OAL_FAIL;
    }

    pst_wlan_pm->ul_wlan_power_state = POWER_STATE_SHUTDOWN;

    /* unmask rx sdio data interrupt */
    oal_sdio_func1_int_unmask(pst_wlan_pm->pst_channel, HISDIO_FUNC1_INT_DREADY);

    oal_channel_wake_unlock(pst_wlan_pm->pst_channel);

    OAM_WARNING_LOG1(
        0, OAM_SF_PWR, "wlan_pm_close release wakelock %lu!\n", pst_wlan_pm->pst_channel->st_sdio_wakelock.lock_count);
    oal_sdio_wakelocks_release_detect(pst_wlan_pm->pst_channel);
#endif
    oal_mutex_unlock(&pm_data->host_mutex);

    hcc_dev_flowctrl_on(hcc_get_default_handler(), 0);
    wlan_pm_idle_sleep_vote(WLAN_ALLOW_IDLESLEEP);
    OAM_WARNING_LOG0(0, OAM_SF_PWR, "wlan_pm_close succ!\n");
    DECLARE_DFT_TRACE_KEY_INFO("wlan_close_succ", OAL_DFT_TRACE_SUCC);

    return OAL_SUCC;
}
#endif
EXPORT_SYMBOL_GPL(wlan_pm_close);

void wlan_pm_dump_host_info_print(struct wlan_pm_s    *pst_wlan_pm)
{
    OAL_IO_PRINT("wakeup_succ:%d\n", pst_wlan_pm->ul_wakeup_succ);
    OAL_IO_PRINT("wakeup_dev_ack:%d\n", pst_wlan_pm->ul_wakeup_dev_ack);
    OAL_IO_PRINT("wakeup_done_callback:%d\n", pst_wlan_pm->ul_wakeup_done_callback);
    OAL_IO_PRINT("wakeup_succ_work_submit:%d\n", pst_wlan_pm->ul_wakeup_succ_work_submit);
    OAL_IO_PRINT("wakeup_fail_wait_sdio:%d\n", pst_wlan_pm->ul_wakeup_fail_wait_sdio);
    OAL_IO_PRINT("wakeup_fail_timeout:%d\n", pst_wlan_pm->ul_wakeup_fail_timeout);
    OAL_IO_PRINT("wakeup_fail_set_reg:%d\n", pst_wlan_pm->ul_wakeup_fail_set_reg);
    OAL_IO_PRINT("wakeup_fail_submit_work:%d\n", pst_wlan_pm->ul_wakeup_fail_submit_work);
    OAL_IO_PRINT("sleep_succ:%d\n", pst_wlan_pm->ul_sleep_succ);
    OAL_IO_PRINT("sleep feed wdg:%d\n", pst_wlan_pm->ul_sleep_feed_wdg_cnt);
    OAL_IO_PRINT("sleep_fail_request:%d\n", pst_wlan_pm->ul_sleep_fail_request);
    OAL_IO_PRINT("sleep_fail_set_reg:%d\n", pst_wlan_pm->ul_sleep_fail_set_reg);
    OAL_IO_PRINT("sleep_fail_wait_timeout:%d\n", pst_wlan_pm->ul_sleep_fail_wait_timeout);
    OAL_IO_PRINT("sleep_fail_forbid:%d\n", pst_wlan_pm->ul_sleep_fail_forbid);
    OAL_IO_PRINT("sleep_work_submit:%d\n", pst_wlan_pm->ul_sleep_work_submit);
    OAL_IO_PRINT("wklock_cnt:%lu\n \n", pst_wlan_pm->pst_channel->st_sdio_wakelock.lock_count);
}


void wlan_pm_dump_host_info(void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();

    if (pst_wlan_pm == NULL) {
        return;
    }
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    OAL_IO_PRINT("----------wlan_pm_dump_host_info begin-----------\n");
    OAL_IO_PRINT("power on:%ld, enable:%ld,g_wlan_pm_switch:%d\n", \
        pst_wlan_pm->ul_wlan_power_state, pst_wlan_pm->ul_wlan_pm_enable, g_wlan_pm_switch);
    OAL_IO_PRINT("dev state:%ld, sleep stage:%d\n", pst_wlan_pm->ul_wlan_dev_state, pst_wlan_pm->ul_slpack);
    OAL_IO_PRINT("open:%d,close:%d\n", pst_wlan_pm->ul_open_cnt, pst_wlan_pm->ul_close_cnt);
    OAL_IO_PRINT("sdio suspend:%d,sdio resume:%d\n", \
        pst_wlan_pm->pst_channel->ul_sdio_suspend, pst_wlan_pm->pst_channel->ul_sdio_resume);
    OAL_IO_PRINT("gpio_intr[no.%lu]:%llu\n", \
        pst_wlan_pm->pst_channel->ul_wlan_irq, pst_wlan_pm->pst_channel->gpio_int_count);
    OAL_IO_PRINT("data_intr:%llu\n", pst_wlan_pm->pst_channel->data_int_count);
    OAL_IO_PRINT("sdio_intr:%llu\n", pst_wlan_pm->pst_channel->sdio_int_count);
    OAL_IO_PRINT("sdio_intr_finish:%llu\n", pst_wlan_pm->pst_channel->data_int_finish_count);

    OAL_IO_PRINT("sdio_no_int_count:%u\n", pst_wlan_pm->pst_channel->func1_stat.func1_no_int_count);
    OAL_IO_PRINT("sdio_err_int_count:%u\n", pst_wlan_pm->pst_channel->func1_stat.func1_err_int_count);
    OAL_IO_PRINT("sdio_msg_int_count:%u\n", pst_wlan_pm->pst_channel->func1_stat.func1_msg_int_count);
    OAL_IO_PRINT("sdio_data_int_count:%u\n", pst_wlan_pm->pst_channel->func1_stat.func1_data_int_count);
    OAL_IO_PRINT("sdio_unknow_int_count:%u\n", pst_wlan_pm->pst_channel->func1_stat.func1_unknow_int_count);

    OAL_IO_PRINT("last gpio irq enter time::%u\n", pst_wlan_pm->pst_channel->ul_last_step_time[1]);
    OAL_IO_PRINT("last sdio do isr enter time::%u\n", pst_wlan_pm->pst_channel->ul_last_step_time[2]);
    OAL_IO_PRINT("last msg handle time::%u\n", pst_wlan_pm->pst_channel->ul_last_step_time[3]);
    OAL_IO_PRINT("last data handle time::%u\n", pst_wlan_pm->pst_channel->ul_last_step_time[4]);
    OAL_IO_PRINT("last sdio isr finish time::%u\n", pst_wlan_pm->pst_channel->ul_last_step_time[5]);

    OAL_IO_PRINT("wakeup_intr:%llu\n", pst_wlan_pm->pst_channel->wakeup_int_count);
    OAL_IO_PRINT("D2H_MSG_WAKEUP_SUCC:%d\n", pst_wlan_pm->pst_channel->msg[D2H_MSG_WAKEUP_SUCC].count);
    OAL_IO_PRINT("D2H_MSG_ALLOW_SLEEP:%d\n", pst_wlan_pm->pst_channel->msg[D2H_MSG_ALLOW_SLEEP].count);
    OAL_IO_PRINT("D2H_MSG_DISALLOW_SLEEP:%d\n", pst_wlan_pm->pst_channel->msg[D2H_MSG_DISALLOW_SLEEP].count);
    OAL_IO_PRINT("D2H_MSG_PANIC:%d\n", pst_wlan_pm->pst_channel->msg[D2H_MSG_DEVICE_PANIC].count);

    OAL_IO_PRINT("wakeup_dev_wait_ack:%d\n", oal_atomic_read(&g_wakeup_dev_wait_ack));

    wlan_pm_dump_host_info_print(pst_wlan_pm);
    OAL_IO_PRINT("----------wlan_pm_dump_host_info end-----------\n");
#endif
}


oal_ulong wlan_pm_init_dev(void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();
    oal_ulong             ul_flag;
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    oal_long              ret;
#endif

    if (pst_wlan_pm == OAL_PTR_NULL) {
        return OAL_FAIL;
    }
    OAM_INFO_LOG0(0, OAM_SF_PWR, "wlan_pm_init_dev!\n");

    board_set_wlan_h2d_pm_state(WLAN_PM_WKUPDEV_LEVEL);

    oal_spin_lock_irq_save(&pst_wlan_pm->pst_channel->st_pm_state_lock, &ul_flag);

    pst_wlan_pm->ul_wlan_dev_state              = HOST_DISALLOW_TO_SLEEP;
    pst_wlan_pm->ul_slpack                      = SLPACK_DEV_DISALLOW;

    oal_spin_unlock_irq_restore(&pst_wlan_pm->pst_channel->st_pm_state_lock, &ul_flag);

#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    /* wait for sdio wakeup */
    ret = oal_down_timeout(&chan_wake_sema, 6 * OAL_TIME_HZ);
    if (ret == -ETIME) {
        oal_channel_wake_unlock(pst_wlan_pm->pst_channel);
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_PM, CHR_PLAT_DRV_ERROR_PM_SDIO_NO_READY);
        return OAL_FAIL;
    }
    oal_up(&chan_wake_sema);
#endif

    return OAL_SUCC;
}


oal_int32 wlan_pm_chan_reinit(void)
{
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    oal_int32    ret;
    ret = oal_sdio_reinit();

    return ret;
#else
    return OAL_SUCC;
#endif
}

oal_ulong wlan_pm_wakeup_dev_request_ack(struct wlan_pm_s *wlan_pm)
{
    int32_t                ret;

    if (wlan_pm == OAL_PTR_NULL) {
        return OAL_FAIL;
    }

#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    oal_wlan_gpio_intr_enable(wlan_pm->pst_channel, OAL_FALSE);
#endif
    wlan_pm->ul_wakeup_gpio_up_cnt++;
    OAM_INFO_LOG1(0, OAM_SF_PWR, "[pm]gpio high level cnt::%u!", wlan_pm->ul_wakeup_gpio_up_cnt);
    /* 先拉低GPIO管脚，延迟1ms后，再拉高GPIO管脚 */
    board_set_wlan_h2d_pm_state(WLAN_PM_SLPREQ_LEVEL);
    mdelay(1);
    board_set_wlan_h2d_pm_state(WLAN_PM_WKUPDEV_LEVEL);

    oal_atomic_set(&g_wakeup_dev_wait_ack, 1);

#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    oal_wlan_gpio_intr_enable(wlan_pm->pst_channel, OAL_TRUE);
#endif
    wlan_pm_state_set(wlan_pm, HOST_DISALLOW_TO_SLEEP);

    // chan reinit
    OAM_INFO_LOG0(0, OAM_SF_PWR, "[pm]channel reinit!");
    /* waitting for dev channel stable */
    mdelay(WLAN_WAIT_SDIO_POWER_UP);
    ret = wlan_pm_chan_reinit();
    if (ret < 0) {
        wlan_pm->ul_wakeup_fail_wait_sdio++;
        wlan_pm_state_set(wlan_pm, HOST_ALLOW_TO_SLEEP);
        oal_channel_wake_unlock(wlan_pm->pst_channel);
        PS_PRINT_ERR("sdio reinit failed, ret:%d!\n", ret);
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "[pm]sdio reinit failed!,  ret:%d!\n", ret);
        return OAL_FAIL;
    }
    OAM_INFO_LOG0(0, OAM_SF_PWR, "[pm]chan reinit done, waitting for dev ack!");

    ret =  oal_wait_for_completion_timeout(&wlan_pm->st_wakeup_done,
                                           (oal_uint32)OAL_MSECS_TO_JIFFIES(WLAN_WAKUP_MSG_WAIT_TIMEOUT));
    if (ret == 0) {
        wlan_pm->ul_wakeup_fail_timeout++;
        OAM_ERROR_LOG1(0, OAM_SF_PWR, "[pm]wlan_pm_wakeup_dev wait device complete fail,wait time %d ms!",
                       WLAN_WAKUP_MSG_WAIT_TIMEOUT);
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_PM, CHR_PLAT_DRV_ERROR_BFG_WKUP_DEV);
        return OAL_FAIL;
    }

    wlan_pm->ul_wakeup_succ++;
    DECLARE_DFT_TRACE_KEY_INFO("wlan_wakeup_succ", OAL_DFT_TRACE_SUCC);
    wlan_pm->ul_wdg_timeout_curr_cnt = 0;
    wlan_pm->ul_packet_cnt           = 0;
    OAM_INFO_LOG0(0, OAM_SF_PWR, "[pm]host wakeup device succ");

    return OAL_SUCC;
}

void wlan_pm_wakeup_fail(struct wlan_pm_s *wlan_pm)
{
    board_set_wlan_h2d_pm_state(WLAN_PM_SLPREQ_LEVEL);
    wlan_pm_state_set(wlan_pm, HOST_EXCEPTION);

    oal_channel_wake_unlock(wlan_pm->pst_channel);
    DECLARE_DFT_TRACE_KEY_INFO("wlan_wakeup_fail", OAL_DFT_TRACE_FAIL);

    oal_msleep(WLAN_WAIT_FOR_ENTER_DFR);

    /* pm唤醒失败，启动dfr流程 */
    OAM_ERROR_LOG0(0, OAM_SF_PWR, "[pm]Now ready to enter DFR process after wlan_wakeup_fail!");
    PS_PRINT_ERR("[pm]wlan wakeup fail\n");
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    oal_wakeup_exception();
#endif
    chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                         CHR_PLT_DRV_EVENT_PM, CHR_PLAT_DRV_ERROR_WIFI_WKUP_DEV);
    return;
}


oal_ulong wlan_pm_wakeup_dev(oal_void)
{
    struct wlan_pm_s       *wlan_pm = wlan_pm_get_drv();
    oal_time_t_stru        wkuptime;
    oal_int64              delta_time;

    if (wlan_pm == OAL_PTR_NULL) {
        PS_PRINT_ERR("OAL_PTR_NULL!\n");
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "[pm]OAL_PTR_NULL!!");
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_PM, CHR_PLAT_DRV_ERROR_PM_WKUP_NON_BUS);
        return OAL_FAIL;
    }
    if (wlan_pm->ul_wlan_power_state == POWER_STATE_SHUTDOWN) {
        return OAL_EFAIL;
    }

    if (wlan_pm->ul_wlan_dev_state == HOST_DISALLOW_TO_SLEEP) {
        return OAL_SUCC;
    } else if (wlan_pm->ul_wlan_dev_state == HOST_EXCEPTION) {
        return OAL_EFAIL;
    }
    oal_channel_wake_lock(wlan_pm->pst_channel);

    OAL_INIT_COMPLETION(&wlan_pm->st_wakeup_done);
    OAM_INFO_LOG0(0, OAM_SF_PWR, "[pm]wake up device!");
    wlan_pm_idle_sleep_vote(WLAN_DISALLOW_IDLESLEEP);
    /* wait for sdio wakeup */
    if ((oal_down_timeout(&chan_wake_sema, 6 * OAL_TIME_HZ)) == -ETIME) {
        wlan_pm->ul_wakeup_fail_wait_sdio++;
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "[pm]sdio controller is not ready!");

        oal_channel_wake_unlock(wlan_pm->pst_channel);
        goto wakeup_fail;
    }
    oal_up(&chan_wake_sema);
    /* 保证睡眠唤醒间的最小时序约束500us */
    wkuptime = oal_ktime_get();
    delta_time = (oal_uint64)oal_ktime_to_us(oal_ktime_sub(wkuptime, wlan_pm->ul_slp_time_us));
    if ((delta_time < 500) && (delta_time > 0)) {
        udelay(500 - delta_time);
    }

    if (wlan_pm_wakeup_dev_request_ack(wlan_pm) != OAL_SUCC) {
        goto wakeup_fail;
    }

    wlan_pm_feed_wdg();
    start_heartbeat();

    return OAL_SUCC;
wakeup_fail:
    wlan_pm_wakeup_fail(wlan_pm);
    return OAL_FAIL;
}

oal_void wlan_pm_wakeup_dev_ack(oal_void)
{
    struct wlan_pm_s    *pst_wlan_pm = OAL_PTR_NULL;

    if (oal_atomic_read(&g_wakeup_dev_wait_ack)) {
        pst_wlan_pm = wlan_pm_get_drv();
        if (pst_wlan_pm == OAL_PTR_NULL) {
            return ;
        }

        pst_wlan_pm->ul_wakeup_dev_ack++;

        OAL_COMPLETE(&pst_wlan_pm->st_wakeup_done);

        oal_atomic_set(&g_wakeup_dev_wait_ack, 0);
    }

    return;
}


oal_ulong wlan_pm_wakeup_host(void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();

    OAM_INFO_LOG0(0, OAM_SF_PWR, "[pm]dev wake up host!");

    OAL_BUG_ON(!pst_wlan_pm);

    oal_channel_wake_lock(pst_wlan_pm->pst_channel);
    wlan_pm_idle_sleep_vote(WLAN_DISALLOW_IDLESLEEP);
    if (wlan_pm_work_submit(pst_wlan_pm, &pst_wlan_pm->st_wakeup_work) != 0) {
        pst_wlan_pm->ul_wakeup_fail_submit_work++;

        oal_channel_wake_unlock(pst_wlan_pm->pst_channel);
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "wlan_pm_wakeup_host submit work fail, release wakelock %lu!\n",
            pst_wlan_pm->pst_channel->st_sdio_wakelock.lock_count);
    } else {
        pst_wlan_pm->ul_wakeup_succ_work_submit++;
    }

    return OAL_SUCC;
}


oal_void wlan_pm_ack_handle(oal_int32 ack, void *data)
{
    struct wlan_pm_s *pst_wlan_pm = (struct wlan_pm_s *)data;
    oal_uint32 ul_tmp;
    oal_spin_lock_stru          pm_spin_lock;
    oal_ulong                   ul_pm_flags;

    oal_spin_lock_init(&pm_spin_lock);

    oal_spin_lock_irq_save(&pm_spin_lock, &ul_pm_flags);
    if (pst_wlan_pm->ul_waitting_for_dev_ack) {
        ul_tmp = --pst_wlan_pm->ul_waitting_for_dev_ack;
    } else {
        oal_spin_unlock_irq_restore(&pm_spin_lock, &ul_pm_flags);
        OAM_INFO_LOG0(0, OAM_SF_PWR, "[pm]not waitting for dev ack !\n");
        return;
    }
    oal_spin_unlock_irq_restore(&pm_spin_lock, &ul_pm_flags);
    if (ul_tmp == 0) {
        pst_wlan_pm->ul_slpack = ack;
        OAL_COMPLETE(&pst_wlan_pm->st_sleep_request_ack);
    } else {
        OAM_INFO_LOG1(0, OAM_SF_PWR, "[pm]not pairing ack :: %d !\n", pst_wlan_pm->ul_waitting_for_dev_ack);
    }
}


oal_int32 wlan_pm_allow_sleep_callback(void *data)
{
    OAM_INFO_LOG0(0, OAM_SF_PWR, "[pm]wlan pm device allow sleep !\n");

    wlan_pm_ack_handle(SLPACK_DEV_ALLOW, data);

    return SUCCESS;
}


oal_int32 wlan_pm_disallow_sleep_callback(void *data)
{
    OAM_INFO_LOG0(0, OAM_SF_PWR, "[pm]wlan pm device ack disallow sleep!\n");

    wlan_pm_ack_handle(SLPACK_DEV_DISALLOW, data);

    return SUCCESS;
}


void wlan_pm_wakeup_work(oal_work_stru *pst_worker)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();
    oal_ulong l_ret;

    OAM_INFO_LOG0(0, OAM_SF_PWR, "wlan_pm_wakeup_work start!\n");

    hcc_tx_transfer_lock(hcc_get_default_handler());

    l_ret = wlan_pm_wakeup_dev();
    if (OAL_UNLIKELY(l_ret != OAL_SUCC)) {
        DECLARE_DFT_TRACE_KEY_INFO("wlan_wakeup_fail", OAL_DFT_TRACE_FAIL);
    }

    hcc_tx_transfer_unlock(hcc_get_default_handler());

    oal_channel_wake_unlock(pst_wlan_pm->pst_channel);
    DECLARE_DFT_TRACE_KEY_INFO("wlan_d2h_wakeup_succ", OAL_DFT_TRACE_SUCC);
    OAM_INFO_LOG1(0, OAM_SF_PWR, "wlan_pm_wakeup_work release wakelock %lu!\n",
        pst_wlan_pm->pst_channel->st_sdio_wakelock.lock_count);
    return;
}


oal_int32 wlan_pm_wakeup_done_callback(void *data)
{
    struct wlan_pm_s *pst_wlan_pm = (struct wlan_pm_s *)data;

    pst_wlan_pm->ul_wakeup_done_callback++;

    OAM_INFO_LOG1(0, OAM_SF_PWR, "[pm]dev ack wakeup success cnt:: %u!", pst_wlan_pm->ul_wakeup_done_callback);

    wlan_pm_wakeup_dev_ack();

    return SUCCESS;
}


oal_int32 wlan_pm_sleep_request(struct wlan_pm_s    *pst_wlan_pm)
{
    return oal_channel_send_msg(pst_wlan_pm->pst_channel, H2D_MSG_SLEEP_REQ);
}


oal_uint32 wlan_pm_set_pm_sts_exception(oal_void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();
    if (pst_wlan_pm == OAL_PTR_NULL) {
        return OAL_FAIL;
    }
    wlan_pm_state_set(pst_wlan_pm, HOST_EXCEPTION);

    return OAL_SUCC;
}

EXPORT_SYMBOL_GPL(wlan_pm_set_pm_sts_exception);


uint32_t wlan_pm_sts_check(struct wlan_pm_s *wlan_pm)
{
    uint32_t wifi_pause_pm = OAL_FALSE;

    if (wlan_pm->ul_wlan_pm_enable == OAL_FALSE) {
        OAM_WARNING_LOG0(0, OAM_SF_PWR, "[pm]pm disable");
        wlan_pm_feed_wdg();
        return OAL_FAIL;
    }

    /* 协议栈回调获取是否pause低功耗 */
    if (wlan_pm->st_wifi_srv_handler.p_wifi_srv_get_pm_pause_func) {
        wifi_pause_pm = wlan_pm->st_wifi_srv_handler.p_wifi_srv_get_pm_pause_func();
    }

    if (wifi_pause_pm == OAL_TRUE) {
        wlan_pm_feed_wdg();
        OAM_INFO_LOG0(0, OAM_SF_PWR, "[pm]scanning, pause pm");
        return OAL_FAIL;
    }

    if (wlan_pm->ul_wlan_dev_state == HOST_ALLOW_TO_SLEEP) {
        OAM_INFO_LOG0(0, OAM_SF_PWR, "[pm]sleeping,need not do again!");
        wlan_pm_feed_wdg();
        return OAL_FAIL;
    } else if (wlan_pm->ul_wlan_dev_state == HOST_EXCEPTION) {
        OAM_INFO_LOG0(0, OAM_SF_PWR, "[pm]host exception, don't sleep!");
        return OAL_FAIL;
    }

    return OAL_SUCC;
}


uint32_t wlan_pm_sleep_requset_and_ack(struct wlan_pm_s *wlan_pm)
{
    oal_spin_lock_stru  pm_spin_lock;
    oal_ulong           pm_flags;

    oal_spin_lock_init(&pm_spin_lock);
    wlan_pm->ul_slpack = SLPACK_DEV_ALLOW;

    // we may not need this anymore
    OAL_INIT_COMPLETION(&wlan_pm->st_sleep_request_ack);
    wlan_pm->ul_sleep_msg_send_cnt++;
    OAM_INFO_LOG1(0, OAM_SF_PWR, "[pm]send sleep msg!cnt::%u", wlan_pm->ul_sleep_msg_send_cnt);

    oal_spin_lock_irq_save(&pm_spin_lock, &pm_flags);
    wlan_pm->ul_waitting_for_dev_ack++;
    oal_spin_unlock_irq_restore(&pm_spin_lock, &pm_flags);
    if (wlan_pm_sleep_request(wlan_pm) != OAL_SUCC) {
        wlan_pm->ul_sleep_fail_request++;
        wlan_pm->ul_waitting_for_dev_ack--;
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "[pm]wlan_pm_sleep_request fail !\n");
        return OAL_FAIL;
    }
    stop_heartbeat();

    if (oal_wait_for_completion_timeout(&wlan_pm->st_sleep_request_ack,
                                        (oal_uint32)OAL_MSECS_TO_JIFFIES(WLAN_SLEEP_MSG_WAIT_TIMEOUT)) == 0) {
        /* 超时处理，拉低gpio通知dev流程完成，再拉高gpio告知dev睡眠失败 */
        oal_spin_lock_irq_save(&pm_spin_lock, &pm_flags);
        board_set_wlan_h2d_pm_state(WLAN_PM_SLPREQ_LEVEL);
        board_set_wlan_h2d_pm_state(WLAN_PM_WKUPDEV_LEVEL);
        oal_spin_unlock_irq_restore(&pm_spin_lock, &pm_flags);

        wlan_pm->ul_slpreq_flag = NO_SLPREQ_STATUS;
        wlan_pm->ul_sleep_fail_wait_timeout++;
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "[pm]wlan_pm_sleep_work wait completion fail cnt::%u !\n",
                         wlan_pm->ul_sleep_fail_wait_timeout);
        return OAL_FAIL;
    }

    // dev ack allow_slp normally, but ...
    if (wlan_pm->ul_slpack == SLPACK_DEV_DISALLOW) { // SLPACK_DEV_ALLOW
        wlan_pm->ul_sleep_fail_forbid++;
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "[pm]wlan_pm_sleep_work device forbid sleep %ld\n", wlan_pm->ul_slpack);
        DECLARE_DFT_TRACE_KEY_INFO("wlan_forbid_sleep", OAL_DFT_TRACE_SUCC);
        return OAL_SUCC;
    }
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    oal_wlan_gpio_intr_enable(wlan_pm->pst_channel, OAL_FALSE);
#endif

    wlan_pm->ul_sleep_gpio_low_cnt++;
    OAM_INFO_LOG1(0, OAM_SF_PWR, "[pm]gpio low!cnt::%u", wlan_pm->ul_sleep_gpio_low_cnt);

    wlan_pm_state_set(wlan_pm, HOST_ALLOW_TO_SLEEP);

    return OAL_SUCC;
}
/* * 函 数 名  : wlan_pm_sleep_fail_proc
   * 功能: 睡眠失败处理
   */
OAL_STATIC void  wlan_pm_sleep_fail_proc(struct wlan_pm_s *pst_wlan_pm)
{
    pst_wlan_pm->ul_sleep_fail_count++;
    wlan_pm_feed_wdg();
    if (pst_wlan_pm->ul_sleep_fail_count > WLAN_WAKEUP_FAIL_MAX_TIMES) {
        OAM_ERROR_LOG1(0, OAM_SF_PWR, "Now ready to enter DFR process after [%d]times wlan_sleep_fail!\n",
                       pst_wlan_pm->ul_sleep_fail_count);
        wlan_pm_stop_wdg(pst_wlan_pm);
        pst_wlan_pm->ul_sleep_fail_count = 0;
        oal_exception_submit(TRANS_FAIL);
    }
    chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV,
                         CHR_PLT_DRV_EVENT_PM, CHR_PLAT_DRV_ERROR_WIFI_SLEEP_REQ);
    return;
}

/* * 函 数 名  : wlan_pm_sleep_forbid_proc
   * 功能: 睡眠失败处理
   */
OAL_STATIC void  wlan_pm_sleep_forbid_proc(struct wlan_pm_s *pst_wlan_pm)
{
    pst_wlan_pm->ul_sleep_fail_count = 0;
    wlan_pm_feed_wdg();
    start_heartbeat();
    return;
}


void wlan_pm_sleep_work(oal_work_stru *pst_worker)
{
    struct wlan_pm_s    *wlan_pm = wlan_pm_get_drv();

    wlan_pm->ul_wdg_timeout_curr_cnt = 0;
    wlan_pm->ul_packet_cnt           = 0;

    hcc_tx_transfer_lock(hcc_get_default_handler());

    OAM_INFO_LOG0(0, OAM_SF_PWR, "[pm]host sleep work");

    if (wlan_pm_sts_check(wlan_pm) != OAL_SUCC) {
        hcc_tx_transfer_unlock(hcc_get_default_handler());
        return;
    }

    if (wlan_pm_sleep_requset_and_ack(wlan_pm) != OAL_SUCC) {
        hcc_tx_transfer_unlock(hcc_get_default_handler());
        wlan_pm_sleep_fail_proc(wlan_pm);
        return;
    }

    if (wlan_pm->ul_slpack == SLPACK_DEV_DISALLOW) {
        hcc_tx_transfer_unlock(hcc_get_default_handler());
        wlan_pm_sleep_forbid_proc(wlan_pm);
        return;
    }
    /* 拉低gpio告知dev流程结束并等待dev去读状态 */
    /* when allow_slp ok, make h2d gpio low */
    board_set_wlan_h2d_pm_state(WLAN_PM_SLPREQ_LEVEL);
    /* 获取当前进入睡眠的绝对时间，以便在唤醒时保证睡眠唤醒间的最小时序约束 */
    wlan_pm->ul_slp_time_us = oal_ktime_get();

    wlan_pm_stop_wdg(wlan_pm);

    /* 等待dev读取gpio状态完毕 */
    udelay(100);

#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    oal_disable_channel_state(wlan_pm->pst_channel, OAL_SDIO_ALL); // psm

    oal_wlan_gpio_intr_enable(wlan_pm->pst_channel, OAL_TRUE);

    oal_channel_wake_unlock(wlan_pm->pst_channel);

    if (wlan_pm->pst_channel->st_sdio_wakelock.lock_count != 0) {
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "[pm]wlan_pm_sleep_work release wakelock %lu!\n",
                         wlan_pm->pst_channel->st_sdio_wakelock.lock_count);
    }
#endif

    hcc_tx_transfer_unlock(hcc_get_default_handler());
    wlan_pm_idle_sleep_vote(WLAN_ALLOW_IDLESLEEP);
    DECLARE_DFT_TRACE_KEY_INFO("wlan_sleep_ok", OAL_DFT_TRACE_SUCC);

    OAM_INFO_LOG0(0, OAM_SF_PWR, "[pm]wlan sleep work succ !\n");

    wlan_pm->ul_sleep_succ++;
    wlan_pm->ul_sleep_fail_count = 0;

    return;
}


oal_ulong wlan_pm_state_get(void)
{
    struct wlan_pm_s   *pst_wlan_pm     = wlan_pm_get_drv();
    if (pst_wlan_pm == NULL) {
        return DEV_SHUTDOWN;
    }
    return pst_wlan_pm->ul_wlan_dev_state;
}


oal_void wlan_pm_state_set(struct wlan_pm_s  *pst_wlan_pm, oal_ulong ul_state)
{
    oal_ulong             ul_flag;
    oal_spin_lock_irq_save(&pst_wlan_pm->pst_channel->st_pm_state_lock, &ul_flag);

    pst_wlan_pm->ul_wlan_dev_state = ul_state;

    oal_spin_unlock_irq_restore(&pst_wlan_pm->pst_channel->st_pm_state_lock, &ul_flag);
}


oal_void  wlan_pm_set_timeout(oal_uint32 ul_timeout)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();

    if (pst_wlan_pm == OAL_PTR_NULL) {
        return ;
    }

    OAM_INFO_LOG1(0, OAM_SF_PWR, "wlan_pm_set_timeout[%d]", ul_timeout);

    pst_wlan_pm->ul_wdg_timeout_cnt = ul_timeout;

    pst_wlan_pm->ul_wdg_timeout_curr_cnt = 0;

    pst_wlan_pm->ul_packet_cnt = 0;

    wlan_pm_feed_wdg();
}
EXPORT_SYMBOL_GPL(wlan_pm_set_timeout);


oal_void  wlan_pm_feed_wdg(oal_void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();

    pst_wlan_pm->ul_sleep_feed_wdg_cnt++;

    oal_timer_start(&pst_wlan_pm->st_watchdog_timer, WLAN_SLEEP_TIMER_PERIOD);
}


oal_int32 wlan_pm_stop_wdg(struct wlan_pm_s *pst_wlan_pm_info)
{
    OAM_INFO_LOG0(0, OAM_SF_PWR, "wlan_pm_stop_wdg \r\n");

    pst_wlan_pm_info->ul_wdg_timeout_curr_cnt = 0;
    pst_wlan_pm_info->ul_packet_cnt = 0;

    if (in_interrupt()) {
        return oal_timer_delete(&pst_wlan_pm_info->st_watchdog_timer);
    } else {
        return oal_timer_delete_sync(&pst_wlan_pm_info->st_watchdog_timer);
    }
}


void wlan_pm_wdg_timeout(unsigned long data)
{
    struct wlan_pm_s *pm_data = (struct wlan_pm_s *)(uintptr_t)data;
    if (pm_data == NULL) {
        return;
    }

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
    if (g_pst_alg_process_func.p_auto_freq_process_func != OAL_PTR_NULL) {
        pm_data->ul_packet_cnt += g_pst_alg_process_func.p_auto_freq_process_func();
    } else {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "wlan_pm_wdg_timeout:NO p_auto_freq_process_func registered");
        goto restart_timer;
    }
#endif

    /* 和调频共用一个timer，低功耗关闭时timer不会停 */
    if (pm_data->ul_wlan_pm_enable != OAL_TRUE) {
        pm_data->ul_packet_cnt = 0;
        goto restart_timer;
    }
    if (pm_data->ul_packet_cnt != 0) {
        pm_data->ul_wdg_timeout_curr_cnt = 0;
        pm_data->ul_packet_cnt           = 0;
        goto restart_timer;
    }

    pm_data->ul_wdg_timeout_curr_cnt++;
    if ((pm_data->ul_wdg_timeout_curr_cnt >= pm_data->ul_wdg_timeout_cnt)) {
        if (wlan_pm_work_submit(pm_data, &pm_data->st_sleep_work) != 0) {
            OAM_WARNING_LOG0(0, OAM_SF_PWR, "wlan_pm_sleep_work submit fail,work is running !\n");
        } else {
            /* 提交了sleep work后，定时器不重启，避免重复提交sleep work */
            pm_data->ul_sleep_work_submit++;
            return;
        }
    }

restart_timer:
    wlan_pm_feed_wdg();

    return;
}


oal_void wlan_pm_resume_test(void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();
    if (wlan_pm_work_submit(pst_wlan_pm, &pst_wlan_pm->st_wakeup_work) != 0) {
        OAM_WARNING_LOG0(0, OAM_SF_PWR, "wlan_pm_resume submit wkup work fail!\n");
    }
    return;
}


oal_void wlan_pm_suspend_test(void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();
    if (wlan_pm_work_submit(pst_wlan_pm, &pst_wlan_pm->st_sleep_work) != 0) {
        pst_wlan_pm->ul_sleep_fail_request++;
        OAM_WARNING_LOG0(0, OAM_SF_PWR, "wlan_pm_suspend submit slp work fail!\n");
    }
    return;
}

struct wlan_pm_s*  wlan_pm_set_default(oal_void)
{
    struct wlan_pm_s *wlan_pm = NULL;

    wlan_pm = oal_kzalloc(sizeof(struct wlan_pm_s), OAL_GFP_KERNEL);
    if (wlan_pm == NULL) {
        OAL_IO_PRINT("[plat_pm]no mem to allocate wlan_pm_data\n");
        return OAL_PTR_NULL;
    }

    memset_s(wlan_pm, sizeof(struct wlan_pm_s), 0, sizeof(struct wlan_pm_s));
    wlan_pm->pst_channel                = oal_get_channel_default_handler();
    /* 默认关低功耗 */
    wlan_pm->ul_wlan_pm_enable          = OAL_FALSE;
    wlan_pm->ul_apmode_allow_pm_flag    = OAL_TRUE;  /* 默认允许下电 */

    /* work queue初始化 */
    wlan_pm->pst_pm_wq = oal_create_singlethread_workqueue("wlan_pm_wq");
    if (wlan_pm->pst_pm_wq == NULL) {
        OAL_IO_PRINT("[plat_pm]Failed to create wlan_pm_wq!");
        oal_free(wlan_pm);
        return OAL_PTR_NULL;
    }

    oal_spin_lock_init(&wlan_pm->pst_channel->st_pm_state_lock);

    /* register wakeup and sleep work */
    OAL_INIT_WORK(&wlan_pm->st_wakeup_work, wlan_pm_wakeup_work);
    OAL_INIT_WORK(&wlan_pm->st_sleep_work,  wlan_pm_sleep_work);
    OAL_INIT_WORK(&wlan_pm->st_freq_adjust_work,  wlan_pm_freq_adjust_work);

    /* host2dev gpio初始化为低 */
    board_set_wlan_h2d_pm_state(WLAN_PM_SLPREQ_LEVEL);

    /* sleep timer初始化 */
    oal_timer_init(&wlan_pm->st_watchdog_timer, 0, wlan_pm_wdg_timeout, (uintptr_t)wlan_pm);

    wlan_pm->ul_wdg_timeout_cnt            = WLAN_SLEEP_DEFAULT_CHECK_CNT;
    wlan_pm->ul_wdg_timeout_curr_cnt       = 0;
    wlan_pm->ul_packet_cnt                 = 0;

    wlan_pm->ul_wlan_power_state           = POWER_STATE_SHUTDOWN;
    wlan_pm->ul_wlan_dev_state             = DEV_SHUTDOWN;
    wlan_pm->ul_slpreq_flag                = NO_SLPREQ_STATUS;
    wlan_pm->ul_slpack                     = SLPACK_DEV_ALLOW;

    wlan_pm->st_wifi_srv_handler.p_wifi_srv_get_pm_pause_func = OAL_PTR_NULL;
    return wlan_pm;
}


struct wlan_pm_s* wlan_pm_init(oal_void)
{
    oal_int32        wlan_irq;
    oal_int32        ret;
    BOARD_INFO       *board_info = NULL;
    struct wlan_pm_s *wl_pm = wlan_pm_set_default();
    if (wl_pm == OAL_PTR_NULL) {
        return OAL_PTR_NULL;
    }

    gpst_wlan_pm_info = wl_pm;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    OAL_INIT_COMPLETION(&wl_pm->st_open_bcpu_done);
    OAL_INIT_COMPLETION(&wl_pm->st_close_bcpu_done);
#endif
    OAL_INIT_COMPLETION(&wl_pm->st_close_done);
    OAL_INIT_COMPLETION(&wl_pm->st_wakeup_done);
    OAL_INIT_COMPLETION(&wl_pm->st_sleep_request_ack);

    oal_channel_message_register(wl_pm->pst_channel, D2H_MSG_WLAN_READY, wlan_pm_set_device_ready, wl_pm);
    oal_channel_message_register(wl_pm->pst_channel, D2H_MSG_WAKEUP_SUCC, wlan_pm_wakeup_done_callback, wl_pm);
    oal_channel_message_register(wl_pm->pst_channel, D2H_MSG_ALLOW_SLEEP, wlan_pm_allow_sleep_callback, wl_pm);
    oal_channel_message_register(wl_pm->pst_channel, D2H_MSG_DISALLOW_SLEEP, wlan_pm_disallow_sleep_callback, wl_pm);
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    oal_channel_message_register(wl_pm->pst_channel, D2H_MSG_HEARTBEAT, wlan_pm_heartbeat_callback, wl_pm);
#endif

#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    wl_pm->pst_channel->data_int_count = 0;
#endif
    wl_pm->pst_channel->wakeup_int_count = 0;
    wl_pm->pst_channel->gpio_int_count = 0;

#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    wl_pm->pst_channel->pst_pm_callback = &gst_wlan_pm_callback;

    board_info = get_board_info();
    if (board_info == OAL_PTR_NULL) {
        OAL_IO_PRINT("failed to get board info\n");
        oal_free(wl_pm);
        return OAL_PTR_NULL;
    }
    wlan_irq = board_info->wlan_wakeup_irq;
    if (wlan_irq != INVALID_IRQ) {
        ret = oal_request_irq(wlan_irq, wlan_gpio_irq, IRQF_NO_SUSPEND | IRQF_TRIGGER_RISING | IRQF_DISABLED,
                              "wifi_gpio_intr", wl_pm->pst_channel);
        if (ret < 0) {
            OAL_IO_PRINT("failed to request wakeup irq\n");
            oal_free(wl_pm);
            return OAL_PTR_NULL;
        }
    }
#endif

    return  wl_pm;
}

void wlan_pm_info_clean(void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv();

#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    pst_wlan_pm->pst_channel->data_int_count                   = 0;
    pst_wlan_pm->pst_channel->wakeup_int_count                 = 0;
    pst_wlan_pm->pst_channel->gpio_int_count                   = 0;
    pst_wlan_pm->pst_channel->sdio_int_count                   = 0;
    pst_wlan_pm->pst_channel->data_int_finish_count            = 0;
    pst_wlan_pm->pst_channel->func1_stat.func1_no_int_count    = 0;
    pst_wlan_pm->pst_channel->func1_stat.func1_err_int_count   = 0;
    pst_wlan_pm->pst_channel->func1_stat.func1_msg_int_count   = 0;
    pst_wlan_pm->pst_channel->func1_stat.func1_data_int_count  = 0;
    pst_wlan_pm->pst_channel->func1_stat.func1_unknow_int_count = 0;

    pst_wlan_pm->pst_channel->msg[D2H_MSG_WAKEUP_SUCC].count = 0;
    pst_wlan_pm->pst_channel->msg[D2H_MSG_ALLOW_SLEEP].count = 0;
    pst_wlan_pm->pst_channel->msg[D2H_MSG_DISALLOW_SLEEP].count = 0;
    pst_wlan_pm->pst_channel->msg[D2H_MSG_DEVICE_PANIC].count = 0;

    pst_wlan_pm->pst_channel->ul_sdio_suspend               = 0;
    pst_wlan_pm->pst_channel->ul_sdio_resume                = 0;
#endif
    pst_wlan_pm->ul_wakeup_succ = 0;
    pst_wlan_pm->ul_wakeup_dev_ack = 0;
    pst_wlan_pm->ul_wakeup_done_callback = 0;
    pst_wlan_pm->ul_wakeup_succ_work_submit = 0;
    pst_wlan_pm->ul_wakeup_gpio_up_cnt = 0;
    pst_wlan_pm->ul_waitting_for_dev_ack = 0;
    pst_wlan_pm->ul_wakeup_fail_wait_sdio = 0;
    pst_wlan_pm->ul_wakeup_fail_timeout = 0;
    pst_wlan_pm->ul_wakeup_fail_set_reg = 0;
    pst_wlan_pm->ul_wakeup_fail_submit_work = 0;

    pst_wlan_pm->ul_sleep_succ = 0;
    pst_wlan_pm->ul_sleep_feed_wdg_cnt = 0;
    pst_wlan_pm->ul_wakeup_done_callback = 0;
    pst_wlan_pm->ul_sleep_fail_set_reg = 0;
    pst_wlan_pm->ul_sleep_fail_wait_timeout = 0;
    pst_wlan_pm->ul_sleep_fail_forbid = 0;
    pst_wlan_pm->ul_sleep_work_submit = 0;
    pst_wlan_pm->ul_sleep_msg_send_cnt = 0;
    pst_wlan_pm->ul_sleep_gpio_low_cnt = 0;

    return;
}


#ifdef __cpluscplus
#if __cplusplus
}
#endif
#endif

