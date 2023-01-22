
#ifndef __OAL_LINUX_RW_LOCK_H__
#define __OAL_LINUX_RW_LOCK_H__


#include <linux/rwlock.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
typedef rwlock_t                oal_rwlock_stru;

/*
 * 功能描述  : 读写锁初始化，把读写锁设置为1（未锁状态）。
 * 输入参数  : pst_lock: 读写锁结构体地址
 */
OAL_STATIC OAL_INLINE oal_void  oal_rw_lock_init(oal_rwlock_stru *pst_lock)
{
    rwlock_init(pst_lock);
}

/*
 * 功能描述  : 获得指定的读锁
 * 输入参数  : pst_lock: 读写锁结构体地址
 */
OAL_STATIC OAL_INLINE oal_void  oal_rw_lock_read_lock(oal_rwlock_stru *pst_lock)
{
    read_lock(pst_lock);
}

OAL_STATIC OAL_INLINE oal_void  oal_rw_lock_read_unlock(oal_rwlock_stru *pst_lock)
{
    read_unlock(pst_lock);
}

OAL_STATIC OAL_INLINE oal_void  oal_rw_lock_write_lock(oal_rwlock_stru *pst_lock)
{
    write_lock(pst_lock);
}

/*
 * 函 数 名  : oal_rw_lock_write_unlock
 * 功能描述  : 释放指定的写锁
 */
OAL_STATIC OAL_INLINE oal_void  oal_rw_lock_write_unlock(oal_rwlock_stru *pst_lock)
{
    write_unlock(pst_lock);
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_rw_lock.h */

