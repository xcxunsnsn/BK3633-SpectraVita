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
	DIST_READ_GAIN1,
	DIST_READ_GAIN2,
};

static UserDataDistStruct userDataDIST; 
static uint8_t temp_again = 0;
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

