

/* 1 头文件包含 */
#include "mac_resource.h"
#include "securec.h"
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV)
#include "dmac_vap.h"
#include "dmac_user.h"
#endif
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
#include "hmac_vap.h"
#include "hmac_user.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_RESOURCE_HOST_C

/* 2 全局变量定义 */
/* 1个device支持的最大关联用户数 */
#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST))
/* g_us_max_asoc_user 4 STA时定义为5， _PRE_8STA 需修改为8, 该值host device均要修改 */
oal_uint16      g_us_max_asoc_user = 5;
#else
oal_uint16      g_us_max_asoc_user = 32;
#endif
mac_res_stru    g_st_mac_res;

/* 3 函数实现 */

oal_uint32  mac_res_check_spec(oal_void)
{
    oal_uint32  ul_ret = OAL_SUCC;
    /* 检查MAC VAP大小 */
    /*lint -e506*/ /*lint -e774*/
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
    /* 如果有人添加了这个结构体，又没有修改对应的大小，导致超过了SIZE，这个判断是有必要的，不是死代码 */
    if (OAL_SIZEOF(hmac_vap_stru) > MAC_RES_VAP_SIZE) {
        OAM_ERROR_LOG2(0, OAM_SF_ANY,
            "{mac_res_check_spec::hmac_vap_stru is over limit! hmac_vap_stru[%d], MAC_RES_VAP_SIZE[%d]}",
            OAL_SIZEOF(hmac_vap_stru), MAC_RES_VAP_SIZE);
        ul_ret = OAL_FAIL;
    }

    if (OAL_SIZEOF(hmac_user_stru) > MAC_RES_USER_SIZE) {
        OAM_ERROR_LOG2(0, OAM_SF_ANY,
            "{mac_res_check_spec::hmac_user_stru is over limit! hmac_user_stru[%d], MAC_RES_USER_SIZE[%d]}",
            OAL_SIZEOF(hmac_user_stru), MAC_RES_USER_SIZE);
        ul_ret = OAL_FAIL;
    }
#elif (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV)
    if (OAL_SIZEOF(dmac_vap_stru) > MAC_RES_VAP_SIZE) {
        OAL_IO_PRINT("{mac_res_check_spec::dmac_vap_stru is over limit!dmac_vap_stru[%d], MAC_RES_VAP_SIZE[%d]}\r\n",
                     OAL_SIZEOF(dmac_vap_stru), MAC_RES_VAP_SIZE);
        ul_ret = OAL_FAIL;
    }

    if (OAL_SIZEOF(dmac_user_stru) > MAC_RES_USER_SIZE) {
        OAL_IO_PRINT("{mac_res_check_spec::dmac_user_stru is over limit!dmac_user_stru[%d], MAC_RES_USER_SIZE[%d]}\r\n",
                     OAL_SIZEOF(dmac_user_stru), MAC_RES_USER_SIZE);
        ul_ret = OAL_FAIL;
    }
#endif
    /*lint -e506*/ /*lint -e774*/

    return ul_ret;
}


#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_DEV))
#pragma arm section rwdata = "ram_data", code = "ram_code", zidata = "ram_data", rodata = "ram_code"
oal_uint8 g_auc_user_res[WLAN_USER_MEM_SIZE] = {0};     /* user资源移到ram空间，方便后续对user数动态调整 */
#pragma arm section rodata, code, rwdata, zidata  // return to default placement

oal_uint8       *g_puc_user_res     = &g_auc_user_res[0];         /* 提升ROM空间调用 */
oal_uint32       g_ul_user_res_len  = WLAN_USER_MEM_SIZE;         /* 与user数g_us_max_asoc_user相匹配 */
oal_uint32       g_ul_user_debug[3] = {0};                        /* 维测 */


