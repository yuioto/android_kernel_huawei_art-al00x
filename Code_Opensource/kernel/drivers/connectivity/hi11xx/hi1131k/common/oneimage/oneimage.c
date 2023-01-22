

/*****************************************************************************
  1 Header File Including
*****************************************************************************/
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/printk.h>

#ifdef HISI_NVRAM_SUPPORT
#include "hisi_oneimage.h"
#endif

#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/stat.h>
#include <linux/of.h>
#include <linux/namei.h>

#include "oneimage.h"
#include "plat_board_adapt.h"
#include "plat_debug.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/* global variables */
#ifdef _PRE_CONFIG_USE_DTS
static char g_chip_type[HW_PROC_BUTT][BUFF_LEN];
#else
static char g_chip_type[HW_PROC_BUTT][BUFF_LEN] = {
    {""},
    {"/system/etc/wifi/wpa_supplicant_hisi.conf"},
    {"/system/etc/wifi/p2p_supplicant_hisi.conf"},
    {"/system/bin/hostapd_hisi"},
    {""}
};
#endif

struct hisi_proc_info g_proc_info[] = {
    {.proc_type      = HW_PROC_CHIPTYPE,
     .proc_node_name = DTS_COMP_HW_CONNECTIVITY_NAME,
     .proc_pro_name  = HW_CONN_PROC_CHIPTYPE_FILE},
    {.proc_type      = HW_PROC_SUPP,
     .proc_node_name = DTS_COMP_HW_HISI_SUPP_CONFIG_NAME,
     .proc_pro_name  = HW_CONN_PROC_SUPP_FILE},
    {.proc_type      = HW_PROC_P2P,
     .proc_node_name = DTS_COMP_HW_HISI_P2P_CONFIG_NAME,
     .proc_pro_name  = HW_CONN_PROC_P2P_FILE},
    {.proc_type      = HW_PROC_HOSTAPD,
     .proc_node_name = DTS_COMP_HW_HISI_HOSTAPD_CONFIG_NAME,
     .proc_pro_name  = HW_CONN_PROC_HOSTAPD_FILE},
    {.proc_type      = HW_PROC_FIRMWARE,
     .proc_node_name = DTS_COMP_HW_HISI_FIRMWARE_CONFIG_NAME,
     .proc_pro_name  = HW_CONN_PROC_FRIMWARE},
    {},
};

/*****************************************************************************
  2 Global Variable Definition
*****************************************************************************/
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
bool is_my_chip(void)
{
#ifdef CONFIG_HWCONNECTIVITY
    if (!isMyConnectivityChip(CHIP_TYPE_HI110X)) {
        PS_PRINT_ERR("cfg dev board chip type is not match, skip driver init\n");
        return false;
    } else {
        PS_PRINT_INFO("cfg dev board type is matched with hisi, continue\n");
    }
    return true;
#else
    return true;
#endif
}
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
bool is_my_nfc_chip(void)
{
#ifdef CONFIG_HWCONNECTIVITY
    if (!isMyNfcChip(NFC_CHIP_TYPE_HI110X)) {
        PS_PRINT_ERR("cfg dev board nfc chip type is not match, skip driver init\n");
        return false;
    } else {
        PS_PRINT_INFO("cfg dev board nfc type is matched with hisi_nfc, continue\n");
    }
    return true;
#else
    return true;
#endif
}

bool is_hisi_chiptype(oal_int32 chip)
{
    oal_int32 ret;
    BOARD_INFO *bd_info = NULL;

    if (is_my_chip() == false) {
        PS_PRINT_ERR("chip type is not hisi\n");
        return false;
    }

    ret = board_chiptype_init();
    if (ret == BOARD_FAIL) {
        PS_PRINT_ERR("sub chip type init fail\n");
        return false;
    }

    bd_info = get_board_info();
    if (unlikely(bd_info == NULL)) {
        PS_PRINT_ERR("board info is null\n");
        return false;
    }

    if (chip != bd_info->chip_nr) {
        return false;
    } else {
        return true;
    }
}

