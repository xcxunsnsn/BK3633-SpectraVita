#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FIFO_HOST_TEST 1
#include "../../projects/app_gatt/app/src/task_fifo.c"

#define CHECK(value) do { if (!(value)) { \
	printf("FAIL line %d: %s\n", __LINE__, #value); exit(1); \
} } while (0)

static uint16_t fake_next_sample;
static uint16_t fake_level;
static uint8_t fake_samples_per_poll;
static int fake_start_ok;
static int fake_status_ok;
static int fake_stop_ok;
static int fake_fail_read_after;
static int fake_timer_pending;
static uint32_t fake_timer_delay;
static uint32_t fake_start_calls;
static uint32_t fake_stop_calls;
static uint32_t fake_read_failures;
static uint32_t fake_quit_calls;
static uint32_t fake_enter_calls;
static uint32_t fake_report_calls;
static uint16_t fake_start_time;
static uint8_t fake_start_gain;
static uint8_t fake_report[256];

static void reset_fakes(void)
{
	fake_next_sample = 1;
	fake_level = 0;
	fake_samples_per_poll = 5;
	fake_start_ok = 1;
	fake_status_ok = 1;
	fake_stop_ok = 1;
	fake_fail_read_after = -1;
	fake_timer_pending = 0;
	fake_timer_delay = 0;
	fake_start_calls = 0;
	fake_stop_calls = 0;
	fake_read_failures = 0;
	fake_quit_calls = 0;
	fake_enter_calls = 0;
	fake_report_calls = 0;
	fake_start_time = 0;
	fake_start_gain = 0;
	memset(fake_report, 0xA5, sizeof(fake_report));
}

bool AS7341_StartFlickerFIFO(uint16_t fd_time, uint8_t fd_gain)
{
	fake_start_calls++;
	fake_start_time = fd_time;
	fake_start_gain = fd_gain;
	return fake_start_ok != 0;
}

bool AS7341_GetFlickerFIFOStatus(uint8_t *level, uint8_t *status6)
{
	uint16_t produced;
	if (!fake_status_ok) return false;
	produced = (uint16_t)(fake_level + fake_samples_per_poll);
	*status6 = produced > 128 ? 0x80 : 0;
	fake_level = produced > 128 ? 128 : produced;
	*level = (uint8_t)fake_level;
	return true;
}

bool AS7341_ReadFlickerFIFO(uint16_t *buf, uint8_t max_samples,
								 uint8_t *len)
{
	uint8_t i;
	*len = 0;
	for (i = 0; i < max_samples; i++) {
		if (fake_fail_read_after == i) {
			fake_fail_read_after = -1;
			fake_read_failures++;
			return false;
		}
		CHECK(fake_level != 0);
		buf[i] = fake_next_sample++;
		fake_level--;
		*len = (uint8_t)(i + 1);
	}
	return true;
}

bool AS7341_StopFlickerFIFO(void)
{
	fake_stop_calls++;
	return fake_stop_ok != 0;
}

bool app_task_get_mutex(uint8_t task_id)
{
	CHECK(task_id == TASK_ID_FIFO);
	return false;
}

void app_task_quit_lowpower(uint8_t task_id)
{
	CHECK(task_id == TASK_ID_FIFO);
	fake_quit_calls++;
}

void app_task_enter_lowpower(uint8_t task_id)
{
	CHECK(task_id == TASK_ID_FIFO);
	fake_enter_calls++;
}

void app_protocol_sensor_data(uint8_t sensor_id, const uint8_t *data,
								  uint16_t len)
{
	CHECK(sensor_id == PROTOCOL_SENSOR_FIFO);
	CHECK(len == sizeof(fake_report));
	memcpy(fake_report, data, len);
	fake_report_calls++;
}

void ke_timer_set(uint16_t timer_id, uint16_t task, uint32_t delay)
{
	CHECK(timer_id == APP_READ_FIFO);
	CHECK(task == TASK_APP);
	fake_timer_pending = 1;
	fake_timer_delay = delay;
}

int uart_printf(const char *format, ...)
{
	(void)format;
	return 0;
}

static uint16_t report_sample(uint16_t index)
{
	return (uint16_t)(fake_report[index * 2] |
						 ((uint16_t)fake_report[index * 2 + 1] << 8));
}

static uint32_t run_until_report(uint32_t limit)
{
	uint32_t callbacks = 0;
	while (fake_report_calls == 0 && callbacks < limit) {
		CHECK(fake_timer_pending != 0);
		CHECK(fake_timer_delay == 1);
		fake_timer_pending = 0;
		app_read_fifo_task();
		callbacks++;
	}
	return callbacks;
}

static void test_collects_exactly_128_samples(void)
{
	uint16_t i;
	uint32_t callbacks;
	reset_fakes();
	fifo_collect_once_report(720, 5);
	CHECK(fake_start_calls == 1 && fake_start_time == 720);
	CHECK(fake_start_gain == 5 && fake_quit_calls == 1);
	callbacks = run_until_report(40);
	CHECK(callbacks == 26);
	CHECK(fake_report_calls == 1 && fake_stop_calls == 1);
	CHECK(fake_enter_calls == 1);
	for (i = 0; i < 128; i++) CHECK(report_sample(i) == (uint16_t)(i + 1));
}

static void test_partial_i2c_read_is_retried_without_a_gap(void)
{
	uint16_t i;
	reset_fakes();
	fifo_collect_once_report(720, 5);
	CHECK(fake_timer_pending != 0);
	fake_timer_pending = 0;
	app_read_fifo_task();
	fake_fail_read_after = 2;
	CHECK(fake_timer_pending != 0);
	fake_timer_pending = 0;
	app_read_fifo_task();
	CHECK(fake_read_failures == 1);
	CHECK(run_until_report(40) < 40);
	for (i = 0; i < 128; i++) CHECK(report_sample(i) == (uint16_t)(i + 1));
}

static void test_timeout_reports_fixed_zero_padded_payload(void)
{
	uint16_t i;
	uint32_t callbacks;
	reset_fakes();
	fake_samples_per_poll = 0;
	fifo_collect_once_report(720, 5);
	callbacks = run_until_report(40);
	/* ceil(128 * 720 * 2.78 us / 10 ms) + 10 margin ticks. */
	CHECK(callbacks == 36);
	CHECK(fake_report_calls == 1 && fake_stop_calls == 1);
	for (i = 0; i < 128; i++) CHECK(report_sample(i) == 0);
}

static void test_start_failure_finishes_and_releases_power(void)
{
	uint16_t i;
	reset_fakes();
	fake_start_ok = 0;
	fifo_collect_once_report(720, 5);
	CHECK(fake_report_calls == 1 && fake_stop_calls == 1);
	CHECK(fake_quit_calls == 1 && fake_enter_calls == 1);
	CHECK(fake_timer_pending == 0);
	for (i = 0; i < 128; i++) CHECK(report_sample(i) == 0);
}

int main(void)
{
	test_collects_exactly_128_samples();
	test_partial_i2c_read_is_retried_without_a_gap();
	test_timeout_reports_fixed_zero_padded_payload();
	test_start_failure_finishes_and_releases_power();
	puts("PASS: AS7341 flicker FIFO host tests");
	return 0;
}
