
#ifndef __OAL_LINUX_MUTEX_H__
#define __OAL_LINUX_MUTEX_H__

#include <linux/mutex.h>
#include "oal_types.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
typedef struct mutex          oal_mutex_stru;

#define    OAL_MUTEX_INIT(mutex)        mutex_init(mutex)
#define    OAL_MUTEX_DESTROY(mutex)        mutex_destroy(mutex)

OAL_STATIC OAL_INLINE oal_void oal_mutex_lock(oal_mutex_stru *lock)
{
    mutex_lock(lock);
}

OAL_STATIC OAL_INLINE oal_int32 oal_mutex_trylock(oal_mutex_stru *lock)
{
    return mutex_trylock(lock);
}

OAL_STATIC OAL_INLINE oal_void oal_mutex_unlock(oal_mutex_stru *lock)
{
    mutex_unlock(lock);
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_mutex.h */

