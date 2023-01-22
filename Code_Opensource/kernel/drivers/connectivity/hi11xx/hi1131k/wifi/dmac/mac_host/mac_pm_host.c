

#if defined(_PRE_WLAN_FEATURE_PM) || defined(_PRE_WLAN_FEATURE_STA_PM)
/* 1 ͷ�ļ����� */
#include "oal_ext_if.h"
#include "hal_ext_if.h"
#include "mac_device.h"
#include "mac_pm.h"
#include "securec.h"
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
#include "dmac_vap.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_MAC_PM_HOST_C

/* 2 ȫ�ֱ������� */
/* 3 ����ʵ�� */

mac_fsm_stru *mac_fsm_create(oal_void *p_oshandle, const oal_uint8 *p_name, oal_void *p_ctx, oal_uint8 uc_init_state,
    const mac_fsm_state_info *p_state_info, oal_uint8 uc_num_states)
{
    mac_fsm_stru   *pst_fsm = OAL_PTR_NULL;
    oal_uint32      ul_loop;

    if (uc_num_states > MAC_FSM_MAX_STATES) {
        OAM_ERROR_LOG1(0, OAM_SF_PWR, "{mac_fsm_create:state number [%d] too big. }", uc_num_states);
        return  OAL_PTR_NULL;
    }

    /* ���״̬��Ϣ˳���Ƿ��״̬����ƥ�� */
    for (ul_loop = 0; ul_loop < uc_num_states; ul_loop++) {
        if (p_state_info[ul_loop].state >= MAC_FSM_MAX_STATES || p_state_info[ul_loop].state != ul_loop) {
            /* OAM��־�в���ʹ��%s */
            OAM_ERROR_LOG2(0, OAM_SF_PWR, "{FSM:entry %d has invalid state %d }", ul_loop, p_state_info[ul_loop].state);
            return OAL_PTR_NULL;
        }
    }

    pst_fsm = (mac_fsm_stru*)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(mac_fsm_stru), OAL_TRUE);
    if (pst_fsm == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(0, OAM_SF_PWR, "{mac_fsm_create:malloc memory for fsm fail,size[%d]!", OAL_SIZEOF(mac_fsm_stru));
        return OAL_PTR_NULL;
    }

    memset_s(pst_fsm, OAL_SIZEOF(mac_fsm_stru), 0, OAL_SIZEOF(mac_fsm_stru));
    pst_fsm->uc_cur_state  = uc_init_state;
    pst_fsm->uc_prev_state = uc_init_state;
    pst_fsm->p_state_info  = p_state_info;
    pst_fsm->uc_num_states = uc_num_states;
    pst_fsm->p_oshandler   = p_oshandle;
    pst_fsm->p_ctx         = p_ctx;
    pst_fsm->us_last_event = MAC_FSM_EVENT_NONE;

    /* strncpy fsm name */
    ul_loop = 0;
    while ((p_name[ul_loop] != '\0') && (ul_loop < MAC_FSM_MAX_NAME - 1)) {
        pst_fsm->uc_name[ul_loop] = p_name[ul_loop];
        ul_loop++;
    }
    if (ul_loop < MAC_FSM_MAX_NAME) {
        pst_fsm->uc_name[ul_loop] = '\0';
    }

    /* ����״̬�� */
    if (pst_fsm->p_state_info[pst_fsm->uc_cur_state].mac_fsm_entry) {
        pst_fsm->p_state_info[pst_fsm->uc_cur_state].mac_fsm_entry(pst_fsm->p_ctx);
    }
    return pst_fsm;
}


oal_void mac_fsm_destroy(mac_fsm_stru* p_fsm)
{
    OAL_MEM_FREE(p_fsm, OAL_TRUE);
    p_fsm = OAL_PTR_NULL;
    return;
}


