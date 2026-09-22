/*
 * 应用层核心文件：协议 V4、五传感器周期调度、外部 Flash 日志和板级低功耗管理。
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
#include "sensor_flash_store.h"
#include "period_due_queue.h"
#include "ext_flash.h"
#include "rwip.h"
#include <string.h>

#define APP_VERSION                 4
#define PROTOCOL_HEAD               0xAA
#define PROTOCOL_TAIL               0xBB
#define PROTOCOL_TX_QUEUE_DEPTH     4

enum led_status {
	LED_ON = 0,
	LED_OFF = 1,
};

typedef struct {
	uint8_t active;       /* 1=该传感器已经收到 0x4E，参与周期采集 */
	uint16_t period_s;    /* 该传感器的实际采样周期，单位：秒 */
	uint16_t elapsed_s;   /* 已累计秒数，到达 period_s 后触发一次采集 */
} protocol_schedule_t;

typedef struct {
	uint16_t len;
	uint8_t data[APP_MSG_MAX_LEN];
} protocol_tx_item_t;

/* 0x50/0x51 多包上传状态；每收到一次 BLE 完成响应才继续下一包。 */
typedef struct {
	uint8_t active;
	uint8_t sensor_id;
	uint8_t packet_pending;
	uint16_t packet_records;
	uint32_t next_index;
	uint32_t total_records;
} protocol_upload_t;

static uint8_t ble_connect_sign;
static uint8_t ble_conidx;
static uint8_t user_allow_sleep;
static uint16_t task_lp_sign;
static uint16_t task_mutex;
static uint8_t flash_count;

static uint8_t direct_pending_mask;
static uint8_t scheduled_pending_mask;
static uint8_t upload_paused;
static period_due_queue_t low_due_queue;

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
static protocol_upload_t protocol_upload;
static uint32_t last_ble_loop_hs;
static uint32_t max_ble_loop_gap_hs;
static uint32_t window_ble_loop_gap_hs;
static uint8_t deferred_command;
static uint8_t deferred_frame[15];
static uint8_t deferred_upload_type;

static uint32_t app_elapsed_hs(uint32_t now, uint32_t before)
{
	return (now - before) & RWIP_MAX_CLOCK_TIME;
}

void app_ble_scheduler_heartbeat(void)
{
	uint32_t now = rwip_time_get().hs & RWIP_MAX_CLOCK_TIME;
	uint32_t gap;
	if (!ble_connect_sign) {
		last_ble_loop_hs = now;
		return;
	}
	gap = app_elapsed_hs(now, last_ble_loop_hs);
	if (gap > max_ble_loop_gap_hs) {
		max_ble_loop_gap_hs = gap;
	}
	if (gap > window_ble_loop_gap_hs) {
		window_ble_loop_gap_hs = gap;
	}
	last_ble_loop_hs = now;
}

void app_ble_timing_report(const char *event, uint8_t reason)
{
	ext_flash_stats_t stats;
	if (!ble_connect_sign && reason == 0xFF) {
		return;
	}
	ext_flash_get_stats(&stats);
	uart_printf("BLE timing %s reason=%02x loop10=%u life=%u ms\r\n",
		event, reason, (window_ble_loop_gap_hs * 5) / 16,
		(max_ble_loop_gap_hs * 5) / 16);
	uart_printf("BLE flash pp=%u/%ums erase=%u/%ums\r\n",
		stats.page_programs, (stats.max_page_program_hs * 5) / 16,
		stats.sector_erases, (stats.max_sector_erase_hs * 5) / 16);
	if (reason == 0xFF) window_ble_loop_gap_hs = 0;
}

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
	period_due_queue_init(&low_due_queue);
	deferred_command = deferred_upload_type = 0;
	memset(&protocol_upload, 0, sizeof(protocol_upload));
	mssFactor_init();
	utc_set_clock(0);
	if (!sensor_flash_store_init()) {
		uart_printf("External flash history unavailable\r\n");
	}

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
	 * scheduled=1：周期采集，结果写入对应外部 Flash 日志。
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

