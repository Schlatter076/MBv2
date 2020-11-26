/*
 * wifi_drive.h
 *
 *  Created on: 2020年6月10日
 *      Author: loyer
 */

#ifndef WIFI_DRIVE_WIFI_DRIVE_H_
#define WIFI_DRIVE_WIFI_DRIVE_H_

#include "bit_band.h"
#include <stdio.h>
#include <stdbool.h>

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif

//wifi数据类型定义
typedef enum
{
	STA, AP, STA_AP
} ENUM_Net_ModeTypeDef;

typedef enum
{
	enumTCP, enumUDP,
} ENUM_NetPro_TypeDef;

typedef enum
{
	Multiple_ID_0 = 0,
	Multiple_ID_1 = 1,
	Multiple_ID_2 = 2,
	Multiple_ID_3 = 3,
	Multiple_ID_4 = 4,
	Single_ID_0 = 5,
} ENUM_ID_NO_TypeDef;

#define WIFI_CH_PD_Pin     GPIO_Pin_9
#define WIFI_CH_PD_Pin_Port     GPIOC
#define WIFI_CH_PD_Pin_Periph_Clock  RCC_APB2Periph_GPIOC

#define WIFI_RST_Pin     GPIO_Pin_2
#define WIFI_RST_Pin_Port     GPIOC
#define WIFI_RST_Pin_Periph_Clock  RCC_APB2Periph_GPIOC

#define WIFI_CH_PD_Pin_SetH     GPIO_SetBits(WIFI_CH_PD_Pin_Port,WIFI_CH_PD_Pin)
#define WIFI_CH_PD_Pin_SetL     GPIO_ResetBits(WIFI_CH_PD_Pin_Port,WIFI_CH_PD_Pin)
#define WIFI_RST_Pin_SetH     GPIO_SetBits(WIFI_RST_Pin_Port,WIFI_RST_Pin)
#define WIFI_RST_Pin_SetL     GPIO_ResetBits(WIFI_RST_Pin_Port,WIFI_RST_Pin)

#define WIFI_USART(fmt, ...)	 Wifi_USART_printf (UART4, fmt, ##__VA_ARGS__)
#define PC_USART(fmt, ...)	 printf (fmt, ##__VA_ARGS__)

#define RX_BUF_MAX_LEN 1024		  //最大接收缓存字节数
extern struct STRUCT_USART_Fram	  //定义一个全局串口数据帧的处理结构体
{
	char Data_RX_BUF[RX_BUF_MAX_LEN];
	union
	{
		__IO u16 InfAll;
		struct
		{
			__IO u16 FramLength :15;                               // 14:0
			__IO u16 FramFinishFlag :1;                                // 15
		} InfBit;
	};
} WIFI_Fram_Record_Struct;

void UART4_Init(u32 bound);
void WIFI_Init(u32 bound);
bool WIFI_Send_AT_Cmd(char *cmd, char *ack1, char *ack2, u32 time);
void WIFI_AT_Test(void);
bool WIFI_Net_Mode_Choose(ENUM_Net_ModeTypeDef enumMode);
bool WIFI_JoinAP(char * pSSID, char * pPassWord);
bool WIFI_Enable_MultipleId(FunctionalState enumEnUnvarnishTx);
bool WIFI_Link_Server(ENUM_NetPro_TypeDef enumE, char * ip, char * ComNum, ENUM_ID_NO_TypeDef id);
bool WIFI_UnvarnishSend(void);
bool WIFI_SendString(FunctionalState enumEnUnvarnishTx, char * pStr, u32 ulStrLength, ENUM_ID_NO_TypeDef ucId);
void WIFI_ExitUnvarnishSend(void);
u8 WIFI_Get_LinkStatus(void);



#endif /* WIFI_DRIVE_WIFI_DRIVE_H_ */
