

#ifndef __PLAT_DEBUG_H__
#define __PLAT_DEBUG_H__

/*****************************************************************************
  1 Include other Head file
*****************************************************************************/
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#include "plat_type.h"
#endif

#include "chr_user.h"
#include "oal_types.h"
#include "oal_util.h"
#include "oal_mem.h"

/*****************************************************************************
  2 Define macro
*****************************************************************************/
enum PLAT_LOGLEVLE {
    PLAT_LOG_ALERT = 0,
    PLAT_LOG_ERR = 1,
    PLAT_LOG_WARNING = 2,
    PLAT_LOG_INFO = 3,
    PLAT_LOG_DEBUG = 4,
};

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)

#define KBUILD_MODNAME "1131 for compile" /* ´ò×® */

enum BUG_ON_CTRL {
    BUG_ON_DISABLE = 0,
    BUG_ON_ENABLE  = 1,
};

extern oal_uint32 g_plat_loglevel;
static oal_int32 g_bug_on_enable = BUG_ON_DISABLE;

#define PS_PRINT_FUNCTION_NAME

#define PS_PRINT_DBG(s, args...)

#if (_HI113X_SW_VERSION == _HI113X_SW_RELEASE)
#define PS_PRINT_INFO(s, args...)
#elif (_HI113X_SW_VERSION == _HI113X_SW_DEBUG)
#define PS_PRINT_INFO(s, args...)           do{ \
        if (PLAT_LOG_INFO <= g_plat_loglevel) \
        { \
            printf("[%s]" s,__func__, ## args);\
        } \
    }while (0)
#endif

#define PS_PRINT_WARNING(s, args...)        do{ \
        if (PLAT_LOG_WARNING <= g_plat_loglevel) \
        { \
            printf("[%s]" s,__func__, ## args);\
        } \
    }while (0)

#define PS_PRINT_ERR(s, args...)            do{ \
        if (PLAT_LOG_ERR <= g_plat_loglevel) \
        { \
            printf("[%s]" s,__func__, ## args); \
        } \
    }while (0)

#define PS_PRINT_ALERT(s, args...)          do{ \
        if (PLAT_LOG_ALERT <= g_plat_loglevel) \
        { \
            printf("[%s]" s,__func__, ## args); \
        } \
    }while (0)

#define PS_BUG_ON(s)                       do{ \
        if((BUG_ON_ENABLE == g_bug_on_enable)) \
        { \
            BUG_ON(s);\
        } \
    }while (0)
#elif (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
extern int g_plat_loglevel;

/*****************************************************************************
  2 Define macro
*****************************************************************************/
#define PS_PRINT_FUNCTION_NAME              do { \
        if (PLAT_LOG_DEBUG <= g_plat_loglevel) \
        { \
            printk(KERN_DEBUG KBUILD_MODNAME ":D]%s]" ,__func__);     \
        } \
    }while (0)

#define PS_PRINT_DBG(s, args...)            do{ \
        if (PLAT_LOG_DEBUG <= g_plat_loglevel) \
        { \
            printk(KERN_DEBUG KBUILD_MODNAME ":D]%s]" s,__func__, ## args); \
        }\
    }while (0)

#define PS_PRINT_INFO(s, args...)           do{ \
        if (PLAT_LOG_INFO <= g_plat_loglevel) \
        { \
            printk(KERN_DEBUG KBUILD_MODNAME ":I]%s]" s,__func__, ## args);\
        } \
    }while (0)

#define PS_PRINT_SUC(s, args...)            do{ \
        if (PLAT_LOG_INFO <= g_plat_loglevel) \
        { \
            printk(KERN_DEBUG KBUILD_MODNAME ":S]%s]" s,__func__, ## args); \
        } \
    }while (0)

#define PS_PRINT_WARNING(s, args...)        do{ \
        if (PLAT_LOG_WARNING <= g_plat_loglevel) \
        { \
            printk(KERN_WARNING KBUILD_MODNAME ":W]%s]" s,__func__, ## args);\
        } \
    }while (0)

#define PS_PRINT_ERR(s, args...)            do{ \
        if (PLAT_LOG_ERR <= g_plat_loglevel) \
        { \
            printk(KERN_ERR KBUILD_MODNAME ":E]%s]" s,__func__, ## args); \
        } \
    }while (0)

#define PS_PRINT_ALERT(s, args...)          do{ \
        if (PLAT_LOG_ALERT <= g_plat_loglevel) \
        { \
            printk(KERN_ALERT KBUILD_MODNAME ":ALERT]%s]" s,__func__, ## args); \
        } \
    }while (0)

#define PS_BUG_ON(s)                       do{ \
        if((BUG_ON_ENABLE == g_bug_on_enable)) \
        { \
            BUG_ON(s);\
        } \
    }while (0)
#endif

#ifdef WIN32
#define PS_PRINT_INFO(s, ...)

#define PS_PRINT_WARNING(s, ...)

#define PS_PRINT_ERR(s, ...)
#endif

/*****************************************************************************
  3 STRUCT DEFINE
*****************************************************************************/

/*****************************************************************************
  4 EXTERN VARIABLE
*****************************************************************************/

/*****************************************************************************
  5 EXTERN FUNCTION
*****************************************************************************/

#endif /* PLAT_DEBUG_H */


