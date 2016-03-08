/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

/* Compiler includes. */
#include <stdint.h>

#include "SmallRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Constants required to manipulate the NVIC. */
#define FitNVIC_SYSTICK_CTRL		( ( volatile uOS32_t *) 0xe000e010 )
#define FitNVIC_SYSTICK_LOAD		( ( volatile uOS32_t *) 0xe000e014 )
#define FitNVIC_INT_CTRL			( ( volatile uOS32_t *) 0xe000ed04 )
#define FitNVIC_SYSPRI2				( ( volatile uOS32_t *) 0xe000ed20 )
#define FitNVIC_SYSTICK_CLK			0x00000004
#define FitNVIC_SYSTICK_INT			0x00000002
#define FitNVIC_SYSTICK_ENABLE		0x00000001
#define FitNVIC_PENDSVSET			0x10000000
#define FitMIN_INTERRUPT_PRIORITY	( 255UL )
#define FitNVIC_PENDSV_PRI			( FitMIN_INTERRUPT_PRIORITY << 16UL )
#define FitNVIC_SYSTICK_PRI			( FitMIN_INTERRUPT_PRIORITY << 24UL )

/* Constants required to set up the initial stack. */
#define FitINITIAL_XPSR				( 0x01000000 )

/* Constants used with memory barrier intrinsics. */
#define FitSY_FULL_READ_WRITE		( 15 )

/* Each task maintains its own interrupt status in the critical nesting
variable. */
static volatile uOSBase_t uxCriticalNesting = 0xaaaaaaaa;

/*
 * Setup the timer to generate the tick interrupts.
 */
static void FitSetupTimerInterrupt( void );

/*
 * Exception handlers.
 */
void FitPendSVHandler( void );
void FitOSTickISR( void );
void FitSVCHandler( void );

/*
 * Start first task is a separate function so it can be tested in isolation.
 */
static void FitStartFirstTask( void );

/*
 * Used to catch tasks that attempt to return from their implementing function.
 */
static void FitTaskExitError( void );

/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
uOSStack_t *FitInitializeStack( uOSStack_t *pxTopOfStack, OSTaskFunction_t TaskFunction, void *pvParameters )
{
	/* Simulate the stack frame as it would be created by a context switch
	interrupt. */
	pxTopOfStack--; /* Offset added to account for the way the MCU uses the stack on entry/exit of interrupts. */
	*pxTopOfStack = FitINITIAL_XPSR;	/* xPSR */
	pxTopOfStack--;
	*pxTopOfStack = ( uOSStack_t ) TaskFunction;	/* PC */
	pxTopOfStack--;
	*pxTopOfStack = ( uOSStack_t ) FitTaskExitError;	/* LR */
	pxTopOfStack -= 5;	/* R12, R3, R2 and R1. */
	*pxTopOfStack = ( uOSStack_t ) pvParameters;	/* R0 */
	pxTopOfStack -= 8; /* R11..R4. */

	return pxTopOfStack;
}
/*-----------------------------------------------------------*/