oal_uint32  mac_res_user_init(oal_void)
{
    oal_uint16      us_max_asoc_user_per_board;
    oal_uint16      us_max_service_vap_num_per_board;
    oal_uint16      us_max_user;
    oal_uint32      ul_loop;
    oal_void       *p_user_info = OAL_PTR_NULL;
    oal_void       *p_idx       = OAL_PTR_NULL;
    oal_void       *p_user_cnt  = OAL_PTR_NULL;
    oal_void       *p_hash_info = OAL_PTR_NULL;
    oal_void       *p_hash_idx  = OAL_PTR_NULL;
    oal_void       *p_hash_cnt  = OAL_PTR_NULL;
    oal_uint16      us_user_info_size;
    oal_uint16      us_idx_size;
    oal_uint16      us_user_cnt_size;

    /* 初始化USER的资源管理内容 */
    /*
     * 最大关联用户 = 1个device支持的最大关联用户数 * board上面的device数目;
     * 最大组播用户 = 1个device支持的最大业务vap数 * board上面的device数目;
     * 最大用户数 = 最大关联用户 + 组播用户个数
     */
    us_max_asoc_user_per_board = (oal_uint16)(g_us_max_asoc_user * MAC_RES_MAX_DEV_NUM);
    us_max_service_vap_num_per_board = WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE * MAC_RES_MAX_DEV_NUM;
    us_max_user = us_max_asoc_user_per_board + us_max_service_vap_num_per_board;

    /* 检查申请的资源池大小是否合适 */
    us_user_info_size = (oal_uint16)(OAL_SIZEOF(mac_res_mem_user_stru) * us_max_user);
    us_idx_size       = (oal_uint16)(OAL_SIZEOF(oal_ulong) * us_max_user);
    us_user_cnt_size  = (oal_uint16)(OAL_SIZEOF(oal_uint8) * us_max_user);
    if (g_ul_user_res_len < (us_user_info_size + us_idx_size + us_user_cnt_size)) {
        OAM_ERROR_LOG2(0, OAM_SF_UM, "{mac_res_user_init::ALLOC_SIZE(%d) too small!,arm(%d).}", WLAN_MEM_LOCAL_SIZE6,
                       (us_user_info_size + us_idx_size + us_user_cnt_size));
        g_ul_user_debug[2]++;
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /* 动态申请用户资源池相关内存 */
    p_user_info = g_puc_user_res;   /* user资源移到ram空间 */
    p_idx       = (oal_uint8 *)p_user_info + us_user_info_size;
    p_user_cnt  = (oal_uint8 *)p_idx + us_idx_size;

    /* 内存初始清0 */
    memset_s(p_user_info, (OAL_SIZEOF(mac_res_mem_user_stru) * us_max_user), 0,
             (OAL_SIZEOF(mac_res_mem_user_stru) * us_max_user));
    memset_s(p_idx,       (OAL_SIZEOF(oal_ulong) * us_max_user), 0, (OAL_SIZEOF(oal_ulong) * us_max_user));
    memset_s(p_user_cnt,  (OAL_SIZEOF(oal_uint8) * us_max_user), 0, (OAL_SIZEOF(oal_uint8) * us_max_user));

    g_st_mac_res.st_user_res.pst_user_info  = p_user_info;
    g_st_mac_res.st_user_res.pul_idx        = p_idx;
    g_st_mac_res.st_user_res.puc_user_cnt   = p_user_cnt;

    g_st_mac_res.st_user_res.us_hmac_priv_size = 0;

    for (ul_loop = 0; ul_loop < us_max_user; ul_loop++) {
        /* 初始化对应的引用计数值为0 */
        g_st_mac_res.st_user_res.puc_user_cnt[ul_loop] = 0;
    }

    /* 初始化HASH桶的资源管理内容 */
    us_user_info_size = (oal_uint16)(OAL_SIZEOF(mac_res_user_hash_stru) * us_max_asoc_user_per_board);
    us_idx_size       = (oal_uint16)(OAL_SIZEOF(oal_ulong) * us_max_asoc_user_per_board);
    us_user_cnt_size  = (oal_uint16)(OAL_SIZEOF(oal_uint8) * us_max_asoc_user_per_board);
    /* 检查申请的资源池大小是否合适 */
    if ((us_user_info_size + us_idx_size + us_user_cnt_size) > WLAN_MEM_LOCAL_SIZE3) {
        OAM_ERROR_LOG2(0, OAM_SF_UM, "{mac_res_user_init::ALLOC_SIZE(%d) too small!,arm(%d).}", WLAN_MEM_LOCAL_SIZE3,
                       (us_user_info_size + us_idx_size + us_user_cnt_size));

        g_ul_user_debug[0]++;
        g_st_mac_res.st_user_res.pst_user_info  = OAL_PTR_NULL;

        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /* 动态申请用户资源池相关内存 */
    p_hash_info = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, WLAN_MEM_LOCAL_SIZE3, OAL_TRUE);
    p_hash_idx  = (oal_uint8 *)p_hash_info + us_user_info_size;
    p_hash_cnt  = (oal_uint8 *)p_hash_idx + us_idx_size;

    if ((p_hash_info == OAL_PTR_NULL) || (p_hash_idx == OAL_PTR_NULL) || (p_hash_cnt == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_UM, "{mac_res_user_init::param null.}");
        OAL_MEM_FREE(p_hash_info, OAL_TRUE);

        /* 释放用户资源池申请的内存 */
        g_ul_user_debug[1]++;

        g_st_mac_res.st_user_res.pst_user_info  = OAL_PTR_NULL;
        g_st_mac_res.st_user_res.pul_idx        = OAL_PTR_NULL;
        g_st_mac_res.st_user_res.puc_user_cnt   = OAL_PTR_NULL;

        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /* 内存初始清0 */
    memset_s(p_hash_info, (OAL_SIZEOF(mac_res_user_hash_stru) * us_max_asoc_user_per_board), 0,
        (OAL_SIZEOF(mac_res_user_hash_stru) * us_max_asoc_user_per_board));
    memset_s(p_hash_idx,  (OAL_SIZEOF(oal_ulong) * us_max_asoc_user_per_board), 0,
        (OAL_SIZEOF(oal_ulong) * us_max_asoc_user_per_board));
    memset_s(p_hash_cnt,  (OAL_SIZEOF(oal_uint8) * us_max_asoc_user_per_board), 0,
        (OAL_SIZEOF(oal_uint8) * us_max_asoc_user_per_board));

    g_st_mac_res.st_hash_res.pst_user_hash_info = p_hash_info;
    g_st_mac_res.st_hash_res.pul_idx            = p_hash_idx;
    g_st_mac_res.st_hash_res.puc_user_cnt       = p_hash_cnt;

    oal_queue_set(&(g_st_mac_res.st_hash_res.st_queue), g_st_mac_res.st_hash_res.pul_idx,
                  (oal_uint8)us_max_asoc_user_per_board);

    for (ul_loop = 0; ul_loop < us_max_asoc_user_per_board; ul_loop++) {
        /* 初始值保存的是对应数组下标值加1 */
        oal_queue_enqueue(&(g_st_mac_res.st_hash_res.st_queue), (oal_void *)(ul_loop + 1));

        /* 初始化对应的引用计数值为0 */
        g_st_mac_res.st_hash_res.puc_user_cnt[ul_loop] = 0;
    }

    return OAL_SUCC;
}


oal_uint32  mac_res_exit(void)
{
    OAL_MEM_FREE((g_st_mac_res.st_user_res.pst_user_info), OAL_TRUE);

    OAL_MEM_FREE((g_st_mac_res.st_hash_res.pst_user_hash_info), OAL_TRUE);

    g_st_mac_res.st_user_res.pst_user_info      = OAL_PTR_NULL;
    g_st_mac_res.st_user_res.pul_idx            = OAL_PTR_NULL;
    g_st_mac_res.st_user_res.puc_user_cnt       = OAL_PTR_NULL;

    g_st_mac_res.st_hash_res.pst_user_hash_info = OAL_PTR_NULL;
    g_st_mac_res.st_hash_res.pul_idx            = OAL_PTR_NULL;
    g_st_mac_res.st_hash_res.puc_user_cnt       = OAL_PTR_NULL;

    return OAL_SUCC;
}


oal_uint32  mac_res_init(oal_void)
{
    oal_ulong        ul_loop;
    oal_uint32      ul_ret;

    memset_s(&g_st_mac_res, OAL_SIZEOF(mac_res_stru), 0, OAL_SIZEOF(mac_res_stru));
    /* 初始化DEV的资源管理内容 */
    oal_queue_set(&(g_st_mac_res.st_dev_res.st_queue), g_st_mac_res.st_dev_res.aul_idx, MAC_RES_MAX_DEV_NUM);

    ul_ret = mac_res_check_spec();
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{mac_res_init::mac_res_user_init failed[%d].}", ul_ret);
        return ul_ret;
    }

    for (ul_loop = 0; ul_loop < MAC_RES_MAX_DEV_NUM; ul_loop++) {
        /* 初始值保存的是对应数组下标值加1 */
        oal_queue_enqueue(&(g_st_mac_res.st_dev_res.st_queue), (oal_void *)(uintptr_t)(ul_loop + 1));

        /* 初始化对应的引用计数值为0 */
        g_st_mac_res.st_dev_res.auc_user_cnt[ul_loop] = 0;
    }

    /* 初始化VAP的资源管理内容 */

    g_st_mac_res.st_vap_res.us_hmac_priv_size = 0;

    for (ul_loop = 0; ul_loop < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; ul_loop++) {
        /* 初始值保存的是对应数组下标值加1 */
        /* 初始化对应的引用计数值为0 */
        g_st_mac_res.st_vap_res.auc_user_cnt[ul_loop] = 0;
    }

    /* 初始化USER的资源管理内容;初始化HASH桶的资源管理内容 */
    ul_ret = mac_res_user_init();
    if (ul_ret != OAL_SUCC) {
        MAC_ERR_LOG1(0, "mac_res_init: mac_res_user_init return err code", ul_ret);
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{mac_res_init::mac_res_user_init failed[%d].}", ul_ret);

        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  mac_res_free_mac_user(oal_uint16 us_idx)
{
    if (OAL_UNLIKELY(us_idx >=
        g_us_max_asoc_user * MAC_RES_MAX_DEV_NUM + WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE * MAC_RES_MAX_DEV_NUM)) {
        return OAL_FAIL;
    }

    if (g_st_mac_res.st_user_res.puc_user_cnt[us_idx] == 0) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "mac_res_free_mac_user::cnt==0! idx:%d", us_idx);
        oal_dump_stack();
        return OAL_ERR_CODE_USER_RES_CNT_ZERO;
    }

    (g_st_mac_res.st_user_res.puc_user_cnt[us_idx])--;

    if (g_st_mac_res.st_user_res.puc_user_cnt[us_idx] != 0) {
        return OAL_SUCC;
    }

    return OAL_SUCC;
}


oal_uint32  mac_res_free_mac_vap(oal_uint32 ul_idx)
{
    if (OAL_UNLIKELY(ul_idx >= WLAN_VAP_SUPPORT_MAX_NUM_LIMIT)) {
        return OAL_FAIL;
    }

    if (g_st_mac_res.st_vap_res.auc_user_cnt[ul_idx] == 0) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "mac_res_free_mac_vap::cnt==0! idx:%d", ul_idx);
        oal_dump_stack();
        return OAL_SUCC;
    }

    (g_st_mac_res.st_vap_res.auc_user_cnt[ul_idx])--;

    if (g_st_mac_res.st_vap_res.auc_user_cnt[ul_idx] != 0) {
        return OAL_SUCC;
    }

    return OAL_SUCC;
}
#else

