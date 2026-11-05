# SpectraVita 多传感器采集系统：BK3633固件与Windows/Android上位机

本仓库包含基于 BK3633 SDK 的多传感器采集固件，以及 Windows / Android 共用的 Flutter 上位机源码。设备通过 I²C 周期采集多光谱、多光谱 FIFO、紫外、接近和加速度数据，保存到板载 W25Q64，再通过自定义 BLE GATT 服务上传至上位机。

## 获取源码与工程入口

```powershell
git clone https://github.com/xcxunsnsn/BK3633-SpectraVita.git
cd BK3633-SpectraVita
```

| 目录 | 内容 | 开发入口 |
| --- | --- | --- |
| `projects/app_gatt/` | BK3633应用、五类传感器、Flash存储及V4协议 | `prj3633.uvproj`，Keil `Target1` |
| `src/` | BK3633 SDK、底层驱动及库 | 由Keil工程引用 |
| `tests/acc_fifo_host/` | 固件主机回归测试 | `run_tests.cmd` |
| `SpectraVita_App/` | Windows与Android共用的上位机源码 | `pubspec.yaml`，Flutter工程根目录 |

Windows和Android共用`SpectraVita_App/lib/`中的界面、协议和BLE业务代码，平台配置分别位于`windows/`和`android/`。打开整个`SpectraVita_App`目录，按[上位机开发与构建说明](SpectraVita_App/README.md)配置环境。

```powershell
cd SpectraVita_App
flutter pub get
flutter analyze
flutter test
flutter build windows --release
flutter build apk --debug
```

上位机原验证环境为Flutter 3.47.2 / Dart 3.13.2。Windows需要Visual Studio C++桌面开发组件，Android需要Android Studio/SDK；两端输出目录与签名注意事项见上位机README。2026-11-05源码交付副本通过静态分析和全部46项测试。

下面介绍设备固件。固件生成镜像、上位机构建缓存和安装包不作为源码提交；固件编译必需的boot/stack镜像、转换工具，以及Android Gradle Wrapper、字体与许可证保留在仓库内。

历史汇报材料可能记录较早实现；当前行为以源码、本文和[协议说明](projects/app_gatt/PROTOCOL_V4_NOTES.md)为准。

固件主机测试可执行`tests/acc_fifo_host/run_tests.cmd`。该脚本中的Visual Studio开发环境路径，以及根目录两个`Keil_Prj_check_*.bat`中的Keil路径，需要按本机安装位置调整；这不影响直接在Keil中打开主工程构建。

当前阶段已经完成并通过基础硬件测试：

- 五种传感器的单次读取；
- 五套互不覆盖的周期采集计划；
- 五种传感器独立的外部 Flash 一天日志；
- 按传感器类型连续多包上传历史数据；
- 采集前上电、采集后待机或掉电；
- FEE2 命令下发和 FEE4 Notify 数据返回；
- Keil MDK 编译：`0 Error(s), 0 Warning(s)`。

> 外部 Flash 版本已经通过 Keil 编译，仍需在实板上确认 JEDEC ID、24 小时记录数量及整日多包上传。

## 1. 硬件组成

| 器件 | 功能 | I²C 7 位地址 |
| --- | --- | ---: |
| BK3633 | 主控与 BLE | — |
| AS7341 | 多光谱与 Flicker FIFO | `0x39` |
| AS7331 | UVA/UVB/UVC | 默认 `0x74`，驱动支持扫描 `0x74～0x77` |
| WH4530A | 接近检测 | `0x38` |
| ADXL345 | 三轴加速度 | `0x53` |
| BQ21080 | 电源/充电管理 | `0x6A` |

传感器共用 BK3633 的 I²C 总线，并共用 `VDD_1P8_PIN`、`VDD_SENSOR_PIN` 控制的传感器电源域。AS7341 普通多光谱与 FIFO 使用同一颗芯片，固件会避免两种任务同时访问它。

## 2. 工程位置和开发环境

Keil 工程：

```text
projects/app_gatt/prj3633.uvproj
```

当前使用环境：

- Keil µVision 5（按本机安装路径打开）；
- ARM Compiler 5.06 update 6；
- MDK v5.25 Legacy Device Support：ARM7、ARM9 与 Cortex-R；
- Beken Writer V2.00；
- UART：`115200, 8-N-1`。

### 编译

1. 使用 Keil 打开 `projects/app_gatt/prj3633.uvproj`。
2. 选择 `Target1`。
3. 执行 **Rebuild**。
4. 确认 Build Output 显示 `0 Error(s), 0 Warning(s)`。

