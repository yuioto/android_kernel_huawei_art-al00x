

#include "oal_sdio.h"
#include "oal_sdio_host_if.h"

#include "plat_debug.h"
#include "plat_sdio.h"
#include "plat_pm.h"
#include "plat_firmware.h"
#include "oal_time.h"

/*
 * Prototype    : sdio_patch_writesb
 * Description  : provide interface for pm driver
 * Input        : oal_uint8* buf, oal_uint32 len
 * Output       : None
 * Return Value : oal_int32
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         :
 *     Author       :
 *     Modification : Created function
 *
 */
oal_int32 sdio_patch_writesb(oal_uint8* buf, oal_uint32 len)
{
    int ret;
    struct oal_sdio* hi_sdio;
    struct sdio_func *func = OAL_PTR_NULL;

    hi_sdio = oal_get_sdio_default_handler();
    if (hi_sdio == NULL) {
        PS_PRINT_ERR("sdio module is NOT initialized\n");
        return -FAILURE;
    }

    func = hi_sdio->func;
    if (func == NULL) {
        PS_PRINT_ERR("Sdio device is NOT initialized\n");
        return -FAILURE;
    }

    if (buf == NULL || len == 0) {
        PS_PRINT_ERR("Write buf is NULL\n");
        return -FAILURE;
    }

    PS_PRINT_DBG("======sdio write:%u\n", len);

    len  = HISDIO_ALIGN_4_OR_BLK(len);

    sdio_claim_host(func);
    ret = oal_sdio_writesb(func, HISDIO_REG_FUNC1_FIFO, buf, len);
    if (ret < 0) {
        PS_PRINT_ERR("oal_sdio_writesb error:%d\n", ret);
    }
    sdio_release_host(func);
    return ret;
}

oal_int32 sdio_patch_readsb_parameter_judge(struct sdio_func *func, oal_uint8* buf, oal_uint32 len)
{
    if (func == NULL) {
        PS_PRINT_ERR("sdio device is NOT initialized\n");
        return -FAILURE;
    }

    if (buf == NULL) {
        PS_PRINT_ERR("Invalid buf read buf\n");
        return -FAILURE;
    }

    if (len == 0) {
        PS_PRINT_ERR("Invalid len read buf\n");
        return -FAILURE;
    }

    return SUCCESS;
}

static oal_int32 sdio_patch_read_fun1_state(struct sdio_func *func, oal_uint32 times)
{
    uint8_t     int_mask;
    int32_t     ret = 0;
    uint32_t    i;

    for (i = 0; i < times; i++) {
        int_mask = oal_sdio_readb(func, HISDIO_REG_FUNC1_INT_STATUS, &ret);
        if (ret) {
            return -FAILURE;
        }
        if (int_mask & HISDIO_FUNC1_INT_MASK) {
            break;
        }
        mdelay(READ_MEG_DELAY);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        cpu_relax();
#endif
    }
    if (i == times) {
        PS_PRINT_ERR("read int mask timeout, i=%d, times:%d", i, times);
        return -FAILURE;
    }

    return SUCCESS;
}

oal_int32 sdio_patch_read_write_sdio(struct sdio_func *func, oal_uint32 timeout)
{
    unsigned long timeout_jiffies;
    oal_uint8     int_mask;
    int           ret = 0;

    timeout_jiffies = OAL_TIME_JIFFY + OAL_MSECS_TO_JIFFIES(timeout);
    for (; ;) {
        int_mask = oal_sdio_readb(func, HISDIO_REG_FUNC1_INT_STATUS, &ret);
        if (ret) {
            PS_PRINT_ERR("read int mask fail, ret=%d", ret);
            sdio_release_host(func);
            return -FAILURE;
        }

        if (int_mask & HISDIO_FUNC1_INT_MASK) {
            break;
        }

        if (oal_time_after(OAL_TIME_JIFFY, timeout_jiffies)) {
            PS_PRINT_ERR("curr_jif:%lu, timeout_jif:%lu, timeout:%d",
                         OAL_TIME_JIFFY, timeout_jiffies, timeout);
            ret = sdio_patch_read_fun1_state(func, timeout);
            if (ret != SUCCESS) {
                sdio_release_host(func);
                return -FAILURE;
            }
            break;
        }
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        cpu_relax();
#endif
    }

    oal_sdio_writeb(func, int_mask, HISDIO_REG_FUNC1_INT_STATUS, &ret);
    if (ret < 0) {
        PS_PRINT_ERR("clear int mask error:%d", ret);
        sdio_release_host(func);
        return -FAILURE;
    }

    return SUCCESS;
}

/*
 * Prototype    : sdio_patch_readsb
 * Description  : provide interface for pm driver
 * Input        : oal_uint8* buf, oal_uint32 len oal_uint32 timeout (ms)
 * Output       : None
 * Return Value : oal_int32
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         :
 *     Author       :
 *     Modification : Created function
 *
 */
oal_int32 sdio_patch_readsb(oal_uint8* buf, oal_uint32 len, oal_uint32 timeout)
{
    int     ret;
    oal_uint32  xfer_count;
    struct oal_sdio* hi_sdio;
    struct sdio_func *func = OAL_PTR_NULL;

    hi_sdio = oal_get_sdio_default_handler();
    if (hi_sdio == NULL) {
        PS_PRINT_ERR("sdio module is NOT initialized\n");
        return -FAILURE;
    }

    func = hi_sdio->func;
    ret = sdio_patch_readsb_parameter_judge(func, buf, len);
    if (ret != SUCCESS) {
        return -FAILURE;
    }

    sdio_claim_host(func);

    ret = sdio_patch_read_write_sdio(func, timeout);
    if (ret != SUCCESS) {
        PS_PRINT_ERR("sdio_patch_read_write_sdio:%d\n", ret);
        return -FAILURE;
    }

    xfer_count = oal_sdio_readl(func, HISDIO_REG_FUNC1_XFER_COUNT, &ret);
    if (ret < 0) {
        sdio_release_host(func);
        PS_PRINT_ERR("read xfer count err:%d\n", ret);
        return -FAILURE;
    }

    if (xfer_count > len) {
        PS_PRINT_ERR("xfer_count:%d len:%d.\n", xfer_count, len);
        sdio_release_host(func);
        return -FAILURE;
    }

    ret = oal_sdio_readsb(func, buf, HISDIO_REG_FUNC1_FIFO, xfer_count);
    if (ret < 0) {
        PS_PRINT_ERR("hsdio_readsb error:%d\n", ret);
        return -FAILURE;
    }

    sdio_release_host(func);

    return xfer_count;
}

