/**
****************************************************************************************
*
* @file gpio.c
*
* @brief icu initialization and specific functions
*
* Copyright (C) Beken 2009-2016
*
* $Rev: $
*
****************************************************************************************
*/

#include <string.h>        // string manipulation
#include "gpio.h"
#include "icu.h"
#include "user_config.h"
#include "BK3633_RegList.h"
extern void app_gpio_sleep(void);

static GPIO_INT_CALLBACK_T gpio_int_cb = NULL; 

void gpio_config(uint8_t gpio, Dir_Type dir, Pull_Type pull)
{
    uint32_t  gpio_temp=0;
    uint8_t port = ((gpio&0xf0)>>4);
    uint8_t  pin = (gpio&0xf);
    
    switch(dir)
    {
        case OUTPUT:           
            gpio_temp &= ~(1<<GPIO_INPUT_EN);
            gpio_temp &= ~(1<<GPIO_OUTPUT_EN);
            break;        
        case INPUT:
            gpio_temp |= (1<<GPIO_OUTPUT_EN);
            gpio_temp |= (1<<GPIO_INPUT_EN);
            break;
        case FLOAT:        
            gpio_temp &= ~(1<<GPIO_INPUT_EN);
            gpio_temp |= (1<<GPIO_OUTPUT_EN);
            break;
        case SC_FUN:
            gpio_temp |= (1<<GPIO_OUTPUT_EN);
            gpio_temp |= (1<<GPIO_2FUN_EN);
            break;
    }

    switch(pull)
    {
    case PULL_HIGH:        
        gpio_temp |= (1<<GPIO_PULL_EN);
        gpio_temp |= (1<<GPIO_PULL_MODE);
        break;
    case PULL_LOW:
        gpio_temp |= (1<<GPIO_PULL_EN);
        gpio_temp &= ~(1<<GPIO_PULL_MODE);
        break;
    case PULL_NONE:
        gpio_temp &= ~(1<<GPIO_PULL_EN);
        break;
    }
    *((volatile unsigned long *) (BASEADDR_GPIO+4*(port*8+pin)))=gpio_temp;
    
}

uint8_t gpio_get_input(uint8_t gpio)
{
    uint32_t temp=0;
    uint8_t port = ((gpio&0xf0)>>4);
    uint8_t  pin = (gpio&0xf);

    temp = *((volatile unsigned long *) (BASEADDR_GPIO+4*(port*8+pin)));
    
    return (temp&(1<<GPIO_INPUT_VA));
}

void gpio_set(uint8_t gpio, uint8_t val)
{
    uint32_t temp=0;
    uint8_t port = ((gpio&0xf0)>>4);
    uint8_t  pin = (gpio&0xf);

    temp = *((volatile unsigned long *) (BASEADDR_GPIO+4*(port*8+pin)));
    if(val)
    {
        temp |= (1<<GPIO_OUTPUT_VA);
    }
    else
    {
        temp &= ~(1<<GPIO_OUTPUT_VA);
    }
    *((volatile unsigned long *) (BASEADDR_GPIO+4*(port*8+pin))) = temp;
    
}

void gpio_set_neg(uint8_t gpio)
{
    uint32_t temp=0;
    uint8_t port = ((gpio&0xf0)>>4);
    uint8_t  pin = (gpio&0xf);

    temp = *((volatile unsigned long *) (BASEADDR_GPIO+4*(port*8+pin)));
    temp ^= (1<<GPIO_OUTPUT_VA);
    *((volatile unsigned long *) (BASEADDR_GPIO+4*(port*8+pin))) = temp;

}

