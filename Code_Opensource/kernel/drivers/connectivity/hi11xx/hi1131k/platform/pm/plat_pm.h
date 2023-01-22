

#ifndef __PLAT_PM_H__
#define __PLAT_PM_H__


/*****************************************************************************
  1 Include other Head file
*****************************************************************************/
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#include <linux/mutex.h>
#include <linux/kernel.h>
#include <linux/wakelock.h>
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#include "plat_pm_wlan.h"
#include "plat_board_adapt.h"
#endif
/*****************************************************************************
  2 Define macro
*****************************************************************************/
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#define PM_SUSPEND_PREPARE  0x0003 /* Going to suspend the system */
#define PM_POST_SUSPEND     0x0004 /* Suspend finished */
#define MMC_PM_KEEP_POWER   (1 << 0)    /* preserve card power during suspend */
#define PM_SUSPEND_PREPARE  0x0003 /* Going to suspend the system */
#define PM_POST_SUSPEND     0x0004 /* Suspend finished */
#define MMC_PM_KEEP_POWER   (1 << 0)    /* preserve card power during suspend */

#define ASYNCB_SUSPENDED    30 /* Serial port is suspended */
#endif

#define SUCCESS                         0
#define FAILURE                         1

#define WAIT_DEVACK_MSEC               100
#define WAIT_DEVACK_TIMEOUT_MSEC       200

#define WAIT_WKUPDEV_MSEC              3400

#define SDIO_REINIT_RETRY              5

#define DMD_EVENT_BUFF_SIZE 1024

#ifdef CONFIG_HUAWEI_DSM
#define DSM_1131K_TCXO_ERROR        909030001
#define DSM_1131K_DOWNLOAD_FIRMWARE 909030033
#define DSM_1131K_HALT              909030035
#define DSM_WIFI_FEMERROR           909030036
#define DSM_SDIO_PROBE_FAIL         909030006

#define DMD_EVENT_BUFF_SIZE 1024

typedef enum sub_system {
    SYSTEM_TYPE_WIFI = 0,
    SYSTEM_TYPE_BT = 1,
    SYSTEM_TYPE_GNSS = 2,
    SYSTEM_TYPE_FM = 3,
    SYSTEM_TYPE_PLATFORM = 4,
    SYSTEM_TYPE_BFG = 5,
    SYSTEM_TYPE_IR = 6,
    SYSTEM_TYPE_NFC = 7,
    SYSTEM_TYPE_BUT,
} SUB_SYSTEM;
#endif

enum WLAN_DEV_STATUS_ENUM {
    HOST_DISALLOW_TO_SLEEP = 0,
    HOST_ALLOW_TO_SLEEP = 1,
    HOST_EXCEPTION = 2,
    DEV_SHUTDOWN = 3,
};

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
/* BFGX系统上电加载异常类型 */
enum BFGX_POWER_ON_EXCEPTION_ENUM {
    BFGX_POWER_FAILED                               = -1,
    BFGX_POWER_SUCCESS                              = 0,

    BFGX_POWER_PULL_POWER_GPIO_FAIL                 = 1,
    BFGX_POWER_TTY_OPEN_FAIL                        = 2,
    BFGX_POWER_TTY_FLOW_ENABLE_FAIL                 = 3,

    BFGX_POWER_WIFI_DERESET_BCPU_FAIL               = 4,
    BFGX_POWER_WIFI_ON_BOOT_UP_FAIL                 = 5,

    BFGX_POWER_WIFI_OFF_BOOT_UP_FAIL                = 6,
    BFGX_POWER_DOWNLOAD_FIRMWARE_FAIL               = 7,

    BFGX_POWER_WAKEUP_FAIL                          = 8,
    BFGX_POWER_OPEN_CMD_FAIL                        = 9,

    BFGX_POWER_ENUM_BUTT,
};

/* wifi系统上电加载异常类型 */
enum WIFI_POWER_ON_EXCEPTION_ENUM {
    WIFI_POWER_FAIL                                 = -1,
    WIFI_POWER_SUCCESS                              = 0,
    WIFI_POWER_PULL_POWER_GPIO_FAIL                 = 1,

    WIFI_POWER_BFGX_OFF_BOOT_UP_FAIL                = 2,
    WIFI_POWER_BFGX_OFF_FIRMWARE_DOWNLOAD_FAIL      = 3,

    WIFI_POWER_BFGX_ON_BOOT_UP_FAIL                 = 4,
    WIFI_POWER_BFGX_DERESET_WCPU_FAIL               = 5,
    WIFI_POWER_BFGX_ON_FIRMWARE_DOWNLOAD_FAIL       = 6,

    WIFI_POWER_ENUM_BUTT,
};

#elif (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
/* wifi系统上电加载异常类型 */
enum WIFI_POWER_ON_EXCEPTION_ENUM {
    WIFI_POWER_FAIL                                 = -1,
    WIFI_POWER_SUCCESS                              = 0,
    WIFI_POWER_BFGX_OFF_BOOT_UP_FAIL                = 1,
    WIFI_POWER_BFGX_OFF_FIRMWARE_DOWNLOAD_FAIL      = 2,

