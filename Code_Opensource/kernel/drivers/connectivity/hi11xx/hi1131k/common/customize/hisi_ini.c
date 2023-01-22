

// 1 Header File Including
#define INI_KO_MODULE

#ifdef INI_KO_MODULE
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include <linux/module.h>
#include <linux/time.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/printk.h>
#ifdef CONFIG_HWCONNECTIVITY
#include "hisi_oneimage.h"
#endif
#endif
#ifdef HISI_NVRAM_SUPPORT
#include <linux/mtd/hisi_nve_interface.h> //lint !e7
#endif
#include "hisi_ini.h"
#include "hisi_customize_wifi.h"
#include "plat_pm.h"
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#include "stdio.h"
#endif

#include "oal_schedule.h"
#include "securec.h"

#define CUST_COMP_NODE             "hi1102,customize"
#define PROC_NAME_INI_FILE_NAME    "ini_file_name"
#define CUST_PATH                  "/data/cust"
/* mutex for open ini file */
oal_mutex_stru      file_mutex;
oal_uint8 g_ini_file_name[INI_FILE_PATH_LEN] = {0};
INI_BOARD_VERSION_STRU g_board_version;
OAL_EXPORT_SYMBOL(g_board_version);

INI_PARAM_VERSION_STRU g_param_version;
OAL_EXPORT_SYMBOL(g_param_version);

#else

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hisi_ini.h"

oal_mutex_stru file_mutex;
static oal_int8 init_mutex_flag = 0;

oal_int8 g_ini_file_path[INI_FILE_PATH_LEN] = {0};
#endif
#define INI_FILE_CFG_PATH       "/sys/hi110x_ps/ini_file_name"

#define INI_VAR_LOG_CFG_STATUS  "log_cfg_status"
#define INI_VAR_LOG_CFG_FILE    "log_cfg_file"
#define INI_VAR_FILE_LEN        40
#define INI_VAR_CFG_ENABLE      1

#define MAX_PATH_LEN            10

// 2 Global Variable Definition
int8_t gac_cfg_file[INI_VAR_FILE_LEN] = {0};
oal_int8 gac_file[INI_VAR_FILE_LEN] = {0};
// 3 Function Definition
#ifndef INI_KO_MODULE
oal_int8 *get_ini_file_path(void)
{
    FILE *fp;
    oal_int32 ret;
    static oal_int32 flag_got_path = 0;

    if (!flag_got_path) {
        fp = fopen(INI_FILE_CFG_PATH, "r");
        if (fp == NULL) {
            INI_ERROR("open %s failed:%s", INI_FILE_CFG_PATH, strerror(errno));
            return NULL;
        }
        ret = fread(g_ini_file_path, INI_FILE_PATH_LEN, 1, fp);
        if (ret < 0) {
            INI_WARNING("read %s failed:%s", INI_FILE_CFG_PATH, strerror(errno));
            fclose(fp);
            return NULL;
        }
        fclose(fp);
        flag_got_path = 1;
    }
    return g_ini_file_path;
}
#else
#if (_PRE_LINUX_PLATFORM == MIAMI_C60)  //lint !e553
int8_t* get_ini_file_path_from_dts(void)
{
    if (strlen(gac_cfg_file) != 0) {
        return gac_cfg_file;
    }
    if (get_ini_file_name_from_dts(PROC_NAME_INI_FILE_NAME, gac_cfg_file, sizeof(gac_cfg_file)) != BOARD_SUCC) {
        INI_ERROR("get_ini_file_path_from_dts: get ini file from dts failed, use default path.");
        return DEFAULT_INI_FILE_PATH;
    }
    INI_DEBUG("get_ini_file_path_from_dts:gac_cfg_file=%s.", gac_cfg_file);
    return gac_cfg_file;
}
#endif
#endif


static INI_FILE * ini_file_open(OAL_CONST oal_int8 * filename, OAL_CONST oal_int8 * para)
{
    INI_FILE * fp;

#ifdef INI_KO_MODULE
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    fp = filp_open(filename, O_RDONLY, 0);
    if (IS_ERR_OR_NULL(fp))
#else
    fp = fopen(filename, para);
    if (0 == fp)
#endif
#endif
    {
        fp = NULL;
    }
    return fp;
}


static oal_int32 ini_file_close(INI_FILE *fp)
{
#ifdef INI_KO_MODULE
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    filp_close(fp, NULL);
#else
    fclose(fp);
#endif
#endif
    return INI_SUCC;
}

#ifdef HISI_NVRAM_SUPPORT

static bool ini_file_exist(const oal_int8 *ini_file_path, int32_t file_path_len)
{
    INI_FILE *fp = NULL;

    if (ini_file_path == NULL || file_path_len <= 0) {
        INI_ERROR("para file_path is NULL\n");
        return false;
    }

    fp = ini_file_open(ini_file_path, "rt");
    if (fp == NULL) {
        INI_DEBUG("%s not exist\n", ini_file_path);
        return false;
    }

    ini_file_close(fp);

    INI_DEBUG("%s exist\n", ini_file_path);

    return true;
}
#endif


static oal_int32 ini_file_seek(INI_FILE *fp, long fp_pos)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    fp->f_pos += fp_pos;
    return INI_SUCC;

#else

    if (fseek(fp, fp_pos, SEEK_CUR) == INI_FAILED) {
        INI_ERROR("fseek failed, fp_pos is %ld; errno is %s", fp_pos, strerror(errno));
        return INI_FAILED;
    } else {
        return INI_SUCC;
    }

#endif
}


