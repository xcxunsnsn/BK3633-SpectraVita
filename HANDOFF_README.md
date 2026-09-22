> **已停用：** 本文件仅保留为历史资料，不再维护，也不是当前状态来源。跨会话继续开发必须读取工作区根目录 `../HANDOFF.md`。

# BK3633 SpectraVita 工程交接说明（历史）

> 以下内容是旧交接快照，可能已经过期，仅供追溯；其中关于禁止清理未提交改动的警告仍然有效。

## 1. 工程位置与构建

- 工作目录：`D:\BK-BLE-Project\MSS_2512`
- Keil工程：`projects\app_gatt\prj3633.uvproj`
- Keil：`D:\keil\UV4\UV4.exe`
- 烧录文件：`projects\app_gatt\output\app\bk3633_app_merge_crc.bin`
- BK3633烧录起始地址：`0x00000000`
- 运行日志串口：`115200, 8N1`（BK Writer下载速率由工具管理）

命令行全量编译：

```powershell
cd D:\BK-BLE-Project\MSS_2512\projects\app_gatt
& 'D:\keil\UV4\UV4.exe' -b 'prj3633.uvproj' -j0 -o 'build.log'
```

2026-09-09 Flicker FIFO Flash读取修复版全量重编译结果：`0 Error(s), 0 Warning(s)`。W25Q数据阶段在同一CS事务中按最多64字节执行checked RX，268字节FIFO槽位拆为`64+64+64+64+12`；4096字节扫描仍保留两个2048字节外层事务。部分读取会明确失败，记录CRC首次不符时完整重读一次，失败上传不清历史。烧录文件`bk3633_app_merge_crc.bin`为236256字节，SHA-256：`E553FED12DF7A395A3561BDF0E769FF4E1A06B1E52605694207E6356AB4BB5B9`。主机与Keil测试通过，实板启动应显示`rx_chunk=64`，现有TYPE=2历史可直接重试上传。

## 2. BLE协议入口

- 服务：`FEE0`
- APP写入：`FEE2`（Write Without Response）
- 设备通知：`FEE4`（Notify，APP必须先订阅）
- 帧：`AA | CMD | DATA | XOR | BB`
- XOR从CMD开始计算，到DATA最后一字节结束，不包含`AA/BB`。

常用命令：

```text
查询设备：    AA 40 40 BB
多光谱单次：  AA 42 1D 57 02 0A 00 BB
FIFO单次：    AA 44 D0 02 05 93 BB
紫外单次：    AA 46 09 4F BB
接近单次：    AA 48 03 4B BB
加速度单次：  AA 4A 4A BB
停止全部周期：AA 52 52 BB

上传多光谱：  AA 50 01 51 BB
上传FIFO：    AA 50 02 52 BB
上传紫外：    AA 50 03 53 BB
上传接近：    AA 50 04 54 BB
上传加速度：  AA 50 05 55 BB
```

单次加速度命令仍是4字节`AA 4A 4A BB`，没有改变。15字节的`0x4E`是原协议中的周期采集配置，不是单次命令。

## 3. 周期采集

`0x4E`格式：

```text
AA 4E TIMESTAMP_LE(4) PERIOD_LE(2) MSS FIFO UV DIST ACC XOR BB
```

每条命令只能有一个传感器选择字节为`01`；要启动全部传感器，需要连续发送五条`0x4E`。当前正式周期：

| 传感器 | 周期 |
| --- | ---: |
| 多光谱 | 60秒 |
| FIFO | 600秒 |
| 紫外 | 60秒 |
| 接近 | 600秒 |
| 加速度 | LOW 1 Hz / HIGH 50 Hz |

时间戳为0的示例：

```text
多光谱：AA 4E 00 00 00 00 3C 00 01 00 00 00 00 73 BB
FIFO：  AA 4E 00 00 00 00 58 02 00 01 00 00 00 15 BB
紫外：  AA 4E 00 00 00 00 3C 00 00 00 01 00 00 73 BB
接近：  AA 4E 00 00 00 00 58 02 00 00 00 01 00 15 BB
加速度：AA 4E 00 00 00 00 01 00 00 00 00 00 01 4E BB
```

