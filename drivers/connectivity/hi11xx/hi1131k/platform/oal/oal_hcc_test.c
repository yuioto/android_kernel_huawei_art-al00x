
#define HISI_LOG_TAG "[HCC_TEST]"
#include "oal_hcc_host_if.h"
#if ((_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION) || defined (_PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT))

#include "securec.h"
#include "oal_kernel_file.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    #include <linux/workqueue.h>
#endif
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    #include <linux/mmc/card.h>
    #include <linux/mmc/sdio_func.h>
    #include <linux/mmc/sdio_ids.h>
    #include <linux/mmc/sdio_func.h>
    #include <linux/mmc/host.h>
#endif

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#define WAIT_FOR_COMPELTION_INTER(PARA) \
    oal_wait_for_completion_interruptible(&(PARA));
#elif (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#define WAIT_FOR_COMPELTION_INTER(PARA) \
    wait_for_completion_interruptible(&(PARA));
#endif
#define HCC_MODE_LEN 128

typedef struct _hcc_test_data_ {
    oal_int32                        mode_idx;
    oal_int32                      pkt_rcvd; /* packet received */
    oal_int32                      pkt_sent; /* packet sent */
    oal_int32                      pkt_gen; /* packet generate */
    oal_int32                      pkt_len; /* packet  length */
    oal_uint64                      total_rcvd_bytes;
    oal_uint64                      total_sent_bytes;
    oal_uint64                      trans_time;
    oal_uint64                      trans_time_us;
    oal_uint64                      throughput;
    hsdio_trans_test_info           trans_info;
} hcc_test_data;

struct  hcc_test_event {
    oal_mutex_stru                mutex_lock; /* sdio test task lock */
    oal_int32                     errorno;
    struct workqueue_struct    *test_workqueue;
    struct work_struct          test_work;
    hcc_test_data     test_data;

    /* hcc perf started,for data transfer */
    oal_int32                      started;

    /* hcc perf statistic */
    ktime_t                     start_time;
    /* The last update time */
    ktime_t                     last_time;

    /* To hcc test sync */
    oal_completion test_done;
    oal_completion test_trans_done;

    hcc_queue_type hcc_test_queue;

    oal_uint8      pad_payload;

    oal_uint8       test_value;
    oal_uint8       verified;

    /* sdio test thread and seam */
};

struct hcc_test_stru {
    const char *mode;
    oal_uint16      start_cmd;
    const char *mode_desc;
    // oal_int32   send_mode;/*1:*/
};

/*
 * 2 Global Variable Definition
 */
OAL_STATIC oal_int32 g_test_force_stop = 0;
struct hcc_test_event  *g_hcc_test_event = NULL;

struct hcc_test_stru g_hcc_test_stru[HCC_TEST_CASE_COUNT] = {
    [HCC_TEST_CASE_TX] = {"tx", HCC_TEST_CMD_START_TX, "HCC_TX_MODE"},
    [HCC_TEST_CASE_RX] = {"rx", HCC_TEST_CMD_START_RX, "HCC_RX_MODE"},
    [HCC_TEST_CASE_LOOP] = {"loop", HCC_TEST_CMD_START_LOOP, "HCC_LOOP_MODE"},
};

#ifdef _PRE_CONFIG_HISI_PANIC_DUMP_SUPPORT
    oal_uint32 wifi_panic_debug = 0;
#ifdef PLATFORM_DEBUG_ENABLE
        module_param(wifi_panic_debug, int, S_IRUGO|S_IWUSR);
#endif
    OAL_STATIC LIST_HEAD(wifi_panic_log_head);
#endif

#ifdef _PRE_CONFIG_HISI_PANIC_DUMP_SUPPORT
    extern oal_int32 hcc_assem_info_print(oal_void* data, char* buf, oal_int32 buf_len);
    extern oal_int32 hcc_flowctrl_info_print(oal_void* data, char* buf, oal_int32 buf_len);
    extern oal_int32 hcc_queues_len_info_print(oal_void* data, char* buf, oal_int32 buf_len);
    extern oal_int32 hcc_queues_pkts_info_print(oal_void* data, char* buf, oal_int32 buf_len);
    extern oal_int32 hsdio_sysfs_info_print(oal_void* data, char* buf, oal_int32 buf_len);
    OAL_STATIC DECLARE_WIFI_PANIC_STRU(hcc_panic_assem_info, hcc_assem_info_print);
    OAL_STATIC DECLARE_WIFI_PANIC_STRU(hcc_panic_flowctrl, hcc_flowctrl_info_print);
    OAL_STATIC DECLARE_WIFI_PANIC_STRU(hcc_panic_queues_len, hcc_queues_len_info_print);
    OAL_STATIC DECLARE_WIFI_PANIC_STRU(hcc_panic_queues_pkts, hcc_queues_pkts_info_print);
    OAL_STATIC DECLARE_WIFI_PANIC_STRU(sdio_panic, hsdio_sysfs_info_print);
#endif

OAL_STATIC oal_kobject* g_conn_syfs_hcc_object = NULL;
OAL_STATIC oal_int32 hcc_test_normal_start(oal_uint16 test_type);
OAL_STATIC oal_int32 hcc_send_test_cmd(const oal_uint8* cmd, oal_int32 hdr_len, oal_int32 cmd_len);
OAL_STATIC oal_int32 hcc_test_start(oal_uint16 start_cmd);
int hcc_test_set_case(hcc_test_data *data);

/*
 * 3 Function Declare
 */
#ifdef _PRE_CONFIG_HISI_PANIC_DUMP_SUPPORT
oal_void hwifi_panic_log_register(hwifi_panic_log* pst_log, oal_void* data)
{
    OAL_BUG_ON(!pst_log);
    pst_log->data = data;
    list_add_tail(&pst_log->list, &wifi_panic_log_head);
    OAL_IO_PRINT("Log module %s added![%pF]\n", pst_log->name, (void*)_RET_IP_);//lint !e571
}

oal_void hwifi_panic_log_unregister(hwifi_panic_log* pst_log)
{
    OAL_BUG_ON(!pst_log);
    list_del(&pst_log->list);
}

oal_module_symbol(hwifi_panic_log_register);
oal_module_symbol(hwifi_panic_log_unregister);

/* should't use dynamic mem when panic */
OAL_STATIC char g_panic_mem[PAGE_SIZE];
void hwifi_panic_log_dump(char* print_level)
{
    oal_uint32 buf_size = PAGE_SIZE;
    char* pst_buf = NULL;
    hwifi_panic_log* pst_log = NULL;
    struct list_head* head = &wifi_panic_log_head;

    print_level = print_level ? : "";

    pst_buf = g_panic_mem;

    printk("%sdump wifi info when panic\n", print_level);
    list_for_each_entry(pst_log, head, list)
    {
        if (pst_log == NULL) {
            printk(KERN_ERR"hwifi_panic_log_dump:pst_log is null\n");
            return;
        }
        pst_buf[0] = '\0';
        printk("%s[%s]:\n", print_level, pst_log->name);
        pst_log->cb(pst_log->data, pst_buf, buf_size);
        printk("%s%s\n", print_level, pst_buf);
        printk("%s\n", print_level);
    }
    printk("%s\n", print_level);
}
oal_module_symbol(hwifi_panic_log_dump);
#endif

OAL_STATIC oal_int32 hcc_flowctrl_info_filling(char *buf, oal_int32 *count, oal_int32 buf_len, struct hcc_handler *hcc)
{
    oal_int32 ret;

    ret = snprintf_s(buf + *count, buf_len - *count,  buf_len - *count - 1, "hcc state:%s[%d]\n",
                     (oal_atomic_read(&hcc->state) == HCC_OFF) ? "off" : "on", oal_atomic_read(&hcc->state));
    if (ret < EOK) {
        return ret;
    }
    *count += ret;

    ret = snprintf_s(buf + *count, buf_len - *count,  buf_len - *count - 1, "flowctrl flag:%s\n",
                     (hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_flag == D2H_MSG_FLOWCTRL_OFF) ? "off" : "on");
    if (ret < EOK) {
        return ret;
    }
    *count += ret;

    ret = snprintf_s(buf + *count, buf_len - *count,  buf_len - *count - 1, "flowctrl on:%d\n",
                     hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_on_count);
    if (ret < EOK) {
        return ret;
    }
    *count += ret;

    ret = snprintf_s(buf + *count, buf_len - *count,  buf_len - *count - 1, "flowctrl off:%d\n",
                     hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_off_count);
    if (ret < EOK) {
        return ret;
    }
    *count += ret;

    ret = snprintf_s(buf + *count, buf_len - *count,  buf_len - *count - 1, "flowctrl reset count:%d\n",
                     hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_reset_count);
    if (ret < EOK) {
        return ret;
    }
    *count += ret;

    ret = snprintf_s(buf + *count, buf_len - *count,  buf_len - *count - 1,
                     "flowctrl hi update count:%d,hi credit value:%u\n",
                     hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_hipri_update_count,
                     hcc->hcc_transer_info.tx_flow_ctrl.uc_hipriority_cnt);
    if (ret < EOK) {
        return ret;
    }
    *count += ret;

    return ret;
}

