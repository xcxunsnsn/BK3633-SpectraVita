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
	TASK_ID_MSS = 0,
	TASK_ID_FIFO,
	TASK_ID_UV,
	TASK_ID_DIST,
	TASK_ID_ACC,
};

enum {
	PROTOCOL_SENSOR_MSS = 0,
	PROTOCOL_SENSOR_FIFO,
	PROTOCOL_SENSOR_UV,
	PROTOCOL_SENSOR_DIST,
	PROTOCOL_SENSOR_ACC,
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
void app_ble_connect_handle(uint8_t conidx);
void app_ble_disconnect_handle(void);
void app_ble_send_upd_handler(void);
void app_led_flash_task(void);
void app_ble_msg_receive(uint8_t conidx, uint8_t *buf, uint16_t len);
void app_protocol_sensor_data(uint8_t sensor_id, const uint8_t *data, uint16_t len);

void app_ble_send_data(uint8_t *buf, uint16_t len);
uint8_t data_fcs(uint8_t *buf, uint16_t len);

#endif // APP_USER_H_