编译完成后生成烧录文件：

```text
projects/app_gatt/output/app/bk3633_app_merge_crc.bin
```

`output/app` 属于本地编译产物并被 Git 忽略，因此从 GitHub 克隆工程后需要先自行编译。

### 烧录

在 Beken Writer 中：

1. 烧录对象选择 `BK3633`；
2. 选择板卡对应串口；
3. 通讯波特率使用已验证的 `921600`；
4. 烧录文件选择 `bk3633_app_merge_crc.bin`；
5. 勾选“烧录主文件”和“烧录后运行”；
6. 开发测试时不要勾选烧录 eFuse、更新授权密钥或其他不可逆选项。

如果出现“读取 Flash ID/设置波特率失败”，先检查板卡供电、BOOT/烧录状态、串口占用和 TX/RX 接线。

## 3. 软件数据链路

```text
手机 APP
  │ FEE2 Write Without Response
  ▼
app_fee0.c：接收并打印原始命令
  ▼
app_user.c：校验协议帧、调度传感器
  ▼
task_mss/fifo/uv/dist/acc.c：完成一次采集
  ▼
driver_AS7341/AS7331/WH4530/ADXL345.c：I²C 访问
  ▼
app_user.c：立即应答或写入 W25Q64 历史
  ▼
FEE4 Notify
  ▼
手机 APP
```

单次读取命令会在测量完成后立即返回；周期采集只保存到外部 Flash，必须由 APP 再发送 `0x50` 请求上传。

## 4. BLE 服务和应用帧

- 广播名称：`SpectraVita`
- 自定义服务：`0xFEE0`
- APP 写入：`0xFEE2`，Write Without Response
- 设备返回：`0xFEE4`，Notify

手机连接后必须先打开 FEE4 Notify，再向 FEE2 发送 HEX 命令。

应用帧格式：

```text
AA | CMD | DATA... | XOR | BB
```

- 多字节整数均采用小端序；
- `XOR` 是从 `CMD` 到最后一个 `DATA` 字节的逐字节异或；
- `AA` 和 `BB` 不参与异或计算。

例如：

```text
48 XOR 03 = 4B
AA 48 03 4B BB
```

## 5. 已实现命令

| APP 请求 | 设备响应 | 功能 | 响应 DATA 长度 |
| --- | --- | --- | ---: |
| `0x40` | `0x41` | 时间戳、电量、固件版本 | 6 |
| `0x42` | `0x43` | AS7341 手动多光谱 | 20 |
| `0x44` | `0x45` | AS7341 Flicker FIFO | 256 |
| `0x46` | `0x47` | AS7331 紫外 | 8 |
| `0x48` | `0x49` | WH4530A 接近 A/B | 4 |
| `0x4A` | `0x4B` | ADXL345 三轴加速度 | 6 |
| `0x4E` | `0x4F` | 配置一种传感器的周期计划 | 1 |
| `0x50` | `0x51` | 连续上传一种传感器的 Flash 历史 | `4 + N` |
| `0x52` | `0x53` | 停止全部周期计划 | 0 |

常用单次读取命令：

```text
查询设备：  AA 40 40 BB
多光谱：    AA 42 1D 57 02 0A 00 BB
FIFO：      AA 44 D0 02 05 93 BB
紫外：      AA 46 09 4F BB
接近：      AA 48 03 4B BB
加速度：    AA 4A 4A BB
```

当前 `0x43` 数据为 `F1～F8 + Clear + NIR`，共 10 个 `uint16_t`、20 字节。APP已经在`0x42`中指定AGAIN，因此应答不重复返回增益；周期自动采集的多光谱Flash记录仍保留实际自动增益，共22字节。`0x49`返回两个不同增益下的接近原始值A/B，共4字节。

AS7341 FIFO按官方公式`tINT_FD=FD_TIME×2.78 µs`计时。默认`FD_TIME=720`时
间隔2.0016 ms、约499.6003 Hz，128次积分约256.2048 ms。固件每10 ms按
`FIFO_LVL`分段搬运，累计128点后立即停止，正常约260 ms完成；不再固定等待
700 ms。内部FIFO容量为128点，第129点会置溢出并丢弃新数据，因此驱动会检查
`STATUS6.FIFO_OV`；清FIFO使用`CONTROL` bit1。

## 6. 五种传感器的独立周期

正式周期统一定义在：

```text
projects/app_gatt/config/sensor_period_config.h
```