oal_uint32  mac_res_user_init(oal_void)
{
    oal_uint16      us_max_asoc_user_per_board;
    oal_uint16      us_max_service_vap_num_per_board;
    oal_uint16      us_max_user;
    oal_ulong        ul_loop;
    oal_void       *p_user_info = OAL_PTR_NULL;
    oal_void       *p_idx       = OAL_PTR_NULL;
    oal_void       *p_user_cnt  = OAL_PTR_NULL;
    oal_void       *p_hash_info = OAL_PTR_NULL;
    oal_void       *p_hash_idx  = OAL_PTR_NULL;
    oal_void       *p_hash_cnt  = OAL_PTR_NULL;

    /* 初始化USER的资源管理内容 */
    /*
     * 最大关联用户 = 1个device支持的最大关联用户数 * board上面的device数目;
     * 最大组播用户 = 1个device支持的最大业务vap数 * board上面的device数目;
     * 最大用户数 = 最大关联用户 + 组播用户个数
     */
    us_max_asoc_user_per_board = (oal_uint16)(g_us_max_asoc_user * MAC_RES_MAX_DEV_NUM);
    us_max_service_vap_num_per_board = WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE * MAC_RES_MAX_DEV_NUM;
    us_max_user = us_max_asoc_user_per_board + us_max_service_vap_num_per_board;

    /* 动态申请用户资源池相关内存 */
    p_user_info = oal_memalloc(OAL_SIZEOF(mac_res_mem_user_stru) * us_max_user);
    p_idx       = oal_memalloc(OAL_SIZEOF(oal_ulong) * us_max_user);
    p_user_cnt  = oal_memalloc(OAL_SIZEOF(oal_uint8) * us_max_user);
    if ((p_user_info == OAL_PTR_NULL) || (p_idx == OAL_PTR_NULL) || (p_user_cnt == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_res_user_init::param null.}");

        if (p_user_info != OAL_PTR_NULL) {
            oal_free(p_user_info);
        }

        if (p_idx != OAL_PTR_NULL) {
            oal_free(p_idx);
        }

        if (p_user_cnt != OAL_PTR_NULL) {
            oal_free(p_user_cnt);
        }

        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /* 内存初始清0 */
    memset_s(p_user_info, (OAL_SIZEOF(mac_res_mem_user_stru) * us_max_user), 0,
        (OAL_SIZEOF(mac_res_mem_user_stru) * us_max_user));
    memset_s(p_idx,       (OAL_SIZEOF(oal_ulong) * us_max_user), 0, (OAL_SIZEOF(oal_ulong) * us_max_user));
    memset_s(p_user_cnt,  (OAL_SIZEOF(oal_uint8) * us_max_user), 0, (OAL_SIZEOF(oal_uint8) * us_max_user));

    g_st_mac_res.st_user_res.pst_user_info  = p_user_info;
    g_st_mac_res.st_user_res.pul_idx        = p_idx;
    g_st_mac_res.st_user_res.puc_user_cnt   = p_user_cnt;

    oal_queue_set(&(g_st_mac_res.st_user_res.st_queue),
                  g_st_mac_res.st_user_res.pul_idx,
                  (oal_uint8)us_max_user);
    g_st_mac_res.st_user_res.us_hmac_priv_size = 0;

    for (ul_loop = 0; ul_loop < us_max_user; ul_loop++) {
        /* 初始值保存的是对应数组下标值加1 */
        oal_queue_enqueue(&(g_st_mac_res.st_user_res.st_queue), (void *)(uintptr_t)(ul_loop + 1));

        /* 初始化对应的引用计数值为0 */
        g_st_mac_res.st_user_res.puc_user_cnt[ul_loop] = 0;
    }

    /* 初始化HASH桶的资源管理内容 */
    p_hash_info = oal_memalloc(OAL_SIZEOF(mac_res_user_hash_stru) * us_max_asoc_user_per_board);
    p_hash_idx  = oal_memalloc(OAL_SIZEOF(oal_ulong) * us_max_asoc_user_per_board);
    p_hash_cnt  = oal_memalloc(OAL_SIZEOF(oal_uint8) * us_max_asoc_user_per_board);
    if ((p_hash_info == OAL_PTR_NULL) || (p_hash_idx == OAL_PTR_NULL) || (p_hash_cnt == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_res_user_init::param null.}");
        if (p_hash_info != OAL_PTR_NULL) {
            oal_free(p_hash_info);
        }

        if (p_hash_idx != OAL_PTR_NULL) {
            oal_free(p_hash_idx);
        }

        if (p_hash_cnt != OAL_PTR_NULL) {
            oal_free(p_hash_cnt);
        }

        /* 释放用户资源池申请的内存 */
        oal_free(p_user_info);
        oal_free(p_idx);
        oal_free(p_user_cnt);

        g_st_mac_res.st_user_res.pst_user_info  = OAL_PTR_NULL;
        g_st_mac_res.st_user_res.pul_idx        = OAL_PTR_NULL;
        g_st_mac_res.st_user_res.puc_user_cnt   = OAL_PTR_NULL;

        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /* 内存初始清0 */
    memset_s(p_hash_info, (OAL_SIZEOF(mac_res_user_hash_stru) * us_max_asoc_user_per_board), 0,
        (OAL_SIZEOF(mac_res_user_hash_stru) * us_max_asoc_user_per_board));
    memset_s(p_hash_idx,  (OAL_SIZEOF(oal_ulong) * us_max_asoc_user_per_board), 0,
        (OAL_SIZEOF(oal_ulong) * us_max_asoc_user_per_board));
    memset_s(p_hash_cnt,  (OAL_SIZEOF(oal_uint8) * us_max_asoc_user_per_board), 0,
        (OAL_SIZEOF(oal_uint8) * us_max_asoc_user_per_board));

    g_st_mac_res.st_hash_res.pst_user_hash_info = p_hash_info;
    g_st_mac_res.st_hash_res.pul_idx            = p_hash_idx;
    g_st_mac_res.st_hash_res.puc_user_cnt       = p_hash_cnt;

    oal_queue_set(&(g_st_mac_res.st_hash_res.st_queue),
                  g_st_mac_res.st_hash_res.pul_idx,
                  (oal_uint8)us_max_asoc_user_per_board);

    for (ul_loop = 0; ul_loop < us_max_asoc_user_per_board; ul_loop++) {
        /* 初始值保存的是对应数组下标值加1 */
        oal_queue_enqueue(&(g_st_mac_res.st_hash_res.st_queue), (oal_void *)(uintptr_t)(ul_loop + 1));

        /* 初始化对应的引用计数值为0 */
        g_st_mac_res.st_hash_res.puc_user_cnt[ul_loop] = 0;
    }

    return OAL_SUCC;
}


oal_uint32  mac_res_exit(void)
{
    oal_free(g_st_mac_res.st_user_res.pst_user_info);
    oal_free(g_st_mac_res.st_user_res.pul_idx);
    oal_free(g_st_mac_res.st_user_res.puc_user_cnt);

    oal_free(g_st_mac_res.st_hash_res.pst_user_hash_info);
    oal_free(g_st_mac_res.st_hash_res.pul_idx);
    oal_free(g_st_mac_res.st_hash_res.puc_user_cnt);

    g_st_mac_res.st_user_res.pst_user_info      = OAL_PTR_NULL;
    g_st_mac_res.st_user_res.pul_idx            = OAL_PTR_NULL;
    g_st_mac_res.st_user_res.puc_user_cnt       = OAL_PTR_NULL;

    g_st_mac_res.st_hash_res.pst_user_hash_info = OAL_PTR_NULL;
    g_st_mac_res.st_hash_res.pul_idx            = OAL_PTR_NULL;
    g_st_mac_res.st_hash_res.puc_user_cnt       = OAL_PTR_NULL;

    return OAL_SUCC;
}


oal_uint32  mac_res_init(oal_void)
{
    oal_ulong        ul_loop;
    oal_uint32       ul_ret;
    memset_s(&g_st_mac_res, OAL_SIZEOF(mac_res_stru), 0, OAL_SIZEOF(mac_res_stru));
    /* 初始化DEV的资源管理内容 */
    oal_queue_set(&(g_st_mac_res.st_dev_res.st_queue),
                  g_st_mac_res.st_dev_res.aul_idx,
                  MAC_RES_MAX_DEV_NUM);

    ul_ret = mac_res_check_spec();
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{mac_res_init::mac_res_user_init failed[%d].}", ul_ret);
        return ul_ret;
    }

    for (ul_loop = 0; ul_loop < MAC_RES_MAX_DEV_NUM; ul_loop++) {
        /* 初始值保存的是对应数组下标值加1 */
        oal_queue_enqueue(&(g_st_mac_res.st_dev_res.st_queue), (oal_void *)(ul_loop + 1));

        /* 初始化对应的引用计数值为0 */
        g_st_mac_res.st_dev_res.auc_user_cnt[ul_loop] = 0;
    }

    /* 初始化VAP的资源管理内容 */
    oal_queue_set(&(g_st_mac_res.st_vap_res.st_queue),
                  g_st_mac_res.st_vap_res.aul_idx,
                  (oal_uint8)WLAN_VAP_SUPPORT_MAX_NUM_LIMIT);
    g_st_mac_res.st_vap_res.us_hmac_priv_size = 0;

    for (ul_loop = 0; ul_loop < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; ul_loop++) {
        /* 初始值保存的是对应数组下标值加1 */
        oal_queue_enqueue(&(g_st_mac_res.st_vap_res.st_queue), (oal_void *)(uintptr_t)(ul_loop + 1));

        /* 初始化对应的引用计数值为0 */
        g_st_mac_res.st_vap_res.auc_user_cnt[ul_loop] = 0;
    }

    /* 初始化USER的资源管理内容;初始化HASH桶的资源管理内容 */
    ul_ret = mac_res_user_init();
    if (ul_ret != OAL_SUCC) {
        MAC_ERR_LOG1(0, "mac_res_init: mac_res_user_init return err code", ul_ret);
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{mac_res_init::mac_res_user_init failed[%d].}", ul_ret);

        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  mac_res_free_mac_user(oal_uint16 us_idx)
{
    if (OAL_UNLIKELY(us_idx >=
        g_us_max_asoc_user * MAC_RES_MAX_DEV_NUM + WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE * MAC_RES_MAX_DEV_NUM)) {
        return OAL_FAIL;
    }

    if (g_st_mac_res.st_user_res.puc_user_cnt[us_idx] == 0) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "mac_res_free_mac_user::cnt==0! idx:%d", us_idx);
        oal_dump_stack();
        return OAL_ERR_CODE_USER_RES_CNT_ZERO;
    }

    (g_st_mac_res.st_user_res.puc_user_cnt[us_idx])--;

    if (g_st_mac_res.st_user_res.puc_user_cnt[us_idx] != 0) {
        return OAL_SUCC;
    }

    /* 入队索引值需要加1操作 */
    oal_queue_enqueue(&(g_st_mac_res.st_user_res.st_queue), (oal_void *)((uintptr_t)us_idx + 1));

    return OAL_SUCC;
}


oal_uint32  mac_res_free_mac_vap(oal_uint32 ul_idx)
{
    if (OAL_UNLIKELY(ul_idx >= WLAN_VAP_SUPPORT_MAX_NUM_LIMIT)) {
        return OAL_FAIL;
    }

    if (g_st_mac_res.st_vap_res.auc_user_cnt[ul_idx] == 0) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "mac_res_free_mac_vap::cnt==0! idx:%d", ul_idx);
        oal_dump_stack();
        return OAL_SUCC;
    }

    (g_st_mac_res.st_vap_res.auc_user_cnt[ul_idx])--;

    if (g_st_mac_res.st_vap_res.auc_user_cnt[ul_idx] != 0) {
        return OAL_SUCC;
    }

    /* 入队索引值需要加1操作 */
    oal_queue_enqueue(&(g_st_mac_res.st_vap_res.st_queue), (oal_void *)(uintptr_t)((oal_ulong)ul_idx + 1));

    return OAL_SUCC;
}


