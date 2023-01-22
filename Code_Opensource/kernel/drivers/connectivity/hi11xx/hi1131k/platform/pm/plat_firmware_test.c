
/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "los_event.h"
#include "plat_firmware.h"
#include "los_typedef.h"
#include "plat_debug.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "fcntl.h"
#include "los_exc.h"
#include "securec.h"

int32_t check_version(void);
int32_t send_msg(uint8_t *data, uint32_t len);
int32_t read_msg(uint8_t *data, int32_t len, uint32_t ms_timeout);
int32_t msg_send_and_recv_except(uint8_t *data, int32_t len, const uint8_t *expect);
int32_t recv_expect_result(const uint8_t *expect);

#define BUFF_LEN         (1024*100)
#define BUFF_LEN_ALIGN   (BUFF_LEN + 32)

char* download_file_cmd_head = "FILES 1 ";
#if 1
char* download_address = "0x40100000 ";
char* read_memory_address = "0x40100000 ";
#else
char* download_address = "0x000F8000 ";
char* read_memory_address = "0x000F8000 ";
#endif

char* read_memory_cmd_head = "READM ";

char *switch_mem_reg_address = "0x40010100 ";
char *switch_mem_reg_value = "0x2 ";
char *write_reg_cmd_head = "WRITEM 2 ";


uint8_t *read_buff = NULL;
uint8_t *send_buff = NULL;
uint8_t *cmd_buff = NULL;

static int32_t check_version_test(void)
{
    PS_PRINT_DBG(" check version\n");
    return check_version();
}

static int32_t read_memory_test(char *cmd, uint32_t read_mem_len)
{
    int32_t ret;
    int32_t len;
    PS_PRINT_DBG(" read memory\n");
    len = HISI_CHANNEL_ALIGN(OS_STR_LEN(cmd) + 1);
    if (len > CMD_BUFF_LEN) {
        PS_PRINT_ERR("len out of range\r\n");
        return -OAL_EFAIL;
    }
    send_msg(cmd, len);
    ret = read_msg(read_buff, read_mem_len, READ_MEG_TIMEOUT);
    if (ret < 0) {
        PS_PRINT_ERR("read memory fail \r\n");
        return -OAL_EFAIL;
    }
    return OAL_SUCC;
}

static int32_t send_file_test(char *cmd, uint32_t file_len)
{
    int32_t ret;
    int32_t len;
    PS_PRINT_DBG(" write file\n");
    len = HISI_CHANNEL_ALIGN(OS_STR_LEN(cmd) + 1);
    if (len > CMD_BUFF_LEN) {
        PS_PRINT_ERR("len out of range\r\n");
        return -OAL_EFAIL;
    }
    ret = msg_send_and_recv_except(cmd, len, MSG_FROM_DEV_READY_OK);
    if (ret < 0) {
        PS_PRINT_ERR("SEND file cmd error\r\n");
        return -OAL_EFAIL;
    }

    /* Wait at least 1 ms */
    /* 发送文件内容 */
    ret = msg_send_and_recv_except(send_buff, file_len, MSG_FROM_DEV_FILES_OK);
    if (ret < 0) {
        PS_PRINT_ERR(" sdio send data fail\n");
        return -OAL_EFAIL;
    }
    return OAL_SUCC;
}

static int32_t write_reg_test(char *cmd)
{
    int32_t ret;
    int32_t len;
    PS_PRINT_DBG(" write regest\n");
    len = HISI_CHANNEL_ALIGN(OS_STR_LEN(cmd) + 1);
    if (len > CMD_BUFF_LEN) {
        PS_PRINT_ERR("len out of range\r\n");
        return -OAL_EFAIL;
    }
    ret = send_msg(cmd, len);
    if (ret < 0) {
        PS_PRINT_ERR("send write reg cmd fail\r\n");
        return -OAL_EFAIL;
    }

    ret = recv_expect_result(MSG_FROM_DEV_WRITEM_OK);
    if (ret < 0) {
        PS_PRINT_ERR("recv expect result fail!\n");
        return -OAL_EFAIL;
    }
    return OAL_SUCC;
}

static void setbuff(uint8_t *buf, uint32_t buf_len)
{
    uint8_t value = 0;
    for (uint32_t index = 0; index < buf_len; index++) {
        buf[index] = value;
        value++;
    }
}

