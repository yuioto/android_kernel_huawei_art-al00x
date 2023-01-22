
#define HISI_LOG_TAG "[HCC]"
#include "oal_util.h"
#include "oal_sdio_host_if.h"
#include "oal_hcc_host_if.h"
#include "oal_profiling.h"
#include "oam_ext_if.h"
#include "exception_rst.h"
#include "securec.h"

#include "oal_schedule.h"

#ifdef CONFIG_MMC
#include "plat_pm_wlan.h"
#endif

#include "oal_channel_host_if.h"

#include "plat_debug.h"

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAL_HCC_HOST_C


#define HCC_TX_FLOW_HI_LEVEL   512
#define HCC_TX_FLOW_LO_LEVEL    128

#define HCC_TRANS_THREAD_POLICY    OAL_SCHED_RR
#define HCC_TRANS_THERAD_PRIORITY       10
#define HCC_TRANS_REALY_THERAD_PRIORITY 97
#define HCC_TRANS_THERAD_NICE           (-10)
OAL_STATIC struct hcc_handler* hcc_tc = NULL;

OAL_STATIC oal_uint32 g_hcc_tx_max_buf_len = 4096;

oal_uint32 g_ul_tcp_ack_wait_sche_cnt = 1;

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
struct alg_process_func_handler g_pst_alg_process_func;
EXPORT_SYMBOL_GPL(g_pst_alg_process_func);
#endif

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
struct custom_process_func_handler g_pst_custom_process_func;
EXPORT_SYMBOL_GPL(g_pst_custom_process_func);
#endif


oal_uint32 hcc_assemble_count = 8;
/*lint -e19 */
oal_module_symbol(hcc_assemble_count);
/*lint +e19 */
/* 1 means hcc rx data process in hcc thread,
  0 means process in sdio thread */
