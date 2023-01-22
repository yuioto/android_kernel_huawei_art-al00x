

#ifndef __WAL_MAIN_H__
#define __WAL_MAIN_H__

#include "oal_ext_if.h"
#include "oam_ext_if.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_MAIN_H

#ifdef _PRE_WLAN_DFT_EVENT
#define WAL_EVENT_WID(_puc_macaddr, _uc_vap_id, en_event_type, _puc_string) \
    oam_event_report(_puc_macaddr, _uc_vap_id, OAM_MODULE_ID_WAL, en_event_type, _puc_string)
#else
#define WAL_EVENT_WID(_puc_macaddr, _uc_vap_id, en_event_type, _puc_string)
#endif

/* HOST CRX事件子类型 */
typedef enum {
    WAL_HOST_CRX_SUBTYPE_CFG,
    WAL_HOST_CRX_SUBTYPE_RESET_HW,

    WAL_HOST_CRX_SUBTYPE_BUTT
} wal_host_crx_subtype_enum;
typedef oal_uint8 wal_host_crx_subtype_enum_uint8;

/* HOST DRX事件子类型 */
typedef enum {
    WAL_HOST_DRX_SUBTYPE_TX,

    WAL_HOST_DRX_SUBTYPE_BUTT
} wal_host_drx_subtype_enum;
typedef oal_uint8 wal_host_drx_subtype_enum_uint8;

extern oam_wal_func_hook_stru g_st_wal_drv_func_hook;

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
typedef struct {
    oal_semaphore_stru       st_txdata_sema;
    oal_kthread_stru        *pst_txdata_thread;
    oal_wait_queue_head_stru st_txdata_wq;
    oal_netbuf_head_stru     st_txdata_netbuf_head;
    oal_uint32               ul_pkt_loss_cnt;
    oal_bool_enum_uint8      en_txthread_enable;
}wal_txdata_thread_stru;

#define WAL_TXDATA_THERAD_PRIORITY 6

#endif

extern oal_int32 wal_main_init(oal_void);
extern oal_void wal_main_exit(oal_void);
extern oal_bool_enum_uint8 wal_get_txdata_thread_enable(oal_void);
extern oal_wakelock_stru g_st_wal_wakelock;
#define wal_wake_lock() oal_wake_lock(&g_st_wal_wakelock)
#define wal_wake_unlock() oal_wake_unlock(&g_st_wal_wakelock)
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of wal_main */
