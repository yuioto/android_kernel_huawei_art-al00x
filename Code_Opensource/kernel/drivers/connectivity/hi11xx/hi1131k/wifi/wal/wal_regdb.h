

#ifndef __WAL_REGDB_H__
#define __WAL_REGDB_H__

#include "oal_ext_if.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_REGDB_H

extern OAL_CONST oal_ieee80211_regdomain_stru g_st_default_regdom;
extern OAL_CONST oal_ieee80211_regdomain_stru *wal_regdb_find_db(const oal_int8 *pc_str);

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
struct callback_head {
    struct callback_head *next;
    void (*func)(struct callback_head *head);
};

#define rcu_head callback_head

struct ieee80211_regdomain {
    struct rcu_head rcu_head;
    unsigned int n_reg_rules;
    char alpha2[2];
    unsigned char dfs_region;
    struct ieee80211_reg_rule reg_rules[];
};

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of wal_regdb.h */
