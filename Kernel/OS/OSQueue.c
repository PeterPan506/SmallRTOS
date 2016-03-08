/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "SmallRTOS.h"
#include "OSTask.h"
#include "OSQueue.h"

static void OSQUnlock( tOSQueue_t * const ptQ ) SMALLRTOS_FUNCTION;
static sOSBase_t OSQIsEmpty( const tOSQueue_t *ptQ ) SMALLRTOS_FUNCTION;
static sOSBase_t OSQIsFull( const tOSQueue_t *ptQ ) SMALLRTOS_FUNCTION;
static sOSBase_t OSQCopyDataIn( tOSQueue_t * const ptQ, const void *pvItemToQueue, const sOSBase_t xPosition ) SMALLRTOS_FUNCTION;
static void OSQCopyDataOut( tOSQueue_t * const ptQ, void * const pvBuffer ) SMALLRTOS_FUNCTION;

#define OSQLock( ptQ )										\
	OS_ENTER_CRITICAL();									\
	{														\
		if( ( ptQ )->xRxLock == OSQSTATE_UNLOCKED )			\
		{													\
			( ptQ )->xRxLock = OSQSTATE_LOCKED;				\
		}													\
		if( ( ptQ )->xTxLock == OSQSTATE_UNLOCKED )			\
		{													\
			( ptQ )->xTxLock = OSQSTATE_LOCKED;				\
		}													\
	}														\
	OS_EXIT_CRITICAL()

sOSBase_t OSQReset( OSQHandle_t QueueHandle, sOSBase_t xNewQueue )
{
	tOSQueue_t * const ptQ = ( tOSQueue_t * ) QueueHandle;

	OS_ENTER_CRITICAL();
	{
		ptQ->pcTail = ptQ->pcHead + ( ptQ->uxMaxNum * ptQ->uxItemSize );
		ptQ->uxCurNum = ( uOSBase_t ) 0U;
		ptQ->pcWriteTo = ptQ->pcHead;
		ptQ->u.pcReadFrom = ptQ->pcHead + ( ( ptQ->uxMaxNum - ( uOSBase_t ) 1U ) * ptQ->uxItemSize );
		ptQ->xRxLock = OSQSTATE_UNLOCKED;
		ptQ->xTxLock = OSQSTATE_UNLOCKED;

		if( xNewQueue == FALSE )
		{
			if( OSListIsEmpty( &( ptQ->tTaskListSend ) ) == FALSE )
			{
				if( OSTaskDeleteFromEventList( &( ptQ->tTaskListSend ) ) == TRUE )
				{
					OSTaskYield();
				}
			}
		}
		else
		{
			OSListInitialise( &( ptQ->tTaskListSend ) );
			OSListInitialise( &( ptQ->tTaskListReceive ) );
		}
	}
	OS_EXIT_CRITICAL();

	return PASS;
}

OSQHandle_t OSQCreate( const uOSBase_t uxQueueLength, const uOSBase_t uxItemSize, const uOS8_t ucQueueType )
{
	tOSQueue_t *ptNewQ;
	size_t xQSizeInBytes;
	OSQHandle_t xReturn = NULL;

	( void ) ucQueueType;

	if( uxItemSize == ( uOSBase_t ) 0 )
	{
		xQSizeInBytes = ( size_t ) 0;
	}
	else
	{
		xQSizeInBytes = ( size_t ) ( uxQueueLength * uxItemSize ) + ( size_t ) 1;
	}

	ptNewQ = ( tOSQueue_t * ) OSMemMalloc( sizeof( tOSQueue_t ) + xQSizeInBytes );

	if( ptNewQ != NULL )
	{
		if( uxItemSize == ( uOSBase_t ) 0 )
		{
			ptNewQ->pcHead = ( sOS8_t * ) ptNewQ;
		}
		else
		{
			ptNewQ->pcHead = ( ( sOS8_t * ) ptNewQ ) + sizeof( tOSQueue_t );
		}

		ptNewQ->uxMaxNum = uxQueueLength;
		ptNewQ->uxItemSize = uxItemSize;
		( void ) OSQReset( ptNewQ, TRUE );

		ptNewQ->ucQueueType = ucQueueType;

		xReturn = ptNewQ;
	}

	return xReturn;
}

