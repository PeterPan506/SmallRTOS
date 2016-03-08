/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

#ifndef OS_TYPE_H_
#define OS_TYPE_H_

#include "FitType.h"
#include "SmallRTOSConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SMALLRTOS_DATA
#define SMALLRTOS_FUNCTION

typedef void (*OSTaskFunction_t)( void * );
typedef void (*OSTimerFunction_t)(void * );
typedef void (*OSCallbackFunction_t)( void *, uOS32_t );

typedef struct xTIME_OUT
{
	sOSBase_t xOverflowCount;
	uOSTick_t uxTimeOnEntering;
} tOSTimeOut_t;

#ifndef 	NULL
#define 	NULL					( ( uOSBase_t ) 0 )
#endif

#ifndef 	FALSE
#define 	FALSE					( ( uOSBase_t ) 0 )
#endif
#ifndef 	TRUE
#define 	TRUE					( ( uOSBase_t ) 1 )
#endif

#define 	PASS					( TRUE )
#define 	FAIL					( FALSE )

#ifndef configTICK_RATE_HZ
  #define	OSTICK_RATE_HZ			( ( uOSTick_t )1000 )
#else
  #define	OSTICK_RATE_HZ			( ( uOSTick_t ) configTICK_RATE_HZ )
#endif

#define 	OSTICKS_PER_MS			( ( uOSTick_t ) OSTICK_RATE_HZ/1000 )

#ifndef FITSTACK_GROWTH
  #define	OSSTACK_GROWTH			( -1 )
#else
  #define	OSSTACK_GROWTH			FITSTACK_GROWTH
#endif

#ifndef FITBYTE_ALIGNMENT
  #define	OSMEM_ALIGNMENT			( 8 )
#else
  #define	OSMEM_ALIGNMENT			FITBYTE_ALIGNMENT
#endif
#define 	OSMEM_ALIGNMENT_MASK	( OSMEM_ALIGNMENT-1 )

// Priority range of the SmallRTOS 0~31
#ifndef configMAX_PRIORITIES
  #define	OSTASK_MAX_PRIORITY		8
#else
  #if (configMAX_PRIORITIES>31)
    #define 	OSTASK_MAX_PRIORITY		31
  #else
    #define	OSTASK_MAX_PRIORITY		configMAX_PRIORITIES
  #endif
#endif


#define 	OSLOWEAST_PRIORITY		0
#define		OSHIGHEAST_PRIORITY		(OSTASK_MAX_PRIORITY-1)

// The total heap size of the SmallRTOS
#ifndef configTOTAL_HEAP_SIZE
  #define	OSTOTAL_HEAP_SIZE		512
#else
  #define	OSTOTAL_HEAP_SIZE		configTOTAL_HEAP_SIZE
#endif

// Mini stack size of a task(Idle task or Monitor task)
#ifndef configMINIMAL_STACK_SIZE
  #define	OSMINIMAL_STACK_SIZE	32
#else
  #define	OSMINIMAL_STACK_SIZE	configMINIMAL_STACK_SIZE
#endif

// Length of name(eg. task name, semaphore name, MsgQ name, Mutex name)
#ifndef configMAX_NAME_LEN
  #define	OSNAME_MAX_LEN			10
#else
  #define	OSNAME_MAX_LEN			configMAX_NAME_LEN
#endif

// The value used as pend forever
#ifndef configPEND_FOREVER_VALUE
  #define	OSPEND_FOREVER_VALUE	( ( uOSTick_t ) 0xFFFFFFFFUL )
#else
  #define	OSPEND_FOREVER_VALUE	configPEND_FOREVER_VALUE
#endif

// Use semaphore or not
#ifndef configUSE_SEMAPHORE
  #define	OS_SEMAPHORE_ON			1
#else
  #define	OS_SEMAPHORE_ON			configUSE_SEMAPHORE
#endif

// Use message queue or not
#ifndef configUSE_MSGQ
  #define	OS_MSGQ_ON				1
#else
  #define	OS_MSGQ_ON				configUSE_MSGQ
#endif

// The max message num in a message queue
#ifndef configMSGQ_MAX_MSGNUM
  #define	OSMSGQ_MAX_MSGNUM		5
#else
  #define	OSMSGQ_MAX_MSGNUM		configMSGQ_MAX_MSGNUM
#endif


// Use mutex or not
#ifndef configUSE_MUTEX
  #define	OS_MUTEX_ON				1
#else
  #define	OS_MUTEX_ON				configUSE_MUTEX
#endif

// Use Timer or not
#ifndef configUSE_TIMER
  #define	OS_TIMER_ON				1
#else
  #define	OS_TIMER_ON				configUSE_TIMER
#endif

#if (OS_MSGQ_ON==1)
#ifndef configCALLBACK_TASK_PRIORITY
  #define	OSCALLBACK_TASK_PRIO	OSHIGHEAST_PRIORITY - 1
#else
  #define	OSCALLBACK_TASK_PRIO	configCALLBACK_TASK_PRIORITY
#endif
#endif //(OS_MSGQ_ON==1)

#ifdef __cplusplus
}
#endif

#endif /* OS_TYPE_H_ */
