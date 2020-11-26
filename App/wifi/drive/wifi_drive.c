/*
 * wifi_drive.c
 *
 *  Created on: 2020��6��10��
 *      Author: loyer
 */
#include "wifi_drive.h"
#include "systick.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "wifi_public.h"
#include "sta_tcpclent_test.h"

struct STRUCT_USART_Fram WIFI_Fram_Record_Struct =
{ 0 };

void UART4_Init(u32 bound)
{

	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; //PC10
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//�����������
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;	//PC11
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;	//��������
	GPIO_Init(GPIOC, &GPIO_InitStructure);	//��ʼ��GPIOC 11

	//Uart4 NVIC ����
	NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;	//��ռ���ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//�����ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���

	//UART4 ��ʼ������
	USART_InitStructure.USART_BaudRate = bound;	//���ڲ�����
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;	//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;	//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;	//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl =
	USART_HardwareFlowControl_None;	//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
	USART_Init(UART4, &USART_InitStructure); //��ʼ������4

	USART_ITConfig(UART4, USART_IT_RXNE | USART_IT_IDLE, ENABLE); //�������ڽ��ܺ����߿����ж�

	USART_Cmd(UART4, ENABLE);                    //ʹ�ܴ���4
}

void UART4_IRQHandler(void)
{
	u8 ucCh;

	if (USART_GetITStatus( UART4, USART_IT_RXNE) != RESET)
	{
		ucCh = USART_ReceiveData( UART4);

		/**
		 * ��������

		 if(ucCh == 0x33) {
		 USART_SendData(UART4, 0x35);
		 while (USART_GetFlagStatus(UART4, USART_FLAG_TC) != SET)
		 ;
		 }
		 */

		if (WIFI_Fram_Record_Struct.InfBit.FramLength < ( RX_BUF_MAX_LEN - 1))
		{
			//Ԥ��1���ֽ�д������
			WIFI_Fram_Record_Struct.Data_RX_BUF[WIFI_Fram_Record_Struct.InfBit.FramLength++] =
					ucCh;
		}
	}

	if (USART_GetITStatus( UART4, USART_IT_IDLE) == SET)              //����֡�������
	{
		WIFI_Fram_Record_Struct.InfBit.FramFinishFlag = 1;

		ucCh = USART_ReceiveData( UART4); //�������������жϱ�־λ(�ȶ�USART_SR��Ȼ���USART_DR)

		TcpClosedFlag =
				strstr(WIFI_Fram_Record_Struct.Data_RX_BUF, "CLOSED\r\n") ?
						1 : 0;

	}
}
void WIFI_Init(u32 bound)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(
			WIFI_CH_PD_Pin_Periph_Clock | WIFI_RST_Pin_Periph_Clock, ENABLE);

	GPIO_InitStructure.GPIO_Pin = WIFI_CH_PD_Pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 	 //�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	 //IO���ٶ�Ϊ50MHz
	GPIO_Init(WIFI_CH_PD_Pin_Port, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = WIFI_RST_Pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 	 //�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	 //IO���ٶ�Ϊ50MHz
	GPIO_Init(WIFI_RST_Pin_Port, &GPIO_InitStructure);

	UART4_Init(bound);
	WIFI_RST_Pin_SetH;
//	WIFI_CH_PD_Pin_SetL;
}

void Wifi_Rst(void)
{
	WIFI_RST_Pin_SetL;
	delay_ms(500);
	WIFI_RST_Pin_SetH;
}

//��wifiģ�鷢��ATָ��
// cmd�������͵�ָ��
// ack1��ack2���ڴ�����Ӧ��ΪNULL������Ӧ������Ϊ���߼���ϵ
// time���ȴ���Ӧ��ʱ��
//����1�����ͳɹ� 0��ʧ��
bool WIFI_Send_AT_Cmd(char *cmd, char *ack1, char *ack2, u32 time)
{
	WIFI_Fram_Record_Struct.InfBit.FramLength = 0;	//���¿�ʼ�����µ����ݰ�
	WIFI_USART("%s\r\n", cmd);
	if (ack1 == 0 && ack2 == 0)	 //����Ҫ��������
	{
		return true;
	}
	delay_ms(time);	  //��ʱtimeʱ��

	WIFI_Fram_Record_Struct.Data_RX_BUF[WIFI_Fram_Record_Struct.InfBit.FramLength] =
			'\0';

	PC_USART("%s", WIFI_Fram_Record_Struct.Data_RX_BUF);

	if (ack1 != 0 && ack2 != 0)
	{
		return (( bool ) strstr(WIFI_Fram_Record_Struct.Data_RX_BUF, ack1)
				|| ( bool ) strstr(WIFI_Fram_Record_Struct.Data_RX_BUF, ack2));
	}
	else if (ack1 != 0)
		return (( bool ) strstr(WIFI_Fram_Record_Struct.Data_RX_BUF, ack1));

	else
		return (( bool ) strstr(WIFI_Fram_Record_Struct.Data_RX_BUF, ack2));

}
void WIFI_AT_Test(void)
{
	char count = 0;
	WIFI_RST_Pin_SetH;
	delay_ms(1000);
	while (count < 10)
	{
		if (WIFI_Send_AT_Cmd("AT", "OK", NULL, 500))
		{
			printf("WIFI���Գɹ�\r\n");
			return;
		}
		Wifi_Rst();
		++count;
	}
}

