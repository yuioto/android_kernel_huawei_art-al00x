

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "plat_firmware.h"
#include "plat_debug.h"
#include "oal_file.h"
#include "oal_sdio_host_if.h"
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "plat_sdio.h"
#include "plat_cali.h"
#include "plat_pm.h"
#include "hisi_ini.h"
#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#include "plat_sdio.h"
#include "los_event.h"
#include "los_typedef.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "fcntl.h"
#include "los_exc.h"
#include "oal_util.h"
#include "oal_time.h"
#include "plat_pm.h"
#endif

#include "oal_channel_host_if.h"
#include "securec.h"
#include "exception_rst.h"

#ifdef _PRE_HI113X_FS_DISABLE
static oal_uint8 firmware_array_wifi_cfg_c01[] = {
#include "c01/plat_wifi_cfg.h"
};

static oal_uint8 firmware_array_rw_bin_c01[] = {
#include "c01/plat_rw.h"
};

DECLARE_FIRMWARE_FILE(wifi_cfg_c01);
DECLARE_FIRMWARE_FILE(rw_bin_c01);

static firmware_file_stru *g_st_wifi_cfg[SOFT_VER_BUTT] = {
    &firmware_file_wifi_cfg_c01,
};

static firmware_file_stru *g_st_rw_bin[SOFT_VER_BUTT] = {
    &firmware_file_rw_bin_c01,
};

#endif
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define WIFI_CFG_C01_PATH                   FIRMWARE_PATH"/"WIFI_CFG_C01_NAME
#define RAM_CHECK_CFG_PATH                  FIRMWARE_PATH"/"RAM_CHECK_CFG_NAME
#define STORE_WIFI_MEM                      MEMDUMP_PATH"dump_mem"

#define FILE_COUNT_PER_SEND           1
#define MIN_FIRMWARE_FILE_TX_BUF_LEN  4096

#define DEVICE_EFUSE_ADDR    0x50000764
#define DEVICE_EFUSE_LENGTH  16

#define DUMP_PCLR_LEN        (2 * (sizeof(uint32_t)))
/*****************************************************************************
  3 全局变量定义
*****************************************************************************/
oal_uint8 *g_auc_cfg_path[SOFT_VER_BUTT] =
{
    (oal_uint8 *)WIFI_CFG_C01_PATH,
    // WIFI_CFG_C02_PATH,
};

/* 存储cfg文件信息，解析cfg文件时赋值，加载的时候使用该变量 */
FIRMWARE_GLOBALS_STRUCT  g_st_cfg_info;
oal_uint32 g_ulJumpCmdResult = CMD_JUMP_EXEC_RESULT_SUCC;
efuse_info_stru g_st_efuse_info = {
    .soft_ver = SOFT_VER_BUTT,
    .mac_h = 0x0,
    .mac_m = 0x0,
    .mac_l = 0x0,
};

// mem check
uint32_t g_mem_check_result = DEVICE_MEM_CHECK_RESULT_FAIL;
// abb trim
uint8_t  g_efuse_die_id[DEVICE_EFUSE_DIEID_LENGTH];
// firmware req
FIRMWARE_MEM *g_firmware_mem = NULL;
/*****************************************************************************
  4 函数实现
*****************************************************************************/
static void firmware_mem_free(FIRMWARE_MEM *firmware_mem);

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
int usleep(unsigned useconds);
int32_t plat_usb_init(void);
void plat_usb_destory(void);
#endif


static oal_int32 firmware_read_msg(oal_uint8 *data, oal_int32 len)
{
    oal_int32  l_len;

    if (OAL_UNLIKELY((data == NULL))) {
        PS_PRINT_ERR("data is NULL\n ");
        return -OAL_EFAIL;
    }
    l_len = oal_channel_patch_readsb(data, len, READ_MEG_TIMEOUT);
    PS_PRINT_DBG("Receive l_len=[%d]\n", l_len);

    return l_len;
}

static oal_int32 firmware_read_msg_timeout(oal_uint8 *data, oal_int32 len, oal_uint32 timeout)
{
    oal_int32  l_len;

    if (OAL_UNLIKELY((data == NULL))) {
        PS_PRINT_ERR("data is NULL\n ");
        return -OAL_EFAIL;
    }

    l_len = oal_channel_patch_readsb(data, len, timeout);
    PS_PRINT_DBG("Receive l_len=[%d], data = [%s]\n", l_len, data);

    return l_len;
}


static oal_int32 firmware_send_msg(oal_uint8 *data, oal_int32 len)
{
    oal_int32   l_ret;

    PS_PRINT_DBG("len = %d\n", len);
#ifdef HW_DEBUG
    print_hex_dump_bytes("firmware_send_msg :", DUMP_PREFIX_ADDRESS, data,
                         (len < 128 ? len : 128));
#endif
    l_ret = oal_channel_patch_writesb(data, len);
    return l_ret;
}

int32_t firmware_mem_init(void)
{
    g_firmware_mem = oal_kzalloc(sizeof(FIRMWARE_MEM), OAL_GFP_KERNEL);
    if (g_firmware_mem == NULL) {
        PS_PRINT_ERR("g_st_firmware_mem KMALLOC failed\n");
        goto nomem;
    }

    g_firmware_mem->ulDataBufLen = MIN_FIRMWARE_FILE_TX_BUF_LEN;
    if (g_firmware_mem->ulDataBufLen < HISDIO_BLOCK_SIZE) {
        PS_PRINT_ERR("sdio max transmit size [%d] is error!\n",  g_firmware_mem->ulDataBufLen);
        goto nomem;
    }

    PS_PRINT_INFO("try to malloc firmware download file buf len is [%d]\n", g_firmware_mem->ulDataBufLen);
    g_firmware_mem->pucDataBuf = (oal_uint8 *)OS_KMALLOC_GFP(g_firmware_mem->ulDataBufLen);
    if (g_firmware_mem->pucDataBuf == NULL) {
        PS_PRINT_ERR("pucDataBuf KMALLOC failed\n");
        goto nomem;
    }
    PS_PRINT_INFO("download firmware file buf len is [%d]\n", g_firmware_mem->ulDataBufLen);

    g_firmware_mem->puc_recv_cmd_buff = (oal_uint8 *)OS_KMALLOC_GFP(CMD_BUFF_LEN);
    if (g_firmware_mem->puc_recv_cmd_buff == NULL) {
        PS_PRINT_ERR("puc_recv_cmd_buff KMALLOC failed\n");
        goto nomem;
    }

    g_firmware_mem->puc_send_cmd_buff = (oal_uint8 *)OS_KMALLOC_GFP(CMD_BUFF_LEN);
    if (g_firmware_mem->puc_send_cmd_buff == NULL) {
        PS_PRINT_ERR("puc_recv_cmd_buff KMALLOC failed\n");
        goto nomem;
    }

    return OAL_SUCC;

nomem:
    firmware_mem_free(g_firmware_mem);
    return -OAL_EFAIL;
}

static FIRMWARE_MEM *firmware_mem_request(void)
{
    return g_firmware_mem;
}

static void firmware_mem_free(FIRMWARE_MEM *firmware_mem)
{
    if (firmware_mem == NULL) {
        PS_PRINT_ERR("g_firmware_mem_mutex is null\n");
        return;
    }
    if (firmware_mem->puc_send_cmd_buff != NULL) {
        oal_free(firmware_mem->puc_send_cmd_buff);
    }
    if (firmware_mem->puc_recv_cmd_buff != NULL) {
        oal_free(firmware_mem->puc_recv_cmd_buff);
    }
    if (firmware_mem->pucDataBuf != NULL) {
        oal_free(firmware_mem->pucDataBuf);
    }
    oal_free(firmware_mem);
    firmware_mem = NULL;
}


static oal_int32 recv_expect_result(const oal_uint8 *expect, FIRMWARE_MEM *firmware_mem)
{
    oal_int32 l_len;
    oal_int32 i;

    if (!OAL_STRLEN((int8_t *)expect)) {
        PS_PRINT_DBG("not wait device to respond!\n");
        return OAL_SUCC;
    }
    if (firmware_mem == NULL || firmware_mem->puc_recv_cmd_buff == NULL) {
        PS_PRINT_ERR("puc_recv_cmd_buff = NULL \n");
        return -OAL_EFAIL;
    }
    memset_s(firmware_mem->puc_recv_cmd_buff, CMD_BUFF_LEN, 0, CMD_BUFF_LEN);
    for (i = 0; i < HOST_DEV_TIMEOUT; i++) {
        l_len = firmware_read_msg(firmware_mem->puc_recv_cmd_buff, CMD_BUFF_LEN);
        if (l_len < 0) {
            PS_PRINT_ERR("recv result fail\n");
            return -OAL_EFAIL;
        }
        if (!oal_memcmp(firmware_mem->puc_recv_cmd_buff, expect, OAL_STRLEN((int8_t *)expect))) {
            PS_PRINT_DBG(" send OAL_SUCC, expect [%s] ok\n", expect);
            return OAL_SUCC;
        } else {
            PS_PRINT_WARNING(" error result[%s], expect [%s], read result again\n", 
                firmware_mem->puc_recv_cmd_buff, expect);
        }
    }

    return -OAL_EFAIL;
}


static oal_int32 recv_expect_result_timeout(const oal_uint8 *expect, FIRMWARE_MEM *firmware_mem, oal_uint32 timeout)
{
    oal_int32 l_len;

    if (!OAL_STRLEN((int8_t *)expect)) {
        PS_PRINT_DBG("not wait device to respond!\n");
        return OAL_SUCC;
    }

    memset_s(firmware_mem->puc_recv_cmd_buff, CMD_BUFF_LEN, 0, CMD_BUFF_LEN);
    l_len = firmware_read_msg_timeout(firmware_mem->puc_recv_cmd_buff, CMD_BUFF_LEN, timeout);
    if (l_len < 0) {
        PS_PRINT_ERR("recv result fail\n");
        return -OAL_EFAIL;
    }

    if (!oal_memcmp(firmware_mem->puc_recv_cmd_buff, expect, OAL_STRLEN((int8_t *)expect))) {
        PS_PRINT_DBG(" send OAL_SUCC, expect [%s] ok\n", expect);
        return OAL_SUCC;
    } else {
        PS_PRINT_WARNING(" error result[%s], expect [%s], read result again\n",
                         firmware_mem->puc_recv_cmd_buff, expect);
    }

    return -OAL_EFAIL;
}


