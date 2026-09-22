/*
 * ADXL345 低/高速运动识别参数。
 * 后续实测调参只改本文件，不需要进入任务状态机查找常量。
 */
#ifndef ACC_MOTION_CONFIG_H_
#define ACC_MOTION_CONFIG_H_

/* LOW使用Sleep/Wakeup的1Hz输出；HIGH使用50Hz Stream FIFO。 */
#define ACC_LOW_ODR_HZ                    1U
#define ACC_HIGH_ODR_HZ                   50U
#define ACC_HIGH_BW_RATE_REG              0x19U /* LOW_POWER=1, RATE=50Hz */
#define ACC_FIFO_WATERMARK_SAMPLES        16U
#define ACC_FIFO_READ_SLICE_SAMPLES       8U /* one I2C slice before returning to BLE */
#define ACC_FIFO_POLL_TICKS               10U /* 100 ms, kernel timer unit is 10 ms */
#define ACC_FLASH_SERVICE_HS              32U /* 10 ms in 312.5 us half-slots */
#define ACC_LOW_DUPLICATE_HS              3000U /* 937.5 ms: suppress same LOW sample */
#define ACC_RAM_QUEUE_SAMPLES             128U
#define ACC_RAM_QUEUE_WARN_SAMPLES        96U
#define ACC_RAM_QUEUE_WARN_RESET_SAMPLES  64U
#define ACC_RAM_QUEUE_YIELD_SAMPLES       32U /* below this, one low-rate sensor may use Flash */
#define ACC_COMMIT_BATCH_SAMPLES          8U
#define ACC_FLASH_STEP_NORMAL_TICKS       2U /* 20 ms while queue pressure is low */
#define ACC_FLASH_STEP_URGENT_TICKS       1U /* 10 ms at q>=32 or while draining */
#define ACC_STATS_HS                      32000U /* 10 seconds */

/* LOW切换后的前2个实际1Hz样本不开放Activity，过滤模式切换瞬态。 */
#define ACC_LOW_ACTIVITY_GUARD_SAMPLES    2U

/* HIGH硬件Inactivity：62.5mg/LSB，3约0.1875g；时间单位1秒。 */
#define ACC_HIGH_INACT_THRESHOLD_REG      3U
#define ACC_HIGH_INACT_TIME_REG           2U
#define ACC_HIGH_INACT_CTL_REG            0x0FU /* AC coupled, XYZ enabled */

/* THRESH_ACT单位为62.5mg/LSB，5约等于0.31g；使用交流耦合XYZ三轴检测。 */
#define ACC_LOW_ACTIVITY_THRESHOLD_REG    5U
#define ACC_LOW_ACTIVITY_CTL_REG          0xF0U

/* 运动状态标记，作为加速度历史记录的第1字节上传。 */
#define ACC_MOTION_MARK_LOW               0x00U
#define ACC_MOTION_MARK_HIGH              0x01U

/*
 * HIGH退出判定量：FIFO中相邻样本三轴绝对差值的最大轴。
 * ADXL345全分辨率模式约3.9mg/LSB，30LSB约0.12g。
 * LOW进入HIGH由上面的硬件Activity阈值判断。
 */
#define ACC_MOTION_EXIT_DELTA_LSB         30U
#define ACC_SOFT_QUIET_SAMPLES            100U /* 50Hz下连续约2秒 */

/* LOW每秒实际采集并保存1组；HIGH每秒实际采集并保存50组。 */
#define ACC_LOW_SAMPLE_PERIOD_S           1U

/* 调试开关：1时串口打印LOW/HIGH切换和FIFO溢出，正式低功耗可改为0。 */
#define ACC_MOTION_DEBUG                   1U

#endif /* ACC_MOTION_CONFIG_H_ */
