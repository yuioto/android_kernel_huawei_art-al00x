

#ifndef __OAL_BUS_IF_H__
#define __OAL_BUS_IF_H__

#include "oal_mem.h"
#include "oal_schedule.h"
#include "oal_util.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#define OAL_MAX_BAR_NUM          6          /* 每个设备最大BAR数量 */
/* 总线设备，DEVICE级 */
typedef struct {
    oal_uint32       ul_device_id;                       /* 设备号,如0x1151 */
    oal_void        *p_dev;                              /* 指向设备的指针 */
    oal_uint32       ul_irq_num;                         /* 设备中断号 */
    oal_uint8        uc_chip_id;                         /* PCIe号，如PCIe0 */
    oal_uint8        uc_bar_num;                         /* BAR数量 */
    oal_uint8        auc_resv[2]; // 预留2个byte，同时用于对齐
    oal_uint32       aul_mem_start[1];
    oal_irq_dev_stru st_irq_info;
} oal_bus_dev_stru;

/* 总线设备，CHIP级 */
typedef struct {
    oal_uint8           uc_chip_id;                                /* PCIe号，如PCIe0 */
    oal_uint8        uc_device_num; /* 该芯片中device个数，应该小于WLAN_DEVICE_MAX_NUM_PER_CHIP */
    oal_uint8        auc_resv[2]; // 预留2个byte，同时用于对齐
    oal_pci_dev_stru   *pst_pci_device;                            /* 指向所属PCI设备的指针 */
    oal_void           *p_pci_dbi_base;                            /* 指向PCI设备DBI接口的指针，用于配置PCIE内部寄存器 */
    oal_uint32       aul_mem_start[1];
    /* device信息 */
    oal_bus_dev_stru st_bus_dev[WLAN_DEVICE_MAX_NUM_PER_CHIP];
} oal_bus_chip_stru;

extern oal_uint8  oal_bus_get_chip_num(oal_void);
extern oal_uint32  oal_bus_inc_chip_num(oal_void);
extern oal_void oal_bus_init_chip_num(oal_void);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_bus_if.h */
