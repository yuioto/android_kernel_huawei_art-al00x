
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
#define OAM_LOG_PARAM_MAX_NUM              4                 /* 可打印最多的参数个数 */
#define OAM_LOG_PRINT_DATA_LENGTH          512               /* 每次写入文件的最大长度 */
#define OAM_LOG_SDT_OUT_STRING_MSG_LEN     128

/* OAM轮替日志相关参数 */
#define LOG_NAME_LEN                       20                /* 轮替日志命名长度 */
#define LOG_OAM_PATH_LEN                   30                /* 轮替日志存储路径长度 */
#define LOG_OAM_ROTATE_SIZE                40                /* 轮替日志大小，以LOG_OAM_ROTATE_BYTE为单元 */
#define LOG_OAM_ROTATE_NUM                 4                 /* 轮替日志数目 */
#define LOG_OAM_ROTATE_BYTE                1000              /* 以byte为单位 */

#define WIFI_OAM_PREFIX                     "wifi_oam_log_"
#define TIME_TOTAL_LEN                      14
#define TIME_PKT_HEAD_LEN                   18

typedef struct {
    char puc_buf[TIME_PKT_HEAD_LEN];
} oam_write2flash_head_stru;

typedef struct _oam_sys_time_stru {
    unsigned short   tm_year;          /* 年 */
    unsigned short   tm_mon;           /* 月 */
    unsigned short   tm_day;           /* 日 */
    unsigned short   tm_hour;          /* ? */
    unsigned short   tm_min;           /* 分 */
    unsigned short   tm_sec;           /* 秒 */
    unsigned short   tm_msec;          /* 毫妙 */
} oam_sys_time_stru;

typedef union _oam_sys_time_union {
    oam_sys_time_stru   tm_time;
    unsigned char       time_data[TIME_TOTAL_LEN];
} oam_sys_time_union;

typedef enum {
    SDT_DUMP_CLOSE     =    0,       /* liteos dump 文件功能关闭 */
    SDT_DUMP_OPEN      =    1        /* liteos dump 文件功能打开 */
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

/* 文件发送结构体 */
typedef struct {
    OM_MSG_HEADER_STRU st_file_header;  /* 文件头信息 */
    oal_uint8 pucdata[FILE_PRINT_DATA_LEN]; /* 文件数据 */
} om_print_file_stru;


/* 轮替日志操作结构体 */
typedef struct oam_log_save {
    FILE*   pst_fd;                             /* 当前处理轮替日志文件fd */
    oal_int8    ac_prefix[LOG_NAME_LEN];            /* 日志文件前缀类似"wifi_log_" */
    oal_int32 st_size;               /* 每个轮替日志大小(以kb为单位) */
    oal_int32 st_num;                /* 轮替日志的数量 */
    oal_int8  st_path[LOG_OAM_PATH_LEN];               /* log config path in *.ini file */
    oal_int32   l_curr_num;                           /* log file num in curr path */
    oal_int8    ac_file[LOG_OAM_PATH_LEN + LOG_NAME_LEN + 1]; /* abs path log file */
} OAM_LOG_ROTATE;

extern EVENT_CB_S oam_event;
/* 轮替全局变量 */
extern OAM_LOG_ROTATE g_st_global ;
/* oam上报开关 */
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
