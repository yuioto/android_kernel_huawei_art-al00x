/*
 * 版权所有 (c) 华为技术有限公司 2001-2020
 * 功能说明 : heartbeat_host.c include file
 */
#ifndef __HEARTBEAT_HOST_H__
#define __HEARTBEAT_HOST_H__
/*****************************************************************************
  1 Include other Head file
*****************************************************************************/
#include "plat_type.h"

/*****************************************************************************
  6 EXTERN FUNCTION
*****************************************************************************/
extern void set_heartbeat_cfg(oal_int32 cfg);
extern oal_int32 stop_heartbeat (void);
extern oal_int32 start_heartbeat(void);
extern oal_int32 update_heartbeat(void);
extern oal_int32 heart_beat_init(void);
extern void heart_beat_release(void);

#endif


