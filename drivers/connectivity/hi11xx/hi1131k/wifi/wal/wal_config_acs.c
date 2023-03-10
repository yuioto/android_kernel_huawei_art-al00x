

#ifdef _PRE_SUPPORT_ACS

// 1 头文件包含
#include "oam_ext_if.h"
#include "frw_ext_if.h"
#include "wal_main.h"
#include "oam_linux_netlink.h"
#include "wal_config_acs.h"
#include "wal_ext_if.h"
#include "mac_vap.h"
#include "mac_resource.h"
#include "mac_device.h"
#include "hmac_ext_if.h"
#include "dmac_acs.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_CONFIG_ACS_C

// 2 全局变量定义
extern oal_void oam_netlink_ops_register(oam_nl_cmd_enum_uint8 en_type,
    oal_uint32 (*p_func)(oal_uint8 *puc_data, oal_uint32 ul_len));
extern oal_void oam_netlink_ops_unregister(oam_nl_cmd_enum_uint8 en_type);
extern oal_int32 oam_netlink_kernel_send(oal_uint8 *puc_data, oal_uint32 ul_data_len, oam_nl_cmd_enum_uint8 en_type);

frw_timeout_stru g_st_acs_timer;


// 3 函数实现

oal_uint32 wal_acs_netlink_recv(const oal_uint8 *puc_data, oal_uint32 ul_len)
{
    oal_uint32        ul_device_num;
    oal_uint32        ul_ret;
    mac_device_stru  *pst_mac_dev = OAL_PTR_NULL;
    mac_vap_stru     *pst_mac_vap = OAL_PTR_NULL;
    mac_acs_cmd_stru *pst_acs_cmd_hdr = OAL_PTR_NULL;

    pst_acs_cmd_hdr = (mac_acs_cmd_stru *)puc_data;

    /* 向所有DEVICE广播一份 */
    for (ul_device_num = 0; ul_device_num < MAC_RES_MAX_DEV_NUM; ul_device_num++) {
        pst_mac_dev = mac_res_get_dev(ul_device_num);
        /* 设备不存在 */
        if (pst_mac_dev == OAL_PTR_NULL) {
            continue;
        }

        /* 设备未初始化 */
        if (pst_mac_dev->en_device_state == OAL_FALSE) {
            continue;
        }

        /* ACS未使能 */
        if (pst_mac_dev->pst_acs == OAL_PTR_NULL) {
            continue;
        }

        // note:假如没有任何业务VAP，则驱动收不到应用层的请求。
        pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_dev->auc_vap_id[0]);
        if (pst_mac_vap == OAL_PTR_NULL) {
            continue;
        }

        ul_ret = hmac_config_set_acs_cmd(pst_mac_vap, (oal_uint16)ul_len, puc_data);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                "{wal_acs_netlink_recv::app send cmd failed:cmd=%d seq=%d dev=%d}",
                pst_acs_cmd_hdr->uc_cmd, pst_acs_cmd_hdr->ul_cmd_cnt, ul_device_num);
        }
    }

    return OAL_SUCC;
}


oal_uint32 wal_acs_response_event_handler(frw_event_mem_stru *pst_event_mem)
{
    mac_acs_response_hdr_stru *pst_acs_resp_hdr;
    frw_event_stru            *pst_event;

    pst_event        = (frw_event_stru *)pst_event_mem->puc_data;
    pst_acs_resp_hdr = (mac_acs_response_hdr_stru *)pst_event->auc_event_data;

    if (pst_acs_resp_hdr->uc_cmd == DMAC_ACS_CMD_DO_SCAN) {
        oal_uint32  puc_real_dat = *(oal_uint32 *)(pst_acs_resp_hdr + 1);

        pst_acs_resp_hdr = (mac_acs_response_hdr_stru *)puc_real_dat;
        oam_netlink_kernel_send((oal_uint8 *)pst_acs_resp_hdr, pst_acs_resp_hdr->ul_len, OAM_NL_CMD_ACS);
        OAL_MEM_FREE((oal_void *)puc_real_dat, OAL_TRUE);
    } else {
        oam_netlink_kernel_send((oal_uint8 *)pst_acs_resp_hdr, pst_acs_resp_hdr->ul_len, OAM_NL_CMD_ACS);
    }

    return OAL_SUCC;
}


oal_uint32 wal_acs_timer_handler(void *p_arg)
{
    oal_uint8   auc_buf[8];
    oal_uint32  ul_len = 4;

    if (memset_s(auc_buf, sizeof(auc_buf), 0, sizeof(auc_buf)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_acs_timer_handler::memset fail !");
        return OAL_FAIL;
    }

    auc_buf[0] = 2;  // ACS PING COMMAND

    wal_acs_netlink_recv(auc_buf, ul_len);

    return OAL_SUCC;
}


oal_uint32 wal_acs_init(oal_void)
{
    oam_netlink_ops_register(OAM_NL_CMD_ACS, wal_acs_netlink_recv);
    /* 测试使用 */
    return OAL_SUCC;
}


oal_uint32 wal_acs_exit(oal_void)
{
    oam_netlink_ops_unregister(OAM_NL_CMD_ACS);

    return OAL_SUCC;
}

#endif // #ifdef _PRE_SUPPORT_ACS

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
