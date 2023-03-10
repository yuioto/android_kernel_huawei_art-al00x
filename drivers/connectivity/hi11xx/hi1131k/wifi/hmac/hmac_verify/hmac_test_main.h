

#ifndef __HMAC_TEST_MAIN_H__
#define __HMAC_TEST_MAIN_H__


// 1 其他头文件包含
#include "oal_ext_if.h"
#include "frw_ext_if.h"
#include "hal_ext_if.h"
#ifdef _PRE_WLAN_ALG_ENABLE
#include "alg_ext_if.h"
#endif
#include "hmac_ext_if.h"
#include "hmac_vap.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_TEST_MAIN_H

// 2 宏定义
/* 用于芯片验证调用前，检查开关 */
#ifdef _PRE_WLAN_CHIP_TEST
#define HMAC_CHIP_TEST_CALL(func_call) do{\
    if(0 != hmac_test_get_chip_test())\
    {\
        (func_call);\
    }\
}while (0);
#else
#define HMAC_CHIP_TEST_CALL(func_call)
#endif

// 3 STRUCT定义
/*
 * 结构说明:以下为测试的主管理结构，用于统一管理所有测试中用到的各种控制开关，各测试特性自行
 * 根据需要在其中添加子结构成员或者是变量
 */
typedef struct dmac_test_mng {
    oal_uint8   uc_hmac_chip_test;
    oal_uint8   auc_pad[3];
} hmac_test_mng_stru;

// 4 OTHERS定义
extern hmac_test_mng_stru  g_st_hmac_test_mng;

// 5 函数声明
extern oal_void     hmac_test_init(oal_void);
extern oal_void hmac_test_set_chip_test(oal_uint8 uc_chip_test_open);
extern oal_uint32 hmac_test_get_chip_test(oal_void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of __HMAC_TEST_MAIN_H__ */

