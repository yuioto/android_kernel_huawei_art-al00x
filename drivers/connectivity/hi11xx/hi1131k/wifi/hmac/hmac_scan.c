

// 1 头文件包含
#include "oam_ext_if.h"
#include "mac_ie.h"
#include "mac_regdomain.h"
#include "mac_device.h"
#include "mac_resource.h"
#include "dmac_ext_if.h"
#include "hmac_fsm.h"
#include "hmac_sme_sta.h"
#include "hmac_resource.h"
#include "hmac_device.h"
#include "hmac_scan.h"
#include "hmac_mgmt_sta.h"
#include "hmac_mgmt_ap.h"
#include "frw_ext_if.h"
#include "securec.h"
#ifdef _PRE_SUPPORT_ACS
#include "dmac_acs.h"
#include "hmac_acs.h"
#endif
#include "hmac_chan_mgmt.h"
#include "hmac_p2p.h"
#ifdef _PRE_WLAN_FEATURE_DFS
#include "hmac_dfs.h"
#endif
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#include "hisi_customize_wifi.h"
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

#ifdef _PRE_WLAN_FEATURE_ROAM
#include "hmac_roam_main.h"
#endif // _PRE_WLAN_FEATURE_ROAM

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_SCAN_C

// 2 全局变量定义
#if defined(_PRE_WLAN_CHIP_TEST_ALG) && ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || \
    (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)) && defined(_PRE_DEBUG_MODE)
struct kobject *g_scan_ct_sys_kobject = OAL_PTR_NULL;
#endif
oal_bool_enum_uint8 g_en_bgscan_enable_flag = OAL_TRUE;


// 3 函数实现
#if defined(_PRE_WLAN_FEATURE_WPA) || defined(_PRE_WLAN_FEATURE_WPA2)
OAL_STATIC oal_bool_enum_uint8 hmac_parse_cipher_suit(mac_bss_dscr_stru *bss_dscr, uint8_t cipher_type,
    const uint8_t *ie_data, uint32_t len);
#endif
#if defined(_PRE_WLAN_CHIP_TEST_ALG) && ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || \
    (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)) && defined(_PRE_DEBUG_MODE)
OAL_STATIC oal_ssize_t  hmac_scan_ct_proc_read(struct kobject *dev, struct kobj_attribute *attr, char *buf);
OAL_STATIC struct kobj_attribute dev_attr_alg_test_result =
    __ATTR(alg_test_result, OAL_S_IRUGO | OAL_S_IWUSR, hmac_scan_ct_proc_read, OAL_PTR_NULL);



OAL_STATIC oal_ssize_t hmac_scan_ct_proc_read(struct kobject *dev, struct kobj_attribute *attr, char *buf)

{
    hmac_device_stru *pst_hmac_device;
    hmac_scan_record_stru *pst_record;
    oal_int32 l_len;

    /* 获取hmac device和扫描运行记录 */
    pst_hmac_device = hmac_res_get_mac_dev(0);
    pst_record = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt);

    l_len = OAL_SIZEOF(mac_scan_chan_stats_stru);
    if (memcpy_s(buf, l_len, &(pst_record->ast_chan_results[0]), l_len) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hmac_scan_ct_proc_read::memcpy_s fail!");
        return OAL_FAIL;
    }

    return l_len;
}


oal_int32 hmac_scan_ct_init(oal_void)
{
    /* hi1102-cb add sys for 51/02 */
#if ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION))
    oal_int32 l_ret = OAL_SUCC;

    if (g_scan_ct_sys_kobject == OAL_PTR_NULL) {
        g_scan_ct_sys_kobject = kobject_create_and_add("scan_ct", OAL_PTR_NULL);
        l_ret = sysfs_create_file(g_scan_ct_sys_kobject, &dev_attr_alg_test_result.attr);
    }

    return l_ret;
#else
    return OAL_SUCC;
#endif
}


oal_void hmac_scan_ct_exit(oal_void)
{
    if (g_scan_ct_sys_kobject != OAL_PTR_NULL) {
        sysfs_remove_file(g_scan_ct_sys_kobject, &dev_attr_alg_test_result.attr);
        kobject_del(g_scan_ct_sys_kobject);
        g_scan_ct_sys_kobject = OAL_PTR_NULL;
    }
}
#endif


OAL_STATIC oal_void hmac_scan_print_scan_params(mac_scan_req_stru *pst_scan_params, mac_vap_stru *pst_mac_vap)
{
    OAM_WARNING_LOG4(pst_scan_params->uc_vap_id,
                     OAM_SF_SCAN,
                     "hmac_scan_print_scan_params::Now Scan channel_num[%d] in [%d]ms with scan_func[0x%x], "
                     " and ssid_num[%d]!",
                     pst_scan_params->uc_channel_nums,
                     pst_scan_params->us_scan_time,
                     pst_scan_params->uc_scan_func,
                     pst_scan_params->uc_ssid_num);

    OAM_WARNING_LOG3(pst_scan_params->uc_vap_id,
                     OAM_SF_SCAN,
                     "hmac_scan_print_scan_params::Scan param, p2p_scan[%d], max_scan_count_per_channel[%d], need back "
                     "home_channel[%d]!",
                     pst_scan_params->bit_is_p2p0_scan,
                     pst_scan_params->uc_max_scan_count_per_channel,
                     pst_scan_params->en_need_switch_back_home_channel);
    return;
}


OAL_STATIC int32_t hmac_wifi_hide_ssid(uint8_t *frame_body, mac_bss_dscr_stru *bss_dscr,
    mac_ieee80211_frame_stru *frame_hdr)
{
    uint8_t *ssid_ie = NULL;
    uint8_t ssid_len = 0;
    uint8_t index;
    uint8_t frame_sub_type;
    uint16_t mac_frame_len;

    mac_frame_len = bss_dscr->ul_mgmt_len - MAC_80211_FRAME_LEN;
    /*lint -e416*/
    if (memcpy_s(frame_body, mac_frame_len, (uint8_t *)(bss_dscr->auc_mgmt_buff + MAC_80211_FRAME_LEN),
        mac_frame_len) != EOK) {
        return OAL_FAIL;
    }
    /*lint +e416*/
    frame_sub_type = mac_get_frame_sub_type((uint8_t *)frame_hdr); // 得到帧类型
    if ((frame_sub_type == WLAN_FC0_SUBTYPE_BEACON) || (frame_sub_type == WLAN_FC0_SUBTYPE_PROBE_RSP)) {
        ssid_ie = mac_get_ssid(frame_body, mac_frame_len, &ssid_len);
        /* 保留4位，如果不足4位，则不隐藏 */
        if (ssid_ie == NULL || ssid_len < 4) {
            return OAL_SUCC;
        }

        for (index = 2; index < ssid_len - 2; index++)  { // 保留前两位和后两位
            *(ssid_ie + index) = 0x78;  // 0x78转换为ASCII码就是字符'x'
        }
    }
    return OAL_SUCC;
}

oal_void hmac_scan_print_scanned_bss_info(oal_uint8 uc_device_id)
{
    hmac_device_stru         *pst_hmac_device   = NULL;
    hmac_bss_mgmt_stru       *pst_bss_mgmt      = NULL;
    hmac_scanned_bss_info    *pst_scanned_bss   = NULL;
    mac_bss_dscr_stru        *pst_bss_dscr      = NULL;
    oal_dlist_head_stru      *pst_entry         = NULL;
    mac_ieee80211_frame_stru *pst_frame_hdr     = NULL;
    oal_uint8                auc_sdt_parse_hdr[MAC_80211_FRAME_LEN];
    uint8_t                  *tmp_mac_body_addr = NULL;

    /* 获取hmac device */
    pst_hmac_device = hmac_res_get_mac_dev(uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_scan_print_scanned_bss_info::pst_hmac_device null.}");
        return;
    }

    /* 获取指向扫描结果的管理结构体地址 */
    pst_bss_mgmt = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.st_bss_mgmt);

    /* 获取锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    /* 遍历扫描到的bss信息 */
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_bss_mgmt->st_bss_list_head)) {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);
        pst_bss_dscr    = &(pst_scanned_bss->st_bss_dscr_info);

        /* 仅显示新申请到的BSS帧 */
        if (pst_scanned_bss->st_bss_dscr_info.en_new_scan_bss == OAL_TRUE) {
            pst_scanned_bss->st_bss_dscr_info.en_new_scan_bss = OAL_FALSE;
            /* 上报beacon和probe帧 */
            pst_frame_hdr  = (mac_ieee80211_frame_stru *)pst_bss_dscr->auc_mgmt_buff;

            /* 将beacon中duration字段(2字节)复用为rssi以及channel,方便SDT显示 */
            if (memcpy_s((uint8_t *)auc_sdt_parse_hdr, MAC_80211_FRAME_LEN, (uint8_t *)pst_frame_hdr,
                         MAC_80211_FRAME_LEN) != EOK) {
                OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_print_scanned_bss_info::memcpy_s fail}");
                oal_spin_unlock(&(pst_bss_mgmt->st_lock));
                return;
            }
            auc_sdt_parse_hdr[2]  = (oal_uint8)pst_bss_dscr->c_rssi;
            auc_sdt_parse_hdr[3]  = pst_bss_dscr->st_channel.uc_chan_number;

            /* 申请一个大小为去掉帧头大小的空间 */
            tmp_mac_body_addr = (uint8_t *)oal_memalloc(pst_bss_dscr->ul_mgmt_len - MAC_80211_FRAME_LEN);
            if (OAL_UNLIKELY(tmp_mac_body_addr == OAL_PTR_NULL)) {
                OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_scan_print_scanned_bss_info::alloc memory failed}");
                continue;
            }
            if (hmac_wifi_hide_ssid(tmp_mac_body_addr, pst_bss_dscr, pst_frame_hdr) != OAL_SUCC) {
                oal_free(tmp_mac_body_addr);
                oal_spin_unlock(&(pst_bss_mgmt->st_lock));
                return;
            }
            /* 上报beacon帧或者probe rsp帧 */
            /*lint -e416*/
            oam_report_80211_frame(BROADCAST_MACADDR, (uint8_t *)auc_sdt_parse_hdr, MAC_80211_FRAME_LEN,
                tmp_mac_body_addr, (uint16_t) pst_bss_dscr->ul_mgmt_len, OAM_OTA_FRAME_DIRECTION_TYPE_RX);
            /*lint +e416*/
            oal_free(tmp_mac_body_addr);
        }
    }
    /* 解除锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    return;
}




hmac_scanned_bss_info *hmac_scan_alloc_scanned_bss(oal_uint32 ul_mgmt_len)
{
    hmac_scanned_bss_info *pst_scanned_bss;

    /* 申请内存，存储扫描到的bss信息 */
    pst_scanned_bss = oal_memalloc(OAL_SIZEOF(hmac_scanned_bss_info) + ul_mgmt_len -
        OAL_SIZEOF(pst_scanned_bss->st_bss_dscr_info.auc_mgmt_buff));
    if (OAL_UNLIKELY(pst_scanned_bss == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN,
            "{hmac_scan_alloc_scanned_bss::alloc memory failed for storing scanned result.}");
        return OAL_PTR_NULL;
    }

    /* 为申请的内存清零 */
    memset_s(pst_scanned_bss,
        OAL_SIZEOF(hmac_scanned_bss_info) + ul_mgmt_len - OAL_SIZEOF(pst_scanned_bss->st_bss_dscr_info.auc_mgmt_buff),
        0,
        OAL_SIZEOF(hmac_scanned_bss_info) + ul_mgmt_len - OAL_SIZEOF(pst_scanned_bss->st_bss_dscr_info.auc_mgmt_buff));

    /* 初始化链表头节点指针 */
    oal_dlist_init_head(&(pst_scanned_bss->st_dlist_head));

    return pst_scanned_bss;
}


oal_uint32 hmac_scan_add_bss_to_list(hmac_scanned_bss_info *pst_scanned_bss, hmac_device_stru *pst_hmac_device)
{
    hmac_bss_mgmt_stru *pst_bss_mgmt; /* 管理扫描结果的结构体 */

    pst_bss_mgmt = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.st_bss_mgmt);
    pst_scanned_bss->st_bss_dscr_info.en_new_scan_bss = OAL_TRUE;

    /* 对链表写操作前加锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    /* 添加扫描结果到链表中，并更新扫描到的bss计数 */
    oal_dlist_add_tail(&(pst_scanned_bss->st_dlist_head), &(pst_bss_mgmt->st_bss_list_head));

    pst_bss_mgmt->ul_bss_num++;
    /* 解锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_scan_del_bss_from_list_nolock(hmac_scanned_bss_info *pst_scanned_bss,
    hmac_device_stru *pst_hmac_device)
{
    hmac_bss_mgmt_stru *pst_bss_mgmt;      /* 管理扫描结果的结构体 */

    pst_bss_mgmt = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.st_bss_mgmt);

    /* 从链表中删除节点，并更新扫描到的bss计数 */
    oal_dlist_delete_entry(&(pst_scanned_bss->st_dlist_head));

    pst_bss_mgmt->ul_bss_num--;

    return OAL_SUCC;
}


oal_void hmac_scan_clean_scan_record(hmac_scan_record_stru *pst_scan_record)
{
    oal_dlist_head_stru *pst_entry = OAL_PTR_NULL;
    hmac_scanned_bss_info *pst_scanned_bss = OAL_PTR_NULL;
    hmac_bss_mgmt_stru *pst_bss_mgmt = OAL_PTR_NULL;

    /* 参数合法性检查 */
    if (pst_scan_record == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_clean_scan_record::pst_scan_record is null.}");
        return;
    }

    /* 1.一定要先清除扫描到的bss信息，再进行清零处理 */
    pst_bss_mgmt = &(pst_scan_record->st_bss_mgmt);

    /* 对链表写操作前加锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    /* 遍历链表，删除扫描到的bss信息 */
    while (oal_dlist_is_empty(&(pst_bss_mgmt->st_bss_list_head)) == OAL_FALSE) {
        pst_entry       = oal_dlist_delete_head(&(pst_bss_mgmt->st_bss_list_head));
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);

        pst_bss_mgmt->ul_bss_num--;

        /* 释放扫描队列里的内存 */
        oal_free(pst_scanned_bss);
    }

    /* 对链表写操作前加锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    /* 2.其它信息清零 */
    memset_s(pst_scan_record, OAL_SIZEOF(hmac_scan_record_stru), 0, OAL_SIZEOF(hmac_scan_record_stru));
    pst_scan_record->en_scan_rsp_status = MAC_SCAN_STATUS_BUTT;     /* 初始化扫描完成时状态码为无效值 */
    pst_scan_record->en_vap_last_state  = MAC_VAP_STATE_BUTT;       /* 必须置BUTT,否则aput停扫描会vap状态恢复错 */

    /* 3.重新初始化bss管理结果链表和锁 */
    pst_bss_mgmt = &(pst_scan_record->st_bss_mgmt);
    oal_dlist_init_head(&(pst_bss_mgmt->st_bss_list_head));
    oal_spin_lock_init(&(pst_bss_mgmt->st_lock));

    OAM_INFO_LOG0(0, OAM_SF_SCAN, "{hmac_scan_clean_scan_record::cleaned scan record success.}");

    return;
}



OAL_STATIC oal_int32 hmac_is_connected_ap_bssid(oal_uint8 uc_device_id, const oal_uint8 auc_bssid[WLAN_MAC_ADDR_LEN])
{
    oal_uint8 uc_vap_idx;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    mac_device_stru *pst_mac_device = OAL_PTR_NULL;

    pst_mac_device = mac_res_get_dev(uc_device_id);
    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_is_connected_ap_bssid::mac_res_get_dev return null.}");
        return OAL_FALSE;
    }

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++) {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(pst_mac_vap == OAL_PTR_NULL)) {
            OAM_WARNING_LOG1(0, OAM_SF_P2P, "{hmac_is_connected_ap_bssid::mac_res_get_mac_vap fail! vap id is %d}",
                pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if (IS_LEGACY_VAP(pst_mac_vap) && (pst_mac_vap->en_vap_state == MAC_VAP_STATE_UP)) {
            if (oal_memcmp(auc_bssid, pst_mac_vap->auc_bssid, WLAN_MAC_ADDR_LEN) == 0) {
                /* 不老化当前关联的AP */
                OAM_INFO_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                    "{hmac_is_connected_ap_bssid::connected AP bssid:%02X:XX:XX:XX:%02X:%02X}", auc_bssid[0],
                    auc_bssid[4], auc_bssid[5]);

                return OAL_TRUE;
            }
        }
    }

    return OAL_FALSE;
}


OAL_STATIC oal_void hmac_scan_clean_expire_scanned_bss(hmac_scan_record_stru *pst_scan_record)
{
    oal_dlist_head_stru *pst_entry = OAL_PTR_NULL;
    oal_dlist_head_stru *pst_entry_tmp = OAL_PTR_NULL;
    hmac_bss_mgmt_stru *pst_bss_mgmt = OAL_PTR_NULL;
    hmac_scanned_bss_info *pst_scanned_bss = OAL_PTR_NULL;
    mac_bss_dscr_stru *pst_bss_dscr = OAL_PTR_NULL;
    oal_uint32 ul_curr_time_stamp;

    /* 参数合法性检查 */
    if (pst_scan_record == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_clean_expire_scanned_bss::scan record is null.}");
        return;
    }

    /* 管理扫描的bss结果的结构体 */
    pst_bss_mgmt = &(pst_scan_record->st_bss_mgmt);

    ul_curr_time_stamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    /* 对链表写操作前加锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    /* 遍历链表，删除上一次扫描结果中到期的bss信息 */
    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_entry_tmp, &(pst_bss_mgmt->st_bss_list_head))
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);
        pst_bss_dscr    = &(pst_scanned_bss->st_bss_dscr_info);

        if (ul_curr_time_stamp - pst_bss_dscr->ul_timestamp < HMAC_SCAN_MAX_SCANNED_BSS_EXPIRE) {
            OAM_INFO_LOG0(0, OAM_SF_SCAN,
                "{hmac_scan_clean_expire_scanned_bss::do not remove the BSS, because it has not expired.}");
            continue;
        }

        /* 不老化当前正在关联的AP */
        if (hmac_is_connected_ap_bssid(pst_scan_record->uc_device_id, pst_bss_dscr->auc_bssid)) {
            continue;
        }

        /* 从链表中删除节点，并更新扫描到的bss计数 */
        oal_dlist_delete_entry(&(pst_scanned_bss->st_dlist_head));
        pst_bss_mgmt->ul_bss_num--;

        /* 释放对应内存 */
        oal_free(pst_scanned_bss);
    }

    /* 对链表写操作前加锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));
    return;
}




mac_bss_dscr_stru *hmac_scan_find_scanned_bss_dscr_by_index(oal_uint8  uc_device_id, oal_uint32 ul_bss_index)
{
    oal_dlist_head_stru *pst_entry = OAL_PTR_NULL;
    hmac_scanned_bss_info *pst_scanned_bss = OAL_PTR_NULL;
    hmac_device_stru *pst_hmac_device = OAL_PTR_NULL;
    hmac_bss_mgmt_stru *pst_bss_mgmt = OAL_PTR_NULL;
    oal_uint8 ul_loop;

    /* 获取hmac device 结构 */
    pst_hmac_device = hmac_res_get_mac_dev(uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_find_scanned_bss_by_index::pst_hmac_device is null.}");
        return OAL_PTR_NULL;
    }

    pst_bss_mgmt = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.st_bss_mgmt);

    /* 对链表删操作前加锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    /* 如果索引大于总共扫描的bss个数，返回异常 */
    if (ul_bss_index >= pst_bss_mgmt->ul_bss_num) {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_scan_find_scanned_bss_by_index::no such bss in bss list!}");

        /* 解锁 */
        oal_spin_unlock(&(pst_bss_mgmt->st_lock));
        return OAL_PTR_NULL;
    }

    ul_loop = 0;
    /* 遍历链表，返回对应index的bss dscr信息 */
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_bss_mgmt->st_bss_list_head))
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);

        /* 相同的bss index返回 */
        if (ul_bss_index == ul_loop) {
            /* 解锁 */
            oal_spin_unlock(&(pst_bss_mgmt->st_lock));
            return &(pst_scanned_bss->st_bss_dscr_info);
        }

        ul_loop++;
    }
    /* 解锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    return OAL_PTR_NULL;
}


hmac_scanned_bss_info *hmac_scan_find_scanned_bss_by_bssid(hmac_bss_mgmt_stru *pst_bss_mgmt, const uint8_t *puc_bssid)
{
    oal_dlist_head_stru   *pst_entry        = OAL_PTR_NULL;
    hmac_scanned_bss_info *pst_scanned_bss  = OAL_PTR_NULL;

    /* 遍历链表，查找链表中是否已经存在相同bssid的bss信息 */
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_bss_mgmt->st_bss_list_head))
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);
        /* 相同的bssid地址 */
        if (oal_compare_mac_addr(pst_scanned_bss->st_bss_dscr_info.auc_bssid, puc_bssid) == 0) {
            return pst_scanned_bss;
        }
    }

    return OAL_PTR_NULL;
}

