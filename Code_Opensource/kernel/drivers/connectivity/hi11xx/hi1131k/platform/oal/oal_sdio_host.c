
#define  HISI_LOG_TAG "[SDIO]"
#include "oal_util.h"
#include "oal_sdio.h"
#include "oal_sdio_host_if.h"
#include "oal_net.h"
#include "heartbeat_host.h"
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "plat_board_adapt.h"
#endif
#ifdef CONFIG_MMC
#include "oal_hcc_host_if.h"
#include "exception_rst.h"
#include "plat_pm.h"
#include "plat_debug.h"
#include "oal_interrupt.h"
#include "oal_thread.h"
#include "securec.h"
#include "plat_firmware.h"
#include "oam_ext_if.h"
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#include "oal_scatterlist.h"
#include "plat_board_adapt.h"
#endif
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAL_SDIO_HOST_C

#ifdef CONFIG_MMC

struct task_struct         *sdio_int_task = NULL;
#undef CONFIG_SDIO_MSG_ACK_HOST2ARM_DEBUG

#ifdef CONFIG_SDIO_DEBUG
static struct oal_sdio* hi_sdio_debug;
#endif

/*
 * 2 Global Variable Definition
 */
OAL_STATIC  oal_completion  sdio_driver_complete;

oal_semaphore_stru chan_wake_sema;

OAL_STATIC  struct oal_sdio  *_hi_sdio_;

OAL_STATIC int8_t* sdio_enum_err_str = "probe timeout";

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
oal_uint32 g_ul_pm_wakeup_event = OAL_FALSE;

#define SDIO_RESET_RETRY                          (5)
enum {
    DUMP_PREFIX_NONE,
    DUMP_PREFIX_ADDRESS,
    DUMP_PREFIX_OFFSET
};

struct sdio_func *p_gst_sdio_func = NULL;

#endif
#define SDIO_RX_RETRY                             5
#define SDIO_REINIT_DELAY                         10

oal_void oal_sdio_dispose_data(struct oal_sdio  *hi_sdio);
oal_int32 oal_sdio_data_sg_irq(struct oal_sdio *hi_sdio);

/* 0 -sdio 1-gpio */
#ifdef _PRE_FEATURE_NO_GPIO
oal_int32 g_hisdio_intr_mode = 0;
#else
oal_int32 g_hisdio_intr_mode = 1;
#endif
module_param(g_hisdio_intr_mode, int, S_IRUGO | S_IWUSR);

#ifdef CONFIG_SDIO_FUNC_EXTEND
oal_uint32 sdio_extend_func = 1;
#else
oal_uint32 sdio_extend_func = 0;
#endif
module_param(sdio_extend_func, uint, S_IRUGO | S_IWUSR);

oal_uint32 wifi_patch_enable = 1;
module_param(wifi_patch_enable, uint, S_IRUGO | S_IWUSR);

OAL_STATIC oal_int32 oal_sdio_single_transfer(struct oal_sdio *hi_sdio, oal_int32 rw,
                                              oal_void* buf, oal_uint32 size);

OAL_STATIC oal_int32 _oal_sdio_transfer_scatt(struct oal_sdio *hi_sdio, oal_int32 rw,
                                              oal_uint32 addr, struct scatterlist *sg,
                                              oal_uint32 sg_len,
                                              oal_uint32 rw_sz);
oal_void dw_mci_sdio_card_detect_change(oal_void);

OAL_STATIC oal_void oal_sdio_print_state(oal_uint32 old_state, oal_uint32 new_state)
{
    if (old_state != new_state) {
        PS_PRINT_INFO("sdio state changed, tx[%s=>%s],rx[%s=>%s]\n",
                      (old_state & OAL_SDIO_TX) ? "on " : "off",
                      (new_state & OAL_SDIO_TX) ? "on " : "off",
                      (old_state & OAL_SDIO_RX) ? "on " : "off",
                      (new_state & OAL_SDIO_RX) ? "on " : "off");
    }
}

oal_int32 oal_get_hisdio_intr_mode(void)
{
    return g_hisdio_intr_mode;
}

struct oal_sdio* oal_alloc_sdio_stru(oal_void)
{
    return _hi_sdio_;
}

struct oal_sdio* oal_get_sdio_default_handler(oal_void)
{
    return _hi_sdio_;
}

oal_uint64 oal_get_gpio_int_count_para(oal_void)
{
    struct oal_sdio *hi_sdio;

    hi_sdio = oal_get_sdio_default_handler();

    return hi_sdio->gpio_int_count;
}

oal_void oal_free_sdio_stru(struct oal_sdio* hi_sdio)
{
    OAL_IO_PRINT("oal_free_sdio_stru\n");
}