| 传感器 | 周期宏 | 当前周期 |
| --- | --- | ---: |
| 多光谱 | `SENSOR_SAMPLE_PERIOD_MSS_S` | 60 秒 |
| FIFO | `SENSOR_SAMPLE_PERIOD_FIFO_S` | 600 秒 |
| 紫外 | `SENSOR_SAMPLE_PERIOD_UV_S` | 60 秒 |
| 接近 | `SENSOR_SAMPLE_PERIOD_DIST_S` | 600 秒 |
| 加速度 | `SENSOR_SAMPLE_PERIOD_ACC_S` | LOW 1 Hz / HIGH 50 Hz |

当前 `SENSOR_SAMPLE_PERIOD_STRICT=1`。APP 下发的周期必须与配置文件中的宏一致，否则设备返回失败。

### 0x4E 格式

```text
AA 4E TIMESTAMP(4) PERIOD_S(2) MSS FIFO UV DIST ACC XOR BB
```

- `TIMESTAMP`：4 字节 Unix 秒，小端序；
- `PERIOD_S`：当前命令所选传感器的实际周期，2 字节小端序；
- 后面五个字节是独占选择位；
- 每条命令必须且只能有一个选择位为 `1`；
- 连续发送五条命令即可建立五套独立计划，后一条不会覆盖前面四条。

测试时可暂时把时间戳设为 0：

```text
多光谱 60 秒： AA 4E 00 00 00 00 3C 00 01 00 00 00 00 73 BB
FIFO 600 秒：  AA 4E 00 00 00 00 58 02 00 01 00 00 00 15 BB
紫外 60 秒：   AA 4E 00 00 00 00 3C 00 00 00 01 00 00 73 BB
接近 600 秒：  AA 4E 00 00 00 00 58 02 00 00 00 01 00 15 BB
加速度 1 秒：  AA 4E 00 00 00 00 01 00 00 00 00 00 01 4E BB
```

每条成功时，FEE4 返回：

```text
AA 4F 00 4F BB
```

串口依次显示：

```text
V4 schedule sensor=1, period=60 s
V4 schedule sensor=2, period=600 s
V4 schedule sensor=3, period=60 s
V4 schedule sensor=4, period=600 s
V4 schedule sensor=5, period=1 s
```

正式 APP 应在命令中填写当前 Unix 时间戳，而不是一直使用 0。

## 7. 停止和历史上传

停止全部后续周期采集：

```text
发送：AA 52 52 BB
返回：AA 53 53 BB
```

停止命令不会清空已经保存的历史。当前正在进行的测量会自然结束并让传感器休眠，但其周期保存标志会被清除，因此该组数据不会写入历史。

按类型上传：

```text
多光谱：AA 50 01 51 BB
FIFO：  AA 50 02 52 BB
紫外：  AA 50 03 53 BB
接近：  AA 50 04 54 BB
加速度：AA 50 05 55 BB
```

`0x51` 当前格式：

```text
AA 51 第一组时间戳(4) 记录1 记录2 ... 记录N XOR BB
```

记录数计算：

```text
记录数 = (完整 0x51 帧长度 - 8) / 单组长度
```

| 类型 | 单组长度 | 正式周期下每天组数 | 每包最多组数 | 最大 0x51 帧长 |
| --- | ---: | ---: | ---: | ---: |
| 多光谱 | 22 | 1440 | 13 | 294 |
| FIFO | 256 | 144 | 1 | 264 |
| 紫外 | 8 | 1440 | 36 | 296 |
| 接近 | 4 | 144 | 73 | 300 |
| 加速度历史 | 7（状态+XYZ） | 动态 | 41 | 295 |

FIFO的256字节数据保存在268字节Flash槽位中。W25Q读取保持CS有效并把数据阶段拆为最多64字节的连续SPI RX，避免BK3633长轮询读取中途返回；CRC首次不符会完整重读一次，连续失败时中止上传且保留历史。

加速度历史每组的第1字节是实际采样速率标记：`00=LOW 1 Hz`、`01=HIGH 50 Hz`，后6字节才是X/Y/Z。LOW下传感器每秒实际采集并保存1组；进入LOW后的前2组样本为保护期，清除旧中断后才开放硬件Activity并允许切到HIGH。HIGH采用Watermark=16和100 ms兜底轮询；P20 ISR首次命中立即屏蔽GPIO，主循环每轮最多读取8组，确认FIFO低于水位且INT1释放后再重开。数据进入128条RAM队列，Flash以最多8条为一批异步提交；队列低于32条时每20 ms推进，高于等于32条或排空时每10 ms推进，定时器只置待办且BLE始终先执行。HIGH由硬件Inactivity优先退出，软件兜底按三轴最大绝对差连续100组判断。四类低速传感器同时到期时每秒最多启动一个；停止或上传异步排空，协议不变。

