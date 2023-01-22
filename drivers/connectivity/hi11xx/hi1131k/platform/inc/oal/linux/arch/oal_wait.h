

#ifndef __OAL_LINUX_WAIT_H__
#define __OAL_LINUX_WAIT_H__

#include <linux/wait.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
typedef wait_queue_entry_t oal_wait_queue_stru;
#else
typedef wait_queue_t oal_wait_queue_stru;
#endif
typedef wait_queue_head_t    oal_wait_queue_head_stru;
#define OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(_pst_wq)     wake_up_interruptible(_pst_wq)

#define OAL_WAIT_QUEUE_WAKE_UP(_pst_wq)     wake_up(_pst_wq)

#define OAL_INTERRUPTIBLE_SLEEP_ON(_pst_wq) interruptible_sleep_on(_pst_wq)

#define OAL_WAIT_QUEUE_INIT_HEAD(_pst_wq)   init_waitqueue_head(_pst_wq)

#define OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(_st_wq, _condition, _timeout) \
    wait_event_interruptible_timeout(_st_wq, _condition, _timeout)

#define OAL_WAIT_EVENT_TIMEOUT(_st_wq, _condition, _timeout) \
    wait_event_timeout(_st_wq, _condition, _timeout)

#define OAL_WAIT_EVENT_INTERRUPTIBLE(_st_wq, _condition)\
    wait_event_interruptible(_st_wq, _condition)
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_wait.h */

