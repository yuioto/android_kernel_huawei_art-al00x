

#ifndef __OAL_LINUX_TASK_H__
#define __OAL_LINUX_TASK_H__

#include <linux/mutex.h>
#include "oal_workqueue.h"
#include "oal_spinlock.h"
#include "oal_wait.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

typedef struct _oal_task_lock_stru_ {
    oal_wait_queue_head_stru    wq;
    struct task_struct               *claimer;   /* task that has host claimed */
    oal_spin_lock_stru               lock;       /* lock for claim and bus ops */
    oal_ulong                           claim_addr;
    oal_uint32                          claimed;
    oal_int32                            claim_cnt;
} oal_task_lock_stru;

typedef struct tasklet_struct       oal_tasklet_stru;
typedef oal_void                    (*oal_defer_func)(oal_ulong);


/* taskletÉùÃ÷ */
#define OAL_DECLARE_TASK    DECLARE_TASKLET

OAL_STATIC OAL_INLINE oal_void  oal_task_init(oal_tasklet_stru *pst_task, oal_defer_func p_func, oal_void *p_args)
{
    tasklet_init(pst_task, p_func, (oal_ulong)p_args);
}


OAL_STATIC OAL_INLINE oal_void oal_task_kill(oal_tasklet_stru *pst_task)
{
    return tasklet_kill(pst_task);
}


OAL_STATIC OAL_INLINE oal_void  oal_task_sched(oal_tasklet_stru *pst_task)
{
    tasklet_schedule(pst_task);
}


OAL_STATIC OAL_INLINE oal_ulong oal_task_is_scheduled(oal_tasklet_stru *pst_task)
{
    return oal_bit_atomic_test(TASKLET_STATE_SCHED, (oal_ulong *)&pst_task->state);
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_task.h */

