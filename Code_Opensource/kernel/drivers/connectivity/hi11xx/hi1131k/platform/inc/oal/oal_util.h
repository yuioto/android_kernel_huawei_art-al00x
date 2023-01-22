

#ifndef __OAL_UTIL_H__
#define __OAL_UTIL_H__

#include "platform_oneimage_define.h"
#include "oal_types.h"
#include "oal_mm.h"
#include "arch/oal_util.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
#define OAL_VA_START            va_start
#define OAL_VA_END              va_end

#define OAL_VA_LIST             va_list

#define OAL_MAC_ADDRESS_LAN 6

/* 拼接为16 bit或者 32bit */
#define OAL_MAKE_WORD16(lsb, msb) ((((oal_uint16)(msb) << 8) & 0xFF00) | (lsb))
#define OAL_MAKE_WORD32(lsw, msw) ((((oal_uint32)(msw) << 16) & 0xFFFF0000) | (lsw))


/* 计算为字节对齐后填充后的长度 */
#define OAL_ROUNDUP(_old_len, _align)  ((((_old_len) + ((_align) - 1)) / (_align)) * (_align))

/* increment with wrap-around */
#define OAL_INCR(_l, _sz)   (_l)++; (_l) &= ((_sz) - 1)
#define OAL_DECR(_l, _sz)   (_l)--; (_l) &= ((_sz) - 1)

/* 获取大小 */
#define OAL_SIZEOF                                  sizeof

/* 获取数组大小 */
#define OAL_ARRAY_SIZE(_ast_array)                  (sizeof(_ast_array) / sizeof((_ast_array)[0]))

/* 四字节对齐 */
#define OAL_GET_4BYTE_ALIGN_VALUE(_ul_size)         (((_ul_size) + 0x03) & (~0x03))

/* 获取当前线程信息 */
#define OAL_CURRENT_TASK     (current_thread_info()->task)

#define OAL_SWAP_BYTEORDER_16(_val) ((((_val) & 0x00FF) << 8) + (((_val) & 0xFF00) >> 8))

#if (_PRE_BIG_CPU_ENDIAN == _PRE_CPU_ENDIAN)          /* BIG_ENDIAN */
#define OAL_BYTEORDER_TO_LE32(_val)        OAL_SWAP_BYTEORDER_32(_val)
#define OAL_BYTEORDER_TO_LE16(_val)        OAL_SWAP_BYTEORDER_16(_val)
#define OAL_MASK_INVERSE(_len, _offset)    ((oal_uint32)(OAL_SWAP_BYTEORDER_32(~(((1 << (_len)) - 1) << (_offset)))))
#define OAL_MASK(_len, _offset)            ((oal_uint32)(OAL_SWAP_BYTEORDER_32(((1 << (_len)) - 1) << (_offset))))
#define OAL_NTOH_16(_val)                  (_val)
#define OAL_NTOH_32(_val)                  (_val)
#define OAL_HTON_16(_val)                  (_val)
#define OAL_HTON_32(_val)                  (_val)

#elif (_PRE_LITTLE_CPU_ENDIAN == _PRE_CPU_ENDIAN)     /* LITTLE_ENDIAN */
#define OAL_BYTEORDER_TO_LE32(_val)        (_val)
#define OAL_BYTEORDER_TO_LE16(_val)        (_val)
#define OAL_MASK_INVERSE(_len, _offset)    ((oal_uint32)(~(((1UL << (_len)) - 1) << (_offset))))
#define OAL_MASK(_len, _offset)            ((oal_uint32)(((1UL << (_len)) - 1) << (_offset)))
#define OAL_NTOH_16(_val)                  OAL_SWAP_BYTEORDER_16(_val)
#define OAL_NTOH_32(_val)                  OAL_SWAP_BYTEORDER_32(_val)
#define OAL_HTON_16(_val)                  OAL_SWAP_BYTEORDER_16(_val)
#define OAL_HTON_32(_val)                  OAL_SWAP_BYTEORDER_32(_val)

#endif

#define OAL_VALUE_EQ_ANY2(_value, _val0, _val1) \
    (((_val0) == (_value)) || ((_val1) == (_value)))
