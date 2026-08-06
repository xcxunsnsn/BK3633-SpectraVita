/**
****************************************************************************************
*
* @file pwm.c
*
* @brief pwm initialization and specific functions
*
* Copyright (C) Beken 2019-2022
*
* $Rev: $
*
****************************************************************************************
*/

/**
****************************************************************************************
* @addtogroup PWM
* @ingroup PWM
* @brief PWM
*
* This is the driver block for PWM
* @{
****************************************************************************************
*/


#include <stddef.h>     // standard definition
#include "rwip_config.h"
#include "rwip.h"
#include "BK3633_RegList.h"
#include "pwm.h"
#include "reg_ipcore.h"
#include "icu.h"      // timer definition
#include "gpio.h"
#include "uart.h"

void (*p_PWM0_Int_Handler[PWM_CHANNEL_NUMBER_MAX])(unsigned char ucPWM_channel);
void (*p_PWM1_Int_Handler[PWM_CHANNEL_NUMBER_MAX])(unsigned char ucPWM_channel);
void pwm0_init(PWM_DRV_DESC *pwm_drv_desc)
{
    if(pwm_drv_desc == NULL)
    {
        return;
    }
    if (pwm_drv_desc->channel > PWM_CHANNEL_NUMBER_MAX)
    {
        return;
    }
    if (pwm_drv_desc->duty_cycle > pwm_drv_desc->end_value)
    {
        return;
    }
    if (pwm_drv_desc->clk_src > PWM_CLK_DPLL)
    {
        return;
    }
    if(pwm_drv_desc->en)
    {
        clrf_SYS_Reg0x3_pwm0_pwd;
    }else
    {
        setf_SYS_Reg0x3_pwm0_pwd;
    }
        
    set_SYS_Reg0x4_pwm0_sel(pwm_drv_desc->clk_src);   
    switch(pwm_drv_desc->channel)
    {
        case 0:
        {
            set_AON_GPIO_Reg0x8_GPIO8_Config(0xc0);
            
            addPWM0_Reg0x2 = pwm_drv_desc->end_value;
            addPWM0_Reg0x3 = pwm_drv_desc->duty_cycle;
                           
        }break;
        
        case 1:
        {
            set_AON_GPIO_Reg0x9_GPIO9_Config(0xc0);
            addPWM0_Reg0x5 = pwm_drv_desc->end_value;
            addPWM0_Reg0x6 = pwm_drv_desc->duty_cycle;
        
        }break;
        
        case 2:
        {
            set_AON_GPIO_Reg0xa_GPIO10_Config(0xc0);
            addPWM0_Reg0x8 = pwm_drv_desc->end_value;
            addPWM0_Reg0x9 = pwm_drv_desc->duty_cycle;
        
        }break;
        
        default:break;
        
    }
    
     addPWM0_Reg0x0 &= ~(0x7 << (5 * pwm_drv_desc->channel + posPWM0_Reg0x0_pwm0_mode));
     addPWM0_Reg0x0 |= (pwm_drv_desc->mode << (5 * pwm_drv_desc->channel + posPWM0_Reg0x0_pwm0_mode)); //set mode
            
     set_PWM0_Reg0x0_pre_div(pwm_drv_desc->pre_divid);
    
     addPWM0_Reg0x0 &= ~(0x3 << (2 * pwm_drv_desc->channel + posPWM0_Reg0x0_pwm0_cpedg_sel));
     addPWM0_Reg0x0 |= (pwm_drv_desc->cpedg_sel << (2 * pwm_drv_desc->channel + posPWM0_Reg0x0_pwm0_cpedg_sel));//set cpedg_sel
    
     addPWM0_Reg0x0 &= ~(0x1 << (pwm_drv_desc->channel + posPWM0_Reg0x0_pwm0_ctnu_mod));
     addPWM0_Reg0x0 |= (pwm_drv_desc->contiu_mode << (pwm_drv_desc->channel + posPWM0_Reg0x0_pwm0_ctnu_mod));
    
    if(pwm_drv_desc->int_en)
    {                
        setf_SYS_Reg0x10_int_pwm0_en;
        addPWM0_Reg0x0 |= (pwm_drv_desc->int_en << (5 * pwm_drv_desc->channel + posPWM0_Reg0x0_pwm0_int_en));//set int en   
        p_PWM0_Int_Handler[pwm_drv_desc->channel] = pwm_drv_desc->p_Int_Handler;
    }else
    {
        addPWM0_Reg0x0 &= ~(0x1 << (5 * pwm_drv_desc->channel + posPWM0_Reg0x0_pwm0_int_en));
    }
    
    if(pwm_drv_desc->en)
    {
        addPWM0_Reg0x0  |= (pwm_drv_desc->en << (5 * pwm_drv_desc->channel + posPWM0_Reg0x0_pwm0_en));
    }else
    {
        addPWM0_Reg0x0  &= ~(pwm_drv_desc->en << (5 * pwm_drv_desc->channel + posPWM0_Reg0x0_pwm0_en));
    }
    
    
}

