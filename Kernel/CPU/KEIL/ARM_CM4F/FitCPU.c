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
#define FitNVIC_SYSTICK_CLK_BIT				( 1UL << 2UL )
#define FitNVIC_SYSTICK_INT_BIT				( 1UL << 1UL )
#define FitNVIC_SYSTICK_ENABLE_BIT			( 1UL << 0UL )
#define FitNVIC_SYSTICK_COUNT_FLAG_BIT		( 1UL << 16UL )
#define FitNVIC_PENDSVCLEAR_BIT 			( 1UL << 27UL )
#define FitNVIC_PEND_SYSTICK_CLEAR_BIT		( 1UL << 25UL )

/* Constants used to detect a Cortex-M7 r0p1 core, which should use the ARM_CM7
r0p1 port. */
#define FitCPUID							( * ( ( volatile uOS32_t * ) 0xE000ed00 ) )
//#define portCORTEX_M7_r0p1_ID				( 0x410FC271UL )
//#define portCORTEX_M7_r0p0_ID				( 0x410FC270UL )

#define FitNVIC_PENDSV_PRI					( ( ( uOS32_t ) FitKERNEL_INTERRUPT_PRIORITY ) << 16UL )
#define FitNVIC_SYSTICK_PRI					( ( ( uOS32_t ) FitKERNEL_INTERRUPT_PRIORITY ) << 24UL )

/* Constants required to check the validity of an interrupt priority. */
#define FitFIRST_USER_INTERRUPT_NUMBER		( 16 )
#define FitNVIC_IP_REGISTERS_OFFSET_16 		( 0xE000E3F0 )
#define FitAIRCR_REG						( * ( ( volatile uOS32_t * ) 0xE000ED0C ) )
#define FitMAX_8_BIT_VALUE					( ( uOS8_t ) 0xff )
#define FitTOP_BIT_OF_BYTE					( ( uOS8_t ) 0x80 )
#define FitMAX_PRIGROUP_BITS				( ( uOS8_t ) 7 )
#define FitPRIORITY_GROUP_MASK				( 0x07UL << 8UL )
#define FitPRIGROUP_SHIFT					( 8UL )

/* Masks off all bits but the VECTACTIVE bits in the ICSR register. */
#define FitVECTACTIVE_MASK					( 0xFFUL )

/* Constants required to manipulate the VFP. */
#define FitFPCCR							( ( volatile uOS32_t * ) 0xe000ef34 ) /* Floating point context control register. */
#define FitASPEN_AND_LSPEN_BITS				( 0x3UL << 30UL )

/* Constants required to set up the initial stack. */
#define FitINITIAL_XPSR						( 0x01000000 )
#define FitINITIAL_EXEC_RETURN				( 0xfffffffd )

/* The systick is a 24-bit counter. */
#define FitMAX_24_BIT_NUMBER				( 0xffffffUL )

/* A fiddle factor to estimate the number of SysTick counts that would have
occurred while the SysTick counter is stopped during tickless idle
calculations. */
#define FitMISSED_COUNTS_FACTOR				( 45UL )

/* Each task maintains its own interrupt status in the critical nesting
variable. */
static uOSBase_t uxCriticalNesting = 0xaaaaaaaa;

/*
 * Setup the timer to generate the tick interrupts.  The implementation in this
 * file is weak to allow application writers to change the timer used to
 * generate the tick interrupt.
 */
void FitSetupTimerInterrupt( void );

/*
 * Exception handlers.
 */
void FitPendSVHandler( void );
void FitOSTickISR( void );
void FitSVCHandler( void );
static void FitStartFirstTask( void );
static void FitEnableVFP( void );
static void FitTaskExitError( void );


