/*
 * usart.h
 *
 *  Created on: 2019��10��19��
 *      Author: Loyer
 */

#ifndef USART_H_
#define USART_H_

#include "bit_band.h"
#include "STMFlash.h"

#define RX_BUF_MAX_LEN 1024		  //�����ջ����ֽ���
extern struct USART_Fram	  //����һ��ȫ�ִ�������֡�Ĵ���ṹ��
{
	char RX_BUF[RX_BUF_MAX_LEN];
	union
	{
		__IO u16 InfAll;
		struct
		{
			__IO u16 FramLength :15;                               // 14:0
			__IO u16 FramFinishFlag :1;                                // 15
		} InfBit;
	};
} USART1_Record_Struct;

extern struct RegisterFram	  //����һ��ȫ�ִ�������֡�Ĵ���ṹ��
{
	char key[16];
	u8 motor_TCtime;
	u8 motor_HTtime;
	char ip[30];
	u8 needConfirmParams;
	u8 heartTime;
	u8 statuHeartTime;
	u8 allowHeart;
	char port[5];
} RegisterParams;

void USART1_Init(u32 bound);
u8 hexStr2Byte(char *hexStr);

#endif /* USART_H_ */
