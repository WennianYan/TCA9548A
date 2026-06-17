/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-01-15     WennianYan   first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "tca9548a.h"

static void tca9548a_scan_devices(const char *i2c_bus_name)
{
    rt_uint8_t addr;
    rt_uint8_t data;
    struct rt_i2c_bus_device *i2c_bus;
    struct rt_i2c_msg msg;
    int found = 0;

    i2c_bus = (struct rt_i2c_bus_device *)rt_device_find(i2c_bus_name);
    if (i2c_bus == RT_NULL)
    {
        rt_kprintf("I2C bus '%s' not found!\n", i2c_bus_name);
        return;
    }

    rt_kprintf("Scanning for TCA9548A devices on I2C bus '%s'...\n", i2c_bus_name);

    for (addr = TCA9548A_ADDR_MIN; addr <= TCA9548A_ADDR_MAX; addr++)
    {
        msg.addr  = addr;
        msg.flags = RT_I2C_RD;
        msg.buf   = &data;
        msg.len   = 1;

        if (rt_i2c_transfer(i2c_bus, &msg, 1) == 1)
        {
            rt_kprintf("  Found device at address 0x%02X (A0=%d, A1=%d, A2=%d)\n",
                       addr,
                       (addr >> 0) & 0x01,
                       (addr >> 1) & 0x01,
                       (addr >> 2) & 0x01);
            found++;
        }
    }

    if (found == 0)
    {
        rt_kprintf("  No TCA9548A devices found!\n");
    }
    else
    {
        rt_kprintf("  Total: %d device(s) found\n", found);
    }
}

static void tca9548a_test_channel_switch(tca9548a_dev_t dev)
{
    rt_uint8_t readback;
    rt_err_t ret;
    int i;

    rt_kprintf("\n--- Channel Switch Test for '%s' ---\n", dev->name);

    rt_kprintf("Disabling all channels...\n");
    ret = tca9548a_disable_all(dev);
    if (ret == TCA9548A_EOK)
    {
        rt_kprintf("  [OK] All channels disabled\n");
    }
    else
    {
        rt_kprintf("  [FAIL] Disable all failed, error=%d\n", ret);
    }

    ret = tca9548a_read_channel(dev, &readback);
    if (ret == TCA9548A_EOK)
    {
        rt_kprintf("  Readback: 0x%02X\n", readback);
    }

    for (i = 0; i < TCA9548A_CHANNEL_MAX; i++)
    {
        rt_kprintf("Enabling channel %d...\n", i);
        ret = tca9548a_enable_channel(dev, i);
        if (ret == TCA9548A_EOK)
        {
            rt_kprintf("  [OK] Channel %d enabled\n", i);
        }
        else
        {
            rt_kprintf("  [FAIL] Enable channel %d failed, error=%d\n", i, ret);
        }

        ret = tca9548a_read_channel(dev, &readback);
        if (ret == TCA9548A_EOK)
        {
            rt_kprintf("  Readback: 0x%02X, expected bit %d set\n", readback, i);
        }

        tca9548a_disable_channel(dev, i);
    }

    rt_kprintf("Enabling all channels...\n");
    ret = tca9548a_enable_all(dev);
    if (ret == TCA9548A_EOK)
    {
        rt_kprintf("  [OK] All channels enabled\n");
    }
    else
    {
        rt_kprintf("  [FAIL] Enable all failed, error=%d\n", ret);
    }

    ret = tca9548a_read_channel(dev, &readback);
    if (ret == TCA9548A_EOK)
    {
        rt_kprintf("  Readback: 0x%02X (expected 0xFF)\n", readback);
    }

    tca9548a_disable_all(dev);

    rt_kprintf("Selecting channels 0, 3, 5, 7...\n");
    ret = tca9548a_select_channel(dev, TCA9548A_CHANNEL_0 | TCA9548A_CHANNEL_3 |
                                       TCA9548A_CHANNEL_5 | TCA9548A_CHANNEL_7);
    if (ret == TCA9548A_EOK)
    {
        rt_kprintf("  [OK] Channels 0,3,5,7 selected\n");
    }
    else
    {
        rt_kprintf("  [FAIL] Select channels failed, error=%d\n", ret);
    }

    ret = tca9548a_read_channel(dev, &readback);
    if (ret == TCA9548A_EOK)
    {
        rt_kprintf("  Readback: 0x%02X (expected 0xA9)\n", readback);
    }

    tca9548a_disable_all(dev);
    rt_kprintf("--- Channel Switch Test Complete ---\n");
}

static void tca9548a_test_multi_device(void)
{
    tca9548a_dev_t dev;
    int i;

    rt_kprintf("\n--- Multi-Device Test ---\n");

    for (i = 0; i < 8; i++)
    {
        char name[RT_NAME_MAX];
        rt_snprintf(name, sizeof(name), "tca9548a_%d", i);
        dev = tca9548a_find(name);
        if (dev != RT_NULL)
        {
            rt_kprintf("  Device '%s' found: bus='%s', addr=0x%02X\n",
                       dev->name,
                       dev->i2c_bus->parent.parent.name,
                       dev->addr);
        }
        else
        {
            rt_kprintf("  Device '%s' not found (not configured)\n", name);
        }
    }

    rt_kprintf("--- Multi-Device Test Complete ---\n");
}

static void tca9548a_example_thread_entry(void *parameter)
{
    rt_kprintf("\n");
    rt_kprintf("========================================\n");
    rt_kprintf("  TCA9548A (PW548A) Driver Example\n");
    rt_kprintf("========================================\n\n");

    /* Scan I2C buses for devices */
    tca9548a_scan_devices(TCA9548A_DEV0_BUS_NAME);

    /* Multi-device test - find all configured devices */
    tca9548a_test_multi_device();

    /* Test channel switching on the first device */
    {
        tca9548a_dev_t dev = tca9548a_find("tca9548a_0");
        if (dev != RT_NULL)
        {
            tca9548a_test_channel_switch(dev);
        }
        else
        {
            rt_kprintf("\nDevice 'tca9548a_0' not configured, skipping channel test.\n");
        }
    }

    rt_kprintf("\n--- I2C Pass-Through Example ---\n");
    rt_kprintf("To use I2C pass-through with named devices:\n");
    rt_kprintf("  1. Find device: tca9548a_dev_t dev = tca9548a_find(\"tca9548a_0\");\n");
    rt_kprintf("  2. Select channel: tca9548a_select_channel(dev, TCA9548A_CHANNEL_0);\n");
    rt_kprintf("  3. Perform I2C operations on the same bus\n");
    rt_kprintf("  4. The downstream device is now accessible\n");
    rt_kprintf("  5. When done: tca9548a_disable_all(dev);\n");
    rt_kprintf("--- I2C Pass-Through Example Complete ---\n");

    rt_kprintf("\n========================================\n");
    rt_kprintf("  TCA9548A Example Completed\n");
    rt_kprintf("========================================\n");
}

static int tca9548a_example_init(void)
{
    rt_thread_t tid;

    tid = rt_thread_create("tca9548a_ex",
                           tca9548a_example_thread_entry,
                           RT_NULL,
                           4096,
                           25,
                           5);

    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
        return 0;
    }

    return -1;
}

INIT_APP_EXPORT(tca9548a_example_init);