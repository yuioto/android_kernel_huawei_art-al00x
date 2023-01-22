

#ifndef __OAL_LINUX_THREAD_H__
#define __OAL_LINUX_THREAD_H__

#include <asm/atomic.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/jiffies.h>
#include <linux/time.h>
#include <asm/param.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
    #include <linux/sched/signal.h>
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
typedef  struct task_struct oal_kthread_stru;

typedef struct _kthread_param_ {
    oal_uint32         ul_stacksize;
    oal_int32           l_prio;
    oal_int32           l_policy;
    oal_int32           l_cpuid;
    oal_int32           l_nice;
} oal_kthread_param_stru;

#define oal_kthread_should_stop   kthread_should_stop
#define oal_schedule              schedule

#define OAL_CURRENT         current

#define OAL_SCHED_FIFO      1
#define OAL_SCHED_RR        2

#define NOT_BIND_CPU        (-1)

typedef int (*oal_thread_func)(void *);
OAL_STATIC OAL_INLINE oal_kthread_stru* oal_kthread_create(const char              *pc_thread_name,
                                                           oal_thread_func          pf_threadfn,
                                                           void                    *p_data,
                                                           oal_kthread_param_stru  *pst_thread_param)
{
    oal_int32 uwRet;
    oal_kthread_stru *pst_kthread = NULL;
    struct sched_param       st_sched_param;
    OAL_BUG_ON(!pst_thread_param);

    pst_kthread = kthread_create(pf_threadfn, p_data, pc_thread_name);
    if (IS_ERR_OR_NULL(pst_kthread)) {
        OAL_IO_PRINT("failed to run theread:%s\n", pc_thread_name);
        return NULL;
    }

    st_sched_param.sched_priority = pst_thread_param->l_prio;
    uwRet = sched_setscheduler(pst_kthread, pst_thread_param->l_policy, &st_sched_param);
    if (OAL_UNLIKELY(uwRet)) {
        OAL_IO_PRINT("%s sched_setscheduler failed! ret =%d, prio=%d\n", pc_thread_name, uwRet,
                     pst_thread_param->l_prio);
    }
    if (pst_thread_param->l_cpuid >= 0) { // cpuid为负数时无效
        kthread_bind(pst_kthread, pst_thread_param->l_cpuid);
    } else {
        OAL_IO_PRINT("did not bind cpu...\n");
    }
    wake_up_process(pst_kthread);
    return pst_kthread;
}


OAL_STATIC OAL_INLINE oal_uint32 oal_set_thread_property(oal_kthread_stru                 *pst_thread,
                                                         oal_kthread_param_stru       *pst_thread_param)
{
    struct sched_param       st_sched_param;
    OAL_BUG_ON(!pst_thread);
    OAL_BUG_ON(!pst_thread_param);

    st_sched_param.sched_priority = pst_thread_param->l_prio;
    OAL_IO_PRINT("set thread scheduler policy %d\n", pst_thread_param->l_policy);

    if (sched_setscheduler(pst_thread, pst_thread_param->l_policy, &st_sched_param)) {
        OAL_IO_PRINT("[Error]set scheduler failed! %d\n", pst_thread_param->l_policy);
        return -OAL_EFAIL;
    }

    if (pst_thread_param->l_policy != SCHED_FIFO && pst_thread_param->l_policy != SCHED_RR) {
        OAL_IO_PRINT("set thread scheduler nice %d\n", pst_thread_param->l_nice);
        set_user_nice(pst_thread, pst_thread_param->l_nice);
    }
    return OAL_SUCC;
}

OAL_STATIC OAL_INLINE oal_void oal_kthread_stop(oal_kthread_stru *pst_thread)
{
    OAL_BUG_ON(!pst_thread);

    send_sig(SIGTERM, pst_thread, 1);
    kthread_stop(pst_thread);
}

OAL_STATIC OAL_INLINE char* oal_get_current_task_name(oal_void)
{
    return current->comm;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