oal_int32 oal_sdio_send_msg(struct oal_sdio* hi_sdio, oal_ulong val)
{
    oal_int32       ret  = OAL_SUCC;
    struct sdio_func           *func = OAL_PTR_NULL;

    OAL_BUG_ON(!hi_sdio);

    if (hi_sdio->func == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{oal_sdio_send_msg::sdio is not initialized,can't send sdio msg!}");
        return -OAL_EINVAL;
    }

    func = hi_sdio->func;
#ifdef CONFIG_SDIO_DEBUG
    OAL_IO_PRINT("send msg to notice device [0x%8x]\n", (oal_uint32)val);
#endif
    if (val >= H2D_MSG_COUNT) {
        OAL_IO_PRINT("[Error]invaild param[%lu]!\n", val);
        return -OAL_EINVAL;
    }
    oal_sdio_wake_lock(hi_sdio);
    sdio_claim_host(func);
    /* sdio message can sent when wifi power on */
    if (hi110x_get_wifi_power_stat() == 0) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{oal_sdio_send_msg::wifi power off,can't send sdio msg!}");
        sdio_release_host(func);
        oal_sdio_wake_unlock(hi_sdio);
        return -OAL_EBUSY;
    }

    oal_sdio_writel(func, (1 << val),
                    HISDIO_REG_FUNC1_WRITE_MSG, &ret);
    if (ret) {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{oal_sdio_send_msg::failed to send sdio msg[%lu]!ret=%d}", val, ret);
    }
    sdio_release_host(func);
    oal_sdio_wake_unlock(hi_sdio);
    return ret;
}

oal_int32 oal_sdio_rw_buf(struct oal_sdio *hi_sdio, oal_int32 rw,
                          oal_uint32 addr, oal_uint8 *buf, oal_uint32 rw_sz)
{
    struct sdio_func *func = hi_sdio->func;
    oal_int32             ret  = OAL_SUCC;

    /* padding len of buf has been assure when alloc */
    rw_sz  = HISDIO_ALIGN_4_OR_BLK(rw_sz);
    if (OAL_WARN_ON(rw_sz != HISDIO_ALIGN_4_OR_BLK(rw_sz))) {
        /* just for debug, remove later */
        OAL_IO_PRINT("invaild len %u\n", rw_sz);
        return -OAL_EINVAL;
    }

    sdio_claim_host(func);
    if (rw == SDIO_READ) {
        ret = oal_sdio_readsb(func, buf, addr, rw_sz);
    } else if (rw == SDIO_WRITE) {
        ret = oal_sdio_writesb(func, addr, buf, rw_sz);
    }

    sdio_release_host(func);

    return ret;
}

oal_int32 oal_sdio_check_rx_len(struct oal_sdio *hi_sdio, oal_uint32 count)
{
    return OAL_SUCC;
}

OAL_STATIC oal_int32 oal_sdio_xfercount_get(struct oal_sdio *hi_sdio, oal_uint32 *xfercount)
{
    oal_int32 ret = 0;
#ifdef CONFIG_SDIO_MSG_ACK_HOST2ARM_DEBUG
    /* read from 0x0c */
    *xfercount = oal_sdio_readl(hi_sdio->func, HISDIO_REG_FUNC1_XFER_COUNT, &ret);
    if (OAL_UNLIKELY(ret)) {
        OAL_IO_PRINT("[ERROR]sdio read single package len failed ret=%d\n", ret);
        return ret;
    }
    hi_sdio->sdio_extend->xfer_count = *xfercount;
#else
    if (sdio_extend_func) {
        *xfercount = hi_sdio->sdio_extend->xfer_count;
        return OAL_SUCC;
    }

    /* read from 0x0c */
    *xfercount = oal_sdio_readl(hi_sdio->func, HISDIO_REG_FUNC1_XFER_COUNT, &ret);
    if (OAL_UNLIKELY(ret)) {
        OAL_IO_PRINT("[E]sdio read xercount failed ret=%d\n", ret);
        DECLARE_DFT_TRACE_KEY_INFO("sdio readl 0x0c fail", OAL_DFT_TRACE_FAIL);
        return ret;
    }
    hi_sdio->sdio_extend->xfer_count = *xfercount;
#endif
    return OAL_SUCC;
}

oal_int32 oal_sdio_data_sg_irq(struct oal_sdio *hi_sdio)
{
    struct sdio_func   *func = OAL_PTR_NULL;
    oal_int32 ret;
    oal_uint32              xfer_count;

    OAL_BUG_ON(!hi_sdio);
    OAL_BUG_ON(!hi_sdio->func);

    func = hi_sdio->func;

    ret = oal_sdio_xfercount_get(hi_sdio, &xfer_count);
    if (OAL_UNLIKELY(ret)) {
        return -OAL_EFAIL;
    }

    if (OAL_UNLIKELY(oal_sdio_check_rx_len(hi_sdio, xfer_count) != OAL_SUCC)) {
        OAL_IO_PRINT("[SDIO][Err]Sdio Rx Single Transfer len[%u] invalid\n", xfer_count);
    }

    /* beacuse get buf may cost lot of time, so release bus first */
    OAL_BUG_ON(hi_sdio->bus_ops.rx == NULL);

    sdio_release_host(func);
    hi_sdio->bus_ops.rx(hi_sdio->bus_data);
    sdio_claim_host(func);

    return OAL_SUCC;
}

oal_int32 oal_sdio_transfer_rx_register(struct oal_sdio *hi_sdio, hisdio_rx rx)
{
    OAL_BUG_ON(!hi_sdio);
    /* add lock later. */
    hi_sdio->bus_ops.rx = rx;
    return OAL_SUCC;
}
oal_module_symbol(oal_sdio_transfer_rx_register);

oal_void oal_sdio_transfer_rx_unregister(struct oal_sdio *hi_sdio)
{
    /* add lock later. */
    hi_sdio->bus_ops.rx = NULL;
}

oal_int32 oal_sdio_message_register(struct oal_sdio *hi_sdio,
                                    oal_uint8 msg, sdio_msg_rx cb,
                                    oal_void* data)
{
    if (hi_sdio == NULL || msg >= D2H_MSG_COUNT)
        return -OAL_EFAIL;
    hi_sdio->msg[msg].msg_rx = cb;
    hi_sdio->msg[msg].data = data;
    return OAL_SUCC;
}

oal_void oal_sdio_message_unregister(struct oal_sdio *hi_sdio,
                                     oal_uint8 msg)
{
    if (hi_sdio == NULL || msg >= D2H_MSG_COUNT)
        return;
    hi_sdio->msg[msg].msg_rx = NULL;
    hi_sdio->msg[msg].msg_rx = NULL;
}

OAL_STATIC oal_int32 oal_sdio_msg_stat(struct oal_sdio *hi_sdio, oal_uint32* msg)
{
    oal_int32 ret = 0;
#ifdef CONFIG_SDIO_MSG_ACK_HOST2ARM_DEBUG
    /* read from old register */
#ifdef CONFIG_SDIO_D2H_MSG_ACK
    *msg = oal_sdio_readl(hi_sdio->func, HISDIO_REG_FUNC1_MSG_FROM_DEV, &ret);
#else
    *msg = oal_sdio_readb(hi_sdio->func, HISDIO_REG_FUNC1_MSG_FROM_DEV, &ret);
#endif

    if (ret) {
        OAL_IO_PRINT("sdio readb error![ret=%d]\n", ret);
        return ret;
    }
    hi_sdio->sdio_extend->msg_stat = *msg;
#else
    if (sdio_extend_func) {
        *msg = hi_sdio->sdio_extend->msg_stat;
    }

    if (*msg == 0) {
        /* no sdio message! */
        return OAL_SUCC;
    }
#ifdef CONFIG_SDIO_D2H_MSG_ACK
    /* read from old register */
    /* 当使用0x30寄存器时需要下发CMD52读0x2B 才会产生HOST2ARM ACK中断 */
    (void)oal_sdio_readb(hi_sdio->func, HISDIO_REG_FUNC1_MSG_HIGH_FROM_DEV, &ret);
    if (ret) {
        OAL_IO_PRINT("[E]sdio readb error![ret=%d]\n", ret);
        DECLARE_DFT_TRACE_KEY_INFO("sdio readl 0x2b fail", OAL_DFT_TRACE_FAIL);
    }
#endif
#endif
    return OAL_SUCC;
}


#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#define cpu_clock(m) (m)
#endif

oal_int32 oal_sdio_msg_irq(struct oal_sdio *hi_sdio)
{
    oal_int32               bit;
    struct sdio_func       *func;
    oal_uint32              msg = 0;
    oal_int32               ret;
    unsigned long           msg_tmp;

    func       = hi_sdio->func;

    /* reading interrupt form ARM Gerneral Purpose Register(0x28)  */
    ret = oal_sdio_msg_stat(hi_sdio, &msg);
    if (ret) {
        return ret;
    }
    msg_tmp = (unsigned long)msg;

    if (!msg) {
        return OAL_SUCC;
    }

#ifdef CONFIG_SDIO_D2H_MSG_ACK
    if (!!((1U << D2H_MSG_FLOWCTRL_OFF) & msg) & !!((1U << D2H_MSG_FLOWCTRL_ON) & msg)) { //lint !e514
        OAL_WARN_ON(1);
    }
#endif
    if (oal_bit_atomic_test(D2H_MSG_DEVICE_PANIC, &msg_tmp)) {
        /* Close sdio. */
        oal_disable_sdio_state(hi_sdio, OAL_SDIO_ALL);
    }

    //    /*Heartbeat msg.*/
    //    /*需要添加心跳处理函数*/
    oal_sdio_release_host(hi_sdio);
    oal_sdio_rx_transfer_unlock(hi_sdio);

    /* 优先处理Panic消息 */
    if (oal_bit_atomic_test_and_clear(D2H_MSG_DEVICE_PANIC, &msg_tmp)) {
        bit = D2H_MSG_DEVICE_PANIC;
        hi_sdio->msg[bit].count++;
        hi_sdio->last_msg = bit;
        hi_sdio->msg[bit].cpu_time = cpu_clock(UINT_MAX);//lint !e569
        if (hi_sdio->msg[bit].msg_rx) {
            hi_sdio->msg[bit].msg_rx(hi_sdio->msg[bit].data);
        }
    }

    update_heartbeat();

    bit = 0;
    oal_bit_atomic_for_each_set(bit, (const unsigned long *)&msg_tmp, D2H_MSG_COUNT)
    {
        OAL_BUG_ON(bit >= D2H_MSG_COUNT);
        hi_sdio->msg[bit].count++;
        hi_sdio->last_msg = bit;
        hi_sdio->msg[bit].cpu_time = cpu_clock(UINT_MAX);//lint !e569
        if (hi_sdio->msg[bit].msg_rx)
            hi_sdio->msg[bit].msg_rx(hi_sdio->msg[bit].data);
    }
    oal_sdio_rx_transfer_lock(hi_sdio);
    oal_sdio_claim_host(hi_sdio);

    return OAL_SUCC;
}

oal_uint32 oal_sdio_credit_info_update(struct oal_sdio *hi_sdio)
{
    oal_uint8 short_free_cnt, large_free_cnt;
    oal_uint32 ret = 0;
    oal_spin_lock(&hi_sdio->sdio_credit_info.credit_lock);

    short_free_cnt = HISDIO_SHORT_PKT_GET(hi_sdio->sdio_extend->credit_info);
    large_free_cnt = HISDIO_LARGE_PKT_GET(hi_sdio->sdio_extend->credit_info);

    if (hi_sdio->sdio_credit_info.short_free_cnt != short_free_cnt) {
#ifdef CONFIG_SDIO_DEBUG
        OAL_IO_PRINT("short free cnt:%d ==> %d\r\n", hi_sdio->sdio_credit_info.short_free_cnt,  short_free_cnt);
#endif
        hi_sdio->sdio_credit_info.short_free_cnt = short_free_cnt;
        ret = 1;
    }

    if (hi_sdio->sdio_credit_info.large_free_cnt != large_free_cnt) {
#ifdef CONFIG_SDIO_DEBUG
        OAL_IO_PRINT("large free cnt:%d ==> %d\r\n", hi_sdio->sdio_credit_info.large_free_cnt,  large_free_cnt);
#endif
        hi_sdio->sdio_credit_info.large_free_cnt = large_free_cnt;
        ret = 1;
    }

    oal_spin_unlock(&hi_sdio->sdio_credit_info.credit_lock);

    return ret;
}

oal_void  oal_sdio_credit_update_cb_register(struct oal_sdio *hi_sdio, hisdio_rx cb)
{
    if (OAL_WARN_ON(hi_sdio->credit_update_cb != NULL))
        return;
    hi_sdio->credit_update_cb = cb;
    return;
}

#ifndef CONFIG_SDIO_MSG_ACK_HOST2ARM_DEBUG
OAL_STATIC oal_int32 oal_sdio_extend_buf_get(struct oal_sdio *hi_sdio)
{
    oal_int32 ret = OAL_SUCC;
    if (sdio_extend_func) {
        ret = oal_sdio_memcpy_fromio(hi_sdio->func, (oal_void*)hi_sdio->sdio_extend,
                                     HISDIO_EXTEND_BASE_ADDR, sizeof(struct hisdio_extend_func));
        if (OAL_LIKELY(ret == OAL_SUCC)) {
#ifdef CONFIG_SDIO_DEBUG
            printk(KERN_DEBUG"=========extend buff:%d=====\n",
                   HISDIO_COMM_REG_SEQ_GET(hi_sdio->sdio_extend->credit_info));
            oal_print_hex_dump((oal_void*)hi_sdio->sdio_extend, sizeof(struct hisdio_extend_func), 32, "extend :");

            /* 此credit更新只在调试时使用 */
            if (oal_sdio_credit_info_update(hi_sdio)) {
                if (OAL_LIKELY(hi_sdio->credit_update_cb))
                    hi_sdio->credit_update_cb(hi_sdio->bus_data);
            }
#endif
        } else {
            DECLARE_DFT_TRACE_KEY_INFO("[SDIO][Err]sdio read extend_buf fail", OAL_DFT_TRACE_FAIL);
        }
    }

    return ret;
}
#else
OAL_STATIC oal_int32 oal_sdio_extend_buf_get(struct oal_sdio *hi_sdio)
{
    oal_int32 ret;
    // if (sdio_extend_func)
    {
        memset_s(hi_sdio->sdio_extend, sizeof(struct hisdio_extend_func),
                 0, sizeof(struct hisdio_extend_func));
        ret = oal_sdio_memcpy_fromio(hi_sdio->func, (oal_void*)&hi_sdio->sdio_extend->credit_info,
                                     HISDIO_EXTEND_BASE_ADDR + 12, HISDIO_EXTEND_REG_COUNT + 4);
#ifdef CONFIG_SDIO_DEBUG
        if (ret == OAL_SUCC) {
            printk(KERN_DEBUG"=========extend buff:%d=====\n",
                   HISDIO_COMM_REG_SEQ_GET(hi_sdio->sdio_extend->credit_info));
            oal_print_hex_dump((oal_void*)hi_sdio->sdio_extend, sizeof(struct hisdio_extend_func), 32, "extend :");
        }
#endif
    }

    return ret;
}
#endif

oal_int32 oal_sdio_transfer_rx_reserved_buff(struct oal_sdio *hi_sdio)
{
    uint32_t  i;
    oal_int32 ret;
    uint32_t  left_size;
    oal_uint32 seg_nums, seg_size;
    struct scatterlist *sg = OAL_PTR_NULL, *sg_t = OAL_PTR_NULL;
    oal_uint32 ul_extend_len = hi_sdio->sdio_extend->xfer_count;

    if (ul_extend_len == 0) {
        DECLARE_DFT_TRACE_KEY_INFO("extend_len is zero", OAL_DFT_TRACE_EXCEP);
        return -OAL_EINVAL;
    }

    seg_size = hi_sdio->rx_reserved_buff_len;

    seg_nums = ((ul_extend_len - 1) / seg_size) + 1;
    if (hi_sdio->scatt_info[SDIO_READ].max_scatt_num < seg_nums) {
        DECLARE_DFT_TRACE_KEY_INFO("rx_seserved_scatt_fail", OAL_DFT_TRACE_EXCEP);
        OAL_IO_PRINT("sdio seg nums :%u large than rx scatt num %u\n", seg_nums,
                     hi_sdio->scatt_info[SDIO_READ].max_scatt_num);
        return -OAL_EINVAL;
    }

    OAL_IO_PRINT("drop the rx buff length:%u\n", ul_extend_len);

    sg = hi_sdio->scatt_info[SDIO_READ].sglist;
    if (sg == OAL_PTR_NULL) {
        OAL_IO_PRINT("oal_sdio_transfer_rx_reserved_buff::sg is null!\n");
        return -OAL_EINVAL;
    }
    sg_init_table(sg, seg_nums);
    left_size = ul_extend_len;
    for_each_sg(sg, sg_t, seg_nums, i)
    {
        if (sg_t == OAL_PTR_NULL) {
            OAL_IO_PRINT("oal_sdio_transfer_rx_reserved_buff::sg_t is null!\n");
            return -OAL_EINVAL;
        }
        sg_set_buf(sg_t, hi_sdio->rx_reserved_buff, OAL_MIN(seg_size, left_size));
        left_size = left_size - seg_size;
    }
    ret = _oal_sdio_transfer_scatt(hi_sdio, SDIO_READ, HISDIO_REG_FUNC1_FIFO, sg, seg_nums, ul_extend_len);
    if (OAL_UNLIKELY(ret)) {
        OAL_IO_PRINT("sdio trans revered mem failed! ret=%d\n", ret);
    }
    return ret;
}

#undef CONFIG_SDIO_RX_NETBUF_ALLOC_FAILED_DEBUG
#ifdef CONFIG_SDIO_RX_NETBUF_ALLOC_FAILED_DEBUG
oal_uint32 rx_alloc_netbuf_debug = 0;
module_param(rx_alloc_netbuf_debug, uint, S_IRUGO | S_IWUSR);
#endif

oal_netbuf_stru *oal_sdio_alloc_rx_netbuf(oal_uint32 ul_len)
{
#ifdef CONFIG_SDIO_RX_NETBUF_ALLOC_FAILED_DEBUG
    if (rx_alloc_netbuf_debug) {
        if (prandom_u32() % 256) {
            return NULL;
        }
    }
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    return __netdev_alloc_skb(NULL, ul_len, GFP_KERNEL);
#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)

#ifdef _PRE_LWIP_ZERO_COPY
    return oal_pbuf_netbuf_alloc(ul_len);
#else
    return dev_alloc_skb(ul_len);
#endif
#endif
}

oal_int32 oal_sdio_build_rx_netbuf_list(struct oal_sdio *hi_sdio,
                                        oal_netbuf_head_stru *head)
{
#ifdef CONFIG_SDIO_FUNC_EXTEND
    oal_int32 i;
    oal_uint8  buff_len;
    oal_uint16 buff_len_t;
#endif
    oal_int32 ret = OAL_SUCC;
    oal_uint32 sum_len = 0;
    oal_netbuf_stru *netbuf = NULL;

#if 0
    if (OAL_UNLIKELY(!sdio_extend_func)) {
        return -OAL_EFAIL;
    }
#endif

    /* always should be empty */
    OAL_BUG_ON(!oal_netbuf_list_empty(head));
#ifdef CONFIG_SDIO_FUNC_EXTEND
    for (i = 0; i < HISDIO_EXTEND_REG_COUNT; i++) {
        buff_len = hi_sdio->sdio_extend->comm_reg[i];
        if (buff_len == 0)
            break;

        buff_len_t = buff_len << HISDIO_D2H_SCATT_BUFFLEN_ALIGN_BITS;

        netbuf = oal_sdio_alloc_rx_netbuf(buff_len_t);
        if (netbuf == NULL) {
            DECLARE_DFT_TRACE_KEY_INFO("sdio_rx_no_mem", OAL_DFT_TRACE_OTHER);
            OAL_IO_PRINT("[WIFI][E]rx no mem:%u, index:%d\n", buff_len, i);
            goto failed_netbuf_alloc;
        }

        oal_netbuf_put(netbuf, buff_len_t);
        sum_len += buff_len_t;
        OAL_BUG_ON(!OAL_NETBUF_HEAD_NEXT(head));
        OAL_BUG_ON(!OAL_NETBUF_HEAD_PREV(head));
        __skb_queue_tail(head, netbuf);
    }

    if (OAL_WARN_ON(HISDIO_ALIGN_4_OR_BLK(sum_len) != hi_sdio->sdio_extend->xfer_count)) {
        DECLARE_DFT_TRACE_KEY_INFO("rx_scatt_len_not_match", OAL_DFT_TRACE_EXCEP);
        OAL_IO_PRINT("[WIFI][E]scatt total len[%u] should = xfercount[%u],after pad len:%u\n",
                     sum_len,
                     hi_sdio->sdio_extend->xfer_count,
                     HISDIO_ALIGN_4_OR_BLK(sum_len));

        hi_sdio->error_stat.rx_scatt_info_not_match++;
        goto failed_netbuf_alloc;
    }
#else
    netbuf = oal_sdio_alloc_rx_netbuf(hi_sdio->sdio_extend->xfer_count);
    if (netbuf == NULL) {
        OAL_IO_PRINT("rx no mem:%u\n", hi_sdio->sdio_extend->xfer_count);
        DECLARE_DFT_TRACE_KEY_INFO("sdio_rx_no_mem", OAL_DFT_TRACE_OTHER);
        goto failed_netbuf_alloc;
    }

    oal_netbuf_put(netbuf, hi_sdio->sdio_extend->xfer_count);
    sum_len += hi_sdio->sdio_extend->xfer_count;
    __skb_queue_tail(head, netbuf);
#endif

    if (OAL_UNLIKELY(oal_netbuf_list_empty(head))) {
#ifdef CONFIG_PRINTK
        printk("unvaild scatt info:xfercount:%u\n", hi_sdio->sdio_extend->xfer_count);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        print_hex_dump_bytes("scatt extend:", DUMP_PREFIX_ADDRESS,
                             hi_sdio->sdio_extend->comm_reg, HISDIO_EXTEND_REG_COUNT);
#endif
#endif
        return -OAL_EINVAL;
    }

    return ret;
failed_netbuf_alloc:
    skb_queue_purge(head);
    oal_sdio_transfer_rx_reserved_buff(hi_sdio);
    return -OAL_ENOMEM;
}

OAL_STATIC oal_int32 oal_sdio_get_func1_int_status(struct oal_sdio *hi_sdio, oal_uint8* int_stat)
{
    oal_int32 ret = 0;
#ifdef CONFIG_SDIO_MSG_ACK_HOST2ARM_DEBUG
    /* read interrupt indicator register */
    *int_stat = oal_sdio_readb(hi_sdio->func, HISDIO_REG_FUNC1_INT_STATUS, &ret);
    if (OAL_UNLIKELY(ret)) {
        OAL_IO_PRINT("[SDIO][Err]failed to read sdio func1 interrupt status!ret=%d\n", ret);
        return ret;
    }
    hi_sdio->sdio_extend->int_stat &= hi_sdio->func1_int_mask;
    hi_sdio->sdio_extend->int_stat = *int_stat;
#else
    if (sdio_extend_func) {
        hi_sdio->sdio_extend->int_stat &= hi_sdio->func1_int_mask;
        *int_stat = (hi_sdio->sdio_extend->int_stat & 0xF);
        return OAL_SUCC;
    } else {
        /* read interrupt indicator register */
        *int_stat = oal_sdio_readb(hi_sdio->func, HISDIO_REG_FUNC1_INT_STATUS, &ret);
        if (OAL_UNLIKELY(ret)) {
            OAL_IO_PRINT("[SDIO][Err]failed to read sdio func1 interrupt status!ret=%d\n", ret);
            return ret;
        }
        *int_stat = (*int_stat) & hi_sdio->func1_int_mask;
    }
#endif
    return OAL_SUCC;
}

OAL_STATIC oal_int32 oal_sdio_clear_int_status(struct oal_sdio *hi_sdio, oal_uint8 int_stat)
{
    oal_int32 ret = 0;
#ifdef CONFIG_SDIO_MSG_ACK_HOST2ARM_DEBUG
    /* clear interrupt mask */
    oal_sdio_writeb(hi_sdio->func, int_stat, HISDIO_REG_FUNC1_INT_STATUS, &ret);
    if (OAL_UNLIKELY(ret)) {
        OAL_IO_PRINT("[SDIO][Err]faild to clear sdio func1 interrupt!ret=%d\n", ret);
        return ret;
    }
#else

    if (sdio_extend_func) {
        return OAL_SUCC;
    }

    /* clear interrupt mask */
    oal_sdio_writeb(hi_sdio->func, int_stat, HISDIO_REG_FUNC1_INT_STATUS, &ret);
    if (OAL_UNLIKELY(ret)) {
        OAL_IO_PRINT("[SDIO][Err]faild to clear sdio func1 interrupt!ret=%d\n", ret);
        return ret;
    }
#endif
    return OAL_SUCC;
}

oal_int32 oal_sdio_do_isr(struct oal_sdio *hi_sdio)
{
    oal_uint8                   int_mask;
    oal_int32                   ret;
    struct sdio_func            *func = OAL_PTR_NULL;
    oal_int32                   rx_retry_count = SDIO_RX_RETRY;

    hi_sdio->sdio_int_count++;
    func       = hi_sdio->func;
    /* sdio bus state access lock by sdio bus claim locked. */
    while (1) {
        if (OAL_UNLIKELY(oal_sdio_get_state(hi_sdio, OAL_SDIO_RX) == OAL_TRUE)) {
            break;
        }
        rx_retry_count--;
        if (rx_retry_count == 0) {
            OAL_IO_PRINT("[SDIO][W][%s]sdio closed,state:%u\n", __FUNCTION__, oal_sdio_get_state(hi_sdio, OAL_SDIO_RX));
            return OAL_SUCC;
        }
        msleep(10);
    }

#ifndef CONFIG_SDIO_MSG_ACK_HOST2ARM_DEBUG
    ret = oal_sdio_extend_buf_get(hi_sdio);
    if (OAL_UNLIKELY(ret)) {
        OAL_IO_PRINT("[SDIO][Err]failed to read sdio extend area ret=%d\n", ret);
        return -OAL_EFAIL;
    }
#endif

    ret = oal_sdio_get_func1_int_status(hi_sdio, &int_mask);
    if (OAL_UNLIKELY(ret)) {
        return ret;
    }

    if (OAL_UNLIKELY((int_mask & HISDIO_FUNC1_INT_MASK) == 0)) {
        // OAL_IO_PRINT("no sdio interrupt occur[%u], unavailable %s interrupt\n",
        //            int_mask,
        hi_sdio->func1_stat.func1_no_int_count++;
        return OAL_SUCC;
    }

    /* clear interrupt mask */
    ret = oal_sdio_clear_int_status(hi_sdio, int_mask);
    if (OAL_UNLIKELY(ret)) {
        OAL_IO_PRINT("[SDIO][Err]sdio clear int status fail\n");
        return ret;
    }

    if (int_mask & HISDIO_FUNC1_INT_RERROR) {
        /* try to read the data again */
        OAL_IO_PRINT("[SDIO][Err]sdio func1 interrupt Error, try to read the data anyway\n");
        hi_sdio->func1_stat.func1_err_int_count++;
    }

    /* message interrupt, flow control */
    if (int_mask & HISDIO_FUNC1_INT_MFARM) {
        hi_sdio->func1_stat.func1_msg_int_count++;
        if (oal_sdio_msg_irq(hi_sdio) != OAL_SUCC) {
            return -OAL_EFAIL;
        }
    }

    if (int_mask & HISDIO_FUNC1_INT_DREADY) {
#ifdef CONFIG_SDIO_MSG_ACK_HOST2ARM_DEBUG
        ret = oal_sdio_extend_buf_get(hi_sdio);
        if (OAL_UNLIKELY(ret)) {
            OAL_IO_PRINT("[SDIO][Err]failed to read sdio extend area ret=%d\n", ret);
            return -OAL_EFAIL;
        }
#endif
        hi_sdio->func1_stat.func1_data_int_count++;
        return oal_sdio_data_sg_irq(hi_sdio);
    }
#ifdef CONFIG_SDIO_DEBUG
    OAL_IO_PRINT("[SDIO]succeed to handle sdio irq\n");
#endif
    hi_sdio->func1_stat.func1_unknow_int_count++;
    return OAL_SUCC;
}

oal_void oal_sdio_isr(struct sdio_func *func)
{
    struct oal_sdio     *hi_sdio = OAL_PTR_NULL;
    oal_int32                     ret;
    oal_uint32                  weight = SDIO_MAX_CONTINUS_RX_COUNT;

    OAL_BUG_ON(!func);

    hi_sdio = sdio_get_drvdata(func);
    if (hi_sdio == NULL) {
        OAL_IO_PRINT("oal_sdio is NULL\n");
        return;
    }

    do {
        oal_sdio_rx_transfer_lock(hi_sdio);
        sdio_claim_host(hi_sdio->func);
        ret = oal_sdio_do_isr(hi_sdio);
        if (OAL_UNLIKELY(ret)) {
            OAL_IO_PRINT("oal_sdio_do_isr fail\n");
            oal_exception_submit(TRANS_FAIL);
            chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                                 CHR_PLT_DRV_EVENT_SDIO, CHR_PLAT_DRV_ERROR_SDIO_INIT_ISR);
        }
        sdio_release_host(hi_sdio->func);
        oal_sdio_rx_transfer_unlock(hi_sdio);
    } while ((ret == OAL_SUCC) && (--weight > 0));
}

