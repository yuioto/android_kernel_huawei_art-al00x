

#ifndef __OAL_NO_FS_ADAPT_H__
#define __OAL_NO_FS_ADAPT_H__

#ifdef _PRE_HI113X_FS_DISABLE
#include "oal_types.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
/* ÎÄ¼þÊôÐÔ */
#define OAL_O_ACCMODE           0
#define OAL_O_RDONLY            0
#define OAL_O_WRONLY            0
#define OAL_O_RDWR              0
#define OAL_O_CREAT             0
#define OAL_O_TRUNC             0
#define OAL_O_APPEND            0
#define OAL_O_EXCL              0
#define OAL_O_NOCTTY            0
#define OAL_O_NONBLOCK          0
#define OAL_O_DSYNC             0
#define OAL_FASYNC              0
#define OAL_O_DIRECT            0
#define OAL_O_LARGEFILE         0
#define OAL_O_DIRECTORY         0
#define OAL_O_NOFOLLOW          0
#define OAL_O_NOATIME           0
#define OAL_O_CLOEXEC           0


#define OAL_SEEK_SET     0     /* Seek from beginning of file.  */
#define OAL_SEEK_CUR     1    /* Seek from current position.  */
#define OAL_SEEK_END     2    /* Set file pointer to EOF plus "offset" */

#define FILE_STORE_MIN_SIZE             0
#define FILE_STORE_MID_SIZE             0x30000
#define FILE_STORE_MAX_SIZE             0x70000

#define FILE_STORE_MIN_CNT             0
#define FILE_STORE_MID_CNT             1
#define FILE_STORE_MAX_CNT             3

typedef struct _oal_file_stru_ {
    oal_ulong  ul_store_index;
    oal_ulong  ul_pos;
} oal_file_stru;

struct st_file_store_info {
    oal_ulong  ul_store_addr;
    oal_ulong  ul_store_length;
    oal_ulong  ul_store_used;
    oal_uint8  *pus_store_path;
};

struct st_file_store_cfg {
    oal_ulong                       base_addr;
    oal_uint32                      total_length;
};
extern oal_file_stru* oal_no_fs_file_open(const oal_int8 *pc_path);
extern oal_int32 oal_no_fs_file_write(oal_file_stru *pst_file, oal_int8 *pc_string, oal_uint32 ul_length);
extern oal_int32  oal_no_fs_file_read(oal_file_stru *pst_file, oal_int8 *pc_buf, oal_uint32 ul_count);
extern oal_void oal_no_fs_file_close(oal_file_stru *pst_file);
extern oal_int64  oal_no_fs_file_lseek(oal_file_stru *pst_file, oal_int64 offset, oal_int32 whence);
extern oal_int32  oal_no_fs_get_file_pos(oal_file_stru *pst_file);
extern oal_void oal_no_fs_config(oal_ulong ul_base_addr, oal_uint32 u_length);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif
#endif /* end of oal_file.h */