sOSBase_t OSQSend( OSQHandle_t QueueHandle, const void * const pvItemToQueue, uOSTick_t xTicksToWait, const sOSBase_t xCopyPosition )
{
	sOSBase_t xEntryTimeSet = FALSE, xYieldRequired;
	tOSTimeOut_t tTimeOut;
	tOSQueue_t * const ptQ = ( tOSQueue_t * ) QueueHandle;

	for( ;; )
	{
		OS_ENTER_CRITICAL();
		{
			if( ( ptQ->uxCurNum < ptQ->uxMaxNum ) || ( xCopyPosition == OSQMODE_OVERWRITE ) )
			{
				xYieldRequired = OSQCopyDataIn( ptQ, pvItemToQueue, xCopyPosition );

				if( OSListIsEmpty( &( ptQ->tTaskListReceive ) ) == FALSE )
				{
					if( OSTaskDeleteFromEventList( &( ptQ->tTaskListReceive ) ) == TRUE )
					{
						OSTaskYield();
					}
				}
				else if( xYieldRequired != FALSE )
				{
					OSTaskYield();
				}

				OS_EXIT_CRITICAL();
				return PASS;
			}
			else
			{
				if( xTicksToWait == ( uOSTick_t ) 0 )
				{
					OS_EXIT_CRITICAL();
					//the queue if full
					return FAIL;
				}
				else if( xEntryTimeSet == FALSE )
				{
					OSTaskSetTimeOutState( &tTimeOut );
					xEntryTimeSet = TRUE;
				}
			}
		}
		OS_EXIT_CRITICAL();

		OSTaskSuspendAll();
		OSQLock( ptQ );

		if( OSTaskCheckForTimeOut( &tTimeOut, &xTicksToWait ) == FALSE )
		{
			if( OSQIsFull( ptQ ) != FALSE )
			{
				OSTaskAddToEventList( &( ptQ->tTaskListSend ), xTicksToWait );

				OSQUnlock( ptQ );

				if( OSTaskResumeAll() == FALSE )
				{
					OSTaskYield();
				}
			}
			else
			{
				/* Try again. */
				OSQUnlock( ptQ );
				( void ) OSTaskResumeAll();
			}
		}
		else
		{
			/* The timeout has expired. */
			OSQUnlock( ptQ );
			( void ) OSTaskResumeAll();
			//the queue if full
			return FAIL;
		}
	}
}

sOSBase_t OSQSendFromISR( OSQHandle_t QueueHandle, const void * const pvItemToQueue, sOSBase_t * const pbYieldTask, const sOSBase_t xCopyPosition )
{
	sOSBase_t xReturn;
	uOSBase_t uxSavedInterruptStatus;
	tOSQueue_t * const ptQ = ( tOSQueue_t * ) QueueHandle;

	uxSavedInterruptStatus = FitSET_INTERRUPT_MASK_FROM_ISR();
	{
		if( ( ptQ->uxCurNum < ptQ->uxMaxNum ) || ( xCopyPosition == OSQMODE_OVERWRITE ) )
		{
			/* Semaphores use OSQGiveFromISR(), so QueueHandle will not be a
			semaphore or mutex.  That means OSQCopyDataIn() cannot result
			in a task disinheriting a priority and OSQCopyDataIn() can be
			called here even though the disinherit function does not check if
			the scheduler is suspended before accessing the ready lists. */		
			( void ) OSQCopyDataIn( ptQ, pvItemToQueue, xCopyPosition );

			if( ptQ->xTxLock == OSQSTATE_UNLOCKED )
			{
				if( OSListIsEmpty( &( ptQ->tTaskListReceive ) ) == FALSE )
				{
					if( OSTaskDeleteFromEventList( &( ptQ->tTaskListReceive ) ) != FALSE )
					{
						if( pbYieldTask != NULL )
						{
							*pbYieldTask = TRUE;
						}

					}
				}
			}
			else
			{
				++( ptQ->xTxLock );
			}

			xReturn = PASS;
		}
		else
		{
			//the queue if full
			xReturn = FAIL;
		}
	}
	FitCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );

	return xReturn;
}