/* 功能描述  : 根据mac地址获取扫描结果中的bss信息 */
void *hmac_scan_get_scanned_bss_by_bssid(mac_vap_stru *mac_vap, uint8_t *mac_addr)
{
    hmac_bss_mgmt_stru    *bss_mgmt = NULL; /* 管理扫描的bss结果的结构体 */
    hmac_scanned_bss_info *scanned_bss_info = NULL;
    hmac_device_stru      *hmac_device = NULL;

    /* 获取hmac device 结构 */
    hmac_device = hmac_res_get_mac_dev(mac_vap->uc_device_id);
    if (hmac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{hmac_scan_get_scanned_bss_by_bssid::pst_hmac_device is null, dev id[%d].}",
                         mac_vap->uc_device_id);
        return OAL_PTR_NULL;
    }

    /* 获取管理扫描的bss结果的结构体 */
    bss_mgmt = &(hmac_device->st_scan_mgmt.st_scan_record_mgmt.st_bss_mgmt);

    oal_spin_lock(&(bss_mgmt->st_lock));

    scanned_bss_info = hmac_scan_find_scanned_bss_by_bssid(bss_mgmt, mac_addr);
    if (scanned_bss_info == NULL) {
        OAM_WARNING_LOG4(mac_vap->uc_vap_id, OAM_SF_SCAN,
                         "{hmac_scan_get_scanned_bss_by_bssid::find the bss failed[%02X:XX:XX:%02X:%02X:%02X]}",
                         mac_addr[0], mac_addr[3], mac_addr[4], mac_addr[5]);
       /* 解锁 */
        oal_spin_unlock(&(bss_mgmt->st_lock));
        return OAL_PTR_NULL;
    }
    /* 解锁 */
    oal_spin_unlock(&(bss_mgmt->st_lock));

    return &(scanned_bss_info->st_bss_dscr_info);
}

#if defined (_PRE_WLAN_FEATURE_WPA2)

OAL_STATIC oal_bool_enum_uint8 hmac_scan_update_bss_list_rsn(mac_bss_dscr_stru *pst_bss_dscr, const oal_uint8 *puc_ie)
{
    oal_uint8  uc_index = 0;
    oal_uint16 us_ver;
    uint32_t   ie_len_left;

    /* *********************************************************************** */
    /*                  RSN Element Format                                   */
    /* --------------------------------------------------------------------- */
    /* |Element ID | Length | Version | Group Cipher Suite | Pairwise Cipher */
    /* --------------------------------------------------------------------- */
    /* |     1     |    1   |    2    |         4          |       2         */
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* Suite Count| Pairwise Cipher Suite List | AKM Suite Count | AKM Suite List */
    /* --------------------------------------------------------------------- */
    /*            |         4*m                |     2           |   4*n     */
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* |RSN Capabilities|PMKID Count|PMKID List|Group Management Cipher Suite */
    /* --------------------------------------------------------------------- */
    /* |        2       |    2      |   16 *s  |               4           | */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /* *********************************************************************** */
    /* RSN IE长度,不包括Element ID 和 Length字段 */
    ie_len_left = puc_ie[1];

    /* 忽略 RSN IE 和 IE 长度 */
    uc_index += 2;

    /* 获取RSN 版本号,2个字节 */
    if (ie_len_left > 2) {
        us_ver = OAL_MAKE_WORD16(puc_ie[uc_index], puc_ie[uc_index + 1]);
        if (us_ver != MAC_RSN_IE_VERSION) {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{hmac_scan_update_bss_list_rsn::invalid us_ver[%d].}", us_ver);
            return OAL_FALSE;
        }

        /* 忽略 RSN 版本号长度,2个字节 */
        uc_index += 2;
        ie_len_left -= 2;
    } else {
        OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{hmac_scan_update_bss_list_rsn::ie too short for version:%d.}", ie_len_left);
        return OAL_FALSE;
    }

    /* 设置 RSNA */
    pst_bss_dscr->st_bss_sec_info.uc_bss_80211i_mode |= DMAC_RSNA_802_11I;

    return hmac_parse_cipher_suit(pst_bss_dscr, DMAC_RSNA_802_11I, puc_ie + uc_index, ie_len_left);
}
#endif

#if defined (_PRE_WLAN_FEATURE_WPA)

OAL_STATIC oal_bool_enum_uint8 hmac_scan_update_bss_list_wpa(mac_bss_dscr_stru *pst_bss_dscr, const oal_uint8 *puc_ie)
{
    oal_uint8  uc_index;
    oal_uint16 us_ver;
    uint32_t   ie_len_left;

    /* *********************************************************************** */
    /*                  WPA Element Format                                   */
    /* --------------------------------------------------------------------- */
    /* |Element ID | Length |    WPA OUI    |  Version |  Group Cipher Suite */
    /* --------------------------------------------------------------------- */
    /* |     1     |   1    |        4      |     2    |         4           */
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* Pairwise Cipher |  Pairwise Cipher   |                 |              */
    /* Suite Count     |    Suite List      | AKM Suite Count |AKM Suite List */
    /* --------------------------------------------------------------------- */
    /*        2        |          4*m       |         2       |     4*n      */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /* *********************************************************************** */
    /* WPA IE长度,不包括Element ID 和 Length字段 */
    ie_len_left = puc_ie[1];
    /* 最短WPA OUI(4 字节) */
    if (ie_len_left < 4) {
        OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{hmac_scan_update_bss_list_wpa::invalid WPA IE LEN:%d.}", ie_len_left);
        return OAL_FALSE;
    }
    /* 忽略 WPA OUI(4 字节) */
    ie_len_left -= 4;
    /* 忽略 WPA IE(1 字节) ，IE 长度(1 字节) ，WPA OUI(4 字节) */
    uc_index = 2 + 4;
    /* 获取版本号,2个字节 */
    if (ie_len_left > 2) {
        us_ver = OAL_MAKE_WORD16(puc_ie[uc_index], puc_ie[uc_index + 1]);
        /* 对比WPA 版本信息 */
        if (us_ver != MAC_WPA_IE_VERSION) {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{hmac_scan_update_bss_list_wpa::invalid WPA version[%d].}", us_ver);
            return OAL_FALSE;
        }

        /* 忽略 版本号 长度 */
        uc_index += 2;
        ie_len_left -= 2;
    } else {
        OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{hmac_scan_update_bss_list_wpa::ie too short for version:%d.}", ie_len_left);
        return OAL_FALSE;
    }
    /* 设置 WPA */
    pst_bss_dscr->st_bss_sec_info.uc_bss_80211i_mode |= DMAC_WPA_802_11I;
    return hmac_parse_cipher_suit(pst_bss_dscr, DMAC_WPA_802_11I, puc_ie + uc_index, ie_len_left);
}
#endif

#if defined(_PRE_WLAN_FEATURE_WPA) || defined(_PRE_WLAN_FEATURE_WPA2)

OAL_STATIC oal_void hmac_scan_update_bss_list_security(mac_bss_dscr_stru *pst_bss_dscr,
                                                       oal_uint8 *puc_frame_body,
                                                       oal_uint16 us_frame_len,
                                                       oal_uint16 us_offset)
{
    oal_uint8           *puc_ie;
    oal_bool_enum_uint8 en_ret;

    /* 安全相关信息元素 */
    /* 清空当前 bss_info 结构中的安全信息 */
    memset_s(&(pst_bss_dscr->st_bss_sec_info), sizeof(mac_bss_80211i_info_stru), 0xff,
             sizeof(mac_bss_80211i_info_stru));
    pst_bss_dscr->st_bss_sec_info.uc_bss_80211i_mode = 0;
    pst_bss_dscr->st_bss_sec_info.auc_rsn_cap[0]     = 0;
    pst_bss_dscr->st_bss_sec_info.auc_rsn_cap[1]     = 0;

#if defined (_PRE_WLAN_FEATURE_WPA2)
    puc_ie = mac_find_ie(MAC_EID_RSN, puc_frame_body + us_offset, (oal_int32)(us_frame_len - us_offset));
    if (puc_ie != OAL_PTR_NULL) {
        /* 更新从beacon 中收到的 RSN 安全相关信息到 pst_bss_dscr 中 */
        en_ret = hmac_scan_update_bss_list_rsn(pst_bss_dscr, puc_ie);
        if (en_ret == OAL_FALSE) {
            /* 如果获取RSN 信息失败，则上报帧体信息到SDT */
            oam_report_80211_frame(BROADCAST_MACADDR,
                                   puc_frame_body - MAC_80211_FRAME_LEN,
                                   MAC_80211_FRAME_LEN,
                                   puc_frame_body,
                                   us_frame_len + MAC_80211_FRAME_LEN,
                                   OAM_OTA_FRAME_DIRECTION_TYPE_RX);
        }
    }
#endif

#if defined (_PRE_WLAN_FEATURE_WPA)
    puc_ie = mac_find_vendor_ie(MAC_WLAN_OUI_MICROSOFT, MAC_OUITYPE_WPA, puc_frame_body + us_offset,
        (oal_int32)(us_frame_len - us_offset));
    if (puc_ie != OAL_PTR_NULL) {
        /* 更新从beacon 中收到的 WPA 安全相关信息到 pst_bss_dscr 中 */
        en_ret = hmac_scan_update_bss_list_wpa(pst_bss_dscr, puc_ie);
        if (en_ret == OAL_FALSE) {
            /* 如果获取安全信息失败，则上报帧体信息到SDT */
            oam_report_80211_frame(BROADCAST_MACADDR,
                                   puc_frame_body - MAC_80211_FRAME_LEN,
                                   MAC_80211_FRAME_LEN,
                                   puc_frame_body,
                                   us_frame_len + MAC_80211_FRAME_LEN,
                                   OAM_OTA_FRAME_DIRECTION_TYPE_RX);
        }
    }
#endif
}
OAL_STATIC oal_bool_enum_uint8 hmac_get_grp_policy(const uint8_t *oui,
                                                   uint8_t       *grp_policy,
                                                   const uint8_t *ie_data,
                                                   uint32_t      *ie_len_left,
                                                   uint8_t       *index)
{
    /* 获取组播密钥套件,4个字节 */
    if (*ie_len_left > 4) {
        if (oal_memcmp(oui, ie_data + *index, MAC_OUI_LEN) != 0) {
            OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_get_grp_policy::invalid group OUI.}");
            return OAL_FALSE;
        }
        *grp_policy = ie_data[*index + MAC_OUI_LEN];

        /* 忽略组播密钥套件长度 */
        *index += 4;
        *ie_len_left -= 4;
    } else {
        OAM_WARNING_LOG1(0, OAM_SF_SCAN,
            "{hmac_get_grp_policy::ie too short group policy:%d.}", *ie_len_left);
        return OAL_FALSE;
    }
    return OAL_TRUE;
}

OAL_STATIC oal_bool_enum_uint8 hmac_get_pcip_policy(const uint8_t *oui,
                                                    uint8_t *pcip_policy,
                                                    const uint8_t *ie_data,
                                                    uint32_t *ie_len_left,
                                                    uint8_t *index)
{
    uint16_t temp;
    uint16_t pcip_num = 0;
    uint16_t suite_count = 0;

    /* 获取成对密钥套件个数,最少2个字节长度 */
    if (*ie_len_left > 2) {
        pcip_num = OAL_MAKE_WORD16(ie_data[*index], ie_data[*index + 1]) & 0xFF;
        *index += 2;
        *ie_len_left -= 2;
    } else {
        OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{hmac_get_grp_policy::ie too short for pairwise key mgmt:%d.}", *ie_len_left);
        return OAL_FALSE;
    }

    /* 确认单播套件个数是否与实际ie长度相匹配 */
    if ((pcip_num == 0) || (pcip_num > *ie_len_left / 4)) {
        OAM_WARNING_LOG2(0, OAM_SF_SCAN, "{hmac_get_grp_policy::ie count botch pairwise:%d,left:%d}",
            pcip_num, *ie_len_left);
        return OAL_FALSE;
    }
    for (temp = 0; temp < pcip_num; temp++) {
        if (oal_memcmp(oui, ie_data + *index, MAC_OUI_LEN) != 0) {
            OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_get_grp_policy::invalid pairwise OUI,ignore this ie.}");
            /* 对于不识别的成对密钥套件，忽略保存 */
            *index += 4;
            *ie_len_left -= 4;
            continue;
        }
        if (suite_count < MAC_PAIRWISE_CIPHER_SUITES_NUM) {
            /* 成对密钥套件个数驱动最大为2，超过则不再继续保存 */
            pcip_policy[suite_count++] = ie_data[*index + MAC_OUI_LEN];
        } else {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN,
                "{hmac_get_grp_policy::us_suite_count reach max,ignore this ie,pcip_num:%d.}", pcip_num);
        }
        *index += 4;
        *ie_len_left -= 4;
    }
    return OAL_TRUE;
}
OAL_STATIC oal_bool_enum_uint8 hmac_get_auth_policy(const uint8_t *oui,
                                                    uint8_t       *auth_policy,
                                                    const uint8_t *ie_data,
                                                    uint32_t      *ie_len_left,
                                                    uint8_t       *index)
{
    uint16_t temp;
    uint16_t suite_count = 0;
    uint16_t auth_num    = 0;

    if (OAL_ANY_NULL_PTR5(oui, auth_policy, ie_data, ie_len_left, index)) {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_get_auth_policy::invalid input params.}");
        return OAL_FALSE;
    }
    /* 获取成对密钥套件个数,最少2个字节长度 */
    if (*ie_len_left > 2) {
        auth_num = OAL_MAKE_WORD16(ie_data[*index], ie_data[*index + 1]) & 0xFF;
        *index += 2;
        *ie_len_left -= 2;
    } else {
        OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{hmac_get_auth_policy::ie too short for aka suite:%d.}", *ie_len_left);
        return OAL_FALSE;
    }
    /* 确认认证套件个数是否与实际ie长度相匹配 */
    if ((auth_num == 0) || (auth_num > (*ie_len_left / 4))) {
        OAM_WARNING_LOG2(0, OAM_SF_SCAN, "{hmac_get_auth_policy::ie count botch aka:%d,left:%d}", auth_num,
            *ie_len_left);
        return OAL_FALSE;
    }
    /* 获取认证类型 */
    for (temp = 0; temp < auth_num; temp++) {
        if (oal_memcmp(oui, ie_data + *index, MAC_OUI_LEN) != 0) {
            OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_get_auth_policy::invalid WPA auth OUI,ignore this ie.}");
            /* 对于不识别的AKM套件，忽略保存 */
            *index += 4;
            *ie_len_left -= 4;
            continue;
        }
        if (suite_count < WLAN_AUTHENTICATION_SUITES) {
            /* AKM套件个数驱动最大为2，超过则不再继续保存 */
            auth_policy[suite_count++] = ie_data[*index + MAC_OUI_LEN];
        } else {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN,
                "{hmac_get_auth_policy::us_suite_count reach max,ignore this ie,auth_num:%d.}", auth_num);
        }
        *index += 4;
        *ie_len_left -= 4;
    }
    return OAL_TRUE;
}


OAL_STATIC oal_bool_enum_uint8 hmac_parse_cipher_suit(mac_bss_dscr_stru *bss_dscr, uint8_t cipher_type,
    const uint8_t *ie_data, uint32_t len)
{
    uint8_t index = 0;
    uint32_t ie_len_left = len;
    uint8_t *oui = OAL_PTR_NULL;
    uint8_t *pcip_policy = OAL_PTR_NULL;
    uint8_t *grp_policy = OAL_PTR_NULL;
    uint8_t *auth_policy = OAL_PTR_NULL;
    mac_bss_80211i_info_stru *bss_80211i_info = &(bss_dscr->st_bss_sec_info);
    /* *********************************************************************** */
    /*                  WPA / RSN Element Format                             */
    /* --------------------------------------------------------------------- */
    /* |Group Cipher Suite | Pairwise Cipher Suite Count                     */
    /* --------------------------------------------------------------------- */
    /* |        4          |       2                                         */
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* | Pairwise Cipher Suite List | AKM Suite Count | AKM Suite List       */
    /* --------------------------------------------------------------------- */
    /* |         4*m                |     2           |   4*n                */
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* |RSN Capabilities|                                                    */
    /* --------------------------------------------------------------------- */
    /* |        2       |                                                    */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /* *********************************************************************** */
    /* 解析WPA/RSN公共的cipher suit */
    /* puc_ie指向Group Cipher Suite,l_len指puc_ie长度 */
    if (cipher_type == DMAC_WPA_802_11I) {
        oui = g_auc_wpa_oui;
        grp_policy  = &(bss_80211i_info->uc_wpa_grp_policy);
        pcip_policy = bss_80211i_info->auc_wpa_pairwise_policy;
        auth_policy = bss_80211i_info->auc_wpa_auth_policy;
    } else if (cipher_type == DMAC_RSNA_802_11I) {
        oui = g_auc_rsn_oui;
        grp_policy  = &(bss_80211i_info->uc_rsn_grp_policy);
        pcip_policy = bss_80211i_info->auc_rsn_pairwise_policy;
        auth_policy = bss_80211i_info->auc_rsn_auth_policy;
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_parse_cipher_suit::invalid OUI.}");
        return OAL_FALSE;
    }
    memset_s(pcip_policy, MAC_PAIRWISE_CIPHER_SUITES_NUM, 0xFF, MAC_PAIRWISE_CIPHER_SUITES_NUM);
    memset_s(auth_policy, MAC_AUTHENTICATION_SUITE_NUM, 0xFF, MAC_AUTHENTICATION_SUITE_NUM);
    /* 获取组播密钥套件 */
    if (hmac_get_grp_policy(oui, grp_policy, ie_data, &ie_len_left, &index) != OAL_TRUE) {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_parse_cipher_suit::hmac_get_grp_policy fail.}");
        return OAL_FALSE;
    }
    /* 获取成对密钥套件个数 */
    if (hmac_get_pcip_policy(oui, pcip_policy, ie_data, &ie_len_left, &index) != OAL_TRUE) {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_parse_cipher_suit::hmac_get_pcip_policy fail.}");
        return OAL_FALSE;
    }
    /* 获取认证套件计数 */
    if (hmac_get_auth_policy(oui, auth_policy, ie_data, &ie_len_left, &index) != OAL_TRUE) {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_parse_cipher_suit::hmac_get_auth_policy fail.}");
        return OAL_FALSE;
    }
    /* 获取RSN能力,2个字节 */
    if (cipher_type == DMAC_RSNA_802_11I) {
        if (ie_len_left >= 2) {
            bss_80211i_info->auc_rsn_cap[0] = *(ie_data + index++);
            bss_80211i_info->auc_rsn_cap[1] = *(ie_data + index++);
            ie_len_left -= 2;
        } else {
            /* 能力信息不关注失败情况,不返回失败,wpa_supplicant在解析时也不关注,可以正常关联 */
            OAM_WARNING_LOG1(0, OAM_SF_SCAN,
                "{hmac_scan_update_bss_list_rsn::ie too short for rsn cap:%d.}", ie_len_left);
        }
    }
    return OAL_TRUE;
}
#endif /* defined(_PRE_WLAN_FEATURE_WPA) || defined(_PRE_WLAN_FEATURE_WPA2) */


OAL_STATIC OAL_INLINE oal_void hmac_scan_update_bss_list_wmm(mac_bss_dscr_stru *pst_bss_dscr,
                                                             oal_uint8         *puc_frame_body,
                                                             oal_uint16        us_frame_len)
{
    oal_uint8 *puc_ie = OAL_PTR_NULL;

    pst_bss_dscr->uc_wmm_cap   = OAL_FALSE;
    pst_bss_dscr->uc_uapsd_cap = OAL_FALSE;

    if (us_frame_len <= (MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN)) {
        OAM_WARNING_LOG1(0, OAM_SF_ASSOC, "{hmac_scan_update_bss_list_wmm::us_frame_len[%d]!}", us_frame_len);
        return;
    }

    us_frame_len   -= (MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN);
    puc_frame_body += (MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN);

    puc_ie = mac_get_wmm_ie(puc_frame_body, us_frame_len);
    if (puc_ie != OAL_PTR_NULL) {
        pst_bss_dscr->uc_wmm_cap = OAL_TRUE;

        /* ---------------------------------------------------------------------------------  */
        /* WMM Information/Parameter Element Format                                           */
        /* ---------------------------------------------------------------------------------- */
        /* EID | IE LEN | OUI | OUIType | OUISubtype | Version | QoSInfo | OUISubtype based | */
        /* ---------------------------------------------------------------------------------  */
        /* 1   |   1    |  3  | 1       | 1          | 1       | 1       | ---------------- | */
        /* ---------------------------------------------------------------------------------  */
        /* puc_ie[1] IE len 不包含EID和LEN字段,获取QoSInfo，uc_ie_len必须大于7字节长度 */
        /* Check if Bit 7 is set indicating U-APSD capability */
        /* wmm ie的第8个字节是QoS info字节 */
        if ((puc_ie[1] >= 7) && (puc_ie[8] & BIT7)) {
            pst_bss_dscr->uc_uapsd_cap = OAL_TRUE;
        }
    } else {
        puc_ie = mac_find_ie(MAC_EID_HT_CAP, puc_frame_body, us_frame_len);
        if (puc_ie != OAL_PTR_NULL) {
            pst_bss_dscr->uc_wmm_cap = OAL_TRUE;
        }
    }
}

#ifdef _PRE_WLAN_FEATURE_11D

