

#ifndef __OAL_EXT_IF_H__
#define __OAL_EXT_IF_H__

#include "platform_oneimage_define.h"
#include "oal_types.h"
#include "oal_util.h"
#include "oal_hardware.h"
#include "oal_schedule.h"
#include "oal_bus_if.h"
#include "oal_mem.h"
#include "oal_net.h"
#include "oal_list.h"
#include "oal_queue.h"
#include "oal_workqueue.h"
#include "arch/oal_ext_if.h"
#include "oal_thread.h"

#if (_PRE_PRODUCT_ID !=_PRE_PRODUCT_ID_HI1131C_DEV)
    #include "oal_aes.h"
    #include "oal_gpio.h"
#endif
/* infusion检查对预编译宏支持不好，在此定义宏支持infusion检查，正式代码不需要 */
#ifdef _PRE_INFUSION_CHECK
    #include "oal_infusion.h"
#endif
/* end infusion */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    #include "exception_rst.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

typedef enum {
    OAL_TRACE_ENTER_FUNC,
    OAL_TRACE_EXIT_FUNC,

    OAL_TRACE_DIRECT_BUTT
} oal_trace_direction_enum;
typedef oal_uint8 oal_trace_direction_enum_uint8;

extern oal_void                       *g_pst_5115_sys_ctl;
extern oal_void                       *g_pst_5610_mode;
extern oal_void                       *g_pst_5610_gpio;
extern oal_int32   oal_main_init(oal_void);
extern oal_void    oal_main_exit(oal_void);
extern oal_uint32 oal_chip_get_version(oal_void);
extern oal_uint8 oal_chip_get_device_num(oal_uint32 ul_chip_ver);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_ext_if.h */
