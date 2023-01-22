
/*****************************************************************************
  1 Include Head file
*****************************************************************************/
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include "plat_debug.h"
#include "plat_pm.h"
#include "exception_rst.h"
#include "oneimage.h"
#include "oal_kernel_file.h"
#include "oal_sdio_host_if.h"
#include "plat_pm_wlan.h"
#include "hisi_ini.h"
#include "heartbeat_host.h"
#include "user_ctrl.h"
#include "securec.h"
#ifdef PLATFORM_DEBUG_ENABLE
#include "plat_pm_wlan.h"
#endif

/*****************************************************************************
  2 Define global variable
*****************************************************************************/
#define CMD_NUM_HEAD            (0x5A << 8)
#define GET_CMD_NUM(cmd)        ((cmd) & 0xFF)
#define GET_CMD_HEAD(cmd)       ((cmd) & (~0xFF))

struct kobject *g_sysfs_hi110x_bfgx    = NULL;
struct kobject *g_sysfs_hisi_pmdbg     = NULL;
struct platform_device *g_hw_ps_device = NULL;

#ifdef PLATFORM_DEBUG_ENABLE
    struct kobject *g_sysfs_hi110x_debug   = NULL;
    oal_int32 g_uart_rx_dump  = UART_DUMP_CLOSE;
#endif

oal_int32 g_plat_loglevel = PLAT_LOG_WARNING;
oal_int32 g_bug_on_enable = BUG_ON_DISABLE;
module_param(g_plat_loglevel, int, S_IRUGO | S_IWUSR);