OAL_STATIC oal_void hmac_scan_update_bss_list_country(mac_bss_dscr_stru *pst_bss_dscr,
                                                      oal_uint8         *puc_frame_body,
                                                      oal_uint16        us_frame_len)
{
    oal_uint8 *puc_ie;
    oal_uint8 uc_offset;

    uc_offset = MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;
    puc_ie = mac_find_ie(MAC_EID_COUNTRY, puc_frame_body + uc_offset, us_frame_len - uc_offset);
    /* 国家码不存在, 全部标记为0 */
    if (puc_ie == OAL_PTR_NULL) {
        pst_bss_dscr->ac_country[0] = 0;
        pst_bss_dscr->ac_country[1] = 0;
        pst_bss_dscr->ac_country[2] = 0;

        return;
    }
    /* 国家码采用2个字节,IE LEN必须大于等于2 */
    if (puc_ie[1] >= 2) {
        pst_bss_dscr->ac_country[0] = (oal_int8)puc_ie[MAC_IE_HDR_LEN];
        pst_bss_dscr->ac_country[1] = (oal_int8)puc_ie[MAC_IE_HDR_LEN + 1];
        pst_bss_dscr->ac_country[2] = 0;
    }
}
#endif


OAL_STATIC oal_void hmac_scan_update_bss_list_11n(mac_bss_dscr_stru *pst_bss_dscr,
                                                  oal_uint8         *puc_frame_body,
                                                  oal_uint16        us_frame_len,
                                                  oal_uint16        us_offset)
{
    oal_uint8              *puc_ie       = OAL_PTR_NULL;
    mac_ht_opern_stru      *pst_ht_op    = OAL_PTR_NULL;
    oal_uint8              uc_sec_chan_offset;
    wlan_bw_cap_enum_uint8 en_ht_cap_bw = WLAN_BW_CAP_20M;
    wlan_bw_cap_enum_uint8 en_ht_op_bw  = WLAN_BW_CAP_20M;

    /* 11n */
    puc_ie = mac_find_ie(MAC_EID_HT_CAP, puc_frame_body + us_offset, us_frame_len - us_offset);
    if ((puc_ie != OAL_PTR_NULL) && (puc_ie[1] >= 2) && hmac_is_ht_mcs_set_valid(puc_ie)) {
        /* puc_ie[2]是HT Capabilities Info的第1个字节 */
        pst_bss_dscr->en_ht_capable = OAL_TRUE;     /* 支持ht */
        pst_bss_dscr->en_ht_ldpc = (puc_ie[2] & BIT0);           /* 支持ldpc */
        en_ht_cap_bw = ((puc_ie[2] & BIT1) >> 1);                /* 取出支持的带宽 */
        pst_bss_dscr->en_ht_stbc = ((puc_ie[2] & BIT7) >> 7);
    }

    /* 默认20M,如果帧内容未携带HT_OPERATION则可以直接采用默认值 */
    pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_20M;

    puc_ie = mac_find_ie(MAC_EID_HT_OPERATION, puc_frame_body + us_offset, us_frame_len - us_offset);
    if ((puc_ie != OAL_PTR_NULL) && (puc_ie[1] >= 2)) { // 增加ie长度异常检查
        pst_ht_op  = (mac_ht_opern_stru *)(puc_ie + MAC_IE_HDR_LEN);

        /* 提取次信道偏移 */
        uc_sec_chan_offset = pst_ht_op->bit_secondary_chan_offset;

        /* 防止ap的channel width=0, 但channel offset = 1或者3 此时以channel width为主 */
        /* ht cap 20/40 enabled && ht operation 40 enabled */
        if ((pst_ht_op->bit_sta_chan_width != 0) && (en_ht_cap_bw > WLAN_BW_CAP_20M)) { // cap > 20M才取channel bw
            if (uc_sec_chan_offset == MAC_SCB) {
                pst_bss_dscr->en_channel_bandwidth =  WLAN_BAND_WIDTH_40MINUS;
                en_ht_op_bw = WLAN_BW_CAP_40M;
            } else if (uc_sec_chan_offset == MAC_SCA) {
                pst_bss_dscr->en_channel_bandwidth =  WLAN_BAND_WIDTH_40PLUS;
                en_ht_op_bw = WLAN_BW_CAP_40M;
            }
        }
    }

    /* 将AP带宽能力取声明能力的最小值，防止AP异常发送超过带宽能力数据，造成数据不通 */
    pst_bss_dscr->en_bw_cap = OAL_MIN(en_ht_cap_bw, en_ht_op_bw);

    puc_ie = mac_find_ie(MAC_EID_EXT_CAPS, puc_frame_body + us_offset, us_frame_len - us_offset);
    if ((puc_ie != OAL_PTR_NULL) && (puc_ie[1] >= 1)) {
        /* Extract 20/40 BSS Coexistence Management Support */
        pst_bss_dscr->uc_coex_mgmt_supp = (puc_ie[2] & BIT0);
    }
}


OAL_STATIC oal_void  hmac_scan_update_bss_list_11ac(mac_bss_dscr_stru *pst_bss_dscr,
                                                    oal_uint8         *puc_frame_body,
                                                    oal_uint16        us_frame_len,
                                                    oal_uint16        us_offset)
{
    oal_uint8 *puc_ie;
    oal_uint8 uc_vht_chan_width;
    oal_uint8 uc_chan_center_freq;
    oal_uint8 uc_supp_ch_width;

    puc_ie = mac_find_ie(MAC_EID_VHT_CAP, puc_frame_body + us_offset, us_frame_len - us_offset);
    if ((puc_ie != OAL_PTR_NULL) && (puc_ie[1] >= MAC_VHT_CAP_IE_LEN)) {
        pst_bss_dscr->en_vht_capable = OAL_TRUE;     /* 支持vht */

        /* 提取Supported Channel Width Set */
        uc_supp_ch_width = ((puc_ie[2] & (BIT3 | BIT2)) >> 2);

        if (uc_supp_ch_width == 0) {
            pst_bss_dscr->en_bw_cap = WLAN_BW_CAP_80M;   /* 80MHz */
        } else if (uc_supp_ch_width == 1) {
            pst_bss_dscr->en_bw_cap = WLAN_BW_CAP_160M;  /* 160MHz */
        }
    }

    puc_ie = mac_find_ie(MAC_EID_VHT_OPERN, puc_frame_body + us_offset, us_frame_len - us_offset);
    if ((puc_ie != OAL_PTR_NULL) && (puc_ie[1] >= MAC_VHT_OPERN_LEN)) {
        uc_vht_chan_width   = puc_ie[2];
        uc_chan_center_freq = puc_ie[3];

        /* 更新带宽信息 */
        if (uc_vht_chan_width == 0) { /* 40MHz */
            /* do nothing，en_channel_bandwidth已经在HT Operation IE中获取 */
        } else if (uc_vht_chan_width == 1) { /* 80MHz */
            switch (uc_chan_center_freq - pst_bss_dscr->st_channel.uc_chan_number) {
                case 6:
                    /***********************************************************************
                    | 主20 | 从20 | 从40       |
                                  |
                                  |中心频率相对于主20偏6个信道
                    ************************************************************************/
                    pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_80PLUSPLUS;
                    break;

                case -2:
                    /***********************************************************************
                    | 从40        | 主20 | 从20 |
                                  |
                                  |中心频率相对于主20偏-2个信道
                    ************************************************************************/
                    pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_80PLUSMINUS;
                    break;

                case 2:
                    /***********************************************************************
                    | 从20 | 主20 | 从40       |
                                  |
                                  |中心频率相对于主20偏2个信道
                    ************************************************************************/
                    pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_80MINUSPLUS;
                    break;

                case -6:
                    /***********************************************************************
                    | 从40        | 从20 | 主20 |
                                  |
                                  |中心频率相对于主20偏-6个信道
                    ************************************************************************/
                    pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_80MINUSMINUS;
                    break;

                default:
                    break;
            }
        } else {
            /* Unsupported Channel BandWidth */
        }
    }
}
#ifdef _PRE_WLAN_FEATURE_1131K_BTCOEX
/*
 * 功能描述  : 通过oui信息判断是否是兼容性芯片厂商
 * 修改历史  :
 * 1.日    期: 2020年07月21日
 *   修改内容: 新生成函数
 */
OAL_STATIC void hmac_scan_compability_bss_chip(mac_bss_dscr_stru *bss_dscr, uint8_t *frame_body,
                                               uint16_t frame_len, uint16_t offset)
{
    bss_dscr->chip_oui_info.btcoex_blacklist_chip_oui = OAL_FALSE;
    bss_dscr->chip_oui_info.chip_oui = WLAN_AP_CHIP_OUI_NORMAL;

    /* 用于识别TUF 1900 ,识别芯片厂商OUI，结合用户bss识别AP类型 */
    if ((mac_find_vendor_ie(MAC_WLAN_CHIP_OUI_BROADCOM, MAC_WLAN_CHIP_OUI_TYPE_BROADCOM,
                            frame_body + offset, frame_len - offset) != NULL)) {
        bss_dscr->chip_oui_info.btcoex_blacklist_chip_oui = OAL_TRUE;
        bss_dscr->chip_oui_info.chip_oui = WLAN_AP_CHIP_OUI_BCM;
    } else if ((mac_find_vendor_ie(MAC_WLAN_CHIP_OUI_RALINK, MAC_WLAN_CHIP_OUI_TYPE_RALINK,
                                   frame_body + offset, frame_len - offset) != NULL)) {
        bss_dscr->chip_oui_info.btcoex_blacklist_chip_oui = OAL_TRUE;
        bss_dscr->chip_oui_info.chip_oui = WLAN_AP_CHIP_OUI_RALINK;
    } else if ((mac_find_vendor_ie(MAC_WLAN_CHIP_OUI_RALINK, MAC_WLAN_CHIP_OUI_TYPE_RALINK1,
                                   frame_body + offset, frame_len - offset) != NULL)) {
        bss_dscr->chip_oui_info.btcoex_blacklist_chip_oui = OAL_TRUE;
        bss_dscr->chip_oui_info.chip_oui = WLAN_AP_CHIP_OUI_RALINK1;
    } else if ((mac_find_vendor_ie(MAC_WLAN_CHIP_OUI_SHENZHEN, MAC_WLAN_CHIP_OUI_TYPE_SHENZHEN,
                                   frame_body + offset, frame_len - offset) != NULL)) {
        bss_dscr->chip_oui_info.btcoex_blacklist_chip_oui = OAL_TRUE;
        bss_dscr->chip_oui_info.chip_oui = WLAN_AP_CHIP_OUI_SHENZHEN;
    } else if ((mac_find_vendor_ie(MAC_WLAN_CHIP_OUI_ATHEROS, MAC_WLAN_CHIP_OUI_TYPE_ATHEROS,
                                   frame_body + offset, frame_len - offset) != NULL)) {
        bss_dscr->chip_oui_info.btcoex_blacklist_chip_oui = OAL_TRUE;
        bss_dscr->chip_oui_info.chip_oui = WLAN_AP_CHIP_OUI_ATHEROS;
    } else if ((mac_find_vendor_ie(MAC_WLAN_CHIP_OUI_MARVELL, MAC_WLAN_CHIP_OUI_TYPE_MARVELL,
                                   frame_body + offset, frame_len - offset) != NULL)) {
        bss_dscr->chip_oui_info.btcoex_blacklist_chip_oui = OAL_TRUE;
        bss_dscr->chip_oui_info.chip_oui = WLAN_AP_CHIP_OUI_MARVELL;
    }
}
#endif

OAL_INLINE OAL_STATIC oal_void hmac_scan_update_bss_list_protocol(mac_bss_dscr_stru *pst_bss_dscr,
                                                                  oal_uint8         *puc_frame_body,
                                                                  oal_uint16        us_frame_len)
    {
    oal_uint16 us_offset = MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;

    /* *********************************************************************** */
    /*                       Beacon Frame - Frame Body                       */
    /* ---------------------------------------------------------------------- */
    /* |Timestamp|BcnInt|CapInfo|SSID|SupRates|DSParamSet|TIM  |CountryElem | */
    /* ---------------------------------------------------------------------- */
    /* |8        |2     |2      |2-34|3-10    |3         |6-256|8-256       | */
    /* ---------------------------------------------------------------------- */
    /* |PowerConstraint |Quiet|TPC Report|ERP |RSN  |WMM |Extended Sup Rates| */
    /* ---------------------------------------------------------------------- */
    /* |3               |8    |4         |3   |4-255|26  | 3-257            | */
    /* ---------------------------------------------------------------------- */
    /* |BSS Load |HT Capabilities |HT Operation |Overlapping BSS Scan       | */
    /* ---------------------------------------------------------------------- */
    /* |7        |28              |24           |16                         | */
    /* ---------------------------------------------------------------------- */
    /* |Extended Capabilities |                                              */
    /* ---------------------------------------------------------------------- */
    /* |3-8                   |                                              */
    /* *********************************************************************** */
    /* wmm */
    hmac_scan_update_bss_list_wmm(pst_bss_dscr, puc_frame_body, us_frame_len);

#if defined(_PRE_WLAN_FEATURE_WPA) || defined(_PRE_WLAN_FEATURE_WPA2)
    /* 11i */
    hmac_scan_update_bss_list_security(pst_bss_dscr, puc_frame_body, us_frame_len, us_offset);
#endif

#ifdef _PRE_WLAN_FEATURE_11D
    /* 11d */
    hmac_scan_update_bss_list_country(pst_bss_dscr, puc_frame_body, us_frame_len);
#endif

    /* 11n */
    hmac_scan_update_bss_list_11n(pst_bss_dscr, puc_frame_body, us_frame_len, us_offset);

    /* 11ac */
    hmac_scan_update_bss_list_11ac(pst_bss_dscr, puc_frame_body, us_frame_len, us_offset);

#ifdef _PRE_WLAN_FEATURE_1131K_BTCOEX
    /* vendor Specific */
    hmac_scan_compability_bss_chip(pst_bss_dscr, puc_frame_body, us_frame_len, us_offset);
#endif
}


oal_uint8 hmac_scan_check_bss_supp_rates(mac_device_stru *pst_mac_dev,
                                         const oal_uint8 *puc_rate,
                                         oal_uint8 uc_bss_rate_num,
                                         oal_uint8 *puc_update_rate)
{
    mac_data_rate_stru *pst_rates;
    oal_uint32 i, j;
    oal_uint8 uc_rate_num = 0;

    pst_rates = mac_device_get_all_rates(pst_mac_dev);
    if (puc_rate != OAL_PTR_NULL) {
        for (i = 0; i < uc_bss_rate_num; i++) {
            for (j = 0; j < MAC_DATARATES_PHY_80211G_NUM; j++) {
                if (((pst_rates[j].uc_mac_rate & 0x7f) == (puc_rate[i] & 0x7f))
                        && (uc_rate_num < MAC_DATARATES_PHY_80211G_NUM)) {
                    puc_update_rate[uc_rate_num] = puc_rate[i];
                    uc_rate_num++;
                    break;
                }
            }
        }
    }

    return uc_rate_num;
}


OAL_INLINE OAL_STATIC oal_uint32 hmac_scan_update_bss_list_rates(mac_bss_dscr_stru *pst_bss_dscr,
                                                                 oal_uint8         *puc_frame_body,
                                                                 oal_uint16        us_frame_len,
                                                                 mac_device_stru   *pst_mac_dev)
{
    oal_uint8 *puc_ie;
    oal_uint8 uc_num_rates = 0;
    oal_uint8 uc_num_ex_rates;
    oal_uint8 us_offset;
    oal_uint8 auc_rates[MAC_DATARATES_PHY_80211G_NUM] = {0};

    /* 设置Beacon帧的field偏移量 */
    us_offset = MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;

    puc_ie = mac_find_ie(MAC_EID_RATES, puc_frame_body + us_offset, us_frame_len - us_offset);
    if (puc_ie != OAL_PTR_NULL) {
        uc_num_rates = hmac_scan_check_bss_supp_rates(pst_mac_dev, puc_ie + MAC_IE_HDR_LEN, puc_ie[1], auc_rates);
        if (uc_num_rates > WLAN_MAX_SUPP_RATES) {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{hmac_scan_update_bss_list_rates::uc_num_rates=%d.}", uc_num_rates);
            uc_num_rates = WLAN_MAX_SUPP_RATES;
        }
        if (memcpy_s(pst_bss_dscr->auc_supp_rates, WLAN_MAX_SUPP_RATES, auc_rates, uc_num_rates) != EOK) {
            OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_scan_update_bss_list_rates::memcpy_s fail!");
            return OAL_FAIL;
        }
        pst_bss_dscr->uc_num_supp_rates = uc_num_rates;
    }

    puc_ie = mac_find_ie(MAC_EID_XRATES, puc_frame_body + us_offset, us_frame_len - us_offset);
    if (puc_ie != OAL_PTR_NULL) {
        uc_num_ex_rates = hmac_scan_check_bss_supp_rates(pst_mac_dev, puc_ie + MAC_IE_HDR_LEN, puc_ie[1], auc_rates);
        if (uc_num_rates + uc_num_ex_rates > WLAN_MAX_SUPP_RATES) { /* 超出支持速率个数 */
            OAM_WARNING_LOG2(0, OAM_SF_SCAN,
                "{hmac_scan_update_bss_list_rates::number of rates too large, uc_num_rates=%d, uc_num_ex_rates=%d.}",
                uc_num_rates, uc_num_ex_rates);

            uc_num_ex_rates = WLAN_MAX_SUPP_RATES - uc_num_rates;
        }

        if (uc_num_ex_rates > 0) {
            if (memcpy_s(&(pst_bss_dscr->auc_supp_rates[uc_num_rates]), MAC_DATARATES_PHY_80211G_NUM,
                auc_rates, uc_num_ex_rates) != EOK) {
                OAM_ERROR_LOG0(0, OAM_SF_SCAN, "hmac_scan_update_bss_list_rates::memcpy fail!");
                return OAL_FAIL;
            }
        }
        pst_bss_dscr->uc_num_supp_rates += uc_num_ex_rates;
    }

    return OAL_SUCC;
}



oal_uint32 hmac_scan_update_bss_dscr(hmac_scanned_bss_info *pst_scanned_bss,
                                     dmac_tx_event_stru    *pst_dtx_event,
                                     oal_uint8             uc_device_id,
                                     oal_uint8             uc_vap_id)
{
    oal_netbuf_stru                     *pst_netbuf                  = pst_dtx_event->pst_netbuf;
    mac_scanned_result_extend_info_stru *pst_scan_result_extend_info = OAL_PTR_NULL;
    mac_device_stru                     *pst_mac_device              = OAL_PTR_NULL;
    hmac_vap_stru                       *pst_hmac_vap                = OAL_PTR_NULL;
    mac_ieee80211_frame_stru            *pst_frame_header            = OAL_PTR_NULL;
    oal_uint8                           *puc_frame_body              = OAL_PTR_NULL;
    mac_bss_dscr_stru                   *pst_bss_dscr                = OAL_PTR_NULL;
    oal_uint8                           *puc_ssid                    = OAL_PTR_NULL;  /* 指向beacon帧中的ssid */
    oal_uint8                           *puc_mgmt_frame              = OAL_PTR_NULL;
    dmac_rx_ctl_stru                    *pst_rx_ctrl                 = OAL_PTR_NULL;
    oal_uint16                          us_netbuf_len = pst_dtx_event->us_frame_len;
    oal_uint16                          us_frame_len;
    oal_uint16                          us_frame_body_len;
    oal_uint16                          us_offset =  MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;
    oal_uint8                           uc_ssid_len;
    oal_uint8                           uc_frame_channel;
#ifdef _PRE_WLAN_FEATURE_TXBF
    mac_11ntxbf_vendor_ie_stru          *puc_txbf_vendor_ie          = OAL_PTR_NULL;
#endif

    /* 获取hmac vap */
    pst_hmac_vap = mac_res_get_hmac_vap(uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_update_bss_dscr::pst_hmac_vap is null.}");
        return OAL_FAIL;
    }

    /* 获取mac device */
    pst_mac_device  = mac_res_get_dev(uc_device_id);
    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_update_bss_dscr::pst_mac_device is null.}");
        return OAL_FAIL;
    }

    /* 获取device上报的扫描结果信息，并将其更新到bss描述结构体中 */
    us_frame_len   = us_netbuf_len - OAL_SIZEOF(mac_scanned_result_extend_info_stru);
    puc_mgmt_frame = (oal_uint8 *)OAL_NETBUF_DATA(pst_netbuf);
    pst_rx_ctrl    = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

    /* 指向netbuf中的上报的扫描结果的扩展信息的位置 */
    pst_scan_result_extend_info = (mac_scanned_result_extend_info_stru *)(puc_mgmt_frame + us_frame_len);

    /* 获取管理帧的帧头和帧体指针 */
    pst_frame_header  = (mac_ieee80211_frame_stru *)puc_mgmt_frame;
    puc_frame_body    = (oal_uint8 *)(puc_mgmt_frame + MAC_80211_FRAME_LEN);
    us_frame_body_len = us_frame_len - MAC_80211_FRAME_LEN;

    /* 获取管理帧中的信道 */
    uc_frame_channel = mac_ie_get_chan_num(puc_frame_body, us_frame_body_len, us_offset,
        pst_rx_ctrl->st_rx_info.uc_channel_number);

    /* 更新bss信息 */
    pst_bss_dscr = &(pst_scanned_bss->st_bss_dscr_info);

    // 解析beacon/probe rsp帧，记录到pst_bss_dscr
    /* 解析并保存ssid */
    puc_ssid = mac_get_ssid(puc_frame_body, (oal_int32)us_frame_body_len, &uc_ssid_len);
    if ((puc_ssid != OAL_PTR_NULL) && (uc_ssid_len != 0)) {
        /* 将查找到的ssid保存到bss描述结构体中 */
        if (memcpy_s(pst_bss_dscr->ac_ssid, WLAN_SSID_MAX_LEN, puc_ssid, uc_ssid_len) != EOK) {
            OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_update_bss_dscr::memcpy_s failed.}");
            return OAL_FAIL;
        }
        pst_bss_dscr->ac_ssid[uc_ssid_len] = '\0';
    }

    /* 解析bssid */
    oal_set_mac_addr(pst_bss_dscr->auc_mac_addr, pst_frame_header->auc_address2);
    oal_set_mac_addr(pst_bss_dscr->auc_bssid, pst_frame_header->auc_address3);

    /* bss基本信息 */
    pst_bss_dscr->en_bss_type = pst_scan_result_extend_info->en_bss_type;

    pst_bss_dscr->us_cap_info = *((oal_uint16 *)(puc_frame_body + MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN));

    pst_bss_dscr->c_rssi      = (oal_int8)pst_scan_result_extend_info->l_rssi;

    /* 解析beacon周期与tim周期 */
    pst_bss_dscr->us_beacon_period = mac_get_beacon_period(puc_frame_body);
    pst_bss_dscr->uc_dtim_period   = mac_get_dtim_period(puc_frame_body, us_frame_body_len);
    pst_bss_dscr->uc_dtim_cnt      = mac_get_dtim_cnt(puc_frame_body, us_frame_body_len);

    /* 信道 */
    pst_bss_dscr->st_channel.uc_chan_number = uc_frame_channel;
    pst_bss_dscr->st_channel.en_band        = mac_get_band_by_channel_num(uc_frame_channel);

    /* 记录速率集 */
    hmac_scan_update_bss_list_rates(pst_bss_dscr, puc_frame_body, us_frame_body_len, pst_mac_device);

    /* 协议类相关信息元素的获取 */
    hmac_scan_update_bss_list_protocol(pst_bss_dscr, puc_frame_body, us_frame_body_len);