oal_uint32 mac_fsm_trans_to_state(mac_fsm_stru* p_fsm, oal_uint8 uc_state)
{
    oal_uint8 uc_cur_state = p_fsm->uc_cur_state;

    if ((uc_state == MAC_FSM_STATE_NONE) || (uc_state >= MAC_FSM_MAX_STATES) || (uc_state >= p_fsm->uc_num_states)) {
        /* OAM��־�в���ʹ��%s */
        OAM_ERROR_LOG2(0, OAM_SF_PWR, "FSM:trans to state %d needs to be a valid state cur_state=%d",
            uc_state, uc_cur_state);
        return OAL_FAIL;
    }

    if (uc_state == uc_cur_state) {
        /* OAM��־�в���ʹ��%s */
        OAM_WARNING_LOG2(0, OAM_SF_PWR, "FSM :trans to state %d EQUAL to current state %d,nothing to do",
            uc_state, uc_cur_state);
        return OAL_SUCC;
    }

    /* OAM��־�в���ʹ��%s */
    OAM_INFO_LOG2(0, OAM_SF_PWR, "FSM: transition from %d => %d ",
        p_fsm->p_state_info[uc_cur_state].state, p_fsm->p_state_info[uc_state].state);

    /* ����ǰһ״̬���˳����� */
    if (p_fsm->p_state_info[p_fsm->uc_cur_state].mac_fsm_exit) {
        p_fsm->p_state_info[p_fsm->uc_cur_state].mac_fsm_exit(p_fsm->p_ctx);
    }

    /* ���ñ�״̬�Ľ��뺯�� */
    if (p_fsm->p_state_info[uc_state].mac_fsm_entry) {
        p_fsm->p_state_info[uc_state].mac_fsm_entry(p_fsm->p_ctx);
    }

    p_fsm->uc_prev_state = uc_cur_state;
    p_fsm->uc_cur_state  = uc_state;

    return OAL_SUCC;
}


oal_uint32 mac_fsm_event_dispatch(mac_fsm_stru* p_fsm, oal_uint16 us_event,
                                  oal_uint16 us_event_data_len, oal_void *p_event_data)
{
    oal_uint32 ul_event_handled = OAL_FALSE;

    if (p_fsm == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "mac_fsm_event_dispatch:p_fsm = OAL_PTR_NULL");
        return OAL_FAIL;
    }

    if ((p_fsm->uc_cur_state != MAC_FSM_STATE_NONE) && (p_fsm->uc_cur_state < p_fsm->uc_num_states)) {
        p_fsm->us_last_event = us_event;
        ul_event_handled = (*p_fsm->p_state_info[p_fsm->uc_cur_state].mac_fsm_event)(p_fsm->p_ctx, us_event,
            us_event_data_len, p_event_data);
    }
    if (ul_event_handled == OAL_FAIL) {
        /* OAM��־�в���ʹ��%s */
        OAM_ERROR_LOG2(0, OAM_SF_PWR, "FSM :mac_fsm_event_dispatch:event[%d] did not handled in state %d",
            us_event, p_fsm->p_state_info[p_fsm->uc_cur_state].state);
        return OAL_FAIL;
    }
    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_PM

hal_lpm_state_enum_uint8  g_pm_hal_state_map[DEV_PWR_STATE_BUTT] = {
    HAL_LPM_STATE_NORMAL_WORK,      /* DEV_PWR_STATE_WORK */
    HAL_LPM_STATE_DEEP_SLEEP,       /* DEV_PWR_STATE_DEEP_SLEEP */
    HAL_LPM_STATE_WOW,              /* DEV_PWR_STATE_WOW */
    HAL_LPM_STATE_IDLE,             /* DEV_PWR_STATE_IDLE */
    HAL_LPM_STATE_POWER_DOWN        /* DEV_PWR_STATE_OFF */
};


mac_pm_arbiter_state_info g_pm_arbiter_state_info[DEV_PWR_STATE_BUTT] = {
    {DEV_PWR_STATE_WORK,        "Work"},
    {DEV_PWR_STATE_DEEP_SLEEP,  "Deep sleep"},
    {DEV_PWR_STATE_WOW,         "Wow"},
    {DEV_PWR_STATE_IDLE,        "Idle"},
    {DEV_PWR_STATE_OFF,         "Poweroff"}
};


oal_uint32 mac_pm_arbiter_init(mac_device_stru* pst_device)
{
    mac_pm_arbiter_stru *pst_arbiter;

    pst_arbiter = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(mac_pm_arbiter_stru), OAL_TRUE);
    if (pst_arbiter == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_device->uc_cfg_vap_id, OAM_SF_PWR, "hmac_pm_arbiter_init FAIL:out of memory");
        return OAL_FAIL;
    }
    memset_s(pst_arbiter, OAL_SIZEOF(mac_pm_arbiter_stru), 0, OAL_SIZEOF(mac_pm_arbiter_stru));
    pst_arbiter->pst_state_info = &g_pm_arbiter_state_info[0];
    pst_device->pst_pm_arbiter = (oal_void*)pst_arbiter;

    return OAL_SUCC;
}


