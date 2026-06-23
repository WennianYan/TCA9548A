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

/* Test result tracking */
static int _test_pass = 0;
static int _test_fail = 0;
static int _test_skip = 0;

#define TEST_LOG(fmt, ...)  rt_kprintf("    " fmt, ##__VA_ARGS__)
#define TEST_PASS(fmt, ...) do { _test_pass++; rt_kprintf("    [PASS] " fmt, ##__VA_ARGS__); } while (0)
#define TEST_FAIL(fmt, ...) do { _test_fail++; rt_kprintf("    [FAIL] " fmt, ##__VA_ARGS__); } while (0)
#define TEST_SKIP(fmt, ...) do { _test_skip++; rt_kprintf("    [SKIP] " fmt, ##__VA_ARGS__); } while (0)

/* All channel bitmask combinations to test */
static const rt_uint8_t _channel_combos[] = {
    /* Single channels */
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
    /* Alternating patterns */
    0x55, 0xAA,
    /* Half-select */
    0x0F, 0xF0,
    /* Pairs */
    0x33, 0xCC,
    /* Specific multi-channel */
    0xA9, 0x56, 0x11, 0x81, 0x42, 0x24,
    /* All and none */
    0xFF, 0x00,
};

static const char *combo_desc(rt_uint8_t mask)
{
    if (mask == 0x00) return "none";
    if (mask == 0xFF) return "all";
    if (mask == 0x55) return "0,2,4,6";
    if (mask == 0xAA) return "1,3,5,7";
    if (mask == 0x0F) return "0-3";
    if (mask == 0xF0) return "4-7";
    if (mask == 0x33) return "0,1,4,5";
    if (mask == 0xCC) return "2,3,6,7";
    return "custom";
}

/* Test: read back and compare */
static rt_bool_t _verify_readback(tca9548a_dev_t dev, rt_uint8_t expected,
                                   const char *desc)
{
    rt_uint8_t readback;
    rt_err_t ret;

    ret = tca9548a_read_channel(dev, &readback);
    if (ret != TCA9548A_EOK)
    {
        TEST_FAIL("read_channel() failed after set %s (0x%02X), err=%d\n",
                  desc, expected, ret);
        return RT_FALSE;
    }

    if (readback != expected)
    {
        TEST_FAIL("%s (0x%02X): readback=0x%02X, expected=0x%02X\n",
                  desc, expected, readback, expected);
        return RT_FALSE;
    }

    TEST_PASS("%s: set=0x%02X, readback=0x%02X\n", desc, expected, readback);
    return RT_TRUE;
}

static int tca9548a_scan_bus(const char *bus_name)
{
    rt_uint8_t addr;
    rt_uint8_t data;
    struct rt_i2c_bus_device *i2c_bus;
    struct rt_i2c_msg msg;
    int found = 0;

    i2c_bus = (struct rt_i2c_bus_device *)rt_device_find(bus_name);
    if (i2c_bus == RT_NULL)
    {
        TEST_LOG("I2C bus '%s' not found\n", bus_name);
        return -1;
    }

    TEST_LOG("Scanning bus '%s' for TCA9548A chips...\n", bus_name);

    for (addr = TCA9548A_ADDR_MIN; addr <= TCA9548A_ADDR_MAX; addr++)
    {
        msg.addr  = addr;
        msg.flags = RT_I2C_RD;
        msg.buf   = &data;
        msg.len   = 1;

        if (rt_i2c_transfer(i2c_bus, &msg, 1) == 1)
        {
            rt_kprintf("    Found chip at 0x%02X (A2=%d A1=%d A0=%d)\n",
                       addr,
                       (addr >> 2) & 0x01,
                       (addr >> 1) & 0x01,
                       (addr >> 0) & 0x01);
            found++;
        }
    }

    if (found == 0)
        TEST_LOG("No TCA9548A chips found on bus '%s'\n", bus_name);
    else
        TEST_LOG("%d chip(s) found on bus '%s'\n", found, bus_name);

    return found;
}

