
#include "oam_main.h"
#include "oam_misc.h"
#include "oam_wdk.h"
#include "plat_type.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAM_MISC_C

oal_uint16  g_us_print2sdt_ctl = 1;        // 1为oam格式日志上报sdt，0为不上报
oal_uint32  g_ul_file_count = 0;            // liteos dump 的文件大小，byte为单位
oal_uint32  g_ul_cnt_statistic = 0;         // 统计已经通过sdt dump到liteos的字节数
oal_uint8   g_uc_uart_ctrl = SDT_DUMP_CLOSE; // liteos dump 文件功能关闭
FILE*       g_pst_fopen_fd = NULL;          // liteos dump 文件全局fd


/* 记录处理的OAM日志数，判断日志是否丢失时用 */
oal_uint32    g_uloam_test_cnt = 0;
/* 记录处理的oam_task_thread 处理的cycle数 */
oal_uint32    g_uloam_test_step = 0;

/* 事件控制结构体 */
EVENT_CB_S oam_event;

/* 轮替全局变量 */
OAM_LOG_ROTATE g_st_global ;

oal_spin_lock_stru g_uart_print_spinlock;

oam_rx_sdt_cmd_func_hook_stru g_oam_uart_rx_sdt_cmd_func_hook;

oal_kthread_stru        *gst_oam_thread;
OAL_STATIC oal_int32 g_l_oam_bind_cpu = -1;

#define OAM_THREAD_POLICY             OAL_SCHED_FIFO
#define OAM_THREAD_PRIORITY       11
#define OAM_THREAD_STACKSIZE  0x1000

oal_void sdt_pkt_head_tail_add(oal_netbuf_stru  *pst_netbuf, oam_data_type_enum_uint8  en_type,
    oam_primid_type_enum_uint8 en_prim_id);
oal_netbuf_stru* sdt_drv_netbuf_delist(oal_void);
void oam_dev_host_uart_print(oal_uint8* pucdata, oal_uint16 us_len);

int rename_old_file(OAM_LOG_ROTATE *pst_log)
{
    int l_curr_num;
    int l_loop;
    char *ac_old_name = NULL;
    char *ac_new_name = NULL;
    int l_ret = EOK;
    char *pc_directory = OAL_PTR_NULL;
    char *pc_prefix = OAL_PTR_NULL;

    ac_old_name = malloc(LOG_OAM_PATH_LEN + LOG_NAME_LEN + 1);
    if (ac_old_name == NULL) {
        return -OAL_EFAIL;
    }
    memset_s(ac_old_name, (LOG_OAM_PATH_LEN + LOG_NAME_LEN + 1), 0, (LOG_OAM_PATH_LEN + LOG_NAME_LEN + 1));
    ac_new_name = malloc(LOG_OAM_PATH_LEN + LOG_NAME_LEN + 1);
    if (ac_new_name == NULL) {
        free(ac_old_name);
        return -OAL_EFAIL;
    }
    memset_s(ac_new_name, (LOG_OAM_PATH_LEN + LOG_NAME_LEN + 1), 0, (LOG_OAM_PATH_LEN + LOG_NAME_LEN + 1));
    if (pst_log == NULL) {
        free(ac_old_name);
        free(ac_new_name);
        return -OAL_EFAIL;
    }
    pc_directory = pst_log->st_path;
    pc_prefix = pst_log->ac_prefix;
    l_curr_num = pst_log->l_curr_num;
    if (l_curr_num >= pst_log->st_num) {
        l_curr_num = pst_log->st_num - 1;
    }
    pst_log->l_curr_num = l_curr_num;

    for (l_loop = l_curr_num; l_loop >= 1; l_loop--) {
        l_ret += snprintf_s(ac_new_name, LOG_OAM_PATH_LEN + LOG_NAME_LEN, LOG_OAM_PATH_LEN + LOG_NAME_LEN - 1,
                            "%s%s%d", pc_directory, pc_prefix, l_loop + 1);
        l_ret += snprintf_s(ac_old_name, LOG_OAM_PATH_LEN + LOG_NAME_LEN, LOG_OAM_PATH_LEN + LOG_NAME_LEN - 1,
                            "%s%s%d", pc_directory, pc_prefix, l_loop);
        if (rename(ac_old_name, ac_new_name) < 0) {
            dprintf("rename ret < 0");
        }
    }
    free(ac_old_name);
    free(ac_new_name);
    ac_old_name = NULL;
    ac_new_name = NULL;
    if (l_ret < EOK) {
        return OAL_FAIL;
    }
    return OAL_SUCC;
}