#define OAL_VALUE_EQ_ANY3(_value, _val0, _val1, _val2) \
    (((_val0) == (_value)) || ((_val1) == (_value)) || ((_val2) == (_value)))

#define OAL_VALUE_EQ_ANY4(_value, _val0, _val1, _val2, _val3) \
    (((_val0) == (_value)) || ((_val1) == (_value)) || ((_val2) == (_value)) || ((_val3) == (_value)))

#define OAL_VALUE_EQ_ALL2(_value, _val0, _val1) \
    (((_val0) == (_value)) && ((_val1) == (_value)))

#define OAL_VALUE_EQ_ALL3(_value0, _value1, _val0, _val1) \
    (((_val0) == (_value0)) && ((_val1) == (_value1)))

#define OAL_VALUE_IN_VALID_RANGE(_value, _start, _end)       (((_value) >= (_start)) && ((_value) <=(_end)))
#define OAL_VALUE_NOT_IN_VALID_RANGE(_value, _start, _end)   (((_value) < (_start)) || ((_value) > (_end)))
#define OAL_ANY_TRUE_VALUE2(_val0, _val1)                    (((_val0) == OAL_TRUE) || ((_val1) == OAL_TRUE))
#define OAL_VALUE_NE_ALL2(value, _val0, _val1)               (((_val0) != (value)) && ((_val1) != (value)))
#define OAL_ANY_ZERO_VALUE2(_val0, _val1)                    (((_val0) == 0) || ((_val1) == 0))
#define OAL_VALUE_IN_VALID_RANGE2(_value0, _value1, _start, _end) \
    (((_value0) >= (_start)) && ((_value0) <= (_end)) && (!(_value1)))

#define OAL_ANY_NULL_PTR1(_ptr1)                             (((_ptr1) == NULL))
#define OAL_ANY_NULL_PTR2(_ptr1, _ptr2)                      (((_ptr1) == NULL) || ((_ptr2) == NULL))
#define OAL_ANY_NULL_PTR3(_ptr1, _ptr2, _ptr3)               (((_ptr1) == NULL) || ((_ptr2) == NULL) || \
                                                              ((_ptr3) == NULL))
#define OAL_ANY_NULL_PTR4(_ptr1, _ptr2, _ptr3, _ptr4)        (((_ptr1) == NULL) || ((_ptr2) == NULL) || \
                                                              ((_ptr3) == NULL) || ((_ptr4) == NULL))
#define OAL_ANY_NULL_PTR5(_ptr1, _ptr2, _ptr3, _ptr4, _ptr5) (((_ptr1) == NULL) || ((_ptr2) == NULL) || \
                                                              ((_ptr3) == NULL) || ((_ptr4) == NULL) || \
                                                              ((_ptr5) == NULL))
#define OAL_ALL_NOT_NULL_PTR2(_ptr1, _ptr2)                   (((_ptr1) != NULL) && ((_ptr2) != NULL))
#define OAL_ALL_TRUE_VALUE2(_val0, _val1)                     (((_val0) == OAL_TRUE) && ((_val1) == OAL_TRUE))

#define OAL_ALL_ZERO_VALUE2(_val0, _val1)                     (((_val0) == 0) && ((_val1) == 0))
#define OAL_ALL_INI_VALUE(_value0, _value1)                   (((_value0) == OAL_SUCC) && ((_value1) != OAL_PTR_NULL))
#define OAL_ALL_INI_VALUE1(_value0, _value1)                  (((_value0) == OAL_FALSE) || ((_value1) == OAL_PTR_NULL))
#define OAL_ALL_INI_VALUE2(_value0, _value1, _value2)         (((_value0) == OAL_TRUE) && ((_value1) == (_value2)))

/* 比较宏 */
#define OAL_MIN(_A, _B) (((_A) < (_B)) ? (_A) : (_B))

/* 比较宏 */
#define OAL_MAX(_A, _B) (((_A) > (_B)) ? (_A) : (_B))

#define OAL_SUB(_A, _B) (((_A) > (_B)) ? ((_A) - (_B)) : (0))

