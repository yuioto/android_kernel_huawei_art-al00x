

#ifndef __OAL_SDIO_COMM_H__
#define __OAL_SDIO_COMM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#undef CONFIG_SDIO_DEBUG
#define CONFIG_SDIO_FUNC_EXTEND

#define CONFIG_SDIO_MSG_FLOWCTRL

#undef CONFIG_CREDIT_MSG_FLOW_CTRL_DEBUG

#undef CONFIG_SDIO_REINIT_SUPPORT

/* SDIO传输时，将所有的SG LIST 合并成一块完整的内存发送!
   CONFIG_HISDIO_H2D_SCATT_LIST_ASSEMBLE在k3和3798平台打开，在3516 liteos和3516 linux关闭
*/
/* 定义此宏表示打开arm to host 的消息回读ACK，
  消息是从ARM发送至host,
  必须读高字节才能产生ACK */
#define CONFIG_SDIO_D2H_MSG_ACK

/* 定义此宏表示打开host to arm 的消息回读ACK，
  消息是从Host发送至ARM,
  必须读高字节才能产生ACK */
#undef CONFIG_SDIO_H2D_MSG_ACK
/* Device to host sdio message type */
typedef enum _HISDIO_D2H_MSG_TYPE_ {
    D2H_MSG_WLAN_READY = 0U,
    D2H_MSG_WOW_COMPLETE = 1U, /* For the credit flow ctrl */
    D2H_MSG_FLOWCTRL_OFF = 2U, /* can't send data */
    D2H_MSG_FLOWCTRL_ON = 3U, /* can send data */
    D2H_MSG_WAKEUP_SUCC = 4U, /* Wakeup done */
    D2H_MSG_ALLOW_SLEEP = 5U, /* ALLOW Sleep */
    D2H_MSG_DISALLOW_SLEEP = 6U, /* DISALLOW Sleep */
    D2H_MSG_DEVICE_PANIC = 7U, /* arm abort */
    D2H_MSG_POWEROFF_ACK = 8U, /* Poweroff cmd ack */
    D2H_MSG_OPEN_BCPU_ACK = 9U, /* OPEN BCPU cmd ack */
    D2H_MSG_CLOSE_BCPU_ACK = 10U, /* CLOSE BCPU cmd ack */
    D2H_MSG_CREDIT_UPDATE = 11U,  /* update high priority buffer credit value */
    D2H_MSG_HIGH_PKT_LOSS = 12U,  /* high pri pkts loss count */
    D2H_MSG_HALT_BCPU = 13U,      /* halt bcpu ack */
    D2H_MSG_HEARTBEAT = 14U,     /* send heartbeat */
    D2H_MSG_COUNT = 16U /* max support msg count */
} HISDIO_D2H_MSG_TYPE;


/* Host to device sdio message type */
typedef enum _HISDIO_H2D_MSG_TYPE_ {
    H2D_MSG_FLOWCTRL_ON         = 0, /* can send data, force to open */
    H2D_MSG_DEVICE_INFO_DUMP    = 1,
    H2D_MSG_DEVICE_MEM_DUMP     = 2,
    H2D_MSG_TEST                = 3,
    H2D_MSG_PM_WLAN_OFF         = 4,
    H2D_MSG_SLEEP_REQ           = 5,
    H2D_MSG_PM_DEBUG            = 6,

    H2D_MSG_RESET_BCPU          = 7,
    H2D_MSG_QUERY_RF_TEMP       = 8,

    H2D_MSG_HCC_SLAVE_THRUPUT_BYPASS = 9,
    H2D_MSG_DEVICE_MEM_INFO          = 10,
    H2D_MSG_STOP_SDIO_TEST      = 11,
    H2D_MSG_PM_BCPU_OFF         = 12,
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    H2D_MSG_FORCESLP_REQ        = 13,
#elif (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    H2D_MSG_HALT_BCPU           = 13,
#endif

    H2D_MSG_COUNT = 16 /* max support msg count */
} HISDIO_H2D_MSG_TYPE;

/* sdio flow control info, free cnt */
#define HISDIO_SHORT_PKT_SET(reg, num)    do { reg = (((reg) & 0xFFFFFF00) | (((num) & 0xFF))); }while (0)
#define HISDIO_LARGE_PKT_SET(reg, num)    do { reg = (((reg) & 0xFFFF00FF) | (((num) & 0xFF) << 8)); }while (0)
#define HISDIO_RESERVE_PKT_SET(reg, num)  do { reg = (((reg) & 0xFF00FFFF) | (((num) & 0xFF) << 16)); }while (0)
#define HISDIO_COMM_REG_SEQ_SET(reg, num) do { reg = (((reg) & 0x00FFFFFF) | (((num) & 0xFF) << 24)); }while (0)

#define HISDIO_SHORT_PKT_GET(reg)           ((reg) & 0xFF)
#define HISDIO_LARGE_PKT_GET(reg)           (((reg) >> 8) & 0xFF)
#define HISDIO_RESERVE_PKT_GET(reg)         (((reg) >> 16) & 0xFF)
#define HISDIO_COMM_REG_SEQ_GET(reg)        (((reg) >> 24) & 0xFF)


#define HISDIO_BLOCK_SIZE   512U         /* one size of data transfer block
                                              size, * 64, 128, 256, 512, 1024 */
/* The max scatter buffers when host to device */
#define HISDIO_HOST2DEV_SCATT_MAX       64
#define HISDIO_HOST2DEV_SCATT_SIZE      64U

/* The max scatter buffers when device to host */
#define HISDIO_DEV2HOST_SCATT_MAX       64
#define HISDIO_DEV2HOST_SCATT_SIZE      64

/* The max scatt num of rx and tx */
#define HISDIO_SCATT_MAX_NUM    (HISDIO_DEV2HOST_SCATT_MAX)

/* 64B used to store the scatt info,1B means 1 pkt. */
#define HISDIO_H2D_SCATT_BUFFLEN_ALIGN_BITS     3
/* 1 << 5 */
/* Host to device's descr align length depends on the
   CONFIG_HISDIO_H2D_SCATT_LIST_ASSEMBLE */
#ifdef CONFIG_HISDIO_H2D_SCATT_LIST_ASSEMBLE
#define HISDIO_H2D_SCATT_BUFFLEN_ALIGN    8U
#else
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION) // 为了保证liteos性能，长度设为8字节对齐
#define HISDIO_H2D_SCATT_BUFFLEN_ALIGN    8U
#else
#define HISDIO_H2D_SCATT_BUFFLEN_ALIGN    32U
#endif
#endif

/* Device To Host,descr just request 4 bytes aligned,
  but 10 bits round [0~1023], so we also aligned to 32 bytes */
#define HISDIO_D2H_SCATT_BUFFLEN_ALIGN_BITS     5
/* 1 << 5 */
#define HISDIO_D2H_SCATT_BUFFLEN_ALIGN    512


#define HSDIO_HOST2DEV_PKTS_MAX_LEN    1544U
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif

