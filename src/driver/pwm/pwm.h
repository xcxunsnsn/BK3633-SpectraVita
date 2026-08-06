/**
 ****************************************************************************************
 *
 * @file pwm.h
 *
 * @brief pwm Driver for pwm operation.
 *
 * Copyright (C) Beken 2019-2021
 *
 *
 ****************************************************************************************
 */

#ifndef _PWM_H_
#define _PWM_H_



#include <stdbool.h>          // standard boolean definitions
#include <stdint.h>           // standard integer functions
/**
 ****************************************************************************************
 * @defgroup PWM 
 * @ingroup DRIVERS
 * @brief pwm driver
 *
 * @{
 *
 ****************************************************************************************
 */
#define PWM_CHANNEL_NUMBER_MAX               3

enum{
    PWM_MODE_IDLE = 0,
    PWM_MODE_PWM  = 1,
    PWM_MODE_TIMER = 2,
    PWM_MODE_CAPTUR = 4,
    PWM_MODE_COUNTER = 5,

};

enum{
    PWM_CLK_ROSC32K = 0,
    PWM_CLK_XTAL16M  = 1,
    PWM_CLK_DPLL = 2,
};
 //----------------------------------------------
// PWM driver description
//----------------------------------------------
typedef struct
{
    unsigned char     channel;        // PWM 0~5, GPIOB P1.0~P1.5

   /* mode:   PWM mode
    * bit[0]:  PWM enable
    *          0: PWM disable
    *          1: PWM enable
    * bit[1]:   PWM interrupt enable
    *          0: PWM interrupt disable
    *          1: PWM interrupt enable
    * bit[4:2]: PWM mode selection
    *          000: IDLE
    *          001: PWM Mode
    *          010: TIMER Mode
    *          100:Captur
    *          101:Counter
    */
    unsigned char     en;
    unsigned char     int_en;
    unsigned char     mode;         //000: IDLE    001: PWM Mode     010: TIMER    100: Capture    101: Counter 
    unsigned char     cpedg_sel;    //00:both edge  01:posedge  10:negedge
    unsigned char     contiu_mode;  //0:not in continue mode  1:continue mode
    unsigned char     clk_src;      // 01:xtal16m   00:rosc32k  1x:DP11
    unsigned char     pre_divid;    // PWM pre-divide clk
    unsigned int    end_value;    // PWM counter end value
    unsigned int    duty_cycle;   // PWM counter duty cycle, 
                                    // this value must smaller or equal to end value
    void (*p_Int_Handler)(unsigned char ucChannel);     // PWM channel Interrupt Handler
} PWM_DRV_DESC;

extern void (*p_PWM0_Int_Handler[PWM_CHANNEL_NUMBER_MAX])(unsigned char ucPWM_channel);
extern void (*p_PWM1_Int_Handler[PWM_CHANNEL_NUMBER_MAX])(unsigned char ucPWM_channel);

void pwm0_init(PWM_DRV_DESC *pwm_drv_desc);
void pwm1_init(PWM_DRV_DESC *pwm_drv_desc);
void pwm1_end_value_duty_cycle_set(uint8_t  ucChannel,uint32_t end_value,uint32_t duty_cycle);

void pwm0_isr(void);
void pwm1_isr(void);
#endif //  _PWM_H_

