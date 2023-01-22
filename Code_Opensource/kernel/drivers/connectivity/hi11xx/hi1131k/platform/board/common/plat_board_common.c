

inline BOARD_INFO *get_board_info(void)
{
    return &g_board_info;
}

#ifndef _PRE_CONFIG_USE_DTS
oal_int32 check_evb_or_fpga(void)
{
#ifdef _PRE_WLAN_CHIP_FPGA
    PS_PRINT_INFO("HI1131 FPGA VERSION\n");
    g_board_info.is_asic = VERSION_FPGA;
#else
    PS_PRINT_INFO("HI1131 ASIC VERSION\n");
    g_board_info.is_asic = VERSION_ASIC;
#endif
    return BOARD_SUCC;
}
#endif

#ifndef _PRE_FEATURE_NO_GPIO
#ifdef CONFIG_GPIOLIB
oal_void board_gpio_pin_mux_init(oal_void);

int board_get_wlan_wkup_gpio_val(void)
{
    return gpio_get_value(g_board_info.wlan_wakeup_host);
}

int board_get_wlan_host_to_dev_gpio_val(void)
{
    return gpio_get_value(g_board_info.wlan_host_to_device);
}

void board_set_host_to_dev_gpio_val_low(void)
{
    gpio_direction_output(g_board_info.wlan_host_to_device, GPIO_LOWLEVEL);
}

void board_set_host_to_dev_gpio_val_high(void)
{
    gpio_direction_output(g_board_info.wlan_host_to_device, GPIO_HIGHLEVEL);
}

void set_host_to_dev_gpio_val(int val)
{
    if (val == GPIO_HIGHLEVEL) {
        board_set_host_to_dev_gpio_val_high();
    } else {
        board_set_host_to_dev_gpio_val_low();
    }
}

void board_set_wlan_h2d_pm_state(unsigned int ul_value)
{
    if (ul_value) {
        gpio_direction_output(g_board_info.wlan_host_to_device, GPIO_HIGHLEVEL);
    } else {
        gpio_direction_output(g_board_info.wlan_host_to_device, GPIO_LOWLEVEL);
    }
}

oal_uint32 board_get_and_clear_wlan_sdio_gpio_intr_state(oal_void)
{
    return GPIO_HIGH;
}

oal_uint32 board_get_and_clear_wlan_wkup_gpio_intr_state(oal_void)
{
    return GPIO_HIGH;
}

void board_power_on(void)
{
#ifdef CONFIG_DEV_CLK_40M
    SET_CLOCK_40M();
#else
    SET_CLOCK_24M();
#endif
    gpio_direction_output(g_board_info.power_on_enable, GPIO_LOWLEVEL);
    mdelay(10); // 延时10MS
    gpio_direction_output(g_board_info.power_on_enable, GPIO_HIGHLEVEL);
    mdelay(30); // 延时30MS
}

void board_power_off(void)
{
    gpio_direction_output(g_board_info.power_on_enable, GPIO_LOWLEVEL);
}

void wlan_rst(void)
{
    gpio_direction_output(g_board_info.wlan_en, GPIO_LOWLEVEL);
    mdelay(5); // 延时5MS
    gpio_direction_output(g_board_info.wlan_en, GPIO_HIGHLEVEL);
    mdelay(30); // 延时30MS
}

