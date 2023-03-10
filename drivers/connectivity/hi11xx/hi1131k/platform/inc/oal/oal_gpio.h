

#ifndef __OAL_GPIO_H__
#define __OAL_GPIO_H__

#ifdef _PRE_BOARD_SD5115
#include "hi_drv_common.h"
#include "hi_drv_gpio.h"
#endif
#include "arch/oal_gpio.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAL_GPIO_H

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#ifdef _PRE_BOARD_SD5115
#define oal_gpio_port_e             HI_GPIO_PORT_E
#define oal_gpio_bit_e              HI_GPIO_BIT_E
#define oal_gpio_bit_attr_s         hi_gpio_bit_attr_s
#define oal_gpio_port_attr_s        hi_gpio_port_attr_s
#define oal_level_e                 HI_LEVEL_E
#else
#endif
#ifdef _PRE_BOARD_SD5115
/* GPIO位配置属性操作 */
OAL_STATIC OAL_INLINE oal_ulong oal_gpio_bit_attr_get(oal_gpio_port_e em_port, oal_gpio_bit_e em_bit,
                                                      oal_gpio_bit_attr_s *pst_attr)
{
    return hi_kernel_gpio_bit_attr_get(em_port, em_bit, pst_attr);
}
OAL_STATIC OAL_INLINE oal_ulong oal_gpio_bit_attr_set(oal_gpio_port_e em_port, oal_gpio_bit_e em_bit, 
    oal_gpio_bit_attr_s *pst_attr)
{
    return hi_kernel_gpio_bit_attr_set(em_port, em_bit, pst_attr);
}
/* GPIO端口属性配置,无特殊需要不要关闭GPIO端口时钟，会导致该组GPIO相关的位不能工作 */
OAL_STATIC OAL_INLINE oal_ulong oal_gpio_port_attr_get(oal_gpio_port_e em_port, oal_gpio_port_attr_s *pst_attr)
{
    return hi_kernel_gpio_port_attr_get(em_port, pst_attr);
}
OAL_STATIC OAL_INLINE oal_ulong oal_gpio_port_attr_set(oal_gpio_port_e em_port, oal_gpio_port_attr_s *pst_attr)
{
    return hi_kernel_gpio_port_attr_set(em_port, pst_attr);
}
/* GPIO端口输出 当该GPIO端口(0~7bit)全部配置为输出模式时, 通过此接口配置输出数据 */
OAL_STATIC OAL_INLINE oal_ulong oal_gpio_port_gpio_write(oal_gpio_port_e em_port, uint uc_data)
{
    return hi_kernel_gpio_write(em_port, uc_data);
}
/* GPIO端口输入 当该GPIO端口(0~7bit)全部配置为输入模式时, 通过此接口得到输入数据 */
OAL_STATIC OAL_INLINE oal_ulong oal_gpio_port_gpio_read(oal_gpio_port_e em_port, uint *puc_data)
{
    return hi_kernel_gpio_read(em_port, puc_data);
}
/* GPIO电平设置, 当该GPIO配置为输出模式时,通过此接口配置输出电平 */
OAL_STATIC OAL_INLINE oal_ulong oal_gpio_bit_write(oal_gpio_port_e em_port, oal_gpio_bit_e em_bit,
                                                   oal_level_e em_level)
{
    return hi_kernel_gpio_bit_write(em_port, em_bit, em_level);
}
/* GPIO输入电平获取，当该GPIO配置为输入时,可通过此接口得到输入电平 */
OAL_STATIC OAL_INLINE oal_ulong oal_gpio_bit_read(oal_gpio_port_e em_port, oal_gpio_bit_e em_bit,
                                                  oal_level_e *pem_level)
{
    return hi_kernel_gpio_bit_read(em_port, em_bit, pem_level);
}
#endif /*_PRE_BOARD_SD5115*/

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_ext_if.h */
