/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#define ACC_DATA_NUM_MAX	20

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	short acc_x1;
	short acc_y1;
	short acc_z1;
} SensorAccStruct;


typedef struct {
	uint16_t 	save_period;
	uint16_t 	data_index;
	uint16_t	remain_num;
	SensorAccStruct 	data[ACC_DATA_NUM_MAX];
    uint8_t     start_sign;
    uint8_t     upload_sign;
} UserDataAccStruct;


void app_read_acc_task(void);
void acc_collect_once_report(uint8_t again);
/* 周期加速度：LOW实际1Hz直读保存，HIGH实际50Hz FIFO全量保存。 */
bool acc_stream_start(void);
bool acc_stream_stop(void);
bool acc_stream_pause(void);
void acc_stream_cancel_pause(void);
void acc_stream_process(void);
void acc_stream_storage_process(void);
void acc_stream_storage_tick(void);
void acc_stream_poll_task(void);
bool acc_stream_resume(void);
void acc_stream_second_tick(void);
bool acc_stream_is_active(void);
bool acc_stream_is_stopped(void);
bool acc_stream_is_paused(void);
bool acc_stream_storage_busy(void);
bool acc_stream_can_yield_flash(void);
void acc_stream_link_report(uint8_t reason);
bool acc_stream_data_complete(void);
void acc_stream_history_cleared(void);
uint8_t acc_stream_motion_state(void);
void acc_gpio_interrupt_callback(void);

#ifdef __cplusplus
}
#endif
