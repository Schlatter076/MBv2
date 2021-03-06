/*
 * tim4.c
 *
 *  Created on: 2020年7月20日
 *      Author: loyer
 */
#include "tim4.h"

volatile u8 initCnt = 0;
u8 hc595Statu = 0xFF;
volatile u32 heartCnt = 0;
volatile u8 statuHeartCnt = 0;
volatile u16 firstStatuHeartCnt = 0;
volatile u8 checkCnt = 0;
volatile u8 allowTCSamePort = 0;
volatile u16 allowTCSamePortCnt = 0;
//volatile u8 modifyPortSta = 0;
volatile u8 curPort = 0xFF;
volatile u8 allowModuleUpdate = 0;
u16 moduleUpCnt = 0;
/*******************************************************************************
 * 函 数 名         : TIM4_Init
 * 函数功能		   : TIM4初始化函数
 * 输    入         : per:重装载值
 psc:分频系数
 * 输    出         : 无
 *******************************************************************************/
void TIM4_Init(u16 per, u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	TIM_DeInit(TIM4);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); //使能TIM4时钟

	TIM_TimeBaseInitStructure.TIM_Period = per;   //自动装载值
	TIM_TimeBaseInitStructure.TIM_Prescaler = psc; //分频系数
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; //设置向上计数模式
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStructure);

	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE); //开启定时器中断
	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);

	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn; //定时器中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4; //抢占优先级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 4;		//子优先级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM4, ENABLE); //使能定时器
}
/*******************************************************************************
 * 函 数 名         : TIM4_IRQHandler
 * 函数功能		   : TIM4中断函数
 * 输    入         : 无
 * 输    出         : 无
 *******************************************************************************/