成功返回`AA 4F 00 4F BB`，失败返回`AA 4F 01 4E BB`。

周期及默认测量参数位于：

- `projects/app_gatt/config/sensor_period_config.h`
- `projects/app_gatt/config/user_config.h`

## 4. 外部Flash

- 器件：板载W25Q64兼容Flash，8 MiB。
- 实板已读到`JEDEC=C8 60 17, ready=1`。
- 周期数据不立即BLE上传，而是写入外部Flash。
- `0x50 TYPE`逐包上传该类型全部历史。
- 上传期间停止触发新采集；暂停加速度前先冻结并保存HIGH中不足16组的FIFO尾部和RAM待写队列。
- 任一传感器的Flash分区写满后立即停止其周期计划，不循环覆盖旧记录；加速度同时停止Stream。上传清除后需重新发送对应`0x4E`才会再次采集。
- 最后一包收到BLE协议栈成功确认后才清除该类型日志。
- 中途断线不清除Flash，重连后从第一包重新上传。

主要文件：

- `app/src/ext_flash.c`：SPI Flash底层驱动和非阻塞擦除/页编程原语
- `app/src/sensor_flash_store.c`：分区、记录、CRC、断电扫描恢复和加速度异步批次
- `app/src/period_due_queue.c`：四类低速传感器到期队列、延迟和合并统计
- `app/src/app_user.c`：协议、调度、Flash上传状态机

## 5. 加速度新实现

ADXL345周期模式采用真正的动态采样率：

```text
LOW：Sleep/Wakeup 1 Hz → 每组直接写Flash（STATE=00）
                         ↓ 保存2组后开放Activity，触发约0.31g
HIGH：50 Hz Stream FIFO → Watermark=16 → 先入RAM队列再全部写Flash（STATE=01）
                         ↓ 硬件Inactivity约2秒，或软件连续100个安静样本
                        回到LOW
```

- LOW：传感器每秒实际采集并保存1组，不再从50 Hz数据中抽取。
- HIGH：传感器实际以50 Hz采样，FIFO中的每组先进入128条RAM队列，再分批顺序写Flash。
- 历史单组7字节：`STATE + X(int16 LE) + Y(int16 LE) + Z(int16 LE)`。
- 单次`0x4B`仍只有6字节XYZ，不带状态。
- LOW进入后先保存2个1 Hz样本，再清除旧中断并开放ADXL345硬件Activity；初始阈值寄存器为5（约0.31g），保护期内的Activity只统计、不切换。
- XZ原理图为`INT1->P20`、`INT2->P37`；代码已按此修正，Watermark映射到INT1。
- Watermark为16，另有100 ms轮询兜底；ISR先屏蔽P20，主循环每次最多读取8组，降到Watermark以下且INT1为低后才重开，阻止高电平源反复进入ISR。加速度Flash擦除/编程发出命令后立即返回，后续主循环每次只查询或推进一个步骤。
- HIGH主退出条件是ADXL345硬件Inactivity：AC耦合XYZ、阈值寄存器3（约0.1875g）、时间2秒；软件兜底为相邻样本变化量连续100组低于30 LSB，约2秒。任一运动样本会清零软件安静计数。
- HIGH退出、停止、上传或重配统一先停止新样本，再排空FIFO和RAM队列；失败时不返回成功停止响应。
- 这属于运动强弱识别，不是经过积分得到的物理速度m/s。

参数：`projects/app_gatt/config/acc_motion_config.h`

主要实现：

- `app/src/driver_ADXL345.c`：LOW 1 Hz、Activity、HIGH 50 Hz、Stream/FIFO
- `app/src/task_acc.c`：动态采样状态机、FIFO/RAM解耦、100 ms兜底、停止排空和统计
- `app/src/app_user.c`：7字节加速度历史写Flash、上传暂停/恢复

加速度使用剩余7 MiB Flash，约365364组：可覆盖全天LOW并额外容纳约95分钟累计HIGH；无法保存全天连续50 Hz。

### 加速度尚需实板验证

发送加速度`0x4E`后串口应出现：

```text
ACC mode -> LOW, ODR=1Hz, activity_reg=5, guard=2
ACC LOW activity armed after 2 samples
V4 schedule sensor=5, period=1 s
```

