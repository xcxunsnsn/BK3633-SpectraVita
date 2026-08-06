/**
 ****************************************************************************************
 *
 * @file timer.h
 *
 * @brief timer Driver for timer operation.
 *
 * Copyright (C) Beken 2019-2021
 *
 *
 ****************************************************************************************
 */

#ifndef _TIMER_H_
#define _TIMER_H_



#include <stdbool.h>          // standard boolean definitions
#include <stdint.h>           // standard integer functions
/**
 ****************************************************************************************
 * @defgroup TIMER 
 * @ingroup DRIVERS
 * @brief timer driver
 *
 * @{
 *
 ****************************************************************************************
 */
#define TIMER_NUMBER_MAX               3
 //----------------------------------------------
// TMR driver description
//----------------------------------------------
typedef struct
{
   
   /* init_en:   TIMER SET
    * bit[0]:  0 init
    * bit[1]:  1 init
    * bit[2]:  2 init
    * bit[3]:  0 en
    * bit[4]:  1 en
    * bit[5]:  2 en
//bit0:0 init,bit1:1 init,bit2:2 init,bit3:0 en, bit4:1 en,bit5:2 en
    */
    unsigned char     init_en;
    unsigned char     clk_div;
    unsigned char     restart[TIMER_NUMBER_MAX];     
    unsigned int      timer_set_val[TIMER_NUMBER_MAX];
    unsigned int      timer_read_val[TIMER_NUMBER_MAX];
                                    // this value must smaller or equal to end value
    void (*p_Int_Handler[TIMER_NUMBER_MAX])(unsigned char ucChannel);     // PWM channel Interrupt Handler
} TMR_DRV_DESC;

void timer0_init(uint8_t ch, uint8_t clk_div,uint8_t restart,uint32_t timer,void (*p_Int_Handler)(unsigned char ucChannel));
void timer0_deinit(uint8_t ch);
void timer0_set(uint8_t ch,uint8_t restart,uint32_t timer);
void timer0_isr(void);
void timer1_isr(void);
#endif //  _TIMER_H_
 
 