//ѡ��wifiģ��Ĺ���ģʽ
// enumMode������ģʽ
//����1��ѡ��ɹ� 0��ѡ��ʧ��
bool WIFI_Net_Mode_Choose(ENUM_Net_ModeTypeDef enumMode)
{
	switch (enumMode)
	{
	case STA:
		return WIFI_Send_AT_Cmd("AT+CWMODE=1", "OK", "no change", 2500);

	case AP:
		return WIFI_Send_AT_Cmd("AT+CWMODE=2", "OK", "no change", 2500);

	case STA_AP:
		return WIFI_Send_AT_Cmd("AT+CWMODE=3", "OK", "no change", 2500);

	default:
		return false;
	}
}

//wifiģ�������ⲿWiFi
//pSSID��WiFi�����ַ���
//pPassWord��WiFi�����ַ���
//����1�����ӳɹ� 0������ʧ��
bool WIFI_JoinAP(char * pSSID, char * pPassWord)
{
	char cCmd[120];

	sprintf(cCmd, "AT+CWJAP=\"%s\",\"%s\"", pSSID, pPassWord);

	return WIFI_Send_AT_Cmd(cCmd, "OK", NULL, 5000);

}

//wifiģ������������
//enumEnUnvarnishTx�������Ƿ������
//����1�����óɹ� 0������ʧ��
bool WIFI_Enable_MultipleId(FunctionalState enumEnUnvarnishTx)
{
	char cStr[20];

	sprintf(cStr, "AT+CIPMUX=%d", (enumEnUnvarnishTx ? 1 : 0));

	return WIFI_Send_AT_Cmd(cStr, "OK", 0, 500);
}

//wifiģ�������ⲿ������
//enumE������Э��
//ip��������IP�ַ���
//ComNum���������˿��ַ���
//id��ģ�����ӷ�������ID
//����1�����ӳɹ� 0������ʧ��
bool WIFI_Link_Server(ENUM_NetPro_TypeDef enumE, char * ip, char * ComNum,
		ENUM_ID_NO_TypeDef id)
{
	char cStr[100] =
	{ 0 }, cCmd[120];

	switch (enumE)
	{
	case enumTCP:
		sprintf(cStr, "\"%s\",\"%s\",%s", "TCP", ip, ComNum);
		break;

	case enumUDP:
		sprintf(cStr, "\"%s\",\"%s\",%s", "UDP", ip, ComNum);
		break;

	default:
		break;
	}

	if (id < 5)
		sprintf(cCmd, "AT+CIPSTART=%d,%s", id, cStr);

	else
		sprintf(cCmd, "AT+CIPSTART=%s", cStr);

	return WIFI_Send_AT_Cmd(cCmd, "OK", "ALREAY CONNECT", 4000);

}

//����wifiģ�����͸������
//����1�����óɹ� 0������ʧ��
bool WIFI_UnvarnishSend(void)
{
	if (!WIFI_Send_AT_Cmd("AT+CIPMODE=1", "OK", 0, 500))
		return false;

	return WIFI_Send_AT_Cmd("AT+CIPSEND", "OK", ">", 500);

}

//wifiģ�鷢���ַ���
//enumEnUnvarnishTx�������Ƿ���ʹ����͸��ģʽ
//pStr��Ҫ���͵��ַ���
//ulStrLength��Ҫ���͵��ַ������ֽ���
//ucId���ĸ�ID���͵��ַ���
//����1�����ͳɹ� 0������ʧ��
bool WIFI_SendString(FunctionalState enumEnUnvarnishTx, char * pStr,
		u32 ulStrLength, ENUM_ID_NO_TypeDef ucId)
{
	char cStr[20];
	bool bRet = false;

	if (enumEnUnvarnishTx)
	{
		WIFI_USART("%s", pStr);

		bRet = true;

	}

	else
	{
		if (ucId < 5)
			sprintf(cStr, "AT+CIPSEND=%d,%d", ucId,
					(unsigned int) ulStrLength + 2);

		else
			sprintf(cStr, "AT+CIPSEND=%d", (unsigned int) ulStrLength + 2);

		WIFI_Send_AT_Cmd(cStr, "> ", 0, 1000);

		bRet = WIFI_Send_AT_Cmd(pStr, "SEND OK", 0, 1000);
	}

	return bRet;

}

//wifiģ���˳�͸��ģʽ
void WIFI_ExitUnvarnishSend(void)
{
	delay_ms(1000);
	WIFI_USART("+++");
	delay_ms(500);
}

//wifi ������״̬�����ʺϵ��˿�ʱʹ��
//����0����ȡ״̬ʧ��
//����2�����ip
//����3����������
//����4��ʧȥ����
u8 WIFI_Get_LinkStatus(void)
{
	if (WIFI_Send_AT_Cmd("AT+CIPSTATUS", "OK", 0, 500))
	{
		if (strstr(WIFI_Fram_Record_Struct.Data_RX_BUF, "STATUS:2\r\n"))
			return 2;

		else if (strstr(WIFI_Fram_Record_Struct.Data_RX_BUF, "STATUS:3\r\n"))
			return 3;

		else if (strstr(WIFI_Fram_Record_Struct.Data_RX_BUF, "STATUS:4\r\n"))
			return 4;

	}

	return 0;

}