oal_uint32 hcc_rx_thread_enable = 1;
oal_uint32 hcc_credit_bottom_value = 2;
oal_uint32 hcc_flowctrl_detect_panic = 0;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)||(_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
module_param(g_hcc_tx_max_buf_len, uint, S_IRUGO|S_IWUSR);
module_param(hcc_assemble_count, uint, S_IRUGO|S_IWUSR);
module_param(hcc_rx_thread_enable, uint, S_IRUGO|S_IWUSR);
module_param(hcc_credit_bottom_value, uint, S_IRUGO|S_IWUSR);
module_param(hcc_flowctrl_detect_panic, uint, S_IRUGO|S_IWUSR);
#endif
OAL_STATIC oal_int32 hcc_send_rx_queue(struct hcc_handler *hcc, hcc_queue_type type);
oal_uint32 hcc_queues_flow_ctrl_len(struct hcc_handler* hcc, hcc_chan_type dir);
OAL_STATIC  oal_void hcc_dev_flowctr_timer_del(struct hcc_handler *hcc);
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
OAL_STATIC  oal_int32 hcc_process_rx_queues(struct hcc_handler *hcc);
OAL_STATIC  oal_int32 hcc_process_tx_queues(struct hcc_handler *hcc);
#endif

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
oal_bool_enum_uint8 hcc_flowctl_get_device_mode(struct hcc_handler *hcc);
oal_void hcc_tx_network_start_subq(struct hcc_handler *hcc, oal_uint16 us_queue_idx);
oal_void hcc_tx_network_stop_subq(struct hcc_handler *hcc, oal_uint16 us_queue_idx);
#endif

struct hcc_handler* hcc_get_default_handler(oal_void)
{
    return hcc_tc;
}
/*lint -e19 */
oal_module_symbol(hcc_get_default_handler);
/*lint +e19 */
OAL_STATIC oal_void hcc_tx_netbuf_destory(struct hcc_handler* hcc)
{
    OAL_BUG_ON(hcc == NULL);
    oal_wake_unlock(&hcc->tx_wake_lock);
}

OAL_STATIC oal_void hcc_tx_netbuf_list_free(oal_netbuf_head_stru *pst_netbuf_hdr)
{
    oal_netbuf_stru *pst_netbuf = OAL_PTR_NULL;
    for (; ;) {
        pst_netbuf = oal_netbuf_delist(pst_netbuf_hdr);
        if (pst_netbuf == NULL)
            break;
        hcc_tx_netbuf_free(pst_netbuf);
    }
}

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
struct alg_process_func_handler* oal_get_alg_process_func(oal_void)
{
    return &g_pst_alg_process_func;
}
/*lint -e19 */
oal_module_symbol(oal_get_alg_process_func);
/*lint +e19 */
#endif

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
struct custom_process_func_handler* oal_get_custom_process_func(oal_void)
{
    return &g_pst_custom_process_func;
}
/*lint -e19 */
oal_module_symbol(oal_get_custom_process_func);
/*lint +e19 */
#endif

OAL_STATIC  oal_netbuf_stru* hcc_tx_assem_descr_get(struct hcc_handler *hcc)
{
    return  oal_netbuf_delist(&hcc->tx_descr_info.tx_assem_descr_hdr);
}

OAL_STATIC  oal_void hcc_tx_assem_descr_put(struct hcc_handler *hcc, oal_netbuf_stru* netbuf)
{
    oal_netbuf_list_tail(&hcc->tx_descr_info.tx_assem_descr_hdr, netbuf);
}

oal_void hcc_clear_next_assem_descr(struct hcc_handler *hcc,
                                    oal_netbuf_stru* descr_netbuf)
{
    OAL_REFERENCE(hcc);
    /* Just need clear the first bytes */
    *(oal_uint8 *)OAL_NETBUF_DATA(descr_netbuf) = 0;
}

#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
#ifdef CONFIG_PRINTK
oal_uint32 hcc_get_large_pkt_free_cnt(struct hcc_handler* hcc)
{
    return oal_sdio_get_large_pkt_free_cnt((struct oal_sdio *)hcc->hi_channel);
}
#endif
#endif
/* align_size must be power of 2 */
OAL_STATIC  oal_netbuf_stru*  hcc_netbuf_len_align(oal_netbuf_stru* netbuf,
                                                   oal_uint32 align_size)
{
    oal_int32 ret;
    oal_uint32 len_algin, tail_room_len;
    oal_uint32 len = OAL_NETBUF_LEN(netbuf);
    if (OAL_IS_ALIGNED(len, align_size)) {
        return netbuf;
    }
    /* align the netbuf */
    len_algin = OAL_ROUND_UP(len, align_size);

    OAL_BUG_ON(len_algin < len);

    tail_room_len = len_algin - len;

    if (OAL_UNLIKELY(tail_room_len > oal_netbuf_tailroom(netbuf))) {
#ifdef _PRE_LWIP_ZERO_COPY_DEBUG
        OAM_ERROR_LOG3(0, 0,
                       "[hcc_send_tx_q_expand_tail] need_tail_room=len_aglin[%d]-len[%d], have_tail_room = %d",
                       len_algin, len, oal_netbuf_tailroom(netbuf));
#endif
        /* tailroom not enough */
        ret = oal_netbuf_expand_head(netbuf, 0, (oal_int32)tail_room_len, GFP_KERNEL);
        if (OAL_WARN_ON(ret != OAL_SUCC)) {
            OAL_IO_PRINT("alloc head room failed,expand tail len=%d\n", tail_room_len);
            DECLARE_DFT_TRACE_KEY_INFO("netbuf align expand head fail", OAL_DFT_TRACE_FAIL);
            return NULL;
        }
    }

    oal_netbuf_put(netbuf, tail_room_len);

    OAL_BUG_ON(!OAL_IS_ALIGNED(OAL_NETBUF_LEN(netbuf), align_size));

    return netbuf;
}

OAL_STATIC oal_void hcc_build_next_assem_descr(struct hcc_handler *hcc,
                                               hcc_queue_type type,
                                               oal_netbuf_head_stru *head,
                                               oal_netbuf_head_stru* next_assembled_head,
                                               oal_netbuf_stru* descr_netbuf,
                                               oal_uint32 remain_len)
{
    oal_int32 i = 0;
    oal_int32 len;
    oal_uint8 *buf;
    oal_netbuf_stru *netbuf = OAL_PTR_NULL;
    oal_netbuf_stru *netbuf_t = OAL_PTR_NULL;
    oal_uint32 assemble_max_count, queue_len, current_trans_len;

    buf = (oal_uint8*)OAL_NETBUF_DATA(descr_netbuf);
    len = (oal_int32)OAL_NETBUF_LEN(descr_netbuf);

    OAL_BUG_ON(!oal_netbuf_list_empty(next_assembled_head));

    assemble_max_count = OAL_MAX(1, hcc_assemble_count);
    queue_len = oal_netbuf_list_len(head);
    current_trans_len = OAL_MIN(queue_len, assemble_max_count);
    current_trans_len = OAL_MIN(current_trans_len, remain_len);

    buf[0] = 0;

    if (current_trans_len == 0) {
        return;
    }

    for (; ;) {
        /* move the netbuf from head queue to prepare-send queue, head->tail */
        netbuf = oal_netbuf_delist(head);
        if (netbuf == NULL) {
            DECLARE_DFT_TRACE_KEY_INFO("buid assem error", OAL_DFT_TRACE_FAIL);
            break;
        }

        /* align the buff len to 32B */
        netbuf_t  = hcc_netbuf_len_align(netbuf, HISDIO_H2D_SCATT_BUFFLEN_ALIGN);
        if (netbuf_t == NULL) {
            /* return to the list */
            oal_netbuf_addlist(head, netbuf);
            break;
        }

        current_trans_len--;

        oal_netbuf_list_tail(next_assembled_head, netbuf_t);
        if (OAL_UNLIKELY(i >= len)) {
            OAL_BUG_ON(1);
        }

        buf[i++] = (oal_uint8)(OAL_NETBUF_LEN(netbuf) >> HISDIO_H2D_SCATT_BUFFLEN_ALIGN_BITS);
        if (current_trans_len == 0) {
            /* send empty */
            if (i != len) {
                buf[i] = 0;
            }
            break;
        }
    }

    if (OAL_LIKELY(!oal_netbuf_list_empty(next_assembled_head)))
        hcc->hcc_transer_info.tx_assem_info.assembled_queue_type = type;
}

oal_int32 hcc_tx_param_check(struct hcc_handler* hcc,
                             struct hcc_transfer_param* param)
{
    if (OAL_UNLIKELY(param->extend_len > hcc->hdr_rever_max_len)) {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        WARN(1, "invaild reserved len %u ,never should over %zu\n",
             param->extend_len, HCC_HDR_RESERVED_MAX_LEN);
#endif
        /* can't free buf here */
        return -OAL_EINVAL;
    }

    if (OAL_WARN_ON(param->main_type >= HCC_ACTION_TYPE_BUTT)) {
        OAL_IO_PRINT("wrong main type:%d\n", param->main_type);
        return -OAL_EINVAL;
    }

    if (OAL_WARN_ON((int32_t)(param->fc_flag & (~HCC_FC_ALL)))) {
        OAL_IO_PRINT("wrong fc_flag:%d\n", param->fc_flag);
        return -OAL_EINVAL;
    }

    if (OAL_WARN_ON(param->queue_id >= HCC_QUEUE_COUNT)) {
        OAL_IO_PRINT("wrong queue_id:%d\n", param->queue_id);
        return -OAL_EINVAL;
    }
    return OAL_SUCC;
}

oal_void hcc_tx_network_stopall_queues(struct hcc_handler *hcc)
{
    OAL_BUG_ON(hcc == NULL);
    if (OAL_LIKELY(hcc->hcc_transer_info.tx_flow_ctrl.net_stopall)) {
        hcc->hcc_transer_info.tx_flow_ctrl.net_stopall();
    }
}

oal_void hcc_tx_network_startall_queues(struct hcc_handler *hcc)
{
    OAL_BUG_ON(hcc == NULL);
    if (OAL_LIKELY(hcc->hcc_transer_info.tx_flow_ctrl.net_startall)) {
        hcc->hcc_transer_info.tx_flow_ctrl.net_startall();
    }
}


oal_void hcc_tx_flow_ctrl_cb_register(flowctrl_cb stopall, flowctrl_cb startall)
{
    if (hcc_tc == NULL) {
        OAL_IO_PRINT("[ERROR]hcc_tx_flow_ctrl_cb_register failed! hcc_tc is NULL\n");
        return;
    }
    hcc_tc->hcc_transer_info.tx_flow_ctrl.net_stopall = stopall;
    hcc_tc->hcc_transer_info.tx_flow_ctrl.net_startall = startall;
}

/*lint -e19 */
oal_module_symbol(hcc_tx_flow_ctrl_cb_register);
/*lint +e19 */
oal_void hcc_tx_wlan_queue_map_set(struct hcc_handler* hcc, hcc_queue_type hcc_queue_id,
    wlan_net_queue_type wlan_queue_id)
{
    hcc_trans_queue *pst_hcc_queue = OAL_PTR_NULL;
    if (OAL_WARN_ON(hcc == NULL)) {
        return;
    }

    if (OAL_WARN_ON(hcc_queue_id >= HCC_QUEUE_COUNT || wlan_queue_id >= WLAN_NET_QUEUE_BUTT)) {
        OAL_IO_PRINT("invaild param,hcc id:%d,wlan id:%d\n", (oal_int32)hcc_queue_id, (oal_int32)wlan_queue_id);
        return;
    }
    pst_hcc_queue = &hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[hcc_queue_id];
    pst_hcc_queue->wlan_queue_id = wlan_queue_id;//lint !e661
}

/*lint -e19 */
oal_module_symbol(hcc_tx_wlan_queue_map_set);
/*lint +e19 */
oal_void hcc_msg_slave_thruput_bypass(oal_void)
{
    OAL_IO_PRINT("hcc_msg_slave_thruput_bypass. \n");
    oal_channel_send_msg(hcc_tc->hi_channel, H2D_MSG_HCC_SLAVE_THRUPUT_BYPASS);
}
#ifndef _PRE_PC_LINT
oal_module_symbol(hcc_msg_slave_thruput_bypass);
#endif

OAL_STATIC oal_uint32 hcc_check_header_vaild(struct hcc_header *hdr)
{
    if (OAL_UNLIKELY(hdr->main_type >= HCC_ACTION_TYPE_BUTT)
        || (HCC_HDR_LEN + hdr->pad_hdr + hdr->pad_payload > HCC_HDR_TOTAL_LEN)) {
        DECLARE_DFT_TRACE_KEY_INFO("hcc_check_header_vaild_fail", OAL_DFT_TRACE_FAIL);
        OAM_ERROR_LOG2(0, 0, "[hcc_check_header_vaild] pad_hdr = %d, pad_payload = %d", hdr->pad_hdr, hdr->pad_payload);
        return OAL_FALSE;
    }
    return OAL_TRUE;
}


oal_int32 hcc_tx(struct hcc_handler* hcc, oal_netbuf_stru* netbuf,
                 struct hcc_transfer_param* param)
{
    oal_uint32 queue_id;
    oal_int32 ret;
    oal_ulong  pad_payload, headroom,
               payload_len, payload_addr,
               pad_hdr;
    oal_uint8* old_data_addr;
    oal_int32 headroom_add = 0;
    struct hcc_header *hdr;
    hcc_trans_queue *pst_hcc_queue;
    struct hcc_tx_cb_stru* pst_cb_stru;

    if (OAL_ANY_NULL_PTR3(param, netbuf, hcc)) {
        return -OAL_EINVAL;
    }

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)||(_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    if (OAL_UNLIKELY(oal_atomic_read(&hcc->state) != HCC_ON)) {
        if (oal_atomic_read(&hcc->state) == HCC_OFF) {
            return -OAL_EBUSY;
        } else if(oal_atomic_read(&hcc->state) == HCC_EXCEPTION) {
            return -OAL_EIO;
        } else {
            OAL_IO_PRINT("invaild hcc state:%d\n", oal_atomic_read(&hcc->state));
            return -OAL_EINVAL;
        }
    }
#endif

    if (OAL_UNLIKELY(hcc_tx_param_check(hcc, param) != OAL_SUCC)) {
        return -OAL_EINVAL;
    }

    /*lint -e522*/
    OAL_WARN_ON(OAL_NETBUF_LEN(netbuf) == 0);
    /*lint +e522*/
    queue_id = param->queue_id;
    pst_hcc_queue = &hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[queue_id];

    if (OAL_WARN_ON((oal_uint64)(param->fc_flag & HCC_FC_WAIT) && (oal_uint64)oal_in_interrupt())) {
        /* when in interrupt,can't sched! */
        param->fc_flag &= ~HCC_FC_WAIT;
    }

    if (param->fc_flag) {
        /* flow control process */
        /* Block if fc */
        if (HCC_FC_WAIT & param->fc_flag) {
#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
            /*lint -e730*/
            ret = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(hcc->hcc_transer_info.tx_flow_ctrl.wait_queue,
                                                       (oal_netbuf_list_len(&pst_hcc_queue->data_queue)
                                                        <  pst_hcc_queue->flow_ctrl.low_waterline),
                                                       60 * OAL_TIME_HZ);//lint !e665 !e666

#else
            /*lint -e730*/
            ret = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(hcc->hcc_transer_info.tx_flow_ctrl.wait_queue,
                                                       hcc_queues_flow_ctrl_len(hcc, HCC_TX) < HCC_TX_FLOW_LO_LEVEL,
                                                       60 * OAL_TIME_HZ);
#endif
            if (ret == 0) {
                OAL_IO_PRINT("[WARN]hcc flow control wait event timeout! too much time locked\n");
                DECLARE_DFT_TRACE_KEY_INFO("hcc flow control wait timeout", OAL_DFT_TRACE_FAIL);
            } else if(ret == -ERESTARTSYS) {
                OAL_IO_PRINT("wifi task was interrupted by a signal\n");
                return -OAL_EFAIL;
            }
        }

        /* control net layer if fc */
        if (HCC_FC_NET & param->fc_flag) {
            /* net layer?? */
            oal_spin_lock_bh(&hcc->hcc_transer_info.tx_flow_ctrl.lock);
#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
            if ((oal_netbuf_list_len(&pst_hcc_queue->data_queue) > pst_hcc_queue->flow_ctrl.high_waterline)
                && (pst_hcc_queue->flow_ctrl.is_stopped == OAL_FALSE)) {
                // OAM_INFO_LOG3(0, OAM_SF_TX, "{hcc_tx: stop_netdev_queue, queue_len[%d] = %d}, thread = %d", queue_id,
                hcc_tx_network_stop_subq(hcc, (oal_uint16)pst_hcc_queue->wlan_queue_id);
                pst_hcc_queue->flow_ctrl.is_stopped = OAL_TRUE;
            }

#else
            if (hcc_queues_flow_ctrl_len(hcc, HCC_TX) > HCC_TX_FLOW_HI_LEVEL) {
                hcc_tx_network_stopall_queues(hcc);
            }
#endif
            oal_spin_unlock_bh(&hcc->hcc_transer_info.tx_flow_ctrl.lock);
        }

        /* control net layer if fc */
        if (HCC_FC_DROP & param->fc_flag) {
            /* 10 netbufs to buff */
            if (oal_netbuf_list_len(&pst_hcc_queue->data_queue) > pst_hcc_queue->flow_ctrl.high_waterline + 100) {
                pst_hcc_queue->loss_pkts++;
                /* drop the netbuf */
                oal_netbuf_free(netbuf);
                return OAL_SUCC;
            }
        }
    }

    if (OAL_GET_THRUPUT_BYPASS_ENABLE(OAL_TX_SDIO_HOST_BYPASS)) {
        oal_netbuf_free(netbuf);
        return OAL_SUCC;
    }

    /* include extend data */
    payload_len = OAL_NETBUF_LEN(netbuf);

    old_data_addr = OAL_NETBUF_DATA(netbuf);

    payload_addr = (oal_ulong)(uintptr_t)OAL_NETBUF_DATA(netbuf) + param->extend_len;

    /* if pad not 0, we must copy the extend data */
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    pad_payload = payload_addr - OAL_ROUND_DOWN(payload_addr, 32);
#else
    pad_payload = payload_addr - OAL_ROUND_DOWN(payload_addr, 4);
#endif
    pad_hdr = HCC_HDR_RESERVED_MAX_LEN - param->extend_len;

    payload_len -= param->extend_len;

    headroom = HCC_HDR_LEN + pad_hdr + pad_payload;

    if (headroom > oal_netbuf_headroom(netbuf)) {
        headroom_add = (oal_int32) (headroom - oal_netbuf_headroom(netbuf));
    }

    if (headroom_add) {
#ifdef _PRE_LWIP_ZERO_COPY_DEBUG
        OAM_ERROR_LOG2(0, 0,
                       "[hcc_tx] headroom not enough! headroom = %d, add_room = %d",
                       oal_netbuf_headroom(netbuf), headroom_add);
#endif
        /* relloc the netbuf */
        ret = oal_netbuf_expand_head(netbuf, headroom_add, 0, GFP_ATOMIC);
        if (OAL_UNLIKELY(ret != OAL_SUCC)) {
            OAL_IO_PRINT("alloc head room failed,netbuf len is %d,expand len:%d\n",
                         OAL_NETBUF_LEN(netbuf), headroom_add);
            return -OAL_EFAIL;
        }
        old_data_addr = OAL_NETBUF_DATA(netbuf);
    }

    hdr = (struct hcc_header *)oal_netbuf_push(netbuf, HCC_HDR_LEN + pad_payload + pad_hdr);
    if (OAL_UNLIKELY(hdr == NULL)) {
        return -OAL_EFAIL;
    }
    OAL_BUG_ON(!OAL_IS_ALIGNED((uintptr_t)hdr, 4));

    if (pad_payload > 0) {
        /* algin the extend data */
        if (memmove_s(old_data_addr - pad_payload, payload_len + param->extend_len, old_data_addr,
                      (payload_len + param->extend_len)) != EOK) {
            OAL_IO_PRINT("hcc_tx::memmove_s failed ! line[643] \n");
        }
    }

    /* fill the hcc header */
    hdr->pad_payload = (oal_uint16)pad_payload;
    hdr->pay_len = (oal_uint16)payload_len;
    hdr->seq = ((oal_uint32)(oal_atomic_inc_return(&hcc->tx_seq))) & 0xFF;;
    hdr->main_type = (oal_uint8) param->main_type;
    hdr->sub_type = (oal_uint8) param->sub_type;
    hdr->pad_hdr = (oal_uint8)pad_hdr;
    hdr->more = 0;
    hdr->option = 0;

    OAL_BUG_ON(hdr->pad_hdr + HCC_HDR_LEN  > HCC_HDR_TOTAL_LEN);

    if (OAL_WARN_ON(hdr->pay_len > g_hcc_tx_max_buf_len)) {
        /* pay_len超过DEVICE 最大内存长度 */
        OAL_IO_PRINT("[ERROR]main:%d, sub:%d,pay len:%d,netbuf len:%d, extend len:%d,pad_payload:%d,max len:%u\n",
                     hdr->main_type,
                     hdr->sub_type,
                     hdr->pay_len,
                     hdr->pay_len + param->extend_len,
                     param->extend_len,
                     hdr->pad_payload,
                     g_hcc_tx_max_buf_len);
        DECLARE_DFT_TRACE_KEY_INFO("hcc_tx_check_param_fail", OAL_DFT_TRACE_FAIL);
        return -OAL_EINVAL;
    } else {
        /* 当长度 在1544 + [1~3] 之内， 牺牲性能,将payload 内存前移 1~3B，节省DEVICE内存 */
        if (hdr->pad_payload + hdr->pay_len > g_hcc_tx_max_buf_len) {
            oal_uint8* pst_dst =  (oal_uint8*)hdr + HCC_HDR_TOTAL_LEN;
            oal_uint8* pst_src = pst_dst + hdr->pad_payload;
            if (memmove_s((void*)pst_dst, HCC_HDR_TOTAL_LEN, (const void *)pst_src, hdr->pay_len) != EOK) {
                OAL_IO_PRINT("hcc_tx::memmove_s failed ! line[680] \n");
            }
            oal_netbuf_trim(netbuf, hdr->pad_payload);
            hdr->pad_payload = 0; /* after memmove,the pad is 0 */
            DECLARE_DFT_TRACE_KEY_INFO("hcc_tx_check_memmove", OAL_DFT_TRACE_SUCC);
        }
    }

    if (OAL_WARN_ON(g_hcc_tx_max_buf_len + HCC_HDR_TOTAL_LEN
                    < OAL_ROUND_UP(OAL_NETBUF_LEN(netbuf), HISDIO_H2D_SCATT_BUFFLEN_ALIGN))) {
        OAL_IO_PRINT("[E]Fatal error,netbuf's len:%d over the payload:%d\n",
                     OAL_ROUND_UP(OAL_NETBUF_LEN(netbuf), HISDIO_H2D_SCATT_BUFFLEN_ALIGN),
                     g_hcc_tx_max_buf_len + HCC_HDR_TOTAL_LEN);
        DECLARE_DFT_TRACE_KEY_INFO("hcc_tx_check_algin_fail", OAL_DFT_TRACE_FAIL);
        return -OAL_EINVAL;
    }

#ifdef CONFIG_HCC_DEBUG
    OAL_IO_PRINT("hcc_tx into queue:%d, main:%d, sub:%d\n",
                 queue_id, param->main_type, param->sub_type);
    OAL_IO_PRINT("hcc tx pkt seq:%d\n", hdr->seq);
    oal_print_hex_dump(OAL_NETBUF_DATA(netbuf), HCC_HDR_TOTAL_LEN, 8, "hcc hdr");
    oal_print_hex_dump(OAL_NETBUF_DATA(netbuf) + HCC_HDR_TOTAL_LEN + hdr->pad_payload,
                       hdr->pay_len, 8, "hcc payload");
#endif

    /* android wakelock,one netbuf one lock */
    oal_wake_lock(&hcc->tx_wake_lock);

    pst_cb_stru = (struct hcc_tx_cb_stru*)OAL_NETBUF_CB(netbuf);
    pst_cb_stru->destory = hcc_tx_netbuf_destory;
    pst_cb_stru->magic = HCC_TX_WAKELOCK_MAGIC;
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    UINTPTR uvIntSave;
    uvIntSave = LOS_IntLock();
#endif
    oal_netbuf_list_tail(&pst_hcc_queue->data_queue, netbuf);
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    LOS_IntRestore(uvIntSave);
#endif

    hcc_sched_transfer(hcc);

    return OAL_SUCC;
}
/*lint -e19 */
oal_module_symbol(hcc_tx);
/*lint +e19 */

OAL_STATIC  oal_void hcc_transfer_rx_handler(struct hcc_handler* hcc, oal_netbuf_stru* netbuf)
{
    /* get the rx buf and enqueue */
    oal_netbuf_list_tail(&hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[DATA_LO_QUEUE].data_queue,
                         netbuf);
    if (hcc_rx_thread_enable == 0)
        hcc_send_rx_queue(hcc, DATA_LO_QUEUE);
}


OAL_STATIC  oal_void hcc_transfer_rx_handler_replace(struct hcc_handler* hcc, oal_netbuf_stru* pst_netbuf_new)
{
    oal_netbuf_stru *pst_netbuf_old;
    hcc_trans_queue *pst_hcc_queue = &hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[DATA_LO_QUEUE];

    /* remove from head */
    pst_netbuf_old = oal_netbuf_delist(&pst_hcc_queue->data_queue);
    if (OAL_LIKELY(pst_netbuf_old != NULL)) {
        pst_hcc_queue->loss_pkts++;
        oal_netbuf_free(pst_netbuf_old);
    }

    /* get the rx buf and enqueue */
    oal_netbuf_list_tail(&pst_hcc_queue->data_queue,
                         pst_netbuf_new);

    if (hcc_rx_thread_enable == 0)
        hcc_send_rx_queue(hcc, DATA_LO_QUEUE);
}

static void hcc_rx_netbuf_list_handler(struct hcc_handler *hcc, oal_netbuf_head_stru *head)
{
    oal_uint32 scatt_count;
    oal_netbuf_stru *netbuf = OAL_PTR_NULL;
    struct hcc_header *pst_hcc_head = OAL_PTR_NULL;

    OAL_BUG_ON(!hcc);

    scatt_count = oal_netbuf_list_len(head);
    if (OAL_UNLIKELY(scatt_count > HISDIO_DEV2HOST_SCATT_MAX)) {
        OAL_IO_PRINT("WARN:why so much scatt buffs[%u]?\n", scatt_count);
        scatt_count = 0;
    }
    hcc->hcc_transer_info.rx_assem_info.info[scatt_count]++;

    for (; ;) {
        netbuf = oal_netbuf_delist(head);
        if (netbuf == NULL)
            break;

        pst_hcc_head = (struct hcc_header *)OAL_NETBUF_DATA(netbuf);
        if (OAL_UNLIKELY(hcc_check_header_vaild(pst_hcc_head) != OAL_TRUE)) {
            OAL_IO_PRINT("hcc_check_header_vaild ERROR\n");
            oal_print_hex_dump((oal_uint8 *)pst_hcc_head, HCC_HDR_TOTAL_LEN, 16, "invaild hcc header: ");
            oal_try_to_dump_device_mem(OAL_FALSE);
        }

        if (hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[DATA_LO_QUEUE].flow_ctrl.enable == OAL_TRUE) {
            if (oal_netbuf_list_len(&hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[DATA_LO_QUEUE].data_queue)
                > hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[DATA_LO_QUEUE].flow_ctrl.high_waterline) {
                hcc_transfer_rx_handler_replace(hcc, netbuf);
            } else {
                hcc_transfer_rx_handler(hcc, netbuf);
            }
        } else {
            hcc_transfer_rx_handler(hcc, netbuf);
        }
    }

    /* sched hcc thread */
    if (hcc_rx_thread_enable == 1) {
        hcc_sched_transfer(hcc);
    }
}


#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)

oal_int32 sdio_transfer_rx_handler(oal_void* data)
{
    oal_int32 ret;
    oal_netbuf_head_stru    head;
    struct hcc_handler* hcc = (struct hcc_handler*)data;

    OAL_BUG_ON(!hcc);

    oal_netbuf_head_init(&head);

    ret = oal_sdio_build_rx_netbuf_list(hcc->hi_channel, &head);
    if (ret != OAL_SUCC) {
        return ret;
    }

    oal_channel_claim_host(hcc->hi_channel);

    ret = oal_sdio_transfer_netbuf_list(hcc->hi_channel, &head, SDIO_READ);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        OAL_IO_PRINT("Failed to read scatt sdio![scatt len:%d]\n", oal_netbuf_list_len(&head));
        oal_channel_release_host(hcc->hi_channel);
        return -OAL_EFAIL;
    }

    oal_channel_release_host(hcc->hi_channel);

#ifdef  CONFIG_SDIO_DEBUG
    oal_netbuf_list_hex_dump(&head);
#endif

    hcc_rx_netbuf_list_handler(hcc, &head);

#ifdef CONFIG_HCC_DEBUG
    // oal_msleep(500);
#endif
    return OAL_SUCC;
}

