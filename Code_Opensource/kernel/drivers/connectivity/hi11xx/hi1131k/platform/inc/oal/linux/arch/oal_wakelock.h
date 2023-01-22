

#ifndef __OAL_LINUX_WAKE_LOCK_H__
#define __OAL_LINUX_WAKE_LOCK_H__

#include <linux/version.h>

#include "oal_mutex.h"
#include "oal_spinlock.h"
#include "securec.h"

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,9,0))
#include <linux/wakelock.h>
#else
#include <linux/ktime.h>
#include <linux/device.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
enum {
    WAKE_LOCK_SUSPEND, /* Prevent suspend */
    WAKE_LOCK_TYPE_COUNT
};

struct wake_lock {
    struct wakeup_source ws;
};

static inline void wake_lock_init(struct wake_lock *lock, int type,
                                  const char *name)
{
    wakeup_source_init(&lock->ws, name);
}

static inline void wake_lock_destroy(struct wake_lock *lock)
{
    wakeup_source_trash(&lock->ws);
}

static inline void wake_lock(struct wake_lock *lock)
{
    __pm_stay_awake(&lock->ws);
}

static inline void wake_lock_timeout(struct wake_lock *lock, long timeout)
{
    __pm_wakeup_event(&lock->ws, jiffies_to_msecs(timeout));
}

static inline void wake_unlock(struct wake_lock *lock)
{
    __pm_relax(&lock->ws);
}

static inline int wake_lock_active(struct wake_lock *lock)
{
    return lock->ws.active;
}
#endif

/*****************************************************************************
  2 STRUCT定义
*****************************************************************************/
typedef struct _oal_wakelock_stru_ {
#ifdef CONFIG_HAS_WAKELOCK
struct wake_lock        st_wakelock;        // wakelock锁
oal_spin_lock_stru      lock;    // wakelock锁操作spinlock锁
#endif
oal_ulong               lock_count;         // 持有wakelock锁的次数
oal_ulong               locked_addr; /* the locked address */
} oal_wakelock_stru;
OAL_STATIC OAL_INLINE oal_void oal_wake_lock_init(oal_wakelock_stru *pst_wakelock, const char* name)
{
#ifdef CONFIG_HAS_WAKELOCK
memset_s((oal_void*)pst_wakelock, sizeof(oal_wakelock_stru), 0, sizeof(oal_wakelock_stru));

wake_lock_init(&pst_wakelock->st_wakelock, WAKE_LOCK_SUSPEND, name ? name : "wake_lock_null");
oal_spin_lock_init(&pst_wakelock->lock);
pst_wakelock->lock_count = 0;
pst_wakelock->locked_addr = 0;
#endif
}

OAL_STATIC OAL_INLINE oal_void oal_wake_lock_exit(oal_wakelock_stru *pst_wakelock)
{
#ifdef CONFIG_HAS_WAKELOCK
    wake_lock_destroy(&pst_wakelock->st_wakelock);
#endif
}

OAL_STATIC OAL_INLINE void oal_wake_lock(oal_wakelock_stru *pst_wakelock)
{
#ifdef CONFIG_HAS_WAKELOCK
    oal_ulong ul_flags;

    oal_spin_lock_irq_save(&pst_wakelock->lock, &ul_flags);
    if (!pst_wakelock->lock_count) {
        wake_lock(&pst_wakelock->st_wakelock);
        pst_wakelock->locked_addr = (oal_ulong)_RET_IP_;
    }
    pst_wakelock->lock_count++;
    oal_spin_unlock_irq_restore(&pst_wakelock->lock, &ul_flags);
#endif
}

OAL_STATIC OAL_INLINE  void oal_wake_unlock(oal_wakelock_stru *pst_wakelock)
{
#ifdef CONFIG_HAS_WAKELOCK
    oal_ulong ul_flags;

    oal_spin_lock_irq_save(&pst_wakelock->lock, &ul_flags);
    if (pst_wakelock->lock_count) {
        pst_wakelock->lock_count--;
        if (!pst_wakelock->lock_count) {
            wake_unlock(&pst_wakelock->st_wakelock);
            pst_wakelock->locked_addr = (oal_ulong)0x0;
        }
    }
    oal_spin_unlock_irq_restore(&pst_wakelock->lock, &ul_flags);
#endif
}

OAL_STATIC OAL_INLINE oal_ulong oal_wakelock_active(oal_wakelock_stru *pst_wakelock)
{
#ifdef CONFIG_HAS_WAKELOCK
    return (oal_ulong)wake_lock_active(&pst_wakelock->st_wakelock);
#else
    return 0;
#endif
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_wakelock.h */

