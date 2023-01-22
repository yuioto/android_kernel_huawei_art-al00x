
#ifndef __OAM_MISC_H__
#define __OAM_MISC_H__

#include "oal_ext_if.h"
#include "los_typedef.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAM_MISC_H

#define OM_FRAME_DELIMITER                 0x7E
#define OM_FRAME_DELIMITER_LENGTH          1
#define OM_FRAME_HEADER_LEN                8
#define OM_FILE_HEADER_LEN                 OAL_SIZEOF(OM_MSG_HEADER_STRU)
#define FILE_PRINT_DATA_LEN                512
#define FILE_FLAG_NUM                      2
#define OAM_LOG_PARAM_MAX_NUM              4                 /* �ɴ�ӡ���Ĳ������� */
#define OAM_LOG_PRINT_DATA_LENGTH          512               /* ÿ��д���ļ�����󳤶� */
#define OAM_LOG_SDT_OUT_STRING_MSG_LEN     128

/* OAM������־��ز��� */
#define LOG_NAME_LEN                       20                /* ������־�������� */
#define LOG_OAM_PATH_LEN                   30                /* ������־�洢·������ */
#define LOG_OAM_ROTATE_SIZE                40                /* ������־��С����LOG_OAM_ROTATE_BYTEΪ��Ԫ */
#define LOG_OAM_ROTATE_NUM                 4                 /* ������־��Ŀ */
#define LOG_OAM_ROTATE_BYTE                1000              /* ��byteΪ��λ */

#define WIFI_OAM_PREFIX                     "wifi_oam_log_"
#define TIME_TOTAL_LEN                      14
#define TIME_PKT_HEAD_LEN                   18

typedef struct {
    char puc_buf[TIME_PKT_HEAD_LEN];
} oam_write2flash_head_stru;

typedef struct _oam_sys_time_stru {
    unsigned short   tm_year;          /* �� */
    unsigned short   tm_mon;           /* �� */
    unsigned short   tm_day;           /* �� */
    unsigned short   tm_hour;          /* ? */
    unsigned short   tm_min;           /* �� */
    unsigned short   tm_sec;           /* �� */
    unsigned short   tm_msec;          /* ���� */
} oam_sys_time_stru;

typedef union _oam_sys_time_union {
    oam_sys_time_stru   tm_time;
    unsigned char       time_data[TIME_TOTAL_LEN];
} oam_sys_time_union;

typedef enum {
    SDT_DUMP_CLOSE     =    0,       /* liteos dump �ļ����ܹر� */
    SDT_DUMP_OPEN      =    1        /* liteos dump �ļ����ܴ� */
} SDT_DUMP_SWITCH;

typedef struct oam_rx_sdt_cmd {
    int (*uart_rx_sdt_cmd_dispose)(char* ch, unsigned int ch_cnt);
} oam_rx_sdt_cmd_func_hook_stru;

#define OAM_WIFI_LOG_PARA_PRESS(vap_id, feature_id, fileid, lev)   (((fileid)&0xFFFF) | (((feature_id)&0xFF) << 16) | (((vap_id)&0xF) << 24) | (((lev)&0xF) << 28))

typedef struct {
    UINT8        ucFrameStart;
    UINT8        ucFuncType;
    UINT8        ucPrimeId;
    UINT8        aucReserver[1];
    UINT16       usFrameLen;
    UINT16       usSN;
} OM_MSG_HEADER_STRU;

/* �ļ����ͽṹ�� */
typedef struct {
    OM_MSG_HEADER_STRU st_file_header;  /* �ļ�ͷ��Ϣ */
    oal_uint8 pucdata[FILE_PRINT_DATA_LEN]; /* �ļ����� */
} om_print_file_stru;


/* ������־�����ṹ�� */
typedef struct oam_log_save {
    FILE*   pst_fd;                             /* ��ǰ����������־�ļ�fd */
    oal_int8    ac_prefix[LOG_NAME_LEN];            /* ��־�ļ�ǰ׺����"wifi_log_" */
    oal_int32 st_size;               /* ÿ��������־��С(��kbΪ��λ) */
    oal_int32 st_num;                /* ������־������ */
    oal_int8  st_path[LOG_OAM_PATH_LEN];               /* log config path in *.ini file */
    oal_int32   l_curr_num;                           /* log file num in curr path */
    oal_int8    ac_file[LOG_OAM_PATH_LEN + LOG_NAME_LEN + 1]; /* abs path log file */
} OAM_LOG_ROTATE;

extern EVENT_CB_S oam_event;
/* ����ȫ�ֱ��� */
extern OAM_LOG_ROTATE g_st_global ;
/* oam�ϱ����� */
extern oal_uint16                       g_us_print2sdt_ctl;
extern oam_rx_sdt_cmd_func_hook_stru g_oam_uart_rx_sdt_cmd_func_hook;
extern oal_uint8 g_uc_uart_ctrl;
extern oal_uint32 g_ul_file_count;
extern void oam_printf_wpa(int level, char* pc_fmt, ...);
extern oal_uint32 oam_dump_file_ack(void);
extern void oam_uart_rx_sdt_cmd_init(void);
extern int oam_init_rotate_log(OAM_LOG_ROTATE *pst_log, int pst_size, int pst_num, char *pc_dir_path, char* pc_prefix);
extern oal_uint32 oam_task_init(oal_void);
extern FILE* g_pst_fopen_fd;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oam_statistics.h */