#endif
oal_int32 sdio_credit_info_update_handler(oal_void* data)
{
#if 0
    struct hcc_handler* hcc = (struct hcc_handler*)data;
    if (hcc_get_large_pkt_free_cnt(hcc) > CONFIG_CREDIT_MSG_FLOW_WATER_LINE)
        hcc_sched_transfer(hcc);
#endif
    return OAL_SUCC;
}

/*
 * check flow_ctrl condition of each hcc queue
 * return:
 *      OAL_TRUE, when flow_ctrl is not working
 *      OAL_FALSE, when flow_ctrl is working
 */
OAL_STATIC oal_int32 _queues_flow_ctrl_check(struct hcc_handler *hcc, hcc_chan_type dir)
{
    oal_int32 i;
    hcc_trans_queue *p_queue;
    hcc_tx_flow_ctrl_info *p_flow_ctrl;

    p_queue = hcc->hcc_transer_info.hcc_queues[dir].queues;
    p_flow_ctrl = &hcc->hcc_transer_info.tx_flow_ctrl;
    for (i = 0; i < HCC_QUEUE_COUNT; i++, p_queue++) {
        /*
         * queue flow_ctrl function check
         *  flow_ctrl function is enabled
         */
        if (p_queue->flow_ctrl.enable == OAL_TRUE) {
            /*
             * Normal priority queue
             * flow_ctrl is disabled
             */
            if (p_queue->flow_ctrl.flow_type == HCC_FLOWCTRL_SDIO) {
                if (oal_netbuf_list_len(&p_queue->data_queue) &&
                    (p_flow_ctrl->flowctrl_flag == D2H_MSG_FLOWCTRL_ON)) {
                    return OAL_TRUE;
                }
            }
        } else { /* flow_ctrl function is disabled */
            /* Normal priority queue */
            if (p_queue->flow_ctrl.flow_type == HCC_FLOWCTRL_SDIO) {
                /* queue length is not zero */
                if (oal_netbuf_list_len(&p_queue->data_queue)) {
                    return OAL_TRUE;
                }
            } else { /* credit queue */
                /* queue length is not zero */
                if (oal_netbuf_list_len(&p_queue->data_queue) &&
                    (p_flow_ctrl->uc_hipriority_cnt > hcc_credit_bottom_value)) {
                    return OAL_TRUE;
                }
            }
        }
    }

    return OAL_FALSE;
}

oal_uint32 hcc_queues_flow_ctrl_len(struct hcc_handler* hcc, hcc_chan_type dir)
{
    oal_int32 i;
    oal_uint32 total;
    hcc_trans_queue *p_queue;

    p_queue = hcc->hcc_transer_info.hcc_queues[dir].queues;
    for (i = 0, total = 0; i < HCC_QUEUE_COUNT; i++, p_queue++) {
        if (p_queue->flow_ctrl.enable == OAL_TRUE) {
            total += oal_netbuf_list_len(&p_queue->data_queue);
        }
    }

    return total;
}

/* Whether the hcc queue have data */
OAL_STATIC oal_int32 _queues_len_check(struct hcc_handler* hcc, hcc_chan_type dir)
{
    oal_int32 i;
    hcc_trans_queue *p_queue;

    p_queue = hcc->hcc_transer_info.hcc_queues[dir].queues;

    for (i = 0; i < HCC_QUEUE_COUNT; i++, p_queue++) {
        if (oal_netbuf_list_len(&p_queue->data_queue)) {
            return OAL_TRUE;
        }
    }
    return OAL_FALSE;
}


OAL_STATIC  oal_int32  hcc_thread_wait_event_cond_check(struct hcc_handler* hcc)
{
    oal_int32 ret;

    OAL_BUG_ON(!hcc);

    /*
     * please first check the condition
     * which may be ok likely to reduce the cpu mips
     */
    ret = _queues_flow_ctrl_check(hcc, HCC_TX)
          || (_queues_len_check(hcc, HCC_RX))
          || (hcc->p_hmac_tcp_ack_need_schedule_func() == OAL_TRUE);
#ifdef _PRE_CONFIG_WLAN_THRANS_THREAD_DEBUG
    if (ret == OAL_TRUE)
        hcc->hcc_transer_info.thread_stat.wait_event_run_count++;
    if (ret == OAL_FALSE)
        hcc->hcc_transer_info.thread_stat.wait_event_block_count++;
#endif
    return ret;
}

#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
OAL_STATIC oal_int32 hcc_send_single_descr(struct hcc_handler *hcc, oal_netbuf_stru* netbuf)
{
    oal_int32 ret;
    oal_netbuf_head_stru head_send;
    OAL_REFERENCE(hcc);
    OAL_BUG_ON(netbuf == NULL);
    oal_netbuf_list_head_init(&head_send);
    oal_netbuf_list_tail(&head_send, netbuf);
    ret = oal_sdio_transfer_netbuf_list(hcc->hi_channel, &head_send, SDIO_WRITE);
    return ret;
}
#endif

oal_int32 hcc_send_descr_control_data(struct hcc_handler *hcc, hcc_descr_type descr_type,
                                      const oal_void* data, oal_uint32 ul_size)
{
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    oal_int32 ret = OAL_SUCC;
    oal_netbuf_stru* netbuf = NULL;
    struct hcc_descr_header* dscr_hdr = OAL_PTR_NULL;
    netbuf = hcc_tx_assem_descr_get(hcc);
    if (OAL_WARN_ON(netbuf == NULL)) {
        OAL_IO_PRINT("[Error]No descr mem!\n");
        return -OAL_ENOMEM;
    }

    dscr_hdr = (struct hcc_descr_header*)OAL_NETBUF_DATA(netbuf);
    dscr_hdr->descr_type = descr_type;

    if (ul_size) {
        if (OAL_WARN_ON(data == NULL)) {
            hcc_tx_assem_descr_put(hcc, netbuf);
            return -OAL_EINVAL;
        }
        if (OAL_WARN_ON(ul_size + OAL_SIZEOF(struct hcc_descr_header) > OAL_NETBUF_LEN(netbuf))) {
            OAL_IO_PRINT("invaild request size:%u,max size:%u\r\n",
                         (oal_uint32)(ul_size + OAL_SIZEOF(struct hcc_descr_header)),
                         (oal_uint32)OAL_NETBUF_LEN(netbuf));
            hcc_tx_assem_descr_put(hcc, netbuf);
            return -OAL_EINVAL;
        }

        /*lint -e124*/
        ret = memcpy_s((oal_void *)((oal_uint8 *)OAL_NETBUF_DATA(netbuf) + OAL_SIZEOF(struct hcc_descr_header)),
                       OAL_NETBUF_LEN(netbuf) - sizeof(struct hcc_descr_header), data, ul_size);
        if (ret != EOK) {
            hcc_tx_assem_descr_put(hcc, netbuf);
            OAL_IO_PRINT("hcc_send_descr_control_data::memcpy_s fail!");
            return OAL_EFAIL;
        }
    }

    ret = hcc_send_single_descr(hcc, netbuf);

    hcc_tx_assem_descr_put(hcc, netbuf);
    return ret;
#else
    return OAL_SUCC;
#endif
}


oal_int32 hcc_tx_netbuf_queue_switch(struct hcc_handler *hcc, hcc_netbuf_queue_type queue_type)
{
    return hcc_send_descr_control_data(hcc, HCC_NETBUF_QUEUE_SWITCH, &queue_type, OAL_SIZEOF(queue_type));
}

oal_int32 hcc_tx_netbuf_test_and_switch_high_pri_queue(struct hcc_handler *hcc, hcc_netbuf_queue_type pool_type)
{
    oal_int32 ret = OAL_EFAIL;
    if (pool_type == HCC_NETBUF_HIGH_QUEUE) {
        ret = hcc_tx_netbuf_queue_switch(hcc, HCC_NETBUF_HIGH_QUEUE);
    }
    return ret;
}

oal_int32 hcc_tx_netbuf_restore_normal_pri_queue(struct hcc_handler *hcc, hcc_netbuf_queue_type pool_type)
{
    oal_int32 ret = OAL_EFAIL;
    if (pool_type == HCC_NETBUF_HIGH_QUEUE) {
        ret = hcc_tx_netbuf_queue_switch(hcc, HCC_NETBUF_NORMAL_QUEUE);
    }
    return ret;
}

OAL_STATIC oal_void  hcc_restore_assemble_netbuf_list(struct hcc_handler *hcc)
{
    hcc_queue_type type;
    oal_netbuf_head_stru *assembled_head;

    type = hcc->hcc_transer_info.tx_assem_info.assembled_queue_type;
    assembled_head = &hcc->hcc_transer_info.tx_assem_info.assembled_head;

    if (type >= HCC_QUEUE_COUNT) {
        type = DATA_LO_QUEUE;
    }

    if (!oal_netbuf_list_empty(assembled_head))
        oal_netbuf_splice_sync(&hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[type].data_queue,
                               assembled_head);
}

