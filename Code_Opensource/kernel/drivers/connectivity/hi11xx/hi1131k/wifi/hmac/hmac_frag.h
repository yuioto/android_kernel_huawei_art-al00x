
#ifndef __HMAC_FRAG_H__
#define __HMAC_FRAG_H__


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "hmac_main.h"
#include "hmac_tx_data.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_FRAG_H
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define HMAC_FRAG_TIMEOUT   2000
#define HMAC_MAX_FRAG_SIZE  2500

/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_netbuf_stru* hmac_defrag_process(hmac_user_stru *pst_hmac_user, oal_netbuf_stru *pst_netbuf,
                                            oal_uint32 ul_hrdsize);
extern oal_uint32  hmac_frag_process_proc(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user,
                                          oal_netbuf_stru *pst_netbuf, mac_tx_ctl_stru *pst_tx_ctl);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of hmac_frag.h */
