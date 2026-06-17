/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-01-15     WennianYan   first version
 */

#include "tca9548a.h"

#define DBG_ENABLE
#define DBG_SECTION_NAME  "TCA9548A"
#define DBG_LEVEL         DBG_LOG
#include <rtdbg.h>

/* Device table for name-based lookup */
static tca9548a_dev_t _tca9548a_dev_table[TCA9548A_MAX_DEVICES] = {0};
static rt_uint8_t _tca9548a_dev_count = 0;

static rt_err_t tca9548a_write_register(tca9548a_dev_t dev, rt_uint8_t data)
{
    struct rt_i2c_msg msg;

    RT_ASSERT(dev != RT_NULL);

    msg.addr  = dev->addr;
    msg.flags = RT_I2C_WR;
    msg.buf   = &data;
    msg.len   = 1;

    if (rt_i2c_transfer(dev->i2c_bus, &msg, 1) != 1)
    {
        LOG_E("I2C write failed");
        return TCA9548A_EIO;
    }

    return TCA9548A_EOK;
}

static rt_err_t tca9548a_read_register(tca9548a_dev_t dev, rt_uint8_t *data)
{
    struct rt_i2c_msg msg;

    RT_ASSERT(dev != RT_NULL);
    RT_ASSERT(data != RT_NULL);

    msg.addr  = dev->addr;
    msg.flags = RT_I2C_RD;
    msg.buf   = data;
    msg.len   = 1;

    if (rt_i2c_transfer(dev->i2c_bus, &msg, 1) != 1)
    {
        LOG_E("I2C read failed");
        return TCA9548A_EIO;
    }

    return TCA9548A_EOK;
}

static rt_err_t tca9548a_register_device(tca9548a_dev_t dev)
{
    if (_tca9548a_dev_count >= TCA9548A_MAX_DEVICES)
    {
        LOG_E("Device table full, cannot register '%s'", dev->name);
        return TCA9548A_ERROR;
    }

    _tca9548a_dev_table[_tca9548a_dev_count++] = dev;
    return TCA9548A_EOK;
}

static void tca9548a_unregister_device(tca9548a_dev_t dev)
{
    rt_uint8_t i;

    for (i = 0; i < _tca9548a_dev_count; i++)
    {
        if (_tca9548a_dev_table[i] == dev)
        {
            _tca9548a_dev_table[i] = _tca9548a_dev_table[_tca9548a_dev_count - 1];
            _tca9548a_dev_table[_tca9548a_dev_count - 1] = RT_NULL;
            _tca9548a_dev_count--;
            return;
        }
    }
}

tca9548a_dev_t tca9548a_init(const char *i2c_bus_name, rt_uint8_t addr,
                             const char *name)
{
    tca9548a_dev_t dev = RT_NULL;

    RT_ASSERT(i2c_bus_name != RT_NULL);
    RT_ASSERT(name != RT_NULL);

    if (addr < TCA9548A_ADDR_MIN || addr > TCA9548A_ADDR_MAX)
    {
        LOG_E("Invalid I2C address: 0x%02X, valid range: 0x%02X-0x%02X",
              addr, TCA9548A_ADDR_MIN, TCA9548A_ADDR_MAX);
        return RT_NULL;
    }

    dev = (tca9548a_dev_t)rt_calloc(1, sizeof(struct tca9548a_device));
    if (dev == RT_NULL)
    {
        LOG_E("Failed to allocate memory for device '%s'", name);
        return RT_NULL;
    }

    dev->i2c_bus = (struct rt_i2c_bus_device *)rt_device_find(i2c_bus_name);
    if (dev->i2c_bus == RT_NULL)
    {
        LOG_E("I2C bus device '%s' not found", i2c_bus_name);
        rt_free(dev);
        return RT_NULL;
    }

    dev->addr = addr;
    dev->current_channel = 0;
    rt_strncpy(dev->name, name, RT_NAME_MAX);

    dev->lock = rt_mutex_create("tca9548a_lock", RT_IPC_FLAG_FIFO);
    if (dev->lock == RT_NULL)
    {
        LOG_E("Failed to create mutex for device '%s'", name);
        rt_free(dev);
        return RT_NULL;
    }

    /* Initialize: disable all channels */
    if (tca9548a_write_register(dev, TCA9548A_CHANNEL_NONE) != TCA9548A_EOK)
    {
        LOG_W("Failed to initialize device '%s'", name);
    }

    tca9548a_register_device(dev);

    LOG_I("TCA9548A '%s' initialized on I2C bus '%s', address 0x%02X",
          name, i2c_bus_name, addr);

    return dev;
}

