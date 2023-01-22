
#ifndef __HAL_HI1102_CALI_DPD_H__
#define __HAL_HI1102_CALI_DPD_H__

#ifdef _PRE_WLAN_RF_110X_CALI_DPD

/*****************************************************************************
  1 ����ͷ�ļ�����
*****************************************************************************/
#include "oal_ext_if.h"
#include "wlan_types.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CALI_DPD_H

/*****************************************************************************
  2 �궨��
*************************************************************************/
#define DPD_CALI_LUT_LENGTH 128

/*****************************************************************************
  7 STRUCT����
*****************************************************************************/
/* �����ṹ */
typedef struct {
    oal_int64 ll_real;
    oal_int64 ll_imag;
} hi1102_complex_stru;

/*****************************************************************************
  10 ��������
*****************************************************************************/
extern oal_uint32 hmac_rf_cali_dpd_corr_calc(const oal_uint32* hi1102_dpd_cali_data_read,
                                             oal_uint32* hi1102_dpd_cali_data_calc);
#endif

#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif

#endif /* end of hal_hi1102_cali_dpd.h */
