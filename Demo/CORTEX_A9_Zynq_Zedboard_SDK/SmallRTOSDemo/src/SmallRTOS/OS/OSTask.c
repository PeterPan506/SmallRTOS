/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/
#include <string.h>
#include "SmallRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

#if configUSE_16_BIT_TICKS == 1
	#define taskEVENT_LIST_ITEM_VALUE_IN_USE	0x8000U
#else
	#define taskEVENT_LIST_ITEM_VALUE_IN_USE	0x80000000UL
#endif

SMALLRTOS_DATA tOSTCB_t * volatile gptCurrentTCB = NULL;

/* Lists for ready and blocked tasks. --------------------*/
SMALLRTOS_DATA static tOSList_t gtOSTaskListReadyArray[ OSHIGHEAST_PRIORITY+1 ];
SMALLRTOS_DATA static tOSList_t gtOSTaskListDelayed1;
SMALLRTOS_DATA static tOSList_t gtOSTaskListDelayed2;
SMALLRTOS_DATA static tOSList_t * volatile gptOSTaskListDelayed;
SMALLRTOS_DATA static tOSList_t * volatile gptOSTaskListOverflowDelayed;
SMALLRTOS_DATA static tOSList_t gtOSTaskListPendingReady;
//suspend task list
SMALLRTOS_DATA static tOSList_t gtOSTaskListSuspended;

// delete task
SMALLRTOS_DATA static tOSList_t gtOSTaskListdead;
SMALLRTOS_DATA static volatile 	uOSBase_t guxTasksDeleted 			= ( uOSBase_t ) 0U;

/* Other file private variables. --------------------------------*/
SMALLRTOS_DATA static volatile 	uOSBase_t guxCurrentNumberOfTasks 	= ( uOSBase_t ) 0U;
SMALLRTOS_DATA static volatile 	uOSTick_t guxTickCount 				= ( uOSTick_t ) 0U;
SMALLRTOS_DATA static volatile 	uOSBase_t guxTopReadyPriority 		= OSLOWEAST_PRIORITY;
SMALLRTOS_DATA static volatile 	sOSBase_t gxSchedulerRunning 		= FALSE;
SMALLRTOS_DATA static volatile 	uOSBase_t guxPendedTicks 			= ( uOSBase_t ) 0U;
SMALLRTOS_DATA static volatile 	sOSBase_t gxYieldPending 			= FALSE;
SMALLRTOS_DATA static volatile 	sOSBase_t gxNumOfOverflows 			= ( sOSBase_t ) 0;
SMALLRTOS_DATA static 			uOSBase_t guxOSTaskNumber 			= ( uOSBase_t ) 0U;
SMALLRTOS_DATA static volatile 	uOSTick_t guxNextTaskUnblockTime	= ( uOSTick_t ) 0U;
SMALLRTOS_DATA static volatile 	uOSBase_t guxSchedulerSuspended		= ( uOSBase_t ) FALSE;

static void OSTaskRecordReadyPriority(uOSBase_t uxPriority)
{
#if ( FIT_QUICK_GET_PRIORITY == 1 )
	guxTopReadyPriority |= ( 1UL << ( uxPriority ) ) ;
#endif
}

static void OSTaskResetReadyPriority(uOSBase_t uxPriority)
{
#if ( FIT_QUICK_GET_PRIORITY == 1 )
	if( OSListGetLength( &( gtOSTaskListReadyArray[ ( uxPriority ) ] ) ) == ( uOSBase_t ) 0 )
	{
		guxTopReadyPriority &= ~( 1UL << ( uxPriority ) );
	}
#endif
}

static uOSBase_t OSTaskFindHighestReadyPriority()
{
	uOSBase_t uxTopPriority = OSHIGHEAST_PRIORITY;

#if ( FIT_QUICK_GET_PRIORITY == 1 )
	FitGET_HIGHEST_PRIORITY( uxTopPriority, guxTopReadyPriority );
#else
	/* Find the highest priority queue that contains ready tasks. */
	while( OSListIsEmpty( &( gtOSTaskListReadyArray[ uxTopPriority ] ) ) )
	{
		--uxTopPriority;
	}
#endif
	return uxTopPriority;
}

static void* OSTaskGetTCBFromHandle(OSTaskHandle_t pxHandle)
{
	return ( ( ( pxHandle ) == NULL ) ? ( tOSTCB_t * ) gptCurrentTCB : ( tOSTCB_t * ) ( pxHandle ) );
}