#define OAL_ABSOLUTE_SUB(_A, _B) (((_A) > (_B)) ? ((_A) - (_B)) : ((_B) - (_A)))

/* 从某个设备读取某个寄存器地址的32-bit寄存器的值。 */
#define OAL_REG_READ32(_addr)    \
    *((OAL_VOLATILE oal_uint32 *)(_addr))

#define OAL_REG_READ16(_addr)    \
    *((OAL_VOLATILE oal_uint16 *)(_addr))

/* 往某个设备某个32-bit寄存器地址写入某个值 */
#define OAL_REG_WRITE32(_addr, _val)    \
    (*((OAL_VOLATILE oal_uint32 *)(_addr)) = (_val))
#define OAL_REG_WRITE16(_addr, _val)    \
    (*((OAL_VOLATILE oal_uint16 *)(_addr)) = (_val))

/* Is val aligned to "align" ("align" must be power of 2) */
#ifndef IS_ALIGNED
#define OAL_IS_ALIGNED(val, align) \
    (((oal_uint32)(val) & ((align) - 1)) == 0)
#else
#define OAL_IS_ALIGNED(val, align)  IS_ALIGNED(val, align)
#endif
#ifdef _PRE_CONFIG_HISI_PANIC_DUMP_SUPPORT
typedef  struct _hwifi_panic_log_   hwifi_panic_log;
typedef oal_int32 (* hwifi_panic_log_cb)(oal_void* data, char* pst_buf, oal_int32 buf_len);
struct _hwifi_panic_log_ {
    struct list_head list;
    /* the log module name */
    char* name;
    hwifi_panic_log_cb    cb;
    oal_void* data;
};
#define DECLARE_WIFI_PANIC_STRU(module_name, func) \
    hwifi_panic_log module_name = \
                                {\
                                .name = #module_name, \
                                .cb = (hwifi_panic_log_cb)func, \
                                }
#endif
#ifdef _PRE_CONFIG_HISI_PANIC_DUMP_SUPPORT

extern oal_void hwifi_panic_log_register(hwifi_panic_log* log, void* data);
extern oal_void hwifi_panic_log_unregister(hwifi_panic_log* log);
extern oal_void hwifi_panic_log_dump(char* print_level);
#else
OAL_STATIC OAL_INLINE oal_void hwifi_panic_log_dump(char* print_level)
{
}
#endif


OAL_STATIC OAL_INLINE oal_uint8  oal_strtohex(const oal_int8 *c_string)
{
    oal_uint8 uc_ret = 0;

    if (*c_string >= '0' && *c_string <= '9') {
        uc_ret = (oal_uint8)(*c_string - '0');
    } else if (*c_string >= 'A' && *c_string <= 'F') {
        uc_ret = (oal_uint8)(*c_string - 'A' + 10);
    } else if (*c_string >= 'a' && *c_string <= 'f') {
        uc_ret = (oal_uint8)(*c_string - 'a' + 10);
    }

    return uc_ret;
}


OAL_STATIC OAL_INLINE oal_void oal_strtoaddr(const char *pc_param,
    oal_uint8 uc_param_len, oal_uint8 *puc_mac_addr, oal_uint8 uc_mac_addr_len)
{
    oal_uint8 uc_char_index;
    const oal_uint32 uc_mac_max_num = 12; /* 单字节遍历是不是正确的mac地址:xx:xx:xx:xx:xx:xx，跳过数字间的12字符 */
    const char *pc_param_end = NULL;

    if (OAL_UNLIKELY((pc_param == NULL) || (puc_mac_addr == NULL))) {
        OAL_WARN_ON(1);
        return;
    }

    pc_param_end = pc_param + uc_param_len;

    /* 获取mac地址,16进制转换 */
    for (uc_char_index = 0; uc_char_index < uc_mac_max_num; uc_char_index++) {
        if (pc_param >= pc_param_end) {
            OAL_WARN_ON(1);
            return;
        }

        if ((*pc_param == ':') || (*pc_param == '-')) {
            pc_param++;
            if (uc_char_index != 0) {
                uc_char_index--;
            }

            continue;
        }
        if ((uc_char_index / 2) >= uc_mac_addr_len) {
            OAL_WARN_ON(1);
            return;
        }

        /* 下列计算是把字符串转换成的mac地址写到buff里，xx:xx 只对应两个mac地址,/2按16字节计算 */
        puc_mac_addr[uc_char_index / 2] =
            (oal_uint8)(puc_mac_addr[uc_char_index / 2] * 16 * (uc_char_index % 2) +
                        oal_strtohex(pc_param));
        pc_param++;
    }
}


