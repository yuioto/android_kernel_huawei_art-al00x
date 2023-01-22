
#ifndef __OAM_LOG_H__
#define __OAM_LOG_H__

#include "oal_ext_if.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define OAM_LOG_PARAM_MAX_NUM           4                                       /* 可打印最多的参数个数 */
#define OAM_LOG_PRINT_DATA_LENGTH       512                                     /* 每次写入文件的最大长度 */

/* 特性宏的缩写见gst_oam_feature_list */
typedef enum {
    OAM_SF_SCAN                 = 0,
    OAM_SF_AUTH,
    OAM_SF_ASSOC,
    OAM_SF_FRAME_FILTER,
    OAM_SF_WMM,

    OAM_SF_DFS                  = 5,
    OAM_SF_NETWORK_MEASURE,
    OAM_SF_ENTERPRISE_VO,
    OAM_SF_HOTSPOTROAM,
    OAM_SF_NETWROK_ANNOUNCE,

    OAM_SF_NETWORK_MGMT         = 10,
    OAM_SF_NETWORK_PWS,
    OAM_SF_PROXYARP,
    OAM_SF_TDLS,
    OAM_SF_CALIBRATE,

    OAM_SF_EQUIP_TEST           = 15,
    OAM_SF_CRYPTO,
    OAM_SF_WPA,
    OAM_SF_WEP,
    OAM_SF_WPS,

    OAM_SF_PMF                  = 20,
    OAM_SF_WAPI,
    OAM_SF_BA,
    OAM_SF_AMPDU,
    OAM_SF_AMSDU,

    OAM_SF_STABILITY            = 25,
    OAM_SF_TCP_OPT,
    OAM_SF_ACS,
    OAM_SF_AUTORATE,
    OAM_SF_TXBF,

    OAM_SF_DYN_RECV             = 30,                        /* dynamin recv */
    OAM_SF_VIVO,                            /* video_opt voice_opt */
    OAM_SF_MULTI_USER,
    OAM_SF_MULTI_TRAFFIC,
    OAM_SF_ANTI_INTF,

    OAM_SF_EDCA                 = 35,
    OAM_SF_SMART_ANTENNA,
    OAM_SF_TPC,
    OAM_SF_TX_CHAIN,
    OAM_SF_RSSI,

    OAM_SF_WOW                  = 40,
    OAM_SF_GREEN_AP,
    OAM_SF_PWR,                             /* psm uapsd fastmode */
    OAM_SF_SMPS,
    OAM_SF_TXOP,

    OAM_SF_WIFI_BEACON          = 45,
    OAM_SF_KA_AP,                           /* keep alive ap */
    OAM_SF_MULTI_VAP,
    OAM_SF_2040,                            /* 20m+40m coex */
    OAM_SF_DBAC,

    OAM_SF_PROXYSTA             = 50,
    OAM_SF_UM,                              /* user managment */
    OAM_SF_P2P,                             /* P2P 特性 */
    OAM_SF_M2U,
    OAM_SF_IRQ,                             /* top half */

    OAM_SF_TX                   = 55,
    OAM_SF_RX,
    OAM_SF_DUG_COEX,
    OAM_SF_CFG,                             /* wal dmac config函数 */
    OAM_SF_FRW,                             /* frw层 */

    OAM_SF_KEEPALIVE            = 60,
    OAM_SF_COEX,
    OAM_SF_HS20                 = 62,	    /* HotSpot 2.0特性 */
    OAM_SF_MWO_DET,
    OAM_SF_CCA_OPT,

    OAM_SF_ROAM                 = 65,       /* roam module, #ifdef _PRE_WLAN_FEATURE_ROAM */
    OAM_SF_DFT,
    OAM_SF_DFR,
    OAM_SF_FIRMWARE,
    OAM_SF_HEARTBEAT,
    OAM_SF_SDIO,
    OAM_SF_BACKUP,
    OAM_SF_ANY,                             /* rifs protection shortgi frag datarate countrycode
                                                coustom_security startup_time lsig monitor wds
                                                hidessid */
    OAM_SF_APP_MSG,
#ifdef _PRE_WLAN_FEATURE_SAE
    OAM_SF_SAE,
#endif
    OAM_SOFTWARE_FEATURE_BUTT
} oam_feature_enum;
extern oal_uint32  oam_log_init(oal_void);
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
extern  oal_uint32  oam_log_sdt_out(oal_uint16      us_level,
    const oal_int8  *pc_func_name, const oal_int8  *pc_fmt, ...);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oam_log.h */