void TIM4_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);

		//第一次上报口状态失败
		if (F4G_Fram_Record_Struct.firstStatuHeartNotSucc)
		{
			firstStatuHeartCnt++;
			if (firstStatuHeartCnt == 2000)
			{
				firstStatuHeartCnt = 0;
				strcpy(F4G_Params.htCMD, "32");
				F4G_Params.STHeart = 1;
			}
		}
		//上电注册成功
		if (F4G_Fram_Record_Struct.registerSuccess)
		{
			F4G_Fram_Record_Struct.registerSuccess = 0;

			LED_Status = 1;
			//LED_Status = 0;

//			HC595_Send_Byte(0xFF); //灯全部灭
			//灯板开始单独控制某一个灯
//			HC595_STATUS.allowSigleContrl = 1;
//			HC595_STATUS.LAST_LED_STATU = 0xFF;
			//收到第一次上报的状态
			F4G_Fram_Record_Struct.init = 0; //初始化全部完成
			//第一次上报充电宝卡口信息
			strcpy(F4G_Params.htCMD, "32");
			F4G_Params.STHeart = 1;
			F4G_Fram_Record_Struct.firstStatuHeartNotSucc = 1;
		}
		//4G模块上电注册中
		if (F4G_Fram_Record_Struct.init)
		{
			initCnt++;
			if (initCnt == 50)
			{
				initCnt = 0;
				LED_Status ^= 1;
//				hc595Statu ^= 0xFF;
//				HC595_Send_Byte(hc595Statu);
			}
		}
		else //初始化完常亮
		{
			LED_Status = 1;
			//LED_Status = 0;
		}
		//上电成功，并成功获取到心跳信息
		if (RegisterParams.allowHeart)
		{
			heartCnt++;
			if (heartCnt == RegisterParams.heartTime * 100)
			{
				heartCnt = 0;
				common_heart();
				statuHeartCnt++;
				if (statuHeartCnt == RegisterParams.statuHeartTime)
				{
					statuHeartCnt = 0;
					strcpy(F4G_Params.htCMD, "90");
					F4G_Params.STHeart = 1;
				}
			}
		}

		if (allowTCSamePort)
		{
			allowTCSamePortCnt++;
			if (allowTCSamePortCnt == 300)
			{
				printf("allow TC.\r\n");
				allowTCSamePortCnt = 0;
				allowTCSamePort = 0;
				curPort = 0xFF;
			}
		}
		//快闪
		if (HC595_STATUS.fastBLINK[0])
		{
			HC595_STATUS.fastCnt[0]++;
			if (HC595_STATUS.fastCnt[0] == 20)
			{
				HC595_STATUS.fastCnt[0] = 0;
				ledBLINK(1);
			}
		}
		if (HC595_STATUS.fastBLINK[1])
		{
			HC595_STATUS.fastCnt[1]++;
			if (HC595_STATUS.fastCnt[1] == 20)
			{
				HC595_STATUS.fastCnt[1] = 0;
				ledBLINK(2);
			}
		}
		if (HC595_STATUS.fastBLINK[2])
		{
			HC595_STATUS.fastCnt[2]++;
			if (HC595_STATUS.fastCnt[2] == 20)
			{
				HC595_STATUS.fastCnt[2] = 0;
				ledBLINK(3);
			}
		}
		if (HC595_STATUS.fastBLINK[3])
		{
			HC595_STATUS.fastCnt[3]++;
			if (HC595_STATUS.fastCnt[3] == 20)
			{
				HC595_STATUS.fastCnt[3] = 0;
				ledBLINK(4);
			}
		}
		if (HC595_STATUS.fastBLINK[4])
		{
			HC595_STATUS.fastCnt[4]++;
			if (HC595_STATUS.fastCnt[4] == 20)
			{
				HC595_STATUS.fastCnt[4] = 0;
				ledBLINK(5);
			}
		}
		if (HC595_STATUS.fastBLINK[5])
		{
			HC595_STATUS.fastCnt[5]++;
			if (HC595_STATUS.fastCnt[5] == 20)
			{
				HC595_STATUS.fastCnt[5] = 0;
				ledBLINK(6);
			}
		}
		//慢闪
		if (HC595_STATUS.slowBLINK[0])
		{
			HC595_STATUS.slowCnt[0]++;
			if (HC595_STATUS.slowCnt[0] == 80)
			{
				HC595_STATUS.slowCnt[0] = 0;
				ledBLINK(1);
			}
		}
		if (HC595_STATUS.slowBLINK[1])
		{
			HC595_STATUS.slowCnt[1]++;
			if (HC595_STATUS.slowCnt[1] == 80)
			{
				HC595_STATUS.slowCnt[1] = 0;
				ledBLINK(2);
			}
		}
		if (HC595_STATUS.slowBLINK[2])
		{
			HC595_STATUS.slowCnt[2]++;
			if (HC595_STATUS.slowCnt[2] == 80)
			{
				HC595_STATUS.slowCnt[2] = 0;
				ledBLINK(3);
			}
		}
		if (HC595_STATUS.slowBLINK[3])
		{
			HC595_STATUS.slowCnt[3]++;
			if (HC595_STATUS.slowCnt[3] == 80)
			{
				HC595_STATUS.slowCnt[3] = 0;
				ledBLINK(4);
			}
		}
		if (HC595_STATUS.slowBLINK[4])
		{
			HC595_STATUS.slowCnt[4]++;
			if (HC595_STATUS.slowCnt[4] == 80)
			{
				HC595_STATUS.slowCnt[4] = 0;
				ledBLINK(5);
			}
		}
		if (HC595_STATUS.slowBLINK[5])
		{
			HC595_STATUS.slowCnt[5]++;
			if (HC595_STATUS.slowCnt[5] == 80)
			{
				HC595_STATUS.slowCnt[5] = 0;
				ledBLINK(6);
			}
		}
		checkCnt++;
		if (checkCnt == 2)
		{
			checkCnt = 0;
			F4G_Params.checkPBst = 1;
		}
	}
}
