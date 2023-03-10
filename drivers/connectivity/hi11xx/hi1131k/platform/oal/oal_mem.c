

#include "platform_spec.h"
#include "oal_mem.h"
#include "oal_main.h"
#include "securec.h"

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#include "oam_ext_if.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAL_MEM_C
#else
#undef  THIS_FILE_ID
#define THIS_FILE_ID 0
#endif

/* 四字节对齐填充缓存 */
#define OAL_MEM_MAX_WORD_ALIGNMENT_BUFFER         3
/*****************************************************************************
  结构名  : oal_mem_subpool_stat
  结构说明: 子内存池统计结构体，维测使用
*****************************************************************************/
typedef struct {
    oal_uint16   us_free_cnt;    /* 本子内存池可用内存块数 */
    oal_uint16   us_total_cnt;   /* 本子内存池内存块总数 */
} oal_mem_subpool_stat;

/*****************************************************************************
  结构名  : oal_mem_pool_stat
  结构说明: 单个内存池统计结构体，维测使用
*****************************************************************************/
typedef struct {
    oal_uint16             us_mem_used_cnt;    /* 本内存池已用内存块 */
    oal_uint16             us_mem_total_cnt;   /* 本内存池一共有多少内存块 */

    oal_mem_subpool_stat   ast_subpool_stat[WLAN_MEM_MAX_SUBPOOL_NUM];
} oal_mem_pool_stat;

/*****************************************************************************
  结构名  : oal_mem_stat
  结构说明: 内存池统计结构体，维测使用
*****************************************************************************/
typedef struct {
    oal_mem_pool_stat ast_mem_start_stat[OAL_MEM_POOL_ID_BUTT];   /* 起始统计信息 */
    oal_mem_pool_stat ast_mem_end_stat[OAL_MEM_POOL_ID_BUTT];     /* 终止统计信息 */
} oal_mem_stat;

/*****************************************************************************
  3 全局变量定义
*****************************************************************************/
/******************************************************************************
    内存池统计信息全局变量，维测使用
*******************************************************************************/
OAL_STATIC oal_mem_stat g_st_mem_stat;
oal_mempool_info_to_sdt_stru    g_st_mempool_info = {0};
#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_mempool_tx_dscr_addr    g_st_tx_dscr_addr;
#endif
/******************************************************************************
    共享描述符内存池配置信息全局变量
*******************************************************************************/
OAL_STATIC  oal_mem_subpool_cfg_stru g_ast_shared_dscr_cfg_table[] = {
    /* 接收描述符大小，块数 */
    { WLAN_MEM_SHARED_RX_DSCR_SIZE  + OAL_MEM_INFO_SIZE + OAL_DOG_TAG_SIZE,  WLAN_MEM_SHARED_RX_DSCR_CNT },
    /* 发送描述符大小，块数 */
    { WLAN_MEM_SHARED_TX_DSCR_SIZE1 + OAL_MEM_INFO_SIZE + OAL_DOG_TAG_SIZE, WLAN_MEM_SHARED_TX_DSCR_CNT1 },
    /* 发送描述符大小，块数 */
    { WLAN_MEM_SHARED_TX_DSCR_SIZE2 + OAL_MEM_INFO_SIZE + OAL_DOG_TAG_SIZE, WLAN_MEM_SHARED_TX_DSCR_CNT2 },
};

/******************************************************************************
    共享管理帧内存池配置信息全局变量
*******************************************************************************/
OAL_STATIC  oal_mem_subpool_cfg_stru g_ast_shared_mgmt_cfg_table[] = {
    /* beacon、Probe Response、Auth Seq3帧大小，块数 */
    { WLAN_MEM_SHARED_MGMT_PKT_SIZE1 + OAL_MEM_INFO_SIZE + OAL_DOG_TAG_SIZE, WLAN_MEM_SHARED_MGMT_PKT_CNT1 },
};

/******************************************************************************
    共享数据帧内存池配置信息全局变量
*******************************************************************************/
OAL_STATIC  oal_mem_subpool_cfg_stru g_ast_shared_data_cfg_table[] = {
    /* 802.11MAC帧头+SNAP头+Ethernet头，块数 */
    { WLAN_MEM_SHARED_DATA_PKT_SIZE + OAL_MEM_INFO_SIZE + OAL_DOG_TAG_SIZE, WLAN_MEM_SHARED_DATA_PKT_CNT }
};

/******************************************************************************
    本地数据内存池配置信息全局变量
*******************************************************************************/
#define TOTAL_WLAN_MEM_LOCAL_SIZE1  (WLAN_MEM_LOCAL_SIZE1 + OAL_MEM_INFO_SIZE + OAL_DOG_TAG_SIZE)
#define TOTAL_WLAN_MEM_LOCAL_SIZE2  (WLAN_MEM_LOCAL_SIZE2 + OAL_MEM_INFO_SIZE + OAL_DOG_TAG_SIZE)
#define TOTAL_WLAN_MEM_LOCAL_SIZE3  (WLAN_MEM_LOCAL_SIZE3 + OAL_MEM_INFO_SIZE + OAL_DOG_TAG_SIZE)
#define TOTAL_WLAN_MEM_LOCAL_SIZE4  (WLAN_MEM_LOCAL_SIZE4 + OAL_MEM_INFO_SIZE + OAL_DOG_TAG_SIZE)
#define TOTAL_WLAN_MEM_LOCAL_SIZE5  (WLAN_MEM_LOCAL_SIZE5 + OAL_MEM_INFO_SIZE + OAL_DOG_TAG_SIZE)
#define TOTAL_WLAN_MEM_LOCAL_SIZE6  (WLAN_MEM_LOCAL_SIZE6 + OAL_MEM_INFO_SIZE + OAL_DOG_TAG_SIZE)

/* 事件内存池配置信息全局变量 */
#define TOTAL_WLAN_MEM_EVENT_SIZE1 (WLAN_MEM_EVENT_SIZE1 + OAL_MEM_INFO_SIZE + OAL_DOG_TAG_SIZE)
#define TOTAL_WLAN_MEM_EVENT_SIZE2 (WLAN_MEM_EVENT_SIZE2 + OAL_MEM_INFO_SIZE + OAL_DOG_TAG_SIZE)
#define TOTAL_WLAN_MEM_EVENT_SIZE  (TOTAL_WLAN_MEM_EVENT_SIZE1 * WLAN_MEM_EVENT_CNT1 \
                                    + TOTAL_WLAN_MEM_EVENT_SIZE2 * WLAN_MEM_EVENT_CNT2 \
                                    + OAL_MEM_MAX_WORD_ALIGNMENT_BUFFER)

/* 2.4.4 共享数据帧内存池配置信息 */
#define TOTAL_WLAN_MEM_SHARED_DATA_PKT_SIZE ((WLAN_MEM_SHARED_DATA_PKT_SIZE + OAL_MEM_INFO_SIZE + OAL_DOG_TAG_SIZE) * \
                                             WLAN_MEM_SHARED_DATA_PKT_CNT + OAL_MEM_MAX_WORD_ALIGNMENT_BUFFER)

#define TOTAL_WLAN_MEM_SHARED_MGMT_PKT_SIZE ((WLAN_MEM_SHARED_MGMT_PKT_SIZE1 + OAL_MEM_INFO_SIZE + OAL_DOG_TAG_SIZE) * \
                                             WLAN_MEM_SHARED_MGMT_PKT_CNT1 + OAL_MEM_MAX_WORD_ALIGNMENT_BUFFER)

/* 本地数据内存池配置信息全局变量 */
#ifndef _PRE_LITEOS_WLAN_FEATURE_MEM_REDUCE
#define TOTAL_WLAN_MEM_LOCAL_SIZE  (TOTAL_WLAN_MEM_LOCAL_SIZE1 * WLAN_MEM_LOCAL_CNT1 + \
                                    TOTAL_WLAN_MEM_LOCAL_SIZE2 * WLAN_MEM_LOCAL_CNT2 + \
                                    TOTAL_WLAN_MEM_LOCAL_SIZE3 * WLAN_MEM_LOCAL_CNT3 + \
                                    TOTAL_WLAN_MEM_LOCAL_SIZE4 * WLAN_MEM_LOCAL_CNT4 + \
                                    TOTAL_WLAN_MEM_LOCAL_SIZE5 * WLAN_MEM_LOCAL_CNT5 + \
                                    TOTAL_WLAN_MEM_LOCAL_SIZE6 * WLAN_MEM_LOCAL_CNT6 + \
                                    OAL_MEM_MAX_WORD_ALIGNMENT_BUFFER)
#else
#define TOTAL_WLAN_MEM_LOCAL_SIZE  (TOTAL_WLAN_MEM_LOCAL_SIZE1 * WLAN_MEM_LOCAL_CNT1 + \
                                    TOTAL_WLAN_MEM_LOCAL_SIZE3 * WLAN_MEM_LOCAL_CNT3 + \
                                    TOTAL_WLAN_MEM_LOCAL_SIZE4 * WLAN_MEM_LOCAL_CNT4 + \
                                    TOTAL_WLAN_MEM_LOCAL_SIZE5 * WLAN_MEM_LOCAL_CNT5 + \
                                    OAL_MEM_MAX_WORD_ALIGNMENT_BUFFER)
#endif

/* MIB内存池配置信息全局变量 */
#define TOTAL_WLAN_MEM_MIB_SIZE1    (WLAN_MEM_MIB_SIZE1 + OAL_MEM_INFO_SIZE + OAL_DOG_TAG_SIZE)
#define TOTAL_WLAN_MEM_MIB_SIZE     (TOTAL_WLAN_MEM_MIB_SIZE1 * WLAN_MEM_MIB_CNT1 + OAL_MEM_MAX_WORD_ALIGNMENT_BUFFER)

#define TOTAL_WLAN_MEM_SHARED_DSCR_SIZE ((WLAN_MEM_SHARED_RX_DSCR_SIZE + OAL_MEM_INFO_SIZE + OAL_DOG_TAG_SIZE) * \
                                         WLAN_MEM_SHARED_RX_DSCR_CNT + \
                                         (WLAN_MEM_SHARED_TX_DSCR_SIZE1 + OAL_MEM_INFO_SIZE + OAL_DOG_TAG_SIZE) * \
                                         WLAN_MEM_SHARED_TX_DSCR_CNT1 + \
                                         (WLAN_MEM_SHARED_TX_DSCR_SIZE2 + OAL_MEM_INFO_SIZE + OAL_DOG_TAG_SIZE) * \
                                         WLAN_MEM_SHARED_TX_DSCR_CNT2 + \
                                         OAL_MEM_MAX_WORD_ALIGNMENT_BUFFER)

OAL_STATIC  oal_mem_subpool_cfg_stru g_ast_local_cfg_table[] = {
    { TOTAL_WLAN_MEM_LOCAL_SIZE1, WLAN_MEM_LOCAL_CNT1 },  /* 第一级大小，块数 */
#ifndef _PRE_LITEOS_WLAN_FEATURE_MEM_REDUCE
    { TOTAL_WLAN_MEM_LOCAL_SIZE2, WLAN_MEM_LOCAL_CNT2 },   /* 第二级大小，块数 */
#endif
    {TOTAL_WLAN_MEM_LOCAL_SIZE3, WLAN_MEM_LOCAL_CNT3},
    {TOTAL_WLAN_MEM_LOCAL_SIZE4, WLAN_MEM_LOCAL_CNT4},
    {TOTAL_WLAN_MEM_LOCAL_SIZE5, WLAN_MEM_LOCAL_CNT5},
#ifndef _PRE_LITEOS_WLAN_FEATURE_MEM_REDUCE
    {TOTAL_WLAN_MEM_LOCAL_SIZE6, WLAN_MEM_LOCAL_CNT6},
#endif
};