static oal_int32 msg_send_and_recv_except(oal_uint8 *data,
                                          oal_int32 len,
                                          const oal_uint8 *expect,
                                          FIRMWARE_MEM *firmware_mem)
{
    oal_int32  i;
    oal_int32  l_ret;

    for (i = 0; i < HOST_DEV_TIMEOUT; i++) {
        l_ret = firmware_send_msg(data, len);
        if (l_ret < 0) {
            continue;
        }

        l_ret = recv_expect_result(expect, firmware_mem);
        if (l_ret == 0) {
            return OAL_SUCC;
        }
    }

    return -OAL_EFAIL;
}


static void *malloc_cmd_buf(oal_uint8 *puc_cfg_info_buf, oal_uint32 ul_index)
{
    oal_int32           l_len;
    oal_uint8          *flag = OAL_PTR_NULL;
    oal_uint8          *p_buf = OAL_PTR_NULL;

    if (puc_cfg_info_buf == NULL) {
        PS_PRINT_ERR("malloc_cmd_buf: buf is NULL!\n");
        return NULL;
    }

    /* 统计命令个数 */
    flag = puc_cfg_info_buf;
    g_st_cfg_info.al_count[ul_index] = 0;
    while (flag != NULL) {
        /* 一个正确的命令行结束符为 ; */
        flag = (uint8_t *)OAL_STRCHR((int8_t *)flag, CMD_LINE_SIGN);
        if (flag == NULL) {
            break;
        }
        g_st_cfg_info.al_count[ul_index]++;
        flag++;
    }
    PS_PRINT_DBG("cfg file cmd count: al_count[%d] = %d\n", ul_index, g_st_cfg_info.al_count[ul_index]);

    /* 申请存储命令空间 */
    l_len = ((g_st_cfg_info.al_count[ul_index]) + CFG_INFO_RESERVE_LEN) * sizeof(struct cmd_type_st);

    p_buf = OS_KMALLOC_GFP(l_len);
    if (p_buf == NULL) {
        PS_PRINT_ERR("kmalloc cmd_type_st fail\n");
        return NULL;
    }
    memset_s((void *)p_buf, l_len, 0, l_len);

    return p_buf;
}


static oal_uint8 *delete_space(oal_uint8 *string, oal_int32 *len)
{
    int i;

    if ((string == NULL) || (len == NULL)) {
        return NULL;
    }

    /* 删除尾部的空格 */
    for (i = *len - 1; i >= 0; i--) {
        if (string[i] != COMPART_KEYWORD) {
            break;
        }
        string[i] = '\0';
    }
    /* 出错 */
    if (i < 0) {
        PS_PRINT_ERR(" string is Space bar\n");
        return NULL;
    }
    /* 在for语句中减去1，这里加上1 */
    *len = i + 1;

    /* 删除头部的空格 */
    for (i = 0; i < *len; i++) {
        if (string[i] != COMPART_KEYWORD) {
            /* 减去空格的个数 */
            *len = *len - i;
            return &string[i];
        }
    }

    return NULL;
}


static oal_file_stru*  open_file_to_readm(oal_uint8 *name)
{
    oal_file_stru *fp = OAL_PTR_NULL;
    int8_t *file_name = NULL;

    if (OAL_WARN_ON(name == NULL)) {
        file_name = "/data/memdump/readm_wifi";
    } else {
        file_name = (int8_t *)name;
    }
    fp = oal_file_open(file_name, (OAL_O_CREAT | OAL_O_RDWR | OAL_O_TRUNC), 0);

    return fp;
}


static oal_int32 recv_device_mem(oal_file_stru *fp, oal_uint8 *pucDataBuf, oal_int32 len)
{
    oal_int32 l_ret = 0;
    oal_uint8 retry = 3;
    oal_int32 lenbuf = 0;

    if (IS_ERR_OR_NULL(fp)) {
        PS_PRINT_ERR("fp is error,fp = 0x%p\n", fp);
        return -OAL_EFAIL;
    }

    if (pucDataBuf == NULL) {
        PS_PRINT_ERR("pucDataBuf is NULL\n");
        return -OAL_EFAIL;
    }

    PS_PRINT_DBG("expect recv len is [%d]\n", len);

    while (len > lenbuf) {
        l_ret = firmware_read_msg(pucDataBuf + lenbuf, len - lenbuf);
        if (l_ret > 0) {
            lenbuf += l_ret;
        } else {
            retry--;
            lenbuf = 0;
            if (retry == 0) {
                l_ret = -OAL_EFAIL;
                PS_PRINT_ERR("time out\n");
                break;
            }
        }
    }

    if (len <= lenbuf) {
        oal_file_write(fp, (int8_t *)pucDataBuf, len);
    }

    return l_ret;
}


static oal_int32 check_version(FIRMWARE_MEM *firmware_mem)
{
    oal_int32   l_ret;
    oal_uint32  l_len;
    oal_int32   i;

    if (firmware_mem == NULL || 
       firmware_mem->puc_recv_cmd_buff == NULL || firmware_mem->puc_send_cmd_buff == NULL) {
        return -OAL_EFAIL;
    }

    for (i = 0; i < HOST_DEV_TIMEOUT; i++) {
        memset_s(firmware_mem->puc_send_cmd_buff, CMD_BUFF_LEN, 0, CMD_BUFF_LEN);

        if (memcpy_s(firmware_mem->puc_send_cmd_buff, CMD_BUFF_LEN,
                     (oal_uint8 *)VER_CMD_KEYWORD, OAL_STRLEN(VER_CMD_KEYWORD)) != EOK) {
            return -OAL_EFAIL;
        }
        l_len = OAL_STRLEN(VER_CMD_KEYWORD);

        firmware_mem->puc_send_cmd_buff[l_len] = COMPART_KEYWORD;
        l_len++;

#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
        l_len = HISDIO_ALIGN_4_OR_BLK(l_len + 1);
#endif
        l_ret = firmware_send_msg(firmware_mem->puc_send_cmd_buff, l_len);
        if (l_ret < 0) {
            continue;
        }

        memset_s(g_st_cfg_info.auc_DevVersion, VERSION_LEN, 0, VERSION_LEN);
        memset_s(firmware_mem->puc_recv_cmd_buff, CMD_BUFF_LEN, 0, CMD_BUFF_LEN);
        l_ret = firmware_read_msg(firmware_mem->puc_recv_cmd_buff, CMD_BUFF_LEN);
        if (l_ret < 0) {
            continue;
        }
        if (memcpy_s(g_st_cfg_info.auc_DevVersion, VERSION_LEN,
                     firmware_mem->puc_recv_cmd_buff, VERSION_LEN) != EOK) {
            return -OAL_EFAIL;
        }

        if (!oal_memcmp((oal_int8 *)g_st_cfg_info.auc_DevVersion,
                        (oal_int8 *)g_st_cfg_info.auc_CfgVersion,
                        OAL_STRLEN((int8_t *)g_st_cfg_info.auc_CfgVersion))) {
            PS_PRINT_INFO("OAL_SUCC: Device Version = [%s], CfgVersion = [%s].\n",
                          g_st_cfg_info.auc_DevVersion, g_st_cfg_info.auc_CfgVersion);
            return OAL_SUCC;
        } else {
            PS_PRINT_ERR("ERROR version,Device Version = [%s], CfgVersion = [%s].\n",
                         g_st_cfg_info.auc_DevVersion, g_st_cfg_info.auc_CfgVersion);
            return -OAL_EFAIL;
        }
    }

    return -OAL_EFAIL;
}


static oal_int32 number_type_cmd_send(const oal_uint8 *Key, const oal_uint8 *Value, FIRMWARE_MEM *firmware_mem)
{
    oal_int32       l_ret, i, n;
    oal_int32       data_len, data_end_index, Value_len;

    if (firmware_mem == NULL || firmware_mem->puc_recv_cmd_buff == NULL || firmware_mem->puc_send_cmd_buff == NULL) {
        return -OAL_EFAIL;
    }

    if (OAL_STRLEN((int8_t *)Key) + OAL_STRLEN((int8_t *)Value) + CMD_SEND_RESERVE_LEN > CMD_BUFF_LEN) {
        PS_PRINT_ERR("the cmd string must be error, key=%s, vlaue=%s \n", Key, Value);
        return -OAL_EFAIL;
    }

    Value_len = OAL_STRLEN((oal_int8 *)Value);

    memset_s(firmware_mem->puc_recv_cmd_buff, CMD_BUFF_LEN, 0, CMD_BUFF_LEN);
    memset_s(firmware_mem->puc_send_cmd_buff, CMD_BUFF_LEN, 0, CMD_BUFF_LEN);

    data_len = OAL_STRLEN((int8_t *)Key);
    if (memcpy_s(firmware_mem->puc_send_cmd_buff, CMD_BUFF_LEN, Key, data_len) != EOK) {
        return -OAL_EFAIL;
    }

    firmware_mem->puc_send_cmd_buff[data_len] = COMPART_KEYWORD;
    data_len = data_len + 1;

    for (i = 0, n = 0; (i <= Value_len) && (n < INT32_STR_LEN); i++) {
        if ((Value[i] == ',') || (i == Value_len)) {
            PS_PRINT_DBG("auc_num = %s, i = %d, n = %d\n", firmware_mem->puc_recv_cmd_buff,  i, n);
            if (n == 0) {
                continue;
            }
            if (memcpy_s((oal_uint8 *)&firmware_mem->puc_send_cmd_buff[data_len], CMD_BUFF_LEN - data_len,
                         firmware_mem->puc_recv_cmd_buff, n) != EOK) {
                return -OAL_EFAIL;
            }
            data_len = data_len + n;

            firmware_mem->puc_send_cmd_buff[data_len] = COMPART_KEYWORD;
            data_len = data_len + 1;

            if (memset_s(firmware_mem->puc_recv_cmd_buff, INT32_STR_LEN, 0, INT32_STR_LEN) != EOK) {
                return -OAL_EFAIL;
            }
            n = 0;
        } else if (Value[i] == COMPART_KEYWORD) {
            continue;
        } else {
            firmware_mem->puc_recv_cmd_buff[n] = Value[i];
            n++;
        }
    }
    data_end_index = data_len + 1;
    firmware_mem->puc_send_cmd_buff[data_end_index] = '\0';
    PS_PRINT_DBG("cmd=%s\r\n", firmware_mem->puc_send_cmd_buff);
    l_ret = firmware_send_msg(firmware_mem->puc_send_cmd_buff, data_len);

    return l_ret;
}