OAL_STATIC OAL_INLINE oal_uint8  oal_get_random(oal_void)
{
    return 1;
}


OAL_STATIC OAL_INLINE oal_uint8  oal_gen_random(oal_uint32 ul_val, oal_uint8 us_rst_flag)
{
    static oal_uint32 ul_rand = 0;
    if (us_rst_flag != 0) {
        ul_rand = ul_val;
    }
    ul_rand = ul_rand * 1664525L + 1013904223L;
    return (oal_uint8) (ul_rand >> 24);
}


OAL_STATIC OAL_INLINE oal_uint8  oal_bit_get_num_one_byte(oal_uint8 uc_byte)
{
    uc_byte = (uc_byte & 0x55) + ((uc_byte >> 1) & 0x55);
    uc_byte = (uc_byte & 0x33) + ((uc_byte >> 2) & 0x33);
    uc_byte = (uc_byte & 0x0F) + ((uc_byte >> 4) & 0x0F);

    return uc_byte;
}


OAL_STATIC OAL_INLINE oal_uint32  oal_bit_get_num_four_byte(oal_uint32 ul_byte)
{
    ul_byte = (ul_byte & 0x55555555) + ((ul_byte >>  1) & 0x55555555);
    ul_byte = (ul_byte & 0x33333333) + ((ul_byte >>  2) & 0x33333333);
    ul_byte = (ul_byte & 0x0F0F0F0F) + ((ul_byte >>  4) & 0x0F0F0F0F);
    ul_byte = (ul_byte & 0x00FF00FF) + ((ul_byte >>  8) & 0x00FF00FF);
    ul_byte = (ul_byte & 0x0000FFFF) + ((ul_byte >> 16) & 0x0000FFFF);

    return ul_byte;
}


OAL_STATIC OAL_INLINE oal_void  oal_bit_set_bit_one_byte(oal_uint8 *puc_byte, oal_bitops nr)
{
    *puc_byte |= ((oal_uint8)(1 << nr));
}


OAL_STATIC OAL_INLINE oal_void  oal_bit_clear_bit_one_byte(oal_uint8 *puc_byte, oal_bitops nr)
{
    *puc_byte &= (~((oal_uint8)(1 << nr)));
}


OAL_STATIC OAL_INLINE oal_void  oal_bit_set_bit_four_byte(oal_uint32 *pul_byte, oal_bitops nr)
{
    *pul_byte |= ((oal_uint32)(1 << nr));
}


OAL_STATIC OAL_INLINE oal_void  oal_bit_clear_bit_four_byte(oal_uint32 *pul_byte, oal_bitops nr)
{
    *pul_byte &= ~((oal_uint32)(1 << nr));
}
OAL_STATIC OAL_INLINE oal_void  oal_bit_set_bit_eight_byte(oal_uint64 *pull_byte, oal_bitops nr)
{
    *pull_byte |= ((oal_uint64)1 << nr);
}
OAL_STATIC OAL_INLINE oal_void  oal_bit_clear_bit_eight_byte(oal_uint64 *pull_byte, oal_bitops nr)
{
    *pull_byte &= ~((oal_uint64)1 << nr);
}


OAL_STATIC OAL_INLINE oal_uint8  oal_bit_find_first_bit_one_byte(oal_uint8 uc_byte)
{
    oal_uint8 uc_ret = 0;

    uc_byte = uc_byte & (oal_uint8)(-uc_byte);

    while (uc_byte != 1) {
        uc_ret++;
        uc_byte = (uc_byte >> 1);

        if (uc_ret > 7) {
            return uc_ret;
        }
    }

    return uc_ret;
}


