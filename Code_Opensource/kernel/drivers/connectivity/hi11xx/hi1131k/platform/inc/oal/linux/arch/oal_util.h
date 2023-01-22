

#ifndef __OAL_LINUX_UTIL_H__
#define __OAL_LINUX_UTIL_H__

/*lint -e322*/
#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/kallsyms.h>
#include <asm/string.h>
#include <linux/thread_info.h>
#include <asm/byteorder.h>
#include <linux/byteorder/generic.h>
#include <linux/bitops.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <asm/delay.h>
#include <asm/memory.h>
#include <linux/kobject.h>      /* hi1102-cb add sys for 51/02 */
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/etherdevice.h>  /* hi1102-cb for random mac address */
/*lint +e322*/
#include "oal_file.h"
#include "oal_mm.h"
#include "oal_atomic.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
/* 32字节序大小端转换 */
#define OAL_SWAP_BYTEORDER_32(_val)        \
    ((((_val) & 0x000000FF) << 24) +     \
     (((_val) & 0x0000FF00) << 8) +       \
     (((_val) & 0x00FF0000) >> 8) +       \
     (((_val) & 0xFF000000) >> 24))


/* 获取CORE ID */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#define OAL_GET_CORE_ID()    (0)
#else
#define OAL_GET_CORE_ID()    smp_processor_id()
#endif


#define OAL_LIKELY(_expr)       likely(_expr)
#define OAL_UNLIKELY(_expr)     unlikely(_expr)
#define OAL_FUNC_NAME           __func__
#define OAL_RET_ADDR            __builtin_return_address(0)/*lint -e571 */

/* 内存读屏障 */
#define OAL_RMB()               rmb()

/* 内存写屏障 */
#define OAL_WMB()               wmb()

/* 内存屏障 */
#define OAL_MB()                mb()

#define OAL_OFFSET_OF          offsetof

#define __OAL_DECLARE_PACKED    __attribute__((__packed__))

#ifndef HISI_LOG_TAG
#define HISI_LOG_TAG
#endif