oal_uint32 mac_pm_arbiter_destroy(mac_device_stru* pst_device)
{
    if (pst_device->pst_pm_arbiter) {
        OAL_MEM_FREE(pst_device->pst_pm_arbiter, OAL_TRUE);
        pst_device->pst_pm_arbiter = OAL_PTR_NULL;
    }

    return OAL_SUCC;
}


oal_uint32 mac_pm_arbiter_alloc_id(mac_device_stru* pst_device, oal_uint8* pst_name,
                                   mac_pm_arbiter_type_enum en_arbiter_type)
{
    mac_pm_arbiter_stru *pst_pm_arbiter = (mac_pm_arbiter_stru*)(pst_device->pst_pm_arbiter);
    oal_uint32 i;

    if (pst_pm_arbiter == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_device->uc_cfg_vap_id, OAM_SF_PWR,
            "hmac_pm_arbiter_alloc_id FAIL:mac device have no arbiter struct");
        return MAC_PWR_ARBITER_ID_INVALID;
    }

    if (en_arbiter_type <= MAC_PWR_ARBITER_TYPE_INVALID || en_arbiter_type >= MAC_PWR_ARBITER_TYPE_BUTT) {
        return MAC_PWR_ARBITER_ID_INVALID;
    }

    /* ��ul_id_bitmap�дӵ�λ��ʼ�������ҵ�һ��Ϊ0��λ��Ϊδ�����ID */
    for (i = 0; i < MAC_PM_ARBITER_MAX_REQUESTORS; i++) {
        if (((1 << i) & pst_pm_arbiter->ul_id_bitmap) == 0) {
            pst_pm_arbiter->ul_id_bitmap |= (1 << i);
            OAL_SPRINTF((char *)&pst_pm_arbiter->requestor[i].auc_id_name[0],
                MAC_PM_ARBITER_MAX_REQ_NAME, "%s", pst_name);
            pst_pm_arbiter->requestor[i].en_arbiter_type = en_arbiter_type;
            pst_pm_arbiter->uc_requestor_num++;
            return i;
        }
    }

    return MAC_PWR_ARBITER_ID_INVALID;
}


oal_uint32 mac_pm_arbiter_free_id(mac_device_stru* pst_device, oal_uint32 ul_arbiter_id)
{
    mac_pm_arbiter_stru *pst_pm_arbiter = (mac_pm_arbiter_stru*)(pst_device->pst_pm_arbiter);
    oal_uint32            ul_loop;

    if (ul_arbiter_id >= MAC_PM_ARBITER_MAX_REQUESTORS) {
        OAM_ERROR_LOG2(pst_device->uc_cfg_vap_id, OAM_SF_PWR, "hmac_pm_arbiter_free_id FAIL:invalid id %d,total %d",
            ul_arbiter_id, pst_pm_arbiter->uc_requestor_num);
        return OAL_FAIL;
    }
    pst_pm_arbiter->ul_id_bitmap &= ~(oal_uint32)(1 << ul_arbiter_id);
    for (ul_loop = 0; ul_loop < DEV_PWR_STATE_BUTT; ul_loop++) {
        pst_pm_arbiter->ul_state_bitmap[ul_loop] &= ~(oal_uint32)(1 << ul_arbiter_id);
    }
    pst_pm_arbiter->requestor[ul_arbiter_id].auc_id_name[0] = 0;
    pst_pm_arbiter->requestor[ul_arbiter_id].en_arbiter_type = MAC_PWR_ARBITER_TYPE_INVALID;
    pst_pm_arbiter->uc_requestor_num--;

    return OAL_SUCC;
}