int check_file_size(OAM_LOG_ROTATE *pst_log)
{
    FILE* fd = NULL;
    long  size;

    if (pst_log == NULL) {
        return -OAL_EFAIL;
    }

    fd = pst_log->pst_fd;

    if (fseek(fd, 0, SEEK_END) != OAL_SUCC) {
        dprintf("fseek failed! FILE: %s,  LINE: %d\n", __FILE__, __LINE__);
        return -OAL_EFAIL;
    }

    size = ftell(fd);
    if (pst_log->st_size > size) {
        return OAL_SUCC;
    }
    fclose(fd);

    rename_old_file(pst_log);
    pst_log->pst_fd = fopen(pst_log->ac_file, "a"); // 每次存入文件都是从wifi_log_1存入的
    if (pst_log->pst_fd == NULL) {
        dprintf("check_file_size fopen failed\n");
        return -OAL_EFAIL;
    }
    pst_log->l_curr_num++;
    return OAL_SUCC;
}

int oam_get_log_file(OAM_LOG_ROTATE *pst_log)
{
    long  l_size;
    int   l_cnt = 0;
    oal_int8  ac_tmp[LOG_OAM_PATH_LEN + 1] = {0};

    pst_log->pst_fd = fopen(pst_log->ac_file, "a");
    while (pst_log->pst_fd == NULL) {
        if (l_cnt > 1) {
            dprintf("oam_get_log_file fopen file failed\n");
            return -OAL_EFAIL;
        }

        pst_log->pst_fd = fopen(pst_log->ac_file, "a");
        l_cnt++;
    }

    if (fseek(pst_log->pst_fd, 0, SEEK_END) != OAL_SUCC) {
        dprintf("fseek failed! FILE: %s,  LINE: %d\n", __FILE__, __LINE__);
        return -OAL_EFAIL;
    }

    l_size = ftell(pst_log->pst_fd);
    if (pst_log->st_size < l_size) {
        if (check_file_size(pst_log) != OAL_SUCC) {
            dprintf("check_file_size failed \n");
            return -OAL_EFAIL;
        }
    }
    return OAL_SUCC;
}

void oam_pkt_time_head(oal_uint16 _us_len, FILE* _pst_fd)
{
    oam_sys_time_union  oam_time_d;
    oam_write2flash_head_stru oam_wr2fs_head_type;
    struct  timeval  tv;
    struct  tm       *ptr;
    time_t          timer;
    int  l_size;
    oal_uint16 count;

    /* 调用获得系统时间的函数,获得时间 */
    timer = time(NULL);
    ptr = localtime(&timer);
    if (ptr == NULL) {
        dprintf("localtime get failed!\n");
        return;
    }
    oam_wr2fs_head_type.puc_buf[0] = 0xaa;
    l_size = (TIME_PKT_HEAD_LEN + _us_len);
    oam_wr2fs_head_type.puc_buf[2] = ((unsigned char)(l_size & 0xff));
    oam_wr2fs_head_type.puc_buf[3] = ((unsigned char)((l_size & 0xff00)>>8));

    if (ptr != NULL) {
        oam_time_d.tm_time.tm_year = ptr->tm_year + 1900;
        oam_time_d.tm_time.tm_mon  = ptr->tm_mon + 1;
        oam_time_d.tm_time.tm_day  = ptr->tm_mday;
        oam_time_d.tm_time.tm_hour = ptr->tm_hour;
        oam_time_d.tm_time.tm_min  = ptr->tm_min;
        oam_time_d.tm_time.tm_sec  = ptr->tm_sec;
        // oam_time_d.tm_time.tm_msec = tv.tv_usec/1000; 没有必要精确到微秒
        oam_time_d.tm_time.tm_msec = 0;
        if (memcpy_s(&(oam_wr2fs_head_type.puc_buf[4]), sizeof(oam_wr2fs_head_type.puc_buf) -  4,
                     oam_time_d.time_data, TIME_TOTAL_LEN) != EOK) {
            dprintf("memcpy_s failed! FILE: %s,  LINE: %d\n", __FILE__, __LINE__);
        }
    }

    count = fwrite(oam_wr2fs_head_type.puc_buf, TIME_PKT_HEAD_LEN, 1, _pst_fd);
    if (count != TIME_PKT_HEAD_LEN) {
        dprintf("file write failed! FILE: %s,  LINE: %d\n", __FILE__, __LINE__);
    }

    return;
}


