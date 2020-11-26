/*
 * tim4.c
 *
 *  Created on: 2020��7��20��
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
 * �� �� ��         : TIM4_Init
 * ��������		   : TIM4��ʼ������
 * ��    ��         : per:��װ��ֵ
 psc:��Ƶϵ��
 * ��    ��         : ��
 *******************************************************************************/
void TIM4_Init(u16 per, u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	TIM_DeInit(TIM4);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); //ʹ��TIM4ʱ��

	TIM_TimeBaseInitStructure.TIM_Period = per;   //�Զ�װ��ֵ
	TIM_TimeBaseInitStructure.TIM_Prescaler = psc; //��Ƶϵ��
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; //�������ϼ���ģʽ
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStructure);

	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE); //������ʱ���ж�
	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);

	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn; //��ʱ���ж�ͨ��
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4; //��ռ���ȼ�
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 4;		//�����ȼ�
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM4, ENABLE); //ʹ�ܶ�ʱ��
}
/*******************************************************************************
 * �� �� ��         : TIM4_IRQHandler
 * ��������		   : TIM4�жϺ���
 * ��    ��         : ��
 * ��    ��         : ��
 *******************************************************************************/
void TIM4_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);

		//��һ���ϱ���״̬ʧ��
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
		//�ϵ�ע��ɹ�
		if (F4G_Fram_Record_Struct.registerSuccess)
		{
			F4G_Fram_Record_Struct.registerSuccess = 0;

			LED_Status = 1;
			//LED_Status = 0;

//			HC595_Send_Byte(0xFF); //��ȫ����
			//�ư忪ʼ��������ĳһ����
//			HC595_STATUS.allowSigleContrl = 1;
//			HC595_STATUS.LAST_LED_STATU = 0xFF;
			//�յ���һ���ϱ���״̬
			F4G_Fram_Record_Struct.init = 0; //��ʼ��ȫ�����
			//��һ���ϱ���籦������Ϣ
			strcpy(F4G_Params.htCMD, "32");
			F4G_Params.STHeart = 1;
			F4G_Fram_Record_Struct.firstStatuHeartNotSucc = 1;
		}
		//4Gģ���ϵ�ע����
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
		else //��ʼ���곣��
		{
			LED_Status = 1;
			//LED_Status = 0;
		}
		//�ϵ�ɹ������ɹ���ȡ��������Ϣ
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
		//����
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
		//����
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
