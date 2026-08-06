/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#define FIFO_DATA_NUM_MAX	20


#ifdef __cplusplus
extern "C" {
#endif

enum fifo_status_e{
	FIFO_READ_720T = 0,    
	FIFO_READ_350T,   
    FIFO_READ_110T,   
    FIFO_READ_50T, 
	FIFO_READ_20T,
	FIFO_READ_LAST,
};

typedef struct {
	uint16_t mss_fd720_data[128];
	uint16_t mss_fd350_data[128];
	uint16_t mss_fd110_data[128];
	uint16_t mss_fd50_data[128];
	uint16_t mss_fd20_data[128];
} SensorFifoStruct;

typedef struct {
	uint16_t 	save_period;
	uint16_t 	data_index;
	uint16_t	remain_num;
	SensorFifoStruct 	data[FIFO_DATA_NUM_MAX];
    uint8_t     upload_sign;
} UserDataFifoStruct;


void app_read_fifo_task(void);
void fifo_collect_once_report(uint16_t atime,uint8_t again);

#ifdef __cplusplus
}
#endif
