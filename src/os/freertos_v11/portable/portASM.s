;/*
; * FreeRTOS Kernel V11.1.0
; * Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
; *
; * SPDX-License-Identifier: MIT
; *
; * Permission is hereby granted, free of charge, to any person obtaining a copy of
; * this software and associated documentation files (the "Software"), to deal in
; * the Software without restriction, including without limitation the rights to
; * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
; * the Software, and to permit persons to whom the Software is furnished to do so,
; * subject to the following conditions:
; *
; * The above copyright notice and this permission notice shall be included in all
; * copies or substantial portions of the Software.
; *
; * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
; * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
; * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
; * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
; * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
; *
; * https://www.FreeRTOS.org
; * https://github.com/FreeRTOS
; *
; */

    INCLUDE portmacro.inc

    IMPORT  vTaskSwitchContext
    IMPORT  xTaskIncrementTick
	IMPORT  uIrqFiqExitFlag
	IMPORT  IRQ_Exception
	IMPORT	FIQ_Exception

    EXPORT  vPortStartFirstTask
    EXPORT  vPreemptiveTick
    EXPORT  vPortYield
	EXPORT	isIRQorFIQContext
    EXPORT  vPortYieldProcessor
	EXPORT  SYSirq_IRQ_Handler		[WEAK]
	EXPORT	SYSirq_FIQ_Handler		[WEAK]
	EXPORT	SoftwareInterrupt_Exception		[WEAK]


    ARM
    AREA    PORT_ASM, CODE, READONLY



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Starting the first task is done by just restoring the context
; setup by pxPortInitialiseStack
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
vPortStartFirstTask

    PRESERVE8

    portRESTORE_CONTEXT

vPortYield

    PRESERVE8

    SVC 0
    bx lr



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Interrupt service routine for preemptive scheduler tick timer
; Only used if portUSE_PREEMPTION is set to 1 in portmacro.h
;
; Uses timer 0 of LPC21XX Family
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

vPreemptiveTick

    PRESERVE8
	LDR     R0, =uIrqFiqExitFlag
    LDR     R0, [R0]
    AND		R0, R0, #0x3
	CMP		R0, #0
	BNE		vProcessTick
	POP		{R0, R1, R2, R3, R12, LR}
	SUBS	PC, LR, #4
vProcessTick
	POP		{R0, R1, R2, R3, R12, LR}
    portSAVE_CONTEXT                    ; Save the context of the current task.
	
	LDR     R4, =uIrqFiqExitFlag
    LDR     R4, [R4]
	AND		R4, R4, #0x2

    LDR R0, =xTaskIncrementTick         ; Increment the tick count.
    MOV LR, PC                          ; This may make a delayed task ready
    BX R0                               ; to run.

	ORR	R0, R0, R4
    CMP R0, #0
    BEQ SkipContextSwitch
    LDR R0, =vTaskSwitchContext         ; Find the highest priority task that
    MOV LR, PC                          ; is ready to run.
    BX R0
SkipContextSwitch
	LDR R0, =uIrqFiqExitFlag
	MOV R1, #0
	STR	R1, [R0]
	portRESTORE_CONTEXT
	

vFiqExitSwitchContext
	PRESERVE8
		
	MRS     R0, SPSR
	AND		R0, R0, #0x1f
	CMP		R0, #0x12
	BEQ		vExitFiq
		
	LDR     R0, =uIrqFiqExitFlag
    LDR     R0, [R0]
    AND		R0, R0, #0x2
	CMP		R0, #0
	BEQ		vExitFiq
	POP		{R0, R1, R2, R3, R12, LR}
	portSAVE_CONTEXT
	
	LDR R0, =vTaskSwitchContext
    MOV LR, PC
    BX R0
	
	LDR     R0, =uIrqFiqExitFlag
    LDR     R1, [R0]
    AND		R1, R1, #0x1
	STR		R1, [R0]
	portRESTORE_CONTEXT
vExitFiq
	POP		{R0, R1, R2, R3, R12, LR}
	SUBS	PC, LR, #4	
																	
isIRQorFIQContext

	PRESERVE8
		
	PUSH {R1}
	MRS R1, CPSR
	AND R1, R1, #0x1f
	MOV R0, #0
	CMP R1, #0x11
	ADDEQ R0, #1
	CMP R1, #0x12
	ADDEQ R0, #2
	POP	{R1}
	BX	LR
	
	
    AREA    sys_irq_entry, CODE, READONLY
	
SYSirq_IRQ_Handler
	PUSH	{R0, R1, R2, R3, R12, LR}
	BLX		IRQ_Exception
	B		vPreemptiveTick


	AREA    sys_fiq_entry, CODE, READONLY
	
SYSirq_FIQ_Handler
	PUSH	{R0, R1, R2, R3, R12, LR}
	BLX		FIQ_Exception
	B 		vFiqExitSwitchContext
		
	AREA    sys_softirq_entry, CODE, READONLY

SoftwareInterrupt_Exception
vPortYieldProcessor

    PRESERVE8
    ; Within an IRQ ISR the link register has an offset from the true return
    ; address, but an SWI ISR does not.  Add the offset manually so the same
    ; ISR return code can be used in both cases.
    ADD LR, LR, #4

    ; Perform the context switch.
    portSAVE_CONTEXT                    ; Save current task context
    LDR R0, =vTaskSwitchContext         ; Get the address of the context switch function
    MOV LR, PC                          ; Store the return address
    BX  R0                              ; Call the contedxt switch function
    portRESTORE_CONTEXT                 ; restore the context of the selected task
	
	END

	