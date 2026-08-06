/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#define DIST_DATA_NUM_MAX	20

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint16_t proximity1;
	uint16_t proximity2;
} SensorDistStruct;

typedef struct {
	uint16_t 	save_period;
	uint16_t 	data_index;
	uint16_t	remain_num;
	SensorDistStruct 	data[DIST_DATA_NUM_MAX];
    uint8_t     upload_sign;
} UserDataDistStruct;


void app_read_proximity_task(void);
void dist_collect_once_report(uint8_t again);


#ifdef __cplusplus
}
#endif
