

#ifndef __HMAC_DFS_H__
#define __HMAC_DFS_H__


#ifdef _PRE_WLAN_FEATURE_DFS

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "frw_ext_if.h"
#include "mac_device.h"
#include "hmac_vap.h"
#include "hmac_scan.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_DFS_H

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define HMAC_DFS_ONE_SEC_IN_MS    1000
#define HMAC_DFS_ONE_MIN_IN_MS    (60 * HMAC_DFS_ONE_SEC_IN_MS)

/* CAC检测时长，5600MHz ~ 5650MHz频段外，默认60秒 */
#define HMAC_DFS_CAC_OUTOF_5600_TO_5650_MHZ_TIME_MS           HMAC_DFS_ONE_MIN_IN_MS

/* CAC检测时长，5600MHz ~ 5650MHz频段内，默认10分钟 */
#define HMAC_DFS_CAC_IN_5600_TO_5650_MHZ_TIME_MS              (10 * HMAC_DFS_ONE_MIN_IN_MS)

/* Off-Channel CAC检测时长，5600MHz ~ 5650MHz频段外，默认6分钟 */
#define HMAC_DFS_OFF_CH_CAC_OUTOF_5600_TO_5650_MHZ_TIME_MS    (6 * HMAC_DFS_ONE_MIN_IN_MS)

/* Off-Channel CAC检测时长，5600MHz ~ 5650MHz频段内，默认60分钟 */
#define HMAC_DFS_OFF_CH_CAC_IN_5600_TO_5650_MHZ_TIME_MS       (60 * HMAC_DFS_ONE_MIN_IN_MS)

/* Non-Occupancy Period时长，默认30分钟 */
#define HMAC_DFS_NON_OCCUPANCY_PERIOD_TIME_MS                 (30 * HMAC_DFS_ONE_MIN_IN_MS)

/* Off-Channel CAC在工作信道上的驻留时长 */
#define HMAC_DFS_OFF_CHAN_CAC_PERIOD_TIME_MS                  15

/* Off-channel CAC在Off-channel信道上的驻留时长 */
#define HMAC_DFS_OFF_CHAN_CAC_DWELL_TIME_MS                   30


/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_void  hmac_dfs_init(mac_device_stru *pst_mac_device);
extern oal_void  hmac_dfs_channel_list_init(mac_device_stru *pst_mac_device);
extern oal_uint32  hmac_dfs_recalculate_channel(mac_device_stru *pst_mac_device,
                                                oal_uint8 *puc_freq,
                                                wlan_channel_bandwidth_enum_uint8 *pen_bandwidth);
extern oal_void  hmac_dfs_cac_start(mac_device_stru *pst_mac_device, hmac_vap_stru *pst_hmac_vap);
extern oal_void  hmac_dfs_cac_stop(mac_device_stru *pst_mac_device);
extern oal_void  hmac_dfs_off_cac_stop(mac_device_stru *pst_mac_device, mac_vap_stru *pst_mac_vap);
extern oal_uint32  hmac_switch_to_new_chan_complete(frw_event_mem_stru *pst_event_mem);
extern oal_uint32  hmac_dfs_radar_detect_event(frw_event_mem_stru *pst_event_mem);
extern oal_uint32  hmac_dfs_ap_wait_start_radar_handler(hmac_vap_stru *pst_hmac_vap);
extern oal_uint32  hmac_dfs_ap_up_radar_handler(hmac_vap_stru *pst_hmac_vap);
extern oal_void  hmac_dfs_radar_wait(mac_device_stru *pst_mac_device, oal_uint8 uc_chan_idx);
extern oal_void test_dfs(oal_uint8 uc_vap_id);
extern oal_void test_csa(oal_uint8 uc_vap_id, oal_uint8 uc_chan_id, oal_uint8 uc_sw_cnt);
extern oal_uint32  hmac_dfs_start_bss(hmac_vap_stru *pst_hmac_vap);
extern oal_void  hmac_dfs_off_chan_cac_start(mac_device_stru *pst_mac_device, hmac_vap_stru *pst_hmac_vap);
extern oal_uint32  hmac_dfs_init_scan_hook(hmac_scan_record_stru *pst_scan_record,
                                           hmac_device_stru *pst_dev);


/*****************************************************************************
  11 内联函数定义
*****************************************************************************/

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 hmac_dfs_need_for_cac(mac_device_stru *pst_mac_device, oal_uint8 uc_channel)
{
    oal_uint8              uc_idx;
    oal_uint32             ul_ret;

    /* dfs使能位 */
    if (mac_dfs_get_dfs_enable(pst_mac_device) == OAL_FALSE) {
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "dfs not enable\n");
        return OAL_FALSE;
    }

    /* 获取信道索引 */
    ul_ret = mac_get_channel_idx_from_num(pst_mac_device->en_max_band, uc_channel, &uc_idx);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_ERROR_LOG2(0, OAM_SF_DFS, "get ch failed, band=%d ch=%d\n", pst_mac_device->en_max_band, uc_channel);
        return OAL_FALSE;
    }

    /* 定时器使能位 */
    if (pst_mac_device->st_dfs.st_dfs_cac_timer.en_is_enabled == OAL_TRUE) {
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "dfs tiemer enabled\n");
        return OAL_FALSE;
    }

    /* 信道状态 */ // 需要看是否返回false
    if (pst_mac_device->st_ap_channel_list[uc_idx].en_ch_status != MAC_CHAN_DFS_REQUIRED) {
        OAM_WARNING_LOG1(0, OAM_SF_DFS, "dfs stat=%d, not need cac\n",
            pst_mac_device->st_ap_channel_list[uc_idx].en_ch_status);
            return OAL_FALSE;
    }

    /* CAC使能位 */
    if (mac_dfs_get_cac_enable(pst_mac_device) == OAL_FALSE) {
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "cac not enabled");
        return OAL_FALSE;
    }

    return OAL_TRUE;
}

OAL_STATIC OAL_INLINE oal_void hmac_dfs_set_cac_time(mac_device_stru *pst_mac_device, oal_uint32 ul_time_ms,
                                                     oal_bool_enum_uint8 en_waether)
{
    if (en_waether) {
        pst_mac_device->st_dfs.st_dfs_info.ul_dfs_cac_in_5600_to_5650_time_ms = ul_time_ms;
    } else {
        pst_mac_device->st_dfs.st_dfs_info.ul_dfs_cac_outof_5600_to_5650_time_ms = ul_time_ms;
    }
}


OAL_STATIC OAL_INLINE oal_void hmac_dfs_set_opern_chan_time(mac_device_stru *pst_mac_device, oal_uint32 ul_time_ms)
{
    pst_mac_device->st_dfs.st_dfs_info.us_dfs_off_chan_cac_opern_chan_dwell_time = (oal_uint16)ul_time_ms;
}


OAL_STATIC OAL_INLINE oal_void  hmac_dfs_set_off_chan_time(mac_device_stru *pst_mac_device, oal_uint32 ul_time_ms)
{
    pst_mac_device->st_dfs.st_dfs_info.us_dfs_off_chan_cac_off_chan_dwell_time = (oal_uint16)ul_time_ms;
}


#endif   /* end of _PRE_WLAN_FEATURE_DFS */

#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif

#endif /* end of hmac_dfs.h */