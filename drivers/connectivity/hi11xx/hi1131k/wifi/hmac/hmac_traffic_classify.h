

#ifndef __HMAC_TRAFFIC_CLASSIFY__
#define __HMAC_TRAFFIC_CLASSIFY__

#ifdef _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN

// 1ͷ�ļ�����
#include "oal_profiling.h"
#include "oal_net.h"
#include "oal_ext_if.h"
#include "frw_ext_if.h"
#include "hmac_tx_data.h"
#include "hmac_tx_amsdu.h"
#include "mac_frame.h"
#include "mac_data.h"
#include "hmac_frag.h"
#include "hmac_11i.h"
#include "hmac_user.h"

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
#include "hmac_ext_if.h"
#endif
#ifdef _PRE_WLAN_FEATURE_MCAST /* �鲥ת���� */
#include "hmac_m2u.h"
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
#include "hmac_proxy_arp.h"
#endif

#include "hmac_device.h"
#include "hmac_resource.h"

#include "hmac_tcp_opt.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_TRAFFIC_CLASSIFY_H

/*
 * �û��ṹ��: ��������ʶ��ҵ�񡢴�ʶ��ҵ������
 * �������hmac_user_stru����hmac_user_stru�ṹ�������Ӻ궨���ֶ�:
 * _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN
 */
// RTPͷ�ṹ��:�ṹ����û�а�������CSRC��ʶ��
typedef struct {
    /* -----------------------------------------------------------------------------
     *                               RTPͷ�ṹ
     * -----------------------------------------------------------------------------
     * |version|P|X|   CSRC��   |M|          PT           |       ���             |
     * |  2bit |1|1|    4bit    |1|        7bit           |         16bit          |
     * -----------------------------------------------------------------------------
     * |                               ʱ��� 32bit                                |
     * -----------------------------------------------------------------------------
     * |                                 SSRC 32bit                                |
     * -----------------------------------------------------------------------------
     * |               CSRC ÿ��CSRC��ʶ��32bit ��ʶ��������CSRC������             |
     * ---------------------------------------------------------------------------
     */
    oal_uint8  uc_version_and_CSRC;    /* �汾��2bit�����λ(P)1bit����չλ(X)1bit��CSRC��Ŀ4bit */
    oal_uint8  uc_payload_type;        /* ���1bit����Ч�غ�����(PT)7bit */
    oal_uint16 us_RTP_idx;             /* RTP������� */
    oal_uint32 ul_RTP_time_stamp;      /* ʱ��� */
    oal_uint32 ul_SSRC;                /* SSRC */
}hmac_tx_rtp_hdr;
// 3 ��������
extern oal_void hmac_tx_traffic_classify(mac_tx_ctl_stru *pst_tx_ctl, mac_ip_header_stru *pst_ip, oal_uint8 *puc_tid);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* endif _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN */

#endif  /* end of hmac_traffic_classify.h */