static oal_int32 parse_file_cmd(oal_uint8 *string, oal_ulong *addr, oal_int8 **path)
{
    oal_uint8 *tmp = OAL_PTR_NULL;
    oal_int32 count;
    oal_uint8 *after = NULL;

    if (string == NULL || addr == NULL || path == NULL) {
        PS_PRINT_ERR("param is error!\n");
        return -OAL_EFAIL;
    }

    /* 获得发送的文件的个数，此处必须为1，string字符串的格式必须是"1,0xXXXXX,file_path" */
    tmp = string;
    while (*tmp == COMPART_KEYWORD) {
        tmp++;
    }
    count = oal_simple_strtoul((int8_t *)tmp, NULL, 10);
    if (count != FILE_COUNT_PER_SEND) {
        PS_PRINT_ERR("the count of send file must be 1, count = [%d]\n", count);
        return -OAL_EFAIL;
    }

    /* 让tmp指向地址的首字母 */
    tmp = (uint8_t *)OAL_STRCHR((int8_t *)string, ',');
    if (tmp == NULL) {
        PS_PRINT_ERR("param string is err!\n");
        return -OAL_EFAIL;
    } else {
        tmp++;
        while (*tmp == COMPART_KEYWORD) {
            tmp++;
        }
    }

    *addr = oal_simple_strtoul((oal_int8 *)tmp, (char **)(&after), 16);
    PS_PRINT_DBG("file to send addr:[0x%lx]\n", *addr);

    /* "1,0xXXXX,file_path" */
    /*         ^          */
    /*       after        */
    after++;
    while (*after == COMPART_KEYWORD) {
        after++;
    }

    PS_PRINT_DBG("after:[%s]\n", after);
    *path = (int8_t *)after;

    return OAL_SUCC;
}


static oal_int32 read_device_mem(p_st_wifi_demp_mem_info pst_mem_dump_info,
                                 oal_file_stru *fp, FIRMWARE_MEM *firmware_mem)
{
    oal_int32 ret = 0;
    oal_uint32 size = 0;
    oal_uint32 offset;
    oal_uint32 remainder = pst_mem_dump_info->size;

    offset = 0;
    if (firmware_mem == NULL || firmware_mem->puc_send_cmd_buff == NULL) {
        PS_PRINT_ERR("puc_send_cmd_buff = NULL \n");
        return -OAL_EFAIL;
    }
    while (remainder > 0) {
        memset_s(firmware_mem->puc_send_cmd_buff, CMD_BUFF_LEN, 0, CMD_BUFF_LEN);

        size = OAL_MIN(remainder, firmware_mem->ulDataBufLen);
        if (snprintf_s((int8_t *)firmware_mem->puc_send_cmd_buff, CMD_BUFF_LEN, CMD_BUFF_LEN - 1, "%s%c0x%lx%c%d%c",
                       RMEM_CMD_KEYWORD,
                       COMPART_KEYWORD,
                       (pst_mem_dump_info->mem_addr + (oal_ulong)offset),
                       COMPART_KEYWORD,
                       size,
                       COMPART_KEYWORD) < EOK) {
            PS_PRINT_ERR("read_device_mem::snprintf_s failed!\n");
            return -OAL_EFAIL;
        }
        PS_PRINT_DBG("read mem cmd:[%s]\n", firmware_mem->puc_send_cmd_buff);
        ret = firmware_send_msg(firmware_mem->puc_send_cmd_buff,
                                OAL_STRLEN((int8_t *)firmware_mem->puc_send_cmd_buff));
        if (ret < 0) {
            PS_PRINT_ERR("wifi mem dump fail, mem_addr is [0x%lx],ret=%d\n", pst_mem_dump_info->mem_addr, ret);
            break;
        }

        ret = recv_device_mem(fp, firmware_mem->pucDataBuf, size);
        if (ret < 0) {
            PS_PRINT_ERR("wifi mem dump fail, mem_addr is [0x%lx],ret=%d\n", pst_mem_dump_info->mem_addr, ret);
            break;
        }

        offset += size;
        remainder -= size;
    }

    return ret;
}
static oal_int32 read_mem(oal_uint8 *Key, const oal_uint8 *Value, FIRMWARE_MEM *firmware_mem)
{
    oal_int32 l_ret;
    oal_int32 size;
    int8_t *flag = OAL_PTR_NULL;
    oal_file_stru *fp = OAL_PTR_NULL;
    struct st_wifi_dump_mem_info read_memory;
    memset_s(&read_memory, sizeof(struct st_wifi_dump_mem_info), 0, sizeof(struct st_wifi_dump_mem_info));

    flag = OAL_STRCHR((int8_t *)Value, ',');
    if (flag == NULL) {
        PS_PRINT_ERR("RECV LEN ERROR..\n");
        return -OAL_EFAIL;
    }
    if (firmware_mem == NULL || firmware_mem->pucDataBuf == NULL) {
        PS_PRINT_ERR("MEM IS NULL \n");
        return -OAL_EFAIL;
    }

    flag++;
    PS_PRINT_DBG("recv len [%s]\n", flag);
    while (*flag == COMPART_KEYWORD) {
        flag++;
    }
    size = oal_simple_strtoul(flag, NULL, 10);

    fp = open_file_to_readm((uint8_t *)DEVICE_MEM_CHECK_RESULT_PATH);
    if (IS_ERR_OR_NULL(fp)) {
        PS_PRINT_ERR("create file error,fp = 0x%p\n", fp);
        return -OAL_EFAIL;
    }

    read_memory.mem_addr = oal_simple_strtoul((int8_t *)Value, NULL, 16);
    read_memory.size = (oal_uint32)size;
    l_ret = read_device_mem(&read_memory, fp, firmware_mem);

    oal_file_close(fp);

    return l_ret;
}


static oal_int32 exec_number_type_cmd(oal_uint8 *Key, oal_uint8 *Value, FIRMWARE_MEM *firmware_mem)
{
    oal_int32       l_ret = -OAL_EFAIL;

    if (!oal_memcmp(Key, VER_CMD_KEYWORD, OAL_STRLEN(VER_CMD_KEYWORD))) {
        l_ret = check_version(firmware_mem);
        if (l_ret < 0) {
            PS_PRINT_ERR("check version FAIL [%d]\n", l_ret);
            return -OAL_EFAIL;
        }
    }

    if (!oal_strcmp((oal_int8 *)Key, WMEM_CMD_KEYWORD)) {
        l_ret = number_type_cmd_send(Key, Value, firmware_mem);
        if (l_ret < 0) {
            PS_PRINT_ERR("send key=[%s],value=[%s] fail\n", Key, Value);
            return l_ret;
        }

        l_ret = recv_expect_result((uint8_t *)MSG_FROM_DEV_WRITEM_OK, firmware_mem);
        if (l_ret < 0) {
            PS_PRINT_ERR("recv expect result fail!\n");
            return l_ret;
        }
    } else if (!oal_strcmp((oal_int8 *)Key, JUMP_CMD_KEYWORD)) {
        g_ulJumpCmdResult = CMD_JUMP_EXEC_RESULT_SUCC;
        l_ret = number_type_cmd_send(Key, Value, firmware_mem);
        if (l_ret < 0) {
            PS_PRINT_ERR("send key=[%s],value=[%s] fail\n", Key, Value);
            return l_ret;
        }

        /* 100000ms timeout */
        l_ret = recv_expect_result_timeout((uint8_t *)MSG_FROM_DEV_JUMP_OK, firmware_mem, READ_MEG_JUMP_TIMEOUT);
        if (l_ret >= 0) {
            PS_PRINT_INFO("JUMP success!\n");
            return l_ret;
        } else {
            PS_PRINT_ERR("CMD JUMP timeout! l_ret=%d\n", l_ret);
            g_ulJumpCmdResult = CMD_JUMP_EXEC_RESULT_FAIL;
            return l_ret;
        }
    } else if (!oal_strcmp((oal_int8 *)Key, RMEM_CMD_KEYWORD)) {
        l_ret = read_mem(Key, Value, firmware_mem);
    }

    return l_ret;
}


static oal_int32 exec_quit_type_cmd(FIRMWARE_MEM *firmware_mem)
{
    oal_int32   l_ret;
    oal_int32   l_len;

    if (firmware_mem == NULL || firmware_mem->puc_send_cmd_buff == NULL) {
        PS_PRINT_ERR("puc_send_cmd_buff = NULL \n");
        return -OAL_EFAIL;
    }

    if (memset_s(firmware_mem->puc_send_cmd_buff, CMD_BUFF_LEN, 0, CMD_BUFF_LEN) != EOK) {
        PS_PRINT_ERR("exec_quit_type_cmd::memset fail!");
        return -OAL_EFAIL;
    }

    if (memcpy_s(firmware_mem->puc_send_cmd_buff, CMD_BUFF_LEN,
                 (oal_uint8 *)QUIT_CMD_KEYWORD, OAL_STRLEN(QUIT_CMD_KEYWORD)) != EOK) {
        PS_PRINT_ERR("exec_quit_type_cmd::memcpy fail!");
        return -OAL_EFAIL;
    }
    l_len = OAL_STRLEN(QUIT_CMD_KEYWORD);

    firmware_mem->puc_send_cmd_buff[l_len] = COMPART_KEYWORD;
    l_len++;

    l_ret = msg_send_and_recv_except(firmware_mem->puc_send_cmd_buff, l_len, 
                                     (uint8_t *)MSG_FROM_DEV_QUIT_OK, 
                                     firmware_mem);

    return l_ret;
}

#ifndef _PRE_HI113X_FS_DISABLE

