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

	// 把 RCC 外设初始化成复位状态，这句是必须的
	RCC_DeInit();

	//使能 HSI
	RCC_HSICmd(ENABLE);

	// 等待 HSI 就绪
	HSIStartUpStatus = RCC->CR & RCC_CR_HSIRDY;

	// 只有 HSI 就绪之后则继续往下执行
	if (HSIStartUpStatus == RCC_CR_HSIRDY)
	{
		//-------------------------------------------------------------//

		// 使能 FLASH 预存取缓冲区
		FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

		// SYSCLK 周期与闪存访问时间的比例设置，这里统一设置成 2
		// 设置成 2 的时候，SYSCLK 低于 48M 也可以工作，如果设置成 0 或者 1 的时候，
		// 如果配置的 SYSCLK 超出了范围的话，则会进入硬件错误，程序就死了
		// 0：0 < SYSCLK <= 24M
		// 1：24< SYSCLK <= 48M
		// 2：48< SYSCLK <= 72M
		FLASH_SetLatency(FLASH_Latency_2);
		//------------------------------------------------------------//

		// AHB 预分频因子设置为 1 分频，HCLK = SYSCLK
		RCC_HCLKConfig(RCC_SYSCLK_Div1);

		// APB2 预分频因子设置为 1 分频，PCLK2 = HCLK
		RCC_PCLK2Config(RCC_HCLK_Div1);

		// APB1 预分频因子设置为 1 分频，PCLK1 = HCLK/2
		RCC_PCLK1Config(RCC_HCLK_Div2);

		//-----------设置各种频率主要就是在这里设置-------------------//
		// 设置 PLL 时钟来源为 HSE，设置 PLL 倍频因子
		// PLLCLK = 4MHz * pllmul
		RCC_PLLConfig(RCC_PLLSource_HSI_Div2, pllmul);
		//-- -----------------------------------------------------//

		// 开启 PLL
		RCC_PLLCmd(ENABLE);

		// 等待 PLL 稳定
		while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
		{
		}

		// 当 PLL 稳定之后，把 PLL 时钟切换为系统时钟 SYSCLK
		RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

		// 读取时钟切换状态位，确保 PLLCLK 被选为系统时钟
		while (RCC_GetSYSCLKSource() != 0x08)
		{
		}
	}
	else
	{
		// 如果 HSI 开启失败，那么程序就会来到这里，用户可在这里添加出错的代码处理
		// 当 HSE 开启失败或者故障的时候，单片机会自动把 HSI 设置为系统时钟，
		// HSI 是内部的高速时钟，8MHZ
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
	__enable_irq(); //开启总中断
	SysTick_Init(36);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组S
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
		//系统弹出
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
		//订单弹出
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
		//状态心跳
		else if (F4G_Params.STHeart)
		{
			//curPort = 0xFF; //状态心跳时取消弹出禁止
			F4G_Params.STHeart = 0;
			scanPowerBank();
			statu_heart(F4G_Params.htCMD);
		}
		//强制心跳
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
					F4G_Params.statuCode[i] = F4G_Params.currentStatuCode[i]; //记取当前状态
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
//		检查连接是否丢失
		if (F4G_Params.serverStatuCnt >= 2 && allowModuleUpdate == 0)
		{
			printf("Data accept Fail twice!\r\n");
			//NVIC_SystemReset();
			//F4G_Init(115200);
			RunApp(); //跳转到程序起始位置
			F4G_Params.serverStatuCnt = 0;
		}
	}
}
