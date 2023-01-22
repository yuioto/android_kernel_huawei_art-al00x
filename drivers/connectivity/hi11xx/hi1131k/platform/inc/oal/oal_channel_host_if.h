

#ifndef __OAL_CHANNEL_HOST_IF_H__
#define __OAL_CHANNEL_HOST_IF_H__

#if (_PRE_FEATURE_SDIO == _PRE_FEATURE_CHANNEL_TYPE)
#include "oal_sdio_host_if.h"
typedef struct oal_sdio   oal_channel_stru;
#define oal_get_channel_default_handler()     oal_get_sdio_default_handler()
#define oal_channel_wake_lock(pst_channel)     oal_sdio_wake_lock(pst_channel)
#define oal_channel_wake_unlock(pst_channel)     oal_sdio_wake_unlock(pst_channel)
#define oal_channel_message_register(pst_channel, msg, cb, data)    oal_sdio_message_register(pst_channel, msg, cb, data)
#define oal_channel_message_unregister(pst_channel, msg)    oal_sdio_message_unregister(pst_channel, msg)
#define oal_channel_patch_readsb(buf, len, ms_timeout)  sdio_patch_readsb(buf, len, ms_timeout)
#define oal_channel_patch_writesb(buf, len)  sdio_patch_writesb(buf, len)
#define oal_channel_claim_host(pst_channel)     oal_sdio_claim_host(pst_channel)
#define oal_channel_release_host(pst_channel)     oal_sdio_release_host(pst_channel)
#define oal_channel_send_msg(pst_channel, val)     oal_sdio_send_msg(pst_channel, val)
#define oal_disable_channel_state(pst_channel, mask)     oal_disable_sdio_state(pst_channel, mask)
#define oal_channel_rx_transfer_lock(pst_channel)     oal_sdio_rx_transfer_lock(pst_channel)
#define oal_channel_rx_transfer_unlock(pst_channel)     oal_sdio_rx_transfer_unlock(pst_channel)
#define oal_channel_init_module(pdata)     oal_sdio_init_module(pdata)
#define oal_channel_exit_module(pst_channel)     oal_sdio_exit_module(pst_channel)
#define oal_channel_transfer_rx_register(pst_channel, rx_handler) oal_sdio_transfer_rx_register(pst_channel, rx_handler)
#define oal_channel_transfer_rx_unregister(pst_channel)     oal_sdio_transfer_rx_unregister(pst_channel)
#define oal_channel_func_probe(pst_channel)     oal_sdio_func_probe(pst_channel)
#define oal_channel_func_remove(pst_channel)     oal_sdio_func_remove(pst_channel)
#define oal_channel_transfer_prepare(pst_channel)   oal_sdio_transfer_prepare(pst_channel)
#endif
#endif

