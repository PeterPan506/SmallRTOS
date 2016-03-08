/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

/* Hardware and starter kit includes. */
#include "gpio.h"
//SmallRTOS
#include "SmallRTOS.h"

OSTaskHandle_t 			LedTaskHandle	= NULL;
OSTaskHandle_t 			CtrlTaskHandle	= NULL;
OSSemHandle_t			LedSemHandle	= NULL;

void SetupHardware( void );
void LedTask( void *pvParameters );
void CtrlTask( void *pvParameters );

int main()
{
	SetupHardware();

	LedSemHandle	= OSSemCreate( );

	// Start the two tasks
	LedTaskHandle	= OSTaskCreate(LedTask, NULL, OSMINIMAL_STACK_SIZE, OSLOWEAST_PRIORITY+1, "Led" );
	CtrlTaskHandle	= OSTaskCreate(CtrlTask, NULL, OSMINIMAL_STACK_SIZE, OSLOWEAST_PRIORITY+1, "NumRev" );

	// Start the scheduler. 
	OSStart();

	//if everything is ok, can't reach here
	for( ;; );

}

void SetupHardware( void )
{
	//定义GPIO初始化结构
	GPIO_InitTypeDef GPIO_InitStruct1;

	//初始化默认的调试端口 
	GPIO_InitStruct1.GPIOx = PTC;                       //C端口
	GPIO_InitStruct1.GPIO_InitState = Bit_RESET;        //如果设置为输出 则初始化输出低电平(RESET) 设置为输入 此项无效
	GPIO_InitStruct1.GPIO_IRQMode = GPIO_IT_DISABLE;    //此引脚不用做外部中断
	GPIO_InitStruct1.GPIO_Pin = GPIO_Pin_8;             //D8引脚
	GPIO_InitStruct1.GPIO_Mode = GPIO_Mode_OPP;         //设置为推挽输出模式

	//执行GPIO初始化 点亮LED
	GPIO_Init(&GPIO_InitStruct1);    

	return;
}

/* The ISR executed when the user button is pushed. */
void vPort_E_ISRHandler( void )
{
}

void CtrlTask( void *pvParameters )
{
	for( ;; )
	{
		OSSemPost(LedSemHandle);
		OSTaskSleep(500*OSTICKS_PER_MS);
	}
}


/*-----------------------------------------------------------*/
void LedTask( void *pvParameters )
{
	int i = 0;
	for( ;; )
	{		
		OSSemPend(LedSemHandle, OSPEND_FOREVER_VALUE);
		GPIO_ToggleBit(PTC, GPIO_Pin_8);
		i += 1;	
	}
}

void vEMAC_TxISRHandler( void ) {}
void vEMAC_RxISRHandler( void ){}
void vEMAC_ErrorISRHandler( void ) {}