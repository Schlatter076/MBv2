/*
 * charge.c
 *
 *  Created on: 2020��6��17��
 *      Author: loyer
 */
#include "charge.h"

void Init_BAT_Charge(void)
{
	GPIO_InitTypeDef GPIO_InitStructure; //����ṹ�����

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2
			| GPIO_Pin_12;  //ѡ����Ҫ���õ�IO��
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	 //�����������ģʽ
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	  //���ô�������
	GPIO_Init(GPIOB, &GPIO_InitStructure); /* ��ʼ��GPIO */

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;  //ѡ����Ҫ���õ�IO��
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	 //�����������ģʽ
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	  //���ô�������
	GPIO_Init(GPIOC, &GPIO_InitStructure); /* ��ʼ��GPIO */

	BAT1 = 1;
	BAT2 = 1;
	BAT3 = 1;
	BAT4 = 1;
	BAT5 = 1;
	BAT6 = 1;
}
/**
 * ���Ƴ�籦��纯��
 * @pb 1-6��ʾ��籦1-6
 * @allow 1-��ʾ����  0-��ʾ������
 */
void controlPowerBankCharge(u8 pb, u8 allow)
{
	switch (pb)
	{
	case 1:
		BAT1 = allow;
		break;
	case 2:
		BAT2 = allow;
		break;
	case 3:
		BAT3 = allow;
		break;
	case 4:
		BAT4 = allow;
		break;
	case 5:
		BAT5 = allow;
		break;
	case 6:
		BAT6 = allow;
		break;
	default:
		break;
	}
}