#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
static oal_int32 ko_read_line(INI_FILE *fp, char *addr, int32_t buf_len)
{
    oal_int32 l_ret;
    oal_int8 auc_tmp[MAX_READ_LINE_NUM];
    oal_int32 cnt;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
        loff_t pos = fp->f_pos;
        l_ret = kernel_read(fp, auc_tmp, MAX_READ_LINE_NUM, &pos);
#else
        l_ret = kernel_read(fp, fp->f_pos, auc_tmp, MAX_READ_LINE_NUM);
#endif
    if (l_ret < 0) {
        INI_ERROR("kernel_line read l_ret < 0");
        return INI_FAILED;
    } else if (l_ret == 0) {
        /* end of file */
        return 0;
    }

    cnt = 0;
    /* 有些文件行结尾有'\r'字符，需要删除 */
    while ((cnt < buf_len - 1) && (auc_tmp[cnt] != '\n') && (auc_tmp[cnt] != '\r')) {
        *addr++ = auc_tmp[cnt];
        cnt++;
    }

    if (cnt <= (MAX_READ_LINE_NUM - 1)) {
        *addr = '\n';
    } else {
        INI_ERROR("ko read_line is unexpected");
        return INI_FAILED;
    }

    /* change file pos to next line */
    ini_file_seek(fp, cnt + 1);
    return l_ret;
}
#endif


static oal_int32 ini_readline_func(INI_FILE *fp, oal_int8 *rd_buf, int32_t buf_len)
{
    oal_int8 auc_tmp[MAX_READ_LINE_NUM];
    oal_int32 l_ret;

    memset_s(auc_tmp, MAX_READ_LINE_NUM, 0, MAX_READ_LINE_NUM);

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    l_ret = ko_read_line(fp, auc_tmp, sizeof(auc_tmp));
    if (l_ret == INI_FAILED) {
        INI_ERROR("ko_read_line failed!!!");
        return INI_FAILED;
    } else if (l_ret == 0) {
        INI_ERROR("end of .ini file!!!");
        return INI_FAILED;
    }
#else
    oal_int8* pau_ret;
    pau_ret = NULL;
    pau_ret = fgets(auc_tmp, MAX_READ_LINE_NUM, fp);
    if (pau_ret == NULL) {
        INI_ERROR("have end of .ini file!!!");
        return INI_FAILED;
    }
#endif
    if (strcpy_s(rd_buf, buf_len, auc_tmp) != EOK) {
        INI_ERROR("ini_readline_func::strncpy_s failed !");
        return INI_FAILED;
    }
    return INI_SUCC;
}


oal_int32 ini_check_str(const int8_t *auc_tmp, const int8_t *puc_var, int32_t search_var_len)
{
    uint16_t auc_len;
    uint16_t curr_var_len;

    if ((puc_var == NULL) || (puc_var[0] == '\0')) {
        INI_ERROR("check if puc_var is NULL or blank");
        return INI_FAILED;
    }

    do {
        auc_len = strlen(auc_tmp);
        curr_var_len = 0;
        while ((curr_var_len < MAX_READ_LINE_NUM - 1) && (auc_tmp[curr_var_len] != '\r') &&
               (auc_tmp[curr_var_len] != '\n') && (auc_tmp[curr_var_len] != 0)) {
            curr_var_len++;
        }

        if ((auc_tmp[0] == '#') || (auc_tmp[0] == ' ') || (auc_tmp[0] == '\n') || (auc_tmp[0] == '\r')) {
            break;
        }
        if (search_var_len > curr_var_len) {
            search_var_len = curr_var_len;
        }
        if (strncmp(auc_tmp, puc_var, search_var_len) == 0) {
            return INI_SUCC;
        } else {
            break;
        }
    }while (0);

    return INI_FAILED;
}


static int32_t ini_check_value(int8_t *puc_value, uint32_t value_len)
{
    const uint32_t value_min_len = 1;

    if (value_len < value_min_len) {
        INI_ERROR("ini_check_value fail, puc_value length %u < 1\n", value_len);
        return INI_FAILED;
    }

    if (puc_value[0] == ' ' || puc_value[value_len - 1] == ' ' || puc_value[0] == '\n') {
        puc_value[0] = '\0';
        INI_ERROR("::%s has blank space or is blank::", puc_value);
        return INI_FAILED;
    }

    /* check \n of line */
    if (puc_value[value_len - 1] == '\n') {
        puc_value[value_len - 1] = '\0';
    }

    /* check \r of line */
    if (puc_value[value_len - 2] == '\r') {
        puc_value[value_len - 2] = '\0';
    }
    return INI_SUCC;
}

static int32_t ini_find_mode_switch_case(int32_t modu, int8_t *mode_var, int32_t var_len)
{
    int32_t ret = EOK;

    if (var_len < 0) {
        INI_ERROR("error: var_len < 0");
        return INI_FAILED;
    }

    switch (modu) {
        case INI_MODU_WIFI:
            ret = strcpy_s(mode_var, var_len, INI_MODE_VAR_WIFI);
            break;
        case INI_MODU_GNSS:
            ret = strcpy_s(mode_var, var_len, INI_MODE_VAR_GNSS);
            break;
        case INI_MODU_BT:
            ret = strcpy_s(mode_var, var_len, INI_MODE_VAR_BT);
            break;
        case INI_MODU_FM:
            ret = strcpy_s(mode_var, var_len, INI_MODE_VAR_FM);
            break;
        case INI_MODU_WIFI_PLAT:
            ret = strcpy_s(mode_var, var_len, INI_MODE_VAR_WIFI_PLAT);
            break;
        case INI_MODU_BFG_PLAT:
            ret = strcpy_s(mode_var, var_len, INI_MODE_VAR_BFG_PLAT);
            break;
        case INI_MODU_PLAT:
        case INI_MODU_HOST_VERSION:
        case INI_MODU_WIFI_MAC:
        case INI_MODU_COEXIST:
            return INI_SUCC;
        default:
            INI_ERROR("not suport modu type!!!");
            return INI_FAILED;
    }
    if (ret != EOK) {
        INI_ERROR("ini_find_mode::strcpy_s failed! [%d]", modu);
        return INI_FAILED;
    }
    return INI_SUCC;
}



