# TCA9548A (PW548A)

## 1、介绍

TCA9548A（芯片丝印：PW548A）是德州仪器（TI）推出的低电压 8 通道 I2C 开关，具有复位功能和电压电平转换能力。通过 I2C 总线控制，可将一个上游 I2C 总线扩展为 8 个下游通道，解决 I2C 设备地址冲突问题。

本软件包提供了 TCA9548A 的完整驱动支持，包括：
- 通过 I2C 总线控制 8 个通道的任意组合选择
- 支持最多 8 个 TCA9548A 设备同时使用
- 每个设备可挂载在不同 I2C 总线上，配置不同的地址
- 设备名称管理，通过 `tca9548a_find()` 按名查找
- 系统启动自动初始化所有配置的设备
- 线程安全操作（互斥锁保护）
- I2C 总线设备扫描检测
- 支持 1.8V、2.5V、3.3V、5V 总线间电压电平转换

### 1.1 目录结构

| 名称 | 说明 |
| ---- | ---- |
| docs  | 文档目录，包含 API 手册 |
| examples | 示例代码目录 |
| inc  | 头文件目录 |
| src  | 源代码目录 |
| LICENSE | 许可证文件 |

### 1.2 许可证

TCA9548A 软件包遵循 Apache-2.0 许可，详见 `LICENSE` 文件。

### 1.3 依赖

- RT-Thread 4.0.0+
- I2C 设备驱动框架

## 2、如何打开 TCA9548A

使用 TCA9548A 软件包需要在 RT-Thread 的包管理器中选择它，具体路径如下：

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

然后让 RT-Thread 的包管理器自动更新，或者使用 `pkgs --update` 命令更新包到 BSP 中。

### 多设备配置

Kconfig 采用级联显示：**只有上一个设备启用后，下一个设备的配置选项才会出现**。

最多可配置 8 个设备。每个设备可以：
- 使用不同的 I2C 总线（如 `i2c1`、`i2c2`）
- 使用不同的 I2C 地址（0x70-0x77）
- 使用独立的设备名称（如 `tca9548a_0`、`sensor_switch`、`display_switch`）

### I2C 地址配置

TCA9548A 的 7-bit I2C 地址由 A0、A1、A2 三个硬件引脚决定：

| A2 | A1 | A0 | 地址 |
|----|----|----|------|
| L  | L  | L  | 0x70 |
| L  | L  | H  | 0x71 |
| L  | H  | L  | 0x72 |
| L  | H  | H  | 0x73 |
| H  | L  | L  | 0x74 |
| H  | L  | H  | 0x75 |
| H  | H  | L  | 0x76 |
| H  | H  | H  | 0x77 |

## 3、使用 TCA9548A

### 自动初始化使用（推荐）

在 menuconfig 中配置好设备后，系统启动时自动初始化。在代码中通过名称获取设备：

```c
#include "tca9548a.h"

int main(void)
{
    tca9548a_dev_t dev;

    /* 通过名称查找设备 */
    dev = tca9548a_find("tca9548a_0");
    if (dev == RT_NULL)
    {
        rt_kprintf("Device not found!\n");
        return -1;
    }

    /* 选择通道 0 */
    tca9548a_select_channel(dev, TCA9548A_CHANNEL_0);

    /* 现在可以访问通道 0 上的下游 I2C 设备 */

    /* 同时选择多个通道 */
    tca9548a_select_channel(dev, TCA9548A_CHANNEL_0 | TCA9548A_CHANNEL_3 | TCA9548A_CHANNEL_7);

    /* 禁用所有通道 */
    tca9548a_disable_all(dev);

    return 0;
}
```

### 手动初始化使用

```c
#include "tca9548a.h"

int main(void)
{
    tca9548a_dev_t dev;

    /* 手动初始化设备 */
    dev = tca9548a_init("i2c1", 0x70, "my_switch");
    if (dev == RT_NULL)
    {
        rt_kprintf("TCA9548A init failed!\n");
        return -1;
    }

    /* 选择通道 0 */
    tca9548a_select_channel(dev, TCA9548A_CHANNEL_0);

    /* 释放资源 */
    tca9548a_deinit(dev);

    return 0;
}
```

### 通道位掩码定义

| 宏定义 | 值 | 说明 |
|--------|-----|------|
| TCA9548A_CHANNEL_0 | 0x01 | 通道 0 |
| TCA9548A_CHANNEL_1 | 0x02 | 通道 1 |
| TCA9548A_CHANNEL_2 | 0x04 | 通道 2 |
| TCA9548A_CHANNEL_3 | 0x08 | 通道 3 |
| TCA9548A_CHANNEL_4 | 0x10 | 通道 4 |
| TCA9548A_CHANNEL_5 | 0x20 | 通道 5 |
| TCA9548A_CHANNEL_6 | 0x40 | 通道 6 |
| TCA9548A_CHANNEL_7 | 0x80 | 通道 7 |
| TCA9548A_CHANNEL_ALL | 0xFF | 所有通道 |
| TCA9548A_CHANNEL_NONE | 0x00 | 无通道 |

### 典型应用场景

**解决 I2C 地址冲突**：当需要连接多个相同地址的 I2C 设备时（例如 8 个 BME280 传感器都使用地址 0x76），可以将每个设备连接到 TCA9548A 的一个通道，通过切换通道来分别访问各个设备。

```
                    +-----------+
    MCU I2C ------> | TCA9548A  |
                    |           |
                    | Channel 0 +------> 传感器 A (0x76)
                    | Channel 1 +------> 传感器 B (0x76)
                    | Channel 2 +------> 传感器 C (0x76)
                    | Channel 3 +------> 传感器 D (0x76)
                    |   ...     |
                    +-----------+
```

**多芯片多总线场景**：可以在不同 I2C 总线上挂载多个 TCA9548A，每个管理自己的下游设备。

```
    MCU I2C1 ----> TCA9548A (0x70, "sensor_switch")
                       +----> 8 个传感器
    MCU I2C2 ----> TCA9548A (0x70, "display_switch")
                       +----> 8 个显示屏
```

* 完整的 API 手册可以访问 [docs/api.md](docs/api.md)
* 更多文档位于 [`/docs`](/docs) 下，使用前 **务必查看**

## 4、注意事项

1. 使用前请确保 I2C 总线驱动已正确配置并启用。
2. 芯片上电后所有通道默认为关闭状态，需要通过 I2C 命令显式选择通道。
3. 如果使用多个 TCA9548A 设备共享同一 I2C 总线，请确保每个设备配置了不同的地址（通过 A0/A1/A2 引脚）。
4. 同一 I2C 总线上最多挂载 8 个 TCA9548A（地址 0x70-0x77 各一个）。
5. 下游 I2C 设备需要独立的上拉电阻，TCA9548A 不会在通道间传递上拉。
6. 软件包索引文件编码必须为 UTF-8 格式。

## 5、联系方式 & 感谢

* 维护：WennianYan
* 邮箱：yanwennian@yeah.net
* 主页：https://github.com/WennianYan/tca9548a