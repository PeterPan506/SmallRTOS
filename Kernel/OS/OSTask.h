/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

#ifndef __OS_TASK_H
#define __OS_TASK_H

#include "OSType.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	eTaskStateRuning	,
	eTaskStateReady 	,
	eTaskStateSuspended	,
	eTaskStateBlocked	,
	eTaskStateDeath		,
	eTaskStateNum
}eOSTaskState_t;

#define OSTaskYield() 						FitYield()
#define OSTaskYieldFromISR(bYieldTask)		FitYIELD_FROM_ISR(bYieldTask)

#define OSStart()							OSStartScheduler()

#define SCHEDULER_SUSPENDED					( ( sOSBase_t ) 0 )
#define SCHEDULER_NOT_STARTED				( ( sOSBase_t ) 1 )
#define SCHEDULER_RUNNING					( ( sOSBase_t ) 2 )

/*
 * Task control block.  A task control block (TCB) is allocated for each task,
 * and stores task state information, including a pointer to the task's context
 * (the task's run time environment, including register values)
 */
typedef struct OSTaskControlBlock
{
	volatile uOSStack_t*	puxTopOfStack;	/*< Points to the location of the last item placed on the tasks stack.  THIS MUST BE THE FIRST MEMBER OF THE TCB STRUCT. */

	tOSListItem_t			tGenericListItem;	/*< The list that the state list item of a task is reference from denotes the state of that task (Ready, Blocked, Suspended ). */
	tOSListItem_t			tEventListItem;		/*< Used to reference a task from an event list. */
	uOSBase_t				uxPriority;			/*< The priority of the task.  0 is the lowest priority. */
	uOSStack_t*				puxStartStack;			/*< Points to the start of the stack. */
	char					pcTaskName[ OSNAME_MAX_LEN ];/*< Descriptive name given to the task when created.  Facilitates debugging only. */

#if ( OSSTACK_GROWTH > 0 )
	uOSBase_t*				puxEndOfStack;		/*< Points to the end of the stack on architectures where the stack grows up from low memory. */
#endif

#if ( portCRITICAL_NESTING_IN_TCB == 1 )
	uOSBase_t 				uxCriticalNesting; 	/*< Holds the critical section nesting depth for ports that do not maintain their own count in the port layer. */
#endif

#if ( configGENERATE_RUN_TIME_STATS == 1 )
	uOS32_t					ulRunTimeCounter;	/*< Stores the amount of time the task has spent in the Running state. */
#endif

#if ( OS_MUTEX_ON == 1 )
	uOSBase_t 				uxBasePriority;		/*< The priority last assigned to the task - used by the priority inheritance mechanism. */
	uOSBase_t 				uxMutexesHeld;
#endif

#if ( 0 )
	eOSTaskState_t			eTaskState;
#endif
} tOSTCB_t;

typedef	tOSTCB_t*		OSTaskHandle_t;

OSTaskHandle_t OSTaskCreate(OSTaskFunction_t	pxTaskFunction,
                            void*				pvParameter,
                            const uOS16_t 		usStackDepth,
                            uOSBase_t			uxPriority,
                            sOS8_t*				pcTaskName) SMALLRTOS_FUNCTION;
void 		OSTaskDelete( OSTaskHandle_t xTaskToDelete ) SMALLRTOS_FUNCTION;
uOSTick_t 	OSGetSystemTicksCount( void ) SMALLRTOS_FUNCTION;
uOS16_t 	OSStartScheduler( void ) SMALLRTOS_FUNCTION;
void 		OSTaskSleep( const uOSTick_t uxTicksToDelay ) SMALLRTOS_FUNCTION;

void 		OSTaskSuspendAll( void ) SMALLRTOS_FUNCTION;
sOSBase_t 	OSTaskResumeAll( void ) SMALLRTOS_FUNCTION;
void 		OSTaskAddToEventList( tOSList_t * const pxEventList, const uOSTick_t uxTicksToWait ) SMALLRTOS_FUNCTION;
sOSBase_t 	OSTaskDeleteFromEventList( const tOSList_t * const pxEventList ) SMALLRTOS_FUNCTION;
sOSBase_t 	OSTaskIncrementTick( void ) SMALLRTOS_FUNCTION;
void 		OSTaskMissedYield( void ) SMALLRTOS_FUNCTION;
void 		OSTaskSwitchContext( void ) SMALLRTOS_FUNCTION;
void 		OSTaskSetTimeOutState( tOSTimeOut_t * const ptTimeOut ) SMALLRTOS_FUNCTION;
sOSBase_t 	OSTaskCheckForTimeOut( tOSTimeOut_t * const ptTimeOut, uOSTick_t * const puxTicksToWait ) SMALLRTOS_FUNCTION;
OSTaskHandle_t OSTaskGetCurrentTaskHandle( void ) SMALLRTOS_FUNCTION;

void 		OSTaskSuspend( OSTaskHandle_t TaskHandle ) SMALLRTOS_FUNCTION;
void 		OSTaskResume( OSTaskHandle_t TaskHandle ) SMALLRTOS_FUNCTION;
sOSBase_t 	OSTaskResumeFromISR( OSTaskHandle_t TaskHandle ) SMALLRTOS_FUNCTION;

sOSBase_t 	OSTaskGetSchedulerState( void ) SMALLRTOS_FUNCTION;

#if ( OS_MUTEX_ON == 1 )
void *		OSTaskIncrementMutexHeldCount( void ) SMALLRTOS_FUNCTION;
void 		OSTaskPriorityInherit( OSTaskHandle_t const pxMutexHolder ) SMALLRTOS_FUNCTION;
sOSBase_t 	OSTaskPriorityDisinherit( OSTaskHandle_t const pxMutexHolder ) SMALLRTOS_FUNCTION;
#endif /* OS_MUTEX_ON */

#ifdef __cplusplus
}
#endif

#endif //__OS_TASK_H
