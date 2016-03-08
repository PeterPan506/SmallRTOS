/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

/* Hardware and starter kit includes. */
#include "stm32f407zg_sk.h"
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
//SmallRTOS
#include "SmallRTOS.h"

OSTaskHandle_t 			LedTaskHandle	= NULL;
OSTaskHandle_t 			CtrlTaskHandle	= NULL;
OSSemHandle_t			LedSemHandle	= NULL;

static void SetupHardware( void );
static void LedTask( void *pvParameters );
static void CtrlTask( void *pvParameters );

int main()
{
	SetupHardware();

	LedSemHandle	= OSSemCreate();

	// Start the two tasks
	LedTaskHandle	= OSTaskCreate(LedTask, NULL, OSMINIMAL_STACK_SIZE, OSLOWEAST_PRIORITY+1, "Led" );
	CtrlTaskHandle	= OSTaskCreate(CtrlTask, NULL, OSMINIMAL_STACK_SIZE, OSLOWEAST_PRIORITY+1, "NumRev" );

	// Start the scheduler. 
	OSStart();

	//if everything is ok, can't reach here
	for( ;; );

}

void ParTestInitialise( void )
{
	/* Initialise all four LEDs that are built onto the starter kit. */
	STM_EVAL_LEDInit( LED1 );
	STM_EVAL_LEDInit( LED2 );
	STM_EVAL_LEDInit( LED3 );
	STM_EVAL_LEDInit( LED4 );
	STM_EVAL_LEDInit( LED5 );
}

static void SetupHardware( void )
{
	/* Setup STM32 system (clock, PLL and Flash configuration) */
	SystemInit();

	/* Ensure all priority bits are assigned as preemption priority bits. */
	NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );

	/* Setup the LED outputs. */
	ParTestInitialise();

	return;

}

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
	int i = 0;
	for( ;; )
	{		
		OSSemPend(LedSemHandle, OSPEND_FOREVER_VALUE);

		STM_EVAL_LEDToggle( LED1 );
		STM_EVAL_LEDToggle( LED2 );
		STM_EVAL_LEDToggle( LED3 );
		STM_EVAL_LEDToggle( LED4 );		
		STM_EVAL_LEDToggle( LED5 );
		i += 1;	
	}
}
