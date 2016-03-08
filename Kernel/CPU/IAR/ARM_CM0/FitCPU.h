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
#define FitNVIC_INT_CTRL			( ( volatile uOS32_t *) 0xe000ed04 )
#define FitNVIC_PENDSVSET			0x10000000
#define FitYIELD()					FitYield()
#define FitEND_SWITCHING_ISR( xSwitchRequired ) if( xSwitchRequired ) 	*(FitNVIC_INT_CTRL) = FitNVIC_PENDSVSET
#define FitYIELD_FROM_ISR( x ) FitEND_SWITCHING_ISR( x )

/* Critical section management. */
extern void FitEnterCritical( void );
extern void FitExitCritical( void );
extern uOS32_t FitSetInterruptMask( void );
extern void FitClearInterruptMask( uOS32_t ulMask );

#define FitDISABLE_INTERRUPTS()					__asm volatile( "cpsid i" )
#define FitENABLE_INTERRUPTS()					__asm volatile( "cpsie i" )
#define FitENTER_CRITICAL()						FitEnterCritical()
#define FitEXIT_CRITICAL()						FitExitCritical()
#define FitSET_INTERRUPT_MASK_FROM_ISR()		FitSetInterruptMask()
#define FitCLEAR_INTERRUPT_MASK_FROM_ISR(x)		FitClearInterruptMask( x )


#define OS_ENTER_CRITICAL()						FitEnterCritical()
#define OS_EXIT_CRITICAL()						FitExitCritical()

uOSStack_t *FitInitializeStack( uOSStack_t *pxTopOfStack, OSTaskFunction_t TaskFunction, void *pvParameters );
sOSBase_t FitStartScheduler( void );

#ifdef __cplusplus
}
#endif

#endif //__FITCPU_HPP