OAL_STATIC OAL_INLINE oal_uint8  oal_bit_find_first_zero_one_byte(oal_uint8 uc_byte)
{
    oal_uint8 uc_ret = 0;

    uc_byte = ~uc_byte;
    uc_byte = uc_byte & (-uc_byte);

    while (uc_byte != 1) {
        uc_ret++;
        uc_byte = (uc_byte >> 1);

        if (uc_ret > 7) {
            return uc_ret;
        }
    }

    return uc_ret;
}


OAL_STATIC OAL_INLINE oal_uint8  oal_bit_find_first_bit_four_byte(oal_uint32 ul_byte)
{
    oal_uint8 uc_ret = 0;

    if (ul_byte == 0) {
        return uc_ret;
    }

    if (!(ul_byte & 0xffff)) {
        ul_byte >>= 16;
        uc_ret += 16;
    }

    if (!(ul_byte & 0xff)) {
        ul_byte >>= 8;
        uc_ret += 8;
    }

    if (!(ul_byte & 0xf)) {
        ul_byte >>= 4;
        uc_ret += 4;
    }

    if (!(ul_byte & 3)) {
        ul_byte >>= 2;
        uc_ret += 2;
    }

    if (!(ul_byte & 1)) {
        uc_ret += 1;
    }

    return uc_ret;
}


OAL_STATIC OAL_INLINE oal_uint8  oal_bit_find_first_zero_four_byte(oal_uint32 ul_byte)
{
    oal_uint8 uc_ret = 0;

    ul_byte = ~ul_byte;

    if (!(ul_byte & 0xffff)) {
        ul_byte >>= 16;
        uc_ret += 16;
    }

    if (!(ul_byte & 0xff)) {
        ul_byte >>= 8;
        uc_ret += 8;
    }

    if (!(ul_byte & 0xf)) {
        ul_byte >>= 4;
        uc_ret += 4;
    }

    if (!(ul_byte & 3)) {
        ul_byte >>= 2;
        uc_ret += 2;
    }

    if (!(ul_byte & 1)) {
        ul_byte >>= 1;
        uc_ret += 1;
    }

    return uc_ret;
}


OAL_STATIC OAL_INLINE oal_void  oal_set_mac_addr(unsigned char *puc_mac_addr1, const unsigned char *puc_mac_addr2)
{
    if (memcpy_s(puc_mac_addr1, OAL_MAC_ADDRESS_LAN, puc_mac_addr2, OAL_MAC_ADDRESS_LAN) != EOK) {
        OAL_IO_PRINT("oal_set_mac_addr: memcpy_s failed.\n");
    }
}


OAL_STATIC OAL_INLINE oal_void  oal_set_mac_addr_zero(oal_uint8 *puc_mac_addr)
{
    puc_mac_addr[0] = 0;
    puc_mac_addr[1] = 0;
    puc_mac_addr[2] = 0;
    puc_mac_addr[3] = 0;
    puc_mac_addr[4] = 0;
    puc_mac_addr[5] = 0;
}


OAL_STATIC OAL_INLINE oal_uint32  oal_compare_mac_addr(const unsigned char *puc_mac_addr1,
                                                       const unsigned char *puc_mac_addr2)

{
    return (puc_mac_addr1[0] ^ puc_mac_addr2[0]) | (puc_mac_addr1[1] ^ puc_mac_addr2[1])
           | (puc_mac_addr1[2] ^ puc_mac_addr2[2]) | (puc_mac_addr1[3] ^ puc_mac_addr2[3])
           | (puc_mac_addr1[4] ^ puc_mac_addr2[4]) | (puc_mac_addr1[5] ^ puc_mac_addr2[5]);
}


OAL_STATIC OAL_INLINE oal_void  oal_set_ip_addr(oal_uint8 *puc_ip_addr1, const oal_uint8 *puc_ip_addr2)
{
    puc_ip_addr1[0] = puc_ip_addr2[0];
    puc_ip_addr1[1] = puc_ip_addr2[1];
    puc_ip_addr1[2] = puc_ip_addr2[2];
    puc_ip_addr1[3] = puc_ip_addr2[3];
}