static oal_int32 ini_find_mode(INI_FILE *fp, oal_int32 modu, const int8_t *puc_var, oal_int8 *puc_value, uint32_t size)
{
    oal_int32 ret;
    oal_int8 auc_tmp[MAX_READ_LINE_NUM] = {0};
    oal_int8 auc_mode_var[INI_STR_MODU_LEN] = {0};
    oal_int8 *puc_val = NULL;

    ret = ini_find_mode_switch_case(modu, auc_mode_var, sizeof(auc_mode_var));
    if (ret != INI_SUCC) {
        INI_ERROR("ini_find_mode::ini_find_mode_switch_case failed!");
        return INI_FAILED;
    }
    while (1) {
        ret = ini_readline_func(fp, auc_tmp, sizeof(auc_tmp));
        if (ret == INI_FAILED) {
            INI_ERROR("have end of .ini file!!!");
            return INI_FAILED;
        }

        if (strstr(auc_tmp, INI_STR_DEVICE_BFG_PLAT) != NULL) {
            INI_ERROR("not find %s!!!", auc_mode_var);
            return INI_FAILED;
        }

        ret = ini_check_str(auc_tmp, auc_mode_var, strlen(auc_mode_var));
        if (ret == INI_SUCC) {
            INI_DEBUG("have found %s", auc_mode_var);
            break;
        } else {
            continue;
        }
    }

    puc_val = strstr(auc_tmp, "=");
    if (puc_val == NULL) {
        INI_ERROR("has not find = in %s", auc_tmp);
        return INI_FAILED;
    }

    /* 使用strlen(puc_val)计算出来的长度不准确，因为从文件中读取出来的行结尾有\r\n字符 */
    if (strncpy_s(puc_value, size, puc_val + 1, size - 1) != EOK) {
        INI_ERROR("ini_find_mode::strncpy_s failed!");
        return INI_FAILED;
    }
    if (strcmp(auc_mode_var, puc_var) == 0) { //lint !e421
        return INI_SUCC_MODE_VAR;
    }
    return INI_SUCC;
}

static int32_t ini_find_modu_switch_case_two(int8_t *modu, int32_t mode_value)
{
    int32_t ret = EOK;

    if (mode_value == INI_MODE_GPSGLONASS) {
        ret = strcpy_s(modu, INI_STR_MODU_LEN, INI_STR_GNSS_GPSGLONASS);
    } else if (mode_value == INI_MODE_BDGPS) {
        ret = strcpy_s(modu, INI_STR_MODU_LEN, INI_STR_GNSS_BDGPS);
    } else {
        INI_ERROR("switch_case_two :: not support mode!!!");
        return INI_FAILED;
    }
    if (ret != EOK) {
        INI_ERROR("switch_case_two::strcpy_s failed! [%d]", mode_value);
        return INI_FAILED;
    }
    return INI_SUCC;
}

static int32_t ini_find_modu_switch_case_one(int8_t *modu, int32_t mode_value)
{
    int32_t ret = EOK;

    /* find mode var */
    if (mode_value == INI_MODE_NORMAL) {
        ret = strcpy_s(modu, INI_STR_MODU_LEN, INI_STR_WIFI_NORMAL);
    } else if (mode_value == INI_MODE_PERFORMANCE) {
        ret = strcpy_s(modu, INI_STR_MODU_LEN, INI_STR_WIFI_PERFORMANCE);
    } else if (mode_value == INI_MODE_CERTIFY) {
        ret = strcpy_s(modu, INI_STR_MODU_LEN, INI_STR_WIFI_CERTIFY);
    } else if (mode_value == INI_MODE_CERTIFY_CE) {
        ret = strcpy_s(modu, INI_STR_MODU_LEN, INI_STR_WIFI_CERTIFY_CE);
    } else {
        INI_ERROR("switch_case_one::not support mode!!!");
        return INI_FAILED;
    }
    if (ret != EOK) {
        INI_ERROR("switch_case_one::strcpy_s failed! [%d]", mode_value);
        return INI_FAILED;
    }
    return INI_SUCC;
}

static int32_t ini_find_modu_switch_case(int32_t num_modu, int8_t *modu, int32_t mode_value)
{
    int32_t ret = EOK;

    switch (num_modu) {
        case INI_MODU_WIFI:
            ini_find_modu_switch_case_one(modu, mode_value);
            break;
        case INI_MODU_GNSS:
            ini_find_modu_switch_case_two(modu, mode_value);
            break;
        case INI_MODU_BT:
             ret = strcpy_s(modu, INI_STR_MODU_LEN, INI_STR_BT_NORMAL);
            break;
        case INI_MODU_FM:
             ret = strcpy_s(modu, INI_STR_MODU_LEN, INI_STR_FM_NORMAL);
            break;
        case INI_MODU_WIFI_PLAT:
             ret = strcpy_s(modu, INI_STR_MODU_LEN, INI_STR_WIFI_PLAT_NORMAL);
            break;
        case INI_MODU_BFG_PLAT:
             ret = strcpy_s(modu, INI_STR_MODU_LEN, INI_STR_BFG_PLAT_NORMAL);
            break;
        case INI_MODU_PLAT:
             ret = strcpy_s(modu, INI_STR_MODU_LEN, INI_STR_PLAT);
            break;
        case INI_MODU_HOST_VERSION:
             ret = strcpy_s(modu, INI_STR_MODU_LEN, INT_STR_HOST_VERSION);
            break;
        case INI_MODU_WIFI_MAC:
             ret = strcpy_s(modu, INI_STR_MODU_LEN, INI_STR_WIFI_MAC);
            break;
        case INI_MODU_COEXIST:
             ret = strcpy_s(modu, INI_STR_MODU_LEN, INI_STR_COEXIST);
            break;
        default:
            INI_ERROR("switch_case::not suport modu type!!!");
            return INI_FAILED;
    }
    if (ret != EOK) {
        INI_ERROR("switch_case::strcpy_s failed! case[%d]", num_modu);
        return INI_FAILED;
    }
    return INI_SUCC;
}


