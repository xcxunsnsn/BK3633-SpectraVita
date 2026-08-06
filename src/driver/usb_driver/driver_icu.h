/*************************************************************
 * @file        driver_icu.h
 * @brief       Header file of driver_icu.c
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par         
 * @attention   
 *
 * @history     2016-09-29 gwf    create this file
 */

#ifndef __DRIVER_ICU_H__

#define __DRIVER_ICU_H__


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */





#define REG_ICU_BASE_ADDR                   (0x00800000UL)

#define REG_ICU_I2S_SDM_USB_ADDR            (REG_ICU_BASE_ADDR + 0x08 * 4)
#define REG_ICU_I2S_SDM_USB_MASK            0x07UL
#define REG_ICU_I2S_SDM_USB                 (*((volatile unsigned long *) REG_ICU_I2S_SDM_USB_ADDR))

#define ICU_USB_PWD_CLEAR()                 do {REG_ICU_I2S_SDM_USB &= ~0x01;} while (0)
#define ICU_USB_PWD_SET()                   do {REG_ICU_I2S_SDM_USB |=  0x01;} while (0)

#define REG_ICU_INT_ENABLE_ADDR             (REG_ICU_BASE_ADDR + 0x10 * 4)
//#define REG_ICU_INT_ENABLE_MASK             (ICU_INT_ENABLE_IRQ_MASK | ICU_INT_ENABLE_FIQ_MASK)
#define REG_ICU_INT_ENABLE                  (*((volatile unsigned long *) REG_ICU_INT_ENABLE_ADDR))

#define ICU_INT_ENABLE_IRQ_USB_POSI         17
#define ICU_INT_ENABLE_IRQ_USB_MASK         (0x01UL << ICU_INT_ENABLE_IRQ_USB_POSI)



#define REG_ICU_INT_GLOBAL_ENABLE_ADDR      (REG_ICU_BASE_ADDR + 0x11 * 4)
#define REG_ICU_INT_GLOBAL_ENABLE           (*((volatile unsigned long *) REG_ICU_INT_GLOBAL_ENABLE_ADDR))

#define ICU_INT_GLOBAL_ENABLE_IRQ_POSI      0
#define ICU_INT_GLOBAL_ENABLE_IRQ_MASK      (0x01UL << ICU_INT_GLOBAL_ENABLE_IRQ_POSI)
#define ICU_INT_GLOBAL_ENABLE_IRQ_SET       (0x01UL << ICU_INT_GLOBAL_ENABLE_IRQ_POSI)

#define ICU_INT_GLOBAL_ENABLE_FIQ_POSI      1
#define ICU_INT_GLOBAL_ENABLE_FIQ_MASK      (0x01UL << ICU_INT_GLOBAL_ENABLE_FIQ_POSI)
#define ICU_INT_GLOBAL_ENABLE_FIQ_SET       (0x01UL << ICU_INT_GLOBAL_ENABLE_FIQ_POSI)

#define ICU_INT_ENABLE_SET(x)               REG_ICU_INT_ENABLE |=  (x);
#define ICU_INT_ENABLE_CLEAR(x)             REG_ICU_INT_ENABLE &= (~(x)); 








#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif      /* __DRIVER_ICU_H__ */
