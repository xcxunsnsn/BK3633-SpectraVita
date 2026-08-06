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

#ifndef __GNUC__
#error "File only included with ARM GCC"
#endif // __GNUC__

#include <stdint.h>

extern uint32_t critical_sec_cnt;

/** @brief Enable interrupts globally in the system.
 * This macro must be used when the initialization phase is over and the interrupts
 * can start being handled by the system.
 */
#define GLOBAL_INT_START()                                                  \
do {                                                                        \
    uint32_t __l_cpsr_tmp;                                                  \
    __asm volatile("MRS %0, CPSR" : "=r"(__l_cpsr_tmp));                    \
    __asm volatile("BIC %0, %1, #0x80" : "=r"(__l_cpsr_tmp) :               \
                 "r"(__l_cpsr_tmp));                                        \
    __asm volatile("MSR CPSR_cxsf, %0" : : "r"(__l_cpsr_tmp));              \
} while(0)

/** @brief Disable interrupts globally in the system.
 * This macro must be used when the system wants to disable all the interrupt
 * it could handle.
 */
#define GLOBAL_INT_STOP()                                                   \
do {                                                                        \
    uint32_t __l_cpsr_tmp;                                                  \
    __asm volatile("MRS %0, CPSR" : "=r"(__l_cpsr_tmp));                    \
    __asm volatile("ORR %0, %1, #0x80" : "=r"(__l_cpsr_tmp) :               \
                 "r"(__l_cpsr_tmp));                                        \
    __asm volatile("MSR CPSR_cxsf, %0" : : "r"(__l_cpsr_tmp));              \
} while(0)

/** @brief Disable interrupts globally in the system.
 * This macro must be used in conjunction with the @ref GLOBAL_INT_RESTORE macro since this
 * last one will close the brace that the current macro opens.  This means that both
 * macros must be located at the same scope level.
 */
#define GLOBAL_INT_DISABLE()                                                \
do {                                                                        \
    uint32_t __l_cpsr_tmp;                                                  \
    uint32_t __l_irq_rest;                                                  \
    __asm volatile("MRS %0, CPSR" : "=r"(__l_cpsr_tmp));                    \
    __asm volatile("AND %0, %1, #0x80" : "=r"(__l_irq_rest) :               \
                 "r"(__l_cpsr_tmp));                                        \
    __asm volatile("ORR %0, %1, #0x80" : "=r"(__l_cpsr_tmp) :               \
                 "r"(__l_cpsr_tmp));                                        \
    __asm volatile("MSR CPSR_cxsf, %0" : : "r"(__l_cpsr_tmp));              \
    critical_sec_cnt++;                                                     \

/** @brief Restore interrupts from the previous global disable.
 * @sa GLOBAL_INT_DISABLE
 */
#define GLOBAL_INT_RESTORE()                                                \
    critical_sec_cnt--;                                                     \
    __asm volatile("MRS %0, CPSR" : "=r"(__l_cpsr_tmp));                    \
    __asm volatile("BIC %0, %1, #0x80" : "=r"(__l_cpsr_tmp) :               \
                 "r"(__l_cpsr_tmp));                                        \
    __asm volatile("ORR %0, %1, %2" : "=r"(__l_cpsr_tmp) :                  \
                 "r"(__l_cpsr_tmp), "r"(__l_irq_rest));                     \
    __asm volatile("MSR CPSR_cxsf, %0" : : "r"(__l_cpsr_tmp));              \
} while(0)

/** @brief Invoke the wait for interrupt procedure of the processor.
 *
 * @warning It is suggested that this macro is called while the interrupts are disabled
 * to have performed the checks necessary to decide to move to sleep mode.
 *
 */
#define WFI()                                                               \
do {                                                                        \
    uint32_t __l_rd;                                                        \
    if(critical_sec_cnt != 1)                                               \
        *((uint32_t*) 0x00000) = 0xDEAD;                                    \
    __asm volatile("MOV %0, #0" : "=r"(__l_rd));                            \
    __asm volatile("MCR p15, 0, %0, c7, c0, 4" : "=r"(__l_rd));             \
} while(0)


#endif // LL_H_