oal_uint32  mac_res_alloc_hmac_user_ex(oal_uint16 *pus_idx, oal_uint16 us_hmac_priv_size)
{
    oal_uint32  ul_idx_temp;
    oal_uint32  ul_user_idx_input;
    oal_uint32  ul_count;
    oal_uint32  ul_max_user;
    oal_uint8   uc_result;

    if (OAL_UNLIKELY(pus_idx == OAL_PTR_NULL)) {
        return OAL_FAIL;
    }

    ul_user_idx_input = *pus_idx + 1;
    ul_max_user = g_us_max_asoc_user * MAC_RES_MAX_DEV_NUM + WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE * MAC_RES_MAX_DEV_NUM;

    // 出队处理
    ul_idx_temp = (uintptr_t)oal_queue_dequeue(&(g_st_mac_res.st_user_res.st_queue));
    // 出队元素与输入元素比较
    if (ul_idx_temp != ul_user_idx_input) {
        uc_result =
            oal_is_element_in_queue(&(g_st_mac_res.st_user_res.st_queue), (oal_void *)((uintptr_t)ul_user_idx_input));
        if (uc_result == OAL_TRUE) {
            ul_count = 0;
            while ((ul_idx_temp != ul_user_idx_input) && (ul_count < ul_max_user)) {
                oal_queue_enqueue(&(g_st_mac_res.st_user_res.st_queue), (oal_void *)((uintptr_t)ul_idx_temp));
                ul_idx_temp = (uintptr_t)oal_queue_dequeue(&(g_st_mac_res.st_user_res.st_queue));
                ul_count++;
            }
        }
    }

    /* 0为无效值 */
    if (ul_idx_temp == 0) {
        return OAL_FAIL;
    }

    *pus_idx = (oal_uint16)(ul_idx_temp - 1);

    g_st_mac_res.st_user_res.us_hmac_priv_size = us_hmac_priv_size;

    (g_st_mac_res.st_user_res.puc_user_cnt[ul_idx_temp - 1])++;

    return OAL_SUCC;
}

