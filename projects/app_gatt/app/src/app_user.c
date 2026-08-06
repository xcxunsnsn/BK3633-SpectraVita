/* Application protocol, sensor scheduling and board power management. */
#include "rwip_config.h"
#include "user_config.h"
#include "app_task.h"
#include "ke_timer.h"
#include "gpio.h"
#include "uart.h"
#include "adc.h"
#include "i2c.h"
#include "driver_AS7341.h"
#include "driver_WH4530.h"
#include "driver_ADXL345.h"
#include "driver_AS7331.h"
#include "driver_BQ21080.h"
#include "app_user.h"
#include "utc_clock.h"
#include "app_fee0.h"
#include <string.h>

#define APP_VERSION                 4
#define PROTOCOL_HEAD               0xAA
#define PROTOCOL_TAIL               0xBB
#define PROTOCOL_TX_QUEUE_DEPTH     4
#define PROTOCOL_HISTORY_MSS_MAX    12
#define PROTOCOL_HISTORY_FIFO_MAX   1
#define PROTOCOL_HISTORY_UV_MAX     20
#define PROTOCOL_HISTORY_DIST_MAX   20
#define PROTOCOL_HISTORY_ACC_MAX    20

enum led_status {
	LED_ON = 0,
	LED_OFF = 1,
};

typedef struct {
	uint8_t active;
	uint16_t min_period_s;
	uint32_t tick;
	uint8_t multiplier[PROTOCOL_SENSOR_COUNT];
} protocol_schedule_t;

typedef struct {
	uint8_t *data;
	uint32_t *timestamps;
	uint16_t record_len;
	uint8_t capacity;
	uint8_t count;
} protocol_history_t;

typedef struct {
	uint16_t len;
	uint8_t data[APP_MSG_MAX_LEN];
} protocol_tx_item_t;

static uint8_t ble_connect_sign;
static uint8_t ble_conidx;
static uint8_t user_allow_sleep;
static uint16_t task_lp_sign;
static uint16_t task_mutex;
static uint8_t flash_count;

static uint8_t direct_pending_mask;
static uint8_t scheduled_pending_mask;
static uint8_t upload_paused;
static protocol_schedule_t protocol_schedule;

static uint8_t mss_atime = 29;
static uint16_t mss_astep = 599;
static uint8_t mss_gain = 10;
static uint16_t fifo_time = 720;
static uint8_t fifo_gain = 5;
static uint8_t uv_gain = 9;
static uint8_t dist_gain;

static protocol_tx_item_t tx_queue[PROTOCOL_TX_QUEUE_DEPTH];
static uint8_t tx_head;
static uint8_t tx_tail;
static uint8_t tx_count;
static uint8_t tx_busy;

static uint8_t history_mss[PROTOCOL_HISTORY_MSS_MAX * 22];
static uint8_t history_fifo[PROTOCOL_HISTORY_FIFO_MAX * 256];
static uint8_t history_uv[PROTOCOL_HISTORY_UV_MAX * 8];
static uint8_t history_dist[PROTOCOL_HISTORY_DIST_MAX * 4];
static uint8_t history_acc[PROTOCOL_HISTORY_ACC_MAX * 6];
static uint32_t history_mss_time[PROTOCOL_HISTORY_MSS_MAX];
static uint32_t history_fifo_time[PROTOCOL_HISTORY_FIFO_MAX];
static uint32_t history_uv_time[PROTOCOL_HISTORY_UV_MAX];
static uint32_t history_dist_time[PROTOCOL_HISTORY_DIST_MAX];
static uint32_t history_acc_time[PROTOCOL_HISTORY_ACC_MAX];
static protocol_history_t protocol_history[PROTOCOL_SENSOR_COUNT];

void sys_delay(uint32_t times)
{
	uint32_t delay_num = 0;
	while (delay_num++ < times) {
		__nop();
	}
}