oal_int32 hcc_flowctrl_info_print(oal_void* data, char* buf, oal_int32 buf_len)
{
    oal_int32 ret;
    oal_int32 count = 0;
    struct hcc_handler* hcc = (struct hcc_handler*)data;

    if (hcc == NULL) {
        return count;
    }

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    if (hcc->hi_channel->func->card->host->claimer) {
        ret = snprintf_s(buf + count, buf_len - count,  buf_len - count - 1, "claim host name:%s\n",
                         hcc->hi_channel->func->card->host->claimer->comm);
        if (ret < EOK) {
            return count;
        }
        count += ret;
    }
#endif

    hcc_flowctrl_info_filling(buf, &count, buf_len, hcc);

    return count;
}

EXPORT_SYMBOL_GPL(hcc_flowctrl_info_print);


oal_int32 hcc_assem_info_print(oal_void* data, char* buf, oal_int32 buf_len)
{
    oal_int32 ret = 0;
    oal_int32 i;
    oal_int32 total = 0;
    struct hcc_handler* hcc = (struct hcc_handler*)data;
    if (hcc == NULL) {
        return ret;
    }
    for (i = 0; i < HCC_TX_ASSEM_INFO_MAX_NUM; i++) {
        if (hcc->hcc_transer_info.tx_assem_info.info[i]) {
            if (hcc->hcc_transer_info.tx_assem_info.info[i])
                ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                                   "[tx][%2d]:%-20u pkts\n", i, hcc->hcc_transer_info.tx_assem_info.info[i]);
            total += (oal_int32)hcc->hcc_transer_info.tx_assem_info.info[i] * (i == 0 ? 1 : i);
        }
    }
    if (total)
        ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "hcc tx total:%d!\n", total);

    total = 0;

    for (i = 0; i < HCC_RX_ASSEM_INFO_MAX_NUM; i++) {
        if (hcc->hcc_transer_info.rx_assem_info.info[i]) {
            total += (oal_int32)hcc->hcc_transer_info.rx_assem_info.info[i] * (i == 0 ? 1 : i);
            if (hcc->hcc_transer_info.rx_assem_info.info[i])
                ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                                   "[rx][%2d]:%-20u pkts\n", i, hcc->hcc_transer_info.rx_assem_info.info[i]);
        }
    }

    if (total)
        ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "hcc rx total:%d!\n", total);
    if (ret < EOK) {
        OAL_IO_PRINT("hcc_assem_info_print::snprintf_s failed!\n");
        return ret;
    }
    return ret;
}

oal_int32 hcc_queues_pkts_info_print(oal_void* data, char* buf, oal_int32 buf_len)
{
    oal_int32 ret = 0;
    int i, j;
    oal_uint64 total;
    hcc_trans_queue *pst_hcc_queue = OAL_PTR_NULL;
    struct hcc_handler* hcc = (struct hcc_handler*)data;
    if (hcc == NULL) {
        return ret;
    }
    ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "queues_pkts_info_show\n");
    for (i = 0; i < HCC_DIR_COUNT; i++) {
        total = 0;
        ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "transfer dir:%s\n", HCC_GET_CHAN_STRING(i));
        for (j = 0; j < HCC_QUEUE_COUNT; j++) {
            pst_hcc_queue = &hcc->hcc_transer_info.hcc_queues[i].queues[j];
            if (pst_hcc_queue->total_pkts || pst_hcc_queue->loss_pkts)
                ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                                   "queue:%4d,pkts num:%10u,loss num:%10u\n", j,
                                   pst_hcc_queue->total_pkts, pst_hcc_queue->loss_pkts);
            total += pst_hcc_queue->total_pkts;
        }
        if (total)
            ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "total:%llu\n", total);
    }

    ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "flow ctrl info show\n");
    for (j = 0; j < HCC_QUEUE_COUNT; j++) {
        pst_hcc_queue = &hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[j];
        ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "tx queue:%4d,%s,low wl:%u, high wl:%u\n",
                           j, (pst_hcc_queue->flow_ctrl.is_stopped == OAL_FALSE) ? "run " : "stop",
                           pst_hcc_queue->flow_ctrl.low_waterline,
                           pst_hcc_queue->flow_ctrl.high_waterline);
    }

#ifdef _PRE_CONFIG_WLAN_THRANS_THREAD_DEBUG
    ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "hcc thread stat info:\n");
    ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "condition true:%llu,false:%llu\n",
                       hcc->hcc_transer_info.thread_stat.wait_event_run_count,
                       hcc->hcc_transer_info.thread_stat.wait_event_block_count);
    ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "thread loop do:%llu,empty:%llu\n",
                       hcc->hcc_transer_info.thread_stat.loop_have_data_count,
                       hcc->hcc_transer_info.thread_stat.loop_no_data_count);
#endif
    if (ret < EOK) {
        OAL_IO_PRINT("hcc_queues_pkts_info_print::snprintf_s failed!\n");
        return ret;
    }
    return ret;
}

oal_int32 hcc_queues_len_info_print(oal_void* data, char* buf, oal_int32 buf_len)
{
    oal_int32 ret = 0;
    oal_int32 i, j;
    struct hcc_handler* hcc = (struct hcc_handler*)data;
    if (hcc == NULL) {
        return ret;
    }
    for (i = 0; i < HCC_DIR_COUNT; i++) {
        ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "dir:%s\n", HCC_GET_CHAN_STRING(i));
        for (j = 0; j < HCC_QUEUE_COUNT; j++) {
            if (oal_netbuf_list_len(&hcc->hcc_transer_info.hcc_queues[i].queues[j].data_queue))
                ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "queue:%d, len:%d\n", j,
                                   oal_netbuf_list_len(&hcc->hcc_transer_info.hcc_queues[i].queues[j].data_queue));
        }
    }
    if (ret < EOK) {
        OAL_IO_PRINT("hcc_queues_len_info_print::snprintf_s failed!\n");
        return ret;
    }
    return ret;
}

OAL_STATIC ssize_t  hcc_get_assem_info(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int ret = 0;
    struct hcc_handler* hcc = OAL_PTR_NULL;
    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    hcc = hcc_get_default_handler();
    if (hcc == NULL) {
        OAL_IO_PRINT("get hcc handler failed!%s\n", __FUNCTION__);
        return ret;
    }

    ret += hcc_assem_info_print(hcc, buf, PAGE_SIZE - ret);

    return ret;
}

OAL_STATIC ssize_t  hcc_set_assem_info(struct kobject *dev, struct kobj_attribute *attr, const char *buf, size_t count)
{
    struct hcc_handler* hcc = OAL_PTR_NULL;

    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);
    hcc = hcc_get_default_handler();
    if (hcc == NULL) {
        OAL_IO_PRINT("get hcc handler failed!%s\n", __FUNCTION__);
        return count;
    }
    hcc_tx_assem_info_reset(hcc);
    hcc_rx_assem_info_reset(hcc);

    OAL_IO_PRINT("reset done!\n");

    return count;
}

OAL_STATIC ssize_t  hcc_get_queues_pkts_info(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int ret = 0;
    struct hcc_handler* hcc = OAL_PTR_NULL;
    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    hcc = hcc_get_default_handler();
    if (hcc == NULL) {
        OAL_IO_PRINT("get hcc handler failed!%s\n", __FUNCTION__);
        return ret;
    }

    ret += hcc_queues_pkts_info_print(hcc, buf, PAGE_SIZE - ret);

    return ret;
}

OAL_STATIC ssize_t  hcc_get_queues_len_info(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int ret = 0;
    struct hcc_handler* hcc = OAL_PTR_NULL;
    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    hcc = hcc_get_default_handler();
    if (hcc == NULL) {
        OAL_IO_PRINT("get hcc handler failed!%s\n", __FUNCTION__);
        return ret;
    }

    ret += hcc_queues_len_info_print(hcc, buf, PAGE_SIZE - ret);

    return ret;
}

