/*
 * 应用层核心文件：协议 V4、五传感器周期调度、RAM 历史缓存和板级低功耗管理。
 *
 * 下行：手机 APP -> FEE2 -> app_ble_msg_receive() -> 启动对应传感器任务。
 * 上行：传感器任务 -> app_protocol_sensor_data() -> FEE4 Notify -> 手机 APP。
 * 周期采集：每条 0x4E 配置一种传感器，五种传感器的周期互相独立。
 */
#include "rwip_config.h"
#include "user_config.h"
#include "sensor_period_config.h"
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
	uint8_t active;       /* 1=该传感器已经收到 0x4E，参与周期采集 */
	uint16_t period_s;    /* 该传感器的实际采样周期，单位：秒 */
	uint16_t elapsed_s;   /* 已累计秒数，到达 period_s 后触发一次采集 */
} protocol_schedule_t;

/* 每种传感器各有一块独立 RAM 缓存，record_len 决定每组记录的字节数。 */
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

/* 下标顺序固定为：多光谱、FIFO、紫外、接近、加速度。 */
static protocol_schedule_t protocol_schedule[PROTOCOL_SENSOR_COUNT];

/* 正式采样周期统一来自 config/sensor_period_config.h。 */
static const uint16_t configured_sensor_period_s[PROTOCOL_SENSOR_COUNT] = {
	SENSOR_SAMPLE_PERIOD_MSS_S,
	SENSOR_SAMPLE_PERIOD_FIFO_S,
	SENSOR_SAMPLE_PERIOD_UV_S,
	SENSOR_SAMPLE_PERIOD_DIST_S,
	SENSOR_SAMPLE_PERIOD_ACC_S,
};

static uint8_t mss_atime = SENSOR_DEFAULT_MSS_ATIME;
static uint16_t mss_astep = SENSOR_DEFAULT_MSS_ASTEP;
static uint8_t mss_gain = SENSOR_DEFAULT_MSS_GAIN;
static uint16_t fifo_time = SENSOR_DEFAULT_FIFO_TIME;
static uint8_t fifo_gain = SENSOR_DEFAULT_FIFO_GAIN;
static uint8_t uv_gain = SENSOR_DEFAULT_UV_GAIN;
static uint8_t dist_gain = SENSOR_DEFAULT_DIST_GAIN;

static protocol_tx_item_t tx_queue[PROTOCOL_TX_QUEUE_DEPTH];
static uint8_t tx_head;
static uint8_t tx_tail;
static uint8_t tx_count;
static uint8_t tx_busy;

/*
 * 当前仅为片内 RAM 小容量循环缓存，并不能保存完整一天。
 * 缓存满后 protocol_history_append() 会删除最旧记录，保留最新记录。
 */
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
	/* 打开传感器公共电源，等待电源稳定后再初始化 I2C。 */
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
	/* 建立“传感器编号 -> 数据区/时间戳区/单组长度/容量”的映射。 */
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