#endif


oal_uint32  mac_res_free_hash(oal_uint32 ul_hash_idx)
{
    if (OAL_UNLIKELY(ul_hash_idx >= (oal_uint32)(g_us_max_asoc_user * MAC_RES_MAX_DEV_NUM))) {
        return OAL_FAIL;
    }

    if (g_st_mac_res.st_hash_res.puc_user_cnt[ul_hash_idx] == 0) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "mac_res_free_hash::cnt==0! idx:%d", ul_hash_idx);
        oal_dump_stack();
        return OAL_SUCC;
    }

    (g_st_mac_res.st_hash_res.puc_user_cnt[ul_hash_idx])--;

    if (g_st_mac_res.st_hash_res.puc_user_cnt[ul_hash_idx] != 0) {
        return OAL_SUCC;
    }

    /* 入队索引值需要加1操作 */
    oal_queue_enqueue(&(g_st_mac_res.st_hash_res.st_queue), (oal_void *)((uintptr_t)ul_hash_idx + 1));

    return OAL_SUCC;
}


oal_uint32  mac_res_set_max_asoc_user(oal_uint16 us_num)
{
    g_us_max_asoc_user = us_num;

    /* 需要系统复位 */

    return OAL_SUCC;
}


oal_uint32  mac_res_alloc_dmac_dev(oal_uint8    *puc_dev_idx)
{
    oal_ulong  ul_dev_idx_temp;

    if (OAL_UNLIKELY(puc_dev_idx == OAL_PTR_NULL)) {
        OAL_IO_PRINT("mac_res_alloc_dmac_dev: OAL_PTR_NULL == pul_dev_idx");
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_res_alloc_dmac_dev::puc_dev_idx null.}");
        return OAL_FAIL;
    }

    ul_dev_idx_temp = (uintptr_t)oal_queue_dequeue(&(g_st_mac_res.st_dev_res.st_queue));
    /* 0为无效值 */
    if (ul_dev_idx_temp == 0) {
        OAL_IO_PRINT("mac_res_alloc_dmac_dev: 0 == ul_dev_idx_temp");
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_res_alloc_dmac_dev::ul_dev_idx_temp=0.}");

        return OAL_FAIL;
    }

    *puc_dev_idx = (oal_uint8)(ul_dev_idx_temp - 1);

    (g_st_mac_res.st_dev_res.auc_user_cnt[ul_dev_idx_temp - 1])++;

    return OAL_SUCC;
}
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

