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

#ifdef __cplusplus
}
#endif