static oal_int32 ini_find_modu(INI_FILE *fp, oal_int32 modu, oal_int8 *puc_var, oal_int8 *puc_value)
{
    oal_int8 auc_tmp[MAX_READ_LINE_NUM] = {0};
    oal_int8 auc_modu[INI_STR_MODU_LEN] = {0};
    oal_int32 l_mode_value = 0;
    int32_t ret;

    memset_s(auc_tmp, MAX_READ_LINE_NUM, 0, MAX_READ_LINE_NUM);

    /* check the mode of moduler */
    if (ini_check_value(puc_value, strlen(puc_value)) == INI_FAILED) {
        return INI_FAILED;
    }

    /* INI_MODU_PLAT has no mode */
    if (modu != INI_MODU_PLAT) {
        l_mode_value = puc_value[0] - '0';
        if (OAL_VALUE_NOT_IN_VALID_RANGE(l_mode_value, 0, 10)) {
            INI_ERROR("not support value %s", puc_value);
            return INI_FAILED;
        }
    }
    ret = ini_find_modu_switch_case(modu, auc_modu, l_mode_value);
    if (ret == INI_FAILED) {
        INI_ERROR("ini_find_modu_switch_case failed !");
        return INI_FAILED;
    }
    /* find the value of mode var, such as ini_wifi_mode
     * every mode except PLAT mode has only one mode var
     */
    while (1) {
        ret = ini_readline_func(fp, auc_tmp, sizeof(auc_tmp));
        if (ret == INI_FAILED) {
            INI_ERROR("have end of .ini file!!!");
            return INI_FAILED;
        }

        if (strstr(auc_tmp, INI_STR_DEVICE_BFG_PLAT) != NULL) {
            INI_ERROR("not find %s!!!", auc_modu);
            return INI_FAILED;
        }

        ret = ini_check_str(auc_tmp, auc_modu, strlen(auc_modu));
        if (ret == INI_SUCC) {
            INI_DEBUG("have found %s", auc_modu);
            break;
        } else {
            continue;
        }
    }

    return INI_SUCC;
}


static oal_int32 ini_find_var(INI_FILE *fp, oal_int32 modu, oal_int8 *puc_var, oal_int8 *puc_value, oal_uint32 size)
{
    oal_int32 ret;
    oal_int8 auc_tmp[MAX_READ_LINE_NUM] = {0};
    size_t search_var_len;

    /* find the modu of var, such as [HOST_WIFI_NORMAL] of wifi moduler */
    ret = ini_find_modu(fp, modu, puc_var, puc_value);
    if (ret == INI_FAILED) {
        return INI_FAILED;
    }

    /* find the var in modu, such as [HOST_WIFI_NORMAL] of wifi moduler */
    while (1) {
        ret = ini_readline_func(fp, auc_tmp, sizeof(auc_tmp));
        if (ret == INI_FAILED) {
            INI_ERROR("have end of .ini file!!!");
            return INI_FAILED;
        }

        if (auc_tmp[0] == '[') {
            INI_ERROR("not find %s!!!, check if var in correct mode", puc_var);
            return INI_FAILED;
        }

        search_var_len = strlen(puc_var);
        ret = ini_check_str(auc_tmp, puc_var, search_var_len);
        if (ret == INI_SUCC && (auc_tmp[search_var_len] == '=')) {
            /* 使用strlen(auc_tmp[search_var_len + 1])计算出来的长度不准确，因为从文件中读取出来的行结尾有\r\n字符 */
            if (strncpy_s(puc_value, size, &auc_tmp[search_var_len + 1], size - 1) != EOK) {   //lint !e679
                INI_ERROR("ini_find_var::strncpy_s failed!");
            }
            break;
        } else {
            continue;
        }
    }
    return INI_SUCC;
}

#ifndef HISI_NVRAM_SUPPORT

