

#ifndef __EXCEPTION_RST_H__
#define __EXCEPTION_RST_H__
/*****************************************************************************
  1 Include other Head file
*****************************************************************************/
#include "plat_type.h"
#include "oal_types.h"
#include "oal_spinlock.h"
#include "oal_workqueue.h"
#include "oal_schedule.h"
/*****************************************************************************
  2 Define macro
*****************************************************************************/
#define WIFI_DUMP_TIMEOUT          1000   /* wifi dump bcpu数据等待时间 */

#define PLAT_EXCEPTION_RESET_IDLE  0      /* plat 没有在处理异常 */
#define PLAT_EXCEPTION_RESET_BUSY  1      /* plat 正在处理异常 */

#define EXCEPTION_SUCCESS          0
#define EXCEPTION_FAIL             1

#define GPIO_HOLD_STATE_INIT       0
#define GPIO_HOLD_STATE_HIGH       1

/* plat cfg cmd */
#define PLAT_CFG_IOC_MAGIC                     'z'
#define PLAT_DFR_CFG_CMD                       _IOW(PLAT_CFG_IOC_MAGIC, PLAT_DFR_CFG, int)
#define PLAT_BEATTIMER_TIMEOUT_RESET_CFG_CMD   _IOW(PLAT_CFG_IOC_MAGIC, PLAT_BEATTIMER_TIMEOUT_RESET_CFG, int)

#define PLAT_EXCEPTION_ENABLE                  1
#define PLAT_EXCEPTION_DISABLE                 0

#define PLAT_EXCP_DUMP_ROTATE_QUEUE_MAX_LEN    20
#define PLAT_WIFI_EXCP_CFG_IOC_MAGIC 'w'
#define PLAT_WIFI_DUMP_FILE_READ_CMD _IOW(PLAT_WIFI_EXCP_CFG_IOC_MAGIC, PLAT_WIFI_DUMP_FILE_READ, int)
/*****************************************************************************
  3 STRUCT DEFINE
*****************************************************************************/

enum EXCEPTION_TYPE_ENUM {
    DEVICE_PANIC      = 0, /* arm exception */
    TRANS_FAIL        = 1, /* read or write failed */
    HOST_EXCP         = 2, /* sdio host controler exception */
    EXCEPTION_TYPE_BUTT,
};

enum PLAT_WIFI_EXCP_CMD_E {
    DFR_HAL_WIFI = 0,
    PLAT_WIFI_DUMP_FILE_READ = 1,
};

struct st_wifi_dump_mem_info {
    oal_ulong  mem_addr;
    oal_uint32 size;
    oal_uint8  *file_name;
};
typedef struct st_wifi_dump_mem_info *p_st_wifi_demp_mem_info;


struct st_exception_info {
    oal_uint32   exception_reset_enable;
    oal_uint32   excetion_type;

    // struct workqueue_struct *plat_exception_rst_workqueue;
    oal_uint32 wifi_exception_cnt;

    /* when recv error data from device, dump device mem */
    #define NOT_DUMP_MEM        0
    oal_uint32  dump_mem_flag;

    oal_work_stru       excp_worker;
    oal_spin_lock_stru  excp_lock;

    void (*wifi_dfr_func)(void);
};

enum DUMP_CMD_TYPE {
    CMD_READM_WIFI_SDIO = 0,
    CMD_READM_WIFI_UART = 1,
    CMD_READM_BFGX_UART = 2,
    CMD_READM_BFGX_SDIO = 3,

    CMD_DUMP_BUFF,
};
#ifndef WIN32
struct st_wifi_dump_mem_driver {
    struct sk_buff_head     quenue;
    wait_queue_head_t       dump_type_wait;
    struct sk_buff_head     dump_type_queue;
    int32_t                 is_open;
    int32_t                 is_working;
};
#endif
/*****************************************************************************
  4 EXTERN VARIABLE
*****************************************************************************/
extern p_st_wifi_demp_mem_info g_apst_panic_dump_mem[];
extern p_st_wifi_demp_mem_info g_apst_trans_fail_dump_mem[];
extern struct st_wifi_dump_mem_driver g_dump_mem_driver;
/*****************************************************************************
  5 全局变量定义
*****************************************************************************/
enum PLAT_CFG {
    PLAT_DFR_CFG                           = 0,
    PLAT_BEATTIMER_TIMEOUT_RESET_CFG,
    PLAT_TRIGGER_EXCEPTION,
    PLAT_POWER_RESET,
    PLAT_CHANNEL_DISABLE,

    PLAT_CFG_BUFF,
};

/*****************************************************************************
  6 EXTERN FUNCTION
*****************************************************************************/
extern void  plat_dfr_cfg_set(oal_ulong arg);
extern oal_int32 get_exception_info_reference(struct st_exception_info **exception_data);
extern oal_int32 plat_exception_handler(oal_uint32 exception_type);
extern oal_int32 wifi_exception_mem_dump(p_st_wifi_demp_mem_info *pst_mem_dump_info);
extern oal_int32 plat_exception_reset_init(void);
extern oal_int32 plat_exception_reset_exit(void);
extern oal_int32 wifi_system_reset(void);
extern void  oal_exception_submit(oal_int32 excep_type);
extern void  oal_wakeup_exception(void);

extern oal_int32 oal_device_panic_callback(void *data);
extern oal_int32 oal_trigger_exception(oal_int32 is_sync);
extern oal_int32 oal_exception_is_busy(void);
extern void  oal_try_to_dump_device_mem(oal_int32 is_sync);
void wifi_exception_dev_panic_info_get(oal_file_stru *fp, p_st_wifi_demp_mem_info pst_mem_dump_info);

#ifndef WIN32
struct st_wifi_dump_mem_driver *plat_get_dump_mem_driver(void);
void  plat_exception_dump_file_rotate_init(void);
void  plat_exception_dump_quenue_clear(void);
void  plat_exception_dump_finish(void);
int32_t plat_exception_dump_enquenue(uint8_t *buf_ptr, uint32_t count);
int32_t plat_excp_send_rotate_cmd_to_app(uint32_t which_dump);
int32_t plat_exception_dump_notice(void);
void wifi_exception_dev_panic_info_get_etc(uint8_t *DataBuf,
                                           uint32_t pos,
                                           p_st_wifi_demp_mem_info pst_mem_dump_info);
void kfree_skb(struct sk_buff *skb);
#endif
#endif

