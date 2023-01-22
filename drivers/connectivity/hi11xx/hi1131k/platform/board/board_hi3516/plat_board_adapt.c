

/*lint -e322*/ /*lint -e7*/
#include <oal_interrupt.h>
#include <asm/hal_platform_ints.h>
#include <linux/delay.h>
#include <gpio.h>
#include <hisoc/gpio.h>
/*lint +e322*/ /*lint +e7*/

#include "plat_board_adapt.h"
#include "plat_debug.h"
#include "arch/oal_util.h"

BOARD_INFO g_board_info = { 0 };

#include "plat_board_common.c"

#ifndef _PRE_FEATURE_NO_GPIO
oal_int32 board_get_wlan_wkup_gpio_val(void)
{
    return gpio_read(g_board_info.wifi_data_intr_gpio_group, g_board_info.wifi_data_intr_gpio_offset);
}

oal_void board_power_off(oal_void)
{
    PS_PRINT_FUNCTION_NAME;
    if (g_board_info.wifi_power_set == OAL_PTR_NULL) {
        PS_PRINT_ERR("board info is not inited!\n");
        return;
    }
    g_board_info.wifi_power_set(OAL_OFF);
    msleep(10); // 延时10MS
}

oal_int32 board_get_wlan_h2d_pm_state(oal_void)
{
    return gpio_read(g_board_info.host_wak_dev_gpio_group, g_board_info.host_wak_dev_gpio_offset);
}


oal_void board_set_wlan_h2d_pm_state(oal_uint32 ul_value)
{
    if (ul_value) {
        gpio_write(g_board_info.host_wak_dev_gpio_group, g_board_info.host_wak_dev_gpio_offset, GPIO_HIGH_LEVEL);
    } else {
        gpio_write(g_board_info.host_wak_dev_gpio_group, g_board_info.host_wak_dev_gpio_offset, GPIO_LOW_LEVEl);
    }
}

oal_uint32 board_get_and_clear_wlan_gpio_intr_state(oal_uint8 gpio_group, oal_uint8 gpio_offset)
{
    oal_uint32 ul_gpio_state;
    ul_gpio_state = gpio_mis_read(gpio_group, gpio_offset);
    gpio_ic_clear(gpio_group, gpio_offset);
    return ul_gpio_state;
}

oal_uint32 board_get_and_clear_wlan_sdio_gpio_intr_state(oal_void)
{
    return GPIO_HIGH;
}

oal_uint32 board_get_and_clear_wlan_wkup_gpio_intr_state(oal_void)
{
    return GPIO_HIGH;
}

oal_void board_power_on(oal_void)
{
    int clock;
    PS_PRINT_FUNCTION_NAME;
    if ((g_board_info.wifi_power_set == OAL_PTR_NULL) || (g_board_info.wifi_rst_set == OAL_PTR_NULL)) {
        PS_PRINT_ERR("board info is not inited!\n");
        return;
    }

    clock = board_get_wlan_h2d_pm_state();
#ifdef _PRE_PMC_COEXIST_24M
    board_set_wlan_h2d_pm_state(GPIO_HIGH_LEVEL); // 电平选择，高为24M,快速启动版本使用共24M，快速启动在sample初始化上电一次，此次同步修改，业务需要网卡重启是才会走到该行代码
#else
    board_set_wlan_h2d_pm_state(GPIO_LOW_LEVEl); // set low, tcxo 40M
#endif
    g_board_info.wifi_power_set(OAL_OFF);
    msleep(20); // 延时20MS
    g_board_info.wifi_power_set(OAL_ON);
    g_board_info.wifi_rst_set(OAL_ON);
    msleep(70); // 延时70MS
    board_set_wlan_h2d_pm_state(clock);
}

oal_void host_pow_off(oal_void)
{
    if (g_board_info.host_pow_off == OAL_PTR_NULL) {
        PS_PRINT_ERR("board info is not inited!\n");
        return;
    }
    PS_PRINT_ERR("host_pow_off!!\n");
    g_board_info.host_pow_off();
}

oal_void wlan_rst(oal_void)
{
    if (g_board_info.wifi_rst_set == OAL_PTR_NULL) {
        PS_PRINT_ERR("board info is not inited!\n");
        return;
    }
    g_board_info.wifi_rst_set(OAL_OFF);
    mdelay(5); // 延时5MS
    g_board_info.wifi_rst_set(OAL_ON);
    mdelay(50); // 延时50MS
}

