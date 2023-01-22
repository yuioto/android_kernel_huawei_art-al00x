

#ifndef __WAL_LINUX_BRIDGE_H__
#define __WAL_LINUX_BRIDGE_H__

// 1 其他头文件包含
#include "oal_ext_if.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_LINUX_BRIDGE_H

// 10 函数声明
extern oal_net_dev_tx_enum wal_bridge_vap_xmit(oal_netbuf_stru *pst_buf, oal_net_device_stru *pst_dev);
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
oal_net_dev_tx_enum wal_vap_start_xmit(oal_netbuf_stru *pst_buf, oal_net_device_stru *pst_dev);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of wal_linux_bridge.h */