sOSBase_t OSQGiveFromISR( OSQHandle_t QueueHandle, sOSBase_t * const pbYieldTask )
{
	sOSBase_t xReturn;
	uOSBase_t uxSavedInterruptStatus;
	tOSQueue_t * const ptQ = ( tOSQueue_t * ) QueueHandle;

	uxSavedInterruptStatus = FitSET_INTERRUPT_MASK_FROM_ISR();
	{
		if( ptQ->uxCurNum < ptQ->uxMaxNum )
		{
			/* A task can only have an inherited priority if it is a mutex
			holder - and if there is a mutex holder then the mutex cannot be
			given from an ISR.  As this is the ISR version of the function it
			can be assumed there is no mutex holder and no need to determine if
			priority disinheritance is needed.  Simply increase the count of
			messages (semaphores) available. */		
			++( ptQ->uxCurNum );

			if( ptQ->xTxLock == OSQSTATE_UNLOCKED )
			{
				if( OSListIsEmpty( &( ptQ->tTaskListReceive ) ) == FALSE )
				{
					if( OSTaskDeleteFromEventList( &( ptQ->tTaskListReceive ) ) != FALSE )
					{
						if( pbYieldTask != NULL )
						{
							*pbYieldTask = TRUE;
						}
					}
				}
			}
			else
			{
				++( ptQ->xTxLock );
			}

			xReturn = PASS;
		}
		else
		{
			//the queue if full
			xReturn = FAIL;
		}
	}
	FitCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );

	return xReturn;
}

sOSBase_t OSQReceive( OSQHandle_t QueueHandle, void * const pvBuffer, uOSTick_t xTicksToWait, const sOSBase_t xJustPeeking )
{
	sOSBase_t xEntryTimeSet = FALSE;
	tOSTimeOut_t tTimeOut;
	sOS8_t *pcOriginalReadPosition;
	tOSQueue_t * const ptQ = ( tOSQueue_t * ) QueueHandle;

	for( ;; )
	{
		OS_ENTER_CRITICAL();
		{
			if( ptQ->uxCurNum > ( uOSBase_t ) 0 )
			{
				pcOriginalReadPosition = ptQ->u.pcReadFrom;

				OSQCopyDataOut( ptQ, pvBuffer );

				if( xJustPeeking == FALSE )
				{
					--( ptQ->uxCurNum );

					#if ( OS_MUTEX_ON == 1 )
					{
						if( ptQ->ucQueueType == OSQTYPE_MUTEX )
						{
							ptQ->pxMutexHolder = ( sOS8_t * ) OSTaskIncrementMutexHeldCount();
						}
					}
					#endif /* OS_MUTEX_ON */

					if( OSListIsEmpty( &( ptQ->tTaskListSend ) ) == FALSE )
					{
						if( OSTaskDeleteFromEventList( &( ptQ->tTaskListSend ) ) == TRUE )
						{
							OSTaskYield();
						}
					}
				}
				else
				{
					ptQ->u.pcReadFrom = pcOriginalReadPosition;

					if( OSListIsEmpty( &( ptQ->tTaskListReceive ) ) == FALSE )
					{
						if( OSTaskDeleteFromEventList( &( ptQ->tTaskListReceive ) ) != FALSE )
						{
							OSTaskYield();
						}
					}
				}

				OS_EXIT_CRITICAL();
				return PASS;
			}
			else
			{
				if( xTicksToWait == ( uOSTick_t ) 0 )
				{
					OS_EXIT_CRITICAL();
					//the queue is empty
					return FAIL;
				}
				else if( xEntryTimeSet == FALSE )
				{
					OSTaskSetTimeOutState( &tTimeOut );
					xEntryTimeSet = TRUE;
				}
			}
		}
		OS_EXIT_CRITICAL();

		OSTaskSuspendAll();
		OSQLock( ptQ );

		if( OSTaskCheckForTimeOut( &tTimeOut, &xTicksToWait ) == FALSE )
		{
			if( OSQIsEmpty( ptQ ) != FALSE )
			{
				#if ( OS_MUTEX_ON == 1 )
				{
					if( ptQ->ucQueueType == OSQTYPE_MUTEX )
					{
						OS_ENTER_CRITICAL();
						{
							OSTaskPriorityInherit( ( void * ) ptQ->pxMutexHolder );
						}
						OS_EXIT_CRITICAL();
					}
				}
				#endif

				OSTaskAddToEventList( &( ptQ->tTaskListReceive ), xTicksToWait );
				OSQUnlock( ptQ );
				if( OSTaskResumeAll() == FALSE )
				{
					OSTaskYield();
				}
			}
			else
			{
				/* Try again. */
				OSQUnlock( ptQ );
				( void ) OSTaskResumeAll();
			}
		}
		else
		{
			OSQUnlock( ptQ );
			( void ) OSTaskResumeAll();
			//the queue is empty
			return FAIL;
		}
	}
}