还应出现`ACC FIFO5 ready: INT1=P20 WM=16 slice=8 RAM=128 batch=8 async-flash irq-mask flash=20/10ms guard=2 inact=3/2s`。晃动时出现
`ACC mode -> HIGH, ODR=50Hz, watermark=16, inact=3/2s`，静止约2～3秒后应出现
`ACC HIGH exit -> LOW, source=HW_INACT`（硬件判断）或`source=SW_QUIET`（软件兜底），随后重新进入LOW的2样本保护期。
统计行应满足`overrun=0`；正常停止时应满足`read=saved`、`q=0`，且`batch`远小于`saved`。
BLE连接参数现为30～50 ms、latency 0、监督超时10秒。断线时新增`BLE timing`行，包含
主循环最大间隔、页编程和扇区擦除次数及最大耗时。

主机模拟测试：`tests\acc_fifo_host\run_tests.cmd`。覆盖中断屏蔽/重开、INT1持续高、屏蔽期间Inactivity、漏中断轮询、Flash长忙、异步擦写、60/600秒错峰、
环形队列回绕、尾部排空、8条两阶段写、页/扇区边界、掉电提交失败和溢出故障；两个测试程序均通过。

## 6. 当前优先问题：WH4530A接近数据为0

接近响应格式：

```text
AA 49 A_L A_H B_L B_H XOR BB
```

- A固定使用增益代码0（1x），作为基准值。
- B使用`0x48`命令DATA低2位指定的增益：0/1/2/3对应1x/2x/4x/8x。
- 发送`AA 48 03 4B BB`时形成A=1x、B=8x对比；发送`AA 48 00 48 BB`时两者都是1x。

当前检查发现的代码风险：

1. `driver_WH4530.c`的`WH4530A_IIC_Read_Distance()`不检查`i2c_read()`返回值，I²C失败可能被当作数据0。
2. 读缓冲区`uint8_t data[2]`没有初始化。
3. 初始化及启动寄存器写入没有回读验证。
4. 代码固定等待约200 ms后读`0x18/0x19`，没有检查数据就绪标志；未完成转换时可能读到`0000`。
5. `WH4530_Init()`注释称执行`0x80`复位，但实际向`0x00`写的是`0x00`，需结合WH4530A手册确认。

下一步应先制作诊断版，不要直接猜测寄存器：

- 检查每次I²C读写结果；
- 回读并串口打印`0x00、0x06、0x07、0x08、0x09`；
- 打印`0x18/0x19`原始字节；
- 临时把两次测量等待延长到500 ms，判断是否为转换时间不足；
- 如果有WH4530A数据手册，按手册确认复位位和数据就绪位，再改成轮询就绪。

相关文件：

- `projects/app_gatt/app/src/driver_WH4530.c`
- `projects/app_gatt/app/src/task_dist.c`
- BK3633 I²C底层：`src/driver/i2c/i2c.c`

继续排查时需要用户提供：单次接近响应原始HEX、从上电到发送命令的完整串口日志、是否同时启动了加速度周期采集，以及WH4530A数据手册。

## 7. 其他协议待确认项

1. `0x51`没有TYPE、记录数、包序号、总包数和明确结束标志；APP必须记住上一条`0x50 TYPE`。
2. 目前以BLE协议栈发送成功作为擦除Flash条件，没有APP业务层ACK。
3. 断线后从第一包重传，不支持中间包断点续传。
4. 加速度HIGH为50 Hz，但`0x51`每包只有一个Unix秒时间戳；若要还原每组20 ms精确时刻，需要扩展协议。
5. 加速度LOW/HIGH阈值仍需实物标定。

## 8. Git状态

- 远端：`https://github.com/xcxunsnsn/BK3633-SpectraVita.git`
- 当前HEAD：`2ec4e0f`，提交时间`2026-08-13 15:01:49 +0800`
- 外部Flash、加速度Stream/Watermark和文档更新目前都只在本地工作区，尚未提交或推送。
- 工作区还包含用户已有的文件删除和其他修改；继续处理时必须先执行`git status --short`并逐项确认，不能覆盖或还原不相关改动。