#ifdef _PRE_WLAN_FEATURE_TXBF
    puc_txbf_vendor_ie = (mac_11ntxbf_vendor_ie_stru *)mac_find_vendor_ie(MAC_HUAWEI_VENDER_IE, MAC_EID_11NTXBF,
        puc_frame_body + us_offset, (oal_int32)(us_frame_body_len - us_offset));
    if (puc_txbf_vendor_ie != OAL_PTR_NULL) {
        pst_bss_dscr->en_11ntxbf = (puc_txbf_vendor_ie->st_11ntxbf.bit_11ntxbf == 1) ? OAL_TRUE : OAL_FALSE;
    }
#endif

    /* 更新时间戳 */
    pst_bss_dscr->ul_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    pst_bss_dscr->ul_mgmt_len = us_frame_len;

    /* 拷贝管理帧内容 */
    if (memcpy_s((uint8_t *)pst_scanned_bss->st_bss_dscr_info.auc_mgmt_buff, (uint32_t)us_frame_len,
        puc_mgmt_frame, (uint32_t)us_frame_len) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_update_bss_dscr::memcpy_s failed.}");
        return OAL_FAIL;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_bool_enum_uint8 hmac_scan_need_update_timestamp(oal_uint8 uc_vap_id, hmac_scanned_bss_info *pst_new_bss,
    hmac_scanned_bss_info *pst_old_bss)
{
    if ((pst_new_bss->st_bss_dscr_info.ac_ssid[0] == '\0') &&
        (pst_old_bss->st_bss_dscr_info.ac_ssid[0] != '\0')) {
        return OAL_FALSE;
    }

    return OAL_TRUE;
}


oal_bool_enum_uint8 hmac_scan_need_update_old_scan_result(oal_uint8 uc_vap_id, hmac_scanned_bss_info *pst_new_bss,
    hmac_scanned_bss_info *pst_old_bss)
{
    
    if ((((mac_ieee80211_frame_stru *)pst_old_bss->st_bss_dscr_info.auc_mgmt_buff)->st_frame_control.bit_sub_type == \
        WLAN_PROBE_RSP) &&
        (((mac_ieee80211_frame_stru *)pst_new_bss->st_bss_dscr_info.auc_mgmt_buff)->st_frame_control.bit_sub_type == \
        WLAN_BEACON) &&
        (pst_old_bss->st_bss_dscr_info.en_new_scan_bss == OAL_TRUE)) {
        return OAL_FALSE;
    }
    return OAL_TRUE;
}

oal_uint32 hmac_scan_proc_scanned_bss(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru        *pst_event           = OAL_PTR_NULL;
    frw_event_hdr_stru    *pst_event_hdr       = OAL_PTR_NULL;
    hmac_vap_stru         *pst_hmac_vap        = OAL_PTR_NULL;
    oal_netbuf_stru       *pst_bss_mgmt_netbuf = OAL_PTR_NULL;
    hmac_device_stru      *pst_hmac_device     = OAL_PTR_NULL;
    dmac_tx_event_stru    *pst_dtx_event       = OAL_PTR_NULL;
    hmac_scanned_bss_info *pst_new_scanned_bss = OAL_PTR_NULL;
    hmac_scanned_bss_info *pst_old_scanned_bss = OAL_PTR_NULL;
    oal_uint32            ul_ret;
    oal_uint16            us_mgmt_len;
    oal_uint8             uc_vap_id;
    hmac_bss_mgmt_stru    *pst_bss_mgmt        = OAL_PTR_NULL;
    oal_uint32            ul_curr_time_stamp;

    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_proc_scanned_bss::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取事件头和事件结构体指针 */
    pst_event           = (frw_event_stru *)pst_event_mem->puc_data;
    pst_event_hdr       = &(pst_event->st_event_hdr);
    pst_dtx_event       = (dmac_tx_event_stru *)pst_event->auc_event_data;
    pst_bss_mgmt_netbuf = pst_dtx_event->pst_netbuf;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_event_hdr->uc_vap_id);
    if (OAL_UNLIKELY(pst_hmac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_proc_scanned_bss::pst_hmac_vap null.}");

        /* 释放上报的bss信息和beacon或者probe rsp帧的内存 */
        oal_netbuf_free(pst_bss_mgmt_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取vap id */
    uc_vap_id = pst_hmac_vap->st_vap_base_info.uc_vap_id;

    /* 获取hmac device 结构 */
    pst_hmac_device = hmac_res_get_mac_dev(pst_event_hdr->uc_device_id);
    if (OAL_UNLIKELY(pst_hmac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_scanned_bss::pst_hmac_device null.}");

        /* 释放上报的bss信息和beacon或者probe rsp帧的内存 */
        oal_netbuf_free(pst_bss_mgmt_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 对dmac上报的netbuf内容进行解析，内容如下所示 */
    /* ********************************************************************************************* */
    /*            netbuf data域的上报的扫描结果的字段的分布                                        */
    /* ------------------------------------------------------------------------------------------  */
    /* beacon/probe rsp body  |     帧体后面附加字段(mac_scanned_result_extend_info_stru)          */
    /* -----------------------------------------------------------------------------------------   */
    /* 收到的beacon/rsp的body | rssi(4字节) | channel num(1字节)| band(1字节)|bss_tye(1字节)|填充  */
    /* ------------------------------------------------------------------------------------------  */
    /*                                                                                             */
    /* ********************************************************************************************* */
    /* 管理帧的长度等于上报的netbuf的长度减去上报的扫描结果的扩展字段的长度 */
    us_mgmt_len = pst_dtx_event->us_frame_len - OAL_SIZEOF(mac_scanned_result_extend_info_stru);

    /* 申请存储扫描结果的内存 */
    pst_new_scanned_bss = hmac_scan_alloc_scanned_bss(us_mgmt_len);
    if (OAL_UNLIKELY(pst_new_scanned_bss == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_proc_scanned_bss::alloc memory failed for storing scanned result.}");

        /* 释放上报的bss信息和beacon或者probe rsp帧的内存 */
        oal_netbuf_free(pst_bss_mgmt_netbuf);
        return OAL_PTR_NULL;
    }

    /* 更新描述扫描结果的bss dscr结构体 */
    ul_ret = hmac_scan_update_bss_dscr(pst_new_scanned_bss, pst_dtx_event, pst_event_hdr->uc_device_id,
        pst_event_hdr->uc_vap_id);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_scanned_bss::hmac_scan_update_bss_dscr failed[%d].}",
            ul_ret);

        /* 释放上报的bss信息和beacon或者probe rsp帧的内存 */
        oal_netbuf_free(pst_bss_mgmt_netbuf);

        /* 释放申请的存储bss信息的内存 */
        oal_free(pst_new_scanned_bss);
        return ul_ret;
    }

    /* 获取管理扫描的bss结果的结构体 */
    pst_bss_mgmt = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.st_bss_mgmt);
    /* 对链表删操作前加锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));
    /* 判断相同bssid的bss是否已经扫描到 */
    pst_old_scanned_bss = hmac_scan_find_scanned_bss_by_bssid(pst_bss_mgmt,
                                                              pst_new_scanned_bss->st_bss_dscr_info.auc_bssid);
    if (pst_old_scanned_bss == OAL_PTR_NULL) {
        /* 解锁 */
        oal_spin_unlock(&(pst_bss_mgmt->st_lock));

        /*lint -e801*/
        goto add_bss;
        /*lint +e801*/
    }

    /* 如果老的扫描的bss的信号强度大于当前扫描到的bss的信号强度，更新当前扫描到的信号强度为最强的信号强度 */
    if (pst_old_scanned_bss->st_bss_dscr_info.c_rssi > pst_new_scanned_bss->st_bss_dscr_info.c_rssi) {
        /* 1s中以内就采用之前的BSS保存的RSSI信息，否则就采用新的RSSI信息 */
        ul_curr_time_stamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();
        if ((ul_curr_time_stamp - pst_old_scanned_bss->st_bss_dscr_info.ul_timestamp) <
            HMAC_SCAN_MAX_SCANNED_RSSI_EXPIRE) {
            OAM_INFO_LOG0(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_scanned_bss::update signal strength value.}");
            pst_new_scanned_bss->st_bss_dscr_info.c_rssi = pst_old_scanned_bss->st_bss_dscr_info.c_rssi;
        }
    }

    if (hmac_scan_need_update_timestamp(uc_vap_id, pst_new_scanned_bss, pst_old_scanned_bss) == OAL_FALSE) {
        /* 解锁 */
        oal_spin_unlock(&(pst_bss_mgmt->st_lock));

        /* 释放申请的存储bss信息的内存 */
        oal_free(pst_new_scanned_bss);

        /* 释放上报的bss信息和beacon或者probe rsp帧的内存 */
        oal_netbuf_free(pst_bss_mgmt_netbuf);

        return OAL_SUCC;
    }
    if (hmac_scan_need_update_old_scan_result(uc_vap_id, pst_new_scanned_bss, pst_old_scanned_bss) == OAL_FALSE) {
        pst_old_scanned_bss->st_bss_dscr_info.ul_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();
        pst_old_scanned_bss->st_bss_dscr_info.c_rssi = pst_new_scanned_bss->st_bss_dscr_info.c_rssi;

        /* 解锁 */
        oal_spin_unlock(&(pst_bss_mgmt->st_lock));

        /* 释放申请的存储bss信息的内存 */
        oal_free(pst_new_scanned_bss);

        /* 释放上报的bss信息和beacon或者probe rsp帧的内存 */
        oal_netbuf_free(pst_bss_mgmt_netbuf);

        return OAL_SUCC;
    }

    /* 从链表中将原先扫描到的相同bssid的bss节点删除 */
    hmac_scan_del_bss_from_list_nolock(pst_old_scanned_bss, pst_hmac_device);
    /* 解锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));
    /* 释放内存 */
    oal_free(pst_old_scanned_bss);

add_bss:
    /* 将扫描结果添加到链表中 */
    hmac_scan_add_bss_to_list(pst_new_scanned_bss, pst_hmac_device);

    /* 释放上报的bss信息和beacon或者probe rsp帧的内存 */
    oal_netbuf_free(pst_bss_mgmt_netbuf);

    return OAL_SUCC;
}


OAL_STATIC oal_void hmac_scan_print_channel_statistics_info(hmac_scan_record_stru *pst_scan_record)
{
    mac_scan_chan_stats_stru *chan_stats = pst_scan_record->ast_chan_results;
    oal_uint8                idx;

    /* 检测本次扫描是否开启了信道测量，如果没有直接返回 */
    if (chan_stats[0].uc_stats_valid == 0) {
        OAM_INFO_LOG0(pst_scan_record->uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_print_channel_statistics_info:: curr scan don't enable channel measure.\n}");
        return;
    }

    /* 打印信道测量结果 */
    OAM_INFO_LOG0(0, OAM_SF_SCAN, "{hmac_scan_print_channel_statistics_info:: The chan measure result: \n}");

    for (idx = 0; idx < pst_scan_record->uc_chan_numbers; idx++) {
        OAM_INFO_LOG1(pst_scan_record->uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_print_channel_statistics_info::Chan num      : %d\n}", chan_stats[idx].uc_channel_number);
        OAM_INFO_LOG1(pst_scan_record->uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_print_channel_statistics_info::Stats cnt     : %d\n}", chan_stats[idx].uc_stats_cnt);
        OAM_INFO_LOG1(pst_scan_record->uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_print_channel_statistics_info::Stats valid   : %d\n}", chan_stats[idx].uc_stats_valid);
        OAM_INFO_LOG1(pst_scan_record->uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_print_channel_statistics_info::Stats time us : %d\n}", chan_stats[idx].ul_total_stats_time_us);
        OAM_INFO_LOG1(pst_scan_record->uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_print_channel_statistics_info::Free time 20M : %d\n}",
            chan_stats[idx].ul_total_free_time_20M_us);
        OAM_INFO_LOG1(pst_scan_record->uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_print_channel_statistics_info::Free time 40M : %d\n}",
            chan_stats[idx].ul_total_free_time_40M_us);
        OAM_INFO_LOG1(pst_scan_record->uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_print_channel_statistics_info::Free time 80M : %d\n}",
            chan_stats[idx].ul_total_free_time_80M_us);
        OAM_INFO_LOG1(pst_scan_record->uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_print_channel_statistics_info::Send time     : %d\n}", chan_stats[idx].ul_total_send_time_us);
        OAM_INFO_LOG1(pst_scan_record->uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_print_channel_statistics_info::Recv time     : %d\n}", chan_stats[idx].ul_total_recv_time_us);
        OAM_INFO_LOG1(pst_scan_record->uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_print_channel_statistics_info::Free power cnt: %d\n}", chan_stats[idx].uc_free_power_cnt);
        OAM_INFO_LOG1(pst_scan_record->uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_print_channel_statistics_info::Free power 20M: %d\n}",
            (oal_int32)chan_stats[idx].s_free_power_stats_20M);
        OAM_INFO_LOG1(pst_scan_record->uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_print_channel_statistics_info::Free power 40M: %d\n}",
            (oal_int32)chan_stats[idx].s_free_power_stats_40M);
        OAM_INFO_LOG1(pst_scan_record->uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_print_channel_statistics_info::Free power 80M: %d\n}",
            (oal_int32)chan_stats[idx].s_free_power_stats_80M);
        OAM_INFO_LOG1(pst_scan_record->uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_print_channel_statistics_info::Radar detected: %d\n}", chan_stats[idx].uc_radar_detected);
        OAM_INFO_LOG1(pst_scan_record->uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_print_channel_statistics_info::Radar bw      : %d\n}", chan_stats[idx].uc_radar_bw);
    }

    return;
}


OAL_STATIC oal_void hmac_scan_print_scan_record_info(hmac_vap_stru *pst_hmac_vap,
    hmac_scan_record_stru *pst_scan_record)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && (_PRE_PRODUCT_ID_HI1131C_HOST == _PRE_PRODUCT_ID)
    oal_time_t_stru st_timestamp_diff;

    /* 获取扫描间隔时间戳 */
    st_timestamp_diff = oal_ktime_sub(oal_ktime_get(), pst_scan_record->st_scan_start_time);

    /* 调用内核接口，打印此次扫描耗时 */
    OAM_WARNING_LOG4(pst_scan_record->uc_vap_id, OAM_SF_SCAN,
        "{hmac_scan_print_scan_record_info::scan comp, scan_status[%d],vap ch_num:%d, cookie[%x], duration time \
        is: [%lu]ms.}", pst_scan_record->en_scan_rsp_status, pst_hmac_vap->st_vap_base_info.st_channel.uc_chan_number,
        pst_scan_record->ull_cookie, ktime_to_ms(st_timestamp_diff));
#endif

    /* 打印扫描到的bss信息 */
    hmac_scan_print_scanned_bss_info(pst_scan_record->uc_device_id);

    /* 信道测量结果 */
    hmac_scan_print_channel_statistics_info(pst_scan_record);

    return;
}


oal_uint32  hmac_scan_proc_scan_comp_event(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru     *pst_event             = OAL_PTR_NULL;
    frw_event_hdr_stru *pst_event_hdr         = OAL_PTR_NULL;
    hmac_vap_stru      *pst_hmac_vap          = OAL_PTR_NULL;
    hmac_device_stru   *pst_hmac_device       = OAL_PTR_NULL;
    mac_scan_rsp_stru  *pst_d2h_scan_rsp_info = OAL_PTR_NULL;
    hmac_scan_stru     *pst_scan_mgmt         = OAL_PTR_NULL;

    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_proc_scan_comp_event::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取事件头和事件结构体指针 */
    pst_event     = (frw_event_stru *)pst_event_mem->puc_data;
    pst_event_hdr = &(pst_event->st_event_hdr);

    /* 获取hmac device */
    pst_hmac_device = hmac_res_get_mac_dev(pst_event_hdr->uc_device_id);
    if (OAL_UNLIKELY(pst_hmac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_proc_scan_comp_event::pst_hmac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_d2h_scan_rsp_info = (mac_scan_rsp_stru *)(pst_event->auc_event_data);
    pst_scan_mgmt = &(pst_hmac_device->st_scan_mgmt);

    if ((pst_event_hdr->uc_vap_id != pst_scan_mgmt->st_scan_record_mgmt.uc_vap_id) ||
            (pst_d2h_scan_rsp_info->ull_cookie != pst_scan_mgmt->st_scan_record_mgmt.ull_cookie)) {
        OAM_WARNING_LOG4(pst_event_hdr->uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_proc_scan_comp_event::Report vap(%d) Scan_complete(cookie %d), but there have anoter vap(%d) \
            scaning(cookie %d) !}",pst_event_hdr->uc_vap_id, pst_d2h_scan_rsp_info->ull_cookie,
            pst_scan_mgmt->st_scan_record_mgmt.uc_vap_id, pst_scan_mgmt->st_scan_record_mgmt.ull_cookie);
        return OAL_SUCC;
    }

    /* 删除扫描超时保护定时器 */
    if (pst_scan_mgmt->st_scan_timeout.en_is_registerd == OAL_TRUE) {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_scan_mgmt->st_scan_timeout));
    }

    /* 获取hmac vap */
    pst_hmac_vap = mac_res_get_hmac_vap(pst_event_hdr->uc_vap_id);
    if (OAL_UNLIKELY(pst_hmac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_scan_comp_event::pst_hmac_vap null.}");

        /* 设置当前处于非扫描状态 */
        pst_scan_mgmt->en_is_scanning = OAL_FALSE;
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 根据当前扫描的类型和当前vap的状态，决定切换vap的状态，如果是前景扫描，才需要切换vap的状态 */
    if (pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        if (pst_hmac_vap->st_vap_base_info.en_vap_state == MAC_VAP_STATE_STA_WAIT_SCAN) {
            /* 改变vap状态到SCAN_COMP */
            hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_STA_SCAN_COMP);
        } else if (pst_hmac_vap->st_vap_base_info.en_vap_state == MAC_VAP_STATE_UP) {
            /* 背景扫描时需要进行帧过滤的配置 */
            hmac_set_rx_filter_value(&(pst_hmac_vap->st_vap_base_info));
        }
    }

    if ((pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_AP)
            && (pst_scan_mgmt->st_scan_record_mgmt.en_vap_last_state != MAC_VAP_STATE_BUTT)) {
        hmac_fsm_change_state(pst_hmac_vap, pst_scan_mgmt->st_scan_record_mgmt.en_vap_last_state);
        pst_scan_mgmt->st_scan_record_mgmt.en_vap_last_state = MAC_VAP_STATE_BUTT;
    }
    /* 根据device上报的扫描结果，上报sme */
    /* 将扫描执行情况(扫描执行成功、还是失败等返回结果)记录到扫描运行记录结构体中 */
    pst_scan_mgmt->st_scan_record_mgmt.en_scan_rsp_status = pst_d2h_scan_rsp_info->en_scan_rsp_status;
    pst_scan_mgmt->st_scan_record_mgmt.ull_cookie         = pst_d2h_scan_rsp_info->ull_cookie;

    /* 上报扫描结果前，清除下到期的扫描bss，防止上报过多到期的bss */
    hmac_scan_clean_expire_scanned_bss(&(pst_scan_mgmt->st_scan_record_mgmt));

