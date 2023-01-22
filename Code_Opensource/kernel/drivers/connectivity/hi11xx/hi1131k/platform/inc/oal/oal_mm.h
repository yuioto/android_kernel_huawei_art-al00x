

#ifndef __OAL_MM_H__
#define __OAL_MM_H__

#include "oal_types.h"
#include "arch/oal_mm.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if _PRE_TEST_MODE_UT == _PRE_TEST_MODE
/* ƒ⁄¥Ê«Â¡„ */
#define OAL_MEMZERO(_p_buf, _ul_size)          memset_s((_p_buf), (_ul_size), 0, (_ul_size))
#endif
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_mm.h */