EXPORT_SYMBOL(is_hisi_chiptype);
#endif
/*****************************************************************************
  3 Function Definition
*****************************************************************************/
#ifdef _PRE_CONFIG_USE_DTS
int read_from_dts(char *buf, int buf_len, const char *node_name, const char *property_name)
{
    struct device_node *np = NULL;
    char *dts_pro_name = NULL;
    int ret;
    int l_strncpy_ret = EOK;

    if (!buf || buf_len <= 0) {
        printk(KERN_ERR "[HW_CONN] invalid input parameters in %s\n", __func__);
        return -EINVAL;
    }

    np = of_find_compatible_node(NULL, NULL, node_name); // should be the same as dts node compatible property
    if (np == NULL) {
        /* if read fail, default is broadcom */
        printk(KERN_ERR "Unable to find hw_connectivity, using default {%s}\n", node_name);
        l_strncpy_ret += strncpy_s(buf, buf_len, CHIP_TYPE_BCM, (buf_len - 1));
        return 0;
    }

    ret = of_property_read_string(np, (const char*)property_name, (const char **)&dts_pro_name);
    if (ret < 0 || dts_pro_name == NULL) {
        /* if read fail, default is broadcom */
        printk(KERN_ERR "[HW_CONN] %s: get {%s}, ret:%d, using default\n", __func__, property_name, ret);
        l_strncpy_ret += strncpy_s(buf, buf_len, CHIP_TYPE_BCM, (buf_len - 1));
        ret = 0;
    } else {
        l_strncpy_ret += strncpy_s(buf, buf_len, dts_pro_name, (buf_len - 1));
    }
    if (l_strncpy_ret != EOK) {
        printk(KERN_ERR "read_from_dts::strncpy_s failed!");
        return 0;
    }
    return ret;
}
#endif

static ssize_t hwconn_read_proc_chiptype(struct file *filp, char __user *buffer, size_t len, loff_t *off)
{
    printk(KERN_DEBUG "[HW_CONN] hwconn_read_proc_chiptype\n");
    if (len < 1) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype read length = 0.\n");
        return -EINVAL;
    }

    printk(KERN_DEBUG "[HW_CONN] hwconn_read_proc_chiptype off = %lu\n", (unsigned long)*off);
    if ((unsigned long)*off > 0) {
        return 0;
    }


    len = strlen(g_chip_type[HW_PROC_CHIPTYPE]);
    if (len <= 0) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype g_chip_type is blank\n");
        return 0;
    }
    printk(KERN_DEBUG "[HW_CONN] chip type[%s] \n", g_chip_type[HW_PROC_CHIPTYPE]);
    if (copy_to_user(buffer, g_chip_type[HW_PROC_CHIPTYPE], len)) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype copy_to_user failed!\n");
        return -EFAULT;
    }

    *off += len;
    return len;
}

static ssize_t hwconn_read_proc_supp(struct file *filp, char __user *buffer, size_t len, loff_t *off)
{
    printk(KERN_INFO "[HW_CONN] hwconn_read_proc_chiptype\n");
    if (len < 1) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype read length = 0.\n");
        return -EINVAL;
    }

    printk(KERN_DEBUG "[HW_CONN] hwconn_read_proc_chiptype off = %lu\n", (unsigned long)*off);
    if ((unsigned long)*off > 0) {
        return 0;
    }


    len = strlen(g_chip_type[HW_PROC_SUPP]);
    if (len <= 0) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype g_chip_type is blank\n");
        return 0;
    }
    printk(KERN_DEBUG "[HW_CONN] chip type[%s] \n", g_chip_type[HW_PROC_SUPP]);
    if (copy_to_user(buffer, g_chip_type[HW_PROC_SUPP], len)) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype copy_to_user failed!\n");
        return -EFAULT;
    }
    *off += len;
    return len;
}