#if 0
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    if (pst_scan_mgmt->st_scan_record_mgmt.st_bss_mgmt.ul_bss_num <= 2) {
        OAM_WARNING_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_SCAN, "{scan completed, but bss num is too few!}");
        /* 下发配置命令，读取所有寄存器 */
        hmac_config_reg_info(&(pst_hmac_vap->st_vap_base_info), OAL_SIZEOF(auc_param), auc_param);
    }
#endif
#endif
    hmac_scan_print_scan_record_info(pst_hmac_vap, &(pst_scan_mgmt->st_scan_record_mgmt));

#ifdef _PRE_WLAN_FEATURE_ROAM
    /* STA背景扫描时，需要提前识别漫游场景 */
    if ((pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_STA) &&
            (pst_hmac_vap->st_vap_base_info.en_vap_state == MAC_VAP_STATE_UP)) {
        hmac_roam_check_bkscan_result(pst_hmac_vap, &(pst_scan_mgmt->st_scan_record_mgmt));
    }
#endif // _PRE_WLAN_FEATURE_ROAM

    /* 如果扫描回调函数不为空，则调用回调函数 */
    if (pst_scan_mgmt->st_scan_record_mgmt.p_fn_cb != OAL_PTR_NULL) {
        pst_scan_mgmt->st_scan_record_mgmt.p_fn_cb(&(pst_scan_mgmt->st_scan_record_mgmt));
    }

    /* 设置当前处于非扫描状态 */
    pst_scan_mgmt->en_is_scanning = OAL_FALSE;

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST

oal_uint32 hmac_scan_proc_obss_scan_comp_event(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru        *pst_event = NULL;
    frw_event_hdr_stru    *pst_event_hdr = NULL;
    dmac_obss_te_a_b_stru *dmac_obss_te_a_b = NULL;
    hmac_vap_stru         *pst_vap = NULL;  /* vap指针 */
    mac_vap_stru          *pst_mac_vap = NULL;

    if (pst_event_mem == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_scan_proc_obss_scan_comp_event::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取事件头和事件结构体指针 */
    pst_event        = (frw_event_stru *)pst_event_mem->puc_data;
    pst_event_hdr    = &(pst_event->st_event_hdr);
    dmac_obss_te_a_b = (dmac_obss_te_a_b_stru *)(pst_event->auc_event_data);

    /* 获取vap结构信息 */
    pst_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_event_hdr->uc_vap_id);
    if (OAL_UNLIKELY(pst_vap == OAL_PTR_NULL)) {
        OAM_WARNING_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_obss_scan_comp_event::pst_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_mac_vap = &pst_vap->st_vap_base_info;

    OAM_INFO_LOG4(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
        "{hmac_scan_proc_obss_scan_comp_event::old_te_a:%d,old_te_b:%d, new_te_a=%d, new_te_b=%d}",
        pst_vap->st_vap_base_info.st_ch_switch_info.ul_chan_report_for_te_a,
        pst_vap->st_vap_base_info.st_ch_switch_info.en_te_b,
        dmac_obss_te_a_b->ul_chan_report_for_te_a, dmac_obss_te_a_b->en_te_b);

    pst_vap->st_vap_base_info.st_ch_switch_info.ul_chan_report_for_te_a = dmac_obss_te_a_b->ul_chan_report_for_te_a;
    pst_vap->st_vap_base_info.st_ch_switch_info.en_te_b = dmac_obss_te_a_b->en_te_b;

    hmac_send_2040_coext_mgmt_frame_sta(pst_mac_vap);

    return OAL_SUCC;
}
#endif


oal_uint32 hmac_scan_proc_scan_req_event_exception(hmac_vap_stru *pst_hmac_vap, oal_void *p_params)
{
    frw_event_mem_stru *pst_event_mem = OAL_PTR_NULL;
    frw_event_stru     *pst_event     = OAL_PTR_NULL;
    hmac_scan_rsp_stru st_scan_rsp;
    hmac_scan_rsp_stru *pst_scan_rsp  = OAL_PTR_NULL;

    if (OAL_UNLIKELY((pst_hmac_vap == OAL_PTR_NULL) || (p_params == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_mgmt_scan_req_exception::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 不支持发起扫描的状态发起了扫描 */
    OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
        "{hmac_mgmt_scan_req_exception::vap state is=%x.}", pst_hmac_vap->st_vap_base_info.en_vap_state);

    memset_s(&st_scan_rsp, OAL_SIZEOF(hmac_scan_rsp_stru), 0, OAL_SIZEOF(hmac_scan_rsp_stru));

    /* 抛扫描完成事件到WAL, 执行SCAN_DONE , 释放扫描请求内存 */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(hmac_scan_rsp_stru));
    if (pst_event_mem == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
            "{hmac_mgmt_scan_req_exception::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    st_scan_rsp.uc_result_code = HMAC_MGMT_REFUSED;
    st_scan_rsp.uc_num_dscr    = 0;

    /* 填写事件 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_CTX,
                       HMAC_HOST_CTX_EVENT_SUB_TYPE_SCAN_COMP_STA,
                       OAL_SIZEOF(hmac_scan_rsp_stru),
                       FRW_EVENT_PIPELINE_STAGE_0,
                       pst_hmac_vap->st_vap_base_info.uc_chip_id,
                       pst_hmac_vap->st_vap_base_info.uc_device_id,
                       pst_hmac_vap->st_vap_base_info.uc_vap_id);

    pst_scan_rsp = (hmac_scan_rsp_stru *)pst_event->auc_event_data;

    if (memcpy_s(pst_scan_rsp, OAL_SIZEOF(hmac_scan_rsp_stru), (void *)(&st_scan_rsp),
                 OAL_SIZEOF(hmac_scan_rsp_stru)) != EOK) {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
            "hmac_mgmt_scan_req_exception::memcpy_s failed!");
        FRW_EVENT_FREE(pst_event_mem);
        return OAL_FAIL;
    }

    /* 分发事件 */
    frw_event_dispatch_event(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}



oal_void  hmac_scan_set_sour_mac_addr_in_probe_req(hmac_vap_stru *pst_hmac_vap,
                                                   oal_uint8 *puc_sour_mac_addr,
                                                   oal_bool_enum_uint8 en_is_rand_mac_addr_scan,
                                                   oal_bool_enum_uint8 en_is_p2p0_scan)
{
#ifdef _PRE_WLAN_FEATURE_P2P
    /* WLAN/P2P 特性情况下，p2p0 和p2p-p2p0 cl 扫描时候，需要使用不同设备 */
    if (en_is_p2p0_scan == OAL_TRUE) {
        oal_set_mac_addr(puc_sour_mac_addr,
            pst_hmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.auc_p2p0_dot11StationID);
    } else
#endif /* _PRE_WLAN_FEATURE_P2P */
    {
        /* 如果随机mac addr扫描特性开启且非P2P场景，设置随机mac addr到probe req帧中 */
        if ((en_is_rand_mac_addr_scan == OAL_TRUE) && (IS_LEGACY_VAP(&(pst_hmac_vap->st_vap_base_info)))) {
            oal_random_ether_addr(puc_sour_mac_addr);
            puc_sour_mac_addr[0] &= (~0x02); /* wlan0 MAC[0] bit1 需要设置为0 */
            puc_sour_mac_addr[1] = 0x11;
            puc_sour_mac_addr[2] = 0x02;

            OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                "{hmac_scan_set_sour_mac_addr_in_probe_req::rand_mac_addr[%02X:XX:XX:%02X:%02X:%02X].}",
                puc_sour_mac_addr[0], puc_sour_mac_addr[3], puc_sour_mac_addr[4], puc_sour_mac_addr[5]);
        } else {
            /* 设置地址为自己的MAC地址 */
            oal_set_mac_addr(puc_sour_mac_addr,
                pst_hmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);
        }
    }

    return;
}


OAL_STATIC oal_uint32 hmac_scan_update_scan_params(hmac_vap_stru *pst_hmac_vap, mac_scan_req_stru *pst_scan_params,
    oal_bool_enum_uint8 en_is_random_mac_addr_scan)
{
    mac_device_stru          *pst_mac_device;
    mac_vap_stru             *pst_mac_vap_temp = OAL_PTR_NULL;
    oal_uint32               ul_ret;
    wlan_vap_mode_enum_uint8 en_vap_mode;

    /* 获取mac device */
    pst_mac_device = mac_res_get_dev(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_device_id, OAM_SF_SCAN,
                         "{hmac_scan_update_scan_params::pst_mac_device null.}");
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }

    /* 1.记录发起扫描的vap id到扫描参数 */
    pst_scan_params->uc_vap_id    = pst_hmac_vap->st_vap_base_info.uc_vap_id;
    pst_scan_params->en_scan_mode = WLAN_SCAN_MODE_FOREGROUND;
    pst_scan_params->en_need_switch_back_home_channel = OAL_FALSE;

    /* 2.修改扫描模式和信道扫描次数: 根据是否存在up状态下的vap，如果是，则是背景扫描，如果不是，则是前景扫描 */
    ul_ret = mac_device_find_up_vap(pst_mac_device, &pst_mac_vap_temp);
    if ((ul_ret == OAL_SUCC) && (pst_mac_vap_temp != OAL_PTR_NULL)) {
        /* 判断vap的类型，如果是sta则为sta的背景扫描，如果是ap，则是ap的背景扫描，其它类型的vap暂不支持背景扫描 */
        en_vap_mode = pst_hmac_vap->st_vap_base_info.en_vap_mode;
        if (en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
            /* 修改扫描参数为sta的背景扫描 */
            pst_scan_params->en_scan_mode = WLAN_SCAN_MODE_BACKGROUND_STA;
        } else if (en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
            /* 修改扫描参数为sta的背景扫描 */
            pst_scan_params->en_scan_mode = WLAN_SCAN_MODE_BACKGROUND_AP;
        } else {
            OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{hmac_scan_update_scan_params::vap mode[%d], not support bg scan.}",
                en_vap_mode);
            return OAL_FAIL;
        }
        pst_scan_params->en_need_switch_back_home_channel = OAL_TRUE;
    }
    /* 3.设置发送的probe req帧中源mac addr */
    pst_scan_params->en_is_random_mac_addr_scan = en_is_random_mac_addr_scan;
    hmac_scan_set_sour_mac_addr_in_probe_req(pst_hmac_vap, pst_scan_params->auc_sour_mac_addr,
        en_is_random_mac_addr_scan, pst_scan_params->bit_is_p2p0_scan);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_scan_check_can_enter_scan_state(mac_vap_stru *pst_mac_vap)
{
    /* p2p有可能进行监听动作，但是和scan req的优先级一样，因此当上层发起的是扫描请求时，统一可使用下面的接口判断 */
    return hmac_p2p_check_can_enter_state(pst_mac_vap, HMAC_FSM_INPUT_SCAN_REQ);
}


OAL_STATIC oal_uint32 hmac_scan_check_is_dispatch_scan_req(hmac_vap_stru *pst_hmac_vap,
    hmac_device_stru *pst_hmac_device)
{
    oal_uint32 ul_ret;

    /* 1.先检测其它vap的状态从而判断是否可进入扫描状态，使得扫描尽量不打断其它的入网流程 */
    ul_ret = hmac_scan_check_can_enter_scan_state(&(pst_hmac_vap->st_vap_base_info));
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_check_is_dispatch_scan_req::Because of err_code[%d], can't enter into scan state.}", ul_ret);
        return ul_ret;
    }

    /* 2.判断当前扫描是否正在执行 */
    if (pst_hmac_device->st_scan_mgmt.en_is_scanning == OAL_TRUE) {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_check_is_dispatch_scan_req::the scan request is rejected.}");
        return OAL_FAIL;
    }

#ifdef _PRE_WLAN_FEATURE_ROAM
    /* 3.判断当前是否正在执行漫游 */
    if (pst_hmac_vap->st_vap_base_info.en_vap_state == MAC_VAP_STATE_ROAMING) {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_check_is_dispatch_scan_req:: roam reject new scan.}");
        return OAL_FAIL;
    }
#endif // _PRE_WLAN_FEATURE_ROAM

    return OAL_SUCC;
}


OAL_STATIC oal_void hmac_scan_proc_last_scan_record(hmac_vap_stru *pst_hmac_vap, hmac_device_stru *pst_hmac_device)
{
    /* 如果是proxysta发起的扫描无需清楚扫描结果 */
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    if (pst_hmac_device->pst_device_base_info->st_cap_flag.bit_proxysta == OAL_TRUE) {
        if ((pst_hmac_vap->st_vap_base_info.st_vap_proxysta.en_is_proxysta == OAL_TRUE) &&
            (pst_hmac_vap->st_vap_base_info.st_vap_proxysta.en_is_main_proxysta == OAL_FALSE)) {
            /* proxysta do nothing */
            OAM_INFO_LOG0(0, OAM_SF_SCAN, "{hmac_scan_proc_scan_req_event:: proxysta don't need clean scan record.}");
            return;
        }
    }
#endif

    OAM_INFO_LOG0(0, OAM_SF_SCAN, "{hmac_scan_proc_scan_req_event:: start clean last scan record.}");

#if 0
    /* 清空上一次扫描记录信息 */
    hmac_scan_clean_scan_record(&(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt));
#else
    /* 本次扫描请求发起时，清除上一次扫描结果中过期的bss信息 */
    hmac_scan_clean_expire_scanned_bss(&(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt));
#endif

    return;
}



OAL_STATIC oal_uint32 hmac_scan_proc_scan_timeout_fn(void *p_arg)
{
    hmac_device_stru      *pst_hmac_device = (hmac_device_stru *)p_arg;
    hmac_vap_stru         *pst_hmac_vap    = OAL_PTR_NULL;
    hmac_scan_record_stru *pst_scan_record = OAL_PTR_NULL;
    oal_uint32            ul_pedding_data = 0;

    /* 获取扫描记录信息 */
    pst_scan_record = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt);

    /* 获取hmac vap */
    pst_hmac_vap = mac_res_get_hmac_vap(pst_scan_record->uc_vap_id);
    if (OAL_UNLIKELY(pst_hmac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_scan_record->uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_scan_timeout_fn::pst_hmac_vap null.}");

        /* 扫描状态恢复为未在执行的状态 */
        pst_hmac_device->st_scan_mgmt.en_is_scanning = OAL_FALSE;
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 根据当前扫描的类型和当前vap的状态，决定切换vap的状态，如果是前景扫描，才需要切换vap的状态 */
    if (pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        if (pst_hmac_vap->st_vap_base_info.en_vap_state == MAC_VAP_STATE_STA_WAIT_SCAN) {
            /* 改变vap状态到SCAN_COMP */
            hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_STA_SCAN_COMP);
        } else if (pst_hmac_vap->st_vap_base_info.en_vap_state == MAC_VAP_STATE_UP) {
            /* 背景扫描时需要进行帧过滤的配置 */
            hmac_set_rx_filter_value(&(pst_hmac_vap->st_vap_base_info));
        }
    }

    if ((pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_AP) &&
        (pst_scan_record->en_vap_last_state != MAC_VAP_STATE_BUTT)) {
        hmac_fsm_change_state(pst_hmac_vap, pst_scan_record->en_vap_last_state);
        pst_scan_record->en_vap_last_state = MAC_VAP_STATE_BUTT;
    }
    /* 设置扫描响应状态为超时 */
    pst_scan_record->en_scan_rsp_status = MAC_SCAN_TIMEOUT;
    OAM_WARNING_LOG1(pst_scan_record->uc_vap_id, OAM_SF_SCAN,
        "{hmac_scan_proc_scan_timeout_fn::scan time out cookie [%x].}", pst_scan_record->ull_cookie);

    /* 如果扫描回调函数不为空，则调用回调函数 */
    if (pst_scan_record->p_fn_cb != OAL_PTR_NULL) {
        OAM_WARNING_LOG0(pst_scan_record->uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_proc_scan_timeout_fn::scan callback func proc.}");
        pst_scan_record->p_fn_cb(pst_scan_record);
    }

    /* DMAC 超时未上报扫描完成，HMAC 下发扫描结束命令，停止DMAC 扫描 */
    hmac_config_scan_abort(&pst_hmac_vap->st_vap_base_info, OAL_SIZEOF(oal_uint32), (oal_uint8 *)&ul_pedding_data);

    /* 扫描状态恢复为未在执行的状态 */
    pst_hmac_device->st_scan_mgmt.en_is_scanning = OAL_FALSE;

    chr_exception(chr_wifi_drv(CHR_WIFI_DRV_EVENT_SCAN, CHR_WIFI_DRV_ERROR_SCAN_TIMEOUT));
    return OAL_SUCC;
}


oal_uint32 hmac_scan_proc_scan_req_event(hmac_vap_stru *pst_hmac_vap, oal_void *p_params)
{
    frw_event_mem_stru    *pst_event_mem           = OAL_PTR_NULL;
    frw_event_stru        *pst_event               = OAL_PTR_NULL;
    mac_scan_req_stru     *pst_h2d_scan_req_params = OAL_PTR_NULL;     /* hmac发送到dmac的扫描请求参数 */
    mac_scan_req_stru     *pst_scan_params         = OAL_PTR_NULL;
    hmac_device_stru      *pst_hmac_device         = OAL_PTR_NULL;
    hmac_scan_record_stru *pst_scan_record         = OAL_PTR_NULL;
    oal_uint32            ul_scan_timeout;
    oal_uint32            ul_ret;

    /* 参数合法性检查 */
    if (OAL_UNLIKELY(OAL_ANY_NULL_PTR2(pst_hmac_vap, p_params))) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_proc_scan_req_event::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 扫描停止模块测试 */
    if ((g_en_bgscan_enable_flag == 0) && (pst_hmac_vap->st_vap_base_info.en_vap_state == MAC_VAP_STATE_UP)) {
        OAM_WARNING_LOG1(0, OAM_SF_SCAN, "hmac_scan_proc_scan_req_event: g_en_bgscan_enable_flag= %d.",
            g_en_bgscan_enable_flag);
        return OAL_FAIL;
    }

    pst_scan_params = (mac_scan_req_stru *)p_params;

    /* 异常判断: 扫描的信道个数为0 */
    if (pst_scan_params->uc_channel_nums == 0) {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_proc_scan_req_event::channel_nums=0.}");
        return OAL_FAIL;
    }

    /* 获取hmac device */
    pst_hmac_device = hmac_res_get_mac_dev(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_proc_scan_req_event::pst_hmac_device[%d] null.}", pst_hmac_vap->st_vap_base_info.uc_device_id);
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }

    /* 更新此次扫描请求的扫描参数 */
    if (pst_scan_params->uc_scan_func == MAC_SCAN_FUNC_P2P_LISTEN) {
        ul_ret = hmac_scan_update_scan_params(pst_hmac_vap, pst_scan_params, OAL_FALSE);
    } else {
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
        ul_ret = hmac_scan_update_scan_params(pst_hmac_vap, pst_scan_params,
            g_st_wlan_customize.uc_random_mac_addr_scan);
#else
        ul_ret = hmac_scan_update_scan_params(pst_hmac_vap, pst_scan_params,
            pst_hmac_device->st_scan_mgmt.en_is_random_mac_addr_scan);
#endif
    }
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_proc_scan_req_event::update scan mode failed, error[%d].}", ul_ret);
        return ul_ret;
    }

    /* 检测是否符合发起扫描请求的条件，如果不符合，直接返回 */
    ul_ret = hmac_scan_check_is_dispatch_scan_req(pst_hmac_vap, pst_hmac_device);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_proc_scan_req_event::Because of error[%d], can't dispatch scan req.}", ul_ret);
        return ul_ret;
    }

    /* 设置扫描模块处于扫描状态，其它扫描请求将丢弃 */
    pst_hmac_device->st_scan_mgmt.en_is_scanning = OAL_TRUE;

    /* 处理上一次扫描记录，目前直接清楚上一次结果，后续可能需要老化时间处理 */
    hmac_scan_proc_last_scan_record(pst_hmac_vap, pst_hmac_device);

    /* 记录扫描发起者的信息，某些模块回调函数使用 */
    pst_scan_record = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt);
    pst_scan_record->uc_chip_id      = pst_hmac_device->pst_device_base_info->uc_chip_id;
    pst_scan_record->uc_device_id    = pst_hmac_device->pst_device_base_info->uc_device_id;
    pst_scan_record->uc_vap_id       = pst_scan_params->uc_vap_id;
    pst_scan_record->uc_chan_numbers = pst_scan_params->uc_channel_nums;
    pst_scan_record->p_fn_cb         = pst_scan_params->p_fn_cb;

    if (pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_proc_scan_req_event::save last en_vap_state:%d}", pst_hmac_vap->st_vap_base_info.en_vap_state);

        pst_scan_record->en_vap_last_state = pst_hmac_vap->st_vap_base_info.en_vap_state;
    }

    pst_scan_record->ull_cookie = pst_scan_params->ull_cookie;

    /* 记录扫描开始时间 */
    pst_scan_record->st_scan_start_time = oal_ktime_get();

    /* 抛扫描请求事件到DMAC, 申请事件内存 */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(mac_scan_req_stru));
    if (pst_event_mem == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_proc_scan_req_event::pst_event_mem null.}");

        /* 恢复扫描状态为非运行状态 */
        pst_hmac_device->st_scan_mgmt.en_is_scanning = OAL_FALSE;
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 如果发起扫描的vap的模式为sta，并且，其关联状态为非up状态，且非p2p监听状态，则切换其扫描状态 */
    if ((pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_STA) &&
            (pst_scan_params->uc_scan_func != MAC_SCAN_FUNC_P2P_LISTEN)) {
        if (pst_hmac_vap->st_vap_base_info.en_vap_state != MAC_VAP_STATE_UP) {
            /* 切换vap的状态为WAIT_SCAN状态 */
            hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_STA_WAIT_SCAN);
        } else {
            /* 背景扫描时需要进行帧过滤的配置 */
            pst_hmac_vap->st_vap_base_info.en_vap_state = MAC_VAP_STATE_STA_WAIT_SCAN;
            hmac_set_rx_filter_value(&(pst_hmac_vap->st_vap_base_info));
            pst_hmac_vap->st_vap_base_info.en_vap_state = MAC_VAP_STATE_UP;
        }
    }

    /* AP的启动扫描做特殊处理，当hostapd下发扫描请求时，VAP还处于INIT状态 */
    if ((pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_AP) &&
            (pst_hmac_vap->st_vap_base_info.en_vap_state == MAC_VAP_STATE_INIT)) {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_proc_scan_req_event::ap startup scan.}");
        hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_AP_WAIT_START);
    }

    /* 填写事件 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_SCAN_REQ,
                       OAL_SIZEOF(mac_scan_req_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_hmac_vap->st_vap_base_info.uc_chip_id,
                       pst_hmac_vap->st_vap_base_info.uc_device_id,
                       pst_hmac_vap->st_vap_base_info.uc_vap_id);

    /* 直接传内容，屏蔽多产品的差异 */
    pst_h2d_scan_req_params = (mac_scan_req_stru *)(pst_event->auc_event_data);

    /* 拷贝扫描请求参数到事件data区域 */
    if (memcpy_s(pst_h2d_scan_req_params, OAL_SIZEOF(mac_scan_req_stru), pst_scan_params,
        OAL_SIZEOF(mac_scan_req_stru)) != EOK) {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
            "hmac_scan_proc_scan_req_event::memcpy_s failed !");
        FRW_EVENT_FREE(pst_event_mem);
        return OAL_FAIL;
    }

    /* 打印扫描参数，测试使用 */
    /* 如果是P2P 发起监听，则设置HMAC 扫描超时时间为P2P 监听时间 */
    if (pst_scan_params->uc_scan_func == MAC_SCAN_FUNC_P2P_LISTEN) {
        ul_scan_timeout = pst_scan_params->us_scan_time * 2;
    } else {
        ul_scan_timeout = WLAN_DEFAULT_MAX_TIME_PER_SCAN;
    }
    hmac_scan_print_scan_params(pst_h2d_scan_req_params, &pst_hmac_vap->st_vap_base_info);

    /* 启动扫描保护定时器，防止因抛事件、核间通信失败等情况下的异常保护，定时器初步的超时时间为4.5秒 */
    FRW_TIMER_CREATE_TIMER(&(pst_hmac_device->st_scan_mgmt.st_scan_timeout),
                           hmac_scan_proc_scan_timeout_fn,
                           ul_scan_timeout,
                           pst_hmac_device,
                           OAL_FALSE,
                           OAM_MODULE_ID_HMAC,
                           pst_hmac_device->pst_device_base_info->ul_core_id);

    /* 如果是p2p listen 记录下listen的信道 */
    if (pst_scan_params->uc_scan_func == MAC_SCAN_FUNC_P2P_LISTEN) {
        pst_hmac_device->st_scan_mgmt.st_p2p_listen_channel = pst_scan_params->ast_channel_list[0];
    }

    /* 分发事件 */
    frw_event_dispatch_event(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}


