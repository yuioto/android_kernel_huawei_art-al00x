#ifdef CONFIG_AP_CUST_2G_MEM
#include "global_ddr_map_2g.h"
#else
#ifdef CONFIG_ANDROID_Q_LAUNCHED
#include "global_ddr_map_common.h"
#else
#include "global_ddr_map_legacy.h"
#endif
#endif
