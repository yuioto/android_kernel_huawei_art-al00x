
#include "frw_timer.h"
#include "frw_main.h"
#include "frw_task.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_FRW_TIMER_C

/* ****************************************************************************
  2 全局变量定义
**************************************************************************** */
oal_dlist_head_stru g_ast_timer_list[WLAN_FRW_MAX_NUM_CORES];
oal_spin_lock_stru g_ast_timer_list_spinlock[WLAN_FRW_MAX_NUM_CORES];
oal_timer_list_stru g_st_timer[WLAN_FRW_MAX_NUM_CORES];
oal_uint32 g_stop_timestamp = 0;
oal_uint32 g_need_restart = OAL_FALSE;
oal_uint32 g_frw_timer_start_stamp[WLAN_FRW_MAX_NUM_CORES] = {0};  // 维测信号，用来记录下一次软中断定时器的启动时间

#ifdef _PRE_DEBUG_MODE

oal_uint32 g_ul_os_time = 0;
frw_timeout_track_stru g_st_timeout_track[FRW_TIMEOUT_TRACK_NUM];
oal_uint8 g_uc_timeout_track_idx = 0;
#endif
/* ****************************************************************************
  3 函数实现
**************************************************************************** */
OAL_STATIC OAL_INLINE oal_void __frw_timer_immediate_destroy_timer(oal_uint32 ul_file_id,
                                                                   oal_uint32 ul_line_num,
                                                                   frw_timeout_stru *pst_timeout);


oal_void frw_timer_init(oal_uint32 ul_delay, oal_timer_func p_func, oal_ulong ui_arg)
{
    oal_uint32 ul_core_id;

    for (ul_core_id = 0; ul_core_id < WLAN_FRW_MAX_NUM_CORES; ul_core_id++) {
        oal_dlist_init_head(&g_ast_timer_list[ul_core_id]);
        oal_spin_lock_init(&g_ast_timer_list_spinlock[ul_core_id]);
        oal_timer_init(&g_st_timer[ul_core_id], ul_delay, p_func, ui_arg);
        g_frw_timer_start_stamp[ul_core_id] = 0;
    }
#ifdef _PRE_DEBUG_MODE
    memset_s(g_st_timeout_track,
             OAL_SIZEOF(frw_timeout_track_stru) * FRW_TIMEOUT_TRACK_NUM, 0,
             OAL_SIZEOF(frw_timeout_track_stru) * FRW_TIMEOUT_TRACK_NUM);
#endif
}


oal_void frw_timer_exit(oal_void)
{
    oal_uint32 ul_core_id;
    for (ul_core_id = 0; ul_core_id < WLAN_FRW_MAX_NUM_CORES; ul_core_id++) {
        oal_timer_delete_sync(&g_st_timer[ul_core_id]);
        g_frw_timer_start_stamp[ul_core_id] = 0;
    }
}


OAL_STATIC oal_void frw_timer_dump(oal_uint32 ul_core_id)
{
    oal_dlist_head_stru *pst_timeout_entry;
    frw_timeout_stru *pst_timeout_element = OAL_PTR_NULL;

    pst_timeout_entry = g_ast_timer_list[ul_core_id].pst_next;
    while (pst_timeout_entry != &g_ast_timer_list[ul_core_id]) {
        if (pst_timeout_entry == OAL_PTR_NULL) {
            OAM_ERROR_LOG0(0, OAM_SF_FRW, "{frw_timer_dump:: time broken break}");
            break;
        }

        if (pst_timeout_entry->pst_next == NULL) {
            /* If next is null,
             the pst_timeout_entry stru maybe released or memset */
            OAM_ERROR_LOG0(0, OAM_SF_FRW, "{frw_timer_dump:: pst_next is null,dump mem}");
            // 偏移64，每次dump 128 per 32
            oal_print_hex_dump(((oal_uint8 *)pst_timeout_entry) - 64, 128, 32, "timer broken: ");
        }

        pst_timeout_element = OAL_DLIST_GET_ENTRY(pst_timeout_entry, frw_timeout_stru, st_entry);
        pst_timeout_entry = pst_timeout_entry->pst_next;
        OAM_ERROR_LOG4(0, OAM_SF_FRW, "{frw_timer_dump:: time_stamp[0x%x] timeout[%d] deleting[%d] enabled[%d]}", 
                       pst_timeout_element->ul_time_stamp, 
                       pst_timeout_element->ul_timeout, 
                       pst_timeout_element->en_is_deleting, 
                       pst_timeout_element->en_is_enabled);
        OAM_ERROR_LOG3(0, OAM_SF_FRW, "{frw_timer_dump:: module_id[%d] file_id[%d] line_num[%d]}", 
                       pst_timeout_element->en_module_id, 
                       pst_timeout_element->ul_file_id, 
                       pst_timeout_element->ul_line_num);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef CONFIG_PRINTK
        if (pst_timeout_element->p_func)
            printk(KERN_ERR "frw_timer_dump func : %pF\n", pst_timeout_element->p_func);
#endif
#endif
    }
}