    WIFI_POWER_ENUM_BUTT,
};
#endif

typedef enum {
    POWER_STATE_SHUTDOWN = 0,
    POWER_STATE_OPEN     = 1,
    POWER_STATE_BUTT     = 2,
}power_state_enum;


/*****************************************************************************
  3 STRUCT DEFINE
*****************************************************************************/

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
/* private data for pm driver */
struct pm_drv_data {
    /* wlan interface pointer */
    struct wlan_pm_s                *pst_wlan_pm_info;

    /* board customize info */
    BOARD_INFO                      *board;

    /* mutex for sync */
    oal_mutex_stru                    host_mutex;

    /* Tasklet to respond to change in hostwake line */
    oal_tasklet_stru           hostwake_task;

    /* workqueue for wkup device */
    struct workqueue_struct *wkup_dev_workqueue;
    struct work_struct wkup_dev_work;
    struct work_struct send_disallow_msg_work;

    /* wait device ack timer */
    struct timer_list bfg_timer;
    struct timer_list dev_ack_timer;

    /* the completion for waiting for host wake up device ok */
    oal_completion host_wkup_dev_comp;
    /* the completion for waiting for dev ack(host allow sleep) ok */
    oal_completion dev_ack_comp;
    /* the completion for waiting for dev boot ok */
    oal_completion dev_bootok_ack_comp;
};
#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
/* private data for pm driver */
struct pm_drv_data {
    /* 3 in 1 interface pointer */
    struct ps_pm_s                  *ps_pm_interface;

    /* wlan interface pointer */
    struct wlan_pm_s                *pst_wlan_pm_info;

    /* board customize info */
    BOARD_INFO                      *board;

    /* wake lock for bfg,be used to prevent host form suspend */
    struct wake_lock                bfg_wake_lock;

    /* mutex for sync */
    oal_mutex_stru                  host_mutex;

    /* bfgx VS. bfg timer spinlock */
    spinlock_t                      node_timer_spinlock;

    /* uart could be used or not */
    oal_uint8   uart_ready;

    /* mark receiving data after set dev as sleep state but before get ack of device */
    oal_uint8 rcvdata_bef_devack_flag;

    /* bfgx sleep state */
    oal_uint8 bfgx_dev_state;

    /* flag for firmware cfg file init */
    oal_ulong firmware_cfg_init_flag;

    /* bfg irq num */
    oal_uint32 bfg_irq;

    /* bfg wakeup host count */
    oal_uint32 bfg_wakeup_host;

    /* gnss lowpower state */
    atomic_t gnss_sleep_flag;

    atomic_t bfg_needwait_devboot_flag;

    /* flag to mark whether enable lowpower or not */
    oal_uint32 bfgx_pm_ctrl_enable;
    oal_uint8  bfgx_lowpower_enable;
    oal_uint8  bfgx_bt_lowpower_enable;
    oal_uint8  bfgx_gnss_lowpower_enable;
    oal_uint8  bfgx_nfc_lowpower_enable;

    /* workqueue for wkup device */
    struct workqueue_struct *wkup_dev_workqueue;
    struct work_struct wkup_dev_work;
    struct work_struct send_disallow_msg_work;

    /* wait device ack timer */
    struct timer_list bfg_timer;
    struct timer_list dev_ack_timer;

    /* the completion for waiting for host wake up device ok */
    struct completion host_wkup_dev_comp;
    /* the completion for waiting for dev ack(host allow sleep) ok */
    struct completion dev_ack_comp;
    /* the completion for waiting for dev boot ok */
    struct completion dev_bootok_ack_comp;
};

struct notifier_block;

typedef int (*notifier_fn_t)(struct notifier_block *nb, unsigned long action, void *data);

struct notifier_block {
    notifier_fn_t notifier_call;
    struct notifier_block *next;
    int priority;
};
#endif

/*****************************************************************************
  4 EXTERN VARIABLE
*****************************************************************************/
/*****************************************************************************
  5 EXTERN FUNCTION
*****************************************************************************/
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)|| (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
extern struct pm_drv_data *pm_get_drvdata(void);
extern oal_int32 wlan_is_shutdown(void);
extern oal_int32 wlan_power_on(void);
extern oal_int32 wlan_power_off(void);
extern int firmware_download_function(oal_uint32 which_cfg);
extern oal_int32 hi110x_get_wifi_power_stat(oal_void);
#ifdef CONFIG_HUAWEI_DSM
#if (_PRE_LINUX_PLATFORM == MIAMI_C60)
extern void hw_1131k_register_dsm_client(void);
extern void hw_1131k_unregister_dsm_client(void);
extern void hw_1131k_dsm_client_notify(int32_t sub_sys, int32_t dsm_id, const int8_t *fmt, ...);
#endif
#endif

#endif

#endif