static void OSTaskListsInitialise( void )
{
	uOSBase_t uxPriority;

	for( uxPriority = ( uOSBase_t ) 0U; uxPriority <= ( uOSBase_t ) OSHIGHEAST_PRIORITY; uxPriority++ )
	{
		OSListInitialise( &( gtOSTaskListReadyArray[ uxPriority ] ) );
	}

	OSListInitialise( &gtOSTaskListDelayed1 );
	OSListInitialise( &gtOSTaskListDelayed2 );
	OSListInitialise( &gtOSTaskListPendingReady );
	OSListInitialise( &gtOSTaskListdead );
	OSListInitialise( &gtOSTaskListSuspended );

	gptOSTaskListDelayed = &gtOSTaskListDelayed1;
	gptOSTaskListOverflowDelayed = &gtOSTaskListDelayed2;
}

static void OSTaskResetCurrentTask()
{
	uOSBase_t uxTopPriority;

	/* Find the highest priority queue that contains ready tasks. */
	uxTopPriority = OSTaskFindHighestReadyPriority();
	OSListGetNextItemTCB( &( gtOSTaskListReadyArray[ uxTopPriority ] ), gptCurrentTCB );
}

static void OSTaskUpdateUnblockTime( void )
{
	tOSTCB_t *ptTCB;

	if( OSListIsEmpty( gptOSTaskListDelayed ) != FALSE )
	{
		guxNextTaskUnblockTime = OSPEND_FOREVER_VALUE;
	}
	else
	{
		( ptTCB ) = ( tOSTCB_t * ) OSListGetHeadItemTCB( gptOSTaskListDelayed );
		guxNextTaskUnblockTime = OSListItemGetValue( &( ( ptTCB )->tGenericListItem ) );
	}
}

static void OSTaskSwitchListDelayed()
{
	tOSList_t *ptTempList;

	ptTempList = gptOSTaskListDelayed;
	gptOSTaskListDelayed = gptOSTaskListOverflowDelayed;
	gptOSTaskListOverflowDelayed = ptTempList;
	gxNumOfOverflows++;
	OSTaskUpdateUnblockTime();
}

static void OSTaskAddToReadyList(tOSTCB_t* ptTCB)
{
	OSTaskRecordReadyPriority( ( ptTCB )->uxPriority );
	OSListInsertItemToEnd( &( gtOSTaskListReadyArray[ ( ptTCB )->uxPriority ] ), &( ( ptTCB )->tGenericListItem ) );
}

static OSTaskHandle_t OSAllocateTCBAndStack( const uOS16_t usStackDepth, uOSStack_t *puxStackBuffer )
{
	OSTaskHandle_t ptNewTCB;
	ptNewTCB = ( OSTaskHandle_t ) OSMemMalloc( sizeof( tOSTCB_t ) );
	if( ptNewTCB != NULL )
	{
		ptNewTCB->puxStartStack = ( uOSStack_t * )OSMemMalloc( ( ( uOS16_t )usStackDepth ) * sizeof( uOSStack_t ));

		if( ptNewTCB->puxStartStack == NULL )
		{
			OSMemFree( ptNewTCB );
			ptNewTCB = NULL;
		}
		else
		{
			memset( (void*)ptNewTCB->puxStartStack, ( uOS32_t ) 0x5555AAAAU, ( uOS32_t ) usStackDepth * sizeof( uOSStack_t ) );
		}
	}

	return ptNewTCB;
}

static void OSTaskInitTCB( tOSTCB_t * const ptTCB, const char * const pcName, uOSBase_t uxPriority, const uOS16_t usStackDepth )
{
	uOSBase_t x;

	// Store the task name in the TCB.
	for( x = ( uOSBase_t ) 0; x < ( uOSBase_t ) OSNAME_MAX_LEN; x++ )
	{
		ptTCB->pcTaskName[ x ] = pcName[ x ];
		if( pcName[ x ] == 0x00 )
		{
			break;
		}
	}
	ptTCB->pcTaskName[ OSNAME_MAX_LEN - 1 ] = '\0';

	if( uxPriority > ( uOSBase_t ) OSHIGHEAST_PRIORITY )
	{
		uxPriority = ( uOSBase_t ) OSHIGHEAST_PRIORITY;
	}

	ptTCB->uxPriority = uxPriority;

	#if ( OS_MUTEX_ON == 1 )
	{
		ptTCB->uxBasePriority = uxPriority;
		ptTCB->uxMutexesHeld = 0;
	}
	#endif // OS_MUTEX_ON

	OSListItemInitialise( &( ptTCB->tGenericListItem ) );
	OSListItemInitialise( &( ptTCB->tEventListItem ) );

	OSListItemSetTCB( &( ptTCB->tGenericListItem ), ptTCB );

	/* Event lists are always in priority order. */
	OSListItemSetValue( &( ptTCB->tEventListItem ), ( uOSTick_t ) OSHIGHEAST_PRIORITY - ( uOSTick_t ) uxPriority + 1 );
	OSListItemSetTCB( &( ptTCB->tEventListItem ), ptTCB );

	#if ( portCRITICAL_NESTING_IN_TCB == 1 )
	{
		ptTCB->uxCriticalNesting = ( uOSBase_t ) 0U;
	}
	#endif /* portCRITICAL_NESTING_IN_TCB */


	#if ( configGENERATE_RUN_TIME_STATS == 1 )
	{
		ptTCB->ulRunTimeCounter = 0UL;
	}
	#endif /* configGENERATE_RUN_TIME_STATS */

	{
		( void ) usStackDepth;
	}

}