/*
 * 函 数 名  : frw_timer_timeout_proc
 * 功能描述  : 遍历timer链表执行到期超时函数
 */
oal_uint32 frw_timer_timeout_proc(frw_event_mem_stru *pst_timeout_event)
{
    oal_dlist_head_stru *pst_timeout_entry = NULL;
    frw_timeout_stru *pst_timeout_element = NULL;
    oal_uint32 ul_present_time;
    oal_uint32 ul_end_time;
    oal_uint32 ul_runtime;
    oal_uint32 ul_core_id;
    oal_uint32 ul_runtime_func_start;
    oal_uint32 ul_runtime_func_end;
    oal_uint32 ul_endtime_func;
    oal_uint32 ul_frw_timer_start;

    if (OAL_UNLIKELY(pst_timeout_event == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_FRW, "{frw_timer_timeout_proc:: pst_timeout_event is null ptr!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_present_time = (oal_uint32)OAL_TIME_GET_STAMP_MS();
    ul_core_id = OAL_GET_CORE_ID();

    /* 执行超时定时器 */
    oal_spin_lock_bh(&g_ast_timer_list_spinlock[ul_core_id]);
    pst_timeout_entry = g_ast_timer_list[ul_core_id].pst_next;

    while (pst_timeout_entry != &g_ast_timer_list[ul_core_id]) {
        if (pst_timeout_entry == OAL_PTR_NULL) {
            OAM_ERROR_LOG0(0, OAM_SF_FRW, "{frw_timer_timeout_proc:: the timer list is broken! }");
            frw_timer_dump(ul_core_id);
            break;  //lint !e527
        }

        pst_timeout_element = OAL_DLIST_GET_ENTRY(pst_timeout_entry, frw_timeout_stru, st_entry);
        pst_timeout_element->ul_curr_time_stamp = ul_present_time;

        /*
         * 一个定时器超时处理函数中创建新的定时器，如果定时器超时，则将相应的定时器进行删除，取消en_is_deleting标记;
         */
        if (frw_time_after(ul_present_time, pst_timeout_element->ul_time_stamp)) {
            /* 删除超时定时器，如果是周期定时器，则将其再添加进去:delete first,then add periodic_timer */
            pst_timeout_element->en_is_registerd = OAL_FALSE;
            oal_dlist_delete_entry(&pst_timeout_element->st_entry);

            if ((pst_timeout_element->en_is_periodic == OAL_TRUE) ||
                (pst_timeout_element->en_is_enabled == OAL_FALSE)) {
                pst_timeout_element->ul_time_stamp = ul_present_time + pst_timeout_element->ul_timeout;
                pst_timeout_element->en_is_registerd = OAL_TRUE;
                frw_timer_add_timer(pst_timeout_element);
            }
            ul_runtime_func_start = (oal_uint32)OAL_TIME_GET_STAMP_MS();
            if (pst_timeout_element->en_is_enabled) {
                oal_spin_unlock_bh(&g_ast_timer_list_spinlock[ul_core_id]);
                pst_timeout_element->p_func(pst_timeout_element->p_timeout_arg);
                oal_spin_lock_bh(&g_ast_timer_list_spinlock[ul_core_id]);
            }

            ul_endtime_func = (oal_uint32)OAL_TIME_GET_STAMP_MS();
            ul_runtime_func_end = (oal_uint32)OAL_TIME_GET_RUNTIME(ul_runtime_func_start, ul_endtime_func);
            if (ul_runtime_func_end >= (oal_uint32)OAL_JIFFIES_TO_MSECS(2)) { /* 2 */
                OAM_WARNING_LOG4(0, OAM_SF_FRW,
                                 "{frw_timer_timeout_proc:: fileid=%u, linenum=%u, moduleid=%u, runtime=%u}",
                                 pst_timeout_element->ul_file_id,
                                 pst_timeout_element->ul_line_num,
                                 pst_timeout_element->en_module_id,
                                 ul_runtime_func_end);
            }
        } else {
            break;
        }
        pst_timeout_entry = g_ast_timer_list[ul_core_id].pst_next;
    }

    /* 获得链表的最小超时时间，重启定时器 */
    if (oal_dlist_is_empty(&g_ast_timer_list[ul_core_id]) == OAL_FALSE) {
        pst_timeout_entry = g_ast_timer_list[ul_core_id].pst_next;
        pst_timeout_element = OAL_DLIST_GET_ENTRY(pst_timeout_entry, frw_timeout_stru, st_entry);
        ul_present_time = (oal_uint32)OAL_TIME_GET_STAMP_MS();
        if (frw_time_after(pst_timeout_element->ul_time_stamp, ul_present_time)) {
            ul_frw_timer_start = (oal_uint32)OAL_TIME_GET_RUNTIME(ul_present_time, pst_timeout_element->ul_time_stamp);

            g_frw_timer_start_stamp[ul_core_id] = pst_timeout_element->ul_time_stamp;
        } else {
            ul_frw_timer_start = FRW_TIMER_DEFAULT_TIME;

            g_frw_timer_start_stamp[ul_core_id] = (ul_present_time + FRW_TIMER_DEFAULT_TIME);
        }

        oal_timer_start(&g_st_timer[ul_core_id], ul_frw_timer_start);
    } else {
        g_frw_timer_start_stamp[ul_core_id] = 0;
    }

    oal_spin_unlock_bh(&g_ast_timer_list_spinlock[ul_core_id]);

    ul_end_time = (oal_uint32)OAL_TIME_GET_STAMP_MS();
    ul_runtime = (oal_uint32)OAL_TIME_GET_RUNTIME(ul_present_time, ul_end_time);
    /* 同device侧检测日志时限一致 */
    if (ul_runtime > (oal_uint32)OAL_JIFFIES_TO_MSECS(2)) { /* 2 */
        OAM_WARNING_LOG1(0, OAM_SF_FRW, "{frw_timer_timeout_proc:: timeout process exucte time too long time[%d]}",
                         ul_runtime);
    }

    return OAL_SUCC;
}

/*
 * 函 数 名  : frw_timer_add_in_order
 * 功能描述  : 向链表中按从小到大的顺序插入节点
 * 输入参数  : pst_new: 要插入的新节点
 *             pst_head: 链表头指针
 */
OAL_STATIC oal_void frw_timer_add_in_order(oal_dlist_head_stru *pst_new, oal_dlist_head_stru *pst_head)
{
    oal_dlist_head_stru *pst_timeout_entry = NULL;
    frw_timeout_stru *pst_timeout_element = NULL;
    frw_timeout_stru *pst_timeout_element_new = NULL;
    oal_uint32 ul_core_id;

    pst_timeout_element_new = OAL_DLIST_GET_ENTRY(pst_new, frw_timeout_stru, st_entry);

    ul_core_id = OAL_GET_CORE_ID();

    /* 搜索链表，查找第一个比pst_timeout_element_new->ul_time_stamp大的位置 */
    if (pst_head != NULL) {
        pst_timeout_entry = pst_head->pst_next;

        while (pst_timeout_entry != pst_head) {
            if (pst_timeout_entry == OAL_PTR_NULL) {
                OAM_ERROR_LOG0(0, OAM_SF_FRW, "{Driver frw_timer_add_in_order:: the timer list is broken! }");
                OAM_ERROR_LOG3(0, OAM_SF_FRW,
                               "{new frw_timer_add_in_order:: time_stamp[0x%x] timeout[%d]  enabled[%d]}",
                               pst_timeout_element_new->ul_time_stamp,
                               pst_timeout_element_new->ul_timeout,
                               pst_timeout_element_new->en_is_enabled);
                OAM_ERROR_LOG3(0, OAM_SF_FRW, "{new frw_timer_add_in_order:: module_id[%d] file_id[%d] line_num[%d]}",
                               pst_timeout_element_new->en_module_id,
                               pst_timeout_element_new->ul_file_id,
                               pst_timeout_element_new->ul_line_num);
                frw_timer_dump(ul_core_id);
                break;
            }

            pst_timeout_element = OAL_DLIST_GET_ENTRY(pst_timeout_entry, frw_timeout_stru, st_entry);
            if (frw_time_after(pst_timeout_element->ul_time_stamp, pst_timeout_element_new->ul_time_stamp)) {
                break;
            }

            pst_timeout_entry = pst_timeout_entry->pst_next;
        }

        if ((pst_timeout_entry != NULL) && (pst_timeout_entry->pst_prev != NULL)) {
            oal_dlist_add(pst_new, pst_timeout_entry->pst_prev, pst_timeout_entry);
        } else {
            OAM_ERROR_LOG0(0, OAM_SF_FRW, "{Driver frw_timer_add_in_order::timer list is broken !}");
        }
    }
}

oal_void frw_timer_add_timer(frw_timeout_stru *pst_timeout)
{
    oal_int32 l_val;

    if (pst_timeout == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_FRW, "{frw_timer_add_timer:: OAL_PTR_NULL == pst_timeout}");
        return;
    }

    if (oal_dlist_is_empty(&g_ast_timer_list[pst_timeout->ul_core_id]) == OAL_TRUE) {
        g_frw_timer_start_stamp[pst_timeout->ul_core_id] = 0;
    }

    /* 将Frw的无序链表改为有序 */
    frw_timer_add_in_order(&pst_timeout->st_entry, &g_ast_timer_list[pst_timeout->ul_core_id]);

    l_val = frw_time_after(g_frw_timer_start_stamp[pst_timeout->ul_core_id], pst_timeout->ul_time_stamp);
    if ((g_frw_timer_start_stamp[pst_timeout->ul_core_id] == 0) || (l_val > 0)) {
        oal_timer_start(&g_st_timer[pst_timeout->ul_core_id], pst_timeout->ul_timeout);
        g_frw_timer_start_stamp[pst_timeout->ul_core_id] = pst_timeout->ul_time_stamp;
    }

    return;
}


oal_void  frw_timer_create_timer(
    oal_uint32 ul_file_id,
    oal_uint32 ul_line_num,
    frw_timeout_stru *pst_timeout,
    frw_timeout_func  p_timeout_func,
    oal_uint32 ul_timeout,
    oal_void *p_timeout_arg,
    oal_bool_enum_uint8  en_is_periodic,
    oam_module_id_enum_uint16   en_module_id,
    oal_uint32 ul_core_id)
{
    if (pst_timeout == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_FRW, "{frw_timer_create_timer:: OAL_PTR_NULL == pst_timeout}");
        return;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    pst_timeout->ul_core_id = 0;
#else
    pst_timeout->ul_core_id = ul_core_id;
#endif

    oal_spin_lock_bh(&g_ast_timer_list_spinlock[pst_timeout->ul_core_id]);

    pst_timeout->p_func = p_timeout_func;
    pst_timeout->p_timeout_arg = p_timeout_arg;
    pst_timeout->ul_timeout = ul_timeout;
    pst_timeout->ul_time_stamp = (oal_uint32)OAL_TIME_GET_STAMP_MS() + ul_timeout;
    pst_timeout->en_is_periodic = en_is_periodic;
    pst_timeout->en_module_id = en_module_id;
    pst_timeout->ul_file_id = ul_file_id;
    pst_timeout->ul_line_num = ul_line_num;
    pst_timeout->en_is_enabled = OAL_TRUE; /* 默认使能 */
    pst_timeout->en_is_deleting = OAL_FALSE;

    if (pst_timeout->en_is_registerd != OAL_TRUE) {
        pst_timeout->en_is_running = OAL_FALSE;
        pst_timeout->en_is_registerd = OAL_TRUE; /* 默认使能 */
        frw_timer_add_timer(pst_timeout);
    } else {
        oal_dlist_delete_entry(&pst_timeout->st_entry);
        frw_timer_add_timer(pst_timeout);
    }

    oal_spin_unlock_bh(&g_ast_timer_list_spinlock[pst_timeout->ul_core_id]);

    return;
}

/*
 * 函 数 名  : __frw_timer_immediate_destroy_timer
 * 功能描述  : 立即删除定时器，无锁
 */
OAL_STATIC OAL_INLINE oal_void __frw_timer_immediate_destroy_timer(oal_uint32 ul_file_id,
                                                                   oal_uint32 ul_line_num,
                                                                   frw_timeout_stru *pst_timeout)
{
    if (pst_timeout->st_entry.pst_prev == OAL_PTR_NULL || pst_timeout->st_entry.pst_next == OAL_PTR_NULL) {
        return;
    }

    if (pst_timeout->en_is_registerd == OAL_FALSE) {
        OAM_WARNING_LOG0(0, OAM_SF_FRW,
                         "{frw_timer_immediate_destroy_timer::This timer is not enabled it should not be deleted}");

        return;
    }

    pst_timeout->en_is_enabled = OAL_FALSE;
    pst_timeout->en_is_registerd = OAL_FALSE;

    oal_dlist_delete_entry(&pst_timeout->st_entry);

    if (oal_dlist_is_empty(&g_ast_timer_list[pst_timeout->ul_core_id]) == OAL_TRUE) {
        g_frw_timer_start_stamp[pst_timeout->ul_core_id] = 0;
    }
}

/*
 * 函 数 名  : frw_timer_immediate_destroy_timer
 * 功能描述  : 立即删除定时器,加锁处理
 */
oal_void frw_timer_immediate_destroy_timer(oal_uint32 ul_file_id,
                                           oal_uint32 ul_line_num,
                                           frw_timeout_stru *pst_timeout)
{
    oal_spin_lock_bh(&g_ast_timer_list_spinlock[pst_timeout->ul_core_id]);
    __frw_timer_immediate_destroy_timer(ul_file_id, ul_line_num, pst_timeout);
    oal_spin_unlock_bh(&g_ast_timer_list_spinlock[pst_timeout->ul_core_id]);
}

oal_void frw_timer_restart_timer(frw_timeout_stru *pst_timeout, oal_uint32 ul_timeout,
                                 oal_bool_enum_uint8 en_is_periodic)
{
    if (pst_timeout == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_FRW, "{frw_timer_restart_timer:: OAL_PTR_NULL == pst_timeout}");
        return;
    }
    /* 删除当前定时器 */
    if (pst_timeout->st_entry.pst_prev == OAL_PTR_NULL || pst_timeout->st_entry.pst_next == OAL_PTR_NULL) {
        OAM_ERROR_LOG4(0, OAM_SF_FRW,
                       "{frw_timer_restart_timer::This timer has been deleted!file_id=%d,line=%d,core=%d,mod=%d}",
                       pst_timeout->ul_file_id, pst_timeout->ul_line_num,
                       pst_timeout->ul_core_id, pst_timeout->en_module_id);
        return;
    }

    if (pst_timeout->en_is_registerd == OAL_FALSE) {
        OAM_ERROR_LOG4(0, OAM_SF_FRW,
                       "{frw_timer_restart_timer::This timer is not registerd!file_id=%d,line=%d,core=%d,mod=%d}",
                       pst_timeout->ul_file_id, pst_timeout->ul_line_num,
                       pst_timeout->ul_core_id, pst_timeout->en_module_id);
        return;
    }

    oal_spin_lock_bh(&g_ast_timer_list_spinlock[pst_timeout->ul_core_id]);
    oal_dlist_delete_entry(&pst_timeout->st_entry);

    pst_timeout->ul_time_stamp = (oal_uint32)OAL_TIME_GET_STAMP_MS() + ul_timeout;
    pst_timeout->ul_timeout = ul_timeout;
    pst_timeout->en_is_periodic = en_is_periodic;
    pst_timeout->en_is_enabled = OAL_TRUE;

    frw_timer_add_timer(pst_timeout);
    oal_spin_unlock_bh(&g_ast_timer_list_spinlock[pst_timeout->ul_core_id]);
}