static tca9548a_dev_t tca9548a_validate_device(const char *bus_name,
                                                rt_uint8_t addr,
                                                const char *dev_name)
{
    tca9548a_dev_t dev;

    /* Step 1: try auto-init result */
    dev = tca9548a_find(dev_name);
    if (dev != RT_NULL)
    {
        if (tca9548a_is_connected(dev))
        {
            TEST_PASS("auto-init OK: '%s' found and connected\n", dev_name);
            return dev;
        }
        TEST_FAIL("auto-init OK but device not responding on bus\n");
        return RT_NULL;
    }

    /* Step 2: auto-init failed, manual init */
    TEST_LOG("auto-init not found for '%s', trying manual init...\n", dev_name);
    dev = tca9548a_init(bus_name, addr, dev_name);
    if (dev == RT_NULL)
    {
        TEST_FAIL("manual init failed for '%s'\n", dev_name);
        return RT_NULL;
    }

    /* Step 3: verify connectivity */
    if (tca9548a_is_connected(dev))
    {
        TEST_PASS("manual init OK: '%s' connected\n", dev_name);
        return dev;
    }

    TEST_FAIL("manual init OK but device not responding\n");
    return RT_NULL;
}

/* Comprehensive test for one device */
static void tca9548a_test_device(tca9548a_dev_t dev)
{
    rt_uint8_t readback;
    rt_err_t ret;
    int i, total;

    rt_kprintf("\n");
    rt_kprintf("========================================\n");
    rt_kprintf("  Test Suite: '%s' (bus='%s', addr=0x%02X)\n",
               dev->name, dev->i2c_bus->parent.parent.name, dev->addr);
    rt_kprintf("========================================\n\n");

    _test_pass = _test_fail = _test_skip = 0;

    /* ================================================================
     * Test 1: tca9548a_is_connected()
     * ================================================================ */
    rt_kprintf("--- Test 1: tca9548a_is_connected() ---\n");
    if (tca9548a_is_connected(dev) == RT_TRUE)
    {
        TEST_PASS("tca9548a_is_connected() = RT_TRUE\n");
    }
    else
    {
        TEST_FAIL("tca9548a_is_connected() = RT_FALSE\n");
    }

    /* ================================================================
     * Test 2: tca9548a_reset()
     * ================================================================ */
    rt_kprintf("\n--- Test 2: tca9548a_reset() ---\n");
    ret = tca9548a_reset(dev);
    if (ret == TCA9548A_EOK)
    {
        TEST_PASS("tca9548a_reset() = TCA9548A_EOK\n");
        _verify_readback(dev, 0x00, "reset() verify");
    }
    else
    {
        TEST_FAIL("tca9548a_reset() failed, err=%d\n", ret);
    }

    /* ================================================================
     * Test 3: tca9548a_disable_all()
     * ================================================================ */
    rt_kprintf("\n--- Test 3: tca9548a_disable_all() ---\n");
    /* First enable all to set a known state */
    tca9548a_enable_all(dev);
    ret = tca9548a_disable_all(dev);
    if (ret == TCA9548A_EOK)
    {
        TEST_PASS("tca9548a_disable_all() = TCA9548A_EOK\n");
        _verify_readback(dev, 0x00, "disable_all() verify");
    }
    else
    {
        TEST_FAIL("tca9548a_disable_all() failed, err=%d\n", ret);
    }

    /* ================================================================
     * Test 4: tca9548a_enable_all()
     * ================================================================ */
    rt_kprintf("\n--- Test 4: tca9548a_enable_all() ---\n");
    ret = tca9548a_enable_all(dev);
    if (ret == TCA9548A_EOK)
    {
        TEST_PASS("tca9548a_enable_all() = TCA9548A_EOK\n");
        _verify_readback(dev, 0xFF, "enable_all() verify");
    }
    else
    {
        TEST_FAIL("tca9548a_enable_all() failed, err=%d\n", ret);
    }
    tca9548a_disable_all(dev);

    /* ================================================================
     * Test 5: tca9548a_enable_channel() / disable_channel() per channel
     * ================================================================ */
    rt_kprintf("\n--- Test 5: enable_channel() / disable_channel() per channel ---\n");
    for (i = 0; i < TCA9548A_CHANNEL_MAX; i++)
    {
        rt_uint8_t expected = (1 << i);

        ret = tca9548a_enable_channel(dev, i);
        if (ret != TCA9548A_EOK)
        {
            TEST_FAIL("enable_channel(%d) failed, err=%d\n", i, ret);
            continue;
        }

        _verify_readback(dev, expected, "enable_channel");

        ret = tca9548a_disable_channel(dev, i);
        if (ret != TCA9548A_EOK)
        {
            TEST_FAIL("disable_channel(%d) failed, err=%d\n", i, ret);
            continue;
        }

        _verify_readback(dev, 0x00, "disable_channel");
    }

    /* ================================================================
     * Test 6: tca9548a_select_channel() - all bitmask combinations
     * ================================================================ */
    rt_kprintf("\n--- Test 6: select_channel() - all combinations ---\n");
    total = sizeof(_channel_combos) / sizeof(_channel_combos[0]);
    for (i = 0; i < total; i++)
    {
        rt_uint8_t mask = _channel_combos[i];

        ret = tca9548a_select_channel(dev, mask);
        if (ret != TCA9548A_EOK)
        {
            TEST_FAIL("select_channel(%s, 0x%02X) failed, err=%d\n",
                      combo_desc(mask), mask, ret);
            continue;
        }

        _verify_readback(dev, mask, "select_channel");
    }
    /* Cleanup */
    tca9548a_disable_all(dev);

    /* ================================================================
     * Test 7: tca9548a_read_channel() - edge cases
     * ================================================================ */
    rt_kprintf("\n--- Test 7: read_channel() edge cases ---\n");

    /* Test read after consecutive enable/disable */
    tca9548a_enable_channel(dev, 0);
    tca9548a_enable_channel(dev, 3);
    tca9548a_enable_channel(dev, 7);
    _verify_readback(dev, TCA9548A_CHANNEL_0 | TCA9548A_CHANNEL_3 | TCA9548A_CHANNEL_7,
                     "consecutive enable 0,3,7");

    tca9548a_disable_channel(dev, 3);
    _verify_readback(dev, TCA9548A_CHANNEL_0 | TCA9548A_CHANNEL_7,
                     "disable 3 from 0,3,7");

    tca9548a_disable_all(dev);

    /* Test NULL pointer protection */
    ret = tca9548a_read_channel(dev, RT_NULL);
    if (ret == TCA9548A_EINVAL)
    {
        TEST_PASS("read_channel(NULL buf) = TCA9548A_EINVAL\n");
    }
    else
    {
        TEST_FAIL("read_channel(NULL buf) returned %d, expected %d\n",
                  ret, TCA9548A_EINVAL);
    }

    /* ================================================================
     * Test 8: tca9548a_select_channel() - invalid channel
     * ================================================================ */
    rt_kprintf("\n--- Test 8: invalid channel / parameter check ---\n");
    ret = tca9548a_enable_channel(dev, 8);
    if (ret == TCA9548A_EINVAL)
    {
        TEST_PASS("enable_channel(8) = TCA9548A_EINVAL\n");
    }
    else
    {
        TEST_FAIL("enable_channel(8) returned %d, expected %d\n", ret, TCA9548A_EINVAL);
    }

    ret = tca9548a_disable_channel(dev, 8);
    if (ret == TCA9548A_EINVAL)
    {
        TEST_PASS("disable_channel(8) = TCA9548A_EINVAL\n");
    }
    else
    {
        TEST_FAIL("disable_channel(8) returned %d, expected %d\n", ret, TCA9548A_EINVAL);
    }

    ret = tca9548a_select_channel(RT_NULL, 0x01);
    if (ret == TCA9548A_EINVAL)
    {
        TEST_PASS("select_channel(NULL dev) = TCA9548A_EINVAL\n");
    }
    else
    {
        TEST_FAIL("select_channel(NULL dev) returned %d, expected %d\n", ret, TCA9548A_EINVAL);
    }

    /* ================================================================
     * Test 9: tca9548a_find() - by name
     * ================================================================ */
    rt_kprintf("\n--- Test 9: tca9548a_find() ---\n");
    {
        tca9548a_dev_t found = tca9548a_find(dev->name);
        if (found == dev)
        {
            TEST_PASS("tca9548a_find(\"%s\") = %p (correct)\n", dev->name, found);
        }
        else
        {
            TEST_FAIL("tca9548a_find(\"%s\") = %p, expected %p\n", dev->name, found, dev);
        }

        found = tca9548a_find("nonexistent_device");
        if (found == RT_NULL)
        {
            TEST_PASS("tca9548a_find(\"nonexistent\") = RT_NULL\n");
        }
        else
        {
            TEST_FAIL("tca9548a_find(\"nonexistent\") returned %p, expected RT_NULL\n", found);
        }

        found = tca9548a_find(RT_NULL);
        if (found == RT_NULL)
        {
            TEST_PASS("tca9548a_find(NULL) = RT_NULL\n");
        }
        else
        {
            TEST_FAIL("tca9548a_find(NULL) returned %p, expected RT_NULL\n", found);
        }
    }

    /* ================================================================
     * Test 10: tca9548a_deinit() + re-init cycle
     * ================================================================ */
    rt_kprintf("\n--- Test 10: deinit / re-init cycle ---\n");
    {
        char saved_name[RT_NAME_MAX];
        char saved_bus[RT_NAME_MAX];
        rt_uint8_t saved_addr;

        /* Save device info before deinit (dev pointer freed after deinit) */
        rt_strncpy(saved_name, dev->name, RT_NAME_MAX);
        rt_strncpy(saved_bus, dev->i2c_bus->parent.parent.name, RT_NAME_MAX);
        saved_addr = dev->addr;

        ret = tca9548a_deinit(dev);
        if (ret == TCA9548A_EOK)
        {
            TEST_PASS("tca9548a_deinit() = TCA9548A_EOK\n");
        }
        else
        {
            TEST_FAIL("tca9548a_deinit() failed, err=%d\n", ret);
        }

        /* Verify deinit */
        {
            tca9548a_dev_t found = tca9548a_find(saved_name);
            if (found == RT_NULL)
            {
                TEST_PASS("tca9548a_find() after deinit = RT_NULL\n");
            }
            else
            {
                TEST_FAIL("tca9548a_find() after deinit returned %p\n", found);
            }
        }

        /* Re-init */
        dev = tca9548a_init(saved_bus, saved_addr, saved_name);
        if (dev != RT_NULL)
        {
            TEST_PASS("tca9548a_init() re-init OK\n");
            if (tca9548a_is_connected(dev))
            {
                TEST_PASS("re-init device is connected\n");
            }
            else
            {
                TEST_FAIL("re-init device not connected\n");
            }
        }
        else
        {
            TEST_FAIL("tca9548a_init() re-init failed\n");
            TEST_SKIP("re-init failed, cannot continue tests\n");
            goto print_report;
        }
    }

    /* ================================================================
     * Test 11: Rapid channel switching stress
     * ================================================================ */
    rt_kprintf("\n--- Test 11: rapid channel switching ---\n");
    {
        int round, ch;
        int rapid_pass = 0, rapid_fail = 0;

        for (round = 0; round < 5; round++)
        {
            for (ch = 0; ch < TCA9548A_CHANNEL_MAX; ch++)
            {
                rt_uint8_t mask = 1 << ch;
                rt_uint8_t rb;

                ret = tca9548a_select_channel(dev, mask);
                if (ret != TCA9548A_EOK)
                {
                    rapid_fail++;
                    continue;
                }

                ret = tca9548a_read_channel(dev, &rb);
                if (ret == TCA9548A_EOK && rb == mask)
                {
                    rapid_pass++;
                }
                else
                {
                    rapid_fail++;
                }
            }
        }

        if (rapid_fail == 0)
        {
            TEST_PASS("rapid switch: %d operations, all passed\n", rapid_pass);
        }
        else
        {
            TEST_FAIL("rapid switch: %d/%d failed\n", rapid_fail, rapid_pass + rapid_fail);
        }
    }
    tca9548a_disable_all(dev);

print_report:
    /* ================================================================
     * Print report
     * ================================================================ */
    rt_kprintf("\n");
    rt_kprintf("========================================\n");
    rt_kprintf("  Test Report: '%s'\n", dev->name);
    rt_kprintf("========================================\n");
    rt_kprintf("  PASS:  %d\n", _test_pass);
    rt_kprintf("  FAIL:  %d\n", _test_fail);
    rt_kprintf("  SKIP:  %d\n", _test_skip);
    rt_kprintf("  TOTAL: %d\n", _test_pass + _test_fail + _test_skip);
    if (_test_fail == 0)
    {
        rt_kprintf("  VERDICT: ALL TESTS PASSED\n");
    }
    else
    {
        rt_kprintf("  VERDICT: %d TEST(S) FAILED\n", _test_fail);
    }
    rt_kprintf("========================================\n\n");
}