OSTaskHandle_t OSTaskCreate(OSTaskFunction_t	pxTaskFunction,
                            void*				pvParameter,
                            const uOS16_t 		usStackDepth,
                            uOSBase_t			uxPriority,
                            sOS8_t*				pcTaskName)
{
	sOSBase_t xStatus;
	OSTaskHandle_t ptNewTCB;
	uOSStack_t *puxTopOfStack;

	ptNewTCB = (tOSTCB_t * )OSAllocateTCBAndStack( usStackDepth, NULL );

	if( ptNewTCB != NULL )
	{
		#if( OSSTACK_GROWTH < 0 )
		{
			puxTopOfStack = ptNewTCB->puxStartStack + ( usStackDepth - ( uOS16_t ) 1 );
			puxTopOfStack = ( uOSStack_t * ) ( ( ( uOS32_t ) puxTopOfStack ) & ( ~( ( uOS32_t ) OSMEM_ALIGNMENT_MASK ) ) );
		}
		#else
		{
			puxTopOfStack = ptNewTCB->puxStartStack;
			ptNewTCB->puxEndOfStack = ptNewTCB->puxStartStack + ( usStackDepth - 1 );
		}
		#endif

		OSTaskInitTCB( ptNewTCB, pcTaskName, uxPriority, usStackDepth );
		ptNewTCB->puxTopOfStack = FitInitializeStack( puxTopOfStack, pxTaskFunction, pvParameter);
		
		OS_ENTER_CRITICAL();
		{
			guxCurrentNumberOfTasks++;
			if( gptCurrentTCB == NULL )
			{
				gptCurrentTCB =  ptNewTCB;

				if( guxCurrentNumberOfTasks == ( uOSBase_t ) 1 )
				{
					OSTaskListsInitialise();
				}
			}
			else
			{
				if( gxSchedulerRunning == FALSE )
				{
					if( gptCurrentTCB->uxPriority <= uxPriority )
					{
						gptCurrentTCB = ptNewTCB;
					}
				}
			}

			guxOSTaskNumber++;
			OSTaskAddToReadyList( ptNewTCB );

			xStatus = PASS;
		}
		OS_EXIT_CRITICAL();
	}
	else
	{
		xStatus = FAIL;
	}

	if( xStatus == PASS )
	{
		if( gxSchedulerRunning != FALSE )
		{
			if( gptCurrentTCB->uxPriority < uxPriority )
			{
				OSTaskYield();
			}
		}
	}
	return ptNewTCB;
}

void OSTaskDelete( OSTaskHandle_t xTaskToDelete )
{
	tOSTCB_t *ptTCB;

	OS_ENTER_CRITICAL();
	{
		ptTCB = OSTaskGetTCBFromHandle( xTaskToDelete );

		if( OSListRemoveItem( &( ptTCB->tGenericListItem ) ) == ( uOSBase_t ) 0 )
		{
			OSTaskResetReadyPriority( ptTCB->uxPriority );
		}

		if( OSListItemGetList( &( ptTCB->tEventListItem ) ) != NULL )
		{
			( void ) OSListRemoveItem( &( ptTCB->tEventListItem ) );
		}

		OSListInsertItemToEnd( &gtOSTaskListdead, &( ptTCB->tGenericListItem ) );

		++guxTasksDeleted;

		guxOSTaskNumber--;
	}
	OS_EXIT_CRITICAL();

	if( gxSchedulerRunning != FALSE )
	{
		if( ptTCB == gptCurrentTCB )
		{
			OSTaskYield();
		}
		else
		{
			OS_ENTER_CRITICAL();
			{
				OSTaskUpdateUnblockTime();
			}
			OS_EXIT_CRITICAL();
		}
	}
}

static void OSTaskDeleteFromDeadList( void )
{
	sOSBase_t xListIsEmpty;

	while( guxTasksDeleted > ( uOSBase_t ) 0U )
	{
		( void ) OSTaskSuspendAll();
		{
			xListIsEmpty = OSListIsEmpty( &gtOSTaskListdead );
		}
		( void ) OSTaskResumeAll();

		if( xListIsEmpty == FALSE )
		{
			tOSTCB_t *ptTCB;
			OS_ENTER_CRITICAL();
			{
				ptTCB = ( tOSTCB_t * ) OSListGetHeadItemTCB( ( &gtOSTaskListdead ) );
				( void ) OSListRemoveItem( &( ptTCB->tGenericListItem ) );
				--guxCurrentNumberOfTasks;
				--guxTasksDeleted;
			}
			OS_EXIT_CRITICAL();

			OSMemFree(ptTCB->puxStartStack);
		}
	}

}