#define DTS_COMP_HI1101_PS_NAME     "hisilicon,hisi_bfgx"
/*****************************************************************************
  3 Function implement
*****************************************************************************/
OAL_STATIC ssize_t show_wifi_pmdbg(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    struct wlan_pm_s *pst_wlan_pm = wlan_pm_get_drv();
    oal_int32         ret = 0;

    if (buf == NULL) {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

#ifdef PLATFORM_DEBUG_ENABLE
    ret += snprintf_s(buf + ret, PAGE_SIZE - ret, PAGE_SIZE - ret - 1, "wifi_pm_debug usage: \n"
                      " 1:dump host info 2:dump device info\n"
                      " 3:open wifi      4:close wifi \n"
                      " 5:enable pm      6:disable pm \n");
#else
    ret += snprintf_s(buf + ret, PAGE_SIZE - ret, PAGE_SIZE - ret - 1, "wifi_pm_debug usage: \n"
                      " 1:dump host info 2:dump device info\n");
#endif

    ret += wlan_pm_host_info_print(pst_wlan_pm, buf + ret, PAGE_SIZE - ret);

    return ret;
}

OAL_STATIC ssize_t store_wifi_pmdbg(struct kobject *dev, struct kobj_attribute *attr, const char *buf, size_t count)
{
    struct wlan_pm_s *pst_wlan_pm = wlan_pm_get_drv();
    int input;

    if (buf == NULL) {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    input = oal_atoi(buf);

    if (pst_wlan_pm == NULL) {
        OAL_IO_PRINT("pm_data is NULL!\n");
        return -FAILURE;
    }

    switch (input) {
        case 1:
            wlan_pm_dump_host_info();
            break;
        case 2:
            wlan_pm_dump_device_info();
            break;
#ifdef PLATFORM_DEBUG_ENABLE
        case 3:
            wlan_pm_open();
            break;
        case 4:
            wlan_pm_close();
            break;
        case 5:
            wlan_pm_enable();
            break;
        case 6:
            wlan_pm_disable();
            break;
        case 7:
            wlan_pm_suspend_test();
            break;
        case 8:
            wlan_pm_resume_test();
            break;
#endif
        default:
            break;
    }

    return count;
}

OAL_STATIC struct kobj_attribute g_wifi_pmdbg =
    __ATTR(wifi_pm, 0644, show_wifi_pmdbg, store_wifi_pmdbg); //lint !e64

OAL_STATIC struct attribute *g_pmdbg_attrs[] = {
    &g_wifi_pmdbg.attr,
    NULL,
};

OAL_STATIC struct attribute_group g_pmdbg_attr_grp = {
    .attrs = g_pmdbg_attrs,
};

/* functions called from subsystems */
/* called by octty from hal to decide open or close uart */
OAL_STATIC ssize_t show_install(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    return 0;
}

OAL_STATIC ssize_t show_ini_file_name(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int32_t len;

    if (buf == NULL) {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    len = snprintf_s(buf, INI_FILE_PATH_LEN, INI_FILE_PATH_LEN - 1, "%s", g_ini_file_name);
    if (len < EOK) {
        PS_PRINT_ERR("show_ini_file_name::snprintf_s failed !\n");
    }
    return len;
}

OAL_STATIC ssize_t show_country_code(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    oal_int8 *country_code = NULL;
    int ret;
    oal_int8 ibuf[COUNTRY_CODE_LEN] = { 0 };
    int32_t len;

    if (buf == NULL) {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    country_code = hwifi_get_country_code();
    if (strncmp(country_code, "99", strlen("99")) == 0) {
        ret = get_cust_conf_string(CUST_MODU_WIFI, "country_code", ibuf, sizeof(ibuf));
        if (ret == INI_SUCC) {
            strncpy_s(buf, COUNTRY_CODE_LEN, ibuf, COUNTRY_CODE_LEN);
            buf[COUNTRY_CODE_LEN - 1] = '\0';
            return strlen(buf);
        } else {
            PS_PRINT_ERR("get dts country_code error\n");
            return 0;
        }
    } else {
        len = snprintf_s(buf, COUNTRY_CODE_LEN, COUNTRY_CODE_LEN - 1, "%s", country_code);
        if (len < EOK) {
            PS_PRINT_ERR("show_country_code::snprintf_s failed !\n");
        }
        return len;
    }
}

OAL_STATIC ssize_t show_wifi_exception_count(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    struct st_exception_info *pst_exception_data = NULL;
    int32_t len;
    if (buf == NULL) {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    get_exception_info_reference(&pst_exception_data);
    if (pst_exception_data == NULL) {
        PS_PRINT_ERR("get exception info reference is error\n");
        return 0;
    }

    PS_PRINT_DBG("exception debug: wifi rst count is %d\n", pst_exception_data->wifi_exception_cnt);
    len = sprintf_s(buf, PAGE_SIZE, "%d\n", pst_exception_data->wifi_exception_cnt); //lint !e421
    if (len < EOK) {
        PS_PRINT_ERR("show_wifi_exception_count::sprintf_s failed !\n");
    }
    return len;
}

OAL_STATIC ssize_t show_loglevel(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int32_t len;
    PS_PRINT_INFO("%s\n", __func__);

    if (buf == NULL) {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    len = sprintf_s(buf, PAGE_SIZE, "curr loglevel=%d, curr bug_on=%d"
                    "\nalert:0\nerr:1\nwarning:2\nfunc|succ|info:3\ndebug:4\nbug_on enable:b\nbug_on disable:B\n",
                    g_plat_loglevel, g_bug_on_enable); //lint !e421
    if (len < EOK) {
        PS_PRINT_ERR("show_loglevel::sprintf_s failed !\n");
    }
    return len;
}

OAL_STATIC ssize_t store_loglevel(struct kobject *dev, struct kobj_attribute *attr, const char *buf, size_t count)
{
    oal_int32 loglevel;

    PS_PRINT_INFO("%s\n", __func__);

    if (buf == NULL) {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    /* bug on set */
    if (*buf == 'b') {
        g_bug_on_enable = BUG_ON_ENABLE;
        PS_PRINT_INFO("BUG_ON enable sucess, g_bug_on_enable = %d\n", g_bug_on_enable);
        return count;
    } else if (*buf == 'B') {
        g_bug_on_enable = BUG_ON_DISABLE;
        PS_PRINT_INFO("BUG_ON disable sucess, g_bug_on_enable = %d\n", g_bug_on_enable);
        return count;
    }

    loglevel = simple_strtol(buf, NULL, 10);
    if (loglevel < PLAT_LOG_ALERT) {
        g_plat_loglevel = PLAT_LOG_ALERT;
    } else if(loglevel > PLAT_LOG_DEBUG) {
        g_plat_loglevel = PLAT_LOG_DEBUG;
    } else {
        g_plat_loglevel = loglevel;
    }

    return count;
}


/* structures specific for sysfs entries */
OAL_STATIC struct kobj_attribute g_ldisc_install =
    __ATTR(install, 0444, show_install, NULL);

OAL_STATIC struct kobj_attribute ini_file_name =
    __ATTR(ini_file_name, 0444, show_ini_file_name, NULL);

OAL_STATIC struct kobj_attribute g_country_code =
    __ATTR(g_country_code, 0444, show_country_code, NULL);

OAL_STATIC struct kobj_attribute g_wifi_rst_count =
    __ATTR(g_wifi_rst_count, 0444, show_wifi_exception_count, NULL);


OAL_STATIC struct kobj_attribute g_loglevel =
    __ATTR(g_loglevel, 0664, show_loglevel, store_loglevel); //lint !e64

#ifdef CONFIG_HI1102_PLAT_HW_CHR
static struct kobj_attribute g_chr_log_switch =
    __ATTR(g_chr_log_switch, 0644, show_chr_log_switch, store_chr_log_switch);
#endif

OAL_STATIC struct attribute *g_bfgx_attrs[] = {
    &g_ldisc_install.attr,
    &ini_file_name.attr,

    &g_country_code.attr,
    &g_wifi_rst_count.attr,
    &g_loglevel.attr,
#ifdef CONFIG_HI1102_PLAT_HW_CHR
    &g_chr_log_switch.attr,
#endif
    NULL,
};

OAL_STATIC struct attribute_group g_bfgx_attr_grp = {
    .attrs = g_bfgx_attrs,
};

#ifdef PLATFORM_DEBUG_ENABLE
OAL_STATIC ssize_t show_exception_dbg(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int32_t len;
    PS_PRINT_INFO("%s\n", __func__);

    if (buf == NULL) {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    len = sprintf_s(buf, PAGE_SIZE, "cmd  func             \n"
                    " 1  close bt          \n"
                    " 2  set beat flat to 0\n"); //lint !e421
    if (len < EOK) {
        PS_PRINT_ERR("show_exception_dbg::sprintf_s failed !\n");
    }
    return len;
}

OAL_STATIC ssize_t store_exception_dbg(struct kobject *dev, struct kobj_attribute *attr, const char *buf,
                                       size_t count)
{
    return 0;
}

OAL_STATIC ssize_t show_uart_rx_dump(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int32_t len;
    PS_PRINT_INFO("%s\n", __func__);

    if (buf == NULL) {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    len = sprintf_s(buf, PAGE_SIZE, "curr uart dump status =%d\n no:0\n yes:1\n", g_uart_rx_dump); //lint !e421
    if (len < EOK) {
        PS_PRINT_ERR("show_uart_rx_dump::sprintf_s failed !\n");
    }
    return len;
}

OAL_STATIC ssize_t store_uart_rx_dump(struct kobject *dev, struct kobj_attribute *attr, const char *buf,
                                      size_t count)
{
    if (buf == NULL) {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    g_uart_rx_dump = simple_strtol(buf, NULL, 10);
    PS_PRINT_INFO("g_uart_rx_dump aft %d\n", g_uart_rx_dump);
    return count;
}

OAL_STATIC ssize_t show_dev_test(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int32_t len;
    PS_PRINT_INFO("%s\n", __func__);

    if (buf == NULL) {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    len = sprintf_s(buf, PAGE_SIZE, "cmd  func\n  1  cause bfgx into panic\n  2  enable exception recovery\n"
                    "3  enable wifi open bcpu\n  4  pull up power gpio\n  5  pull down power gpio\n"
                    "6  uart loop test       \n"); //lint !e421
    if (len < EOK) {
        PS_PRINT_ERR("show_dev_test::sprintf_s failed !\n");
    }
    return len;
}

OAL_STATIC ssize_t store_dev_test(struct kobject *dev, struct kobj_attribute *attr, const char *buf, size_t count)
{
    oal_int32 cmd;
    struct st_exception_info *pst_exception_data = NULL;

    PS_PRINT_INFO("%s\n", __func__);

    if (buf == NULL) {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    get_exception_info_reference(&pst_exception_data);
    if (pst_exception_data == NULL) {
        PS_PRINT_ERR("get exception info reference is error\n");
        return 0;
    }

    cmd = simple_strtol(buf, NULL, 10);
    switch (cmd) {
        case 2:
            PS_PRINT_INFO("cmd %d,enable platform dfr\n", cmd);
            pst_exception_data->exception_reset_enable = PLAT_EXCEPTION_ENABLE;
            break;
        case 4:
            PS_PRINT_INFO("cmd %d,test pull up power gpio\n", cmd);
            board_power_on();
            break;
        case 5:
            PS_PRINT_INFO("cmd %d,test pull down power gpio\n", cmd);
            board_power_off();
            break;

        default:
            PS_PRINT_ERR("unknown cmd %d\n", cmd);
            break;
    }

    return count;
}

OAL_STATIC ssize_t show_wifi_mem_dump(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int32_t len;
    PS_PRINT_INFO("%s\n", __func__);

    if (buf == NULL) {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    len = sprintf_s(buf, PAGE_SIZE, "cmd         func             \n"
                    " 1    uart halt wcpu         \n"
                    " 2    uart read wifi pub reg \n"
                    " 3    uart read wifi priv reg\n"
                    " 4    uart read wifi mem     \n"
                    " 5    equal cmd 1+2+3+4      \n"); //lint !e421
    if (len < EOK) {
        PS_PRINT_ERR("show_wifi_mem_dump::sprintf_s failed !\n");
    }
    return len;
}

OAL_STATIC ssize_t store_wifi_mem_dump(struct kobject *dev, struct kobj_attribute *attr, const char *buf,
                                       size_t count)
{
    return 0;
}
/*lint -e64*/
OAL_STATIC struct kobj_attribute g_plat_exception_dbg =
    __ATTR(exception_dbg, 0644, show_exception_dbg, store_exception_dbg);

OAL_STATIC struct kobj_attribute g_uart_dumpctrl =
    __ATTR(uart_rx_dump, 0664, show_uart_rx_dump, store_uart_rx_dump);

OAL_STATIC struct kobj_attribute g_wifi_dev_test =
    __ATTR(wifi_test, 0664, show_dev_test, store_dev_test);

OAL_STATIC struct kobj_attribute g_wifi_mem_dump =
    __ATTR(wifi_mem, 0664, show_wifi_mem_dump, store_wifi_mem_dump);
/*lint +e64*/
OAL_STATIC struct attribute *g_hi110x_debug_attrs[] = {
    &g_plat_exception_dbg.attr,
    &g_uart_dumpctrl.attr,
    &g_wifi_dev_test.attr,
    &g_wifi_mem_dump.attr,
    NULL,
};

OAL_STATIC struct attribute_group g_hi110x_debug_attr_grp = {
    .attrs = g_hi110x_debug_attrs,
};
#endif


oal_int32 user_ctrl_init(void)
{
    int status;
    struct kobject *pst_root_object = NULL;

    pst_root_object = oal_get_sysfs_root_object();
    if (pst_root_object == NULL) {
        PS_PRINT_ERR("[E]get root sysfs object failed!\n");
        return -EFAULT;
    }

    g_sysfs_hisi_pmdbg = kobject_create_and_add("pmdbg", pst_root_object);
    if (g_sysfs_hisi_pmdbg == NULL) {
        PS_PRINT_ERR("Failed to creat g_sysfs_hisi_pmdbg !!!\n ");
        goto fail;
    }

    status = oal_debug_sysfs_create_group(g_sysfs_hisi_pmdbg, &g_pmdbg_attr_grp);
    if (status) {
        PS_PRINT_ERR("failed to create g_sysfs_hisi_pmdbg sysfs entries\n");
        goto fail;
    }

    g_sysfs_hi110x_bfgx = kobject_create_and_add("hi110x_ps", NULL);
    if (g_sysfs_hi110x_bfgx == NULL) {
        PS_PRINT_ERR("Failed to creat g_sysfs_hi110x_ps !!!\n ");
        goto fail;
    }

    status = sysfs_create_group(g_sysfs_hi110x_bfgx, &g_bfgx_attr_grp);
    if (status) {
        PS_PRINT_ERR("failed to create g_sysfs_hi110x_bfgx sysfs entries\n");
        goto fail;
    }

#ifdef PLATFORM_DEBUG_ENABLE
    g_sysfs_hi110x_debug = kobject_create_and_add("hi110x_debug", NULL);
    if (g_sysfs_hi110x_debug == NULL) {
        PS_PRINT_ERR("Failed to creat g_sysfs_hi110x_debug !!!\n ");
        goto fail;
    }

    status = oal_debug_sysfs_create_group(g_sysfs_hi110x_debug, &g_hi110x_debug_attr_grp);
    if (status) {
        PS_PRINT_ERR("failed to create g_sysfs_hi110x_debug sysfs entries\n");
        goto fail;
    }
#endif

    return 0;

fail:
    user_ctrl_exit();
    return -EFAULT;
}

void user_ctrl_exit(void)
{
#ifdef PLATFORM_DEBUG_ENABLE
    if (g_sysfs_hi110x_debug != NULL) {
        oal_debug_sysfs_remove_group(g_sysfs_hi110x_debug, &g_hi110x_debug_attr_grp);
        kobject_put(g_sysfs_hi110x_debug);
        g_sysfs_hi110x_debug = NULL;
    }
#endif

    if (g_sysfs_hi110x_bfgx != NULL) {
        sysfs_remove_group(g_sysfs_hi110x_bfgx, &g_bfgx_attr_grp);
        kobject_put(g_sysfs_hi110x_bfgx);
        g_sysfs_hi110x_bfgx = NULL;
    }
#ifdef PLATFORM_DEBUG_ENABLE
    if (g_sysfs_hisi_pmdbg != NULL) {
        oal_debug_sysfs_remove_group(g_sysfs_hisi_pmdbg, &g_pmdbg_attr_grp);
        kobject_put(g_sysfs_hisi_pmdbg);
        g_sysfs_hisi_pmdbg = NULL;
    }
#endif
}


oal_int32 ps_get_core_reference(struct ps_plat_s **core_data)
{
    struct platform_device *pdev = NULL;

    pdev = g_hw_ps_device;
    if (!pdev) {
        *core_data = NULL;
        PS_PRINT_ERR("%s pdev is NULL\n", __func__);
        return FAILURE;
    }

    *core_data  = dev_get_drvdata(&pdev->dev);
    if ((*core_data) == NULL) {
        PS_PRINT_ERR("ps_plat_d is NULL\n");
        return FAILURE;
    }

    return SUCCESS;
}

#ifdef PLATFORM_DEBUG_ENABLE
static int32_t hw_debug_data_parse(const oal_int8 __user *buf)
{
    unsigned int temp;

    if (0 > kstrtouint(buf, 16, &temp)) {
        return -EINVAL;
    }

    if (CMD_NUM_HEAD != GET_CMD_HEAD(temp)) {
        return -EINVAL;
    }

    return _IO(PLAT_CFG_IOC_MAGIC, GET_CMD_NUM(temp));
}
#endif

OAL_STATIC oal_long hw_debug_ioctl(struct file *file, oal_uint32 cmd, oal_ulong arg)
{
    if (file == NULL) {
        PS_PRINT_ERR("file is null\n");
        return -EINVAL;
    }

    switch (cmd) {
        case PLAT_DFR_CFG_CMD:
            plat_dfr_cfg_set(arg);
            break;
        case PLAT_BEATTIMER_TIMEOUT_RESET_CFG_CMD:
            set_heartbeat_cfg(arg);
            break;
        case _IO(PLAT_CFG_IOC_MAGIC, PLAT_TRIGGER_EXCEPTION):
            PS_PRINT_WARNING("PLAT_TRIGGER_EXCEPTION\r\n");
            oal_trigger_exception(OAL_TRUE);
            // when panic msg come up, the function oal_sdio_device_panic_callback will be called
            break;
        case _IO(PLAT_CFG_IOC_MAGIC, PLAT_POWER_RESET):
            PS_PRINT_WARNING("PLAT_POWER_RESET\r\n");
            wifi_system_reset();
            break;
        case _IO(PLAT_CFG_IOC_MAGIC, PLAT_CHANNEL_DISABLE):
            PS_PRINT_WARNING("PLAT_CHANNEL_DISABLE\r\n");
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
            oal_wlan_gpio_intr_enable(oal_get_sdio_default_handler(), OAL_FALSE);
#endif
            break;

        default:
            PS_PRINT_WARNING("hw_debug_ioctl cmd = %d not find\n", cmd);
            return -EINVAL;
    }

    return 0;
}


OAL_STATIC oal_int32 hw_debug_open(struct inode *inode, struct file *filp)
{
    struct ps_plat_s *ps_core_d = NULL;
    int32_t ret;

    PS_PRINT_INFO("%s", __func__);

    ret = ps_get_core_reference(&ps_core_d);
    if (ret != SUCCESS) {
        PS_PRINT_ERR("hw_debug_open::ps_get_core_reference error! \n");
    }
    if (unlikely((ps_core_d == NULL) || (inode == NULL) || (filp == NULL))) {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    (ps_core_d->g_debug_cnt)++;
    PS_PRINT_INFO("%s g_debug_cnt=%d", __func__, ps_core_d->g_debug_cnt);
    atomic_set(&ps_core_d->dbg_func_has_open, 1);

    ps_core_d->dbg_read_delay = DBG_READ_DEFAULT_TIME;

    return 0;
}


OAL_STATIC ssize_t hw_debug_read(struct file *filp, char __user *buf,
                                 size_t count, loff_t *f_pos)
{

    PS_PRINT_DBG("not support yet\n");

    return count;
}


#ifdef PLATFORM_DEBUG_ENABLE
OAL_STATIC ssize_t hw_debug_write(struct file *filp, const char __user *buf,
                                  size_t count, loff_t *f_pos)
{
    int32_t cmd;

    PS_PRINT_FUNCTION_NAME;

    cmd = hw_debug_data_parse(buf);
    if (cmd > 0) {
        hw_debug_ioctl(filp, cmd, 0);
        return count;
    }

    PS_PRINT_ERR("not supported yet\r\n");
    return count;
}
#endif


OAL_STATIC oal_int32 hw_debug_release(struct inode *inode, struct file *filp)
{
    struct ps_plat_s *ps_core_d = NULL;
    int32_t ret;

    PS_PRINT_INFO("%s", __func__);

    ret = ps_get_core_reference(&ps_core_d);
    if (ret != SUCCESS) {
        PS_PRINT_ERR("hw_debug_release::ps_get_core_reference error !\n");
    }
    if (unlikely((ps_core_d == NULL) || (inode == NULL) || (filp == NULL))) {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    ps_core_d->g_debug_cnt--;
    PS_PRINT_INFO("%s g_debug_cnt=%d", __func__, ps_core_d->g_debug_cnt);

    return 0;
}
/*lint -e64*/
OAL_STATIC const struct file_operations g_hw_debug_fops = {
    .owner          = THIS_MODULE,
    .open           = hw_debug_open,
#ifdef PLATFORM_DEBUG_ENABLE
    .write          = hw_debug_write,
#endif
    .read           = hw_debug_read,
    .unlocked_ioctl = hw_debug_ioctl,
    .release        = hw_debug_release,
};
/*lint +e64*/
OAL_STATIC struct miscdevice g_hw_debug_device = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = "hwdbg",
    .fops   = &g_hw_debug_fops,
};

/* 1131k 异常时，获取memdump信息发送给 oam_hisi进行文件保存 */
int32_t plat_excp_dump_rotate_cmd_read(unsigned long arg, struct st_wifi_dump_mem_driver *p_dumpmem_driver)
{
    uint32_t __user *puser = (uint32_t __user *)(uintptr_t)arg;
    struct sk_buff *skb = NULL;

    if (!access_ok(VERIFY_WRITE, (uintptr_t)puser, (int32_t)sizeof(uint32_t))) {
        PS_PRINT_ERR("address can not write\n");
        return -EINVAL;
    }

    if (wait_event_interruptible(p_dumpmem_driver->dump_type_wait,
                                 (skb_queue_len(&p_dumpmem_driver->dump_type_queue)) > 0)) { //lint !e666
        return -EINVAL;
    }

    skb = skb_dequeue(&p_dumpmem_driver->dump_type_queue);
    if (skb == NULL) {
        PS_PRINT_WARNING("skb is NULL\n");
        return -EINVAL;
    }

    if (copy_to_user(puser, skb->data, sizeof(uint32_t))) {
        PS_PRINT_WARNING("copy_to_user err!restore it, len=%d,arg=%ld\n", (int32_t)sizeof(uint32_t), arg);
        skb_queue_head(&p_dumpmem_driver->dump_type_queue, skb);
        return -EINVAL;
    }

    PS_PRINT_INFO("read rotate cmd [%d] from queue\n", *(uint32_t *)skb->data);

    skb_pull(skb, skb->len);
    kfree_skb(skb);

    return 0;
}
OAL_STATIC int32_t hw_excp_read(struct file *filp, int8_t __user *buf,
                          size_t count, loff_t *f_pos, struct st_wifi_dump_mem_driver *p_dumpmem_driver)
{
    struct sk_buff *skb = NULL;
    uint16_t count1;

    if (unlikely((buf == NULL) || (filp == NULL))) {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }
    if ((skb = skb_dequeue(&p_dumpmem_driver->quenue)) == NULL) {
        return 0;
    }

    /* read min value from skb->len or count */
    count1 = min_t(size_t, skb->len, count);
    if (copy_to_user(buf, skb->data, count1)) {
        PS_PRINT_ERR("copy_to_user is err!\n");
        skb_queue_head(&p_dumpmem_driver->quenue, skb);
        return -EFAULT;
    }

    /* have read count1 byte */
    skb_pull(skb, count1);

    /* if skb->len = 0: read is over */
    if (skb->len == 0) { /* curr skb data have read to user */
        kfree_skb(skb);
    } else { /* if don,t read over; restore to skb queue */
        skb_queue_head(&p_dumpmem_driver->quenue, skb);
    }

    return count1;
}

OAL_STATIC long hw_wifiexcp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int32_t ret = 0;
    struct st_wifi_dump_mem_driver *p_dumpmem_driver;

    if (file == NULL) {
        PS_PRINT_ERR("file is null\n");
        return -EINVAL;
    }

    switch (cmd) {
        case PLAT_WIFI_DUMP_FILE_READ_CMD:
            p_dumpmem_driver = plat_get_dump_mem_driver();
            ret = plat_excp_dump_rotate_cmd_read(arg, p_dumpmem_driver);
            break;
        default:
            PS_PRINT_WARNING("hw_debug_ioctl cmd = %d not find\n", cmd);
            return -EINVAL;
    }

    return ret;
}

OAL_STATIC ssize_t hw_wifiexcp_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    int32_t ret = 0;
    struct st_wifi_dump_mem_driver *p_dumpmem_driver;

    p_dumpmem_driver = plat_get_dump_mem_driver();
    ret = hw_excp_read(filp, buf, count, f_pos, p_dumpmem_driver);

    return ret;
}
/*lint -e64*/
OAL_STATIC const struct file_operations g_hw_wifiexcp_fops = {
    .owner = THIS_MODULE,
    .read = hw_wifiexcp_read,
    .unlocked_ioctl = hw_wifiexcp_ioctl,
    .compat_ioctl = hw_wifiexcp_ioctl,
};

OAL_STATIC struct miscdevice g_hw_wifiexcp_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "hwwifiexcp",
    .fops = &g_hw_wifiexcp_fops,
};
/*lint +e64*/
/*
 * This references is the per-PS platform device in the arch/arm/
 * board-xx.c file.
 */
OAL_STATIC oal_int32 ps_probe(struct platform_device *pdev)
{
    struct ps_plat_s *ps_plat_d = NULL;
    oal_int32  err;
#if ((_PRE_LINUX_PLATFORM == K3V5) || (_PRE_LINUX_PLATFORM == MIAMI_C20)) //lint !e553
    struct device_node *np = NULL;
    const char  *pdev_name = NULL;
#endif

    PS_PRINT_FUNCTION_NAME;

#if ((_PRE_LINUX_PLATFORM == K3V5) || (_PRE_LINUX_PLATFORM == MIAMI_C20)) //lint !e553
    // should be the same as dts node compatible property
    np = of_find_compatible_node(NULL, NULL, DTS_COMP_HI1101_PS_NAME);
    if (np == NULL) {
        PS_PRINT_ERR("Unable to find %s\n", DTS_COMP_HI1101_PS_NAME);
        return -ENOENT;
    }

    if (of_property_read_string(np, "hi1102,uart_port", &pdev_name))
    {
        PS_PRINT_ERR("%s node doesn't have uart-dev property!\n", np->name);
        return -ENOENT;
    }
#endif

    ps_plat_d = kzalloc(sizeof(struct ps_plat_s), GFP_KERNEL); //lint !e527
    if (ps_plat_d == NULL) {
        PS_PRINT_ERR("no mem to allocate\n");
        return -ENOMEM;
    }
    dev_set_drvdata(&pdev->dev, ps_plat_d);
    /* get reference of pdev */
    ps_plat_d->pm_pdev = pdev;
    g_hw_ps_device = pdev;

    err = user_ctrl_init();
    if (err != SUCCESS) {
        PS_PRINT_ERR("bfgx_user_ctrl_init failed\n");
        goto user_ctrl_init_fail;
    }
    err = misc_register(&g_hw_debug_device);
    if (err != SUCCESS) {
        PS_PRINT_ERR("Failed to register debug inode\n");
        goto misc_register_fail;
    }

    err = misc_register(&g_hw_wifiexcp_device);
    if (err != 0) {
        PS_PRINT_ERR("Failed to register hw_wifiexcp_device inode\n");
        goto register_wifiexcp_fail;
    }

    // ps_plat_d 不需要释放
    return 0; //lint !e429
register_wifiexcp_fail:
    misc_deregister(&g_hw_debug_device);
misc_register_fail:
    user_ctrl_exit();
user_ctrl_init_fail:
    dev_set_drvdata(&pdev->dev, NULL);
    kfree(ps_plat_d);
    return err;
}


oal_int32 ps_suspend(struct platform_device *pdev, pm_message_t state)
{
    return 0;
}


oal_int32 ps_resume(struct platform_device *pdev)
{
    return 0;
}


OAL_STATIC oal_int32 ps_remove(struct platform_device *pdev)
{
    struct ps_plat_s *ps_plat_d = NULL;

    misc_deregister(&g_hw_debug_device);
    PS_PRINT_INFO("misc debug device have removed\n");
    user_ctrl_exit();

    ps_plat_d = dev_get_drvdata(&pdev->dev);
    if (ps_plat_d != NULL) {
        kfree(ps_plat_d);
        ps_plat_d = NULL;
    } else {
        PS_PRINT_ERR("ps_plat_d is null\n");
    }

    g_hw_ps_device = NULL;

    PS_PRINT_INFO("sysfs user ctrl removed\n");

    return 0;
}
#ifdef _PRE_CONFIG_USE_DTS
static struct of_device_id g_hi1101_ps_match_table[] = {
    {
        .compatible = DTS_COMP_HI1101_PS_NAME,
        .data = NULL,
    },
    { },
};
#endif

/*  platform_driver struct for PS module */
OAL_STATIC struct platform_driver g_ps_platform_driver = {
    .probe      = ps_probe,
    .remove     = ps_remove,
    .suspend    = ps_suspend,
    .resume     = ps_resume,
    .driver     = {
        .name   = "hisi_bfgx",
        .owner  = THIS_MODULE,
#ifdef _PRE_CONFIG_USE_DTS
        .of_match_table = g_hi1101_ps_match_table,
#endif
    },
};

oal_int32 hw_ps_init(void)
{
    oal_int32 ret;

    PS_PRINT_FUNCTION_NAME;

    ret = platform_driver_register(&g_ps_platform_driver);
    if (ret) {
        PS_PRINT_ERR("Unable to register platform bfgx driver.\n");
    }
    return ret;
}

void hw_ps_exit(void)
{
    platform_driver_unregister(&g_ps_platform_driver);
}

