/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-01-15     WennianYan   first version
 */

#ifndef __TCA9548A_H__
#define __TCA9548A_H__

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TCA9548A (PW548A) I2C address range */
#define TCA9548A_ADDR_MIN         0x70
#define TCA9548A_ADDR_MAX         0x77
#define TCA9548A_ADDR_DEFAULT     0x70

/* Number of channels */
#define TCA9548A_CHANNEL_MAX      8

/* Maximum number of devices supported in auto-init */
#define TCA9548A_MAX_DEVICES      8

/* Control register bit masks */
#define TCA9548A_CHANNEL_0        (1 << 0)
#define TCA9548A_CHANNEL_1        (1 << 1)
#define TCA9548A_CHANNEL_2        (1 << 2)
#define TCA9548A_CHANNEL_3        (1 << 3)
#define TCA9548A_CHANNEL_4        (1 << 4)
#define TCA9548A_CHANNEL_5        (1 << 5)
#define TCA9548A_CHANNEL_6        (1 << 6)
#define TCA9548A_CHANNEL_7        (1 << 7)
#define TCA9548A_CHANNEL_ALL      0xFF
#define TCA9548A_CHANNEL_NONE     0x00

/* Error codes */
#define TCA9548A_EOK              0
#define TCA9548A_ERROR            (-1)
#define TCA9548A_EINVAL           (-2)
#define TCA9548A_ETIMEOUT         (-3)
#define TCA9548A_EIO              (-4)

/* Device structure */
struct tca9548a_device
{
    struct rt_i2c_bus_device *i2c_bus;
    rt_uint8_t  addr;
    rt_uint8_t  current_channel;
    rt_mutex_t  lock;
    char        name[RT_NAME_MAX];
};

typedef struct tca9548a_device *tca9548a_dev_t;

/**
 * Initialize the TCA9548A (PW548A) device.
 *
 * @param i2c_bus_name   The I2C bus device name (e.g. "i2c1")
 * @param addr           The 7-bit I2C address of the device (0x70-0x77)
 * @param name           Device name for identification
 *
 * @return  Device handle on success, RT_NULL on failure
 */
tca9548a_dev_t tca9548a_init(const char *i2c_bus_name, rt_uint8_t addr,
                             const char *name);

/**
 * Deinitialize the TCA9548A device and release resources.
 *
 * @param dev  Device handle
 *
 * @return  TCA9548A_EOK on success, negative error code on failure
 */
rt_err_t tca9548a_deinit(tca9548a_dev_t dev);

/**
 * Find a TCA9548A device by its name.
 *
 * @param name  Device name
 *
 * @return  Device handle on success, RT_NULL if not found
 */
tca9548a_dev_t tca9548a_find(const char *name);

/**
 * Select one or more channels on the TCA9548A.
 *
 * @param dev       Device handle
 * @param channels  Bit mask of channels to select
 *
 * @return  TCA9548A_EOK on success, negative error code on failure
 */
rt_err_t tca9548a_select_channel(tca9548a_dev_t dev, rt_uint8_t channels);

/**
 * Read the current channel selection state from the device.
 *
 * @param dev       Device handle
 * @param channels  Output: current channel selection bit mask
 *
 * @return  TCA9548A_EOK on success, negative error code on failure
 */
rt_err_t tca9548a_read_channel(tca9548a_dev_t dev, rt_uint8_t *channels);

/**
 * Enable a single channel.
 *
 * @param dev      Device handle
 * @param channel  Channel number (0-7)
 *
 * @return  TCA9548A_EOK on success, negative error code on failure
 */
rt_err_t tca9548a_enable_channel(tca9548a_dev_t dev, rt_uint8_t channel);

/**
 * Disable a single channel.
 *
 * @param dev      Device handle
 * @param channel  Channel number (0-7)
 *
 * @return  TCA9548A_EOK on success, negative error code on failure
 */
rt_err_t tca9548a_disable_channel(tca9548a_dev_t dev, rt_uint8_t channel);

/**
 * Disable all channels.
 *
 * @param dev  Device handle
 *
 * @return  TCA9548A_EOK on success, negative error code on failure
 */
rt_err_t tca9548a_disable_all(tca9548a_dev_t dev);

/**
 * Enable all channels.
 *
 * @param dev  Device handle
 *
 * @return  TCA9548A_EOK on success, negative error code on failure
 */
rt_err_t tca9548a_enable_all(tca9548a_dev_t dev);

/**
 * Reset the TCA9548A device (disable all channels).
 *
 * @param dev  Device handle
 *
 * @return  TCA9548A_EOK on success, negative error code on failure
 */
rt_err_t tca9548a_reset(tca9548a_dev_t dev);

/**
 * Check if the TCA9548A device is accessible on the I2C bus.
 *
 * @param dev  Device handle
 *
 * @return  RT_TRUE if accessible, RT_FALSE otherwise
 */
rt_bool_t tca9548a_is_connected(tca9548a_dev_t dev);

#ifdef __cplusplus
}
#endif

#endif /* __TCA9548A_H__ */