/* 检查内存子池是否对齐 */
// cppcheck-suppress *
#if (TOTAL_WLAN_MEM_LOCAL_SIZE1 % OAL_MEM_INFO_SIZE)
#error alignment fault error
#endif
// cppcheck-suppress *
#if (TOTAL_WLAN_MEM_LOCAL_SIZE2 % OAL_MEM_INFO_SIZE)
#error alignment fault error
#endif
// cppcheck-suppress *
#if (TOTAL_WLAN_MEM_LOCAL_SIZE3 % OAL_MEM_INFO_SIZE)
#error alignment fault error
#endif
// cppcheck-suppress *
#if (TOTAL_WLAN_MEM_LOCAL_SIZE4 % OAL_MEM_INFO_SIZE)
#error alignment fault error
#endif
// cppcheck-suppress *
#if (TOTAL_WLAN_MEM_LOCAL_SIZE5 % OAL_MEM_INFO_SIZE)
#error alignment fault error
#endif
// cppcheck-suppress *
#if (TOTAL_WLAN_MEM_LOCAL_SIZE6 % OAL_MEM_INFO_SIZE)
#error alignment fault error
#endif

/******************************************************************************
    事件内存池配置信息全局变量
*******************************************************************************/
#define TOTAL_WLAN_MEM_EVENT_SIZE1 (WLAN_MEM_EVENT_SIZE1 + OAL_MEM_INFO_SIZE + OAL_DOG_TAG_SIZE)
#define TOTAL_WLAN_MEM_EVENT_SIZE2 (WLAN_MEM_EVENT_SIZE2 + OAL_MEM_INFO_SIZE + OAL_DOG_TAG_SIZE)
OAL_STATIC  oal_mem_subpool_cfg_stru g_ast_event_cfg_table[] = {
    {TOTAL_WLAN_MEM_EVENT_SIZE1, WLAN_MEM_EVENT_CNT1},
    {TOTAL_WLAN_MEM_EVENT_SIZE2, WLAN_MEM_EVENT_CNT2},
};

#if (TOTAL_WLAN_MEM_EVENT_SIZE1 % OAL_MEM_INFO_SIZE)
#error alignment fault error
#endif
#if (TOTAL_WLAN_MEM_EVENT_SIZE2 % OAL_MEM_INFO_SIZE)
#error alignment fault error
#endif

/******************************************************************************
    MIB内存池配置信息全局变量
*******************************************************************************/
OAL_STATIC  oal_mem_subpool_cfg_stru g_ast_mib_cfg_table[] = {
    {TOTAL_WLAN_MEM_MIB_SIZE1, WLAN_MEM_MIB_CNT1},
};

#if (TOTAL_WLAN_MEM_MIB_SIZE1 % OAL_MEM_INFO_SIZE)
#error alignment fault error
#endif

#ifdef WIN32
OAL_STATIC oal_uint8 *mem_pool[OAL_MEM_POOL_ID_BUTT] = {0};
OAL_STATIC oal_uint8 mem_pool_event[TOTAL_WLAN_MEM_EVENT_SIZE];
OAL_STATIC oal_uint8 mem_pool_shared_data_pkt[TOTAL_WLAN_MEM_SHARED_DATA_PKT_SIZE];
OAL_STATIC oal_uint8 mem_pool_shared_mgmt_pkt[TOTAL_WLAN_MEM_SHARED_MGMT_PKT_SIZE];
OAL_STATIC oal_uint8 mem_pool_local[TOTAL_WLAN_MEM_LOCAL_SIZE];
OAL_STATIC oal_uint8 mem_pool_mib[TOTAL_WLAN_MEM_MIB_SIZE];
OAL_STATIC oal_uint8 mem_pool_shared_dscr[TOTAL_WLAN_MEM_SHARED_DSCR_SIZE];
#else
OAL_STATIC oal_uint8 *mem_pool[OAL_MEM_POOL_ID_BUTT] = {0};
OAL_STATIC oal_uint8 mem_pool_event[TOTAL_WLAN_MEM_EVENT_SIZE] __attribute__((aligned(8)));
#ifndef _PRE_LITEOS_WLAN_FEATURE_MEM_REDUCE
OAL_STATIC oal_uint8 mem_pool_shared_data_pkt[TOTAL_WLAN_MEM_SHARED_DATA_PKT_SIZE] __attribute__((aligned(8)));
OAL_STATIC oal_uint8 mem_pool_shared_mgmt_pkt[TOTAL_WLAN_MEM_SHARED_MGMT_PKT_SIZE] __attribute__((aligned(8)));
OAL_STATIC oal_uint8 mem_pool_shared_dscr[TOTAL_WLAN_MEM_SHARED_DSCR_SIZE] __attribute__((aligned(8)));
#endif
OAL_STATIC oal_uint8 mem_pool_local[TOTAL_WLAN_MEM_LOCAL_SIZE] __attribute__((aligned(8)));
OAL_STATIC oal_uint8 mem_pool_mib[TOTAL_WLAN_MEM_MIB_SIZE] __attribute__((aligned(8)));
#endif

/******************************************************************************
    netbuf内存池配置信息全局变量
*******************************************************************************/
#define TOTAL_WLAN_MEM_NETBUF_SIZE1 (WLAN_MEM_NETBUF_SIZE1 + OAL_NETBUF_MAINTAINS_SIZE + OAL_DOG_TAG_SIZE)
#define TOTAL_WLAN_MEM_NETBUF_SIZE2 (WLAN_MEM_NETBUF_SIZE2 + OAL_NETBUF_MAINTAINS_SIZE + OAL_DOG_TAG_SIZE)
#define TOTAL_WLAN_MEM_NETBUF_SIZE3 (WLAN_MEM_NETBUF_SIZE3 + OAL_NETBUF_MAINTAINS_SIZE + OAL_DOG_TAG_SIZE)

/******************************************************************************
    sdt netbuf内存池配置信息全局变量
*******************************************************************************/
#define TOTAL_WLAN_MEM_SDT_NETBUF_SIZE1 (WLAN_MEM_SDT_NETBUF_SIZE1 + OAL_MEM_INFO_SIZE)
#define TOTAL_WLAN_MEM_SDT_NETBUF_SIZE2 (WLAN_MEM_SDT_NETBUF_SIZE2 + OAL_MEM_INFO_SIZE)
#define TOTAL_WLAN_MEM_SDT_NETBUF_SIZE3 (WLAN_MEM_SDT_NETBUF_SIZE3 + OAL_MEM_INFO_SIZE)
#define TOTAL_WLAN_MEM_SDT_NETBUF_SIZE4 (WLAN_MEM_SDT_NETBUF_SIZE4 + OAL_MEM_INFO_SIZE)
OAL_STATIC  oal_mem_subpool_cfg_stru g_ast_sdt_netbuf_cfg_table[] = {
    {TOTAL_WLAN_MEM_SDT_NETBUF_SIZE1, WLAN_MEM_SDT_NETBUF_SIZE1_CNT},
    {TOTAL_WLAN_MEM_SDT_NETBUF_SIZE2, WLAN_MEM_SDT_NETBUF_SIZE2_CNT},
    {TOTAL_WLAN_MEM_SDT_NETBUF_SIZE3, WLAN_MEM_SDT_NETBUF_SIZE3_CNT},
    {TOTAL_WLAN_MEM_SDT_NETBUF_SIZE4, WLAN_MEM_SDT_NETBUF_SIZE4_CNT},
};

/******************************************************************************
    总的内存池配置信息全局变量
*******************************************************************************/
OAL_STATIC  oal_mem_pool_cfg_stru g_ast_mem_pool_cfg_table[] = {
    /*       内存池ID                           内存池子内存池个数               四字节对齐      内存池配置信息 */
    {OAL_MEM_POOL_ID_EVENT,           OAL_ARRAY_SIZE(g_ast_event_cfg_table),       {0, 0}, g_ast_event_cfg_table},
#ifndef _PRE_LITEOS_WLAN_FEATURE_MEM_REDUCE
    {OAL_MEM_POOL_ID_SHARED_DATA_PKT, OAL_ARRAY_SIZE(g_ast_shared_data_cfg_table), {0, 0}, g_ast_shared_data_cfg_table},
    {OAL_MEM_POOL_ID_SHARED_MGMT_PKT, OAL_ARRAY_SIZE(g_ast_shared_mgmt_cfg_table), {0, 0}, g_ast_shared_mgmt_cfg_table},
#endif
    {OAL_MEM_POOL_ID_LOCAL,           OAL_ARRAY_SIZE(g_ast_local_cfg_table),       {0, 0}, g_ast_local_cfg_table},
    {OAL_MEM_POOL_ID_MIB,             OAL_ARRAY_SIZE(g_ast_mib_cfg_table),         {0, 0}, g_ast_mib_cfg_table},
#ifndef _PRE_LITEOS_WLAN_FEATURE_MEM_REDUCE
    {OAL_MEM_POOL_ID_SHARED_DSCR,     OAL_ARRAY_SIZE(g_ast_shared_dscr_cfg_table), {0, 0}, g_ast_shared_dscr_cfg_table},
    {OAL_MEM_POOL_ID_SDT_NETBUF,      OAL_ARRAY_SIZE(g_ast_sdt_netbuf_cfg_table),  {0, 0}, g_ast_sdt_netbuf_cfg_table}
#endif
};

/******************************************************************************
    用于索引netbuf内存块与内存池ID的映射关系
*******************************************************************************/
/******************************************************************************
    用于索引sdt netbuf内存块与内存池ID的映射关系
*******************************************************************************/
OAL_STATIC oal_uint32 g_ul_truesize_to_pool_id_sdt[OAL_ARRAY_SIZE(g_ast_sdt_netbuf_cfg_table)] = {0};

/******************************************************************************
    netbuf内存块data指针相对于head指针的偏移
*******************************************************************************/
/******************************************************************************
    netbuf内存块data指针相对于head指针的偏移
*******************************************************************************/
oal_uint32 g_ul_sdt_netbuf_def_data_offset[OAL_ARRAY_SIZE(g_ast_sdt_netbuf_cfg_table)] = {0};

/******************************************************************************
    内存池信息全局变量，存储整个内存管理中所有内存池信息
    所有内存管理的函数都基于此全局变量进行操作
*******************************************************************************/
OAL_STATIC oal_mem_pool_stru g_ast_mem_pool[OAL_MEM_POOL_ID_BUTT];

/******************************************************************************
    malloc内存指针记录
*******************************************************************************/
OAL_STATIC oal_uint8 *g_pauc_pool_base_addr[OAL_MEM_POOL_ID_BUTT] = {OAL_PTR_NULL};

/******************************************************************************
    netbuf内存索引表地址
*******************************************************************************/
/******************************************************************************
    sdt netbuf内存索引表地址
*******************************************************************************/
OAL_STATIC oal_netbuf_stru **g_ppst_sdt_netbuf_stack_mem;

/* 总内存块个数 */
#ifndef _PRE_LITEOS_WLAN_FEATURE_MEM_REDUCE
#define OAL_MEM_BLK_TOTAL_CNT   (WLAN_MEM_SHARED_RX_DSCR_CNT + WLAN_MEM_SHARED_TX_DSCR_CNT1 + WLAN_MEM_SHARED_TX_DSCR_CNT2 +\
                                 WLAN_MEM_SHARED_MGMT_PKT_CNT1 + \
                                 WLAN_MEM_SHARED_DATA_PKT_CNT + \
                                 WLAN_MEM_LOCAL_CNT1 + WLAN_MEM_LOCAL_CNT2 + WLAN_MEM_LOCAL_CNT3 + \
                                 WLAN_MEM_LOCAL_CNT4 + WLAN_MEM_LOCAL_CNT5 +\
                                 WLAN_MEM_LOCAL_CNT6 + \
                                 WLAN_MEM_EVENT_CNT1 + \
                                 WLAN_MEM_EVENT_CNT2 + \
                                 WLAN_MEM_MIB_CNT1)
