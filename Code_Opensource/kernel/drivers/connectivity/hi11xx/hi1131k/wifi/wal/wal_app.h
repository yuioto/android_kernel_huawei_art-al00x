
#ifndef __WAL_APP_H__
#define __WAL_APP_H__

#include "oal_types.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


// 3 ö�ٶ���
typedef enum {
    WAL_MONITOR_SWITCH_OFF,
    WAL_MONITOR_SWITCH_ON,
    WAL_MONITOR_SWITCH_BUTT
} wal_monitor_switch_enum;

typedef oal_uint8 wal_monitor_switch_enum_uint8;

typedef enum {
    WAL_CMD_SET_MONITOR,
    WAL_CMD_SET_CHANNEL,
    WAL_CMD_BUTT
} wal_cmd_type;
typedef oal_uint8 wal_cmd_type_uint8;

typedef enum {
    WAL_MSG_UNSPEC,
    WAL_MSG_FRAME,
    WAL_MSG_BUTT
} wal_msg_type;

typedef enum {
    WAL_OPS_UNSPEC,
    WAL_OPS_HISILINK, // ����hisilink����Ϣ
    WAL_OPS_WPA,      // ����wpa_supplicant����Ϣ
    WAL_OPS_BUTT
} wal_ops_enum;
typedef enum {
    WAL_ATTR_UNSPEC,
    WAL_ATTR_MSG,
    WAL_ATTR_BUTT
} wal_attr_enum;

// 7 STRUCT����
typedef struct {
    oal_uint8 auc_netdev_name[8]; // netdev������
    oal_uint8 uc_msg_type;        // ��Ϣ����
    oal_uint8 uc_ack_flag;        // �Ƿ���Ҫ��ACK
    oal_uint8 auc_res[2];
    oal_uint32 ul_data_len; // ���ݲ��ֵĳ���
} wal_app_msg_hdr;

typedef struct {
    oal_uint8 en_monitor_mode;
    oal_int8 c_rssi_level;
    oal_uint8 auc_res[2];
} wal_monitor_info_stru;

#define WAL_APP_MSG_DATA_MAX_LEN 4
typedef struct {
    wal_app_msg_hdr st_msg_hdr;
    oal_uint8 auc_data[4]; // ����ָ�����ݲ����ײ�
} wal_app_msg;

typedef struct {
    oal_uint32 ul_len;     // ���ݰ��ĳ���
    oal_uint8 auc_data[4]; // ����ָ�����ݰ����ײ�
} wal_frame_info;

// 10 ��������
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_int32 wal_genetlink_init(void);
void wal_genetlink_exit(void);
#endif
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
