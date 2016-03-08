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

#define FitKERNEL_INTERRUPT_PRIORITY 255

/* Constants required to manipulate the core.  Registers first... */
#define FitNVIC_SYSTICK_CTRL_REG			( * ( ( volatile uOS32_t * ) 0xe000e010 ) )
#define FitNVIC_SYSTICK_LOAD_REG			( * ( ( volatile uOS32_t * ) 0xe000e014 ) )
#define FitNVIC_SYSTICK_CURRENT_VALUE_REG	( * ( ( volatile uOS32_t * ) 0xe000e018 ) )
#define FitNVIC_SYSPRI2_REG					( * ( ( volatile uOS32_t * ) 0xe000ed20 ) )
/* ...then bits in the registers. */
#define FitNVIC_SYSTICK_CLK_BIT	        	( 1UL << 2UL )
#define FitNVIC_SYSTICK_INT_BIT				( 1UL << 1UL )
#define FitNVIC_SYSTICK_ENABLE_BIT			( 1UL << 0UL )
#define FitNVIC_SYSTICK_COUNT_FLAG_BIT		( 1UL << 16UL )
#define FitNVIC_PENDSVCLEAR_BIT 			( 1UL << 27UL )
#define FitNVIC_PEND_SYSTICK_CLEAR_BIT		( 1UL << 25UL )

/* Masks off all bits but the VECTACTIVE bits in the ICOS register. */
#define FitVECTACTIVE_MASK					( 0x1FUL )

#define FitNVIC_PENDSV_PRI					( ( ( uOS32_t ) FitKERNEL_INTERRUPT_PRIORITY ) << 16UL )
#define FitNVIC_SYSTICK_PRI					( ( ( uOS32_t ) FitKERNEL_INTERRUPT_PRIORITY ) << 24UL )

/* Constants required to set up the initial stack. */
#define FitINITIAL_XPSR						( 0x01000000 )
#define FitINITIAL_EXEC_RETURN				( 0xfffffffd )

/* Scheduler utilities. */
#define FitNVIC_INT_CTRL_REG				( * ( ( volatile uOS32_t * ) 0xe000ed04 ) )
#define FitNVIC_PENDSVSET_BIT				( 1UL << 28UL )

/* Constants required to handle critical sections. */
#define FitNO_CRITICAL_NESTING		( ( unsigned long ) 0 )
static volatile uOSBase_t uxCriticalNesting = 9999UL;

/*
 * The scheduler can only be started from ARM mode, hence the inclusion of
 * this function here.
 */
void FitStartFirstTask( void );
void FitSetupTimerInterrupt( void );
void FitPendSVHandler( void );
void FitOSTickISR( void );
void FitSVCHandler( void );
static void FitTaskExitError( void );



uOSStack_t *FitInitializeStack( uOSStack_t *pxTopOfStack,
		OSTaskFunction_t TaskFunction, void *pvParameters )
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
	pxTopOfStack -= 8;	/* R11, R10, R9, R8, R7, R6, R5 and R4. */

	return pxTopOfStack;	
}

static void FitTaskExitError( void )
{
	/* A function that implements a task must not exit or attempt to return to
	its caller as there is nothing to return to.  If a task wants to exit it
	should instead call OSTaskDelete( NULL ). */
	
	FitDISABLE_INTERRUPTS();
	for( ;; );
}

__asm void FitSVCHandler( void )
{
	PRESERVE8

	ldr	r3, =gptCurrentTCB	/* Restore the context. */
	ldr r1, [r3]			/* Use gptCurrentTCB to get the gptCurrentTCB address. */
	ldr r0, [r1]			/* The first item in gptCurrentTCB is the task top of stack. */
	ldmia r0!, {r4-r11}		/* Pop the registers that are not automatically saved on exception entry and the critical nesting count. */
	msr psp, r0				/* Restore the task stack pointer. */
	isb
	mov r0, #0
	msr	basepri, r0
	orr r14, #0xd
	bx r14	
}