OAL_STATIC ssize_t  hcc_get_flowctrl_info(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int ret = 0;
    struct hcc_handler* hcc = OAL_PTR_NULL;
    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    hcc = hcc_get_default_handler();
    if (hcc == NULL) {
        OAL_IO_PRINT("get hcc handler failed!%s\n", __FUNCTION__);
        return ret;
    }

    ret += hcc_flowctrl_info_print(hcc, buf, PAGE_SIZE - ret);

    return ret;
}

oal_int32 hcc_wakelock_info_print(struct hcc_handler* hcc, char* buf, oal_int32 buf_len)
{
    oal_int32 ret;
    oal_int32 count = 0;

    if (hcc == NULL) {
        return count;
    }
#ifdef CONFIG_PRINTK
    if (hcc->tx_wake_lock.locked_addr) {
        ret = snprintf_s(buf + count, buf_len - count, buf_len - count - 1, "wakelocked by:%pf\n",
                         (oal_void *)(uintptr_t)(hcc->tx_wake_lock.locked_addr));
        if (ret < EOK) {
            return count;
        }
        count += ret;
    }
#endif

    ret = snprintf_s(buf + count, buf_len - count, buf_len - count - 1, "hold %lu locks\n",
                     hcc->tx_wake_lock.lock_count);
    if (ret < EOK) {
        OAL_IO_PRINT("hcc_wakelock_info_print::snprintf_s failed !\n");
        return count;
    }
    count += ret;

    return count;
}

