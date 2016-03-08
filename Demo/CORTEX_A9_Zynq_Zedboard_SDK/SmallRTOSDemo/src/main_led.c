/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/
#include <stdio.h>
#include "SmallRTOS.h"

/* Xilinx includes. */
#include <stdio.h>
#include "platform.h"
#include "xparameters.h"
#include "xgpio.h"

#ifdef __cplusplus
extern "C" {
#endif


/************************** Constant Definitions *****************************/

/*
 * The following constant maps to the name of the hardware instances that
 * were created in the EDK XPS system.
 */
#define XPAR_LEDS_ID XPAR_AXI_GPIO_0_BASEADDR


/*
 * The following constant is used to determine which channel of the GPIO is
 * used for the LED if there are 2 channels supported.
 */
#define LED_CHANNEL 1

/************************** Variable Definitions *****************************/

/*
 * The following are declared globally so they are zeroed and so they are
 * easily accessible from a debugger
 */

XGpio Gpio; /* The Instance of the GPIO Driver */

OSTaskHandle_t 		CtrlTaskHandle = NULL;
OSTaskHandle_t 		LedTaskHandle = NULL;
OSSemHandle_t		LedSemHandle = NULL;

static void TaskCtrl( void *pvParameters );
static void TaskLed( void *pvParameters );

static int SetupGpio( void );


int main_led()
{	
    SetupGpio();

	LedSemHandle = OSSemCreate();//0, "Led"

	// configure and start tasks
	CtrlTaskHandle = OSTaskCreate(TaskCtrl, NULL, OSMINIMAL_STACK_SIZE, OSLOWEAST_PRIORITY+1, "Ctrl" );
	LedTaskHandle = OSTaskCreate(TaskLed, NULL, OSMINIMAL_STACK_SIZE, OSLOWEAST_PRIORITY+2, "Led" );

	/* Start the tasks and timer running. */
	OSStart();

	for( ;; );

    return 0;	
}


static int SetupGpio( void )
{
    int Status;

	// Initialize the GPIO driver
	Status = XGpio_Initialize(&Gpio, XPAR_LEDS_ID);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	// Set the direction for all signals to be outputs
	XGpio_SetDataDirection(&Gpio, LED_CHANNEL, 0x00);

	return XST_SUCCESS;
}

static void TaskCtrl( void *pvParameters )
{

	for( ;; )
	{
		xil_printf("Ctrl Task\r\n");
		OSSemPost(LedSemHandle);
		OSTaskSleep(500*OSTICKS_PER_MS);
	}
}

static void TaskLed( void *pvParameters )
{
	short i = 0;
	for( ;; )
	{
		xil_printf("Led Task\r\n");
		OSSemPend(LedSemHandle, OSPEND_FOREVER_VALUE);
//		OSTaskSleep(500*OSTICKS_PER_MS);

		if(i>7)
		{
			i = 0;
		}
		XGpio_DiscreteWrite(&Gpio, LED_CHANNEL, (1<<i));
		i += 1;
	}
}

#ifdef __cplusplus
}
#endif