static void app_sensor_power_on(void)
{
	gpio_set(VDD_1P8_PIN, 1);
	gpio_set(VDD_SENSOR_PIN, 1);
	/* Match the proven ID-only sequence: rail settle, I2C reset, settle. */
	sys_delay(200000);
	i2c_init(0, 100000);
	sys_delay(50000);
}

void app_uart_callback(uint8_t *buf, uint8_t len)
{
	(void)buf;
	(void)len;
}

static void protocol_history_init(void)
{
	memset(protocol_history, 0, sizeof(protocol_history));
	protocol_history[PROTOCOL_SENSOR_MSS].data = history_mss;
	protocol_history[PROTOCOL_SENSOR_MSS].timestamps = history_mss_time;
	protocol_history[PROTOCOL_SENSOR_MSS].record_len = 22;
	protocol_history[PROTOCOL_SENSOR_MSS].capacity = PROTOCOL_HISTORY_MSS_MAX;
	protocol_history[PROTOCOL_SENSOR_FIFO].data = history_fifo;
	protocol_history[PROTOCOL_SENSOR_FIFO].timestamps = history_fifo_time;
	protocol_history[PROTOCOL_SENSOR_FIFO].record_len = 256;
	protocol_history[PROTOCOL_SENSOR_FIFO].capacity = PROTOCOL_HISTORY_FIFO_MAX;
	protocol_history[PROTOCOL_SENSOR_UV].data = history_uv;
	protocol_history[PROTOCOL_SENSOR_UV].timestamps = history_uv_time;
	protocol_history[PROTOCOL_SENSOR_UV].record_len = 8;
	protocol_history[PROTOCOL_SENSOR_UV].capacity = PROTOCOL_HISTORY_UV_MAX;
	protocol_history[PROTOCOL_SENSOR_DIST].data = history_dist;
	protocol_history[PROTOCOL_SENSOR_DIST].timestamps = history_dist_time;
	protocol_history[PROTOCOL_SENSOR_DIST].record_len = 4;
	protocol_history[PROTOCOL_SENSOR_DIST].capacity = PROTOCOL_HISTORY_DIST_MAX;
	protocol_history[PROTOCOL_SENSOR_ACC].data = history_acc;
	protocol_history[PROTOCOL_SENSOR_ACC].timestamps = history_acc_time;
	protocol_history[PROTOCOL_SENSOR_ACC].record_len = 6;
	protocol_history[PROTOCOL_SENSOR_ACC].capacity = PROTOCOL_HISTORY_ACC_MAX;
}

static void protocol_history_clear_all(void)
{
	uint8_t i;
	for (i = 0; i < PROTOCOL_SENSOR_COUNT; i++) {
		protocol_history[i].count = 0;
	}
}

static void protocol_history_append(uint8_t sensor_id, const uint8_t *data,
									uint16_t len)
{
	protocol_history_t *history;
	uint8_t index;
	uint32_t now;

	if (sensor_id >= PROTOCOL_SENSOR_COUNT) {
		return;
	}
	history = &protocol_history[sensor_id];
	if (len != history->record_len) {
		return;
	}
	utc_update();
	now = utc_get_clock();
	if (history->count >= history->capacity) {
		if (history->capacity > 1) {
			memmove(history->data, history->data + history->record_len,
					(history->capacity - 1) * history->record_len);
			memmove(history->timestamps, history->timestamps + 1,
					(history->capacity - 1) * sizeof(uint32_t));
		}
		history->count = history->capacity - 1;
	}
	index = history->count;
	memcpy(history->data + index * history->record_len, data, len);
	history->timestamps[index] = now;
	history->count++;
}

