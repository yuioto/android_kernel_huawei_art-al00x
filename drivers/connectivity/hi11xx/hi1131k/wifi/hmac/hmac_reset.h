
#ifndef __HMAC_RESET_H__
#define __HMAC_RESET_H__

// 1 其他头文件包含
#include "mac_vap.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_RESET_H

// 2 函数声明
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
extern uint32_t hmac_reset_sys_event(mac_vap_stru *pst_mac_vap, uint8_t uc_len, uint8_t *puc_param);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of hmac_reset.h */
