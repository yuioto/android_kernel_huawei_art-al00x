

#define HISI_LOG_TAG "[plat_init]"
#include "plat_main.h"
#ifndef WIN32
#include "plat_board_adapt.h"
#endif
#include "plat_firmware.h"
#include "plat_debug.h"
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "plat_debug.h"
#include "oneimage.h"
#include "plat_pm.h"
#include "oal_kernel_file.h"
#include "oal_util.h"
#include "hisi_ini.h"
#include "oam_ext_if.h"
#include "user_ctrl.h"
#endif
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_PLAT_MAIN_C

int plat_exception_reset_init(void);
int plat_exception_reset_exit(void);
int low_power_init(void);
void  low_power_exit(void);
int hw_ps_init(void);
void hw_ps_exit(void);

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
int hw_misc_connectivity_init(void);
void hw_misc_connectivity_exit(void);
#endif

#ifdef CONFIG_HI1102_PLAT_HW_CHR
int chr_miscdevs_init(void);
void chr_miscdevs_exit(void);
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
int isAsic(void)
{
    if ((get_board_info())->is_asic == VERSION_ASIC) {
        return VERSION_ASIC;
    } else {
        return VERSION_FPGA;
    }
}

EXPORT_SYMBOL_GPL(isAsic);
#else
int isAsic(void)
{
    return 1;
}
#endif


oal_int32  plat_init(oal_void)
{
    oal_int32   l_return;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifdef _PRE_CONFIG_USE_DTS
    if (is_my_chip() == false) {
        return OAL_SUCC;
    }
#endif
#endif

#ifdef CONFIG_HUAWEI_DSM
#if(_PRE_LINUX_PLATFORM == MIAMI_C60)
    hw_1131k_register_dsm_client();
#endif
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    l_return = ini_cfg_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("plat_init: customize init fail\r\n");
        goto customize_init_failed;
    }
#endif

    l_return = plat_exception_reset_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("plat_init: plat_exception_reset_init fail\r\n");
        goto plat_exception_rst_init_fail;
    }

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifdef CONFIG_HI1102_PLAT_HW_CHR
    l_return = chr_miscdevs_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("plat_init: chr_miscdev_init return error code: %d\r\n", l_return);

        goto chr_miscdevs_init_fail;
    }
#endif
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    l_return = hi110x_board_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("plat_init: hi110x_board_init fail\r\n");
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_INIT, CHR_PLAT_DRV_ERROR_PLAT_INIT);
        goto board_init_fail;
    }
#endif

    l_return = oal_main_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("plat_init: oal_main_init return error code: %d\r\n", l_return);
        goto oal_main_init_fail;
    }

#ifdef _PRE_CONFIG_USE_DTS
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    l_return = hw_misc_connectivity_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("plat_init: hw_misc_connectivity_init return error code: %d\r\n", l_return);
        goto  hw_misc_connectivity_init_fail;
    }
#endif
#endif

    l_return = oam_main_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("plat_init: oam_main_init return error code: %d\r\n", l_return);
        goto oam_main_init_fail;
    }

    l_return = sdt_drv_main_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("plat_init: sdt_drv_main_init return error code: %d\r\n", l_return);
        goto sdt_drv_main_init_fail;
    }

    l_return = frw_main_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("plat_init: frw_main_init return error code: %d\r\n", l_return);
        goto frw_main_init_fail;
    }

    l_return = low_power_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("plat_init: low_power_init return error code: %d\r\n", l_return);
        goto low_power_init_fail;
    }

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifdef _PRE_CONFIG_USE_DTS
    l_return = hw_ps_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("plat_init: hw_ps_init return error code: %d\r\n", l_return);
        goto hw_ps_init_fail;
    }
#else
    l_return = user_ctrl_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("plat_init: user_ctrl_init return error code: %d\r\n", l_return);
        goto user_ctrl_init_fail;
    }
#endif
#endif

    l_return = plat_firmware_init();
    if (l_return != OAL_SUCC) {
        OAL_IO_PRINT("plat_init: plat_firmware_init return error code: %d\r\n", l_return);
        goto firmware_cfg_init_fail;
    }

    /* 启动完成后，输出打印 */
    PS_PRINT_WARNING("plat_init:: platform_main_init finish!\r\n");

    return OAL_SUCC;

firmware_cfg_init_fail:
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifdef _PRE_CONFIG_USE_DTS
    hw_ps_exit();
hw_ps_init_fail:
#else
    user_ctrl_exit();
user_ctrl_init_fail:
#endif
#endif

    low_power_exit();
low_power_init_fail:
    frw_main_exit();
frw_main_init_fail:
    sdt_drv_main_exit();
sdt_drv_main_init_fail:
    oam_main_exit();
oam_main_init_fail:

#ifdef _PRE_CONFIG_USE_DTS
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    hw_misc_connectivity_exit();
hw_misc_connectivity_init_fail:
#endif
#endif
    oal_main_exit();
oal_main_init_fail:
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    hi110x_board_exit();
board_init_fail:
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifdef CONFIG_HI1102_PLAT_HW_CHR
    chr_miscdevs_exit();
