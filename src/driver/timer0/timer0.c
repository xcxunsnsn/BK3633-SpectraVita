/**
****************************************************************************************
*
* @file timer.c
*
* @brief timer initialization and specific functions
*
* Copyright (C) Beken 2019-2022
*
* $Rev: $
*
****************************************************************************************
*/

/**
****************************************************************************************
* @addtogroup TIME
* @ingroup TIME
* @brief TIME
*
* This is the driver block for TIME
* @{
****************************************************************************************
*/


#include <stddef.h>     // standard definition
#include "rwip_config.h"
#include "rwip.h"
#include "BK3633_RegList.h"
#include "timer0.h"
#include "reg_ipcore.h"
#include "icu.h"      // timer definition
#include "gpio.h"
#include "uart.h"

void (*p_TIM0_Int_Handler[TIMER_NUMBER_MAX])(unsigned char ucTIM_channel);
void (*p_TIM1_Int_Handler[TIMER_NUMBER_MAX])(unsigned char ucTIM_channel);

TMR_DRV_DESC tmr0_env ,tmr1_env;

static void time0_env_init(void)
{
    static uint8_t init_flag = 0;
    if(init_flag == 0)
    {
        memset(&tmr0_env,sizeof(TMR_DRV_DESC),0);
        init_flag = 1;
    }
}
void timer0_init(uint8_t ch, uint8_t clk_div,uint8_t restart,uint32_t timer,void (*p_Int_Handler)(unsigned char ucChannel))//timer uints 1ms
{
   // uart_printf("timer0_init[%d],%d \r\n",ch,timer);
    uint32_t timer_val;
        
    timer_val = timer * 16; // 1us
    
    if (ch >= TIMER_NUMBER_MAX)
    {
        return;
    }
    if (clk_div > 0Xf)
    {
        return;
    }
    time0_env_init();
       
    clrf_SYS_Reg0x3_tim0_pwd;
    set_SYS_Reg0x4_tim0_sel(1);
    if(!(tmr0_env.init_en & 0x07))
    {
        set_TIMER0_Reg0x3_clk_div(clk_div); 
        tmr0_env.clk_div = clk_div;
    }
    timer_val =  timer_val /(tmr0_env.clk_div + 1); 
      
    tmr0_env.init_en |=(0x01 << ch);   
    tmr0_env.restart[ch] = restart;
    tmr0_env.p_Int_Handler[ch] = p_Int_Handler;
    tmr0_env.timer_set_val[ch] = timer_val;
    tmr0_env.init_en |= (0x01 << (ch + 3));
    switch(ch)
    {
        case 0:
        {
            addTIMER0_Reg0x0 = timer_val;
            setf_TIMER0_Reg0x3_timer0_en;
            
        }break;
        case 1:
        {
             addTIMER0_Reg0x1 = timer_val;
            setf_TIMER0_Reg0x3_timer1_en;
        }break;
        case 2:
        {
            addTIMER0_Reg0x2 = timer_val;
            setf_TIMER0_Reg0x3_timer2_en;
        }break;
        
        default:
            break;
    }
    setf_SYS_Reg0x10_int_timer0_en;  
}

void timer0_deinit(uint8_t ch)//timer uints 1ms
{
    uart_printf("timer0_deinit[%d] \r\n",ch);
   
    if (ch >= TIMER_NUMBER_MAX)
    {
        return;
    }
    switch(ch)
    {
        case 0:
        {          
            clrf_TIMER0_Reg0x3_timer0_en;           
        }break;
        case 1:
        {      
            clrf_TIMER0_Reg0x3_timer1_en;
        }break;
        case 2:
        {
          
            clrf_TIMER0_Reg0x3_timer2_en;
        }break;
        
        default:
            break;
    }  
    tmr0_env.init_en &= ~(0x01 << ch);
    tmr0_env.init_en &= ~(0x01 << (ch + 3));
    if(!(tmr0_env.init_en & 0x07))
    {
        setf_SYS_Reg0x3_tim0_pwd;
        clrf_SYS_Reg0x10_int_timer0_en;  
    }
    tmr0_env.p_Int_Handler[ch] = NULL;  
    uart_printf("tmr0_env.init_en:%x\r\n",tmr0_env.init_en);
}


