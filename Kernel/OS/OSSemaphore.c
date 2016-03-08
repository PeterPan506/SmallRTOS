/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

#include "SmallRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (OS_SEMAPHORE_ON==1)

/* Semaphores do not actually store or copy data, so have an item size of
zero. */
#define SEMAPHORE_QUEUE_ITEM_LENGTH ( ( uOSBase_t ) 0 )

OSSemHandle_t OSSemCreateCount( const uOSBase_t uxMaxCount, const uOSBase_t uxInitialCount )
{
	OSSemHandle_t xHandle;

	xHandle = OSQCreate( uxMaxCount, SEMAPHORE_QUEUE_ITEM_LENGTH, OSQTYPE_COUNTING_SEMAPHORE );

	if( xHandle != NULL )
	{
		( ( tOSQueue_t * ) xHandle )->uxCurNum = uxInitialCount;
	}

	return xHandle;
}

OSSemHandle_t OSSemCreate( void )
{
	return OSSemCreateCount( ( uOSBase_t )0xFF, ( uOSBase_t )0 );
}

void OSSemDelete(OSSemHandle_t SemHandle)
{
	OSQDelete( ( OSQHandle_t )SemHandle );
}

sOSBase_t OSSemPend( OSSemHandle_t SemHandle, uOSTick_t xTicksToWait)
{
	return OSQReceive( ( OSQHandle_t )SemHandle, NULL, xTicksToWait, FALSE );
}

sOSBase_t OSSemPendFromISR( OSSemHandle_t SemHandle)
{
	sOSBase_t sxRet = 0;
	sOSBase_t bYieldTask = FALSE;

	sxRet = OSQReceiveFromISR( ( OSQHandle_t )SemHandle, NULL, &bYieldTask );
	if(SCHEDULER_RUNNING == OSTaskGetSchedulerState())
	{
		OSTaskYieldFromISR( bYieldTask );
	}
	return sxRet;
}

sOSBase_t OSSemPost( OSSemHandle_t SemHandle)
{
	return OSQSend( ( OSQHandle_t )SemHandle, NULL, semGIVE_BLOCK_TIME, OSQMODE_SEND_TO_BACK );
}

sOSBase_t OSSemPostFromISR( OSSemHandle_t SemHandle)
{
	sOSBase_t sxRet = 0;
	sOSBase_t bYieldTask = FALSE;

	sxRet = OSQGiveFromISR( ( OSQHandle_t ) SemHandle, &bYieldTask );
	if(SCHEDULER_RUNNING == OSTaskGetSchedulerState())
	{
		OSTaskYieldFromISR( bYieldTask );
	}
	return sxRet;
}

#endif //(OS_SEMAPHORE_ON==1)

#ifdef __cplusplus
}
#endif