/* Macro helper: validate one configured device */
#define VALIDATE_AND_TEST_DEVICE(n) \
    do { \
        rt_kprintf("\n  Device %d: '%s' (bus='%s', addr=0x%02X)\n", \
                   n, TCA9548A_DEV##n##_NAME, \
                   TCA9548A_DEV##n##_BUS_NAME, \
                   TCA9548A_DEV##n##_ADDR); \
        tca9548a_scan_bus(TCA9548A_DEV##n##_BUS_NAME); \
        tca9548a_dev_t _dev##n = tca9548a_validate_device( \
            TCA9548A_DEV##n##_BUS_NAME, \
            TCA9548A_DEV##n##_ADDR, \
            TCA9548A_DEV##n##_NAME); \
        if (_dev##n != RT_NULL) { \
            validated++; \
            tca9548a_test_device(_dev##n); \
        } \
    } while (0)

static void tca9548a_example_thread_entry(void *parameter)
{
    int validated = 0;
    int configured = 0;

    rt_kprintf("\n");
    rt_kprintf("========================================\n");
    rt_kprintf("  TCA9548A (PW548A) Driver Example\n");
    rt_kprintf("========================================\n\n");

    rt_kprintf("--- Hardware Scan & Device Validation ---\n");

#ifdef TCA9548A_DEV0_ENABLE
    configured++; VALIDATE_AND_TEST_DEVICE(0);
#endif
#ifdef TCA9548A_DEV1_ENABLE
    configured++; VALIDATE_AND_TEST_DEVICE(1);
#endif
#ifdef TCA9548A_DEV2_ENABLE
    configured++; VALIDATE_AND_TEST_DEVICE(2);
#endif
#ifdef TCA9548A_DEV3_ENABLE
    configured++; VALIDATE_AND_TEST_DEVICE(3);
#endif
#ifdef TCA9548A_DEV4_ENABLE
    configured++; VALIDATE_AND_TEST_DEVICE(4);
#endif
#ifdef TCA9548A_DEV5_ENABLE
    configured++; VALIDATE_AND_TEST_DEVICE(5);
#endif
#ifdef TCA9548A_DEV6_ENABLE
    configured++; VALIDATE_AND_TEST_DEVICE(6);
#endif
#ifdef TCA9548A_DEV7_ENABLE
    configured++; VALIDATE_AND_TEST_DEVICE(7);
#endif

    rt_kprintf("\n");
    rt_kprintf("========================================\n");
    rt_kprintf("  Overall Summary\n");
    rt_kprintf("========================================\n");
    rt_kprintf("  Kconfig configured: %d device(s)\n", configured);
    rt_kprintf("  Validated & tested: %d device(s)\n", validated);
    if (validated == 0)
    {
        rt_kprintf("\n  Troubleshooting:\n");
        rt_kprintf("  1. Check I2C bus driver is enabled in BSP\n");
        rt_kprintf("  2. Check TCA9548A chip is powered and wired\n");
        rt_kprintf("  3. Check Kconfig bus name, address, device name\n");
    }
    rt_kprintf("========================================\n\n");
}

static int tca9548a_example_init(void)
{
    rt_thread_t tid;

    tid = rt_thread_create("tca9548a_ex",
                           tca9548a_example_thread_entry,
                           RT_NULL,
                           8192,
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