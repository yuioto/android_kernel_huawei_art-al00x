
#ifndef __OAL_LINUX_SEMAPHORE_H__
#define __OAL_LINUX_SEMAPHORE_H__

#include <linux/completion.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

typedef struct semaphore          oal_semaphore_stru;

OAL_STATIC OAL_INLINE oal_void oal_sema_init(oal_semaphore_stru *sem, oal_int32 val)
{
    sema_init(sem, val);
}


OAL_STATIC OAL_INLINE oal_void oal_up(oal_semaphore_stru *sem)
{
    up(sem);
}

OAL_STATIC OAL_INLINE oal_void oal_down(oal_semaphore_stru *sem)
{
    down(sem);
}

OAL_STATIC OAL_INLINE oal_int32 oal_down_timeout(oal_semaphore_stru *sem, oal_int32 jiffies)
{
    return down_timeout(sem, jiffies);
}

OAL_STATIC OAL_INLINE oal_int32 oal_down_interruptible(oal_semaphore_stru *sem)
{
    return down_interruptible(sem);
}

OAL_STATIC OAL_INLINE oal_int32 oal_down_trylock(oal_semaphore_stru *sem)
{
    return down_trylock(sem);
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_completion.h */

