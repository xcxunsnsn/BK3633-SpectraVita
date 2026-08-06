/**
 ****************************************************************************************
 *
 * @file intc.h
 *
 * @brief Declaration of the Interrupt Controller API.
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */

#ifndef _INTC_H_
#define _INTC_H_

/**
 ****************************************************************************************
 * @addtogroup INTC INTC
 * @ingroup DRIVERS
 *
 * @brief Declaration of the Interrupt Controller API.
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "compiler.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/** @name Mapping of the peripherals interrupts in the interrupt controller.
 * @{
 */
#define INTC_UNUSED31   (31)
#define INTC_UNUSED30   (30)
#define INTC_UNUSED29   (29)
#define INTC_UNUSED28   (28)
#define INTC_UNUSED27   (27)
#define INTC_UNUSED26   (26)
#define INTC_UNUSED25   (25)
#define INTC_UNUSED24   (24)
#define INTC_UNUSED23   (23)
#define INTC_UNUSED22   (22)
#define INTC_UNUSED21   (21)
#define INTC_UNUSED20   (20)
#define INTC_UNUSED19   (19)
#define INTC_UNUSED18   (18)
#define INTC_UNUSED17   (17)
#define INTC_UNUSED16   (16)
#define INTC_UNUSED15   (15)
#define INTC_UNUSED14   (14)
#define INTC_UNUSED13   (13)
#define INTC_UNUSED12   (12)
#define INTC_UNUSED11   (11)
#define INTC_UNUSED10   (10)
#define INTC_UNUSED9    (9)
#define INTC_UNUSED8    (8)
#define INTC_UNUSED7    (7)
#define INTC_UNUSED6    (6)
#define INTC_UNUSED5    (5)
#define INTC_UNUSED4    (4)
#define INTC_UNUSED3    (3)
#define INTC_UNUSED2    (2)
#define INTC_UNUSED1    (1)
#define INTC_UNUSED0    (0)

                //@@// 3633//rwip
#define INTC_TIMER      (2)//(10)
#define INTC_UART       (4)//(3)
#define INTC_UART_2     (5)//(11)
#define INTC_BLE        (20)//(4)
#define INTC_BT         (21)//(0)
#define INTC_BTDM       (22)//(5)
//#define INTC_COMMON     (6)

/// @} INTC mapping


///////////Beken platform used
//FIQ
#define INT_STATUS_USB_bit       (0x01<<17)
#define INT_STATUS_DMA_bit       (0x01<<18)
#define INT_BK24_bit             (0x01<<19)
#define INT_STATUS_RWBLE_bit     (0x01<<20)
#define INT_STATUS_RWBT_bit      (0x01<<21)
#define INT_STATUS_RWDM_bit      (0x01<<22)
//IRQ
#define INT_STATUS_PWM0_bit      (0x01<<0)
#define INT_STATUS_PWM1_bit      (0x01<<1)
#define INT_STATUS_TMR0_bit      (0x01<<2)
#define INT_STATUS_TMR1_bit      (0x01<<3)
#define INT_STATUS_UART0_bit     (0x01<<4)
#define INT_STATUS_UART2_bit     (0x01<<5)
#define INT_STATUS_SPI0_bit      (0x01<<6)
#define INT_STATUS_I2C0_bit      (0x01<<7)
#define INT_STATUS_ADC_bit       (0x01<< 8)
#define INT_STATUS_AON_GPIO_bit  (0x01<< 9)
#define INT_STATUS_RTC_bit       (0x01<< 10)
#define INT_STATUS_I2S_bit       (0x01<< 11)
#define INT_STATUS_AON_RTC_bit   (0x01<< 12)

#define FAST_IRQ_ENTRY  __attribute__((section("sys_irq_entry")))
#define FAST_FIQ_ENTRY  __attribute__((section("sys_fiq_entry")))
#define FAST_SOFTIRQ_ENTRY  __attribute__((section("sys_softirq_entry")))

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initialize and configure the reference interrupt controller.
 *
 * This function configures the interrupt controller according to the system needs.
 *
 ****************************************************************************************
 */
void intc_init(void);

/**
 ****************************************************************************************
 * @brief Clear status of interrupts.
 *
 * This function clear interrupt status.
 *
 ****************************************************************************************
 */
void intc_stat_clear(void);

/**
 ****************************************************************************************
 * @brief IRQ Handler.
 *
 ****************************************************************************************
 */
/*__IRQ */void intc_irq(void);

/**
 ****************************************************************************************
 * @brief FIQ Handler.
 *
 ****************************************************************************************
 */
/*__FIQ*/ void intc_fiq(void);

/// @} INTC

#endif // _INTC_H_
