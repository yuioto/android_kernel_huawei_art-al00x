

#ifndef __WAL_LINUX_ATCMDSRV_H__
#define __WAL_LINUX_ATCMDSRV_H__

// 1 其他头文件包含
#include "oal_ext_if.h"
#include "wlan_types.h"
#include "wlan_spec.h"
#include "mac_vap.h"
#include "hmac_ext_if.h"
#include "wal_ext_if.h"
#include "wal_config.h"
#include "wal_linux_ioctl.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_LINUX_ATCMDSRV_H
// 2 宏定义
#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST))
/* atcmdsrv私有命令宏定义 */
#define WAL_ATCMDSRV_IOCTL_VERIFY_CODE           1102
#define WAL_ATCMDSRV_IOCTL_VERIFY_CODE_1131      80211 /* common_service modify verify code to 80211 */
#define WAL_ATCMDSRV_IOCTL_MAC_LEN               6
#define WAL_ATCMDSRV_IOCTL_DBB_LEN               256
#define WAL_ATCMDSRV_NV_WINVRAM_LENGTH           104  /* WINVRAM内存空间 */
#define WAL_ATCMDSRV_DIE_ID_LENGTH               16
#define WAL_ATCMDSRV_IOCTL_COUNTRY_LEN           3
#define WAL_ATCMDSRV_MAC_DBB_NUM                 0x3115040c
#define WAL_ATCMDSRV_GET_HEX_CHAR(tmp)           (((tmp) > 9) ? ((tmp - 10) + 'A') : ((tmp) + '0'))

/* WINVRAM内存空间 */
#define WAL_ATCMDSRB_DBB_NUM_TIME                (5 * OAL_TIME_HZ)
#define WAL_ATCMDSRB_CHECK_FEM_PA                (5 * OAL_TIME_HZ)
#define WAL_ATCMDSRB_GET_RX_PCKT                 (5 * OAL_TIME_HZ)
#define WLAN_HT_ONLY_MODE_2G                     WLAN_HT_ONLY_MODE + 3
#define WLAN_VHT_ONLY_MODE_2G                    WLAN_VHT_ONLY_MODE + 3

#define MAC_PARA_LEN                             3
#define MAC_PARA_NUM                             6
#define HISI_CMD_MAX_LEN                         40
#define MAC_REG_INFO                             20

#define MAC_EFUSE_CNT                            3
#define MAC_EFUSE_L                              0x50000728
#define MAC_EFUSE_M                              0x5000072C
#define MAC_EFUSE_H                              0x50000730

#define CLOSE_ALWYAS_TX                          0
#define OPEN_ALWYAS_TX                           1

#define BIT_OFFSET_0                             0
#define BIT_OFFSET_4                             4
#define BIT_OFFSET_8                             8
#define BIT_OFFSET_12                            12
#define BIT_OFFSET_16                            16
#define BIT_OFFSET_20                            20
#define BIT_OFFSET_24                            24
#define BIT_OFFSET_28                            28
#define SEC2MSEC                                 1000
// 3 枚举定义
/* atcmdsrv枚举类型 */
enum WAL_ATCMDSRV_IOCTL_CMD {
    WAL_ATCMDSRV_IOCTL_CMD_WI_FREQ_SET = 0, /* set wifi freq */
    WAL_ATCMDSRV_IOCTL_CMD_WI_POWER_SET,    /* set  power */
    WAL_ATCMDSRV_IOCTL_CMD_MODE_SET,        /* set burst tx/rx mode */
    WAL_ATCMDSRV_IOCTL_CMD_DATARATE_SET,
    WAL_ATCMDSRV_IOCTL_CMD_BAND_SET,
    WAL_ATCMDSRV_IOCTL_CMD_ALWAYS_TX_SET,
    WAL_ATCMDSRV_IOCTL_CMD_DBB_GET,
    WAL_ATCMDSRV_IOCTL_CMD_HW_STATUS_GET, /* hardware status */
    WAL_ATCMDSRV_IOCTL_CMD_ALWAYS_RX_SET, /* always rx set */
    WAL_ATCMDSRV_IOCTL_CMD_HW_ADDR_SET,   /* set mac addr */
    WAL_ATCMDSRV_IOCTL_CMD_RX_PKCG_GET,   /* get rx pkcg */
    WAL_ATCMDSRV_IOCTL_CMD_PM_SWITCH_SET, /* low power switch set */
    WAL_ATCMDSRV_IOCTL_CMD_RX_RSSI_GET,   /* rx rssi get */
    WAL_ATCMDSRV_IOCTL_CMD_CHIPCHECK_SET,
    WAL_ATCMDSRV_IOCTL_CMD_CHIPCHECK_RESULT,
    WAL_ATCMDSRV_IOCTL_CMD_CHIPCHECK_TIME,
    WAL_ATCMDSRV_IOCTL_CMD_UART_LOOP_SET,
    WAL_ATCMDSRV_IOCTL_CMD_SDIO_LOOP_SET,
    WAL_ATCMDSRV_IOCTL_CMD_RD_CALDATA,    /* read caldata from dts */
    WAL_ATCMDSRV_IOCTL_CMD_SET_CALDATA,
    WAL_ATCMDSRV_IOCTL_CMD_GET_EFUSE_RESULT, /* read caldata from dts */
    WAL_ATCMDSRV_IOCTL_CMD_SET_ANT,
    WAL_ATCMDSRV_IOCTL_CMD_DIEID_INFORM,  /* read die_id form device */
    WAL_ATCMDSRV_IOCTL_CMD_SET_COUNTRY,
    WAL_ATCMDSRV_IOCTL_CMD_GET_UPCCODE,
    WAL_ATCMDSRV_IOCTL_CMD_SET_CONN_POWER,     /* 设置最大发送功率 */
    WAL_ATCMDSRV_IOCTL_CMD_SET_BSS_EXPIRE_AGE, /* 设置扫描结果老化时间 */
    WAL_ATCMDSRV_IOCTL_CMD_GET_CONN_INFO,      /* 获取连接信息 */

