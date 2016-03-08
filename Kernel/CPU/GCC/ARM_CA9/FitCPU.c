/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial supFit, development, Fiting, licensing and training services.
***********************************************************************************************************/

#include "SmallRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A critical section is exited when the critical section nesting count reaches
this value. */
#define FitNO_CRITICAL_NESTING			( ( uOS32_t ) 0 )

/* In all GICs 255 can be written to the priority mask register to unmask all
(but the lowest) interrupt priority. */
#define FitUNMASK_VALUE					( 0xFFUL )

/* Tasks are not created with a floating point context, but can be given a
floating point context after they have been created.  A variable is stored as
part of the tasks context that holds FitNO_FLOATING_POINT_CONTEXT if the task
does not have an FPU context, or any other value if the task does have an FPU
context. */
#define FitNO_FLOATING_POINT_CONTEXT	( ( uOSStack_t ) 0 )

/* Constants required to setup the initial task context. */
#define FitINITIAL_SPSR					( ( uOSStack_t ) 0x1f ) /* System mode, ARM mode, IRQ enabled FIQ enabled. */
#define FitTHUMB_MODE_BIT				( ( uOSStack_t ) 0x20 )
#define FitINTERRUPT_ENABLE_BIT			( 0x80UL )
#define FitTHUMB_MODE_ADDRESS			( 0x01UL )

/* Used by FitASSERT_IF_INTERRUPT_PRIORITY_INVALID() when ensuring the binary
point is zero. */
#define FitBINARY_POINT_BITS			( ( uint8_t ) 0x03 )

/* Masks all bits in the APSR other than the mode bits. */
#define FitAPSR_MODE_BITS_MASK			( 0x1F )

/* The value of the mode bits in the APSR when the CPU is executing in user
mode. */
#define FitAPSR_USER_MODE				( 0x10 )

/* The critical section macros only mask interrupts up to an application
determined priority level.  Sometimes it is necessary to turn interrupt off in
the CPU itself before modifying certain hardware registers. */
#define FitCPU_IRQ_DISABLE()										\
	__asm volatile ( "CPSID i" );									\
	__asm volatile ( "DSB" );										\
	__asm volatile ( "ISB" );

#define FitCPU_IRQ_ENABLE()											\
	__asm volatile ( "CPSIE i" );									\
	__asm volatile ( "DSB" );										\
	__asm volatile ( "ISB" );


/* Macro to unmask all interrupt priorities. */
#define FitCLEAR_INTERRUPT_MASK()									\
{																	\
	FitCPU_IRQ_DISABLE();											\
	FitICCPMR_PRIORITY_MASK_REGISTER = FitUNMASK_VALUE;				\
	__asm volatile (	"DSB		\n"								\
						"ISB		\n" );							\
	FitCPU_IRQ_ENABLE();											\
}

#define FitINTERRUPT_PRIORITY_REGISTER_OFFSET		0x400UL
#define FitMAX_8_BIT_VALUE							( ( uint8_t ) 0xff )
#define FitBIT_0_SET								( ( uint8_t ) 0x01 )

/* Let the user override the pre-loading of the initial LR with the address of
FitTaskExitError() in case is messes up unwinding of the stack in the
debugger. */
#ifdef configTASK_RETURN_ADDRESS
	#define FitTASK_RETURN_ADDRESS	configTASK_RETURN_ADDRESS
#else
	#define FitTASK_RETURN_ADDRESS	FitTaskExitError
#endif

/*-----------------------------------------------------------*/

/*
 * Starts the first task executing.  This function is necessarily written in
 * assembly code so is implemented in FitASM.s.
 */
extern void FitRestoreTaskContext( void );

/*
 * Used to catch tasks that attempt to return from their implementing function.
 */
static void FitTaskExitError( void );

/*-----------------------------------------------------------*/

/* A variable is used to keep track of the critical section nesting.  This
variable has to be stored as part of the task context and must be initialised to
a non zero value to ensure interrupts don't inadvertently become unmasked before
the scheduler starts.  As it is stored as part of the task context it will
automatically be set to 0 when the first task is started. */
volatile uint32_t ulCriticalNesting = 9999UL;

/* Saved as part of the task context.  If ulFitTaskHasFPUContext is non-zero then
a floating point context must be saved and restored for the task. */
uint32_t ulFitTaskHasFPUContext = FALSE;

/* Set to 1 to pend a context switch from an ISR. */
uint32_t ulFitYieldRequired = FALSE;

/* Counts the interrupt nesting depth.  A context switch is only performed if
if the nesting depth is 0. */
uint32_t ulFitInterruptNesting = 0UL;

__attribute__(( used )) const uint32_t ulICCIAR = FitICCIAR_INTERRUPT_ACKNOWLEDGE_REGISTER_ADDRESS;
__attribute__(( used )) const uint32_t ulICCEOIR = FitICCEOIR_END_OF_INTERRUPT_REGISTER_ADDRESS;
__attribute__(( used )) const uint32_t ulICCPMR	= FitICCPMR_PRIORITY_MASK_REGISTER_ADDRESS;
__attribute__(( used )) const uint32_t ulMaxAPIPriorityMask = ( configMAX_API_CALL_INTERRUPT_PRIORITY << FitPRIORITY_SHIFT );

