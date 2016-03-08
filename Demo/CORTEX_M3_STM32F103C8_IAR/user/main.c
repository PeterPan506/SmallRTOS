/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

/* Hardware and starter kit includes. */
#include "stm32f10x.h"
//SmallRTOS
#include "SmallRTOS.h"
#include "GPIOLIKE51.h"


OSTaskHandle_t			LedTaskHandle		= NULL;
OSTaskHandle_t			CtrlTaskHandle		= NULL;
OSSemHandle_t			LedSemHandle		= NULL;
OSTimerHandle_t			Timer100msHandle	= NULL;

static void LedTask( void *pvParameters );
static void CtrlTask( void *pvParameters );
static void Timer100msFunction( void *pvParameters );

static void SetupHardware( void );
static void GPIO_Configuration(void);

int main()
{
	SetupHardware();
	GPIO_Configuration();

	LedSemHandle 	= OSSemCreate();

	// Start the two tasks
	LedTaskHandle 	= OSTaskCreate(LedTask, NULL, OSMINIMAL_STACK_SIZE, OSLOWEAST_PRIORITY+1, "Led" );
	CtrlTaskHandle 	= OSTaskCreate(CtrlTask, NULL, OSMINIMAL_STACK_SIZE, OSLOWEAST_PRIORITY+1, "Ctrl" );
	
	Timer100msHandle = OSTimerCreate(Timer100msFunction, NULL, 100, "timer100ms");

	OSTimerStart(Timer100msHandle);
        
	// Start the scheduler. 
	OSStart();

	//if everything is ok, can't reach here
	for( ;; );

}

static void SetupHardware( void )
{
	/* Start with the clocks in their expected state. */
	RCC_DeInit();

	/* Enable HSE (high speed external clock). */
	RCC_HSEConfig( RCC_HSE_ON );

	/* Wait till HSE is ready. */
	while( RCC_GetFlagStatus( RCC_FLAG_HSERDY ) == RESET )
	{
	}

	/* 2 wait states required on the flash. */
	*( ( unsigned long * ) 0x40022000 ) = 0x02;

	/* HCLK = SYSCLK */
	RCC_HCLKConfig( RCC_SYSCLK_Div1 );

	/* PCLK2 = HCLK */
	RCC_PCLK2Config( RCC_HCLK_Div1 );

	/* PCLK1 = HCLK/2 */
	RCC_PCLK1Config( RCC_HCLK_Div2 );

	/* PLLCLK = 8MHz * 9 = 72 MHz. */
	RCC_PLLConfig( RCC_PLLSource_HSE_Div1, RCC_PLLMul_9 );

	/* Enable PLL. */
	RCC_PLLCmd( ENABLE );

	/* Wait till PLL is ready. */
	while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
	{
	}

	/* Select PLL as system clock source. */
	RCC_SYSCLKConfig( RCC_SYSCLKSource_PLLCLK );

	/* Wait till PLL is used as system clock source. */
	while( RCC_GetSYSCLKSource() != 0x08 )
	{
	}

	/* Enable GPIOA, GPIOB, GPIOC, GPIOD, GPIOE and AFIO clocks */
	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |RCC_APB2Periph_GPIOC
							| RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE | RCC_APB2Periph_AFIO, ENABLE );

	/* SPI2 Periph clock enable */
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_SPI2, ENABLE );


	/* Set the Vector Table base address at 0x08000000 */
	NVIC_SetVectorTable( NVIC_VectTab_FLASH, 0x0 );

	NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );

	/* Configure HCLK clock as SysTick clock source. */
	SysTick_CLKSourceConfig( SysTick_CLKSource_HCLK );

}

//=============================================================================
//文件名称：GPIO_Configuration
//功能概要：GPIO初始化
//参数说明：无
//函数返回：无
//=============================================================================
static void GPIO_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  
  RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOC , ENABLE); 						 
//=============================================================================
//LED -> PC13
//=============================================================================			 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
  GPIO_Init(GPIOC, &GPIO_InitStructure);
}

static void CtrlTask( void *pvParameters )
{
	for( ;; )
	{
		OSSemPost(LedSemHandle);
		OSTaskSleep(100*OSTICKS_PER_MS);
	}
}


/*-----------------------------------------------------------*/
static void LedTask( void *pvParameters )
{
	int i = 0;
	for( ;; )
	{
		OSSemPend(LedSemHandle, OSPEND_FOREVER_VALUE);	
		PCout(13) = i;
		i = 1 - i;
	}
}

static void Timer100msFunction( void *pvParameters )
{
	//do something
}