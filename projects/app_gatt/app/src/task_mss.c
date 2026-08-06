#include "task_mss.h"
#include "driver_AS7341.h"
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
#include "nvds.h" 
#include "app_user.h"

#define NVDS_TAG_FACTOR	0xB0

#define DEFAULT_MSS_FACTOR	100
#define MSS_DATA_NUM_MAX	20

enum mss_status_e{
	MSS_INIT = 0,       //power on delay 200us
	MSS_READ_ONE = 1,   //200ms
    MSS_READ_TWO = 2,   //200ms
	MSS_READ_THREE,   //200ms
    MSS_READ_FOUR,   //200ms
    MSS_READ_LAST, //600ms
};

typedef struct {
	uint16_t f1;
	uint16_t f2;
	uint16_t f3;
	uint16_t f4;
	uint16_t f5;
	uint16_t f6;
	uint16_t f7;
	uint16_t f8;
	uint16_t clear;
	uint16_t nir;
}  FactorStruct;

typedef struct {
	uint16_t agin;
	uint16_t f1;
	uint16_t f2;
	uint16_t f3;
	uint16_t f4;
	uint16_t f5;
	uint16_t f6;
	uint16_t f7;
	uint16_t f8;
	uint16_t clear;
	uint16_t nir;
} SensorMssStruct;

typedef struct {
	uint16_t 	save_period;
	uint16_t 	data_index;
	uint16_t	remain_num;
	SensorMssStruct 	data[MSS_DATA_NUM_MAX];
	uint8_t 	batt_level;
    uint8_t     upload_sign;
} UserDataMssStruct;	


static UserDataMssStruct userDataMSS; 	

static uint8_t temp_atime = SENSOR_DEFAULT_MSS_ATIME;
static uint16_t temp_astep = SENSOR_DEFAULT_MSS_ASTEP;
static uint8_t temp_again = SENSOR_DEFAULT_MSS_GAIN;

static uint8_t mss_atime = SENSOR_DEFAULT_MSS_ATIME;
static uint16_t mss_astep = SENSOR_DEFAULT_MSS_ASTEP;
static uint8_t mss_again = SENSOR_DEFAULT_MSS_GAIN;
static uint8_t mss_status = MSS_INIT;

static uint8_t remain_report = 0;

static SensorMssStruct latestData;
static FactorStruct mssFactor;

sModeOneData_t data1;
sModeTwoData_t data2;