/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
uOSStack_t *FitInitializeStack( uOSStack_t *pxTopOfStack,
		OSTaskFunction_t TaskFunction, void *pvParameters )
{
	/* Setup the initial stack of the task.  The stack is set exactly as
	expected by the FitRESTORE_CONTEXT() macro.

	The fist real value on the stack is the status register, which is set for
	system mode, with interrupts enabled.  A few NULLs are added first to ensure
	GDB does not try decoding a non-existent return address. */
	*pxTopOfStack = ( uOSStack_t ) NULL;
	pxTopOfStack--;
	*pxTopOfStack = ( uOSStack_t ) NULL;
	pxTopOfStack--;
	*pxTopOfStack = ( uOSStack_t ) NULL;
	pxTopOfStack--;
	*pxTopOfStack = ( uOSStack_t ) FitINITIAL_SPSR;

	if( ( ( uint32_t ) TaskFunction & FitTHUMB_MODE_ADDRESS ) != 0x00UL )
	{
		/* The task will start in THUMB mode. */
		*pxTopOfStack |= FitTHUMB_MODE_BIT;
	}

	pxTopOfStack--;

	/* Next the return address, which in this case is the start of the task. */
	*pxTopOfStack = ( uOSStack_t ) TaskFunction;
	pxTopOfStack--;

	/* Next all the registers other than the stack pointer. */
	*pxTopOfStack = ( uOSStack_t ) FitTASK_RETURN_ADDRESS;	/* R14 */
	pxTopOfStack--;
	*pxTopOfStack = ( uOSStack_t ) 0x12121212;	/* R12 */
	pxTopOfStack--;
	*pxTopOfStack = ( uOSStack_t ) 0x11111111;	/* R11 */
	pxTopOfStack--;
	*pxTopOfStack = ( uOSStack_t ) 0x10101010;	/* R10 */
	pxTopOfStack--;
	*pxTopOfStack = ( uOSStack_t ) 0x09090909;	/* R9 */
	pxTopOfStack--;
	*pxTopOfStack = ( uOSStack_t ) 0x08080808;	/* R8 */
	pxTopOfStack--;
	*pxTopOfStack = ( uOSStack_t ) 0x07070707;	/* R7 */
	pxTopOfStack--;
	*pxTopOfStack = ( uOSStack_t ) 0x06060606;	/* R6 */
	pxTopOfStack--;
	*pxTopOfStack = ( uOSStack_t ) 0x05050505;	/* R5 */
	pxTopOfStack--;
	*pxTopOfStack = ( uOSStack_t ) 0x04040404;	/* R4 */
	pxTopOfStack--;
	*pxTopOfStack = ( uOSStack_t ) 0x03030303;	/* R3 */
	pxTopOfStack--;
	*pxTopOfStack = ( uOSStack_t ) 0x02020202;	/* R2 */
	pxTopOfStack--;
	*pxTopOfStack = ( uOSStack_t ) 0x01010101;	/* R1 */
	pxTopOfStack--;
	*pxTopOfStack = ( uOSStack_t ) pvParameters; /* R0 */
	pxTopOfStack--;

	/* The task will start with a critical nesting count of 0 as interrupts are
	enabled. */
	*pxTopOfStack = FitNO_CRITICAL_NESTING;
	pxTopOfStack--;

	/* The task will start without a floating point context.  A task that uses
	the floating point hardware must call FitTaskUsesFPU() before executing
	any floating point instructions. */
	*pxTopOfStack = FitNO_FLOATING_POINT_CONTEXT;

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

uOSBase_t FitStartScheduler( void )
{
	uint32_t ulAPSR;

	{
		volatile uint32_t ulOriginalPriority;
		volatile uint8_t * const pucFirstUserPriorityRegister = ( volatile uint8_t * const ) ( configINTERRUPT_CONTROLLER_BASE_ADDRESS + FitINTERRUPT_PRIORITY_REGISTER_OFFSET );
		volatile uint8_t ucMaxPriorityValue;

		/* Determine how many priority bits are implemented in the GIC.

		Save the interrupt priority value that is about to be clobbered. */
		ulOriginalPriority = *pucFirstUserPriorityRegister;

		/* Determine the number of priority bits available.  First write to
		all possible bits. */
		*pucFirstUserPriorityRegister = FitMAX_8_BIT_VALUE;

		/* Read the value back to see how many bits stuck. */
		ucMaxPriorityValue = *pucFirstUserPriorityRegister;

		/* Shift to the least significant bits. */
		while( ( ucMaxPriorityValue & FitBIT_0_SET ) != FitBIT_0_SET )
		{
			ucMaxPriorityValue >>= ( uint8_t ) 0x01;
		}

		/* Restore the clobbered interrupt priority register to its original
		value. */
		*pucFirstUserPriorityRegister = ulOriginalPriority;
	}

	/* Only continue if the CPU is not in User mode.  The CPU must be in a
	Privileged mode for the scheduler to start. */
	__asm volatile ( "MRS %0, APSR" : "=r" ( ulAPSR ) );
	ulAPSR &= FitAPSR_MODE_BITS_MASK;

	if( ulAPSR != FitAPSR_USER_MODE )
	{
		/* Only continue if the binary point value is set to its lowest possible
		setting.   */
		
		if( ( FitICCBPR_BINARY_POINT_REGISTER & FitBINARY_POINT_BITS ) <= FitMAX_BINARY_POINT_VALUE )
		{
			/* Interrupts are turned off in the CPU itself to ensure tick does
			not execute	while the scheduler is being started.  Interrupts are
			automatically turned back on in the CPU when the first task starts
			executing. */
			FitCPU_IRQ_DISABLE();

			/* Start the timer that generates the tick ISR. */
			configSETUP_TICK_INTERRUPT();

			/* Start the first task executing. */
			FitRestoreTaskContext();
		}
	}

	/* Will only get here if xTaskStartScheduler() was called with the CPU in
	a non-privileged mode or the binary point register was not set to its lowest
	possible value.  FitTaskExitError() is referenced to prevent a compiler
	warning about it being defined but not referenced in the case that the user
	defines their own exit address. */
	( void ) FitTaskExitError;
	return 0;
}
/*-----------------------------------------------------------*/

void vFitEndScheduler( void )
{
	/* Not implemented in Fits where there is nothing to return to.
	Artificially force an assert. */
}
/*-----------------------------------------------------------*/

void FitEnterCritical( void )
{
	/* Mask interrupts up to the max syscall interrupt priority. */
	FitSetInterruptMask();

	/* Now interrupts are disabled ulCriticalNesting can be accessed
	directly.  Increment ulCriticalNesting to keep a count of how many times
	FitENTER_CRITICAL() has been called. */
	ulCriticalNesting++;

}
/*-----------------------------------------------------------*/

void FitExitCritical( void )
{
	if( ulCriticalNesting > FitNO_CRITICAL_NESTING )
	{
		/* Decrement the nesting count as the critical section is being
		exited. */
		ulCriticalNesting--;

		/* If the nesting level has reached zero then all interrupt
		priorities must be re-enabled. */
		if( ulCriticalNesting == FitNO_CRITICAL_NESTING )
		{
			/* Critical nesting has reached zero so all interrupt priorities
			should be unmasked. */
			FitCLEAR_INTERRUPT_MASK();
		}
	}
}
/*-----------------------------------------------------------*/

void FitOSTickISR( void )
{
	/* Set interrupt mask before altering scheduler structures.   The tick
	handler runs at the lowest priority, so interrupts cannot already be masked,
	so there is no need to save and restore the current mask value.  It is
	necessary to turn off interrupts in the CPU itself while the ICCPMR is being
	updated. */
	FitCPU_IRQ_DISABLE();
	FitICCPMR_PRIORITY_MASK_REGISTER = ( uint32_t ) ( configMAX_API_CALL_INTERRUPT_PRIORITY << FitPRIORITY_SHIFT );
	__asm volatile (	"dsb		\n"
						"isb		\n" );
	FitCPU_IRQ_ENABLE();

	/* Increment the RTOS tick. */
	if( OSTaskIncrementTick() != FALSE )
	{
		ulFitYieldRequired = TRUE;
	}

	/* Ensure all interrupt priorities are active again. */
	FitCLEAR_INTERRUPT_MASK();
	configCLEAR_TICK_INTERRUPT();
}
/*-----------------------------------------------------------*/

void FitTaskUsesFPU( void )
{
uint32_t ulInitialFPSCR = 0;

	/* A task is registering the fact that it needs an FPU context.  Set the
	FPU flag (which is saved as part of the task context). */
	ulFitTaskHasFPUContext = TRUE;

	/* Initialise the floating point status register. */
	__asm volatile ( "FMXR 	FPSCR, %0" :: "r" (ulInitialFPSCR) );
}
/*-----------------------------------------------------------*/

void FitClearInterruptMask( uint32_t ulNewMaskValue )
{
	if( ulNewMaskValue == FALSE )
	{
		FitCLEAR_INTERRUPT_MASK();
	}
}
/*-----------------------------------------------------------*/

uint32_t FitSetInterruptMask( void )
{
uint32_t ulReturn;

	/* Interrupt in the CPU must be turned off while the ICCPMR is being
	updated. */
	FitCPU_IRQ_DISABLE();
	if( FitICCPMR_PRIORITY_MASK_REGISTER == ( uint32_t ) ( configMAX_API_CALL_INTERRUPT_PRIORITY << FitPRIORITY_SHIFT ) )
	{
		/* Interrupts were already masked. */
		ulReturn = TRUE;
	}
	else
	{
		ulReturn = FALSE;
		FitICCPMR_PRIORITY_MASK_REGISTER = ( uint32_t ) ( configMAX_API_CALL_INTERRUPT_PRIORITY << FitPRIORITY_SHIFT );
		__asm volatile (	"dsb		\n"
							"isb		\n" );
	}
	FitCPU_IRQ_ENABLE();

	return ulReturn;
}

#ifdef __cplusplus
}
#endif
