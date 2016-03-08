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

/* Constants used with memory barrier intrinsics. */
#define FitSY_FULL_READ_WRITE		( 15 )

/*-----------------------------------------------------------*/

/* Scheduler utilities. */
#define FitYIELD()																\
{																				\
	/* Set a PendSV to request a context switch. */								\
	FitNVIC_INT_CTRL_REG = FitNVIC_PENDSVSET_BIT;								\
																				\
	/* Barriers are normally not required but do ensure the code is completely	\
	within the specified behaviour for the architecture. */						\
	__dsb( FitSY_FULL_READ_WRITE );											\
	__isb( FitSY_FULL_READ_WRITE );											\
}
/*-----------------------------------------------------------*/

#define FitNVIC_INT_CTRL_REG		( * ( ( volatile uOS32_t * ) 0xe000ed04 ) )
#define FitNVIC_PENDSVSET_BIT		( 1UL << 28UL )
#define FitEND_SWITCHING_ISR( xSwitchRequired ) if( xSwitchRequired != FALSE ) FitYIELD()
#define FitYIELD_FROM_ISR( x ) FitEND_SWITCHING_ISR( x )
/*-----------------------------------------------------------*/

/* Critical section management. */
extern void FitEnterCritical( void );
extern void FitExitCritical( void );

#define FitDISABLE_INTERRUPTS()					FitRaiseBasePRI()
#define FitENABLE_INTERRUPTS()					FitSetBasePRI( 0 )
#define FitENTER_CRITICAL()						FitEnterCritical()
#define FitEXIT_CRITICAL()						FitExitCritical()
#define FitSET_INTERRUPT_MASK_FROM_ISR()		FitRaiseBasePRIFromISR()
#define FitCLEAR_INTERRUPT_MASK_FROM_ISR(x)		FitSetBasePRI(x)

#define OS_ENTER_CRITICAL()						FitEnterCritical()
#define OS_EXIT_CRITICAL()						FitExitCritical()

#define FIT_QUICK_GET_PRIORITY	1
#define FitGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities ) uxTopPriority = ( 31 - __clz( ( uxReadyPriorities ) ) )


#ifndef FIT_FORCE_INLINE
	#define FIT_FORCE_INLINE __forceinline
#endif

/*-----------------------------------------------------------*/

static FIT_FORCE_INLINE void FitSetBasePRI( uOS32_t ulBASEPRI )
{
	__asm
	{
		/* Barrier instructions are not used as this function is only used to
		lower the BASEPRI value. */
		msr basepri, ulBASEPRI
	}
}
/*-----------------------------------------------------------*/

static FIT_FORCE_INLINE void FitRaiseBasePRI( void )
{
uOS32_t ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;

	__asm
	{
		/* Set BASEPRI to the max syscall priority to effect a critical
		section. */
		cpsid i
		msr basepri, ulNewBASEPRI
		dsb
		isb
		cpsie i
	}
}
/*-----------------------------------------------------------*/

static FIT_FORCE_INLINE uOS32_t FitRaiseBasePRIFromISR( void )
{
uOS32_t ulReturn, ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;

	__asm
	{
		/* Set BASEPRI to the max syscall priority to effect a critical
		section. */
		mrs ulReturn, basepri
		cpsid i
		msr basepri, ulNewBASEPRI
		dsb
		isb
		cpsie i
	}

	return ulReturn;
}

uOSStack_t *FitInitializeStack( uOSStack_t *pxTopOfStack,
		OSTaskFunction_t TaskFunction, void *pvParameters );

#ifdef __cplusplus
}
#endif

#endif //__FITCPU_HPP