void oam_log_roll_log(oal_uint8* pucdata, oal_uint16 len)
{
    FILE* fd = NULL;
    oal_int32 ret;
    oal_uint16 count;
    ret = oam_get_log_file(&g_st_global);
    if (ret != OAL_SUCC) {
        dprintf("oam_get_log_file failed \n");
        return;
    }
    fd = g_st_global.pst_fd;
    if (fd != NULL) {
        oam_pkt_time_head(len, fd);
        count = fwrite(pucdata, len, 1, fd);
        if (count != len) {
            dprintf("file write failed!  FILE: %s,  LINE: %d\n", __FILE__, __LINE__);
        }
        fclose(fd);
    }
}

OAL_STATIC oal_int32 oam_task_thread(oal_void* ul_bind_cpu)
{
    oal_int32       ret;
    oal_netbuf_stru  *pst_netbuf = OAL_PTR_NULL;
    oal_int32 len;
    oal_uint8 *buf = NULL;
    oal_allow_signal(SIGTERM);
#ifdef  _PRE_FRW_EVENT_PROCESS_TRACE_DEBUG
    frw_event_last_pc_trace(__FUNCTION__, __LINE__, (oal_uint32)(oal_ulong)ul_bind_cpu);
#endif
    for (;;) {
        if (oal_kthread_should_stop()) {
            break;
        }

        /* 有写事件操作，pend状态转ready，处理完恢复pend状态，尽量不抢wifi业务线程资源 */
        ret = LOS_EventRead(&oam_event, BIT0, LOS_WAITMODE_OR | LOS_WAITMODE_CLR, LOS_WAIT_FOREVER);
        if (ret == LOS_ERRNO_EVENT_READ_TIMEOUT) {
            dprintf("oam_task_thread:error! EventRead timeout!\n");
        }

        g_uloam_test_step++;
        pst_netbuf = sdt_drv_netbuf_delist();
        while (pst_netbuf != NULL) {
            g_uloam_test_cnt++;
            buf = (oal_uint8*)OAL_NETBUF_DATA(pst_netbuf);
            len = (oal_int32)OAL_NETBUF_LEN(pst_netbuf);
            oam_dev_host_uart_print(buf, len);

            oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
            pst_netbuf = sdt_drv_netbuf_delist();
        }
#ifdef  _PRE_FRW_EVENT_PROCESS_TRACE_DEBUG
        frw_event_last_pc_trace(__FUNCTION__, __LINE__, (oal_uint32)(oal_ulong)ul_bind_cpu);
#endif
    }
    return OAL_SUCC;
}