OAL_STATIC ssize_t  hcc_get_wakelock_info(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int ret = 0;
    struct hcc_handler* hcc = OAL_PTR_NULL;
    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    hcc = hcc_get_default_handler();
    if (hcc == NULL) {
        OAL_IO_PRINT("get hcc handler failed!%s\n", __FUNCTION__);
        return ret;
    }

    ret += hcc_wakelock_info_print(hcc, buf, PAGE_SIZE - ret);

    return ret;
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    OAL_STATIC struct kobj_attribute dev_attr_flowctrl =
    __ATTR(flowctrl, S_IRUGO, hcc_get_flowctrl_info, NULL);
    OAL_STATIC struct kobj_attribute dev_attr_assem_info =
    __ATTR(assem_info, (S_IRUGO | S_IWUSR), hcc_get_assem_info, hcc_set_assem_info); //lint !e64
    OAL_STATIC struct kobj_attribute dev_attr_queues_pkts =
    __ATTR(queues_pkts, S_IRUGO, hcc_get_queues_pkts_info, NULL);
    OAL_STATIC struct kobj_attribute dev_attr_queues_len =
    __ATTR(queues_len, S_IRUGO, hcc_get_queues_len_info, NULL);
    OAL_STATIC struct kobj_attribute dev_attr_wakelock =
    __ATTR(wakelock, S_IRUGO, hcc_get_wakelock_info, NULL);
#endif


oal_uint64 hcc_test_get_trans_time(ktime_t start_time, ktime_t stop_time)
{
    ktime_t trans_time;
    oal_uint64  trans_us;

    trans_time = oal_ktime_sub(stop_time, start_time);

    trans_us = (oal_uint64)oal_ktime_to_us(trans_time);
    if (trans_us == 0) {
        trans_us = 1;
    }

    return trans_us;
}

oal_void hcc_test_throughput_cac(oal_uint64 trans_bytes, ktime_t start_time, ktime_t stop_time)
{
    oal_uint64 trans_us;
    oal_uint64 temp;
    oal_uint64 us_to_s; // converted  usecond to second 

    trans_us = hcc_test_get_trans_time(start_time, stop_time);
    temp = (trans_bytes);
    temp = temp * 1000u;
    temp = temp * 1000u;
    temp = (temp >> 17);
    temp = div_u64(temp, trans_us);
    us_to_s = trans_us;
    g_hcc_test_event->test_data.trans_time_us = trans_us;
    do_div(us_to_s, 1000000u); //lint !e58 !e64 !e501 !e414
    g_hcc_test_event->test_data.throughput = temp;
    g_hcc_test_event->test_data.trans_time = us_to_s;
}

OAL_STATIC  oal_void hcc_test_throughput_gen(oal_void)
{
    if (g_hcc_test_event->test_data.mode_idx == HCC_TEST_CASE_TX) {
        hcc_test_throughput_cac(g_hcc_test_event->test_data.total_sent_bytes,
                                g_hcc_test_event->start_time,
                                g_hcc_test_event->last_time);
    } else if (g_hcc_test_event->test_data.mode_idx == HCC_TEST_CASE_RX) {
        hcc_test_throughput_cac(g_hcc_test_event->test_data.total_rcvd_bytes,
                                g_hcc_test_event->start_time,
                                g_hcc_test_event->last_time);
    } else if (g_hcc_test_event->test_data.mode_idx == HCC_TEST_CASE_LOOP) {
        hcc_test_throughput_cac(g_hcc_test_event->test_data.total_rcvd_bytes +
                                g_hcc_test_event->test_data.total_sent_bytes,
                                g_hcc_test_event->start_time,
                                g_hcc_test_event->last_time);
    }
}

OAL_STATIC  oal_uint64 hcc_test_utilization_ratio_gen(oal_uint64 payload_size, oal_uint64 transfer_size)
{
    oal_uint64 ret;
    payload_size = payload_size * 1000;
    if (transfer_size)
        ret = div_u64(payload_size, transfer_size);
    else
        ret = 0;
    return ret;
}

/* 统计发送方向的丢包率，接收方向默认不丢包 */
OAL_STATIC  oal_uint32 hcc_test_tx_pkt_loss_gen(oal_uint32 tx_pkts, oal_uint32 actual_tx_pkts)
{
    oal_uint32 ul_loss;
    // g_hcc_test_event->test_data.pkt_sent
    if (tx_pkts == actual_tx_pkts || !tx_pkts)
        return 0;
    if (tx_pkts < actual_tx_pkts)
        return 0;

    ul_loss = tx_pkts - actual_tx_pkts;
    return ul_loss * 1000 / tx_pkts;
}


OAL_STATIC oal_int32 hcc_test_rcvd(oal_uint8 stype, hcc_netbuf_stru* pst_hcc_netbuf, oal_uint8 *pst_context)
{
    oal_netbuf_stru* pst_netbuf = pst_hcc_netbuf->pst_netbuf;
    OAL_REFERENCE(pst_context);

    if (OAL_LIKELY(stype == HCC_TEST_SUBTYPE_DATA)) {
        oal_int32 filter_flag = 0;

        /* 计算总共数据包长度 */
        if (OAL_UNLIKELY(g_hcc_test_event->test_data.pkt_len != OAL_NETBUF_LEN(pst_netbuf))) {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
            if (printk_ratelimit()) {
                OAL_IO_PRINT("[E]recvd netbuf pkt len:%d,but request len:%d\n",
                             OAL_NETBUF_LEN(pst_netbuf),
                             g_hcc_test_event->test_data.pkt_len);
            }
#endif
            filter_flag = 1;
        }

        if (g_hcc_test_event->verified) {
            oal_int32 i;
            oal_int32 flag = 0;
            oal_uint8 *data =  OAL_NETBUF_DATA(pst_netbuf);
            for (i = 0; i < (int32_t)OAL_NETBUF_LEN(pst_netbuf); i++) {
                if (*(data + i) != g_hcc_test_event->test_value) {
                    flag = 1;
                    OAL_IO_PRINT("[E]data wrong, [i:%d] value:%x should be %x\n",
                                 i, *(data + i), g_hcc_test_event->test_value);
                    break;
                }
            }

            if (flag) {
                oal_print_hex_dump(data, OAL_NETBUF_LEN(pst_netbuf), 32, "hcc rx verified ");
                filter_flag = 1;
            }
        }

        if (!filter_flag) {
            /* filter_flag=1 时接收的数据包不符合要求，则过滤掉 */
            g_hcc_test_event->test_data.pkt_rcvd++;
            g_hcc_test_event->test_data.total_rcvd_bytes += OAL_NETBUF_LEN(pst_netbuf);
            g_hcc_test_event->last_time = oal_ktime_get();
        }
    } else if (stype == HCC_TEST_SUBTYPE_CMD) {
        hcc_test_cmd_stru cmd = {0};

        if (memcpy_s(&cmd, OAL_SIZEOF(hcc_test_cmd_stru),
                     OAL_NETBUF_DATA(pst_netbuf), OAL_SIZEOF(hcc_test_cmd_stru)) != EOK) {
            oal_netbuf_free(pst_netbuf);
            OAL_IO_PRINT("memcpy_s failed.\n");
            return OAL_FAIL;
        }

        if (cmd.cmd_type == HCC_TEST_CMD_STOP_TEST) {
            if (memcpy_s(&g_hcc_test_event->test_data.trans_info, OAL_SIZEOF(hsdio_trans_test_info),
                         hcc_get_test_cmd_data(OAL_NETBUF_DATA(pst_netbuf)),
                         OAL_SIZEOF(hsdio_trans_test_info)) != EOK) {
                oal_netbuf_free(pst_netbuf);
                OAL_IO_PRINT("memcpy_s failed.\n");
                return OAL_FAIL;
            }
        }

        g_hcc_test_event->last_time = oal_ktime_get();

        OAL_COMPLETE(&g_hcc_test_event->test_trans_done);
    } else {
        /* unkown subtype */
        OAL_IO_PRINT("receive unkown stype:%d\n", stype);
    }

    oal_netbuf_free(pst_netbuf);

    return OAL_SUCC;
}


OAL_STATIC oal_int32 hcc_test_sent(struct hcc_handler* hcc, struct hcc_transfer_param* param, oal_uint16 start_cmd)
{
    oal_uint8 pad_payload = g_hcc_test_event->pad_payload;
    oal_netbuf_stru*       pst_netbuf;
    /*
    * 1) alloc memory for skb,
    * 2) skb free when send after dequeue from tx queue
    * */
    pst_netbuf  = hcc_netbuf_alloc(g_hcc_test_event->test_data.pkt_len + pad_payload);
    if (pst_netbuf == NULL) {
        OAL_IO_PRINT("hwifi alloc skb fail.\n");
        return -OAL_EFAIL;
    }

    if (pad_payload) {
        oal_netbuf_reserve(pst_netbuf, pad_payload);
    }

    if (memset_s(oal_netbuf_put(pst_netbuf, g_hcc_test_event->test_data.pkt_len),
                 g_hcc_test_event->test_data.pkt_len, g_hcc_test_event->test_value,
                 g_hcc_test_event->test_data.pkt_len) != EOK) {
        OAL_IO_PRINT("hcc_test_sent::memset fail!");
        oal_netbuf_free(pst_netbuf);
        pst_netbuf = NULL;
        return -OAL_EFAIL;
    }

    if (start_cmd == HCC_TEST_SUBTYPE_DATA) {
        g_hcc_test_event->test_data.total_sent_bytes += OAL_NETBUF_LEN(pst_netbuf);
    }

    return hcc_tx(hcc, pst_netbuf, param);
}

OAL_STATIC oal_int32 hcc_send_test_cmd(const oal_uint8* cmd, oal_int32 hdr_len, oal_int32 cmd_len)
{
    oal_netbuf_stru*       pst_netbuf = OAL_PTR_NULL;
    struct hcc_transfer_param st_hcc_transfer_param = {0};
    struct hcc_handler* hcc = hcc_get_default_handler();
    if (hcc == NULL) {
        return -OAL_EFAIL;
    }

    pst_netbuf  = hcc_netbuf_alloc(cmd_len);
    if (pst_netbuf == NULL) {
        OAL_IO_PRINT("hwifi alloc skb fail.\n");
        return -OAL_EFAIL;
    }

    if (hdr_len > cmd_len) {
        hdr_len = cmd_len;
    }

    if (memcpy_s(oal_netbuf_put(pst_netbuf, cmd_len), cmd_len, cmd, hdr_len) != EOK) {
        OAL_IO_PRINT("memcpy_s error, destlen=%d, srclen=%d\n ", cmd_len, hdr_len);
        oal_netbuf_free(pst_netbuf);
        pst_netbuf = NULL;
        return -OAL_EFAIL;
    }

    hcc_hdr_param_init(&st_hcc_transfer_param,
                       HCC_ACTION_TYPE_TEST,
                       HCC_TEST_SUBTYPE_CMD,
                       0,
                       HCC_FC_WAIT,
                       g_hcc_test_event->hcc_test_queue);
    return hcc_tx(hcc, pst_netbuf, &st_hcc_transfer_param);
}

OAL_STATIC oal_int32 hcc_test_rx_start(oal_uint16 start_cmd)
{
    oal_uint32      cmd_len;
    oal_int32       ret;
    hcc_test_cmd_stru  *pst_cmd = NULL;
    hsdio_trans_test_rx_info *pst_rx_info = NULL;

    struct hcc_handler *hcc;
    hcc = hcc_get_default_handler();
    if (hcc == NULL) {
        return -OAL_EFAIL;
    }
    cmd_len = OAL_SIZEOF(hcc_test_cmd_stru) + OAL_SIZEOF(hsdio_trans_test_rx_info);
    pst_cmd = (hcc_test_cmd_stru*)oal_memalloc(cmd_len);
    if (pst_cmd == NULL) {
        return -OAL_EFAIL;
    }

    OAL_INIT_COMPLETION(&g_hcc_test_event->test_trans_done);
    g_hcc_test_event->test_data.pkt_rcvd = 0;
    g_hcc_test_event->test_data.pkt_sent = 0;
    g_hcc_test_event->test_data.total_rcvd_bytes = 0;
    g_hcc_test_event->test_data.total_sent_bytes = 0;
    g_hcc_test_event->test_data.throughput = 0;
    g_hcc_test_event->test_data.trans_time = 0;
    g_hcc_test_event->last_time = g_hcc_test_event->start_time = oal_ktime_get();

    memset_s((oal_void*)pst_cmd, cmd_len, 0, cmd_len);
    pst_cmd->cmd_type = start_cmd;
    pst_cmd->cmd_len  = cmd_len;

    pst_rx_info = (hsdio_trans_test_rx_info*)hcc_get_test_cmd_data(pst_cmd);

    pst_rx_info->total_trans_pkts = g_hcc_test_event->test_data.pkt_gen;
    pst_rx_info->pkt_len = g_hcc_test_event->test_data.pkt_len;
    pst_rx_info->pkt_value = g_hcc_test_event->test_value;

    if (hcc_send_test_cmd((oal_uint8*)pst_cmd, pst_cmd->cmd_len, pst_cmd->cmd_len) != OAL_SUCC) {
        oal_free(pst_cmd);
        return -OAL_EFAIL;
    }

    oal_free(pst_cmd);
    g_hcc_test_event->last_time = oal_ktime_get();

    /* 等待回来的CMD命令 */
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    ret = oal_wait_for_completion_interruptible(&g_hcc_test_event->test_trans_done);
#elif (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    ret = wait_for_completion_interruptible(&g_hcc_test_event->test_trans_done);
#endif
    if (ret < 0) {
        ret = -OAL_EFAIL;
        oal_channel_send_msg(hcc->hi_channel, H2D_MSG_STOP_SDIO_TEST);
    }

    if (g_test_force_stop) {
        oal_channel_send_msg(hcc->hi_channel, H2D_MSG_STOP_SDIO_TEST);
        g_test_force_stop = 0;
        oal_msleep(100);
    }

    OAL_COMPLETE(&g_hcc_test_event->test_done);
    return ret;
}

OAL_STATIC oal_int32 hcc_test_normal_start(oal_uint16 start_cmd)
{
    oal_int32            ret;
    oal_int32            retry_count = 0;
    oal_int32            i;
    hcc_test_cmd_stru  cmd = { 0 };

    struct hcc_transfer_param st_hcc_transfer_param = {0};
    struct hcc_handler* hcc = OAL_PTR_NULL;
    hcc_hdr_param_init(&st_hcc_transfer_param, HCC_ACTION_TYPE_TEST, HCC_TEST_SUBTYPE_DATA, 0,
                       HCC_FC_WAIT, g_hcc_test_event->hcc_test_queue);
    hcc = hcc_get_default_handler();
    if (hcc == NULL) {
        return -OAL_EFAIL;
    }

    OAL_INIT_COMPLETION(&g_hcc_test_event->test_trans_done);
    g_hcc_test_event->test_data.pkt_rcvd = 0;
    g_hcc_test_event->test_data.pkt_sent = 0;
    g_hcc_test_event->test_data.total_rcvd_bytes = 0;
    g_hcc_test_event->test_data.total_sent_bytes = 0;
    g_hcc_test_event->test_data.throughput = 0;
    g_hcc_test_event->test_data.trans_time = 0;

    cmd.cmd_type = start_cmd;
    cmd.cmd_len = OAL_SIZEOF(hcc_test_cmd_stru) + OAL_SIZEOF(hsdio_trans_test_info);
    if (hcc_send_test_cmd((oal_uint8*)&cmd, OAL_SIZEOF(cmd), cmd.cmd_len) != OAL_SUCC) {
        return -OAL_EFAIL;
    }
    /* wait the device recv the cmd,change the test mode! */
    oal_msleep(50);
    g_hcc_test_event->last_time = g_hcc_test_event->start_time = oal_ktime_get();
    for (i = 0; i < g_hcc_test_event->test_data.pkt_gen; i++) {
        ret = hcc_test_sent(hcc, &st_hcc_transfer_param, HCC_TEST_SUBTYPE_DATA);
        if (ret < 0) {
            OAL_IO_PRINT("hcc test gen pkt send fail.\n");
            break;
        }

        g_hcc_test_event->test_data.pkt_sent++ ;
        g_hcc_test_event->last_time = oal_ktime_get();

        if (OAL_UNLIKELY(g_hcc_test_event->started == OAL_FALSE)) {
            ret = -OAL_EFAIL;
            break;
        }
    }

    cmd.cmd_type = HCC_TEST_CMD_STOP_TEST;
    hcc_send_test_cmd((oal_uint8*)&cmd, OAL_SIZEOF(cmd), cmd.cmd_len);

    g_hcc_test_event->last_time = oal_ktime_get();

retry:

    /* 等待回来的CMD命令 */
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    ret = oal_wait_for_completion_interruptible_timeout(&g_hcc_test_event->test_trans_done, OAL_MSECS_TO_JIFFIES(500));
#elif (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    ret = wait_for_completion_interruptible_timeout(&g_hcc_test_event->test_trans_done, OAL_MSECS_TO_JIFFIES(500));
#endif
    if (ret < 0) {
        OAL_IO_PRINT("Test Event  terminated ret=%d\n", ret);
        ret = -OAL_EFAIL;
        hcc_send_test_cmd((oal_uint8*)&cmd, OAL_SIZEOF(cmd), cmd.cmd_len);
    } else if (ret == 0) {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        /* cmd response timeout */
        if (retry_count++ < 1) {
            oal_msleep(100);
            hcc_send_test_cmd((oal_uint8*)&cmd, OAL_SIZEOF(cmd), cmd.cmd_len);
            g_hcc_test_event->last_time = ktime_get();
            OAL_IO_PRINT("resend the stop cmd!retry count:%d\n", retry_count);
            goto retry;
        } else {
            OAL_IO_PRINT("resend the stop cmd timeout!retry count:%d\n", retry_count);
            ret = -OAL_EFAIL;
        }
#endif
    } else {
        if (g_test_force_stop) {
            hcc_send_test_cmd((oal_uint8*)&cmd, OAL_SIZEOF(cmd), cmd.cmd_len);

            g_hcc_test_event->last_time = oal_ktime_get();

            g_test_force_stop = 0;
            OAL_IO_PRINT("normal start force stop\n");
            oal_msleep(100);
        }
        ret = OAL_SUCC;
    }

    OAL_COMPLETE(&g_hcc_test_event->test_done);
    return ret;
}

OAL_STATIC oal_int32 hcc_test_start(oal_uint16 start_cmd)
{
    OAL_IO_PRINT("%s Test start.\n", g_hcc_test_stru[g_hcc_test_event->test_data.mode_idx].mode);
    if (g_hcc_test_event->test_data.mode_idx == HCC_TEST_CASE_RX) {
        return hcc_test_rx_start(start_cmd);
    } else {
        return hcc_test_normal_start(start_cmd);
    }
}


oal_void hcc_test_work(struct work_struct *work)
{
    oal_uint16       start_cmd;
    oal_int32       ret;

    start_cmd = g_hcc_test_stru[g_hcc_test_event->test_data.mode_idx].start_cmd;

    /* hcc test start */
    ret = hcc_test_start(start_cmd);
    if (ret == -OAL_EFAIL) {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        g_hcc_test_event->errorno = ret;
#endif
        OAL_COMPLETE(&g_hcc_test_event->test_done);
        OAL_IO_PRINT("hcc test work start test pkt send fail.\n");
        return;
    }
}

OAL_STATIC int32_t hcc_test_get_para_part1(char *buf, ssize_t *count, ssize_t max_len, ssize_t max_count)
{
    int         ret;
    const char *mode_str = OAL_PTR_NULL;
    oal_int32   tmp_mode_idx;

    tmp_mode_idx = g_hcc_test_event->test_data.mode_idx;

    mode_str = "unknown";

    if ((tmp_mode_idx >= 0) && (tmp_mode_idx < (int32_t)OAL_ARRAY_SIZE(g_hcc_test_stru))) {
        mode_str = g_hcc_test_stru[tmp_mode_idx].mode;
    }

    hcc_test_throughput_gen();

    ret = snprintf_s(buf + *count, max_len - *count, max_count - *count, "Test_Mode: %s\n", mode_str);
    if (ret < 0) {
        return ret;
    }
    *count += ret;

    ret = snprintf_s(buf + *count, max_len - *count, max_count - *count, "Actual sent %d pkts, request %llu bytes\n",
                     g_hcc_test_event->test_data.pkt_sent,
                     ((oal_uint64)g_hcc_test_event->test_data.pkt_sent) * //lint !e571
                     (oal_uint64)g_hcc_test_event->test_data.pkt_len);//lint !e571
    if (ret < 0) {
        return ret;
    }
    *count += ret;

    ret = snprintf_s(buf + *count, max_len - *count, max_count - *count, "Actual rcvd %d pkts, request %llu bytes\n",
                     g_hcc_test_event->test_data.pkt_rcvd,
                     ((oal_uint64)g_hcc_test_event->test_data.pkt_rcvd) * //lint !e571
                     (oal_uint64)g_hcc_test_event->test_data.pkt_len);//lint !e571
    if (ret < 0) {
        return ret;
    }
    *count += ret;

    ret = snprintf_s(buf + *count, max_len - *count, max_count - *count,
                     "PayloadSend %llu bytes, SDIOActualSend  %llu bytes\n",
                     g_hcc_test_event->test_data.total_sent_bytes,
                     g_hcc_test_event->test_data.trans_info.total_h2d_trans_bytes);
    if (ret < 0) {
        return ret;
    }
    *count += ret;

    ret = snprintf_s(buf + *count, max_len - *count, max_count - *count,
                     "PayloadRcvd %llu bytes, SDIOActualRecv  %llu bytes\n",
                     g_hcc_test_event->test_data.total_rcvd_bytes,
                     g_hcc_test_event->test_data.trans_info.total_d2h_trans_bytes);
    if (ret < 0) {
        return ret;
    }
    *count += ret;

    return ret;
}

OAL_STATIC int32_t hcc_test_get_para_part2(char *buf, ssize_t *count, ssize_t max_len, ssize_t max_count)
{
    int         ret;

    /* SDIO通道利用率 */
    ret = snprintf_s(buf + *count, max_len - *count, max_count - *count, "Hcc Utilization Ratio %llu\n",
                     hcc_test_utilization_ratio_gen(g_hcc_test_event->test_data.total_sent_bytes +
                                                    g_hcc_test_event->test_data.total_rcvd_bytes,
                                                    g_hcc_test_event->test_data.trans_info.total_h2d_trans_bytes +
                                                    g_hcc_test_event->test_data.trans_info.total_d2h_trans_bytes));
    if (ret < 0) {
        return *count;
    }
    *count += ret;

    /* 发送方向的丢包率 */
    ret = snprintf_s(buf + *count, max_len - *count, max_count - *count,
                     "TxPackageLoss %u, pkt_sent: %d actual_tx_pkts: %u\n",
                     hcc_test_tx_pkt_loss_gen(g_hcc_test_event->test_data.pkt_sent,
                                              g_hcc_test_event->test_data.trans_info.actual_tx_pkts),
                     g_hcc_test_event->test_data.pkt_sent, g_hcc_test_event->test_data.trans_info.actual_tx_pkts);
    if (ret < 0) {
        return *count;
    }
    *count += ret;

    ret = snprintf_s(buf + *count, max_len - *count, max_count - *count, "Requet Generate %d pkts\n",
                     g_hcc_test_event->test_data.pkt_gen);
    if (ret < 0) {
        return *count;
    }
    *count += ret;

    ret = snprintf_s(buf + *count, max_len - *count, max_count - *count, "Per-package Length %d\n",
                     g_hcc_test_event->test_data.pkt_len);
    if (ret < 0) {
        return *count;
    }
    *count += ret;

    ret = snprintf_s(buf + *count, max_len - *count, max_count - *count, 
                     "TranserTimeCost %llu Seconds, %llu microsecond\n",
                     g_hcc_test_event->test_data.trans_time, g_hcc_test_event->test_data.trans_time_us);
    if (ret < 0) {
        return *count;
    }
    *count += ret;

    ret = snprintf_s(buf + *count, max_len - *count, max_count - *count, "Throughput %u Mbps\n",
                     (oal_int32)g_hcc_test_event->test_data.throughput);
    if (ret < 0) {
        return *count;
    }
    *count += ret;

    return ret;
}


#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    ssize_t  hcc_test_get_para(struct kobject *dev, struct kobj_attribute *attr, char *buf)
#elif (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    OAL_STATIC ssize_t  hcc_test_get_para(struct kobject *dev, struct kobj_attribute *attr, char *buf)
#endif
{
    int         ret;
    ssize_t     count = 0;
    ssize_t     max_len = PAGE_SIZE - 1;
    ssize_t     max_count = PAGE_SIZE - 2;

    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    ret = hcc_test_get_para_part1(buf, &count, max_len, max_count);
    if (ret < 0) {
        OAL_IO_PRINT("hcc_test_get_para::hcc_test_get_para_part1 fail\n");
        return count;
    }

    ret = hcc_test_get_para_part2(buf, &count, max_len, max_count);
    if (ret < 0) {
        OAL_IO_PRINT("hcc_test_get_para::hcc_test_get_para_part1 fail\n");
    }

    return count;
}
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
void hcc_test_result_report(oal_uint32 throughput, oal_uint64 seconds)
{
    const oal_uint8 *mode_desc;
    oal_uint32  idx;

    idx = g_hcc_test_event->test_data.mode_idx;

    mode_desc = g_hcc_test_stru[idx].mode_desc;
    printk("\n[WIFI]########%s##SDIO Test %d pkts sent##########\n", mode_desc, g_hcc_test_event->test_data.pkt_sent);
    printk("[WIFI]########%s##SDIO Test %d pkts rcvd##########\n", mode_desc, g_hcc_test_event->test_data.pkt_rcvd);
    printk("[WIFI]########%s##SDIO Test %d pkts generate##########\n", mode_desc, g_hcc_test_event->test_data.pkt_gen);
    printk("[WIFI]########%s##SDIO Test pkt lengh %d##########\n", mode_desc, g_hcc_test_event->test_data.pkt_len);
    printk("[WIFI]########%s##SDIO Test Throughput %u Mbps##########\n", mode_desc, throughput);
    printk("[WIFI]########Hi110x SDIO Test## Used  %llu seconds##########\n\n", seconds);
}
#endif

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    ssize_t  hcc_test_set_para(struct kobject *dev, struct kobj_attribute *attr, const char *buf[], size_t count)
#elif (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    OAL_STATIC ssize_t  hcc_test_set_para(struct kobject *dev, struct kobj_attribute *attr,
                                          const char *buf, size_t count)
#endif
{
    hcc_test_data  data = {0};
    oal_int32              tmp_pkt_len;
    oal_int32              tmp_pkt_gen;
    char                mode[HCC_MODE_LEN] = {0};
    oal_int32              i;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
#endif
    OAL_BUG_ON(buf == NULL);

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    if ((sscanf_s(buf[0], "%s", mode, HCC_MODE_LEN) != 1) || (sscanf_s(buf[1], "%d", &tmp_pkt_len) != 1)
        || (sscanf_s(buf[2], "%d", &tmp_pkt_gen) != 1))
#elif (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if ((sscanf_s(buf, "%15s %d %d", mode, HCC_MODE_LEN, &tmp_pkt_len, &tmp_pkt_gen) < 1))
#endif
    {
        return -OAL_EINVAL;
    }

    for (i = 0; i < (int32_t)OAL_ARRAY_SIZE(g_hcc_test_stru); i++) {
        /* find mode if match */
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
        if (!strcmp(g_hcc_test_stru[i].mode, mode))
#elif (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        if (sysfs_streq(g_hcc_test_stru[i].mode, mode))
#endif
        {
            break;
        }
    }
    if (i == OAL_ARRAY_SIZE(g_hcc_test_stru)) {
        return -OAL_EINVAL;
    }

    data.pkt_len = tmp_pkt_len;
    data.pkt_gen = tmp_pkt_gen;
    data.mode_idx = i;

    if (hcc_test_set_case(&data)) {
        return -OAL_EINVAL;
    }

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    hcc_test_result_report(g_hcc_test_event->test_data.throughput, g_hcc_test_event->test_data.trans_time);
    hcc_test_exit_module(hcc_get_default_handler());
#endif

    return count;
}
#if (_PRE_OS_VERSION_LINUX  == _PRE_OS_VERSION)
    OAL_STATIC struct kobj_attribute dev_attr_test =
    __ATTR(test, (S_IRUGO | S_IWUSR), hcc_test_get_para, hcc_test_set_para); //lint !e64