static void FitTaskExitError( void )
{
	/* A function that implements a task must not exit or attempt to return to
	its caller as there is nothing to return to.  If a task wants to exit it
	should instead call OSTaskDelete( NULL ).*/

	FitDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/

void FitSVCHandler( void )
{
	/* This function is no longer used, but retained for backward
	compatibility. */
}
/*-----------------------------------------------------------*/

__asm void FitStartFirstTask( void )
{
	extern gptCurrentTCB;

	PRESERVE8

	/* The MSP stack is not reset as, unlike on M3/4 parts, there is no vector
	table offset register that can be used to locate the initial stack value.
	Not all M0 parts have the application vector table at address 0. */

	ldr	r3, =gptCurrentTCB	/* Obtain location of gptCurrentTCB. */
	ldr r1, [r3]
	ldr r0, [r1]			/* The first item in gptCurrentTCB is the task top of stack. */
	adds r0, #32			/* Discard everything up to r0. */
	msr psp, r0				/* This is now the new top of stack to use in the task. */
	movs r0, #2				/* Switch to the psp stack. */
	msr CONTROL, r0
	isb
	pop {r0-r5}				/* Pop the registers that are saved automatically. */
	mov lr, r5				/* lr is now in r5. */
	cpsie i					/* The first task has its context and interrupts can be enabled. */
	pop {pc}				/* Finally, pop the PC to jump to the user defined task code. */

	ALIGN
}
/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
sOSBase_t FitStartScheduler( void )
{
	/* Make PendSV, CallSV and SysTick the same priroity as the kernel. */
	*(FitNVIC_SYSPRI2) |= FitNVIC_PENDSV_PRI;
	*(FitNVIC_SYSPRI2) |= FitNVIC_SYSTICK_PRI;

	/* Start the timer that generates the tick ISR.  Interrupts are disabled
	here already. */
	FitSetupTimerInterrupt();

	/* Initialise the critical nesting count ready for the first task. */
	uxCriticalNesting = 0;

	/* Start the first task. */
	FitStartFirstTask();

	/* Should not get here! */
	return 0;
}
/*-----------------------------------------------------------*/

void FitEndScheduler( void )
{

}
/*-----------------------------------------------------------*/

void FitYield( void )
{
	/* Set a PendSV to request a context switch. */
	*( FitNVIC_INT_CTRL ) = FitNVIC_PENDSVSET;

	/* Barriers are normally not required but do ensure the code is completely
	within the specified behaviour for the architecture. */
	__dsb( FitSY_FULL_READ_WRITE );
	__isb( FitSY_FULL_READ_WRITE );
}
/*-----------------------------------------------------------*/

void FitEnterCritical( void )
{
    FitDISABLE_INTERRUPTS();
    uxCriticalNesting++;
	__dsb( FitSY_FULL_READ_WRITE );
	__isb( FitSY_FULL_READ_WRITE );
}
/*-----------------------------------------------------------*/

void FitExitCritical( void )
{
    uxCriticalNesting--;
    if( uxCriticalNesting == 0 )
    {
        FitENABLE_INTERRUPTS();
    }
}
/*-----------------------------------------------------------*/

__asm uOS32_t ulSetInterruptMaskFromISR( void )
{
	mrs r0, PRIMASK
	cpsid i
	bx lr
}
/*-----------------------------------------------------------*/

__asm void vClearInterruptMaskFromISR( uOS32_t ulMask )
{
	msr PRIMASK, r0
	bx lr
}
/*-----------------------------------------------------------*/

__asm void FitPendSVHandler( void )
{
	extern OSTaskSwitchContext
	extern gptCurrentTCB

	PRESERVE8

	mrs r0, psp

	ldr	r3, =gptCurrentTCB 	/* Get the location of the current TCB. */
	ldr	r2, [r3]

	subs r0, #32			/* Make space for the remaining low registers. */
	str r0, [r2]			/* Save the new top of stack. */
	stmia r0!, {r4-r7}		/* Store the low registers that are not saved automatically. */
	mov r4, r8				/* Store the high registers. */
	mov r5, r9
	mov r6, r10
	mov r7, r11
	stmia r0!, {r4-r7}

	push {r3, r14}
	cpsid i
	bl OSTaskSwitchContext
	cpsie i
	pop {r2, r3}			/* lr goes in r3. r2 now holds tcb pointer. */

	ldr r1, [r2]
	ldr r0, [r1]			/* The first item in gptCurrentTCB is the task top of stack. */
	adds r0, #16			/* Move to the high registers. */
	ldmia r0!, {r4-r7}		/* Pop the high registers. */
	mov r8, r4
	mov r9, r5
	mov r10, r6
	mov r11, r7

	msr psp, r0				/* Remember the new top of stack for the task. */

	subs r0, #32			/* Go back for the low registers that are not automatically restored. */
	ldmia r0!, {r4-r7}      /* Pop low registers.  */

	bx r3
	ALIGN
}
/*-----------------------------------------------------------*/

void FitOSTickISR( void )
{
uOS32_t ulPreviousMask;

	ulPreviousMask = FitSET_INTERRUPT_MASK_FROM_ISR();
	{
		/* Increment the RTOS tick. */
		if( OSTaskIncrementTick() != FALSE )
		{
			/* Pend a context switch. */
			*(FitNVIC_INT_CTRL) = FitNVIC_PENDSVSET;
		}
	}
	FitCLEAR_INTERRUPT_MASK_FROM_ISR( ulPreviousMask );
}
/*-----------------------------------------------------------*/

/*
 * Setup the systick timer to generate the tick interrupts at the required
 * frequency.
 */
void FitSetupTimerInterrupt( void )
{
	/* Configure SysTick to interrupt at the requested rate. */
	*(FitNVIC_SYSTICK_LOAD) = ( configCPU_CLOCK_HZ / OSTICK_RATE_HZ ) - 1UL;
	*(FitNVIC_SYSTICK_CTRL) = FitNVIC_SYSTICK_CLK | FitNVIC_SYSTICK_INT | FitNVIC_SYSTICK_ENABLE;
}
/*-----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif
