

#ifndef __OAL_LINUX_COMPLETION_H__
#define __OAL_LINUX_COMPLETION_H__

#include <linux/completion.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
typedef struct completion           oal_completion;

#define OAL_INIT_COMPLETION(_my_completion) init_completion(_my_completion)

#define OAL_COMPLETE(_my_completion)        complete(_my_completion)

#define OAL_WAIT_FOR_COMPLETION(_my_completion)        wait_for_completion(_my_completion)

#define OAL_COMPLETE_ALL(_my_completion)        complete_all(_my_completion)


OAL_STATIC OAL_INLINE oal_uint32  oal_wait_for_completion_timeout(oal_completion *pst_completion, oal_uint32 ul_timeout)
{
    return wait_for_completion_timeout(pst_completion, ul_timeout);
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_completion.h */