OAL_STATIC  oal_int32 hcc_send_assemble_reset(struct hcc_handler *hcc)
{
    oal_int32 ret;

    hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_reset_count++;

    /* 当只发送一个聚合描述符包，并且聚合个数为0描述通知Device 重置聚合信息 */
    ret = hcc_send_descr_control_data(hcc, HCC_DESCR_ASSEM_RESET, NULL, 0);

    hcc_restore_assemble_netbuf_list(hcc);

    return ret;
}

OAL_STATIC oal_int32 hcc_send_data_packet(struct hcc_handler* hcc,
                                          oal_netbuf_head_stru *head,
                                          hcc_queue_type type,
                                          oal_netbuf_head_stru *next_assembled_head,
                                          hcc_send_mode mode,
                                          oal_uint32 *remain_len)
{
    oal_uint8 *buf = NULL;
    oal_uint32 total_send;
    oal_int32 ret = OAL_SUCC;
    oal_netbuf_head_stru head_send;
    oal_netbuf_stru *netbuf = OAL_PTR_NULL;
    oal_netbuf_stru *descr_netbuf = OAL_PTR_NULL;
    oal_netbuf_stru *netbuf_t = OAL_PTR_NULL;
    oal_uint32 *info = OAL_PTR_NULL;
    hcc_trans_queue *pst_hcc_queue = OAL_PTR_NULL;
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    oal_uint8 uc_credit;
#endif

    if (*remain_len == 0) {
        return OAL_SUCC;
    }

#ifdef CONFIG_HCC_DEBUG
    OAL_IO_PRINT("send queue:%d, mode:%d,next assem len:%d\n",
                 type, mode,
                 oal_netbuf_list_len(next_assembled_head));
#endif

    pst_hcc_queue = &hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[type];
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    if (pst_hcc_queue->flow_ctrl.flow_type == HCC_FLOWCTRL_CREDIT) {
        /* credit flowctrl */
        uc_credit = hcc->hcc_transer_info.tx_flow_ctrl.uc_hipriority_cnt;

        /* 高优先级流控: credit值为0时不发送 */
        if (!(uc_credit > hcc_credit_bottom_value)) {
            return OAL_SUCC;
        }
    }
#endif
    descr_netbuf = hcc_tx_assem_descr_get(hcc);
    if (descr_netbuf == NULL) {
        ret = -OAL_ENOMEM;
        /*lint -e801*/
        goto failed_get_assem_descr;
    }

    info = hcc->hcc_transer_info.tx_assem_info.info;

    oal_netbuf_list_head_init(&head_send);

    if (oal_netbuf_list_empty(next_assembled_head)) {
        /* single send */
        netbuf = oal_netbuf_delist(head);
        if (netbuf == NULL) {
            OAL_IO_PRINT("netbuf is NULL [len:%d]\n", oal_netbuf_list_len(head));
            ret = OAL_SUCC;
            /*lint -e801*/
            goto failed_get_sig_buff;
        }

        netbuf_t = hcc_netbuf_len_align(netbuf, HISDIO_H2D_SCATT_BUFFLEN_ALIGN);
        if (OAL_UNLIKELY(netbuf_t == NULL)) {
            /* return to the list */
            oal_netbuf_addlist(head, netbuf);
            /*lint -e801*/
            goto failed_align_netbuf;
        }

        oal_netbuf_list_tail(&head_send, netbuf);
        info[0]++;
    } else {
        oal_uint32 assemble_len = oal_netbuf_list_len(next_assembled_head);
        OAL_BUG_ON(assemble_len > HISDIO_HOST2DEV_SCATT_SIZE);
        /* move the assem list to send queue */
        oal_netbuf_splice_init(next_assembled_head, &head_send);
        info[assemble_len]++;
    }

    total_send = oal_netbuf_list_len(&head_send);
    if (*remain_len >= total_send)
        *remain_len -= total_send;
    else
        *remain_len = 0;

    OAL_BUG_ON(!oal_netbuf_list_empty(next_assembled_head));

    if (hcc_assem_send == mode) {
        hcc_build_next_assem_descr(hcc, type, head, next_assembled_head,
                                   descr_netbuf, *remain_len);
    } else {
        buf = OAL_NETBUF_DATA(descr_netbuf);
        *((oal_uint32*) buf) = 0;
    }

    /* add the assem descr buf */
    oal_netbuf_addlist(&head_send, descr_netbuf);
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    ret = oal_sdio_transfer_netbuf_list(hcc->hi_channel, &head_send, SDIO_WRITE);
#endif

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
    if (g_pst_alg_process_func.p_auto_freq_count_func != OAL_PTR_NULL) {
        g_pst_alg_process_func.p_auto_freq_count_func(total_send);
    }
#endif

    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[type].total_pkts += total_send;

#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    /* 高优先级流控: 更新credit值 */
    if (pst_hcc_queue->flow_ctrl.flow_type == HCC_FLOWCTRL_CREDIT) {
        oal_spin_lock(&(hcc->hcc_transer_info.tx_flow_ctrl.st_hipri_lock));

        uc_credit = hcc->hcc_transer_info.tx_flow_ctrl.uc_hipriority_cnt;
        uc_credit = (uc_credit > total_send) ? (oal_uint8)(uc_credit - total_send) : 0;
        hcc->hcc_transer_info.tx_flow_ctrl.uc_hipriority_cnt = uc_credit;

        oal_spin_unlock(&(hcc->hcc_transer_info.tx_flow_ctrl.st_hipri_lock));
    }
#endif

    descr_netbuf = oal_netbuf_delist(&head_send);
    if (descr_netbuf == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hcc_send_data_packet::oal_netbuf_delist fail.descr_netbuf is NULL.");
        ret = OAL_SUCC;
        goto failed_get_assem_descr;
    }

    hcc_tx_assem_descr_put(hcc, descr_netbuf);

    /* free the sent netbuf,release the wakelock */
    hcc_tx_netbuf_list_free(&head_send);

    return ret;

failed_align_netbuf:
failed_get_sig_buff:
    hcc_tx_assem_descr_put(hcc, descr_netbuf);

failed_get_assem_descr:
    return ret;
}

oal_void hcc_clear_all_queues(struct hcc_handler *hcc, oal_int32 is_need_lock)
{
    oal_int32 i;
    oal_netbuf_head_stru *pst_head = OAL_PTR_NULL;

    if (OAL_WARN_ON(hcc == NULL)) {
        OAL_IO_PRINT("[E]hcc is null\n");
        return;
    }

    if (is_need_lock == OAL_TRUE) {
        hcc_tx_transfer_lock(hcc);
    }

    /* Restore assem queues */
    hcc_restore_assemble_netbuf_list(hcc);

    /* Clear all tx queues */
    for (i = 0; i < HCC_QUEUE_COUNT; i++) {
        oal_uint32 ul_list_len;
        pst_head = &hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].data_queue;
        ul_list_len = oal_netbuf_list_len(pst_head);
        hcc_tx_netbuf_list_free(pst_head);
        if (ul_list_len) {
            OAL_IO_PRINT("Clear queue:%d,total %u hcc tx pkts!\n", i, ul_list_len);
        }
    }

    if (is_need_lock == OAL_TRUE) {
        hcc_tx_transfer_unlock(hcc);
    }

    /* Clear all rx queues */
    if (is_need_lock == OAL_TRUE) {
        hcc_rx_transfer_lock(hcc);
    }

    for (i = 0; i < HCC_QUEUE_COUNT; i++) {
        oal_uint32 ul_list_len;
        pst_head = &hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[i].data_queue;
        ul_list_len = oal_netbuf_list_len(pst_head);
        oal_netbuf_queue_purge(pst_head);
        if (ul_list_len) {
            OAL_IO_PRINT("Clear queue:%d,total %u hcc rx pkts!\n", i, ul_list_len);
        }
    }

    if (is_need_lock == OAL_TRUE) {
        hcc_rx_transfer_unlock(hcc);
    }
}

oal_void hcc_change_state(struct hcc_handler *hcc, oal_int32 state)
{
    oal_int32 old_state;
    oal_int32 new_state;
    if (OAL_WARN_ON(hcc == NULL)) {
        return;
    }

    old_state = oal_atomic_read(&hcc->state);

    oal_atomic_set(&hcc->state, state);

    new_state = oal_atomic_read(&hcc->state);
    if (old_state != new_state) {
        PS_PRINT_WARNING("hcc state %s[%d]=>%s[%d]\n",
                         old_state == HCC_ON ? "on " : "off",
                         old_state,
                         new_state == HCC_ON ? "on " : "off",
                         new_state);
    }
}

oal_void hcc_change_state_exception(oal_void)
{
    hcc_change_state(hcc_get_default_handler(), HCC_EXCEPTION);
}

oal_void hcc_enable(struct hcc_handler *hcc, oal_int32 is_need_lock)
{
    oal_int32 i;
    if (OAL_WARN_ON(hcc == NULL)) {
        return;
    }

    if (is_need_lock == OAL_TRUE) {
        hcc_transfer_lock(hcc);
    }

    hcc_clear_all_queues(hcc, OAL_FALSE);

    for (i = 0; i < HCC_QUEUE_COUNT; i++) {
        oal_spin_lock_bh(&hcc->hcc_transer_info.tx_flow_ctrl.lock);
        hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].flow_ctrl.is_stopped = OAL_FALSE;
        oal_spin_unlock_bh(&hcc->hcc_transer_info.tx_flow_ctrl.lock);
    }

    hcc_change_state(hcc, HCC_ON);

    if (is_need_lock == OAL_TRUE) {
        hcc_transfer_unlock(hcc);
    }
}

oal_void hcc_disable(struct hcc_handler *hcc, oal_int32 is_need_lock)
{
    if (OAL_WARN_ON(hcc == NULL)) {
        return;
    }

    hcc_change_state(hcc, HCC_OFF);
    /* disable flow ctrl detect timer */
    hcc_dev_flowctr_timer_del(hcc);
    oal_cancel_delayed_work_sync(&hcc->hcc_transer_info.tx_flow_ctrl.worker);

    if (is_need_lock == OAL_TRUE) {
        hcc_transfer_lock(hcc);
    }
    hcc_clear_all_queues(hcc, OAL_FALSE);
    if (is_need_lock == OAL_TRUE) {
        hcc_transfer_unlock(hcc);
    }
    /* must wake up tx thread after clear the hcc queues */
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&hcc->hcc_transer_info.tx_flow_ctrl.wait_queue);
}


OAL_STATIC int hcc_send_tx_queue(struct hcc_handler *hcc, hcc_queue_type type)
{
    oal_int32 ret;
    oal_int32 count = 0;
    oal_uint32 queue_len, remain_len, remain_len_t;
    oal_netbuf_head_stru *head = OAL_PTR_NULL;
    oal_netbuf_head_stru *next_assembled_head = OAL_PTR_NULL;
    hcc_send_mode send_mode;
    hcc_trans_queue *pst_hcc_queue = OAL_PTR_NULL;
    hcc_netbuf_queue_type pool_type;
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    oal_uint8 uc_credit;
    oal_uint32 ul_pool_type_flag = OAL_FALSE;
#endif
    OAL_BUG_ON(!hcc);

    if (OAL_UNLIKELY(type >= HCC_QUEUE_COUNT)) {
        OAL_IO_PRINT("unkown hcc queue type %d\n", type);
        return count;
    }

    pst_hcc_queue = &hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[type];
    head = &pst_hcc_queue->data_queue;

    hcc_tx_transfer_lock(hcc);

    if (oal_netbuf_list_empty(head)) {
#ifdef CONFIG_HCC_DEBUG
        OAL_IO_PRINT("queue type %d is empty\n", type);
#endif
        hcc_tx_transfer_unlock(hcc);
        return count;
    }

    queue_len = oal_netbuf_list_len(head);
    remain_len = queue_len;

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
#ifdef CONFIG_HCC_DEBUG
    OAL_IO_PRINT("before_update: hcc_send_tx_queue: queue_type = %d, burst_limit = %d, remain_len = %d\r\n",
                 type, pst_hcc_queue->burst_limit, remain_len);
#endif
    remain_len = OAL_MIN(pst_hcc_queue->burst_limit, remain_len);
#endif

    remain_len_t = remain_len;
    next_assembled_head = &hcc->hcc_transer_info.tx_assem_info.assembled_head;

    if (OAL_UNLIKELY(!oal_netbuf_list_empty(next_assembled_head))) {
        /* First enter the queue, should single send */
        OAL_IO_PRINT("[WARN]reset assemble package![queue type:%d,len:%d]\n",
                     type, oal_netbuf_list_len(next_assembled_head));
        if (hcc_send_assemble_reset(hcc) != OAL_SUCC) {
            hcc_tx_transfer_unlock(hcc);
            /* send one pkt */
            return 1;
        }
    }

    send_mode = pst_hcc_queue->send_mode;
    pool_type = pst_hcc_queue->netbuf_pool_type;

    while (remain_len != 0) {
#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)

#ifdef CONFIG_SDIO_MSG_FLOWCTRL
        if (pst_hcc_queue->flow_ctrl.enable == OAL_TRUE) {
            if (pst_hcc_queue->flow_ctrl.flow_type == HCC_FLOWCTRL_SDIO) {
                /* flow ctrl */
                if (hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_flag != D2H_MSG_FLOWCTRL_ON) {
#ifdef CONFIG_HCC_DEBUG
                    OAL_IO_PRINT("[WARNING]can't send data, flow off\n");
#endif
                    if (!oal_netbuf_list_empty(&hcc->hcc_transer_info.tx_assem_info.assembled_head)) {
                        if (hcc_send_assemble_reset(hcc) != OAL_SUCC) {
                            hcc_tx_transfer_unlock(hcc);
                            count++;
                            /* send one pkt */
                            return count;
                        }
                    }
                    hcc_tx_transfer_unlock(hcc);
                    return count;
                }
            }
        }
#endif
        if (pst_hcc_queue->flow_ctrl.flow_type == HCC_FLOWCTRL_CREDIT) {
            uc_credit = hcc->hcc_transer_info.tx_flow_ctrl.uc_hipriority_cnt;

            /* 高优先级如果没有内存，直接返回规避死循环问题。 */
            if (!(uc_credit > hcc_credit_bottom_value)) {
                if (ul_pool_type_flag == OAL_TRUE) {
                    /* 恢复成普通优先级 */
                    hcc_tx_netbuf_restore_normal_pri_queue(hcc, pool_type);
                }
                hcc_tx_transfer_unlock(hcc);
                return OAL_SUCC;
            }

            if (ul_pool_type_flag == OAL_FALSE) {
                if (hcc_tx_netbuf_test_and_switch_high_pri_queue(hcc, pool_type) == OAL_SUCC) {
                    ul_pool_type_flag = OAL_TRUE;
                }
            }
        }
#endif
        ret = hcc_send_data_packet(hcc, head, type, next_assembled_head,
                                   send_mode, &remain_len);

        OAL_BUG_ON(remain_len > remain_len_t);
        if (OAL_LIKELY(ret == OAL_SUCC)) {
            count += (oal_int32) (remain_len_t - remain_len);
        }

#ifdef CONFIG_SDIO_MSG_FLOWCTRL
        oal_spin_lock_bh(&hcc->hcc_transer_info.tx_flow_ctrl.lock);

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
        if ((oal_netbuf_list_len(&pst_hcc_queue->data_queue)
             < pst_hcc_queue->flow_ctrl.low_waterline)) {
            if (pst_hcc_queue->flow_ctrl.is_stopped == OAL_TRUE) {
                pst_hcc_queue->flow_ctrl.is_stopped = OAL_FALSE;
                hcc_tx_network_start_subq(hcc, (oal_uint16) pst_hcc_queue->wlan_queue_id);
            }
            OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&hcc->hcc_transer_info.tx_flow_ctrl.wait_queue);
        }

#else
        if (hcc_queues_flow_ctrl_len(hcc, HCC_TX) < HCC_TX_FLOW_LO_LEVEL) {
            OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&hcc->hcc_transer_info.tx_flow_ctrl.wait_queue);
            hcc_tx_network_startall_queues(hcc);
        }
