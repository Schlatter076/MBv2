/*
 * app.c
 *
 *  Created on: 2020��10��26��
 *      Author: loyer
 */
#include "app.h"

pFunction Jump_To_Application;
uint32_t JumpAddress;

void RunApp(void)
{
	if (((*(__IO uint32_t*) ApplicationAddress) & 0x2FFE0000) == 0x20000000)
	{
		printf("\r\nRun to app.\r\n");
		RCC_DeInit(); //�ر�����
		__disable_irq();
		TIM_Cmd(TIM4, DISABLE); //�رն�ʱ��4
		USART_Cmd(USART1, DISABLE);
		USART_Cmd(USART2, DISABLE);
		JumpAddress = *(__IO uint32_t*) (ApplicationAddress + 4);
		Jump_To_Application = (pFunction) JumpAddress;
		__set_MSP(*(__IO uint32_t*) ApplicationAddress);
		Jump_To_Application();
	}
	else
	{
		printf("\r\nRun to app error.\r\n");
		NVIC_SystemReset(); //��תʧ�ܣ�����ϵͳ
	}
}