oal_int32 board_gpio_init(void)
{
    oal_int32 ret;
    PS_PRINT_ERR("board_gpio_init enter\n");

    board_gpio_pin_mux_init();

    g_board_info.power_on_enable = GPIO_PMU_PWRON;
    ret = gpio_request_one(g_board_info.power_on_enable, GPIOF_OUT_INIT_LOW, PROC_NAME_GPIO_POWEN_ON);
    if (ret) {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_POWEN_ON);
        goto err_get_power_on_gpio;
    }

    g_board_info.wlan_en = GPIO_WL_RST_N;
    ret = gpio_request_one(g_board_info.wlan_en, GPIOF_OUT_INIT_HIGH, PROC_NAME_GPIO_WLAN_EN);
    if (ret) {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_WLAN_EN);
        goto err_get_wlan_en_gpio;
    }

    /* wifi wake host gpio request */
    g_board_info.wlan_wakeup_host = GPIO_DEVICE_TO_HOST;
    ret = gpio_request_one(g_board_info.wlan_wakeup_host, GPIOF_IN, PROC_NAME_GPIO_WLAN_WAKEUP_HOST);
    if (ret) {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_WLAN_WAKEUP_HOST);
        goto err_get_wlan_wkup_host_gpio;
    }

    /* host to device gpio request */
    g_board_info.wlan_host_to_device = GPIO_HOST_TO_DEVICE;
    ret = gpio_request_one(g_board_info.wlan_host_to_device, GPIOF_OUT_INIT_HIGH, PROC_NAME_GPIO_HOST_TO_DEVICE);
    if (ret) {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_HOST_TO_DEVICE);
        goto err_get_host_to_device;
    }

    /* data interrupt gpio request */
    g_board_info.wlan_data_intr = GPIO_DATA_INTR;
    ret = gpio_request_one(g_board_info.wlan_data_intr, GPIOF_IN, PROC_NAME_GPIO_DATA_INTR);
    if (ret) {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_DATA_INTR);
        goto err_get_wlan_data_intr;
    }

    return BOARD_SUCC;

    err_get_wlan_data_intr:
        gpio_free(g_board_info.wlan_host_to_device);
    err_get_host_to_device:
        gpio_free(g_board_info.wlan_wakeup_host);
    err_get_wlan_wkup_host_gpio:
        gpio_free(g_board_info.wlan_en);
    err_get_wlan_en_gpio:
        gpio_free(g_board_info.power_on_enable);
    err_get_power_on_gpio:

    return BOARD_FAIL;
}

oal_int32 board_irq_init(void)
{
    oal_uint32 irq;
    oal_int32 gpio;

    PS_PRINT_INFO("in func\n");

    gpio = g_board_info.wlan_data_intr;
    irq = gpio_to_irq(gpio);
    g_board_info.wlan_irq = irq;

    PS_PRINT_INFO("wlan_irq is %d\n", g_board_info.wlan_irq);

    gpio = g_board_info.wlan_wakeup_host;
    irq = gpio_to_irq(gpio);
    g_board_info.wlan_wakeup_irq = irq;

    PS_PRINT_INFO("wlan_wakeup_irq is %d\n", g_board_info.wlan_wakeup_irq);
    return BOARD_SUCC;
}


oal_int32 hi110x_board_init(void)
{
    oal_int32 ret;

    PS_PRINT_FUNCTION_NAME;
    ret = check_evb_or_fpga();
    if (ret != BOARD_SUCC) {
        return BOARD_FAIL;
    }

    ret = board_gpio_init();
    if (ret != BOARD_SUCC) {
        return BOARD_FAIL;
    }

    ret = board_irq_init();
    if (ret != BOARD_SUCC) {
        goto err_gpio_source;
    }

    return ret;
err_gpio_source:
        gpio_free(g_board_info.wlan_wakeup_host);
        gpio_free(g_board_info.power_on_enable);
        return BOARD_FAIL;
}

void hi110x_board_exit(void)
{
    PS_PRINT_FUNCTION_NAME;

    gpio_free(g_board_info.power_on_enable);
    gpio_free(g_board_info.wlan_en);
    gpio_free(g_board_info.wlan_wakeup_host);
    gpio_free(g_board_info.wlan_host_to_device);
    gpio_free(g_board_info.wlan_data_intr);

    /* 卸载成功后，输出打印 */
    printk("board exit ok!\n");
    return;
}


#endif

#else
oal_void board_power_on(oal_void)
{
}

int board_get_wlan_wkup_gpio_val(void)
{
    return 0;
}

oal_void wlan_rst(oal_void)
{
}

oal_void board_power_off(oal_void)
{
}

oal_void board_set_wlan_h2d_pm_state(oal_uint32 ul_value)
{
    (void)(ul_value);
}

oal_uint32 board_get_and_clear_wlan_sdio_gpio_intr_state(oal_void)
{
    return 0;
}

oal_uint32 board_get_and_clear_wlan_wkup_gpio_intr_state(oal_void)
{
    return 0;
}

void board_set_host_to_dev_gpio_val_low(void)
{
}

void board_set_host_to_dev_gpio_val_high(void)
{
}

oal_int32 hi110x_board_init(void)
{
    oal_int32 ret;

    PS_PRINT_FUNCTION_NAME;
    ret = check_evb_or_fpga();
    if (ret != BOARD_SUCC) {
        return BOARD_FAIL;
    }

    return BOARD_SUCC;
}

void hi110x_board_exit(void)
{
}
#endif


