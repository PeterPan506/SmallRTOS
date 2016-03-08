/***********************************************************************
文件名称：LED.h
功    能：led  IO初始化
编写时间：2013.4.25
编 写 人：
注    意：
***********************************************************************/

#ifndef _LED_H_
#define _LED_H_

#define LED1_ON 		GPIO_ResetBits(GPIOE , GPIO_Pin_9)
#define LED2_ON 		GPIO_ResetBits(GPIOE , GPIO_Pin_11)
#define LED3_ON 		GPIO_ResetBits(GPIOE , GPIO_Pin_13)
#define LED4_ON 		GPIO_ResetBits(GPIOE , GPIO_Pin_14)


#define LED1_OFF 		GPIO_SetBits(GPIOE , GPIO_Pin_9)
#define LED2_OFF 		GPIO_SetBits(GPIOE , GPIO_Pin_11)
#define LED3_OFF 		GPIO_SetBits(GPIOE , GPIO_Pin_13)
#define LED4_OFF 		GPIO_SetBits(GPIOE , GPIO_Pin_14)

void LED_Configuration(void);
void LED_Blink(void);
void One_LED_ON(unsigned char led_num);


#endif

