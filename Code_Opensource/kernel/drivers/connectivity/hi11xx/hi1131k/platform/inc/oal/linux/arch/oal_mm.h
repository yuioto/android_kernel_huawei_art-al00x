
#ifndef __OAL_LINUX_MM_H__
#define __OAL_LINUX_MM_H__

/*lint -e322*/
#include <linux/slab.h>
#include <linux/hardirq.h>
#include <linux/vmalloc.h>
/*lint +e322*/
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
#define OAL_MEMCMP                                  memcmp

#define OAL_GFP_KERNEL                          GFP_KERNEL
#define OAL_GFP_ATOMIC                          GFP_ATOMIC

/*
 * 功能描述  : 申请核心态的内存空间，并填充0
 *          对于Linux操作系统而言，需要考虑中断上下文和内核上下文的不同情况(GFP_KERNEL和GFP_ATOMIC)。
 */
OAL_STATIC OAL_INLINE oal_void* oal_memalloc(oal_uint32 ul_size)
{
    oal_int32   l_flags = GFP_KERNEL;
    oal_void   *puc_mem_space = OAL_PTR_NULL;

    /* 不睡眠或在中断程序中标志置为GFP_ATOMIC */
    if (in_interrupt() || irqs_disabled()) {
        l_flags = GFP_ATOMIC;
    }

    if (unlikely(ul_size == 0)) {
        return OAL_PTR_NULL;
    }

    puc_mem_space = kmalloc(ul_size, l_flags);
    if (puc_mem_space == OAL_PTR_NULL) {
        return OAL_PTR_NULL;
    }

    return puc_mem_space;
}


OAL_STATIC OAL_INLINE oal_void*  oal_kzalloc(oal_uint32 ul_size, oal_int32 l_flags)
{
    return kzalloc(ul_size, l_flags);
}

OAL_STATIC OAL_INLINE oal_void*  oal_vmalloc(oal_uint32 ul_size)
{
    return vmalloc(ul_size);
}

OAL_STATIC OAL_INLINE oal_void  oal_free(const oal_void *p_buf)
{
    if (p_buf == NULL) {
        return;
    }
    kfree(p_buf);
}

OAL_STATIC OAL_INLINE oal_void  oal_vfree(const oal_void *p_buf)
{
    vfree(p_buf);
}

OAL_STATIC OAL_INLINE oal_long  oal_memcmp(const void *p_buf1, const void *p_buf2, oal_uint32 ul_count)
{
    return memcmp(p_buf1, p_buf2, ul_count);
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_mm.h */

