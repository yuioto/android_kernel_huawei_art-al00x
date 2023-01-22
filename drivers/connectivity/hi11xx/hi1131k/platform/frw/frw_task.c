
#include "platform_spec.h"
#include "frw_main.h"
#include "frw_event_main.h"
#include "frw_task.h"
#include "hal_ext_if.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_FRW_TASK_C

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#define FRW_TASK_PRIO 1
#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
// performance_opt:wifi_driver thread prio :10->5
#define FRW_TASK_PRIO (5)
#endif
#define FRW_TASK_SIZE 0x2000

#define FRW_THREAD_NAME_MAX_SIZE 20

/* ****************************************************************************
  2 全局变量定义
**************************************************************************** */
oal_int8 g_frw_thread_name[WLAN_FRW_MAX_NUM_CORES][FRW_THREAD_NAME_MAX_SIZE] = { "0" };
/* *****************************************************************************
    事件处理全局变量
****************************************************************************** */
frw_task_stru g_ast_event_task[WLAN_FRW_MAX_NUM_CORES];

/* ****************************************************************************
  3 函数实现
**************************************************************************** */
#if (_PRE_FRW_FEATURE_PROCCESS_ENTITY_TYPE == _PRE_FRW_FEATURE_PROCCESS_ENTITY_THREAD)


OAL_STATIC oal_int32 frw_task_thread(oal_void *arg)
{
    oal_uint32 ul_bind_cpu = (oal_uint32)(uintptr_t)arg;
    oal_int32 ret;
#if (_PRE_FRW_FEATURE_PROCCESS_ENTITY_TYPE == _PRE_FRW_FEATURE_PROCCESS_ENTITY_THREAD)
    oal_uint32 ul_empty_count = 0;
#endif
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    allow_signal(SIGTERM);
#endif
    for (;;) {
#if (_PRE_FRW_FEATURE_PROCCESS_ENTITY_TYPE == _PRE_FRW_FEATURE_PROCCESS_ENTITY_THREAD)
        oal_uint32 ul_event_count;
#endif
        if (oal_kthread_should_stop()) {
            break;
        }

        /* state为TASK_INTERRUPTIBLE，condition不成立则线程阻塞，直到被唤醒进入waitqueue */
        /*lint -e730*/
#ifdef _PRE_FRW_EVENT_PROCESS_TRACE_DEBUG
        frw_event_last_pc_trace(__FUNCTION__, __LINE__, ul_bind_cpu);
#endif
        ret = OAL_WAIT_EVENT_INTERRUPTIBLE(g_ast_event_task[ul_bind_cpu].frw_wq,
            OAL_TRUE == frw_task_thread_condition_check(ul_bind_cpu));//lint !e666
        /*lint +e730*/
        if (OAL_UNLIKELY(ret == -ERESTARTSYS)) {
            OAL_IO_PRINT("wifi task %s was interrupted by a signal\n", oal_get_current_task_name());
            break;
        }
#if (_PRE_FRW_FEATURE_PROCCESS_ENTITY_TYPE == _PRE_FRW_FEATURE_PROCCESS_ENTITY_THREAD)
        g_ast_event_task[ul_bind_cpu].ul_total_loop_cnt++;

        ul_event_count = g_ast_event_task[ul_bind_cpu].ul_total_event_cnt;
#endif
        frw_event_process_all_event(ul_bind_cpu);
#if (_PRE_FRW_FEATURE_PROCCESS_ENTITY_TYPE == _PRE_FRW_FEATURE_PROCCESS_ENTITY_THREAD)
        if (ul_event_count == g_ast_event_task[ul_bind_cpu].ul_total_event_cnt) {
            /* 空转 */
            ul_empty_count++;
            if (ul_empty_count == 10000) { // 空转10000次以后判定为调度次数太多
                DECLARE_DFT_TRACE_KEY_INFO("frw_sched_too_much", OAL_DFT_TRACE_EXCEP);
            }
        } else {
            if (ul_empty_count > g_ast_event_task[ul_bind_cpu].ul_max_empty_count) {
                g_ast_event_task[ul_bind_cpu].ul_max_empty_count = ul_empty_count;
            }
            ul_empty_count = 0;
        }
#endif
#ifdef _PRE_FRW_EVENT_PROCESS_TRACE_DEBUG
        frw_event_last_pc_trace(__FUNCTION__, __LINE__, ul_bind_cpu);
#endif
#if (!defined(CONFIG_PREEMPT) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION))
        cond_resched();
#endif
    }

    return 0;
}


