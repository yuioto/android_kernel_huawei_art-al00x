

#ifndef __OAL_LINUX_TIMER_H__
#define __OAL_LINUX_TIMER_H__

#include <linux/timer.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

typedef struct timer_list              oal_timer_list_stru;

typedef void (*oal_timer_func)(oal_ulong);
/*
 * ��������  : ��ʼ����ʱ��
 */
OAL_STATIC OAL_INLINE oal_void  oal_timer_init(oal_timer_list_stru *pst_timer, oal_uint32 ul_delay,
                                               oal_timer_func p_func, oal_ulong ui_arg)
{
    init_timer(pst_timer);
    pst_timer->expires = jiffies + msecs_to_jiffies(ul_delay);
    pst_timer->function = p_func;
    pst_timer->data = ui_arg;
}

/*
 * ��������  : ɾ����ʱ��
 */
OAL_STATIC OAL_INLINE oal_int32  oal_timer_delete(oal_timer_list_stru *pst_timer)
{
    return del_timer(pst_timer);
}

/*
 * ��������  : ͬ��ɾ����ʱ�������ڶ��
 */
OAL_STATIC OAL_INLINE oal_int32  oal_timer_delete_sync(oal_timer_list_stru *pst_timer)
{
    return del_timer_sync(pst_timer);
}

/*
 * ��������  : ���ʱ��
 */
OAL_STATIC OAL_INLINE oal_void  oal_timer_add(oal_timer_list_stru *pst_timer)
{
    add_timer(pst_timer);
}

/*
 * ��������  : ������ʱ��
 * �������  : pst_timer: �ṹ��ָ��
 *            ui_expires: ����������¼�
 */
OAL_STATIC OAL_INLINE oal_int32  oal_timer_start(oal_timer_list_stru *pst_timer, oal_ulong ui_delay)
{
    return mod_timer(pst_timer, (jiffies + msecs_to_jiffies(ui_delay)));
}

/*
 * ��������  : ָ��cpu,������ʱ��,����ʱtimerҪ���ڷǼ���״̬���߻�����
 * �������  : pst_timer: �ṹ��ָ��
 *            ui_expires: ����������¼�
 */
OAL_STATIC OAL_INLINE oal_void  oal_timer_start_on(oal_timer_list_stru *pst_timer, oal_ulong ui_delay, oal_int32 cpu)
{
    pst_timer->expires = jiffies + msecs_to_jiffies(ui_delay);
    add_timer_on(pst_timer, cpu);
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_timer.h */

