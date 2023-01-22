

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_net.h"
#include "oal_types.h"
#include "oam_ext_if.h"
#include "mac_vap.h"
#include "mac_resource.h"
#include "hmac_vap.h"
#include "hmac_auto_adjust_freq.h"
#include "hmac_ext_if.h"
#include "hmac_blockack.h"

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "plat_pm_wlan.h"
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include <linux/pm_qos.h>
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_AUTO_ADJUST_FREQ_C
/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
#ifndef WIN32
OAL_STATIC oal_uint32 g_pre_jiffies            = 0;
OAL_STATIC oal_uint32 g_adjust_count            = 0;

/* 由定制化进行初始化 */
host_speed_freq_level_stru g_host_speed_freq_level[] = {
    /* pps门限                   CPU主频下限                     DDR主频下限 */
    {PPS_VALUE_0,          CPU_MIN_FREQ_VALUE_0,            DDR_MIN_FREQ_VALUE_0},
    {PPS_VALUE_1,          CPU_MIN_FREQ_VALUE_1,            DDR_MIN_FREQ_VALUE_1},
    {PPS_VALUE_2,          CPU_MIN_FREQ_VALUE_2,            DDR_MIN_FREQ_VALUE_2},
    {PPS_VALUE_3,          CPU_MIN_FREQ_VALUE_3,            DDR_MIN_FREQ_VALUE_3},
};
host_speed_freq_level_stru g_host_no_ba_freq_level[] = {
    /* pps门限                        CPU主频下限                      DDR主频下限 */
    {NO_BA_PPS_VALUE_0,          CPU_MIN_FREQ_VALUE_0,            DDR_MIN_FREQ_VALUE_0},
    {NO_BA_PPS_VALUE_1,          CPU_MIN_FREQ_VALUE_1,            DDR_MIN_FREQ_VALUE_1},
    {NO_BA_PPS_VALUE_2,          CPU_MIN_FREQ_VALUE_2,            DDR_MIN_FREQ_VALUE_2},
    {NO_BA_PPS_VALUE_3,          CPU_MIN_FREQ_VALUE_2,            DDR_MIN_FREQ_VALUE_2},
};
device_speed_freq_level_stru g_device_speed_freq_level[] = {
    /* device主频类型 */
    {FREQ_IDLE},
    {FREQ_MIDIUM},
    {FREQ_HIGHER},
    {FREQ_HIGHEST},
};

struct pm_qos_request *g_pst_wifi_auto_ddr = NULL;


freq_lock_control_stru g_freq_lock_control = {0};
OAL_STATIC oal_uint32 g_ul_wifi_rxtx_total          = 0;

OAL_STATIC oal_uint32 g_ul_orig_cpu_min_freq       = 0;
OAL_STATIC oal_uint32 g_ul_orig_cpu_max_freq       = 0;
OAL_STATIC oal_uint32 g_ul_orig_ddr_min_freq       = 0;
OAL_STATIC oal_uint32 g_ul_orig_ddr_max_freq       = 0;
#else
oal_uint32 g_pre_jiffies;

host_speed_freq_level_stru g_host_speed_freq_level[] = {
    /* pps门限                   CPU主频下限                     DDR主频下限 */
    {PPS_VALUE_0,          CPU_MIN_FREQ_VALUE_0,            DDR_MIN_FREQ_VALUE_0},
    {PPS_VALUE_1,          CPU_MIN_FREQ_VALUE_1,            DDR_MIN_FREQ_VALUE_1},
    {PPS_VALUE_2,          CPU_MIN_FREQ_VALUE_2,            DDR_MIN_FREQ_VALUE_2},
    {PPS_VALUE_3,          CPU_MIN_FREQ_VALUE_3,            DDR_MIN_FREQ_VALUE_3},
};
device_speed_freq_level_stru g_device_speed_freq_level[] = {
    /* device主频类型 */
    {FREQ_IDLE},
    {FREQ_MIDIUM},
    {FREQ_HIGHEST},
    {FREQ_HIGHEST},
};

freq_lock_control_stru g_freq_lock_control;
oal_uint32 g_ul_wifi_rxtx_total;

