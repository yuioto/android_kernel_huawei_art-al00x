


#ifndef __OAL_LINUX_ATOMIC_H__
#define __OAL_LINUX_ATOMIC_H__

#include <asm/atomic.h>
#include <linux/bitops.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

typedef atomic_t                oal_atomic;
typedef oal_ulong    oal_bitops;
#define oal_atomic_inc_return   atomic_inc_return
#define        oal_bit_atomic_for_each_set(nr, p_addr, size)     for_each_set_bit(nr, p_addr, size)
/*
 * 功能描述  : 读取原子变量的值
 * 输入参数  : *p_vector: 需要进行原子操作的原子变量地址
 */
OAL_STATIC OAL_INLINE oal_int32  oal_atomic_read(oal_atomic *p_vector)
{
    return atomic_read(p_vector);
}

/*
 * 功能描述  :原子地设置原子变量p_vector值为ul_val
 * 输入参数  : *p_vector: 需要进行原子操作的原子变量地址
 */
OAL_STATIC OAL_INLINE oal_void  oal_atomic_set(oal_atomic *p_vector, oal_int32 l_val)
{
    atomic_set(p_vector, l_val);
}

/*
 * 功能描述  :原子的给入参减1
 * 输入参数  : *p_vector: 需要进行原子操作的原子变量地址
 */
OAL_STATIC OAL_INLINE oal_void oal_atomic_dec(oal_atomic *p_vector)
{
    atomic_dec(p_vector);
}

/*
 * 功能描述  :原子的给入参加一
 * 输入参数  : *p_vector: 需要进行原子操作的原子变量地址
 */
OAL_STATIC OAL_INLINE oal_void  oal_atomic_inc(oal_atomic *p_vector)
{
    atomic_inc(p_vector);
}

/*
 * 功能描述  :原子递增后检查结果是否为0
 * 输入参数  : *p_vector: 需要进行原子操作的原子变量地址
 */
OAL_STATIC OAL_INLINE oal_int32  oal_atomic_inc_and_test(oal_atomic *p_vector)
{
    return atomic_inc_and_test(p_vector);
}

/*
 * 功能描述  :原子递减后检查结果是否为0
 * 输入参数  : *p_vector: 需要进行原子操作的原子变量地址
 */
OAL_STATIC OAL_INLINE oal_int32  oal_atomic_dec_and_test(oal_atomic *p_vector)
{
    return atomic_dec_and_test(p_vector);
}

OAL_STATIC OAL_INLINE oal_void  oal_bit_atomic_set(oal_int32 nr, OAL_VOLATILE oal_bitops *p_addr)
{
    set_bit(nr, p_addr);
}

OAL_STATIC OAL_INLINE oal_int32  oal_bit_atomic_test(oal_int32 nr, const OAL_VOLATILE oal_bitops *p_addr)
{
    return test_bit(nr, p_addr);
}

/*
 * 功能描述  : 原子的对某个位进行置1操作，并返回该位置的旧值。
 * 输入参数  : *p_vector: 需要进行原子操作的原子变量地址
 *             nr: 需要设置的位
 */
OAL_STATIC OAL_INLINE oal_bitops  oal_bit_atomic_test_and_set(oal_int32 nr, OAL_VOLATILE oal_bitops *p_addr)
{
    return test_and_set_bit(nr, p_addr);
}


OAL_STATIC OAL_INLINE oal_bitops  oal_bit_atomic_test_and_clear(oal_int32 nr, OAL_VOLATILE oal_bitops *p_addr)
{
    return (oal_bitops)test_and_clear_bit(nr, p_addr);
}
/*
 * 功能描述  : 封装各个操作系统平台下对某个位进行原子清0操作
 * 输入参数  : *p_vector: 需要进行原子操作的原子变量地址
 *             nr: 需要清零的位
 */
OAL_STATIC OAL_INLINE oal_void  oal_bit_atomic_clear(oal_int32 nr, OAL_VOLATILE oal_bitops *p_addr)
{
    clear_bit(nr, p_addr);
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_atomic.h */