uOSStack_t *FitInitializeStack( uOSStack_t *pxTopOfStack,
		OSTaskFunction_t TaskFunction, void *pvParameters )
{
	/* Simulate the stack frame as it would be created by a context switch
	interrupt. */

	/* Offset added to account for the way the MCU uses the stack on entry/exit
	of interrupts, and to ensure alignment. */
	pxTopOfStack--;

	*pxTopOfStack = FitINITIAL_XPSR;	/* xPSR */
	pxTopOfStack--;
	*pxTopOfStack = ( uOSStack_t ) TaskFunction;	/* PC */
	pxTopOfStack--;
	*pxTopOfStack = ( uOSStack_t ) FitTaskExitError;	/* LR */

	/* Save code space by skipping register initialisation. */
	pxTopOfStack -= 5;	/* R12, R3, R2 and R1. */
	*pxTopOfStack = ( uOSStack_t ) pvParameters;	/* R0 */

	/* A save method is being used that requires each task to maintain its
	own exec return value. */
	pxTopOfStack--;
	*pxTopOfStack = FitINITIAL_EXEC_RETURN;

	pxTopOfStack -= 8;	/* R11, R10, R9, R8, R7, R6, R5 and R4. */

	return pxTopOfStack;	
}

static void FitTaskExitError( void )
{
	/* A function that implements a task must not exit or attempt to return to
	its caller as there is nothing to return to.  If a task wants to exit it
	should instead call OSTaskDelete( NULL ).*/
	
	FitDISABLE_INTERRUPTS();
	for( ;; );
}

__asm void FitSVCHandler( void )
{
	PRESERVE8

	/* Get the location of the current TCB. */
	ldr	r3, = gptCurrentTCB
	ldr r1, [r3]
	ldr r0, [r1]
	/* Pop the core registers. */
	ldmia r0!, {r4-r11, r14}
	msr psp, r0
	isb
	mov r0, #0
	msr	basepri, r0
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

__asm void FitEnableVFP( void )
{
	PRESERVE8

	/* The FPU enable bits are in the CPACR. */
	ldr.w r0, =0xE000ED88
	ldr	r1, [r0]

	/* Enable CP10 and CP11 coprocessors, then save back. */
	orr	r1, r1, #( 0xf << 20 )
	str r1, [r0]
	bx	r14
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

	/* Ensure the VFP is enabled - it should be anyway. */
	FitEnableVFP();

	/* Lazy save always. */
	*( FitFPCCR ) |= FitASPEN_AND_LSPEN_BITS;

	/* Start the first task. */
	FitStartFirstTask();

	/* Should not get here! */
	return 0;	
}

void FitEndScheduler( void )
{

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
	/* Get the location of the current TCB. */
	ldr	r3, =gptCurrentTCB
	ldr	r2, [r3]

	/* Is the task using the FPU context?  If so, push high vfp registers. */
	tst r14, #0x10
	it eq
	vstmdbeq r0!, {s16-s31}

	/* Save the core registers. */
	stmdb r0!, {r4-r11, r14}

	/* Save the new top of stack into the first member of the TCB. */
	str r0, [r2]

	stmdb sp!, {r3}
	mov r0, #configMAX_SYSCALL_INTERRUPT_PRIORITY
	msr basepri, r0
	dsb
	isb	
	bl OSTaskSwitchContext
	mov r0, #0
	msr basepri, r0
	ldmia sp!, {r3}

	/* The first item in pxCurrentTCB is the task top of stack. */
	ldr r1, [r3]
	ldr r0, [r1]

	/* Pop the core registers. */
	ldmia r0!, {r4-r11, r14}

	/* Is the task using the FPU context?  If so, pop the high vfp registers
	too. */
	tst r14, #0x10
	it eq
	vldmiaeq r0!, {s16-s31}

	msr psp, r0
	isb
	#ifdef WORKAROUND_PMU_CM001 /* XMC4000 specific errata */
		#if WORKAROUND_PMU_CM001 == 1
			push { r14 }
			pop { pc }
			nop
		#endif
	#endif

	bx r14
	nop
}

void FitOSTickISR( void )
{
	/* The SysTick runs at the lowest interrupt priority, so when this interrupt
	executes all interrupts must be unmasked.  There is therefore no need to
	save and then restore the interrupt mask value as its value is already
	known. */
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

__asm uOS32_t FitGetIPSR( void )
{
	PRESERVE8

	mrs r0, ipsr
	bx r14
}
#ifdef __cplusplus
}
#endif
