
#ifndef __BOARD_H__
#define __BOARD_H__
/*****************************************************************************
  1 Include other Head file
*****************************************************************************/
#include "plat_type.h"

/*****************************************************************************
  2 Define macro
*****************************************************************************/
#define BOARD_SUCC                           0
#define BOARD_FAIL                           (-1)

#define GPIO_LOWLEVEL                        0
#define GPIO_HIGHLEVEL                       1

#define NO_NEED_POWER_PREPARE                0
#define NEED_POWER_PREPARE                   1

#define GPIO_HOST_TO_DEVICE                  222
#define GPIO_PMU_PWRON                       24
#define GPIO_WL_RST_N                        29

#define GPIO_SDIO_CLK                        128
#define GPIO_SDIO_CMD                        129
#define GPIO_SDIO_DAT0                       130
#define GPIO_SDIO_DAT1                       131
#define GPIO_SDIO_DAT2                       132
#define GPIO_SDIO_DAT3                       133

#define IOCFG_BASE_GPIO_FOR_SD_CLK           0xFC182800
#define GPIO_CURRENT_NUMBER_FOR_SD_CLK       GPIO_SDIO_CLK
#define GPIO_MOD_MASK                        0x3
#define GPIO_SDIO_REMAP_SIZE                 0x1000
#define GPIO_SDIO_OFFSET                     0x4

/* hi11xx */
#define DTS_NODE_HISI_HI11XX                "hisilicon,hi110x"
#define DTS_COMP_HISI_HI11XX_BOARD_NAME     DTS_NODE_HISI_HI11XX
#define DTS_PROP_SUBCHIP_TYPE_VERSION       "hi110x,subchip_type"

#define DTS_NODE_HI110X                     "hisilicon,hi1102"
#define DTS_NODE_HI110X_BFGX                "hisilicon,hisi_bfgx"
#define DTS_NODE_HI110X_WIFI                "hisilicon,hisi_wifi"

#define DTS_COMP_HI110X_BOARD_NAME          DTS_NODE_HI110X

#define DTS_PROP_GPIO_POWEN_ON              "hi1102,gpio_power_on"
#define DTS_PROP_GPIO_WLAN_WAKEUP_HOST      "hi1102,gpio_wlan_wakeup_host"
#define DTS_PROP_GPIO_BFGN_WAKEUP_HOST      "hi1102,gpio_bfgn_wakeup_host"
#define DTS_PROP_GPIO_BFGN_IR_CTRL          "hi1102,gpio_bfgn_ir_ctrl"
#define DTS_PROP_UART_POART                 "hi1102,uart_port"
#define DTS_PROP_CLK_32K                    "huawei,pmu_clk32b"
#define DTS_PROP_VERSION                    "hi1102,asic_version"
#define DTS_PROP_POWER_PREPARE              "hi1102,power_prepare"

#define PROC_NAME_GPIO_POWEN_ON             "power_on_enable"
#define PROC_NAME_GPIO_WLAN_EN              "wlan_en_enable"
#define PROC_NAME_GPIO_WLAN_WAKEUP_HOST     "wlan_wake_host"
#define PROC_NAME_GPIO_HOST_TO_DEVICE       "host_to_device"


#define VERSION_FPGA                0
#define VERSION_ASIC                1

#define GET_CLOCK_GPIO_VAL()                board_get_wlan_host_to_dev_gpio_val()
#define SET_CLOCK_40M()                     board_set_host_to_dev_gpio_val_low()
#define SET_CLOCK_24M()                     board_set_host_to_dev_gpio_val_high()
#define RESTORE_CLOCK_GPIO_VAL(val)         set_host_to_dev_gpio_val(val)

#define             OAL_SDIO_CARD_DETECT_CHANGE(v)      do { dw_mci_sdio_card_detect_change();\
                                                                                                    } while (0)
#define GPIO_HIGH                   1
#define INVALID_IRQ                 (-1)

#define PLAT_EXCEPTION_DEV_PANIC_LR_ADDR       0x5CBC
#define PLAT_EXCEPTION_DEV_PANIC_PC_LR_LEN     0x8

#define BOARD_VERSION_LEN    128
#define BOARD_VERSION_NAME_HI1102  "hi1102"
#define BOARD_VERSION_NAME_HI1103  "hi1103"
#define BOARD_VERSION_NAME_HI1105  "hi1105"
#define BOARD_VERSION_NAME_HI1102A "hi1102a"
#define BOARD_VERSION_NAME_HI1131K "hi1131k"

/* private data for pm driver */
typedef struct {
    /* hi110x gpio info */
    oal_int32 power_on_enable;
    oal_int32 wlan_en;
    oal_int32 wlan_wakeup_host;
    oal_int32 wlan_host_to_device;

    /* hi110x irq info */
    oal_uint32 wlan_irq;
    oal_uint32 wlan_wakeup_irq;

    /* hi110x clk info */
    const char *clk_32k_name;
    struct clk* clk_32k;

    /* device hisi board version */
    const char* chip_type;
    oal_int32 chip_nr;

    /* evb or fpga verison */
    oal_int32 is_asic;

    /* prepare before board power on */
    oal_int32 need_power_prepare;
    struct pinctrl *pctrl;
    struct pinctrl_state *pins_normal;
    struct pinctrl_state *pins_idle;
}BOARD_INFO;

typedef struct _device_vesion_board {
    oal_uint32 index;
    const char name[BOARD_VERSION_LEN + 1];
} DEVICE_BOARD_VERSION;

enum hisi_device_board {
    // HI110X 项目使用 0 ~ 999
    BOARD_VERSION_HI1102 = 0,
    BOARD_VERSION_HI1103 = 1,
    BOARD_VERSION_HI1102A = 2,
    BOARD_VERSION_HI1105 = 3,

    // HI113X 项目使用 1000 ~ 2000
    BOARD_VERSION_HI1131K = 1000,
    BOARD_VERSION_BOTT,
};

/*****************************************************************************
  4 EXTERN VARIABLE
*****************************************************************************/
/*****************************************************************************
  5 EXTERN FUNCTION
*****************************************************************************/
extern BOARD_INFO *get_board_info(void);
extern oal_int32 hi110x_board_init(void);
extern void hi110x_board_exit(void);
extern void board_power_on(void);
extern void board_power_off(void);
extern int board_get_bwkup_gpio_val(void);
extern int board_get_wlan_wkup_gpio_val(void);
extern int board_get_wlan_host_to_dev_gpio_val(void);
extern void board_set_host_to_dev_gpio_val_low(void);
extern void board_set_host_to_dev_gpio_val_high(void);
extern void board_set_wlan_h2d_pm_state(unsigned int ul_value);
extern oal_uint32 board_get_and_clear_wlan_sdio_gpio_intr_state(oal_void);
extern oal_uint32 board_get_and_clear_wlan_wkup_gpio_intr_state(oal_void);
extern void set_host_to_dev_gpio_val(int val);
extern void wlan_rst(void);
extern oal_int32 board_chiptype_init(void);

#endif