#endif
        oal_spin_unlock_bh(&hcc->hcc_transer_info.tx_flow_ctrl.lock);
#endif
#if (!defined(CONFIG_PREEMPT) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION))
        cond_resched();//lint !e522
#endif
    }

#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    if (pst_hcc_queue->flow_ctrl.flow_type == HCC_FLOWCTRL_CREDIT) {
        if (ul_pool_type_flag == OAL_TRUE) {
            hcc_tx_netbuf_restore_normal_pri_queue(hcc, pool_type);
        }
    }
#endif
    hcc_tx_transfer_unlock(hcc);

    return count;
}

oal_int32 hcc_rx_register(struct hcc_handler *hcc, oal_uint8 mtype, hcc_rx_post_do post_do, hcc_rx_pre_do pre_do)
{
    hcc_rx_action    *rx_action = OAL_PTR_NULL;

    if (OAL_WARN_ON(hcc == NULL)) {
        return -OAL_EINVAL;
    }

    rx_action = &(hcc->hcc_transer_info.rx_action_info.action[mtype]);

    if (OAL_WARN_ON(rx_action->post_do != NULL)) {
#ifdef CONFIG_PRINTK
        OAL_IO_PRINT("repeat regist hcc action :%d,it's had %pF\n", mtype, rx_action->post_do);
#else
        OAL_IO_PRINT("repeat regist hcc action :%d,it's had %p\n", mtype, rx_action->post_do);
#endif
        return -OAL_EBUSY;
    }
    /* 此处暂时不加互斥锁，由流程保证。 */
    rx_action->post_do = post_do;
    rx_action->pre_do = pre_do;

    return OAL_SUCC;
}
/*lint -e19*/
oal_module_symbol(hcc_rx_register);
/*lint +e19*/
oal_int32 hcc_rx_unregister(struct hcc_handler *hcc, oal_uint8 mtype)
{
    hcc_rx_action    *rx_action = OAL_PTR_NULL;

    if (OAL_WARN_ON((hcc == NULL) || (mtype >= HCC_ACTION_TYPE_BUTT))) {
        OAL_IO_PRINT("Invalid params:hcc = %p, main type = %u\n", hcc, mtype);
        return -OAL_EFAIL;
    }

    rx_action = &hcc->hcc_transer_info.rx_action_info.action[mtype];//lint !e613

    memset_s((oal_void*)rx_action, OAL_SIZEOF(hcc_rx_action), 0, OAL_SIZEOF(hcc_rx_action));
    return OAL_SUCC;
}
/*lint -e19*/
oal_module_symbol(hcc_rx_unregister);
/*lint +e19*/
OAL_STATIC oal_uint32 hcc_check_rx_netbuf_vaild(oal_netbuf_stru *netbuf)
{
    struct hcc_header *hdr;
    oal_int32 netbuf_len;

    hdr = (struct hcc_header *)OAL_NETBUF_DATA(netbuf);
    netbuf_len = (oal_int32) OAL_NETBUF_LEN(netbuf);

    if (OAL_UNLIKELY(hcc_check_header_vaild(hdr) != OAL_TRUE))
        return OAL_FALSE;

    if (OAL_UNLIKELY(hdr->pad_payload + HCC_HDR_TOTAL_LEN +  hdr->pay_len > netbuf_len))
        return OAL_FALSE;

    return OAL_TRUE;
}


OAL_STATIC  oal_int32 hcc_rx(struct hcc_handler *hcc, oal_netbuf_stru *netbuf)
{
    struct hcc_header *hdr = OAL_PTR_NULL;
    int32_t                 netbuf_len;
    oal_int32               extend_len;

    OAL_BUG_ON(!hcc);

    hdr = (struct hcc_header *)OAL_NETBUF_DATA(netbuf);
    netbuf_len = (int32_t) OAL_NETBUF_LEN(netbuf);

    if (OAL_UNLIKELY(hcc_check_rx_netbuf_vaild(netbuf) != OAL_TRUE)) {
        oal_print_hex_dump((oal_uint8 *)hdr, HCC_HDR_TOTAL_LEN, 16, "invaild hcc_rx header: ");
        oal_try_to_dump_device_mem(OAL_FALSE);
        return -OAL_EFAIL;
    }

    extend_len = HCC_HDR_TOTAL_LEN - HCC_HDR_LEN - hdr->pad_hdr;

    if (hdr->pad_payload) {
        /* memmove */
        oal_uint8* extend_addr = (oal_uint8*)hdr + HCC_HDR_LEN + hdr->pad_hdr;
        if (memmove_s(extend_addr + hdr->pad_payload, netbuf_len - HCC_HDR_LEN - hdr->pad_hdr - hdr->pad_payload,
                      extend_addr, (uint32_t)extend_len) != EOK) {
            OAL_IO_PRINT("hcc_tx::memmove_s failed !\n");
            return OAL_FAIL;
        }
    }

    if (HCC_HDR_LEN + hdr->pad_hdr + hdr->pad_payload > OAL_NETBUF_LEN(netbuf)) {
        /* add check detail */
        OAL_IO_PRINT("[Error]wrong pad_hdr,hcc hdr too long,len:%d!\n", (oal_int32)OAL_NETBUF_LEN(netbuf));
        oal_print_hex_dump((oal_uint8*)OAL_NETBUF_DATA(netbuf),
                           (oal_int32)OAL_NETBUF_LEN(netbuf), 32, "rx wrong data: ");
        /* print the data buff here! */
        return -OAL_EINVAL;
    }

#ifdef CONFIG_HCC_DEBUG
    OAL_IO_PRINT("[WIFI]hcc rx pkt seq:%d\n", hdr->seq);
    oal_print_hex_dump((oal_uint8*)hdr, HCC_HDR_TOTAL_LEN, 8, "hcc hdr:");
    oal_print_hex_dump(OAL_NETBUF_DATA(netbuf) + HCC_HDR_TOTAL_LEN + hdr->pad_payload,
                       hdr->pay_len, 8, "hcc payload:");
#endif

    oal_netbuf_pull(netbuf, HCC_HDR_LEN + hdr->pad_hdr + hdr->pad_payload);

    /* 传出去的netbuf len 包含extend_len长度! */
    oal_netbuf_trim(netbuf, OAL_NETBUF_LEN(netbuf) - hdr->pay_len - (oal_uint32)extend_len);

    OAL_NETBUF_NEXT(netbuf) = NULL;
    OAL_NETBUF_PREV(netbuf) = NULL;

    return OAL_SUCC;
}

OAL_STATIC oal_int32 hcc_send_rx_queue(struct hcc_handler *hcc, hcc_queue_type type)
{
    oal_int32 count = 0;
    oal_int32 pre_ret;
    oal_uint8 *pst_pre_context = NULL;
    oal_netbuf_head_stru *netbuf_hdr = OAL_PTR_NULL;
    hcc_rx_action    *rx_action = OAL_PTR_NULL;
    oal_netbuf_stru *pst_netbuf = OAL_PTR_NULL;
    hcc_netbuf_stru  st_hcc_netbuf;
    struct hcc_header *pst_hcc_head = OAL_PTR_NULL;

    OAL_BUG_ON(type >= HCC_QUEUE_COUNT);

    netbuf_hdr = &hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[type].data_queue;

    /* 依次处理队列中每个netbuf */
    for (; ;) {
        pst_netbuf = oal_netbuf_delist(netbuf_hdr);
        if (pst_netbuf == NULL) {
            break;
        }

        pst_hcc_head = (struct hcc_header *)OAL_NETBUF_DATA(pst_netbuf);
        if (OAL_UNLIKELY(hcc_check_header_vaild(pst_hcc_head) != OAL_TRUE)) {
            oal_print_hex_dump((oal_uint8 *)pst_hcc_head, HCC_HDR_TOTAL_LEN, 16, "invaild hcc header: ");
            DECLARE_DFT_TRACE_KEY_INFO("hcc_rx_hdr_err", OAL_DFT_TRACE_EXCEP);
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "invaild hcc rx head");
            count++;
            oal_netbuf_free(pst_netbuf);
            return count;
        }

        rx_action = &hcc->hcc_transer_info.rx_action_info.action[pst_hcc_head->main_type];

        pre_ret = OAL_SUCC;

        /* prepare */
        if (rx_action->pre_do) {
            st_hcc_netbuf.pst_netbuf = pst_netbuf;
            st_hcc_netbuf.len = (oal_int32)OAL_NETBUF_LEN(pst_netbuf);
            pre_ret = rx_action->pre_do (pst_hcc_head->sub_type, &st_hcc_netbuf, &pst_pre_context);
        }

        if (pre_ret == OAL_SUCC) {
            if (OAL_LIKELY(hcc_rx(hcc, pst_netbuf) == OAL_SUCC)) {
                if (OAL_LIKELY(rx_action->post_do != NULL)) {
                    st_hcc_netbuf.pst_netbuf = pst_netbuf;
                    st_hcc_netbuf.len = (oal_int32)OAL_NETBUF_LEN(pst_netbuf);
                    rx_action->post_do (pst_hcc_head->sub_type, &st_hcc_netbuf, pst_pre_context);
                } else {
                    OAM_ERROR_LOG2(0, OAM_SF_ANY, "hcc mtype:%d,stype:%d did't register cb function!",
                                   pst_hcc_head->main_type,
                                   pst_hcc_head->sub_type);
                    oal_print_hex_dump((oal_uint8*)pst_hcc_head, HCC_HDR_TOTAL_LEN, 32, "hcc invaild header: ");
                    oal_print_hex_dump(OAL_NETBUF_DATA(pst_netbuf),
                                       (oal_int32)OAL_NETBUF_LEN(pst_netbuf), 32, "hcc invaild header(payload): ");
                    oal_netbuf_free(pst_netbuf);
                }
            } else {
                oal_netbuf_free(pst_netbuf);
            }
            hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[type].total_pkts++;
            count++;
        } else {
            /* simple process, when pre do failed,
            keep the netbuf in list,
            and skip the loop */
            oal_netbuf_addlist(netbuf_hdr, pst_netbuf);
            break;
        }
#if (!defined(CONFIG_PREEMPT) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION))
        cond_resched();//lint !e522
#endif
    }
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    /* 执行do_softirq,避免netif_rx_softirq调度延时 */
    if (count) {
        oal_local_bh_disable(); // liuxiang
        oal_local_bh_enable(); // liuxiang
    }
#endif
    return count;
}

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
oal_void hcc_tx_network_start_subq(struct hcc_handler *hcc, oal_uint16 us_queue_idx)
{
    OAL_BUG_ON(hcc == NULL);

    if (OAL_LIKELY(hcc->hcc_transer_info.tx_flow_ctrl.start_subq)) {
        hcc->hcc_transer_info.tx_flow_ctrl.start_subq(us_queue_idx);
    }
}

oal_void hcc_tx_network_stop_subq(struct hcc_handler *hcc, oal_uint16 us_queue_idx)
{
    OAL_BUG_ON(hcc == NULL);

    if (OAL_LIKELY(hcc->hcc_transer_info.tx_flow_ctrl.stop_subq)) {
        hcc->hcc_transer_info.tx_flow_ctrl.stop_subq(us_queue_idx);
    }
}


