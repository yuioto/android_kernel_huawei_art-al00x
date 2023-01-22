
#ifndef __OAL_LINUX_INTERRUPT_H__
#define __OAL_LINUX_INTERRUPT_H__

#include <linux/interrupt.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
OAL_STATIC OAL_INLINE oal_int32 oal_request_irq(oal_uint32 irq, irq_handler_t handler, oal_ulong flags,
                                                const oal_int8 *name, oal_void *dev)
{
    return request_irq(irq, handler, flags, name, dev);
}

OAL_STATIC OAL_INLINE oal_void oal_free_irq(oal_uint32 irq, oal_void *dev)
{
    free_irq(irq, dev);
}

OAL_STATIC OAL_INLINE oal_void oal_enable_irq(oal_uint32 irq)
{
    enable_irq(irq);
}

OAL_STATIC OAL_INLINE oal_void oal_disable_irq(oal_uint32 irq)
{
    disable_irq(irq);
}

OAL_STATIC OAL_INLINE oal_void oal_disable_irq_nosync(oal_uint32 irq)
{
    disable_irq_nosync(irq);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_completion.h */