void app_peripheral_init(void)
{
	bq21080_status_t bq_status;
	uint8_t bq_device_id = 0xFF;
	uint8_t bq_mask_id = 0xFF;

	gpio_config(BLE_LED_PIN, OUTPUT, PULL_NONE);
	gpio_set(BLE_LED_PIN, LED_OFF);
	gpio_config(VDD_1P8_PIN, OUTPUT, PULL_NONE);
	gpio_set(VDD_1P8_PIN, 0);
	gpio_config(VDD_SENSOR_PIN, OUTPUT, PULL_NONE);
	gpio_set(VDD_SENSOR_PIN, 0);
	gpio_config(DC_DET_PIN, INPUT, PULL_NONE);
	gpio_config(ADXL_IN1_PIN, INPUT, PULL_NONE);
	gpio_config(ADXL_IN2_PIN, INPUT, PULL_NONE);
	gpio_config(WH4530_INT_PIN, INPUT, PULL_NONE);
	gpio_config(UV_READY_PIN, INPUT, PULL_NONE);
	gpio_config(UV_SYN_PIN, INPUT, PULL_NONE);
	gpio_config(ADXL_CS_PIN, OUTPUT, PULL_NONE);
	gpio_set(ADXL_CS_PIN, 1);
	i2c_init(0, 100000);

	bq_status = bq21080_init();
	if (bq_status == BQ21080_OK) {
		bq_status = bq21080_read_device_id(&bq_device_id, &bq_mask_id);
	}
	if (bq_status == BQ21080_OK) {
		uart_printf("BQ21080 MASK_ID=0x%x, Device_ID=0x%x\r\n",
					bq_mask_id, bq_device_id);
	} else {
		uart_printf("BQ21080 failed, status=%d\r\n", bq_status);
	}
}

void app_user_init(void)
{
	uint8_t as7331_ok;
	uint8_t as7341_ok;
	uint8_t adxl345_ok;

	memset(&protocol_schedule, 0, sizeof(protocol_schedule));
	protocol_history_init();
	mssFactor_init();
	utc_set_clock(0);

	/* Do one startup health check, then return every sensor to its low-power state. */
	app_sensor_power_on();
	as7331_ok = AS7331_Get_ID();
	as7341_ok = AS7341_Get_ID();
	adxl345_ok = ADXL345_Get_ID();
	uart_printf("V4 sensor check: AS7331=%d AS7341=%d ADXL345=%d\r\n",
				as7331_ok, as7341_ok, adxl345_ok);
	if (as7331_ok) {
		AS7331_Set_Power_Down(AS7331_PD_ON);
	}
	AS7341_disableALL();
	ADXL345_Enable(false);
	gpio_set(VDD_1P8_PIN, 0);
	gpio_set(VDD_SENSOR_PIN, 0);
	uart_printf("BLE protocol V4 ready\r\n");
}

void app_sensor_id_test(void)
{
	uint8_t sensor_passed;
	uint8_t as7331_ok;
	uint8_t as7341_ok;
	uint8_t adxl345_ok;

	app_sensor_power_on();
	gpio_set(ADXL_CS_PIN, 1);
	uart_printf("\r\n=== I2C sensor ID test ===\r\n");
	as7331_ok = AS7331_Get_ID();
	as7341_ok = AS7341_Get_ID();
	adxl345_ok = ADXL345_Get_ID();
	sensor_passed = as7331_ok + as7341_ok + adxl345_ok;
	uart_printf("=== ID result: %d/3 (AS7331=%d AS7341=%d ADXL345=%d) ===\r\n",
				sensor_passed, as7331_ok, as7341_ok, adxl345_ok);
	while (1) {
		__nop();
	}
}

bool app_allow_sleep(void)
{
	return user_allow_sleep;
}

void app_clear_no_sleep(void)
{
	user_allow_sleep = true;
}

void app_task_enter_lowpower(uint8_t task_id)
{
	task_lp_sign &= ~(0x01 << task_id);
	if ((task_lp_sign & 0x03) == 0) {
		AS7341_disableALL();
	}
	if (task_id == TASK_ID_MSS || task_id == TASK_ID_FIFO) {
		task_mutex &= ~(0x01 << task_id);
	}
	if (task_lp_sign == 0) {
		gpio_set(VDD_1P8_PIN, 0);
		gpio_set(VDD_SENSOR_PIN, 0);
	}
}