oal_void hcc_flowctl_operate_subq_register(hcc_flowctl_start_subq start_subq, hcc_flowctl_stop_subq stop_subq)
{
    if (hcc_tc == NULL) {
        OAL_IO_PRINT("[ERROR]hcc_flowctl_operate_subq_register failed! hcc_tc is NULL\n");
        return;
    }
    hcc_tc->hcc_transer_info.tx_flow_ctrl.start_subq = start_subq;
    hcc_tc->hcc_transer_info.tx_flow_ctrl.stop_subq = stop_subq;
    return;
}


oal_void   hcc_host_set_flowctl_param(oal_uint8 uc_queue_type, oal_uint16 us_burst_limit,
                                      oal_uint16 us_low_waterline, oal_uint16 us_high_waterline)
{
    if (uc_queue_type >= HCC_QUEUE_COUNT) {
        OAL_IO_PRINT("CONFIG_ERROR: hcc_host_set_flowctl_param: uc_queue_type = %d\r\n", uc_queue_type);
        return;
    }
    hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[uc_queue_type].burst_limit = us_burst_limit;
    hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[uc_queue_type].flow_ctrl.low_waterline = us_low_waterline;
    hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[uc_queue_type].flow_ctrl.high_waterline = us_high_waterline;

    OAL_IO_PRINT("hcc_host_set_flowctl_param, queue[%d]: burst limit = %d, low_waterline = %d, high_waterline =%d\r\n",
                 uc_queue_type, us_burst_limit, us_low_waterline, us_high_waterline);
    return;
}

oal_void    hcc_host_get_flowctl_param(oal_uint8 uc_queue_type, oal_uint16 *pus_burst_limit,
                                       oal_uint16 *pus_low_waterline, oal_uint16 *pus_high_waterline)
{
    if (uc_queue_type >= HCC_QUEUE_COUNT) {
        OAL_IO_PRINT("CONFIG_ERROR: hcc_host_get_flowctl_param: uc_queue_type = %d\r\n", uc_queue_type);
        return;
    }

    *pus_burst_limit = hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[uc_queue_type].burst_limit;
    *pus_low_waterline = hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[uc_queue_type].flow_ctrl.low_waterline;
    *pus_high_waterline = hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[uc_queue_type].flow_ctrl.high_waterline;
    return;
}

oal_void   hcc_host_get_flowctl_stat(oal_void)
{
    oal_uint16 us_queue_idx;

    /* 输出各个队列的状态信息 */
    for (us_queue_idx = 0; us_queue_idx < HCC_QUEUE_COUNT; us_queue_idx++) {
        OAL_IO_PRINT("Q[%d]:bst_lmt[%d],low_wl[%d],high_wl[%d]\r\n",
                     us_queue_idx,
                     hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].burst_limit,
                     hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].flow_ctrl.low_waterline,
                     hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].flow_ctrl.high_waterline);

        OAL_IO_PRINT("      wlan_q[%d],stoped?[%d],q_len[%d],total_pkt[%d],loss_pkt[%d]\r\n",
                     hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].wlan_queue_id,
                     hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].flow_ctrl.is_stopped,
                     oal_netbuf_list_len(&hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].data_queue),
                     hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].total_pkts,
                     hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].loss_pkts);
    }

    return;
}

oal_bool_enum_uint8 hcc_flowctl_get_device_mode(struct hcc_handler *hcc)
{
    OAL_BUG_ON(hcc == NULL);
    if (OAL_LIKELY(hcc->hcc_transer_info.tx_flow_ctrl.get_mode)) {
        return hcc->hcc_transer_info.tx_flow_ctrl.get_mode();
    }

    return OAL_FALSE;
}

oal_void hcc_flowctl_get_device_mode_register(hcc_flowctl_get_mode get_mode)
{
    if (hcc_tc == OAL_PTR_NULL) {
        OAL_IO_PRINT("[ERROR]hcc_flowctl_get_device_mode_register failed! hcc_tc is NULL\n");
        return;
    }
    hcc_tc->hcc_transer_info.tx_flow_ctrl.get_mode = get_mode;
}

oal_void    hcc_host_update_vi_flowctl_param(oal_uint32 be_cwmin, oal_uint32 vi_cwmin)
{
    oal_uint16  us_burst_limit;
    oal_uint16  us_low_waterline;
    oal_uint16  us_high_waterline;

    /* 如果vi与be的edca参数设置为一致，则更新VI的拥塞控制参数 */
    if (be_cwmin == vi_cwmin) {
        hcc_host_get_flowctl_param(DATA_UDP_BE_QUEUE, &us_burst_limit, &us_low_waterline, &us_high_waterline);
        hcc_host_set_flowctl_param(DATA_UDP_VI_QUEUE, us_burst_limit, us_low_waterline, us_high_waterline);
    } else { // 否则设置vi的拥塞控制参数为默认值
        hcc_host_set_flowctl_param(DATA_UDP_VI_QUEUE, 40, 40, 80);
    }
}

/*lint -e19 */
oal_module_symbol(hcc_flowctl_get_device_mode_register);
oal_module_symbol(hcc_flowctl_operate_subq_register);
oal_module_symbol(hcc_host_update_vi_flowctl_param);

/*lint +e19 */
#else


OAL_STATIC  oal_int32 hcc_process_tx_queues(struct hcc_handler *hcc)
{
    oal_int32 i;
    oal_int32 ret = 0;
    for (i = 0; i < HCC_QUEUE_COUNT; i++) {
        /* send tx queue */
        ret += hcc_send_tx_queue(hcc, (hcc_queue_type)i);
    }
    return ret;
}


OAL_STATIC  oal_int32 hcc_process_rx_queues(struct hcc_handler *hcc)
{
    oal_int32 i;

    for (i = 0; i < HCC_QUEUE_COUNT; i++) {
        hcc_send_rx_queue(hcc, (hcc_queue_type)i);
    }
    return OAL_SUCC;
}

#endif


oal_int32 hcc_thread_process(struct hcc_handler *hcc)
{
    oal_int32 ret = 0;
#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
    oal_bool_enum_uint8      en_device_is_sta;
    OAL_STATIC oal_uint32 s_ul_schedule_cnt = 0;

    en_device_is_sta = hcc_flowctl_get_device_mode(hcc);

    s_ul_schedule_cnt++;

    if (en_device_is_sta == OAL_TRUE) {
        /* Tx Tcp Data queue > Rx Tcp Ack
          Rx Tcp Data > Tx Tcp Ack
          Tx Tcp Data queue > Rx Tcp Data queue */
        ret += hcc_send_rx_queue(hcc, CTRL_QUEUE);
        ret += hcc_send_tx_queue(hcc, CTRL_QUEUE);

        ret += hcc_send_rx_queue(hcc, DATA_HI_QUEUE);
        ret += hcc_send_tx_queue(hcc, DATA_HI_QUEUE);

        if ((s_ul_schedule_cnt % LO_QUEUE_ADJUST_PRIORITY_PERIOD) == 0) {
            /* Tx Lo < Rx Lo */
            ret += hcc_send_rx_queue(hcc, DATA_LO_QUEUE);
            ret += hcc_send_tx_queue(hcc, DATA_LO_QUEUE);

            /* 下行TCP优先 */
            ret += hcc_send_rx_queue(hcc, DATA_TCP_DATA_QUEUE);
            ret += hcc_send_tx_queue(hcc, DATA_TCP_ACK_QUEUE);

            ret += hcc_send_tx_queue(hcc, DATA_TCP_DATA_QUEUE);
            ret += hcc_send_rx_queue(hcc, DATA_TCP_ACK_QUEUE);
        } else {
            /* 下行TCP优先 */
            ret += hcc_send_rx_queue(hcc, DATA_TCP_DATA_QUEUE);
            ret += hcc_send_tx_queue(hcc, DATA_TCP_ACK_QUEUE);

            ret += hcc_send_tx_queue(hcc, DATA_TCP_DATA_QUEUE);
            ret += hcc_send_rx_queue(hcc, DATA_TCP_ACK_QUEUE);

            /* Tx Lo < Rx Lo */
            ret += hcc_send_rx_queue(hcc, DATA_LO_QUEUE);
            ret += hcc_send_tx_queue(hcc, DATA_LO_QUEUE);
        }

        ret += hcc_send_rx_queue(hcc, DATA_UDP_VO_QUEUE);
        ret += hcc_send_tx_queue(hcc, DATA_UDP_VO_QUEUE);

        ret += hcc_send_rx_queue(hcc, DATA_UDP_VI_QUEUE);
        ret += hcc_send_tx_queue(hcc, DATA_UDP_VI_QUEUE);

        ret += hcc_send_rx_queue(hcc, DATA_UDP_BE_QUEUE);
        ret += hcc_send_tx_queue(hcc, DATA_UDP_BE_QUEUE);

        ret += hcc_send_rx_queue(hcc, DATA_UDP_BK_QUEUE);
        ret += hcc_send_tx_queue(hcc, DATA_UDP_BK_QUEUE);
    } else {
        /* Tx Tcp Data queue > Rx Tcp Ack
          Rx Tcp Data > Tx Tcp Ack
          Tx Tcp Data queue < Rx Tcp Data queue */
        ret += hcc_send_tx_queue(hcc, CTRL_QUEUE);
        ret += hcc_send_rx_queue(hcc, CTRL_QUEUE);

        ret += hcc_send_tx_queue(hcc, DATA_HI_QUEUE);
        ret += hcc_send_rx_queue(hcc, DATA_HI_QUEUE);

        if ((s_ul_schedule_cnt % LO_QUEUE_ADJUST_PRIORITY_PERIOD) == 0) {
            /* Tx Lo > Rx Lo */
            ret += hcc_send_tx_queue(hcc, DATA_LO_QUEUE);
            ret += hcc_send_rx_queue(hcc, DATA_LO_QUEUE);

            ret += hcc_send_tx_queue(hcc, DATA_TCP_DATA_QUEUE);
            ret += hcc_send_rx_queue(hcc, DATA_TCP_ACK_QUEUE);

            ret += hcc_send_rx_queue(hcc, DATA_TCP_DATA_QUEUE);
            ret += hcc_send_tx_queue(hcc, DATA_TCP_ACK_QUEUE);
        } else {
            ret += hcc_send_tx_queue(hcc, DATA_TCP_DATA_QUEUE);
            ret += hcc_send_rx_queue(hcc, DATA_TCP_ACK_QUEUE);

            ret += hcc_send_rx_queue(hcc, DATA_TCP_DATA_QUEUE);
            ret += hcc_send_tx_queue(hcc, DATA_TCP_ACK_QUEUE);

            /* Tx Lo > Rx Lo */
            ret += hcc_send_tx_queue(hcc, DATA_LO_QUEUE);
            ret += hcc_send_rx_queue(hcc, DATA_LO_QUEUE);
        }

        /* udp业务 */
        ret += hcc_send_tx_queue(hcc, DATA_UDP_VO_QUEUE);
        ret += hcc_send_rx_queue(hcc, DATA_UDP_VO_QUEUE);

        ret += hcc_send_tx_queue(hcc, DATA_UDP_VI_QUEUE);
        ret += hcc_send_rx_queue(hcc, DATA_UDP_VI_QUEUE);

        ret += hcc_send_tx_queue(hcc, DATA_UDP_BE_QUEUE);
        ret += hcc_send_rx_queue(hcc, DATA_UDP_BE_QUEUE);

        ret += hcc_send_tx_queue(hcc, DATA_UDP_BK_QUEUE);
        ret += hcc_send_rx_queue(hcc, DATA_UDP_BK_QUEUE);
    }
#else
    hcc_process_rx_queues(hcc);
    hcc_process_tx_queues(hcc);
#endif
    return ret;
}

oal_void hcc_set_tcpack_cnt(oal_uint32 ul_val)
{
    g_ul_tcp_ack_wait_sche_cnt = ul_val;
    return;
}