static void protocol_stop_schedule_full(uint8_t sensor_id)
{
	if (sensor_id >= PROTOCOL_SENSOR_COUNT) {
		return;
	}
	/*
	 * Flash full is a terminal state for the current periodic plan.  Keep all
	 * existing records intact and do not sample again until a new 0x4E plan is
	 * explicitly configured after the history has been uploaded/cleared.
	 */
	protocol_schedule[sensor_id].active = 0;
	protocol_schedule[sensor_id].elapsed_s = 0;
	scheduled_pending_mask &= (uint8_t)~(0x01U << sensor_id);
	period_due_queue_clear(&low_due_queue, sensor_id);
	if (sensor_id == PROTOCOL_SENSOR_ACC) {
		acc_stream_stop();
	}
	uart_printf("Flash full: sensor=%u schedule stopped, old data preserved\r\n",
			sensor_id + 1);
}

void app_period_task(void)
{
	uint8_t i;
	uint8_t due_sensor;
	uint8_t any_active = 0;
	uint16_t late_s;
	const period_due_stats_t *due_stats;

	utc_update();
	/* Stop/upload/reconfigure drains acceleration over multiple main-loop slices. */
	if (deferred_command != 0) {
		for (i = 0; i < PROTOCOL_SENSOR_COUNT; i++)
			if (protocol_schedule[i].active) any_active = 1;
		if (any_active) ke_timer_set(APP_PERIOD_TIMER, TASK_APP, 100);
		return;
	}
	period_due_queue_tick(&low_due_queue);
	/* 公共定时器每秒进入一次，但五种传感器分别累计自己的 elapsed_s。 */
	for (i = 0; i < PROTOCOL_SENSOR_COUNT; i++) {
		if (!protocol_schedule[i].active) {
			continue;
		}
		if (sensor_flash_store_full(i)) {
			period_due_queue_clear(&low_due_queue, i);
			protocol_stop_schedule_full(i);
			continue;
		}
		any_active = 1;
		protocol_schedule[i].elapsed_s++;
		if (protocol_schedule[i].elapsed_s >= protocol_schedule[i].period_s) {
			protocol_schedule[i].elapsed_s = 0;
			/* 加速度LOW每秒直读保存；HIGH每秒轮询FIFO作为中断保险。 */
			if (i == PROTOCOL_SENSOR_ACC) {
				if (!upload_paused) {
					acc_stream_second_tick();
				}
				continue;
			}
			/* Low-rate sensors become due together at 60/600 s. Queue them and
			 * start no more than one per scheduler tick. */
			if (!upload_paused)
				period_due_queue_mark(&low_due_queue, i,
					(scheduled_pending_mask & (0x01U << i)) != 0);
		}
	}
	if (!upload_paused && period_due_queue_take(&low_due_queue,
			scheduled_pending_mask, acc_stream_can_yield_flash(),
			&due_sensor, &late_s)) {
		due_stats = period_due_queue_stats(&low_due_queue, due_sensor);
		uart_printf("Schedule start sensor=%u late=%us due=%u start=%u missed=%u\r\n",
			due_sensor + 1, late_s, due_stats->due, due_stats->started,
			due_stats->missed);
		protocol_trigger_sensor(due_sensor, 1);
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

static void protocol_resume_acc_stream(void)
{
	bool resumed;

	if (!protocol_schedule[PROTOCOL_SENSOR_ACC].active ||
		sensor_flash_store_full(PROTOCOL_SENSOR_ACC)) {
		return;
	}
	resumed = acc_stream_is_active() ? acc_stream_resume() : acc_stream_start();
	if (!resumed) {
		protocol_schedule[PROTOCOL_SENSOR_ACC].active = 0;
		uart_printf("ACC Stream resume failed\r\n");
	}
}

static void protocol_upload_abort(const char *reason)
{
	if (protocol_upload.active) {
		uart_printf("Flash upload aborted: %s\r\n", reason);
	}
	memset(&protocol_upload, 0, sizeof(protocol_upload));
	upload_paused = 0;
	protocol_resume_acc_stream();
}

static bool protocol_upload_queue_next(void)
{
	uint8_t data[APP_MSG_MAX_LEN - 4];
	uint16_t data_len = 4;
	uint16_t record_len;
	uint16_t max_records;
	uint16_t records;
	uint16_t i;
	uint32_t timestamp = 0;
	uint32_t record_timestamp;

	if (!protocol_upload.active || protocol_upload.packet_pending) {
		return false;
	}
	record_len = sensor_flash_store_record_len(protocol_upload.sensor_id);
	if (record_len == 0) {
		protocol_upload_abort("record length");
		return false;
	}
	max_records = (uint16_t)((sizeof(data) - 4) / record_len);
	records = (uint16_t)(protocol_upload.total_records - protocol_upload.next_index);
	if (records > max_records) {
		records = max_records;
	}
	for (i = 0; i < records; i++) {
		if (!sensor_flash_store_read(protocol_upload.sensor_id,
				protocol_upload.next_index + i, &record_timestamp,
				&data[data_len], record_len)) {
			protocol_upload_abort("Flash read/CRC");
			return false;
		}
		if (i == 0) {
			timestamp = record_timestamp;
		}
		data_len += record_len;
	}
	/* 空日志也返回一个仅含当前时间戳的 0x51，便于 APP 判断上传已完成。 */
	if (records == 0) {
		utc_update();
		timestamp = utc_get_clock();
	}
	memcpy(data, &timestamp, 4);
	protocol_upload.packet_records = records;
	protocol_upload.packet_pending = 1;
	if (!protocol_send_frame(0x51, data, data_len)) {
		protocol_upload_abort("BLE queue");
		return false;
	}
	return true;
}

void app_ble_connect_handle(uint8_t conidx)
{
	ble_conidx = conidx;
	ble_connect_sign = 1;
	last_ble_loop_hs = rwip_time_get().hs & RWIP_MAX_CLOCK_TIME;
	max_ble_loop_gap_hs = window_ble_loop_gap_hs = 0;
	ext_flash_reset_stats();
	flash_count = 4;
	ke_timer_set(APP_LED_FLASH, TASK_APP, 10);
}

void app_ble_disconnect_handle(uint8_t reason)
{
	uint8_t upload_waiting = deferred_command == 0x50;
	app_ble_timing_report("disconnect", reason);
	acc_stream_link_report(reason);
	gpio_set(BLE_LED_PIN, LED_OFF);
	ble_connect_sign = 0;
	direct_pending_mask = 0;
	protocol_tx_reset();
	deferred_command = deferred_upload_type = 0;
	if (upload_waiting) acc_stream_cancel_pause();
	/* 断线时不清 Flash；重新连接后再次发送 0x50 会从第一包重新上传。 */
	protocol_upload_abort("BLE disconnected");
}

void app_ble_send_upd_handler(void)
{
	/* 本函数只负责启动发送；队头只能在 app_ble_send_complete() 中弹出。 */
	if (!tx_busy && ble_connect_sign && tx_count != 0 &&
		app_fee4_send_ntf(ble_conidx, tx_queue[tx_head].len,
						tx_queue[tx_head].data)) {
		tx_busy = 1;
	}

}

void app_ble_send_complete(uint8_t status)
{
	uint8_t upload_packet;
	uint8_t completed_sensor;
	uint8_t clear_ok;

	if (!tx_busy || tx_count == 0) {
		return;
	}
	tx_busy = 0;
	if (status != 0) {
		protocol_tx_reset();
		protocol_upload_abort("BLE notify failed");
		return;
	}
	upload_packet = protocol_upload.active && protocol_upload.packet_pending;
	tx_head = (tx_head + 1) % PROTOCOL_TX_QUEUE_DEPTH;
	tx_count--;

	if (upload_packet) {
		protocol_upload.packet_pending = 0;
		protocol_upload.next_index += protocol_upload.packet_records;
		if (protocol_upload.next_index < protocol_upload.total_records) {
			protocol_upload_queue_next();
			return;
		}
		/* 最后一包已由协议栈确认成功，此时才清除该传感器当天数据。 */
		completed_sensor = protocol_upload.sensor_id;
		clear_ok = 0;
		if (sensor_flash_store_clear(protocol_upload.sensor_id)) {
			clear_ok = 1;
			if (protocol_upload.sensor_id == PROTOCOL_SENSOR_ACC) {
				acc_stream_history_cleared();
			}
			uart_printf("Flash upload complete and history cleared, sensor=%u\r\n",
						protocol_upload.sensor_id + 1);
			/* 若没有收到 0x52，立即建立下一天日志，原周期计划继续运行。 */
			if (protocol_schedule[protocol_upload.sensor_id].active) {
				uint32_t next_start_time;
				utc_update();
				next_start_time = utc_get_clock();
				if (!sensor_flash_store_start(protocol_upload.sensor_id,
						next_start_time,
						protocol_schedule[protocol_upload.sensor_id].period_s)) {
					protocol_schedule[protocol_upload.sensor_id].active = 0;
					uart_printf("Flash next-day history start failed, sensor=%u\r\n",
							protocol_upload.sensor_id + 1);
				}
			}
		} else {
			uart_printf("Flash upload complete but clear failed, sensor=%u\r\n",
						protocol_upload.sensor_id + 1);
		}
		memset(&protocol_upload, 0, sizeof(protocol_upload));
		upload_paused = 0;
		/* 上传任何历史时都暂停加速度；结束后恢复。ACC 清除失败则保留停机。 */
		if (completed_sensor != PROTOCOL_SENSOR_ACC || clear_ok) {
			protocol_resume_acc_stream();
		}
	}
	app_ble_send_upd_handler();
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
	/* 周期采集结果写入外部 Flash，不立即通过 BLE 主动上传。 */
	if (scheduled_pending_mask & mask) {
		uint32_t now;
		utc_update();
		now = utc_get_clock();
		if (!sensor_flash_store_append(sensor_id, now, data, len)) {
			uart_printf("Flash append failed, sensor=%u count=%u\r\n",
						sensor_id + 1, sensor_flash_store_count(sensor_id));
			if (sensor_flash_store_full(sensor_id)) {
				protocol_stop_schedule_full(sensor_id);
			} else {
				/* 非“已满”的写入失败通常是 SPI/Flash 故障，停止该传感器避免反复写。 */
				protocol_schedule[sensor_id].active = 0;
			}
		} else if (sensor_flash_store_full(sensor_id)) {
			protocol_stop_schedule_full(sensor_id);
		}
		scheduled_pending_mask &= ~mask;
	}
	/* 单次读取命令的结果立即通过对应响应命令返回。 */
	if (direct_pending_mask & mask) {
		/*
		 * AS7341 单次命令 0x42 已由 APP 指定 AGAIN，0x43 不重复返回增益。
		 * task_mss.c 内部结构首字段仍是 uint16_t AGAIN，周期历史要保留它，
		 * 因此这里只在单次应答时跳过前 2 字节，返回 F1～F8/CLEAR/NIR。
		 */
		if (sensor_id == PROTOCOL_SENSOR_MSS && len == 22) {
			protocol_send_frame(response_command[sensor_id], data + 2, len - 2);
		} else {
			protocol_send_frame(response_command[sensor_id], data, len);
		}
		direct_pending_mask &= ~mask;
	}
}

bool app_protocol_acc_stream_sample(uint8_t motion_state,
									int16_t x, int16_t y, int16_t z)
{
	uint8_t result;
	utc_update();
	result = app_protocol_acc_commit(utc_get_clock(), motion_state, x, y, z);
	return result == ACC_COMMIT_OK || result == ACC_COMMIT_LAST;
}

uint8_t app_protocol_acc_commit(uint32_t timestamp, uint8_t motion_state,
                               int16_t x, int16_t y, int16_t z)
{
	uint8_t record[1][ACC_HISTORY_RECORD_LEN];
	uint8_t accepted;
	uint8_t result;
	uint16_t value;

	/* 每条加速度历史：0=LOW/1=HIGH，随后为 X/Y/Z 的 int16_t 小端值。 */
	record[0][0] = motion_state;
	value = (uint16_t)x;
	record[0][1] = (uint8_t)value;
	record[0][2] = (uint8_t)(value >> 8);
	value = (uint16_t)y;
	record[0][3] = (uint8_t)value;
	record[0][4] = (uint8_t)(value >> 8);
	value = (uint16_t)z;
	record[0][5] = (uint8_t)value;
	record[0][6] = (uint8_t)(value >> 8);
	result = app_protocol_acc_commit_batch(&timestamp, record, 1, &accepted);
	return accepted == 1 ? result :
		(result == ACC_COMMIT_FULL ? ACC_COMMIT_FULL : ACC_COMMIT_ERROR);
}

uint8_t app_protocol_acc_commit_batch(
		const uint32_t timestamps[],
		uint8_t records[][ACC_HISTORY_RECORD_LEN],
		uint8_t requested, uint8_t *accepted)
{
	uint8_t result;

	if (accepted == 0) {
		return ACC_COMMIT_ERROR;
	}
	*accepted = 0;
	/* Internal queue commit may drain after acquisition has been frozen. */
	if (upload_paused) {
		return ACC_COMMIT_ERROR;
	}
	result = sensor_flash_store_append_acc_batch(
		timestamps, records, requested, accepted);
	if (result == SENSOR_FLASH_BATCH_ERROR) {
		uart_printf("ACC Flash append failed, count=%u\r\n",
				sensor_flash_store_count(PROTOCOL_SENSOR_ACC));
		protocol_schedule[PROTOCOL_SENSOR_ACC].active = 0;
		return ACC_COMMIT_ERROR;
	}
	if (result == SENSOR_FLASH_BATCH_FULL) {
		protocol_schedule[PROTOCOL_SENSOR_ACC].active = 0;
		return ACC_COMMIT_FULL;
	}
	if (result == SENSOR_FLASH_BATCH_LAST) {
		protocol_schedule[PROTOCOL_SENSOR_ACC].active = 0;
		return ACC_COMMIT_LAST;
	}
	return ACC_COMMIT_OK;
}

uint8_t app_protocol_acc_commit_async_begin(
		const uint32_t timestamps[],
		uint8_t records[][ACC_HISTORY_RECORD_LEN], uint8_t requested)
{
	uint8_t result;
	if (upload_paused) return ACC_COMMIT_ERROR;
	result = sensor_flash_store_acc_async_begin(timestamps, records, requested);
	if (result == SENSOR_FLASH_BATCH_BUSY) return ACC_COMMIT_BUSY;
	if (result == SENSOR_FLASH_BATCH_FULL) return ACC_COMMIT_FULL;
	return ACC_COMMIT_ERROR;
}

uint8_t app_protocol_acc_commit_async_process(uint8_t *accepted)
{
	uint8_t result = sensor_flash_store_acc_async_process(accepted);
	if (result == SENSOR_FLASH_BATCH_BUSY) return ACC_COMMIT_BUSY;
	if (result == SENSOR_FLASH_BATCH_OK) return ACC_COMMIT_OK;
	if (result == SENSOR_FLASH_BATCH_LAST) {
		protocol_schedule[PROTOCOL_SENSOR_ACC].active = 0;
		return ACC_COMMIT_LAST;
	}
	if (result == SENSOR_FLASH_BATCH_FULL) {
		protocol_schedule[PROTOCOL_SENSOR_ACC].active = 0;
		return ACC_COMMIT_FULL;
	}
	protocol_schedule[PROTOCOL_SENSOR_ACC].active = 0;
	return ACC_COMMIT_ERROR;
}

bool app_protocol_acc_flash_allowed(void)
{
	uint8_t other = (uint8_t)(scheduled_pending_mask &
		~(0x01U << PROTOCOL_SENSOR_ACC));
	return other == 0 && !protocol_upload.active;
}

void app_protocol_acc_stream_fault(const char *reason)
{
	protocol_schedule[PROTOCOL_SENSOR_ACC].active = 0;
	protocol_schedule[PROTOCOL_SENSOR_ACC].elapsed_s = 0;
	scheduled_pending_mask &= (uint8_t)~(0x01U << PROTOCOL_SENSOR_ACC);
	uart_printf("ACC adaptive sampling stopped: %s\r\n", reason);
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

	if (result == 0 && selected_sensor == PROTOCOL_SENSOR_ACC &&
		(!acc_stream_is_stopped() || !acc_stream_data_complete())) result = 1;
	if (result == 0) {
		utc_set_clock(start_time);
		protocol_schedule[selected_sensor].active = 0;
		/* 重配时丢弃该传感器尚未完成的旧周期样本，避免写进新一天日志。 */
		scheduled_pending_mask &= ~(0x01 << selected_sensor);
		period_due_queue_clear(&low_due_queue, selected_sensor);
		if (!sensor_flash_store_start(selected_sensor, start_time, period)) {
			result = 1;
			uart_printf("V4 schedule rejected: external Flash unavailable\r\n");
		}
	}
	if (result == 0) {
		for (i = 0; i < PROTOCOL_SENSOR_COUNT; i++) {
			if (protocol_schedule[i].active) {
				schedule_was_running = 1;
				break;
			}
		}
		/* 只重置当前传感器，前面已配置的其他四种传感器继续独立运行。 */
		protocol_schedule[selected_sensor].active = 1;
		protocol_schedule[selected_sensor].period_s = period;
		protocol_schedule[selected_sensor].elapsed_s = 0;
		if (selected_sensor == PROTOCOL_SENSOR_ACC && !acc_stream_start()) {
			protocol_schedule[selected_sensor].active = 0;
			result = 1;
			uart_printf("V4 ACC Stream start failed\r\n");
		}
		if (result == 0 && !schedule_was_running) {
			ke_timer_set(APP_PERIOD_TIMER, TASK_APP, 100);
		}
		if (result == 0) {
			uart_printf("V4 schedule sensor=%u, period=%u s\r\n",
					selected_sensor + 1, period);
		}
	} else {
		uart_printf("V4 schedule rejected: period=%u, selected=%u, configured=%u\r\n",
				period, selected_count,
				selected_count == 1 ? configured_sensor_period_s[selected_sensor] : 0);
	}
	protocol_send_frame(0x4F, &result, 1);
}

static void protocol_upload_history(uint8_t type)
{
	uint8_t sensor_id;

	/* 0x50 的 TYPE：1多光谱、2 FIFO、3紫外、4接近、5加速度。 */
	if (type < 1 || type > PROTOCOL_SENSOR_COUNT) {
		uart_printf("Flash upload TYPE error: %u\r\n", type);
		return;
	}
	sensor_id = type - 1;
	if (!sensor_flash_store_ready() || protocol_upload.active ||
		tx_busy || tx_count != 0 || !ble_connect_sign) {
		uart_printf("Flash upload busy/not ready\r\n");
		return;
	}
	upload_paused = 1;
	memset(&protocol_upload, 0, sizeof(protocol_upload));
	protocol_upload.active = 1;
	protocol_upload.sensor_id = sensor_id;
	protocol_upload.total_records = sensor_flash_store_count(sensor_id);
	uart_printf("Flash upload start sensor=%u, total=%u\r\n",
				sensor_id + 1, protocol_upload.total_records);
	protocol_upload_queue_next();
}

void app_protocol_background_process(void)
{
	uint8_t selected_count;
	uint8_t i;
	if (deferred_command == 0) return;

	if (deferred_command == 0x52) {
		if (!acc_stream_is_stopped()) return;
		if (acc_stream_data_complete()) protocol_send_frame(0x53, 0, 0);
		else uart_printf("Stop incomplete: ACC history not fully committed\r\n");
		deferred_command = 0;
		return;
	}
	if (deferred_command == 0x50) {
		if (scheduled_pending_mask != 0) return;
		if (acc_stream_is_active() && !acc_stream_is_paused()) return;
		if (acc_stream_storage_busy()) return;
		protocol_upload_history(deferred_upload_type);
		deferred_command = 0;
		return;
	}
	if (deferred_command == 0x4E) {
		selected_count = 0;
		for (i = 0; i < PROTOCOL_SENSOR_COUNT; i++)
			if (deferred_frame[8 + i] == 1) selected_count++;
		if (selected_count == 1 && deferred_frame[12] == 1 &&
			!acc_stream_is_stopped()) {
			acc_stream_stop();
			return;
		}
		if (acc_stream_storage_busy() || scheduled_pending_mask != 0) return;
		protocol_configure_schedule(deferred_frame);
		deferred_command = 0;
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
	/* Flash控制命令按顺序执行，完成前不接受新的传感器命令。 */
	if (protocol_upload.active || deferred_command != 0) {
		uart_printf("Protocol busy: Flash history is uploading\r\n");
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
		memcpy(deferred_frame, buf, sizeof(deferred_frame));
		deferred_command = 0x4E;
		break;
	case 0x50:
		if (buf[2] < 1 || buf[2] > PROTOCOL_SENSOR_COUNT) {
			uart_printf("Flash upload TYPE error: %u\r\n", buf[2]);
			break;
		}
		if (!sensor_flash_store_ready() || protocol_upload.active ||
			tx_busy || tx_count != 0 || !ble_connect_sign) {
			uart_printf("Flash upload busy/not ready\r\n");
			break;
		}
		period_due_queue_clear_all(&low_due_queue);
		deferred_upload_type = buf[2];
		deferred_command = 0x50;
		acc_stream_pause();
		break;
	case 0x52:
		memset(protocol_schedule, 0, sizeof(protocol_schedule));
		scheduled_pending_mask = 0;
		period_due_queue_clear_all(&low_due_queue);
		ke_timer_clear(APP_PERIOD_TIMER, TASK_APP);
		deferred_command = 0x52;
		acc_stream_stop();
		break;
	default:
		break;
	}
}