sOSBase_t OSQReceiveFromISR( OSQHandle_t QueueHandle, void * const pvBuffer, sOSBase_t * const pbYieldTask )
{
	sOSBase_t xReturn;
	uOSBase_t uxSavedInterruptStatus;
	tOSQueue_t * const ptQ = ( tOSQueue_t * ) QueueHandle;

	uxSavedInterruptStatus = FitSET_INTERRUPT_MASK_FROM_ISR();
	{
		if( ptQ->uxCurNum > ( uOSBase_t ) 0 )
		{
			OSQCopyDataOut( ptQ, pvBuffer );
			--( ptQ->uxCurNum );

			if( ptQ->xRxLock == OSQSTATE_UNLOCKED )
			{
				if( OSListIsEmpty( &( ptQ->tTaskListSend ) ) == FALSE )
				{
					if( OSTaskDeleteFromEventList( &( ptQ->tTaskListSend ) ) != FALSE )
					{
						if( pbYieldTask != NULL )
						{
							*pbYieldTask = TRUE;
						}
					}
				}
			}
			else
			{
				++( ptQ->xRxLock );
			}
			xReturn = PASS;
		}
		else
		{
			xReturn = FAIL;
		}
	}
	FitCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );

	return xReturn;
}

sOSBase_t OSQPeekFromISR( OSQHandle_t QueueHandle,  void * const pvBuffer )
{
	sOSBase_t xReturn;
	uOSBase_t uxSavedInterruptStatus;
	sOS8_t *pcOriginalReadPosition;
	tOSQueue_t * const ptQ = ( tOSQueue_t * ) QueueHandle;

	uxSavedInterruptStatus = FitSET_INTERRUPT_MASK_FROM_ISR();
	{
		if( ptQ->uxCurNum > ( uOSBase_t ) 0 )
		{
			pcOriginalReadPosition = ptQ->u.pcReadFrom;
			OSQCopyDataOut( ptQ, pvBuffer );
			ptQ->u.pcReadFrom = pcOriginalReadPosition;

			xReturn = PASS;
		}
		else
		{
			xReturn = FAIL;
		}
	}
	FitCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );

	return xReturn;
}

uOSBase_t OSQGetSpaceNum( const OSQHandle_t QueueHandle )
{
	uOSBase_t uxReturn;
	tOSQueue_t *ptQ;

	ptQ = ( tOSQueue_t * ) QueueHandle;

	OS_ENTER_CRITICAL();
	{
		uxReturn = ptQ->uxMaxNum - ptQ->uxCurNum;
	}
	OS_EXIT_CRITICAL();

	return uxReturn;
}

uOSBase_t OSQGetItemNum( const OSQHandle_t QueueHandle )
{
	uOSBase_t uxReturn;

	OS_ENTER_CRITICAL();
	{
		uxReturn = ( ( tOSQueue_t * ) QueueHandle )->uxCurNum;
	}
	OS_EXIT_CRITICAL();

	return uxReturn;
}

void OSQDelete( OSQHandle_t QueueHandle )
{
	tOSQueue_t * const ptQ = ( tOSQueue_t * ) QueueHandle;

	OSMemFree( ptQ );
}

uOS8_t OSQGetQueueType( OSQHandle_t QueueHandle )
{
	return ( ( tOSQueue_t * ) QueueHandle )->ucQueueType;
}

