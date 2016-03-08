/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial supFit, development, Fiting, licensing and training services.
***********************************************************************************************************/

#ifndef __FITCPU_HPP
#define __FITCPU_HPP

#include "OSType.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Scheduler utilities. */
#define FitYIELD()											\
{															\
	/* Set a PendSV to request a context switch. */			\
	FitNVIC_INT_CTRL_REG = FitNVIC_PENDSVSET_BIT;			\
	__DSB();												\
	__ISB();												\
}

#define FitNVIC_INT_CTRL_REG		( * ( ( volatile uOS32_t * ) 0xe000ed04 ) )
#define FitNVIC_PENDSVSET_BIT		( 1UL << 28UL )
#define FitEND_SWITCHING_ISR( xSwitchRequired ) if( xSwitchRequired != FALSE ) FitYIELD()
#define FitYIELD_FROM_ISR( x ) 		FitEND_SWITCHING_ISR( x )
#define FitYield( ) 		        FitYIELD( )
/*-----------------------------------------------------------*/

#include <intrinsics.h>
#define FIT_QUICK_GET_PRIORITY		1
#define FitGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities ) uxTopPriority = ( 31 - __CLZ( ( uxReadyPriorities ) ) )

/* Critical section management. */
extern void FitEnterCritical( void );
extern void FitExitCritical( void );

#define FitDISABLE_INTERRUPTS()								\
{															\
	__set_BASEPRI( configMAX_SYSCALL_INTERRUPT_PRIORITY );	\
	__DSB();												\
	__ISB();												\
}

#define FitENABLE_INTERRUPTS()					__set_BASEPRI( 0 )
#define FitENTER_CRITICAL()						FitEnterCritical()
#define FitEXIT_CRITICAL()						FitExitCritical()
#define FitSET_INTERRUPT_MASK_FROM_ISR()		__get_BASEPRI(); FitDISABLE_INTERRUPTS()
#define FitCLEAR_INTERRUPT_MASK_FROM_ISR(x)		__set_BASEPRI( x )
/*-----------------------------------------------------------*/
#define OS_ENTER_CRITICAL()						FitEnterCritical()
#define OS_EXIT_CRITICAL()						FitExitCritical()

uOSStack_t *FitInitializeStack( uOSStack_t *pxTopOfStack, OSTaskFunction_t TaskFunction, void *pvParameters );
sOSBase_t FitStartScheduler( void );

#ifdef __cplusplus
}
#endif

#endif //__FITCPU_HPP
