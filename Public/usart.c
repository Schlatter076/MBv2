#include "usart.h"
#include "stdio.h"
#include "usart_4G.h"
#include "string.h"
#include "base64.h"
#include "motor.h"
#include "audio.h"
#include "bat_usart.h"
#include "L74HC595.h"
#include "tim4.h"

struct USART_Fram USART1_Record_Struct =
{ 0 };
struct RegisterFram RegisterParams =
{ 0 };
//*/
/*******************************************************************************
 * �� �� ��         : USART1_Init
 * ��������		   : USART1��ʼ������
 * ��    ��         : bound:������
 * ��    ��         : ��
 *******************************************************************************/
void USART1_Init(u32 bound)
{
	//GPIO�˿�����
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	USART_DeInit(USART1);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	/*  ����GPIO��ģʽ��IO�� */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //TX			   //�������PA9
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	    //�����������
	GPIO_Init(GPIOA, &GPIO_InitStructure); /* ��ʼ����������IO */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;	    //RX			 //��������PA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;		  //ģ������
	GPIO_Init(GPIOA, &GPIO_InitStructure); /* ��ʼ��GPIO */

	//USART1 ��ʼ������
	USART_InitStructure.USART_BaudRate = bound;		  //����������
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;		 //�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;		  //һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;		  //����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl =
	USART_HardwareFlowControl_None;		  //��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
	USART_Init(USART1, &USART_InitStructure); //��ʼ������1

	USART_Cmd(USART1, ENABLE);  //ʹ�ܴ���1

	USART_ClearFlag(USART1, USART_FLAG_TC);

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);  //��������ж�

	//Usart1 NVIC ����
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;  //����1�ж�ͨ��
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;  //��ռ���ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ�����

	//��ʼ���ע�����
	ReadAPPServer();
	printf("RegisterParams=%s\r\n", APPServer);
	char *tem = NULL;
	char res[35] =
	{ 0 };
	tem = strtok(APPServer, "-");
	memcpy(RegisterParams.key, tem, 16); //ȡkey
	if (strcmp("aaaaaaaaaaaaaaaa", RegisterParams.key) == 0)
	{
		printf("key is right!\r\n");
	}
	tem = strtok(NULL, "-");
	strcpy(res, tem); //ָ��ip��port
	//ȡʣ�µĲ���
	tem = strtok(NULL, "-");
	RegisterParams.needConfirmParams = atoi(tem);
	tem = strtok(NULL, "-");
	RegisterParams.motor_TCtime = atoi(tem);
	tem = strtok(NULL, "-");
	RegisterParams.motor_HTtime = atoi(tem);
	//ȡip��port
	// tem = res;
	tem = strtok(res, ":");
	strcpy(RegisterParams.ip, tem);
	tem = strtok(NULL, ":");
	strncpy(RegisterParams.port, tem, 5);
	if (strncmp("13401", RegisterParams.port, 5) == 0)
	{
		printf("port is right!\r\n");
	}
	printf("key=%s,ip=%s,port=%s,confirm=%d,TCtime=%d,HTtime=%d\r\n",
			RegisterParams.key, RegisterParams.ip, RegisterParams.port,
			RegisterParams.needConfirmParams, RegisterParams.motor_TCtime,
			RegisterParams.motor_HTtime);
}
/*******************************************************************************
 * �� �� ��         : USART1_IRQHandler
 * ��������		   : USART1�жϺ���
 * ��    ��         : ��
 * ��    ��         : ��
 *******************************************************************************/