oal_uint32  hmac_scan_proc_sched_scan_req_event(hmac_vap_stru *pst_hmac_vap, oal_void *p_params)
{
    frw_event_mem_stru    *pst_event_mem       = OAL_PTR_NULL;
    frw_event_stru        *pst_event           = OAL_PTR_NULL;
    hmac_device_stru      *pst_hmac_device     = OAL_PTR_NULL;
    hmac_scan_record_stru *pst_scan_record     = OAL_PTR_NULL;
    mac_pno_scan_stru     *pst_pno_scan_params = OAL_PTR_NULL;
    oal_uint32            ul_ret;

    /* 参数合法性检查 */
    if (OAL_UNLIKELY((pst_hmac_vap == OAL_PTR_NULL) || (p_params == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_proc_sched_scan_req_event::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_pno_scan_params = (mac_pno_scan_stru *)p_params;

    /* 判断PNO调度扫描下发的过滤的ssid个数小于等于0 */
    if (pst_pno_scan_params->l_ssid_count <= 0) {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_proc_sched_scan_req_event::ssid_count <=0.}");
        return OAL_FAIL;
    }

    /* 获取hmac device */
    pst_hmac_device = hmac_res_get_mac_dev(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_proc_sched_scan_req_event::pst_hmac_device[%d] null.}",
            pst_hmac_vap->st_vap_base_info.uc_device_id);
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }

    /* 检测是否符合发起扫描请求的条件，如果不符合，直接返回 */
    ul_ret = hmac_scan_check_is_dispatch_scan_req(pst_hmac_vap, pst_hmac_device);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_proc_sched_scan_req_event::Because of error[%d], can't dispatch scan req.}", ul_ret);
        return ul_ret;
    }

    /* 清空上一次的扫描结果 */
    hmac_scan_proc_last_scan_record(pst_hmac_vap, pst_hmac_device);

    /* 记录扫描发起者的信息，某些模块回调函数使用 */
    pst_scan_record = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt);
    pst_scan_record->uc_chip_id   = pst_hmac_device->pst_device_base_info->uc_chip_id;
    pst_scan_record->uc_device_id = pst_hmac_device->pst_device_base_info->uc_device_id;
    pst_scan_record->uc_vap_id    = pst_hmac_vap->st_vap_base_info.uc_vap_id;
    pst_scan_record->p_fn_cb      = pst_pno_scan_params->p_fn_cb;

    /* 抛扫描请求事件到DMAC, 申请事件内存 */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(mac_pno_scan_stru *));
    if (pst_event_mem == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_proc_sched_scan_req_event::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 填写事件 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_SCHED_SCAN_REQ,
                       OAL_SIZEOF(mac_pno_scan_stru *),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_hmac_vap->st_vap_base_info.uc_chip_id,
                       pst_hmac_vap->st_vap_base_info.uc_device_id,
                       pst_hmac_vap->st_vap_base_info.uc_vap_id);

    /* 事件data域内携带PNO扫描请求参数 */
    if (memcpy_s(frw_get_event_payload(pst_event_mem), OAL_SIZEOF(mac_pno_scan_stru *),
        (oal_uint8 *)&pst_pno_scan_params, OAL_SIZEOF(mac_pno_scan_stru *)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "hmac_scan_proc_sched_scan_req_event_etc::memcpy fail!");
        FRW_EVENT_FREE(pst_event_mem);
        return OAL_FAIL;
    }
    /* 分发事件 */
    frw_event_dispatch_event(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}


oal_uint32 hmac_scan_process_chan_result_event(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru            *pst_event              = OAL_PTR_NULL;
    frw_event_hdr_stru        *pst_event_hdr          = OAL_PTR_NULL;
    hmac_device_stru          *pst_hmac_device        = OAL_PTR_NULL;
    dmac_crx_chan_result_stru *pst_chan_result_param  = OAL_PTR_NULL;
    hmac_scan_record_stru     *pst_scan_record        = OAL_PTR_NULL;
    oal_uint8                 uc_scan_idx;

    /* 获取事件信息 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    pst_event_hdr = &(pst_event->st_event_hdr);
    pst_chan_result_param = (dmac_crx_chan_result_stru *)(pst_event->auc_event_data);
    uc_scan_idx = pst_chan_result_param->uc_scan_idx;

    /* 获取hmac device */
    pst_hmac_device = hmac_res_get_mac_dev(pst_event_hdr->uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_process_chan_result_event::pst_hmac_device is null.}");
        return OAL_FAIL;
    }

    pst_scan_record = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt);

    /* 检查上报的索引是否合法 */
    if (uc_scan_idx >= pst_scan_record->uc_chan_numbers) {
        /* dmac上报的扫描结果超出了要扫描的信道个数 */
        OAM_WARNING_LOG2(0, OAM_SF_SCAN,
            "{hmac_scan_process_chan_result_event::result from dmac error! scan_idx[%d], chan_numbers[%d].}",
            uc_scan_idx, pst_scan_record->uc_chan_numbers);

        return OAL_FAIL;
    }

    pst_scan_record->ast_chan_results[uc_scan_idx] = pst_chan_result_param->st_chan_result;

    return OAL_SUCC;
}


oal_void hmac_scan_init(hmac_device_stru *pst_hmac_device)
{
    hmac_scan_stru     *pst_scan_mgmt = OAL_PTR_NULL;
    hmac_bss_mgmt_stru *pst_bss_mgmt  = OAL_PTR_NULL;

    /* 初始化扫描管理结构体信息 */
    pst_scan_mgmt = &(pst_hmac_device->st_scan_mgmt);
    memset_s(pst_scan_mgmt, OAL_SIZEOF(hmac_scan_stru), 0, OAL_SIZEOF(hmac_scan_stru));
    pst_scan_mgmt->en_is_scanning = OAL_FALSE;
    pst_scan_mgmt->st_scan_record_mgmt.en_vap_last_state = MAC_VAP_STATE_BUTT;

    /* 初始化bss管理结果链表和锁 */
    pst_bss_mgmt = &(pst_scan_mgmt->st_scan_record_mgmt.st_bss_mgmt);
    oal_dlist_init_head(&(pst_bss_mgmt->st_bss_list_head));
    oal_spin_lock_init(&(pst_bss_mgmt->st_lock));

    /* 初始化内核下发扫描request资源锁 */
    oal_spin_lock_init(&(pst_scan_mgmt->st_scan_request_spinlock));

    /* 初始化 st_wiphy_mgmt 结构 */
    OAL_WAIT_QUEUE_INIT_HEAD(&(pst_scan_mgmt->st_wait_queue));

#if  defined(_PRE_WLAN_CHIP_TEST_ALG) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && defined(_PRE_DEBUG_MODE)
    hmac_scan_ct_init();
#endif

    return;
}


oal_void hmac_scan_exit(hmac_device_stru *pst_hmac_device)
{
    hmac_scan_stru *pst_scan_mgmt = OAL_PTR_NULL;

    /* 清空扫描记录信息 */
    hmac_scan_clean_scan_record(&(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt));

    /* 清除扫描管理结构体信息 */
    pst_scan_mgmt = &(pst_hmac_device->st_scan_mgmt);
    memset_s(pst_scan_mgmt, OAL_SIZEOF(hmac_scan_stru), 0, OAL_SIZEOF(hmac_scan_stru));
    pst_scan_mgmt->en_is_scanning = OAL_FALSE;

#if  defined(_PRE_WLAN_CHIP_TEST_ALG) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && defined(_PRE_DEBUG_MODE)
    hmac_scan_ct_exit();
#endif

    return;
}


#ifdef _PRE_DEBUG_MODE

oal_void hmac_scan_test_cb(void *p_scan_record)
{
    hmac_scan_record_stru *scan_record = OAL_PTR_NULL;
    hmac_bss_mgmt_stru *pst_bss_mgmt = OAL_PTR_NULL;
    mac_bss_dscr_stru *pst_bss_dscr = OAL_PTR_NULL;
    hmac_scanned_bss_info *pst_scanned_bss = OAL_PTR_NULL;
    oal_dlist_head_stru *pst_entry = OAL_PTR_NULL;
    oal_time_us_stru st_curr_time;
    oal_uint8 idx;

    if (p_scan_record == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "p_scan_record is null ptr.");
        return;
    }

    scan_record = (hmac_scan_record_stru *)p_scan_record;

    /* 打印信道测量结果 */
    OAL_IO_PRINT("The chan measure result: \n");
    for (idx = 0; idx < scan_record->uc_chan_numbers; idx++) {
        OAL_IO_PRINT("[channel_result]Chan num      : %d\n", scan_record->ast_chan_results[idx].uc_channel_number);
        OAL_IO_PRINT("[channel_result]Stats cnt     : %d\n", scan_record->ast_chan_results[idx].uc_stats_cnt);
        OAL_IO_PRINT("[channel_result]Stats valid   : %d\n", scan_record->ast_chan_results[idx].uc_stats_valid);
        OAL_IO_PRINT("[channel_result]Stats time us : %d\n", scan_record->ast_chan_results[idx].ul_total_stats_time_us);
        OAL_IO_PRINT("[channel_result]Free time 20M : %d\n",
            scan_record->ast_chan_results[idx].ul_total_free_time_20M_us);
        OAL_IO_PRINT("[channel_result]Free time 40M : %d\n",
            scan_record->ast_chan_results[idx].ul_total_free_time_40M_us);
        OAL_IO_PRINT("[channel_result]Free time 80M : %d\n",
            scan_record->ast_chan_results[idx].ul_total_free_time_80M_us);
        OAL_IO_PRINT("[channel_result]Send time     : %d\n", scan_record->ast_chan_results[idx].ul_total_send_time_us);
        OAL_IO_PRINT("[channel_result]Recv time     : %d\n", scan_record->ast_chan_results[idx].ul_total_recv_time_us);
        OAL_IO_PRINT("[channel_result]Free power cnt: %d\n", scan_record->ast_chan_results[idx].uc_free_power_cnt);
        OAL_IO_PRINT("[channel_result]Free power 20M: %d\n", scan_record->ast_chan_results[idx].s_free_power_stats_20M);
        OAL_IO_PRINT("[channel_result]Free power 40M: %d\n", scan_record->ast_chan_results[idx].s_free_power_stats_40M);
        OAL_IO_PRINT("[channel_result]Free power 80M: %d\n", scan_record->ast_chan_results[idx].s_free_power_stats_80M);
        OAL_IO_PRINT("[channel_result]Radar detected: %d\n", scan_record->ast_chan_results[idx].uc_radar_detected);
        OAL_IO_PRINT("[channel_result]Radar bw      : %d\n\n", scan_record->ast_chan_results[idx].uc_radar_bw);
    }

    /* 打印BSS结果 */
    OAL_IO_PRINT("The bss result: \n");

    /* 获取扫描结果的管理结构地址 */
    pst_bss_mgmt = &(scan_record->st_bss_mgmt);

    /* 获取锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    /* 遍历扫描到的bss信息 */
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_bss_mgmt->st_bss_list_head))
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);
        pst_bss_dscr = &(pst_scanned_bss->st_bss_dscr_info);
        if ((pst_bss_dscr == OAL_PTR_NULL) || (pst_bss_dscr->auc_mgmt_buff == OAL_PTR_NULL)) {
            continue;
        }

        OAL_IO_PRINT("Chan num      : %d\n", pst_bss_dscr->st_channel.uc_chan_number);
        OAL_IO_PRINT("BSSID         : 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n\n", pst_bss_dscr->auc_bssid[0],
            pst_bss_dscr->auc_bssid[1], pst_bss_dscr->auc_bssid[2], pst_bss_dscr->auc_bssid[3],
            pst_bss_dscr->auc_bssid[4], pst_bss_dscr->auc_bssid[5]);
    }

    /* 释放锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    /* 打印此次扫描耗时 */
    oal_time_get_stamp_us(&st_curr_time);
    OAL_IO_PRINT("Scan start time: %d %d\n", (oal_uint32)scan_record->st_scan_start_timestamp.i_sec,
        (oal_uint32)scan_record->st_scan_start_timestamp.i_usec);
    OAL_IO_PRINT("Scan end   time: %d %d\n\n", (oal_uint32)st_curr_time.i_sec, (oal_uint32)st_curr_time.i_usec);

    return;
}


oal_uint32 hmac_scan_test(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_scan_req_stru st_scan_req;
    oal_uint8 uc_chan_idx;
    oal_uint32 ul_ret;
    oal_uint8 uc_2g_chan_num = 0;
    oal_uint8 uc_chan_number = 0;
    oal_int8 ac_param[15] = {0};
    oal_bool_enum_uint8 en_2g_all = OAL_FALSE;
    oal_bool_enum_uint8 en_5g_all = OAL_FALSE;
    mac_ioctl_scan_test_config_stru *scan_test = OAL_PTR_NULL;
    hmac_vap_stru *pst_hmac_vap = OAL_PTR_NULL;

    scan_test = (mac_ioctl_scan_test_config_stru *)puc_param;

    if (memcpy_s(ac_param, sizeof(ac_param), scan_test->ac_scan_type,
        sizeof(scan_test->ac_scan_type)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_scan_test::memcpy_s failed!}");
        return OAL_FAIL;
    }
    memset_s(&st_scan_req, OAL_SIZEOF(st_scan_req), 0, OAL_SIZEOF(st_scan_req));

    if (oal_strcmp("2g", ac_param) == 0) {
        en_2g_all = OAL_TRUE;
    }
#ifdef _PRE_WLAN_FEATURE_5G
    else if (oal_strcmp("5g", ac_param) == 0) {
        en_5g_all = OAL_TRUE;
    } else if (oal_strcmp("all", ac_param) == 0) {
        en_2g_all = OAL_TRUE;
        en_5g_all = OAL_TRUE;
    }
#endif /* _PRE_WLAN_FEATURE_5G */
    else {
        /* 指定单个信道 */
        uc_chan_number = (oal_uint8)oal_atoi(ac_param);
    }

    if (en_2g_all) {
        /* 2G全信道 */
        for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_2_BUTT; uc_chan_idx++) {
            ul_ret = mac_is_channel_idx_valid(WLAN_BAND_2G, uc_chan_idx);
            if (ul_ret == OAL_SUCC) {
                mac_get_channel_num_from_idx(WLAN_BAND_2G, uc_chan_idx, &uc_chan_number);

                st_scan_req.ast_channel_list[uc_2g_chan_num].uc_chan_number = uc_chan_number;
                st_scan_req.ast_channel_list[uc_2g_chan_num].en_band        = WLAN_BAND_2G;
                st_scan_req.ast_channel_list[uc_2g_chan_num].uc_idx         = uc_chan_idx;
                st_scan_req.ast_channel_list[uc_2g_chan_num].en_bandwidth   = scan_test->en_bandwidth;
                st_scan_req.uc_channel_nums++;
                uc_2g_chan_num++;
            }
        }
    }
#ifdef _PRE_WLAN_FEATURE_5G
    if (en_5g_all) {
        oal_uint8 uc_5g_chan_num = 0;
        /* 5G全信道 */
        for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_5_BUTT; uc_chan_idx++) {
            ul_ret = mac_is_channel_idx_valid(WLAN_BAND_5G, uc_chan_idx);
            if (ul_ret == OAL_SUCC) {
                mac_get_channel_num_from_idx(WLAN_BAND_5G, uc_chan_idx, &uc_chan_number);

                st_scan_req.ast_channel_list[uc_2g_chan_num + uc_5g_chan_num].uc_chan_number = uc_chan_number;
                st_scan_req.ast_channel_list[uc_2g_chan_num + uc_5g_chan_num].en_band = WLAN_BAND_5G;
                st_scan_req.ast_channel_list[uc_2g_chan_num + uc_5g_chan_num].uc_idx = uc_chan_idx;
                st_scan_req.ast_channel_list[uc_2g_chan_num + uc_5g_chan_num].en_bandwidth = scan_test->en_bandwidth;
                st_scan_req.uc_channel_nums++;
                uc_5g_chan_num++;
            }
        }
    }
#endif /* _PRE_WLAN_FEATURE_5G */

    if (OAL_VALUE_EQ_ALL2(OAL_FALSE, en_2g_all, en_5g_all)) {
        if (uc_chan_number < 15) {
            ul_ret = mac_is_channel_num_valid(WLAN_BAND_2G, uc_chan_number);
            if (ul_ret != OAL_SUCC) {
                OAL_IO_PRINT("Invalid chan num: %d. return!\n", uc_chan_number);
                return OAL_FAIL;
            }
            st_scan_req.ast_channel_list[0].en_band = WLAN_BAND_2G;
        }
#ifdef _PRE_WLAN_FEATURE_5G
        else {
            ul_ret = mac_is_channel_num_valid(WLAN_BAND_5G, uc_chan_number);
            if (ul_ret != OAL_SUCC) {
                OAL_IO_PRINT("Invalid chan num: %d. return!\n", uc_chan_number);
                return OAL_FAIL;
            }
            st_scan_req.ast_channel_list[0].en_band = WLAN_BAND_5G;
        }

        st_scan_req.ast_channel_list[0].uc_chan_number = uc_chan_number;
        st_scan_req.ast_channel_list[0].en_bandwidth   = scan_test->en_bandwidth;

        mac_get_channel_idx_from_num(st_scan_req.ast_channel_list[0].en_band,
                                     st_scan_req.ast_channel_list[0].uc_chan_number,
                                     &(st_scan_req.ast_channel_list[0].uc_idx));

        st_scan_req.uc_channel_nums = 1;
#else
        {
            OAL_IO_PRINT("Invalid chan num: %d. return!\n", uc_chan_number);
            return OAL_FAIL;
        }
#endif /* _PRE_WLAN_FEATURE_5G */
    }

    st_scan_req.en_bss_type  = WLAN_MIB_DESIRED_BSSTYPE_INFRA;
    st_scan_req.en_scan_type = WLAN_SCAN_TYPE_ACTIVE;
    st_scan_req.uc_bssid_num = 0;
    st_scan_req.uc_ssid_num  = 0;

    oal_set_mac_addr(st_scan_req.auc_bssid[0], BROADCAST_MACADDR);
    st_scan_req.uc_bssid_num = 1;

    st_scan_req.us_scan_time = WLAN_DEFAULT_ACTIVE_SCAN_TIME;
    st_scan_req.en_scan_mode = WLAN_SCAN_MODE_FOREGROUND;
    st_scan_req.uc_scan_func = MAC_SCAN_FUNC_ALL;

    st_scan_req.p_fn_cb = hmac_scan_test_cb;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    hmac_scan_proc_scan_req_event(pst_hmac_vap, &st_scan_req);

    return OAL_SUCC;
}
#endif


