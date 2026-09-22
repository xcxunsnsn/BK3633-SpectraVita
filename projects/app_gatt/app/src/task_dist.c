/*
 * WH4530A 接近传感器采集任务。
 * 每次采集先用固定1x得到A，再用命令指定增益得到B，最终组成4字节记录。
 */
#include "task_dist.h"
#include "driver_WH4530.h"
#include "rwip_config.h"          // SW configuration
#include "user_config.h"

#include "app_task.h" 
#include "ke_timer.h"             // Kernel timer
#include "app_dis.h"              // Device Information Module Definition
#include "diss_task.h"
#include "app_batt.h"             // Battery Module Definition
#include "bass_task.h"
#include "app_oads.h"             
#include "oads_task.h"              
#include "gpio.h"
#include "app_user.h"

enum {
	DIST_INIT = 0,
	DIST_CONFIGURE,
	DIST_WAIT_GAIN1,
	DIST_WAIT_GAIN2,
};

#define DIST_RESET_DELAY_TU       2U   /* 20 ms */
#define DIST_READY_POLL_TU        2U   /* 20 ms */
#define DIST_READY_MAX_POLLS     25U   /* 500 ms */
#define DIST_INVALID_VALUE   0xFFFFU

static UserDataDistStruct userDataDIST; 
static uint8_t temp_again = SENSOR_DEFAULT_DIST_GAIN;
static uint8_t dist_status = DIST_INIT;
static uint8_t dist_ready_polls;

static void dist_finish_report(void)
{
	WH4530_Enable(false);
	app_protocol_sensor_data(PROTOCOL_SENSOR_DIST,
			(uint8_t *)&userDataDIST.data[userDataDIST.data_index],
			sizeof(SensorDistStruct));
	app_task_enter_lowpower(TASK_ID_DIST);
	dist_status = DIST_INIT;
	if (userDataDIST.data_index < DIST_DATA_NUM_MAX - 1)
	{
		userDataDIST.data_index++;
	}
}

static bool dist_start_measurement(uint8_t gain)
{
	dist_ready_polls = 0;
	return WH4530_Enable(false) &&
		   WH4530_Set_Gain(gain) &&
		   WH4530_Clear_Status() &&
		   WH4530_Enable(true);
}

static bool dist_measurement_ready(uint8_t *status)
{
	if (!WH4530_Read_Status(status))
	{
		return false;
	}
	return (*status & WH4530_STATUS_PS_READY) != 0;
}

void app_read_proximity_task(void)
{
	SensorDistStruct *sample = &userDataDIST.data[userDataDIST.data_index];
	uint8_t status = 0xFF;
	uint8_t raw[2] = {0xFF, 0xFF};
	bool ready;

	switch(dist_status)
	{
		case DIST_INIT:
			app_task_quit_lowpower(TASK_ID_DIST);
			sample->proximity1 = DIST_INVALID_VALUE;
			sample->proximity2 = DIST_INVALID_VALUE;
			if (!WH4530_Reset())
			{
				uart_printf("WH4530 reset I2C failed\r\n");
				dist_finish_report();
				break;
			}
			dist_status = DIST_CONFIGURE;
			ke_timer_set(APP_READ_DIST, TASK_APP, DIST_RESET_DELAY_TU);
			break;

		case DIST_CONFIGURE:
			/* A固定使用增益代码0（1x），作为不同命令之间的基准。 */
			if (!WH4530_Init() || !dist_start_measurement(0x00))
			{
				uart_printf("WH4530 configuration/readback failed\r\n");
				dist_finish_report();
				break;
			}
			dist_status = DIST_WAIT_GAIN1;
			ke_timer_set(APP_READ_DIST, TASK_APP, DIST_READY_POLL_TU);
			break;

		case DIST_WAIT_GAIN1:
			ready = dist_measurement_ready(&status);
			if (!ready)
			{
				if (status != 0xFF && ++dist_ready_polls < DIST_READY_MAX_POLLS)
				{
					ke_timer_set(APP_READ_DIST, TASK_APP, DIST_READY_POLL_TU);
					break;
				}
				uart_printf("WH4530 gain A timeout/I2C error, flag=%02x polls=%u\r\n",
						status, dist_ready_polls);
				dist_finish_report();
				break;
			}
			if ((status & WH4530_STATUS_DATA_INVALID) ||
				!WH4530_Read_Distance(&sample->proximity1, raw))
			{
				uart_printf("WH4530 gain A invalid/read failed, flag=%02x\r\n", status);
				dist_finish_report();
				break;
			}
			uart_printf("WH4530 PS A gain=0 flag=%02x raw=%02x %02x value=%u\r\n",
					status, raw[0], raw[1], sample->proximity1);
			/* B使用0x48命令DATA低2位指定的增益代码。 */
			if (!dist_start_measurement(temp_again & 0x03))
			{
				uart_printf("WH4530 gain B start failed\r\n");
				dist_finish_report();
				break;
			}
			dist_status = DIST_WAIT_GAIN2;
			ke_timer_set(APP_READ_DIST, TASK_APP, DIST_READY_POLL_TU);
			break;

		case DIST_WAIT_GAIN2:
			ready = dist_measurement_ready(&status);
			if (!ready)
			{
				if (status != 0xFF && ++dist_ready_polls < DIST_READY_MAX_POLLS)
				{
					ke_timer_set(APP_READ_DIST, TASK_APP, DIST_READY_POLL_TU);
					break;
				}
				uart_printf("WH4530 gain B timeout/I2C error, flag=%02x polls=%u\r\n",
						status, dist_ready_polls);
				dist_finish_report();
				break;
			}
			if ((status & WH4530_STATUS_DATA_INVALID) ||
				!WH4530_Read_Distance(&sample->proximity2, raw))
			{
				uart_printf("WH4530 gain B invalid/read failed, flag=%02x\r\n", status);
				dist_finish_report();
				break;
			}
			uart_printf("WH4530 PS B gain=%u flag=%02x raw=%02x %02x value=%u\r\n",
					temp_again & 0x03, status, raw[0], raw[1], sample->proximity2);
			dist_finish_report();
			break;
	}
}

void dist_collect_once_report(uint8_t again)
{
	temp_again = again;
	if(dist_status == DIST_INIT)
	{
		app_read_proximity_task();
	}
}