static void OSTaskAddToDelayedList(tOSTCB_t* ptTCB, const uOSTick_t uxTimeToWake )
{
	if(ptTCB == NULL)
	{
		ptTCB = gptCurrentTCB;
	}
	OSListItemSetValue( &( ptTCB->tGenericListItem ), uxTimeToWake );

	if( uxTimeToWake < guxTickCount )
	{
		OSListInsertItem( gptOSTaskListOverflowDelayed, &( ptTCB->tGenericListItem ) );
	}
	else
	{
		OSListInsertItem( gptOSTaskListDelayed, &( ptTCB->tGenericListItem ) );

		if( uxTimeToWake < guxNextTaskUnblockTime )
		{
			guxNextTaskUnblockTime = uxTimeToWake;
		}
	}
}

void OSTaskAddToEventList( tOSList_t * const pxEventList, const uOSTick_t uxTicksToWait )
{
	uOSTick_t uxTimeToWake;

	OSListInsertItem( pxEventList, &( gptCurrentTCB->tEventListItem ) );

	if( OSListRemoveItem( &( gptCurrentTCB->tGenericListItem ) ) == ( uOSBase_t ) 0 )
	{
		OSTaskResetReadyPriority(gptCurrentTCB->uxPriority);
	}

	if( uxTicksToWait == OSPEND_FOREVER_VALUE )
	{
		OSListInsertItemToEnd( &gtOSTaskListSuspended, &( gptCurrentTCB->tGenericListItem ) );
	}
	else
	{
		uxTimeToWake = guxTickCount + uxTicksToWait;
		OSTaskAddToDelayedList( gptCurrentTCB, uxTimeToWake );
	}

}

sOSBase_t OSTaskDeleteFromEventList( const tOSList_t * const pxEventList )
{
	tOSTCB_t *pxUnblockedTCB;
	sOSBase_t xReturn;

	pxUnblockedTCB = ( tOSTCB_t * ) OSListGetHeadItemTCB( pxEventList );

	( void ) OSListRemoveItem( &( pxUnblockedTCB->tEventListItem ) );

	if( guxSchedulerSuspended == ( uOSBase_t ) FALSE )
	{
		( void ) OSListRemoveItem( &( pxUnblockedTCB->tGenericListItem ) );
		OSTaskAddToReadyList( pxUnblockedTCB );
	}
	else
	{
		OSListInsertItemToEnd( &( gtOSTaskListPendingReady ), &( pxUnblockedTCB->tEventListItem ) );
	}

	if( pxUnblockedTCB->uxPriority > gptCurrentTCB->uxPriority )
	{
		xReturn = TRUE;
		gxYieldPending = TRUE;
	}
	else
	{
		xReturn = FALSE;
	}

	return xReturn;
}

sOSBase_t OSTaskIncrementTick( void )
{
	tOSTCB_t * ptTCB;
	uOSTick_t uxItemValue;
	sOSBase_t xSwitchRequired = FALSE;

	if( guxSchedulerSuspended == ( sOSBase_t ) FALSE )
	{
		++guxTickCount;

		{
			const uOSTick_t uxConstTickCount = guxTickCount;

			if( uxConstTickCount == ( uOSTick_t ) 0U )
			{
				OSTaskSwitchListDelayed();
			}

			if( uxConstTickCount >= guxNextTaskUnblockTime )
			{
				for( ;; )
				{
					if( OSListIsEmpty( gptOSTaskListDelayed ) != FALSE )
					{
						guxNextTaskUnblockTime = OSPEND_FOREVER_VALUE;//portMAX_DELAY;
						break;
					}
					else
					{
						ptTCB = ( tOSTCB_t * ) OSListGetHeadItemTCB( gptOSTaskListDelayed );
						uxItemValue = OSListItemGetValue( &( ptTCB->tGenericListItem ) );

						if( uxConstTickCount < uxItemValue )
						{
							guxNextTaskUnblockTime = uxItemValue;
							break;
						}

						( void ) OSListRemoveItem( &( ptTCB->tGenericListItem ) );

						if( OSListItemGetList( &( ptTCB->tEventListItem ) ) != NULL )
						{
							( void ) OSListRemoveItem( &( ptTCB->tEventListItem ) );
						}

						OSTaskAddToReadyList( ptTCB );

						if( ptTCB->uxPriority >= gptCurrentTCB->uxPriority )
						{
							xSwitchRequired = TRUE;
						}
					}
				}
			}
		}
	}
	else
	{
		++guxPendedTicks;
	}

	if( gxYieldPending != FALSE )
	{
		xSwitchRequired = TRUE;
	}

	#if (OS_TIMER_ON==1)
	{
		OSTimerIncrementTick();
	}
	#endif /* (OS_TIMER_ON==1) */

	return xSwitchRequired;
}