void timer0_set(uint8_t ch,uint8_t restart,uint32_t timer)
{
    uart_printf("timer0_set[%d],%d \r\n",ch,timer);
    if (ch >= TIMER_NUMBER_MAX)
    {
        return;
    }
    if(!(tmr0_env.init_en & (0x01 << ch)))
    {
        uart_printf("tmr0_[%d] not init\r\n",ch);
        return;
    }
    uint32_t timer_val;
    timer_val = timer * 16;
    timer_val =  timer_val /(tmr0_env.clk_div + 1);  
    uart_printf("timer_val:%d\r\n",timer_val);
    tmr0_env.restart[ch] = restart;
     switch(ch)
    {
        case 0:
        {   
            clrf_TIMER0_Reg0x3_timer0_en; 
            addTIMER0_Reg0x0 = timer_val; 
            setf_TIMER0_Reg0x3_timer0_en;            
        }break;
        case 1:
        {      
            clrf_TIMER0_Reg0x3_timer1_en;
            addTIMER0_Reg0x1 = timer_val;
            setf_TIMER0_Reg0x3_timer1_en;
        }break;
        case 2:
        {
            clrf_TIMER0_Reg0x3_timer2_en;
            addTIMER0_Reg0x2 = timer_val;
            setf_TIMER0_Reg0x3_timer2_en;
        }break;
        
        default:
            break;
    }  

}

void timer0_en(uint8_t ch)
{
    addTIMER0_Reg0x3 &= (0x01 << ch);
}

void timer0_disen(uint8_t ch)
{
    addTIMER0_Reg0x3 &= (0x01 << ch);
}

void timer0_isr(void)
{
    int i;
    unsigned long ulIntStatus;

    ulIntStatus = (addTIMER0_Reg0x3 >> 7)& 0x7;;
  //  uart_printf("ulIntStatus:0x%x\r\n",ulIntStatus);
    for (i=0; i<TIMER_NUMBER_MAX; i++)
    {
        if (ulIntStatus & (0x01<<i))
        {
            if (!tmr0_env.restart[i])
            {
                addTIMER0_Reg0x3 &= ~(0x01 << i);
              //  uart_printf("addTIMER0_Reg0x3:0x%x\r\n",addTIMER0_Reg0x3);
            }
            if (tmr0_env.p_Int_Handler[i] != NULL)
            {
                (void)tmr0_env.p_Int_Handler[i]((unsigned char)i);
            }
        }
    }    
    do
    {
        addTIMER0_Reg0x3 |= (ulIntStatus >> 7);
    } while ((addTIMER0_Reg0x3 >> 7) & ulIntStatus & 0x7);   // delays
}


void timer0_0_isr_demo_handler(unsigned char ucChannel)
{
    static  rwip_time_t cur_time;
    static rwip_time_t pre_time;
    static uint32_t test_cnt = 0;
    uint32_t diff_tmr;
    cur_time = rwip_time_get();
    diff_tmr = (cur_time.hs - pre_time.hs) * 3125 /10000;
    pre_time = cur_time;
    uart_printf("t0 test_cnt:%d,d_t:%d\r\n",test_cnt,diff_tmr);   
    test_cnt++; 
    if(test_cnt > 50)
    {
      
        set_PMU_Reg0x1_reg0_w_en(1);// enable reg0 write enable.
        setf_PMU_Reg0x0_all_reset; // force reset all
    }
}

void timer0_test_demo(void)
{
    timer0_init(0,0,1,100000,timer0_0_isr_demo_handler);//units 1us,totoal 100ms
}