oal_void frw_timer_stop_timer(frw_timeout_stru *pst_timeout)
{
    if (pst_timeout == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_FRW, "{frw_timer_stop_timer:: OAL_PTR_NULL == pst_timeout}");
        return;
    }

    pst_timeout->en_is_enabled = OAL_FALSE;
}


oal_uint8 g_uc_timer_pause = OAL_FALSE;
#if defined(_PRE_FRW_TIMER_BIND_CPU) && defined(CONFIG_NR_CPUS)
oal_uint32 g_ul_frw_timer_cpu_count[CONFIG_NR_CPUS] = {0};
oal_uint32 frw_timer_get_cpu_count(oal_uint32 configNrCpus)
{
    return g_ul_frw_timer_cpu_count[configNrCpus];
}
#endif
oal_uint32  g_auc_timer_debug[4] = {0}; // 4长字空间
EXPORT_SYMBOL_GPL(g_auc_timer_debug);


OAL_STATIC uint8_t frw_dispatch_timeout_event(uint32_t core_id)
{
    frw_event_mem_stru *event_mem = OAL_PTR_NULL;
    frw_event_stru *event = OAL_PTR_NULL;
    event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(frw_event_stru));
    /* 返回值检查 */
    if (OAL_UNLIKELY(event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_FRW, "{frw_dispatch_timeout_event:: FRW_EVENT_ALLOC failed!}");
        return OAL_FALSE;
    }

    event = (frw_event_stru *)event_mem->puc_data;

    /* 填充事件头 */
    FRW_FIELD_SETUP((&event->st_event_hdr), en_type, (FRW_EVENT_TYPE_TIMEOUT));
    FRW_FIELD_SETUP((&event->st_event_hdr), uc_sub_type, (FRW_TIMEOUT_TIMER_EVENT));
    FRW_FIELD_SETUP((&event->st_event_hdr), us_length, (WLAN_MEM_EVENT_SIZE1));
    FRW_FIELD_SETUP((&event->st_event_hdr), en_pipeline, (FRW_EVENT_PIPELINE_STAGE_0));
    FRW_FIELD_SETUP((&event->st_event_hdr), uc_chip_id, 0);
    FRW_FIELD_SETUP((&event->st_event_hdr), uc_device_id, 0);
    FRW_FIELD_SETUP((&event->st_event_hdr), uc_vap_id, 0);

    /* 抛事件 */
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
    frw_event_post_event(event_mem, core_id);