oal_uint32  mac_res_alloc_hmac_dev(oal_uint8    *puc_dev_idx)
{
    oal_ulong  ul_dev_idx_temp;

    if (OAL_UNLIKELY(puc_dev_idx == OAL_PTR_NULL)) {
        OAL_IO_PRINT("mac_res_alloc_hmac_dev: OAL_PTR_NULL == pul_dev_idx");
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_res_alloc_hmac_dev::puc_dev_idx null.}");

        return OAL_FAIL;
    }

    ul_dev_idx_temp = (uintptr_t)oal_queue_dequeue(&(g_st_mac_res.st_dev_res.st_queue));
    /* 0为无效值 */
    if (ul_dev_idx_temp == 0) {
        OAL_IO_PRINT("mac_res_alloc_hmac_dev: 0 == ul_dev_idx_temp");
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_res_alloc_hmac_dev::ul_dev_idx_temp=0.}");

        return OAL_FAIL;
    }

    *puc_dev_idx = (oal_uint8)(ul_dev_idx_temp - 1);

    (g_st_mac_res.st_dev_res.auc_user_cnt[ul_dev_idx_temp - 1])++;

    return OAL_SUCC;
}
#endif // #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)
#else

oal_uint32  mac_res_alloc_hmac_dev(oal_uint32    ul_dev_idx)
{
    if (OAL_UNLIKELY(ul_dev_idx >= MAC_RES_MAX_DEV_NUM)) {
        MAC_ERR_LOG(0, "mac_res_alloc_hmac_dev: ul_dev_idx >= MAC_RES_MAX_DEV_NUM");
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{mac_res_alloc_hmac_dev::invalid ul_dev_idx[%d].}", ul_dev_idx);

        return OAL_FAIL;
    }

    (g_st_mac_res.st_dev_res.auc_user_cnt[ul_dev_idx])++;

    return OAL_SUCC;
}
#endif