#undef COFNIG_TEST_SDIO_INT_LOSS

OAL_STATIC oal_int32 oal_gpio_rxdata_thread(oal_void *data)
{
#ifdef COFNIG_TEST_SDIO_INT_LOSS
    int i;
#endif
    oal_int32              ret;
    struct oal_sdio        *hi_sdio;
    struct sdio_func       *func = OAL_PTR_NULL;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_kthread_param_stru st_thread_param = {0};
#endif
    hi_sdio = (struct oal_sdio *)data;

    OAL_BUG_ON(!hi_sdio);

    func        = hi_sdio->func;

    PS_PRINT_INFO("hisi wifi sched ctrl rx thread high speed\n");

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    st_thread_param.l_prio   = HISDIO_RX_THREAD_PRIORITY;
    st_thread_param.l_nice   = HISDIO_RX_THREAD_NICE;
    st_thread_param.l_policy = HISDIO_SDIO_RX_THREAD_POLICY;
    oal_set_thread_property(OAL_CURRENT, &st_thread_param);
    allow_signal(SIGTERM);
#endif

    while (OAL_LIKELY(!oal_down_interruptible(&hi_sdio->gpio_rx_sema))) {
        if (OAL_UNLIKELY(oal_kthread_should_stop())) {
            OAL_IO_PRINT("exit gpio rx thread\n");
            break;
        }
#ifdef COFNIG_TEST_SDIO_INT_LOSS
        for (i = 0; i < 5; i++) {
#endif
            /* start to read GPIO interrupt */
            oal_sdio_rx_transfer_lock(hi_sdio);
            sdio_claim_host(func);
            oal_wake_lock(&hi_sdio->st_sdio_rx_wakelock);
            ret = oal_sdio_do_isr(hi_sdio);
            oal_wake_unlock(&hi_sdio->st_sdio_rx_wakelock);
            if (OAL_UNLIKELY(ret)) {
                OAL_IO_PRINT("oal_sdio_do_isr fail\n");
                oal_exception_submit(TRANS_FAIL);
                chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                                     CHR_PLT_DRV_EVENT_SDIO, CHR_PLAT_DRV_ERROR_SDIO_INIT_RX_DATA_PROC);
            }
            hi_sdio->data_int_finish_count++;
            sdio_release_host(hi_sdio->func);
            oal_sdio_rx_transfer_unlock(hi_sdio);
#ifdef COFNIG_TEST_SDIO_INT_LOSS
        }
#endif
    }

    OAL_IO_PRINT("gpio rx data is terminated\n");
    return OAL_SUCC;
}

oal_atomic g_wakeup_dev_wait_ack;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_uint32 g_ul_pm_wakeup_event = OAL_FALSE;
EXPORT_SYMBOL_GPL(g_ul_pm_wakeup_event);
#endif
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
oal_atomic g_pm_spinlock_get;
#endif

irqreturn_t wlan_gpio_irq(oal_int32 irq, oal_void *dev_id)
{
    struct oal_sdio *hi_sdio;
    oal_ulong                ul_state;
    oal_ulong                ul_sdio_data_gpio_state;
    oal_ulong                ul_wkup_gpio_state;
    hi_sdio = (struct oal_sdio *)dev_id;

    /* 进入中断后先缓存唤醒脚和数据中断脚的中断状态并清除中断标志 */
    ul_wkup_gpio_state = board_get_and_clear_wlan_wkup_gpio_intr_state();
    ul_sdio_data_gpio_state = board_get_and_clear_wlan_sdio_gpio_intr_state();
    if (hi_sdio == NULL) {
        OAL_IO_PRINT(KERN_ERR"GPIO interrupt function param is NULL\r\n");
        return IRQ_HANDLED;
    }

    if (!hi_sdio->pst_pm_callback || !hi_sdio->pst_pm_callback->wlan_pm_state_get) {
        OAL_IO_PRINT("GPIO interrupt function pm param is NULL\r\n");
        return IRQ_HANDLED;
    }

    hi_sdio->gpio_int_count++;

    ul_state = hi_sdio->pst_pm_callback->wlan_pm_state_get();
    if (ul_state == HOST_DISALLOW_TO_SLEEP) {
        if (ul_sdio_data_gpio_state) {
            /* 0==HOST_DISALLOW_TO_SLEEP表示不允许休眠 */
            hi_sdio->data_int_count++;

            oal_up(&hi_sdio->gpio_rx_sema);
        }
    } else if (ul_state == HOST_ALLOW_TO_SLEEP) {
        if (ul_wkup_gpio_state) {
            OAL_BUG_ON(!hi_sdio->pst_pm_callback->wlan_pm_wakeup_host);
            hi_sdio->wakeup_int_count++;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
            g_ul_pm_wakeup_event = OAL_TRUE;
#endif
            hi_sdio->pst_pm_callback->wlan_pm_wakeup_host();
        }
    } else {
        OAL_IO_PRINT("invalid gpio irq\r\n");
    }
    return IRQ_HANDLED;
}

oal_int32 oal_register_gpio_intr(struct oal_sdio *hi_sdio)
{
    oal_int32 ret;
    oal_kthread_param_stru st_thread_param = {0};
    unsigned int wlan_irq;
    BOARD_INFO *pst_board = get_board_info();
    if (pst_board == OAL_PTR_NULL) {
        OAL_IO_PRINT("[SDIO][ERROR]board info is NULL!\n");
        return -OAL_EFAIL;
    }

    wlan_irq = pst_board->wlan_irq;

    PS_PRINT_INFO("wlan gpio irq = %u\n", wlan_irq);
    if (!wlan_irq) {
        OAL_IO_PRINT("[SDIO][ERROR]wlan irq should't be 0\n");
        return -OAL_EFAIL;
    }

    oal_spin_lock_init(&hi_sdio->st_irq_lock);
    hi_sdio->ul_wlan_irq = pst_board->wlan_irq;

    st_thread_param.l_cpuid      = NOT_BIND_CPU;
    st_thread_param.l_policy     = HISDIO_SDIO_RX_THREAD_POLICY;
    st_thread_param.l_prio       = HISDIO_GPIO_RX_THREAD_PRIORITY;
    st_thread_param.ul_stacksize = HISDIO_RX_THREAD_STACKSIZE;

    oal_sema_init(&hi_sdio->gpio_rx_sema, 0);
    /* create thread for gpio rx data in interrupt handler */
    hi_sdio->gpio_rx_tsk = oal_kthread_create("oal_gpio_rx_data", oal_gpio_rxdata_thread,
                                              (void *)hi_sdio, &st_thread_param);
    if (hi_sdio->gpio_rx_tsk == NULL) {
        OAL_IO_PRINT("[SDIO][ERROR]failed to create rx data thread\n");
        return -OAL_EFAIL;
    }

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_atomic_set(&g_wakeup_dev_wait_ack, 0);
#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    oal_atomic_set(&g_pm_spinlock_get, 0);
#endif

    ret = oal_request_irq(wlan_irq, wlan_gpio_irq, IRQF_NO_SUSPEND | IRQF_TRIGGER_RISING | IRQF_DISABLED,
                          "wifi_gpio_intr", hi_sdio);
    if (ret < 0) {
        OAL_IO_PRINT("failed to request wlan irq\n");
        oal_msleep(100);
        oal_kthread_stop(hi_sdio->gpio_rx_tsk);
        hi_sdio->gpio_rx_tsk = NULL;
        return ret;
    }
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_wlan_gpio_intr_enable(hi_sdio, OAL_FALSE);  /* 1131C-debug */
#endif

    PS_PRINT_INFO("success to register sdio gpio intr\n");
    return OAL_SUCC;
}

oal_void oal_unregister_gpio_intr(struct oal_sdio *hi_sdio)
{
    /* disable wlan irq */
    oal_wlan_gpio_intr_enable(hi_sdio, OAL_FALSE);

    /* free irq when sdio driver deinit */
    oal_free_irq(hi_sdio->ul_wlan_irq, hi_sdio);
    oal_kthread_stop(hi_sdio->gpio_rx_tsk);
    hi_sdio->gpio_rx_tsk = NULL;
}

oal_void oal_wlan_gpio_intr_enable(struct oal_sdio *hi_sdio, oal_uint32  ul_en)
{
#ifndef _PRE_FEATURE_NO_GPIO
    oal_ulong            flags;

    oal_spin_lock_irq_save(&hi_sdio->st_irq_lock, &flags);
    if (ul_en) {
        oal_enable_irq(hi_sdio->ul_wlan_irq);
    } else {
        oal_disable_irq_nosync(hi_sdio->ul_wlan_irq);
    }
    oal_spin_unlock_irq_restore(&hi_sdio->st_irq_lock, &flags);
#endif
}

oal_int32 oal_register_sdio_intr(struct oal_sdio *hi_sdio)
{
    oal_int32 ret;

    sdio_claim_host(hi_sdio->func);
    /* use sdio bus line data1 for sdio data interrupt */
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    ret = sdio_claim_irq(hi_sdio->func, oal_sdio_isr);
#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    ret = sdio_require_irq(hi_sdio->func, oal_sdio_isr);
#endif
    if (ret < 0) {
        OAL_IO_PRINT("failed to register sdio interrupt\n");
        sdio_release_host(hi_sdio->func);
        return -OAL_EFAIL;
    }
    sdio_release_host(hi_sdio->func);
    OAL_IO_PRINT("sdio interrupt register!\n");
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    pm_runtime_get_sync(mmc_dev(hi_sdio->func->card->host));
#endif
    return ret;
}

oal_void oal_unregister_sdio_intr(struct oal_sdio *hi_sdio)
{
    sdio_claim_host(hi_sdio->func);
    /* use sdio bus line data1 for sdio data interrupt */
    sdio_release_irq(hi_sdio->func);
    sdio_release_host(hi_sdio->func);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    pm_runtime_put_sync(mmc_dev(hi_sdio->func->card->host));
#endif
}

oal_int32 oal_sdio_interrupt_register(struct oal_sdio *hi_sdio)
{
#ifndef _PRE_FEATURE_NO_GPIO
    oal_int32 ret;

    if (g_hisdio_intr_mode) {
        /* use gpio interrupt for sdio data interrupt */
        ret = oal_register_gpio_intr(hi_sdio);
        if (ret < 0) {
            OAL_IO_PRINT("failed to register gpio interrupt\n");
            return ret;
        }
    } else {
        /* use sdio interrupt for sdio data interrupt */
        ret = oal_register_sdio_intr(hi_sdio);
        if (ret < 0) {
            OAL_IO_PRINT("failed to register sdio interrupt\n");
            return ret;
        }
    }
#endif
    return OAL_SUCC;
}