static ssize_t hwconn_read_proc_p2p(struct file *filp, char __user *buffer, size_t len, loff_t *off)
{
    printk(KERN_DEBUG "[HW_CONN] hwconn_read_proc_chiptype\n");
    if (len < 1) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype or read length = 0.\n");
        return -EINVAL;
    }

    printk(KERN_DEBUG "[HW_CONN] hwconn_read_proc_chiptype off = %lu\n", (unsigned long)*off);
    if ((unsigned long)*off > 0) {
        return 0;
    }


    len = strlen(g_chip_type[HW_PROC_P2P]);
    if (len <= 0) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype g_chip_type is blank\n");
        return 0;
    }
    printk(KERN_DEBUG "[HW_CONN] chip type[%s] \n", g_chip_type[HW_PROC_P2P]);
    if (copy_to_user(buffer, g_chip_type[HW_PROC_P2P], len)) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype copy_to_user failed!\n");
        return -EFAULT;
    }
    *off += len;
    return len;
}

static ssize_t hwconn_read_proc_apd(struct file *filp, char __user *buffer, size_t len, loff_t *off)
{
    printk(KERN_DEBUG "[HW_CONN] hwconn_read_proc_chiptype\n");
    if (len < 1) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype or read length = 0.\n");
        return -EINVAL;
    }

    printk(KERN_DEBUG "[HW_CONN] hwconn_read_proc_chiptype off = %lu\n", (unsigned long)*off);
    if ((unsigned long)*off > 0) {
        return 0;
    }


    len = strlen(g_chip_type[HW_PROC_HOSTAPD]);
    if (len <= 0) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype g_chip_type is blank\n");
        return 0;
    }
    printk(KERN_DEBUG "[HW_CONN] chip type[%s] \n", g_chip_type[HW_PROC_HOSTAPD]);
    if (copy_to_user(buffer, g_chip_type[HW_PROC_HOSTAPD], len)) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype copy_to_user failed!\n");
        return -EFAULT;
    }
    *off += len;
    return len;
}

static ssize_t hwconn_read_proc_firmware(struct file *filp, char __user *buffer, size_t len, loff_t *off)
{
    printk(KERN_DEBUG "[HW_CONN] hwconn_read_proc_chiptype\n");
    if (len < 1) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype read length = 0.\n");
        return -EINVAL;
    }

    printk(KERN_DEBUG "[HW_CONN] hwconn_read_proc_chiptype off = %lu\n", (unsigned long)*off);
    if ((unsigned long)*off > 0) {
        return 0;
    }


    len = strlen(g_chip_type[HW_PROC_FIRMWARE]);
    if (len <= 0) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype g_chip_type is blank\n");
        return 0;
    }
    printk(KERN_DEBUG "[HW_CONN] chip type[%s] \n", g_chip_type[HW_PROC_FIRMWARE]);
    if (copy_to_user(buffer, g_chip_type[HW_PROC_FIRMWARE], len)) {
        printk(KERN_ERR "[HW_CONN] hwconn_read_proc_chiptype copy_to_user failed!\n");
        return -EFAULT;
    }
    *off += len;
    return len;
}