#else
    frw_event_dispatch_event(event_mem);
#endif
    FRW_EVENT_FREE(event_mem);
    return OAL_TRUE;
}


oal_void frw_timer_timeout_proc_event(oal_ulong ui_arg)
{
    oal_uint32 ul_core_id = 0;

#if defined(_PRE_FRW_TIMER_BIND_CPU) && defined(CONFIG_NR_CPUS)
    do {
        oal_uint32 cpu_id = smp_processor_id();
        if (cpu_id < CONFIG_NR_CPUS) {
            g_ul_frw_timer_cpu_count[cpu_id]++;
        }
    } while (0);
#endif

    g_auc_timer_debug[0]++;
    if (g_uc_timer_pause == OAL_TRUE) {
        return;
    }

    /*lint -e539*/ /*lint -e830*/
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
    for (ul_core_id = 0; ul_core_id < WLAN_FRW_MAX_NUM_CORES; ul_core_id++) {
        if (frw_task_get_state(ul_core_id)) {
#endif
            /* 如果定时器链表为空，说明定时器已经被删除，超时函数不应该再继续发送超时事件，否则会唤醒hisi_frw0线程 */
            if (oal_dlist_is_empty(&g_ast_timer_list[ul_core_id])) {
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
                continue;
#endif
            }
            g_auc_timer_debug[1]++;
            if (frw_dispatch_timeout_event(ul_core_id) == OAL_FALSE) {
                return;
            }
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
        }
    }
#endif
    /*lint +e539*/ /*lint +e830*/
}



