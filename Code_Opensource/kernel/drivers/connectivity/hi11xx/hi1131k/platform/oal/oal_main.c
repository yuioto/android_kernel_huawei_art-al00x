
#include "oal_main.h"
#include "oal_mem.h"
#include "oal_schedule.h"
#include "oal_net.h"
#include "securec.h"

#include "oal_hcc_host_if.h"
#include "oal_kernel_file.h"

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
#ifndef WIN32
#include "plat_firmware.h"
#endif
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)||(_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
void __iomem *g_l2cache_base;
#endif


oal_int32  ATTR_OAL_NO_FUNC_TRACE oal_main_init(oal_void)
{
    oal_uint32  ul_rslt;
#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT
    if (oal_conn_sysfs_root_obj_init() == NULL) {
        OAL_IO_PRINT("hisi root sysfs init failed\n");
    }
#endif

    /* 为了解各模块的启动时间，增加时间戳打印 */
    /* 内存池初始化 */
    ul_rslt = oal_mem_init_pool();
    if (ul_rslt != OAL_SUCC) {
        OAL_IO_PRINT("oal_main_init: oal_mem_init_pool return error code: %d", ul_rslt);
        OAL_WARN_ON(1);
        return -OAL_EFAIL;
    }

#if ((_PRE_PRODUCT_ID_HI1131C_HOST == _PRE_PRODUCT_ID) || (_PRE_PRODUCT_ID_HI1131C_DEV ==_PRE_PRODUCT_ID))
    /* Hi1102 SDIO总线初始化接口 TBD */
    /* 初始化: 总线上的chip数量增加1 */
    oal_bus_init_chip_num();
    ul_rslt = oal_bus_inc_chip_num();
    if (ul_rslt != OAL_SUCC) {
        OAL_IO_PRINT("oal_pci_probe: oal_bus_inc_chip_num failed!\n");
        OAL_WARN_ON(1);
        return -OAL_EIO;
    }
#endif

    /* 启动成功 */
#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
    memset_s(g_past_net_device, WLAN_VAP_SUPPORT_MAX_NUM_LIMIT * OAL_SIZEOF(oal_net_device_stru *),
             0, WLAN_VAP_SUPPORT_MAX_NUM_LIMIT * OAL_SIZEOF(oal_net_device_stru *));
#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
    /* HCC初始化 */
    if (OAL_UNLIKELY(hcc_module_init() == NULL)) {
        OAL_IO_PRINT("[ERROR]hcc_module_init return err null\n");
        return -OAL_EFAIL;
    }
#endif

#ifdef _PRE_CONFIG_HISI_CONN_SOFTWDFT
    if (OAL_UNLIKELY(oal_softwdt_init() != OAL_SUCC)) {
        OAL_IO_PRINT("oal_softwdt_init init failed!\n");
        return -OAL_EFAIL;
    }
#endif

#ifdef _PRE_OAL_FEATURE_KEY_PROCESS_TRACE
    if (OAL_UNLIKELY(oal_dft_init() != OAL_SUCC)) {
        OAL_IO_PRINT("oal_dft_init init failed!\n");
        return -OAL_EFAIL;
    }
#endif

    return OAL_SUCC;
}


oal_void  ATTR_OAL_NO_FUNC_TRACE oal_main_exit(oal_void)
{
#ifdef _PRE_OAL_FEATURE_KEY_PROCESS_TRACE
    oal_dft_exit();
#endif

#ifdef _PRE_CONFIG_HISI_CONN_SOFTWDFT
    oal_softwdt_exit();
#endif

    hcc_module_exit(hcc_get_default_handler());

#if ((_PRE_PRODUCT_ID_HI1131C_HOST == _PRE_PRODUCT_ID) || (_PRE_PRODUCT_ID_HI1131C_DEV ==_PRE_PRODUCT_ID))
    /* Hi1102 SDIO总线exit接口(不下电) TBD */
    /* chip num初始化:0 */
    oal_bus_init_chip_num();
#endif

    /* 内存池卸载 */
    oal_mem_exit();
#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT
    oal_conn_sysfs_root_boot_obj_exit();
    oal_conn_sysfs_root_obj_exit();
#endif

    return ;
}

#if 0

void ATTR_OAL_NO_FUNC_TRACE __cyg_profile_func_enter(void *this_func, void *call_site)
{

}


void ATTR_OAL_NO_FUNC_TRACE __cyg_profile_func_exit(void *this_func, void *call_site)
{

}
#endif

oal_uint32  oal_chip_get_version(oal_void)
{
    oal_uint32 ul_chip_ver;

#if (_PRE_WLAN_REAL_CHIP == _PRE_WLAN_CHIP_SIM)

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
    /* 1102 02需要SOC提供寄存器后实现 */
    ul_chip_ver   = WLAN_CHIP_VERSION_HI1151V100H;
#endif

#else /* else _PRE_WLAN_REAL_CHIP != _PRE_WLAN_CHIP_SIM */

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
    /* 1102 02需要SOC提供寄存器后实现 */
    ul_chip_ver   = WLAN_CHIP_VERSION_HI1151V100H;
#endif

#endif
    return ul_chip_ver;
}

oal_uint8 oal_chip_get_device_num(oal_uint32   ul_chip_ver)
{
    oal_uint8   uc_device_nums = 0;

    switch  (ul_chip_ver) {
        case WLAN_CHIP_VERSION_HI1151V100H:
        case WLAN_CHIP_VERSION_HI1151V100L:
            uc_device_nums = WLAN_CHIP_DBSC_DEVICE_NUM;
            break;

        default:
            uc_device_nums = 0;
            break;
    }

    return uc_device_nums;
}

/*lint -e578*//*lint -e19*/
oal_module_symbol(oal_chip_get_version);
oal_module_symbol(oal_chip_get_device_num);
oal_module_symbol(oal_main_init);
oal_module_symbol(oal_main_exit);
oal_module_symbol(g_l2cache_base);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

