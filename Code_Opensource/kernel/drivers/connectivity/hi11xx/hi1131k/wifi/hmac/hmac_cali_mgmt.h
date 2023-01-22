
#ifndef __HMAC_CALI_MGMT_H__
#define __HMAC_CALI_MGMT_H__

#if(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "frw_ext_if.h"
#include "dmac_ext_if.h"
#include "hmac_vap.h"
#include "plat_cali.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CALI_MGMT_H

/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint32  hmac_save_cali_event(frw_event_mem_stru *pst_event_mem);
extern oal_uint32 hmac_send_cali_data(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif

#endif /* end of hmac_mgmt_classifier.h */


