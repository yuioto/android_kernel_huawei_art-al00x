

#ifndef __HMAC_RESOURCE_H__
#define __HMAC_RESOURCE_H__


//  1 其他头文件包含
#include "oal_types.h"
#include "oal_queue.h"
#include "mac_resource.h"
#include "mac_device.h"
#include "hmac_device.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_RESOURCE_H


// 2 全局变量声明
extern uint8_t g_auc_hmac_macaddr[WLAN_MAC_ADDR_LEN];

// 3 STRUCT定义
/* 存储hmac device结构的资源结构体 */
typedef struct {
    hmac_device_stru ast_hmac_dev_info[MAC_RES_MAX_DEV_NUM];
    oal_queue_stru st_queue;
    oal_ulong aul_idx[MAC_RES_MAX_DEV_NUM];
    uint8_t auc_user_cnt[MAC_RES_MAX_DEV_NUM];
#ifdef _PRE_WLAN_FEATURE_DOUBLE_CHIP
    uint8_t auc_resv[2];   /* 单芯片下MAC_RES_MAX_DEV_NUM是1，双芯片下MAC_RES_MAX_DEV_NUM是2 */
#else
    uint8_t auc_resv[3];
#endif
} hmac_res_device_stru;

/* 存储hmac res资源结构体 */
typedef struct {
    hmac_res_device_stru st_hmac_dev_res;
} hmac_res_stru;

// 4 OTHERS定义
extern hmac_res_stru g_st_hmac_res;


// 5 函数声明
extern uint32_t hmac_res_alloc_mac_dev(uint32_t ul_dev_idx);
extern uint32_t hmac_res_free_mac_dev(uint32_t ul_dev_idx);
extern hmac_device_stru *hmac_res_get_mac_dev(uint32_t ul_dev_idx);
extern uint32_t hmac_res_init(void);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
extern uint32_t hmac_res_exit(mac_board_stru *pst_hmac_board);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of mac_resource.h */