oal_uint32  oam_task_init(oal_void)
{
    oal_kthread_param_stru st_thread_param = {0};

    st_thread_param.l_cpuid      = NOT_BIND_CPU;
    st_thread_param.l_policy     = OAM_THREAD_POLICY;
    st_thread_param.l_prio       = OAM_THREAD_PRIORITY;
    st_thread_param.ul_stacksize = OAM_THREAD_STACKSIZE;

    LOS_EventInit(&oam_event);

    oal_spin_lock_init(&g_uart_print_spinlock);

    gst_oam_thread = oal_kthread_create("hisi_oam_task", oam_task_thread, (void *)g_l_oam_bind_cpu, &st_thread_param);
    if (gst_oam_thread == NULL) {
        OAL_IO_PRINT("[OAM][ERROR]failed to create oamthread\n");
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}

int oam_init_rotate_log(OAM_LOG_ROTATE *pst_log, int pst_size, int pst_num, char *pc_dir_path, char *pc_prefix)
{
    size_t len;

    if (pst_log == NULL || pst_size == 0 || pst_num == 0
        || pc_dir_path == NULL || pc_prefix == NULL) {
        return -OAL_EFAIL;
    }
    pst_log->st_size = pst_size;
    pst_log->st_size *= LOG_OAM_ROTATE_BYTE; // 以byte为单位
    pst_log->st_num = pst_num;

    len = strlen(pc_dir_path);
    if (strncpy_s(pst_log->st_path, sizeof(pst_log->st_path), pc_dir_path, len) != EOK) {
        dprintf("strncpy_s failed FILE: %s,  LINE: %d\n", __FILE__, __LINE__);
    }
    len = 0;

    len = strlen(pc_prefix);
    if (strncpy_s(pst_log->ac_prefix, sizeof(pst_log->ac_prefix), pc_prefix, len) != EOK) {
        dprintf("strncpy_s failed FILE: %s,  LINE: %d\n", __FILE__, __LINE__);
    }

    /* 每次初始化都是从轮替日志编号1开始，跟02 oam_hisi app保持一致 */
    pst_log->l_curr_num = 1;
    if (snprintf_s(pst_log->ac_file, LOG_OAM_PATH_LEN + LOG_NAME_LEN,
                   LOG_OAM_PATH_LEN + LOG_NAME_LEN - 1, "%s%s%d",
                   pst_log->st_path, pst_log->ac_prefix, 1) < EOK) {
        dprintf("oam_init_rotate_log::snprintf_s failed !");
        return OAL_FAIL;
    }

    return OAL_SUCC;
}

oal_int32 write_rx_cmd2file(char* ch_cmd, unsigned int count)
{
    FILE* fd = NULL;
    int write_errno;
    unsigned int ret;
    g_ul_cnt_statistic++;

    ret = fwrite(ch_cmd, count, 1, g_pst_fopen_fd);
    if (ret != count) {
        dprintf("file write failed FILE: %s,  LINE: %d\n", __FILE__, __LINE__);
    }

    if (g_ul_file_count <= g_ul_cnt_statistic) {
        g_uc_uart_ctrl = SDT_DUMP_CLOSE;
        g_ul_file_count = 0;
        g_ul_cnt_statistic = 0;
        fclose(g_pst_fopen_fd);
        g_pst_fopen_fd = NULL;
    }

    return OAL_SUCC;
}

void oam_uart_rx_sdt_cmd_init(void)
{
    g_oam_uart_rx_sdt_cmd_func_hook.uart_rx_sdt_cmd_dispose = write_rx_cmd2file;
}

char uart_putc(char c);
void oam_dev_host_uart_print(oal_uint8* pucdata, oal_uint16 us_len)
{
    oal_int32 ul_cnt;

    oal_spin_lock(g_uart_print_spinlock);

    if (g_us_print2sdt_ctl == 0) {
        oal_spin_unlock(&g_uart_print_spinlock);
        return ;
    }

    for (ul_cnt = 0; ul_cnt < us_len; ul_cnt++) {
        uart_putc(*((oal_uint8 *)pucdata++));
    }

    oal_spin_unlock(&g_uart_print_spinlock);
}

/* 从pc dump RW.bin文件到1131C liteos的jffs2文件系统中，liteos收到
pc 下发的dump命令，回复收到命令ACK，ACK是个空的sdt格式包 */
oal_uint32 oam_dump_file_ack()
{
    oal_netbuf_stru  *pst_netbuf;
    oal_uint16 ul_cnt = 0;
    oal_uint8* pucdata = NULL;
    pst_netbuf = oam_alloc_data2sdt(0);
    if (pst_netbuf == NULL) {
        return OAL_FAIL;
    }
    sdt_pkt_head_tail_add(pst_netbuf, OAM_DATA_TYPE_ACK, OAM_PRIMID_TYPE_OUTPUT_CONTENT);
    /* 回复ACK 为了时效性，不加入sdt链表处理，直接串口上报 */
    pucdata = (oal_uint8*)OAL_NETBUF_DATA(pst_netbuf);
    oal_spin_lock(&g_uart_print_spinlock);
    for (; ul_cnt < OAL_NETBUF_LEN(pst_netbuf); ul_cnt++) {
        uart_putc(*((oal_uint8 *)pucdata++));
    }
    oal_spin_unlock(&g_uart_print_spinlock);
    oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);

    return OAL_SUCC;
}