void pwm1_init(PWM_DRV_DESC *pwm_drv_desc)
{
    if(pwm_drv_desc == NULL)
    {
        return;
    }
    if (pwm_drv_desc->channel > PWM_CHANNEL_NUMBER_MAX)
    {
        return;
    }
    if (pwm_drv_desc->duty_cycle > pwm_drv_desc->end_value)
    {
        return;
    }
    if (pwm_drv_desc->clk_src > PWM_CLK_DPLL)
    {
        return;
    }
    if(pwm_drv_desc->en)
    {
        clrf_SYS_Reg0x3_pwm1_pwd;
    }else
    {
        setf_SYS_Reg0x3_pwm1_pwd;
    }
    
    set_SYS_Reg0x4_pwm1_sel(pwm_drv_desc->clk_src); 
    switch(pwm_drv_desc->channel)
    {
        case 0:
        {
            set_AON_GPIO_Reg0xb_GPIO11_Config(0x40);
            
            addPWM1_Reg0x2 = pwm_drv_desc->end_value;
            addPWM1_Reg0x3 = pwm_drv_desc->duty_cycle;
                           
        }break;
        
        case 1:
        {
            set_AON_GPIO_Reg0xc_GPIO12_Config(0x40);
            addPWM1_Reg0x5 = pwm_drv_desc->end_value;
            addPWM1_Reg0x6 = pwm_drv_desc->duty_cycle;
        
        }break;
        
        case 2:
        {
            set_AON_GPIO_Reg0xd_GPIO13_Config(0x40);
            addPWM1_Reg0x8 = pwm_drv_desc->end_value;
            addPWM1_Reg0x9 = pwm_drv_desc->duty_cycle;
        
        }break;
        
        default:break;
        
    }
    
     addPWM1_Reg0x0 &= ~(0x7 << (5 * pwm_drv_desc->channel + posPWM1_Reg0x0_pwm0_mode));
     addPWM1_Reg0x0 |= (pwm_drv_desc->mode << (5 * pwm_drv_desc->channel + posPWM1_Reg0x0_pwm0_mode)); //set mode
            
     set_PWM1_Reg0x0_pre_div(pwm_drv_desc->pre_divid);
    
     addPWM1_Reg0x0 &= ~(0x3 << (2 * pwm_drv_desc->channel + posPWM1_Reg0x0_pwm0_cpedg_sel));
     addPWM1_Reg0x0 |= (pwm_drv_desc->cpedg_sel << (2 * pwm_drv_desc->channel + posPWM1_Reg0x0_pwm0_cpedg_sel));//set cpedg_sel
    
     addPWM1_Reg0x0 &= ~(0x1 << (pwm_drv_desc->channel + posPWM1_Reg0x0_pwm0_ctnu_mod));
     addPWM1_Reg0x0 |= (pwm_drv_desc->contiu_mode << (pwm_drv_desc->channel + posPWM1_Reg0x0_pwm0_ctnu_mod));
    
    if(pwm_drv_desc->int_en)
    {                
        setf_SYS_Reg0x10_int_pwm1_en;
        addPWM1_Reg0x0 |= (pwm_drv_desc->int_en << (5 * pwm_drv_desc->channel + posPWM1_Reg0x0_pwm0_int_en));//set int en   
        p_PWM1_Int_Handler[pwm_drv_desc->channel] = pwm_drv_desc->p_Int_Handler;
    }else
    {
        addPWM1_Reg0x0 &= ~(0x1 << (5 * pwm_drv_desc->channel + posPWM1_Reg0x0_pwm0_int_en));
    }
    
    if(pwm_drv_desc->en)
    {
        addPWM1_Reg0x0  |= (pwm_drv_desc->en << (5 * pwm_drv_desc->channel + posPWM1_Reg0x0_pwm0_en));
    }else
    {
        addPWM1_Reg0x0  &= ~(pwm_drv_desc->en << (5 * pwm_drv_desc->channel + posPWM1_Reg0x0_pwm0_en));
    }
    
    
    uart_printf("addPWM1_Reg0x0:0x%08x\r\n",addPWM1_Reg0x0);
    uart_printf("addPWM1_Reg0x1:0x%08x\r\n",addPWM1_Reg0x1);
    uart_printf("addPWM1_Reg0x2:0x%08x\r\n",addPWM1_Reg0x2);
    uart_printf("addPWM1_Reg0x3:0x%08x\r\n",addPWM1_Reg0x3);
    
    uart_printf("addSYS_Reg0x3:0x%08x\r\n",addSYS_Reg0x3);
    uart_printf("addSYS_Reg0x4:0x%08x\r\n",addSYS_Reg0x4);
    
         
}