oal_void frw_timer_delete_all_timer(oal_void)
{
    oal_dlist_head_stru *pst_timeout_entry = OAL_PTR_NULL;
    frw_timeout_stru *pst_timeout_element = OAL_PTR_NULL;

    oal_uint32 ul_core_id;

    for (ul_core_id = 0; ul_core_id < WLAN_FRW_MAX_NUM_CORES; ul_core_id++) {
        oal_spin_lock_bh(&g_ast_timer_list_spinlock[ul_core_id]);
        /* 删除所有待删除定时器 */
        pst_timeout_entry = g_ast_timer_list[ul_core_id].pst_next;

        while (pst_timeout_entry != &g_ast_timer_list[ul_core_id]) {
            pst_timeout_element = OAL_DLIST_GET_ENTRY(pst_timeout_entry, frw_timeout_stru, st_entry);

            pst_timeout_entry = pst_timeout_entry->pst_next;

            /* 删除定时器 */
            oal_dlist_delete_entry(&pst_timeout_element->st_entry);
        }
        g_frw_timer_start_stamp[ul_core_id] = 0;
        oal_spin_unlock_bh(&g_ast_timer_list_spinlock[ul_core_id]);
    }
}

/*
 * 函 数 名  : frw_timer_clean_timer
 * 功能描述  : 删除指定模块残留的所有定时器
 *             本函数不能解决残留定时器的所有问题，一旦发现有残留，需要进行处理。
 */