oal_int32 hcc_transfer_thread(oal_void *data)
{
#ifdef CONFIG_MMC
    oal_uint32 count;
    oal_int32 ret;
    struct hcc_handler* hcc = OAL_PTR_NULL;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_kthread_param_stru st_thread_param = {0};
#endif

#ifdef _PRE_WLAN_TCP_OPT
    OAL_STATIC oal_uint8 ack_loop_count = 0;
#endif

    hcc = (struct hcc_handler *)data;

    OAL_BUG_ON(!hcc);

    PS_PRINT_WARNING("hisi wifi hcc transfer thread enter\n");
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    st_thread_param.l_prio   = HCC_TRANS_REALY_THERAD_PRIORITY;
    st_thread_param.l_nice   = HCC_TRANS_THERAD_NICE;
    st_thread_param.l_policy = HCC_TRANS_THREAD_POLICY;
    oal_set_thread_property(OAL_CURRENT, &st_thread_param);
    allow_signal(SIGTERM);
#endif

    for (;;) {
        count = 0;

        if (OAL_UNLIKELY(oal_kthread_should_stop())) {
            OAL_IO_PRINT("hisi wifi hcc transfer thread leave\n");
            break;
        }

        ret = OAL_WAIT_EVENT_INTERRUPTIBLE(hcc->hcc_transer_info.hcc_transfer_wq,
                                           (hcc_thread_wait_event_cond_check(hcc) == OAL_TRUE));//lint !e666
        if (OAL_UNLIKELY(ret == -ERESTARTSYS)) {
            OAL_IO_PRINT("wifi task %s was interrupted by a signal\n", oal_get_current_task_name());
            break;
        }
#ifdef _PRE_WLAN_TCP_OPT
        if (hcc->p_hmac_tcp_ack_process_func != OAL_PTR_NULL) {
            ack_loop_count++;
            if (ack_loop_count >= g_ul_tcp_ack_wait_sche_cnt) {
                ack_loop_count = 0;
                hcc->p_hmac_tcp_ack_process_func();
            }
        }
#endif

        count |= (oal_uint32)hcc_thread_process(hcc);
#ifdef _PRE_CONFIG_WLAN_THRANS_THREAD_DEBUG
        if (count) {
            hcc->hcc_transer_info.thread_stat.loop_have_data_count++;
        } else {
            hcc->hcc_transer_info.thread_stat.loop_no_data_count++;
        }
#else
        OAL_REFERENCE(count);
#endif
    }
#else
    OAL_IO_PRINT("hisi wifi hcc transfer thread done!%p\n", data);
#endif
    OAL_IO_PRINT("hisi wifi hcc transfer thread done!\n");
    return OAL_SUCC;
}
/*lint -e19*/
oal_module_symbol(hcc_thread_process);
oal_module_symbol(hcc_transfer_thread);
oal_module_symbol(g_ul_tcp_ack_wait_sche_cnt);
/*lint +e19*/
OAL_STATIC  oal_void hcc_dev_flowctr_timer_del(struct hcc_handler *hcc)
{
    if (oal_in_interrupt()) {
        oal_timer_delete(&hcc->hcc_transer_info.tx_flow_ctrl.flow_timer);
    } else {
        oal_timer_delete_sync(&hcc->hcc_transer_info.tx_flow_ctrl.flow_timer);
    }
}

oal_void hcc_dev_flowctrl_on(struct hcc_handler *hcc, oal_uint8 need_notify_dev)
{
    hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_on_count++;
#ifdef CONFIG_HCC_DEBUG
    OAL_IO_PRINT("start tranferring to device\n");
#endif

    if (hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_flag == D2H_MSG_FLOWCTRL_OFF) {
        hcc_dev_flowctr_timer_del(hcc);
        hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_flag = D2H_MSG_FLOWCTRL_ON;
        hcc_sched_transfer(hcc);
    }

    if (need_notify_dev) {
        OAL_IO_PRINT("Host turn on dev flow ctrl...\n");
        oal_channel_send_msg(hcc->hi_channel, H2D_MSG_FLOWCTRL_ON);
    }
}

oal_void hcc_dev_flowctrl_off(struct hcc_handler *hcc)
{
    if (hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_flag == D2H_MSG_FLOWCTRL_ON) {
        oal_timer_start(&hcc->hcc_transer_info.tx_flow_ctrl.flow_timer,
                        hcc->hcc_transer_info.tx_flow_ctrl.timeout);
    }

    hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_flag = D2H_MSG_FLOWCTRL_OFF;
    hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_off_count++;
#ifdef CONFIG_HCC_DEBUG
    OAL_IO_PRINT("stop tranferring to device\n");
#endif
}


oal_int32 hcc_message_register(struct hcc_handler* hcc, oal_uint8 msg,
                               hcc_msg_rx cb, oal_void* data)
{
    return oal_channel_message_register(hcc->hi_channel, msg, cb, data);
}

oal_void hcc_message_unregister(struct hcc_handler* hcc, oal_uint8 msg)
{
    oal_channel_message_unregister(hcc->hi_channel, msg);
}


oal_void hcc_transfer_queues_init(struct hcc_handler *hcc)
{
    oal_int32 i, j;
    for (i = 0; i < HCC_DIR_COUNT; i++) {
        for (j = 0; j < HCC_QUEUE_COUNT; j++) {
            oal_netbuf_head_init(&hcc->hcc_transer_info.hcc_queues[i].queues[j].data_queue);
        }
    }
}

oal_int32 hcc_tx_assem_descr_init(struct hcc_handler *hcc)
{
    oal_int32 i;
    oal_int32 ret = OAL_SUCC;
    oal_netbuf_stru *netbuf = NULL;

    oal_netbuf_head_init(&hcc->tx_descr_info.tx_assem_descr_hdr);

    /* assem descr ping-pong buff, 2 should be ok */
    hcc->tx_descr_info.descr_num = 2;

    for (i = 0; i < hcc->tx_descr_info.descr_num; i++) {
        netbuf = oal_netbuf_alloc(HISDIO_HOST2DEV_SCATT_SIZE, 0, 0);
        if (netbuf == NULL) {
            /*lint -e801*/
            goto failed_netbuf_alloc;
        }

        oal_netbuf_put(netbuf, HISDIO_HOST2DEV_SCATT_SIZE);
        memset_s(OAL_NETBUF_DATA(netbuf), OAL_NETBUF_LEN(netbuf), 0, OAL_NETBUF_LEN(netbuf));
        oal_netbuf_list_tail(&hcc->tx_descr_info.tx_assem_descr_hdr, netbuf);
        OAL_BUG_ON(!OAL_IS_ALIGNED(((uintptr_t)OAL_NETBUF_DATA(netbuf)), 4));
    }

    OAL_BUG_ON(HISDIO_HOST2DEV_SCATT_SIZE < 4);

    return ret;
failed_netbuf_alloc:
    oal_netbuf_list_purge(&hcc->tx_descr_info.tx_assem_descr_hdr);
    return -OAL_ENOMEM;
}

oal_void hcc_tx_assem_descr_exit(struct hcc_handler *hcc)
{
    oal_netbuf_list_purge(&hcc->tx_descr_info.tx_assem_descr_hdr);
}

oal_void hcc_tx_assem_info_reset(struct hcc_handler *hcc)
{
    memset_s(hcc->hcc_transer_info.tx_assem_info.info, OAL_SIZEOF(hcc->hcc_transer_info.tx_assem_info.info),
             0, OAL_SIZEOF(hcc->hcc_transer_info.tx_assem_info.info));
}

oal_void hcc_rx_assem_info_reset(struct hcc_handler *hcc)
{
    memset_s(hcc->hcc_transer_info.rx_assem_info.info, OAL_SIZEOF(hcc->hcc_transer_info.rx_assem_info.info),
             0, OAL_SIZEOF(hcc->hcc_transer_info.rx_assem_info.info));
}

oal_void hcc_assem_info_init(struct hcc_handler *hcc)
{
    hcc->hcc_transer_info.tx_assem_info.assemble_max_count = hcc_assemble_count;
    hcc_tx_assem_info_reset(hcc);
    hcc_rx_assem_info_reset(hcc);
    oal_netbuf_list_head_init(&hcc->hcc_transer_info.tx_assem_info.assembled_head);
}

oal_void hcc_trans_limit_parm_init(struct hcc_handler  *hcc)
{
#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
    oal_int32 i;
    hcc_trans_queues *pst_hcc_tx_queues = &hcc->hcc_transer_info.hcc_queues[HCC_TX];

    for (i = 0; i < HCC_QUEUE_COUNT; i++) {
        hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].burst_limit = (oal_uint32)HCC_FLUSH_ALL;
        hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[i].burst_limit = (oal_uint32)HCC_FLUSH_ALL;
    }

    pst_hcc_tx_queues->queues[CTRL_QUEUE].burst_limit = 256;
    pst_hcc_tx_queues->queues[DATA_HI_QUEUE].burst_limit = 256;
    pst_hcc_tx_queues->queues[DATA_LO_QUEUE].burst_limit = 256;
    pst_hcc_tx_queues->queues[DATA_TCP_DATA_QUEUE].burst_limit = 256;
    pst_hcc_tx_queues->queues[DATA_TCP_ACK_QUEUE].burst_limit = 256;
    pst_hcc_tx_queues->queues[DATA_UDP_BK_QUEUE].burst_limit = 10;
    pst_hcc_tx_queues->queues[DATA_UDP_BE_QUEUE].burst_limit = 20;
    pst_hcc_tx_queues->queues[DATA_UDP_VI_QUEUE].burst_limit = 40;
    pst_hcc_tx_queues->queues[DATA_UDP_VO_QUEUE].burst_limit = 60;
#endif
}

oal_void hcc_trans_send_mode_init(struct hcc_handler  *hcc)
{
    oal_int32 i;

    for (i = 0; i < HCC_QUEUE_COUNT; i++) {
        hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].send_mode = hcc_assem_send;
    }
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_HI_QUEUE].send_mode = hcc_single_send;
}

OAL_STATIC OAL_VOLATILE oal_int32 g_flowctrl_info_flag = 0;
OAL_STATIC oal_void hcc_dev_flow_ctrl_timeout_isr(oal_ulong arg)
{
    struct hcc_handler *hcc = (struct hcc_handler *)(uintptr_t)arg;
    if (hcc == NULL) {
        OAL_IO_PRINT("hcc is null\n");
        return;
    }
    /* flowctrl lock too much time. */
    DECLARE_DFT_TRACE_KEY_INFO("hcc_flow_lock_timeout", OAL_DFT_TRACE_EXCEP);
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{SDIO flow ctrl had off for %lu ms, it's a long time}\n",
                     hcc->hcc_transer_info.tx_flow_ctrl.timeout);
    g_flowctrl_info_flag = 0;

    /* If work is idle,queue a new work. */
    if (oal_work_is_busy(&hcc->hcc_transer_info.tx_flow_ctrl.worker.work) == 0) {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        oal_queue_delayed_system_work(&hcc->hcc_transer_info.tx_flow_ctrl.worker, 0);
#endif
    }
}

oal_void hcc_flowctrl_deadlock_detect_worker(oal_work_stru *pst_flow_work)
{
    struct hcc_handler  *hcc = hcc_get_default_handler();
    if (hcc == NULL) {
        OAL_IO_PRINT("hcc_flowctrl_deadlock_detect_worker hcc is null\n");
        return;
    }
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hcc_flowctrl_deadlock_detect_worker action,%d}\n",
                     g_flowctrl_info_flag);
    if (g_flowctrl_info_flag == 0) {
        g_flowctrl_info_flag = 1 ;
        OAL_SMP_MB();
        /* queue system_wq delay work,and send other message 20ms later. */
        /* print device mem */
        hcc_print_device_mem_info();
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        oal_queue_delayed_system_work(&hcc->hcc_transer_info.tx_flow_ctrl.worker, OAL_MSECS_TO_JIFFIES(20));
#endif
    } else if (g_flowctrl_info_flag == 1) {
        /* print device mem */
        if (hcc_flowctrl_detect_panic) {
            hcc_trigger_device_panic();
        } else {
            hcc_print_device_mem_info();
        }
    }

    return;
}

oal_void hcc_trans_flow_ctrl_info_init(struct hcc_handler  *hcc)
{
    oal_int32 i;

    hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_flag = D2H_MSG_FLOWCTRL_ON;
    hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_off_count = 0;
    hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_on_count = 0;
    oal_spin_lock_init(&hcc->hcc_transer_info.tx_flow_ctrl.lock);
    hcc->hcc_transer_info.tx_flow_ctrl.timeout = 20 * 1000;
    OAL_INIT_DELAYED_WORK(&hcc->hcc_transer_info.tx_flow_ctrl.worker, hcc_flowctrl_deadlock_detect_worker);
    oal_timer_init(&hcc->hcc_transer_info.tx_flow_ctrl.flow_timer,
                   hcc->hcc_transer_info.tx_flow_ctrl.timeout,
                   hcc_dev_flow_ctrl_timeout_isr,
                   (uintptr_t)hcc);

    hcc->hcc_transer_info.tx_flow_ctrl.uc_hipriority_cnt = HCC_FLOW_HIGH_PRI_BUFF_CNT;
    oal_spin_lock_init(&hcc->hcc_transer_info.tx_flow_ctrl.st_hipri_lock);

    for (i = 0; i < HCC_QUEUE_COUNT; i++) {
        /* TX queue */
        hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].flow_ctrl.enable = OAL_TRUE;
        hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].flow_ctrl.flow_type = HCC_FLOWCTRL_SDIO;
        hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].flow_ctrl.is_stopped = OAL_FALSE;
        hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].flow_ctrl.low_waterline = 512;
        hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].flow_ctrl.high_waterline = 1024;
        hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].netbuf_pool_type = HCC_NETBUF_NORMAL_QUEUE;

        /* RX queue */
        hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[i].flow_ctrl.enable = OAL_TRUE;
        hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].flow_ctrl.is_stopped = OAL_FALSE;
        hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[i].flow_ctrl.low_waterline = 128;
        hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[i].flow_ctrl.high_waterline = 512;
    }

    /* Additional high priority flow_ctrl settings */
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_HI_QUEUE].flow_ctrl.flow_type = HCC_FLOWCTRL_CREDIT;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_HI_QUEUE].flow_ctrl.enable = OAL_FALSE;

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL

    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[CTRL_QUEUE].flow_ctrl.low_waterline = 128;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[CTRL_QUEUE].flow_ctrl.high_waterline = 256;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[CTRL_QUEUE].netbuf_pool_type = HCC_NETBUF_HIGH_QUEUE;

    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_HI_QUEUE].flow_ctrl.low_waterline = 128;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_HI_QUEUE].flow_ctrl.high_waterline = 256;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_HI_QUEUE].netbuf_pool_type = HCC_NETBUF_HIGH_QUEUE;

    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_LO_QUEUE].flow_ctrl.low_waterline = 128;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_LO_QUEUE].flow_ctrl.high_waterline = 256;

    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_TCP_DATA_QUEUE].flow_ctrl.low_waterline = 128;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_TCP_DATA_QUEUE].flow_ctrl.high_waterline = 256;

    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_TCP_ACK_QUEUE].flow_ctrl.low_waterline = 128;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_TCP_ACK_QUEUE].flow_ctrl.high_waterline = 256;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_UDP_BK_QUEUE].flow_ctrl.low_waterline = 10;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_UDP_BK_QUEUE].flow_ctrl.high_waterline = 20;

    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_UDP_BE_QUEUE].flow_ctrl.low_waterline = 20;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_UDP_BE_QUEUE].flow_ctrl.high_waterline = 40;