oal_uint32 hmac_bgscan_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, const oal_uint8 *puc_param)
{
    oal_bool_enum_uint8 *pen_bgscan_enable_flag;

    pen_bgscan_enable_flag = (oal_bool_enum_uint8 *)puc_param; /* 背景扫描停止使能位 */

    /* 背景扫描停止命令 */
    if (*pen_bgscan_enable_flag) {
        g_en_bgscan_enable_flag = OAL_TRUE;
    } else {
        g_en_bgscan_enable_flag = OAL_FALSE;
    }

    OAM_WARNING_LOG1(0, OAM_SF_SCAN, "hmac_bgscan_enable: g_en_bgscan_enable_flag= %d.", g_en_bgscan_enable_flag);

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_DBAC

oal_uint32 hmac_scan_start_dbac(mac_device_stru *pst_dev)
{
    oal_uint8  auc_cmd[32];
    oal_uint16 us_len;
    oal_uint32 ul_ret = OAL_FAIL;
    oal_uint8  uc_idx;
#define DBAC_START_STR     " dbac start"
#define DBAC_START_STR_LEN 11
    mac_vap_stru  *pst_mac_vap = OAL_PTR_NULL;

    mac_ioctl_alg_config_stru  *pst_alg_config = (mac_ioctl_alg_config_stru *)auc_cmd;

    if (memcpy_s(auc_cmd + OAL_SIZEOF(mac_ioctl_alg_config_stru),
        sizeof(auc_cmd) - OAL_SIZEOF(mac_ioctl_alg_config_stru), (const int8_t *)DBAC_START_STR, 11) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_scan_start_dbac::memcpy_s failed!}");
        return OAL_FAIL;
    }
    auc_cmd[OAL_SIZEOF(mac_ioctl_alg_config_stru) + DBAC_START_STR_LEN ] = 0;

    pst_alg_config->uc_argc = 2;
    pst_alg_config->auc_argv_offset[0] = 1;
    pst_alg_config->auc_argv_offset[1] = 6;

    for (uc_idx = 0; uc_idx < pst_dev->uc_vap_num; uc_idx++) {
        pst_mac_vap = mac_res_get_mac_vap(pst_dev->auc_vap_id[uc_idx]);
        if (pst_mac_vap != OAL_PTR_NULL && pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
            break;
        }
    }
    if (pst_mac_vap) {
        us_len = OAL_SIZEOF(mac_ioctl_alg_config_stru) + DBAC_START_STR_LEN + 1;
        ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_ALG, us_len, auc_cmd);
        if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                "{hmac_config_alg::hmac_config_send_event failed[%d].}", ul_ret);
        }
        OAL_IO_PRINT("start dbac\n");
    } else {
        OAL_IO_PRINT("no vap found to start dbac\n");
    }

    return ul_ret;
}
#endif

oal_uint32 hmac_start_all_bss_of_device(hmac_device_stru *pst_hmac_dev)
{
    oal_uint8       uc_idx;
    hmac_vap_stru   *pst_hmac_vap = OAL_PTR_NULL;
    mac_device_stru *pst_dev = pst_hmac_dev->pst_device_base_info;

    OAM_WARNING_LOG1(0, OAM_SF_ACS, "{hmac_start_all_bss_of_device:device id=%d}",
        pst_hmac_dev->pst_device_base_info->uc_device_id);
    if (OAL_UNLIKELY(pst_dev == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_start_all_bss_of_device::pst_device_base_info null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    for (uc_idx = 0; uc_idx < pst_dev->uc_vap_num; uc_idx++) {
        pst_hmac_vap = mac_res_get_hmac_vap(pst_dev->auc_vap_id[uc_idx]);
        if (pst_hmac_vap == NULL) {
            OAM_WARNING_LOG2(0, OAM_SF_ACS, "hmac_start_all_bss_of_device:null ap, idx=%d id=%d",
                uc_idx, pst_dev->auc_vap_id[uc_idx]);
            continue;
        }

        if (pst_hmac_vap != OAL_PTR_NULL && (pst_hmac_vap->st_vap_base_info.en_vap_state == MAC_VAP_STATE_AP_WAIT_START
#ifdef _PRE_WLAN_FEATURE_DBAC
                 || (mac_is_dbac_enabled(pst_dev) && pst_hmac_vap->st_vap_base_info.en_vap_state == MAC_VAP_STATE_PAUSE)
#endif
                 || pst_hmac_vap->st_vap_base_info.en_vap_state == MAC_VAP_STATE_UP)) {
            if (hmac_chan_start_bss(pst_hmac_vap) == OAL_SUCC) {
                OAM_WARNING_LOG4(0, OAM_SF_ACS, "start vap %d on ch=%d band=%d bw=%d\n", pst_dev->auc_vap_id[uc_idx],
                    pst_hmac_vap->st_vap_base_info.st_channel.uc_chan_number,
                    pst_hmac_vap->st_vap_base_info.st_channel.en_band,
                    pst_hmac_vap->st_vap_base_info.st_channel.en_bandwidth);
            }
        } else {
            OAM_WARNING_LOG0(0, OAM_SF_ACS, "start vap error\n");
            continue;
        }
    }

#ifdef _PRE_WLAN_FEATURE_DBAC
    if (mac_is_dbac_enabled(pst_dev)) {
        return hmac_scan_start_dbac(pst_dev);
    }
#endif

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST

OAL_STATIC oal_uint32 hmac_get_pri_sec_chan(mac_bss_dscr_stru *pst_bss_dscr, oal_uint32 *pul_pri_chan,
    oal_uint32 *pul_sec_chan)
{
    *pul_pri_chan = *pul_sec_chan = 0;

    *pul_pri_chan = pst_bss_dscr->st_channel.uc_chan_number;

    OAM_INFO_LOG1(0, OAM_SF_2040, "hmac_get_pri_sec_chan:pst_bss_dscr->st_channel.en_bandwidth = %d\n",
        pst_bss_dscr->en_channel_bandwidth);

    if (pst_bss_dscr->en_channel_bandwidth == WLAN_BAND_WIDTH_40PLUS) {
        *pul_sec_chan = *pul_pri_chan + 4;
    } else if (pst_bss_dscr->en_channel_bandwidth == WLAN_BAND_WIDTH_40MINUS) {
        *pul_sec_chan = *pul_pri_chan - 4;
    } else {
        OAM_WARNING_LOG1(0, OAM_SF_2040, "hmac_get_pri_sec_chan: pst_bss_dscr is not support 40Mhz, \
            *pul_sec_chan = %d\n", *pul_sec_chan);
    }

    OAM_INFO_LOG2(0, OAM_SF_2040, "*pul_pri_chan = %d, *pul_sec_chan = %d\n\n", *pul_pri_chan, *pul_sec_chan);

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_5G

OAL_STATIC oal_void hmac_switch_pri_sec(mac_vap_stru *pst_mac_vap)
{
    if (pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40PLUS) {
        pst_mac_vap->st_channel.uc_chan_number = pst_mac_vap->st_channel.uc_chan_number + 4;
        pst_mac_vap->st_channel.en_bandwidth = WLAN_BAND_WIDTH_40MINUS;
    } else if (pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40MINUS) {
        pst_mac_vap->st_channel.uc_chan_number = pst_mac_vap->st_channel.uc_chan_number - 4;
        pst_mac_vap->st_channel.en_bandwidth = WLAN_BAND_WIDTH_40PLUS;
    } else {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040,
            "hmac_switch_pri_sec:en_bandwidth = %d\n not need obss scan\n", pst_mac_vap->st_channel.en_bandwidth);
    }
}
#endif /* _PRE_WLAN_FEATURE_5G */


OAL_STATIC oal_bool_enum_uint8 hmac_obss_check_40mhz_2g(mac_vap_stru *pst_mac_vap,
                                                        hmac_scan_record_stru *pst_scan_record)
{
    oal_uint32            ul_pri_freq;
    oal_uint32            ul_sec_freq;
    oal_uint32            ul_affected_start;
    oal_uint32            ul_affected_end;
    oal_uint32            ul_pri;
    oal_uint32            ul_sec;
    oal_uint32            ul_sec_chan, ul_pri_chan;
    hmac_bss_mgmt_stru    *pst_bss_mgmt = NULL;
    mac_bss_dscr_stru     *pst_bss_dscr = NULL;
    hmac_scanned_bss_info *pst_scanned_bss = NULL;
    oal_dlist_head_stru   *pst_entry = NULL;

    /* 获取主信道、次信道中心频点 */
    ul_pri_freq = (oal_int32)g_ast_freq_map_2g[pst_mac_vap->st_channel.uc_chan_number - 1].us_freq;

    if (pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40PLUS) {
        ul_sec_freq = ul_pri_freq + 20;
    } else if (pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40MINUS) {
        ul_sec_freq = ul_pri_freq - 20;
    } else {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040,
            "hmac_obss_check_40mhz_2g:en_bandwidth = %d not need obss\n", pst_mac_vap->st_channel.en_bandwidth);
        return OAL_TRUE;
    }

    /* 2.4G共存检测,检测范围是40MHz带宽中心频点为中心,左右各25MHZ */
    ul_affected_start = ((ul_pri_freq + ul_sec_freq) >> 1) - 25;
    ul_affected_end = ((ul_pri_freq + ul_sec_freq) >> 1) + 25;

    OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_2040,
        "hmac_obss_check_40mhz_2g:40 MHz affected channel range: [%d, %d] MHz", ul_affected_start, ul_affected_end);

    /* 获取扫描结果的管理结构地址 */
    pst_bss_mgmt = &(pst_scan_record->st_bss_mgmt);

    /* 获取锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    /* 遍历扫描到的bss信息 */
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_bss_mgmt->st_bss_list_head))
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);
        pst_bss_dscr    = &(pst_scanned_bss->st_bss_dscr_info);
        if (pst_bss_dscr == OAL_PTR_NULL) {
            OAM_WARNING_LOG0(0, OAM_SF_2040, "{hmac_obss_check_40mhz_2g::pst_bss_dscr is NULL}");
            continue;
        }

        ul_pri = (oal_int32)g_ast_freq_map_2g[pst_bss_dscr->st_channel.uc_chan_number - 1].us_freq;
        ul_sec = ul_pri;

        OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_2040,
            "pst_bss_dscr->st_channel.uc_chan_number = %d, ul_pri = %d\n", pst_bss_dscr->st_channel.uc_chan_number,
            ul_pri);

        /* 获取扫描到的BSS的信道、频点信息 */
        hmac_get_pri_sec_chan(pst_bss_dscr, &ul_pri_chan, &ul_sec_chan);

        /* 该BSS为40MHz带宽,计算次信道频点 */
        if (ul_sec_chan) {
            if (ul_sec_chan < ul_pri_chan)
                ul_sec = ul_pri - 20;
            else
                ul_sec = ul_pri + 20;
        }

        if ((ul_pri < ul_affected_start || ul_pri > ul_affected_end) &&
                (ul_sec < ul_affected_start || ul_sec > ul_affected_end))
            continue; /* not within affected channel range */

        /*  */
        if (ul_sec_chan) {
            if (ul_pri_freq != ul_pri || ul_sec_freq != ul_sec) {
                OAM_INFO_LOG4(0, OAM_SF_2040,
                    "hmac_obss_check_40mhz_2g:40 MHz pri/sec <%d, %d >mismatch with BSS <%d, %d>\n", ul_pri_freq,
                    ul_sec_freq, ul_pri, ul_sec);
                /* 解除锁 */
                oal_spin_unlock(&(pst_bss_mgmt->st_lock));
                return OAL_FALSE;
            }
        }
    }

    /* 解除锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    return OAL_TRUE;
}

#ifdef _PRE_WLAN_FEATURE_5G

OAL_STATIC oal_bool_enum_uint8 hmac_obss_check_40mhz_5g(mac_vap_stru *pst_mac_vap,
    hmac_scan_record_stru *pst_scan_record)
{
    oal_uint32            ul_pri_chan;
    oal_uint32            ul_sec_chan;
    oal_uint32            ul_pri_bss;
    oal_uint32            ul_sec_bss;
    oal_uint32            ul_bss_pri_chan;
    oal_uint32            ul_bss_sec_chan;
    oal_uint8             uc_match;
    hmac_bss_mgmt_stru    *pst_bss_mgmt;
    mac_bss_dscr_stru     *pst_bss_dscr;
    hmac_scanned_bss_info *pst_scanned_bss;
    oal_dlist_head_stru   *pst_entry;

    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040,
        "hmac_obss_check_40mhz_5g:pst_mac_vap->st_channel.uc_chan_number = %d\n",
        pst_mac_vap->st_channel.uc_chan_number);

    /* 获取主信道和次信道 */
    ul_pri_chan = pst_mac_vap->st_channel.uc_chan_number;
    if (pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40PLUS) {
        ul_sec_chan = ul_pri_chan + 4;
    } else if (pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40MINUS) {
        ul_sec_chan = ul_pri_chan - 4;
    } else {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040,
            "hmac_obss_check_40mhz_5g: en_bandwidth = %d not need obss scan", pst_mac_vap->st_channel.en_bandwidth);
        return OAL_TRUE;
    }

    /* 获取扫描结果的管理结构地址 */
    pst_bss_mgmt = &(pst_scan_record->st_bss_mgmt);

    /* 获取锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    /* 若在次信道检测到Beacon, 但是主信道上没有, 则需要交换主次信道 */
    ul_pri_bss = ul_sec_bss = 0;
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_bss_mgmt->st_bss_list_head))
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);
        pst_bss_dscr    = &(pst_scanned_bss->st_bss_dscr_info);

        if (pst_bss_dscr == OAL_PTR_NULL) {
            OAM_WARNING_LOG0(0, OAM_SF_2040, "{hmac_obss_check_40mhz_5g::pst_bss_dscr is NULL}");
            continue;
        }

        OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "hmac_obss_check_40mhz_5g:bss uc_channel_number = %d\n",
            pst_bss_dscr->st_channel.uc_chan_number);
        if (pst_bss_dscr->st_channel.uc_chan_number == ul_pri_chan)
            ul_pri_bss++;
        else if (pst_bss_dscr->st_channel.uc_chan_number == ul_sec_chan)
            ul_sec_bss++;
    }

    if (ul_sec_bss && !ul_pri_bss) {
        OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040, "hmac_obss_check_40mhz_5g:Switch own primary and secondary \
            channel to get secondary channel with no Beacons from other BSSes\n");

        hmac_switch_pri_sec(pst_mac_vap);

        /* 此处主次交换完后直接返回即可, 按hostapd-2.4.0版本修改 */
        /* 释放锁 */
        oal_spin_unlock(&(pst_bss_mgmt->st_lock));
        return OAL_TRUE;
    }

    /*
     * Match PRI/SEC channel with any existing HT40 BSS on the same
     * channels that we are about to use (if already mixed order in
     * existing BSSes, use own preference).
     */
    uc_match = OAL_FALSE;
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_bss_mgmt->st_bss_list_head))
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);
        pst_bss_dscr    = &(pst_scanned_bss->st_bss_dscr_info);
        if (pst_bss_dscr == OAL_PTR_NULL) {
            OAM_WARNING_LOG0(0, OAM_SF_2040, "{hmac_obss_check_40mhz_5g::pst_bss_dscr is NULL}");
            continue;
        }

        hmac_get_pri_sec_chan(pst_bss_dscr, &ul_bss_pri_chan, &ul_bss_sec_chan);
        if (ul_pri_chan == ul_bss_pri_chan && ul_sec_chan == ul_bss_sec_chan) {
            uc_match = OAL_TRUE;
            break;
        }
    }

    if (!uc_match) {
        OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_bss_mgmt->st_bss_list_head))
        {
            pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);
            pst_bss_dscr    = &(pst_scanned_bss->st_bss_dscr_info);
            if (pst_bss_dscr == OAL_PTR_NULL) {
                OAM_WARNING_LOG0(0, OAM_SF_2040, "{hmac_obss_check_40mhz_5g::pst_bss_dscr is NULL}");
                continue;
            }

            hmac_get_pri_sec_chan(pst_bss_dscr, &ul_bss_pri_chan, &ul_bss_sec_chan);

            if ((ul_pri_chan == ul_bss_sec_chan) && (ul_sec_chan == ul_bss_pri_chan)) {
                OAM_INFO_LOG0(0, OAM_SF_2040,
                    "hmac_obss_check_40mhz_5g:switch own primary and secondary channel due to BSS overlap with\n");

                hmac_switch_pri_sec(pst_mac_vap);
                break;
            }
        }
    }

    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040,
        "hmac_obss_check_40mhz_5g:After check, pst_mac_vap->st_channel.uc_chan_number = %d\n",
        pst_mac_vap->st_channel.uc_chan_number);

    /* 解除锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    return OAL_TRUE;
}
#endif /* _PRE_WLAN_FEATURE_5G */


oal_uint32 hmac_obss_init_scan_hook(hmac_scan_record_stru *pst_scan_record,
                                    hmac_device_stru      *pst_dev)
{
    oal_uint8                         uc_idx;
    oal_bool_enum_uint8               en_bandwidth_40MHz = OAL_FALSE;
    oal_bool_enum_uint8               en_2g_first_vap_bandwidth_40MHz = OAL_FALSE;
    mac_vap_stru                      *pst_mac_vap = NULL;
    oal_uint8                         uc_2g_check_band = OAL_FALSE;
#ifdef _PRE_WLAN_FEATURE_5G
    oal_uint8                         uc_5g_check_band = OAL_FALSE;
    oal_uint8                         uc_5g_first_vap_chan_number = 0;
    wlan_channel_bandwidth_enum_uint8 en_5g_first_vap_bandwidth = WLAN_BAND_WIDTH_20M;
#endif /* _PRE_WLAN_FEATURE_5G */

    OAM_WARNING_LOG0(0, OAM_SF_ACS, "hmac_obss_init_scan_hook run\n");

    /* 遍历每个vap,检测信道和带宽 */
    for (uc_idx = 0; uc_idx < pst_dev->pst_device_base_info->uc_vap_num; uc_idx++) {
        pst_mac_vap = mac_res_get_mac_vap(pst_dev->pst_device_base_info->auc_vap_id[uc_idx]);
        if (pst_mac_vap && pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
            OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_2040,
                "hmac_obss_init_scan_hook:original channel=%d, bandwidth=%d.\n",
                pst_mac_vap->st_channel.uc_chan_number, pst_mac_vap->st_channel.en_bandwidth);

            if (pst_mac_vap->st_channel.en_band == WLAN_BAND_2G) {
                if (pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40PLUS ||
                        pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40MINUS) {
                    /* 只检测第一个vap，后面的跟随第一个vap的带宽和信道 */
                    if (uc_2g_check_band == OAL_FALSE) {
                        /* 2G vap 20/40共存检测 */
                        en_bandwidth_40MHz = hmac_obss_check_40mhz_2g(pst_mac_vap, pst_scan_record);

                        OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040,
                            "hmac_obss_init_scan_hook:2.4G first vap, en_bandwidth_40MHz=%d\n", en_bandwidth_40MHz);
                            uc_2g_check_band = OAL_TRUE;
                            en_2g_first_vap_bandwidth_40MHz = en_bandwidth_40MHz;
                    } else {
                        /* 第一个vap检测完成之后,后续的同频段vap信道和带宽都跟随第一个 */
                        OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040,
                            "hmac_obss_init_scan_hook:follow bandwidth of first 2g vap.\n");
                            en_bandwidth_40MHz = en_2g_first_vap_bandwidth_40MHz;
                    }
                }

                /* 设置带宽 */
                if (en_bandwidth_40MHz == OAL_FALSE) {
                    OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040,
                        "hmac_obss_init_scan_hook:cann't set 40MHz bandwidth,change to 20MHz.\n");
                        pst_mac_vap->st_channel.en_bandwidth = WLAN_BAND_WIDTH_20M;
                }
            }