oal_void oam_file_print(oal_int8* pc_file_name)
{
    oal_file_stru    *fp;
    oal_int32 l_file_size;
    oal_int32 l_rest_count;
    oal_int32 l_idx;
    oal_uint8 *pc_buf = NULL;
    oal_int32 l_read_num;
    oal_uint16 us_data_len = FILE_PRINT_DATA_LEN - FILE_FLAG_NUM * OAL_SIZEOF(oal_uint8);
    om_print_file_stru* pst_file_msg = NULL;
    oal_uint16 ul_cnt;
    oal_uint8* pucdata = NULL;
    oal_ulong ul_irq_save;

    fp = oal_file_open(pc_file_name, (OAL_O_RDONLY), 0);
    if (fp == NULL) {
        return;
    }

    pc_buf = (oal_uint8*)oal_memalloc(us_data_len);
    if (pc_buf == NULL) {
        oal_file_close(fp);
        return;
    }

    pst_file_msg = (om_print_file_stru*)oal_memalloc(OAL_SIZEOF(om_print_file_stru));
    if (pst_file_msg == NULL) {
        free(pc_buf);
        oal_file_close(fp);
        return;
    }

    l_file_size = oal_file_lseek(fp, 0, OAL_SEEK_END);
    oal_file_lseek(fp, 0, OAL_SEEK_SET);

    if (l_file_size <= 0) {
        free(pst_file_msg);
        free(pc_buf);
        oal_file_close(fp);
        return;
    }

    l_rest_count = l_file_size;
    pst_file_msg->st_file_header.ucFrameStart = OM_FRAME_DELIMITER;
    pst_file_msg->st_file_header.ucFuncType = OAM_DATA_TYPE_FILE;

    for (l_idx = 0; l_idx <= l_file_size / us_data_len; l_idx++) {
        l_read_num = oal_file_read(fp, pc_buf, us_data_len);
        if (l_read_num <= 0) {
            break;
        }
        l_rest_count -= l_read_num;
        if (l_rest_count != 0) {
            pst_file_msg->pucdata[0] = 0; /* 0: 文件未读完 */
        } else {
            pst_file_msg->pucdata[0] = 1; /* 1: 文件读取完毕 */
        }

        if (memcpy_s(pst_file_msg->pucdata + 1, FILE_PRINT_DATA_LEN - 1, pc_buf, l_read_num) != EOK) {
            dprintf("oam_file_print memcpy_s failed FILE: %s,  LINE: %d\n", __FILE__, __LINE__);
            free(pst_file_msg);
            free(pc_buf);
            oal_file_close(fp);
            return ;
        }
        pst_file_msg->st_file_header.usFrameLen = OM_FILE_HEADER_LEN + l_read_num
                                                  + FILE_FLAG_NUM * OAL_SIZEOF(oal_uint8);
        ((oal_uint8*)pst_file_msg)[pst_file_msg->st_file_header.usFrameLen - 1] = OM_FRAME_DELIMITER;
        pucdata = (oal_uint8*)pst_file_msg;
        oal_spin_lock_irq_save(&g_uart_print_spinlock, &ul_irq_save);
        for (ul_cnt = 0; ul_cnt < ((oal_uint16) pst_file_msg->st_file_header.usFrameLen); ul_cnt++) {
            uart_putc(*pucdata++);
        }
        oal_spin_unlock_irq_restore(&g_uart_print_spinlock, &ul_irq_save);
    }

    free(pst_file_msg);
    free(pc_buf);
    oal_file_close(fp);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