#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_UDP_BK_QUEUE].flow_ctrl.low_waterline = 200;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_UDP_BK_QUEUE].flow_ctrl.high_waterline = 400;

    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_UDP_BE_QUEUE].flow_ctrl.low_waterline = 200;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_UDP_BE_QUEUE].flow_ctrl.high_waterline = 400;
#endif
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_UDP_VI_QUEUE].flow_ctrl.low_waterline = 40;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_UDP_VI_QUEUE].flow_ctrl.high_waterline = 80;

    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_UDP_VO_QUEUE].flow_ctrl.low_waterline = 60;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_UDP_VO_QUEUE].flow_ctrl.high_waterline = 120;
#endif
}

oal_int32 hcc_flow_on_callback(oal_void* data)
{
    hcc_dev_flowctrl_on((struct hcc_handler *)data, 0);
    return OAL_SUCC;
}

oal_int32 hcc_flow_off_callback(oal_void* data)
{
    hcc_dev_flowctrl_off((struct hcc_handler *)data);
    return OAL_SUCC;
}


oal_int32  hcc_credit_update_callback(oal_void* data)
{
    oal_uint8           uc_large_cnt;
    struct hcc_handler *hcc     = (struct hcc_handler *)data;

    uc_large_cnt = HISDIO_LARGE_PKT_GET(hcc->hi_channel->sdio_extend->credit_info);
    if (uc_large_cnt == 0) {
        OAM_WARNING_LOG0(0, OAM_SF_PWR, "[hcc]credit updata zero!");
    }

    oal_spin_lock(&(hcc->hcc_transer_info.tx_flow_ctrl.st_hipri_lock));

    hcc->hcc_transer_info.tx_flow_ctrl.uc_hipriority_cnt = uc_large_cnt;

    oal_spin_unlock(&(hcc->hcc_transer_info.tx_flow_ctrl.st_hipri_lock));

    hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_hipri_update_count++;

    hcc_sched_transfer(hcc);

    return OAL_SUCC;
}

oal_int32  hcc_high_pkts_loss_callback(oal_void* data)
{
    OAL_REFERENCE(data);
    DECLARE_DFT_TRACE_KEY_INFO("sdio_high_pkts_loss", OAL_DFT_TRACE_EXCEP);
    return OAL_SUCC;
}

OAL_STATIC void hcc_module_init_partone(void)
{
    uint32_t tx_max_len;

    OAL_BUG_ON(HCC_HDR_LEN > HCC_HDR_TOTAL_LEN);

    /* main_type:4 只能表示16种类型 */
    OAL_BUG_ON(HCC_ACTION_TYPE_BUTT > 15);
    /* 1544-the max netbuf len of device */
    /*lint -e778*/
    g_hcc_tx_max_buf_len = OAL_ROUND_DOWN(HSDIO_HOST2DEV_PKTS_MAX_LEN, HISDIO_H2D_SCATT_BUFFLEN_ALIGN);
    /* one byte,2^8=256 */
    tx_max_len = (1 << (8 + HISDIO_H2D_SCATT_BUFFLEN_ALIGN_BITS));
    g_hcc_tx_max_buf_len = OAL_MIN(g_hcc_tx_max_buf_len, tx_max_len);
    /*lint +e778*/
    return;
}

struct hcc_handler* hcc_module_init(void)
{
    struct hcc_handler* hcc = NULL;

    hcc_module_init_partone();

    hcc = (struct hcc_handler*)oal_memalloc(OAL_SIZEOF(struct hcc_handler));
    if (hcc == NULL) {
        OAL_IO_PRINT("hcc mem alloc  failed!\n");
        return NULL;
    }
    memset_s((oal_void*)hcc, OAL_SIZEOF(struct hcc_handler), 0, OAL_SIZEOF(struct hcc_handler));

    hcc->hdr_rever_max_len = HCC_HDR_RESERVED_MAX_LEN;

    hcc->hi_channel = oal_channel_init_module((oal_void*)hcc);
    /*lint -e801*/
    if (hcc->hi_channel == NULL) {
        OAL_IO_PRINT("sdio init module failed!\n");
        goto failed_sdio_init;
    }

#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
#ifdef CONFIG_SDIO_MSG_FLOWCTRL
    oal_sdio_credit_update_cb_register(hcc->hi_channel, sdio_credit_info_update_handler);
#endif
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    oal_atomic_set(&g_wakeup_dev_wait_ack, 0);
#endif
#endif

#ifndef WIN32
    if (oal_channel_func_probe(hcc->hi_channel) != OAL_SUCC) {
        OAL_IO_PRINT("sdio probe failed!\n");
        goto failed_sdio_probe;
    }
#endif

    /* disable hcc default */
    oal_atomic_set(&hcc->state, HCC_OFF);

    OAL_WAIT_QUEUE_INIT_HEAD(&hcc->hcc_transer_info.hcc_transfer_wq);    /* queues init */
    hcc_transfer_queues_init(hcc);

    hcc_trans_flow_ctrl_info_init(hcc);

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
    g_pst_alg_process_func.p_auto_freq_count_func = OAL_PTR_NULL;
    g_pst_alg_process_func.p_auto_freq_process_func = OAL_PTR_NULL;
    g_pst_alg_process_func.p_auto_freq_set_lock_mod_func = OAL_PTR_NULL;
    g_pst_alg_process_func.p_auto_freq_adjust_to_level_func = OAL_PTR_NULL;
#endif

#ifndef WIN32
    OAL_MUTEX_INIT(&hcc->tx_transfer_lock);
#endif

    OAL_WAIT_QUEUE_INIT_HEAD(&hcc->hcc_transer_info.tx_flow_ctrl.wait_queue);

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#if !defined(_PRE_WLAN_TCP_OPT) || defined(WIN32)
    hcc->hcc_transer_info.hcc_transfer_thread = oal_kthread_create("hisi_hcc", hcc_transfer_thread, hcc, NULL);
    if (!hcc->hcc_transer_info.hcc_transfer_thread) {
        OAL_IO_PRINT("hcc thread create failed!\n");
        goto failed_create_hcc_thread;
    }

#endif
#endif

#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    if (oal_channel_transfer_rx_register(hcc->hi_channel, sdio_transfer_rx_handler) != OAL_SUCC) {
#endif
        OAL_IO_PRINT("sdio rx transfer callback register failed!\n");
        goto failed_rx_cb_reg;
    }

    hcc_assem_info_init(hcc);
    hcc_trans_limit_parm_init(hcc);
    hcc_trans_send_mode_init(hcc);

    OAL_BUG_ON(!OAL_IS_ALIGNED(HISDIO_HOST2DEV_SCATT_SIZE, HISDIO_H2D_SCATT_BUFFLEN_ALIGN));

    if (hcc_tx_assem_descr_init(hcc) != OAL_SUCC) {
        OAL_IO_PRINT("hcc tx assem descrt alloc failed!\n");
        goto failed_tx_assem_descr_alloc;
    }

    if (hcc_message_register(hcc, D2H_MSG_FLOWCTRL_ON, hcc_flow_on_callback, hcc) != OAL_SUCC) {
        OAL_IO_PRINT("register flow ctrl on failed!\n");
        goto failed_reg_flowon_msg;
    }

    if (hcc_message_register(hcc, D2H_MSG_FLOWCTRL_OFF, hcc_flow_off_callback, hcc) != OAL_SUCC) {
        OAL_IO_PRINT("register flow ctrl off failed!\n");
        goto failed_reg_flowoff_msg;
    }

#if (!((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && (_PRE_FEATURE_USB == _PRE_FEATURE_CHANNEL_TYPE)))
    hcc_message_register(hcc, D2H_MSG_CREDIT_UPDATE, hcc_credit_update_callback, hcc);
#endif

    hcc_message_register(hcc, D2H_MSG_HIGH_PKT_LOSS, hcc_high_pkts_loss_callback, hcc);

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if (hcc_test_init_module(hcc) != OAL_SUCC) {
        OAL_IO_PRINT("register flow ctrl off failed!\n");
        goto failed_hcc_test_init;
    }
#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#endif

    oal_wake_lock_init(&hcc->tx_wake_lock, "hcc_tx");

    /* hcc_tc is used to test! */
    hcc_tc = hcc;

    return hcc;

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
failed_hcc_test_init:
    hcc_message_unregister(hcc, D2H_MSG_FLOWCTRL_OFF);
#endif

failed_reg_flowoff_msg:
    hcc_message_unregister(hcc, D2H_MSG_FLOWCTRL_ON);
failed_reg_flowon_msg:
    hcc_tx_assem_descr_exit(hcc);
failed_tx_assem_descr_alloc:
    oal_channel_transfer_rx_unregister(hcc->hi_channel);
failed_rx_cb_reg:
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#if !defined(_PRE_WLAN_TCP_OPT) || defined(WIN32)
    oal_kthread_stop(hcc->hcc_transer_info.hcc_transfer_thread);
    hcc->hcc_transer_info.hcc_transfer_thread = NULL;
#endif
#endif
#if !defined(_PRE_WLAN_TCP_OPT) || defined(WIN32)
failed_create_hcc_thread:
#endif

#ifndef WIN32
    OAL_MUTEX_DESTROY(&hcc->tx_transfer_lock);
#endif

#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
    oal_channel_func_remove(hcc->hi_channel);
#endif
failed_sdio_probe:
    chr_exception(chr_wifi_dev(CHR_WIFI_DEV_EVENT_CHIP, CHR_WIFI_DEV_ERROR_SDIO_ENUM));
    oal_channel_exit_module(hcc->hi_channel);
failed_sdio_init:
    oal_free(hcc);
    /*lint +e801*/
    return NULL;
}


oal_void hcc_module_exit(struct hcc_handler* hcc)
{
    oal_wake_lock_exit(&hcc->tx_wake_lock);

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    hcc_test_exit_module(hcc);
#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    /* destroy hcc_test thread when done! */
    // hcc_test_exit_module(hcc);
#endif

    hcc_message_unregister(hcc, D2H_MSG_FLOWCTRL_OFF);
    hcc_message_unregister(hcc, D2H_MSG_FLOWCTRL_ON);
    hcc_tx_assem_descr_exit(hcc);

    oal_channel_transfer_rx_unregister(hcc->hi_channel);

#if !defined(_PRE_WLAN_TCP_OPT) || defined(WIN32)
    oal_kthread_stop(hcc->hcc_transer_info.hcc_transfer_thread);
    hcc->hcc_transer_info.hcc_transfer_thread = NULL;
#endif
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)||(_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    OAL_MUTEX_DESTROY(&hcc->tx_transfer_lock);
#endif

    oal_timer_delete_sync(&hcc->hcc_transer_info.tx_flow_ctrl.flow_timer);
    oal_cancel_delayed_work_sync(&hcc->hcc_transer_info.tx_flow_ctrl.worker);

    oal_channel_func_remove(hcc->hi_channel);
    oal_channel_exit_module(hcc->hi_channel);
    oal_free(hcc);
    hcc_tc = NULL;
}

#ifdef CONFIG_PRINTK
oal_void hcc_sched_transfer_test(oal_void)
{
    hcc_sched_transfer(hcc_tc);
}

oal_void hcc_dev_flowctrl_on_test(oal_void)
{
    if (hcc_tc != NULL)
        hcc_dev_flowctrl_on(hcc_tc, 1);
}
#endif
#ifdef CONFIG_MMC
oal_void hcc_device_info_dump(oal_void)
{
    OAL_IO_PRINT("hcc_device_info_dump\n");
    oal_channel_send_msg(hcc_tc->hi_channel, H2D_MSG_DEVICE_INFO_DUMP);
}

oal_void hcc_device_mem_dump(oal_void)
{
    OAL_IO_PRINT("hcc_device_mem_dump\n");
    oal_channel_send_msg(hcc_tc->hi_channel, H2D_MSG_DEVICE_MEM_DUMP);
}
#endif
/*lint -e19 */
oal_void hcc_trigger_device_panic(oal_void)
{
#ifdef CONFIG_MMC
    wlan_pm_disable();
    oal_channel_send_msg(hcc_tc->hi_channel, H2D_MSG_TEST);
    wlan_pm_enable();
#endif
}
oal_module_symbol(hcc_trigger_device_panic);

oal_void hcc_print_device_mem_info(oal_void)
{
#ifdef CONFIG_MMC
    wlan_pm_disable();
    oal_channel_send_msg(hcc_tc->hi_channel, H2D_MSG_DEVICE_MEM_INFO);

    wlan_pm_enable();
#endif
}
oal_module_symbol(hcc_print_device_mem_info);

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
oal_module_symbol(hcc_host_set_flowctl_param);
oal_module_symbol(hcc_host_get_flowctl_param);
oal_module_symbol(hcc_host_get_flowctl_stat);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