oal_void frw_timer_clean_timer(oam_module_id_enum_uint16 en_module_id)
{
    oal_dlist_head_stru *pst_timeout_entry = NULL;
    frw_timeout_stru *pst_timeout_element = NULL;
    oal_uint32 ul_core_id;

    for (ul_core_id = 0; ul_core_id < WLAN_FRW_MAX_NUM_CORES; ul_core_id++) {
        oal_spin_lock_bh(&g_ast_timer_list_spinlock[ul_core_id]);
        pst_timeout_entry = g_ast_timer_list[ul_core_id].pst_next;

        while (pst_timeout_entry != &g_ast_timer_list[ul_core_id]) {
            if (pst_timeout_entry == NULL) {
                OAL_IO_PRINT("!!!====TIMER LIST BROKEN====!!!\n");
                break;
            }

            pst_timeout_element = OAL_DLIST_GET_ENTRY(pst_timeout_entry, frw_timeout_stru, st_entry);
            pst_timeout_entry = pst_timeout_entry->pst_next;

            if (en_module_id == pst_timeout_element->en_module_id) {
                oal_dlist_delete_entry(&pst_timeout_element->st_entry);
            }
        }

        if (oal_dlist_is_empty(&g_ast_timer_list[ul_core_id])) {
            g_frw_timer_start_stamp[ul_core_id] = 0;
        }
        oal_spin_unlock_bh(&g_ast_timer_list_spinlock[ul_core_id]);
    }
}