void gpio_set_int_mode(uint8_t gpio,uint8_t mode)
{
  //  uint32_t temp=0;
    uint8_t port = ((gpio&0xf0)>>4);
    uint8_t  pin = (gpio&0xf);
    if((port*8+pin)<16)
    {
        REG_GPIO_WUATOD_TYPE &= ~(3<<(2*(port*8+pin)));
        REG_GPIO_WUATOD_TYPE |= mode<<(2*(port*8+pin));
    }
    else
    {
        REG_GPIO_WUATOD_TYPE1 &= ~(3<<(2*((port*8+pin)-16)));
        REG_GPIO_WUATOD_TYPE1 |= mode<<(2*((port*8+pin)-16));
    }
    REG_GPIO_WUATOD_ENABLE |= 1<<(port*8+pin);
    REG_GPIO_WUATOD_STATUS |= 1<<(port*8+pin);
    addPMU_Reg0x3 |= 1<<(port*8+pin);
}


void gpio_init(void)
{
#if DEBUG_HW
#if(DEBUG_HW_DIGITAL)
    SYS_REG0X0C = (0x01 << 5) + (0x01 << 4) + (4);
#else
    SYS_REG0X0C |= (0x01 << 4);
    SYS_REG0X0C |= (0x01 << 5);
#endif    
    gpio_config(0x20,SC_FUN,PULL_NONE);
    gpio_config(0x21,SC_FUN,PULL_NONE);
    gpio_config(0x22,SC_FUN,PULL_NONE);
    gpio_config(0x23,SC_FUN,PULL_NONE);
    gpio_config(0x24,SC_FUN,PULL_NONE);
    gpio_config(0x25,SC_FUN,PULL_NONE);
    gpio_config(0x26,SC_FUN,PULL_NONE);
    gpio_config(0x27,SC_FUN,PULL_NONE);
#if(!DEBUG_HW_DIGITAL)    
    SYS_REG0X0C |= (0x01 << 3);
    SYS_REG0X0C &= ~(0x07 << 0);
    SYS_REG0X0C |= (0x07 << 0);
#endif
#endif

    gpio_config(0x04,OUTPUT,PULL_NONE);
    gpio_config(0x05,OUTPUT,PULL_NONE);
    gpio_config(0x06,OUTPUT,PULL_NONE);
    gpio_config(0x07,OUTPUT,PULL_NONE);


    REG_GPIO_WUATOD_ENABLE=0;
}


void gpio_triger(uint8_t gpio)
{
    gpio_set(gpio, 1);
    gpio_set(gpio, 0);
}


void gpio_cb_register(GPIO_INT_CALLBACK_T cb)
{
    if(cb)
    {
        gpio_int_cb = cb;
    }
}

void gpio_isr(void)
{

    //SYS_REG0X10_INT_EN &= ~(0x01 << POS_SYS_REG0X10_INT_EN_GPIO);
    if(gpio_int_cb)
    {
        (*gpio_int_cb)();
    }

}



void gpio_wakeup_config(uint8_t gpio)
{
    gpio_set_int_mode(gpio,FALL_EDGE);  
}

void gpio_wakeup(uint8_t gpio)
{
     uint8_t port = ((gpio&0xf0)>>4);
    uint8_t  pin = (gpio&0xf);

    REG_GPIO_WUATOD_ENABLE &= ~(1<<(port*8+pin));
    REG_GPIO_WUATOD_STATUS=0xffffffff;
}



void DEBUG_MSG(uint8_t x)
{
#if GPIO_DBG_MSG
    addAON_GPIO_Reg0xe=0x00;
    addAON_GPIO_Reg0x2=(x&0x01)<<1;
    addAON_GPIO_Reg0x3=(x&0x02);
    addAON_GPIO_Reg0x4=(x&0x04)>>1;
    addAON_GPIO_Reg0x5=(x&0x08)>>2;
    addAON_GPIO_Reg0x6=(x&0x10)>>3;
    addAON_GPIO_Reg0x7=(x&0x20)>>4;
    addAON_GPIO_Reg0xc=(x&0x40)>>5;
    addAON_GPIO_Reg0xd=(x&0x80)>>6;
    addAON_GPIO_Reg0xe=0x02;
#endif
}