oal_void oal_sdio_interrupt_unregister(struct oal_sdio *hi_sdio)
{
    if (g_hisdio_intr_mode) {
        /* use GPIO interrupt for sdio data interrupt */
        oal_unregister_gpio_intr(hi_sdio);
    } else {
        /* use sdio interrupt for sdio data interrupt */
        oal_unregister_sdio_intr(hi_sdio);
    }
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_ulong oal_sdio_get_sleep_state(struct oal_sdio *hi_sdio)
{
    int    ret;
    oal_ulong ul_value;

    sdio_claim_host(hi_sdio->func);
    ul_value = sdio_f0_readb(hi_sdio->func, HISDIO_WAKEUP_DEV_REG, &ret);
    sdio_release_host(hi_sdio->func);

    return ul_value;
}
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_void oal_sdio_get_dev_pm_state(struct oal_sdio *hi_sdio, oal_ulong *pst_ul_f1,
                                   oal_ulong *pst_ul_f2, oal_ulong *pst_ul_f3, oal_ulong *pst_ul_f4)
{
    int    ret;

    sdio_claim_host(hi_sdio->func);
    *pst_ul_f1 = sdio_f0_readb(hi_sdio->func, 0xf1, &ret);
    *pst_ul_f2 = sdio_f0_readb(hi_sdio->func, 0xf2, &ret);
    *pst_ul_f3 = sdio_f0_readb(hi_sdio->func, 0xf3, &ret);
    *pst_ul_f4 = sdio_f0_readb(hi_sdio->func, 0xf4, &ret);
    sdio_release_host(hi_sdio->func);

    return ;
}
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_int32 oal_sdio_wakeup_dev(struct oal_sdio *hi_sdio)
{
    int    ret;

    oal_sdio_claim_host(hi_sdio);
    sdio_f0_writeb(hi_sdio->func, DISALLOW_TO_SLEEP_VALUE, HISDIO_WAKEUP_DEV_REG, &ret);
    oal_sdio_release_host(hi_sdio);

    return ret;
}
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_int32 oal_sdio_sleep_dev(struct oal_sdio *hi_sdio)
{
    int    ret;

    oal_sdio_claim_host(hi_sdio);
    sdio_f0_writeb(hi_sdio->func, ALLOW_TO_SLEEP_VALUE, HISDIO_WAKEUP_DEV_REG, &ret);
    oal_sdio_release_host(hi_sdio);

    return ret;
}
#endif

oal_int32 oal_sdio_dev_init(struct oal_sdio *hi_sdio)
{
    struct sdio_func   *func = OAL_PTR_NULL;
    oal_int32               ret;

    OAL_BUG_ON(!hi_sdio);

    func = hi_sdio->func;

    oal_sdio_claim_host(hi_sdio);
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    oal_disable_sdio_state(hi_sdio, OAL_SDIO_ALL);
#endif
    SDIO_EN_TIMEOUT(func)  = 1000;

    ret = sdio_enable_func(func);
    if (ret < 0) {
        OAL_IO_PRINT("failed to enable sdio function! ret=%d\n", ret);
        goto failed_enabe_func;
    }

    ret = sdio_set_block_size(func, HISDIO_BLOCK_SIZE);
    if (ret) {
        OAL_IO_PRINT("failed to set sdio blk size! ret=%d\n", ret);
        goto failed_set_block_size;
    }

    /* before enable sdio function 1, clear its interrupt flag, no matter it exist or not */
    oal_sdio_writeb(func, HISDIO_FUNC1_INT_MASK, HISDIO_REG_FUNC1_INT_STATUS, &ret);
    if (ret) {
        OAL_IO_PRINT("failed to clear sdio interrupt! ret=%d\n", ret);
        goto failed_clear_func1_int;
    }
    /*
     * enable four interrupt sources in function 1:
     *      data ready for host to read
     *      read data error
     *      message from arm is available
     *      device has receive message from host
     * */
    oal_sdio_writeb(func, HISDIO_FUNC1_INT_MASK, HISDIO_REG_FUNC1_INT_ENABLE, &ret);
    if (ret < 0) {
        OAL_IO_PRINT("failed to enable sdio interrupt! ret=%d\n", ret);
        goto failed_enable_func1;
    }

    oal_enable_sdio_state(hi_sdio, OAL_SDIO_ALL);
    oal_sdio_release_host(hi_sdio);

    PS_PRINT_INFO("sdio function %d enabled.\n", SDIO_FUNC_NUM(func));

    return OAL_SUCC;
failed_enable_func1:
failed_clear_func1_int:
failed_set_block_size:
    sdio_disable_func(func);
failed_enabe_func:
    oal_sdio_release_host(hi_sdio);
    return ret;
}

OAL_STATIC oal_void oal_sdio_dev_deinit(struct oal_sdio *hi_sdio)
{
    struct sdio_func   *func = OAL_PTR_NULL;
    oal_int32           ret = 0;

    OAL_BUG_ON(!hi_sdio);
    OAL_BUG_ON(!hi_sdio->func);

    func  = hi_sdio->func;

    sdio_claim_host(func);
    oal_sdio_writeb(func, 0, HISDIO_REG_FUNC1_INT_ENABLE, &ret);
    oal_sdio_interrupt_unregister(hi_sdio);
    sdio_disable_func(func);
    oal_disable_sdio_state(hi_sdio, OAL_SDIO_ALL);
    sdio_release_host(func);

    OAL_IO_PRINT("oal_sdio_dev_deinit! \n");
}

oal_int32 oal_sdio_get_state(struct oal_sdio *hi_sdio, oal_uint32 mask)
{
    OAL_BUG_ON(!hi_sdio);
    if ((hi_sdio->state & mask) == mask) {
        return OAL_TRUE;
    } else {
        return OAL_FALSE;
    }
}

oal_void oal_enable_sdio_state(struct oal_sdio *hi_sdio, oal_uint32 mask)
{
    oal_uint32 old_state;

    OAL_BUG_ON(!hi_sdio);
    oal_sdio_claim_host(hi_sdio);
    old_state = hi_sdio->state;
    hi_sdio->state |= mask;
    oal_sdio_print_state(old_state, hi_sdio->state);
    oal_sdio_release_host(hi_sdio);
}

oal_void oal_disable_sdio_state(struct oal_sdio *hi_sdio, oal_uint32 mask)
{
    oal_uint32 old_state;

    OAL_BUG_ON(!hi_sdio);
    oal_sdio_claim_host(hi_sdio);
    old_state = hi_sdio->state;
    hi_sdio->state &= ~mask;
    oal_sdio_print_state(old_state, hi_sdio->state);
    oal_sdio_release_host(hi_sdio);
}

struct oal_sdio* oal_sdio_alloc(struct sdio_func *func)
{
    struct oal_sdio     *hi_sdio = OAL_PTR_NULL;

    OAL_BUG_ON(!func);

    /* alloce sdio control struct */
    hi_sdio = oal_get_sdio_default_handler();
    if (hi_sdio == NULL) {
        OAL_IO_PRINT(KERN_ERR"Failed to alloc hi_sdio!\n");
        return NULL;
    }

    hi_sdio->func           = func;

    OAL_MUTEX_INIT(&hi_sdio->rx_transfer_lock);

    /* func keep a pointer to oal_sdio */
    sdio_set_drvdata(func, hi_sdio);

    return hi_sdio;
}

OAL_STATIC oal_void oal_sdio_free(struct oal_sdio *hi_sdio)
{
    if (hi_sdio == NULL) {
        return;
    }
    OAL_MUTEX_DESTROY(&hi_sdio->rx_transfer_lock);
    oal_free_sdio_stru(hi_sdio);
}

OAL_STATIC oal_int32 oal_sdio_probe(struct sdio_func *func, const struct sdio_device_id *ids)
{
    struct oal_sdio     *hi_sdio = OAL_PTR_NULL;
    oal_int32           ret;
    uint32_t            size;

    OAL_BUG_ON(!func);
    OAL_BUG_ON(!ids);

    PS_PRINT_INFO(KERN_ERR"sdio function[%d] match\n", SDIO_FUNC_NUM(func));

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    p_gst_sdio_func = func;
#endif
    /* alloce sdio control struct */
    hi_sdio = oal_sdio_alloc(func);
    if (hi_sdio == NULL) {
        sdio_enum_err_str = "failed to alloc hi_sdio!";
        OAL_IO_PRINT(KERN_ERR"%s\n", sdio_enum_err_str);
        goto failed_sdio_alloc;
    }

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    /* register interrupt process function */
    ret = oal_sdio_interrupt_register(hi_sdio);
    if (ret < 0) {
        sdio_enum_err_str = "failed to register sdio interrupt";
        OAL_IO_PRINT("%s\n", sdio_enum_err_str);
        printf("failed to register sdio interrupt\n");
        goto failed_sdio_int_reg;
    }
#endif

    oal_disable_sdio_state(hi_sdio, OAL_SDIO_ALL);

    if (oal_sdio_dev_init(hi_sdio) != OAL_SUCC) {
        sdio_enum_err_str = "sdio dev init failed";
        goto failed_sdio_dev_init;
    }

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    /* Print the sdio's cap */
    OAL_IO_PRINT("max_segs:%u, max_blk_size:%u,max_blk_count:%u,,max_seg_size:%u,max_req_size:%u\n",
                 SDIO_GET_MAX_SEGS(func), SDIO_GET_MAX_BLK_SIZE(func),
                 SDIO_GET_MAX_BLOCK_COUNT(func), SDIO_GET_MAX_SEG_SIZE(func),
                 SDIO_GET_MAX_REQ_SIZE(func));
#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    PS_PRINT_INFO("max_blk_size:%u,max_blk_count:%u,max_req_size:%u\n",
                  SDIO_GET_MAX_BLK_SIZE(func), SDIO_GET_MAX_BLOCK_COUNT(func), SDIO_GET_MAX_REQ_SIZE(func));
#endif
    size = oal_sdio_func_max_req_size(hi_sdio);
    PS_PRINT_INFO("transer limit size:%u\n", size);

    PS_PRINT_INFO("+++++++++++++func->enable_timeout= [%d]++++++++++++++++\n", SDIO_EN_TIMEOUT(func));

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    /* register interrupt process function */
    ret = oal_sdio_interrupt_register(hi_sdio);
    if (ret < 0) {
        sdio_enum_err_str = "failed to register sdio interrupt";
        OAL_IO_PRINT("%s\n", sdio_enum_err_str);
        goto failed_sdio_int_reg;
    }
#endif

    oal_wake_lock_init(&hi_sdio->st_sdio_wakelock, "wlan_sdio_lock");
    oal_wake_lock_init(&hi_sdio->st_sdio_rx_wakelock, "wlan_sdio_rx_lock");

    oal_sema_init(&chan_wake_sema, 1);

    OAL_COMPLETE(&sdio_driver_complete);

    return OAL_SUCC;
failed_sdio_int_reg:
failed_sdio_dev_init:
    oal_sdio_free(hi_sdio);
failed_sdio_alloc:
    chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                         CHR_PLT_DRV_EVENT_SDIO, CHR_PLAT_DRV_ERROR_SDIO_INIT_PROB_FAIL);
    return -OAL_EFAIL;
}

oal_void oal_sdio_wakelocks_release_detect(struct oal_sdio *pst_hi_sdio)
{
    /* before call this function , please make sure the rx/tx queue is empty and no data transfer!! */
    OAL_BUG_ON(!pst_hi_sdio);
    if (oal_sdio_wakelock_active(pst_hi_sdio)) {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        OAL_IO_PRINT("[E]We still hold %s   %lu wake locks, Now release all",
                     pst_hi_sdio->st_sdio_wakelock.st_wakelock.ws.name,
                     pst_hi_sdio->st_sdio_wakelock.lock_count);
#endif
        DECLARE_DFT_TRACE_KEY_INFO("wlan_wakelock_error_hold", OAL_DFT_TRACE_EXCEP);
        pst_hi_sdio->st_sdio_wakelock.lock_count = 1;
        oal_sdio_wake_unlock(pst_hi_sdio);
    }
}

OAL_STATIC oal_int32 oal_sdio_single_transfer(struct oal_sdio *hi_sdio, oal_int32 rw,
                                              oal_void *buf, oal_uint32 size)
{
    OAL_BUG_ON(!hi_sdio);
    OAL_BUG_ON(!hi_sdio->func);
    OAL_BUG_ON(!buf);
    OAL_BUG_ON((uintptr_t)buf & 0x3);

    return oal_sdio_rw_buf(hi_sdio, rw, HISDIO_REG_FUNC1_FIFO, buf, size);
}

oal_int32 oal_sdio_transfer_tx(struct oal_sdio *hi_sdio, oal_netbuf_stru* netbuf)
{
    oal_int32 ret;
    uint32_t  tailroom, tailroom_add;

    tailroom  = HISDIO_ALIGN_4_OR_BLK(OAL_NETBUF_LEN(netbuf))
                           - OAL_NETBUF_LEN(netbuf);
    if (tailroom > oal_netbuf_tailroom(netbuf)) {
        tailroom_add = tailroom - oal_netbuf_tailroom(netbuf);
        /* relloc the netbuf */
        ret = oal_netbuf_expand_head(netbuf, 0, tailroom_add, GFP_ATOMIC);
        if (OAL_UNLIKELY(ret != OAL_SUCC)) {
            OAL_IO_PRINT("alloc tail room failed\n");
            return -OAL_EFAIL;
        }
    }

    oal_netbuf_put(netbuf, tailroom);

    return oal_sdio_single_transfer(hi_sdio, SDIO_WRITE, OAL_NETBUF_DATA(netbuf), OAL_NETBUF_LEN(netbuf));
}
oal_module_symbol(oal_sdio_transfer_tx);

oal_void check_sg_format(struct scatterlist *sg, oal_uint32 sg_len)
{
    uint32_t i;
    struct scatterlist *sg_t = OAL_PTR_NULL;
    for_each_sg(sg, sg_t, sg_len, i)
    {
        if (OAL_UNLIKELY(sg_t == NULL)) {
            return;
        }
        if (OAL_WARN_ON(((uintptr_t)sg_virt(sg_t) & 0x03) || (sg_t->length & 0x03))) {
            OAL_IO_PRINT("check_sg_format:[i:%u][addr:%p][len:%u]\n", i, sg_virt(sg_t), sg_t->length);
        }
    }
}

oal_void dump_sg_format(struct scatterlist *sg, oal_uint32 sg_len)
{
    uint32_t i;
    struct scatterlist *sg_t = OAL_PTR_NULL;
    OAL_IO_PRINT("sg dump nums:%d\n", sg_len);
    if (sg == OAL_PTR_NULL) {
        OAL_IO_PRINT("dump_sg_format::sg is null!\n");
        return;
    }
    for_each_sg(sg, sg_t, sg_len, i)
    {
        if (sg_t == OAL_PTR_NULL) {
            OAL_IO_PRINT("dump_sg_format::sg_t is null!\n");
            return;
        }
        OAL_IO_PRINT("sg descr:%3u,addr:%p,len:%6d\n", i, sg_virt(sg_t), sg_t->length);
    }
}

