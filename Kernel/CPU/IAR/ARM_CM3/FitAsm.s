/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

#include <SmallRTOSConfig.h>

	RSEG    CODE:CODE(2)
	thumb

	EXTERN gptCurrentTCB
	EXTERN OSTaskSwitchContext

	PUBLIC FitPendSVHandler
	PUBLIC FitSetInterruptMask
	PUBLIC FitClearInterruptMask        
	PUBLIC FitSVCHandler
	PUBLIC FitStartFirstTask



/*-----------------------------------------------------------*/

FitPendSVHandler:
	mrs r0, psp
	isb
	ldr	r3, =gptCurrentTCB			/* Get the location of the current TCB. */
	ldr	r2, [r3]

	stmdb r0!, {r4-r11}				/* Save the remaining registers. */
	str r0, [r2]					/* Save the new top of stack into the first member of the TCB. */

	stmdb sp!, {r3, r14}
	mov r0, #configMAX_SYSCALL_INTERRUPT_PRIORITY
	msr basepri, r0
	dsb
	isb	
	bl OSTaskSwitchContext
	mov r0, #0
	msr basepri, r0
	ldmia sp!, {r3, r14}

	ldr r1, [r3]
	ldr r0, [r1]					/* The first item in gptCurrentTCB is the task top of stack. */
	ldmia r0!, {r4-r11}				/* Pop the registers. */
	msr psp, r0
	isb
	bx r14

/*-----------------------------------------------------------*/

FitSetInterruptMask:
	mrs r0, basepri
	mov r1, #configMAX_SYSCALL_INTERRUPT_PRIORITY
	msr basepri, r1
	bx r14

/*-----------------------------------------------------------*/

FitClearInterruptMask:
	msr basepri, r0
	bx r14
        
/*-----------------------------------------------------------*/

FitSVCHandler:
	/* Get the location of the current TCB. */
	ldr	r3, =gptCurrentTCB
	ldr r1, [r3]
	ldr r0, [r1]
	/* Pop the core registers. */
	ldmia r0!, {r4-r11}
	msr psp, r0
	isb
	mov r0, #0
	msr	basepri, r0
	orr r14, r14, #13
	bx r14

/*-----------------------------------------------------------*/

FitStartFirstTask
	/* Use the NVIC offset register to locate the stack. */
	ldr r0, =0xE000ED08
	ldr r0, [r0]
	ldr r0, [r0]
	/* Set the msp back to the start of the stack. */
	msr msp, r0
	/* Call SVC to start the first task, ensuring interrupts are enabled. */
	cpsie i
	cpsie f
	dsb
	isb
	svc 0

	END