uOSTick_t OSGetSystemTicksCount( void )
{
	uOSTick_t uxTicks;

	uxTicks = guxTickCount;

	return uxTicks;
}

OSTaskHandle_t OSTaskGetCurrentTaskHandle( void )
{
	OSTaskHandle_t xReturn;

	/* A critical section is not required as this is not called from
	an interrupt and the current TCB will always be the same for any
	individual execution thread. */
	xReturn = gptCurrentTCB;

	return xReturn;
}

void OSTaskSwitchContext( void )
{
	if( guxSchedulerSuspended != ( uOSBase_t ) FALSE )
	{
		gxYieldPending = TRUE;
	}
	else
	{
		gxYieldPending = FALSE;

		OSTaskResetCurrentTask();
	}
}


void OSTaskSuspendAll( void )
{
	++guxSchedulerSuspended;
}

sOSBase_t OSTaskResumeAll( void )
{
	tOSTCB_t *ptTCB;
	sOSBase_t xAlreadyYielded = FALSE;

	OS_ENTER_CRITICAL();
	{
		--guxSchedulerSuspended;

		if( guxSchedulerSuspended == ( uOSBase_t ) FALSE )
		{
			if( guxCurrentNumberOfTasks > ( uOSBase_t ) 0U )
			{
				while( OSListIsEmpty( &gtOSTaskListPendingReady ) == FALSE )
				{
					ptTCB = ( tOSTCB_t * ) OSListGetHeadItemTCB( ( &gtOSTaskListPendingReady ) );
					( void ) OSListRemoveItem( &( ptTCB->tEventListItem ) );
					( void ) OSListRemoveItem( &( ptTCB->tGenericListItem ) );
					OSTaskAddToReadyList( ptTCB );

					if( ptTCB->uxPriority >= gptCurrentTCB->uxPriority )
					{
						gxYieldPending = TRUE;
					}
				}

				if( guxPendedTicks > ( uOSBase_t ) 0U )
				{
					while( guxPendedTicks > ( uOSBase_t ) 0U )
					{
						if( OSTaskIncrementTick() != FALSE )
						{
							gxYieldPending = TRUE;
						}
						--guxPendedTicks;
					}
				}

				if( gxYieldPending == TRUE )
				{
					xAlreadyYielded = TRUE;
					OSTaskYield();
				}
			}
		}
	}
	OS_EXIT_CRITICAL();

	return xAlreadyYielded;
}

sOSBase_t OSTaskGetSchedulerState( void )
{
	sOSBase_t xReturn;

	if( gxSchedulerRunning == FALSE )
	{
		xReturn = SCHEDULER_NOT_STARTED;
	}
	else
	{
		if( guxSchedulerSuspended == FALSE )
		{
			xReturn = SCHEDULER_RUNNING;
		}
		else
		{
			xReturn = SCHEDULER_SUSPENDED;
		}
	}

	return xReturn;
}

void OSTaskSleep( const uOSTick_t uxTicksToDelay )
{
	uOSTick_t uxTimeToWake;
	uOSBase_t xAlreadyYielded = FALSE;

	if( uxTicksToDelay > ( uOSTick_t ) 0U )
	{
		OSTaskSuspendAll();
		{
			uxTimeToWake = guxTickCount + uxTicksToDelay;

			if( OSListRemoveItem( &( gptCurrentTCB->tGenericListItem ) ) == ( uOSBase_t ) 0 )
			{
				OSTaskResetReadyPriority(gptCurrentTCB->uxPriority);
			}

			OSTaskAddToDelayedList( gptCurrentTCB, uxTimeToWake );
		}
		xAlreadyYielded = OSTaskResumeAll();
	}

	if( xAlreadyYielded == FALSE )
	{
		OSTaskYield();
	}
}

void OSTaskSetTimeOutState( tOSTimeOut_t * const ptTimeOut )
{
	ptTimeOut->xOverflowCount = gxNumOfOverflows;
	ptTimeOut->uxTimeOnEntering = guxTickCount;
}

sOSBase_t OSTaskCheckForTimeOut( tOSTimeOut_t * const ptTimeOut, uOSTick_t * const puxTicksToWait )
{
	sOSBase_t xReturn;

	OS_ENTER_CRITICAL();
	{
		const uOSTick_t uxConstTickCount = guxTickCount;

		if( *puxTicksToWait == OSPEND_FOREVER_VALUE )
		{
			xReturn = FALSE;
		}
		else if( ( gxNumOfOverflows != ptTimeOut->xOverflowCount ) && ( uxConstTickCount >= ptTimeOut->uxTimeOnEntering ) )
		{
			xReturn = TRUE;
		}
		else if( ( uxConstTickCount - ptTimeOut->uxTimeOnEntering ) < *puxTicksToWait )
		{
			*puxTicksToWait -= ( uxConstTickCount -  ptTimeOut->uxTimeOnEntering );
			OSTaskSetTimeOutState( ptTimeOut );
			xReturn = FALSE;
		}
		else
		{
			xReturn = TRUE;
		}
	}
	OS_EXIT_CRITICAL();

	return xReturn;
}

