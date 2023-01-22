

#ifndef __OAL_LINUX_TIME_H__
#define __OAL_LINUX_TIME_H__

#include <linux/jiffies.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/rtc.h>
#include <oal_types.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
#if (_PRE_CHIP_BITS_MIPS32 == _PRE_CHIP_BITS)
/* 32位寄存器最大长度 */
#define OAL_TIME_US_MAX_LEN  (0xFFFFFFFF - 1)

#elif (_PRE_CHIP_BITS_MIPS64 == _PRE_CHIP_BITS)
/* 64位寄存器最大长度 */
#define OAL_TIME_US_MAX_LEN  (0xFFFFFFFFFFFFFFFF - 1)

#endif


/* 获取毫秒级时间戳 */
#define OAL_TIME_GET_STAMP_MS() jiffies_to_msecs(jiffies)

/* 获取高精度毫秒时间戳,精度1ms */
#define OAL_TIME_GET_HIGH_PRECISION_MS()  oal_get_time_stamp_from_timeval()

#define OAL_ENABLE_CYCLE_COUNT()
#define OAL_DISABLE_CYCLE_COUNT()
#define OAL_GET_CYCLE_COUNT() 0

/* 寄存器反转模块运行时间计算 */
#define OAL_TIME_CALC_RUNTIME(_ul_start, _ul_end) \
    /*lint -e(573) */((((OAL_TIME_US_MAX_LEN) / HZ) * 1000) + ((OAL_TIME_US_MAX_LEN) % HZ) * (1000 / HZ) - \
    (_ul_start) + (_ul_end))

#define OAL_TIME_JIFFY    jiffies

#define OAL_TIME_HZ       HZ

#define OAL_MSECS_TO_JIFFIES(_msecs)    msecs_to_jiffies(_msecs)

#define OAL_JIFFIES_TO_MSECS(_jiffies)      jiffies_to_msecs(_jiffies)

/* 获取从_ul_start到_ul_end的时间差 */
#define OAL_TIME_GET_RUNTIME(_ul_start, _ul_end) \
    (((_ul_start) > (_ul_end)) ? (OAL_TIME_CALC_RUNTIME((_ul_start), (_ul_end))) : ((_ul_end) - (_ul_start)))

typedef struct {
    oal_long i_sec;
    oal_long i_usec;
} oal_time_us_stru;

typedef ktime_t oal_time_t_stru;
typedef struct timeval oal_timeval_stru;
typedef struct rtc_time oal_rtctime_stru;

OAL_STATIC OAL_INLINE oal_void  oal_time_get_stamp_us(oal_time_us_stru *pst_usec)
{
    struct timespec ts;

    getnstimeofday(&ts);

    pst_usec->i_sec     = ts.tv_sec;

    pst_usec->i_usec    = ts.tv_nsec / 1000; // us转 ns，除1000
}


OAL_STATIC OAL_INLINE oal_time_t_stru oal_ktime_get(oal_void)
{
    return ktime_get();
}

OAL_STATIC OAL_INLINE oal_time_t_stru oal_ktime_sub(const oal_time_t_stru lhs, const oal_time_t_stru rhs)
{
    return ktime_sub(lhs, rhs);
}

OAL_STATIC OAL_INLINE oal_uint64 oal_get_time_stamp_from_timeval(oal_void)
{
    struct timeval tv;
    oal_uint64 curr_time;

    do_gettimeofday(&tv);
    curr_time = tv.tv_usec;
    do_div(curr_time, 1000);
    curr_time = curr_time + tv.tv_sec * 1000;

    return curr_time;
}

OAL_STATIC OAL_INLINE oal_void oal_do_gettimeofday(oal_timeval_stru *tv)
{
    do_gettimeofday(tv);
}
OAL_STATIC OAL_INLINE oal_void oal_rtc_time_to_tm(oal_ulong time, oal_rtctime_stru *tm)
{
    rtc_time_to_tm(time, tm);
}

OAL_STATIC OAL_INLINE oal_uint32 oal_time_is_before(oal_ulong ui_time)
{
    return (oal_uint32)time_is_before_jiffies(ui_time);
}


OAL_STATIC OAL_INLINE oal_uint32 oal_time_after(oal_uint32 ul_time_a, oal_uint32 ul_time_b)
{
    return (oal_uint32)time_after((oal_ulong)ul_time_a, (oal_ulong)ul_time_b);
}

OAL_STATIC OAL_INLINE oal_ulong oal_ktime_to_us(const oal_time_t_stru kt)
{
    return ktime_to_us(kt);
}

OAL_STATIC OAL_INLINE oal_uint32 oal_time_before_eq(oal_uint32 ul_time_a, oal_uint32 ul_time_b)
{
    return (oal_uint32)time_before_eq((oal_ulong)ul_time_a, (oal_ulong)ul_time_b);
}

OAL_STATIC OAL_INLINE oal_uint32 oal_time_before(oal_uint32 ul_time_a, oal_uint32 ul_time_b)
{
    return (oal_uint32)time_before((oal_ulong)ul_time_a, (oal_ulong)ul_time_b);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_time.h */