void app_mss_integration_task(void)
{
	uint32_t temp;
	
	if(app_task_get_mutex(TASK_ID_MSS) == true)
	{
		ke_timer_set(APP_MSS_INTEGRATION, TASK_APP,20);
		return;
	}
	
    switch(mss_status)
    {
        case MSS_INIT:
			app_task_quit_lowpower(TASK_ID_MSS);
			AS7341_Init(eSpm);
			mss_again = 0;
			if(remain_report == 2)
			{
				AS7341_ATIME_config(temp_atime);
				AS7341_ASTEP_config(temp_astep);
				AS7341_AGAIN_config(temp_again);
                remain_report = 1;
                mss_status = MSS_READ_THREE;
			}else{
				AS7341_ATIME_config(mss_atime);
				AS7341_ASTEP_config(mss_astep);
				AS7341_AGAIN_config(mss_again);
                mss_status = MSS_READ_ONE;
			}
			AS7341_Enable(true);
            AS7341_StartMeasure(eF1F4ClearNIR);
            
            ke_timer_set(APP_MSS_INTEGRATION, TASK_APP,20);
            break;
        
        case MSS_READ_ONE:
            data1 = AS7341_ReadSpectralDataOne();
            AS7341_StartMeasure(eF5F8ClearNIR);
            mss_status = MSS_READ_TWO;
            ke_timer_set(APP_MSS_INTEGRATION, TASK_APP,20);
            break;
        
        case MSS_READ_TWO:
            data2 = AS7341_ReadSpectralDataTwo();
            mss_status = MSS_READ_THREE;

            latestData.f1 = data1.channel1 * mssFactor.f1 / DEFAULT_MSS_FACTOR;
            latestData.f2 = data1.channel2 * mssFactor.f2 / DEFAULT_MSS_FACTOR;
            latestData.f3 = data1.channel3 * mssFactor.f3 / DEFAULT_MSS_FACTOR;
            latestData.f4 = data1.channel4 * mssFactor.f4 / DEFAULT_MSS_FACTOR;
            latestData.f5 = data2.channel5 * mssFactor.f5 / DEFAULT_MSS_FACTOR;
            latestData.f6 = data2.channel6 * mssFactor.f6 / DEFAULT_MSS_FACTOR;
            latestData.f7 = data2.channel7 * mssFactor.f7 / DEFAULT_MSS_FACTOR;
            latestData.f8 = data2.channel8 * mssFactor.f8 / DEFAULT_MSS_FACTOR;
            latestData.clear = data2.CLEAR * mssFactor.clear / DEFAULT_MSS_FACTOR;
            latestData.nir = data2.NIR * mssFactor.nir / DEFAULT_MSS_FACTOR;
		
			if (latestData.clear == 0)
			{
				uart_printf("AS7341 clear channel is zero; use minimum gain\n");
				temp = 0;
				mss_again = 0;
			}
			else
			{
				temp = 600000/latestData.clear/2;
		
				if(temp > 5120)
				{
					mss_again = 10;
				}else if(temp > 2560)
				{
					mss_again = 9;
				}else if(temp > 1280)
				{
					mss_again = 8;
				}else if(temp > 640)
				{
					mss_again = 7;
				}else if(temp > 320)
				{
					mss_again = 6;
				}else if(temp > 160)
				{
					mss_again = 5;
				}else if(temp > 80)
				{
					mss_again = 4;
				}else if(temp > 40)
				{
					mss_again = 3;
				}else if(temp > 20)
				{
					mss_again = 2;
				}else if(temp > 10)
				{
					mss_again = 1;
				}else{
					mss_again = 0;
				}
			}
			AS7341_ATIME_config(mss_atime);
			AS7341_ASTEP_config(mss_astep);
			AS7341_AGAIN_config(mss_again);
			
			AS7341_StartMeasure(eF1F4ClearNIR);
			ke_timer_set(APP_MSS_INTEGRATION, TASK_APP,20);
            break;
			
		case MSS_READ_THREE:
			data1 = AS7341_ReadSpectralDataOne();
            AS7341_StartMeasure(eF5F8ClearNIR);
            mss_status = MSS_READ_FOUR;
            ke_timer_set(APP_MSS_INTEGRATION, TASK_APP,20);
            break;
		
		case MSS_READ_FOUR:
			data2 = AS7341_ReadSpectralDataTwo();
            mss_status = MSS_INIT;
			app_task_enter_lowpower(TASK_ID_MSS);
			//AS7341_disableALL();
            latestData.f1 = data1.channel1 * mssFactor.f1 / DEFAULT_MSS_FACTOR;
            latestData.f2 = data1.channel2 * mssFactor.f2 / DEFAULT_MSS_FACTOR;
            latestData.f3 = data1.channel3 * mssFactor.f3 / DEFAULT_MSS_FACTOR;
            latestData.f4 = data1.channel4 * mssFactor.f4 / DEFAULT_MSS_FACTOR;
            latestData.f5 = data2.channel5 * mssFactor.f5 / DEFAULT_MSS_FACTOR;
            latestData.f6 = data2.channel6 * mssFactor.f6 / DEFAULT_MSS_FACTOR;
            latestData.f7 = data2.channel7 * mssFactor.f7 / DEFAULT_MSS_FACTOR;
            latestData.f8 = data2.channel8 * mssFactor.f8 / DEFAULT_MSS_FACTOR;
            latestData.clear = data2.CLEAR * mssFactor.clear / DEFAULT_MSS_FACTOR;
            latestData.nir = data2.NIR * mssFactor.nir / DEFAULT_MSS_FACTOR;
			latestData.agin = mss_again;
            if(remain_report == 1)
            {
                latestData.agin = temp_again;
                remain_report = 0;
                app_protocol_sensor_data(PROTOCOL_SENSOR_MSS,
								 (uint8_t *)&latestData, sizeof(latestData));
            }else{
				memcpy(&userDataMSS.data[userDataMSS.data_index],&latestData,sizeof(latestData));
				if(userDataMSS.data_index < MSS_DATA_NUM_MAX-1)
				{
					userDataMSS.data_index++;
				}
			}

			if(remain_report == 2)
			{
				app_mss_integration_task();
			}
            break;
    }
}

void mss_collect_once_report(uint8_t atime,uint16_t astep,uint8_t again)
{
	temp_atime = atime;
	temp_astep = astep;
	temp_again = again;
	
	remain_report = 2;
	if(mss_status == MSS_INIT)
	{
		app_mss_integration_task();
	} 
}

void mssFactor_init(void)
{
	FactorStruct temp;
	uint8_t len;
    if (nvds_get(NVDS_TAG_FACTOR, &len, (uint8_t *)&temp) == NVDS_OK )//&&\
		temp.fcs == data_fcs((uint8_t *)&temp,sizeof(temp) - 1)) 
	{
		memcpy(&mssFactor,&temp,sizeof(temp));	
		//uart_printf("A %d\n",temp.fcs);
	}else{
		mssFactor.f1 = DEFAULT_MSS_FACTOR; mssFactor.f2 = DEFAULT_MSS_FACTOR; mssFactor.f3 = DEFAULT_MSS_FACTOR;
        mssFactor.f4 = DEFAULT_MSS_FACTOR; mssFactor.f5 = DEFAULT_MSS_FACTOR; mssFactor.f6 = DEFAULT_MSS_FACTOR;
        mssFactor.f7 = DEFAULT_MSS_FACTOR; mssFactor.f8 = DEFAULT_MSS_FACTOR; mssFactor.nir = DEFAULT_MSS_FACTOR;
		mssFactor.clear = DEFAULT_MSS_FACTOR; 
        //mssFactor.fcs = data_fcs((uint8_t *)&mssFactor,sizeof(mssFactor) - 1);
        nvds_put(NVDS_TAG_FACTOR,sizeof(mssFactor),(uint8_t *)&mssFactor);
		//uart_printf("B %d\n",mssFactor.fcs);
	}
	uart_printf("MSS factor is %d %d %d %d %d %d %d %d \n%d %d \n",mssFactor.f1,mssFactor.f2,\
			mssFactor.f3,mssFactor.f4,mssFactor.f5,mssFactor.f6,mssFactor.f7,mssFactor.f8,mssFactor.nir,mssFactor.clear);
}