void OSTaskMissedYield( void )
{
	gxYieldPending = TRUE;
}

eOSTaskState_t OSTaskGetState( OSTaskHandle_t TaskHandle )
{
	eOSTaskState_t eReturn;
	tOSList_t *ptStateList;
	const tOSTCB_t * const ptTCB = ( tOSTCB_t * ) TaskHandle;

	if( ptTCB == gptCurrentTCB )
	{
		eReturn = eTaskStateRuning;
	}
	else
	{
		OS_ENTER_CRITICAL();
		{
			ptStateList = ( tOSList_t * ) OSListItemGetList( &( ptTCB->tGenericListItem ) );
		}
		OS_EXIT_CRITICAL();

		if( ( ptStateList == gptOSTaskListDelayed ) || ( ptStateList == gptOSTaskListOverflowDelayed ) )
		{
			eReturn = eTaskStateBlocked;
		}
		else if( ptStateList == &gtOSTaskListSuspended )
		{
			if( OSListItemGetList( &( ptTCB->tEventListItem ) ) == NULL )
			{
				eReturn = eTaskStateSuspended;
			}
			else
			{
				eReturn = eTaskStateBlocked;
			}
		}

		else if( ptStateList == &gtOSTaskListdead )
		{
			eReturn = eTaskStateDeath;
		}
		else
		{
			eReturn = eTaskStateReady;
		}
	}

	return eReturn;
}

uOSBase_t OSTaskGetPriority( OSTaskHandle_t TaskHandle )
{
	tOSTCB_t *ptTCB;
	uOSBase_t uxReturn;

	OS_ENTER_CRITICAL();
	{
		/* If null is passed in here then it is the priority of the that
		called uxTaskPriorityGet() that is being queried. */
		ptTCB = OSTaskGetTCBFromHandle( TaskHandle );
		uxReturn = ptTCB->uxPriority;
	}
	OS_EXIT_CRITICAL();

	return uxReturn;
}

uOSBase_t OSTaskGetPriorityFromISR( OSTaskHandle_t TaskHandle )
{
	tOSTCB_t *ptTCB;
	uOSBase_t uxReturn, uxSavedInterruptState;

	uxSavedInterruptState = FitSET_INTERRUPT_MASK_FROM_ISR();
	{
		ptTCB = OSTaskGetTCBFromHandle( TaskHandle );
		uxReturn = ptTCB->uxPriority;
	}
	FitCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptState );

	return uxReturn;
}

void OSTaskSetPriority( OSTaskHandle_t TaskHandle, uOSBase_t uxNewPriority )
{
	tOSTCB_t *ptTCB;
	uOSBase_t uxCurrentBasePriority, uxPriorityUsedOnEntry;
	sOSBase_t xYieldRequired = FALSE;

	if( uxNewPriority > ( uOSBase_t ) OSHIGHEAST_PRIORITY )
	{
		uxNewPriority = ( uOSBase_t ) OSHIGHEAST_PRIORITY;
	}

	OS_ENTER_CRITICAL();
	{
		ptTCB = OSTaskGetTCBFromHandle( TaskHandle );

		#if ( OS_MUTEX_ON == 1 )
		{
			uxCurrentBasePriority = ptTCB->uxBasePriority;
		}
		#else
		{
			uxCurrentBasePriority = ptTCB->uxPriority;
		}
		#endif

		if( uxCurrentBasePriority != uxNewPriority )
		{
			if( uxNewPriority > uxCurrentBasePriority )
			{
				if( ptTCB != gptCurrentTCB )
				{
					if( uxNewPriority >= gptCurrentTCB->uxPriority )
					{
						xYieldRequired = TRUE;
					}
				}
			}
			else if( ptTCB == gptCurrentTCB )
			{
				xYieldRequired = TRUE;
			}

			uxPriorityUsedOnEntry = ptTCB->uxPriority;

			#if ( OS_MUTEX_ON == 1 )
			{
				if( ptTCB->uxBasePriority == ptTCB->uxPriority )
				{
					ptTCB->uxPriority = uxNewPriority;
				}

				ptTCB->uxBasePriority = uxNewPriority;
			}
			#else
			{
				ptTCB->uxPriority = uxNewPriority;
			}
			#endif

			if( ( OSListItemGetValue( &( ptTCB->tEventListItem ) ) & taskEVENT_LIST_ITEM_VALUE_IN_USE ) == 0UL )
			{
				OSListItemSetValue( &( ptTCB->tEventListItem ), ( ( uOSTick_t ) OSHIGHEAST_PRIORITY - ( uOSTick_t ) uxNewPriority + 1 ) );
			}

			if( OSListContainListItem( &( gtOSTaskListReadyArray[ uxPriorityUsedOnEntry ] ), &( ptTCB->tGenericListItem ) ) != FALSE )
			{
				if( OSListRemoveItem( &( ptTCB->tGenericListItem ) ) == ( uOSBase_t ) 0 )
				{
					OSTaskResetReadyPriority( uxPriorityUsedOnEntry );
				}

				OSTaskAddToReadyList( ptTCB );
			}

			if( xYieldRequired == TRUE )
			{
				OSTaskYield();
			}
		}
	}
	OS_EXIT_CRITICAL();
}

