/*
 * wifi_drive.c
 *
 *  Created on: 2020年6月10日
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
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;	//PC11
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;	//浮空输入
	GPIO_Init(GPIOC, &GPIO_InitStructure);	//初始化GPIOC 11

	//Uart4 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;	//抢占优先级0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//子优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器

	//UART4 初始化设置
	USART_InitStructure.USART_BaudRate = bound;	//串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;	//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;	//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;	//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl =
	USART_HardwareFlowControl_None;	//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
	USART_Init(UART4, &USART_InitStructure); //初始化串口4

	USART_ITConfig(UART4, USART_IT_RXNE | USART_IT_IDLE, ENABLE); //开启串口接受和总线空闲中断

	USART_Cmd(UART4, ENABLE);                    //使能串口4
}

void UART4_IRQHandler(void)
{
	u8 ucCh;

	if (USART_GetITStatus( UART4, USART_IT_RXNE) != RESET)
	{
		ucCh = USART_ReceiveData( UART4);

		/**
		 * 做测试用

		 if(ucCh == 0x33) {
		 USART_SendData(UART4, 0x35);
		 while (USART_GetFlagStatus(UART4, USART_FLAG_TC) != SET)
		 ;
		 }
		 */

		if (WIFI_Fram_Record_Struct.InfBit.FramLength < ( RX_BUF_MAX_LEN - 1))
		{
			//预留1个字节写结束符
			WIFI_Fram_Record_Struct.Data_RX_BUF[WIFI_Fram_Record_Struct.InfBit.FramLength++] =
					ucCh;
		}
	}

	if (USART_GetITStatus( UART4, USART_IT_IDLE) == SET)              //数据帧接收完毕
	{
		WIFI_Fram_Record_Struct.InfBit.FramFinishFlag = 1;

		ucCh = USART_ReceiveData( UART4); //由软件序列清除中断标志位(先读USART_SR，然后读USART_DR)

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
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 	 //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	 //IO口速度为50MHz
	GPIO_Init(WIFI_CH_PD_Pin_Port, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = WIFI_RST_Pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 	 //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	 //IO口速度为50MHz
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

//对wifi模块发送AT指令
// cmd：待发送的指令
// ack1，ack2：期待的响应，为NULL表不需响应，两者为或逻辑关系
// time：等待响应的时间
//返回1：发送成功 0：失败
bool WIFI_Send_AT_Cmd(char *cmd, char *ack1, char *ack2, u32 time)
{
	WIFI_Fram_Record_Struct.InfBit.FramLength = 0;	//从新开始接收新的数据包
	WIFI_USART("%s\r\n", cmd);
	if (ack1 == 0 && ack2 == 0)	 //不需要接收数据
	{
		return true;
	}
	delay_ms(time);	  //延时time时间

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
			printf("WIFI测试成功\r\n");
			return;
		}
		Wifi_Rst();
		++count;
	}
}

//选择wifi模块的工作模式
// enumMode：工作模式
//返回1：选择成功 0：选择失败
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

//wifi模块连接外部WiFi
//pSSID：WiFi名称字符串
//pPassWord：WiFi密码字符串
//返回1：连接成功 0：连接失败
bool WIFI_JoinAP(char * pSSID, char * pPassWord)
{
	char cCmd[120];

	sprintf(cCmd, "AT+CWJAP=\"%s\",\"%s\"", pSSID, pPassWord);

	return WIFI_Send_AT_Cmd(cCmd, "OK", NULL, 5000);

}

//wifi模块启动多连接
//enumEnUnvarnishTx：配置是否多连接
//返回1：配置成功 0：配置失败
bool WIFI_Enable_MultipleId(FunctionalState enumEnUnvarnishTx)
{
	char cStr[20];

	sprintf(cStr, "AT+CIPMUX=%d", (enumEnUnvarnishTx ? 1 : 0));

	return WIFI_Send_AT_Cmd(cStr, "OK", 0, 500);
}

//wifi模块连接外部服务器
//enumE：网络协议
//ip：服务器IP字符串
//ComNum：服务器端口字符串
//id：模块连接服务器的ID
//返回1：连接成功 0：连接失败
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

//配置wifi模块进入透传发送
//返回1：配置成功 0：配置失败
bool WIFI_UnvarnishSend(void)
{
	if (!WIFI_Send_AT_Cmd("AT+CIPMODE=1", "OK", 0, 500))
		return false;

	return WIFI_Send_AT_Cmd("AT+CIPSEND", "OK", ">", 500);

}

//wifi模块发送字符串
//enumEnUnvarnishTx：声明是否已使能了透传模式
//pStr：要发送的字符串
//ulStrLength：要发送的字符串的字节数
//ucId：哪个ID发送的字符串
//返回1：发送成功 0：发送失败
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

//wifi模块退出透传模式
void WIFI_ExitUnvarnishSend(void)
{
	delay_ms(1000);
	WIFI_USART("+++");
	delay_ms(500);
}

//wifi 的连接状态，较适合单端口时使用
//返回0：获取状态失败
//返回2：获得ip
//返回3：建立连接
//返回4：失去连接
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