#endif

OAL_STATIC ssize_t  hcc_test_set_abort_test(struct kobject *dev, struct kobj_attribute *attr,
                                            const char *buf, size_t count)
{
    struct hcc_handler* hcc;
    hcc = hcc_get_default_handler();
    OAL_IO_PRINT("hcc_test_set_abort_test msg send\n");
    oal_channel_send_msg(hcc->hi_channel, H2D_MSG_STOP_SDIO_TEST);

    oal_msleep(500);
    return count;
}
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    OAL_STATIC struct kobj_attribute dev_attr_abort_test =
    __ATTR(abort_test, S_IWUSR, NULL, hcc_test_set_abort_test); //lint !e64
#endif

OAL_STATIC ssize_t  hcc_test_set_value(struct kobject *dev, struct kobj_attribute *attr, const char *buf, size_t count)
{
    oal_uint32      value;

    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    if ((sscanf_s(buf, "0x%2x", &value) != 1)) {
        OAL_IO_PRINT("set value one char!\n");
        return -OAL_EINVAL;
    }

    g_hcc_test_event->test_value = value;

    return count;
}

OAL_STATIC ssize_t  hcc_test_get_value(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int ret = 0;
    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    ret +=  snprintf_s(buf + ret, PAGE_SIZE - 1, PAGE_SIZE - 2, "0x%2x\n", g_hcc_test_event->test_value);
    if (ret < EOK) {
        OAL_IO_PRINT("hcc_test_get_value::snprintf_s failed!\n");
        return ret;
    }
    return ret;
}

OAL_STATIC ssize_t  hcc_test_set_queue_id(struct kobject *dev, struct kobj_attribute *attr, 
                                          const char *buf, size_t count)
{
    oal_uint32      queue_id;

    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    if ((sscanf_s(buf, "%u", &queue_id) != 1)) {
        OAL_IO_PRINT("set value one char!\n");
        return -OAL_EINVAL;
    }
    if (queue_id >= HCC_QUEUE_COUNT) {
        OAL_IO_PRINT("wrong queue id:%u\n", queue_id);
        return count;
    }
    g_hcc_test_event->hcc_test_queue = (hcc_queue_type)queue_id;

    return count;
}

OAL_STATIC ssize_t  hcc_test_get_queue_id(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int ret = 0;
    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    ret +=  snprintf_s(buf + ret, PAGE_SIZE - 1, PAGE_SIZE - 2, "%d\n", g_hcc_test_event->hcc_test_queue);
    if (ret < EOK) {
        OAL_IO_PRINT("hcc_test_get_queue_id::snprintf_s failed!\n");
        return ret;
    }
    return ret;
}

OAL_STATIC ssize_t  hcc_test_set_pad_payload(struct kobject *dev, struct kobj_attribute *attr,
                                             const char *buf, size_t count)
{
    oal_uint32      pad_payload;

    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    if ((sscanf_s(buf, "%u", &pad_payload) != 1)) {
        OAL_IO_PRINT("set value one char!\n");
        return -OAL_EINVAL;
    }

    g_hcc_test_event->pad_payload = (oal_uint8)pad_payload;

    return count;
}

OAL_STATIC ssize_t  hcc_test_get_pad_payload(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int ret = 0;
    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    ret +=  snprintf_s(buf + ret, PAGE_SIZE - 1, PAGE_SIZE - 2, "%d\n", g_hcc_test_event->pad_payload);
    if (ret < EOK) {
        OAL_IO_PRINT("hcc_test_get_pad_payload::snprintf_s failed !\n");
        return ret;
    }
    return ret;
}

OAL_STATIC ssize_t  hcc_test_set_verified(struct kobject *dev, struct kobj_attribute *attr,
                                          const char *buf, size_t count)
{
    oal_uint32      verified;

    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    if ((sscanf_s(buf, "%u", &verified) != 1)) {
        OAL_IO_PRINT("set value one char!\n");
        return -OAL_EINVAL;
    }
    verified = !!verified;
    g_hcc_test_event->verified = (oal_uint8)verified;

    return count;
}

