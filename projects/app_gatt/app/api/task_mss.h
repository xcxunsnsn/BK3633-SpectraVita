/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif




void mssFactor_init(void);
void app_mss_integration_task(void);
void mss_collect_once_report(uint8_t atime,uint16_t astep,uint8_t again);

#ifdef __cplusplus
}
#endif
