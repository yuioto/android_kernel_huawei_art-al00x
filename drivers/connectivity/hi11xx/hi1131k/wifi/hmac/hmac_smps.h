

#ifndef __HMAC_SMPS_H__
#define __HMAC_SMPS_H__


#ifdef _PRE_WLAN_FEATURE_SMPS

// 1 ����ͷ�ļ�����
#include "oal_ext_if.h"
#include "hmac_main.h"
#include "oam_ext_if.h"
#include "mac_resource.h"
#include "dmac_ext_if.h"
#include "mac_device.h"
#include "mac_vap.h"
#include "mac_user.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_SMPS_H

// 2 ��������
extern oal_uint32 hmac_smps_update_status(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user,
    oal_bool_enum_uint8 en_plus_user);
extern oal_uint32 hmac_smps_user_asoc_update(oal_uint8 uc_prev_smps_mode, mac_user_stru *pst_mac_user,
    mac_vap_stru *pst_mac_vap);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of _PRE_WLAN_FEATURE_SMPS */

#endif /* end of hmac_smps.h */
