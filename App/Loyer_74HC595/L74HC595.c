/*
 * L74HC595.c
 *
 *  Created on: 2020��6��28��
 *      Author: loyer
 */
#include "L74HC595.h"

struct HC595Fram HC595_STATUS =
{ 0 };

void Init_74HC595(void)
{
	GPIO_InitTypeDef GPIO_InitStructure; //����ṹ�����
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_4;  //ѡ����Ҫ���õ�IO��
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	 //�����������ģʽ
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	  //���ô�������
	GPIO_Init(GPIOA, &GPIO_InitStructure); /* ��ʼ��GPIO */

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;  //ѡ����Ҫ���õ�IO��
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	 //�����������ģʽ
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	  //���ô�������
	GPIO_Init(GPIOC, &GPIO_InitStructure); /* ��ʼ��GPIO */

	_HC595_CLK = 0;
	_HC595_LATCH = 0;
	_HC595_DATA = 0;
	//�ư忪ʼ��������ĳһ����
	HC595_STATUS.allowSigleContrl = 1;
}
/***
 *74HC595 ����һ���ֽ�
 *����74HC595��SI���ŷ���һ���ֽ�
 */
void HC595_Send_Byte(u8 byte)
{
	u8 i;
	for (i = 0; i < 8; i++)  //һ���ֽ�8λ������8�Σ�һ��һλ��ѭ��8�Σ��պ�����8λ
	{
		/****  ����1�������ݴ���DS����    ****/
		if (byte & 0x80)        //�ȴ����λ��ͨ���������жϵڰ��Ƿ�Ϊ1
			_HC595_DATA = 1;    //����ڰ�λ��1������ 595 DS���ӵ���������ߵ�ƽ
		else
			//��������͵�ƽ
			_HC595_DATA = 0;

		/*** ����2��SHCPÿ����һ�������أ���ǰ��bit�ͱ�������λ�Ĵ��� ***/
		_HC595_CLK = 0;   // SHCP����
		delay_us(1);           // �ʵ���ʱ
		_HC595_CLK = 1;  // SHCP���ߣ� SHCP����������
		delay_us(1);
		byte <<= 1;		// ����һλ������λ����λ�ƣ�ͨ��	if (byte & 0x80)�жϵ�λ�Ƿ�Ϊ1
	}
	//�������ʹ��
	_HC595_LATCH = 0;
	delay_us(1);
	_HC595_LATCH = 1;
	delay_us(1);
}
/**
 * ����ĳһ��led��
 * @led 1-6��ʾ��1-6
 */
void ledON(u8 led)
{
	if (HC595_STATUS.allowSigleContrl)
	{
		HC595_STATUS.LAST_LED_STATU &= ~(1 << (7 - led)); //��ĳλ����
		HC595_Send_Byte(HC595_STATUS.LAST_LED_STATU);
	}
}

void ledOFF(u8 led)
{
	if (HC595_STATUS.allowSigleContrl)
	{
		HC595_STATUS.LAST_LED_STATU |= (1 << (7 - led)); //��ĳλ��λ
		HC595_Send_Byte(HC595_STATUS.LAST_LED_STATU);
	}
}

void ledBLINK(u8 led)
{
	if (HC595_STATUS.allowSigleContrl)
	{
		HC595_STATUS.LAST_LED_STATU ^= (1 << (7 - led)); //��ĳλ��λ
		HC595_Send_Byte(HC595_STATUS.LAST_LED_STATU);
	}
}

