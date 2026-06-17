# TCA9548A (PW548A)

## 1. Introduction

The TCA9548A (chip marking: PW548A) is a low-voltage 8-channel I2C switch with reset from Texas Instruments. It features 1-to-8 bidirectional translating switches controlled via the I2C bus, allowing voltage-level translation between 1.8V, 2.5V, 3.3V, and 5V buses.

This package provides a complete driver for the TCA9548A, including:
- Channel selection (single or combination) via I2C control register
- Support for up to 8 TCA9548A devices simultaneously
- Each device can be on a different I2C bus with different address
- Device name management with `tca9548a_find()` for name-based lookup
- Auto-initialization of all configured devices at system startup
- Thread-safe operations with mutex
- I2C bus scanning for device detection
- Voltage-level translation between different bus voltages

### 1.1 Directory Structure

| Name | Description |
| ---- | ---- |
| docs  | Documentation directory |
| examples | Example code directory |
| inc  | Header files directory |
| src  | Source code directory |
| LICENSE | License file |

### 1.2 License

TCA9548A package is licensed under Apache-2.0. See `LICENSE` file for details.

### 1.3 Dependencies

- RT-Thread 4.0.0+
- I2C device driver

## 2. How to Enable TCA9548A

To use the TCA9548A package, select it in RT-Thread's package manager:

```
RT-Thread online packages
    peripheral libraries and drivers --->
        [*] TCA9548A: Low Voltage 8-Channel I2C Switch with Reset (PW548A)
            [*] Enable TCA9548A example
            TCA9548A Device Configuration  --->
                [*] Device 0 - Enable
                    Device 0 - I2C bus name     --> "i2c1"
                    Device 0 - I2C address      --> 0x70
                    Device 0 - Device name      --> "tca9548a_0"
                [ ] Device 1 - Enable
                [ ] Device 2 - Enable
                ...
            Version (latest)  --->
```

Then update the packages using `pkgs --update` command.

### Multi-Device Configuration

Kconfig uses cascading display: **the next device's options only appear when the previous device is enabled**.

Up to 8 devices can be configured. Each device can:
- Use a different I2C bus (e.g., `i2c1`, `i2c2`)
- Use a different I2C address (0x70-0x77)
- Have a unique name (e.g., `tca9548a_0`, `sensor_switch`, `display_switch`)

### I2C Address

The 7-bit address is determined by A0, A1, A2 hardware pins:

| A2 | A1 | A0 | Address |
|----|----|----|---------|
| L  | L  | L  | 0x70    |
| L  | L  | H  | 0x71    |
| L  | H  | L  | 0x72    |
| L  | H  | H  | 0x73    |
| H  | L  | L  | 0x74    |
| H  | L  | H  | 0x75    |
| H  | H  | L  | 0x76    |
| H  | H  | H  | 0x77    |

## 3. Usage

### Auto-Init (Recommended)

Devices are automatically initialized at system startup based on menuconfig settings. Get device handle by name:

```c
#include "tca9548a.h"

int main(void)
{
    tca9548a_dev_t dev;

    /* Find device by name */
    dev = tca9548a_find("tca9548a_0");
    if (dev == RT_NULL)
    {
        rt_kprintf("Device not found!\n");
        return -1;
    }

    /* Select channel 0 */
    tca9548a_select_channel(dev, TCA9548A_CHANNEL_0);

    /* Now access downstream I2C device on channel 0 */

    /* Select multiple channels */
    tca9548a_select_channel(dev, TCA9548A_CHANNEL_0 | TCA9548A_CHANNEL_3 | TCA9548A_CHANNEL_7);

    /* Disable all channels */
    tca9548a_disable_all(dev);

    return 0;
}
```

### Manual Init

```c
#include "tca9548a.h"

int main(void)
{
    tca9548a_dev_t dev;

    /* Manually initialize */
    dev = tca9548a_init("i2c1", 0x70, "my_switch");
    if (dev == RT_NULL)
    {
        rt_kprintf("TCA9548A init failed!\n");
        return -1;
    }

    /* Select channel 0 */
    tca9548a_select_channel(dev, TCA9548A_CHANNEL_0);

    /* Release resources */
    tca9548a_deinit(dev);

    return 0;
}
```

### Channel Bit Masks

| Macro | Value | Description |
|-------|-------|-------------|
| TCA9548A_CHANNEL_0 | 0x01 | Channel 0 |
| TCA9548A_CHANNEL_1 | 0x02 | Channel 1 |
| TCA9548A_CHANNEL_2 | 0x04 | Channel 2 |
| TCA9548A_CHANNEL_3 | 0x08 | Channel 3 |
| TCA9548A_CHANNEL_4 | 0x10 | Channel 4 |
| TCA9548A_CHANNEL_5 | 0x20 | Channel 5 |
| TCA9548A_CHANNEL_6 | 0x40 | Channel 6 |
| TCA9548A_CHANNEL_7 | 0x80 | Channel 7 |
| TCA9548A_CHANNEL_ALL | 0xFF | All channels |
| TCA9548A_CHANNEL_NONE | 0x00 | No channels |

### Typical Applications

**Resolving I2C address conflicts**: When multiple I2C devices with the same address need to be connected, connect each device to a TCA9548A channel and switch channels to access them individually.

```
                    +-----------+
    MCU I2C ------> | TCA9548A  |
                    |           |
                    | Channel 0 +------> Sensor A (0x76)
                    | Channel 1 +------> Sensor B (0x76)
                    | Channel 2 +------> Sensor C (0x76)
                    | Channel 3 +------> Sensor D (0x76)
                    |   ...     |
                    +-----------+
```

**Multi-chip multi-bus**: Multiple TCA9548A devices on different I2C buses, each managing their own downstream devices.

```
    MCU I2C1 ----> TCA9548A (0x70, "sensor_switch")
                       +----> 8 sensors
    MCU I2C2 ----> TCA9548A (0x70, "display_switch")
                       +----> 8 displays
```

* Full API reference: [docs/api.md](docs/api.md)
* More documentation: [`/docs`](/docs)

## 4. Notes

1. Ensure the I2C bus driver is properly configured and enabled before use.
2. All channels are deselected after power-on. Explicitly select channels via I2C commands.
3. When using multiple TCA9548A devices on the same I2C bus, configure each device with a different address.
4. Maximum 8 TCA9548A devices on a single I2C bus (addresses 0x70-0x77).
5. Downstream I2C devices require independent pull-up resistors.
6. Package index files must be UTF-8 encoded.

## 5. Contact

* Maintainer: WennianYan
* Email: yanwennian@yeah.net
* Homepage: https://github.com/WennianYan/tca9548a