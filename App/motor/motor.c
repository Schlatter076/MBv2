/*
 * motor.c
 *
 *  Created on: 2020��6��17��
 *      Author: loyer
 */
#include "motor.h"

void Init_Motor(void)
{
	GPIO_InitTypeDef GPIO_InitStructure; //����ṹ�����

	RCC_APB2PeriphClockCmd(
	RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14
			| GPIO_Pin_15;  //ѡ����Ҫ���õ�IO��
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	 //�����������ģʽ
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	  //���ô�������
	GPIO_Init(GPIOC, &GPIO_InitStructure); /* ��ʼ��GPIO */

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5
			| GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;  //ѡ����Ҫ���õ�IO��
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	 //�����������ģʽ
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	  //���ô�������
	GPIO_Init(GPIOB, &GPIO_InitStructure); /* ��ʼ��GPIO */

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;  //ѡ����Ҫ���õ�IO��
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	 //�����������ģʽ
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	  //���ô�������
	GPIO_Init(GPIOD, &GPIO_InitStructure); /* ��ʼ��GPIO */

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	//�ر�JTAG������SWD���ͷ�PB3 PB4 PA15
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

	//ʹȫ���������
	GPIO_ResetBits(GPIOB, 0x3F8); //PB3-9
	GPIO_ResetBits(GPIOC, 0xF000); //PC12-15
	GPIO_ResetBits(GPIOD, GPIO_Pin_2);

	//GPIO_SetBits(GPIOB, GPIO_Pin_4);

}
/**
 * ʹ��ﱣ�ֿ�����ס��籦��״̬
 */
void motor_reset(void)
{
	u8 index;
	for (index = 0; index < 6; index++)
	{
		if (!IsMotorArravalsOrigin(index + 1))
		{
			motor_run(index + 1, BACKWARD);
			while (IsMotorArravalsOrigin(index + 1))
				;
			motor_stop(index + 1);
		}
	}
}
/**
 * �������ת��
 * @mot ����1-6�������1-6
 * @dir 1--��ת  0--��ת(������ʱʹ�ú궨��FORWARD��BACKWARD)
 */
void motor_run(u8 mot, u8 dir)
{
	switch (mot)
	{
	case 1:
		PC_out(15) = dir;
		PC_out(14) = dir ? 0 : 1;
		break;
	case 2:
		PC_out(13) = dir;
		PB_out(9) = dir ? 0 : 1;
		break;
	case 3:
		PB_out(8) = dir;
		PB_out(7) = dir ? 0 : 1;
		break;
	case 4:
		PB_out(6) = dir;
		PB_out(5) = dir ? 0 : 1;
		break;
	case 5:
		PB_out(4) = dir;
		PB_out(3) = dir ? 0 : 1;
		break;
	case 6:
		PD_out(2) = dir;
		PC_out(12) = dir ? 0 : 1;
		break;
	default:
		break;
	}
}
/**
 * ������籦
 * @powerBank 1-6��ʾ��籦1-6
 * @play ��������
 */
void popUP_powerBank(u8 powerBank, u8 play)
{
	u16 cnt;
	if (powerBank >= 1 && powerBank <= 6 && play)
	{
		play_audio(1);
	}
	switch (powerBank)
	{
	case 1:
		PC_out(15) = 1;
		PC_out(14) = 0;
		for (cnt = 0; cnt < RegisterParams.motor_TCtime; cnt++)
		{
			delay_ms(100);
		}
		PC_out(15) = 0;
		PC_out(14) = 1;
		for (cnt = 0; cnt < RegisterParams.motor_HTtime; cnt++)
		{
			delay_ms(100);
		}
		break;
	case 2:
		PC_out(13) = 1;
		PB_out(9) = 0;
		for (cnt = 0; cnt < RegisterParams.motor_TCtime; cnt++)
		{
			delay_ms(100);
		}
		PC_out(13) = 0;
		PB_out(9) = 1;
		for (cnt = 0; cnt < RegisterParams.motor_HTtime; cnt++)
		{
			delay_ms(100);
		}
		break;
	case 3:
		PB_out(8) = 1;
		PB_out(7) = 0;
		for (cnt = 0; cnt < RegisterParams.motor_TCtime; cnt++)
		{
			delay_ms(100);
		}
		PB_out(8) = 0;
		PB_out(7) = 1;
		for (cnt = 0; cnt < RegisterParams.motor_HTtime; cnt++)
		{
			delay_ms(100);
		}
		break;
	case 4:
		PB_out(6) = 1;
		PB_out(5) = 0;
		for (cnt = 0; cnt < RegisterParams.motor_TCtime; cnt++)
		{
			delay_ms(100);
		}
		PB_out(6) = 0;
		PB_out(5) = 1;
		for (cnt = 0; cnt < RegisterParams.motor_HTtime; cnt++)
		{
			delay_ms(100);
		}
		break;
	case 5:
		PB_out(4) = 1;
		PB_out(3) = 0;
		for (cnt = 0; cnt < RegisterParams.motor_TCtime; cnt++)
		{
			delay_ms(100);
		}
		PB_out(4) = 0;
		PB_out(3) = 1;
		for (cnt = 0; cnt < RegisterParams.motor_HTtime; cnt++)
		{
			delay_ms(100);
		}
		break;
	case 6:
		PD_out(2) = 1;
		PC_out(12) = 0;
		for (cnt = 0; cnt < RegisterParams.motor_TCtime; cnt++)
		{
			delay_ms(100);
		}
		PD_out(2) = 0;
		PC_out(12) = 1;
		for (cnt = 0; cnt < RegisterParams.motor_HTtime; cnt++)
		{
			delay_ms(100);
		}
		break;
	default:
		break;
	}
	motor_stop(powerBank);
}

void popUP_All(void)
{
	printf("popUP ALL\r\n");
	for (int i = 1; i < 7; i++)
	{
		motor_run(i, 1);
	}
	delay_ms(1000);
	delay_ms(1000);
	delay_ms(1000);
	for (int i = 1; i < 7; i++)
	{
		motor_run(i, 0);
	}
	delay_ms(1000);
	delay_ms(1500);
	for (int i = 1; i < 7; i++)
	{
		motor_stop(i);
	}
}

/**
 * ʹ���ֹͣת��
 * @mot 1-6��ʾ���1-6
 */
void motor_stop(u8 mot)
{
	switch (mot)
	{
	case 1:
		PC_out(15) = 0;
		PC_out(14) = 0;
		break;
	case 2:
		PC_out(13) = 0;
		PB_out(9) = 0;
		break;
	case 3:
		PB_out(8) = 0;
		PB_out(7) = 0;
		break;
	case 4:
		PB_out(6) = 0;
		PB_out(5) = 0;
		break;
	case 5:
		PB_out(4) = 0;
		PB_out(3) = 0;
		break;
	case 6:
		PD_out(2) = 0;
		PC_out(12) = 0;
		break;
	default:
		break;
	}
}
/**
 * ����Ƿ񵽴�ԭ��
 * @mot 1-6�������1-6
 * @return ��0--����   0--δ��
 */
u8 IsMotorArravalsOrigin(u8 mot)
{
	key_statu = (key_statu >> 8) & 0xFF; //ȡ����㶯���ض�Ӧ��ֵ
	return (key_statu & (1 << (mot - 1)));
}
