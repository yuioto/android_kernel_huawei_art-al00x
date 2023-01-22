
/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "plat_firmware.h"
#include "plat_cali.h"
#include "plat_debug.h"

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define RF_CALI_DATA_BUF_LEN  (sizeof(oal_cali_param_stru))

/*****************************************************************************
  3 全局变量定义
*****************************************************************************/
/* 保存校准数据的buf */
oal_uint8 *g_pucCaliDataBuf = NULL;
oal_uint8 g_uc_netdev_is_open = OAL_FALSE;
host_cali_stru g_host_cali_param;
uint8_t g_wifi_cali_permission = OAL_FALSE;
/*****************************************************************************
  4 函数实现
*****************************************************************************/

oal_int32 get_cali_count(oal_uint32 *count)
{
    oal_cali_param_stru *pst_cali_data = NULL;
    oal_uint16 cali_count;
    oal_uint32 cali_parm;

    if (count == NULL) {
        PS_PRINT_ERR("count is NULL\n");
        return -OAL_EFAIL;
    }

    if (g_pucCaliDataBuf == NULL) {
        PS_PRINT_ERR("g_pucCaliDataBuf is NULL\n");
        return -OAL_EFAIL;
    }

    pst_cali_data = (oal_cali_param_stru *)g_pucCaliDataBuf;
    cali_count    = pst_cali_data->st_cali_update_info.ul_cali_time;
    cali_parm     = *(oal_uint32 *)&(pst_cali_data->st_cali_update_info);

    PS_PRINT_WARNING("cali count is [%d], cali update info is [%d]\n", cali_count, cali_parm);
    *count = cali_parm;

    return OAL_SUCC;
}

void *get_cali_data_buf_addr(void)
{
    return (void *)&g_host_cali_param;
}

void clear_cali_data_buff(void)
{
    memset_s(&g_host_cali_param, OAL_SIZEOF(host_cali_stru), 0, OAL_SIZEOF(host_cali_stru));
}

uint8_t api_get_wifi_cali_apply(void)
{
    return g_wifi_cali_permission;
}

void api_set_wifi_cali_apply(uint8_t permission)
{
    g_wifi_cali_permission = permission;
}
EXPORT_SYMBOL(get_cali_data_buf_addr);
EXPORT_SYMBOL(g_uc_netdev_is_open);
EXPORT_SYMBOL(clear_cali_data_buff);
EXPORT_SYMBOL(api_get_wifi_cali_apply);
EXPORT_SYMBOL(api_set_wifi_cali_apply);

oal_int32 cali_data_buf_malloc(void)
{
    oal_uint8 *buffer = NULL;

    buffer = oal_kzalloc(RF_CALI_DATA_BUF_LEN, OAL_GFP_KERNEL);
    if (buffer == NULL) {
        return -OAL_EFAIL;
    }
    g_pucCaliDataBuf = buffer;

    return OAL_SUCC;
}


void cali_data_buf_free(void)
{
    if (g_pucCaliDataBuf != NULL) {
        oal_free(g_pucCaliDataBuf);
    }
    g_pucCaliDataBuf = NULL;
}