void pwm1_end_value_duty_cycle_set(uint8_t ucChannel,uint32_t end_value,uint32_t duty_cycle)
{
    switch(ucChannel)
    {
        case 0:
        {                      
            addPWM1_Reg0x2 = end_value;
            addPWM1_Reg0x3 = duty_cycle;                         
        }break;
        
        case 1:
        {           
            addPWM1_Reg0x5 = end_value;
            addPWM1_Reg0x6 = duty_cycle;       
        }break;
        
        case 2:
        {           
            addPWM1_Reg0x8 = end_value;
            addPWM1_Reg0x9 = duty_cycle;       
        }break;
        
        default:break;
        
    }
}



void pwm0_isr(void)
{
    int i;
    unsigned long ulIntStatus;

    ulIntStatus = addPWM0_Reg0x1;

    for (i=0; i<PWM_CHANNEL_NUMBER_MAX; i++)
    {
        if (ulIntStatus & (0x01<<i))
        {
            if (p_PWM0_Int_Handler[i] != NULL)
            {
                (void)p_PWM0_Int_Handler[i]((unsigned char)i);
            }
        }
    }
    do
    {
        addPWM0_Reg0x1 = ulIntStatus;
    } while (addPWM0_Reg0x1 & ulIntStatus & 0x7);   // delays
}


void pwm1_isr(void)
{
    int i;
    unsigned long ulIntStatus;

    ulIntStatus = addPWM1_Reg0x1;

    for (i=0; i<PWM_CHANNEL_NUMBER_MAX; i++)
    {
        if (ulIntStatus & (0x01<<i))
        {
            if (p_PWM1_Int_Handler[i] != NULL)
            {
                (void)p_PWM1_Int_Handler[i]((unsigned char)i);
            }
        }
    }
    do
    {
        addPWM1_Reg0x1 = ulIntStatus;
    } while (addPWM1_Reg0x1 & ulIntStatus & 0x7);   // delays
}

void pwm_change_duty_isr_handler(unsigned char ucChannel)
{
    
    
}


void pwm_timer_demo_handler(unsigned char ucChannel)
{   
    static  rwip_time_t cur_time;
    static rwip_time_t pre_time;
    static uint32_t test_cnt = 0;
    uint32_t diff_tmr;
    cur_time = rwip_time_get();
    diff_tmr = (cur_time.hs - pre_time.hs) * 3125 /10000;
    pre_time = cur_time;
    uart_printf("pwm0 test_cnt:%d,d_t:%d\r\n",test_cnt++,diff_tmr);


}
void pwm_mode_timer_demo(void) // t:100ms
{
    uart_printf("%s\r\n",__func__); 
    PWM_DRV_DESC pwm_drv_desc;
    pwm_drv_desc.channel = 2;
    pwm_drv_desc.clk_src = PWM_CLK_XTAL16M;
    pwm_drv_desc.contiu_mode = 0;
    pwm_drv_desc.cpedg_sel = 1;
    pwm_drv_desc.duty_cycle = 10000;
    pwm_drv_desc.end_value = 1600000;  
    pwm_drv_desc.int_en = 1;
    pwm_drv_desc.en = 1;
    pwm_drv_desc.mode = PWM_MODE_TIMER;
    pwm_drv_desc.pre_divid = 0;
    pwm_drv_desc.p_Int_Handler = pwm_timer_demo_handler;
    pwm0_init(&pwm_drv_desc);

}

void pwm_mode_pwm_demo(void) // t: period 100ms, 50% duty
{
    PWM_DRV_DESC pwm_drv_desc;
    pwm_drv_desc.channel = 0;
    pwm_drv_desc.clk_src = PWM_CLK_XTAL16M;
    pwm_drv_desc.contiu_mode = 0;
    pwm_drv_desc.cpedg_sel = 1;
    pwm_drv_desc.duty_cycle = 800000;
    pwm_drv_desc.end_value = 1600000;//1600000; 
    pwm_drv_desc.int_en = 0;
    pwm_drv_desc.en = 1;
    pwm_drv_desc.mode = PWM_MODE_PWM;
    pwm_drv_desc.pre_divid = 0;
    pwm_drv_desc.p_Int_Handler = NULL;
    pwm0_init(&pwm_drv_desc);
}

