
#ifndef __OAL_LINUX_KERNEL_FILE_H__
#define __OAL_LINUX_KERNEL_FILE_H__

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/syscalls.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define OAL_KERNEL_DS           KERNEL_DS

/* 文件属性 */
#define OAL_O_ACCMODE           O_ACCMODE
#define OAL_O_RDONLY            O_RDONLY
#define OAL_O_WRONLY            O_WRONLY
#define OAL_O_RDWR              O_RDWR
#define OAL_O_CREAT             O_CREAT
#define OAL_O_TRUNC             O_TRUNC
#define OAL_O_APPEND            O_APPEND

#define OAL_PRINT_FORMAT_LENGTH     512                     /* 打印格式字符串的最大长度 */

typedef struct file             oal_file;
typedef mm_segment_t            oal_mm_segment_t;
OAL_STATIC OAL_INLINE oal_mm_segment_t oal_get_fs(oal_void)
{
    return  get_fs();
}

OAL_STATIC OAL_INLINE oal_void oal_set_fs(oal_mm_segment_t fs)
{
    return set_fs(fs);
}

OAL_STATIC OAL_INLINE oal_long oal_kernel_file_close(oal_file *pst_file)
{
    return filp_close(pst_file, NULL);
}

OAL_STATIC OAL_INLINE int oal_debug_sysfs_create_group(struct kobject *kobj,
                                                       const struct attribute_group *grp)
{
#ifdef PLATFORM_DEBUG_ENABLE
    return sysfs_create_group(kobj, grp);
#else
    OAL_REFERENCE(kobj);
    OAL_REFERENCE(grp);
    return 0;
#endif
}

OAL_STATIC OAL_INLINE void oal_debug_sysfs_remove_group(struct kobject *kobj,
                                                        const struct attribute_group *grp)
{
#ifdef PLATFORM_DEBUG_ENABLE
    sysfs_remove_group(kobj, grp);
#else
    OAL_REFERENCE(kobj);
    OAL_REFERENCE(grp);
#endif
}

OAL_STATIC OAL_INLINE int oal_debug_sysfs_create_file(struct kobject *kobj,
                                                      const struct attribute *attr)
{
#ifdef PLATFORM_DEBUG_ENABLE
    return sysfs_create_file(kobj, attr);
#else
    OAL_REFERENCE(kobj);
    OAL_REFERENCE(attr);
    return 0;
#endif
}

OAL_STATIC OAL_INLINE void oal_debug_sysfs_remove_file(struct kobject *kobj,
                                                       const struct attribute *attr)
{
#ifdef PLATFORM_DEBUG_ENABLE
    sysfs_remove_file(kobj, attr);
#else
    OAL_REFERENCE(kobj);
    OAL_REFERENCE(attr);
#endif
}

#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT
extern oal_kobject* oal_get_sysfs_root_object(oal_void);
extern oal_kobject* oal_get_sysfs_root_boot_object(oal_void);
extern oal_kobject* oal_conn_sysfs_root_obj_init(oal_void);
extern oal_void oal_conn_sysfs_root_obj_exit(oal_void);
extern oal_void oal_conn_sysfs_root_boot_obj_exit(oal_void);
#endif
extern oal_file* oal_kernel_file_open(const oal_uint8 *path, oal_int32 ul_attribute);
extern loff_t oal_kernel_file_size(oal_file *pst_file);
extern oal_void *oal_kernel_file_read(oal_file *pst_file, loff_t ul_fsize);
extern oal_long oal_kernel_file_write(oal_file *pst_file, const oal_uint8 *pst_buf, loff_t fsize);
extern oal_long oal_kernel_file_print(oal_file *pst_file, const oal_int8 *pc_fmt, ...);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_main */
