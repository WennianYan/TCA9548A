# TCA9548A API

## 初始化设备

`tca9548a_dev_t tca9548a_init(const char *i2c_bus_name, rt_uint8_t addr, const char *name)`

初始化 TCA9548A (PW548A) 设备。

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|`i2c_bus_name`     | I2C 总线设备名称，例如 "i2c1"        |
|`addr`             | 7-bit I2C 地址，范围 0x70-0x77      |
|`name`             | 设备名称，用于后续查找               |
| **返回**          | **描述**                            |
| 非 RT_NULL        | 设备句柄，初始化成功                 |
| RT_NULL           | 初始化失败                          |

## 释放设备

`rt_err_t tca9548a_deinit(tca9548a_dev_t dev)`

释放 TCA9548A 设备资源。

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|`dev`              | 设备句柄                            |
| **返回**          | **描述**                            |
| TCA9548A_EOK      | 成功                                |
| TCA9548A_EINVAL   | 参数无效                            |

## 查找设备

`tca9548a_dev_t tca9548a_find(const char *name)`

通过设备名称查找已初始化的 TCA9548A 设备。

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|`name`             | 设备名称                            |
| **返回**          | **描述**                            |
| 非 RT_NULL        | 设备句柄                            |
| RT_NULL           | 未找到该名称的设备                  |

## 选择通道

`rt_err_t tca9548a_select_channel(tca9548a_dev_t dev, rt_uint8_t channels)`

选择指定的通道，支持任意通道组合。

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|`dev`              | 设备句柄                            |
|`channels`         | 通道位掩码                          |
| **返回**          | **描述**                            |
| TCA9548A_EOK      | 成功                                |
| TCA9548A_EINVAL   | 参数无效                            |
| TCA9548A_EIO      | I2C 通信错误                        |

## 读取通道状态

`rt_err_t tca9548a_read_channel(tca9548a_dev_t dev, rt_uint8_t *channels)`

从设备读取当前通道选择状态。

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|`dev`              | 设备句柄                            |
|`channels`         | 输出：当前通道选择位掩码             |
| **返回**          | **描述**                            |
| TCA9548A_EOK      | 成功                                |
| TCA9548A_EINVAL   | 参数无效                            |
| TCA9548A_EIO      | I2C 通信错误                        |

## 启用单个通道

`rt_err_t tca9548a_enable_channel(tca9548a_dev_t dev, rt_uint8_t channel)`

启用指定通道（保持其他通道状态不变）。

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|`dev`              | 设备句柄                            |
|`channel`          | 通道编号 (0-7)                      |
| **返回**          | **描述**                            |
| TCA9548A_EOK      | 成功                                |
| TCA9548A_EINVAL   | 参数无效                            |
| TCA9548A_EIO      | I2C 通信错误                        |

## 禁用单个通道

`rt_err_t tca9548a_disable_channel(tca9548a_dev_t dev, rt_uint8_t channel)`

禁用指定通道（保持其他通道状态不变）。

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|`dev`              | 设备句柄                            |
|`channel`          | 通道编号 (0-7)                      |
| **返回**          | **描述**                            |
| TCA9548A_EOK      | 成功                                |
| TCA9548A_EINVAL   | 参数无效                            |
| TCA9548A_EIO      | I2C 通信错误                        |

## 禁用所有通道

`rt_err_t tca9548a_disable_all(tca9548a_dev_t dev)`

禁用所有通道。

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|`dev`              | 设备句柄                            |
| **返回**          | **描述**                            |
| TCA9548A_EOK      | 成功                                |
| TCA9548A_EINVAL   | 参数无效                            |

## 启用所有通道

`rt_err_t tca9548a_enable_all(tca9548a_dev_t dev)`

启用所有 8 个通道。

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|`dev`              | 设备句柄                            |
| **返回**          | **描述**                            |
| TCA9548A_EOK      | 成功                                |
| TCA9548A_EINVAL   | 参数无效                            |

## 复位

`rt_err_t tca9548a_reset(tca9548a_dev_t dev)`

复位设备（禁用所有通道）。

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|`dev`              | 设备句柄                            |
| **返回**          | **描述**                            |
| TCA9548A_EOK      | 成功                                |
| TCA9548A_EINVAL   | 参数无效                            |

## 检查连接

`rt_bool_t tca9548a_is_connected(tca9548a_dev_t dev)`

检查设备是否在 I2C 总线上可访问。

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|`dev`              | 设备句柄                            |
| **返回**          | **描述**                            |
| RT_TRUE           | 设备可访问                          |
| RT_FALSE          | 设备不可访问或参数无效              |

## 使用示例

```c
#include "tca9548a.h"

int main(void)
{
    tca9548a_dev_t dev;

    /* 通过名称查找设备（在 menuconfig 中配置后自动初始化） */
    dev = tca9548a_find("tca9548a_0");
    if (dev == RT_NULL)
    {
        rt_kprintf("Device not found!\n");
        return -1;
    }

    /* 选择通道 0 */
    tca9548a_select_channel(dev, TCA9548A_CHANNEL_0);

    /* 现在可以访问通道 0 上的下游 I2C 设备 */

    /* 同时选择通道 0、3、7 */
    tca9548a_select_channel(dev, TCA9548A_CHANNEL_0 | TCA9548A_CHANNEL_3 | TCA9548A_CHANNEL_7);

    /* 禁用所有通道 */
    tca9548a_disable_all(dev);

    return 0;
}
```

## 多设备配置（menuconfig）

在 menuconfig 中可以配置最多 8 个 TCA9548A 设备，每个设备可以：
- 挂载在不同的 I2C 总线上
- 使用不同的 I2C 地址
- 有独立的设备名称

配置完成后，系统启动时自动初始化所有设备，通过 `tca9548a_find("设备名")` 获取句柄。