## 8. 低功耗流程

每次周期采集遵循：

```text
周期到达
→ 打开 VDD_1P8/VDD_SENSOR
→ 初始化并启动指定传感器
→ 等待测量完成
→ 读取并保存一组数据
→ 传感器进入 Standby/Power Down
→ 所有传感器任务空闲后关闭公共电源
```

具体处理：

- AS7331：停止测量并进入 Power Down；
- ADXL345周期模式：LOW为Sleep/Wakeup 1 Hz，HIGH为50 Hz Stream FIFO、Watermark=16；停止/上传时进入Standby；
- WH4530A：完成 A/B 两个增益测量后关闭；
- AS7341：多光谱/FIFO完成后关闭测量模块；
- BK3633：BLE和任务空闲时进入 `MCU_REDUCE_VO_SLEEP`。

当前公共调度器仍每秒唤醒一次BK3633累计五种周期，因此还有进一步优化空间。最终功耗应断开USB转串口后，用功耗仪或示波器测量确认。

## 9. 已完成的联调验证

当前板卡与手机 GATT 工具已经验证：

- `AA 4A 4A BB`能够读取并返回10字节加速度帧；
- 原RAM版本已验证加速度1秒周期保存并上传20组数据；
- 紫外60秒周期运行约5分钟后能够上传5组数据；
- 五条`0x4E`连续配置后，多种传感器计划能够同时保留；
- 联合测试得到多光谱12组、FIFO 1组、紫外多组、接近1组和加速度20组；
- 当前手机能够完整接收272字节多光谱历史帧和264字节FIFO历史帧。

外部Flash版本已完成Keil全量编译（0 Error、0 Warning），但W25Q64 JEDEC识别、断电恢复和完整一天上传仍需在实板上验证。

HEX数据中出现较多`00`通常是正常的小端格式。例如`0F 00`表示`0x000F=15`，不是两个无效字节。只有完整字段持续为`00 00`时，才需要结合光照、距离和增益进一步检查传感器数据。

## 10. 主要代码位置

```text
projects/app_gatt/
├─ config/
│  ├─ user_config.h                板级配置与传感器默认参数
│  ├─ sensor_period_config.h       五种正式采样周期
│  └─ acc_motion_config.h          加速度阈值、FIFO和LOW/HIGH参数
├─ app/
│  ├─ api/app_user.h               任务和协议公共定义
│  └─ src/
│     ├─ app_fee0.c                FEE2/FEE4 GATT入口
│     ├─ app_user.c                V4协议、周期调度、Flash上传、低功耗
│     ├─ ext_flash.c               W25Q64底层SPI读写/擦除/休眠
│     ├─ sensor_flash_store.c      五分区日志、时间戳与CRC
│     ├─ task_mss.c                AS7341多光谱状态机
│     ├─ task_fifo.c               AS7341 FIFO状态机
│     ├─ task_uv.c                 AS7331状态机
│     ├─ task_dist.c               WH4530A状态机
│     ├─ task_acc.c                ADXL345状态机
│     └─ driver_*.c                各器件I²C驱动
├─ prj3633.uvproj                  Keil工程
├─ PROTOCOL_V4_NOTES.md            协议与分层调试详解
└─ SENSOR_DEFAULTS.md              默认测量参数说明
```

## 11. 当前限制

- `0x51`没有TYPE、记录数、包序号、总包数和结束标志，APP需结合上一条`0x50 TYPE`及通知静默判断完成；
- 当前只有BLE协议栈发送确认，没有APP业务层持久化ACK；
- 中途断线可保留数据并从头重传，但暂不支持从中间包断点续传；
- FIFO固定上传256字节，没有携带实际有效长度；正常路径严格收满128点，只有启动、I²C或超时异常时才以0填充未收满尾部；
- 大帧依赖足够的ATT MTU，小MTU手机需要应用层分片；
- 每条`0x4E`都会重新设置公共软件时钟，正式APP应保证时间戳正确且单调；
- 加速度HIGH为50 Hz，但当前`0x51`每包只有一个Unix秒时间戳；APP可按状态字节识别采样率，若要恢复精确20 ms时刻，协议还需增加毫秒时间或序号；
- 当前上传的是驱动输出值，物理单位、标定和数据精度不属于本阶段验收范围。

更完整的协议、帧长度、抓包方法和排查流程见 [PROTOCOL_V4_NOTES.md](projects/app_gatt/PROTOCOL_V4_NOTES.md)。
