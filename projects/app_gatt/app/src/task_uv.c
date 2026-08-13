/*
 * AS7331 紫外传感器状态机：初始化 -> 设置增益并开始测量 -> 读取 UVA/UVB/UVC/温度。
 * 测量结束后主动停止并进入 Power Down，降低周期采集之间的功耗。
 */
#include "task_uv.h"
#include "driver_AS7331.h"
#include "rwip_config.h"          // SW configuration
#include "user_config.h"

#include "app_task.h" 
#include "ke_timer.h"             // Kernel timer
#include "app_user.h"

enum {
	UV_DEV_RESET = 0,    /* 重新初始化 AS7331 */
	UV_PARAM_INIT,       /* 写入增益并开始一次测量 */
	UV_READ_DATA,        /* 等待就绪并读取四路结果 */
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
			/* 数据尚未就绪时最多重试 5 次，避免一直占用传感器电源。 */
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


