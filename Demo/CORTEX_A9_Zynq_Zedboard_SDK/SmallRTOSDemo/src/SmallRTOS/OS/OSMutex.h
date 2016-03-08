/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

#ifndef __OS_MUTEX_H
#define __OS_MUTEX_H

#include "OSType.h"
#include "OSQueue.h"


#ifdef __cplusplus
extern "C" {
#endif

#if (OS_MUTEX_ON==1)

/* When the tOSQueue_t structure is used to represent a base queue its pcHead and
pcTail members are used as pointers into the queue storage area.  When the
tOSQueue_t structure is used to represent a mutex pcHead and pcTail pointers are
not necessary, and the pcHead pointer is set to NULL to indicate that the
pcTail pointer actually points to the mutex holder (if any).  Map alternative
names to the pcHead and pcTail structure members to ensure the readability of
the code is maintained despite this dual use of two structure members.  An
alternative implementation would be to use a union, but use of a union is
against the coding standard (although an exception to the standard has been
permitted where the dual use also significantly changes the type of the
structure member). */
#define pxMutexHolder					pcTail
//#define uxQueueType						pcHead
//#define queueQUEUE_IS_MUTEX				NULL


typedef		OSQHandle_t		OSMutexHandle_t;


OSQHandle_t 	OSQCreateMutex( const uOS8_t ucQueueType ) SMALLRTOS_FUNCTION;

OSMutexHandle_t OSMutexCreate( void ) SMALLRTOS_FUNCTION;
void 			OSMutexDelete( OSMutexHandle_t MutexHandle ) SMALLRTOS_FUNCTION;

sOSBase_t 		OSMutexLock( OSMutexHandle_t MutexHandle, uOSTick_t xTicksToWait) SMALLRTOS_FUNCTION;
sOSBase_t 		OSMutexUnlock( OSMutexHandle_t MutexHandle) SMALLRTOS_FUNCTION;

#endif //(OS_MUTEX_ON==1)

#ifdef __cplusplus
}
#endif

#endif //__OS_MUTEX_HPP