int32_t loop_file_test(void)
{
    int32_t ret;
    uint32_t len = BUFF_LEN_ALIGN;
    uint32_t align_len;
    uint64_t index = 0;

    setbuff((uint8_t *)send_buff, BUFF_LEN);
    while (len < BUFF_LEN) {
        PS_PRINT_DBG(">>>>>>>>>>>>>>>>>>>>>>index=%llu\r\n", ++index);
        align_len  = HISI_CHANNEL_ALIGN(len);
        PS_PRINT_DBG("--------------------->len=%d, align_len=%d\r\n", len, align_len);
        if (snprintf_s((char *)cmd_buff, CMD_BUFF_LEN, CMD_BUFF_LEN - 1, "%s%s",
                       download_file_cmd_head, download_address) < EOK) {
            PS_PRINT_ERR("loop_file_test::snprintf_s failed !\n");
            return OAL_FAIL;
        }
        ret = send_file_test(cmd_buff, align_len);
        if (ret != OAL_SUCC) {
            return -OAL_EFAIL;
        }
        if (snprintf_s(cmd_buff, CMD_BUFF_LEN, CMD_BUFF_LEN - 1, "%s%s%d%s",
                       read_memory_cmd_head, read_memory_address, align_len, "  ") < EOK) {
            PS_PRINT_ERR("loop_file_test::snprintf_s failed !\n");
            return OAL_FAIL;
        }
        ret = read_memory_test(cmd_buff, align_len);
        if (ret != OAL_SUCC) {
            return -OAL_EFAIL;
        }
        if (!OS_MEM_CMP(send_buff, read_buff, len)) {
            PS_PRINT_DBG(" cmp succ, len=0x%x\n", len);
            continue;
        } else {
            PS_PRINT_ERR(" cmp error, len=0x%x\n", len);
            return -OAL_EFAIL;
        }
    }
    return OAL_SUCC;
}


int32_t firmware_test(void)
{
    int32_t ret;
    read_buff = OS_KMALLOC_GFP(BUFF_LEN);
    send_buff = OS_KMALLOC_GFP(BUFF_LEN);
    cmd_buff  = OS_KMALLOC_GFP(CMD_BUFF_LEN);
    g_aus_recv_cmd_buff = OS_KMALLOC_GFP(CMD_BUFF_LEN);
    g_aus_send_cmd_buff = OS_KMALLOC_GFP(CMD_BUFF_LEN);
    if (read_buff == NULL || send_buff == NULL || cmd_buff == NULL
        || g_aus_recv_cmd_buff == NULL || g_aus_send_cmd_buff == NULL) {
        printf("malloc buff fail\r\n");
        return -1;
    }

    ret = check_version_test();
    if (ret != OAL_SUCC) {
        ret = -OAL_EFAIL;
        goto firmware_test_end;
    }
    setbuff((uint8_t *)send_buff, BUFF_LEN);

    if (snprintf_s((char *)cmd_buff, CMD_BUFF_LEN, CMD_BUFF_LEN - 1, "%s%s%s",
                   write_reg_cmd_head,
                   switch_mem_reg_address,
                   switch_mem_reg_value) < EOK) {
        PS_PRINT_ERR("firmware_test::snprintf_s failed !\n");
        goto firmware_test_end;
    }
    ret = write_reg_test((char *)cmd_buff);
    if (ret != OAL_SUCC) {
        ret = -OAL_EFAIL;
        goto firmware_test_end;
    }

    ret = loop_file_test();
    if (ret != OAL_SUCC) {
        ret = -OAL_EFAIL;
        goto firmware_test_end;
    }
    ret = OAL_SUCC;

firmware_test_end:
    OS_MEM_KFREE(read_buff);
    read_buff = OAL_PTR_NULL;
    OS_MEM_KFREE(send_buff);
    send_buff = OAL_PTR_NULL;
    OS_MEM_KFREE(cmd_buff);
    cmd_buff = OAL_PTR_NULL;
    OS_MEM_KFREE(g_aus_recv_cmd_buff);
    g_aus_recv_cmd_buff = OAL_PTR_NULL;
    OS_MEM_KFREE(g_aus_send_cmd_buff);
    g_aus_send_cmd_buff = OAL_PTR_NULL;
    return ret;
}

