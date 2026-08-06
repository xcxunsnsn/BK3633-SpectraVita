#include "task_fifo.h"
#include "driver_AS7341.h"
#include "rwip_config.h"
#include "user_config.h"
#include "app_task.h"
#include "ke_timer.h"
#include "app_user.h"
#include <string.h>

enum {
	FIFO_IDLE = 0,
	FIFO_WAIT_DATA,
};

static uint8_t fifo_status;
static uint16_t fifo_time = 720;
static uint8_t fifo_gain = 5;
static uint16_t fifo_data[128];

void app_read_fifo_task(void)
{
	uint8_t length = 0;

	if (fifo_status == FIFO_IDLE) {
		if (app_task_get_mutex(TASK_ID_FIFO)) {
			ke_timer_set(APP_READ_FIFO, TASK_APP, 20);
			return;
		}
		app_task_quit_lowpower(TASK_ID_FIFO);
		memset(fifo_data, 0, sizeof(fifo_data));
		AS7341_SetFlickerFIFO(fifo_time, fifo_gain, 2);
		fifo_status = FIFO_WAIT_DATA;
		ke_timer_set(APP_READ_FIFO, TASK_APP, 70);
		return;
	}

	AS7341_ReadFlickerFIFO(fifo_data, &length);
	app_task_enter_lowpower(TASK_ID_FIFO);
	fifo_status = FIFO_IDLE;
	/* V4 defines a fixed 128 x uint16 payload; unread entries stay zero. */
	app_protocol_sensor_data(PROTOCOL_SENSOR_FIFO,
				(uint8_t *)fifo_data, sizeof(fifo_data));
}

void fifo_collect_once_report(uint16_t atime, uint8_t again)
{
	fifo_time = atime;
	fifo_gain = again;
	if (fifo_status == FIFO_IDLE) {
		app_read_fifo_task();
	}
}