static sOSBase_t OSQCopyDataIn( tOSQueue_t * const ptQ, const void *pvItemToQueue, const sOSBase_t xPosition )
{
	sOSBase_t xReturn = FALSE;

	if( ptQ->uxItemSize == ( uOSBase_t ) 0 )
	{
		#if ( OS_MUTEX_ON == 1 )
		{
			if( ptQ->ucQueueType == OSQTYPE_MUTEX )
			{
				/* The mutex is no longer being held. */
				xReturn = OSTaskPriorityDisinherit( ( void * ) ptQ->pxMutexHolder );
				ptQ->pxMutexHolder = NULL;
			}
		}
		#endif /* OS_MUTEX_ON */
	}
	else if( xPosition == OSQMODE_SEND_TO_BACK )
	{
		( void ) memcpy( ( void * ) ptQ->pcWriteTo, pvItemToQueue, ( size_t ) ptQ->uxItemSize );
		ptQ->pcWriteTo += ptQ->uxItemSize;
		if( ptQ->pcWriteTo >= ptQ->pcTail )
		{
			ptQ->pcWriteTo = ptQ->pcHead;
		}
	}
	else
	{
		( void ) memcpy( ( void * ) ptQ->u.pcReadFrom, pvItemToQueue, ( size_t ) ptQ->uxItemSize );
		ptQ->u.pcReadFrom -= ptQ->uxItemSize;
		if( ptQ->u.pcReadFrom < ptQ->pcHead )
		{
			ptQ->u.pcReadFrom = ( ptQ->pcTail - ptQ->uxItemSize );
		}


		if( xPosition == OSQMODE_OVERWRITE )
		{
			if( ptQ->uxCurNum > ( uOSBase_t ) 0 )
			{
				--( ptQ->uxCurNum );
			}
		}
	}

	++( ptQ->uxCurNum );

	return xReturn;
}

static void OSQCopyDataOut( tOSQueue_t * const ptQ, void * const pvBuffer )
{
	if( ptQ->uxItemSize != ( uOSBase_t ) 0 )
	{
		ptQ->u.pcReadFrom += ptQ->uxItemSize;
		if( ptQ->u.pcReadFrom >= ptQ->pcTail )
		{
			ptQ->u.pcReadFrom = ptQ->pcHead;
		}

		( void ) memcpy( ( void * ) pvBuffer, ( void * ) ptQ->u.pcReadFrom, ( size_t ) ptQ->uxItemSize );
	}
}

static void OSQUnlock( tOSQueue_t * const ptQ )
{
	/* THIS FUNCTION MUST BE CALLED WITH THE SCHEDULER SUSPENDED. */

	OS_ENTER_CRITICAL();
	{
		while( ptQ->xTxLock > OSQSTATE_LOCKED )
		{
			if( OSListIsEmpty( &( ptQ->tTaskListReceive ) ) == FALSE )
			{
				if( OSTaskDeleteFromEventList( &( ptQ->tTaskListReceive ) ) != FALSE )
				{
					OSTaskMissedYield();
				}
			}
			else
			{
				break;
			}

			--( ptQ->xTxLock );
		}

		ptQ->xTxLock = OSQSTATE_UNLOCKED;
	}
	OS_EXIT_CRITICAL();

	/* Do the same for the Rx lock. */
	OS_ENTER_CRITICAL();
	{
		while( ptQ->xRxLock > OSQSTATE_LOCKED )
		{
			if( OSListIsEmpty( &( ptQ->tTaskListSend ) ) == FALSE )
			{
				if( OSTaskDeleteFromEventList( &( ptQ->tTaskListSend ) ) != FALSE )
				{
					OSTaskMissedYield();
				}

				--( ptQ->xRxLock );
			}
			else
			{
				break;
			}
		}

		ptQ->xRxLock = OSQSTATE_UNLOCKED;
	}
	OS_EXIT_CRITICAL();
}

static sOSBase_t OSQIsEmpty( const tOSQueue_t *ptQ )
{
	sOSBase_t xReturn;

	OS_ENTER_CRITICAL();
	{
		if( ptQ->uxCurNum == ( uOSBase_t )  0 )
		{
			xReturn = TRUE;
		}
		else
		{
			xReturn = FALSE;
		}
	}
	OS_EXIT_CRITICAL();

	return xReturn;
}

static sOSBase_t OSQIsFull( const tOSQueue_t *ptQ )
{
sOSBase_t xReturn;

	OS_ENTER_CRITICAL();
	{
		if( ptQ->uxCurNum == ptQ->uxMaxNum )
		{
			xReturn = TRUE;
		}
		else
		{
			xReturn = FALSE;
		}
	}
	OS_EXIT_CRITICAL();

	return xReturn;
}








