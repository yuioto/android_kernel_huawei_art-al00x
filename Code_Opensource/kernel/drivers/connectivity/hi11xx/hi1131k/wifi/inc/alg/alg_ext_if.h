

#ifndef __ALG_EXT_IF_H__
#define __ALG_EXT_IF_H__

// 1 其他头文件包含
#include "oal_types.h"
#include "hal_ext_if.h"
#include "dmac_ext_if.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_ALG_EXT_IF_H

// 10 函数声明
extern oal_int32  alg_main_init(oal_void);
#ifdef _PRE_1131C_DEV_FUNCTION_NOT_USED
extern oal_void  alg_main_exit(oal_void);
#endif  /* _PRE_1131C_DEV_FUNCTION_NOT_USED */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of alg_ext_if.h */
