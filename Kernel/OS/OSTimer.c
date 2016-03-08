/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

#include "SmallRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (OS_TIMER_ON==1)
	
SMALLRTOS_DATA static OSTimerHandle_t		gtOSTimerList	= NULL;
SMALLRTOS_DATA static volatile uOSBase_t	guxTimerTicks	= 0;

#if (OS_MSGQ_ON==1)
//////////////////SRCallbackFunction////////////////////////
SMALLRTOS_DATA static OSMsgQHandle_t 		gOSTimerMsgQHandle = NULL;
SMALLRTOS_DATA static OSTaskHandle_t		gOSTimerMoniteTaskHandle = NULL;

static void OSTimerMoniteTask( void *pvParameters)
{
	tOSTimerMsg_t TimerMsg;

	for( ;; )
	{
		if(TRUE==OSMsgQReceive(gOSTimerMsgQHandle, (void*)&TimerMsg, OSPEND_FOREVER_VALUE))
		{
			if(TimerMsg.pxTimerFunction!=NULL)
			{
				TimerMsg.pxTimerFunction(TimerMsg.pvParameter);
			}
		}
	}
}
#endif//#if (OS_MSGQ_ON==1)

OSTimerHandle_t OSTimerCreate(OSTimerFunction_t Function, void* pvParameter, uOSBase_t uxPeriodicTimeMS, sOS8_t* pcName)
{
	uOSBase_t x;
	OSTimerHandle_t timer;
	
	OS_ENTER_CRITICAL();
	if(uxPeriodicTimeMS==0)
	{
		return NULL;
	}
	timer = (OSTimerHandle_t)OSMemMalloc(sizeof(tOSTimer_t));
	if (timer == NULL) 
	{
		return NULL;
	}	

	// Store the timer name.
	for( x = ( uOSBase_t ) 0; x < ( uOSBase_t ) OSNAME_MAX_LEN; x++ )
	{
		timer->pcTimerName[ x ] = pcName[ x ];
		if( pcName[ x ] == 0x00 )
		{
			break;
		}
	}
	timer->pcTimerName[ OSNAME_MAX_LEN - 1 ] = '\0';
		
	timer->NextTimerHandle 	= NULL;
	timer->uxRelatTicks		= uxPeriodicTimeMS*OSTICKS_PER_MS;
	timer->uxPeriodicTime 	= uxPeriodicTimeMS;
	timer->bPeriod 			= TRUE;

	timer->tTimerMsg.pxTimerFunction = Function;
	timer->tTimerMsg.pvParameter		= pvParameter;

#if (OS_MSGQ_ON==1)
	if( gOSTimerMsgQHandle==NULL )
	{
		gOSTimerMsgQHandle = OSMsgQCreate(10, sizeof(tOSTimerMsg_t));
	}
	if( gOSTimerMoniteTaskHandle==NULL )
	{
		gOSTimerMoniteTaskHandle = OSTaskCreate(OSTimerMoniteTask, NULL, OSMINIMAL_STACK_SIZE, OSHIGHEAST_PRIORITY, "SRCbMsgTask" );
	}
#endif //#if (OS_MSGQ_ON==1)
	
	OS_EXIT_CRITICAL();
	
	return timer;
}

uOSBase_t OSTimerDelete(OSTimerHandle_t TimerHandle)
{
	OS_ENTER_CRITICAL();

	if(0==OSTimerDeleteFromList(TimerHandle))
	{
		OSMemFree(TimerHandle);
	}	
	
	OS_EXIT_CRITICAL();

	return 0;
}