void app_task_quit_lowpower(uint8_t task_id)
{
	if (task_lp_sign == 0) {
		app_sensor_power_on();
	}
	task_lp_sign |= 0x01 << task_id;
	if (task_id == TASK_ID_MSS || task_id == TASK_ID_FIFO) {
		task_mutex |= 0x01 << task_id;
	}
}

bool app_task_get_mutex(uint8_t task_id)
{
	if (task_id == TASK_ID_MSS) {
		return (task_mutex & (0x01 << TASK_ID_FIFO)) != 0;
	}
	if (task_id == TASK_ID_FIFO) {
		return (task_mutex & (0x01 << TASK_ID_MSS)) != 0;
	}
	return false;
}

static void protocol_trigger_sensor(uint8_t sensor_id, uint8_t scheduled)
{
	uint8_t mask = 0x01 << sensor_id;
	if (scheduled) {
		scheduled_pending_mask |= mask;
	} else {
		direct_pending_mask |= mask;
	}

	switch (sensor_id) {
	case PROTOCOL_SENSOR_MSS:
		mss_collect_once_report(mss_atime, mss_astep, mss_gain);
		break;
	case PROTOCOL_SENSOR_FIFO:
		fifo_collect_once_report(fifo_time, fifo_gain);
		break;
	case PROTOCOL_SENSOR_UV:
		uv_collect_once_report(uv_gain);
		break;
	case PROTOCOL_SENSOR_DIST:
		dist_collect_once_report(dist_gain);
		break;
	case PROTOCOL_SENSOR_ACC:
		acc_collect_once_report(0);
		break;
	default:
		break;
	}
}

void app_period_task(void)
{
	uint8_t i;
	utc_update();
	if (!protocol_schedule.active) {
		return;
	}
	protocol_schedule.tick++;
	if (!upload_paused) {
		for (i = 0; i < PROTOCOL_SENSOR_COUNT; i++) {
			if (protocol_schedule.multiplier[i] != 0 &&
				(protocol_schedule.tick % protocol_schedule.multiplier[i]) == 0) {
				protocol_trigger_sensor(i, 1);
			}
		}
	}
	ke_timer_set(APP_PERIOD_TIMER, TASK_APP,
				(uint32_t)protocol_schedule.min_period_s * 100);
}

static void protocol_tx_reset(void)
{
	tx_head = 0;
	tx_tail = 0;
	tx_count = 0;
	tx_busy = 0;
}

static bool protocol_tx_enqueue(const uint8_t *data, uint16_t len)
{
	if (len > APP_MSG_MAX_LEN || tx_count >= PROTOCOL_TX_QUEUE_DEPTH) {
		uart_printf("BLE TX queue full, len=%d\r\n", len);
		return false;
	}
	tx_queue[tx_tail].len = len;
	memcpy(tx_queue[tx_tail].data, data, len);
	tx_tail = (tx_tail + 1) % PROTOCOL_TX_QUEUE_DEPTH;
	tx_count++;
	app_ble_send_upd_handler();
	return true;
}

static bool protocol_send_frame(uint8_t command, const uint8_t *data, uint16_t len)
{
	uint8_t frame[APP_MSG_MAX_LEN];
	uint16_t frame_len = len + 4;

	if (frame_len > APP_MSG_MAX_LEN) {
		return false;
	}
	frame[0] = PROTOCOL_HEAD;
	frame[1] = command;
	if (len != 0) {
		memcpy(&frame[2], data, len);
	}
	frame[len + 2] = data_fcs(&frame[1], len + 1);
	frame[len + 3] = PROTOCOL_TAIL;
	return protocol_tx_enqueue(frame, frame_len);
}

