
#include "oam_config.h"
#include "oal_aes.h"
#include "oal_file.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
oam_customize_stru g_oam_customize;

oal_void  oam_register_init_hook(oam_msg_moduleid_enum_uint8 en_moduleid,  p_oam_customize_init_func p_func)
{
    g_oam_customize.customize_init[en_moduleid] = p_func;
}


oal_int32  oam_cfg_get_one_item(
                                oal_int8       *pc_cfg_data_buf,
                                const oal_int8 *pc_section,
                                const oal_int8 *pc_key,
                                oal_int32      *pl_val)
{
    /* ****************************配置文件内容格式**************************** */
    /*                              [section]                                 */
    /*                              key等于val                                   */
    /* ************************************************************************ */
    oal_int8        *pc_section_addr;
    oal_int8        *pc_key_addr = OAL_PTR_NULL;
    oal_int8        *pc_val_addr = OAL_PTR_NULL;
    oal_int8        *pc_equal_sign_addr = OAL_PTR_NULL;    /* 等号的地址 */
    oal_int8        *pc_tmp = OAL_PTR_NULL;
    oal_uint8        uc_key_len;
    oal_int8         ac_val[OAM_CFG_VAL_MAX_LEN] = { 0 };  /* 暂存配置项的值 */
    oal_uint8        uc_index = 0;

    /* 查找section在配置文件中的位置 */
    pc_section_addr = oal_strstr(pc_cfg_data_buf, pc_section);
    if (pc_section_addr == OAL_PTR_NULL) {
        OAL_IO_PRINT("oam_cfg_get_one_item::section not found!\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 从section所在位置开始查找配置项对应的字符串 */
    pc_key_addr = oal_strstr(pc_section_addr, pc_key);
    if (pc_key_addr == OAL_PTR_NULL) {
        OAL_IO_PRINT("oam_cfg_get_one_item::key not found!\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 查找配置项的值 */
    uc_key_len  = (oal_uint8)OAL_STRLEN(pc_key);

    /* 检查key后面是否紧接着'=',如果不是的话，那当前要查找的key有可能是另一个
       key的前缀，需要继续往后查找
    */
    pc_equal_sign_addr = pc_key_addr + uc_key_len;
    while (*(pc_equal_sign_addr) != '=') {
        pc_key_addr = oal_strstr(pc_equal_sign_addr, pc_key);
        if (pc_key_addr == OAL_PTR_NULL) {
            OAL_IO_PRINT("oam_cfg_get_one_item::key not found!\n");
            return OAL_ERR_CODE_PTR_NULL;
        }

        pc_equal_sign_addr = pc_key_addr + uc_key_len;
    }

    /* 检查val是否存在 */
    pc_val_addr = pc_equal_sign_addr + OAM_CFG_EQUAL_SIGN_LEN;
    if ((*(pc_val_addr) == '\n') || (*(pc_val_addr) == '\0')) {
        return OAL_FAIL;
    }

    for (pc_tmp = pc_val_addr; (*pc_tmp != '\n') && (*pc_tmp != '\0'); pc_tmp++) {
        ac_val[uc_index] = *pc_tmp;
        uc_index++;
    }

    /* 获取出来的有可能不是10进制的数，需要switch-case一下 TBD */
    *pl_val = oal_atoi(ac_val);

    return OAL_SUCC;
}



oal_int32  oam_cfg_read_file_to_buf(
    oal_int8   *pc_cfg_data_buf,
    oal_uint32  ul_file_size)
{
    oal_file_stru     *p_file;
    oal_int32          l_ret;

    p_file = oal_file_open(OAL_CFG_FILE_PATH, (OAL_O_RDONLY), 0);
    if (p_file == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    l_ret = oal_file_read(p_file, pc_cfg_data_buf, ul_file_size);
    if (l_ret <= 0) {
        oal_file_close(p_file);
        return OAL_FAIL;
    }

    oal_file_close(p_file);

    return OAL_SUCC;
}


oal_uint32  oam_cfg_decrypt_all_item(
    oal_aes_key_stru *pst_aes_key,
    oal_int8         *pc_ciphertext,
    oal_int8         *pc_plaintext,
    oal_uint32        ul_cipher_len)
{
#if 0
    oal_uint32      ul_loop  = 0;
    oal_uint32      ul_round;
    oal_uint8      *puc_cipher_tmp;
    oal_uint8      *puc_plain_tmp;

    /* AES加密块的大小是16字节，如果密文长度不是16的倍数，则不能正确解密 */
    if ((ul_cipher_len % OAL_AES_BLOCK_SIZE) != 0) {
        OAL_IO_PRINT("oam_cfg_decrypt_all_item::ciphertext length invalid!\n");
        return OAL_FAIL;
    }

    if (ul_cipher_len == 0) {
        OAL_IO_PRINT("oam_cfg_decrypt_all_item::ciphertext length is 0!\n");
        return OAL_FAIL;
    }

    ul_round = (ul_cipher_len >> 4);
    puc_cipher_tmp = (oal_uint8 *)pc_ciphertext;
    puc_plain_tmp  = (oal_uint8 *)pc_plaintext;

    while (ul_loop < ul_round) {
        oal_aes_decrypt(pst_aes_key, puc_plain_tmp, puc_cipher_tmp);

        ul_loop++;
        puc_cipher_tmp += OAL_AES_BLOCK_SIZE;
        puc_plain_tmp  += OAL_AES_BLOCK_SIZE;
    }
#endif

    return OAL_SUCC;
}


/*lint -e19*/
oal_module_symbol(oam_register_init_hook);
oal_module_symbol(oam_cfg_get_one_item);
oal_module_symbol(oam_cfg_read_file_to_buf);
oal_module_symbol(oam_cfg_decrypt_all_item);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

