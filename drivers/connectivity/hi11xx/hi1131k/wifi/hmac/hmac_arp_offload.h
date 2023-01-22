
#ifndef __HMAC_ARP_OFFLOAD_H__
#define __HMAC_ARP_OFFLOAD_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "mac_vap.h"
#include "dmac_ext_if.h"
#include "hmac_vap.h"
#include "hmac_config.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_ARP_OFFLOAD_H


#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD

/*****************************************************************************
  1 函数声明
*****************************************************************************/
extern oal_uint32 hmac_arp_offload_set_ip_addr(mac_vap_stru            *pst_mac_vap,
                                               dmac_ip_type_enum_uint8 en_type,
                                               dmac_ip_oper_enum_uint8 en_oper,
                                               const void              *pst_ip_addr);
extern oal_uint32 hmac_arp_offload_enable(mac_vap_stru *pst_mac_vap, oal_switch_enum_uint8 en_switch);
#endif


#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif

#endif /* end of hmac_arp_offload.h */