#else
#define OAL_MEM_BLK_TOTAL_CNT   (WLAN_MEM_LOCAL_CNT1 + WLAN_MEM_LOCAL_CNT3 + WLAN_MEM_LOCAL_CNT4 + WLAN_MEM_LOCAL_CNT5 +\
                                 WLAN_MEM_EVENT_CNT1 + \
                                 WLAN_MEM_EVENT_CNT2 + \
                                 WLAN_MEM_MIB_CNT1)
#endif

/* 一个内存块结构大小 + 一个指针大小 */
#define OAL_MEM_CTRL_BLK_SIZE   (OAL_SIZEOF(oal_mem_stru *) + OAL_SIZEOF(oal_mem_stru))

/* netbuf内存块个数 */
#define OAL_MEM_NETBUF_BLK_TOTAL_CNT    (WLAN_MEM_NETBUF_CNT1 + WLAN_MEM_NETBUF_CNT2 + WLAN_MEM_NETBUF_CNT3)

/* sdt netbuf内存块个数 */
#define OAL_MEM_SDT_NETBUF_BLK_TOTAL_CNT    (WLAN_MEM_SDT_NETBUF_SIZE1_CNT + WLAN_MEM_SDT_NETBUF_SIZE2_CNT +\
                                             WLAN_MEM_SDT_NETBUF_SIZE3_CNT + WLAN_MEM_SDT_NETBUF_SIZE4_CNT)

/* netbuf内存指针大小 * 2 */
#define OAL_MEM_NETBUF_CTRL_BLK_SIZE    (OAL_SIZEOF(oal_netbuf_stru *) * 2)

/* 内存块结构体内存大小 */
/*
#define OAL_MEM_CTRL_BLK_TOTAL_SIZE  (OAL_MEM_BLK_TOTAL_CNT * OAL_MEM_CTRL_BLK_SIZE + \
                                      OAL_MEM_NETBUF_BLK_TOTAL_CNT * OAL_MEM_NETBUF_CTRL_BLK_SIZE + \
                                      OAL_MEM_SDT_NETBUF_BLK_TOTAL_CNT * OAL_MEM_NETBUF_CTRL_BLK_SIZE)
*/
#ifndef _PRE_LITEOS_WLAN_FEATURE_MEM_REDUCE
#define OAL_MEM_CTRL_BLK_TOTAL_SIZE  (OAL_MEM_BLK_TOTAL_CNT * OAL_MEM_CTRL_BLK_SIZE + \
                                      OAL_MEM_SDT_NETBUF_BLK_TOTAL_CNT * OAL_MEM_NETBUF_CTRL_BLK_SIZE)
#else
#define OAL_MEM_CTRL_BLK_TOTAL_SIZE (OAL_MEM_CTRL_BLK_SIZE * OAL_MEM_BLK_TOTAL_CNT)
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
OAL_STATIC oal_netbuf_stru *g_pst_sdt_netbuf_base_addr[OAL_MEM_SDT_NETBUF_BLK_TOTAL_CNT] = {OAL_PTR_NULL};
#endif
/*****************************************************************************
  结构名  : oal_mem_ctrl_blk_stru
  结构说明: 将内存块所用空间封装成一个结构体
*****************************************************************************/
typedef struct {
    oal_uint8  auc_base_addr[OAL_MEM_CTRL_BLK_TOTAL_SIZE];
    oal_uint32 ul_idx;
} oal_mem_ctrl_blk_stru;

/******************************************************************************
    控制块内存空间，为内存块结构体和指向内存块结构体的指针分配空间
    由函数oal_mem_ctrl_blk_alloc调用
*******************************************************************************/
OAL_STATIC oal_mem_ctrl_blk_stru g_st_ctrl_blk;

/*****************************************************************************
  4 宏定义
*****************************************************************************/
/* 对于enhanced类型的申请接口与释放接口，每一个内存块都包含一个4字节的头部， */
/* 用来指向内存块管理结构体oal_mem_struc，整个内存块的结构如下所示。         */
/*                                                                            */
/* +-------------------+---------------------------------------------------+ */
/* | oal_mem_stru addr |                    payload                        | */
/* +-------------------+---------------------------------------------------+ */
/* |      4 byte       |                                                   | */
/* +-------------------+---------------------------------------------------+ */
/* #define OAL_MEM_INFO_SIZE    4                                            */
/* 内存池加锁 */
#define OAL_MEM_SPIN_LOCK_BH(_st_spinlock)  \
    {                                       \
        oal_spin_lock_bh(&(_st_spinlock));    \
    }

/* 内存池解锁 */
#define OAL_MEM_SPIN_UNLOCK_BH(_st_spinlock)   \
    {                                          \
        oal_spin_unlock_bh(&(_st_spinlock));     \
    }

/* 内存池加锁(关中断) */
#define OAL_MEM_SPIN_LOCK_IRQSAVE(_st_spinlock, _ul_flag)   \
    {                                                       \
        oal_spin_lock_irq_save(&(_st_spinlock), &(_ul_flag));   \
    }

/* 内存池解锁(开中断) */
#define OAL_MEM_SPIN_UNLOCK_IRQRESTORE(_st_spinlock, _ul_flag)  \
    {                                                           \
        oal_spin_unlock_irq_restore(&(_st_spinlock), &(_ul_flag));  \
    }

/*****************************************************************************
  5 函数实现
*****************************************************************************/

OAL_STATIC oal_void  oal_mem_init_ctrl_blk(oal_void)
{
    g_st_ctrl_blk.ul_idx = 0;
}


OAL_STATIC oal_uint8* oal_mem_ctrl_blk_alloc(oal_uint32 ul_size)
{
    oal_uint8 *puc_alloc;

    ul_size = OAL_GET_4BYTE_ALIGN_VALUE(ul_size);

    if ((g_st_ctrl_blk.ul_idx + ul_size) > OAL_MEM_CTRL_BLK_TOTAL_SIZE) {
        OAL_IO_PRINT("[file = %s, line = %d], oal_mem_ctrl_blk_alloc, not_enough memory!\n",
                     __FILE__, __LINE__);
        return OAL_PTR_NULL;
    }

    puc_alloc = g_st_ctrl_blk.auc_base_addr + g_st_ctrl_blk.ul_idx;
    g_st_ctrl_blk.ul_idx += ul_size;

    return puc_alloc;
}


oal_mem_pool_stru* oal_mem_get_pool(oal_mem_pool_id_enum_uint8 en_pool_id)
{
    if (OAL_UNLIKELY(en_pool_id >= OAL_MEM_POOL_ID_BUTT)) {
        OAL_IO_PRINT("[file = %s, line = %d], oal_mem_get_pool, array overflow!\n",
                     __FILE__, __LINE__);
        return OAL_PTR_NULL;
    }

    return &g_ast_mem_pool[en_pool_id];
}

#ifdef _PRE_DEBUG_MODE

oal_mempool_tx_dscr_addr* oal_mem_get_tx_dscr_addr(oal_void)
{
    return &g_st_tx_dscr_addr;
}

oal_void oal_mem_stop_rcd_rls(oal_void)
{
    if (g_st_tx_dscr_addr.us_rcd_rls_stop_flag == 0) {
        g_st_tx_dscr_addr.us_rcd_rls_stop_flag = 1;
    }
}

oal_uint16 oal_mem_get_stop_flag(oal_void)
{
    return (g_st_tx_dscr_addr.us_rcd_rls_stop_flag >= OAL_TX_DSCR_RCD_TAIL_CNT);
}
#endif

oal_mem_pool_cfg_stru* oal_mem_get_pool_cfg_table(oal_mem_pool_id_enum_uint8 en_pool_id)
{
    if (OAL_UNLIKELY(en_pool_id >= OAL_MEM_POOL_ID_BUTT)) {
        OAL_IO_PRINT("[file = %s, line = %d], oal_mem_get_pool_cfg_table, array overflow!\n",
                     __FILE__, __LINE__);
        return OAL_PTR_NULL;
    }

    return &g_ast_mem_pool_cfg_table[en_pool_id];
}


OAL_STATIC oal_netbuf_stru* oal_mem_find_available_netbuf(oal_mem_subpool_stru *pst_mem_subpool,
                                                          oal_mem_pool_id_enum en_netbuf_id)
{
    oal_netbuf_stru   *pst_netbuf = OAL_PTR_NULL;
    oal_uint16         us_top;
    oal_uint16         us_loop;
    oal_uint16         us_has_popped_netbuf = 0;
    oal_netbuf_stru  **ppst_netbuf_stack_mem;

    us_top = pst_mem_subpool->us_free_cnt;

    if (en_netbuf_id == OAL_MEM_POOL_ID_SDT_NETBUF) {
        ppst_netbuf_stack_mem = g_ppst_sdt_netbuf_stack_mem;
    } else {
        return OAL_PTR_NULL;
    }

    while (us_top != 0) {
        us_top--;
        pst_netbuf = (oal_netbuf_stru *)pst_mem_subpool->ppst_free_stack[us_top];
        if (oal_netbuf_read_user(pst_netbuf) == 1) {
            break;
        }

        /* 如果netbuf的引用计数不为1，记录已弹出的netbuf内存指针的地址 */
        ppst_netbuf_stack_mem[us_has_popped_netbuf++] = pst_netbuf;
    }

    /* 将已弹出的netbuf内存指针再压回堆栈中 */
    for (us_loop = us_has_popped_netbuf; us_loop > 0; us_loop--) {
        pst_mem_subpool->ppst_free_stack[us_top++] = (oal_netbuf_stru *)ppst_netbuf_stack_mem[us_loop - 1];
    }

    /* 如果弹出的netbuf指针个数等于内存池可用内存个数，则认为该子内存池中暂时还没有可使用的内存(还被Qdisc缓存着) */
    if (us_has_popped_netbuf == pst_mem_subpool->us_free_cnt) {
        return OAL_PTR_NULL;
    }

    /* 更新子内存池可用内存块数 */
    pst_mem_subpool->us_free_cnt--;

    return pst_netbuf;
}


OAL_STATIC oal_void  oal_mem_release(oal_void)
{
    oal_uint32  ul_pool_id;

    for (ul_pool_id = 0; ul_pool_id < OAL_MEM_POOL_ID_BUTT; ul_pool_id++) {
        if (g_pauc_pool_base_addr[ul_pool_id] != OAL_PTR_NULL) {
            g_pauc_pool_base_addr[ul_pool_id] = OAL_PTR_NULL;
        }
    }
}


OAL_STATIC oal_void  oal_mem_sdt_netbuf_release(oal_void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
    oal_uint32    ul_loop;

    for (ul_loop = 0; ul_loop < OAL_MEM_SDT_NETBUF_BLK_TOTAL_CNT; ul_loop++) {
        if (g_pst_sdt_netbuf_base_addr[ul_loop] == OAL_PTR_NULL) {
            continue;
        }

        /* 无论netbuf引用计数是多少，统一将其设置为1 */
        oal_netbuf_set_user(g_pst_sdt_netbuf_base_addr[ul_loop], 1);

        oal_netbuf_free(g_pst_sdt_netbuf_base_addr[ul_loop]);

        g_pst_sdt_netbuf_base_addr[ul_loop] = OAL_PTR_NULL;
    }
#endif
}


