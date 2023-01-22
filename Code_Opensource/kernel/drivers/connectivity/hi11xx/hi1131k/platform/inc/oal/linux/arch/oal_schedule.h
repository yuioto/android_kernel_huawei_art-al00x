
#ifndef __OAL_LINUX_SCHEDULE_H__
#define __OAL_LINUX_SCHEDULE_H__

/*lint -e322*/
#include <linux/interrupt.h>
#include <asm/param.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <asm/uaccess.h>


#include "oal_mutex.h"
#include "oal_semaphore.h"
#include "oal_task.h"
#include "oal_rw_lock.h"
#include "oal_spinlock.h"
#include "oal_wakelock.h"
#include "oal_timer.h"
#include "oal_time.h"
#include "oal_atomic.h"
#include "oal_wait.h"
#include "oal_completion.h"
#include "oal_mm.h"
#include "securec.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
    #include <uapi/linux/sched/types.h>
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
/*lint +e322*/
#define oal_in_interrupt()  in_interrupt()

#define oal_in_atomic()     in_atomic()


typedef oal_uint32 (*oal_module_func_t)(oal_void);

/* 模块入口 */
#define oal_module_init(_module_name)   module_init(_module_name)

#define oal_module_license(_license_name) MODULE_LICENSE(_license_name)

#define oal_module_param    module_param

#define OAL_S_IRUGO         S_IRUGO

/* 获取毫秒级时间戳 */
#define oal_time_get_stamp_ms() jiffies_to_msecs(jiffies)

/* 寄存器反转模块运行时间计算 */
#define oal_time_calc_runtime(_ul_start, _ul_end)                              \
    (uint32_t)((oal_div_u64((uint64_t)(OAL_TIME_US_MAX_LEN), HZ) * 1000) + \
                 (((OAL_TIME_US_MAX_LEN) % HZ) * (1000 / HZ)) - (_ul_start) + (_ul_end))

/* 获取从_ul_start到_ul_end的时间差 */
#define oal_time_get_runtime(_ul_start, _ul_end) \
    (((_ul_start) > (_ul_end)) ? (oal_time_calc_runtime((_ul_start), (_ul_end))) : ((_ul_end) - (_ul_start)))

/* 模块出口 */
#define oal_module_exit(_module_name)   module_exit(_module_name)

/* 模块符号导出 */
#define oal_module_symbol(_symbol)      EXPORT_SYMBOL(_symbol)
#define OAL_MODULE_DEVICE_TABLE(_type, _name) MODULE_DEVICE_TABLE(_type, _name)

#ifdef HI1131C_SDIO_DETECT_SUPPORT/*sdio_detect.ko not need export symbol*/
#define OAL_EXPORT_SYMBOL(_symbol)
#else
#define OAL_EXPORT_SYMBOL(_symbol)      EXPORT_SYMBOL(_symbol)
#endif

#define oal_smp_call_function_single(core, task, info, wait) smp_call_function_single(core, task, info, wait)

typedef struct proc_dir_entry       oal_proc_dir_entry_stru;

OAL_STATIC OAL_INLINE oal_uint32  oal_copy_from_user(oal_void *p_to, const oal_void *p_from, oal_uint32 ul_size)
{
    return (oal_uint32)copy_from_user(p_to, p_from, (oal_ulong)ul_size);
}

OAL_STATIC OAL_INLINE oal_uint32  oal_copy_to_user(oal_void *p_to, const oal_void *p_from, oal_uint32 ul_size)
{
    return (oal_uint32)copy_to_user(p_to, p_from, (oal_ulong)ul_size);
}

/*
 * 函 数 名  : oal_div_u64
 * 功能描述  : linux uint64 除法
 */
OAL_STATIC OAL_INLINE uint64_t oal_div_u64(uint64_t dividend, uint32_t divisor)
{
    uint64_t ull_tmp;

    ull_tmp = div_u64(dividend, divisor);

    return ull_tmp;
}


OAL_STATIC OAL_INLINE oal_proc_dir_entry_stru* oal_create_proc_entry(const oal_int8 *pc_name, oal_uint16 us_mode,
                                                                     oal_proc_dir_entry_stru *pst_parent)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,44))
    return NULL;
#else
    return create_proc_entry(pc_name, us_mode, pst_parent);
#endif
}


OAL_STATIC OAL_INLINE void oal_remove_proc_entry(const oal_int8 *pc_name, oal_proc_dir_entry_stru *pst_parent)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,35))
#else
    return remove_proc_entry(pc_name, pst_parent);
#endif
}


#ifdef _PRE_OAL_FEATURE_TASK_NEST_LOCK

extern oal_void _oal_smp_task_lock_(oal_task_lock_stru* pst_lock, uintptr_t claim_addr);
#define oal_smp_task_lock(lock) \
    /*lint -e(24) */_oal_smp_task_lock_(lock, (uintptr_t)_THIS_IP_)


OAL_STATIC OAL_INLINE oal_void oal_smp_task_unlock(oal_task_lock_stru* pst_lock)
{
    oal_ulong flags;

    if (OAL_WARN_ON((oal_ulong)in_interrupt() || in_atomic())) {
        return;
    }

    if (OAL_UNLIKELY(!pst_lock->claimed)) {
        OAL_WARN_ON(1);
        return;
    }

    oal_spin_lock_irq_save(&pst_lock->lock, &flags);
    if (--pst_lock->claim_cnt) {
        oal_spin_unlock_irq_restore(&pst_lock->lock, &flags);
    } else {
        pst_lock->claimed = 0;
        pst_lock->claimer = NULL;
        oal_spin_unlock_irq_restore(&pst_lock->lock, &flags);
        wake_up(&pst_lock->wq);
    }
}


OAL_STATIC OAL_INLINE oal_void oal_smp_task_lock_init(oal_task_lock_stru* pst_lock)
{
    memset_s((oal_void*)pst_lock, sizeof(oal_task_lock_stru), 0, sizeof(oal_task_lock_stru));

    oal_spin_lock_init(&pst_lock->lock);
    OAL_WAIT_QUEUE_INIT_HEAD(&pst_lock->wq);
    pst_lock->claimed = 0;
    pst_lock->claim_cnt = 0;
}
#endif


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_schedule.h */
