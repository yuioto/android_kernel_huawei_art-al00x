

#ifndef __HMAC_BTCOEX_H__
#define __HMAC_BTCOEX_H__

#ifdef _PRE_WLAN_FEATURE_1131K_BTCOEX
/* 1 ����ͷ�ļ����� */
#include "oal_ext_if.h"
#include "frw_ext_if.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_BTCOEX_H

/* 2 �궨�� */
#define MAX_BTCOEX_BSS_IN_BL 16

/* 3 ö�ٶ��� */

/* 4 ȫ�ֱ������� */

/* 5 ��Ϣͷ���� */

/* 6 ��Ϣ���� */

/* 7 STRUCT���� */
typedef struct {
    uint8_t user_mac_addr[WLAN_MAC_ADDR_LEN];   /* ������MAC��ַ */
    uint8_t type;                               /* д������������� */
    uint8_t used;                               /* �Ƿ��Ѿ�д��������MAC��ַ */
} hmac_btcoex_delba_exception_stru;

typedef struct {
    hmac_btcoex_delba_exception_stru hmac_btcoex_delba_exception[MAX_BTCOEX_BSS_IN_BL];
    uint8_t                          exception_bss_index;   /* ������MAC��ַ�������±� */
    uint8_t                          resv[3];
} hmac_device_btcoex_stru;

typedef struct {
    frw_timeout_stru delba_opt_timer;       /* ����ARP REQ��������ʱ�� */
    oal_atomic       rx_unicast_pkt_to_lan; /* ���յ��ĵ���֡���� */
} hmac_btcoex_arp_req_process_stru;

typedef struct {
    uint16_t last_baw_start; /* ��һ�ν��յ�ADDBA REQ�е�baw_startֵ */
    uint16_t last_seq_num;   /* ��һ�ν��յ�ADDBA REQ�е�seq_numֵ */
    uint8_t  delba_enable;   /* �Ƿ�������ɾ��BA */
    uint8_t  resv[3];
} hmac_btcoex_addba_req_stru;

typedef struct {
    hmac_btcoex_arp_req_process_stru hmac_btcoex_arp_req_process;
    hmac_btcoex_addba_req_stru       hmac_btcoex_addba_req;
    uint8_t                          ba_size;
    uint8_t                          rx_no_pkt_count;   /* ��ʱʱ����û���յ�֡�Ĵ��� */
    uint8_t                          resv[2];
} hmac_vap_btcoex_stru;

/* 8 �������� */

#endif
#endif
