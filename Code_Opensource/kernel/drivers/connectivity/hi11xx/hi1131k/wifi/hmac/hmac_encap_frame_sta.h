
#ifndef __PREPARE_FRAME_STA_H__
#define __PREPARE_FRAME_STA_H__


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "oal_types.h"
#include "hmac_vap.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint32 hmac_mgmt_encap_asoc_req_sta(hmac_vap_stru *pst_hmac_sta, oal_uint8 *puc_req_frame);
extern oal_uint16  hmac_mgmt_encap_auth_req(hmac_vap_stru *pst_sta, oal_uint8 *puc_mgmt_frame);
extern oal_uint16  hmac_mgmt_encap_auth_req_seq3(hmac_vap_stru *pst_sta, oal_uint8 *puc_mgmt_frame,
                                                 oal_uint8 *puc_mac_hrd);
extern oal_uint16  hmac_encap_2040_coext_mgmt(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_buffer, oal_uint8 uc_coext_info,
                                              oal_uint32 ul_chan_report);


#ifdef _PRE_WLAN_FEATURE_SMPS
extern oal_uint32  hmac_mgmt_encap_smps_sta(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_buffer);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of hmac_encap_frame_sta.h */
