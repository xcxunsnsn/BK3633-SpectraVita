/**
 ****************************************************************************************
 *
 * @file app_user.h
 *
 * @brief Header file - APPUSER.
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */

#ifndef APP_USER_H_
#define APP_USER_H_


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"    // SW configuration
#include "task_mss.h"
#include "task_fifo.h"
#include "task_uv.h"
#include "task_dist.h"
#include "task_acc.h"

/*
 * DEFINES
 ****************************************************************************************
 */
#define APP_MSG_MAX_LEN	300

/*
 * ENUMERATIONS
 ****************************************************************************************
 */
enum {
	TASK_ID_MSS = 0,   /* AS7341 普通多光谱任务 */
	TASK_ID_FIFO,      /* AS7341 闪烁 FIFO 任务 */
	TASK_ID_UV,        /* AS7331 紫外任务 */
	TASK_ID_DIST,      /* WH4530A 接近任务 */
	TASK_ID_ACC,       /* ADXL345 加速度任务 */
};

/* 协议内部传感器编号；顺序必须与 0x4E 的五个 X 和 0x50 TYPE 保持一致。 */
enum {
	PROTOCOL_SENSOR_MSS = 0,  /* X[0]，TYPE=1 */
	PROTOCOL_SENSOR_FIFO,     /* X[1]，TYPE=2 */
	PROTOCOL_SENSOR_UV,       /* X[2]，TYPE=3 */
	PROTOCOL_SENSOR_DIST,     /* X[3]，TYPE=4 */
	PROTOCOL_SENSOR_ACC,      /* X[4]，TYPE=5 */
	PROTOCOL_SENSOR_COUNT,
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
void app_peripheral_init(void);
void app_user_init(void);
void app_sensor_id_test(void);
bool app_allow_sleep(void);
void app_clear_no_sleep(void);
void app_task_enter_lowpower(uint8_t task_id);
void app_task_quit_lowpower(uint8_t task_id);
bool app_task_get_mutex(uint8_t task_id);
void app_period_task(void);
void app_ble_scheduler_heartbeat(void);
void app_ble_timing_report(const char *event, uint8_t reason);
void app_ble_connect_handle(uint8_t conidx);
void app_ble_disconnect_handle(uint8_t reason);
void app_ble_send_upd_handler(void);
/* FEE4 Notify 的协议栈完成回调；status=0 才代表手机侧发送成功。 */
void app_ble_send_complete(uint8_t status);
void app_led_flash_task(void);
void app_ble_msg_receive(uint8_t conidx, uint8_t *buf, uint16_t len);
void app_protocol_sensor_data(uint8_t sensor_id, const uint8_t *data, uint16_t len);
/* Legacy immediate wrapper; FIFO3 uses the batch commit API below. */
bool app_protocol_acc_stream_sample(uint8_t motion_state,
									int16_t x, int16_t y, int16_t z);
#define ACC_COMMIT_ERROR 0U
#define ACC_COMMIT_OK    1U
#define ACC_COMMIT_LAST  2U /* Accepted this record and reached capacity. */
#define ACC_COMMIT_FULL  3U /* Already full: no record accepted. */
#define ACC_COMMIT_BUSY  4U
#define ACC_COMMIT_BATCH_MAX 8U
#define ACC_HISTORY_RECORD_LEN 7U
uint8_t app_protocol_acc_commit_batch(
		const uint32_t timestamps[],
		uint8_t records[][ACC_HISTORY_RECORD_LEN],
		uint8_t requested, uint8_t *accepted);
uint8_t app_protocol_acc_commit(uint32_t timestamp, uint8_t state,
                               int16_t x, int16_t y, int16_t z);
uint8_t app_protocol_acc_commit_async_begin(
		const uint32_t timestamps[],
		uint8_t records[][ACC_HISTORY_RECORD_LEN], uint8_t requested);
uint8_t app_protocol_acc_commit_async_process(uint8_t *accepted);
bool app_protocol_acc_flash_allowed(void);
/* 运行中切换采样模式失败时只停止加速度计划，不影响其他传感器。 */
void app_protocol_acc_stream_fault(const char *reason);
void app_protocol_background_process(void);

void app_ble_send_data(uint8_t *buf, uint16_t len);
uint8_t data_fcs(uint8_t *buf, uint16_t len);

#endif // APP_USER_H_