chr_miscdevs_init_fail:
#endif
    plat_exception_reset_exit();
#endif

plat_exception_rst_init_fail:
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    ini_cfg_exit();
customize_init_failed:
#endif

    return l_return;
}


oal_void plat_exit(oal_void)
{
    plat_firmware_clear();

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifdef _PRE_CONFIG_USE_DTS
    hw_ps_exit();
#else
    user_ctrl_exit();
#endif
#endif

    low_power_exit();
    frw_main_exit();

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    sdt_drv_main_exit();
#endif

    oam_main_exit();

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifdef _PRE_CONFIG_USE_DTS
    hw_misc_connectivity_exit();
#endif
#endif

    oal_main_exit();

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    hi110x_board_exit();
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifdef CONFIG_HI1102_PLAT_HW_CHR
    chr_miscdevs_exit();
#endif
#endif

    plat_exception_reset_exit();

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    ini_cfg_exit();
#endif

#ifdef CONFIG_HUAWEI_DSM
#if(_PRE_LINUX_PLATFORM == MIAMI_C60)
    hw_1131k_unregister_dsm_client();
#endif
#endif

    return;
}

/*lint -e578*//*lint -e19*/
#if (_PRE_PRODUCT_ID_HI1131C_HOST == _PRE_PRODUCT_ID)
#ifndef CONFIG_HI110X_KERNEL_MODULES_BUILD_SUPPORT
#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT
oal_int32 g_plat_init_flag = 0;
oal_int32 g_plat_init_ret;
/* built-in */
OAL_STATIC ssize_t  plat_sysfs_set_init(struct kobject *dev, struct kobj_attribute *attr, const char *buf, size_t count)
{
    char            mode[128] = {0};
    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    if ((sscanf_s(buf, "%20s", mode, sizeof(mode)) != 1)) {
        OAL_IO_PRINT("set value one param!\n");
        return -OAL_EINVAL;
    }

    if (sysfs_streq("init", mode)) {
        /* init */
        if (g_plat_init_flag == 0) {
            g_plat_init_ret = plat_init();
            g_plat_init_flag = 1;
        } else {
            OAL_IO_PRINT("double init!\n");
        }
    } else {
        OAL_IO_PRINT("invalid input:%s\n", mode);
    }

    return count;
}

OAL_STATIC ssize_t  plat_sysfs_get_init(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int ret = 0;
    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    if (g_plat_init_flag == 1) {
        if (g_plat_init_ret == OAL_SUCC) {
            ret +=  snprintf_s(buf + ret, OAL_PAGE_SIZE - ret, OAL_PAGE_SIZE - ret - 1, "running\n");
        } else {
            ret +=  snprintf_s(buf + ret,
                               OAL_PAGE_SIZE - ret,
                               OAL_PAGE_SIZE - ret - 1,
                               "boot failed ret=%d\n",
                               g_plat_init_ret);
        }
    } else {
        ret +=  snprintf_s(buf + ret, OAL_PAGE_SIZE - ret, OAL_PAGE_SIZE - ret - 1, "uninit\n");
    }
    if (ret < EOK) {
        OAL_IO_PRINT("plat_sysfs_get_init::snprintf_s failed! \n");
        return ret;
    }
    return ret;
}
OAL_STATIC struct kobj_attribute dev_attr_plat =
    __ATTR(plat, S_IRUGO | S_IWUSR, plat_sysfs_get_init, plat_sysfs_set_init);
OAL_STATIC struct attribute *plat_init_sysfs_entries[] = {
    &dev_attr_plat.attr,
    NULL
};

OAL_STATIC struct attribute_group plat_init_attribute_group = {
    .attrs = plat_init_sysfs_entries,
};

oal_int32  plat_sysfs_init(oal_void)
{
    oal_int32 ret;
    oal_kobject*     pst_root_boot_object = NULL;

    if (is_hisi_chiptype(BOARD_VERSION_HI1131K) == false) {
        OAL_IO_PRINT("plat_sysfs_init: chiptype is not hi1131k.");
        return OAL_SUCC;
    }
    OAL_IO_PRINT("plat_sysfs_init: chiptype is hi1131k.");

    pst_root_boot_object = oal_get_sysfs_root_boot_object();
    if (pst_root_boot_object == NULL) {
        OAL_IO_PRINT("[E]get root boot sysfs object failed!\n");
        return -OAL_EBUSY;
    }
    ret = sysfs_create_group(pst_root_boot_object, &plat_init_attribute_group);
    if (ret) {
        OAL_IO_PRINT("sysfs create plat boot group fail.ret=%d\n", ret);
        ret = -OAL_ENOMEM;
        return ret;
    }

    return ret;
}

oal_void  plat_sysfs_exit(oal_void)
{
    /* need't exit,built-in */
    return;
}
oal_module_init(plat_sysfs_init);
oal_module_exit(plat_sysfs_exit);
#endif
#else
// #ifdef CONFIG_HI1102_PLATFORM_MODULE
oal_module_init(plat_init);
oal_module_exit(plat_exit);
#endif
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

