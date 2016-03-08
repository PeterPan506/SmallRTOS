/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/
#include <stdio.h>
#include "platform.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "interrupt.h"
#include "SmallRTOS.h"

/* Xilinx includes. */
#include "platform.h"
#include "xparameters.h"
#include "xscutimer.h"
#include "xscugic.h"
#include "xil_exception.h"

#ifdef __cplusplus
extern "C" {
#endif

short				gIntID = 1;
unsigned int 		gIntCount = 0;

OSTaskHandle_t 		HWTaskHandle = NULL;
OSTaskHandle_t 		GBTaskHandle = NULL;
OSTaskHandle_t 		SimTaskHandle = NULL;
OSSemHandle_t		GBSemaphoreHandle = NULL;

/* The queue used by both tasks. */
OSMsgQHandle_t 		MsgQHandle = NULL;

static void TaskHelloWorld( void *pvParameters );
static void TaskGoodBye( void *pvParameters );
static void TaskSimulateInterrupt( void *pvParameters );
static void IntTestFunction(void *CallbackRef);

/* The interrupt controller is initialised in this file, and made available to
other modules. */
extern XScuGic xInterruptController;

extern void FitInstallSmallRTOSVectorTable( void );

int main_general( void )
{
	/* Create the queue. */
	MsgQHandle = OSMsgQCreate( 1, sizeof( uint32_t ) );
	GBSemaphoreHandle = OSSemCreate();

	// configure and start tasks
	HWTaskHandle = OSTaskCreate(TaskHelloWorld, NULL, OSMINIMAL_STACK_SIZE, OSLOWEAST_PRIORITY+1, "HW");
	GBTaskHandle = OSTaskCreate(TaskGoodBye, NULL, OSMINIMAL_STACK_SIZE, OSLOWEAST_PRIORITY+2, "GB");
	SimTaskHandle = OSTaskCreate(TaskSimulateInterrupt, NULL, OSMINIMAL_STACK_SIZE, OSLOWEAST_PRIORITY+1, "SimInt");

	/* Start the tasks and timer running. */
	OSStart();

	for( ;; );
	
    return 0;
}

//interrupt function
static void IntTestFunction(void *CallbackRef)
{
//	OSSemPostFromISR(GBSemaphoreHandle);
	gIntCount += 1;
	OSMsgQSendFromISR(MsgQHandle, &gIntCount);
}

static void TaskSimulateInterrupt( void *pvParameters )
{

	gpInterruptController = &xInterruptController;
	ConfigInterrupt(0, IntTestFunction, gIntID);

	for( ;; )
	{
		xil_printf("Task Simulate Interrupt\r\n");
		OSTaskSleep(100*OSTICKS_PER_MS);
		XScuGic_SoftwareIntr(gpInterruptController, gIntID, XSCUGIC_SPI_CPU0_MASK);
	}
}

static void TaskHelloWorld( void *pvParameters )
{
	const unsigned long ulValueToSend = 100UL;

	/* Remove compiler warning about unused parameter. */
	( void ) pvParameters;
	( void ) ulValueToSend;

	for( ;; )
	{
		xil_printf("Hello World\r\n");
//		OSSemPost(GBSemaphoreHandle);
//		OSMsgQSend( MsgQHandle, &ulValueToSend, OSPEND_FOREVER_VALUE );

		OSTaskSleep(200*OSTICKS_PER_MS);
	}
}

static void TaskGoodBye( void *pvParameters )
{
	unsigned long ulReceivedValue;
	const unsigned long ulExpectedValue = 100UL;

	/* Remove compiler warning about unused parameter. */
	( void ) pvParameters;
	for( ;; )
	{

		xil_printf("Good Bye\r\n");

		OSMsgQReceive( MsgQHandle, &ulReceivedValue, OSPEND_FOREVER_VALUE );
//		OSSemPend(GBSemaphoreHandle, OSPEND_FOREVER_VALUE);

		/*  To get here something must have been received from the queue, but
		is it the expected value?  If it is, toggle the LED. */
		if( ulReceivedValue == ulExpectedValue )
		{
			ulReceivedValue = 0U;
		}
	}
}

#ifdef __cplusplus
}
#endif