OAL_STATIC oal_uint32  oal_mem_create_subpool(oal_mem_pool_id_enum_uint8 en_pool_id, oal_uint8 *puc_base_addr)
{
    oal_mem_pool_stru      *pst_mem_pool;
    oal_mem_subpool_stru   *pst_mem_subpool;
    oal_mem_stru           *pst_mem;
    oal_mem_stru          **ppst_stack_mem;
    oal_uint8               uc_subpool_id;
    oal_uint32              ul_blk_id;

    if (puc_base_addr == OAL_PTR_NULL) {
        OAL_IO_PRINT("[file = %s, line = %d], oal_mem_create_subpool, pointer is NULL!\n",
                     __FILE__, __LINE__);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mem_pool = oal_mem_get_pool(en_pool_id);
    if (pst_mem_pool == OAL_PTR_NULL) {
        OAL_IO_PRINT("[file = %s, line = %d], oal_mem_create_subpool, pointer is NULL!\n",
                     __FILE__, __LINE__);
        return OAL_ERR_CODE_OAL_MEM_GET_POOL_FAIL;
    }

    /* 申请可用内存地址索引表，每个内存池申请一次，后面分割给每个子内存池使用 */
    ppst_stack_mem = (oal_mem_stru **)oal_mem_ctrl_blk_alloc(sizeof(oal_mem_stru *) * pst_mem_pool->us_mem_total_cnt);
    /* 从一块数组空间申请内存，申请失败时不需释放 */
    if (ppst_stack_mem == OAL_PTR_NULL) {
        OAL_IO_PRINT("[file = %s, line = %d], oal_mem_create_subpool, pointer is NULL!\n",
                     __FILE__, __LINE__);
        return OAL_ERR_CODE_OAL_MEM_ALLOC_CTRL_BLK_FAIL;
    }

    /* 申请oal_mem_stru结构体，每个内存池申请一次，后面分割给每个子内存池使用 */
    pst_mem = (oal_mem_stru *)oal_mem_ctrl_blk_alloc(sizeof(oal_mem_stru) * pst_mem_pool->us_mem_total_cnt);
    /* 从一块数组空间申请内存，申请失败时不需释放 */
    if (pst_mem == OAL_PTR_NULL) {
        OAL_IO_PRINT("[file = %s, line = %d], oal_mem_create_subpool, pointer is NULL!\n",
                     __FILE__, __LINE__);
        return OAL_ERR_CODE_OAL_MEM_ALLOC_CTRL_BLK_FAIL;
    }

    memset_s(ppst_stack_mem, sizeof(oal_mem_stru *) * pst_mem_pool->us_mem_total_cnt, 0,
             sizeof(oal_mem_stru *) * pst_mem_pool->us_mem_total_cnt);
    memset_s((void *)pst_mem, sizeof(oal_mem_stru) * pst_mem_pool->us_mem_total_cnt, 0,
             sizeof(oal_mem_stru) * pst_mem_pool->us_mem_total_cnt);

    /* 记录该内存池初始oal_mem_stru结构的指针，检查内存信息时使用 */
    pst_mem_pool->pst_mem_start_addr = pst_mem;

    /* 设置各子池所有内存块结构体信息，建立各内存块与payload的关系 */
    for (uc_subpool_id = 0; uc_subpool_id < pst_mem_pool->uc_subpool_cnt; uc_subpool_id++) {
        /* 得到每一级子内存池信息 */
        pst_mem_subpool = &(pst_mem_pool->ast_subpool_table[uc_subpool_id]);

        /* 建立子内存数索引表和可用内存索引表的关系 */
        pst_mem_subpool->ppst_free_stack = (void **)ppst_stack_mem;

        for (ul_blk_id = 0; ul_blk_id < pst_mem_subpool->us_total_cnt; ul_blk_id++) {
            pst_mem->en_pool_id        = en_pool_id;
            pst_mem->uc_subpool_id     = uc_subpool_id;
            pst_mem->us_len            = pst_mem_subpool->us_len;
            pst_mem->en_mem_state_flag = OAL_MEM_STATE_FREE;
            pst_mem->uc_user_cnt       = 0;
            pst_mem->puc_origin_data   = puc_base_addr;       /* 建立oal_mem_st与对应payload的关系 */
            pst_mem->puc_data          = pst_mem->puc_origin_data;

#ifdef _PRE_DEBUG_MODE
            if ((uc_subpool_id > 0) && (en_pool_id == OAL_MEM_POOL_ID_SHARED_DSCR)) {
                g_st_tx_dscr_addr.ul_tx_dscr_addr[g_st_tx_dscr_addr.us_tx_dscr_cnt++] =
                    (uintptr_t)(pst_mem->puc_data + OAL_MEM_INFO_SIZE);
            }

            memset_s(pst_mem->ul_alloc_core_id,    OAL_SIZEOF(oal_uint32) * WLAN_MEM_MAX_USERS_NUM, 0,
                     OAL_SIZEOF(oal_uint32) * WLAN_MEM_MAX_USERS_NUM);
            memset_s(pst_mem->ul_alloc_file_id,    OAL_SIZEOF(oal_uint32) * WLAN_MEM_MAX_USERS_NUM, 0,
                     OAL_SIZEOF(oal_uint32) * WLAN_MEM_MAX_USERS_NUM);
            memset_s(pst_mem->ul_alloc_line_num,   OAL_SIZEOF(oal_uint32) * WLAN_MEM_MAX_USERS_NUM, 0,
                     OAL_SIZEOF(oal_uint32) * WLAN_MEM_MAX_USERS_NUM);
            memset_s(pst_mem->ul_alloc_time_stamp, OAL_SIZEOF(oal_uint32) * WLAN_MEM_MAX_USERS_NUM, 0,
                     OAL_SIZEOF(oal_uint32) * WLAN_MEM_MAX_USERS_NUM);
            pst_mem->ul_trace_file_id    = 0;
            pst_mem->ul_trace_line_num   = 0;
            pst_mem->ul_trace_time_stamp = 0;
            /* 设置狗牌 */
            *((oal_uint32 *)(pst_mem->puc_origin_data + pst_mem->us_len - OAL_DOG_TAG_SIZE)) = (oal_uint32)OAL_DOG_TAG;
#endif
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
            pst_mem->ul_return_addr  = 0;
#endif

            *ppst_stack_mem = pst_mem;
            ppst_stack_mem++;
            pst_mem++;

            puc_base_addr += pst_mem_subpool->us_len;
        }
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  oal_mem_create_sdt_netbuf_subpool(oal_mem_pool_id_enum_uint8 en_pool_id)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
    oal_mem_pool_stru      *pst_mem_pool;
    oal_mem_subpool_stru   *pst_mem_subpool;
    oal_netbuf_stru       **ppst_stack_mem;
    oal_netbuf_stru        *pst_netbuf;
    oal_uint8               uc_subpool_id;
    oal_uint32              ul_blk_id;
    oal_uint32              ul_mem_total_cnt;
    uint32_t                sdt_netbuf_base_addr_index;

    pst_mem_pool = oal_mem_get_pool(en_pool_id);
    if (pst_mem_pool == OAL_PTR_NULL) {
        OAL_IO_PRINT("[file = %s, line = %d], oal_mem_create_sdt_netbuf_subpool, pointer is NULL!\n",
                     __FILE__, __LINE__);
        return OAL_ERR_CODE_OAL_MEM_GET_POOL_FAIL;
    }

    /* 申请可用内存地址索引表，每个内存池申请一次，后面分割给每个子内存池使用 */
    ppst_stack_mem = (oal_netbuf_stru **)oal_mem_ctrl_blk_alloc(sizeof(oal_netbuf_stru *) *
                                                                pst_mem_pool->us_mem_total_cnt);
    /* 从一块数组空间申请内存，申请失败时不需释放 */
    if (ppst_stack_mem == OAL_PTR_NULL) {
        OAL_IO_PRINT("[file = %s, line = %d], oal_mem_create_sdt_netbuf_subpool, pointer is NULL!\n",
                     __FILE__, __LINE__);
        return OAL_ERR_CODE_OAL_MEM_ALLOC_CTRL_BLK_FAIL;
    }

    /* 申请临时内存地址索引表，供后续查找可用内存使用 */
    g_ppst_sdt_netbuf_stack_mem = (oal_netbuf_stru **)oal_mem_ctrl_blk_alloc(sizeof(oal_netbuf_stru *) *
                                                                             pst_mem_pool->us_mem_total_cnt);
    if (g_ppst_sdt_netbuf_stack_mem == OAL_PTR_NULL) {
        OAL_IO_PRINT("[file = %s, line = %d], oal_mem_create_sdt_netbuf_subpool, pointer is NULL!\n",
                     __FILE__, __LINE__);
        return OAL_ERR_CODE_OAL_MEM_ALLOC_CTRL_BLK_FAIL;
    }
    memset_s(g_ppst_sdt_netbuf_stack_mem, sizeof(oal_mem_stru *) * pst_mem_pool->us_mem_total_cnt, 0,
             sizeof(oal_mem_stru *) * pst_mem_pool->us_mem_total_cnt);
    memset_s(ppst_stack_mem, sizeof(oal_mem_stru *) * pst_mem_pool->us_mem_total_cnt, 0,
             sizeof(oal_mem_stru *) * pst_mem_pool->us_mem_total_cnt);

    ul_mem_total_cnt = 0;
    for (uc_subpool_id = 0; uc_subpool_id < pst_mem_pool->uc_subpool_cnt; uc_subpool_id++) {
        /* 得到每一级子内存池信息 */
        pst_mem_subpool = &(pst_mem_pool->ast_subpool_table[uc_subpool_id]);

        /* 建立子内存数索引表和可用内存索引表的关系 */
        pst_mem_subpool->ppst_free_stack = (void **)ppst_stack_mem;

        for (ul_blk_id = 0; ul_blk_id < pst_mem_subpool->us_total_cnt; ul_blk_id++) {
            pst_netbuf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, pst_mem_subpool->us_len, OAL_NETBUF_PRIORITY_MID);
            if (OAL_UNLIKELY(pst_netbuf == OAL_PTR_NULL)) {
                oal_mem_sdt_netbuf_release();
                OAL_IO_PRINT("[file = %s, line = %d], oal_mem_create_sdt_netbuf_subpool, pointer is NULL!\n",
                             __FILE__, __LINE__);
                return OAL_ERR_CODE_ALLOC_MEM_FAIL;
            }

            if (ul_blk_id == 0) {
                g_ul_truesize_to_pool_id_sdt[uc_subpool_id] = pst_netbuf->truesize;

                g_ul_sdt_netbuf_def_data_offset[uc_subpool_id] =
                    ((uintptr_t)pst_netbuf->data > (uintptr_t)pst_netbuf->head) ?
                    ((uintptr_t)pst_netbuf->data - (uintptr_t)pst_netbuf->head) : 0;
            }
            sdt_netbuf_base_addr_index = ul_mem_total_cnt + ul_blk_id;
            g_pst_sdt_netbuf_base_addr[sdt_netbuf_base_addr_index] = pst_netbuf;

            *ppst_stack_mem = pst_netbuf;
            ppst_stack_mem++;
        }

        ul_mem_total_cnt += pst_mem_subpool->us_total_cnt;
    }
#endif
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  oal_mem_create_pool(oal_mem_pool_id_enum_uint8 en_pool_id, oal_uint8 *puc_base_addr)
{
    oal_uint8                           uc_subpool_id;
    oal_uint8                           uc_subpool_cnt;
    oal_mem_pool_stru                  *pst_mem_pool;
    oal_mem_subpool_stru               *pst_mem_subpool;
    oal_uint32                          ul_ret;
    OAL_CONST oal_mem_pool_cfg_stru    *pst_mem_pool_cfg;
    OAL_CONST oal_mem_subpool_cfg_stru *pst_mem_subpool_cfg;

    /* 入参判断 */
    if (en_pool_id >= OAL_MEM_POOL_ID_BUTT) {
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    pst_mem_pool = oal_mem_get_pool(en_pool_id);
    if (pst_mem_pool == OAL_PTR_NULL) {
        OAL_IO_PRINT("[file = %s, line = %d], oal_mem_create_pool, pointer is NULL!\n", __FILE__, __LINE__);
        return OAL_ERR_CODE_OAL_MEM_GET_POOL_FAIL;
    }

    pst_mem_pool_cfg = oal_mem_get_pool_cfg_table(en_pool_id);
    if (pst_mem_pool_cfg == OAL_PTR_NULL) {
        OAL_IO_PRINT("[file = %s, line = %d], oal_mem_create_pool, pointer is NULL!\n", __FILE__, __LINE__);
        return OAL_ERR_CODE_OAL_MEM_GET_CFG_TBL_FAIL;
    }

    /* 初始化内存池的通用变量 */
    uc_subpool_cnt = pst_mem_pool_cfg->uc_subpool_cnt;

    pst_mem_pool->uc_subpool_cnt  = uc_subpool_cnt;
    pst_mem_pool->us_mem_used_cnt = 0;

    pst_mem_pool->us_max_byte_len = pst_mem_pool_cfg->pst_subpool_cfg_info[uc_subpool_cnt - 1].us_size;
    if (pst_mem_pool->us_max_byte_len >= WLAN_MEM_MAX_BYTE_LEN) {
        OAL_IO_PRINT("[file = %s, line = %d], oal_mem_create_pool, exceeds the max length!\n", __FILE__, __LINE__);
        return OAL_ERR_CODE_OAL_MEM_EXCEED_MAX_LEN;
    }

    if (pst_mem_pool->uc_subpool_cnt > WLAN_MEM_MAX_SUBPOOL_NUM) {
        return OAL_ERR_CODE_OAL_MEM_EXCEED_SUBPOOL_CNT;
    }

    /* 子池索引表初始清零 */
    memset_s((void *)pst_mem_pool->ast_subpool_table, sizeof(pst_mem_pool->ast_subpool_table), 0,
             sizeof(pst_mem_pool->ast_subpool_table));

    /* 设置每一级子内存池 */
    for (uc_subpool_id = 0; uc_subpool_id < uc_subpool_cnt; uc_subpool_id++) {
        pst_mem_subpool_cfg           = pst_mem_pool_cfg->pst_subpool_cfg_info + uc_subpool_id;
        pst_mem_subpool               = &(pst_mem_pool->ast_subpool_table[uc_subpool_id]);

        pst_mem_subpool->us_free_cnt  = pst_mem_subpool_cfg->us_cnt;
        pst_mem_subpool->us_total_cnt = pst_mem_subpool_cfg->us_cnt;
        pst_mem_subpool->us_len       = pst_mem_subpool_cfg->us_size;

        oal_spin_lock_init(&pst_mem_subpool->st_spinlock);

        pst_mem_pool->us_mem_total_cnt += pst_mem_subpool_cfg->us_cnt;   /* 设置总内存块数 */
    }

    if (en_pool_id == OAL_MEM_POOL_ID_SDT_NETBUF) {
        /* 创建sdt netbuf内存池 */
        ul_ret = oal_mem_create_sdt_netbuf_subpool(en_pool_id);
    } else {
        /* 创建普通内存池 */
        ul_ret = oal_mem_create_subpool(en_pool_id, puc_base_addr);
    }

    return ul_ret;
}

/*
 * 函 数 名  : oal_mem_init_static_mem_map
 * 功能描述  : 初始化静态内存池
 */
OAL_STATIC oal_void oal_mem_init_static_mem_map(oal_void)
{
    memset_s(mem_pool_event, OAL_SIZEOF(mem_pool_event), 0, OAL_SIZEOF(mem_pool_event));
#ifndef _PRE_LITEOS_WLAN_FEATURE_MEM_REDUCE
    memset_s(mem_pool_shared_data_pkt, OAL_SIZEOF(mem_pool_shared_data_pkt), 0, OAL_SIZEOF(mem_pool_shared_data_pkt));
    memset_s(mem_pool_shared_mgmt_pkt, OAL_SIZEOF(mem_pool_shared_mgmt_pkt), 0, OAL_SIZEOF(mem_pool_shared_mgmt_pkt));
#endif
    memset_s(mem_pool_local, OAL_SIZEOF(mem_pool_local), 0, OAL_SIZEOF(mem_pool_local));
    memset_s(mem_pool_mib, OAL_SIZEOF(mem_pool_mib), 0, OAL_SIZEOF(mem_pool_mib));
#ifndef _PRE_LITEOS_WLAN_FEATURE_MEM_REDUCE
    memset_s(mem_pool_shared_dscr, OAL_SIZEOF(mem_pool_shared_dscr), 0, OAL_SIZEOF(mem_pool_shared_dscr));
#endif

    mem_pool[OAL_MEM_POOL_ID_EVENT]           = mem_pool_event;
#ifndef _PRE_LITEOS_WLAN_FEATURE_MEM_REDUCE
    mem_pool[OAL_MEM_POOL_ID_SHARED_DATA_PKT] = mem_pool_shared_data_pkt;
    mem_pool[OAL_MEM_POOL_ID_SHARED_MGMT_PKT] = mem_pool_shared_mgmt_pkt;
#endif
    mem_pool[OAL_MEM_POOL_ID_LOCAL]           = mem_pool_local;
    mem_pool[OAL_MEM_POOL_ID_MIB]             = mem_pool_mib;
#ifndef _PRE_LITEOS_WLAN_FEATURE_MEM_REDUCE
    mem_pool[OAL_MEM_POOL_ID_SHARED_DSCR]     = mem_pool_shared_dscr;
#endif
}


oal_uint32  oal_mem_init_pool(oal_void)
{
    oal_uint32    ul_pool_id;         /* 内存池循环计数变量 */
    oal_uint32    ul_ret;
    oal_uint8    *puc_base_addr;               /* 定义malloc申请的内存基地址 */

    memset_s((oal_void *)g_ast_mem_pool, OAL_SIZEOF(g_ast_mem_pool), 0, OAL_SIZEOF(g_ast_mem_pool));
    memset_s((oal_void *)g_pauc_pool_base_addr, OAL_SIZEOF(g_pauc_pool_base_addr),
             0, OAL_SIZEOF(g_pauc_pool_base_addr));
#ifdef _PRE_DEBUG_MODE
    memset_s(&g_st_tx_dscr_addr, OAL_SIZEOF(g_st_tx_dscr_addr), 0, OAL_SIZEOF(g_st_tx_dscr_addr));
#endif

    /* 初始化控制块内存 */
    oal_mem_init_ctrl_blk();

    /* 初始化静态内存对应关系 */
    oal_mem_init_static_mem_map();

#ifndef _PRE_LITEOS_WLAN_FEATURE_MEM_REDUCE
    for (ul_pool_id = 0; ul_pool_id < OAL_MEM_POOL_ID_SDT_NETBUF; ul_pool_id++) {
#else
    for (ul_pool_id = 0; ul_pool_id < OAL_MEM_POOL_ID_BUTT; ul_pool_id++) {
#endif
        puc_base_addr = mem_pool[ul_pool_id];
        if (puc_base_addr == OAL_PTR_NULL) {
            oal_mem_release();
            OAL_IO_PRINT("[file = %s, line = %d], oal_mem_init_pool, memory allocation fail!\n",
                         __FILE__, __LINE__);
            return OAL_ERR_CODE_ALLOC_MEM_FAIL;
        }

        /* 记录每个内存池oal_malloc分配的地址 */
        g_pauc_pool_base_addr[ul_pool_id] = puc_base_addr;

        puc_base_addr = (oal_uint8 *)(uintptr_t)OAL_GET_4BYTE_ALIGN_VALUE((uintptr_t)puc_base_addr);

        ul_ret = oal_mem_create_pool((oal_uint8)ul_pool_id, puc_base_addr);
        if (ul_ret != OAL_SUCC) {
            oal_mem_release();
            OAL_IO_PRINT("[file = %s, line = %d], oal_mem_init_pool, oal_mem_create_pool failed!\n",
                         __FILE__, __LINE__);
            return ul_ret;
        }
    }

    /* 创建sdt netbuf内存池 */
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
    return oal_mem_create_pool(OAL_MEM_POOL_ID_SDT_NETBUF, OAL_PTR_NULL);
#elif (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    return OAL_SUCC;
#endif
}


oal_mem_stru* oal_mem_alloc_enhanced(
    oal_uint32                    ul_file_id,
    oal_uint32                    ul_line_num,
    oal_mem_pool_id_enum_uint8    en_pool_id,
    oal_uint16                    us_len,
    oal_uint8                     uc_lock)
{
    oal_mem_pool_stru    *pst_mem_pool;
    oal_mem_subpool_stru *pst_mem_subpool;
    oal_mem_stru         *pst_mem;
    oal_uint8             uc_subpool_id;
    oal_ulong              ul_irq_flag = 0;

    /* 获取内存池 */
    pst_mem_pool = oal_mem_get_pool(en_pool_id);
    if (pst_mem_pool == OAL_PTR_NULL) {
        return OAL_PTR_NULL;
    }

    us_len += OAL_DOG_TAG_SIZE;

    /* 异常: 申请长度不在该内存池内  */
    if (OAL_UNLIKELY(us_len > pst_mem_pool->us_max_byte_len)) {
        return OAL_PTR_NULL;
    }

    pst_mem = OAL_PTR_NULL;

    for (uc_subpool_id = 0; uc_subpool_id < pst_mem_pool->uc_subpool_cnt; uc_subpool_id++) {
        pst_mem_subpool = &(pst_mem_pool->ast_subpool_table[uc_subpool_id]);

        OAL_MEM_SPIN_LOCK_IRQSAVE(pst_mem_subpool->st_spinlock, ul_irq_flag)
        if ((pst_mem_subpool->us_len < us_len) || (pst_mem_subpool->us_free_cnt == 0)) {
            OAL_MEM_SPIN_UNLOCK_IRQRESTORE(pst_mem_subpool->st_spinlock, ul_irq_flag)
            continue;
        }

        /* 获取一个没有使用的oal_mem_stru结点 */
        pst_mem_subpool->us_free_cnt--;
        pst_mem = (oal_mem_stru *)pst_mem_subpool->ppst_free_stack[pst_mem_subpool->us_free_cnt];

        pst_mem->puc_data          = pst_mem->puc_origin_data;
        pst_mem->uc_user_cnt       = 1;
        pst_mem->en_mem_state_flag = OAL_MEM_STATE_ALLOC;

        pst_mem_pool->us_mem_used_cnt++;

#ifdef _PRE_DEBUG_MODE
        pst_mem->ul_alloc_core_id[pst_mem->uc_user_cnt - 1]    = OAL_GET_CORE_ID();
        pst_mem->ul_alloc_file_id[pst_mem->uc_user_cnt - 1]    = ul_file_id;
        pst_mem->ul_alloc_line_num[pst_mem->uc_user_cnt - 1]   = ul_line_num;
        pst_mem->ul_alloc_time_stamp[pst_mem->uc_user_cnt - 1] = (oal_uint32)OAL_TIME_GET_STAMP_MS();
#endif
        OAL_MEM_SPIN_UNLOCK_IRQRESTORE(pst_mem_subpool->st_spinlock, ul_irq_flag)
        break;
    }

#ifdef _PRE_DEBUG_MODE
    if (pst_mem == OAL_PTR_NULL) {
    }
#endif

    return pst_mem;
}


oal_uint32  oal_mem_free_enhanced(
    oal_uint32      ul_file_id,
    oal_uint32      ul_line_num,
    oal_mem_stru   *pst_mem,
    oal_uint8       uc_lock)
{
    oal_mem_pool_stru      *pst_mem_pool;
    oal_mem_subpool_stru   *pst_mem_subpool;
#ifdef _PRE_DEBUG_MODE
    oal_uint32              ul_dog_tag;
#endif

    oal_ulong              ul_irq_flag = 0;
    if (OAL_UNLIKELY(pst_mem == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mem_pool = oal_mem_get_pool(pst_mem->en_pool_id);
    if (pst_mem_pool == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mem_subpool = &(pst_mem_pool->ast_subpool_table[pst_mem->uc_subpool_id]);

#ifdef _PRE_DEBUG_MODE
    if ((pst_mem->en_pool_id == OAL_MEM_POOL_ID_SHARED_DSCR) && (pst_mem->uc_subpool_id > 0) &&
        (oal_mem_get_stop_flag() == 0)) {
        g_st_tx_dscr_addr.us_released_tx_dscr_cnt %= OAL_TX_DSCR_ITEM_NUM;
        g_st_tx_dscr_addr.ast_tx_dscr_info[g_st_tx_dscr_addr.us_released_tx_dscr_cnt].ul_released_addr =
            (uintptr_t)(pst_mem->puc_data);
        g_st_tx_dscr_addr.ast_tx_dscr_info[g_st_tx_dscr_addr.us_released_tx_dscr_cnt].ul_release_file_id = ul_file_id;
        g_st_tx_dscr_addr.ast_tx_dscr_info[g_st_tx_dscr_addr.us_released_tx_dscr_cnt].ul_release_line_num =
            ul_line_num;
        g_st_tx_dscr_addr.ast_tx_dscr_info[g_st_tx_dscr_addr.us_released_tx_dscr_cnt++].ul_release_ts =
            (oal_uint32)OAL_TIME_GET_STAMP_MS();
    }
#endif

    OAL_MEM_SPIN_LOCK_IRQSAVE(pst_mem_subpool->st_spinlock, ul_irq_flag)

    /* 异常: 内存写越界 */
#ifdef _PRE_DEBUG_MODE
    ul_dog_tag = (*((oal_uint32 *)(pst_mem->puc_origin_data + pst_mem->us_len - OAL_DOG_TAG_SIZE)));
    if (ul_dog_tag != OAL_DOG_TAG) {
        /* 恢复狗牌设置 */
        OAL_MEM_SPIN_UNLOCK_IRQRESTORE(pst_mem_subpool->st_spinlock, ul_irq_flag)
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        OAL_BUG_ON(1);
#endif
        return OAL_ERR_CODE_OAL_MEM_DOG_TAG;
    }
#endif

    /* 异常: 释放一块已经被释放的内存 */
    if (OAL_UNLIKELY(pst_mem->en_mem_state_flag == OAL_MEM_STATE_FREE)) {
        OAL_MEM_SPIN_UNLOCK_IRQRESTORE(pst_mem_subpool->st_spinlock, ul_irq_flag)
        return OAL_ERR_CODE_OAL_MEM_ALREADY_FREE;
    }

    /* 异常: 释放一块引用计数为0的内存 */
    if (OAL_UNLIKELY(pst_mem->uc_user_cnt == 0)) {
        OAL_MEM_SPIN_UNLOCK_IRQRESTORE(pst_mem_subpool->st_spinlock, ul_irq_flag)
        return OAL_ERR_CODE_OAL_MEM_USER_CNT_ERR;
    }

#ifdef _PRE_DEBUG_MODE
    pst_mem->ul_alloc_core_id[pst_mem->uc_user_cnt - 1]    = 0;
    pst_mem->ul_alloc_file_id[pst_mem->uc_user_cnt - 1]    = ul_file_id;
    pst_mem->ul_alloc_line_num[pst_mem->uc_user_cnt - 1]   = ul_line_num;
    pst_mem->ul_alloc_time_stamp[pst_mem->uc_user_cnt - 1] = 0;
#endif

    pst_mem->uc_user_cnt--;

    /* 该内存块上是否还有其他共享用户，直接返回 */
    if (pst_mem->uc_user_cnt != 0) {
        OAL_MEM_SPIN_UNLOCK_IRQRESTORE(pst_mem_subpool->st_spinlock, ul_irq_flag)

        return OAL_SUCC;
    }

    /* 异常: 该子内存池可用内存块数目超过整个子内存池总内存块数 */
    if (OAL_UNLIKELY(pst_mem_subpool->us_free_cnt >= pst_mem_subpool->us_total_cnt)) {
        OAL_MEM_SPIN_UNLOCK_IRQRESTORE(pst_mem_subpool->st_spinlock, ul_irq_flag)

        return OAL_ERR_CODE_OAL_MEM_EXCEED_TOTAL_CNT;
    }

#ifdef _PRE_DEBUG_MODE
    pst_mem->ul_trace_file_id    = 0;
    pst_mem->ul_trace_line_num   = 0;
    pst_mem->ul_trace_time_stamp = 0;
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    pst_mem->ul_return_addr  = 0;
#endif

    pst_mem->en_mem_state_flag = OAL_MEM_STATE_FREE;

    pst_mem_subpool->ppst_free_stack[pst_mem_subpool->us_free_cnt] = (void *)pst_mem;
    pst_mem_subpool->us_free_cnt++;

    pst_mem_pool->us_mem_used_cnt--;

    OAL_MEM_SPIN_UNLOCK_IRQRESTORE(pst_mem_subpool->st_spinlock, ul_irq_flag)

    return OAL_SUCC;
}


oal_void* oal_mem_alloc(
    oal_uint32                    ul_file_id,
    oal_uint32                    ul_line_num,
    oal_mem_pool_id_enum_uint8    en_pool_id,
    oal_uint16                    us_len,
    oal_uint8                     uc_lock)
{
    oal_mem_stru *pst_mem;

    /* 异常: 申请长度为零 */
    if (OAL_UNLIKELY(us_len == 0)) {
        return OAL_PTR_NULL;
    }

    us_len += OAL_MEM_INFO_SIZE;

    pst_mem = oal_mem_alloc_enhanced(ul_file_id, ul_line_num, en_pool_id, us_len, uc_lock);
    if (OAL_UNLIKELY(pst_mem == OAL_PTR_NULL)) {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        if (en_pool_id < OAL_MEM_POOL_ID_SHARED_DSCR) {
            oal_mem_print_normal_pool_info(en_pool_id);
        }
#endif

        return OAL_PTR_NULL;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    pst_mem->ul_return_addr  = oal_get_func_return_address();
#endif

    pst_mem->puc_data = pst_mem->puc_origin_data + OAL_MEM_INFO_SIZE;

    *((oal_ulong *)(pst_mem->puc_data - OAL_MEM_INFO_SIZE)) = (uintptr_t)pst_mem;

    return (oal_void *)pst_mem->puc_data;
}


oal_uint32  oal_mem_free(
    oal_uint32    ul_file_id,
    oal_uint32    ul_line_num,
    oal_void     *p_data,
    oal_uint8     uc_lock)
{
    oal_mem_stru   *pst_mem;
    oal_uint32      ul_data;
    oal_uint32      ul_ret;

    if (OAL_UNLIKELY(p_data == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mem = (oal_mem_stru *)(*((uintptr_t *)((uintptr_t)p_data - OAL_MEM_INFO_SIZE)));
    if (((uintptr_t)pst_mem) < 0xfff) {
        ul_data = (*((uintptr_t *)((uintptr_t)p_data - OAL_MEM_INFO_SIZE - OAL_DOG_TAG_SIZE)));
        OAL_IO_PRINT("oal_mem_free mem covered 0x%x \n", ul_data);
        return OAL_FAIL;
    }

    ul_ret = oal_mem_free_enhanced(ul_file_id, ul_line_num, pst_mem, uc_lock);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "oal_mem_free failed, ul_ret %d", ul_ret);
#ifdef _PRE_DEBUG_MODE
        OAM_ERROR_LOG4(0, OAM_SF_ANY,
                       "oal_mem_free failed, last free file_id %d, line_num %d, current free file_id %d, line_num %d",
                       pst_mem->ul_alloc_file_id[0], pst_mem->ul_alloc_line_num[0], ul_file_id, ul_line_num);
#endif
#endif
    }

    return ul_ret;
}


oal_netbuf_stru* oal_mem_sdt_netbuf_alloc(oal_uint16 us_len, oal_uint8 uc_lock)
{
    oal_mem_pool_stru      *pst_mem_pool;
    oal_mem_subpool_stru   *pst_mem_subpool;
    oal_netbuf_stru        *pst_netbuf = OAL_PTR_NULL;
    oal_uint8               uc_subpool_id;
    oal_ulong                ul_irq_flag = 0;
    oal_uint32              ul_headroom;

    /* 获取内存池 */
    pst_mem_pool = &g_ast_mem_pool[OAL_MEM_POOL_ID_SDT_NETBUF];

    /* 异常: 申请长度不在该内存池内  */
    if (OAL_UNLIKELY(us_len > pst_mem_pool->us_max_byte_len)) {
        return OAL_PTR_NULL;
    }

    for (uc_subpool_id = 0; uc_subpool_id < pst_mem_pool->uc_subpool_cnt; uc_subpool_id++) {
        pst_mem_subpool = &(pst_mem_pool->ast_subpool_table[uc_subpool_id]);

        OAL_MEM_SPIN_LOCK_IRQSAVE(pst_mem_subpool->st_spinlock, ul_irq_flag)

        /* 如果该子内存池可用内存块数为0，或者长度不符合，则跳到下一级子内存池寻找 */
        if ((pst_mem_subpool->us_len < us_len) || (pst_mem_subpool->us_free_cnt == 0)) {
            OAL_MEM_SPIN_UNLOCK_IRQRESTORE(pst_mem_subpool->st_spinlock, ul_irq_flag)
            continue;
        }

        /* 如果该子内存池中找不到一块可用内存(引用计数为1的内存)，则跳到下一级子内存池寻找 */
        pst_netbuf = oal_mem_find_available_netbuf(pst_mem_subpool, OAL_MEM_POOL_ID_SDT_NETBUF);
        if (pst_netbuf == OAL_PTR_NULL) {
            OAL_MEM_SPIN_UNLOCK_IRQRESTORE(pst_mem_subpool->st_spinlock, ul_irq_flag)
            continue;
        }

        /* 引用计数加1 */
        oal_netbuf_increase_user(pst_netbuf);

        pst_mem_pool->us_mem_used_cnt++;

        /* netbuf的data指针复位 */
#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
        ul_headroom = OAL_NETBUF_DEFAULT_DATA_OFFSET;
#else
        ul_headroom = g_ul_sdt_netbuf_def_data_offset[uc_subpool_id];
#endif
        if (oal_netbuf_headroom(pst_netbuf) > ul_headroom) {
            oal_netbuf_push(pst_netbuf, oal_netbuf_headroom(pst_netbuf) - ul_headroom);
        } else {
            oal_netbuf_pull(pst_netbuf, ul_headroom - oal_netbuf_headroom(pst_netbuf));
        }

        OAL_BUG_ON(oal_netbuf_headroom(pst_netbuf) != ul_headroom);

        oal_netbuf_trim(pst_netbuf, pst_netbuf->len);

        OAL_BUG_ON(pst_netbuf->len);

        OAL_MEM_SPIN_UNLOCK_IRQRESTORE(pst_mem_subpool->st_spinlock, ul_irq_flag)

        break;
    }

    return pst_netbuf;
}


OAL_STATIC OAL_INLINE oal_uint32 oal_mem_find_sdt_netbuf_subpool_id(oal_netbuf_stru   * pst_netbuf,
                                                                    oal_mem_pool_stru * pst_mem_pool,
                                                                    oal_uint8         * puc_subpool_id)
{
    oal_uint8 uc_subpool_id;

    for (uc_subpool_id = 0; uc_subpool_id < pst_mem_pool->uc_subpool_cnt; uc_subpool_id++) {
        if (g_ul_truesize_to_pool_id_sdt[uc_subpool_id] == pst_netbuf->truesize) {
            *puc_subpool_id = uc_subpool_id;

            return OAL_SUCC;
        }
    }

    return OAL_FAIL;
}


oal_uint32  oal_mem_sdt_netbuf_free(oal_netbuf_stru *pst_netbuf, oal_uint8 uc_lock)
{
    /* liteos sdt专用内存后续优化，暂用netbuf公用内存池 */
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_uint8               uc_subpool_id;
    oal_mem_pool_stru      *pst_mem_pool;
    oal_mem_subpool_stru   *pst_mem_subpool;
    oal_uint32              ul_ret;
    oal_ulong                ul_irq_flag = 0;

    if (OAL_UNLIKELY(pst_netbuf == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mem_pool = &g_ast_mem_pool[OAL_MEM_POOL_ID_SDT_NETBUF];

    /* 获取该netbuf内存所属的子内存池ID */
    ul_ret = oal_mem_find_sdt_netbuf_subpool_id(pst_netbuf, pst_mem_pool, &uc_subpool_id);
    if (OAL_UNLIKELY(ul_ret != OAL_SUCC)) {
        oal_netbuf_free(pst_netbuf);
        return OAL_ERR_CODE_OAL_MEM_SKB_SUBPOOL_ID_ERR;
    }

    pst_mem_subpool = &(pst_mem_pool->ast_subpool_table[uc_subpool_id]);

    OAL_MEM_SPIN_LOCK_IRQSAVE(pst_mem_subpool->st_spinlock, ul_irq_flag)

    /* 异常: 该子内存池可用内存块数目超过整个子内存池总内存块数 */
    if (OAL_UNLIKELY(pst_mem_subpool->us_free_cnt >= pst_mem_subpool->us_total_cnt)) {
        OAL_MEM_SPIN_UNLOCK_IRQRESTORE(pst_mem_subpool->st_spinlock, ul_irq_flag)

        return OAL_ERR_CODE_OAL_MEM_EXCEED_TOTAL_CNT;
    }

    /* 将该netbuf内存归还给相应的子内存池 */
    pst_mem_subpool->ppst_free_stack[pst_mem_subpool->us_free_cnt] = (void *)pst_netbuf;
    pst_mem_subpool->us_free_cnt++;

    pst_mem_pool->us_mem_used_cnt--;
    oal_netbuf_free(pst_netbuf);
    OAL_MEM_SPIN_UNLOCK_IRQRESTORE(pst_mem_subpool->st_spinlock, ul_irq_flag)

#elif(_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    oal_netbuf_free(pst_netbuf);
#endif

    return OAL_SUCC;
}


oal_uint32  oal_mempool_info_to_sdt_register(oal_stats_info_up_to_sdt  p_up_mempool_info,
                                             oal_memblock_info_up_to_sdt p_up_memblock_info)
{
    g_st_mempool_info.p_mempool_info_func = p_up_mempool_info;
    g_st_mempool_info.p_memblock_info_func = p_up_memblock_info;

    return OAL_SUCC;
}


oal_void  oal_mem_info(oal_mem_pool_id_enum_uint8 en_pool_id)
{
    oal_mem_pool_stru      *pst_mem_pool;
    oal_mem_subpool_stru   *pst_mem_subpool;
    oal_uint8               uc_subpool_id;

    pst_mem_pool = oal_mem_get_pool(en_pool_id);
    if (OAL_UNLIKELY(pst_mem_pool == OAL_PTR_NULL)) {
        return;
    }

    for (uc_subpool_id = 0; uc_subpool_id < pst_mem_pool->uc_subpool_cnt; uc_subpool_id++) {
        pst_mem_subpool = &(pst_mem_pool->ast_subpool_table[uc_subpool_id]);

        /* 子池使用情况，oal_mem_leak函数中会上报每个内存块的信息 */
        if (g_st_mempool_info.p_mempool_info_func != OAL_PTR_NULL) {
            g_st_mempool_info.p_mempool_info_func(en_pool_id,
                                                  pst_mem_pool->us_mem_total_cnt,
                                                  pst_mem_pool->us_mem_used_cnt,
                                                  uc_subpool_id,
                                                  pst_mem_subpool->us_total_cnt,
                                                  pst_mem_subpool->us_free_cnt);
        }
    }
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

oal_uint8 oal_get_func_name(oal_uint8 *buff, oal_ulong call_func_ddr)
{
    oal_uint8 buf_cnt;

#if (_PRE_OS_VERSION == _PRE_OS_VERSION_LINUX)
    buf_cnt = sprint_symbol((int8_t *)buff, call_func_ddr);
#else
    buf_cnt = 0;
    *buff = '\0';
#endif

    return buf_cnt;
}


oal_ulong oal_get_func_return_address(oal_void)
{
    oal_ulong ul_ret_addr;

#if (_PRE_OS_VERSION == _PRE_OS_VERSION_LINUX)
    ul_ret_addr = (uintptr_t)__builtin_return_address(0); //lint !e571
#else
    ul_ret_addr = 0;
#endif

    return ul_ret_addr;
}
oal_module_symbol(oal_get_func_return_address);


oal_void oal_mem_print_funcname(oal_ulong func_addr)
{
    oal_uint8              ac_buff[OAL_MEM_SPRINT_SYMBOL_SIZE] = {0};
    oal_uint8              ac_buff_head[100] = {0};
    oal_uint8              uc_size;

    uc_size = oal_get_func_name(ac_buff, func_addr);
    /* OTA打印函数符号 */
    if (snprintf_s((int8_t *)ac_buff_head, sizeof(ac_buff_head), sizeof(ac_buff_head) - 1, "Func: ") < EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "oal_mem_print_funcname::snprintf_s failed !");
        return;
    }
    if (strncat_s((int8_t *)ac_buff_head, sizeof(ac_buff_head), (int8_t *)ac_buff, uc_size) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "oal_mem_print_funcname::strncat_s failed !");
        return;
    }
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oam_print((int8_t *)ac_buff_head);
#endif
}
oal_module_symbol(oal_mem_print_funcname);


oal_uint32 oal_mem_return_addr_count(oal_mem_subpool_stru *pst_mem_subpool, oal_mem_stru *pst_mem_base,
                                     oal_ulong call_func_addr)
{
    oal_uint16             us_loop;
    oal_mem_stru          *pst_mem;
    oal_uint32             us_count = 0;

    pst_mem = pst_mem_base;

    for (us_loop = 0; us_loop < pst_mem_subpool->us_total_cnt; us_loop++) {
        if ((pst_mem->en_mem_state_flag == OAL_MEM_STATE_ALLOC) && (call_func_addr == pst_mem->ul_return_addr)) {
            us_count++;
        }
        pst_mem++;
    }

    return us_count;
}


oal_uint8 oal_mem_func_addr_is_registerd(const oal_ulong* ua_func_addr, oal_uint8 uc_func_size,
                                         oal_uint8* p_func_loop, oal_ulong call_func_addr)
{
    oal_uint8 uc_loop = 0;

    /* 非零则判断是否函数地址相同 */
    while (ua_func_addr[uc_loop]) {
        /* 如果存在则返回true */
        if (ua_func_addr[uc_loop] == call_func_addr) {
            return OAL_TRUE;
        }

        uc_loop++;
        if (uc_func_size == uc_loop) {
            break;
        }
    }

    if (uc_func_size == uc_loop) {
        uc_loop = 0;
    }

    /* 不存在则需要记录数组下标 */
    *p_func_loop = uc_loop;

    return OAL_FALSE;
}


oal_void oal_mem_print_normal_pool_info(oal_mem_pool_id_enum_uint8 en_pool_id)
{
    oal_mem_pool_stru     *pst_mem_pool;
    oal_mem_subpool_stru  *pst_mem_subpool = NULL;
    oal_mem_stru          *pst_mem;
    oal_mem_stru          *pst_mem_base;
    oal_uint16             us_loop;
    oal_uint8              uc_subpool_id;
    oal_ulong              ua_func_addr[50] = {0};
    oal_uint8              uc_func_size = 50;
    oal_uint8              us_func_loop = 0;
    oal_uint32             us_ret_count;
    oal_uint8              ac_buff[OAL_MEM_SPRINT_SYMBOL_SIZE] = {0};
    oal_uint8              ac_buff_head[200] = {0};
    oal_uint8              uc_size;

    pst_mem_pool = oal_mem_get_pool(en_pool_id);
    if (OAL_UNLIKELY(pst_mem_pool == OAL_PTR_NULL)) {return;}
    OAM_WARNING_LOG4(0, OAM_SF_CFG, "{host memory info:pool id=%d,subpool cnt=%d,mem block total cnt=%d,used cnt=%d.}",
        en_pool_id, pst_mem_pool->uc_subpool_cnt, pst_mem_pool->us_mem_total_cnt, pst_mem_pool->us_mem_used_cnt);
    pst_mem = pst_mem_pool->pst_mem_start_addr;
    OAL_REFERENCE(pst_mem_subpool);
    /* 循环每一个子池 */
    for (uc_subpool_id = 0; uc_subpool_id < pst_mem_pool->uc_subpool_cnt; uc_subpool_id++) {
        /* 得到每一级子内存池信息 */
        pst_mem_subpool = &(pst_mem_pool->ast_subpool_table[uc_subpool_id]);
        OAM_WARNING_LOG4(0, OAM_SF_CFG, "{host subpool id[%d] info: len =%d, mem block total cnt=%d, free cnt=%d.}",
            uc_subpool_id, pst_mem_subpool->us_len, pst_mem_subpool->us_total_cnt, pst_mem_subpool->us_free_cnt);
        /* 保存新子池基地址 */
        pst_mem_base = pst_mem;
        /* 循环查询每一个子池的mem block */
        for (us_loop = 0; us_loop < pst_mem_subpool->us_total_cnt; us_loop++) {
            if ((pst_mem->ul_return_addr == 0) && (pst_mem->en_mem_state_flag == OAL_MEM_STATE_ALLOC)) {
                OAM_WARNING_LOG2(0, OAM_SF_CFG,
                                 "{oal_mem_print_normal_pool_info::subpool id[%d] mem block[%d]has no call func addr}",
                                 uc_subpool_id, us_loop);
                /* 查询下一个内存块 */
            } else if ((pst_mem->en_mem_state_flag == OAL_MEM_STATE_ALLOC) && (oal_mem_func_addr_is_registerd(
                ua_func_addr, uc_func_size, &us_func_loop, pst_mem->ul_return_addr) == OAL_FALSE)) {
                ua_func_addr[us_func_loop] = pst_mem->ul_return_addr;
                us_ret_count = oal_mem_return_addr_count(pst_mem_subpool, pst_mem_base, pst_mem->ul_return_addr);
                uc_size = oal_get_func_name(ac_buff, pst_mem->ul_return_addr);
                /* OTA打印函数符号 */
                if (snprintf_s((int8_t *)ac_buff_head, sizeof(ac_buff_head), sizeof(ac_buff_head) - 1,
                               "[%d] mem blocks occupied by ", us_ret_count) < EOK) {
                    OAM_ERROR_LOG0(0, OAM_SF_CFG, "oal_mem_print_normal_pool_info::snprintf_s failed!");
                }
                if (strncat_s((int8_t *)ac_buff_head, sizeof(ac_buff_head), (int8_t *)ac_buff, uc_size) != EOK) {
                    OAM_ERROR_LOG0(0, OAM_SF_CFG, "oal_mem_print_normal_pool_info::strncat_s failed!");
                }
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
                oam_print((int8_t *)ac_buff_head);
#endif
            }
            /* 已注册，则查询下一个内存块 */
            pst_mem++;
        }
        /* 查询完一个子池，清空数组 */
        memset_s(ua_func_addr, uc_func_size, 0, uc_func_size);
    }
}
oal_module_symbol(oal_mem_print_normal_pool_info);


oal_void oal_mem_print_pool_info(oal_void)
{
    oal_uint8 uc_loop;

    for (uc_loop = 0; uc_loop <= OAL_MEM_POOL_ID_SHARED_DSCR; uc_loop++) {
        oal_mem_print_normal_pool_info(uc_loop);
    }
}
oal_module_symbol(oal_mem_print_pool_info);

#endif


oal_void  oal_mem_leak(oal_mem_pool_id_enum_uint8 en_pool_id)
{
#ifdef _PRE_DEBUG_MODE
    oal_mem_pool_stru     *pst_mem_pool;
    oal_mem_stru          *pst_mem;
    oal_bool_enum_uint8    en_flag = OAL_TRUE;
    oal_uint16             us_loop;

    pst_mem_pool = oal_mem_get_pool(en_pool_id);
    if (OAL_UNLIKELY(pst_mem_pool == OAL_PTR_NULL)) {
        return;
    }

    pst_mem = pst_mem_pool->pst_mem_start_addr;

    for (us_loop = 0; us_loop < pst_mem_pool->us_mem_total_cnt; us_loop++) {
        /* 如果行号 != 0，则说明有内存没有被释放(不可能在第0行申请内存) */
        if (pst_mem->ul_alloc_line_num[0] != 0) {
            /* 打印当前时间戳 */
            if (en_flag == OAL_TRUE) {
                OAL_IO_PRINT("[memory leak] current time stamp: %u.\n", (oal_uint32)OAL_TIME_GET_STAMP_MS());
                en_flag = OAL_FALSE;
            }

            OAL_IO_PRINT("[memory leak] user_cnt: %u, pool_id: %u, subpool_id: %u, len: %u, "
                         "alloc_core_id = %u, alloc_file_id: %u, alloc_line_num: %u, alloc_time_stamp: %u, "
                         "trace_file_id: %u, trace_line_num: %u, trace_time_stamp: %u.\n",
                         pst_mem->uc_user_cnt,
                         pst_mem->en_pool_id,
                         pst_mem->uc_subpool_id,
                         pst_mem->us_len,
                         pst_mem->ul_alloc_core_id[0],
                         pst_mem->ul_alloc_file_id[0],
                         pst_mem->ul_alloc_line_num[0],
                         pst_mem->ul_alloc_time_stamp[0],
                         pst_mem->ul_trace_file_id,
                         pst_mem->ul_trace_line_num,
                         pst_mem->ul_trace_time_stamp);
        }

        /* 每个内存块的信息，在oal_mem_info中会上报每个子池的信息 zouhongliang SDT */
        if (g_st_mempool_info.p_memblock_info_func != OAL_PTR_NULL) {
            g_st_mempool_info.p_memblock_info_func(pst_mem->puc_origin_data,
                                                   pst_mem->uc_user_cnt,
                                                   pst_mem->en_pool_id,
                                                   pst_mem->uc_subpool_id,
                                                   pst_mem->us_len,
                                                   pst_mem->ul_alloc_file_id[0],
                                                   pst_mem->ul_alloc_line_num[0]);
        }

        pst_mem++;
    }
#endif
}


OAL_STATIC oal_void  oal_mem_statistics(oal_mem_pool_stat * past_mem_pool_stat)
{
    oal_mem_pool_id_enum_uint8    en_pool_id;
    oal_uint8                     uc_subpool_id;
    oal_mem_pool_stru            *pst_mem_pool;
    oal_mem_subpool_stru         *pst_mem_subpool;

    for (en_pool_id = 0; en_pool_id < OAL_MEM_POOL_ID_BUTT; en_pool_id++) {
        pst_mem_pool = &g_ast_mem_pool[en_pool_id];

        /* 记录本内存池使用总量 */
        past_mem_pool_stat[en_pool_id].us_mem_used_cnt  = pst_mem_pool->us_mem_used_cnt;
        past_mem_pool_stat[en_pool_id].us_mem_total_cnt = pst_mem_pool->us_mem_total_cnt;

        /* 记录各子池使用状况 */
        for (uc_subpool_id = 0; uc_subpool_id < pst_mem_pool->uc_subpool_cnt; uc_subpool_id++) {
            pst_mem_subpool = &(pst_mem_pool->ast_subpool_table[uc_subpool_id]);

            past_mem_pool_stat[en_pool_id].ast_subpool_stat[uc_subpool_id].us_free_cnt  = pst_mem_subpool->us_free_cnt;
            past_mem_pool_stat[en_pool_id].ast_subpool_stat[uc_subpool_id].us_total_cnt = pst_mem_subpool->us_total_cnt;
        }
    }
}


OAL_STATIC oal_uint32  oal_mem_check(oal_mem_pool_stat * past_stat_start, oal_mem_pool_stat * past_stat_end)
{
    oal_uint8                     uc_bitmap = 0;
    oal_mem_pool_id_enum_uint8    en_pool_id;
    oal_uint8                     uc_subpool_id;
    oal_mem_pool_stru            *pst_mem_pool;

    for (en_pool_id = 0; en_pool_id < OAL_MEM_POOL_ID_BUTT; en_pool_id++) {
        /* 查看本内存池总量(两次统计结果是否一致) */
        if ((past_stat_start[en_pool_id].us_mem_used_cnt != past_stat_end[en_pool_id].us_mem_used_cnt) ||
            (past_stat_start[en_pool_id].us_mem_total_cnt != past_stat_end[en_pool_id].us_mem_total_cnt)) {
            uc_bitmap |= (oal_uint8)(1 << en_pool_id);
            continue;
        }

        pst_mem_pool = &g_ast_mem_pool[en_pool_id];

        /* 查看各子内存池使用状况(两次统计结果是否一致) */
        for (uc_subpool_id = 0; uc_subpool_id < pst_mem_pool->uc_subpool_cnt; uc_subpool_id++) {
            if ((past_stat_start[en_pool_id].ast_subpool_stat[uc_subpool_id].us_free_cnt !=
                 past_stat_end[en_pool_id].ast_subpool_stat[uc_subpool_id].us_free_cnt)       ||
                (past_stat_start[en_pool_id].ast_subpool_stat[uc_subpool_id].us_total_cnt !=
                 past_stat_end[en_pool_id].ast_subpool_stat[uc_subpool_id].us_total_cnt)) {
                uc_bitmap |= (oal_uint8)(1 << en_pool_id);
                continue;
            }
        }
    }

    /* 两次统计结果一致(没有内存泄漏)，返回 */
    if (uc_bitmap == 0) {
        OAL_IO_PRINT("no memory leak!\n");
        return OAL_FALSE;
    }

    /* 两次统计结果不一致(有内存泄漏)，打印有泄漏的内存池的统计信息 */
    OAL_IO_PRINT("memory leak!\n");
    for (en_pool_id = 0; en_pool_id < OAL_MEM_POOL_ID_BUTT; en_pool_id++) {
        if (!(uc_bitmap & (1 << en_pool_id))) {
            continue;
        }

        pst_mem_pool = &g_ast_mem_pool[en_pool_id];

        OAL_IO_PRINT("                      Start\t\tEnd\t\n");
        OAL_IO_PRINT("pool(%d) used cnt:     %d\t\t%d\t\n", en_pool_id, past_stat_start[en_pool_id].us_mem_used_cnt,
                     past_stat_end[en_pool_id].us_mem_used_cnt);
        OAL_IO_PRINT("pool(%d) total cnt:    %d\t\t%d\t\n", en_pool_id, past_stat_start[en_pool_id].us_mem_total_cnt,
                     past_stat_end[en_pool_id].us_mem_total_cnt);
        for (uc_subpool_id = 0; uc_subpool_id < pst_mem_pool->uc_subpool_cnt; uc_subpool_id++) {
            OAL_IO_PRINT("subpool(%d) free cnt:  %d\t\t%d\t\n", uc_subpool_id,
                         past_stat_start[en_pool_id].ast_subpool_stat[uc_subpool_id].us_free_cnt,
                         past_stat_end[en_pool_id].ast_subpool_stat[uc_subpool_id].us_free_cnt);
            OAL_IO_PRINT("subpool(%d) total cnt: %d\t\t%d\t\n", uc_subpool_id,
                         past_stat_start[en_pool_id].ast_subpool_stat[uc_subpool_id].us_total_cnt,
                         past_stat_end[en_pool_id].ast_subpool_stat[uc_subpool_id].us_total_cnt);
        }

        OAL_IO_PRINT("\n");
    }

    return OAL_TRUE;
}


oal_void  oal_mem_start_stat(oal_void)
{
    memset_s(&g_st_mem_stat, OAL_SIZEOF(g_st_mem_stat), 0, OAL_SIZEOF(g_st_mem_stat));

    /* 记录各内存池使用状况 */
    oal_mem_statistics(g_st_mem_stat.ast_mem_start_stat);
}


oal_uint32  oal_mem_end_stat(oal_void)
{
    oal_mem_statistics(g_st_mem_stat.ast_mem_end_stat);

    /* 检测内存池是否有泄漏 */
    return oal_mem_check(g_st_mem_stat.ast_mem_start_stat, g_st_mem_stat.ast_mem_end_stat);
}


oal_uint32  oal_mem_trace_enhanced(oal_uint32      ul_file_id,
                                   oal_uint32      ul_line_num,
                                   oal_mem_stru   *pst_mem,
                                   oal_uint8       uc_lock)
{
    oal_mem_pool_stru      *pst_mem_pool;
    oal_mem_subpool_stru   *pst_mem_subpool;

    if (OAL_UNLIKELY(pst_mem == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mem_pool = &g_ast_mem_pool[pst_mem->en_pool_id];

    pst_mem_subpool = &(pst_mem_pool->ast_subpool_table[pst_mem->uc_subpool_id]);

#ifdef _PRE_DEBUG_MODE
    OAL_MEM_SPIN_LOCK_BH(pst_mem_subpool->st_spinlock)

    pst_mem->ul_trace_file_id    = ul_file_id;
    pst_mem->ul_trace_line_num   = ul_line_num;
    pst_mem->ul_trace_time_stamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    OAL_MEM_SPIN_UNLOCK_BH(pst_mem_subpool->st_spinlock)
#endif

    return OAL_SUCC;
}


oal_uint32  oal_mem_trace(oal_uint32    ul_file_id,
                          oal_uint32    ul_line_num,
                          oal_void     *p_data,
                          oal_uint8     uc_lock)
{
    oal_mem_stru   *pst_mem;

    if (OAL_UNLIKELY(p_data == OAL_PTR_NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mem = (oal_mem_stru *)(*((uintptr_t *)((uintptr_t)p_data - OAL_MEM_INFO_SIZE)));

    return oal_mem_trace_enhanced(ul_file_id, ul_line_num, pst_mem, uc_lock);
}


oal_uint32  oal_mem_exit(oal_void)
{
    /* 卸载普通内存池 */
    oal_mem_release();

    /* 卸载netbuf内存池 */
    oal_mem_sdt_netbuf_release();

    memset_s(g_ast_mem_pool, OAL_SIZEOF(g_ast_mem_pool), 0, OAL_SIZEOF(g_ast_mem_pool));

    return OAL_SUCC;
}

/*lint -e19*/
oal_module_symbol(oal_mem_free);
oal_module_symbol(oal_mem_alloc);

/*lint -e19*/
oal_module_symbol(oal_mem_alloc_enhanced);
oal_module_symbol(oal_mem_free_enhanced);
oal_module_symbol(oal_mem_sdt_netbuf_alloc);
oal_module_symbol(oal_mem_sdt_netbuf_free);
oal_module_symbol(oal_mem_leak);
oal_module_symbol(oal_mem_info);
oal_module_symbol(oal_mem_trace_enhanced);
oal_module_symbol(oal_mem_trace);
oal_module_symbol(oal_mempool_info_to_sdt_register);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
oal_module_symbol(g_pst_sdt_netbuf_base_addr);
#endif
#ifdef _PRE_DEBUG_MODE
oal_module_symbol(g_st_tx_dscr_addr);
oal_module_symbol(oal_mem_get_tx_dscr_addr);
oal_module_symbol(oal_mem_stop_rcd_rls);
oal_module_symbol(oal_mem_get_stop_flag);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

