
/*****************************************************************************
  1 Header File Including
*****************************************************************************/
#include "plat_debug.h"
#include "plat_pm.h"

/*****************************************************************************
  2 Global Variable Definition
*****************************************************************************/
OAL_STATIC struct kobject *g_sysfs_hi110x_power = NULL;

/*****************************************************************************
  3 Function Definition
*****************************************************************************/
OAL_STATIC ssize_t bt_power_state_show(struct kobject *kobj,
                                       struct kobj_attribute *attr, char *buf)
{
    return 0;
}

OAL_STATIC struct kobj_attribute bt_power_attr = {
    .attr = {
        .name = "bt_power_state",
        .mode = S_IRUGO | S_IWUSR,
    },
    .show   = bt_power_state_show,
};

OAL_STATIC ssize_t fm_power_state_show(struct kobject *kobj,
                                       struct kobj_attribute *attr, char *buf)
{
    return 0;
}

OAL_STATIC struct kobj_attribute fm_power_attr = {
    .attr = {
        .name = "fm_power_state",
        .mode = S_IRUGO | S_IWUSR,
    },
    .show   = fm_power_state_show,
};

OAL_STATIC ssize_t gnss_power_state_show(struct kobject *kobj,
                                         struct kobj_attribute *attr, char *buf)
{
    return 0;
}

OAL_STATIC struct kobj_attribute gnss_power_attr = {
    .attr = {
        .name = "gnss_power_state",
        .mode = S_IRUGO | S_IWUSR,
    },
    .show   = gnss_power_state_show,
};

OAL_STATIC ssize_t nfc_power_state_show(struct kobject *kobj,
                                        struct kobj_attribute *attr, char *buf)
{
    return 0;
}

OAL_STATIC struct kobj_attribute nfc_power_attr = {
    .attr = {
        .name = "nfc_power_state",
        .mode = S_IRUGO | S_IWUSR,
    },
    .show   = nfc_power_state_show,
};

OAL_STATIC ssize_t ir_power_state_show(struct kobject *kobj,
                                       struct kobj_attribute *attr, char *buf)
{
    return 0;
}

OAL_STATIC struct kobj_attribute ir_power_attr = {
    .attr = {
        .name = "ir_power_state",
        .mode = S_IRUGO | S_IWUSR,
    },
    .show   = ir_power_state_show,
};

OAL_STATIC ssize_t bfg_sleep_state_show(struct kobject *kobj,
                                        struct kobj_attribute *attr, char *buf)
{
    struct pm_drv_data *pm_data = pm_get_drvdata();
    if (NULL == pm_data) {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    return sprintf_s(buf, PAGE_SIZE, "%d\n", pm_data->bfgx_dev_state);
}

OAL_STATIC struct kobj_attribute bfg_sleep_attr = {
    .attr = {
        .name = "bfg_sleep_state",
        .mode = S_IRUGO | S_IWUSR,
    },
    .show = bfg_sleep_state_show,
};

OAL_STATIC ssize_t bfg_wakeup_host_show(struct kobject *kobj,
                                        struct kobj_attribute *attr, char *buf)
{
    int32_t len;
    struct pm_drv_data *pm_data = pm_get_drvdata();

    if (NULL == pm_data) {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    len = sprintf_s(buf, PAGE_SIZE, "%d\n", pm_data->bfg_wakeup_host);
    if (len < 0) {
        PS_PRINT_ERR("bfg_wakeup_host_show::sprintf_s failed!\n");
    }
    return len;
}
OAL_STATIC struct kobj_attribute bfg_wakeup_host_attr = {
    .attr = {
        .name = "bfg_wakeup_host",
        .mode = S_IRUGO | S_IWUSR,
    },
    .show = bfg_wakeup_host_show,
};
OAL_STATIC struct attribute *bfg_power_attrs[] = {
    &bt_power_attr.attr,
    &fm_power_attr.attr,
    &gnss_power_attr.attr,
    &nfc_power_attr.attr,
    &ir_power_attr.attr,
    &bfg_sleep_attr.attr,
    &bfg_wakeup_host_attr.attr,
    NULL
};

OAL_STATIC struct attribute_group pm_state_group = {
    .attrs = bfg_power_attrs,
};

oal_int32 pm_user_ctrl_init(void)
{
    oal_int32 ret;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    g_sysfs_hi110x_power = kobject_create_and_add("hi110x_power", NULL);
    if (g_sysfs_hi110x_power == NULL) {
        PS_PRINT_ERR("Failed to creat g_sysfs_hi110x_power !!!\n ");
        return -ENOMEM;
    }

    ret = sysfs_create_group(g_sysfs_hi110x_power, &pm_state_group);
    if (ret) {
        PS_PRINT_ERR("failed to create pm properties!\n");
    }
#endif
    return ret;
}


void pm_user_ctrl_exit(void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    sysfs_remove_group(g_sysfs_hi110x_power, &pm_state_group);
    kobject_put(g_sysfs_hi110x_power);
    g_sysfs_hi110x_power = NULL;
    return;
#endif
}