oal_void mac_pm_arbiter_to_state(mac_device_stru *pst_device, oal_uint32 ul_arbiter_id,
                                 oal_uint8  uc_state_from, oal_uint8  uc_state_to)
{
    mac_pm_arbiter_stru         *pst_pm_arbiter = (mac_pm_arbiter_stru*)(pst_device->pst_pm_arbiter);
    oal_bool_enum_uint8          en_can_trans = OAL_TRUE;
    oal_uint32                   i;

    if (ul_arbiter_id >= MAC_PM_ARBITER_MAX_REQUESTORS) {
        OAM_ERROR_LOG2(pst_device->uc_cfg_vap_id, OAM_SF_PWR, "mac_pm_arbiter_to_state FAIL:invalid id %d,total %d",
            ul_arbiter_id, pst_pm_arbiter->uc_requestor_num);
        return;
    }

    if ((uc_state_from >= DEV_PWR_STATE_BUTT) || (uc_state_to >= DEV_PWR_STATE_BUTT) ||
        (uc_state_from == uc_state_to)) {
        OAM_ERROR_LOG2(pst_device->uc_cfg_vap_id, OAM_SF_PWR,
                       "mac_pm_arbiter_to_state FAIL:invalid state from %d to %d", uc_state_from, uc_state_to);
        return;
    }

    /* ͶƱ�߱����Ȼ������״̬�л�������ԭ״̬��bitmap�������л�״̬��bitmap */
    pst_pm_arbiter->ul_state_bitmap[uc_state_from] &= ~(oal_uint32)(1 << ul_arbiter_id);
    pst_pm_arbiter->ul_state_bitmap[uc_state_to] |= (1 << ul_arbiter_id);

    /* OAM��־�в���ʹ��%s */
    OAM_INFO_LOG3(pst_device->uc_cfg_vap_id, OAM_SF_PWR, "PM arbiter:%d vote to transmit from state %d to state %d",
                  ul_arbiter_id, uc_state_from, uc_state_to);

    /* ����״̬��ֻҪ1Ʊ,device�ͱ���Ҫ�л� */
    if (uc_state_to == DEV_PWR_STATE_WORK) {
        en_can_trans = OAL_TRUE;
    } else {
        /* ������г�Ա��ͶƱ�ˣ�device����״̬�л� */
        for (i = 0; i < pst_pm_arbiter->uc_requestor_num; i++) {
            if (pst_pm_arbiter->requestor[i].en_arbiter_type != MAC_PWR_ARBITER_TYPE_INVALID) {
                if (!((1 << i) & pst_pm_arbiter->ul_state_bitmap[uc_state_to])) {
                    en_can_trans = OAL_FALSE;
                    break;
                }
            }
        }
    }

    if (en_can_trans == OAL_TRUE) {
        pst_pm_arbiter->uc_prev_state = pst_pm_arbiter->uc_cur_state;

        /* ����hal��ӿ� */
        if (OAL_SUCC == mac_pm_set_hal_state(pst_device, uc_state_to)) {
            pst_pm_arbiter->uc_cur_state = uc_state_to;
        }
    }

    return;
}


oal_uint32 mac_pm_set_hal_state(mac_device_stru *pst_device, oal_uint8 uc_state_to)
{
    hal_lpm_state_param_stru     st_para;
    mac_pm_arbiter_stru         *pst_pm_arbiter = (mac_pm_arbiter_stru*)(pst_device->pst_pm_arbiter);

    memset_s(&st_para, OAL_SIZEOF(hal_lpm_state_param_stru), 0, OAL_SIZEOF(hal_lpm_state_param_stru));
    switch (uc_state_to) {
        case DEV_PWR_STATE_WORK:
            /* �ָ�����ͨ����beacon interval���� */
            st_para.bit_set_bcn_interval = OAL_TRUE;
#ifdef _PRE_WLAN_FEATURE_DBAC
            if (mac_is_dbac_enabled(pst_device)) {
                st_para.bit_set_bcn_interval = OAL_FALSE;
            }
#endif
            st_para.ul_idle_bcn_interval = pst_device->ul_beacon_interval;
            break;
        case DEV_PWR_STATE_DEEP_SLEEP:
            st_para.bit_gpio_sleep_en = 0;
            st_para.bit_soft_sleep_en = 1;
            st_para.ul_sleep_time     = 100; /* 100ms */
            break;
        case DEV_PWR_STATE_WOW:
            break;
        case DEV_PWR_STATE_IDLE:
            /* ��ͨ������,beacon����AP_IDLE_BCN_INTERVAL */
            st_para.bit_set_bcn_interval = OAL_TRUE;
#ifdef _PRE_WLAN_FEATURE_DBAC
            if (mac_is_dbac_enabled(pst_device)) {
                st_para.bit_set_bcn_interval = OAL_FALSE;
            }
#endif
            st_para.ul_idle_bcn_interval = WLAN_BEACON_INTVAL_IDLE;
            break;
        case DEV_PWR_STATE_OFF:
            break;
        default:
        {
            return OAL_FAIL;
        }
    }

    pst_pm_arbiter->uc_prev_state = pst_pm_arbiter->uc_cur_state;
    pst_pm_arbiter->uc_cur_state  = uc_state_to;

    hal_set_lpm_state(pst_device->pst_device_stru,
                      g_pm_hal_state_map[pst_pm_arbiter->uc_prev_state],
                      g_pm_hal_state_map[uc_state_to],
                      &st_para);

    return OAL_SUCC;
}
#endif
#endif
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

