


#ifndef __HMAC_EDCA_OPT_H__
#define __HMAC_EDCA_OPT_H__


#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "frw_ext_if.h"
#include "mac_device.h"
#include "dmac_ext_if.h"
#include "oam_ext_if.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define HMAC_EDCA_OPT_MIN_PKT_LEN   256 /* 小于该长度的ip报文不被统计，排除chariot控制报文 */

#define HMAC_EDCA_OPT_TIME_MS       30000 /* edca参数调整默认定时器 */

#define HMAC_EDCA_OPT_PKT_NUM       ((HMAC_EDCA_OPT_TIME_MS) >> 3) /* 平均每毫秒报文个数 */

#define WLAN_EDCA_OPT_MAX_WEIGHT_STA    (3)
#define WLAN_EDCA_OPT_WEIGHT_STA        (2)
/*****************************************************************************
  3 枚举定义
*****************************************************************************/
typedef enum {
    WLAN_TX_TCP_DATA = 0,    /* 发送TCP data */
    WLAN_RX_TCP_DATA = 1,    /* 接收TCP data */
    WLAN_TX_UDP_DATA = 2,    /* 发送UDP data */
    WLAN_RX_UDP_DATA = 3,    /* 接收UDP data */

    WLAN_TXRX_DATA_BUTT = 4,
}wlan_txrx_data_type_enum;
typedef oal_uint8 wlan_txrx_data_enum_uint8;
/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_void hmac_edca_opt_rx_pkts_stat(oal_uint16 us_assoc_id, oal_uint8 uc_tidno, mac_ip_header_stru *pst_ip);
extern oal_void hmac_edca_opt_tx_pkts_stat(mac_tx_ctl_stru  *pst_tx_ctl, oal_uint8 uc_tidno,
    mac_ip_header_stru *pst_ip);
extern oal_uint32 hmac_edca_opt_timeout_fn(oal_void *p_arg);

#endif   /* end of _PRE_WLAN_FEATURE_EDCA_OPT_AP */

#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif

#endif /* end of hmac_edca_opt.h */