/*
    printk 打印级别lev <= KERN_INFO 时会输出到kmsg的ring buff中，
    同时输出到标准输出流中，根据hi113x 3516 linux版本维测需求，
    设置printk 打印lev为KERN_INFO
*/
#ifdef _HI113X_PRINTK_STDOUT
#define OAL_IO_PRINT(fmt, arg...)   \
    do {                            \
        printk(KERN_INFO HISI_LOG_TAG ""fmt, ##arg);\
    } while (0)
#else
#define OAL_IO_PRINT(fmt, arg...)   \
    do {                            \
        printk(KERN_DEBUG HISI_LOG_TAG ""fmt, ##arg);\
    } while (0)
#endif

#define OAL_IO_PRINT_ERR(fmt, arg...)

#define OAL_BUG_ON(_con)        BUG_ON(_con)

#define OAL_WARN_ON(condition)  WARN_ON(condition)

/* 虚拟地址转物理地址 */
#define OAL_VIRT_TO_PHY_ADDR(_virt_addr)            (virt_to_phys(_virt_addr) - 0x80000000)

/* 物理地址转虚拟地址 */
#define OAL_PHY_TO_VIRT_ADDR(_phy_addr)             phys_to_virt((_phy_addr) + 0x80000000)

#define OAL_PAGE_SIZE PAGE_SIZE

typedef struct device_attribute     oal_device_attribute_stru;
typedef struct device               oal_device_stru;
#define OAL_DEVICE_ATTR             DEVICE_ATTR
#define OAL_S_IRUGO                 S_IRUGO
#define OAL_S_IWGRP                 S_IWGRP
#define OAL_S_IWUSR                 S_IWUSR
typedef struct kobject              oal_kobject;
#define OAL_STRLEN                                  strlen
#define OAL_STRSTR                                  strstr
#define OAL_STRCMP                                  strcmp
#define OAL_STRCHR                                  strchr


/* #define oal_random_ether_addr(addr) random_ether_addr(addr) */
/* hi1102-cb for sys interface  51/02 */
/* #define random_ether_addr(addr) eth_random_addr(addr) */
/* static inline void eth_random_addr(u8 *addr) */
OAL_STATIC  OAL_INLINE void oal_random_ether_addr(oal_uint8 *addr)
{
    random_ether_addr(addr);
}



OAL_STATIC OAL_INLINE __attribute_const__ oal_uint16  oal_byteorder_host_to_net_uint16(oal_uint16 us_byte)
{
    return htons(us_byte);
}


OAL_STATIC OAL_INLINE __attribute_const__ oal_uint16  oal_byteorder_net_to_host_uint16(oal_uint16 us_byte)
{
    return ntohs(us_byte);
}


OAL_STATIC OAL_INLINE __attribute_const__ oal_uint32  oal_byteorder_host_to_net_uint32(oal_uint32 ul_byte)
{
    return htonl(ul_byte);
}


OAL_STATIC OAL_INLINE __attribute_const__ oal_uint32  oal_byteorder_net_to_host_uint32(oal_uint32 ul_byte)
{
    return ntohl(ul_byte);
}



OAL_STATIC OAL_INLINE oal_int32  oal_atoi(const char *c_string)
{
    oal_int32 l_ret = 0;
    oal_int32 flag = 0;

    for (; ; c_string++) {
        switch (*c_string) {
            case '0' ... '9':
                l_ret = 10 * l_ret + (*c_string - '0');

                break;
            case '-':
                flag = 1;
                break;

            case ' ':
                continue;

            default:

                return ((flag == 0) ? l_ret : (-l_ret));;
        }
    }
}

OAL_STATIC OAL_INLINE oal_void  oal_itoa(oal_int32 l_val, oal_int8 *c_string, oal_uint8 uc_strlen)
{
    if (snprintf_s(c_string, uc_strlen, uc_strlen - 1, "%d", l_val) < EOK) {
        printk("oal_itoa::snprintf_s failed!");
        return;
    }
}
OAL_STATIC OAL_INLINE oal_int8 *oal_strtok(oal_int8 *pc_token, OAL_CONST oal_int8 *pc_delemit, oal_int8 **ppc_context)
{
    static oal_uint8 *pc_nextoken = NULL;

    oal_uint8 *pc_str = NULL;
    OAL_CONST oal_uint8 *pc_ctrl = (oal_uint8 *)pc_delemit;
    const oal_uint32 ul_map_size = 32;
    oal_uint8 uc_map[ul_map_size]; /* 因编译原因，暂不支持用const变量定义数组大小 */
    oal_int32 l_count;

    if (OAL_UNLIKELY(pc_delemit == NULL)) {
        OAL_WARN_ON(1);
        return NULL;
    }

    /* Clear control map */
    for (l_count = 0; l_count < ul_map_size; l_count++) {
        uc_map[l_count] = 0;
    }

    /* Set bits in delimiter table */
    do {
        uc_map[*pc_ctrl >> 3] |= (1 << (*pc_ctrl & 7));
    } while (*pc_ctrl++);

    /*
     * Initialize str. If string is NULL, set str to the saved
     * pointer (i.e., continue breaking tokens out of the string
     * from the last strtok call)
     */
    if (pc_token != NULL) {
        pc_str = (oal_uint8 *)pc_token;
    } else {
        pc_str = pc_nextoken;
    }

    /*
     * Find beginning of token (skip over leading delimiters). Note that
     * there is no token iff this loop sets str to point to the terminal
     * null (*str == '\0')
     */
    while ((uc_map[*pc_str >> 3] & (1 << (*pc_str & 7))) && *pc_str) {
        pc_str++;
    }

    pc_token = (oal_int8 *)pc_str;

    /* Find the end of the token. If it is not the end of the string, put a null there. */
    for (; *pc_str; pc_str++) {
        if (uc_map[*pc_str >> 3] & (1 << (*pc_str & 7))) {
            *pc_str++ = '\0';
            break;
        }
    }

    /* Update nextoken (or the corresponding field in the per-thread data structure */
    pc_nextoken = pc_str;

    /* Determine if a token has been found. */
    if ((uintptr_t)pc_token == (uintptr_t)pc_str) {
        return NULL;
    } else {
        return pc_token;
    }
}


/* Works only for digits and letters, but small and fast */
#define TOLOWER(x) ((oal_uint8)(x) | 0x20)

#define isdigit(c) ('0' <= (c) && (c) <= '9')

#define isxdigit(c) (('0' <= (c) && (c) <= '9') \
                     || ('a' <= (c) && (c) <= 'f') \
                     || ('A' <= (c) && (c) <= 'F'))

#define OAL_ROUND_UP        round_up
#define OAL_ROUND_DOWN      round_down

OAL_STATIC OAL_INLINE unsigned int simple_guess_base(const char *cp)
{
    if (cp[0] == '0') {
        if (TOLOWER(cp[1]) == 'x' && isxdigit(cp[2]))
            return 16;
        else
            return 8;
    } else {
        return 10;
    }
}

OAL_STATIC OAL_INLINE unsigned long long oal_simple_strtoull(const oal_int8 *cp, oal_int8 **endp, unsigned int base)
{
    unsigned long long result = 0;

    if (base == 0)
        base = simple_guess_base(cp);

    if (base == 16 && cp[0] == '0' && TOLOWER(cp[1]) == 'x')
        cp += 2;

    while (isxdigit(*cp)) {
        unsigned int value;

        value = isdigit(*cp) ? *cp - '0' : TOLOWER(*cp) - 'a' + 10;
        if (value >= base)
            break;
        result = result * base + value;
        cp++;
    }
    if (endp != NULL)
        *endp = (oal_int8 *)cp;

    return result;
}


OAL_STATIC OAL_INLINE oal_long  oal_strtol(OAL_CONST oal_int8 *pc_nptr, oal_int8 **ppc_endptr, oal_int32 l_base)
{
    /* 跳过空格 */
    while (' ' == (*pc_nptr))
    {
        pc_nptr++;
    }

    if (*pc_nptr == '-')
        return -oal_simple_strtoull(pc_nptr + 1, ppc_endptr, l_base);

    return oal_simple_strtoull(pc_nptr, ppc_endptr, l_base);
}


OAL_STATIC OAL_INLINE oal_void  oal_udelay(oal_ulong u_loops)
{
    udelay(u_loops);
}


OAL_STATIC OAL_INLINE oal_void  oal_mdelay(oal_ulong u_loops)
{
    mdelay(u_loops);
}


OAL_STATIC OAL_INLINE oal_uint32  oal_kallsyms_lookup_name(OAL_CONST oal_uint8 *uc_var_name)
{
    return kallsyms_lookup_name(uc_var_name);
}



OAL_STATIC OAL_INLINE oal_void oal_dump_stack(oal_void)
{
    dump_stack();
}


OAL_STATIC OAL_INLINE oal_void  oal_msleep(oal_uint32 ul_usecs)
{
    msleep(ul_usecs);
}
OAL_STATIC OAL_INLINE oal_ulong oal_simple_strtoul(OAL_CONST oal_int8 *string, char **result, oal_uint32 base)
{
    return simple_strtoul(string, result, base);
}

OAL_STATIC OAL_INLINE oal_uint32 oal_strlen(const char *str)
{
    return strlen(str);
}

OAL_STATIC OAL_INLINE int oal_strncmp(const char *s1, const char *s2, size_t n)
{
    return strncmp(s1, s2, n);
}
OAL_STATIC OAL_INLINE oal_int oal_strncasecmp(OAL_CONST oal_int8 *p_buf1,
                                              OAL_CONST oal_int8 *p_buf2,
                                              oal_uint32 ul_count)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    return strncasecmp(p_buf1, p_buf2, ul_count);
#else
    return strncmp(p_buf1, p_buf2, ul_count); /* windows still use strncmp */
#endif
}


OAL_STATIC OAL_INLINE oal_void oal_print_hex_dump(const oal_uint8 *addr, oal_int32 len, oal_int32 groupsize,
                                                  const oal_int8 *pre_str)
{
#ifdef CONFIG_PRINTK
    OAL_REFERENCE(groupsize);
    printk(KERN_DEBUG"buf %p,len:%d\n",
           addr,
           len);
    print_hex_dump(KERN_DEBUG, pre_str, DUMP_PREFIX_ADDRESS, 16, 1, // dump 大小16
                   addr, len, true);
    printk(KERN_DEBUG"\n");
#endif
}
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_util.h */
