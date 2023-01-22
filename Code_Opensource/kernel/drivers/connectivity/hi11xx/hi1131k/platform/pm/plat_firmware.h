

#ifndef __PLAT_FIRMWARE_H__
#define __PLAT_FIRMWARE_H__

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#include "stdlib.h"
#elif (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "oal_net.h"
#endif
#include "exception_rst.h"
#include "oal_types.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define READ_MEG_TIMEOUT            2000     /* 2000ms */
#define READ_MEG_JUMP_TIMEOUT       100000   /* 100s */

#define FILE_CMD_WAIT_TIME_MIN      5000     /* 5000us */
#define FILE_CMD_WAIT_TIME_MAX      5100     /* 5100us */

#define VERSION_LEN                 64
#define READ_CFG_BUF_LEN            2048

#define DOWNLOAD_CMD_LEN            32
#define DOWNLOAD_CMD_PARA_LEN       100

#define HOST_DEV_TIMEOUT            3
#define INT32_STR_LEN               32

#define SHUTDOWN_TX_CMD_LEN         64

#define CMD_JUMP_EXEC_RESULT_SUCC   0
#define CMD_JUMP_EXEC_RESULT_FAIL   1

#define READ_MEG_DELAY              1

/* 以下是发往device命令的关键字 */
#define VER_CMD_KEYWORD             "VERSION"
#define JUMP_CMD_KEYWORD            "JUMP"
#define FILES_CMD_KEYWORD           "FILES"
#define RMEM_CMD_KEYWORD            "READM"
#define WMEM_CMD_KEYWORD            "WRITEM"
#define QUIT_CMD_KEYWORD            "QUIT"

/* 以下是device对命令执行成功返回的关键字，host收到一下关键字则命令执行成功 */
#define MSG_FROM_DEV_WRITEM_OK      "WRITEM OK"
#define MSG_FROM_DEV_READM_OK       ""
#define MSG_FROM_DEV_FILES_OK       "FILES OK"
#define MSG_FROM_DEV_READY_OK       "READY"
#define MSG_FROM_DEV_JUMP_OK        "JUMP OK"
#define MSG_FROM_DEV_SET_OK         "SET OK"
#define MSG_FROM_DEV_QUIT_OK        ""

/* 以下是cfg文件配置命令的参数头，一条合法的cfg命令格式为:参数头+命令关键字(QUIT命令除外) */
#define FILE_TYPE_CMD_KEY           "ADDR_FILE_"
#define NUM_TYPE_CMD_KEY            "PARA_"

#define COMPART_KEYWORD             ' '
#define CMD_LINE_SIGN               ';'
#define CFG_INFO_RESERVE_LEN        2
#define CMD_SEND_RESERVE_LEN        10

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#define OS_MEM_KFREE(p)                     free(p)
#define OS_KMALLOC_GFP(size)                memalign(32, SKB_DATA_ALIGN(size))
#define OS_VMALLOC_GFP(size)                malloc(size)
#define OS_MEM_VFREE(p)                     free(p)
#elif (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#define OS_KMALLOC_GFP(size)                oal_memalloc(size)
#endif
#define HIUSB_ALIGN_32(len)             (ALIGN((len), 32))

/* mem check */
#define DEVICE_MEM_CHECK_FLAG_ADDR  0x50000018
#define DEVICE_MEM_CHECK_FLAG_LEN   4
#define DEVICE_MEM_CHECK_RESULT_CFG "0x000f8000,1024"
#define DEVICE_MEM_CHECK_RESULT_LEN  1024
#define DEVICE_MEM_CHECK_RESULT_PATH "/data/vendor/log/wifi/memdump/readm_wifi"
#define DEVICE_MEM_CHECK_RESULT_SUCC 0
#define DEVICE_MEM_CHECK_RESULT_FAIL 1
#define DEVICE_MEM_CHECK_CFG_PATH    "/system/vendor/firmware/c01/wifi_cfg_memcheck"

/* psram_type
 * base addr  0x50000778
 * len [0-31],[0-15]有效
 * psram type 类型 包含ap 和 wb
 * 其中 bit[4:7]记录是ap还是wb  如果值为0(0000)则为ap,如果值为4(0100)则为wb
 */
#define DEVICE_EFUSE_DIEID_ADDR         0x50000740
#define DEVICE_EFUSE_DIEID_LENGTH       32
#define DEVICE_EFUSE_DIEID_NUMS         16

// firmware read retry time
#define FIRMWARE_READ_TETRY_TIME        3