static oal_int32 exec_file_type_cmd(oal_uint8 *Key, oal_uint8 *Value, FIRMWARE_MEM *firmware_mem)
{
    oal_ulong addr;
    oal_uint32 addr_send;
    oal_int8 *path = OAL_PTR_NULL;
    oal_int32 ret;
    oal_uint32 file_len;
    oal_uint32 per_send_len;
    oal_uint32 send_count;
    oal_int32 rdlen;
    oal_uint32 i;
    oal_uint32 offset = 0;
    oal_file_stru *fp = OAL_PTR_NULL;

    if (firmware_mem == NULL ||
       firmware_mem->pucDataBuf == NULL ||
       firmware_mem->puc_send_cmd_buff == NULL ||
       firmware_mem->pucDataBuf == NULL) {
        PS_PRINT_ERR("mem is NULL \n");
        return -OAL_EFAIL;
    }

    ret = parse_file_cmd(Value, &addr, &path);
    if (ret < 0) {
        PS_PRINT_ERR("parse file cmd fail!\n");
        return ret;
    }

    fp = oal_file_open(path, (OAL_O_RDONLY), 0);
    if (IS_ERR_OR_NULL(fp)) {
        PS_PRINT_ERR("filp_open [%s] fail!!, fp=%p\n", path, fp);
        return -OAL_EFAIL;
    }

    /* 获取file文件大小 */
    file_len = oal_file_lseek(fp, 0, OAL_SEEK_END);
    if (file_len == 0) {
        PS_PRINT_ERR("file_len = 0! [%s] fail!!, fp=%p\n", path, fp);
        oal_file_close(fp);
        return -OAL_EFAIL;
    }

    /* 恢复fp->f_pos到文件开头 */
    oal_file_lseek(fp, 0, OAL_SEEK_SET);

    PS_PRINT_DBG("file len is [%d]\n", file_len);

    per_send_len = (firmware_mem->ulDataBufLen > file_len) ? file_len : firmware_mem->ulDataBufLen;

    send_count = (file_len + per_send_len - 1) / per_send_len;

    for (i = 0; i < send_count; i++) {
        rdlen = oal_file_read(fp, (int8_t *)firmware_mem->pucDataBuf, per_send_len);
        if (rdlen > 0) {
            PS_PRINT_DBG("len of kernel_read is [%d], i=%d\n", rdlen, i);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
            OAL_FILE_POS(fp) += rdlen;
#endif
        } else {
            PS_PRINT_ERR("len of kernel_read is error! ret=[%d], i=%d\n", rdlen, i);
            oal_file_close(fp);
            return -OAL_EFAIL;
        }

        addr_send = (oal_uint32)(addr + offset);
        PS_PRINT_DBG("send addr is [0x%x], i=%d\n", addr_send, i);

        if (snprintf_s((int8_t *)firmware_mem->puc_send_cmd_buff, CMD_BUFF_LEN, CMD_BUFF_LEN - 1, "%s%c%d%c0x%x%c",
                       FILES_CMD_KEYWORD, COMPART_KEYWORD,
                       FILE_COUNT_PER_SEND, COMPART_KEYWORD,
                       addr_send, COMPART_KEYWORD) < EOK) {
            PS_PRINT_ERR("exec_file_type_cmd::snprintf_s failed !\n");
            oal_file_close(fp);
            return -OAL_EFAIL;
        }

        /* 发送地址 */
        PS_PRINT_DBG("send file addr cmd is [%s]\n", firmware_mem->puc_send_cmd_buff);
        ret = msg_send_and_recv_except(firmware_mem->puc_send_cmd_buff,
                                       OAL_STRLEN((int8_t *)firmware_mem->puc_send_cmd_buff), 
                                       (uint8_t *)MSG_FROM_DEV_READY_OK, 
                                       firmware_mem);
        if (ret < 0) {
            PS_PRINT_ERR("SEND [%s] error\n", firmware_mem->puc_send_cmd_buff);
            oal_file_close(fp);
            return -OAL_EFAIL;
        }
        /* Wait at least 5 ms */
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        usleep_range(FILE_CMD_WAIT_TIME_MIN, FILE_CMD_WAIT_TIME_MAX);
#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
        usleep(FILE_CMD_WAIT_TIME_MIN);
#endif
        /* 发送文件内容 */
        ret = msg_send_and_recv_except(firmware_mem->pucDataBuf, rdlen, (uint8_t *)MSG_FROM_DEV_FILES_OK, firmware_mem);
        if (ret < 0) {
            PS_PRINT_ERR(" send data fail\n");
            oal_file_close(fp);
            return -OAL_EFAIL;
        }
        offset += rdlen;
    }
    oal_file_close(fp);

    /* 发送的长度要和文件的长度一致 */
    if (offset != file_len) {
        PS_PRINT_ERR("file send len is err! send len is [%d], file len is [%d]\n", offset, file_len);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}
#else
static oal_int32 exec_file_type_cmd(oal_uint8 *Key, oal_uint8 *Value, FIRMWARE_MEM *firmware_mem)
{
    oal_ulong addr;
    oal_uint32 addr_send;
    oal_int8 *path = OAL_PTR_NULL;
    oal_int32 ret;
    oal_uint32 file_len;
    oal_uint32 per_send_len;
    oal_uint32 send_count;
    oal_int32 rdlen;
    oal_uint32 i = 0;
    oal_uint32 offset = 0;
    oal_file_stru *fp;
    oal_uint8* p_offset = NULL;
    oal_int32 file_len_count;
    oal_uint32  ul_soft_ver;

    if (firmware_mem == NULL ||
       firmware_mem->pucDataBuf == NULL ||
       firmware_mem->puc_send_cmd_buff == NULL) {
        PS_PRINT_ERR("mem is NULL \n");
        return -OAL_EFAIL;
    }

    ul_soft_ver = get_device_soft_version();
    if (ul_soft_ver >= SOFT_VER_BUTT) {
        PS_PRINT_ERR("device soft version is invalid!\n");
        return -OAL_EFAIL;
    }

    ret = parse_file_cmd(Value, &addr, &path);
    if (ret < 0) {
        PS_PRINT_ERR("parse file cmd fail!\n");
        return ret;
    }

    file_len = g_st_rw_bin[ul_soft_ver]->len;
    file_len_count = file_len;

    PS_PRINT_DBG("download firmware file path: [%s], file len is [%d]\n", file_len);

    per_send_len = (firmware_mem->ulDataBufLen > file_len) ? file_len : firmware_mem->ulDataBufLen;

    while (file_len_count > 0) {
        rdlen = per_send_len < file_len_count ? per_send_len : file_len_count;
        if (memcpy_s(firmware_mem->pucDataBuf, firmware_mem->ulDataBufLen,
                     g_st_rw_bin[ul_soft_ver]->addr + offset, rdlen) != EOK) {
            PS_PRINT_ERR("exec_file_type_cmd: memcpy_s failed! \n");
            return -OAL_EFAIL;
        }

        addr_send = (oal_uint32)(addr + offset);
        if (snprintf_s(firmware_mem->puc_send_cmd_buff, CMD_BUFF_LEN, CMD_BUFF_LEN - 1, "%s%c%d%c0x%x%c",
                       FILES_CMD_KEYWORD, COMPART_KEYWORD, FILE_COUNT_PER_SEND,
                       COMPART_KEYWORD, addr_send, COMPART_KEYWORD) < EOK) {
            PS_PRINT_ERR("exec_file_type_cmd::snprintf_s failed !\n");
            return -OAL_EFAIL;
        }

        /* 发送地址 */
        PS_PRINT_DBG("send file addr:[0x%x],cmd:[%s]\n", addr_send, firmware_mem->puc_send_cmd_buff);
        ret = msg_send_and_recv_except(firmware_mem->puc_send_cmd_buff,
                                       OAL_STRLEN(firmware_mem->puc_send_cmd_buff), 
                                       MSG_FROM_DEV_READY_OK, 
                                       firmware_mem);
        if (ret < 0) {
            PS_PRINT_ERR("SEND [%s] error\n", firmware_mem->puc_send_cmd_buff);
            return -OAL_EFAIL;
        }
        /* Wait at least 5 ms */
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        usleep_range(FILE_CMD_WAIT_TIME_MIN, FILE_CMD_WAIT_TIME_MAX);
#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
        usleep(FILE_CMD_WAIT_TIME_MIN);
#endif
        /* 发送文件内容 */
        ret = msg_send_and_recv_except(firmware_mem->pucDataBuf, rdlen, MSG_FROM_DEV_FILES_OK, firmware_mem);
        if (ret < 0) {
            PS_PRINT_ERR(" send data fail\n");
            return -OAL_EFAIL;
        }

        offset += rdlen;
        file_len_count -= rdlen;
    }

    /* 发送的长度要和文件的长度一致 */
    if (offset != file_len) {
        PS_PRINT_ERR("file send len is err! send len is [%d], file len is [%d]\n", offset, file_len);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}
#endif


static oal_int32 execute_download_cmd(oal_int32 cmd_type,
                                      oal_uint8 *cmd_name,
                                      oal_uint8 *cmd_para,
                                      FIRMWARE_MEM *firmware_mem)
{
    oal_int32 l_ret;

    switch (cmd_type) {
        case FILE_TYPE_CMD:
            PS_PRINT_DBG(" command type FILE_TYPE_CMD\n");
            l_ret = exec_file_type_cmd(cmd_name, cmd_para, firmware_mem);
            break;
        case NUM_TYPE_CMD:
            PS_PRINT_DBG(" command type NUM_TYPE_CMD\n");
            l_ret = exec_number_type_cmd(cmd_name, cmd_para, firmware_mem);
            break;
        case QUIT_TYPE_CMD:
            PS_PRINT_DBG(" command type QUIT_TYPE_CMD\n");
            l_ret = exec_quit_type_cmd(firmware_mem);
            break;
        default:
            PS_PRINT_ERR("command type error[%d]\n", cmd_type);
            l_ret = -OAL_EFAIL;
            break;
    }

    return l_ret;
}


static oal_int32 firmware_read_cfg(const oal_uint8 *puc_CfgPatch, oal_uint8 *puc_read_buffer)
{
    oal_file_stru    *fp = OAL_PTR_NULL;
    oal_int32                   l_ret;

    if ((puc_CfgPatch == NULL) || (puc_read_buffer == NULL)) {
        PS_PRINT_ERR("para is NULL\n");
        return -OAL_EFAIL;
    }

    fp = oal_file_open((int8_t *)puc_CfgPatch, (OAL_O_RDONLY), 0);
    if (IS_ERR_OR_NULL(fp)) {
        PS_PRINT_ERR("open file %s fail, fp=%p\n", puc_CfgPatch, fp);
        return -OAL_EFAIL;
    }

    memset_s(puc_read_buffer, READ_CFG_BUF_LEN, 0, READ_CFG_BUF_LEN);
    l_ret = oal_file_read(fp, (int8_t *)puc_read_buffer, READ_CFG_BUF_LEN);
    *(puc_read_buffer + READ_CFG_BUF_LEN - 1) = '\0';
    oal_file_close(fp);
    fp = NULL;

    return l_ret;
}


static oal_int32 firmware_parse_cmd(oal_uint8 *puc_cfg_buffer, oal_uint8 *puc_cmd_name, oal_uint8 *puc_cmd_para)
{
    oal_int32       l_ret;
    oal_int32       l_cmdlen;
    oal_int32       l_paralen;
    int8_t         *begin;
    int8_t         *end = OAL_PTR_NULL;
    int8_t         *link = OAL_PTR_NULL;
    oal_uint8      *handle = OAL_PTR_NULL;
    oal_uint8      *handle_temp = OAL_PTR_NULL;

    begin = (int8_t *)puc_cfg_buffer;
    if (OAL_ANY_NULL_PTR3(puc_cfg_buffer, puc_cmd_name, puc_cmd_para)) {
        PS_PRINT_ERR("para is NULL\n");
        return ERROR_TYPE_CMD;
    }

    /* 注释行 */
    if (puc_cfg_buffer[0] == '@') {
        return ERROR_TYPE_CMD;
    }

    /* 错误行，或者退出命令行 */
    link = OAL_STRCHR(begin, '=');
    if (link == NULL) {
        /* 退出命令行 */
        if (OAL_STRSTR((oal_int8 *)puc_cfg_buffer, QUIT_CMD_KEYWORD) != NULL) {
            return QUIT_TYPE_CMD;
        }
        return ERROR_TYPE_CMD;
    }

    /* 错误行，没有结束符 */
    end = OAL_STRCHR(link, ';');
    if (end == NULL) {
        return ERROR_TYPE_CMD;
    }

    l_cmdlen = link - begin;

    /* 删除关键字的两边空格 */
    handle = delete_space((oal_uint8 *)begin, &l_cmdlen);
    if (handle == NULL) {
        return ERROR_TYPE_CMD;
    }

    /* 判断命令类型 */
    if (!oal_memcmp(handle, FILE_TYPE_CMD_KEY, OAL_STRLEN(FILE_TYPE_CMD_KEY))) {
        handle_temp = (uint8_t *)OAL_STRSTR((int8_t *)handle, FILE_TYPE_CMD_KEY);
        if (handle_temp == NULL) {
            PS_PRINT_ERR("'ADDR_FILE_'is not handle child string, handle=%s", handle);
            return ERROR_TYPE_CMD;
        }
        handle = handle_temp + OAL_STRLEN(FILE_TYPE_CMD_KEY);
        l_cmdlen = l_cmdlen - OAL_STRLEN(FILE_TYPE_CMD_KEY);
        l_ret = FILE_TYPE_CMD;
    } else if (!oal_memcmp(handle, NUM_TYPE_CMD_KEY, OAL_STRLEN(NUM_TYPE_CMD_KEY))) {
        handle_temp = (uint8_t *)OAL_STRSTR((int8_t *)handle, NUM_TYPE_CMD_KEY);
        if (handle_temp == NULL) {
            PS_PRINT_ERR("'PARA_' is not handle child string, handle=%s", handle);
            return ERROR_TYPE_CMD;
        }
        handle = handle_temp + OAL_STRLEN(NUM_TYPE_CMD_KEY);
        l_cmdlen = l_cmdlen - OAL_STRLEN(NUM_TYPE_CMD_KEY);
        l_ret = NUM_TYPE_CMD;
    } else {
        return ERROR_TYPE_CMD;
    }

    if (l_cmdlen > DOWNLOAD_CMD_LEN || l_cmdlen < 0) {
        PS_PRINT_ERR("cmd len out of range!\n");
        return ERROR_TYPE_CMD;
    }
    if (memcpy_s(puc_cmd_name, DOWNLOAD_CMD_LEN, handle, l_cmdlen) != EOK) {
        PS_PRINT_ERR("firmware_parse_cmd: memcpy_s failed! \n");
        return ERROR_TYPE_CMD;
    }

    /* 删除值两边空格 */
    begin = link + 1;
    l_paralen = end - begin;
    if (l_paralen > DOWNLOAD_CMD_PARA_LEN || l_paralen < 0) {
        PS_PRINT_ERR("para len out of range!\n");
        return ERROR_TYPE_CMD;
    }

    handle = delete_space((oal_uint8 *)begin, &l_paralen);
    if (handle == NULL)  {
        return ERROR_TYPE_CMD;
    }
    if (memcpy_s(puc_cmd_para, DOWNLOAD_CMD_PARA_LEN, handle, l_paralen) != EOK) {
        PS_PRINT_ERR("firmware_parse_cmd: memcpy_s failed! \n");
        return ERROR_TYPE_CMD;
    }

    return l_ret;
}


static oal_int32 firmware_parse_cfg(oal_uint8 *puc_cfg_info_buf, oal_int32 l_buf_len, oal_uint32 ul_index)
{
    oal_int32           i;
    oal_int32           l_len;
    oal_int32           l_ret;
    oal_uint8          *flag = OAL_PTR_NULL;
    oal_uint8          *begin = OAL_PTR_NULL;
    oal_uint8          *end = OAL_PTR_NULL;
    oal_int32           cmd_type;
    oal_uint8           cmd_name[DOWNLOAD_CMD_LEN];
    oal_uint8           cmd_para[DOWNLOAD_CMD_PARA_LEN];
    oal_uint32          cmd_para_len;

    if (puc_cfg_info_buf == NULL) {
        PS_PRINT_ERR("puc_cfg_info_buf is NULL!\n");
        return -OAL_EFAIL;
    }

    g_st_cfg_info.apst_cmd[ul_index] = (struct cmd_type_st *)malloc_cmd_buf(puc_cfg_info_buf, ul_index);
    if (g_st_cfg_info.apst_cmd[ul_index] == NULL) {
        PS_PRINT_ERR(" malloc_cmd_buf fail!\n");
        return -OAL_EFAIL;
    }

    /* 解析CMD BUF */
    flag = puc_cfg_info_buf;
    l_len = l_buf_len;
    i = 0;
    while ((i < g_st_cfg_info.al_count[ul_index]) && (flag < &puc_cfg_info_buf[l_len])) {
        /*
         * 获取配置文件中的一行,配置文件必须是unix格式.
         * 配置文件中的某一行含有字符 @ 则认为该行为注释行
         */
        begin = flag;
        end   = (uint8_t *)OAL_STRCHR((int8_t *)flag, '\n');
        if (end == NULL) {           /* 文件的最后一行，没有换行符 */
            PS_PRINT_DBG("lost of new line!\n");
            end = &puc_cfg_info_buf[l_len];
        } else if (end == begin) {     /* 该行只有一个换行符 */
            PS_PRINT_DBG("blank line\n");
            flag = end + 1;
            continue;
        }
        *end = '\0';

        PS_PRINT_DBG("operation string is [%s]\n", begin);

        memset_s(cmd_name, DOWNLOAD_CMD_LEN, 0, DOWNLOAD_CMD_LEN);
        memset_s(cmd_para, DOWNLOAD_CMD_PARA_LEN, 0, DOWNLOAD_CMD_PARA_LEN);
        cmd_type = firmware_parse_cmd(begin, cmd_name, cmd_para);

        PS_PRINT_DBG("cmd type=[%d],cmd_name=[%s],cmd_para=[%s]\n", cmd_type, cmd_name, cmd_para);

        if (cmd_type != ERROR_TYPE_CMD) { /* 正确的命令类型，增加 */
            g_st_cfg_info.apst_cmd[ul_index][i].cmd_type = cmd_type;
            l_ret = memcpy_s(g_st_cfg_info.apst_cmd[ul_index][i].cmd_name, DOWNLOAD_CMD_LEN, 
                             cmd_name, DOWNLOAD_CMD_LEN);
            l_ret += memcpy_s(g_st_cfg_info.apst_cmd[ul_index][i].cmd_para, DOWNLOAD_CMD_PARA_LEN, 
                              cmd_para, DOWNLOAD_CMD_PARA_LEN);
            if (l_ret != EOK) {
                PS_PRINT_ERR("firmware_parse_cfg: memcpy_s failed! \n");
                return -OAL_EFAIL;
            }
            g_st_cfg_info.apst_cmd[ul_index][i].cmd_name[DOWNLOAD_CMD_LEN - 1] = '\0';
            g_st_cfg_info.apst_cmd[ul_index][i].cmd_para[DOWNLOAD_CMD_PARA_LEN - 1] = '\0';
            /* 获取配置版本号 */
            if (!oal_memcmp(g_st_cfg_info.apst_cmd[ul_index][i].cmd_name,
                            VER_CMD_KEYWORD, OAL_STRLEN(VER_CMD_KEYWORD))) {
                cmd_para_len = OAL_STRLEN((int8_t *)g_st_cfg_info.apst_cmd[ul_index][i].cmd_para);
                if (cmd_para_len > VERSION_LEN) {
                    PS_PRINT_DBG("cmd_para_len = %d over auc_CfgVersion length", cmd_para_len);
                    return -OAL_EFAIL;
                }

                if (memcpy_s(g_st_cfg_info.auc_CfgVersion, VERSION_LEN,
                             g_st_cfg_info.apst_cmd[ul_index][i].cmd_para, cmd_para_len) != EOK) {
                    PS_PRINT_ERR("firmware_parse_cfg: memcpy_s failed! \n");
                    return -OAL_EFAIL;
                }
                PS_PRINT_DBG("g_CfgVersion = [%s].\n", g_st_cfg_info.auc_CfgVersion);
            }
            i++;
        }
        flag = end + 1;
    }

    /* 根据实际命令个数，修改最终的命令个数 */
    g_st_cfg_info.al_count[ul_index] = i;
    PS_PRINT_INFO("effective cmd count: al_count[%d] = %d\n", ul_index, g_st_cfg_info.al_count[ul_index]);

    return OAL_SUCC;
}


static oal_int32 firmware_get_cfg(oal_uint8 *puc_CfgPatch, oal_uint32 ul_index)
{
    oal_uint8   *puc_read_cfg_buf = OAL_PTR_NULL;
    oal_int32   l_readlen;
    oal_int32   l_ret;
    oal_uint32  ul_soft_ver = 0;

    if (puc_CfgPatch == NULL) {
        PS_PRINT_ERR("cfg file path is null!\n");
        return -OAL_EFAIL;
    }

    /* cfg文件限定在小于2048,如果cfg文件的大小确实大于2048，可以修改READ_CFG_BUF_LEN的值 */
    puc_read_cfg_buf = OS_KMALLOC_GFP(READ_CFG_BUF_LEN);
    if (puc_read_cfg_buf == NULL) {
        PS_PRINT_ERR("kmalloc READ_CFG_BUF fail!\n");
        return -OAL_EFAIL;
    }

    memset_s(puc_read_cfg_buf, READ_CFG_BUF_LEN, 0, READ_CFG_BUF_LEN);

#ifndef _PRE_HI113X_FS_DISABLE
    (oal_void)ul_soft_ver;
    l_readlen = firmware_read_cfg(puc_CfgPatch, puc_read_cfg_buf);
    if (l_readlen < 0) {
        PS_PRINT_ERR("read cfg error!\n");
        oal_free(puc_read_cfg_buf);
        puc_read_cfg_buf = NULL;
        return -OAL_EFAIL;
    } else if (l_readlen > READ_CFG_BUF_LEN - 1) {
    /* 减1是为了确保cfg文件的长度不超过READ_CFG_BUF_LEN，因为firmware_read_cfg最多只会读取READ_CFG_BUF_LEN长度的内容 */
        PS_PRINT_ERR("cfg file [%s] larger than %d\n", puc_CfgPatch, READ_CFG_BUF_LEN);
        oal_free(puc_read_cfg_buf);
        puc_read_cfg_buf = NULL;
        return -OAL_EFAIL;
    } else {
        PS_PRINT_DBG("read cfg file [%s] ok, size is [%d]\n", puc_CfgPatch, l_readlen);
    }
#else
    ul_soft_ver = get_device_soft_version();
    if (ul_soft_ver >= SOFT_VER_BUTT) {
        PS_PRINT_ERR("device soft version is invalid!\n");
        oal_free(puc_read_cfg_buf);
        puc_read_cfg_buf = NULL;
        return -OAL_EFAIL;
    }

    l_readlen = g_st_wifi_cfg[ul_soft_ver]->len;
    if (l_readlen > READ_CFG_BUF_LEN) {
        PS_PRINT_ERR("read cfg error!\n");
        oal_free(puc_read_cfg_buf);
        puc_read_cfg_buf = NULL;
        return -OAL_EFAIL;
    }

    if (memcpy_s(puc_read_cfg_buf, READ_CFG_BUF_LEN, g_st_wifi_cfg[ul_soft_ver]->addr, l_readlen) != EOK) {
        PS_PRINT_ERR("oal_memcopy error!\n");
        oal_free(puc_read_cfg_buf);
        puc_read_cfg_buf = NULL;
        return -OAL_EFAIL;
    }
#endif

    l_ret = firmware_parse_cfg(puc_read_cfg_buf, l_readlen, ul_index);
    if (l_ret < 0) {
        PS_PRINT_ERR("parse cfg error!\n");
    }

    oal_free(puc_read_cfg_buf);
    puc_read_cfg_buf = NULL;

    return l_ret;
}

int32_t firmware_execute_cmd(FIRMWARE_MEM *firmware_mem)
{
    int32_t     i;
    int32_t     ret;
    int32_t     l_cmd_type;
    uint8_t    *puc_cmd_name = OAL_PTR_NULL;
    uint8_t    *puc_cmd_para = OAL_PTR_NULL;

    PS_PRINT_INFO("start download NUM_TYPE_CMD\n");
    for (i = 0; i < g_st_cfg_info.al_count[WIFI_CFG]; i++) {
        l_cmd_type   = g_st_cfg_info.apst_cmd[WIFI_CFG][i].cmd_type;
        puc_cmd_name = g_st_cfg_info.apst_cmd[WIFI_CFG][i].cmd_name;
        puc_cmd_para = g_st_cfg_info.apst_cmd[WIFI_CFG][i].cmd_para;
        if (l_cmd_type == NUM_TYPE_CMD) {
            PS_PRINT_DBG("firmware down start cmd[%d]:type[%d], name[%s]\n", i, l_cmd_type, puc_cmd_name);
            ret = execute_download_cmd(l_cmd_type, puc_cmd_name, puc_cmd_para, firmware_mem);
            if (ret < 0) {
                PS_PRINT_ERR("download firmware cmd fail\n");
                return -OAL_EFAIL;
            }

            PS_PRINT_DBG("firmware down finish cmd[%d]:type[%d], name[%s]\n", i, l_cmd_type, puc_cmd_name);
        }
    }
    PS_PRINT_INFO("finish download firmware cmd\n");

    return OAL_SUCC;
}


int32_t device_create_mem_dump_file_name(p_st_wifi_demp_mem_info pst_mem_dump_info,
                                         int8_t *file_name,
                                         int32_t file_name_len)
{
    oal_int32   ret = EOK;
    oal_timeval_stru tv;
    oal_rtctime_stru tm;
    memset_s(&tv, sizeof(oal_timeval_stru), 0, sizeof(oal_timeval_stru));
    memset_s(&tm, sizeof(oal_rtctime_stru), 0, sizeof(oal_rtctime_stru));

    oal_do_gettimeofday(&tv);
    oal_rtc_time_to_tm(tv.tv_sec, &tm);

    if (pst_mem_dump_info == NULL || file_name == NULL) {
        PS_PRINT_ERR("device_create_mem_dump_file_name::para NULL\n");
        return -OAL_EFAIL;
    }

    ret = snprintf_s(file_name, file_name_len, file_name_len - 1, "%s_%04d%02d%02d%02d%02d%02d_%s.bin",
                     STORE_WIFI_MEM,
                     tm.tm_year + 1900,
                     tm.tm_mon + 1,
                     tm.tm_mday,
                     tm.tm_hour,
                     tm.tm_min,
                     tm.tm_sec,
                     pst_mem_dump_info->file_name);
    if (ret < EOK) {
        PS_PRINT_ERR("device_mem_dump::snprintf_s failed! \n");
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}



static oal_int32 recv_device_mem_etc(oal_uint8 *pucDataBuf, oal_int32 len)
{
    oal_int32 l_ret = 0;
    oal_uint8 retry = FIRMWARE_READ_TETRY_TIME;
    oal_int32 lenbuf = 0;

    if (pucDataBuf == NULL) {
        PS_PRINT_ERR("pucDataBuf is NULL\n");
        return -OAL_EFAIL;
    }

    PS_PRINT_DBG("expect recv len is [%d]\n", len);

    while (len > lenbuf) {
        l_ret = firmware_read_msg(pucDataBuf + lenbuf, len - lenbuf);
        if (l_ret > 0) {
            lenbuf += l_ret;
        } else {
            retry--;
            lenbuf = 0;
            if (retry == 0) {
                l_ret = -OAL_EFAIL;
                PS_PRINT_ERR("time out\n");
                break;
            }
        }
    }

    if (len <= lenbuf) {
        plat_exception_dump_enquenue(pucDataBuf, len);
    }

    return l_ret;
}



static oal_int32 read_device_mem_etc(p_st_wifi_demp_mem_info pst_mem_dump_info,
                                     FIRMWARE_MEM *firmware_mem)
{
    oal_int32 ret = 0;
    oal_uint32 size = 0;
    oal_uint32 offset = 0;
    oal_uint32 remainder = pst_mem_dump_info->size;
    uint32_t pos;
    uint32_t disallowed = OAL_FALSE;

    if (firmware_mem == NULL || firmware_mem->puc_send_cmd_buff == NULL) {
        PS_PRINT_ERR("puc_send_cmd_buff = NULL \n");
        return -OAL_EFAIL;
    }
    while (remainder > 0) {
        memset_s(firmware_mem->puc_send_cmd_buff, CMD_BUFF_LEN, 0, CMD_BUFF_LEN);

        size = OAL_MIN(remainder, firmware_mem->ulDataBufLen);
        if (snprintf_s((int8_t *)firmware_mem->puc_send_cmd_buff, CMD_BUFF_LEN, CMD_BUFF_LEN - 1, "%s%c0x%lx%c%d%c",
                       RMEM_CMD_KEYWORD,
                       COMPART_KEYWORD,
                       (pst_mem_dump_info->mem_addr + (oal_ulong)offset),
                       COMPART_KEYWORD,
                       size,
                       COMPART_KEYWORD) < EOK) {
            PS_PRINT_ERR("read_device_mem::snprintf_s failed!\n");
            return -OAL_EFAIL;
        }
        PS_PRINT_WARNING("read mem cmd:[%s]\n", firmware_mem->puc_send_cmd_buff);
        ret = firmware_send_msg(firmware_mem->puc_send_cmd_buff,
                                OAL_STRLEN((int8_t *)firmware_mem->puc_send_cmd_buff));
        if (ret < 0) {
            PS_PRINT_ERR("wifi mem dump fail, mem_addr is [0x%lx],ret=%d\n", pst_mem_dump_info->mem_addr, ret);
            break;
        }

        ret = recv_device_mem_etc(firmware_mem->pucDataBuf, size);
        if (ret < 0) {
            PS_PRINT_ERR("wifi mem dump fail, mem_addr is [0x%lx],ret=%d\n", pst_mem_dump_info->mem_addr, ret);
            break;
        }
        offset += size;
        remainder -= size;
        if ((offset >= PLAT_EXCEPTION_DEV_PANIC_LR_ADDR + DUMP_PCLR_LEN) && (disallowed == OAL_FALSE)) {
            disallowed = OAL_TRUE;
            pos = PLAT_EXCEPTION_DEV_PANIC_LR_ADDR - (offset - size);
            wifi_exception_dev_panic_info_get_etc(firmware_mem->pucDataBuf, pos, pst_mem_dump_info);
        }
    }

    return ret;
}


oal_int32 device_mem_dump_etc(p_st_wifi_demp_mem_info *pst_mem_dump_info)
{
    oal_int32 ret = OAL_SUCC;
    int32_t   i = 0;
    FIRMWARE_MEM *firmware_mem = NULL;
    oal_time_t_stru time_start, time_stop;
    oal_uint64  trans_us;
    const uint32_t ul_buff_size = sizeof(int32_t);
    uint8_t buff[ul_buff_size];
    uint32_t *pcount = (uint32_t *)&buff[0];

    if (pst_mem_dump_info == NULL) {
        PS_PRINT_ERR("pst_wifi_dump_info is NULL\n");
        return -OAL_EFAIL;
    }
    firmware_mem = firmware_mem_request();
    if (firmware_mem == NULL) {
        PS_PRINT_ERR("firmware_mem_request fail\n");
        return -OAL_EFAIL;
    }

    plat_exception_dump_notice();

    while (pst_mem_dump_info[i] != NULL) {
        time_start = oal_ktime_get();
        *pcount = pst_mem_dump_info[i]->size;
        plat_exception_dump_enquenue(buff, ul_buff_size);
        OAL_IO_PRINT("device mem addr:%lu, size:0x%x, filename:%s\n",
                     pst_mem_dump_info[i]->mem_addr,
                     pst_mem_dump_info[i]->size,
                     pst_mem_dump_info[i]->file_name);
        ret = read_device_mem_etc(pst_mem_dump_info[i], firmware_mem);
        if (ret < 0) {
            break;
        }
        time_stop = oal_ktime_get();
        trans_us = (oal_uint64)oal_ktime_to_us(oal_ktime_sub(time_stop, time_start));
        OAL_IO_PRINT("device get mem cost %llu us\n", trans_us);
        i++;
    }

    plat_exception_dump_finish();

    return ret;
}


oal_int32 firmware_download(oal_uint32 ul_index)
{
    oal_int32 l_ret;
    oal_int32 i;
    oal_int32 l_cmd_type;
    oal_uint8 *puc_cmd_name = OAL_PTR_NULL;
    oal_uint8 *puc_cmd_para = OAL_PTR_NULL;
    FIRMWARE_MEM *firmware_mem = NULL;

    if (ul_index >= CFG_FILE_TOTAL) {
        PS_PRINT_ERR("ul_index [%d] is error!\n", ul_index);
        return -OAL_EFAIL;
    }

    PS_PRINT_INFO("start download firmware, ul_index = [%d]\n", ul_index);

    if (g_st_cfg_info.al_count[ul_index] == 0) {
        PS_PRINT_ERR("firmware download cmd count is 0, ul_index = [%d]\n", ul_index);
        return -OAL_EFAIL;
    }

    firmware_mem = firmware_mem_request();
    if (firmware_mem == NULL) {
        PS_PRINT_ERR("firmware_mem_request fail\n");
        return -OAL_EFAIL;
    }

    for (i = 0; i < g_st_cfg_info.al_count[ul_index]; i++) {
        l_cmd_type   = g_st_cfg_info.apst_cmd[ul_index][i].cmd_type;
        puc_cmd_name = g_st_cfg_info.apst_cmd[ul_index][i].cmd_name;
        puc_cmd_para = g_st_cfg_info.apst_cmd[ul_index][i].cmd_para;

        PS_PRINT_DBG("cmd[%d]:type[%d], name[%s], para[%s]\n", i, l_cmd_type, puc_cmd_name, puc_cmd_para);

        PS_PRINT_DBG("firmware down start cmd[%d]:type[%d], name[%s]\n", i, l_cmd_type, puc_cmd_name);

        l_ret = execute_download_cmd(l_cmd_type, puc_cmd_name, puc_cmd_para, firmware_mem);
        if (l_ret < 0) {
            if (ul_index == RAM_REG_TEST_CFG) {
                if ((!oal_memcmp(puc_cmd_name, JUMP_CMD_KEYWORD, OAL_STRLEN(JUMP_CMD_KEYWORD))) && 
                    (g_ulJumpCmdResult == CMD_JUMP_EXEC_RESULT_FAIL)) {
                    /* device mem check 返回失败，继续执行READM命令，将结果读上来 */
                    PS_PRINT_ERR("Device Mem Reg check result is fail\n");
                    continue;
                }
            }
            PS_PRINT_ERR("download firmware fail\n");
            l_ret = -OAL_EFAIL;
            goto done;
        }

        PS_PRINT_DBG("firmware down finish cmd[%d]:type[%d], name[%s]\n", i, l_cmd_type, puc_cmd_name);
    }
    PS_PRINT_INFO("finish download firmware\n");
    l_ret = OAL_SUCC;
done:
    return l_ret;
}


EXPORT_SYMBOL(firmware_download);

efuse_info_stru *get_efuse_info_handler(oal_void)
{
    return &g_st_efuse_info;
}
EXPORT_SYMBOL(get_efuse_info_handler);

oal_uint32 get_device_soft_version(oal_void)
{
    efuse_info_stru *pst_efuse_info;
    oal_uint32   ul_soft_ver;
    pst_efuse_info = get_efuse_info_handler();
    if (pst_efuse_info == OAL_PTR_NULL) {
        PS_PRINT_ERR("pst_efuse_info is NULL!\n");
        return SOFT_VER_BUTT;
    }

    ul_soft_ver = pst_efuse_info->soft_ver;
    if (ul_soft_ver >= SOFT_VER_BUTT) {
        PS_PRINT_ERR("soft_ver is invalid!\n");
        return SOFT_VER_BUTT;
    }

    return ul_soft_ver;
}

static oal_int32 firmware_read_efuse_handle(FIRMWARE_MEM *firmware_mem,
                                            oal_ulong  addr,
                                            oal_uint32 size,
                                            efuse_info_stru *pst_efuse_info)
{
    oal_int32       l_ret;
    if ((firmware_mem == NULL) || (pst_efuse_info == NULL)) {
        PS_PRINT_ERR("para is NULL!\n");
        return -OAL_EFAIL;
    }
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    size = HISDIO_ALIGN_4_OR_BLK(size);
#endif
    if (size > firmware_mem->ulDataBufLen) {
        PS_PRINT_ERR("device mac length is too long !\n");
        return -OAL_EFAIL;
    }
    if (snprintf_s((int8_t *)firmware_mem->puc_send_cmd_buff, CMD_BUFF_LEN, CMD_BUFF_LEN - 1,
                   "%s%c0x%lx%c%d%c",
                   RMEM_CMD_KEYWORD, COMPART_KEYWORD, addr, COMPART_KEYWORD, size, COMPART_KEYWORD) < EOK) {
        PS_PRINT_ERR("firmware_read_efuse_info::snprintf_s failed !\n");
        return -OAL_EFAIL;
    }
    PS_PRINT_DBG("read mac cmd:[%s]\n", firmware_mem->puc_send_cmd_buff);
    l_ret = firmware_send_msg(firmware_mem->puc_send_cmd_buff, OAL_STRLEN((int8_t *)firmware_mem->puc_send_cmd_buff));
    if (l_ret < 0) {
        PS_PRINT_ERR("read device mac cmd send fail![%d]\n", l_ret);
        return -OAL_EFAIL;
    }

    l_ret = firmware_read_msg(firmware_mem->pucDataBuf, size);
    if (l_ret < 0) {
        PS_PRINT_ERR("read device mac fail![%d]\n", l_ret);
        return -OAL_EFAIL;
    }

    if (memcpy_s(pst_efuse_info, sizeof(efuse_info_stru),
                 firmware_mem->pucDataBuf, sizeof(efuse_info_stru)) != EOK) {
        PS_PRINT_ERR("memcpy_s fail!\n");
        return -OAL_EFAIL;
    }

    PS_PRINT_INFO("chip_id[0x%x]\nchip_ver[0x%x]\n", pst_efuse_info->chip_id, pst_efuse_info->chip_ver);
    PS_PRINT_INFO("soft_ver[0x%x]\nhost_ver[0x%x]\n", pst_efuse_info->soft_ver, pst_efuse_info->host_ver);
    PS_PRINT_INFO("mac_h[0x%x]\nmac_m[0x%x]\n", pst_efuse_info->mac_h, pst_efuse_info->mac_m);
    PS_PRINT_INFO("mac_l[0x%x]\n", pst_efuse_info->mac_l);
    return OAL_SUCC;
}

oal_int32 firmware_read_efuse_info(oal_void)
{
    oal_int32       l_ret;
    oal_uint32      ul_size = DEVICE_EFUSE_LENGTH;
    oal_ulong       ul_mac_addr = DEVICE_EFUSE_ADDR;
    FIRMWARE_MEM    *firmware_mem = NULL;
    efuse_info_stru *pst_efuse_info = NULL;

    pst_efuse_info = get_efuse_info_handler();
    if (pst_efuse_info == OAL_PTR_NULL) {
        PS_PRINT_ERR("pst_efuse_info is NULL!\n");
        return -OAL_EFAIL;
    }
    memset_s(pst_efuse_info, sizeof(efuse_info_stru), 0, sizeof(efuse_info_stru));

    firmware_mem = firmware_mem_request();
    if (firmware_mem == NULL) {
        PS_PRINT_ERR("firmware_mem_request fail\n");
        return -OAL_EFAIL;
    }

    l_ret = firmware_read_efuse_handle(firmware_mem, ul_mac_addr, ul_size, pst_efuse_info);
    if (l_ret != OAL_SUCC) {
        PS_PRINT_ERR("firmware_read_efuse_handle err !\n");
        goto failed;
    }

    return OAL_SUCC;

failed:
    return -OAL_EFAIL;
}


oal_int32 plat_firmware_init(void)
{
    oal_int32  l_ret;
    oal_uint32   ul_soft_ver;

    ul_soft_ver = get_device_soft_version();
    if (ul_soft_ver >= SOFT_VER_BUTT) {
        PS_PRINT_ERR("device soft version is invalid!\n");
        return -OAL_EFAIL;
    }

    /* 解析cfg文件 */
    l_ret = firmware_get_cfg(g_auc_cfg_path[ul_soft_ver], WIFI_CFG);
    if (l_ret < 0) {
        PS_PRINT_ERR("get cfg file [%s] fail\n", g_auc_cfg_path[ul_soft_ver]);
        goto cfg_file_init_fail;
    }

    return OAL_SUCC;
cfg_file_init_fail:
    plat_firmware_clear();
    return -OAL_EFAIL;
}

EXPORT_SYMBOL(plat_firmware_init);


oal_int32 plat_firmware_clear(void)
{
    oal_int32 i;

    for (i = 0; i < CFG_FILE_TOTAL; i++) {
        g_st_cfg_info.al_count[i] = 0;
        if (g_st_cfg_info.apst_cmd[i] != NULL) {
            oal_free(g_st_cfg_info.apst_cmd[i]);
            g_st_cfg_info.apst_cmd[i] = NULL;
        }
    }
    return OAL_SUCC;
}

EXPORT_SYMBOL(plat_firmware_clear);
EXPORT_SYMBOL(get_device_soft_version);

/*
 * 功能描述     : 31k Device 内存检测配置文件解析
 * 修改历史     : 新增
 * 1.日    期    : 2020年2月25日
 *   修改内容   : 新增
 */
OAL_STATIC int32_t device_mem_check_cfg_parse(void)
{
    int32_t  ret;
    /* 解析cfg文件 */
    ret = firmware_get_cfg((uint8_t *)DEVICE_MEM_CHECK_CFG_PATH, RAM_REG_TEST_CFG);
    if (ret < 0) {
        PS_PRINT_ERR("get cfg file [%s] fail\n", DEVICE_MEM_CHECK_CFG_PATH);
        return OAL_EFAIL;
    }

    return OAL_SUCC;
}

OAL_STATIC int32_t firmware_get_efuse_value_by_sdio(FIRMWARE_MEM *firmware_mem, uint32_t addr, uint32_t size)
{
    int32_t ret;

    if (firmware_mem == NULL) {
        PS_PRINT_ERR("firmware_mem_request fail\n");
        return -OAL_EFAIL;
    }

    if (size > firmware_mem->ulDataBufLen) {
        PS_PRINT_ERR("device request length is too long !\n");
        return -OAL_EFAIL;
    }

    if (snprintf_s((int8_t *)firmware_mem->puc_send_cmd_buff, CMD_BUFF_LEN, CMD_BUFF_LEN - 1,
                   "%s%c0x%lx%c%d%c",
                   RMEM_CMD_KEYWORD, COMPART_KEYWORD, addr, COMPART_KEYWORD, size, COMPART_KEYWORD) < EOK) {
        PS_PRINT_ERR("firmware_read_efuse_info::snprintf_s failed !\n");
        return -OAL_EFAIL;
    }

    PS_PRINT_DBG("request cmd:[%s]\n", firmware_mem->puc_send_cmd_buff);
    ret = firmware_send_msg(firmware_mem->puc_send_cmd_buff, OAL_STRLEN((int8_t *)firmware_mem->puc_send_cmd_buff));
    if (ret < 0) {
        PS_PRINT_ERR("request cmd send fail![%d]\n", ret);
        return -OAL_EFAIL;
    }

    ret = firmware_read_msg(firmware_mem->pucDataBuf, size);
    if (ret < 0) {
        PS_PRINT_ERR("request cmd read fail![%d]\n", ret);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}


/*
 * 功能描述     : 31k Device 内存检测结果打印
 * 修改历史     : 新增
 * 1.日    期    : 2020年2月25日
 *   修改内容   : 新增
 */
OAL_STATIC int32_t device_mem_check_result_print(void)
{
    oal_file_stru               *fp = NULL;
    int32_t                     ret_len;
    uint32_t                    i;
    uint32_t                    result_len;
    struct mem_check_error_info_st    result[DEVICE_MEM_CHECK_RESULT_LEN / sizeof(struct mem_check_error_info_st)];

    fp = oal_file_open(DEVICE_MEM_CHECK_RESULT_PATH, (OAL_O_RDONLY), 0);
    if (fp == NULL) {
        PS_PRINT_ERR("open file %s fail, fp=%p\n", DEVICE_MEM_CHECK_RESULT_PATH, fp);
        return -OAL_EFAIL;
    }

    memset_s(result, DEVICE_MEM_CHECK_RESULT_LEN, 0, DEVICE_MEM_CHECK_RESULT_LEN);
    ret_len = oal_file_read(fp, (oal_int8 *)result, DEVICE_MEM_CHECK_RESULT_LEN);
    oal_file_close(fp);
    fp = NULL;

    if (ret_len != DEVICE_MEM_CHECK_RESULT_LEN) {
        PS_PRINT_ERR("read file %s fail, ret_len=%d\n", DEVICE_MEM_CHECK_RESULT_PATH, ret_len);
        return -OAL_EFAIL;
    }
    result_len = DEVICE_MEM_CHECK_RESULT_LEN / sizeof(struct mem_check_error_info_st);
    for (i = 0; i < result_len; i++) {
        PS_PRINT_ERR("resverd: 0x%08x, err_addr: 0x%08x, err_data: 0x%08x, exp_data: 0x%08x\n",
                     result[i].resverd, result[i].err_addr, result[i].err_data, result[i].exp_data);
    }

    return OAL_SUCC;
}

/*
 * 功能描述     : 31k Device 内存检测结果, 错误地址信息等
 * 修改历史     : 新增
 * 1.日    期    : 2020年2月25日
 *   修改内容   : 新增
 */
OAL_STATIC int32_t device_mem_check_result(void)
{
    int32_t         ret;
    FIRMWARE_MEM    *firmware_mem = NULL;

    firmware_mem = firmware_mem_request();
    if (firmware_mem == NULL) {
        PS_PRINT_ERR("firmware_mem_request fail\n");
        return -OAL_EFAIL;
    }

    // 获取
    ret = read_mem((uint8_t *)RMEM_CMD_KEYWORD, (uint8_t *)DEVICE_MEM_CHECK_RESULT_CFG, firmware_mem);
    if (ret < 0) {
        PS_PRINT_ERR("read_mem fail, ret:%d\n", ret);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}

/*
 * 功能描述     : 31k Device 内存检测结果标示 成功0 失败1
 * 修改历史     : 新增
 * 1.日    期    : 2020年2月25日
 *   修改内容   : 新增
 */
OAL_STATIC int32_t device_mem_check_result_flag(void)
{
    int32_t         ret;
    uint32_t        addr;
    uint32_t        size;
    uint32_t        res_flag;
    FIRMWARE_MEM    *firmware_mem = NULL;

    firmware_mem = firmware_mem_request();
    if (firmware_mem == NULL) {
        PS_PRINT_ERR("firmware_mem_request fail\n");
        return -OAL_EFAIL;
    }

    addr = DEVICE_MEM_CHECK_FLAG_ADDR;
    size = DEVICE_MEM_CHECK_FLAG_LEN;

    if (size > firmware_mem->ulDataBufLen) {
        PS_PRINT_ERR("device request length is too long !\n");
        return -OAL_EFAIL;
    }

    // 获取
    ret = firmware_get_efuse_value_by_sdio(firmware_mem, addr, size);
    if (ret != OAL_SUCC) {
        PS_PRINT_ERR("firmware_get_psram_type_by_sdio fail!\n");
        return -OAL_EFAIL;
    }
    res_flag = *(uint32_t *)firmware_mem->pucDataBuf;
    device_mem_check_set_result(res_flag);
    if (res_flag != DEVICE_MEM_CHECK_RESULT_SUCC) {
        device_mem_check_result();
        device_mem_check_result_print();
    }

    return OAL_SUCC;
}

int32_t device_mem_check(void)
{
    int32_t         ret;
    uint64_t        total_time;
    oal_time_t_stru start_time;
    oal_time_t_stru end_time;
    oal_time_t_stru trans_time;

    struct pm_drv_data *pm_data = pm_get_drvdata();
    if (pm_data == NULL) {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -OAL_EFAIL;
    }
    // 配置文件解析
    ret = device_mem_check_cfg_parse();
    if (ret != OAL_SUCC) {
        PS_PRINT_ERR("device cfg parse failed.\n");
        return -OAL_EFAIL;
    }

    start_time = oal_ktime_get();
    PS_PRINT_INFO("device ram reg test!\n");
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    ret = firmware_download_function(RAM_REG_TEST_CFG);
    if (ret == SUCCESS) {
        PS_PRINT_INFO("device ram reg test success!\n");
    } else {
        PS_PRINT_INFO("device ram reg test failed!\n");
    }

    ret = device_mem_check_result_flag();
    if (ret != OAL_SUCC) {
        PS_PRINT_ERR("device_mem_check_result_flag failed.\n");
    }

#endif
    oal_disable_channel_state(pm_data->pst_wlan_pm_info->pst_channel, OAL_SDIO_ALL);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    board_power_off();
#endif
    end_time    = oal_ktime_get();
    trans_time  = oal_ktime_sub(end_time, start_time);
    total_time  = (unsigned long long)oal_ktime_to_us(trans_time);
    PS_PRINT_WARNING("device mem reg test time [%llu]us\n", total_time);

    return OAL_SUCC;
}

void device_mem_check_set_result(uint32_t data)
{
    g_mem_check_result = data;
}

uint32_t device_mem_check_get_result(void)
{
    return g_mem_check_result;
}

EXPORT_SYMBOL(device_mem_check);
EXPORT_SYMBOL(device_mem_check_get_result);

// 获取psram类型接口
uint8_t *get_die_id_info_handler(uint32_t *len)
{
    *len = DEVICE_EFUSE_DIEID_LENGTH;
    return g_efuse_die_id;
}

EXPORT_SYMBOL(get_die_id_info_handler);

int32_t firmware_read_efuse_die_id(oal_void)
{
    int32_t      ret;
    uint32_t     size;
    uint32_t     addr;
    FIRMWARE_MEM *firmware_mem = NULL;
    uint32_t     efuse_die_id[DEVICE_EFUSE_DIEID_NUMS] = {0};
    uint32_t     i;
    uint32_t     cnt = 0;

    firmware_mem = firmware_mem_request();
    if (firmware_mem == NULL) {
        PS_PRINT_ERR("firmware_mem_request fail\n");
        return -OAL_EFAIL;
    }

    addr = DEVICE_EFUSE_DIEID_ADDR;
    size = DEVICE_EFUSE_DIEID_LENGTH;
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    size = HISDIO_ALIGN_4_OR_BLK(size);
#endif

    ret = firmware_get_efuse_value_by_sdio(firmware_mem, addr, size);
    if (ret != OAL_SUCC) {
        PS_PRINT_ERR("firmware_get_psram_type_by_sdio fail!\n");
        return -OAL_EFAIL;
    }
    if (memcpy_s(efuse_die_id, sizeof(uint32_t) * DEVICE_EFUSE_DIEID_NUMS,
                 firmware_mem->pucDataBuf, DEVICE_EFUSE_DIEID_LENGTH) != EOK) {
        PS_PRINT_ERR("memcpy_s fail!\n");
        return -OAL_EFAIL;
    }
    for (i = 0; i < DEVICE_EFUSE_DIEID_NUMS; i++) {
        g_efuse_die_id[cnt++] = efuse_die_id[i] & 0xFF;
        g_efuse_die_id[cnt++] = (efuse_die_id[i] & 0xFF00) >> 8;
    }

    return OAL_SUCC;
}

