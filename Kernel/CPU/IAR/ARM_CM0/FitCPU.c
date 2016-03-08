/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial supFit, development, Fiting, licensing and training services.
***********************************************************************************************************/

/* IAR Compiler includes. */
#include <intrinsics.h>

#include "SmallRTOS.h" 

#ifdef __cplusplus
extern "C" {
#endif
/* Constants required to manipulate the NVIC. */
#define FitNVIC_SYSTICK_CTRL		( ( volatile uOS32_t *) 0xe000e010 )
#define FitNVIC_SYSTICK_LOAD		( ( volatile uOS32_t *) 0xe000e014 )
#define FitNVIC_SYSPRI2				( ( volatile uOS32_t *) 0xe000ed20 )
#define FitNVIC_SYSTICK_CLK			0x00000004
#define FitNVIC_SYSTICK_INT			0x00000002
#define FitNVIC_SYSTICK_ENABLE		0x00000001
#define FitMIN_INTERRUPT_PRIORITY	( 255UL )
#define FitNVIC_PENDSV_PRI			( FitMIN_INTERRUPT_PRIORITY << 16UL )
#define FitNVIC_SYSTICK_PRI			( FitMIN_INTERRUPT_PRIORITY << 24UL )

/* Constants required to set up the initial stack. */
#define FitINITIAL_XPSR				( 0x01000000 )

/* Each task maintains its own interrupt status in the critical nesting
variable. */
static uOSBase_t uxCriticalNesting = 0xaaaaaaaa;

static void FitSetupTimerInterrupt( void );
void FitOSTickISR( void );
extern void FitStartFirstTask( void );
static void FitTaskExitError( void );

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
	should instead call vTaskDelete( NULL ).*/

	FitDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
sOSBase_t FitStartScheduler( void )
{
	/* Make PendSV and SysTick the lowest priority interrupts. */
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
	*(FitNVIC_INT_CTRL) = FitNVIC_PENDSVSET;

	/* Barriers are normally not required but do ensure the code is completely
	within the specified behaviour for the architecture. */
	__DSB();
	__ISB();
}
/*-----------------------------------------------------------*/

void FitEnterCritical( void )
{
	FitDISABLE_INTERRUPTS();
	uxCriticalNesting++;
	__DSB();
	__ISB();
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
static void FitSetupTimerInterrupt( void )
{
	/* Configure SysTick to interrupt at the requested rate. */
	*(FitNVIC_SYSTICK_LOAD) = ( configCPU_CLOCK_HZ / OSTICK_RATE_HZ ) - 1UL;
	*(FitNVIC_SYSTICK_CTRL) = FitNVIC_SYSTICK_CLK | FitNVIC_SYSTICK_INT | FitNVIC_SYSTICK_ENABLE;
}

#ifdef __cplusplus
}
#endif
