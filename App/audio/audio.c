/*
 * audio.c
 *
 *  Created on: 2020��6��20��
 *      Author: loyer
 */
#include "audio.h"

void Init_Audio(void)
{
	GPIO_InitTypeDef GPIO_InitStructure; //����ṹ�����
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;  //ѡ����Ҫ���õ�IO��
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	 //�����������ģʽ
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	  //���ô�������
	GPIO_Init(GPIOA, &GPIO_InitStructure); /* ��ʼ��GPIO */

	//��ʼ���õͲ�����
	GPIO_ResetBits(GPIOA, GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7);

}
/**
 * ��������ģ����ĵڼ���
 * @aud �ڼ�������
 */
void play_audio(u8 aud)
{
	u8 i;
	TRIG2_H;
	delay_us(100);
	TRIG2_L;
	delay_us(100);
	for(i = 0; i < aud; i++)
	{
		TRIG1_H;
		delay_us(100);
		TRIG1_L;
	}
}

