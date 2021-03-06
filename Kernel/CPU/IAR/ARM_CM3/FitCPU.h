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
extern void FitYield( void );
#define FitNVIC_INT_CTRL_REG		( * ( ( volatile uOS32_t * ) 0xe000ed04UL ) )
#define FitNVIC_PENDSVSET_BIT		( 1UL << 28UL )
#define FitYIELD()					FitYield()
#define FitEND_SWITCHING_ISR( xSwitchRequired ) if( xSwitchRequired ) FitNVIC_INT_CTRL_REG = FitNVIC_PENDSVSET_BIT
#define FitYIELD_FROM_ISR( x ) 		FitEND_SWITCHING_ISR( x )
/*-----------------------------------------------------------*/

#include <intrinsics.h>
#define FIT_QUICK_GET_PRIORITY		1
#define FitGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities ) uxTopPriority = ( 31 - __CLZ( ( uxReadyPriorities ) ) )

/* Critical section management. */
extern void FitEnterCritical( void );
extern void FitExitCritical( void );
extern uOS32_t FitSetInterruptMask( void );
extern void FitClearInterruptMask( uOS32_t ulNewMask );

#define FitDISABLE_INTERRUPTS()					FitSetInterruptMask()
#define FitENABLE_INTERRUPTS()					FitClearInterruptMask( 0 )
#define FitENTER_CRITICAL()						FitEnterCritical()
#define FitEXIT_CRITICAL()						FitExitCritical()
#define FitSET_INTERRUPT_MASK_FROM_ISR()		FitSetInterruptMask()
#define FitCLEAR_INTERRUPT_MASK_FROM_ISR(x)		FitClearInterruptMask( x )
/*-----------------------------------------------------------*/

#define OS_ENTER_CRITICAL()						FitEnterCritical()
#define OS_EXIT_CRITICAL()						FitExitCritical()

uOSStack_t *FitInitializeStack( uOSStack_t *pxTopOfStack, OSTaskFunction_t TaskFunction, void *pvParameters );
sOSBase_t FitStartScheduler( void );

#ifdef __cplusplus
}
#endif

#endif //__FITCPU_HPP