    WAL_ATCMDSRV_IOCTL_CMD_TEST_BUTT
};

typedef enum {
    ATCMDSRV_WIFI_DISCONNECT,
    ATCMDSRV_WIFI_CONNECTED,
} atcmdsrv_wifi_conn_info_enum;
typedef oal_uint8 atcmdsrv_wifi_conn_info_enum_uint8;

// 7 STRUCT定义
struct wal_atcmdsrv_wifi_connect_info {
    atcmdsrv_wifi_conn_info_enum_uint8 en_status;
    oal_uint8 auc_ssid[WLAN_SSID_MAX_LEN];
    oal_uint8 auc_bssid[WLAN_MAC_ADDR_LEN];
    oal_int8 c_rssi;
};

/* 1102 使用atcmdsrv 下发命令 */
typedef struct wal_atcmdsrv_wifi_priv_cmd {
    /* 校验位,取值1102,与其他平台区别开来 */
    int32_t   l_verify;
    int32_t   l_cmd;                                          /* 命令号 */

    union {
        int32_t freq;
        int32_t userpow;
        int32_t pow;
        int32_t mode;
        int32_t datarate;
        int32_t bandwidth;
        int32_t always_tx;
        int32_t fem_pa_status;
        int32_t always_rx;
        int32_t rx_pkcg;
        int32_t pm_switch;
        int32_t rx_rssi;
        int32_t chipcheck_result;
        oal_uint64 chicheck_time;
        int32_t uart_loop_set;
        int32_t sdio_loop_set;
        int32_t efuse_status;
        int32_t ant;
        int32_t upc_code;
        int32_t pin_status;
        int32_t pmu_status;
        int32_t mimo_channel;
        int8_t hw_mac[WAL_ATCMDSRV_IOCTL_MAC_LEN];        /* wifi MAC地址，需要将字符串转换为数组 */
        int8_t dbb[WAL_ATCMDSRV_IOCTL_DBB_LEN];               /* 256 */
        uint8_t caldata[WAL_ATCMDSRV_NV_WINVRAM_LENGTH];      /* 104 */
        uint16_t die_id[WAL_ATCMDSRV_DIE_ID_LENGTH];          /* 16 */
        int8_t countrycode[WAL_ATCMDSRV_IOCTL_COUNTRY_LEN];   /* 3 */
        uint32_t bss_expire_age;                              /* 产线扫描结果老化时间 */
        struct wal_atcmdsrv_wifi_connect_info wifi_connect_info;
    } pri_data;
}wal_atcmdsrv_wifi_priv_cmd_stru;

// 10 函数声明
extern int32_t wal_atcmdsrv_wifi_priv_cmd(oal_net_device_stru *net_dev, oal_ifreq_stru *ifr, int32_t cmd);
extern uint32_t device_mem_check_get_result(void);
extern uint8_t *get_die_id_info_handler(uint32_t *len);
extern int32_t  device_mem_check(void);
extern int32_t  conn_test_sdio_loop(char *param);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of wal_linux_ioctl.h */