__asm void FitStartFirstTask( void )
{	
	PRESERVE8

	/* Use the NVIC offset register to locate the stack. */
	ldr r0, =0xE000ED08
	ldr r0, [r0]
	ldr r0, [r0]
	/* Set the msp back to the start of the stack. */
	msr msp, r0
	/* Globally enable interrupts. */
	cpsie i
	cpsie f
	dsb
	isb
	/* Call SVC to start the first task. */
	svc 0
	nop
	nop	
}

uOSBase_t FitStartScheduler( void )
{
	/* Make PendSV and SysTick the lowest priority interrupts. */
	FitNVIC_SYSPRI2_REG |= FitNVIC_PENDSV_PRI;
	FitNVIC_SYSPRI2_REG |= FitNVIC_SYSTICK_PRI;

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

void FitEndScheduler( void )
{

}

/*
 * Called by OSTaskYield() to manually force a context switch.
 *
 * When a context switch is performed from the task level the saved task
 * context is made to look as if it occurred from within the tick ISR.  This
 * way the same restore context function can be used when restoring the context
 * saved from the ISR or that saved from a call to FitYield.
 */
void FitYield( void )															
{																				
	/* Set a PendSV to request a context switch. */								
	FitNVIC_INT_CTRL_REG = FitNVIC_PENDSVSET_BIT;								

}


void FitEnterCritical( void )
{
	FitDISABLE_INTERRUPTS();
	uxCriticalNesting++;
}

void FitExitCritical( void )
{
	uxCriticalNesting--;
	if( uxCriticalNesting == 0 )
	{
		FitENABLE_INTERRUPTS();
	}	
}


__asm void FitPendSVHandler( void )
{
	extern uxCriticalNesting;
	extern gptCurrentTCB;
	extern OSTaskSwitchContext;

	PRESERVE8

	mrs r0, psp
	isb

	ldr	r3, =gptCurrentTCB		/* Get the location of the current TCB. */
	ldr	r2, [r3]

	stmdb r0!, {r4-r11}			/* Save the remaining registers. */
	str r0, [r2]				/* Save the new top of stack into the first member of the TCB. */

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
	ldr r0, [r1]				/* The first item in pxCurrentTCB is the task top of stack. */
	ldmia r0!, {r4-r11}			/* Pop the registers and the critical nesting count. */
	msr psp, r0
	isb
	bx r14
	nop			
}


void FitOSTickISR()
{
	// Increment the RTOS tick count, then look for the highest priority
	// task that is ready to run. 
	( void ) FitSET_INTERRUPT_MASK_FROM_ISR();
	{
		/* Increment the RTOS tick. */
		if( OSTaskIncrementTick() != FALSE )
		{
			/* A context switch is required.  Context switching is performed in
			the PendSV interrupt.  Pend the PendSV interrupt. */
			FitNVIC_INT_CTRL_REG = FitNVIC_PENDSVSET_BIT;
		}
	}
	FitCLEAR_INTERRUPT_MASK_FROM_ISR( 0 );
}

void FitSetupTimerInterrupt( void )
{
	/* Configure SysTick to interrupt at the requested rate. */
	FitNVIC_SYSTICK_LOAD_REG = ( configCPU_CLOCK_HZ / OSTICK_RATE_HZ ) - 1UL;
	FitNVIC_SYSTICK_CTRL_REG = ( FitNVIC_SYSTICK_CLK_BIT | FitNVIC_SYSTICK_INT_BIT | FitNVIC_SYSTICK_ENABLE_BIT );
}

__asm uOS32_t FitSetInterruptMask( void )
{
	PRESERVE8

	mrs r0, basepri
	mov r1, #configMAX_SYSCALL_INTERRUPT_PRIORITY
	msr basepri, r1
	bx r14
}
/*-----------------------------------------------------------*/

__asm void FitClearInterruptMask( uOS32_t ulNewMask )
{
	PRESERVE8

	msr basepri, r0
	bx r14
}
#ifdef __cplusplus
}
#endif