OAL_STATIC ssize_t  hcc_test_get_verified(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int ret = 0;
    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    ret +=  snprintf_s(buf + ret, PAGE_SIZE - 1, PAGE_SIZE - 2, "%d\n", g_hcc_test_event->verified);
    if (ret < EOK) {
        OAL_IO_PRINT("hcc_test_get_verified::snprintf_s failed!\n");
        return ret;
    }
    return ret;
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
OAL_STATIC struct kobj_attribute dev_attr_value =
    __ATTR(value, (S_IRUGO | S_IWUSR), hcc_test_get_value, hcc_test_set_value); //lint !e64
OAL_STATIC struct kobj_attribute dev_attr_queue_id =
    __ATTR(queue_id, (S_IRUGO | S_IWUSR), hcc_test_get_queue_id, hcc_test_set_queue_id); //lint !e64
OAL_STATIC struct kobj_attribute dev_attr_pad_payload =
    __ATTR(pad_payload, (S_IRUGO | S_IWUSR), hcc_test_get_pad_payload, hcc_test_set_pad_payload); //lint !e64
OAL_STATIC struct kobj_attribute dev_attr_verified =
    __ATTR(verified, (S_IRUGO | S_IWUSR), hcc_test_get_verified, hcc_test_set_verified); //lint !e64

OAL_STATIC struct attribute *hcc_test_sysfs_entries[] = {
    &dev_attr_test.attr,
    &dev_attr_abort_test.attr,
    &dev_attr_value.attr,
    &dev_attr_verified.attr,
    &dev_attr_queue_id.attr,
    &dev_attr_pad_payload.attr,
    // &dev_attr_Readme.attr,
    NULL
};

OAL_STATIC struct attribute_group hcc_test_attribute_group = {
    .name = "test",
    .attrs = hcc_test_sysfs_entries,
};

OAL_STATIC struct attribute *hcc_sysfs_entries[] = {
    &dev_attr_flowctrl.attr,
    &dev_attr_assem_info.attr,
    &dev_attr_queues_pkts.attr,
    &dev_attr_queues_len.attr,
    &dev_attr_wakelock.attr,
    NULL
};
OAL_STATIC struct attribute_group hcc_attribute_group = {
    // .name = "hcc",
    .attrs = hcc_sysfs_entries,
};
#endif

oal_int32 hsdio_sysfs_info_print(oal_void *data, char *buf, oal_int32 buf_len)
{
    oal_int32 ret = 0;
    oal_int32 bit;
    struct oal_sdio *hi_sdio = (struct oal_sdio *)data;
    ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "sdio info, state:0x%4x\n", hi_sdio->state);
    ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                       "gpio_int_count:%llu \n",  hi_sdio->gpio_int_count);
    ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                       "wakeup_int_count:%llu \n", hi_sdio->wakeup_int_count);
    ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                       "func1_err_reg_info:%u \n", hi_sdio->func1_stat.func1_err_reg_info);
    ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                       "func1_err_int_count:%u \n", hi_sdio->func1_stat.func1_err_int_count);
    ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                       "func1_ack_int_acount:%u \n", hi_sdio->func1_stat.func1_ack_int_acount);
    ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                       "func1_msg_int_count:%u \n", hi_sdio->func1_stat.func1_msg_int_count);
    ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                       "func1_data_int_count:%u \n", hi_sdio->func1_stat.func1_data_int_count);
    ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                       "func1_no_int_count:%u \n", hi_sdio->func1_stat.func1_no_int_count);
    ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "\nsdio error stat:\n");
    ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                       "rx_scatt_info_not_match:%d\n", hi_sdio->error_stat.rx_scatt_info_not_match);
    ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "msg count info:\n");
    ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "tx scatt buf len:%u\n", hi_sdio->scatt_buff.len);
    for (bit = 0; bit < D2H_MSG_COUNT; bit++) {
        if (hi_sdio->msg[bit].count)
            ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1,
                               "msg [%d] count:%u:,last update time:%llu\n",
                               bit, hi_sdio->msg[bit].count,
                               hi_sdio->msg[bit].cpu_time);
    }
    ret +=  snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "last msg:%u\n", hi_sdio->last_msg);
    if (ret < EOK) {
        OAL_IO_PRINT("hsdio_sysfs_info_print::snprintf_s failed !\n");
        return ret;
    }
    return ret;
}

OAL_STATIC ssize_t  hsdio_get_sdio_info(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int ret = 0;
    struct hcc_handler* hcc = OAL_PTR_NULL;
    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    hcc = hcc_get_default_handler();
    if (hcc == NULL) {
        OAL_IO_PRINT("get hcc handler failed!%s\n", __FUNCTION__);
        return ret;
    }
    /* add by zhanglu begin */
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    if (hcc->hi_channel)
        ret += hsdio_sysfs_info_print(hcc->hi_channel, buf, PAGE_SIZE - ret);
#endif
    /* add by zhanglu end */
    return ret;
}

oal_int32 hsdio_wakelock_info_print(struct oal_sdio *hi_sdio, char* buf, oal_int32 buf_len)
{
    oal_int32 ret;
    oal_int32 count = 0;
#ifdef CONFIG_PRINTK
    if (hi_sdio->st_sdio_wakelock.locked_addr) {
        ret = snprintf_s(buf + count, buf_len - count, buf_len - count - 1, "wakelocked by:%pf\n",
                         (oal_void *)(uintptr_t)(hi_sdio->st_sdio_wakelock.locked_addr));
        if (ret < EOK) {
            return count;
        }
        count += ret;
    }
#endif

    ret = snprintf_s(buf + count, buf_len - count, buf_len - count - 1, "hold %lu locks\n",
                     hi_sdio->st_sdio_wakelock.lock_count);
    if (ret < EOK) {
        OAL_IO_PRINT("hsdio_wakelock_info_print::snprintf_s failed !\n");
        return count;
    }
    count += ret;
    return count;
}