#if ( OS_MUTEX_ON == 1 )
void *OSTaskIncrementMutexHeldCount( void )
{
	if( gptCurrentTCB != NULL )
	{
		( gptCurrentTCB->uxMutexesHeld )++;
	}
	return gptCurrentTCB;
}

void OSTaskPriorityInherit( OSTaskHandle_t const pxMutexHolder )
{
	tOSTCB_t * const ptTCB = ( tOSTCB_t * ) pxMutexHolder;

	if( pxMutexHolder != NULL )
	{
		if( ptTCB->uxPriority < gptCurrentTCB->uxPriority )
		{
			if( ( OSListItemGetValue( &( ptTCB->tEventListItem ) ) & taskEVENT_LIST_ITEM_VALUE_IN_USE ) == 0UL )
			{
				OSListItemSetValue( &( ptTCB->tEventListItem ), ( uOSTick_t ) OSHIGHEAST_PRIORITY - ( uOSTick_t ) gptCurrentTCB->uxPriority + 1 );
			}

			if( OSListContainListItem( &( gtOSTaskListReadyArray[ ptTCB->uxPriority ] ), &( ptTCB->tGenericListItem ) ) != FALSE )
			{
				if( OSListRemoveItem( &( ptTCB->tGenericListItem ) ) == ( uOSBase_t ) 0 )
				{
					OSTaskResetReadyPriority( ptTCB->uxPriority );
				}

				ptTCB->uxPriority = gptCurrentTCB->uxPriority;
				OSTaskAddToReadyList( ptTCB );
			}
			else
			{
				/* Just inherit the priority. */
				ptTCB->uxPriority = gptCurrentTCB->uxPriority;
			}
		}
	}
}

sOSBase_t OSTaskPriorityDisinherit( OSTaskHandle_t const pxMutexHolder )
{
	tOSTCB_t * const ptTCB = ( tOSTCB_t * ) pxMutexHolder;
	sOSBase_t xReturn = FALSE;

	if( pxMutexHolder != NULL )
	{
		( ptTCB->uxMutexesHeld )--;

		if( ptTCB->uxPriority != ptTCB->uxBasePriority )
		{
			if( ptTCB->uxMutexesHeld == ( uOSBase_t ) 0 )
			{
				if( OSListRemoveItem( &( ptTCB->tGenericListItem ) ) == ( uOSBase_t ) 0 )
				{
					OSTaskResetReadyPriority( ptTCB->uxPriority );
				}

				ptTCB->uxPriority = ptTCB->uxBasePriority;

				OSListItemSetValue( &( ptTCB->tEventListItem ), ( uOSTick_t ) OSHIGHEAST_PRIORITY - ( uOSTick_t ) ptTCB->uxPriority + 1 );
				OSTaskAddToReadyList( ptTCB );

				xReturn = TRUE;
			}
		}
	}

	return xReturn;
}
#endif /* ( OS_MUTEX_ON == 1 ) */


void OSTaskSuspend( OSTaskHandle_t TaskHandle )
{
	tOSTCB_t *ptTCB;

	OS_ENTER_CRITICAL();
	{
		/* If TaskHandle is null then it is the running task that is
		being suspended. */
		ptTCB = OSTaskGetTCBFromHandle( TaskHandle );

		/* Remove task from the ready/delayed list */
		if( OSListRemoveItem( &( ptTCB->tGenericListItem ) ) == ( uOSBase_t ) 0 )
		{
			OSTaskResetReadyPriority( ptTCB->uxPriority );
		}

		/* Is the task waiting on an event list */
		if( OSListItemGetList( &( ptTCB->tEventListItem ) ) != NULL )
		{
			( void ) OSListRemoveItem( &( ptTCB->tEventListItem ) );
		}

		/* place the task in the suspended list. */
		OSListInsertItemToEnd( &gtOSTaskListSuspended, &( ptTCB->tGenericListItem ) );
	}
	OS_EXIT_CRITICAL();

	if( gxSchedulerRunning != FALSE )
	{
		/* Update the next expected unblock time */
		OS_ENTER_CRITICAL();
		{
			OSTaskUpdateUnblockTime();
		}
		OS_EXIT_CRITICAL();
	}

	if( ptTCB == gptCurrentTCB )
	{
		if( gxSchedulerRunning != FALSE )
		{
			OSTaskYield();
		}
		else
		{
			if( OSListGetLength( &gtOSTaskListSuspended ) == guxCurrentNumberOfTasks )
			{
				gptCurrentTCB = NULL;
			}
			else
			{
				OSTaskSwitchContext();
			}
		}
	}
}

