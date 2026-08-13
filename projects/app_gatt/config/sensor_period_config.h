/*
 * 五种传感器的正式采样周期配置。
 * 后续老师修改周期时优先改本文件，不需要进入 app_user.c 查找。
 */
#ifndef SENSOR_PERIOD_CONFIG_H_
#define SENSOR_PERIOD_CONFIG_H_

/* 单位均为秒，数值范围为 1～65535。 */
#define SENSOR_SAMPLE_PERIOD_MSS_S       60U   /* AS7341 多光谱：1 分钟 */
#define SENSOR_SAMPLE_PERIOD_FIFO_S      600U  /* AS7341 FIFO：10 分钟 */
#define SENSOR_SAMPLE_PERIOD_UV_S        60U   /* AS7331 紫外：1 分钟 */
#define SENSOR_SAMPLE_PERIOD_DIST_S      600U  /* WH4530A 接近：10 分钟 */
#define SENSOR_SAMPLE_PERIOD_ACC_S       1U    /* ADXL345 加速度：1 秒 */

/*
 * 1：0x4E 中的周期必须等于上面的配置，否则返回失败（正式版本推荐）。
 * 0：接受 APP 发送的任意非零周期，适合把 60/600 秒临时改短进行快速测试。
 */
#define SENSOR_SAMPLE_PERIOD_STRICT      1

#endif /* SENSOR_PERIOD_CONFIG_H_ */
