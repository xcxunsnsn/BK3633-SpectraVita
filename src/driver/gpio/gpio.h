/**
 ****************************************************************************************
 *
 * @file gpio.h
 *
 * @brief gpio Driver for gpio operation.
 *
 * Copyright (C) Beken 2009-2015
 *
 *
 ****************************************************************************************
 */

#ifndef _GPIO_H_
#define _GPIO_H_



#include <stdbool.h>          // standard boolean definitions
#include <stdint.h>           // standard integer functions

#define BASEADDR_GPIO                                           0x00800A00

#define REG_GPIO_10                     *((volatile unsigned long*)(BASEADDR_GPIO+8*4))
#define REG_GPIO_11                     *((volatile unsigned long*)(BASEADDR_GPIO+9*4))
#define REG_GPIO_12                     *((volatile unsigned long*)(BASEADDR_GPIO+10*4))
#define REG_GPIO_13                     *((volatile unsigned long*)(BASEADDR_GPIO+11*4))
#define REG_GPIO_14                     *((volatile unsigned long*)(BASEADDR_GPIO+12*4))
#define REG_GPIO_15                     *((volatile unsigned long*)(BASEADDR_GPIO+13*4))
#define REG_GPIO_WUATOD_TYPE            *((volatile unsigned long*)(BASEADDR_GPIO+0x30*4))
#define REG_GPIO_WUATOD_TYPE1           *((volatile unsigned long*)(BASEADDR_GPIO+0x31*4))
#define REG_GPIO_WUATOD_ENABLE          *((volatile unsigned long*)(BASEADDR_GPIO+0x33*4))
#define REG_GPIO_WUATOD_STATUS          *((volatile unsigned long*)(BASEADDR_GPIO+0x35*4))

#define GPIO_INPUT_VA   0
#define GPIO_OUTPUT_VA  1
#define GPIO_INPUT_EN   2
#define GPIO_OUTPUT_EN  3
#define GPIO_PULL_MODE  4
#define GPIO_PULL_EN    5
#define GPIO_2FUN_EN    6
#define GPIO_MONITOR_EN    7


 typedef enum
{
    INPUT,
    OUTPUT,
    FLOAT,
    SC_FUN,
}Dir_Type;

typedef enum
{
    PULL_LOW,
    PULL_HIGH,
    PULL_NONE
}Pull_Type;
 
typedef enum
{
    VOLTAGE_LOW,
    VOLTAGE_HIGHT,   
    RISE_EDGE,
    FALL_EDGE,
}int_mode;

typedef void (*GPIO_INT_CALLBACK_T)(void);   

void gpio_cb_register(GPIO_INT_CALLBACK_T cb);


void gpio_init(void);
void gpio_config(uint8_t gpio, Dir_Type dir, Pull_Type pull);
uint8_t gpio_get_input(uint8_t gpio);
void gpio_set(uint8_t gpio, uint8_t val);
void gpio_target(uint8_t gpio);
void gpio_isr(void);
void gpio_wakeup_config(uint8_t gpio);
void gpio_wakeup(uint8_t gpio); 
void gpio_triger(uint8_t gpio);
void DEBUG_MSG(uint8_t x);
void gpio_set_neg(uint8_t gpio);
void gpio_set_int_mode(uint8_t gpio,uint8_t mode);
void gpio_set_monitor(uint8_t gpio);

 
#endif 

