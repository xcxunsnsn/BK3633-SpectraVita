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


#ifndef __RTC_H__
#define __RTC_H__


#define BASEADDR_AON_RTC                                       0x00800900
//addrtc_aon_Reg0x0
#define AON_RTC_REG0X0                                       *((volatile unsigned long *) (BASEADDR_AON_RTC+0x0*4))
#define POS_AON_RTC_REG0X0_CLK_EN                            6
#define POS_AON_RTC_REG0X0_tick_int                          5
#define POS_AON_RTC_REG0X0_int                               4
#define POS_AON_RTC_REG0X0_tick_int_en                       3
#define POS_AON_RTC_REG0X0_int_en                            2
#define POS_AON_RTC_REG0X0_cnt_stop                          1
#define POS_AON_RTC_REG0X0_cnt_reset                         0


#define AON_RTC_REG0X1                                       *((volatile unsigned long *) (BASEADDR_AON_RTC+0x1*4))
#define AON_RTC_REG0X2                                       *((volatile unsigned long *) (BASEADDR_AON_RTC+0x2*4))
#define AON_RTC_REG0X3                                       *((volatile unsigned long *) (BASEADDR_AON_RTC+0x3*4))



#define AON_RTC_1000MS   32001   //1S
#define AON_RTC_8MS   256   //8ms


void aon_rtc_init(void);
void aon_rtc_isr(void);
#ifdef USE_FREERTOS
void aon_rtc_set_sleep_tick(unsigned int ticks);
void aon_restore_ticktimer(unsigned int maxticks);
#endif

#endif

