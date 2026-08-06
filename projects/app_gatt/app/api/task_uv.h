/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#define UV_DATA_NUM_MAX	20

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint16_t uva;
	uint16_t uvb;
	uint16_t uvc;
	uint16_t temperature;
} SensorUvStruct;

typedef struct {
	uint16_t 	save_period;
	uint16_t 	data_index;
	uint16_t	remain_num;
	SensorUvStruct 	data[UV_DATA_NUM_MAX];
    uint8_t     upload_sign;
} UserDataUvStruct;


void app_read_uv_task(void);
void uv_collect_once_report(uint8_t again);

#ifdef __cplusplus
}
#endif