uOSBase_t OSTimerAddToList(OSTimerHandle_t const TimerHandle)
{
	OSTimerHandle_t tempTimer;
	
	TimerHandle->uxRelatTicks	= TimerHandle->uxPeriodicTime*OSTICKS_PER_MS;
	//Add to list
	if (gtOSTimerList == NULL) 
	{
		gtOSTimerList = TimerHandle;
	}
	else
	{
		if (gtOSTimerList->uxRelatTicks > TimerHandle->uxRelatTicks) 
		{
			gtOSTimerList->uxRelatTicks -= TimerHandle->uxRelatTicks;
			TimerHandle->NextTimerHandle = gtOSTimerList;
			gtOSTimerList = TimerHandle;
		} 
		else 
		{
			for(tempTimer = gtOSTimerList; tempTimer != NULL; tempTimer = tempTimer->NextTimerHandle) 
			{
				TimerHandle->uxRelatTicks -= tempTimer->uxRelatTicks;
				if (tempTimer->NextTimerHandle == NULL || tempTimer->NextTimerHandle->uxRelatTicks > TimerHandle->uxRelatTicks) 
				{
					if (tempTimer->NextTimerHandle != NULL) 
					{
						tempTimer->NextTimerHandle->uxRelatTicks -= TimerHandle->uxRelatTicks;
					}
					TimerHandle->NextTimerHandle = tempTimer->NextTimerHandle;
					tempTimer->NextTimerHandle = TimerHandle;
					break;
				}
			}
		}	
	}
	
	return 0;
}

uOSBase_t OSTimerDeleteFromList(OSTimerHandle_t const TimerHandle)
{
	OSTimerHandle_t prevTimer;
    OSTimerHandle_t tempTimer;
	
	if (gtOSTimerList == NULL || TimerHandle==NULL) 
	{
		return 1;
	}

	for (tempTimer = gtOSTimerList, prevTimer = NULL; tempTimer != NULL; prevTimer = tempTimer, tempTimer = tempTimer->NextTimerHandle) 
	{
		if (TimerHandle==tempTimer)
		{
			/* We have a match */
			/* Unlink from previous in list */
			if (prevTimer == NULL) 
			{
				gtOSTimerList = tempTimer->NextTimerHandle;
			} 
			else 
			{
				prevTimer->NextTimerHandle = tempTimer->NextTimerHandle;
			}
			/* If not the last one, add time of this one back to next */
			if (tempTimer->NextTimerHandle != NULL) 
			{
				tempTimer->NextTimerHandle->uxRelatTicks += tempTimer->uxRelatTicks;
			}
			return 0;
		}
	}
	return 1;
}

void OSTimerIncrementTick( void )
{
	OSTimerHandle_t TimerHandle = NULL;
	OSTimerFunction_t TimerFunction = NULL;
	void* 	pParameter = NULL;

	( void )TimerFunction;
	( void )pParameter;

	// Process timer service
	if(gtOSTimerList!=NULL)
	{
		guxTimerTicks++;
		if(gtOSTimerList->uxRelatTicks<=guxTimerTicks)
		{
			guxTimerTicks = 0;
			
			gtOSTimerList->uxRelatTicks = 0;//update the ticks of timer
			TimerHandle = gtOSTimerList;
			OSTimerDeleteFromList(TimerHandle);
			
#if (OS_MSGQ_ON==1)
			OSMsgQSendFromISR(gOSTimerMsgQHandle, &(TimerHandle->tTimerMsg));
#else
			TimerFunction = TimerHandle->tTimerMsg.pxTimerFunction;
			pParameter = TimerHandle->tTimerMsg.pvParameter;
			if(TimerFunction!=NULL)
			{
				TimerFunction(pParameter);
			}
#endif //#if (OS_MSGQ_ON==1)

			if(TimerHandle->bPeriod>0)
			{
				OSTimerAddToList(TimerHandle);
			}
		}		
	}
}

uOSBase_t OSTimerStart(OSTimerHandle_t const TimerHandle)
{
	uOSBase_t ReturnValue = 0;
	
	OS_ENTER_CRITICAL();

	if(TimerHandle==NULL)
	{
		ReturnValue = 1;
	}
	else
	{
		OSTimerAddToList(TimerHandle);
	}
	OS_EXIT_CRITICAL();

	return ReturnValue;
}

uOSBase_t OSTimerStop(OSTimerHandle_t const TimerHandle)
{
	uOSBase_t ReturnValue = 0;
	
	OS_ENTER_CRITICAL();

	if(TimerHandle==NULL)
	{
		ReturnValue = 1;
	}
	else
	{	
		OSTimerDeleteFromList(TimerHandle);
	}

	OS_EXIT_CRITICAL();

	return ReturnValue;
}

#endif //(OS_TIMER_ON==1)

#ifdef __cplusplus
}
#endif