oal_int32 board_gpio_init(oal_void)
{
    oal_int32 ret;
    UINTPTR uwIntSave;
    uwIntSave = LOS_IntLock();
    gpio_dir_config(g_board_info.wifi_data_intr_gpio_group, g_board_info.wifi_data_intr_gpio_offset, GPIO_DIR_IN);
    gpio_dir_config(g_board_info.host_wak_dev_gpio_group, g_board_info.host_wak_dev_gpio_offset, GPIO_DIR_OUT);
    gpio_dir_config(g_board_info.dev_wak_host_gpio_group, g_board_info.dev_wak_host_gpio_offset, GPIO_DIR_IN);

    (VOID)LOS_IntRestore(uwIntSave);
    return BOARD_SUCC;
}

oal_int32 board_irq_init(oal_void)
{
    oal_int32 ret;
    UINTPTR uwIntSave;
    uwIntSave = LOS_IntLock();

    gpio_dir_config(g_board_info.wifi_data_intr_gpio_group, g_board_info.wifi_data_intr_gpio_offset, GPIO_DIR_IN);
    gpio_is_config(g_board_info.wifi_data_intr_gpio_group, g_board_info.wifi_data_intr_gpio_offset, GPIO_IS_EDGE);
    gpio_ibe_config(g_board_info.wifi_data_intr_gpio_group, g_board_info.wifi_data_intr_gpio_offset, GPIO_IBE_SINGLE);
    gpio_iev_config(g_board_info.wifi_data_intr_gpio_group, g_board_info.wifi_data_intr_gpio_offset, GPIO_IEV_POSEDGE);
    gpio_ic_clear(g_board_info.wifi_data_intr_gpio_group, g_board_info.wifi_data_intr_gpio_offset);
    gpio_ie_config(g_board_info.wifi_data_intr_gpio_group, g_board_info.wifi_data_intr_gpio_offset, GPIO_IE_ENABLE);

    gpio_dir_config(g_board_info.dev_wak_host_gpio_group, g_board_info.dev_wak_host_gpio_offset, GPIO_DIR_IN);
    gpio_is_config(g_board_info.dev_wak_host_gpio_group, g_board_info.dev_wak_host_gpio_offset, GPIO_IS_EDGE);
    gpio_ibe_config(g_board_info.dev_wak_host_gpio_group, g_board_info.dev_wak_host_gpio_offset, GPIO_IBE_SINGLE);
    gpio_iev_config(g_board_info.dev_wak_host_gpio_group, g_board_info.dev_wak_host_gpio_offset, GPIO_IEV_POSEDGE);
    gpio_ic_clear(g_board_info.dev_wak_host_gpio_group, g_board_info.dev_wak_host_gpio_offset);
    gpio_ie_config(g_board_info.dev_wak_host_gpio_group, g_board_info.dev_wak_host_gpio_offset, GPIO_IE_ENABLE);

    g_board_info.wlan_irq =
        GPIO_TO_IRQ(g_board_info.wifi_data_intr_gpio_group, g_board_info.wifi_data_intr_gpio_offset);
    g_board_info.wlan_wakeup_irq =
        GPIO_TO_IRQ(g_board_info.dev_wak_host_gpio_group, g_board_info.dev_wak_host_gpio_offset);

    (VOID)LOS_IntRestore(uwIntSave);

    return BOARD_SUCC;
}

oal_int32 hi110x_board_init(oal_void)
{
    oal_int32 ret;

    PS_PRINT_FUNCTION_NAME;

    ret = check_evb_or_fpga();
    if (ret != BOARD_SUCC) {
        return BOARD_FAIL;
    }

    board_gpio_init();
    board_irq_init();

    ret = BOARD_SUCC;
    return ret;
}

oal_void hi110x_board_exit(oal_void)
{
    g_board_info.wlan_irq = INVALID_IRQ;
    g_board_info.wlan_wakeup_irq = INVALID_IRQ;
}
#endif

oal_void sdio_card_detect_change(oal_int32 val)
{
    if (g_board_info.wifi_sdio_detect == OAL_PTR_NULL) {
        PS_PRINT_ERR("board info is not inited!\n");
        return;
    }
    g_board_info.wifi_sdio_detect();
}
