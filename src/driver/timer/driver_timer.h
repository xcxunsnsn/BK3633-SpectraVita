//======================================================
// File Name:     BK_TIMER_Api.h
// Author:        yubei
// Created Date:  @2015-02-012
// Description:   funtion to apply TIMER Interface
//======================================================

#ifndef _DRIVER_TIMER__
#define _DRIVER_TIMER__

#include "Define_system.h"
typedef void (*TIMER_INT_CALLBACK_T)(void);

#define T_16M    1
#define T_32K    0

#define TIMER_0         0x00
#define TIMER_1         0x01
#define TIMER_2         0x02

#if    (KEYBOARD)
typedef enum{
    cRC_TIMER_2ms = 0x0,
    cRC_TIMER_4ms,
    cRC_TIMER_8ms,
}eRC_TIMER_DUTY;

#endif
extern volatile uint8_t bk_timer_hit;

void timer_cb_register(uint8_t Timer, uint8_t index,TIMER_INT_CALLBACK_T cb);
void Timer_Initial(uint8_t timer,uint8_t src,uint8_t div);
void Timer_ISR(void);
void Timer0_Stop(uint8_t index);

void Timer0_Start(uint8_t index,uint32_t time);
void Timer1_Start(uint8_t index,uint32_t cnt);
void Timer1_ReStart(uint8_t index,uint32_t cnt);
void Timer1_Stop(uint8_t index);
void Timer_ISR0(void);
void Timer_ISR1(void);
void Timer_ISR2(void);
void Delay(int num);
void Timer0_1m5_Start(uint8_t index,uint32_t cnt);



#endif


