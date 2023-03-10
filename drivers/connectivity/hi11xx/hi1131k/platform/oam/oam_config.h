
#ifndef __OAM_CONFIG_H__
#define __OAM_CONFIG_H__

#include "oal_ext_if.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/* 配置项的值在配置文件中占的最大字节数 */
#define   OAM_CFG_VAL_MAX_LEN       20
/* 等于号(=)长度 */
#define   OAM_CFG_EQUAL_SIGN_LEN    1
/* '\0'长度 */
#define   OAM_CFG_STR_END_SIGN_LEN  1
typedef oal_uint32 (* p_oam_customize_init_func)(oal_void);
/* 待枚举值和BFGIN归一 */
/* primID pattern */
enum OM_MSG_MODULEID {
    OM_WIFI_HOST    = 0x00,
    OM_WIFI         = 0x01,
    OM_BT           = 0x02,
    OM_GNSS         = 0x03,
    OM_FM           = 0x04,
    OM_PF           = 0x05,
    OM_MODULEID_BUTT
};
typedef oal_uint8 oam_msg_moduleid_enum_uint8;

typedef struct {
    p_oam_customize_init_func customize_init[OM_MODULEID_BUTT];       /* 所有定制化的初始函数 */
} oam_customize_stru;

extern oam_customize_stru g_oam_customize;

extern oal_void  oam_register_init_hook(oam_msg_moduleid_enum_uint8 en_moduleid,  p_oam_customize_init_func p_func);
extern oal_int32  oam_cfg_get_one_item(
                                       oal_int8       *pc_cfg_data_buf,
                                       const oal_int8 *pc_section,
                                       const oal_int8 *pc_key,
                                       oal_int32      *pl_val);

extern oal_int32  oam_cfg_read_file_to_buf(
    oal_int8   *pc_cfg_data_buf,
    oal_uint32  ul_file_size);

extern oal_uint32  oam_cfg_decrypt_all_item(
    oal_aes_key_stru *pst_aes_key,
    oal_int8         *pc_ciphertext,
    oal_int8         *pc_plaintext,
    oal_uint32        ul_cipher_len);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oam_config.h */
