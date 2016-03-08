/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

/* Hardware and starter kit includes. */
#include "stm32f10x.h"
//SmallRTOS
#include "SmallRTOS.h"
#include "HardwareInit.h"
#include "LED.h"

OSTaskHandle_t 			LedTaskHandle		= NULL;
OSTaskHandle_t 			CtrlTaskHandle		= NULL;
OSSemHandle_t		LedSemHandle		= NULL;

static void LedTask( void *pvParameters );
static void CtrlTask( void *pvParameters );

int main()
{
	SetupHardware();
	LED_Configuration();
	
	LedSemHandle 	= OSSemCreate();

	// Start the two tasks
	LedTaskHandle 	= OSTaskCreate(LedTask, NULL, OSMINIMAL_STACK_SIZE, OSLOWEAST_PRIORITY+1, "Led" );
	CtrlTaskHandle 	= OSTaskCreate(CtrlTask, NULL, OSMINIMAL_STACK_SIZE, OSLOWEAST_PRIORITY+1, "Ctrl" );
	
	// Start the scheduler. 
	OSStart();

	//if everything is ok, can't reach here
	for( ;; );

}

/*-----------------------------------------------------------*/
static void CtrlTask( void *pvParameters )
{
	for( ;; )
	{
		OSSemPost(LedSemHandle);
		OSTaskSleep(500*OSTICKS_PER_MS);
	}
}


/*-----------------------------------------------------------*/
static void LedTask( void *pvParameters )
{
	int i = 1;
	for( ;; )
	{
		OSSemPend(LedSemHandle, OSPEND_FOREVER_VALUE);	
		
		One_LED_ON(i);
		i = i+1;
		if(i==5)
		{
			i = 1;
		}
	}
}
