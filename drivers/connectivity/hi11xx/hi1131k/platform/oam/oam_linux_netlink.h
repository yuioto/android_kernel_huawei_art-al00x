
#ifndef __OAM_LINUX_NETLINK_H__
#define __OAM_LINUX_NETLINK_H__

#include "oal_ext_if.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION))

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAM_LINUX_NETLINK_H

#if(_PRE_CONFIG_TARGET_PRODUCT == _PRE_TARGET_PRODUCT_TYPE_WS835DMB)
#define OAM_NETLINK_ID   29         // 1151honor835 修改成29，防止和其他产品的ko加载以及业务运行时创建的netlink产生冲突
#else
#define OAM_NETLINK_ID   25
#endif

typedef struct {
    oal_sock_stru   *pst_nlsk;
    oal_uint32       ul_pid;
} oam_netlink_stru;

typedef struct {
    oal_uint32 (*p_oam_sdt_func)(oal_uint8 *puc_data, oal_uint32 ul_len);
    oal_uint32 (*p_oam_hut_func)(oal_uint8 *puc_data, oal_uint32 ul_len);
    oal_uint32 (*p_oam_alg_func)(oal_uint8 *puc_data, oal_uint32 ul_len);
    oal_uint32 (*p_oam_daq_func)(oal_uint8 *puc_data, oal_uint32 ul_len);
    oal_uint32 (*p_oam_reg_func)(oal_uint8 *puc_data, oal_uint32 ul_len);
    oal_uint32 (*p_oam_acs_func)(oal_uint8 *puc_data, oal_uint32 ul_len);
} oam_netlink_proto_ops;

extern oam_netlink_stru        g_st_netlink;
extern oam_netlink_proto_ops   g_st_netlink_ops;

extern oal_uint32  oam_netlink_kernel_create(oal_void);
extern oal_void oam_netlink_kernel_release(oal_void);

#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)

#endif


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oam_linux_netlink.h */