/*lint -e64*/
static const struct file_operations hwconn_proc_fops[]= {
    {
        .owner = THIS_MODULE,
        .read = hwconn_read_proc_chiptype,
    },
    {
        .owner = THIS_MODULE,
        .read = hwconn_read_proc_supp,
    },
    {
        .owner = THIS_MODULE,
        .read = hwconn_read_proc_p2p,
    },
    {
        .owner = THIS_MODULE,
        .read = hwconn_read_proc_apd,
    },
        {
        .owner = THIS_MODULE,
        .read = hwconn_read_proc_firmware,
    },
    {},
};
/*lint +e64*/
static int create_hwconn_proc_file(void)
{
    int ret;
    int index;
    char node_path[NODE_PATH_LEN];
    struct path                         path;
    struct proc_dir_entry *hwconn_dir = NULL;
    struct proc_dir_entry *hwconn_chiptype_file = NULL;

    ret = kern_path(PROC_CONN_DIR, LOOKUP_FOLLOW, &path);
    if (ret) {
        hwconn_dir = proc_mkdir(HW_CONN_PROC_DIR, NULL);
        if (hwconn_dir == NULL) {
            printk(KERN_ERR "[HW_CONN] proc dir create failed\n");
            ret = -ENOMEM;
            return ret;
        }

        index = HW_PROC_CHIPTYPE;
    } else {
        index = HW_PROC_SUPP;
    }

    for (; index < HW_PROC_BUTT; index++) {
#ifdef _PRE_CONFIG_USE_DTS
        memset_s(g_chip_type[index], sizeof(g_chip_type[index]), 0, sizeof(g_chip_type[index]));
        ret = read_from_dts(g_chip_type[index], sizeof(g_chip_type[index]), g_proc_info[index].proc_node_name,
                            g_proc_info[index].proc_pro_name);
        if (ret < 0) {
            printk(KERN_INFO "[HW_CONN read info from dts fail\n");
            continue;
        }
#endif
        if (hwconn_dir != NULL) {
            hwconn_chiptype_file = proc_create(g_proc_info[index].proc_pro_name,
                S_IRUGO, hwconn_dir, &hwconn_proc_fops[index]); //lint !e64
        } else {
            memset_s(node_path, NODE_PATH_LEN, 0, NODE_PATH_LEN);
            if (snprintf_s(node_path, NODE_PATH_LEN, NODE_PATH_LEN - 1, "%s/%s",
                           HW_CONN_PROC_DIR, g_proc_info[index].proc_pro_name) < EOK) {
                printk(KERN_ERR "[HW_CONN]read node path failed! fail\n");
            }
            printk(KERN_DEBUG "[HW_CONN node_path[%s]\n", node_path);
            hwconn_chiptype_file = proc_create(node_path, S_IRUGO, NULL, &hwconn_proc_fops[index]); //lint !e64
        }

        if (hwconn_chiptype_file == NULL) {
            printk(KERN_ERR "[HW_CONN] proc info file create failed\n");
            ret = -ENOMEM;
            return ret;
        }
    }

    return 0;
}

static int hw_connectivity_probe(struct platform_device *pdev)
{
    int ret;

    ret = create_hwconn_proc_file();
    if (ret) {
        printk(KERN_ERR "[HW_CONN] create proc file failed.\n");
        return ret;
    }

    return 0;
}

static void hw_connectivity_shutdown(struct platform_device *pdev)
{
    printk(KERN_INFO "[HW_CONN] hw_connectivity_shutdown!\n");
    return;
}

static const struct of_device_id hw_connectivity_match_table[] = {
    {
        .compatible = DTS_COMP_HW_CONNECTIVITY_NAME,   // compatible must match with which defined in dts
        .data = NULL,
    },
    {},
};

static struct platform_driver hw_connectivity_driver = {
    .probe          = hw_connectivity_probe,
    .suspend        = NULL,
    .remove         = NULL,
    .shutdown       = hw_connectivity_shutdown,
    .driver = {
        .name = DTS_COMP_HW_CONNECTIVITY_NAME,
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(hw_connectivity_match_table), // dts required code
    },
};

int hw_misc_connectivity_init(void)
{
    printk(KERN_ERR "[HW_CONN] hw_connectivity_init enter\n");
    return platform_driver_register(&hw_connectivity_driver);
}

void hw_misc_connectivity_exit(void)
{
    platform_driver_unregister(&hw_connectivity_driver);
}

#ifdef _PRE_CONFIG_USE_DTS
int read_nfc_conf_name_from_dts(char *buf, int buf_len, const char *node_name, const char *property_name)
{
    return read_from_dts(buf, buf_len, node_name, property_name);
}
#endif

MODULE_AUTHOR("DRIVER_AUTHOR");
MODULE_DESCRIPTION("Huawei connectivity OneTrack driver");
MODULE_LICENSE("GPL");

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