oal_int32 read_conf_from_file(oal_int8 *name, oal_int8 *pc_arr, oal_uint32 size)
{
    INI_FILE *fp;
    oal_int32 len;
    oal_uint32 read_len;

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    fp = filp_open(NVRAM_CUST_FILE, O_RDONLY, 0660);
    if (IS_ERR(fp)) {
#else
    fp = fopen(NVRAM_CUST_FILE, "r");
    if (fp == NULL) {
#endif
#ifdef INI_KO_MODULE
        INI_DEBUG("open %s failed", NVRAM_CUST_FILE);
#else
        INI_DEBUG("open %s failed, errno is %s", NVRAM_CUST_FILE, strerror(errno));
#endif
        return INI_FAILED;
    }

    INI_DEBUG("open %s succ", NVRAM_CUST_FILE);

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    read_len = INI_MIN(size, NUM_OF_NV_PARAMS);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
    len = kernel_read(fp, pc_arr, read_len, 0);
#else
    len = kernel_read(fp, 0, pc_arr, read_len);
#endif
#else
    len = fread(pc_arr, 1, HISI_CUST_NVRAM_LEN,  fp);
#endif
    if (len < 0) {
        ini_file_close(fp);
#ifdef INI_KO_MODULE
        INI_ERROR("read %s failed, len = %d", NVRAM_CUST_FILE, len);
#else
        INI_ERROR("read %s failed, len = %d, errno is %s", NVRAM_CUST_FILE, len, strerror(errno));
#endif
        return INI_FAILED;
    }
    INI_DEBUG("read %s succ", NVRAM_CUST_FILE);
    ini_file_close(fp);

    return INI_SUCC;
}

oal_int32 write_conf_to_file(oal_int8 *name, oal_int8 *pc_arr)
{
    oal_file_stru *fp;
    oal_uint32 len;

    fp = oal_file_open(NVRAM_CUST_FILE, O_CREAT | O_RDWR, 0664);
    if (fp == NULL) {
        return INI_FAILED;
    }
    if (IS_ERR(fp)) {
        INI_DEBUG("open %s failed", NVRAM_CUST_FILE);
        return INI_FAILED;
    }

    INI_DEBUG("open %s succ", NVRAM_CUST_FILE);

    len = oal_file_write(fp, pc_arr, HISI_CUST_NVRAM_LEN);

    INI_DEBUG("write len:%d", len);

    oal_file_close(fp);

    return INI_SUCC;
}

#else

oal_int32 read_conf_from_nvram(oal_int8 *name, oal_int8 *pc_out, oal_uint32 size)
{
#ifdef  INI_KO_MODULE
    struct hisi_nve_info_user  info; //lint !e565
    oal_uint32 len;
#endif
#ifndef INI_KO_MODULE
    oal_int8 buff[HISI_CUST_NVRAM_LEN] = {0};
#endif
    oal_int32 ret;

#ifndef INI_KO_MODULE
    ret = hweiOperaNVWR(HISI_CUST_NVRAM_READ, HISI_CUST_NVRAM_NUM, HISI_CUST_NVRAM_NAME, HISI_CUST_NVRAM_LEN, buff);
    if (ret < -1) {
        INI_ERROR("read nvm failed");
        return INI_FAILED;
    }
#else
    /*lint -e115 -e63 -e409*/
    memset_s(&info, sizeof(info), 0, sizeof(info));
    if (strcpy_s(info.nv_name, NV_NAME_LENGTH, HISI_CUST_NVRAM_NAME) != EOK) {
        INI_ERROR("read_conf_from_nvram::strcpy_s failed!");
        return INI_FAILED;
    }
    info.nv_number = HISI_CUST_NVRAM_NUM;
    info.valid_size = HISI_CUST_NVRAM_LEN;
    info.nv_operation = HISI_CUST_NVRAM_READ;

    ret = hisi_nve_direct_access(&info);
    if (ret < -1) {
        INI_ERROR("read nvm failed");
        return INI_FAILED;
    } else {
        len = INI_MIN(size, NUM_OF_NV_PARAMS);
        if (memcpy_s(pc_out, size, info.nv_data, len) != EOK) {
            INI_ERROR("memcpt info.nv_data failed");
            return INI_FAILED;
        }
    }
    /*lint +e115 +e63 +e409*/
#endif
    return INI_SUCC;
}

oal_int32 write_conf_to_nvram(oal_int8 *name, const oal_int8 *pc_arr)
{
#ifdef  INI_KO_MODULE
    struct hisi_nve_info_user  info;
#endif
    oal_int32 ret;

#ifndef INI_KO_MODULE
    ret = hweiOperaNVWR(HISI_CUST_NVRAM_WRITE, HISI_CUST_NVRAM_NUM, HISI_CUST_NVRAM_NAME, HISI_CUST_NVRAM_LEN, pc_arr);
    if (ret < -1) {
        INI_ERROR("write nvm failed");
        return INI_FAILED;
    }
#else
    /*lint -e115 -e63 -e409*/
    memset_s(&info, sizeof(info), 0, sizeof(info));
    if (strcpy_s(info.nv_name, NV_NAME_LENGTH, HISI_CUST_NVRAM_NAME) != EOK) {
        INI_ERROR("write_conf_to_nvram strcpy_s nvm failed");
        return INI_FAILED;
    }
    info.nv_number = HISI_CUST_NVRAM_NUM;
    info.valid_size = HISI_CUST_NVRAM_LEN;
    info.nv_operation = HISI_CUST_NVRAM_WRITE;
    if (memcpy_s(info.nv_data, HISI_CUST_NVRAM_LEN, pc_arr, HISI_CUST_NVRAM_LEN) != EOK) {
        INI_ERROR("memcpy pc_arr failed");
        return INI_FAILED;
    }

    ret = hisi_nve_direct_access(&info);
    if (ret < -1) {
        INI_ERROR("write nvm failed");
        return INI_FAILED;
    }
    /*lint +e115 +e63 +e409*/
#endif

    return INI_SUCC;
}
#endif


#ifndef INI_KO_MODULE
oal_int32 ini_config_init(oal_int32 modu)
{
    oal_int8 log_status[10] = {0};
    oal_int32 log_value;
    oal_int32 l_ret;
    INI_FILE *fp = NULL;
    /* init only once */
    if (modu != INI_MODU_PLAT) {
        if (INI_FILE_PATH == NULL) {
            INI_ERROR("didn't get ini file path!!!");
            return INI_FAILED;
        }
        if (strncpy_s(gac_file, INI_VAR_FILE_LEN, INI_FILE_PATH, INI_VAR_FILE_LEN - 1) != EOK) {
            INI_ERROR("ini_config_init::strncpy_s failed!");
            return INI_FAILED;
        }
        gac_file[INI_VAR_FILE_LEN - 1] = '\0';
        return INI_SUCC;
    }

    INI_DEBUG("open ini file:%s", gac_file);
    fp = ini_file_open(gac_file, "rt");
    if (fp == 0) {
        INI_ERROR("open %s failed!!!", gac_file);
        return INI_FAILED;
    }
    INI_DEBUG("open %s succ!", gac_file);

    l_ret = ini_find_var(fp, INI_MODU_PLAT, INI_VAR_LOG_CFG_STATUS, log_status, sizeof(log_status));
    if (l_ret == INI_FAILED) {
        ini_file_close(fp);
        return INI_FAILED;
    }

    if (ini_check_value(log_status, strlen(log_status)) == INI_FAILED) {
        ini_file_close(fp);
        return INI_FAILED;
    }
    sscanf_s(log_status, "%d", &log_value);
    if (log_value < 0) {
        INI_ERROR("log_status value=%d not support!!!", log_value);
        ini_file_close(fp);
        return INI_FAILED;
    }
    INI_INFO("::log_status value=%d::", log_value);
    ini_file_close(fp);

    return INI_SUCC;
}
#endif

oal_int32 get_cust_conf_string(oal_int32 modu, oal_int8 *puc_var, oal_int8 *puc_value, oal_uint32 size)
{
    oal_int32 ret;
#ifdef INI_KO_MODULE
    struct device_node *np = OAL_PTR_NULL;
    oal_int32 len;
    oal_int8 out_str[HISI_CUST_NVRAM_LEN] = {0};
#endif

    switch (modu) {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifdef INI_KO_MODULE
        case CUST_MODU_DTS:
        {
            np = of_find_compatible_node(NULL, NULL, CUST_COMP_NODE);
            if (np == NULL) {
                INI_ERROR("no compatible node found");
                return INI_FAILED;
            }
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 44))
            len = of_property_count_u8_elems(np, puc_var);
#else
            ret = of_property_read_string(np, puc_var, (const char **)&out_str);
#endif
            if (len < 0) {
                INI_ERROR("can't get len of value(%s)", puc_var);
                return INI_FAILED;
            }
            len = INI_MIN(len, NUM_OF_NV_PARAMS);
            INI_DEBUG("get len of value(%s), read len is %d", puc_var, len);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 44))
            ret = of_property_read_u8_array(np, puc_var, (uint8_t *)out_str, len);
