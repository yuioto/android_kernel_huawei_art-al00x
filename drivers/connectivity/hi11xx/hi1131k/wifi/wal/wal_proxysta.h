

#ifndef __WAL_PROXYSTA_H__
#define __WAL_PROXYSTA_H__

#include "oal_ext_if.h"
#include "frw_ext_if.h"
#include "mac_vap.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_PROXYSTA_H

#ifdef _PRE_WLAN_FEATURE_PROXYSTA

extern oal_uint32 wal_proxysta_add_vap(frw_event_mem_stru *pst_event_mem);
extern oal_uint32 wal_proxysta_remove_vap(mac_vap_stru *pst_mac_vap);

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of wal_proxysta.h */
