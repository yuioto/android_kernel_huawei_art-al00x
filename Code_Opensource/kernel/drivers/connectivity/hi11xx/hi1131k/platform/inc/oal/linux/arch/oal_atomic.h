


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
 * ��������  : ��ȡԭ�ӱ�����ֵ
 * �������  : *p_vector: ��Ҫ����ԭ�Ӳ�����ԭ�ӱ�����ַ
 */
OAL_STATIC OAL_INLINE oal_int32  oal_atomic_read(oal_atomic *p_vector)
{
    return atomic_read(p_vector);
}

/*
 * ��������  :ԭ�ӵ�����ԭ�ӱ���p_vectorֵΪul_val
 * �������  : *p_vector: ��Ҫ����ԭ�Ӳ�����ԭ�ӱ�����ַ
 */
OAL_STATIC OAL_INLINE oal_void  oal_atomic_set(oal_atomic *p_vector, oal_int32 l_val)
{
    atomic_set(p_vector, l_val);
}

/*
 * ��������  :ԭ�ӵĸ���μ�1
 * �������  : *p_vector: ��Ҫ����ԭ�Ӳ�����ԭ�ӱ�����ַ
 */
OAL_STATIC OAL_INLINE oal_void oal_atomic_dec(oal_atomic *p_vector)
{
    atomic_dec(p_vector);
}

/*
 * ��������  :ԭ�ӵĸ���μ�һ
 * �������  : *p_vector: ��Ҫ����ԭ�Ӳ�����ԭ�ӱ�����ַ
 */
OAL_STATIC OAL_INLINE oal_void  oal_atomic_inc(oal_atomic *p_vector)
{
    atomic_inc(p_vector);
}

/*
 * ��������  :ԭ�ӵ����������Ƿ�Ϊ0
 * �������  : *p_vector: ��Ҫ����ԭ�Ӳ�����ԭ�ӱ�����ַ
 */
OAL_STATIC OAL_INLINE oal_int32  oal_atomic_inc_and_test(oal_atomic *p_vector)
{
    return atomic_inc_and_test(p_vector);
}

/*
 * ��������  :ԭ�ӵݼ��������Ƿ�Ϊ0
 * �������  : *p_vector: ��Ҫ����ԭ�Ӳ�����ԭ�ӱ�����ַ
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
 * ��������  : ԭ�ӵĶ�ĳ��λ������1�����������ظ�λ�õľ�ֵ��
 * �������  : *p_vector: ��Ҫ����ԭ�Ӳ�����ԭ�ӱ�����ַ
 *             nr: ��Ҫ���õ�λ
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
 * ��������  : ��װ��������ϵͳƽ̨�¶�ĳ��λ����ԭ����0����
 * �������  : *p_vector: ��Ҫ����ԭ�Ӳ�����ԭ�ӱ�����ַ
 *             nr: ��Ҫ�����λ
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