#else
            ret = of_property_read_string(np, puc_var, (const char **)&out_str);
#endif
            if (ret < 0) {
                INI_ERROR("conf str:%s property is not found", puc_var);
                return INI_FAILED;
            }
            if (memcpy_s(puc_value, size, out_str, len) != EOK) {
                INI_ERROR("get_cust_conf_string::memcpy_s failed!");
            }
            INI_DEBUG("conf string:%s get value:%s", puc_var, puc_value);
            return INI_SUCC;
        }
#endif
        case CUST_MODU_NVRAM:
#ifdef HISI_NVRAM_SUPPORT
            ret = read_conf_from_nvram(puc_var, puc_value, size);
#else
            ret = read_conf_from_file(puc_var, puc_value, size);
#endif
            if (ret < 0) {
                INI_ERROR("read nv_conf failed, ret(%d)", ret);
                return INI_FAILED;
            }
            return INI_SUCC;
#endif
        default:
        {
            return ini_find_var_value(modu, puc_var, puc_value, size);
        }
    }
}

OAL_EXPORT_SYMBOL(get_cust_conf_string);


oal_int32 set_cust_conf_string(oal_int32 modu, oal_int8 *name, oal_int8 *var)
{
    oal_int32 ret;
    if (modu != CUST_MODU_NVRAM) {
        INI_ERROR("NOT SUPPORT MODU TO WRITE");
        return INI_FAILED;
    }

#ifdef HISI_NVRAM_SUPPORT
    ret = write_conf_to_nvram(name, var);
#else
    ret = write_conf_to_file(name, var);
#endif

    return ret;
}


oal_int32 get_cust_conf_int32(oal_int32 modu, oal_int8 *puc_var, oal_int32 *puc_value)
{
    oal_int32 ret;
#ifdef INI_KO_MODULE
    struct device_node *np = NULL;
#endif

    switch (modu) {
#ifdef INI_KO_MODULE
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        case CUST_MODU_DTS:
        {
            np = of_find_compatible_node(NULL, NULL, CUST_COMP_NODE);
            if (np == NULL) {
                INI_ERROR("no compatible node found");
                return INI_FAILED;
            }

            ret = of_property_read_u32(np, puc_var, (uint32_t *)puc_value);
            if (ret < 0) {
                /* driver has log, delete this log */
                INI_DEBUG("conf %s is not exist, ret = %d", puc_var, ret);
                return INI_FAILED;
            }

            INI_DEBUG("conf %s get vale:%d", puc_var, *puc_value);
            return INI_SUCC;
        }
#endif
#endif
        default:
        {
            oal_int8  out_str[INI_READ_VALUE_LEN] = {0};

            ret = ini_find_var_value(modu, puc_var, out_str, sizeof(out_str));
            if (ret < 0) {
                /* ini_find_var_value has error log, delete this log */
                INI_DEBUG("cust modu didn't get var of %s.", puc_var);
                return INI_FAILED;
            }
            if (!strncmp(out_str, "0x", strlen("0x")) || !strncmp(out_str, "0X", strlen("0X"))) {
                INI_DEBUG("get hex of:%s.", puc_var);
                ret = sscanf_s(out_str, "%x", puc_value);
            } else {
                ret = sscanf_s(out_str, "%d", puc_value);
            }

            if (ret < 0) {
                INI_ERROR("%s trans to int failed", puc_var);
                return INI_FAILED;
            }
            INI_DEBUG("conf %s get vale:%d", puc_var, *puc_value);

            return INI_SUCC;
        }
    }
}

OAL_EXPORT_SYMBOL(get_cust_conf_int32);

#ifdef INI_KO_MODULE

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
void print_device_version(INI_FILE *fp)
{
    oal_int8  version_buff[INI_VERSION_STR_LEN] = {0};
    oal_int32 l_ret;

    l_ret = ini_find_var(fp, INI_MODU_PLAT, INI_VAR_PLAT_BOARD, version_buff, sizeof(version_buff));
    if (l_ret == INI_FAILED) {
        version_buff[0] = '\0';
        fp->f_pos = 0;
        return ;
    }
    if (ini_check_value(version_buff, strlen(version_buff)) == INI_FAILED) {
        fp->f_pos = 0;
        return;
    }
    if (strncpy_s((int8_t *)g_board_version.board_version, INI_VERSION_STR_LEN, version_buff,
        INI_VERSION_STR_LEN - 1) != EOK)  {
        INI_ERROR("print_device_version::strncpy_s one failed !");
        return;
    }
    INI_INFO("::g_board_version.board_version = %s::", g_board_version.board_version);
    fp->f_pos = 0;

    l_ret = ini_find_var(fp, INI_MODU_PLAT, INI_VAR_PLAT_PARAM, version_buff, sizeof(version_buff));
    if (l_ret == INI_FAILED) {
        version_buff[0] = '\0';
        fp->f_pos = 0;
        return;
    }
    if (ini_check_value(version_buff, strlen(version_buff)) == INI_FAILED) {
        fp->f_pos = 0;
        return;
    }
    if (strncpy_s((int8_t *)g_param_version.param_version, INI_VERSION_STR_LEN, version_buff,
        INI_VERSION_STR_LEN - 1) != EOK) {
        INI_ERROR("print_device_version::strncpy_s two failed !");
        return;
    }
    INI_INFO("::g_param_version.param_version = %s::", g_param_version.param_version);
    fp->f_pos = 0;
    return;
}
#endif
#endif
extern int32_t __chr_exception_para(uint32_t chr_errno, uint8_t *chr_ptr, uint16_t chr_len);

