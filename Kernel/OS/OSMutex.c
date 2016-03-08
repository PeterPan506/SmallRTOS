/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

#include "SmallRTOS.h"
//#include "OSSemaphore.h"
//#include "OSMutex.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (OS_MUTEX_ON==1)

#define queueMUTEX_GIVE_BLOCK_TIME		 ( ( uOSTick_t ) 0U )

OSQHandle_t OSQCreateMutex( const uOS8_t ucQueueType )
{
	tOSQueue_t *ptNewQ;

	/* Allocate the new queue structure. */
	ptNewQ = ( tOSQueue_t * ) OSMemMalloc( sizeof( tOSQueue_t ) );
	if( ptNewQ != NULL )
	{
		/* Information required for priority inheritance. */
		ptNewQ->pcHead = NULL;
		ptNewQ->pxMutexHolder = NULL;//ptNewQ->pcTail = NULL;
		/* Queues used as a mutex no data is actually copied into or out
		of the queue. */
		ptNewQ->pcWriteTo = NULL;
		ptNewQ->u.pcReadFrom = NULL;

		/* Each mutex has a length of 1 (like a binary semaphore) and
		an item size of 0 as nothing is actually copied into or out
		of the mutex. */
		ptNewQ->uxCurNum = ( uOSBase_t ) 0U;
		ptNewQ->uxMaxNum = ( uOSBase_t ) 1U;
		ptNewQ->uxItemSize = ( uOSBase_t ) 0U;
		ptNewQ->xRxLock = OSQSTATE_UNLOCKED;
		ptNewQ->xTxLock = OSQSTATE_UNLOCKED;

		ptNewQ->ucQueueType = ucQueueType;

		/* Ensure the event queues start with the correct state. */
		OSListInitialise( &( ptNewQ->tTaskListSend ) );
		OSListInitialise( &( ptNewQ->tTaskListReceive ) );

		/* Start with the semaphore in the expected state. */
		( void ) OSQSend( ptNewQ, NULL, ( uOSTick_t ) 0U, OSQMODE_SEND_TO_BACK );
	}
	return ptNewQ;
}

OSMutexHandle_t OSMutexCreate( void )
{
	return OSQCreateMutex( OSQTYPE_MUTEX );
}

void OSMutexDelete( OSMutexHandle_t MutexHandle )
{
	OSQDelete( ( OSQHandle_t )MutexHandle );
}

sOSBase_t OSMutexLock( OSMutexHandle_t MutexHandle, uOSTick_t xTicksToWait)
{
	return OSSemPend( ( OSSemHandle_t )MutexHandle, xTicksToWait);
}

sOSBase_t OSMutexUnlock( OSMutexHandle_t MutexHandle)
{
	return OSSemPost( ( OSSemHandle_t )MutexHandle);
}

#endif //(OS_MUTEX_ON==1)

#ifdef __cplusplus
}
#endif