/*****************************************************************************
  3 枚举定义
*****************************************************************************/


enum FIRMWARE_CFG_CMD_ENUM {
    ERROR_TYPE_CMD = 0,            /* 错误的命令 */
    FILE_TYPE_CMD,                 /* 下载文件的命令 */
    NUM_TYPE_CMD,                  /* 下载配置参数的命令 */
    QUIT_TYPE_CMD,                 /* 退出命令 */
    SHUTDOWN_WIFI_TYPE_CMD,        /* SHUTDOWN WCPU命令 */
};


enum FIRMWARE_CFG_FILE_ENUM {
    WIFI_CFG = 0,
    RAM_REG_TEST_CFG,
    CFG_FILE_TOTAL
};

enum DEV_SOFT_VER_TYPE {
    SOFT_VER_CO1 = 0,
    SOFT_VER_CO2,
    SOFT_VER_BUTT
};


/*****************************************************************************
  3 STRUCT DEFINE
*****************************************************************************/


/*****************************************************************************
  4 全局变量定义
*****************************************************************************/


/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/
typedef struct cmd_type_st {
    oal_int32       cmd_type;
    oal_uint8      cmd_name[DOWNLOAD_CMD_LEN];
    oal_uint8       cmd_para[DOWNLOAD_CMD_PARA_LEN];
}CMD_TYPE_STRUCT;

typedef struct firmware_globals_st {
    oal_int32            al_count[CFG_FILE_TOTAL];      /* 存储每个cfg文件解析后有效的命令个数 */
    CMD_TYPE_STRUCT  *apst_cmd[CFG_FILE_TOTAL];         /* 存储每个cfg文件的有效命令 */
    oal_uint8            auc_CfgVersion[VERSION_LEN];   /* 存储cfg文件中配置的版本号信息 */
    oal_uint8            auc_DevVersion[VERSION_LEN];   /* 存储加载时device侧上报的版本号信息 */
}FIRMWARE_GLOBALS_STRUCT;

typedef struct _firmware_mem {
    /* 保存firmware file内容的buffer，先将文件读到这个buffer中，然后从这个向device buffer发送 */
    oal_uint8  *pucDataBuf;
    /* pucDataBuf的长度 */
    oal_uint32 ulDataBufLen;

#define CMD_BUFF_LEN       256
    oal_uint8  *puc_recv_cmd_buff;
    oal_uint8  *puc_send_cmd_buff;
}FIRMWARE_MEM;

typedef struct _efuse_info_st_ {
    oal_uint32   chip_id:8;
    oal_uint32   chip_ver:2;
    oal_uint32   soft_ver:2;
    oal_uint32   host_ver:2;
    oal_uint32   resv_b2:2;
    oal_uint32   chip_ver_butt:16;

    oal_uint32   mac_h:16;
    oal_uint32   mac_h_butt:16;

    oal_uint32   mac_m:16;
    oal_uint32   mac_m_butt:16;

    oal_uint32   mac_l:16;
    oal_uint32   mac_l_butt:16;
}efuse_info_stru;

typedef struct _firmware_file_st_ {
    oal_uint8 *addr;
    oal_uint32 len;
}firmware_file_stru;

#define DECLARE_FIRMWARE_FILE(filename)    \
    static firmware_file_stru  firmware_file_##filename = { \
        .addr = firmware_array_##filename,           \
        .len  = sizeof(firmware_array_##filename),     \
    }

/* mem check err info */
struct mem_check_error_info_st {
    uint32_t resverd;
    uint32_t err_addr;
    uint32_t err_data;
    uint32_t exp_data;
};
/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/

/*****************************************************************************
  10 函数声明
*****************************************************************************/

extern oal_int32 firmware_download(oal_uint32 ul_index);
extern oal_int32 plat_firmware_init(void);
extern oal_int32 plat_firmware_clear(void);
extern oal_int32 firmware_read_efuse_info(oal_void);
extern efuse_info_stru *get_efuse_info_handler(oal_void);
extern oal_uint32 get_device_soft_version(oal_void);
extern int32_t device_mem_check(void);
extern void device_mem_check_set_result(uint32_t data);
extern struct pm_drv_data *pm_get_drvdata(void);
extern int32_t firmware_read_efuse_die_id(oal_void);
extern int32_t device_mem_dump_etc(p_st_wifi_demp_mem_info *pst_mem_dump_info);
extern int32_t firmware_mem_init(void);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of plat_firmware.h */
