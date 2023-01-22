

#ifndef __PLAT_SDIO_H__
#define __PLAT_SDIO_H__

#include "plat_type.h"

extern oal_int32 sdio_patch_writesb(oal_uint8* buf, oal_uint32 len);
extern oal_int32 sdio_patch_readsb(oal_uint8* buf, oal_uint32 len, oal_uint32 timeout);

#endif