static void protocol_history_clear(uint8_t sensor_id)
{
	/* 重新配置某一种传感器时，只清除该传感器的旧历史。 */
	if (sensor_id < PROTOCOL_SENSOR_COUNT) {
		protocol_history[sensor_id].count = 0;
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
	/* RAM 已满：整体前移一组，覆盖最旧数据。 */
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

	/* 上电后默认不自动采集，必须由 APP 发送 0x4E 启动各传感器。 */
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
	/* 某任务完成后清除占用位；所有任务空闲时关闭传感器公共电源。 */
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
	/* 采集开始前打开公共电源，并记录当前正在工作的任务。 */
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
	/*
	 * scheduled=1：周期采集，结果写入对应 RAM 历史缓存。
	 * scheduled=0：APP 单次查询，结果立即组成 0x43/45/47/49/4B 返回。
	 */
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
	uint8_t any_active = 0;

	utc_update();
	/* 公共定时器每秒进入一次，但五种传感器分别累计自己的 elapsed_s。 */
	for (i = 0; i < PROTOCOL_SENSOR_COUNT; i++) {
		if (!protocol_schedule[i].active) {
			continue;
		}
		any_active = 1;
		protocol_schedule[i].elapsed_s++;
		if (protocol_schedule[i].elapsed_s >= protocol_schedule[i].period_s) {
			protocol_schedule[i].elapsed_s = 0;
			/* 历史上传期间跳过到期采集，避免采集和大数据发送互相影响。 */
			if (!upload_paused) {
				protocol_trigger_sensor(i, 1);
			}
		}
	}
	if (any_active) {
		/* One-second scheduler tick; each sensor keeps an independent period. */
		ke_timer_set(APP_PERIOD_TIMER, TASK_APP, 100);
	}
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
	/* BLE 发送采用队列串行化，上一条 Notify 完成后才发送下一条。 */
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
	/* 统一封装应用帧：AA | CMD | DATA | XOR | BB。 */
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
	/* 周期采集结果只保存到 RAM，不立即通过 BLE 主动上传。 */
	if (scheduled_pending_mask & mask) {
		protocol_history_append(sensor_id, data, len);
		scheduled_pending_mask &= ~mask;
	}
	/* 单次读取命令的结果立即通过对应响应命令返回。 */
	if (direct_pending_mask & mask) {
		protocol_send_frame(response_command[sensor_id], data, len);
		direct_pending_mask &= ~mask;
	}
}

static void protocol_configure_schedule(const uint8_t *buf)
{
	uint8_t result = 0;
	uint8_t i;
	uint8_t schedule_was_running = 0;
	uint8_t selected_count = 0;
	uint8_t selected_sensor = 0;
	uint32_t start_time;
	uint16_t period;

	/*
	 * 0x4E DATA：Unix 时间(4) + 周期秒数(2) + 五个传感器选择位(5)。
	 * 五个选择位必须且只能有一个为 1；本函数只更新被选中的那套计划。
	 */
	memcpy(&start_time, &buf[2], 4);
	period = (uint16_t)buf[6] | ((uint16_t)buf[7] << 8);
	if (period == 0) {
		result = 1;
	}
	for (i = 0; i < PROTOCOL_SENSOR_COUNT; i++) {
		if (buf[8 + i] > 1) {
			result = 1;
		}
		if (buf[8 + i] == 1) {
			selected_sensor = i;
			selected_count++;
		}
	}
	if (selected_count != 1) {
		result = 1;
	}
	/* 严格模式下，APP 周期必须与 sensor_period_config.h 中的宏一致。 */
	if (selected_count == 1 && SENSOR_SAMPLE_PERIOD_STRICT &&
		period != configured_sensor_period_s[selected_sensor]) {
		result = 1;
	}

	if (result == 0) {
		for (i = 0; i < PROTOCOL_SENSOR_COUNT; i++) {
			if (protocol_schedule[i].active) {
				schedule_was_running = 1;
				break;
			}
		}
		utc_set_clock(start_time);
		/* 只重置当前传感器，前面已配置的其他四种传感器继续独立运行。 */
		protocol_history_clear(selected_sensor);
		protocol_schedule[selected_sensor].active = 1;
		protocol_schedule[selected_sensor].period_s = period;
		protocol_schedule[selected_sensor].elapsed_s = 0;
		if (!schedule_was_running) {
			ke_timer_set(APP_PERIOD_TIMER, TASK_APP, 100);
		}
		uart_printf("V4 schedule sensor=%u, period=%u s\r\n",
				selected_sensor + 1, period);
	} else {
		uart_printf("V4 schedule rejected: period=%u, selected=%u, configured=%u\r\n",
				period, selected_count,
				selected_count == 1 ? configured_sensor_period_s[selected_sensor] : 0);
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

	/* 0x50 的 TYPE：1多光谱、2 FIFO、3紫外、4接近、5加速度。 */
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
	/* 当前版本一次只组装一个 0x51；完整一天的 Flash 多包上传尚未实现。 */
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
	/* 第一步先检查帧头、帧尾和异或校验，错误帧不执行任何传感器操作。 */
	if (len < 4 || buf[0] != PROTOCOL_HEAD || buf[len - 1] != PROTOCOL_TAIL ||
		data_fcs(&buf[1], len - 3) != buf[len - 2]) {
		uart_printf("Protocol frame check failed, len=%d\r\n", len);
		return;
	}
	command = buf[1];
	/* 第二步按命令字检查固定长度，防止越界读取 DATA。 */
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

	/* 第三步执行命令：单次读取、周期配置、历史上传或停止采集。 */
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
		memset(protocol_schedule, 0, sizeof(protocol_schedule));
		scheduled_pending_mask = 0;
		ke_timer_clear(APP_PERIOD_TIMER, TASK_APP);
		protocol_send_frame(0x53, 0, 0);
		break;
	default:
		break;
	}
}