char usart1TestBuf[18] =
{ 0 };
void USART1_IRQHandler(void)                	//����1�жϷ������
{
	u8 r;
	char *res = NULL;
	u8 mot = 0;
	char buf[128];
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  //�����ж�
	{
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
		r = USART_ReceiveData(USART1);  //(USART1->DR);	//��ȡ���յ�������
		if (USART1_Record_Struct.InfBit.FramLength < (RX_BUF_MAX_LEN - 1))
		{
			USART1_Record_Struct.RX_BUF[USART1_Record_Struct.InfBit.FramLength++] =
					r;
		}
		if (r == ']')
		{
			WDeviceID = strtok(USART1_Record_Struct.RX_BUF, "]");
			printf("write DeviceID=%s\r\n", WDeviceID);
			STMFLASH_Write(EEPROM_ADDR, (u16 *) WDeviceID, 8);
//			STMFLASH_Read(EEPROM_ADDR, (u16 *) RDeviceID, 8);
//			printf("�ɹ�д���豸���:%s\r\n", RDeviceID);
			USART1_Record_Struct.InfBit.FramLength = 0; //���¿�ʼ����������
		}
		else if (r == '$')
		{
			res = strtok(USART1_Record_Struct.RX_BUF, "$");
			Send_AT_Cmd(res, "OK", NULL, 500);
			USART1_Record_Struct.InfBit.FramLength = 0; //���¿�ʼ����������
		}
		else if (r == '#')
		{
			res = strtok(USART1_Record_Struct.RX_BUF, "#");
			controlPowerbank(hexStr2Byte(res));
			USART1_Record_Struct.InfBit.FramLength = 0; //���¿�ʼ����������
		}
		else if (r == '%')
		{
			res = strtok(USART1_Record_Struct.RX_BUF, "%");
			USART1_Record_Struct.InfBit.FramLength = 0; //���¿�ʼ����������
			if (*res == 'l') //��ĳ����
			{
				mot = *(res + 1) - 0x30;
				ledON(mot);
			}
			else if (*res == 'd') //��ĳ����
			{
				mot = *(res + 1) - 0x30;
				ledOFF(mot);
			}
			else if (*res == 'a') //ֱ�Ӳ���ȫ��
			{
				res++;
				HC595_Send_Byte(hexStr2Byte(res));
			}
			else if (*res == 'u') //�������ģ��̼�
			{
				allowModuleUpdate = 1;
			}
			else if (*res == 'c') //��ȡ4Gģ�����Ϣ
			{
				allowModuleUpdate = 0;
			}
			else if (*res == 'r') //��ȡ4Gģ�����Ϣ
			{
				PC_USART("%s", F4G_Fram_Record_Struct.Data_RX_BUF);
				F4G_Fram_Record_Struct.InfBit.FramLength = 0;
				memset(F4G_Fram_Record_Struct.Data_RX_BUF, '\0', RX4G_BUF_MAX_LEN);
			}
		}
		else if (r == '@')
		{
			res = strtok(USART1_Record_Struct.RX_BUF, "@");
			USART1_Record_Struct.InfBit.FramLength = 0; //���¿�ʼ����������
			if (*res == 't')
			{
				mot = *(res + 1) - 0x30;
				printf("Trig the Audio_%d.\r\n", mot);
				play_audio(mot);
			}
			else if (*res == '&')
			{
				res++;
//				statu_heart("90");
//				for(int i = 0; i <6; i++)
//				{
//					checkPowerbankStatus(i, usart1TestBuf);
//				}
				checkPowerbankStatus((u8) atoi(res) - 1, usart1TestBuf);
				//communicationTest(atoi(res) - 1);
				USART1_Record_Struct.InfBit.FramLength = 0; //���¿�ʼ����������
			}
			else if (*res == 'e')
			{
				F4G_ExitUnvarnishSend();
			}
			else if (*res == 's')
			{
				res++;
				base64_encode((const unsigned char *) res, buf);
				F4G_sendStr(buf);
			}
			else if (*res == 'f')
			{
				mot = *(res + 1) - 0x30;
				printf("now motor forward=%d\r\n", mot);
				motor_run(mot, FORWARD);
			}
			else if (*res == 'b')
			{
				mot = *(res + 1) - 0x30;
				printf("now motor backward=%d\r\n", mot);
				motor_run(mot, BACKWARD);
			}
			else if (*res == 'p')
			{
				mot = *(res + 1) - 0x30;
				printf("now motor stop=%d\r\n", mot);
				motor_stop(mot);
			}
			else if (*res == 'r')
			{
				STMFLASH_Read(EEPROM_ADDR, (u16 *) RDeviceID, 8);
				printf("read DeviceID=%s\r\n", RDeviceID);
				USART1_Record_Struct.InfBit.FramLength = 0; //���¿�ʼ����������
			}
			else if (*res == 'x')
			{
				printf("now Exchange TX3_RX3\r\n");
				TX3_RX3_Exchange;
			}
			else if (*res == 'y')
			{
				mot = *(res + 1) - 0x30;
				communicateWithPort(mot);
			}
		}
	}
	if (USART_GetFlagStatus(USART1, USART_FLAG_ORE) != RESET)
	{
		USART_ClearFlag(USART1, USART_FLAG_ORE);
		USART_ReceiveData(USART1);
	}
}

u8 hexStr2Byte(char *hexStr)
{
	unsigned char highByte, lowByte;
	highByte = *hexStr;
	lowByte = *(hexStr + 1);

	if (highByte > 0x39)
		highByte -= 0x37;
	else
		highByte -= 0x30;

	if (lowByte > 0x39)
		lowByte -= 0x37;
	else
		lowByte -= 0x30;

	return (u8) ((highByte << 4) | lowByte);
}

