/*
 * AS7341 闪烁 FIFO 采集任务。
 * 配置 FD_TIME/GAIN 后每 10 ms 搬运一次 FIFO，累计满 128 个 uint16_t 数据点即停止。
 * 它与普通多光谱任务共用同一颗 AS7341，因此必须通过互斥状态错开访问。
 */
#ifdef FIFO_HOST_TEST
#include "fifo_test_platform.h"
#else
#include "task_fifo.h"
#include "driver_AS7341.h"
#include "rwip_config.h"
#include "user_config.h"
#include "app_task.h"
#include "ke_timer.h"
#include "app_user.h"
#endif
#include <string.h>

#define FIFO_TARGET_SAMPLES          128U
#define FIFO_POLL_TICKS              1U   /* kernel timer unit is 10 ms */
#define FIFO_TIMEOUT_MARGIN_TICKS    10U  /* tolerate 100 ms scheduling/I2C delay */
#define FIFO_STATUS_OVERFLOW         0x80U
#define FIFO_STATUS_TRIGGER_ERROR    0x10U

enum {
	FIFO_IDLE = 0,       /* 尚未启动采集 */
	FIFO_WAIT_DATA,      /* 已启动，等待 FIFO 数据形成 */
};

static uint8_t fifo_status;
static uint16_t fifo_time = SENSOR_DEFAULT_FIFO_TIME;
static uint8_t fifo_gain = SENSOR_DEFAULT_FIFO_GAIN;
static uint16_t fifo_data[FIFO_TARGET_SAMPLES];
static uint16_t fifo_count;
static uint16_t fifo_wait_ticks;
static uint16_t fifo_timeout_ticks;
static uint8_t fifo_overflow_seen;

static uint16_t fifo_calculate_timeout_ticks(uint16_t fd_time)
{
	/* ceil(128 * FD_TIME * 2.78 us / 10 ms), then add a bounded margin. */
	uint32_t scaled = (uint32_t)FIFO_TARGET_SAMPLES * fd_time * 278U;
	uint32_t ticks = (scaled + 999999UL) / 1000000UL;
	if (ticks == 0) ticks = 1;
	return (uint16_t)(ticks + FIFO_TIMEOUT_MARGIN_TICKS);
}

static void fifo_finish(void)
{
	bool stop_ok = AS7341_StopFlickerFIFO();

	if (!stop_ok || fifo_count != FIFO_TARGET_SAMPLES || fifo_overflow_seen) {
		uart_printf("AS7341 FIFO finish: samples=%u ticks=%u overflow=%u stop=%u\r\n",
					fifo_count, fifo_wait_ticks, fifo_overflow_seen, stop_ok ? 1 : 0);
	}
	app_task_enter_lowpower(TASK_ID_FIFO);
	fifo_status = FIFO_IDLE;
	/* V4 defines a fixed 128 x uint16 payload; missing entries stay zero. */
	app_protocol_sensor_data(PROTOCOL_SENSOR_FIFO,
				(uint8_t *)fifo_data, sizeof(fifo_data));
}

void app_read_fifo_task(void)
{
	uint8_t level;
	uint8_t status6;
	uint8_t request;
	uint8_t received;
	bool read_ok;

	if (fifo_status == FIFO_IDLE) {
		/* 普通多光谱正在使用 AS7341 时，稍后重试，不能同时访问。 */
		if (app_task_get_mutex(TASK_ID_FIFO)) {
			ke_timer_set(APP_READ_FIFO, TASK_APP, 20);
			return;
		}
		app_task_quit_lowpower(TASK_ID_FIFO);
		memset(fifo_data, 0, sizeof(fifo_data));
		fifo_count = 0;
		fifo_wait_ticks = 0;
		fifo_overflow_seen = 0;
		fifo_timeout_ticks = fifo_calculate_timeout_ticks(fifo_time);
		if (!AS7341_StartFlickerFIFO(fifo_time, fifo_gain)) {
			uart_printf("AS7341 FIFO start failed: FD_TIME=%u gain=%u\r\n",
						fifo_time, fifo_gain);
			fifo_finish();
			return;
		}
		fifo_status = FIFO_WAIT_DATA;
		ke_timer_set(APP_READ_FIFO, TASK_APP, FIFO_POLL_TICKS);
		return;
	}

	fifo_wait_ticks = (uint16_t)(fifo_wait_ticks + FIFO_POLL_TICKS);
	if (!AS7341_GetFlickerFIFOStatus(&level, &status6)) {
		uart_printf("AS7341 FIFO status read failed at tick=%u\r\n",
					fifo_wait_ticks);
		if (fifo_wait_ticks >= fifo_timeout_ticks) fifo_finish();
		else ke_timer_set(APP_READ_FIFO, TASK_APP, FIFO_POLL_TICKS);
		return;
	}

	if ((status6 & FIFO_STATUS_OVERFLOW) != 0) fifo_overflow_seen = 1;
	if ((status6 & FIFO_STATUS_TRIGGER_ERROR) != 0)
		uart_printf("AS7341 FIFO trigger timing error, STATUS6=0x%02x\r\n", status6);

	request = level;
	if (request > FIFO_TARGET_SAMPLES - fifo_count)
		request = (uint8_t)(FIFO_TARGET_SAMPLES - fifo_count);
	if (request != 0) {
		read_ok = AS7341_ReadFlickerFIFO(&fifo_data[fifo_count], request,
										&received);
		fifo_count = (uint16_t)(fifo_count + received);
		if (!read_ok)
			uart_printf("AS7341 FIFO data read failed: got=%u total=%u\r\n",
						received, fifo_count);
	}

	if (fifo_count >= FIFO_TARGET_SAMPLES || fifo_wait_ticks >= fifo_timeout_ticks) {
		fifo_finish();
		return;
	}
	ke_timer_set(APP_READ_FIFO, TASK_APP, FIFO_POLL_TICKS);
}

void fifo_collect_once_report(uint16_t atime, uint8_t again)
{
	fifo_time = atime;
	fifo_gain = again;
	if (fifo_status == FIFO_IDLE) {
		app_read_fifo_task();
	}
}
