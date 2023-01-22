
#ifndef WAL_LINUX_CFGVENDOR_H_
#define WAL_LINUX_CFGVENDOR_H_

// 其他头文件包含
#include "oal_types.h"
#include "oal_net.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_LINUX_CFGVENDOR_H

#if defined(_PRE_WLAN_FEATURE_APF) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)

#define ATTRIBUTE_U32_LEN      (OAL_NLMSG_HDRLEN + 4)
#define VENDOR_ID_OVERHEAD     ATTRIBUTE_U32_LEN
#define VENDOR_SUBCMD_OVERHEAD ATTRIBUTE_U32_LEN
#define VENDOR_DATA_OVERHEAD   (OAL_NLMSG_HDRLEN)

#define VENDOR_REPLY_OVERHEAD (VENDOR_ID_OVERHEAD + VENDOR_SUBCMD_OVERHEAD + VENDOR_DATA_OVERHEAD)

#define APF_PROGRAM_MAX_LEN 512
#define APF_VERSION         2

enum wal_apf_attributes {
    APF_ATTRIBUTE_VERSION,
    APF_ATTRIBUTE_MAX_LEN,
    APF_ATTRIBUTE_PROGRAM,
    APF_ATTRIBUTE_PROGRAM_LEN
};

typedef enum {
    // define all packet filter related commands between 0x1800 and 0x18FF
    VENDOR_NL80211_SUBCMD_PKT_FILTER_RANGE_START = 0x1800,
    VENDOR_NL80211_SUBCMD_PKT_FILTER_RANGE_END = 0x18FF,
} wal_vendor_sub_command_enum;

enum wal_vendor_subcmd {
    APF_SUBCMD_GET_CAPABILITIES = VENDOR_NL80211_SUBCMD_PKT_FILTER_RANGE_START,
    APF_SUBCMD_SET_FILTER,

    VENDOR_SUBCMD_MAX
};

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

extern void wal_cfgvendor_init(oal_wiphy_stru *wiphy);
extern void wal_cfgvendor_deinit(oal_wiphy_stru *wiphy);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif  // defined(_PRE_WLAN_FEATURE_APF) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#endif  // WAL_LINUXCFGVENDOR_H_
