/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

#ifndef __OS_SEMAPHORE_H
#define __OS_SEMAPHORE_H

#include "OSType.h"
#include "OSQueue.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (OS_SEMAPHORE_ON==1)

typedef OSQHandle_t OSSemHandle_t;

#define semBINARY_SEMAPHORE_QUEUE_LENGTH	( ( uOS8_t ) 1U )
#define semSEMAPHORE_QUEUE_ITEM_LENGTH		( ( uOS8_t ) 0U )
#define semGIVE_BLOCK_TIME					( ( uOSTick_t ) 0U )

OSSemHandle_t 	OSSemCreate( void ) SMALLRTOS_FUNCTION;
OSSemHandle_t 	OSSemCreateCount( const uOSBase_t uxMaxCount, const uOSBase_t uxInitialCount ) SMALLRTOS_FUNCTION;

void 			OSSemDelete(OSSemHandle_t SemHandle) SMALLRTOS_FUNCTION;

sOSBase_t 		OSSemPend( OSSemHandle_t SemHandle, uOSTick_t xTicksToWait) SMALLRTOS_FUNCTION;
sOSBase_t 		OSSemPendFromISR( OSSemHandle_t SemHandle ) SMALLRTOS_FUNCTION;

sOSBase_t 		OSSemPost( OSSemHandle_t SemHandle) SMALLRTOS_FUNCTION;
sOSBase_t 		OSSemPostFromISR( OSSemHandle_t SemHandle ) SMALLRTOS_FUNCTION;

#endif //(OS_SEMAPHORE_ON==1)

#ifdef __cplusplus
}
#endif

#endif //__OS_SEMAPHORE_HPP