rt_err_t tca9548a_deinit(tca9548a_dev_t dev)
{
    if (dev == RT_NULL)
    {
        return TCA9548A_EINVAL;
    }

    tca9548a_unregister_device(dev);
    tca9548a_write_register(dev, TCA9548A_CHANNEL_NONE);

    if (dev->lock != RT_NULL)
    {
        rt_mutex_delete(dev->lock);
        dev->lock = RT_NULL;
    }

    rt_free(dev);

    LOG_I("TCA9548A deinitialized");

    return TCA9548A_EOK;
}

tca9548a_dev_t tca9548a_find(const char *name)
{
    rt_uint8_t i;

    if (name == RT_NULL)
    {
        return RT_NULL;
    }

    for (i = 0; i < _tca9548a_dev_count; i++)
    {
        if (_tca9548a_dev_table[i] != RT_NULL &&
            rt_strcmp(_tca9548a_dev_table[i]->name, name) == 0)
        {
            return _tca9548a_dev_table[i];
        }
    }

    return RT_NULL;
}

rt_err_t tca9548a_select_channel(tca9548a_dev_t dev, rt_uint8_t channels)
{
    rt_err_t ret;

    if (dev == RT_NULL)
    {
        return TCA9548A_EINVAL;
    }

    rt_mutex_take(dev->lock, RT_WAITING_FOREVER);

    ret = tca9548a_write_register(dev, channels);
    if (ret == TCA9548A_EOK)
    {
        dev->current_channel = channels;
    }

    rt_mutex_release(dev->lock);

    return ret;
}

rt_err_t tca9548a_read_channel(tca9548a_dev_t dev, rt_uint8_t *channels)
{
    rt_err_t ret;

    if (dev == RT_NULL || channels == RT_NULL)
    {
        return TCA9548A_EINVAL;
    }

    rt_mutex_take(dev->lock, RT_WAITING_FOREVER);

    ret = tca9548a_read_register(dev, channels);
    if (ret == TCA9548A_EOK)
    {
        dev->current_channel = *channels;
    }

    rt_mutex_release(dev->lock);

    return ret;
}

rt_err_t tca9548a_enable_channel(tca9548a_dev_t dev, rt_uint8_t channel)
{
    rt_uint8_t new_channels;
    rt_err_t ret;

    if (dev == RT_NULL || channel >= TCA9548A_CHANNEL_MAX)
    {
        return TCA9548A_EINVAL;
    }

    rt_mutex_take(dev->lock, RT_WAITING_FOREVER);

    new_channels = dev->current_channel | (1 << channel);
    ret = tca9548a_write_register(dev, new_channels);
    if (ret == TCA9548A_EOK)
    {
        dev->current_channel = new_channels;
    }

    rt_mutex_release(dev->lock);

    return ret;
}

rt_err_t tca9548a_disable_channel(tca9548a_dev_t dev, rt_uint8_t channel)
{
    rt_uint8_t new_channels;
    rt_err_t ret;

    if (dev == RT_NULL || channel >= TCA9548A_CHANNEL_MAX)
    {
        return TCA9548A_EINVAL;
    }

    rt_mutex_take(dev->lock, RT_WAITING_FOREVER);

    new_channels = dev->current_channel & ~(1 << channel);
    ret = tca9548a_write_register(dev, new_channels);
    if (ret == TCA9548A_EOK)
    {
        dev->current_channel = new_channels;
    }

    rt_mutex_release(dev->lock);

    return ret;
}

