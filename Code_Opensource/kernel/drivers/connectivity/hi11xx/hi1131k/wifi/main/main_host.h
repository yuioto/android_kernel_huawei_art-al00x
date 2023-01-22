

#ifndef __MAIN_HOST_H__
#define __MAIN_HOST_H__

#include "oal_ext_if.h"
#include "oam_ext_if.h"
#include "frw_ext_if.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAIN_H

extern oal_int32 hi1102_host_main_init(oal_void);
extern oal_void hi1102_host_main_exit(oal_void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
