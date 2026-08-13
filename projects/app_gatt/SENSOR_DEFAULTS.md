# 传感器默认参数

所有上电默认测量参数集中定义在 `config/user_config.h`：

| 宏 | 默认值 | 含义 |
| --- | ---: | --- |
| `SENSOR_DEFAULT_MSS_ATIME` | 29 | AS7341 光谱积分 ATIME |
| `SENSOR_DEFAULT_MSS_ASTEP` | 599 | AS7341 光谱积分 ASTEP |
| `SENSOR_DEFAULT_MSS_GAIN` | 10 | AS7341 光谱增益档位 |
| `SENSOR_DEFAULT_FIFO_TIME` | 720 | AS7341 FIFO 积分参数 |
| `SENSOR_DEFAULT_FIFO_GAIN` | 5 | AS7341 FIFO 增益档位 |
| `SENSOR_DEFAULT_UV_GAIN` | 9 | AS7331 紫外增益档位 |
| `SENSOR_DEFAULT_DIST_GAIN` | 0 | WH4530A 接近增益档位 |

修改这些宏后需要重新编译和烧录。APP 通过协议 V4 的
`0x42/0x44/0x46/0x48` 命令发送参数时，会在运行期间覆盖相应默认值，
因此日常参数测试不需要修改固件。