oal_int32 ini_find_var_value(oal_int32 modu, oal_int8 *puc_var, oal_int8 *puc_value, oal_uint32 size)
{
    INI_FILE *fp = NULL;
    oal_int32 l_ret;

#ifdef INI_KO_MODULE
    static oal_int32 sl_once = 1;
#endif

    if (puc_var == NULL || puc_var[0] == '\0' || puc_value == NULL) {
        INI_ERROR("check if puc_var and puc_value is NULL or blank");
        return INI_FAILED;
    }

#ifndef INI_KO_MODULE
    if (init_mutex_flag == 0) {
        INI_INIT_MUTEX(&file_mutex, NULL);
        init_mutex_flag = 1;
    }
#endif

    INI_MUTEX_LOCK(&file_mutex);

#ifndef INI_KO_MODULE
    l_ret = ini_config_init(modu);
    if (l_ret == INI_FAILED) {
        INI_MUTEX_UNLOCK(&file_mutex);
        return INI_FAILED;
    }
#endif

    INI_DEBUG("INI_FILE_PATH=%s.", INI_FILE_PATH);
    fp = ini_file_open(INI_FILE_PATH, "rt");
    if (fp == 0) {
        fp = NULL;
        INI_ERROR("open %s failed!!!", INI_FILE_PATH);
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV, CHR_PLT_DRV_EVENT_INI,
                             CHR_PLAT_DRV_ERROR_INI_MISS);
        INI_MUTEX_UNLOCK(&file_mutex);
        return INI_FAILED;
    }
#ifdef INI_KO_MODULE
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    /* init g_board_version.board_version and g_param_version.param_version
     * just once while call ini_find_var_value */
    if (sl_once) {
        print_device_version(fp);
        sl_once = 0;
    }
#endif
#endif

    l_ret = ini_find_mode(fp, modu, puc_var, puc_value, size);
    if (l_ret == INI_FAILED) {
        ini_file_close(fp);
        INI_MUTEX_UNLOCK(&file_mutex);
        return INI_FAILED;
    } else if (l_ret == INI_SUCC_MODE_VAR) {
        INI_DEBUG("::return %s:%s::", puc_var, puc_value);
        ini_file_close(fp);
        INI_MUTEX_UNLOCK(&file_mutex);
        return  ini_check_value(puc_value, strlen(puc_value));
    }

    /* find puc_var in .ini return puc_value */
    if ((ini_find_var(fp, modu, puc_var, puc_value, size)) == INI_FAILED) {
        puc_value[0] = '\0';
        ini_file_close(fp);
        INI_MUTEX_UNLOCK(&file_mutex);
        return INI_FAILED;
    }

    ini_file_close(fp);
    INI_MUTEX_UNLOCK(&file_mutex);

    /* check blank space of puc_value */
    if (ini_check_value(puc_value, strlen(puc_value)) == INI_SUCC) {
        INI_DEBUG("::return %s:%s::", puc_var, puc_value);
        return INI_SUCC;
    }
    return INI_FAILED;
}

OAL_EXPORT_SYMBOL(ini_find_var_value);

oal_int32 board_ini_find_modu(INI_FILE *fp)
{
    oal_int32 ret;
    oal_int8  ac_line_tmp[MAX_READ_LINE_NUM] = {0};
    while (1) {
        ret = ini_readline_func(fp, ac_line_tmp, sizeof(ac_line_tmp));
        if (ret == INI_FAILED) {
            PS_PRINT_ERR("have end of .ini file!!!");
            return INI_FAILED;
        }

        ret = ini_check_str(ac_line_tmp, INI_STR_BOARD_CFG, strlen(INI_STR_BOARD_CFG));
        if (ret == INI_SUCC) {
            PS_PRINT_INFO("have found %s", INI_STR_BOARD_CFG);
            break;
        } else {
            continue;
        }
    }

    return INI_SUCC;
}

oal_int32 board_ini_find_var(INI_FILE *fp, const oal_int8 *puc_var, oal_uint32 *pul_value)
{
    oal_int32 ret;
    oal_int8  ac_line_tmp[MAX_READ_LINE_NUM] = {0};
    oal_int8  ac_value_temp[INI_READ_VALUE_LEN] = {0};

    size_t search_var_len;

    while (1) {
        ret = ini_readline_func(fp, ac_line_tmp, sizeof(ac_line_tmp));
        if (ret == INI_FAILED) {
            PS_PRINT_ERR("have end of .ini file!!!");
            return INI_FAILED;
        }

        if (ac_line_tmp[0] == '[') {
            INI_ERROR("not find %s!!!, check if var in correct mode", puc_var);
            return INI_FAILED;
        }

        search_var_len = strlen(puc_var);
        ret = ini_check_str(ac_line_tmp, puc_var, search_var_len);
        if (ret == INI_SUCC) {
            if (strncpy_s(ac_value_temp, INI_READ_VALUE_LEN, &ac_line_tmp[search_var_len + 1], //lint !e679
                MAX_READ_LINE_NUM - search_var_len) != EOK) {
                INI_ERROR("board_ini_find_var::strncpy_s failed!");
                return INI_FAILED;
            }
            break;
        } else {
            continue;
        }
    }

    if (ini_check_value(ac_value_temp, strlen(ac_value_temp)) == INI_FAILED) {
        return INI_FAILED;
    }

    if ((!strncmp(ac_value_temp, "0x", strlen("0x")) || !strncmp(ac_value_temp, "0X", strlen("0X"))) &&
        (strlen(ac_value_temp) <= MAX_PATH_LEN)) {
        ret = sscanf_s(ac_value_temp, "%x", pul_value);
    } else {
        return INI_FAILED;
    }

    if (ret < 0) {
        INI_ERROR("%s trans to int failed", ac_value_temp);
        return INI_FAILED;
    }
    return INI_SUCC;
}

