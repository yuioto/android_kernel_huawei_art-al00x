

#ifndef __HMAC_DATA_ACQ_H__
#define __HMAC_DATA_ACQ_H__


#ifdef _PRE_WLAN_FEATURE_DAQ

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_types.h"
#include "mac_vap.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_DAQ
#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_DATA_ACQ_H

/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint8 g_uc_data_acq_used;
extern oal_void  hmac_data_acq_init(oal_void);
extern oal_void  hmac_data_acq_exit(oal_void);
extern oal_void  hmac_data_acq_down_vap(mac_vap_stru *pst_mac_vap);

#endif  /* end of _PRE_WLAN_FEATURE_DAQ */


#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif

#endif /* end of hmac_data_acq.h */
