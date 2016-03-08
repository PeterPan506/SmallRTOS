/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

/* Hardware and starter kit includes. */
#include "stm32f0xx.h"
//SmallRTOS
#include "SmallRTOS.h"

OSTimerHandle_t			Timer500msHandle	= NULL;
OSTaskHandle_t 			CtrlTaskHandle		= NULL;
OSTaskHandle_t 			LedTaskHandle		= NULL;
OSSemHandle_t 			LedSemHandle		= NULL;

void CtrlTask( void *pvParameters );
void LedTask( void *pvParameters );
void Timer500ms( void *pvParameters );

void GPIO_Configuration(void);

int main()
{
	RCC_ClocksTypeDef RCC_Clocks;

	/* SysTick end of count event each 1ms */
	RCC_GetClocksFreq(&RCC_Clocks);
	SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);
	GPIO_Configuration();

	//创建500ms的定时器，分配定时器服务函数为Timer500ms
	Timer500msHandle = OSTimerCreate(Timer500ms, NULL, 500, "T500ms");
	//创建信号量
	LedSemHandle	= OSSemCreate();
	//创建任务，控制LED灯
	CtrlTaskHandle	= OSTaskCreate(CtrlTask, NULL, OSMINIMAL_STACK_SIZE, OSLOWEAST_PRIORITY+1, "Ctrl" );
	LedTaskHandle	= OSTaskCreate(LedTask, NULL, OSMINIMAL_STACK_SIZE, OSLOWEAST_PRIORITY+1, "Led" );

	OSTimerStart(Timer500msHandle);
	
	// Start the scheduler. 
	OSStart();

	//if everything is ok, can't reach here
	for( ;; );

}


//=============================================================================
//文件名称：GPIO_Configuration
//功能概要：GPIO初始化
//参数说明：无
//函数返回：无
//=============================================================================
void GPIO_Configuration(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  
  /* 使能GPIOB时钟 */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);

  /* 配置LED相应引脚PB1*/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void LedTask( void *pvParameters )
{
	int i = 1;

	for( ;; )
	{
		OSSemPend(LedSemHandle, OSPEND_FOREVER_VALUE);	
		
		i = 1 - i;
		if(i==0)
		{
			GPIO_SetBits(GPIOB,GPIO_Pin_1);    
		}
		else
		{
			GPIO_ResetBits(GPIOB,GPIO_Pin_1);
		}            
	}
}

void CtrlTask( void *pvParameters )
{
	for( ;; )
	{
		OSSemPost(LedSemHandle);	
		
		OSTaskSleep(500*OSTICKS_PER_MS);        
	}
}

void Timer500ms( void *pvParameters )
{

}