#include "task_acc.h"
#include "driver_ADXL345.h"
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
	ACC_PARAM_INIT = 0,
	ACC_READ_DATA,
};

static UserDataAccStruct userDataACC; 	

static uint8_t acc_status;

void app_read_acc_task(void)
{   
	switch(acc_status)
	{
		case ACC_PARAM_INIT:
			app_task_quit_lowpower(TASK_ID_ACC);
			ADXL345_Init();
			ADXL345_Enable(true);
			acc_status = ACC_READ_DATA;
			ke_timer_set(APP_READ_ACC, TASK_APP, 10);
			break;
		
		case ACC_READ_DATA:
			ADXL345_read_data(&userDataACC.data[userDataACC.data_index].acc_x1,&userDataACC.data[userDataACC.data_index].acc_y1,&userDataACC.data[userDataACC.data_index].acc_z1);
			app_protocol_sensor_data(PROTOCOL_SENSOR_ACC,
					(uint8_t *)&userDataACC.data[userDataACC.data_index],
					sizeof(SensorAccStruct));
			ADXL345_Enable(false);
			app_task_enter_lowpower(TASK_ID_ACC);
			if(userDataACC.data_index < ACC_DATA_NUM_MAX-1)
			{
				userDataACC.data_index++;
			}
			acc_status = ACC_PARAM_INIT;
	}
}


void acc_collect_once_report(uint8_t again)
{
	(void)again;
	if (acc_status == ACC_PARAM_INIT)
	{
		app_read_acc_task();
	}
}