oal_uint32 g_ul_orig_cpu_min_freq;
oal_uint32 g_ul_orig_cpu_max_freq;
oal_uint32 g_ul_orig_ddr_min_freq;
oal_uint32 g_ul_orig_ddr_max_freq;
#endif
#ifdef WIN32
#define mutex_init(mux)
#define mutex_lock(mux)
#define mutex_unlock(mux)
#define spin_lock_init(mux)
#define mutex_destroy(mux)
#define spin_unlock_bh(mux)
#endif


oal_uint8 hmac_set_auto_freq_mod(oal_uint8 uc_freq_enable)
{
    if (g_freq_lock_control.uc_lock_mod == uc_freq_enable) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_set_auto_freq_mod, set the same value[%d]}", uc_freq_enable);
        return OAL_SUCC;
    }

    g_freq_lock_control.uc_lock_mod = uc_freq_enable;
    if (uc_freq_enable == FREQ_LOCK_ENABLE) {
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_set_auto_freq_mod  set host enable[%d], set device enable level[%d]}",
            g_freq_lock_control.uc_lock_mod, g_freq_lock_control.uc_curr_lock_level);

        hmac_set_device_freq(g_freq_lock_control.uc_curr_lock_level);
    } else {
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_set_auto_freq_mod  set host enable[%d], set device diable level[%d]}",
            g_freq_lock_control.uc_lock_mod, FREQ_HIGHEST);

        hmac_set_device_freq(FREQ_HIGHEST);
    }
    return OAL_SUCC;
}

oal_bool_enum_uint8 hmac_set_auto_freq_debug_print(oal_bool_enum_uint8 en_debug_print)
{
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_set_auto_freq_debug_print en_debug_print = %d!}",
        g_freq_lock_control.en_debug_print);
    g_freq_lock_control.en_debug_print = en_debug_print;
    return 0;
}


oal_bool_enum_uint8 hmac_set_auto_freq_bypass_device_auto_freq(oal_bool_enum_uint8 en_bypass_device)
{
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_set_auto_freq_debug_print en_bypass_device_freq = %d!}",
        g_freq_lock_control.en_bypass_device_freq);
    g_freq_lock_control.en_bypass_device_freq = en_bypass_device;
    return 0;
}


oal_int32 hmac_set_auto_freq_process_func(oal_void)
{
    struct alg_process_func_handler* pst_alg_process_func_handler;

    g_freq_lock_control.uc_lock_mod = FREQ_LOCK_DISABLE;
    g_freq_lock_control.en_debug_print = OAL_FALSE;
    g_freq_lock_control.en_bypass_device_freq = OAL_FALSE;
    g_freq_lock_control.uc_curr_lock_level = 0;
    pst_alg_process_func_handler = oal_get_alg_process_func();
    if (pst_alg_process_func_handler == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_set_auto_freq_process_func get handler failed!}");
    } else {
        pst_alg_process_func_handler->p_auto_freq_count_func = hmac_hcc_auto_freq_count;
        pst_alg_process_func_handler->p_auto_freq_process_func = hmac_hcc_auto_freq_process;
        pst_alg_process_func_handler->p_auto_freq_set_lock_mod_func = hmac_set_auto_freq_mod;
        pst_alg_process_func_handler->p_auto_freq_adjust_to_level_func = hmac_adjust_freq_to_level;
    }

    return OAL_SUCC;
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 hmac_get_cpu_freq_raw(oal_uint8 uc_freq_type, oal_uint32 *pst_ul_freq_value)
{
    return 0;
}


oal_bool_enum_uint8 hmac_set_cpu_freq_raw(oal_uint8 uc_freq_type, oal_uint32 ul_freq_value)
{
    return 0;
}