OAL_STATIC OAL_INLINE oal_bool_enum_uint8  oal_cmp_seq_num(
    oal_uint32   ul_seq_num1,
    oal_uint32   ul_seq_num2,
    oal_uint32   ul_diff_value)
{
    if (((ul_seq_num1 < ul_seq_num2) && ((ul_seq_num2 - ul_seq_num1) < ul_diff_value))
        || ((ul_seq_num1 > ul_seq_num2) && ((ul_seq_num1 - ul_seq_num2) > ul_diff_value))) {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}


OAL_STATIC OAL_INLINE oal_int32 oal_strcmp(const char *pc_src, const char *pc_dst)
{
    oal_int8  c_c1;
    oal_int8  c_c2;
    oal_int32 l_ret;

    do {
        c_c1 = *pc_src++;
        c_c2 = *pc_dst++;
        l_ret = c_c1 - c_c2;
        if (l_ret) {
            break;
        }
    } while (c_c1);

    return l_ret;
}


OAL_STATIC OAL_INLINE oal_int8 *oal_strim(oal_int8 *pc_s)
{
    oal_uint32 ul_size;
    oal_int8  *pc_end = OAL_PTR_NULL;

    while (*pc_s == ' ') {
        ++pc_s;
    }

    ul_size = OAL_STRLEN(pc_s);
    if (!ul_size) {
        return pc_s;
    }

    pc_end = pc_s + ul_size - 1;
    while (pc_end >= pc_s && *pc_end == ' ') {
        pc_end--;
    }

    *(pc_end + 1) = '\0';

    return pc_s;
}


OAL_STATIC OAL_INLINE oal_int8*  oal_strstr(oal_int8 *pc_s1, const oal_int8 *pc_s2)
{
    return OAL_STRSTR(pc_s1, pc_s2);
}



OAL_STATIC OAL_INLINE oal_uint32  oal_init_lut(oal_uint8  *puc_lut_index_table, oal_uint8 uc_bmap_len)
{
    oal_uint8   uc_lut_idx;

    for (uc_lut_idx = 0; uc_lut_idx < uc_bmap_len; uc_lut_idx++) {
        puc_lut_index_table[uc_lut_idx] = 0;
    }

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE oal_uint8  oal_get_lut_index(
    oal_uint8      *puc_lut_index_table,
    oal_uint8       uc_bmap_len,
    oal_uint16      us_max_lut_size)
{
    oal_uint8       uc_byte;
    oal_uint8       uc_bit;
    oal_uint8       uc_temp;
    oal_uint16      us_index;

    for (uc_byte = 0; uc_byte < uc_bmap_len; uc_byte++) {
        uc_temp = puc_lut_index_table[uc_byte];

        for (uc_bit = 0; uc_bit < 8; uc_bit++) {
            if ((uc_temp & (1 << uc_bit)) == 0x0) {
                us_index = (uc_byte * 8 + uc_bit); // bit to byte / 8 
                if (us_index < us_max_lut_size) {
                    puc_lut_index_table[uc_byte] |= (oal_uint8)(1 << uc_bit);

                    return (oal_uint8)us_index;
                } else {
                    return (oal_uint8)us_max_lut_size;
                }
            }
        }
    }

    return (oal_uint8)us_max_lut_size;
}


OAL_STATIC OAL_INLINE oal_void  oal_del_lut_index(oal_uint8 *puc_lut_index_table, oal_uint8 uc_idx)
{
    oal_uint8 uc_byte = uc_idx / 8; // bit to byte / 8
    oal_uint8 uc_bit  = uc_idx % 8; // bit to byte / 8

    puc_lut_index_table[uc_byte] &= ~(oal_uint8)(1 << uc_bit);
}


OAL_STATIC OAL_INLINE oal_uint32* oal_get_virt_addr(oal_uint32 *pul_phy_addr)
{
    /* 空指针无需转化 */
    if (pul_phy_addr == OAL_PTR_NULL) {
        return pul_phy_addr;
    }

    return (oal_uint32 *)OAL_PHY_TO_VIRT_ADDR((oal_ulong)pul_phy_addr);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_util.h */
