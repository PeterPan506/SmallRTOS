/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial supFit, development, Fiting, licensing and training services.
***********************************************************************************************************/

#ifndef __FITCPU_HPP
#define __FITCPU_HPP
#include "stdint.h"
#include "OSType.h"

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------
 * Fit specific definitions.
 *
 * The settings in this file configure SmallRTOS correctly for the given hardware
 * and compiler.
 *
 * These settings should not be altered.
 *-----------------------------------------------------------
 */

/* Task utilities. */

/* Called at the end of an ISR that can cause a context switch. */
#define FitEND_SWITCHING_ISR( xSwitchRequired )\
{												\
extern uint32_t ulFitYieldRequired;			\
												\
	if( xSwitchRequired != 0 )			\
	{											\
		ulFitYieldRequired = 1;			\
	}											\
}

#define FitYIELD_FROM_ISR( x ) FitEND_SWITCHING_ISR( x )
#define FitYIELD() __asm volatile ( "SWI 0" );

#define FitYield()		FitYIELD()

/*-----------------------------------------------------------
 * Critical section control
 *----------------------------------------------------------*/

extern void FitEnterCritical( void );
extern void FitExitCritical( void );
extern uint32_t FitSetInterruptMask( void );
extern void FitClearInterruptMask( uint32_t ulNewMaskValue );
extern void FitInstallSmallRTOSVectorTable( void );

/* These macros do not globally disable/enable interrupts.  They do mask off
interrupts that have a priority below configMAX_API_CALL_INTERRUPT_PRIORITY. */
#define FitENTER_CRITICAL()						FitEnterCritical();
#define FitEXIT_CRITICAL()						FitExitCritical();
#define FitDISABLE_INTERRUPTS()					FitSetInterruptMask()
#define FitENABLE_INTERRUPTS()					FitClearInterruptMask( 0 )
#define FitSET_INTERRUPT_MASK_FROM_ISR()		FitSetInterruptMask()
#define FitCLEAR_INTERRUPT_MASK_FROM_ISR(x)		FitClearInterruptMask(x)


/* Prototype of the SmallRTOS tick handler.  This must be installed as the
handler for whichever peripheral is used to generate the RTOS tick. */
void FitOSTickISR( void );

/* Any task that uses the floating point unit MUST call FitTaskUsesFPU()
before any floating point instructions are executed. */
void FitTaskUsesFPU( void );
#define FitTASK_USES_FLOATING_POINT() FitTaskUsesFPU()

#define FitLOWEST_INTERRUPT_PRIORITY ( ( ( uint32_t ) configUNIQUE_INTERRUPT_PRIORITIES ) - 1UL )
#define FitLOWEST_USABLE_INTERRUPT_PRIORITY ( FitLOWEST_INTERRUPT_PRIORITY - 1UL )

#define FIT_QUICK_GET_PRIORITY		1
#define FitGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities ) uxTopPriority = ( 31 - __builtin_clz( uxReadyPriorities ) )

#define FitNOP() __asm volatile( "NOP" )

/* The number of bits to shift for an interrupt priority is dependent on the
number of bits implemented by the interrupt controller. */
#if configUNIQUE_INTERRUPT_PRIORITIES == 16
	#define FitPRIORITY_SHIFT 4
	#define FitMAX_BINARY_POINT_VALUE	3
#elif configUNIQUE_INTERRUPT_PRIORITIES == 32
	#define FitPRIORITY_SHIFT 3
	#define FitMAX_BINARY_POINT_VALUE	2
#elif configUNIQUE_INTERRUPT_PRIORITIES == 64
	#define FitPRIORITY_SHIFT 2
	#define FitMAX_BINARY_POINT_VALUE	1
#elif configUNIQUE_INTERRUPT_PRIORITIES == 128
	#define FitPRIORITY_SHIFT 1
	#define FitMAX_BINARY_POINT_VALUE	0
#elif configUNIQUE_INTERRUPT_PRIORITIES == 256
	#define FitPRIORITY_SHIFT 0
	#define FitMAX_BINARY_POINT_VALUE	0
#else
	#error Invalid configUNIQUE_INTERRUPT_PRIORITIES setting.  configUNIQUE_INTERRUPT_PRIORITIES must be set to the number of unique priorities implemented by the target hardware
#endif

/* Interrupt controller access addresses. */
#define FitICCPMR_PRIORITY_MASK_OFFSET  						( 0x04 )
#define FitICCIAR_INTERRUPT_ACKNOWLEDGE_OFFSET 				( 0x0C )
#define FitICCEOIR_END_OF_INTERRUPT_OFFSET 					( 0x10 )
#define FitICCBPR_BINARY_POINT_OFFSET							( 0x08 )
#define FitICCRPR_RUNNING_PRIORITY_OFFSET						( 0x14 )

#define FitINTERRUPT_CONTROLLER_CPU_INTERFACE_ADDRESS 		( configINTERRUPT_CONTROLLER_BASE_ADDRESS + configINTERRUPT_CONTROLLER_CPU_INTERFACE_OFFSET )
#define FitICCPMR_PRIORITY_MASK_REGISTER 					( *( ( volatile uint32_t * ) ( FitINTERRUPT_CONTROLLER_CPU_INTERFACE_ADDRESS + FitICCPMR_PRIORITY_MASK_OFFSET ) ) )
#define FitICCIAR_INTERRUPT_ACKNOWLEDGE_REGISTER_ADDRESS 	( FitINTERRUPT_CONTROLLER_CPU_INTERFACE_ADDRESS + FitICCIAR_INTERRUPT_ACKNOWLEDGE_OFFSET )
#define FitICCEOIR_END_OF_INTERRUPT_REGISTER_ADDRESS 		( FitINTERRUPT_CONTROLLER_CPU_INTERFACE_ADDRESS + FitICCEOIR_END_OF_INTERRUPT_OFFSET )
#define FitICCPMR_PRIORITY_MASK_REGISTER_ADDRESS 			( FitINTERRUPT_CONTROLLER_CPU_INTERFACE_ADDRESS + FitICCPMR_PRIORITY_MASK_OFFSET )
#define FitICCBPR_BINARY_POINT_REGISTER 					( *( ( const volatile uint32_t * ) ( FitINTERRUPT_CONTROLLER_CPU_INTERFACE_ADDRESS + FitICCBPR_BINARY_POINT_OFFSET ) ) )
#define FitICCRPR_RUNNING_PRIORITY_REGISTER 				( *( ( const volatile uint32_t * ) ( FitINTERRUPT_CONTROLLER_CPU_INTERFACE_ADDRESS + FitICCRPR_RUNNING_PRIORITY_OFFSET ) ) )



#define OS_ENTER_CRITICAL()			FitEnterCritical()
#define OS_EXIT_CRITICAL()			FitExitCritical()

uOSStack_t *FitInitializeStack( uOSStack_t *pxTopOfStack,
		OSTaskFunction_t TaskFunction, void *pvParameters );
uOSBase_t FitStartScheduler( void );

#ifdef __cplusplus
	} /* extern C */
#endif

#endif //__FITCPU_HPP