OAL_STATIC OAL_INLINE oal_bool_enum_uint8 hmac_get_ddr_freq_raw(oal_uint8 uc_freq_type, oal_uint32 *pst_ul_freq_value)
{
    struct file* filp = NULL;
    mm_segment_t old_fs;
    oal_int8 buf[12] = {0};

    if (uc_freq_type == SCALING_MAX_FREQ)
        filp = filp_open(DDR_MAX_FREQ, O_RDONLY, 0);
    else if (uc_freq_type == SCALING_MIN_FREQ)
        filp = filp_open(DDR_MIN_FREQ, O_RDONLY, 0);
    else
        return OAL_EFAIL;

    if (IS_ERR_OR_NULL(filp)) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_get_ddr_freq_raw:freq = %d error !}", uc_freq_type);
        return OAL_EFAIL;
    }
    old_fs = get_fs();  //lint !e63
    set_fs(KERNEL_DS);  //lint !e501
    filp->f_pos = 0;    //lint !e613
    filp->f_op->read(filp, buf, 12, &filp->f_pos); //lint !e613
    filp_close(filp, NULL); //lint !e613 !e668
    set_fs(old_fs);

    if (kstrtouint(buf, 10, pst_ul_freq_value) != 0) {
        printk("error to get cpu freq\n");
        return OAL_EFAIL;
    }

    return 0;
}


oal_bool_enum_uint8 hmac_set_ddr_freq_raw(oal_uint8 uc_freq_type, oal_uint32 ul_freq_value)
{
/* HI1131C modify begin */
#if defined(CONFIG_ARCH_HI3650)
    pm_qos_update_request(g_pst_wifi_auto_ddr, ul_freq_value);
#else
    /* HI3798MV2X don't support android lower power */
#endif
    return 0;
}
#endif
void hmac_adjust_freq_to_level(void)
{
    uint8_t req_lock_level = g_freq_lock_control.uc_req_lock_level;

    OAM_WARNING_LOG1(0, OAM_SF_PWR, "{hmac_adjust_freq_to_level[%d]}", req_lock_level);
    if (hmac_is_device_ba_setup()) {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        hmac_set_cpu_freq_raw(SCALING_MIN_FREQ, g_host_speed_freq_level[req_lock_level].ul_min_cpu_freq);
        hmac_set_ddr_freq_raw(SCALING_MIN_FREQ, g_host_speed_freq_level[req_lock_level].ul_min_ddr_freq);
#endif
    } else {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        hmac_set_cpu_freq_raw(SCALING_MIN_FREQ, g_host_no_ba_freq_level[req_lock_level].ul_min_cpu_freq);
        hmac_set_ddr_freq_raw(SCALING_MIN_FREQ, g_host_no_ba_freq_level[req_lock_level].ul_min_ddr_freq);
#endif
    }
    if (g_freq_lock_control.en_bypass_device_freq == OAL_FALSE) {
        hmac_set_device_freq(g_device_speed_freq_level[req_lock_level].uc_device_type);
    }
    g_freq_lock_control.uc_curr_lock_level = req_lock_level;
}


void hmac_perform_calc_rwtotal_throughput(oal_uint32 ul_rxtx_total, oal_uint32 ul_sdio_dur_us)
{
    if (ul_sdio_dur_us != 0) {
        g_freq_lock_control.ul_total_sdio_rate = (ul_rxtx_total * 1000) / ul_sdio_dur_us;

        if (g_freq_lock_control.en_debug_print == OAL_TRUE) {
            OAM_WARNING_LOG4(0, OAM_SF_ANY,
                "{SDIO tx statistic:packet_rate = %lu pps,sumlen = %lu B,[use time] = %lu ms,g_adjust_count = %d!}",
                g_freq_lock_control.ul_total_sdio_rate, ul_rxtx_total, ul_sdio_dur_us, g_adjust_count);
        }
    }
}
oal_uint8 hmac_get_freq_level(oal_uint32 ul_speed)
{
    oal_uint8 level_idx;

    if (hmac_is_device_ba_setup()) {
        if (ul_speed <= g_host_speed_freq_level[1].ul_speed_level) {
            level_idx = 0;
        } else if ((ul_speed > g_host_speed_freq_level[1].ul_speed_level) &&
            (ul_speed <= g_host_speed_freq_level[2].ul_speed_level)) {
            level_idx = 1;
        } else if ((ul_speed > g_host_speed_freq_level[2].ul_speed_level) &&
            (ul_speed <= g_host_speed_freq_level[3].ul_speed_level)) {
            level_idx = 2;
        } else {
            level_idx = 3;
        }
    } else {
        if (ul_speed <= g_host_no_ba_freq_level[1].ul_speed_level) {
            level_idx = 0;
        } else if ((ul_speed > g_host_no_ba_freq_level[1].ul_speed_level) &&
            (ul_speed <= g_host_no_ba_freq_level[2].ul_speed_level)) {
            level_idx = 1;
        } else if ((ul_speed > g_host_no_ba_freq_level[2].ul_speed_level) &&
            (ul_speed <= g_host_no_ba_freq_level[3].ul_speed_level)) {
            level_idx = 2;
        } else {
            level_idx = 3;
        }
    }
    return level_idx;
}

