

#ifndef __PLAT_BOARD_COMMON_H__
#define __PLAT_BOARD_COMMON_H__

#define BOARD_SUCC                           0
#define BOARD_FAIL                           (-1)

#define VERSION_FPGA                0
#define VERSION_ASIC                1

#define GPIO_LOWLEVEL                        0
#define GPIO_HIGHLEVEL                       1

#define SDIO_REG_WRITEL(Addr, Value)  ((*(volatile unsigned int *)(Addr)) = (Value))
#define SDIO_REG_READL(Addr)          (*(volatile unsigned int *)(Addr))

#define GET_CLOCK_GPIO_VAL()                board_get_wlan_host_to_dev_gpio_val()
#define SET_CLOCK_40M()                     board_set_host_to_dev_gpio_val_low()
#define SET_CLOCK_24M()                     board_set_host_to_dev_gpio_val_high()
#define RESTORE_CLOCK_GPIO_VAL(val)         set_host_to_dev_gpio_val(val)

#define GPIO_HIGH                               1
#define INVALID_IRQ                             (-1)

#define NO_NEED_POWER_PREPARE                0
#define NEED_POWER_PREPARE                   1

#define PROC_NAME_GPIO_POWEN_ON             "power_on_enable"
#define PROC_NAME_GPIO_WLAN_EN              "wlan_en_enable"
#define PROC_NAME_GPIO_WLAN_WAKEUP_HOST     "wlan_wake_host"
#define PROC_NAME_GPIO_HOST_TO_DEVICE       "host_to_device"
#define PROC_NAME_GPIO_DATA_INTR            "wlan_data_intr"

#define             OAL_SDIO_CARD_DETECT_CHANGE(v)      do { sdio_card_detect_change(v);\
                                                                        } while (0)


/*****************************************************************************
  3 STRUCT DEFINE
*****************************************************************************/
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
/* private data for pm driver */
typedef struct {
    /* hi110x gpio info */
    oal_int32 power_on_enable;
    oal_int32 wlan_en;
    oal_int32 wlan_wakeup_host;
    oal_int32 wlan_host_to_device;
    oal_int32 wlan_data_intr;
    /* hi110x irq info */
    oal_uint32 wlan_irq;
    oal_uint32 wlan_wakeup_irq;
    /* evb or fpga verison */
    oal_int32 is_asic;
}BOARD_INFO;
#endif
/*****************************************************************************
  5 EXTERN FUNCTION
*****************************************************************************/
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
extern BOARD_INFO *get_board_info(void);
#endif
extern oal_int32 hi110x_board_init(void);
extern void hi110x_board_exit(void);
extern void board_power_on(void);
extern void board_power_off(void);
extern int board_get_bwkup_gpio_val(void);
extern int board_get_wlan_wkup_gpio_val(void);
extern int board_get_wlan_host_to_dev_gpio_val(void);
extern void board_set_host_to_dev_gpio_val_low(void);
extern void board_set_host_to_dev_gpio_val_high(void);
extern void set_host_to_dev_gpio_val(int val);
extern void board_set_wlan_h2d_pm_state(unsigned int ul_value);
extern oal_uint32 board_get_and_clear_wlan_sdio_gpio_intr_state(oal_void);
extern oal_uint32 board_get_and_clear_wlan_wkup_gpio_intr_state(oal_void);
extern void wlan_rst(void);
extern oal_void sdio_card_detect_change(oal_int32 val);

#endif