#ifdef CONFIG_HISDIO_H2D_SCATT_LIST_ASSEMBLE
oal_int32 oal_sdio_tx_scatt_list_merge(struct oal_sdio *hi_sdio,
                                       struct scatterlist *sg,
                                       oal_uint32 sg_len,
                                       oal_uint32 rw_sz)
{
    uint32_t  i;
    uint32_t offset = 0;
    errno_t   l_ret;
    uint32_t left_size, nents;
    struct scatterlist *sg_t = OAL_PTR_NULL;
    oal_uint8* pst_scatt_buff = (oal_uint8*)hi_sdio->scatt_buff.buff;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_uint32 seg_size = SDIO_GET_MAX_SEG_SIZE(hi_sdio->func);
#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    oal_uint32 seg_size = SDIO_GET_MAX_BLK_SIZE(hi_sdio->func);
#endif

    if (OAL_UNLIKELY(rw_sz > hi_sdio->scatt_buff.len)) {
        OAL_IO_PRINT("[E]sdio tx request %u bytes,scatt buf had %u,failed!\n", rw_sz, hi_sdio->scatt_buff.len);
        OAL_WARN_ON(1);
        return -OAL_ENOMEM;
    }

    if (sg == OAL_PTR_NULL) {
        OAL_IO_PRINT("oal_sdio_tx_scatt_list_merge::sg is null!\n");
        return -OAL_EINVAL;
    }
    for_each_sg(sg, sg_t, sg_len, i)
    {
        if (sg_t == OAL_PTR_NULL) {
            OAL_IO_PRINT("oal_sdio_tx_scatt_list_merge::sg_t is null!\n");
            return -OAL_EINVAL;
        }
        l_ret = memcpy_s(pst_scatt_buff + offset, hi_sdio->scatt_buff.len - offset, sg_virt(sg_t), sg_t->length);
        if (l_ret != EOK) {
            OAL_IO_PRINT("oal_sdio_tx_scatt_list_merge: memcpy_s failed!\n");
            return OAL_EINVAL;
        }
        offset += sg_t->length;
    }

    if (OAL_UNLIKELY(offset > rw_sz)) {
        OAL_IO_PRINT("[E]%s offset:%u > rw_sz:%u!\n", __FUNCTION__, offset, rw_sz);
        OAL_WARN_ON(1);
        return -OAL_EINVAL;
    }

    left_size = offset;
    /* reset the sg list! */
    nents = ((left_size - 1) / seg_size) + 1;
    if (OAL_UNLIKELY(nents > (oal_int32)sg_len)) {
        OAL_IO_PRINT("[E]%s merged scatt list num %d > sg_len:%u,max seg size:%u\n",
                     __FUNCTION__, nents, sg_len, seg_size);
        OAL_WARN_ON(1);
        return -OAL_ENOMEM;
    }

    sg_init_table(sg, nents);
    for_each_sg(sg, sg_t, nents, i) {
        if (sg_t == OAL_PTR_NULL) {
            OAL_IO_PRINT("oal_sdio_tx_scatt_list_merge::sg_t is null!\n");
            return -OAL_EINVAL;
        }
        sg_set_buf(sg_t, pst_scatt_buff + (oal_ulong)i * seg_size, OAL_MIN(seg_size, left_size));
        left_size = left_size - seg_size;
    }

    return nents;
}
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_int32 oal_mmc_io_rw_scat_extended(
                            struct oal_sdio *hi_sdio,
                            oal_int32 write,
                            oal_uint32 fn,
                            oal_uint32 addr, oal_int32 incr_addr,
                            struct scatterlist *sg, oal_uint32 sg_len,
                            oal_uint32 blocks, oal_uint32 blksz)
{
    struct mmc_request mrq = {0};
    struct mmc_command cmd = {0};
    struct mmc_data data = {0};
    struct mmc_card *card = OAL_PTR_NULL;

    OAL_BUG_ON(!hi_sdio);
    OAL_BUG_ON(!sg);
    OAL_BUG_ON(sg_len == 0);
    OAL_BUG_ON(fn > 7);

    if (OAL_WARN_ON(blksz == 0))
        return -EINVAL;

    /* sanity check */
    if (OAL_UNLIKELY(addr & ~0x1FFFF))
        return -EINVAL;

    card = hi_sdio->func->card;

    /* sg format */
// #ifdef CONFIG_SDIO_DEBUG
    check_sg_format(sg, sg_len);
// #endif
#ifdef CONFIG_HISDIO_H2D_SCATT_LIST_ASSEMBLE
#ifndef LITEOS_IPC_CODE
    if (write) {
        /* copy the buffs ,align to SDIO_BLOCK
        Fix the sdio host ip fifo depth issue temporarily */
        oal_int32 ret = oal_sdio_tx_scatt_list_merge(hi_sdio, sg, sg_len, blocks * blksz);
        if (OAL_LIKELY(ret > 0)) {
            sg_len = ret;
        } else {
            return ret;
        }
    }
#endif
#endif

    mrq.cmd = &cmd;
    mrq.data = &data;

    cmd.opcode = SD_IO_RW_EXTENDED;
    cmd.arg = write ? 0x80000000 : 0x00000000;
    cmd.arg |= fn << 28;
    cmd.arg |= (uint32_t)incr_addr ? 0x04000000u : 0x00000000u;
    cmd.arg |= addr << 9;
    if (blocks == 1 && blksz <= 512)
        cmd.arg |= (blksz == 512) ? 0 : blksz;  /* byte mode */
    else
        cmd.arg |= 0x08000000 | blocks;     /* block mode */
    cmd.flags = MMC_RSP_SPI_R5 | MMC_RSP_R5 | MMC_CMD_ADTC;

    data.blksz = blksz;
    data.blocks = blocks;
    data.flags = write ? MMC_DATA_WRITE : MMC_DATA_READ;
    data.sg = sg;
    data.sg_len = sg_len;
#ifdef CONFIG_SDIO_DEBUG
    OAL_IO_PRINT("[blksz:%u][blocks:%u][sg_len:%u][mode:%s]\n", blksz, blocks, sg_len, write ? "write" : "read");
    OAL_IO_PRINT("%s : [cmd opcode:%d][cmd arg:0x%8x][cmd flags: 0x%8x]\n",
                 mmc_hostname(card->host), cmd.opcode, cmd.arg, cmd.flags);
    OAL_IO_PRINT("Sdio %s data transfer start\n", write ? "write" : "read");
#endif

    mmc_set_data_timeout(&data, card);

    mmc_wait_for_req(card->host, &mrq);

#ifdef CONFIG_SDIO_DEBUG
    OAL_IO_PRINT("wait for %s tranfer over.\n", write ? "write" : "read");
#endif
    if (cmd.error)
        return cmd.error;
    if (data.error)
        return data.error;
    if (OAL_WARN_ON((int32_t)mmc_host_is_spi(card->host))) {
        OAL_IO_PRINT("HiSi WiFi  driver do not support spi sg transfer!\n");
        return -EIO;
    }
    if (cmd.resp[0] & R5_ERROR)
        return -EIO;
    if (cmd.resp[0] & R5_FUNCTION_NUMBER)
        return -EINVAL;
    if (cmd.resp[0] & R5_OUT_OF_RANGE)
        return -ERANGE;
#ifdef CONFIG_SDIO_DEBUG
    do {
        int i;
        struct scatterlist *sg_t;
        for_each_sg(data.sg, sg_t, data.sg_len, i)
        {
            printk(KERN_DEBUG"======netbuf pkts %d, len:%d=========\n", i, sg_t->length);
            oal_print_hex_dump(sg_virt(sg_t), sg_t->length, 32, "sg buf  :");
        }
    }while (0);
    OAL_IO_PRINT("Transfer done. %s sucuess!\n", write ? "write" : "read");
#endif
    return 0;
}
oal_module_symbol(oal_mmc_io_rw_scat_extended);
#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)

#ifndef LITEOS_IPC_CODE
oal_int32 oal_mmc_io_rw_scat_extended(
                            struct oal_sdio *hi_sdio,
                            oal_int32 write,
                            oal_uint32 fn,
                            oal_uint32 addr, oal_int32 incr_addr,
                            struct scatterlist *sg, oal_uint32 sg_len,
                            oal_uint32 blocks, oal_uint32 blksz)
{
    struct mmc_request mrq = {0};
    struct mmc_cmd cmd = {0};
    struct mmc_data data = {0};
    struct mmc_card *card = OAL_PTR_NULL;

    OAL_BUG_ON(!hi_sdio);
    OAL_BUG_ON(!sg);
    OAL_BUG_ON(sg_len == 0);
    OAL_BUG_ON(fn > 7);

    if (OAL_WARN_ON(blksz == 0))
        return -EINVAL;

    /* sanity check */
    if (OAL_UNLIKELY(addr & ~0x1FFFF))
        return -EINVAL;

    card = hi_sdio->func->card;

    /* sg format */
// #ifdef CONFIG_SDIO_DEBUG
    check_sg_format(sg, sg_len);
// #endif
#ifdef CONFIG_HISDIO_H2D_SCATT_LIST_ASSEMBLE
    if (write) {
        /* copy the buffs ,align to SDIO_BLOCK
        Fix the sdio host ip fifo depth issue temporarily */
        oal_int32 ret = oal_sdio_tx_scatt_list_merge(hi_sdio, sg, sg_len, blocks * blksz);
        if (OAL_LIKELY(ret > 0)) {
            sg_len = ret;
        } else {
            return ret;
        }
    }
#endif

    mrq.cmd = &cmd;
    mrq.data = &data;

    cmd.cmd_code = SDIO_RW_EXTENDED;
    cmd.arg = write ? 0x80000000 : 0x00000000;
    cmd.arg |= fn << 28;
    cmd.arg |= incr_addr ? 0x04000000 : 0x00000000;
    cmd.arg |= addr << 9;
    if (blocks == 1 && blksz <= 512)
        cmd.arg |= (blksz == 512) ? 0 : blksz;  /* byte mode */
    else
        cmd.arg |= 0x08000000 | blocks;     /* block mode */
    cmd.resp_type = MMC_RESP_SPI_R5 | MMC_RESP_R5 | MMC_CMD_ADTC;

    data.blocksz = blksz;
    data.blocks = blocks;
    data.data_flags = write ? MMC_DATA_WRITE : MMC_DATA_READ;
    data.sg = sg;
    data.sg_len = sg_len;
#ifdef CONFIG_SDIO_DEBUG
    OAL_IO_PRINT("[blksz:%u][blocks:%u][sg_len:%u][mode:%s]\n", blksz, blocks, sg_len, write ? "write" : "read");
    OAL_IO_PRINT("%s : [cmd opcode:%d][cmd arg:0x%8x][cmd flags: 0x%8x]\n",
                 mmc_hostname(card->host), cmd.opcode, cmd.arg, cmd.flags);
    OAL_IO_PRINT("Sdio %s data transfer start\n", write ? "write" : "read");
#endif

    mmc_set_data_timeout(&data, card);

    mmc_wait_for_req(card->host, &mrq);

#ifdef CONFIG_SDIO_DEBUG
    OAL_IO_PRINT("wait for %s tranfer over.\n", write ? "write" : "read");
#endif
    if (cmd.err)
        return cmd.err;
    if (data.err)
        return data.err;
    if (OAL_WARN_ON(is_mmc_host_spi(card->host))) {
        OAL_IO_PRINT("HiSi WiFi  driver do not support spi sg transfer!\n");
        return -EIO;
    }
    if (cmd.resp[0] & SDIO_R5_ERROR)
        return -EIO;
    if (cmd.resp[0] & SDIO_R5_FUNCTION_NUMBER)
        return -EINVAL;
    if (cmd.resp[0] & SDIO_R5_OUT_OF_RANGE)
        return -ERANGE;
#ifdef CONFIG_SDIO_DEBUG
    do {
        int i;
        struct scatterlist *sg_t;
        for_each_sg(data.sg, sg_t, data.sg_len, i)
        {
            printk(KERN_DEBUG"======netbuf pkts %d, len:%d=========\n", i, sg_t->length);
            oal_print_hex_dump(sg_virt(sg_t), sg_t->length, 32, "sg buf  :");
        }
    }while (0);
    OAL_IO_PRINT("Transfer done. %s sucuess!\n", write ? "write" : "read");
#endif

    return 0;
}
oal_module_symbol(oal_mmc_io_rw_scat_extended);
#endif
#endif

OAL_STATIC oal_int32 _oal_sdio_transfer_scatt(struct oal_sdio *hi_sdio, oal_int32 rw,
                                              oal_uint32 addr, struct scatterlist *sg,
                                              oal_uint32 sg_len, oal_uint32 rw_sz)
{
#ifdef CONFIG_HISI_SDIO_TIME_DEBUG
    oal_time_t_stru time_start;
#endif
    oal_int32 ret;
    oal_int32 write = (rw == SDIO_READ) ? 0 : 1;
    struct sdio_func *func = hi_sdio->func;
    sdio_claim_host(func);

    /* continue only when tx/rx all opened! */
    if (OAL_UNLIKELY(oal_sdio_get_state(hi_sdio, OAL_SDIO_ALL) != OAL_TRUE)) {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        if (printk_ratelimit())
#endif
        OAL_IO_PRINT("[W][%s]sdio closed,state:%u\n", __FUNCTION__, oal_sdio_get_state(hi_sdio, OAL_SDIO_ALL));
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        schedule();
#endif
        sdio_release_host(func);
        return -OAL_EFAIL;
    }
    ret = oal_mmc_io_rw_scat_extended(hi_sdio, write, SDIO_FUNC_NUM(hi_sdio->func), addr, 0,
                                      sg, sg_len, (rw_sz / HISDIO_BLOCK_SIZE) ? : 1,
                                      min(rw_sz, (oal_uint32)HISDIO_BLOCK_SIZE));

#ifdef CONFIG_HISI_SDIO_TIME_DEBUG
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    time_start = ktime_get();
#endif
#endif
    if (OAL_UNLIKELY(ret)) {
#ifdef CONFIG_HISI_SDIO_TIME_DEBUG
        /* If sdio transfer failed, dump the sdio info */
        oal_uint64  trans_us;
        oal_time_t_stru time_stop = oal_ktime_get();
        trans_us = (oal_uint64)oal_ktime_to_us(oal_ktime_sub(time_stop, time_start));
        printk(KERN_WARNING"[E]sdio_transfer_scatt fail=%d, time cost:%llu us,[addr:%u, sg_len:%u,rw_sz:%u]\n",
                            ret, trans_us, addr, sg_len, rw_sz);

#endif
        if (write) {
            DECLARE_DFT_TRACE_KEY_INFO("sdio_write_fail", OAL_DFT_TRACE_FAIL);
        } else {
            DECLARE_DFT_TRACE_KEY_INFO("sdio_read_fail", OAL_DFT_TRACE_FAIL);
        }
        oal_exception_submit(TRANS_FAIL);
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_SDIO, CHR_PLAT_DRV_ERROR_SDIO_WR_FAIL);
    }
    sdio_release_host(func);
    return ret;
}

