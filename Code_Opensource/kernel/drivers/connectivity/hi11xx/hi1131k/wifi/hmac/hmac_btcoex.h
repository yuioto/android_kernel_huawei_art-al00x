

#ifndef __HMAC_BTCOEX_H__
#define __HMAC_BTCOEX_H__

#ifdef _PRE_WLAN_FEATURE_1131K_BTCOEX
/* 1 其他头文件包含 */
#include "oal_ext_if.h"
#include "frw_ext_if.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_BTCOEX_H

/* 2 宏定义 */
#define MAX_BTCOEX_BSS_IN_BL 16

/* 3 枚举定义 */

/* 4 全局变量声明 */

/* 5 消息头定义 */

/* 6 消息定义 */

/* 7 STRUCT定义 */
typedef struct {
    uint8_t user_mac_addr[WLAN_MAC_ADDR_LEN];   /* 黑名单MAC地址 */
    uint8_t type;                               /* 写入黑名单的类型 */
    uint8_t used;                               /* 是否已经写过黑名单MAC地址 */
} hmac_btcoex_delba_exception_stru;

typedef struct {
    hmac_btcoex_delba_exception_stru hmac_btcoex_delba_exception[MAX_BTCOEX_BSS_IN_BL];
    uint8_t                          exception_bss_index;   /* 黑名单MAC地址的数组下标 */
    uint8_t                          resv[3];
} hmac_device_btcoex_stru;

typedef struct {
    frw_timeout_stru delba_opt_timer;       /* 发送ARP REQ后启动定时器 */
    oal_atomic       rx_unicast_pkt_to_lan; /* 接收到的单播帧个数 */
} hmac_btcoex_arp_req_process_stru;

typedef struct {
    uint16_t last_baw_start; /* 上一次接收到ADDBA REQ中的baw_start值 */
    uint16_t last_seq_num;   /* 上一次接收到ADDBA REQ中的seq_num值 */
    uint8_t  delba_enable;   /* 是否允许共存删建BA */
    uint8_t  resv[3];
} hmac_btcoex_addba_req_stru;

typedef struct {
    hmac_btcoex_arp_req_process_stru hmac_btcoex_arp_req_process;
    hmac_btcoex_addba_req_stru       hmac_btcoex_addba_req;
    uint8_t                          ba_size;
    uint8_t                          rx_no_pkt_count;   /* 超时时间内没有收到帧的次数 */
    uint8_t                          resv[2];
} hmac_vap_btcoex_stru;

/* 8 函数声明 */

#endif
#endif
