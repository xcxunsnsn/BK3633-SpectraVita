/*
 * WH4530A 接近传感器采集任务。
 * 每次采集连续得到 A/B 两个增益下的原始接近值，最终组成 4 字节记录。
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
	DIST_INIT = 0,       /* 初始化并启动命令指定增益 A */
	DIST_READ_GAIN1,     /* 读取 A，然后切换到固定增益 3 */
	DIST_READ_GAIN2,     /* 读取 B，组合 A/B 后上报 */
};

static UserDataDistStruct userDataDIST; 
static uint8_t temp_again = SENSOR_DEFAULT_DIST_GAIN;
static uint8_t dist_status = DIST_INIT;

void app_read_proximity_task(void)
{
	switch(dist_status)
	{
		case DIST_INIT:
			app_task_quit_lowpower(TASK_ID_DIST);
			WH4530_Init();
			WH4530_Set_Gain(temp_again & 0x03);
			WH4530_Enable(true);
			dist_status = DIST_READ_GAIN1;
			ke_timer_set(APP_READ_DIST, TASK_APP,20);
			break;
		
		case DIST_READ_GAIN1:
			/* A 使用命令增益；B 固定使用增益 3，保留原工程双增益逻辑。 */
			userDataDIST.data[userDataDIST.data_index].proximity1 = WH4530A_IIC_Read_Distance();
			WH4530_Enable(false);
			WH4530_Set_Gain(0x03);
			WH4530_Enable(true);
			dist_status = DIST_READ_GAIN2;
			ke_timer_set(APP_READ_DIST, TASK_APP,20);
			break;
		
		case DIST_READ_GAIN2:
			userDataDIST.data[userDataDIST.data_index].proximity2 = WH4530A_IIC_Read_Distance();
			app_protocol_sensor_data(PROTOCOL_SENSOR_DIST,
					(uint8_t *)&userDataDIST.data[userDataDIST.data_index],
					sizeof(SensorDistStruct));
			WH4530_Enable(false);
			app_task_enter_lowpower(TASK_ID_DIST);
			dist_status = DIST_INIT;
			if(userDataDIST.data_index < DIST_DATA_NUM_MAX-1)
			{
				userDataDIST.data_index++;
			}
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

