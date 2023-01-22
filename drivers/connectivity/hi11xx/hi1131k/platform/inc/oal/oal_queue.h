

#ifndef __OAL_DATA_STRU_H__
#define __OAL_DATA_STRU_H__

#include "oal_types.h"
#include "oal_mem.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID 0

#define OAL_QUEUE_DESTROY

/* 判断x是否是2的整数幂 */
#define OAL_IS_NOT_POW_OF_2(_x)   ((_x) & ((_x) - 1))
typedef struct {
    oal_uint8           uc_element_cnt;    /* 本队列中已经存放的元素个数 */
    oal_uint8           uc_max_elements;   /* 本队列中所能存放的最大元素个数 */
    oal_uint8           uc_tail_index;     /* 指向下一个元素入队位置的索引 */
    oal_uint8           uc_head_index;     /* 指向当前元素出队位置的索引 */
    uintptr_t           *pul_buf;           /* 队列缓存 */
} oal_queue_stru;

OAL_STATIC OAL_INLINE oal_void oal_queue_set(oal_queue_stru *pst_queue, oal_ulong *pul_buf, oal_uint8 uc_max_elements)
{
    pst_queue->pul_buf         = (uintptr_t *)pul_buf;

    pst_queue->uc_tail_index   = 0;
    pst_queue->uc_head_index   = 0;
    pst_queue->uc_element_cnt  = 0;
    pst_queue->uc_max_elements = uc_max_elements;
}


OAL_STATIC OAL_INLINE oal_uint32 oal_queue_init(oal_queue_stru *pst_queue, oal_uint8 uc_max_events)
{
    oal_ulong *pul_buf = OAL_PTR_NULL;

    if (uc_max_events == 0) {
        return OAL_SUCC;
    } else {
        if (OAL_UNLIKELY(OAL_IS_NOT_POW_OF_2(uc_max_events))) {
            return OAL_ERR_CODE_CONFIG_UNSUPPORT;
        }

        pul_buf = (oal_ulong *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL,
                                             (oal_uint16)(uc_max_events * OAL_SIZEOF(oal_ulong)), OAL_TRUE);
        if (OAL_UNLIKELY(pul_buf == OAL_PTR_NULL)) {
            return OAL_ERR_CODE_ALLOC_MEM_FAIL;
        }

        memset_s(pul_buf, uc_max_events * sizeof(oal_ulong), 0, uc_max_events * sizeof(oal_ulong));
        oal_queue_set(pst_queue, pul_buf, uc_max_events);

        return OAL_SUCC;
    }
}


OAL_STATIC OAL_INLINE oal_void  oal_queue_destroy(oal_queue_stru *pst_queue)
{
    if (pst_queue == OAL_PTR_NULL) {
        return;
    }

    if (pst_queue->pul_buf == OAL_PTR_NULL) {
        return;
    }

    OAL_MEM_FREE(pst_queue->pul_buf, OAL_TRUE);

    oal_queue_set(pst_queue, OAL_PTR_NULL, 0);
}


OAL_STATIC OAL_INLINE oal_uint32  oal_queue_enqueue(oal_queue_stru *pst_queue, const oal_void *p_element)
{
    oal_uint8   uc_tail_index;

    /* 异常: 队列已满 */
    if (pst_queue->uc_element_cnt == pst_queue->uc_max_elements) {
        return OAL_FAIL;
    }

    uc_tail_index = pst_queue->uc_tail_index;

    /* 将元素的地址保存在队列中 */
    pst_queue->pul_buf[uc_tail_index] = (oal_ulong)(uintptr_t)p_element;

    uc_tail_index++;

    pst_queue->uc_tail_index = ((uc_tail_index >= pst_queue->uc_max_elements) ? 0 : uc_tail_index);

    pst_queue->uc_element_cnt++;

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE oal_void* oal_queue_dequeue(oal_queue_stru *pst_queue)
{
    oal_uint8    uc_head_index;
    oal_void     *p_element = OAL_PTR_NULL;

    /* 异常: 队列为空 */
    if (pst_queue->uc_element_cnt == 0) {
        return OAL_PTR_NULL;
    }

    uc_head_index = pst_queue->uc_head_index;

    p_element = (oal_void *)pst_queue->pul_buf[uc_head_index];

    uc_head_index++;

    pst_queue->uc_head_index = ((uc_head_index >= pst_queue->uc_max_elements) ? 0 : uc_head_index);
    pst_queue->uc_element_cnt--;

    return p_element;
}


OAL_STATIC OAL_INLINE oal_uint8 oal_is_element_in_queue(oal_queue_stru *pst_queue, const oal_void *p_element)
{
    oal_uint8     uc_start_index;
    oal_uint32    ul_cnt;

    if (p_element == OAL_PTR_NULL) {
        return OAL_FALSE;
    }

    uc_start_index = pst_queue->uc_head_index;

    for (ul_cnt = 0; ul_cnt < pst_queue->uc_element_cnt; ul_cnt++) {
        if (p_element == (oal_void *)pst_queue->pul_buf[uc_start_index]) {
            return OAL_TRUE;
        }

        uc_start_index++;
        uc_start_index = ((uc_start_index >= pst_queue->uc_max_elements) ? 0 : uc_start_index);
    }

    return OAL_FALSE;
}


OAL_STATIC OAL_INLINE oal_uint8  oal_queue_get_length(oal_queue_stru *pst_queue)
{
    if (pst_queue == OAL_PTR_NULL) {
        return 0;
    }

    return pst_queue->uc_element_cnt;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_data_stru.h */
