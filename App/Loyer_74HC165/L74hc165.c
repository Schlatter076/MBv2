/*
 * L74hc165.c
 *
 *  Created on: 2020��6��17��
 *      Author: loyer
 */
#include "L74hc165.h"

volatile u16 key_statu = 0xFF;

void Init_74HC165(void)
{
	GPIO_InitTypeDef GPIO_InitStructure; //����ṹ�����
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;  //ѡ����Ҫ���õ�IO��
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	 //�����������ģʽ
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	  //���ô�������
	GPIO_Init(GPIOA, &GPIO_InitStructure); /* ��ʼ��GPIO */

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;  //ѡ����Ҫ���õ�IO��
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	 //�����������ģʽ
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	  //���ô�������
	GPIO_Init(GPIOA, &GPIO_InitStructure); /* ��ʼ��GPIO */

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;  //ѡ����Ҫ���õ�IO��
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	 //�������ݿ���������
	GPIO_Init(GPIOA, &GPIO_InitStructure); /* ��ʼ��GPIO */

//	_HC165_PL = 1;  //����ȡ״̬
//	_HC165_CP = 1;  //ʱ�����ߣ����ڲ���������
	read_74HC165();

}
u16 read_74HC165(void)
{
	u8 i;
	u16 temp = 0;
	static u16 old_statu = 0xffff;
	_HC165_PL = 1;
	delay_us(1);
	_HC165_PL = 0; //PL���ͣ���ȡ����״̬
	delay_us(1);
	_HC165_PL = 1; //PL���ߣ�ֹͣ������ȡ
	delay_us(1);
	_HC165_CP = 1;
	delay_us(1);
	if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12) == 0) //�͵�ƽ��ʾ������ͨ
		temp += 1;
	for (i = 0; i < 15; i++) //��Ϊ���λ������λ���ɶ�ȡ����ѭ��(16-1)�Σ����ΰѴθ�λ�Ƶ����λ����һ��74HC165���ȶ�ȡ
	{
		_HC165_CP = 0;  //Ϊ�Ͷ�ȡ����
		delay_us(1);
		_HC165_CP = 1;
		delay_us(1);
		temp = temp << 1;
		if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12) == 0)
			temp += 1;

	}
	_HC165_PL = 1;
	_HC165_CP = 1;
	temp &= 0x3f3f;
	if(old_statu != temp)
	{
		printf("key_statu=%04X\r\n", temp);
		old_statu = temp;
	}
	return temp;
}