static sOSBase_t OSTaskIsSuspended( const OSTaskHandle_t TaskHandle )
{
	sOSBase_t xReturn = FALSE;
	const tOSTCB_t * const ptTCB = ( tOSTCB_t * ) TaskHandle;

	/* Is the task being resumed actually in the suspended list? */
	if( OSListContainListItem( &gtOSTaskListSuspended, &( ptTCB->tGenericListItem ) ) != FALSE )
	{
		/* Has the task already been resumed from within an ISR? */
		if( OSListContainListItem( &gtOSTaskListPendingReady, &( ptTCB->tEventListItem ) ) == FALSE )
		{
			/* Is it in the suspended list because it is in the	Suspended
			state, or because is is blocked with no timeout? */
			if( OSListContainListItem( NULL, &( ptTCB->tEventListItem ) ) != FALSE )
			{
				xReturn = TRUE;
			}
		}
	}

	return xReturn;
}

void OSTaskResume( OSTaskHandle_t TaskHandle )
{
	tOSTCB_t * const ptTCB = ( tOSTCB_t * ) TaskHandle;

	if( ( ptTCB != NULL ) && ( ptTCB != gptCurrentTCB ) )
	{
		OS_ENTER_CRITICAL();
		{
			if( OSTaskIsSuspended( ptTCB ) != FALSE )
			{
				/* In a critical section we can access the ready lists. */
				( void ) OSListRemoveItem(  &( ptTCB->tGenericListItem ) );
				OSTaskAddToReadyList( ptTCB );

				if( ptTCB->uxPriority >= gptCurrentTCB->uxPriority )
				{
					OSTaskYield();
				}
			}
		}
		OS_EXIT_CRITICAL();
	}
}


sOSBase_t OSTaskResumeFromISR( OSTaskHandle_t TaskHandle )
{
	sOSBase_t xYieldRequired = FALSE;
	tOSTCB_t * const ptTCB = ( tOSTCB_t * ) TaskHandle;
	uOSBase_t uxSavedInterruptStatus;

	uxSavedInterruptStatus = FitSET_INTERRUPT_MASK_FROM_ISR();
	{
		if( OSTaskIsSuspended( ptTCB ) != FALSE )
		{
			/* Check the ready lists can be accessed. */
			if( guxSchedulerSuspended == ( uOSBase_t ) FALSE )
			{
				/* Ready lists can be accessed so move the task from the
				suspended list to the ready list directly. */
				if( ptTCB->uxPriority >= gptCurrentTCB->uxPriority )
				{
					xYieldRequired = TRUE;
				}

				( void ) OSListRemoveItem( &( ptTCB->tGenericListItem ) );
				OSTaskAddToReadyList( ptTCB );
			}
			else
			{
				/* The delayed or ready lists cannot be accessed so the task
				is held in the pending ready list until the scheduler is
				unsuspended. */
				OSListInsertItemToEnd( &( gtOSTaskListPendingReady ), &( ptTCB->tEventListItem ) );
			}
		}
	}
	FitCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );

	return xYieldRequired;
}


static void OSIdleTask( void *pvParameters)
{
	int i = 0;
	for( ;; )
	{
		// if there is not any other task ready, then OS enter idle task;
		 i += 1;
		 if(guxTasksDeleted > ( uOSBase_t ) 0U)
		 {
			 OSTaskDeleteFromDeadList();
		 }
	}
}

uOS16_t OSStartScheduler( void )
{
	uOS16_t ReturnValue = 0;
	OSTaskHandle_t TaskHandle = NULL;

	TaskHandle = OSTaskCreate(OSIdleTask, NULL, OSMINIMAL_STACK_SIZE, OSLOWEAST_PRIORITY, "OSIdleTask");
	if(TaskHandle != NULL)
	{
		guxNextTaskUnblockTime = OSPEND_FOREVER_VALUE;
		gxSchedulerRunning = TRUE;
		guxTickCount = ( uOSTick_t ) 0U;

		FitStartScheduler();
	}
	else
	{
		ReturnValue = 1;
	}

	// Should not get here!
	return ReturnValue;
}

#ifdef __cplusplus
}
#endif