oal_int32 oal_sdio_transfer_scatt(struct oal_sdio *hi_sdio, oal_int32 rw,
                                  oal_uint32 addr, struct scatterlist *sg,
                                  oal_uint32 sg_len, oal_uint32 sg_max_len,
                                  oal_uint32 rw_sz)
{
    oal_int32 ret;
    oal_uint32 align_len;
    oal_uint32 align_t;
#ifdef CONFIG_SDIO_DEBUG
    oal_int32 write = (rw == SDIO_READ) ? 0 : 1;
#endif

    OAL_BUG_ON(!hi_sdio);
    OAL_BUG_ON(!rw_sz);
    OAL_BUG_ON(sg_max_len < sg_len);

    if (OAL_WARN_ON(!sg_len)) {
        OAL_IO_PRINT("Sdio %s Scatter list num should never be zero, total request len: %u\n",
                     rw == SDIO_READ ? "SDIO READ" : "SDIO WRITE",
                     rw_sz);
        return -OAL_EINVAL;
    }

    align_t = HISDIO_ALIGN_4_OR_BLK(rw_sz);
    align_len = align_t - rw_sz;

    if (OAL_LIKELY(align_len)) {
        if (OAL_UNLIKELY(sg_len + 1 > sg_max_len)) {
            OAL_IO_PRINT("sg list over,sg_len:%u, sg_max_len:%u\n", sg_len, sg_max_len);
            return -OAL_ENOMEM;
        }
        sg_set_buf(&sg[sg_len], hi_sdio->sdio_align_buff, align_len);
        sg_len++;
    }
    sg_mark_end(&sg[sg_len - 1]);

#ifdef CONFIG_SDIO_DEBUG
    OAL_IO_PRINT("sdio %s request %u bytes transfer, scatter list num %u, used %u bytes to align\n",
                 write ? "write" : "read", rw_sz, sg_len, align_len);
#endif

    rw_sz = align_t;

    /* sdio scatter list driver ,when letter than 512 bytes bytes mode, other blockmode */
    OAL_WARN_ON((rw_sz >= HISDIO_BLOCK_SIZE) && (rw_sz & (HISDIO_BLOCK_SIZE - 1)));
    OAL_WARN_ON((rw_sz < HISDIO_BLOCK_SIZE)  && (rw_sz & (4 - 1)));

    if (OAL_WARN_ON((int32_t)(align_len & 0x3))) {
        OAL_IO_PRINT("not 4 bytes align:%u\n", align_len);
    }

    ret = _oal_sdio_transfer_scatt(hi_sdio, rw, addr, sg, sg_len, rw_sz);

    return ret;
}

oal_int32 oal_sdio_transfer_netbuf_list(struct oal_sdio *hi_sdio,
                                        oal_netbuf_head_stru *head,
                                        oal_int32 rw)
{
    oal_uint8 sg_realloc = 0;
    oal_int32 ret;
    oal_int32 idx = 0;
    oal_uint32 queue_len;
    oal_uint32 sum_len = 0;
    oal_uint32 request_sg_len;
    oal_netbuf_stru* netbuf = NULL;
    oal_netbuf_stru* tmp = NULL;
    struct scatterlist *sg = OAL_PTR_NULL;
    struct sg_table sgtable = {0};
    OAL_BUG_ON(!hi_sdio);
    OAL_BUG_ON(!head);

    if (OAL_WARN_ON(rw >= SDIO_OPT_BUTT)) {
        OAL_IO_PRINT("invaild rw:%d\n", rw);
        return -OAL_EINVAL;
    }

    if (OAL_WARN_ON(oal_netbuf_list_empty(head))) {
        return -OAL_EINVAL;
    }

    if (rw == SDIO_WRITE) {
        if (hi_sdio->pst_pm_callback->wlan_pm_wakeup_dev() != OAL_SUCC) {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wlan_pm_wakeup_dev::host wakeup device failed}");
            return -OAL_EBUSY;
        }
    }

    queue_len = oal_netbuf_list_len(head);
    /* must realloc the sg list mem, alloc more sg for the align buff */
    request_sg_len = queue_len + 1;
    if (OAL_UNLIKELY(request_sg_len > hi_sdio->scatt_info[rw].max_scatt_num)) {
        OAL_IO_PRINT("transfer_netbuf_list realloc sg!, request:%d,max scatt num:%d\n",
                     request_sg_len, hi_sdio->scatt_info[rw].max_scatt_num);
        /* must realloc the sg list mem, alloc more sgs for the align buff */
        if (sg_alloc_table(&sgtable, request_sg_len, GFP_KERNEL)) {
            OAL_IO_PRINT("transfer_netbuf_list alloc sg failed!\n");
            return -OAL_ENOMEM;
        }
        sg_realloc = 1;
        sg = sgtable.sgl;
    } else {
        sg = hi_sdio->scatt_info[rw].sglist;
    }

    if (memset_s(sg, sizeof(struct scatterlist) * request_sg_len, 0,
                 sizeof(struct scatterlist) * request_sg_len) != EOK) {
        OAL_IO_PRINT("transfer_netbuf_list::memset fail!");
        return -OAL_EFAIL;
    }

    skb_queue_walk_safe(head, netbuf, tmp)
    {
        /* assert, should drop the scatt transfer, TBD... */
        OAL_BUG_ON(!OAL_IS_ALIGNED((uintptr_t)OAL_NETBUF_DATA(netbuf), 4));
        if (OAL_WARN_ON(!OAL_IS_ALIGNED(OAL_NETBUF_LEN(netbuf), HISDIO_H2D_SCATT_BUFFLEN_ALIGN))) {
            /* This should never happned, debug */
            oal_netbuf_hex_dump(netbuf);
        }
        OAL_BUG_ON(!OAL_IS_ALIGNED(OAL_NETBUF_LEN(netbuf), HISDIO_H2D_SCATT_BUFFLEN_ALIGN));
        sg_set_buf(&sg[idx], OAL_NETBUF_DATA(netbuf), OAL_NETBUF_LEN(netbuf));
        sum_len += OAL_NETBUF_LEN(netbuf);
        idx++;
    }

    if (OAL_UNLIKELY(idx > queue_len)) {
        OAL_IO_PRINT("idx:%d, queue_len:%d\n", idx, queue_len);
        OAL_BUG_ON(idx > queue_len);
    }
    ret = oal_sdio_transfer_scatt(hi_sdio, rw, HISDIO_REG_FUNC1_FIFO, sg, idx, request_sg_len, sum_len);
    if (sg_realloc) {
        sg_free_table(&sgtable);
    }

    return ret;
}

OAL_STATIC oal_void oal_sdio_remove(struct sdio_func *func)
{
    struct oal_sdio *hi_sdio = OAL_PTR_NULL;

    OAL_BUG_ON(!func);

    hi_sdio = (struct oal_sdio *)sdio_get_drvdata(func);
    if (hi_sdio == NULL) {
        OAL_IO_PRINT("[Error]Invalid NULL hi_sdio!\n");
        return;
    }

    oal_wake_lock_exit(&hi_sdio->st_sdio_wakelock);
    oal_wake_lock_exit(&hi_sdio->st_sdio_rx_wakelock);
    oal_sdio_dev_deinit(hi_sdio);
    oal_sdio_free(hi_sdio);
    sdio_set_drvdata(func, NULL);

    OAL_IO_PRINT("hisilicon connectivity sdio driver has been removed.");
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
OAL_STATIC oal_int32 oal_sdio_suspend(struct device *dev)
{
    /* to be implement */
    struct sdio_func *func = OAL_PTR_NULL;
    struct oal_sdio *hi_sdio = OAL_PTR_NULL;

    OAL_IO_PRINT(KERN_ERR"+++++++sdio suspend+++++++++++++\n");
    if (dev == NULL) {
        OAL_IO_PRINT("[WARN]dev is null\n");
        return OAL_SUCC;
    }
    func = dev_to_sdio_func(dev);
    hi_sdio = sdio_get_drvdata(func);
    if (hi_sdio == NULL) {
        OAL_IO_PRINT("hi_sdio is null\n");
        return OAL_SUCC;
    }

    if (oal_down_interruptible(&chan_wake_sema)) {
        OAL_IO_PRINT(KERN_ERR"chan_wake_sema down failed.");
        return -OAL_EFAIL;
    }

    if (oal_sdio_wakelock_active(hi_sdio)) {
        /* has wake lock so stop controller's suspend,
         * otherwise controller maybe error while sdio reinit */
        OAL_IO_PRINT(KERN_ERR"Already wake up");
        oal_up(&chan_wake_sema);
        return -OAL_EFAIL;
    }
    DECLARE_DFT_TRACE_KEY_INFO("sdio_android_suspend", OAL_DFT_TRACE_SUCC);
    hi_sdio->ul_sdio_suspend++;
    return OAL_SUCC;
}
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
OAL_STATIC oal_int32 oal_sdio_resume(struct device *dev)
{
    struct sdio_func *func = OAL_PTR_NULL;
    struct oal_sdio *hi_sdio = OAL_PTR_NULL;

    OAL_IO_PRINT("+++++++sdio resume+++++++++++++\n");
    if (dev == NULL) {
        OAL_IO_PRINT("[WARN]dev is null\n");
        return OAL_SUCC;
    }
    func = dev_to_sdio_func(dev);
    hi_sdio = sdio_get_drvdata(func);
    if (hi_sdio == NULL) {
        OAL_IO_PRINT("hi_sdio is null\n");
        return OAL_SUCC;
    }

    oal_up(&chan_wake_sema);

    hi_sdio->ul_sdio_resume++;
    DECLARE_DFT_TRACE_KEY_INFO("sdio_android_resume", OAL_DFT_TRACE_SUCC);

    return OAL_SUCC;
}
#endif

OAL_STATIC struct sdio_device_id const oal_sdio_ids[] = {
    { SDIO_DEVICE(HISDIO_VENDOR_ID_HISI, HISDIO_PRODUCT_ID_HISI) },
    {},
};

MODULE_DEVICE_TABLE(sdio, oal_sdio_ids);

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
OAL_STATIC const struct dev_pm_ops oal_sdio_pm_ops = {
    .suspend = oal_sdio_suspend,
    .resume = oal_sdio_resume,
};
#endif

oal_void oal_sdio_dev_shutdown(struct device *dev)
{
    /* android poweroff */
    struct oal_sdio *hi_sdio = oal_get_sdio_default_handler();
    if (hi_sdio == NULL)
        return;

    if (oal_sdio_get_state(hi_sdio, OAL_SDIO_ALL) != OAL_TRUE) {
        /* wlan power off */
        return;
    }

    /* disable sdio/gpio interrupt before android poweroff */
    if (g_hisdio_intr_mode) {
        /* gpio interrupt */
        oal_wlan_gpio_intr_enable(hi_sdio, 0);
    } else {
        /* sdio interrupt */
        oal_int32   ret;
        oal_sdio_claim_host(hi_sdio);
        ret = sdio_disable_func(hi_sdio->func);
        oal_sdio_release_host(hi_sdio);
        if (ret) {
            OAL_IO_PRINT("wlan shutdown faile,ret=%d!\n", ret);
            return;
        }
    }

    OAL_IO_PRINT("wlan shutdown sucuess!\n");
}
OAL_STATIC  struct sdio_driver oal_sdio_driver = {
    .name       = "oal_sdio",
    .id_table   = oal_sdio_ids,
    .probe      = oal_sdio_probe,
    .remove     = oal_sdio_remove,

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    .drv        = {
        .owner  = THIS_MODULE,
        .pm     = &oal_sdio_pm_ops,
        .shutdown = oal_sdio_dev_shutdown,
    }
#endif
};
/* notify the mmc to probe sdio device. */
oal_void oal_sdio_detectcard_to_core(oal_int32 val)
{
    PS_PRINT_INFO("hi110x mmc detect\n");
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    if (val == 0) {
        struct sdio_func *func = NULL;
        func = sdio_get_func(1, oal_sdio_ids[0].vendor, oal_sdio_ids[0].device);
        if (func != NULL) {
            oal_sdio_driver.remove(func);
        }
    }
#endif
    OAL_SDIO_CARD_DETECT_CHANGE(val);

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    if (val == 1) {
        struct sdio_func *func = NULL;
        func = sdio_get_func(1, oal_sdio_ids[0].vendor, oal_sdio_ids[0].device);
        if (func != NULL) {
            oal_sdio_driver.probe(func, oal_sdio_ids);
        }
    }
#endif
}

oal_void hi_wlan_power_set(oal_int32 on)
{
    /*
     * this should be done in mpw1
     * it depends on the gpio used to power up and down 1101 chip
     *
     * */
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)||(_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    if (on) {
        OAL_IO_PRINT("sdio probe:pull up power on gpio\n");
        board_power_on();
    } else {
        OAL_IO_PRINT("sdio probe:pull down power on gpio\n");
        board_power_off();
    }
#endif
}

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
oal_int32 oal_sdio_func_probe_resume(void)
{
    oal_int32 ret;

    OAL_INIT_COMPLETION(&sdio_driver_complete);

    PS_PRINT_INFO(KERN_ERR"start to register sdio module");

    ret = sdio_register_driver(&oal_sdio_driver);
    if (ret) {
        OAL_IO_PRINT("register sdio driver Failed ret=%d\n", ret);
        goto failed_sdio_reg;
    }
    oal_sdio_detectcard_to_core(1);
    if (oal_wait_for_completion_timeout(&sdio_driver_complete, 10 * HZ)) {
        PS_PRINT_WARNING("hisi sdio load sucuess, sdio enum done.\n");
    } else {
        OAL_IO_PRINT("sdio enum timeout, reason[%s]\n", sdio_enum_err_str);
        goto failed_sdio_enum;
    }
    return OAL_SUCC;
failed_sdio_enum:
    sdio_unregister_driver(&oal_sdio_driver);

failed_sdio_reg:
    /* sdio can not remove!
      hi_sdio_detectcard_to_core(0); */
    return -OAL_EFAIL;
}
#endif

oal_int32 oal_sdio_func_probe(struct oal_sdio* hi_sdio)
{
    oal_int32 ret;
    if (hi_sdio == NULL)
        return -OAL_EFAIL;

    OAL_INIT_COMPLETION(&sdio_driver_complete);

    OAL_IO_PRINT(KERN_ERR"start to register sdio module");

    ret = sdio_register_driver(&oal_sdio_driver);
    if (ret) {
        OAL_IO_PRINT("register sdio driver Failed ret=%d\n", ret);
        goto failed_sdio_reg;
    }
    hi_wlan_power_set(1);
    /* notify mmc core to detect sdio device */
    oal_sdio_detectcard_to_core(1);

    if (oal_wait_for_completion_timeout(&sdio_driver_complete, 10 * HZ)) {
        OAL_IO_PRINT("hisi sdio load sucuess, sdio enum done.\n");
    } else {
        OAL_IO_PRINT("sdio enum timeout, reason[%s]\n", sdio_enum_err_str);
#ifdef CONFIG_HUAWEI_DSM
#if (_PRE_LINUX_PLATFORM == MIAMI_C60)
        hw_1131k_dsm_client_notify(SYSTEM_TYPE_PLATFORM, DSM_SDIO_PROBE_FAIL, "%s: sdio probe fail\n", __FUNCTION__);
#endif
#endif
        goto failed_sdio_enum;
    }

    if (firmware_mem_init() != OAL_SUCC) {
        goto failed_sdio_enum;
    }

    if (firmware_read_efuse_info() != OAL_SUCC) {
        goto failed_sdio_enum;
    }

    if (firmware_read_efuse_die_id() != OAL_SUCC) {
        goto failed_sdio_enum;
    }

    OAL_IO_PRINT("{oal_sdio_func_probe::shutdown wifi after init sdio.\n}");
    oal_sdio_claim_host(hi_sdio);
    oal_disable_sdio_state(hi_sdio, OAL_SDIO_ALL);
    hi_wlan_power_set(0);
    oal_sdio_release_host(hi_sdio);
    return OAL_SUCC;
failed_sdio_enum:
    sdio_unregister_driver(&oal_sdio_driver);
failed_sdio_reg:
    /* sdio can not remove!
      hi_sdio_detectcard_to_core(0); */
    hi_wlan_power_set(0);
    chr_exception(chr_wifi_dev(CHR_WIFI_DEV_EVENT_CHIP, CHR_WIFI_DEV_ERROR_SDIO_ENUM));
    return -OAL_EFAIL;
}

oal_int32 oal_sdio_func_reset(void)
{
    oal_int32 l_need_retry = 1;
    OAL_INIT_COMPLETION(&sdio_driver_complete);

    oal_sdio_detectcard_to_core(0);

    wlan_rst();
detect_retry:
    /* notify mmc core to detect sdio device */
    oal_sdio_detectcard_to_core(1);
    if (oal_wait_for_completion_timeout(&sdio_driver_complete, 5 * HZ)) {
        OAL_IO_PRINT("hisi sdio load sucuess, sdio enum done.\n");
    } else {
        OAL_IO_PRINT("sdio enum timeout, reason[%s]\n", sdio_enum_err_str);

        if (l_need_retry) {
            OAL_IO_PRINT("sdio enum retry.\n");
            l_need_retry = 0;
            hi_wlan_power_set(1);
            goto detect_retry;
        }
        goto failed_sdio_enum;
    }

    return OAL_SUCC;

failed_sdio_enum:
    sdio_unregister_driver(&oal_sdio_driver);
    hi_wlan_power_set(0);
    return -OAL_EFAIL;
}

oal_void oal_sdio_func_remove(struct oal_sdio* hi_sdio)
{
    sdio_unregister_driver(&oal_sdio_driver);
    oal_sdio_detectcard_to_core(0);
    hi_wlan_power_set(0);
}

oal_void oal_sdio_credit_info_init(struct oal_sdio* hi_sdio)
{
    hi_sdio->sdio_credit_info.large_free_cnt = 0;
    hi_sdio->sdio_credit_info.short_free_cnt = 0;
    oal_spin_lock_init(&hi_sdio->sdio_credit_info.credit_lock);
}

struct oal_sdio* oal_sdio_init_module(oal_void* data)
{
#ifdef CONFIG_HISDIO_H2D_SCATT_LIST_ASSEMBLE
    oal_uint32 tx_scatt_buff_len = 0;
#endif
    oal_uint32 ul_rx_seg_size;
    struct oal_sdio* hi_sdio = OAL_PTR_NULL;

    PS_PRINT_INFO("hii110x sdio driver installing...\n");
    hi_sdio = (struct oal_sdio*)oal_memalloc(OAL_SIZEOF(struct oal_sdio));
    if (hi_sdio == NULL) {
        OAL_IO_PRINT("[E]alloc oal_sdio failed [%d]\n", (oal_int32)OAL_SIZEOF(struct oal_sdio));
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        chr_exception(chr_wifi_drv(CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_SDIO_INIT));
#endif
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_SDIO, CHR_PLAT_DRV_ERROR_ALLOC_MEM);
        return NULL;
    }
    memset_s((oal_void*)hi_sdio, OAL_SIZEOF(struct oal_sdio), 0, OAL_SIZEOF(struct oal_sdio));

#ifdef CONFIG_SDIO_FUNC_EXTEND
    sdio_extend_func = 1;
#else
    sdio_extend_func = 0;
#endif

    ul_rx_seg_size = ALIGN((HSDIO_HOST2DEV_PKTS_MAX_LEN), HISDIO_BLOCK_SIZE);
    /* alloc rx reserved mem */
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    hi_sdio->rx_reserved_buff = (oal_void*)memalign(CACHE_ALIGNED_SIZE, ul_rx_seg_size);
#else
    hi_sdio->rx_reserved_buff = (oal_void*)oal_memalloc(ul_rx_seg_size);
#endif
    if (hi_sdio->rx_reserved_buff == NULL) {
        OAL_IO_PRINT("[E]alloc rx_reserved_buff failed [%u]\n", ul_rx_seg_size);
        goto failed_rx_reserved_buff_alloc;
    }
    hi_sdio->rx_reserved_buff_len = ul_rx_seg_size;

    hi_sdio->func1_int_mask = HISDIO_FUNC1_INT_MASK;

    oal_sdio_credit_info_init(hi_sdio);

    hi_sdio->sdio_extend = (struct hisdio_extend_func*)oal_memalloc(sizeof(struct hisdio_extend_func));
    if (hi_sdio->sdio_extend == NULL) {
        OAL_IO_PRINT("[E]alloc sdio_extend failed [%d]\n", (oal_int32)sizeof(struct hisdio_extend_func));
        goto failed_sdio_extend_alloc;
    }
    memset_s(hi_sdio->sdio_extend, sizeof(struct hisdio_extend_func), 0, sizeof(struct hisdio_extend_func));
    hi_sdio->bus_data = data;
    _hi_sdio_ = hi_sdio;
#ifdef CONFIG_SDIO_DEBUG
    hi_sdio_debug = hi_sdio;
#endif

    hi_sdio->scatt_info[SDIO_READ].max_scatt_num = HISDIO_DEV2HOST_SCATT_MAX + 1;

    hi_sdio->scatt_info[SDIO_READ].sglist = oal_kzalloc(
                    OAL_SIZEOF(struct scatterlist)*(HISDIO_DEV2HOST_SCATT_MAX + 1), OAL_GFP_KERNEL);
    if (hi_sdio->scatt_info[SDIO_READ].sglist == NULL) {
        goto failed_sdio_read_sg_alloc;
    }

    /* 1 for algin buff, 1 for scatt info buff */
    hi_sdio->scatt_info[SDIO_WRITE].max_scatt_num = HISDIO_HOST2DEV_SCATT_MAX + 2;
    hi_sdio->scatt_info[SDIO_WRITE].sglist = oal_kzalloc(
                    OAL_SIZEOF(struct scatterlist)*(hi_sdio->scatt_info[SDIO_WRITE].max_scatt_num), OAL_GFP_KERNEL);
    if (hi_sdio->scatt_info[SDIO_WRITE].sglist == NULL) {
        goto failed_sdio_write_sg_alloc;
    }

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    hi_sdio->sdio_align_buff = memalign(CACHE_ALIGNED_SIZE, SKB_DATA_ALIGN(HISDIO_BLOCK_SIZE));
#else
    hi_sdio->sdio_align_buff = oal_kzalloc(HISDIO_BLOCK_SIZE, OAL_GFP_KERNEL);
#endif
    if (hi_sdio->sdio_align_buff == NULL) {
        goto failed_sdio_align_buff_alloc;
    }
#ifdef CONFIG_HISDIO_H2D_SCATT_LIST_ASSEMBLE
    tx_scatt_buff_len = HISDIO_HOST2DEV_SCATT_SIZE + HISDIO_HOST2DEV_SCATT_MAX * (HCC_HDR_TOTAL_LEN +
                OAL_ROUND_UP(HSDIO_HOST2DEV_PKTS_MAX_LEN, HISDIO_H2D_SCATT_BUFFLEN_ALIGN));
    tx_scatt_buff_len = HISDIO_ALIGN_4_OR_BLK(tx_scatt_buff_len);

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    tx_scatt_buff_len = SKB_DATA_ALIGN(tx_scatt_buff_len);
    hi_sdio->scatt_buff.buff = memalign(CACHE_ALIGNED_SIZE, tx_scatt_buff_len);
#else
    hi_sdio->scatt_buff.buff = oal_memalloc(tx_scatt_buff_len);
#endif

    if (hi_sdio->scatt_buff.buff == NULL) {
        OAL_IO_PRINT("alloc scatt_buff failed,request %u bytes\n", tx_scatt_buff_len);
        goto failed_sdio_scatt_buff_alloc;
    }
    hi_sdio->scatt_buff.len = tx_scatt_buff_len;
#endif

    oal_sdio_message_register(hi_sdio, D2H_MSG_DEVICE_PANIC, oal_device_panic_callback, NULL);
    return hi_sdio;
#ifdef CONFIG_HISDIO_H2D_SCATT_LIST_ASSEMBLE
failed_sdio_scatt_buff_alloc:
    oal_free(hi_sdio->sdio_align_buff);
#endif
failed_sdio_align_buff_alloc:
    oal_free(hi_sdio->scatt_info[SDIO_WRITE].sglist);
failed_sdio_write_sg_alloc:
    oal_free(hi_sdio->scatt_info[SDIO_READ].sglist);
failed_sdio_read_sg_alloc:
    oal_free(hi_sdio->sdio_extend);
failed_sdio_extend_alloc:
    oal_free(hi_sdio->rx_reserved_buff);
failed_rx_reserved_buff_alloc:
    oal_free(hi_sdio);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    chr_exception(chr_wifi_drv(CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_SDIO_INIT));
#endif
    chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                         CHR_PLT_DRV_EVENT_SDIO, CHR_PLAT_DRV_ERROR_SDIO_MODULE);
    return NULL;
}
oal_module_symbol(oal_sdio_init_module);