void app_ble_connect_handle(uint8_t conidx)
{
	ble_conidx = conidx;
	ble_connect_sign = 1;
	flash_count = 4;
	ke_timer_set(APP_LED_FLASH, TASK_APP, 10);
}

void app_ble_disconnect_handle(void)
{
	gpio_set(BLE_LED_PIN, LED_OFF);
	ble_connect_sign = 0;
	direct_pending_mask = 0;
	upload_paused = 0;
	protocol_tx_reset();
}

void app_ble_send_upd_handler(void)
{
	if (tx_busy) {
		tx_busy = 0;
		tx_head = (tx_head + 1) % PROTOCOL_TX_QUEUE_DEPTH;
		if (tx_count != 0) {
			tx_count--;
		}
	}
	if (ble_connect_sign && tx_count != 0 &&
		app_fee4_send_ntf(ble_conidx, tx_queue[tx_head].len,
						tx_queue[tx_head].data)) {
		tx_busy = 1;
	}
	if (tx_count == 0) {
		upload_paused = 0;
	}
}

void app_led_flash_task(void)
{
	if (flash_count != 0) {
		gpio_set(BLE_LED_PIN, (flash_count & 1) ? LED_ON : LED_OFF);
		flash_count--;
		ke_timer_set(APP_LED_FLASH, TASK_APP, 10);
	} else {
		/* Do not keep the status LED on for the entire BLE connection. */
		gpio_set(BLE_LED_PIN, LED_OFF);
	}
}

void app_ble_send_data(uint8_t *buf, uint16_t len)
{
	protocol_tx_enqueue(buf, len);
}

uint8_t data_fcs(uint8_t *buf, uint16_t len)
{
	uint16_t i;
	uint8_t fcs = 0;
	for (i = 0; i < len; i++) {
		fcs ^= buf[i];
	}
	return fcs;
}

static uint8_t app_battery_percent(void)
{
	uint16_t adc_value;
	adc_init(BAT_ADC, 1);
	adc_value = adc_get_value(BAT_ADC, 1);
	if (adc_value <= 133) {
		return 0;
	}
	adc_value = (adc_value - 133) * 100 / 25;
	return adc_value > 100 ? 100 : (uint8_t)adc_value;
}

static void app_msg_respond_dev_info(void)
{
	uint8_t data[6];
	uint32_t utc_time;
	utc_update();
	utc_time = utc_get_clock();
	memcpy(data, &utc_time, 4);
	data[4] = app_battery_percent();
	data[5] = APP_VERSION;
	protocol_send_frame(0x41, data, sizeof(data));
}

void app_protocol_sensor_data(uint8_t sensor_id, const uint8_t *data, uint16_t len)
{
	static const uint8_t response_command[PROTOCOL_SENSOR_COUNT] = {
		0x43, 0x45, 0x47, 0x49, 0x4B
	};
	uint8_t mask;

	if (sensor_id >= PROTOCOL_SENSOR_COUNT) {
		return;
	}
	mask = 0x01 << sensor_id;
	if (scheduled_pending_mask & mask) {
		protocol_history_append(sensor_id, data, len);
		scheduled_pending_mask &= ~mask;
	}
	if (direct_pending_mask & mask) {
		protocol_send_frame(response_command[sensor_id], data, len);
		direct_pending_mask &= ~mask;
	}
}

static void protocol_configure_schedule(const uint8_t *buf)
{
	uint8_t result = 0;
	uint32_t start_time;
	uint16_t min_period;

	memcpy(&start_time, &buf[2], 4);
	min_period = (uint16_t)buf[6] | ((uint16_t)buf[7] << 8);
	if (min_period == 0) {
		result = 1;
	} else {
		ke_timer_clear(APP_PERIOD_TIMER, TASK_APP);
		utc_set_clock(start_time);
		protocol_history_clear_all();
		protocol_schedule.active = 1;
		protocol_schedule.min_period_s = min_period;
		protocol_schedule.tick = 0;
		memcpy(protocol_schedule.multiplier, &buf[8], PROTOCOL_SENSOR_COUNT);
		ke_timer_set(APP_PERIOD_TIMER, TASK_APP, (uint32_t)min_period * 100);
	}
	protocol_send_frame(0x4F, &result, 1);
}