#ifdef _PRE_WLAN_FEATURE_5G
            else if (pst_mac_vap->st_channel.en_band == WLAN_BAND_5G) {
                if (pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40PLUS ||
                        pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40MINUS) {
                    if (uc_5g_check_band == OAL_FALSE) {
                        /* 5G vap 20/40共存检测 */
                        en_bandwidth_40MHz = hmac_obss_check_40mhz_5g(pst_mac_vap, pst_scan_record);
                        uc_5g_check_band = OAL_TRUE;
                        uc_5g_first_vap_chan_number = pst_mac_vap->st_channel.uc_chan_number;
                        en_5g_first_vap_bandwidth = pst_mac_vap->st_channel.en_bandwidth;
                    } else {
                        /* 第一个vap检测完成之后,后续的同频段vap信道和带宽都跟随第一个 */
                        OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040,
                            "hmac_obss_init_scan_hook:follow bandwidth of first 5g vap.\n");
                            pst_mac_vap->st_channel.uc_chan_number = uc_5g_first_vap_chan_number;
                            pst_mac_vap->st_channel.en_bandwidth = en_5g_first_vap_bandwidth;
                    }
                }
            }
#endif /* _PRE_WLAN_FEATURE_5G */
            else {
                OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040,
                    "hmac_obss_init_scan_hook: en_band = %d not support!", pst_mac_vap->st_channel.en_band);
                continue;
            }

            /* 记录信道和带宽 */
            pst_dev->ast_best_channel[pst_mac_vap->st_channel.en_band].uc_chan_number =
                pst_mac_vap->st_channel.uc_chan_number;
            pst_dev->ast_best_channel[pst_mac_vap->st_channel.en_band].en_bandwidth =
                pst_mac_vap->st_channel.en_bandwidth;
            pst_dev->pst_device_base_info->uc_max_channel = pst_mac_vap->st_channel.uc_chan_number;
            pst_mac_vap->st_ch_switch_info.en_user_pref_bandwidth = pst_mac_vap->st_channel.en_bandwidth;
            OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_2040,
                "hmac_obss_init_scan_hook:After adjust, channel=%d, bandwidth=%d.\n",
                pst_mac_vap->st_channel.uc_chan_number, pst_mac_vap->st_channel.en_bandwidth);
        }
    }

    return OAL_SUCC;
}
#endif

#if defined(_PRE_SUPPORT_ACS) || defined(_PRE_WLAN_FEATURE_DFS) || defined(_PRE_WLAN_FEATURE_20_40_80_COEXIST)

oal_bool_enum_uint8 hmac_device_in_init_scan(mac_device_stru *pst_mac_device)
{
    hmac_device_stru *pst_hmac_device = OAL_PTR_NULL;

    pst_hmac_device = hmac_res_get_mac_dev(pst_mac_device->uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "hmac_res_get_mac_dev:pst_hmac_device is null\n");
        return OAL_FALSE;
    } else {
        return pst_hmac_device->en_in_init_scan;
    }
}


mac_need_init_scan_res_enum_uint8 hmac_need_init_scan(hmac_device_stru *pst_hmac_device, mac_vap_stru *pst_in_mac_vap,
    mac_try_init_scan_type en_type)
{
    mac_need_init_scan_res_enum_uint8 en_result = MAC_INIT_SCAN_NOT_NEED;
    mac_device_stru *pst_mac_device = pst_hmac_device->pst_device_base_info;

    if (OAL_UNLIKELY(pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_need_init_scan::pst_device_base_info null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_in_mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_AP) {
        return MAC_INIT_SCAN_NOT_NEED;
    }

    if (en_type == MAC_TRY_INIT_SCAN_VAP_UP && !pst_hmac_device->en_start_via_priv) {
        return MAC_INIT_SCAN_NOT_NEED;
    }

    if (pst_hmac_device->en_in_init_scan) {
        return en_type == MAC_TRY_INIT_SCAN_RESCAN ? MAC_INIT_SCAN_NOT_NEED : MAC_INIT_SCAN_IN_SCAN;
    }

#ifdef _PRE_SUPPORT_ACS
    en_result |= ((mac_get_acs_switch(pst_mac_device) == MAC_ACS_SW_INIT ||
                  mac_get_acs_switch(pst_mac_device) == MAC_ACS_SW_BOTH ? OAL_TRUE : OAL_FALSE));
    if (en_type == MAC_TRY_INIT_SCAN_RESCAN) {
        return en_result;
    }
#endif

#ifdef _PRE_WLAN_FEATURE_DFS
    /*lint -e514*/
    en_result |= mac_dfs_get_dfs_enable(pst_mac_device) &&
                 mac_dfs_get_cac_enable(pst_hmac_device->pst_device_base_info);
    /*lint +e514*/

#endif

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    en_result |= mac_get_2040bss_switch(pst_mac_device);
#endif

    return en_result;
}


oal_uint32 hmac_init_scan_timeout(void *p_arg)
{
    hmac_device_stru *pst_dev = (hmac_device_stru *)p_arg;

    if (!pst_dev->en_init_scan) {
        return OAL_SUCC;
    }

    pst_dev->en_init_scan = OAL_FALSE;
    pst_dev->en_in_init_scan = OAL_FALSE;
    hmac_start_all_bss_of_device(pst_dev);

    return OAL_SUCC;
}


oal_uint32 hmac_init_scan_cancel_timer(hmac_device_stru *pst_hmac_dev)
{
    if (pst_hmac_dev && pst_hmac_dev->st_scan_mgmt.st_init_scan_timeout.en_is_enabled) {
        /* 关闭超时定时器 */
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_hmac_dev->st_scan_mgmt.st_init_scan_timeout);
    }

    return OAL_SUCC;
}


oal_void hmac_init_scan_cb(void *p_scan_record)
{
    hmac_scan_record_stru *pst_scan_record = (hmac_scan_record_stru *)p_scan_record;
    oal_uint8             uc_device_id     = pst_scan_record->uc_device_id;
    hmac_device_stru      *pst_hmac_dev    = hmac_res_get_mac_dev(uc_device_id);
    mac_device_stru       *pst_mac_dev     = NULL;

    OAM_WARNING_LOG0(0, OAM_SF_ACS, "{hmac_init_scan_cb called}");

    if (pst_hmac_dev == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(0, OAM_SF_ACS, "{hmac_init_scan_cb:pst_hmac_dev=NULL, device_id=%d}", uc_device_id);
        return;
    }

    pst_mac_dev = pst_hmac_dev->pst_device_base_info;
    if (pst_hmac_dev == OAL_PTR_NULL || pst_mac_dev == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(0, OAM_SF_ACS, "{hmac_init_scan_cb:pst_mac_dev=NULL, device_id=%d}", uc_device_id);
        return;
    }

#ifdef _PRE_SUPPORT_ACS
    if (mac_get_acs_switch(pst_mac_dev) == MAC_ACS_SW_INIT || mac_get_acs_switch(pst_mac_dev) == MAC_ACS_SW_BOTH) {
        OAM_WARNING_LOG0(0, OAM_SF_ACS, "acs  enable, post and return\n");
        /* 若ACS在运行中，则等待ACS APP返回响应结果 */
        if (hmac_acs_init_scan_hook(pst_scan_record, pst_hmac_dev) == OAL_SUCC) {
            return;
        }
    }
#endif

    OAM_WARNING_LOG0(0, OAM_SF_ACS, "acs not enable, cancel timer\n");
    /* ACS未执行，超时保护至此结束 */
    hmac_init_scan_cancel_timer(pst_hmac_dev);

    /* 若ACS未执行或者执行失败，继续执行后续操作 */
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    if (mac_get_2040bss_switch(pst_mac_dev)) {
        hmac_obss_init_scan_hook(pst_scan_record, pst_hmac_dev);
    }
#endif

    OAM_WARNING_LOG3(0, OAM_SF_ACS, "<<< rsp=5:scan report ch=%d bss=%d init=%d\n", pst_scan_record->uc_chan_numbers,
        pst_scan_record->st_bss_mgmt.ul_bss_num, pst_hmac_dev->en_init_scan);
#ifdef _PRE_WLAN_FEATURE_DFS
    if (mac_dfs_get_dfs_enable(pst_mac_dev)) {
        /* 若成功开始了CAC或者已经开始CAC，返回，由CAC超时函数处理VAP START */
        if (hmac_dfs_init_scan_hook(pst_scan_record, pst_hmac_dev) == OAL_SUCC) {
            return;
        }
    }
#endif

    /* ACS未运行、DFS未运行，直接启动BSS */
    hmac_init_scan_timeout(pst_hmac_dev);
}


oal_uint32 hmac_init_scan_do(hmac_device_stru *pst_hmac_dev, mac_vap_stru *pst_mac_vap, mac_init_scan_req_stru *pst_cmd)
{
    oal_uint8                         uc_idx, uc_cnt;
    mac_scan_req_stru                 st_scan_req;
    wlan_channel_band_enum_uint8      en_band;
    wlan_channel_bandwidth_enum_uint8 en_bandwidth;
    oal_uint32                        ul_ret = OAL_FAIL;
    hmac_vap_stru                     *pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(0, OAM_SF_ACS, "{hmac_init_scan_do:get hmac vap=NULL, vapid=%d}", pst_mac_vap->uc_vap_id);
        return OAL_FAIL;
    }

    memset_s(&st_scan_req, OAL_SIZEOF(st_scan_req), 0, OAL_SIZEOF(st_scan_req));
    st_scan_req.en_scan_mode  = pst_cmd->auc_arg[0];
    st_scan_req.en_scan_type  = pst_cmd->auc_arg[1];
    st_scan_req.uc_scan_func  = pst_cmd->auc_arg[2];
    st_scan_req.en_bss_type   = WLAN_MIB_DESIRED_BSSTYPE_INFRA;
    st_scan_req.uc_bssid_num  = 0;
    st_scan_req.uc_ssid_num   = 0;

    st_scan_req.uc_max_scan_count_per_channel = 1;
    st_scan_req.uc_max_send_probe_req_count_per_channel = st_scan_req.en_scan_mode == WLAN_SCAN_MODE_FOREGROUND ? 2 : 1;

    st_scan_req.us_scan_time = st_scan_req.en_scan_mode == WLAN_SCAN_MODE_FOREGROUND ? 120 : 30;
    st_scan_req.uc_probe_delay = 0;
    st_scan_req.uc_vap_id = pst_mac_vap->uc_vap_id; /* 其实是该device下的vap_id[0] */
    st_scan_req.p_fn_cb = hmac_init_scan_cb;

    st_scan_req.uc_channel_nums = 0; /* 信道列表中信道的个数 */
    uc_cnt = 0;
    for (uc_idx = 0; uc_idx < pst_cmd->ul_cmd_len; uc_idx += 2) {
        en_band      = pst_cmd->auc_data[uc_idx] & 0X0F;
        en_bandwidth = (pst_cmd->auc_data[uc_idx] >> 4) & 0x0F;
        if (mac_is_channel_num_valid(en_band, pst_cmd->auc_data[uc_idx + 1]) != OAL_SUCC) {
            OAM_WARNING_LOG3(0, OAM_SF_ACS, "{hmac_init_scan_do:invalid channel number, ch=%d, band=%d bw=%d}",
                pst_cmd->auc_data[uc_idx + 1], en_band, en_bandwidth);
            OAL_IO_PRINT("{hmac_init_scan_do:invalid channel number, ch=%d, band=%d bw=%d}\n",
                pst_cmd->auc_data[uc_idx + 1], en_band, en_bandwidth);

            continue;
        }

        st_scan_req.ast_channel_list[uc_cnt].uc_chan_number = pst_cmd->auc_data[uc_idx + 1];
        st_scan_req.ast_channel_list[uc_cnt].en_band        = en_band;
        st_scan_req.ast_channel_list[uc_cnt].en_bandwidth   = en_bandwidth;
        ul_ret = mac_get_channel_idx_from_num(st_scan_req.ast_channel_list[uc_cnt].en_band,
                                              st_scan_req.ast_channel_list[uc_cnt].uc_chan_number,
                                              &st_scan_req.ast_channel_list[uc_cnt].uc_idx);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG2(0, OAM_SF_ACS,
                "{hmac_init_scan_do:mac_get_channel_idx_from_num failed en_band:[%d],chan_number:[%d]}",
                st_scan_req.ast_channel_list[uc_cnt].en_band, st_scan_req.ast_channel_list[uc_cnt].uc_chan_number);
        }
        uc_cnt++;
    }

    OAL_IO_PRINT(">>> got=5:do scan mode=%d func=0x%x type=%d ch_cnt=%d\n",
                 st_scan_req.en_scan_mode,
                 st_scan_req.uc_scan_func, st_scan_req.en_scan_type, uc_cnt);

    if (uc_cnt != 0) {
        st_scan_req.uc_channel_nums = uc_cnt;

        /* 直接调用扫描模块扫描请求处理函数 */
        ul_ret = hmac_scan_proc_scan_req_event(pst_hmac_vap, &st_scan_req);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN, "hmac_init_scan_do:hmac_scan_add_req failed, ret=%d", ul_ret);
        }
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "hmac_init_scan_do:no valid channel found, not scan");
    }

    return ul_ret;
}

oal_bool_enum_uint8 hmac_init_scan_skip_channel(hmac_device_stru *pst_hmac_dev,
                                                wlan_channel_band_enum_uint8 en_band,
                                                oal_uint8 uc_idx)
{
    /* skip any illegal channel */
    if (mac_is_channel_idx_valid(en_band, uc_idx) != OAL_SUCC) {
        return OAL_TRUE;
    }
    if (OAL_UNLIKELY(pst_hmac_dev->pst_device_base_info == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_init_scan_skip_channel::pst_device_base_info null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_hmac_dev->pst_device_base_info && pst_hmac_dev->pst_device_base_info->en_max_band != en_band) {
        return  OAL_TRUE;
    }

    return OAL_FALSE;
}

uint32_t hmac_init_scan_process(hmac_device_stru *pst_hmac_dev, mac_vap_stru *pst_mac_vap, mac_scan_op_enum_uint8 en_op)
{
    oal_uint8  ast_buf[OAL_SIZEOF(mac_acs_cmd_stru) - 4 + WLAN_MAX_CHANNEL_NUM * 2];
    oal_uint8  uc_idx;
    oal_uint8  *puc_dat, uc_tot, uc_chan_num;
    oal_uint32 ul_ret;

    mac_init_scan_req_stru *pst_cmd = (mac_init_scan_req_stru *)ast_buf;

    puc_dat = pst_cmd->auc_data;

    pst_hmac_dev->en_init_scan    = (en_op ==  MAC_SCAN_OP_INIT_SCAN) ? OAL_TRUE : OAL_FALSE;
    pst_hmac_dev->en_in_init_scan = OAL_TRUE;

    pst_cmd->uc_cmd = 5;
    pst_cmd->auc_arg[0] = (en_op ==  MAC_SCAN_OP_FG_SCAN_ONLY || en_op ==  MAC_SCAN_OP_INIT_SCAN) ?
                          WLAN_SCAN_MODE_FOREGROUND : WLAN_SCAN_MODE_BACKGROUND_AP;

    pst_cmd->auc_arg[1] = WLAN_SCAN_TYPE_ACTIVE;
    pst_cmd->auc_arg[2] = MAC_SCAN_FUNC_MEAS | MAC_SCAN_FUNC_STATS | MAC_SCAN_FUNC_BSS;
    pst_cmd->ul_cmd_cnt = 0;

    uc_tot = 0;
    for (uc_idx = 0; uc_idx < MAC_CHANNEL_FREQ_2_BUTT; uc_idx++) {
        if (!hmac_init_scan_skip_channel(pst_hmac_dev, WLAN_BAND_2G, uc_idx)) {
            mac_get_channel_num_from_idx(WLAN_BAND_2G, uc_idx, &uc_chan_num);
            *puc_dat++ = ((WLAN_BAND_WIDTH_20M) << 4) | WLAN_BAND_2G;
            *puc_dat++ = uc_chan_num;
            uc_tot++;
        }
    }
#ifdef _PRE_WLAN_FEATURE_5G
    for (uc_idx = 0; uc_idx < MAC_CHANNEL_FREQ_5_BUTT; uc_idx++) {
        if (!hmac_init_scan_skip_channel(pst_hmac_dev, WLAN_BAND_5G, uc_idx)) {
            mac_get_channel_num_from_idx(WLAN_BAND_5G, uc_idx, &uc_chan_num);
            *puc_dat++ = ((WLAN_BAND_WIDTH_20M) << 4) | WLAN_BAND_5G;
            *puc_dat++ = uc_chan_num;
            uc_tot++;
        }
    }
#endif /* _PRE_WLAN_FEATURE_5G */

    pst_cmd->ul_cmd_len = uc_tot * 2;

    /* best信道清空为0，在start bss作为合法性判断依据 */
    if (pst_hmac_dev->en_init_scan) {
        memset_s(pst_hmac_dev->ast_best_channel, OAL_SIZEOF(pst_hmac_dev->ast_best_channel), 0,
            OAL_SIZEOF(pst_hmac_dev->ast_best_channel));
    }

    /* 启动定时器, 超时后强制启动BSS: 保护时间包括扫描时长与APP交互时长 */
    {
        FRW_TIMER_CREATE_TIMER(&pst_hmac_dev->st_scan_mgmt.st_init_scan_timeout,
                               hmac_init_scan_timeout,
                               6000,
                               pst_hmac_dev,
                               OAL_FALSE,
                               OAM_MODULE_ID_HMAC,
                               pst_hmac_dev->pst_device_base_info->ul_core_id);
    }

    ul_ret = hmac_init_scan_do(pst_hmac_dev, pst_mac_vap, pst_cmd);
    if (ul_ret != OAL_SUCC) {
        pst_hmac_dev->en_init_scan    = OAL_FALSE;
        pst_hmac_dev->en_in_init_scan = OAL_FALSE;
    }

    return ul_ret;
}

oal_uint32  hmac_init_scan_try(mac_device_stru *pst_mac_device, mac_vap_stru *pst_in_mac_vap,
    mac_try_init_scan_type en_type)
{
    oal_uint8           uc_idx;
    mac_vap_stru        *pst_mac_vap = NULL;
    mac_vap_stru        *pst_mac_vap_scan = NULL;
    hmac_vap_stru       *pst_hmac_vap = NULL;
    oal_uint8           en_scan_type;
    hmac_device_stru    *pst_hmac_device = hmac_res_get_mac_dev(pst_mac_device->uc_device_id);
    oal_bool_enum_uint8 en_need_do_init_scan = OAL_FALSE;

    if (pst_hmac_device == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{hmac_init_scan_try:mac_res_get_hmac_vap failed. device_id:%d.}",
            pst_mac_device->uc_device_id);
        return OAL_FAIL;
    }
    en_scan_type = hmac_need_init_scan(pst_hmac_device, pst_in_mac_vap, en_type);
    if (en_scan_type == MAC_INIT_SCAN_NOT_NEED) {
        return OAL_FAIL;
    } else if(en_scan_type == MAC_INIT_SCAN_IN_SCAN) {
        OAL_IO_PRINT("just in init scan\n");

        mac_vap_init_rates(pst_in_mac_vap);
        pst_hmac_vap = mac_res_get_hmac_vap(pst_in_mac_vap->uc_vap_id);
        if (pst_hmac_vap == OAL_PTR_NULL) {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{hmac_init_scan_try:mac_res_get_hmac_vap failed vap_id:%d.}",
                pst_in_mac_vap->uc_vap_id);
            return OAL_FAIL;
        }
        if (pst_hmac_vap->st_vap_base_info.en_vap_state == MAC_VAP_STATE_UP) {
            hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_AP_WAIT_START);
        }

        return OAL_SUCC;
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "need init scan");
    }

    for (uc_idx = 0; uc_idx < pst_hmac_device->pst_device_base_info->uc_vap_num; uc_idx++) {
        pst_mac_vap = mac_res_get_mac_vap(pst_hmac_device->pst_device_base_info->auc_vap_id[uc_idx]);
        if (!pst_mac_vap) {
            continue;
        }

        if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
            mac_vap_init_rates(pst_mac_vap);

            /* 强制设置AP侧状态机为 WAIT_START，因为需要执行初始信道检查 */
            pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
            if (pst_hmac_vap == OAL_PTR_NULL) {
                OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{hmac_init_scan_try::need init scan fail.vap_id = %u}",
                    pst_mac_vap->uc_vap_id);
                continue;
            }
            switch (pst_hmac_vap->st_vap_base_info.en_vap_state) {
                case MAC_VAP_STATE_UP:
                case MAC_VAP_STATE_PAUSE: // dbac
                    hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_AP_WAIT_START);
                // no break here!
                case MAC_VAP_STATE_AP_WAIT_START:
                    en_need_do_init_scan = OAL_TRUE;
                    pst_mac_vap_scan = pst_mac_vap;
                    break;
                default :
                    break;
            }
        }
    }

    if (en_need_do_init_scan) {
        return hmac_init_scan_process(pst_hmac_device, pst_mac_vap_scan, MAC_SCAN_OP_INIT_SCAN);
    }

    return OAL_SUCC;
}

#endif
/*lint -e578*//*lint -e19*/
oal_module_symbol(hmac_scan_find_scanned_bss_dscr_by_index);
oal_module_symbol(hmac_scan_find_scanned_bss_by_bssid);
/*lint -e578*//*lint -e19*/

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