oal_void hmac_adjust_freq(oal_void)
{
    oal_uint8 uc_req_lock_level;
    oal_uint32 ul_cur_jiffies;
    oal_uint32 ul_sdio_dur_us;

    ul_cur_jiffies = jiffies;
    ul_sdio_dur_us = OAL_JIFFIES_TO_MSECS(ul_cur_jiffies - g_pre_jiffies);
    g_pre_jiffies = ul_cur_jiffies;

    /* 计算调频级别 */
    hmac_perform_calc_rwtotal_throughput(g_ul_wifi_rxtx_total, ul_sdio_dur_us);
    g_freq_lock_control.uc_req_lock_level = hmac_get_freq_level(g_freq_lock_control.ul_total_sdio_rate);

    uc_req_lock_level = g_freq_lock_control.uc_req_lock_level;

    oal_mutex_lock(&g_freq_lock_control.st_lock_freq_mtx);
    if (uc_req_lock_level != g_freq_lock_control.uc_curr_lock_level) {
        if (uc_req_lock_level != g_freq_lock_control.uc_curr_lock_level) {
            if (uc_req_lock_level < g_freq_lock_control.uc_curr_lock_level) {
                /* 连续MAX_DEGRADE_FREQ_TIME_THRESHOLD后才降频，保证性能 */
                g_adjust_count++;
                if (g_ul_wifi_rxtx_total != 0) {
                    if (g_adjust_count >= MAX_DEGRADE_FREQ_COUNT_THRESHOLD_SUCCESSIVE_10) {
                        g_adjust_count = 0;
                        wlan_pm_adjust_feq();
                    }
                } else {
                    if (g_adjust_count >= MAX_DEGRADE_FREQ_COUNT_THRESHOLD_SUCCESSIVE_2) {
                        g_adjust_count = 0;
                        wlan_pm_adjust_feq();
                    }
                }
            } else {
                /* 升频不等待，立即执行保证性能 */
                g_adjust_count = 0;
                wlan_pm_adjust_feq();
            }
        } else {
            g_adjust_count = 0;
        }
    } else {
        g_adjust_count = 0;
    }
    oal_mutex_unlock(&g_freq_lock_control.st_lock_freq_mtx);
}


