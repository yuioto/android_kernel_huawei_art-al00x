
#include "oal_kernel_file.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID  OAM_FILE_ID_OAL_KERNEL_FILE_C

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)

/*
 * 内核文件打开函数
 * 参数为文件路径
 * 操作file类型结构变量
 *
 */
oal_file* oal_kernel_file_open(const oal_uint8 *path, oal_int32 ul_attribute)
{
    oal_file* pst_file;
    pst_file = filp_open((int8_t *)path, ul_attribute, 0777);
    if (IS_ERR(pst_file)) {
        return 0;
    }

    return pst_file;
}


loff_t oal_kernel_file_size(oal_file *pst_file)
{
    struct inode *pst_inode = OAL_PTR_NULL;
    loff_t        ul_fsize = 0;

    if (pst_file->f_path.dentry != OAL_PTR_NULL) {
        pst_inode = pst_file->f_path.dentry->d_inode;
        ul_fsize = pst_inode->i_size;
    }

    return ul_fsize;
}


oal_void *oal_kernel_file_read(oal_file *pst_file, loff_t ul_fsize)
{
    char        *pst_buff;
    loff_t      *pos;

    pos = &(pst_file->f_pos);
    pst_buff = (char *)kmalloc(ul_fsize + 100, GFP_KERNEL);
    if (OAL_UNLIKELY(pst_buff == NULL)) {
        OAL_WARN_ON(1);
        return 0;
    }

    vfs_read(pst_file, pst_buff, ul_fsize, pos);

    return pst_buff;
}


oal_long oal_kernel_file_write(oal_file *pst_file, const oal_uint8 *pst_buf, loff_t fsize)
{
    loff_t *pst_pos = &(pst_file->f_pos);

    vfs_write(pst_file, (int8_t *)pst_buf, fsize, pst_pos);

    return OAL_SUCC;
}


oal_long oal_kernel_file_print(oal_file *pst_file, const oal_int8 *pc_fmt, ...)
{
    oal_int8                    auc_str_buf[OAL_PRINT_FORMAT_LENGTH];   /* 保存要打印的字符串 buffer used during I/O */
    OAL_VA_LIST                 pc_args;
    oal_int32                   ret;

    if (pst_file == OAL_PTR_NULL || pc_fmt == OAL_PTR_NULL) {
        return OAL_FAIL;
    }

    OAL_VA_START(pc_args, pc_fmt);
    ret = vsnprintf_s(auc_str_buf, OAL_PRINT_FORMAT_LENGTH, OAL_PRINT_FORMAT_LENGTH - 1, pc_fmt, pc_args);
    OAL_VA_END(pc_args);

    if ((ret < 0) || (ret > OAL_PRINT_FORMAT_LENGTH)) {
        return OAL_FAIL;
    }

    return oal_kernel_file_write(pst_file, (uint8_t *)auc_str_buf, OAL_STRLEN(auc_str_buf));
}
#endif

#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT

OAL_STATIC oal_kobject* g_conn_syfs_root_object = NULL;
OAL_STATIC oal_kobject* g_conn_syfs_root_boot_object = NULL;

oal_kobject* oal_get_sysfs_root_object(oal_void)
{
    if (g_conn_syfs_root_object != NULL)
        return g_conn_syfs_root_object;
    g_conn_syfs_root_object = kobject_create_and_add("hisys", OAL_PTR_NULL);
    return g_conn_syfs_root_object;
}

oal_kobject* oal_get_sysfs_root_boot_object(oal_void)
{
    OAL_STATIC oal_kobject *root_boot_object = NULL;
    if (g_conn_syfs_root_boot_object)
        return g_conn_syfs_root_boot_object;
    root_boot_object = oal_get_sysfs_root_object();
    if (root_boot_object == NULL)
        return NULL;
    g_conn_syfs_root_boot_object = kobject_create_and_add("boot", root_boot_object);
    return g_conn_syfs_root_boot_object;
}

oal_kobject* oal_conn_sysfs_root_obj_init(oal_void)
{
    return oal_get_sysfs_root_object();
}

oal_void oal_conn_sysfs_root_obj_exit(oal_void)
{
    if (g_conn_syfs_root_object != NULL) {
        kobject_put(g_conn_syfs_root_object);
        g_conn_syfs_root_object = NULL;
    }
}

oal_void oal_conn_sysfs_root_boot_obj_exit(oal_void)
{
    if (g_conn_syfs_root_boot_object != NULL) {
        kobject_del(g_conn_syfs_root_boot_object);
        g_conn_syfs_root_boot_object = NULL;
    }
}
oal_module_symbol(oal_get_sysfs_root_object);
oal_module_symbol(oal_get_sysfs_root_boot_object);
#else
oal_kobject* oal_get_sysfs_root_object(oal_void)
{
    return NULL;
}

oal_kobject* oal_conn_sysfs_root_obj_init(oal_void)
{
    return NULL;
}

oal_void oal_conn_sysfs_root_obj_exit(oal_void)
{
    return;
}

oal_void oal_conn_sysfs_root_boot_obj_exit(oal_void)
{
    return;
}
#endif


/*lint -e19*/
oal_module_symbol(oal_kernel_file_open);
oal_module_symbol(oal_kernel_file_size);
oal_module_symbol(oal_kernel_file_read);
oal_module_symbol(oal_kernel_file_write);
oal_module_symbol(oal_kernel_file_print);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