oal_uint32 frw_task_init(oal_void)
{
    oal_ulong ul_core_id;
    oal_kthread_stru *pst_kthread = OAL_PTR_NULL;
    oal_kthread_param_stru st_thread_param = { 0 };

    memset_s(g_ast_event_task, OAL_SIZEOF(g_ast_event_task), 0, OAL_SIZEOF(g_ast_event_task));

    for (ul_core_id = 0; ul_core_id < WLAN_FRW_MAX_NUM_CORES; ul_core_id++) {
        OAL_WAIT_QUEUE_INIT_HEAD(&g_ast_event_task[ul_core_id].frw_wq);

        memset_s(&st_thread_param, OAL_SIZEOF(oal_kthread_param_stru), 0, OAL_SIZEOF(oal_kthread_param_stru));
        st_thread_param.l_cpuid = ul_core_id;
        st_thread_param.l_policy = OAL_SCHED_RR;
        st_thread_param.l_prio = FRW_TASK_PRIO;
        st_thread_param.ul_stacksize = FRW_TASK_SIZE;

        memset_s(g_frw_thread_name[ul_core_id], FRW_THREAD_NAME_MAX_SIZE, 0, FRW_THREAD_NAME_MAX_SIZE);
        if (snprintf_s(g_frw_thread_name[ul_core_id], FRW_THREAD_NAME_MAX_SIZE, FRW_THREAD_NAME_MAX_SIZE - 1,
            "hisi_frw%lu", ul_core_id) < EOK) {
            OAM_ERROR_LOG0(0, OAM_SF_FRW, "{frw_task_init:: snprintf_s failed !}");
            return OAL_FAIL;
        }
        pst_kthread = oal_kthread_create(g_frw_thread_name[ul_core_id], frw_task_thread,
            (oal_void *)(uintptr_t)ul_core_id, &st_thread_param);
        if (IS_ERR_OR_NULL(pst_kthread)) {
            return OAL_FAIL;
        }

        g_ast_event_task[ul_core_id].pst_event_kthread = pst_kthread;
        g_ast_event_task[ul_core_id].uc_task_state = FRW_TASK_STATE_IRQ_UNBIND;
    }

    return OAL_SUCC;
}


oal_void frw_task_exit(oal_void)
{
    oal_uint32 ul_core_id;

    for (ul_core_id = 0; ul_core_id < WLAN_FRW_MAX_NUM_CORES; ul_core_id++) {
        if (g_ast_event_task[ul_core_id].pst_event_kthread) {
            oal_kthread_stop(g_ast_event_task[ul_core_id].pst_event_kthread);
            g_ast_event_task[ul_core_id].pst_event_kthread = OAL_PTR_NULL;
        }
    }
}


oal_void  frw_task_event_handler_register(oal_void (*p_func)(oal_ulong))
{
}


oal_void frw_task_sched(oal_uint32 ul_core_id)
{
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&g_ast_event_task[ul_core_id].frw_wq);
}


oal_void frw_task_set_state(oal_uint32 ul_core_id, oal_uint8 uc_task_state)
{
    g_ast_event_task[ul_core_id].uc_task_state = uc_task_state;
}

oal_uint8 frw_task_get_state(oal_uint32 ul_core_id)
{
    return g_ast_event_task[ul_core_id].uc_task_state;
}

#elif (_PRE_FRW_FEATURE_PROCCESS_ENTITY_TYPE == _PRE_FRW_FEATURE_PROCCESS_ENTITY_TASKLET)


oal_uint32 frw_task_init(oal_void)
{
    oal_uint32 ul_core_id;

    for (ul_core_id = 0; ul_core_id < WLAN_FRW_MAX_NUM_CORES; ul_core_id++) {
        oal_task_init(&g_ast_event_task[ul_core_id].st_event_tasklet,
                      g_ast_event_task[ul_core_id].p_event_handler_func, 0);
    }
    return OAL_SUCC;
}


oal_void frw_task_exit(oal_void)
{
    oal_uint32 ul_core_id;

    for (ul_core_id = 0; ul_core_id < WLAN_FRW_MAX_NUM_CORES; ul_core_id++) {
        oal_task_kill(&g_ast_event_task[ul_core_id].st_event_tasklet);
    }
}


oal_void frw_task_event_handler_register(oal_void (*p_func)(oal_ulong))
{
    oal_uint32 ul_core_id;

    if (OAL_UNLIKELY(p_func == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_FRW, "{frw_task_event_handler_register:: p_func is null ptr}");
        return;
    }

    for (ul_core_id = 0; ul_core_id < WLAN_FRW_MAX_NUM_CORES; ul_core_id++) {
        g_ast_event_task[ul_core_id].p_event_handler_func = p_func;
    }
}


OAL_STATIC oal_void frw_remote_task_receive(oal_tasklet_stru *info)
{
    oal_tasklet_stru *pst_task = (oal_tasklet_stru *)info;
    oal_task_sched(pst_task);
}


OAL_STATIC oal_void frw_task_sched_on_cpu(oal_tasklet_stru *pst_task, oal_uint32 ul_core_id)
{
    oal_smp_call_function_single(ul_core_id, frw_remote_task_receive, (void *)pst_task, 0);
}


oal_void frw_task_sched(oal_uint32 ul_core_id)
{
    oal_uint32 ul_this_cpu = OAL_GET_CORE_ID();

    if (oal_task_is_scheduled(&g_ast_event_task[ul_core_id].st_event_tasklet)) {
        return;
    }

    if (ul_this_cpu == ul_core_id) {
        oal_task_sched(&g_ast_event_task[ul_core_id].st_event_tasklet);
    } else {
        frw_task_sched_on_cpu(&g_ast_event_task[ul_core_id].st_event_tasklet, ul_core_id);
    }
}


oal_void frw_task_set_state(oal_uint32 ul_core_id, oal_uint8 uc_task_state)
{
}


oal_uint8 frw_task_get_state(oal_uint32 ul_core_id)
{
    return FRW_TASK_STATE_IRQ_BIND;
}

#endif

#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
oal_module_symbol(frw_task_set_state);
oal_module_symbol(frw_task_get_state);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
