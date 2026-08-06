/**
****************************************************************************************
* @addtogroup RTC
* @ingroup beken corp
* @brief RTC
* @author Alen
*
* This is the driver block for RTC
* @{
****************************************************************************************
*/


#include <stdint.h>        // standard integer definition
#include <string.h>        // string manipulation
#include <stddef.h>        // standard definition
#include "aon_rtc.h"
#include "user_config.h"
#include "BK3633_RegList.h"
#include "uart2.h"
#include "intc.h"
#include "reg_intc.h"

#if(AON_RTC_DRIVER)

#ifndef USE_FREERTOS
void aon_rtc_init(void)
{
    setf_rtc_aon_Reg0x0_rtc_clk_en ;  //enable rtc clk
#ifdef __HID_TSET__
    addrtc_aon_Reg0x1 = AON_RTC_8MS;//AON_RTC_1000MS;//AON_RTC_UP_VALUE;// set up_val
    addrtc_aon_Reg0x2 = AON_RTC_8MS;//AON_RTC_1000MS;//set  tick_val
#else
    addrtc_aon_Reg0x1 = AON_RTC_1000MS;//AON_RTC_UP_VALUE;// set up_val
    addrtc_aon_Reg0x2 = AON_RTC_1000MS;//set  tick_val
    
#endif
    setf_rtc_aon_Reg0x0_rtc_aon_int  ;  // clear aon int
    setf_rtc_aon_Reg0x0_rtc_tick_int ;  //clear tick int
    setf_rtc_aon_Reg0x0_rtc_tick_int_en  ;       //rtc tick_int_enable
    setf_rtc_aon_Reg0x0_rtc_aon_int_en     ;           // rtc aon_int_enable

    setf_SYS_Reg0x10_int_aon_rtc_en  ;   // rtc int enable

}

 unsigned int rtc_cnt=0;

void aon_rtc_isr(void)
{   
    setf_rtc_aon_Reg0x0_rtc_tick_int ;     
    rtc_cnt++;

 //   uart_printf("rtc_cnt=%d\n",rtc_cnt++);
    //uart_printf("rtc_cnt=%d\n",ip_slotclk_sclk_getf());
    //ip_slotclk_sclk_setf(0x10000);   
    //uart_printf("rtc_cnt=%d\n",ip_slotclk_sclk_getf());
    
}
#else

#include "FreeRTOS.h"
#include "task.h"

static const uint32_t rtos_tick_step = (1000/configTICK_RATE_HZ)*32;
static unsigned int rtc_cnt = 0;

void aon_rtc_init(void) //rtos used
{
    setf_rtc_aon_Reg0x0_rtc_clk_en ;  //enable rtc clk
        setf_rtc_aon_Reg0x0_rtc_cnt_reset;
        
        clrf_rtc_aon_Reg0x0_rtc_tick_int_en;
        clrf_rtc_aon_Reg0x0_rtc_aon_int_en;
        setf_rtc_aon_Reg0x0_rtc_aon_int  ;  // clear aon int
    setf_rtc_aon_Reg0x0_rtc_tick_int ;  //clear tick int

    addrtc_aon_Reg0x1 = 0xFFFFFFFF;
    addrtc_aon_Reg0x2 = rtos_tick_step;
        rtc_cnt = rtos_tick_step;
    
    setf_rtc_aon_Reg0x0_rtc_tick_int_en;       //rtc tick_int_enable
    setf_SYS_Reg0x10_int_aon_rtc_en;   // rtc int enable
        clrf_rtc_aon_Reg0x0_rtc_cnt_reset;
}

void aon_rtc_set_sleep_tick(unsigned int ticks)
{
    #if configUSE_TICKLESS_IDLE
        clrf_rtc_aon_Reg0x0_rtc_tick_int_en;
        setf_rtc_aon_Reg0x0_rtc_tick_int;
        if(ticks < 0x7FFFFFFF)
        {
            //uint32_t tick_set = addrtc_aon_Reg0x3 + ticks * rtos_tick_step;
            uint32_t tick_set = rtc_cnt + ticks * rtos_tick_step;
            addrtc_aon_Reg0x2 = tick_set;
            while(addrtc_aon_Reg0x5 != addrtc_aon_Reg0x2){
                addrtc_aon_Reg0x2 = tick_set;
            }
            setf_rtc_aon_Reg0x0_rtc_tick_int;
            intc_status_clear(INT_STATUS_AON_RTC_bit);
            setf_rtc_aon_Reg0x0_rtc_tick_int_en;
        }
    #endif
}

void aon_restore_ticktimer(unsigned int maxticks)
{
    #if configUSE_TICKLESS_IDLE
    uint32_t diff_tick;
    uint32_t restore_tick = 0;
		uint32_t tem_rtc_cnt = rtc_cnt;
		diff_tick = tem_rtc_cnt - addrtc_aon_Reg0x3;
    while(diff_tick < 10 || diff_tick > 0x7FFFFFFF)
    {
        restore_tick++;
        tem_rtc_cnt += rtos_tick_step;
        diff_tick = tem_rtc_cnt - addrtc_aon_Reg0x3;
    }
    if(!restore_tick) return;
    if(restore_tick > maxticks)
		{
			vTaskStepTick(maxticks);
			rtc_cnt += maxticks*rtos_tick_step;
		}
		else
		{
			vTaskStepTick(restore_tick);
			rtc_cnt = tem_rtc_cnt;
		}
		if(get_rtc_aon_Reg0x0_rtc_tick_int) return;
    clrf_rtc_aon_Reg0x0_rtc_tick_int_en;
    setf_rtc_aon_Reg0x0_rtc_tick_int;  
    addrtc_aon_Reg0x2 = tem_rtc_cnt;
    while(addrtc_aon_Reg0x5 != addrtc_aon_Reg0x2){
        addrtc_aon_Reg0x2 = tem_rtc_cnt;
    }
    setf_rtc_aon_Reg0x0_rtc_tick_int_en;
    #endif
}

void aon_rtc_isr(void)
{   
        uint32_t diff_tick;
        uint32_t catchup_ticks = 0;
    setf_rtc_aon_Reg0x0_rtc_tick_int;
        diff_tick = rtc_cnt - addrtc_aon_Reg0x3;
    while(diff_tick < 10 || diff_tick > 0x7FFFFFFF)
        {
            catchup_ticks++;
            rtc_cnt += rtos_tick_step;
            diff_tick = rtc_cnt - addrtc_aon_Reg0x3;
        }
        addrtc_aon_Reg0x2 = rtc_cnt;
        if(catchup_ticks)
        {
            if(catchup_ticks > 1)
            {
							if(xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED)
							{
								xTaskResumeAll();
								if(xTaskCatchUpTicks(catchup_ticks-1) == pdTRUE)
								{
									portEXIT_SWITCHING_ISR(pdTRUE);
								}
								vTaskSuspendAll();
							}
							else
							{
								if(xTaskCatchUpTicks(catchup_ticks-1) == pdTRUE)
								{
									portEXIT_SWITCHING_ISR(pdTRUE);
								}
							}
            }
            #if configUSE_PREEMPTION == 1
            portTICK_INC();
            #else
            void vNonPreemptiveTick( void );
            vNonPreemptiveTick();
            #endif
        }
}

#endif



#endif