oal_void  oal_sdio_exit_module(struct oal_sdio* hi_sdio)
{
    OAL_IO_PRINT("sdio module unregistered\n");
#ifdef CONFIG_HISDIO_H2D_SCATT_LIST_ASSEMBLE
    oal_free(hi_sdio->scatt_buff.buff);
#endif
    oal_free(hi_sdio->sdio_align_buff);
    oal_free(hi_sdio->scatt_info[SDIO_WRITE].sglist);
    oal_free(hi_sdio->scatt_info[SDIO_READ].sglist);
    oal_free(hi_sdio->sdio_extend);
    oal_free(hi_sdio->rx_reserved_buff);
    oal_free(hi_sdio);
    _hi_sdio_ = NULL;
#ifdef CONFIG_SDIO_DEBUG
    hi_sdio_debug = NULL;
#endif
}
oal_module_symbol(oal_sdio_exit_module);

#ifdef CONFIG_SDIO_DEBUG
/* ST */
void oal_sdio_tc_msg_001(int msg)
{
    OAL_IO_PRINT("send msg 0x%8X\n", msg);
    wlan_pm_disable();
    oal_sdio_send_msg(hi_sdio_debug, msg);
    wlan_pm_enable();
}
void oal_sdio_tc_buf_tx_001(void)
{
    oal_int32 ret;
    void *buf = oal_memalloc(512 * 3);
    if (buf == NULL) {
        OAL_IO_PRINT("oal_sdio_tc_buf_tx_001 memmalloc buf error\n");
        return;
    }
    memset_s(buf, 512 * 3, 0xff, 512 * 3);
    ret = oal_sdio_single_transfer(hi_sdio_debug, SDIO_WRITE, buf, 512 * 3);
    if (ret) {
        OAL_IO_PRINT("[WIFI] oal_sdio_tc_buf_tx_001 failed=%d\n", ret);
    }
}

void oal_sdio_tc_extend_001(void)
{
    int ret;
    char *buf = NULL;
    oal_netbuf_stru *netbuf = oal_netbuf_alloc(HISDIO_FUNC1_EXTEND_REG_LEN, 0, 0);
    if (netbuf == NULL) {
        printk("alloc netbuf fail\n");
        return;
    }
    oal_netbuf_put(netbuf, HISDIO_ALIGN_4_OR_BLK(HISDIO_FUNC1_EXTEND_REG_LEN));
    buf = OAL_NETBUF_DATA(netbuf);
    memset_s(buf, OAL_NETBUF_LEN(netbuf), 0, OAL_NETBUF_LEN(netbuf));
    sdio_claim_host(hi_sdio_debug->func);
    ret = oal_sdio_memcpy_fromio(hi_sdio_debug->func, buf, HISDIO_FUNC1_EXTEND_REG_BASE, HISDIO_FUNC1_EXTEND_REG_LEN);
    sdio_release_host(hi_sdio_debug->func);
    if (ret) {
        printk("read failed ret=%d\n", ret);
    }
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    print_hex_dump_bytes("extend :", DUMP_PREFIX_ADDRESS, buf, HISDIO_FUNC1_EXTEND_REG_LEN);
#endif
    oal_netbuf_free(netbuf);
}

void oal_sdio_tc_mem_cp_from(int offset, int len)
{
    int ret;
    char *buf = NULL;
    oal_netbuf_stru *netbuf = oal_netbuf_alloc(len, 0, 0);
    if (netbuf == NULL) {
        printk("alloc fail\n");
        return;
    }
    oal_netbuf_put(netbuf, HISDIO_ALIGN_4_OR_BLK(len));
    buf = OAL_NETBUF_DATA(netbuf);
    memset_s(buf, OAL_NETBUF_LEN(netbuf), 0, OAL_NETBUF_LEN(netbuf));
    sdio_claim_host(hi_sdio_debug->func);
    ret = oal_sdio_memcpy_fromio(hi_sdio_debug->func, buf, offset, len);
    sdio_release_host(hi_sdio_debug->func);
    if (ret) {
        printk("read failed ret=%d\n", ret);
    }
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    print_hex_dump_bytes("extend :", DUMP_PREFIX_ADDRESS, buf, len);
#endif
    oal_netbuf_free(netbuf);
}

void oal_sdio_read_func0(int offset, int len)
{
    int i;
    int j = 0;
    int ret = 0;
    char *buf = NULL;
    oal_netbuf_stru *netbuf = oal_netbuf_alloc(len, 0, 0);
    if (netbuf == NULL) {
        printk("alloc fail\n");
        return;
    }
    oal_netbuf_put(netbuf, len);
    buf = OAL_NETBUF_DATA(netbuf);
    memset_s(buf, OAL_NETBUF_LEN(netbuf), 0, OAL_NETBUF_LEN(netbuf));
    for (i = 0; i < len; i++) {
        sdio_claim_host(hi_sdio_debug->func);
        *(buf + i) = sdio_f0_readb(hi_sdio_debug->func, offset + i, &ret);
        sdio_release_host(hi_sdio_debug->func);
        if (ret) {
            printk("read error ret=%d\n", ret);
            *(buf + i) = 0;
            break;
        }
        j++;
    }
    if (i != j)
        printk("request read %d bytes, but only read %d bytes\n", len, j);
    printk("##################%s##########[offset:0x%X, len:0x%X]\n", __FUNCTION__, offset, len);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    print_hex_dump_bytes("read_func0: ", DUMP_PREFIX_ADDRESS, buf, j);
#endif
    oal_netbuf_free(netbuf);
}

void oal_sdio_read_func1(int offset, int len)
{
    int i;
    int j = 0;
    int ret = 0;
    char *buf = NULL;
    oal_netbuf_stru *netbuf = oal_netbuf_alloc(len, 0, 0);
    if (netbuf == NULL) {
        printk("alloc fail\n");
        return;
    }
    oal_netbuf_put(netbuf, len);
    buf = OAL_NETBUF_DATA(netbuf);
    memset_s(buf, OAL_NETBUF_LEN(netbuf), 0, OAL_NETBUF_LEN(netbuf));
    for (i = 0; i < len; i++) {
        sdio_claim_host(hi_sdio_debug->func);
        *(buf + i) = oal_sdio_readb(hi_sdio_debug->func, offset + i, &ret);
        sdio_release_host(hi_sdio_debug->func);
        if (ret) {
            printk("read error ret=%d\n", ret);
            *(buf + i) = 0;
            break;
        }
        j++;
    }
    if (i != j)
        printk("request read %d bytes, but only read %d bytes\n", len, j);
    printk("##################%s##########[offset:0x%X, len:0x%X]\n", __FUNCTION__, offset, len);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    print_hex_dump_bytes("read_func0: ", DUMP_PREFIX_ADDRESS, buf, j);
#endif
    oal_netbuf_free(netbuf);
}

