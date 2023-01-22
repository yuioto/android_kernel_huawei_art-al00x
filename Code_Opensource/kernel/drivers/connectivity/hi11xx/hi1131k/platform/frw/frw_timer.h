
#ifndef __FRW_TIMER_H__
#define __FRW_TIMER_H__

#include "frw_ext_if.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_FRW_TIMER_H

#define FRW_TIME_UL_MAX 0xFFFFFFFF
#define FRW_TIMER_MAX_TIMEOUT (FRW_TIME_UL_MAX >> 1) /* 32位最大值的1/2 */

extern oal_uint32 frw_timer_timeout_proc(frw_event_mem_stru *pst_timeout_event);
extern oal_void frw_timer_init(oal_uint32 ul_delay, oal_timer_func p_func, oal_ulong ui_arg);
extern oal_void frw_timer_exit(oal_void);
extern oal_void frw_timer_timeout_proc_event(oal_ulong ui_arg);
oal_uint32 frw_timer_get_cpu_count(oal_uint32 configNrCpus);

/* return true if the time a is after time b,in case of overflow and wrap around to zero */
OAL_STATIC OAL_INLINE oal_int32 frw_time_after(oal_uint32 a, oal_uint32 b)
{
    return ((oal_int32)((b) - (a)) <= 0);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of frw_timer.h */