rt_err_t tca9548a_disable_all(tca9548a_dev_t dev)
{
    return tca9548a_select_channel(dev, TCA9548A_CHANNEL_NONE);
}

rt_err_t tca9548a_enable_all(tca9548a_dev_t dev)
{
    return tca9548a_select_channel(dev, TCA9548A_CHANNEL_ALL);
}

rt_err_t tca9548a_reset(tca9548a_dev_t dev)
{
    if (dev == RT_NULL)
    {
        return TCA9548A_EINVAL;
    }

    return tca9548a_disable_all(dev);
}

rt_bool_t tca9548a_is_connected(tca9548a_dev_t dev)
{
    rt_uint8_t dummy;

    if (dev == RT_NULL)
    {
        return RT_FALSE;
    }

    rt_mutex_take(dev->lock, RT_WAITING_FOREVER);

    if (tca9548a_read_register(dev, &dummy) == TCA9548A_EOK)
    {
        rt_mutex_release(dev->lock);
        return RT_TRUE;
    }

    rt_mutex_release(dev->lock);
    return RT_FALSE;
}

/* Auto-initialize all configured devices from Kconfig */
static int tca9548a_auto_init(void)
{
    int count = 0;

#ifdef TCA9548A_DEV0_ENABLE
    if (tca9548a_init(TCA9548A_DEV0_BUS_NAME, TCA9548A_DEV0_ADDR,
                      TCA9548A_DEV0_NAME) != RT_NULL) count++;
#endif

#ifdef TCA9548A_DEV1_ENABLE
    if (tca9548a_init(TCA9548A_DEV1_BUS_NAME, TCA9548A_DEV1_ADDR,
                      TCA9548A_DEV1_NAME) != RT_NULL) count++;
#endif

#ifdef TCA9548A_DEV2_ENABLE
    if (tca9548a_init(TCA9548A_DEV2_BUS_NAME, TCA9548A_DEV2_ADDR,
                      TCA9548A_DEV2_NAME) != RT_NULL) count++;
#endif

#ifdef TCA9548A_DEV3_ENABLE
    if (tca9548a_init(TCA9548A_DEV3_BUS_NAME, TCA9548A_DEV3_ADDR,
                      TCA9548A_DEV3_NAME) != RT_NULL) count++;
#endif

#ifdef TCA9548A_DEV4_ENABLE
    if (tca9548a_init(TCA9548A_DEV4_BUS_NAME, TCA9548A_DEV4_ADDR,
                      TCA9548A_DEV4_NAME) != RT_NULL) count++;
#endif

#ifdef TCA9548A_DEV5_ENABLE
    if (tca9548a_init(TCA9548A_DEV5_BUS_NAME, TCA9548A_DEV5_ADDR,
                      TCA9548A_DEV5_NAME) != RT_NULL) count++;
#endif

#ifdef TCA9548A_DEV6_ENABLE
    if (tca9548a_init(TCA9548A_DEV6_BUS_NAME, TCA9548A_DEV6_ADDR,
                      TCA9548A_DEV6_NAME) != RT_NULL) count++;
#endif

#ifdef TCA9548A_DEV7_ENABLE
    if (tca9548a_init(TCA9548A_DEV7_BUS_NAME, TCA9548A_DEV7_ADDR,
                      TCA9548A_DEV7_NAME) != RT_NULL) count++;
#endif

    if (count > 0)
    {
        LOG_I("TCA9548A auto-init: %d device(s) initialized", count);
    }

    return 0;
}
INIT_DEVICE_EXPORT(tca9548a_auto_init);