void oal_sdio_readsb_test(int offset, int len)
{
    int ret;
    char *buf = NULL;
    oal_netbuf_stru *netbuf = oal_netbuf_alloc(len, 0, 0);
    if (netbuf == NULL) {
        printk("alloc fail\n");
        return;
    }
    oal_netbuf_put(netbuf, len);
    buf = OAL_NETBUF_DATA(netbuf);
    memset_s(buf, OAL_NETBUF_LEN(netbuf), 0, OAL_NETBUF_LEN(netbuf));
    sdio_claim_host(hi_sdio_debug->func);
    ret = oal_sdio_readsb(hi_sdio_debug->func, buf, offset, len);
    sdio_release_host(hi_sdio_debug->func);
    if (ret) {
        printk("read failed ret=%d\n", ret);
    }
    printk("##################%s##########[offset:0x%X, len:0x%X]\n", __FUNCTION__, offset, len);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    print_hex_dump_bytes("oal readsb :", DUMP_PREFIX_ADDRESS, buf, len);
#endif
    oal_netbuf_free(netbuf);
}

void oal_sdio_sched_001(void)
{
    if (hi_sdio_debug == NULL)
        return;
    printk("##################%s##########\n", __FUNCTION__);
    if (g_hisdio_intr_mode)
        oal_up(&hi_sdio_debug->gpio_rx_sema);
    else
        mmc_signal_sdio_irq(hi_sdio_debug->func->card->host);
}

void oal_sdio_dump_extend_buf(void)
{
    if (hi_sdio_debug != NULL) {
        sdio_claim_host(hi_sdio_debug->func);
        oal_sdio_extend_buf_get(hi_sdio_debug);
        sdio_release_host(hi_sdio_debug->func);
#ifdef CONFIG_SDIO_DEBUG
        printk(KERN_DEBUG"=========extend buff:%d=====\n",
                            HISDIO_COMM_REG_SEQ_GET(hi_sdio_debug->sdio_extend->credit_info));
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        print_hex_dump_bytes("extend :", DUMP_PREFIX_ADDRESS,
                             (oal_void*)hi_sdio_debug->sdio_extend, sizeof(struct hisdio_extend_func));
#endif
#endif
    }
}
#endif

oal_uint32 oal_sdio_func_max_req_size(struct oal_sdio *pst_hi_sdio)
{
    oal_uint32 max_blocks;
    oal_uint32 size, size_device;
    oal_uint32 size_host;
    OAL_BUG_ON(pst_hi_sdio == NULL);
    /* host transer limit */
    /* Blocks per command is limited by host count, host transfer
    * size and the maximum for IO_RW_EXTENDED of 511 blocks. */
    max_blocks = OAL_MIN(SDIO_GET_MAX_BLOCK_COUNT(pst_hi_sdio->func), 511u);
    size = max_blocks * HISDIO_BLOCK_SIZE;

    size = OAL_MIN(size, SDIO_GET_MAX_REQ_SIZE(pst_hi_sdio->func));

    /* device transer limit,per adma descr limit 32K in bootloader,
    and total we have 20 descs */
    size_device = (32 * 1024) * 20;

#ifdef HOST_SDIO_MAX_TRANSFER_SIZE
    size_host = HOST_SDIO_MAX_TRANSFER_SIZE;
#else
    size_host = 0xffffffff;
#endif
    size = OAL_MIN(size, size_device);
    size = OAL_MIN(size, size_host);
    return size;
}

oal_int32 oal_sdio_transfer_prepare(struct oal_sdio *hi_sdio)
{
    oal_enable_sdio_state(hi_sdio, OAL_SDIO_ALL);
#ifdef _PRE_FEATURE_NO_GPIO
    if (oal_register_sdio_intr(hi_sdio) < 0) {
        OAL_IO_PRINT("failed to register sdio interrupt\n");
        return -OAL_EFAIL;
    }
#else
    oal_wlan_gpio_intr_enable(hi_sdio, OAL_TRUE);
#endif
    return OAL_SUCC;
}
#else
struct oal_sdio* oal_sdio_init_module(void* data)
{
    return NULL;
}

int32_t oal_sdio_func_probe(struct oal_sdio* hi_sdio)
{
    return -OAL_EFAIL;
}

int32_t oal_sdio_transfer_rx_register(struct oal_sdio *hi_sdio, hisdio_rx rx)
{
    return -OAL_EFAIL;
}

int32_t oal_sdio_message_register(struct oal_sdio *hi_sdio, uint8_t msg, sdio_msg_rx cb, void* data)
{
    return -OAL_EFAIL;
}

int32_t oal_sdio_build_rx_netbuf_list(struct oal_sdio *hi_sdio, oal_netbuf_head_stru *head)
{
    return OAL_SUCC;
}

int32_t oal_sdio_transfer_netbuf_list(struct oal_sdio *hi_sdio, oal_netbuf_head_stru *head, int32_t rw)
{
    return -OAL_FAIL;
}
#endif

oal_void oal_netbuf_list_hex_dump(oal_netbuf_head_stru* head)
{
#ifdef CONFIG_PRINTK
    oal_int32 index = 0;
    oal_netbuf_stru *netbuf = OAL_PTR_NULL;
    oal_netbuf_stru *tmp = OAL_PTR_NULL;
    if (!skb_queue_len(head))
        return;
    printk(KERN_DEBUG"prepare to dump %d pkts=========\n", skb_queue_len(head));
    skb_queue_walk_safe(head, netbuf, tmp)
    {
        index++;
        printk(KERN_DEBUG"======netbuf pkts %d, len:%d=========\n", index, OAL_NETBUF_LEN(netbuf));
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        print_hex_dump_bytes("netbuf  :", DUMP_PREFIX_ADDRESS, OAL_NETBUF_DATA(netbuf), OAL_NETBUF_LEN(netbuf));
#endif
    }
#else
    OAL_REFERENCE(head);
#endif
}

oal_void oal_netbuf_hex_dump(oal_netbuf_stru* netbuf)
{
#ifdef CONFIG_PRINTK
    printk(KERN_DEBUG"==prepare to netbuf,%p,len:%d=========\n",
           OAL_NETBUF_DATA(netbuf),
           OAL_NETBUF_LEN(netbuf));
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    print_hex_dump_bytes("netbuf  :", DUMP_PREFIX_ADDRESS, OAL_NETBUF_DATA(netbuf), OAL_NETBUF_LEN(netbuf));
#endif
#else
    OAL_REFERENCE(netbuf);
#endif
}

/*lint -e19*/
oal_module_symbol(oal_get_gpio_int_count_para);
/*lint +e19*/
oal_uint32 oal_sdio_get_large_pkt_free_cnt(struct oal_sdio *hi_sdio)
{
    oal_uint32 free_cnt;
    OAL_BUG_ON(!hi_sdio);
    oal_spin_lock(&hi_sdio->sdio_credit_info.credit_lock);
    free_cnt = (oal_uint32)hi_sdio->sdio_credit_info.large_free_cnt;
    oal_spin_unlock(&hi_sdio->sdio_credit_info.credit_lock);
    return free_cnt;
}

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
oal_int32 oal_sdio_reinit(void)
{
    oal_int32 ret;
    oal_int32 reset_retry_count = SDIO_RESET_RETRY;
    struct oal_sdio *pst_sdio = NULL;
    struct sdio_func *pst_func = NULL;

    pst_sdio = oal_get_sdio_default_handler();
    if (pst_sdio == NULL) {
        OAL_IO_PRINT("sdio handler is NULL!\n");
        return -OAL_EFAIL;
    }

    pst_func = pst_sdio->func;
    if (pst_func == NULL) {
        OAL_IO_PRINT("sdio func is NULL!\n");
        return -OAL_EFAIL;
    }

    oal_sdio_claim_host(pst_sdio);
    oal_disable_sdio_state(pst_sdio, OAL_SDIO_ALL);

    while (1) {
        if (sdio_reset_comm(pst_func->card) == OAL_SUCC) {
            break;
        }
        reset_retry_count--;
        if (reset_retry_count == 0) { // by frost
            dprintf("reset sdio failed after retry %d times,exit %s!", SDIO_RESET_RETRY, __FUNCTION__);
            oal_sdio_release_host(pst_sdio);
            return -OAL_EFAIL;
        }
    }

    SDIO_EN_TIMEOUT(pst_func) = 1000;
    ret = sdio_enable_func(pst_func);
    if (ret < 0) {
        OAL_IO_PRINT("failed to enable sdio function! ret=%d\n", ret);
        goto failed_enabe_func;
    }
    ret = sdio_set_block_size(pst_func, HISDIO_BLOCK_SIZE);
    if (ret) {
        OAL_IO_PRINT("failed to set sdio blk size! ret=%d\n", ret);
        goto failed_set_block_size;
    }
    if (pst_func->card->host->caps.bits.cap_sdio_irq) {
            oal_sdio_writeb(pst_func, HISDIO_FUNC1_INT_MASK, HISDIO_REG_FUNC1_INT_ENABLE, &ret);
            if (ret < 0) {
                OAL_IO_PRINT("failed to enable sdio interrupt! ret=%d\n", ret);
                printf("failed to enable sdio interrupt! ret=%d\n", ret);
                goto failed_enable_func1;
            }
    }
    oal_enable_sdio_state(pst_sdio, OAL_SDIO_ALL);
    oal_sdio_release_host(pst_sdio);

    OAM_INFO_LOG1(0, OAM_SF_SDIO, "sdio function %d enabled.\n", SDIO_FUNC_NUM(pst_func));

    OAM_INFO_LOG0(0, OAM_SF_SDIO, "pm reinit sdio success...\n");

    return OAL_SUCC;

failed_enable_func1:
failed_clear_func1_int:
failed_set_block_size:
    sdio_disable_func(pst_func);
failed_enabe_func:
    oal_sdio_release_host(pst_sdio);
    chr_exception(chr_wifi_drv(CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_SDIO_INIT));
    return ret;
}
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_int32 sdio_dev_init(struct sdio_func *func)
{
    oal_int32 ret;
    struct pm_drv_data *pm_data = NULL;

    sdio_claim_host(func);

    func->enable_timeout = 1000;

    ret = sdio_enable_func(func);
    if (ret < 0) {
        PS_PRINT_ERR("failed to enable sdio function! ret=%d\n", ret);
    }

    ret = sdio_set_block_size(func, HISDIO_BLOCK_SIZE);
    if (ret < 0) {
        PS_PRINT_ERR("failed to set sdio blk size! ret=%d\n", ret);
    }

    /* before enable sdio function 1, clear its interrupt flag, no matter it exist or not */
    /* if clear the interrupt, host can't receive dev wake up ok ack. debug by 1131c */
#if 0
    oal_sdio_writeb(func, HISDIO_FUNC1_INT_MASK, HISDIO_REG_FUNC1_INT_STATUS, &ret);
    if (ret < 0) {
        PS_PRINT_ERR("failed to clear sdio interrupt! ret=%d\n", ret);
    }
#endif
    /*
     * enable four interrupt sources in function 1:
     *      data ready for host to read
     *      read data error
     *      message from arm is available
     *      device has receive message from host
     * */
    oal_sdio_writeb(func, HISDIO_FUNC1_INT_MASK, HISDIO_REG_FUNC1_INT_ENABLE, &ret);
    if (ret < 0) {
        PS_PRINT_ERR("failed to enable sdio interrupt! ret=%d\n", ret);
    }
    pm_data = pm_get_drvdata();
    oal_enable_sdio_state(pm_data->pst_wlan_pm_info->pst_channel, OAL_SDIO_ALL);
    sdio_release_host(func);

    PS_PRINT_INFO("sdio function %d enabled.\n", func->num);

    return ret;
}

int32_t oal_sdio_mmc_power_off(void)
{
    oal_int32 ret;
    oal_int32 retry_times = SDIO_REINIT_RETRY;
    struct pm_drv_data *pm_data = pm_get_drvdata();
    if (pm_data == NULL) {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }
    oal_sdio_claim_host(pm_data->pst_wlan_pm_info->pst_channel);
    while (retry_times--) {
        ret = mmc_power_save_host(pm_data->pst_wlan_pm_info->pst_channel->func->card->host);
        if (ret < 0) {
            PS_PRINT_ERR("failed to mmc_power_restore_host fail return %x\n", ret);
        } else {
            PS_PRINT_ERR("mmc_power_restore_succ\n");
            break;
        }
        mdelay(SDIO_REINIT_DELAY);
    }

    oal_sdio_release_host(pm_data->pst_wlan_pm_info->pst_channel);
    return SUCCESS;
}

oal_int32 oal_sdio_reinit(void)
{
    oal_int32 ret;
    oal_int32 retry_times = SDIO_REINIT_RETRY;
    struct pm_drv_data *pm_data = pm_get_drvdata();
    if (pm_data == NULL) {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    oal_sdio_claim_host(pm_data->pst_wlan_pm_info->pst_channel);
    oal_disable_sdio_state(pm_data->pst_wlan_pm_info->pst_channel, OAL_SDIO_ALL);
    while (retry_times--) {
        PS_PRINT_INFO("start to power restore sdio\n");
        ret = mmc_power_save_host(pm_data->pst_wlan_pm_info->pst_channel->func->card->host);
        pm_data->pst_wlan_pm_info->pst_channel->func->card->host->pm_flags &= ~MMC_PM_KEEP_POWER;
        ret = mmc_power_restore_host(pm_data->pst_wlan_pm_info->pst_channel->func->card->host);
        pm_data->pst_wlan_pm_info->pst_channel->func->card->host->pm_flags |= MMC_PM_KEEP_POWER;
        if (ret < 0) {
            PS_PRINT_ERR("failed to mmc_power_restore_host fail return %x\n", ret);
            chr_exception(chr_wifi_drv(CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_SDIO_INIT));
        } else {
            PS_PRINT_INFO("mmc_power_restore_succ\n");
            break;
        }
        /* 先拉低GPIO管脚，延迟1ms后，再拉高GPIO管脚 */
        board_set_wlan_h2d_pm_state(WLAN_PM_SLPREQ_LEVEL);
        mdelay(1);
        board_set_wlan_h2d_pm_state(WLAN_PM_WKUPDEV_LEVEL);
        mdelay(10);
    }

    if (ret < 0) {
        PS_PRINT_ERR("failed to reinit sdio,fail return %x\n", ret);
        oal_sdio_release_host(pm_data->pst_wlan_pm_info->pst_channel);
        return -FAILURE;
    }

    if (sdio_dev_init(pm_data->pst_wlan_pm_info->pst_channel->func) != SUCCESS) {
        PS_PRINT_ERR("sdio dev reinit failed\n");
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_SDIO, CHR_PLAT_DRV_ERROR_SDIO_INIT);
        oal_sdio_release_host(pm_data->pst_wlan_pm_info->pst_channel);
        return -FAILURE;
    }

    oal_sdio_release_host(pm_data->pst_wlan_pm_info->pst_channel);
    PS_PRINT_INFO("sdio_dev_init ok\n");

    return SUCCESS;
}
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