oal_void hmac_wifi_init_freq_threshold(void)
{
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_void hmac_wifi_auto_ddr_init(oal_void)
{
    g_pst_wifi_auto_ddr = (struct pm_qos_request *)kmalloc(sizeof(struct pm_qos_request), GFP_KERNEL);
    if (g_pst_wifi_auto_ddr == NULL) {
        OAL_IO_PRINT("[AUTODDR]pm_qos_request alloc memory failed.\n");
        return;
    }
    g_pst_wifi_auto_ddr->pm_qos_class = 0;
/* HI1131C modify begin */
#if defined(CONFIG_ARCH_HI3650)
    pm_qos_add_request(g_pst_wifi_auto_ddr, PM_QOS_MEMORY_THROUGHPUT,
                       PM_QOS_MEMORY_THROUGHPUT_DEFAULT_VALUE);
#else
    /* HI3798MV2X don't support android lower power */
#endif
/* HI1131C modify end */
    return;
}
oal_void hmac_wifi_auto_ddr_exit(oal_void)
{
    /* HI1131C modify begin */
#if defined(CONFIG_ARCH_HI3650)
    pm_qos_remove_request(g_pst_wifi_auto_ddr);
#else
    /* HI3798MV2X don't support android lower power */
#endif /* HI1131C modify end */

    kfree(g_pst_wifi_auto_ddr);
    g_pst_wifi_auto_ddr = NULL;
}
#endif


oal_void hmac_wifi_auto_freq_ctrl_init(void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if (g_freq_lock_control.en_is_inited != OAL_TRUE) {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_wifi_auto_freq_ctrl_init enter!}");

        mutex_init(&g_freq_lock_control.st_lock_freq_mtx);

        mutex_lock(&g_freq_lock_control.st_lock_freq_mtx);

        g_pre_jiffies = jiffies;
        g_freq_lock_control.uc_lock_mod = FREQ_LOCK_DISABLE;
        g_freq_lock_control.uc_curr_lock_level = FREQ_IDLE;
        hmac_get_cpu_freq_raw(SCALING_MIN_FREQ, &g_ul_orig_cpu_min_freq);
        hmac_get_cpu_freq_raw(SCALING_MAX_FREQ, &g_ul_orig_cpu_max_freq);
#if 0
        hmac_get_ddr_freq_raw(SCALING_MIN_FREQ, &g_ul_orig_ddr_min_freq);
        hmac_get_ddr_freq_raw(SCALING_MAX_FREQ, &g_ul_orig_ddr_max_freq);
#else
        hmac_wifi_auto_ddr_init();
#endif
        hmac_wifi_init_freq_threshold();
        OAM_WARNING_LOG4(0, OAM_SF_ANY,
            "{hmac_wifi_auto_freq_ctrl_init cpu_min_freq = %d,cpu_max_freq = %d,ddr_min_freq = %d,ddr_max_freq = %d}",
            g_ul_orig_cpu_min_freq, g_ul_orig_cpu_max_freq, g_ul_orig_ddr_min_freq, g_ul_orig_ddr_max_freq);
        g_freq_lock_control.en_is_inited = OAL_TRUE;
        mutex_unlock(&g_freq_lock_control.st_lock_freq_mtx);
    }
#endif
}


oal_void hmac_wifi_auto_freq_ctrl_deinit(void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_wifi_auto_freq_ctrl_deinit enter!}");
    mutex_lock(&g_freq_lock_control.st_lock_freq_mtx);
    if (g_freq_lock_control.uc_lock_mod == FREQ_LOCK_ENABLE) {
        hmac_set_cpu_freq_raw(SCALING_MIN_FREQ, g_ul_orig_cpu_min_freq);
        hmac_set_cpu_freq_raw(SCALING_MAX_FREQ, g_ul_orig_cpu_max_freq);

        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hw_wifi_freq_ctrl_destroy freq lock release here!}");
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hw_wifi_freq_ctrl_destroy freq lock has already been released!}");
    }
    g_freq_lock_control.uc_lock_mod = FREQ_LOCK_DISABLE;
    g_freq_lock_control.uc_curr_lock_level = 0;
#if 1
    hmac_wifi_auto_ddr_exit();
#endif

    mutex_unlock(&g_freq_lock_control.st_lock_freq_mtx);
    mutex_destroy(&g_freq_lock_control.st_lock_freq_mtx);
#endif
}
oal_uint32 hmac_wifi_tx_rx_counter(oal_uint32 ul_pkt_count)
{
    g_ul_wifi_rxtx_total = g_ul_wifi_rxtx_total + ul_pkt_count;

    return 0;
}
oal_void hmac_hcc_auto_freq_count(oal_uint32 ul_pkt_count)
{
    g_ul_wifi_rxtx_total = g_ul_wifi_rxtx_total + ul_pkt_count;
}
oal_uint32 hmac_hcc_auto_freq_process(oal_void)
{
    oal_uint32 ul_return_total_count;

    /* 保存之前的值，返回给平台 */
    ul_return_total_count = g_ul_wifi_rxtx_total;
    if (g_freq_lock_control.uc_lock_mod == FREQ_LOCK_ENABLE) {
        hmac_adjust_freq();
    }
    g_ul_wifi_rxtx_total = 0;
    return ul_return_total_count;
}


#endif


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