static void protocol_upload_history(uint8_t type)
{
	protocol_history_t *history;
	uint8_t data[APP_MSG_MAX_LEN - 4];
	uint16_t data_len;
	uint8_t sensor_id;
	uint32_t timestamp;

	if (type < 1 || type > PROTOCOL_SENSOR_COUNT) {
		return;
	}
	sensor_id = type - 1;
	history = &protocol_history[sensor_id];
	utc_update();
	timestamp = history->count ? history->timestamps[0] : utc_get_clock();
	memcpy(data, &timestamp, 4);
	data_len = 4;
	if (history->count != 0) {
		memcpy(data + data_len, history->data,
				history->count * history->record_len);
		data_len += history->count * history->record_len;
	}
	upload_paused = 1;
	if (protocol_send_frame(0x51, data, data_len)) {
		history->count = 0;
	} else {
		upload_paused = 0;
	}
}

void app_ble_msg_receive(uint8_t conidx, uint8_t *buf, uint16_t len)
{
	uint8_t command;
	uint16_t expected_len = 0;

	ble_conidx = conidx;
	if (len < 4 || buf[0] != PROTOCOL_HEAD || buf[len - 1] != PROTOCOL_TAIL ||
		data_fcs(&buf[1], len - 3) != buf[len - 2]) {
		uart_printf("Protocol frame check failed, len=%d\r\n", len);
		return;
	}
	command = buf[1];
	switch (command) {
	case 0x40: expected_len = 4; break;
	case 0x42: expected_len = 8; break;
	case 0x44: expected_len = 7; break;
	case 0x46: expected_len = 5; break;
	case 0x48: expected_len = 5; break;
	case 0x4A: expected_len = 4; break;
	case 0x4E: expected_len = 15; break;
	case 0x50: expected_len = 5; break;
	case 0x52: expected_len = 4; break;
	default:
		uart_printf("Unsupported protocol command 0x%x\r\n", command);
		return;
	}
	if (len != expected_len) {
		uart_printf("Protocol command 0x%x length error: %d/%d\r\n",
					command, len, expected_len);
		return;
	}

	switch (command) {
	case 0x40:
		app_msg_respond_dev_info();
		break;
	case 0x42:
		mss_atime = buf[2];
		mss_astep = (uint16_t)buf[3] | ((uint16_t)buf[4] << 8);
		mss_gain = buf[5];
		protocol_trigger_sensor(PROTOCOL_SENSOR_MSS, 0);
		break;
	case 0x44:
		fifo_time = (uint16_t)buf[2] | ((uint16_t)buf[3] << 8);
		fifo_gain = buf[4];
		protocol_trigger_sensor(PROTOCOL_SENSOR_FIFO, 0);
		break;
	case 0x46:
		uv_gain = buf[2];
		protocol_trigger_sensor(PROTOCOL_SENSOR_UV, 0);
		break;
	case 0x48:
		dist_gain = buf[2];
		protocol_trigger_sensor(PROTOCOL_SENSOR_DIST, 0);
		break;
	case 0x4A:
		protocol_trigger_sensor(PROTOCOL_SENSOR_ACC, 0);
		break;
	case 0x4E:
		protocol_configure_schedule(buf);
		break;
	case 0x50:
		protocol_upload_history(buf[2]);
		break;
	case 0x52:
		protocol_schedule.active = 0;
		ke_timer_clear(APP_PERIOD_TIMER, TASK_APP);
		protocol_send_frame(0x53, 0, 0);
		break;
	default:
		break;
	}
}
