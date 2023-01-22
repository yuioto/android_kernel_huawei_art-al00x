
#ifndef __BOARD_H__
#define __BOARD_H__
/*****************************************************************************
  1 Include other Head file
*****************************************************************************/
#include "plat_type.h"
#include "asm/platform.h"
#include "oal_types.h"
#include "mmc_core.h"
#include "plat_board_common.h"

/*****************************************************************************
  2 Define macro
*****************************************************************************/
#define             GPIO_LOW_LEVEl                       0
#define             GPIO_HIGH_LEVEL                      1

#define             GPIO_IS_EDGE                         0
#define             GPIO_IS_LEVEL                        1

#define             GPIO_IBE_SINGLE                      0
#define             GPIO_IBE_DOUBLE                      1


#define             GPIO_IEV_NEGEDGE                     0
#define             GPIO_IEV_POSEDGE                     1

#define             GPIO_IEV_LOW_LEVEL                   0
#define             GPIO_IEV_HIGH_LEVEL                  1

#define             GPIO_IE_DISABLE                      0
#define             GPIO_IE_ENABLE                       1

#define             INVALID_IRQ                          (-1)

#ifndef OAL_OFF
#define             OAL_OFF                              0
#endif

#ifndef OAL_ON
#define             OAL_ON                                    1
#endif


#define PLAT_EXCEPTION_DEV_PANIC_LR_ADDR       0xDCBC
#define PLAT_EXCEPTION_DEV_PANIC_PC_LR_LEN     0x8

/*****************************************************************************
  3 STRUCT DEFINE
*****************************************************************************/
typedef struct {
    oal_uint32 wlan_irq;

    oal_uint32 wifi_data_intr_gpio_group;
    oal_uint32 wifi_data_intr_gpio_offset;

    oal_uint32 dev_wak_host_gpio_group;
    oal_uint32 dev_wak_host_gpio_offset;

    oal_uint32 host_wak_dev_gpio_group;
    oal_uint32 host_wak_dev_gpio_offset;

    oal_void (*wifi_power_set)(oal_uint8);
    oal_void (*wifi_rst_set)(oal_uint8);
    oal_void (*wifi_sdio_detect)(oal_void);
    oal_void (*host_pow_off)(oal_void);
    /***************************************************
        此分割线以上为提供给应用层的数据结构，不能修改
    ****************************************************/
    oal_uint32 wlan_wakeup_irq;

    /* evb or fpga verison */
    oal_int32 is_asic;
}BOARD_INFO;

/*****************************************************************************
  5 EXTERN FUNCTION
*****************************************************************************/
extern BOARD_INFO *get_board_info(void);
extern int sdio_reset_comm(struct mmc_card *card);

#endif

