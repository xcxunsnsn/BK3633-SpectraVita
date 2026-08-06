/**
 ****************************************************************************************
 *
 * @file ll.h
 *
 * @brief Declaration of low level functions.
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */

#ifndef LL_H_
#define LL_H_

#if 1//def CFG_CPU_BK3633

#include <stdint.h>

#include "compiler.h"

#define KEIL_INTRINSICS_INTERRUPT

#if 0
#include "uart2.h"
/** @brief Enable interrupts globally in the system.
 * This macro must be used when the initialization phase is over and the interrupts
 * can start being handled by the system.
 */

/* keil Compiler intrinsics for controlling IRQ and FIQ interrupts
*/

extern void __enable_irq_(uint32_t irq);
extern void __enable_fiq_(uint32_t fiq);
extern uint32_t  __disable_irq_(void);
extern uint32_t  __disable_fiq_(void);



/** @brief Disable interrupts globally in the system.
 * This macro must be used when the system wants to disable all the interrupt
 * it could handle.
 */

#define GLOBAL_INT_STOP();    \
do { \
    __disable_fiq_(); \
    __disable_irq_(); \
} while(0);

/* * @brief Disable interrupts globally in the system.
 * This macro must be used in conjunction with the @ref GLOBAL_INT_RESTORE macro since this
 * last one will close the brace that the current macro opens.  This means that both
 * macros must be located at the same scope level.
 */
#define GLOBAL_INT_DISABLE(); \
do { \
    uint32_t  fiq_tmp; \
    uint32_t  irq_tmp; \
    fiq_tmp = __disable_fiq_(); \
    irq_tmp = __disable_irq_();
#define GLOBAL_INT_RESTORE(); \
    if(fiq_tmp)           \
    {                      \
        __enable_fiq_(fiq_tmp);    \
    }                      \
    if(irq_tmp)           \
    {                      \
        __enable_irq_(irq_tmp);    \
    }                      \
} while(0) ;

#else
/** @brief Enable interrupts globally in the system.
 * This macro must be used when the initialization phase is over and the interrupts
 * can start being handled by the system.
 */

/* keil Compiler intrinsics for controlling IRQ and FIQ interrupts
*/

extern void __enable_irq(void);
extern void __enable_fiq(void);
extern int  __disable_irq(void);
extern int  __disable_fiq(void);


#define GLOBAL_INT_START(); \
do { \
    __enable_fiq(); \
    __enable_irq(); \
} while(0);

/** @brief Disable interrupts globally in the system.
 * This macro must be used when the system wants to disable all the interrupt
 * it could handle.
 */

#define GLOBAL_INT_STOP();        \
do { \
    __disable_fiq(); \
    __disable_irq(); \
} while(0);

/* * @brief Disable interrupts globally in the system.
 * This macro must be used in conjunction with the @ref GLOBAL_INT_RESTORE macro since this
 * last one will close the brace that the current macro opens.  This means that both
 * macros must be located at the same scope level.
 */
#define GLOBAL_INT_DISABLE(); \
do { \
    uint32_t  fiq_tmp; \
    uint32_t  irq_tmp; \
    fiq_tmp = __disable_fiq(); \
    irq_tmp = __disable_irq();
#define GLOBAL_INT_RESTORE(); \
    if(!fiq_tmp)           \
    {                      \
        __enable_fiq();    \
    }                      \
    if(!irq_tmp)           \
    {                      \
        __enable_irq();    \
    }                      \
} while(0) ;

#endif

/** @brief Invoke the wait for interrupt procedure of the processor.
 *
 * @warning It is suggested that this macro is called while the interrupts are disabled
 * to have performed the checks necessary to decide to move to sleep mode.
 *
 */
__INLINE void WFI(void)
{
    uint32_t __l_rd;
#pragma arm
    __asm
    {
        MOV __l_rd, 0;
        //MCR p15, 0, __l_rd, c7, c0, 4;
    }
}

#else
#ifndef __arm__
#error "File only included with RVDS!"
#endif // __arm__

#include <stdint.h>
#include "arch.h"
#include "reg_intc.h"

/** @brief Enable interrupts globally in the system.
 * This macro must be used when the initialization phase is over and the interrupts
 * can start being handled by the system.
 */
#ifndef CFG_CPU_CORTEX_M1
#define GLOBAL_INT_START()                                                  \
do {                                                                        \
    uint32_t __l_cpsr_tmp;                                                  \
    __asm("                                                                 \
        MRS     __l_cpsr_tmp, CPSR;                                         \
        BIC     __l_cpsr_tmp, __l_cpsr_tmp, #0x80;                          \
        MSR     CPSR_cxsf, __l_cpsr_tmp;                                    \
    ");                                                                     \
} while(0)
#else
#define GLOBAL_INT_START()
#endif

/** @brief Disable interrupts globally in the system.
 * This macro must be used when the system wants to disable all the interrupt
 * it could handle.
 */
#ifndef CFG_CPU_CORTEX_M1
#define GLOBAL_INT_STOP()                                                   \
do {                                                                        \
    uint32_t __l_cpsr_tmp;                                                  \
    __asm("                                                                 \
        MRS     __l_cpsr_tmp, CPSR;                                         \
        ORR     __l_cpsr_tmp, __l_cpsr_tmp, #0x80;                          \
        MSR     CPSR_cxsf, __l_cpsr_tmp;                                    \
    ");                                                                     \
} while(0)
#else
#define GLOBAL_INT_STOP()
#endif


/** @brief Disable interrupts globally in the system.
 * This macro must be used in conjunction with the @ref GLOBAL_INT_RESTORE macro since this
 * last one will close the brace that the current macro opens.  This means that both
 * macros must be located at the same scope level.
 */

#ifndef CFG_CPU_CORTEX_M1
 #define GLOBAL_INT_DISABLE()                                                \
do {                                                                        \
    uint32_t __l_cpsr_tmp;                                                  \
    uint32_t __l_irq_rest;                                                  \
    __asm("                                                                 \
        MRS     __l_cpsr_tmp, CPSR;                                         \
        AND     __l_irq_rest, __l_cpsr_tmp, #0x80;                          \
        ORR     __l_cpsr_tmp, __l_cpsr_tmp, #0x80;                          \
        MSR     CPSR_cxsf, __l_cpsr_tmp;                                    \
    ")
#else
#define GLOBAL_INT_DISABLE()
#endif



/** @brief Restore interrupts from the previous global disable.
 * @sa GLOBAL_INT_DISABLE
 */

#ifndef CFG_CPU_CORTEX_M1
#define GLOBAL_INT_RESTORE()                                                \
    __asm("                                                                 \
        MRS     __l_cpsr_tmp, CPSR;                                         \
        BIC     __l_cpsr_tmp, __l_cpsr_tmp, #0x80;                          \
        ORR     __l_cpsr_tmp, __l_cpsr_tmp, __l_irq_rest;                   \
        MSR     CPSR_cxsf, __l_cpsr_tmp;                                    \
    ");                                                                     \
} while(0)
#else
#define GLOBAL_INT_RESTORE()
#endif

/** @brief Invoke the wait for interrupt procedure of the processor.
 *
 * @warning It is suggested that this macro is called while the interrupts are disabled
 * to have performed the checks necessary to decide to move to sleep mode.
 *
 */
#define WFI()                                                                   \
    do {                                                                                     \
        GLOBAL_INT_DISABLE();                                                                \
        while(intc_irqstatus_get() == 0);                                                    \
        GLOBAL_INT_RESTORE();                                                                \
    } while (0)

#endif

#endif // LL_H_
