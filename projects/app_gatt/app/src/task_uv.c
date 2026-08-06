#include "task_uv.h"
#include "driver_AS7331.h"
#include "rwip_config.h"          // SW configuration
#include "user_config.h"

#include "app_task.h" 
#include "ke_timer.h"             // Kernel timer
#include "app_user.h"

enum {
	UV_DEV_RESET = 0,
	UV_PARAM_INIT,
	UV_READ_DATA,
};

UserDataUvStruct userDataUV;

static uint8_t uv_status;
static uint8_t uv_read_retry;
static uint8_t uv_gain = SENSOR_DEFAULT_UV_GAIN;

void app_read_uv_task(void)
{
	uint16_t uv_data[4];
	
	switch(uv_status)
	{
		case UV_DEV_RESET:
			app_task_quit_lowpower(TASK_ID_UV);
			if (!AS7331_Init())
			{
				uart_printf("AS7331 init failed\r\n");
				app_task_enter_lowpower(TASK_ID_UV);
				break;
			}
			uv_read_retry = 0;
			uv_status = UV_PARAM_INIT;
			ke_timer_set(APP_READ_UV, TASK_APP,20);
			break;
		
		case UV_PARAM_INIT:
			AS7331_StartWithGain(uv_gain);
			uv_status = UV_READ_DATA;
			ke_timer_set(APP_READ_UV, TASK_APP,20);
			break;
		
		case UV_READ_DATA:
			if (!AS7331_GetData(&uv_data[0],&uv_data[1],&uv_data[2],&uv_data[3]))
			{
				if (uv_read_retry++ < 5)
				{
					ke_timer_set(APP_READ_UV, TASK_APP,10);
					break;
				}

				uart_printf("AS7331 measurement timeout\r\n");
				AS7331_Set_Measurement(AS7331_MEASUREMENT_STOP);
				AS7331_Set_Power_Down(AS7331_PD_ON);
				app_task_enter_lowpower(TASK_ID_UV);
				uv_status = UV_DEV_RESET;
				break;
			}
			userDataUV.data[userDataUV.data_index].temperature = uv_data[0];
			userDataUV.data[userDataUV.data_index].uva = uv_data[1];
			userDataUV.data[userDataUV.data_index].uvb = uv_data[2];
			userDataUV.data[userDataUV.data_index].uvc = uv_data[3];
			app_protocol_sensor_data(PROTOCOL_SENSOR_UV,
					(uint8_t *)&userDataUV.data[userDataUV.data_index],
					sizeof(SensorUvStruct));
			AS7331_Set_Measurement(AS7331_MEASUREMENT_STOP);
			AS7331_Set_Power_Down(AS7331_PD_ON);
			app_task_enter_lowpower(TASK_ID_UV);
			uv_status = UV_DEV_RESET;
			if(userDataUV.data_index < DIST_DATA_NUM_MAX-1)
			{
				userDataUV.data_index++;
			}
			break;
	}		
}

void uv_collect_once_report(uint8_t again)
{
	uv_gain = again;
	if (uv_status == UV_DEV_RESET)
	{
		app_read_uv_task();
	}
}


