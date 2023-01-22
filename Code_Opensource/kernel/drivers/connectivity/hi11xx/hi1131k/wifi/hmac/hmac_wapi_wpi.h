

#include "oam_wdk.h"
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_WAPI_WPI_H
#ifdef _PRE_WLAN_FEATURE_WAPI


// ��������
extern oal_uint32  hmac_wpi_encrypt(const oal_uint8 *puc_iv, oal_uint8 *puc_bufin, oal_uint32 ul_buflen,
                                    const oal_uint8 *puc_key, oal_uint8* puc_bufout);

extern oal_uint32  hmac_wpi_decrypt(const oal_uint8* puc_iv, oal_uint8* puc_bufin, oal_uint32 ul_buflen,
                                    const oal_uint8* puc_key, oal_uint8* puc_bufout);

extern void hmac_wpi_swap_pn(oal_uint8 *puc_pn, oal_uint8 uc_len);

extern oal_uint32  hmac_wpi_pmac(const oal_uint8* puc_iv, oal_uint8* puc_buf, oal_uint32 ul_pamclen,
                                 const oal_uint8* puc_key, oal_uint8* puc_mic, oal_uint8  uc_mic_len);

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif



