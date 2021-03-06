/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/


#ifndef SMALLRTOS_CONFIG_H
#define SMALLRTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 *----------------------------------------------------------*/

/* Ensure stdint is only used by the compiler, and not the assembler. */
#ifdef __ICCARM__
	#include <stdint.h>
	extern uint32_t SystemCoreClock;
#endif

#define configCPU_CLOCK_HZ			( 48000000 ) //定义CPU运行主频
#define configTICK_RATE_HZ			( 1000 )             //定义SmallRTOS系统中ticks频率

#define configMINIMAL_STACK_SIZE		( 60 ) //定义任务占用的最小Stack空间
#define configTOTAL_HEAP_SIZE			( 0x200*4 ) //定义系统占用的Heap空间
#define configMAX_NAME_LEN						( 10 )  //定义任务、信号量、消息队列等变量中的名称长度
#define configMAX_PRIORITIES					( 5 )   //定义任务最大优先级
#define configUSE_SEMAPHORE                     ( 1 )   //是否启用系统信号量功能 0关闭 1启用
#define configUSE_MUTEX							( 0 )   //是否启用互斥信号量功能 0关闭 1启用
#define configUSE_MSGQ                          ( 0 )   //是否启用系统消息队列功能 0关闭 1启用
#define configMSGQ_MAX_MSGNUM                   ( 1 )  //定义消息队列中消息的门限值
#define configPEND_FOREVER_VALUE                ( 0xFFFFFFFF ) //定义信号量及消息队列中永久等待的数值


#define FitSVCHandler           SVC_Handler
#define FitPendSVHandler        PendSV_Handler
#define FitOSTickISR            SysTick_Handler

#endif /* SMALLRTOS_CONFIG_H */

