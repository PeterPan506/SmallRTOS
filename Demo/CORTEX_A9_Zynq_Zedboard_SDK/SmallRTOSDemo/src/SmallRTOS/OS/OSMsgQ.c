/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

#include "SmallRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (OS_MSGQ_ON==1)

OSMsgQHandle_t OSMsgQCreate( const uOSBase_t uxQueueLength, const uOSBase_t uxItemSize)
{
	return OSQCreate( uxQueueLength, uxItemSize, OSQTYPE_BASE );
}

void OSMsgQDelete(OSMsgQHandle_t MsgQHandle)
{
	sOSBase_t uxMsgNumInQ;

	uxMsgNumInQ = OSQGetItemNum( MsgQHandle );
	(void)uxMsgNumInQ;

	OSQDelete( ( OSQHandle_t )MsgQHandle );
}

sOSBase_t OSMsgQSendToHead( OSMsgQHandle_t MsgQHandle, const void * const pvItemToQueue, uOSTick_t xTicksToWait)
{
	return OSQSend( ( OSQHandle_t )MsgQHandle, pvItemToQueue, xTicksToWait, OSQMODE_SEND_TO_FRONT );
}

sOSBase_t OSMsgQSendToTail( OSMsgQHandle_t MsgQHandle, const void * const pvItemToQueue, uOSTick_t xTicksToWait)
{
	return OSQSend( ( OSQHandle_t )MsgQHandle, pvItemToQueue, xTicksToWait, OSQMODE_SEND_TO_BACK );
}

sOSBase_t OSMsgQSend( OSMsgQHandle_t MsgQHandle, const void * const pvItemToQueue, uOSTick_t xTicksToWait)
{
	return OSQSend( ( OSQHandle_t )MsgQHandle, pvItemToQueue, xTicksToWait, OSQMODE_SEND_TO_BACK );
}

sOSBase_t OSMsgQOverwrite( OSMsgQHandle_t MsgQHandle, const void * const pvItemToQueue)
{
	return OSQSend( ( OSQHandle_t )MsgQHandle, pvItemToQueue, 0, OSQMODE_OVERWRITE );
}

sOSBase_t OSMsgQPeek( OSMsgQHandle_t MsgQHandle, void * const pvBuffer, uOSTick_t xTicksToWait)
{
	return OSQReceive( ( OSQHandle_t )MsgQHandle, pvBuffer, xTicksToWait, TRUE );
}

sOSBase_t OSMsgQReceive( OSMsgQHandle_t MsgQHandle, void * const pvBuffer, uOSTick_t xTicksToWait)
{
	return OSQReceive( ( OSQHandle_t )MsgQHandle, pvBuffer, xTicksToWait, FALSE );
}

sOSBase_t OSMsgQReceiveFromISR( OSMsgQHandle_t MsgQHandle, void * const pvBuffer)
{
	sOSBase_t sxRet = 0;
	sOSBase_t bYieldTask = FALSE;
	
	sxRet = OSQReceiveFromISR( ( OSQHandle_t )MsgQHandle, pvBuffer, &bYieldTask );
	if(SCHEDULER_RUNNING == OSTaskGetSchedulerState())
	{
		OSTaskYieldFromISR( bYieldTask );
	}

	return sxRet;
}

sOSBase_t OSMsgQSendToHeadFromISR( OSMsgQHandle_t MsgQHandle, const void * const pvItemToQueue)
{
	sOSBase_t sxRet = 0;
	sOSBase_t bYieldTask = FALSE;
	
	sxRet = OSQSendFromISR( ( OSQHandle_t )MsgQHandle, pvItemToQueue, &bYieldTask, OSQMODE_SEND_TO_FRONT );
	if(SCHEDULER_RUNNING == OSTaskGetSchedulerState())
	{
		OSTaskYieldFromISR( bYieldTask );
	}

	return sxRet;
}

sOSBase_t OSMsgQSendToTailFromISR( OSMsgQHandle_t MsgQHandle, const void * const pvItemToQueue)
{
	sOSBase_t sxRet = 0;
	sOSBase_t bYieldTask = FALSE;
	
	sxRet = OSQSendFromISR( ( OSQHandle_t )MsgQHandle, pvItemToQueue, &bYieldTask, OSQMODE_SEND_TO_BACK );
	if(SCHEDULER_RUNNING == OSTaskGetSchedulerState())
	{
		OSTaskYieldFromISR( bYieldTask );
	}

	return sxRet;
}

sOSBase_t OSMsgQOverwriteFromISR( OSMsgQHandle_t MsgQHandle, const void * const pvItemToQueue)
{
	sOSBase_t sxRet = 0;
	sOSBase_t bYieldTask = FALSE;
	
	sxRet = OSQSendFromISR( ( OSQHandle_t )MsgQHandle, pvItemToQueue, &bYieldTask, OSQMODE_OVERWRITE );
	if(SCHEDULER_RUNNING == OSTaskGetSchedulerState())
	{
		OSTaskYieldFromISR( bYieldTask );
	}

	return sxRet;
}

sOSBase_t OSMsgQSendFromISR( OSMsgQHandle_t MsgQHandle, const void * const pvItemToQueue)
{
	sOSBase_t sxRet = 0;
	sOSBase_t bYieldTask = FALSE;
	
	sxRet = OSQSendFromISR( ( OSQHandle_t )MsgQHandle, pvItemToQueue, &bYieldTask, OSQMODE_SEND_TO_BACK );
	if(SCHEDULER_RUNNING == OSTaskGetSchedulerState())
	{
		OSTaskYieldFromISR( bYieldTask );
	}

	return sxRet;
}

#endif //(OS_MSGQ_ON==1)

#ifdef __cplusplus
}
#endif
