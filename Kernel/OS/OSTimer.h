/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

#ifndef __OS_TIMER_H
#define __OS_TIMER_H

#include "OSType.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (OS_TIMER_ON==1)

typedef struct tTimerMsg
{
	OSTimerFunction_t		pxTimerFunction;	/* << The callback function to execute. */
	void *					pvParameter;		/* << The value that will be used as the callback functions first parameter. */
} tOSTimerMsg_t;

typedef struct tOSTimer
{
	struct tOSTimer *		NextTimerHandle;
	uOSBase_t				uxPeriodicTime;
	uOSBase_t				uxRelatTicks;					// Time;
	sOSBase_t				bPeriod;
	tOSTimerMsg_t			tTimerMsg;
	sOS8_t					pcTimerName[OSNAME_MAX_LEN];	// name of the timer
}tOSTimer_t;

typedef		tOSTimer_t*			OSTimerHandle_t;

OSTimerHandle_t OSTimerCreate(OSTimerFunction_t Function, void* pvParameter, uOSBase_t uxPeriodicTimeMS, sOS8_t* pcName) SMALLRTOS_FUNCTION;
uOSBase_t 		OSTimerDelete(OSTimerHandle_t TimerHandle) SMALLRTOS_FUNCTION;
uOSBase_t 		OSTimerAddToList(OSTimerHandle_t const TimerHandle) SMALLRTOS_FUNCTION;
uOSBase_t 		OSTimerDeleteFromList(OSTimerHandle_t const TimerHandle) SMALLRTOS_FUNCTION;
void 			OSTimerIncrementTick( void ) SMALLRTOS_FUNCTION;
uOSBase_t 		OSTimerStart(OSTimerHandle_t const TimerHandle) SMALLRTOS_FUNCTION;
uOSBase_t 		OSTimerStop(OSTimerHandle_t const TimerHandle) SMALLRTOS_FUNCTION;

#endif //(OS_TIMER_ON==1)

#ifdef __cplusplus
}
#endif

#endif //__SR_TIMER_HPP