oal_void frw_timer_dump_timer(oal_uint32 ul_core_id)
{
    oal_dlist_head_stru *pst_dlist_entry = OAL_PTR_NULL;
    frw_timeout_stru *pst_timer = OAL_PTR_NULL;
    oal_uint32 ul_cnt = 0;

    OAM_WARNING_LOG0(0, OAM_SF_ANY, "frw_timer_dump_timer::timer dump start.");
    OAL_DLIST_SEARCH_FOR_EACH(pst_dlist_entry, &g_ast_timer_list[ul_core_id])
    {
        pst_timer = OAL_DLIST_GET_ENTRY(pst_dlist_entry, frw_timeout_stru, st_entry);

        OAM_WARNING_LOG4(0, OAM_SF_ANY, "TIMER NO.%d, file id[%d], line num[%d], func addr[0x%08x]",
                         ul_cnt,
                         pst_timer->ul_file_id,
                         pst_timer->ul_line_num,
                         (uintptr_t)pst_timer->p_func);
        OAM_WARNING_LOG4(0, OAM_SF_ANY, "tiemr enabled[%d], running[%d], registerd[%d], deleting[%d]",
                         pst_timer->en_is_enabled,
                         pst_timer->en_is_running,
                         pst_timer->en_is_registerd,
                         pst_timer->en_is_deleting);
        OAM_WARNING_LOG4(0, OAM_SF_ANY, "timerout[%u], start timer stamp[%u], curr timer stamp[%u], period[%d]",
                         pst_timer->ul_timeout,
                         pst_timer->ul_time_stamp,
                         pst_timer->ul_curr_time_stamp,
                         pst_timer->en_is_periodic);
        ul_cnt++;
    }
    OAM_WARNING_LOG0(0, OAM_SF_ANY, "frw_timer_dump_timer::timer dump end.");
}

/*lint -e578*/ /*lint -e19*/
oal_module_symbol(frw_timer_restart_timer);
oal_module_symbol(frw_timer_create_timer);
oal_module_symbol(frw_timer_dump_timer);
oal_module_symbol(frw_timer_stop_timer);
oal_module_symbol(frw_timer_add_timer);
oal_module_symbol(frw_timer_immediate_destroy_timer);
oal_module_symbol(frw_timer_delete_all_timer);
oal_module_symbol(frw_timer_exit);
oal_module_symbol(frw_timer_clean_timer);
oal_module_symbol(g_uc_timer_pause);

#ifdef _PRE_DEBUG_MODE
oal_module_symbol(g_st_timeout_track);
oal_module_symbol(g_uc_timeout_track_idx);
#endif


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