#ifdef INI_KO_MODULE
int ini_cfg_init(void)
{
#ifdef HISI_NVRAM_SUPPORT
    oal_int32 ret;
    oal_int8 auc_dts_ini_path[INI_FILE_PATH_LEN]  = {0};
    oal_int8 auc_cust_ini_path[INI_FILE_PATH_LEN] = {0};

#ifdef CONFIG_HWCONNECTIVITY
    if (!isMyConnectivityChip(CHIP_TYPE_HI110X)) {
        INI_ERROR("cfg ini chip type is not match, skip driver init");
        return -EINVAL;
    } else {
        INI_INFO("cfg init type is matched with hi110x, continue");
    }
#endif

    INI_DEBUG("hi110x ini config search init!\n");

    ret = get_cust_conf_string(CUST_MODU_DTS, PROC_NAME_INI_FILE_NAME, auc_dts_ini_path, sizeof(auc_dts_ini_path));
    if (ret < 0) {
        INI_ERROR("can't find dts proc %s\n", PROC_NAME_INI_FILE_NAME);
    }
    if (snprintf_s(auc_cust_ini_path, sizeof(auc_cust_ini_path), sizeof(auc_cust_ini_path) - 1,
        "%s%s", CUST_PATH, auc_dts_ini_path) < EOK) {
        INI_ERROR("ini_cfg_init::snprintf_s failed!");
    }

    /* 如果ini文件在cust中，则使用cust中的ini文件，否则使用dts中配置的ini文件 */
    if (ini_file_exist(auc_cust_ini_path, sizeof(auc_cust_ini_path))) {
        ret = snprintf_s((int8_t *)g_ini_file_name, sizeof(g_ini_file_name),
            sizeof(g_ini_file_name) - 1, "%s", auc_cust_ini_path);
        INI_INFO("%s@%s\n", PROC_NAME_INI_FILE_NAME, g_ini_file_name);
    } else if (ini_file_exist(auc_dts_ini_path, sizeof(auc_dts_ini_path))) {
        ret = snprintf_s((int8_t *)g_ini_file_name, sizeof(g_ini_file_name),
            sizeof(g_ini_file_name) - 1, "%s", auc_dts_ini_path);
        INI_INFO("%s@%s\n", PROC_NAME_INI_FILE_NAME, g_ini_file_name);
    } else {
        INI_ERROR("hi110x ini file not exist\n");
    }
    if (ret < 0) {
        INI_ERROR("ini_cfg_init::snprintf_s failed!");
    }
#endif
    INI_INIT_MUTEX(&file_mutex);
#ifdef INI_TEST
    ini_find_var_value(INI_MODU_WIFI, "wifi_ini_mode", auc_value);
    ini_find_var_value(INI_MODU_GNSS, "gnss_ini_mode", auc_value);
    ini_find_var_value(INI_MODU_BT, "bt_ini_mode", auc_value);
    ini_find_var_value(INI_MODU_FM, "fm_ini_mode", auc_value);
    ini_find_var_value(INI_MODU_WIFI_PLAT, "wifi_plat_ini_mode", auc_value);
    ini_find_var_value(INI_MODU_BFG_PLAT, "bfg_plat_ini_mode", auc_value);
    ini_find_var_value(INI_MODU_WIFI, "loglevel", auc_value);
    ini_find_var_value(INI_MODU_WIFI, "data_rate_down", auc_value);
    ini_find_var_value(INI_MODU_WIFI, "data_rate_up", auc_value);
    ini_find_var_value(INI_MODU_GNSS, "gnss_gpsglonass", auc_value);
    ini_find_var_value(INI_MODU_GNSS, "gnss_bdgps", auc_value);
    ini_find_var_value(INI_MODU_BT, "bt_normal", auc_value);
    ini_find_var_value(INI_MODU_FM, "fm_normal", auc_value);
    ini_find_var_value(INI_MODU_WIFI_PLAT, "wifi_plat_normal", auc_value);
    ini_find_var_value(INI_MODU_BFG_PLAT, "bfg_plat_normal", auc_value);
#endif

    return INI_SUCC;
}
OAL_EXPORT_SYMBOL(ini_cfg_init);

void ini_cfg_exit(void)
{
    INI_DEBUG("hi110x ini config search exit!\n");
}
OAL_EXPORT_SYMBOL(ini_cfg_exit);
#else
#ifdef INI_TEST
oal_int32 main(oal_int32 argc, oal_int8** argv)
{
    oal_int8 auc_value[50];

    INI_DEBUG("k3v2 .ini config search init!\n");

    ini_write_var_value(INI_MODU_WIFI, "wifi_ini_mode", "2");
    ini_write_var_value(INI_MODU_WIFI, "wifi_ini_mode", "1");
    ini_write_var_value(INI_MODU_WIFI, "wifi_ini_mode", "0");
    ini_write_var_value(INI_MODU_GNSS, "gnss_ini_mode", "1");
    ini_write_var_value(INI_MODU_BT, "bt_ini_mode", "0");
    ini_write_var_value(INI_MODU_FM, "fm_ini_mode", "0");
    ini_write_var_value(INI_MODU_WIFI_PLAT, "wifi_plat_ini_mode", "0");
    ini_write_var_value(INI_MODU_BFG_PLAT, "bfg_plat_ini_mode", "0");
    ini_find_var_value(INI_MODU_WIFI, "wifi_ini_mode", auc_value);
    ini_find_var_value(INI_MODU_GNSS, "gnss_ini_mode", auc_value);
    ini_find_var_value(INI_MODU_BT, "bt_ini_mode", auc_value);
    ini_find_var_value(INI_MODU_FM, "fm_ini_mode", auc_value);
    ini_find_var_value(INI_MODU_WIFI_PLAT, "wifi_plat_ini_mode", auc_value);
    ini_find_var_value(INI_MODU_BFG_PLAT, "bfg_plat_ini_mode", auc_value);
    ini_find_var_value(INI_MODU_WIFI, "loglevel", auc_value);
    ini_find_var_value(INI_MODU_WIFI, "data_rate_down", auc_value);
    ini_find_var_value(INI_MODU_WIFI, "data_rate_up", auc_value);
    ini_find_var_value(INI_MODU_WIFI, "hcc_rx_lo_queue", auc_value);
    ini_find_var_value(INI_MODU_GNSS, "gnss_gpsglonass", auc_value);
    ini_find_var_value(INI_MODU_GNSS, "gnss_bdgps", auc_value);
    ini_find_var_value(INI_MODU_BT, "bt_normal", auc_value);
    ini_find_var_value(INI_MODU_FM, "fm_normal", auc_value);
    ini_find_var_value(INI_MODU_WIFI_PLAT, "wifi_plat_normal", auc_value);
    ini_find_var_value(INI_MODU_BFG_PLAT, "bfg_plat_normal", auc_value);
    ini_find_var_value(INI_MODU_PLAT, "g_param_version.param_version", auc_value);
    INI_DEBUG("k3v2 .ini config over!\n");
    return 0;
}
#endif
#endif