OAL_STATIC ssize_t  hsdio_get_wakelock_info(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int ret = 0;
    struct hcc_handler* hcc = OAL_PTR_NULL;
    OAL_BUG_ON(dev == NULL);
    OAL_BUG_ON(attr == NULL);
    OAL_BUG_ON(buf == NULL);

    hcc = hcc_get_default_handler();
    if (hcc == NULL) {
        OAL_IO_PRINT("get hcc handler failed!%s\n", __FUNCTION__);
        return ret;
    }
    /* add by zhanglu begin */
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    if (hcc->hi_channel)
        ret += hsdio_wakelock_info_print((struct oal_sdio *)hcc->hi_channel, buf, PAGE_SIZE - ret);
#endif
    return ret;
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
OAL_STATIC struct kobj_attribute dev_attr_sdio_info = __ATTR(sdio_info, S_IRUGO, hsdio_get_sdio_info, NULL);
OAL_STATIC struct kobj_attribute dev_attr_sdio_wakelock = __ATTR(wakelock, S_IRUGO, hsdio_get_wakelock_info, NULL);

OAL_STATIC struct attribute *hsdio_sysfs_entries[] = {
    &dev_attr_sdio_info.attr,
    &dev_attr_sdio_wakelock.attr,
    NULL
};

OAL_STATIC struct attribute_group hsdio_attribute_group = {
    .name = "sdio",
    .attrs = hsdio_sysfs_entries,
};
#endif
int hcc_test_set_case(hcc_test_data *data)
{
    int ret;
    int errorno;

    OAL_BUG_ON(data == NULL);
    if (OAL_UNLIKELY(g_hcc_test_event->test_workqueue == NULL)) {
        /* create workqueue */
        g_hcc_test_event->test_workqueue = oal_create_singlethread_workqueue("hcc_test");
        if (g_hcc_test_event->test_workqueue == NULL) {
            OAL_IO_PRINT("work queue create fail.\n");
            return -OAL_ENOMEM;
        }
        INIT_WORK(&g_hcc_test_event->test_work, hcc_test_work);
    }

    oal_mutex_lock(&g_hcc_test_event->mutex_lock);
    if (g_hcc_test_event->started == OAL_TRUE) {
        oal_mutex_unlock(&g_hcc_test_event->mutex_lock);
        return -OAL_EINVAL;
    }

    OAL_IO_PRINT("%s Test Start,pkts:%d,len:%d\n", g_hcc_test_stru[data->mode_idx].mode, data->pkt_gen, data->pkt_len);

    g_hcc_test_event->started = OAL_TRUE;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    g_hcc_test_event->errorno = OAL_SUCC;
#endif

    if (memcpy_s(&g_hcc_test_event->test_data, sizeof(hcc_test_data), data, sizeof(hcc_test_data)) != EOK) {
        oal_mutex_unlock(&g_hcc_test_event->mutex_lock);
        return -OAL_EINVAL;
    }
    g_test_force_stop = 0;
    OAL_INIT_COMPLETION(&g_hcc_test_event->test_done);

    queue_work(g_hcc_test_event->test_workqueue, &g_hcc_test_event->test_work);

    ret = WAIT_FOR_COMPELTION_INTER(g_hcc_test_event->test_done);
    if (ret < 0) {
        OAL_IO_PRINT("Test Event  terminated ret=%d\n", ret);
        g_hcc_test_event->started = OAL_FALSE;
        OAL_COMPLETE(&g_hcc_test_event->test_trans_done);
        g_test_force_stop = 1;
        cancel_work_sync(&g_hcc_test_event->test_work);
    }

    hcc_test_throughput_gen();
    if (memcpy_s(data, sizeof(hcc_test_data), &g_hcc_test_event->test_data, sizeof(hcc_test_data)) != EOK) {
        oal_mutex_unlock(&g_hcc_test_event->mutex_lock);
        return -OAL_EINVAL;
    }
    g_hcc_test_event->started = OAL_FALSE;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    errorno = g_hcc_test_event->errorno;
#endif
    oal_mutex_unlock(&g_hcc_test_event->mutex_lock);
    return errorno;
}

int conn_test_sdio_loop(char *param)
{
    hcc_test_data data = {0};
    OAL_REFERENCE(param);
    data.pkt_len = 1500;
    data.pkt_gen = 10000;
    data.mode_idx = HCC_TEST_CMD_START_LOOP;

    if (hcc_test_set_case(&data)) {
        OAL_IO_PRINT("conn_test_sdio_loop test sdio failed!\n");
        return -1;
    }
    return 0;
}
EXPORT_SYMBOL(conn_test_sdio_loop);

oal_void hcc_test_get_case(hcc_test_data *data)
{
    OAL_BUG_ON(data == NULL);
    hcc_test_throughput_gen();
    if (memcpy_s((oal_void*)data, sizeof(hcc_test_data),
                 (oal_void*)&g_hcc_test_event->test_data, sizeof(hcc_test_data)) != EOK) {
        OAL_IO_PRINT("hcc_test_get_case memcpy_s failed!\n");
    }
}

#ifdef _PRE_CONFIG_HISI_PANIC_DUMP_SUPPORT
OAL_STATIC oal_int32 hwifi_panic_handler(struct notifier_block *this,
                                         oal_ulong event, oal_void *unused)
{
    if (wifi_panic_debug)
        hwifi_panic_log_dump(KERN_ERR);
    else
        printk(KERN_WARNING"wifi panic debug off\n");
    return NOTIFY_OK;
}

OAL_STATIC struct notifier_block hwifi_panic_notifier = {
    .notifier_call  = hwifi_panic_handler,
};
#endif

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    #define HCC_TEST_TASK_SIZE                        0x2000
    #define HCC_TEST_TASK_PRIO                        10
#endif

oal_int32  hcc_test_init_module(struct hcc_handler* hcc)
{
    oal_int32       ret = OAL_SUCC;
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    oal_int32       taskid;
#endif
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_kobject*     pst_root_object = NULL;

    pst_root_object = oal_get_sysfs_root_object();
    if (pst_root_object == NULL) {
        OAL_IO_PRINT("[E]get root sysfs object failed!\n");
        return -OAL_EFAIL;
    }

    g_conn_syfs_hcc_object = kobject_create_and_add("hcc", pst_root_object);
    if (g_conn_syfs_hcc_object == NULL) {
        goto fail_g_conn_syfs_hcc_object;
    }
    ret = oal_debug_sysfs_create_group(g_conn_syfs_hcc_object, &hsdio_attribute_group);
    if (ret) {
        ret = -OAL_ENOMEM;
        OAL_IO_PRINT("sysfs create sdio_sysfs_entries group fail.ret=%d\n", ret);
        goto fail_create_sdio_group;
    }
    /* create the files associated with this kobject */
    ret = oal_debug_sysfs_create_group(g_conn_syfs_hcc_object, &hcc_test_attribute_group);
    if (ret) {
        ret = -OAL_ENOMEM;
        OAL_IO_PRINT("sysfs create test group fail.ret=%d\n", ret);
        goto fail_create_hcc_test_group;
    }
    ret = oal_debug_sysfs_create_group(g_conn_syfs_hcc_object, &hcc_attribute_group);
    if (ret) {
        OAL_IO_PRINT("sysfs create hcc group fail.ret=%d\n", ret);
        ret = -OAL_ENOMEM;
        goto fail_create_hcc_group;
    }
#endif
    /* alloc memory for perf_action pointer */
    g_hcc_test_event = oal_kzalloc(sizeof(*g_hcc_test_event), OAL_GFP_KERNEL);
    if (g_hcc_test_event == NULL) {
        OAL_IO_PRINT("error kzalloc g_hcc_test_event mem.\n");
        ret = -OAL_ENOMEM;
        goto fail_g_hcc_test_event;
    }

    g_hcc_test_event->hcc_test_queue = DATA_LO_QUEUE;
    g_hcc_test_event->test_value = 0x5a;

    /* register callback for rx */
    ret = hcc_rx_register(hcc, HCC_ACTION_TYPE_TEST, hcc_test_rcvd, NULL);
    if (ret != OAL_SUCC) {
        OAL_IO_PRINT("error %d register callback for rx.\n", ret);
        ret = -OAL_EFAIL;
        goto fail_rx_cb_register;
    }

    /* mutex lock init */
    OAL_MUTEX_INIT(&g_hcc_test_event->mutex_lock);

    OAL_INIT_COMPLETION(&g_hcc_test_event->test_done);
    OAL_INIT_COMPLETION(&g_hcc_test_event->test_trans_done);

    /* init hcc_test param */
    g_hcc_test_event->test_data.mode_idx = -1;
    g_hcc_test_event->test_data.pkt_len  = 0;
    g_hcc_test_event->test_data.pkt_sent = 0;
    g_hcc_test_event->test_data.pkt_gen  = 0;
    g_hcc_test_event->started  = OAL_FALSE;

#ifdef _PRE_CONFIG_HISI_PANIC_DUMP_SUPPORT
    atomic_notifier_chain_register(&panic_notifier_list,
                                   &hwifi_panic_notifier);
#endif

#ifdef _PRE_CONFIG_HISI_PANIC_DUMP_SUPPORT
    hwifi_panic_log_register(&sdio_panic, hcc->hi_channel);
    hwifi_panic_log_register(&hcc_panic_assem_info, hcc);
    hwifi_panic_log_register(&hcc_panic_flowctrl, hcc);

    hwifi_panic_log_register(&hcc_panic_queues_len, hcc);
    hwifi_panic_log_register(&hcc_panic_queues_pkts, hcc);
#endif
    return ret;
fail_rx_cb_register:
    kfree(g_hcc_test_event);
    g_hcc_test_event = NULL;
fail_g_hcc_test_event:
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_debug_sysfs_remove_group(g_conn_syfs_hcc_object, &hcc_attribute_group);
#endif
fail_create_hcc_group:
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_debug_sysfs_remove_group(g_conn_syfs_hcc_object, &hcc_test_attribute_group);
#endif
fail_create_hcc_test_group:
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_debug_sysfs_remove_group(g_conn_syfs_hcc_object, &hsdio_attribute_group);
#endif
fail_create_sdio_group:
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    kobject_put(g_conn_syfs_hcc_object);
#endif
fail_g_conn_syfs_hcc_object:
    return ret;
}

oal_void  hcc_test_exit_module(struct hcc_handler *hcc)
{
#ifdef _PRE_CONFIG_HISI_PANIC_DUMP_SUPPORT
    atomic_notifier_chain_unregister(&panic_notifier_list,
                                     &hwifi_panic_notifier);
#endif

    if (g_hcc_test_event->test_workqueue != NULL) {
        destroy_workqueue(g_hcc_test_event->test_workqueue);
        g_hcc_test_event->test_workqueue = NULL;
    }
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_debug_sysfs_remove_group(g_conn_syfs_hcc_object, &hcc_attribute_group);
    oal_debug_sysfs_remove_group(g_conn_syfs_hcc_object, &hcc_test_attribute_group);
    oal_debug_sysfs_remove_group(g_conn_syfs_hcc_object, &hsdio_attribute_group);
    kobject_put(g_conn_syfs_hcc_object);
#endif
    hcc_rx_unregister(hcc, HCC_ACTION_TYPE_TEST);
    OAL_MUTEX_DESTROY(&g_hcc_test_event->mutex_lock);
    kfree(g_hcc_test_event);
    g_hcc_test_event = NULL;
}

#endif