oal_uint32  mac_res_free_dev(oal_uint32 ul_dev_idx)
{
    if (OAL_UNLIKELY(ul_dev_idx >= MAC_RES_MAX_DEV_NUM)) {
        MAC_ERR_LOG(0, "mac_res_free_dev: ul_dev_idx >= MAC_RES_MAX_DEV_NUM");
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{mac_res_free_dev::invalid ul_dev_idx[%d].}", ul_dev_idx);

        return OAL_FAIL;
    }

    if (g_st_mac_res.st_dev_res.auc_user_cnt[ul_dev_idx] == 0) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "mac_res_free_dev::cnt==0! idx:%d", ul_dev_idx);
        oal_dump_stack();
        return OAL_SUCC;
    }

    (g_st_mac_res.st_dev_res.auc_user_cnt[ul_dev_idx])--;

    if (g_st_mac_res.st_dev_res.auc_user_cnt[ul_dev_idx] != 0) {
        return OAL_SUCC;
    }

    /* 入队索引值需要加1操作 */
    oal_queue_enqueue(&(g_st_mac_res.st_dev_res.st_queue), (oal_void *)((uintptr_t)ul_dev_idx + 1));

    return OAL_SUCC;
}


oal_uint16  mac_res_get_max_asoc_user(oal_void)
{
    return g_us_max_asoc_user;
}

/*lint -e19*/
oal_module_symbol(g_st_mac_res);
oal_module_symbol(mac_res_get_max_asoc_user);
oal_module_symbol(mac_res_set_max_asoc_user);
oal_module_symbol(g_us_max_asoc_user);
oal_module_symbol(mac_res_free_dev);
oal_module_symbol(mac_res_alloc_hmac_dev);
oal_module_symbol(mac_res_get_dev);
oal_module_symbol(mac_res_free_mac_user);
oal_module_symbol(mac_res_free_mac_vap);
oal_module_symbol(mac_res_free_hash);
/*lint +e19*/

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif