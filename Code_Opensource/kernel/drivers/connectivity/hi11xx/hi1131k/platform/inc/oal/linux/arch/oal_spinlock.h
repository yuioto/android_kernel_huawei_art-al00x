

#ifndef __OAL_LINUX_SPINLOCK_H__
#define __OAL_LINUX_SPINLOCK_H__

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAL_SPINLOCK_H

#include <linux/spinlock.h>
#include "oal_util.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
/*****************************************************************************
  2 宏定义
*****************************************************************************/

#define OAL_SPIN_LOCK_MAGIC_TAG 0xdead4ead
typedef struct _oal_spin_lock_stru_ {
#ifdef CONFIG_SPIN_LOCK_MAGIC_DEBUG
    oal_uint32  magic;
    oal_uint32  reserved;
#endif
    spinlock_t  lock;
} oal_spin_lock_stru;

#ifdef CONFIG_SPIN_LOCK_MAGIC_DEBUG
#define OAL_DEFINE_SPINLOCK(x)  oal_spin_lock_stru x = {\
    .magic = OAL_SPIN_LOCK_MAGIC_TAG, \
    .lock = __SPIN_LOCK_UNLOCKED(x) }
#else
#define OAL_DEFINE_SPINLOCK(x)  oal_spin_lock_stru x = {\
    .lock = __SPIN_LOCK_UNLOCKED(x) }
#endif

/* 函数指针，用来指向需要自旋锁保护的的函数 */
typedef oal_uint32              (*oal_irqlocked_func)(oal_void *);

OAL_STATIC OAL_INLINE oal_void  oal_spin_lock_init(oal_spin_lock_stru *pst_lock)
{
    spin_lock_init(&pst_lock->lock);
#ifdef CONFIG_SPIN_LOCK_MAGIC_DEBUG
    pst_lock->magic = OAL_SPIN_LOCK_MAGIC_TAG;
#endif
}

OAL_STATIC OAL_INLINE oal_void  oal_spin_lock_magic_bug(oal_spin_lock_stru *pst_lock)
{
#ifdef CONFIG_SPIN_LOCK_MAGIC_DEBUG
    if (OAL_UNLIKELY((oal_uint32)OAL_SPIN_LOCK_MAGIC_TAG != pst_lock->magic)) {
#ifdef CONFIG_PRINTK
        /* spinlock never init or memory overwrite? */
        printk(KERN_EMERG "[E]SPIN_LOCK_BUG: spinlock:%p on CPU#%d, %s,magic:%08x should be %08x\n", pst_lock,
               raw_smp_processor_id(), current->comm, pst_lock->magic, OAL_SPIN_LOCK_MAGIC_TAG);
        print_hex_dump(KERN_EMERG, "spinlock_magic: ", DUMP_PREFIX_ADDRESS, 16, 1, // 长度16
                       (oal_uint8 *)((uintptr_t)pst_lock - 32), 32 + sizeof(oal_spin_lock_stru) + 32, true); // 32偏移
        printk(KERN_EMERG"\n");
#endif
        OAL_WARN_ON(1);
    }
#endif
}


OAL_STATIC OAL_INLINE oal_void  oal_spin_lock(oal_spin_lock_stru *pst_lock)
{
    oal_spin_lock_magic_bug(pst_lock);
    spin_lock(&pst_lock->lock);
}


OAL_STATIC OAL_INLINE oal_void  oal_spin_unlock(oal_spin_lock_stru *pst_lock)
{
    oal_spin_lock_magic_bug(pst_lock);
    spin_unlock(&pst_lock->lock);
}


OAL_STATIC OAL_INLINE oal_void oal_spin_lock_bh(oal_spin_lock_stru *pst_lock)
{
    oal_spin_lock_magic_bug(pst_lock);
    spin_lock_bh(&pst_lock->lock);
}


OAL_STATIC OAL_INLINE oal_void oal_spin_unlock_bh(oal_spin_lock_stru *pst_lock)
{
    oal_spin_lock_magic_bug(pst_lock);
    spin_unlock_bh(&pst_lock->lock);
}


OAL_STATIC OAL_INLINE oal_void  oal_spin_lock_irq_save(oal_spin_lock_stru *pst_lock, oal_ulong *pui_flags)
{
    oal_spin_lock_magic_bug(pst_lock);
    spin_lock_irqsave(&pst_lock->lock, *pui_flags);
}


OAL_STATIC OAL_INLINE oal_void  oal_spin_unlock_irq_restore(oal_spin_lock_stru *pst_lock, const oal_ulong *pui_flags)
{
    oal_spin_lock_magic_bug(pst_lock);
    spin_unlock_irqrestore(&pst_lock->lock, *pui_flags);
}


OAL_STATIC OAL_INLINE oal_uint32  oal_spin_lock_irq_exec(oal_spin_lock_stru *pst_lock, oal_irqlocked_func func,
                                                         oal_void *p_arg, oal_ulong *pui_flags)
{
    oal_uint32  ul_rslt;

    spin_lock_irqsave(&pst_lock->lock, *pui_flags);

    ul_rslt = func(p_arg);

    spin_unlock_irqrestore(&pst_lock->lock, *pui_flags);

    return ul_rslt;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_spinlock.h */

