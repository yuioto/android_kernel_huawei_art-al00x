
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
 * ��������  : ��д����ʼ�����Ѷ�д������Ϊ1��δ��״̬����
 * �������  : pst_lock: ��д���ṹ���ַ
 */
OAL_STATIC OAL_INLINE oal_void  oal_rw_lock_init(oal_rwlock_stru *pst_lock)
{
    rwlock_init(pst_lock);
}

/*
 * ��������  : ���ָ���Ķ���
 * �������  : pst_lock: ��д���ṹ���ַ
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
 * �� �� ��  : oal_rw_lock_write_unlock
 * ��������  : �ͷ�ָ����д��
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

