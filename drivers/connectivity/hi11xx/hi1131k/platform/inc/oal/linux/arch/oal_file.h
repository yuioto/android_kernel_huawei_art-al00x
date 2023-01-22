

#ifndef __OAL_LINUX_FILE_H__
#define __OAL_LINUX_FILE_H__

#include <linux/fs.h>
#include "oal_mm.h"
#include "oal_util.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
/*****************************************************************************
  2 宏定义
*****************************************************************************/
/* 文件属性 */
#define OAL_O_ACCMODE           O_ACCMODE
#define OAL_O_RDONLY            O_RDONLY
#define OAL_O_WRONLY            O_WRONLY
#define OAL_O_RDWR              O_RDWR
#define OAL_O_CREAT             O_CREAT
#define OAL_O_TRUNC             O_TRUNC
#define OAL_O_APPEND            O_APPEND

#define OAL_SEEK_SET      SEEK_SET     /* Seek from beginning of file.  */
#define OAL_SEEK_CUR     SEEK_CUR    /* Seek from current position.  */
#define OAL_SEEK_END     SEEK_END    /* Set file pointer to EOF plus "offset" */

#define OAL_FILE_POS(pst_file)      ((pst_file)->fp->f_pos)
#define OAL_FILE_FAIL            OAL_PTR_NULL
#define OAL_PRINT_ERR(s, args...) printk(KERN_ERR ":E]%s]" s, __func__, ## args);

typedef struct _oal_file_stru_ {
    struct file  *fp;
} oal_file_stru;


OAL_STATIC OAL_INLINE oal_file_stru* oal_file_open(const oal_int8 *pc_path, oal_int32 flags, oal_int32 rights)
{
    oal_file_stru   *pst_file = NULL;
    pst_file = oal_kzalloc(sizeof(oal_file_stru), OAL_GFP_KERNEL);
    if (pst_file == NULL) {
        OAL_PRINT_ERR("oal_kzalloc fail!!\n");
        return OAL_PTR_NULL;
    }
    pst_file->fp = filp_open(pc_path, flags, rights);
    if (IS_ERR_OR_NULL(pst_file->fp)) {
        OAL_PRINT_ERR("filp_open [%s] fail!!, fp=%p, errno:%ld\n", pc_path, pst_file->fp, PTR_ERR(pst_file->fp));
        oal_free(pst_file);
        return OAL_PTR_NULL;
    }
    return pst_file;
}


OAL_STATIC OAL_INLINE oal_uint32 oal_file_write(oal_file_stru *pst_file, const oal_int8 *pc_string,
                                                oal_uint32 ul_length)
{
    oal_uint32 ul_ret;
    mm_segment_t fs;
    fs = get_fs();
    set_fs(KERNEL_DS);
    ul_ret = vfs_write(pst_file->fp, pc_string, ul_length, &(pst_file->fp->f_pos));
    set_fs(fs);
    return ul_ret;
}


OAL_STATIC OAL_INLINE oal_void oal_file_close(oal_file_stru *pst_file)
{
    filp_close(pst_file->fp, NULL);
    oal_free(pst_file);
    pst_file = NULL;
}


OAL_STATIC OAL_INLINE oal_int32  oal_file_read(oal_file_stru *pst_file,
                                               oal_int8 *pc_buf,
                                               oal_uint32 ul_count)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
    loff_t pos = pst_file->fp->f_pos;
    return kernel_read(pst_file->fp, pc_buf, ul_count, &pos);
#else
    return kernel_read(pst_file->fp, pst_file->fp->f_pos, pc_buf, ul_count);
#endif
}

OAL_STATIC OAL_INLINE oal_int64  oal_file_lseek(oal_file_stru *pst_file, oal_int64 offset, oal_int32 whence)
{
    return vfs_llseek(pst_file->fp, offset, whence);
}


OAL_STATIC OAL_INLINE oal_int32  oal_file_size(const oal_int8 *pc_path, oal_uint32   *pul_file_size)
{
    oal_file_stru     *p_file;

    p_file = oal_file_open(pc_path, (OAL_O_RDONLY), 0);
    if (p_file == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    *pul_file_size = p_file->fp->f_path.dentry->d_inode->i_size;

    oal_file_close(p_file);

    return OAL_SUCC;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_file.h */
