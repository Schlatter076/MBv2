/* Includes */
#include <stddef.h>
#include "stm32f10x.h"
#include "SysTick.h"
#include "usart.h"
#include "stdio.h"
#include "wifi_drive.h"
#include "sta_tcpclent_test.h"
#include "usart_4G.h"
#include "L74hc165.h"
#include "STMFlash.h"
#include "motor.h"
#include "L74hc165.h"
#include "audio.h"
#include "charge.h"
#include "bat_usart.h"
#include "L74HC595.h"
#include "led.h"
#include "tim4.h"
#include "app.h"

//volatile u8 curPort = 0xFF;

void HSI_SetSysClock(uint32_t pllmul)
{
	__IO uint32_t HSIStartUpStatus = 0;

	// �� RCC �����ʼ���ɸ�λ״̬������Ǳ����
	RCC_DeInit();

	//ʹ�� HSI
	RCC_HSICmd(ENABLE);

	// �ȴ� HSI ����
	HSIStartUpStatus = RCC->CR & RCC_CR_HSIRDY;

	// ֻ�� HSI ����֮�����������ִ��
	if (HSIStartUpStatus == RCC_CR_HSIRDY)
	{
		//-------------------------------------------------------------//

		// ʹ�� FLASH Ԥ��ȡ������
		FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

		// SYSCLK �������������ʱ��ı������ã�����ͳһ���ó� 2
		// ���ó� 2 ��ʱ��SYSCLK ���� 48M Ҳ���Թ�����������ó� 0 ���� 1 ��ʱ��
		// ������õ� SYSCLK �����˷�Χ�Ļ���������Ӳ�����󣬳��������
		// 0��0 < SYSCLK <= 24M
		// 1��24< SYSCLK <= 48M
		// 2��48< SYSCLK <= 72M
		FLASH_SetLatency(FLASH_Latency_2);
		//------------------------------------------------------------//

		// AHB Ԥ��Ƶ��������Ϊ 1 ��Ƶ��HCLK = SYSCLK
		RCC_HCLKConfig(RCC_SYSCLK_Div1);

		// APB2 Ԥ��Ƶ��������Ϊ 1 ��Ƶ��PCLK2 = HCLK
		RCC_PCLK2Config(RCC_HCLK_Div1);

		// APB1 Ԥ��Ƶ��������Ϊ 1 ��Ƶ��PCLK1 = HCLK/2
		RCC_PCLK1Config(RCC_HCLK_Div2);

		//-----------���ø���Ƶ����Ҫ��������������-------------------//
		// ���� PLL ʱ����ԴΪ HSE������ PLL ��Ƶ����
		// PLLCLK = 4MHz * pllmul
		RCC_PLLConfig(RCC_PLLSource_HSI_Div2, pllmul);
		//-- -----------------------------------------------------//

		// ���� PLL
		RCC_PLLCmd(ENABLE);

		// �ȴ� PLL �ȶ�
		while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
		{
		}

		// �� PLL �ȶ�֮�󣬰� PLL ʱ���л�Ϊϵͳʱ�� SYSCLK
		RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

		// ��ȡʱ���л�״̬λ��ȷ�� PLLCLK ��ѡΪϵͳʱ��
		while (RCC_GetSYSCLKSource() != 0x08)
		{
		}
	}
	else
	{
		// ��� HSI ����ʧ�ܣ���ô����ͻ���������û�����������ӳ���Ĵ��봦��
		// �� HSE ����ʧ�ܻ��߹��ϵ�ʱ�򣬵�Ƭ�����Զ��� HSI ����Ϊϵͳʱ�ӣ�
		// HSI ���ڲ��ĸ���ʱ�ӣ�8MHZ
		while (1)
		{

		}
	}
}
/**
 **===========================================================================
 **
 **  Abstract: main program
 **
 **===========================================================================
 */
int main(void)
{
	int i = 0;
//	SystemInit();
	HSI_SetSysClock(RCC_PLLMul_9);
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x10000);
	__enable_irq(); //�������ж�
	SysTick_Init(36);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //�ж����ȼ����� ��2��S
	USART1_Init(115200);
	printf("Now In APP\r\n");
//	WIFI_Init(115200);
	Init_74HC165();
	Init_Audio();
	Init_Motor();
	Init_BAT_Charge();
	Init_Bat_Usart(115200);
	Init_74HC595();
	Init_led();
	TIM4_Init(10, 36000 - 1); //10ms
	//scanPowerBank();
	F4G_Init(115200);

//	delay_ms(1000);
//	WIFI_STA_TCPClient_Test();

	/* Infinite loop */
	while (1)
	{
//		if(modifyPortSta)
//		{
//			modifyPortSta = 0;
//			curPort = 0xFF;
//		}
//		if(F4G_Params.linkedClosed)
//		{
//			F4G_Params.linkedClosed = 0;
//			printf("TCP connect was closed\r\n");
//			checkLinkedStatus();
//		}
		//ϵͳ����
		if (F4G_Params.sysTC)
		{
			F4G_Params.sysTC = 0;
			if (curPort != F4G_Params.port)
			{
				curPort = F4G_Params.port;
				popUP_powerBank(F4G_Params.port + 1, F4G_Params.play);
				sys_tc(F4G_Params.port, F4G_Params.cmd);
				//curPort = 0xFF;
				allowTCSamePort = 1;
				//printf("bool=%d\r\n", allowTCSamePort);
			}
		}
		//��������
		else if (F4G_Params.ddTC)
		{
			F4G_Params.ddTC = 0;
			if (curPort != F4G_Params.port)
			{
				curPort = F4G_Params.port;
				popUP_powerBank(F4G_Params.port + 1, 1);
				dd_tc(F4G_Params.port, "41");
				//curPort = 0xFF;
				allowTCSamePort = 1;
			}
		}
		//״̬����
		else if (F4G_Params.STHeart)
		{
			//curPort = 0xFF; //״̬����ʱȡ��������ֹ
			F4G_Params.STHeart = 0;
			scanPowerBank();
			statu_heart(F4G_Params.htCMD);
		}
		//ǿ������
		else if (F4G_Params.reqSTheart)
		{
			F4G_Params.reqSTheart = 0;
			scanPowerBank();
			statu_heart(F4G_Params.htCMD);
		}
		else if (F4G_Params.checkPBst)
//		else
		{
			F4G_Params.checkPBst = 0;
			for (i = 0; i < 6; i++)
			{
				F4G_Params.currentStatuCode[i] = checkPowerbankStatus(i,
						powerbankStatu.powerBankBuf[i]);

//				if ((F4G_Params.currentStatuCode[i] != F4G_Params.statuCode[i])
//						&& F4G_Params.allowCheckChange)
				if ((F4G_Params.currentStatuCode[i] != F4G_Params.statuCode[i]))
				{
					if (curPort == i)
					{
						curPort = 0xFF;
					}
					F4G_Params.statuCode[i] = F4G_Params.currentStatuCode[i]; //��ȡ��ǰ״̬
					currentPortStatuChanged(i);
					break;
				}
			}
		}
		if (F4G_Params.popAll)
		{
			F4G_Params.popAll = 0;
			popUP_All();
		}
//		��������Ƿ�ʧ
		if (F4G_Params.serverStatuCnt >= 2 && allowModuleUpdate == 0)
		{
			printf("Data accept Fail twice!\r\n");
			//NVIC_SystemReset();
			//F4G_Init(115200);
			RunApp(); //��ת��������ʼλ��
			F4G_Params.serverStatuCnt = 0;
		}
	}
}
