/*
 * ��Ȩ���� (c) ��Ϊ�������޹�˾ 2001-2020
 */


/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include <linux/slab.h>
#include "heartbeat_host.h"
#include "plat_debug.h"
#include "plat_pm.h"
#include "oal_sdio_host_if.h"
#include "exception_rst.h"
#include "oal_timer.h"
#include "oam_ext_if.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HEARTBEAT_HOST_C

/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/
#define BEAT_TIME_MESC             5000      /* ������ʱʱ��Ϊ2���� */

typedef struct _heart_beat {
    struct timer_list timer;

#define HEARTBEATEN     1
#define HEARTBEATDIS    0
    oal_int32 heartbeat_en;
}heart_beat_stru;

heart_beat_stru *g_pst_heartbeat = NULL;

/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/
void set_heartbeat_cfg(oal_int32 cfg)
{
    if (g_pst_heartbeat == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_HEARTBEAT, "g_pst_heartbeat is  NULL\r\n");
        return;
    }
    g_pst_heartbeat->heartbeat_en = cfg ? HEARTBEATEN : HEARTBEATDIS;
}

oal_int32 start_heartbeat(void)
{
    /* usb�͹��ĳ�����ͨ�����µ磬Ϊ��ʡ���ģ��ر�heartbeat */
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)

    if (g_pst_heartbeat == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_HEARTBEAT, "g_pst_heartbeat is  NULL\r\n");
        return -EINVAL;
    }

    oal_timer_start(&(g_pst_heartbeat->timer), BEAT_TIME_MESC);
    g_pst_heartbeat->heartbeat_en = HEARTBEATEN;
#endif
    return OAL_SUCC;
}

oal_int32 stop_heartbeat (void)
{
    /* usb�͹��ĳ�����ͨ�����µ磬Ϊ��ʡ���ģ��ر�heartbeat */
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)

    if (g_pst_heartbeat == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_HEARTBEAT, "g_pst_heartbeat is  NULL\r\n");
        return -EINVAL;
    }

    g_pst_heartbeat->heartbeat_en = HEARTBEATDIS;
    oal_timer_delete_sync(&(g_pst_heartbeat->timer));
#endif
    return OAL_SUCC;
}

oal_int32 update_heartbeat(void)
{
    /* usb�͹��ĳ�����ͨ�����µ磬Ϊ��ʡ���ģ��ر�heartbeat */
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    if ((g_pst_heartbeat == NULL) || (g_pst_heartbeat->heartbeat_en == HEARTBEATDIS)) {
        return -EINVAL;
    }

    oal_timer_start(&(g_pst_heartbeat->timer), BEAT_TIME_MESC);
#endif
    return OAL_SUCC;
}

/*
 * ��������   : ������ʱ���������ú������������ж��������У�����������˯�ߵĲ���
 * �޸���ʷ   :
 *   �޸����� : �����ɺ���
 */
static void heartbeat_expire_func(oal_ulong data)
{
    OAL_REFERENCE(data);

    if (g_pst_heartbeat == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_HEARTBEAT, "g_pst_heartbeat is NULL\r\n");
        return;
    }

    if (g_pst_heartbeat->heartbeat_en == HEARTBEATDIS) {
        OAM_WARNING_LOG0(0, OAM_SF_HEARTBEAT, "g_pst_heartbeat->heartbeat_en disable\r\n");
        update_heartbeat();
        return;
    }

    OAM_ERROR_LOG0(0, OAM_SF_HEARTBEAT, "enter heartbeat_expire_func\r\n");
    //  todo...    heartbeat exception
    //  in soft interrupt, cannot sleep
    oal_exception_submit(TRANS_FAIL);

    return;
}


oal_int32 heart_beat_init(void)
{
    if (g_pst_heartbeat != NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_HEARTBEAT, "g_pst_heartbeat is not NULL\r\n");
        return -EINVAL;
    }
    g_pst_heartbeat = kzalloc(sizeof(heart_beat_stru), GFP_KERNEL);
    if (g_pst_heartbeat == NULL) {
        OAM_INFO_LOG0(0, OAM_SF_HEARTBEAT, "malloc fail, g_pst_heartbeat is NULL\r\n");
        return -ENOMEM;
    }
    oal_timer_init(&(g_pst_heartbeat->timer), BEAT_TIME_MESC, heartbeat_expire_func, 0);

    g_pst_heartbeat->heartbeat_en = HEARTBEATDIS;
    return OAL_SUCC;
}

void heart_beat_release(void)
{
    if (g_pst_heartbeat == NULL) {
        OAM_INFO_LOG0(0, OAM_SF_HEARTBEAT, "g_pst_heartbeat is  NULL\r\n");
        return;
    }
    stop_heartbeat();
    kfree(g_pst_heartbeat);
    g_pst_heartbeat = NULL;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

