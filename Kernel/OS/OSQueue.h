/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

#ifndef __OS_QUEUE_H
#define __OS_QUEUE_H

#include "OSType.h"

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Definition of the queue used by the scheduler.
 * Items are queued by copy, not reference.
 */
typedef struct tOSQueue
{
	sOS8_t *					pcHead;	
	sOS8_t *					pcTail;	
	sOS8_t *					pcWriteTo;

	union					
	{
		sOS8_t *				pcReadFrom;
		uOSBase_t 				uxRecursiveCallCount;
	} u;

	char						pcQueueName[ OSNAME_MAX_LEN ];

	tOSList_t 					tTaskListSend;
	tOSList_t 					tTaskListReceive;

	volatile uOSBase_t 			uxCurNum;	
	uOSBase_t 					uxMaxNum;
	uOSBase_t 					uxItemSize;

	volatile sOSBase_t 			xRxLock;
	volatile sOSBase_t 			xTxLock;

	uOS8_t 						ucQueueType;

} tOSQueue_t;

typedef tOSQueue_t* 	OSQHandle_t;

/* For internal use only. */
#define	OSQMODE_SEND_TO_BACK			( ( sOSBase_t ) 0 )
#define	OSQMODE_SEND_TO_FRONT			( ( sOSBase_t ) 1 )
#define OSQMODE_OVERWRITE				( ( sOSBase_t ) 2 )

/* For internal use only.  These definitions *must* match those in queue.c. */
#define OSQTYPE_BASE					( ( uOS8_t ) 0U )
#define OSQTYPE_MUTEX 					( ( uOS8_t ) 1U )
#define OSQTYPE_COUNTING_SEMAPHORE		( ( uOS8_t ) 2U )
#define OSQTYPE_BINARY_SEMAPHORE		( ( uOS8_t ) 3U )
#define OSQTYPE_RECURSIVE_MUTEX			( ( uOS8_t ) 4U )

/* Constants used with the xRxLock and xTxLock structure members. */
#define OSQSTATE_UNLOCKED				( ( sOSBase_t ) -1 )
#define OSQSTATE_LOCKED					( ( sOSBase_t ) 0 )


OSQHandle_t OSQCreate( const uOSBase_t uxQueueLength, const uOSBase_t uxItemSize, const uOS8_t ucQueueType ) SMALLRTOS_FUNCTION;
void 		OSQDelete( OSQHandle_t QueueHandle ) SMALLRTOS_FUNCTION;
sOSBase_t 	OSQSend( OSQHandle_t QueueHandle, const void * const pvItemToQueue, uOSTick_t xTicksToWait, const sOSBase_t xCopyPosition ) SMALLRTOS_FUNCTION;
sOSBase_t 	OSQSendFromISR( OSQHandle_t QueueHandle, const void * const pvItemToQueue, sOSBase_t * const pbYieldTask, const sOSBase_t xCopyPosition ) SMALLRTOS_FUNCTION;
sOSBase_t 	OSQGiveFromISR( OSQHandle_t QueueHandle, sOSBase_t * const pbYieldTask ) SMALLRTOS_FUNCTION;
sOSBase_t 	OSQReceive( OSQHandle_t QueueHandle, void * const pvBuffer, uOSTick_t xTicksToWait, const sOSBase_t xJustPeek ) SMALLRTOS_FUNCTION;
sOSBase_t 	OSQReceiveFromISR( OSQHandle_t QueueHandle, void * const pvBuffer, sOSBase_t * const pbYieldTask ) SMALLRTOS_FUNCTION;
sOSBase_t 	OSQPeekFromISR( OSQHandle_t QueueHandle, void * const pvBuffer ) SMALLRTOS_FUNCTION;

uOSBase_t 	OSQGetSpaceNum( const OSQHandle_t QueueHandle ) SMALLRTOS_FUNCTION;
uOSBase_t 	OSQGetItemNum( const OSQHandle_t QueueHandle ) SMALLRTOS_FUNCTION;

sOSBase_t 	OSQReset( OSQHandle_t QueueHandle, sOSBase_t xNewQueue ) SMALLRTOS_FUNCTION;
uOS8_t 		OSQGetQueueType( OSQHandle_t QueueHandle ) SMALLRTOS_FUNCTION;

#ifdef __cplusplus
}
#endif

#endif /* __OS_QUEUE_H */

