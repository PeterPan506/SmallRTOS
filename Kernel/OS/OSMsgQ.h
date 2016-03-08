/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

#ifndef __OS_MSGQ_H
#define __OS_MSGQ_H

#include "OSType.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (OS_MSGQ_ON==1)

typedef OSQHandle_t OSMsgQHandle_t;

OSMsgQHandle_t 	OSMsgQCreate( const uOSBase_t uxQueueLength, const uOSBase_t uxItemSize) SMALLRTOS_FUNCTION;
void 			OSMsgQDelete(OSMsgQHandle_t MsgQHandle) SMALLRTOS_FUNCTION;

sOSBase_t 		OSMsgQSendToHead( OSMsgQHandle_t MsgQHandle, const void * const pvItemToQueue, uOSTick_t xTicksToWait) SMALLRTOS_FUNCTION;
sOSBase_t 		OSMsgQSendToTail( OSMsgQHandle_t MsgQHandle, const void * const pvItemToQueue, uOSTick_t xTicksToWait) SMALLRTOS_FUNCTION;
sOSBase_t 		OSMsgQSend( OSMsgQHandle_t MsgQHandle, const void * const pvItemToQueue, uOSTick_t xTicksToWait) SMALLRTOS_FUNCTION;
sOSBase_t 		OSMsgQOverwrite( OSMsgQHandle_t MsgQHandle, const void * const pvItemToQueue) SMALLRTOS_FUNCTION;
sOSBase_t 		OSMsgQSendToHeadFromISR( OSMsgQHandle_t MsgQHandle, const void * const pvItemToQueue) SMALLRTOS_FUNCTION;
sOSBase_t 		OSMsgQSendToTailFromISR( OSMsgQHandle_t MsgQHandle, const void * const pvItemToQueue) SMALLRTOS_FUNCTION;
sOSBase_t 		OSMsgQSendFromISR( OSMsgQHandle_t MsgQHandle, const void * const pvItemToQueue) SMALLRTOS_FUNCTION;
sOSBase_t 		OSMsgQOverwriteFromISR( OSMsgQHandle_t MsgQHandle, const void * const pvItemToQueue) SMALLRTOS_FUNCTION;

sOSBase_t 		OSMsgQPeek( OSMsgQHandle_t MsgQHandle, void * const pvBuffer, uOSTick_t xTicksToWait) SMALLRTOS_FUNCTION;
sOSBase_t 		OSMsgQReceive( OSMsgQHandle_t MsgQHandle, void * const pvBuffer, uOSTick_t xTicksToWait) SMALLRTOS_FUNCTION;
sOSBase_t 		OSMsgQReceiveFromISR( OSMsgQHandle_t MsgQHandle, void * const pvBuffer) SMALLRTOS_FUNCTION;

#endif //(OS_MSGQ_ON==1)

#ifdef __cplusplus
}
#endif

#endif //__OS_MESSAGE_HPP
