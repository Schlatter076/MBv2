/*
 * usart_4G.c
 *
 *  Created on: 2020��6��11��
 *      Author: loyer
 */
#include "usart_4G.h"

struct STRUCT_USART_Fr F4G_Fram_Record_Struct =
{ 0 };

struct F4G_Params_Fram F4G_Params =
{ 0 };

void USART2_Init(uint32_t bound)
{
	USART_DeInit(USART2);

	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PA2  TXD
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//�����������
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;	//PA3  RXD
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;	//��������
	GPIO_Init(GPIOA, &GPIO_InitStructure);	//��ʼ��GPIOA3

	//Usart2 NVIC ����
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;	//��ռ���ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		//�����ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���

	//USART2 ��ʼ������
	USART_InitStructure.USART_BaudRate = bound;	//���ڲ�����
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;	//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;	//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;	//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl =
	USART_HardwareFlowControl_None;	//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
	USART_Init(USART2, &USART_InitStructure); //��ʼ������2

	USART_ITConfig(USART2, USART_IT_RXNE | USART_IT_IDLE, ENABLE); //�������ڽ��ܺ����߿����ж�

	USART_Cmd(USART2, ENABLE);
}

void F4G_Init(u32 bound)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	F4G_Fram_Record_Struct.init = 1;
	F4G_Fram_Record_Struct.allowProcessServerData = 0;

	USART2_Init(bound);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = PKEY_4G | RST_4G;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 	 //�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	 //IO���ٶ�Ϊ50MHz

	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//����
	do
	{
		PKEY_4G_Pin_SetH;
		delay_ms(1000);
		delay_ms(1000);
		delay_ms(1000);
		//delay_ms(1000);
		PKEY_4G_Pin_SetL;
		//��λ4Gģ��
		RST_4G_Pin_SetH;
		delay_ms(1100);
		RST_4G_Pin_SetL;
		delay_ms(500);

		F4G_ExitUnvarnishSend();
		Send_AT_Cmd("AT+CIPCLOSE", "OK", NULL, 500);
		Send_AT_Cmd("AT+RSTSET", "OK", NULL, 500);
	} while (!F4G_AT_Test());

	//��ȡ4Gģ�������Ϣ
	getModuleMes();
	//���ӵ�������
	if (RegisterParams.ip != NULL && RegisterParams.port != NULL)
	{
		while (!connectToTCPServer(RegisterParams.ip, RegisterParams.port))
			;
	}
	else
	{
		while (!connectToTCPServer(TCPServer_IP, TCPServer_PORT))
			;
	}
	u8 request_cnt = 0;
	//�ϵ�ע��
	while (1)
	{
		if (request_cnt >= 3)
		{
			NVIC_SystemReset(); //����
			//RunApp();
		}
		request(REQ_REGISTER);
		request_cnt++;
		delay_ms(500);
		if (F4G_Fram_Record_Struct.InfBit.FramFinishFlag)
		{
			F4G_Fram_Record_Struct.InfBit.FramFinishFlag = 0;
			split(F4G_Fram_Record_Struct.DeData, ",");
			//ע��ɹ�
			if (memcmp(F4G_Fram_Record_Struct.Server_Command[1],
			RES_REGISTER, 2) == 0)
			{
				char *tem = F4G_Fram_Record_Struct.ServerData;
				tem = strtok(F4G_Fram_Record_Struct.ServerData, "_");
				RegisterParams.heartTime = atoi(tem);
				tem = strtok(NULL, "_");
				RegisterParams.statuHeartTime = atoi(tem);
				printf("ht=%d,sht=%d\r\n", RegisterParams.heartTime,
						RegisterParams.statuHeartTime);
				break;
			}
		}
	}
	F4G_Fram_Record_Struct.registerSuccess = 1;
	RegisterParams.allowHeart = 1;
	F4G_Fram_Record_Struct.allowProcessServerData = 1;
//	strncpy(F4G_Params.htCMD, "32", 2);
//	F4G_Params.STHeart = 1;
}

void USART2_IRQHandler(void)
{
	u8 ucCh;
	if (USART_GetITStatus( USART2, USART_IT_RXNE) != RESET)
	{
		USART_ClearITPendingBit(USART2, USART_IT_RXNE);
		ucCh = USART_ReceiveData( USART2);

		if (F4G_Fram_Record_Struct.InfBit.FramLength < (RX4G_BUF_MAX_LEN - 1))
		{
			F4G_Fram_Record_Struct.Data_RX_BUF[F4G_Fram_Record_Struct.InfBit.FramLength++] =
					ucCh;
		}
		else
		{
			printf("4G Cmd size over.\r\n");
			memset(F4G_Fram_Record_Struct.Data_RX_BUF, 0, RX4G_BUF_MAX_LEN);
			F4G_Fram_Record_Struct.InfBit.FramLength = 0;
		}
		//�յ��������˷��ص�����
		if (ucCh == ']'
				&& (bool) strchr(F4G_Fram_Record_Struct.Data_RX_BUF, '['))
		{
			char *res = F4G_Fram_Record_Struct.Data_RX_BUF;
			F4G_Params.serverStatuCnt = 0;
			while (*res != '[')
			{
				res++;
			}
			while (*res == '[')
				res++;

			F4G_Fram_Record_Struct.base64Str = strtok(res, "]");
//			printf("before Decryption=%s\r\n",
//					F4G_Fram_Record_Struct.base64Str);
			base64_decode((const char *) F4G_Fram_Record_Struct.base64Str,
					(unsigned char *) F4G_Fram_Record_Struct.DeData);
			printf("after Decryption=%s\r\n", F4G_Fram_Record_Struct.DeData);
			//split(F4G_Fram_Record_Struct.DeData, ",");
			memset(F4G_Fram_Record_Struct.Data_RX_BUF, '\0', RX4G_BUF_MAX_LEN);
			F4G_Fram_Record_Struct.InfBit.FramLength = 0;
			F4G_Fram_Record_Struct.InfBit.FramFinishFlag = 1;
		}
		//=====================================================================
		if (F4G_Fram_Record_Struct.allowProcessServerData)
		{
			if (F4G_Fram_Record_Struct.InfBit.FramFinishFlag)
			{
				F4G_Fram_Record_Struct.InfBit.FramFinishFlag = 0;
				split(F4G_Fram_Record_Struct.DeData, ",");
				//��������
				if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "33", 2)
						== 0)
				{
					F4G_Params.serverStatuCnt = 0;
					F4G_Fram_Record_Struct.firstStatuHeartNotSucc = 0;
					F4G_Params.allowCheckChange = 1;
				}
				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "91",
						2) == 0)
				{
					F4G_Params.serverStatuCnt = 0;
				}
				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "94",
						2) == 0)
				{
					F4G_Params.serverStatuCnt = 0;
				}
				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "34",
						2) == 0)
				{
					//������籦
					char *tem = strtok(F4G_Fram_Record_Struct.ServerData, "-");

					F4G_Params.port = atoi(tem);
					tem = strtok(NULL, "-");
					F4G_Params.play = (u8) atoi(tem);
					strcpy(F4G_Params.cmd, REQ_TC_POWERBANK);
					F4G_Params.sysTC = 1;
				}
				//�յ��ϱ�����Ϣ�Ļظ�
				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "36",
						2) == 0)
				{
					if (F4G_Params.play)
					{
						play_audio(2);
					}
				}
				//�黹�ɹ�
				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "38",
						2) == 0)
				{
					if (F4G_Params.play)
					{
						play_audio(3); //�黹�ɹ�
					}
				}
				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "71",
						2) == 0)
				{
					if (atoi(F4G_Fram_Record_Struct.ServerData) == 1)
					{
						play_audio(2); //���ɹ�
					}
					else if (atoi(F4G_Fram_Record_Struct.ServerData) == 2)
					{
						play_audio(3); //�黹�ɹ�
					}
				}
				//ǿ������
				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "92",
						2) == 0)
				{
					strcpy(F4G_Params.htCMD, "93");
					F4G_Params.reqSTheart = 1;
				}
				//�����豸
				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "60",
						2) == 0)
				{
					response2reset("61");
					NVIC_SystemReset();
				}
				//����������籦
				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "40",
						2) == 0)
				{
					//������籦
					char *temp = strtok(F4G_Fram_Record_Struct.ServerData, "-");

					F4G_Params.port = atoi(temp);
					temp = strtok(NULL, "-");
					strcpy(F4G_Params.dd, temp);
					temp = strtok(NULL, "-");
					F4G_Params.play = (u8) atoi(temp);
					strcpy(F4G_Params.cmd, "41");

					F4G_Params.ddTC = 1;
					//					popUP_powerBank(F4G_Params.port + 1, 1);
					//					dd_tc(F4G_Params.port, "41");

				}
				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "42",
						2) == 0)
				{
					play_audio(2);
				}
//				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "FE",
//						2) == 0)
//				{
//					//��ȡ��γ��
//					F4G_Params.getLongitude = 1;
//				}
//				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "11",
//						2) == 0)
//				{
//					//����籦����ID
//
//				}
				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "99",
						2) == 0)
				{
					//�������г�籦
					F4G_Params.popAll = 1;
					//popUP_All();
				}
				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "77",
						2) == 0)
				{
					//�޸ĺ���ĳ�������쳣״̬
					//�ӿڲ���������_״̬��1��ʾд����0��ʾ������ȡ
					//������籦
					char *tem = strtok(F4G_Fram_Record_Struct.ServerData, "-");
					u8 kk = atoi(tem);
					tem = strtok(NULL, "-");
					u8 ksta = atoi(tem);
					WriteIgnoreLock(kk, ksta);
				}
			}
		}
	}
	if (USART_GetFlagStatus(USART2, USART_FLAG_ORE) != RESET)
	{
		USART_ClearFlag(USART2, USART_FLAG_ORE);
		USART_ReceiveData(USART2);
	}
//	if (USART_GetITStatus( USART2, USART_IT_IDLE) == SET)              //����֡�������
//	{
//		USART_ClearFlag(USART2, USART_IT_IDLE);
//		USART_ReceiveData( USART2); //�������������жϱ�־λ(�ȶ�USART_SR��Ȼ���USART_DR)
//
//		F4G_Params.linkedClosed =
//				strstr(F4G_Fram_Record_Struct.Data_RX_BUF, "CLOSED\r\n") ?
//						1 : 0;
//	}
//	USART_ClearFlag(USART2, USART_FLAG_TC);
}
/**
 * ��λ4Gģ��
 */
void reset_4G_module(void)
{
//��λ4Gģ��
	RST_4G_Pin_SetH;
//PKEY_4G_Pin_SetL;
	delay_ms(1100);
	delay_ms(1100);
	RST_4G_Pin_SetL;
//PKEY_4G_Pin_SetH;
}

void powerOn4G(void)
{
	PKEY_4G_Pin_SetH;
	delay_ms(1000);
	delay_ms(1000);
	delay_ms(1000);
	delay_ms(1000);
	PKEY_4G_Pin_SetL;
}

/**
 * ��strͨ��delims���зָ�,���õ��ַ��������res��
 * @str ��ת���������ַ���
 * @delims �ָ���
 */
void split(char str[], char *delims)
{
	char *result = str;
	u8 inx = 0;
	while (inx < 2)
	{
		result++;
		if (*result == ',')
		{
			++inx;
		}
	}
	result++;
	memcpy(F4G_Fram_Record_Struct.ServerData, result, BASE64_BUF_LEN);
//printf("comd2=%s\r\n", F4G_Fram_Record_Struct.ServerData);
	result = strtok(str, delims);
	F4G_Fram_Record_Struct.Server_Command[0] = result;
	result = strtok( NULL, delims);
	F4G_Fram_Record_Struct.Server_Command[1] = result;
}

/**
 * ��F4Gģ�鷢��ATָ��
 * @cmd�������͵�ָ��
 * @ack1 @ack2���ڴ�����Ӧ��ΪNULL������Ӧ������Ϊ���߼���ϵ
 * @time���ȴ���Ӧ��ʱ��
 * @return 1�����ͳɹ� 0��ʧ��
 */
bool Send_AT_Cmd(char *cmd, char *ack1, char *ack2, u32 time)
{
	F4G_Fram_Record_Struct.InfAll = 0; //���¿�ʼ�������ݰ�
	F4G_USART("%s\r\n", cmd);
	if (ack1 == 0 && ack2 == 0)	 //����Ҫ��������
	{
		return true;
	}
	delay_ms(time);	  //��ʱtimeʱ��

	F4G_Fram_Record_Struct.Data_RX_BUF[F4G_Fram_Record_Struct.InfBit.FramLength] =
			'\0';

	PC_USART("%s", F4G_Fram_Record_Struct.Data_RX_BUF);

	if (ack1 != 0 && ack2 != 0)
	{
		return ((bool) strstr(F4G_Fram_Record_Struct.Data_RX_BUF, ack1)
				|| (bool) strstr(F4G_Fram_Record_Struct.Data_RX_BUF, ack2));
	}
	else if (ack1 != 0)
		return ((bool) strstr(F4G_Fram_Record_Struct.Data_RX_BUF, ack1));

	else
		return ((bool) strstr(F4G_Fram_Record_Struct.Data_RX_BUF, ack2));

}
/**
 * ����4Gģ���Ƿ�����(֧��ATָ��)
 */
u8 F4G_AT_Test(void)
{
	char count = 0;
	while (count < 8)
	{
		Send_AT_Cmd("AT", "OK", NULL, 500);
		++count;
	}
	if (Send_AT_Cmd("AT", "OK", NULL, 500))
	{
		printf("test 4G module success!\r\n");
		return 1;
	}
	printf("test 4G module fail!\r\n");
	return 0;
}
/**
 * ���ӵ�������
 * @IP ������ip��ַ
 * @PORT ���������ŵĶ˿�
 * @return 1--���ӳɹ�  0--����ʧ��
 */
bool connectToTCPServer(char *IP, char * PORT)
{
	char gCmd[100] =
	{ 0 };
//1.����ģʽΪTCP͸��ģʽ
	Send_AT_Cmd("AT+CIPMODE=1", "OK", NULL, 500);
	sprintf(gCmd, "AT+CIPSTART=\"TCP\",\"%s\",%s", IP, PORT);
	return Send_AT_Cmd(gCmd, "OK", NULL, 1800);
}

void reconnectToTCPServer(void)
{
	char gCmd[100] =
	{ 0 };
	sprintf(gCmd, "AT+CIPSTART=\"TCP\",\"%s\",%s", RegisterParams.ip,
			RegisterParams.port);
	while (!Send_AT_Cmd(gCmd, "ALREADY CONNECT", NULL, 1800))
		;
}

/**
 * ͸��ģʽ��4Gģ�鷢���ַ���
 */
void F4G_sendStr(char *str)
{
	u8 i = 0;
	char cmd[512] = "{(";
	strcat(cmd, str);
	strcat(cmd, "}");
//	sprintf(cmd, "{(%s}", str);
	while (cmd[i] != '}')
	{
		USART_SendData(USART2, cmd[i]);
		while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
			;
		i++;
	}
	USART_SendData(USART2, cmd[i]);
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
		;
//	printf("%s\r\n", cmd);
}

/**
 * 4Gģ���˳�͸��ģʽ
 */
void F4G_ExitUnvarnishSend(void)
{
	delay_ms(1000);
	F4G_USART("+++");
	delay_ms(500);
}
/***********************���¿�ʼΪ�������ͨ��ҵ����벿��*************************************/
void getModuleMes(void)
{
	char *result = NULL;
	u8 inx = 0;

//	Send_AT_Cmd("AT+SAPBR=0,1", "OK", NULL, 500); //ȥ����
//
//	while (!Send_AT_Cmd("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", "OK", NULL, 500))
//		;
//	while (!Send_AT_Cmd("AT+SAPBR=3,1,\"APN\",\"\"", "OK", NULL, 500))
//		;
//	while (!Send_AT_Cmd("AT+SAPBR=1,1", "OK", NULL, 500))
//		;
//	while (!Send_AT_Cmd("AT+SAPBR=2,1", "OK", NULL, 500))
//		;
	//AT + CIPGSMLOC = 1, 1
	//��ȡ��γ�Ⱥ�ʱ��
	while (!Send_AT_Cmd("AT+CIPGSMLOC=1,1", "OK", NULL, 500))
		;
//	result = strtok(F4G_Fram_Record_Struct.Data_RX_BUF, ",");
//	result = strtok( NULL, ",");
//	result = strtok( NULL, ",");
//	strcpy(F4G_Fram_Record_Struct.locations[0], result);
//	printf("longitude is \"%s\"\n", F4G_Fram_Record_Struct.locations[0]);
//	result = strtok( NULL, ",");
//	strcpy(F4G_Fram_Record_Struct.locations[1], result);
//	printf("latitude is \"%s\"\n", F4G_Fram_Record_Struct.locations[1]);

	strcpy(F4G_Fram_Record_Struct.locations[0], "0");
	strcpy(F4G_Fram_Record_Struct.locations[1], "0");
//
//	Send_AT_Cmd("AT+SAPBR=0,1", "OK", NULL, 500); //ȥ����
//
//	while (!Send_AT_Cmd("AT+CPIN?", "OK", NULL, 500))
//		;
//	while (!Send_AT_Cmd("AT+CGATT?", "OK", NULL, 500))
//		;
//	while (!Send_AT_Cmd("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", "OK", NULL, 500))
//		;
//	while (!Send_AT_Cmd("AT+SAPBR=3,1,\"APN\",\"\"", "OK", NULL, 500))
//		;
//	while (!Send_AT_Cmd("AT+SAPBR=1,1", "OK", NULL, 500))
//		;
//��ȡ����������
	while (!Send_AT_Cmd("AT+CCID", "OK", NULL, 500))
		;
	result = F4G_Fram_Record_Struct.Data_RX_BUF;
	inx = 0;
	while (!(*result <= '9' && *result >= '0'))
	{
		result++;
	}
//��ֵΪ��ĸ������ʱ
	while ((*result <= '9' && *result >= '0')
			|| (*result <= 'Z' && *result >= 'A')
			|| (*result <= 'z' && *result >= 'a'))
	{
		F4G_Fram_Record_Struct.ccid[inx++] = *result;
		result++;
	}
	printf("CCID=%s\r\n", F4G_Fram_Record_Struct.ccid);

//��ȡģ��������Ϣ
	while (!Send_AT_Cmd("AT+COPS=0,1", "OK", NULL, 1000))
		;
	while (!Send_AT_Cmd("AT+COPS?", "OK", NULL, 500))
		;
	if ((bool) strstr(F4G_Fram_Record_Struct.Data_RX_BUF, "CMCC"))
	{
		F4G_Fram_Record_Struct.cops = '3';
	}
	else if ((bool) strstr(F4G_Fram_Record_Struct.Data_RX_BUF, "UNICOM"))
	{
		F4G_Fram_Record_Struct.cops = '6';
	}
	else
	{
		F4G_Fram_Record_Struct.cops = '9';
	}
	printf("COPS is \"%c\"\n", F4G_Fram_Record_Struct.cops);
//��ȡ�ź�
	while (!Send_AT_Cmd("AT+CSQ", "OK", NULL, 500))
		;
	result = F4G_Fram_Record_Struct.Data_RX_BUF;
	while (*result++ != ':')
		;
	result++;
	F4G_Fram_Record_Struct.rssi = atoi(strtok(result, ","));
	printf("CSQ is %d\n", F4G_Fram_Record_Struct.rssi);
	//��ʱ����ͨAPN����
	if (F4G_Fram_Record_Struct.cops == '3')
	{
		while (!Send_AT_Cmd("AT+CPNETAPN=1,cmiot,\"\",\"\",0", "OK",
		NULL, 1800))
			;
	}
	else if (F4G_Fram_Record_Struct.cops == '6')
	{
		while (!Send_AT_Cmd("AT+CPNETAPN=1,UNIM2M.NJM2MAPN,\"\",\"\",0", "OK",
		NULL, 1800))
			;
	}
	else if (F4G_Fram_Record_Struct.cops == '9')
	{
		while (!Send_AT_Cmd("AT+CPNETAPN=1,CTNET,\"\",\"\",0", "OK",
		NULL, 1800))
			;
	}
	delay_ms(1000); //ǿ�Ƶȴ�1S
}
/**
 * ��װ����λ��ͨѶʱ���ַ���
 * @buf �����ַ���
 * @cmd ������
 * @ccid ����������
 * @net ��ǰʹ������
 * @module ��ǰʹ�õ�ģ��
 * @ver ��ǰ�̼��汾��
 * @attr ��ǰ��γ��
 * @num  ���ڻ���
 * @return ��
 */
void communicateWithHost(char buf[], char *cmd, char *ccid, char net,
		char *module, char *ver, char *longitude, char *latitude, char *num)
{
	const char* template = "%s,%s,%s-%c-%s-%s-%s_%s-%s";
	ReadDeviceID();
	sprintf(buf, template, RDeviceID, cmd, ccid, net, module, ver, longitude,
			latitude, num);
}

void catRequestStr(char buf[], char *cmd)
{
	ReadVersion();
	communicateWithHost(buf, cmd, F4G_Fram_Record_Struct.ccid,
			F4G_Fram_Record_Struct.cops, "2", RVersion,
			F4G_Fram_Record_Struct.locations[0],
			F4G_Fram_Record_Struct.locations[1], "06");
}
/**
 * ���������������
 * @cmd ����Ŀ�����
 */
void request(char *cmd)
{
	char rBuf1[128] =
	{ 0 };
	catRequestStr(rBuf1, cmd);
	request2Server(rBuf1);
}

void response2reset(char *cmd)
{
	char rBuf2[128] =
	{ 0 };
	ReadDeviceID();
	sprintf(rBuf2, "%s,%s", RDeviceID, cmd);
	request2Server(rBuf2);
}
/**
 * ��ͨ����
 */
void common_heart(void)
{
	const char *heart = "{(}";
//	printf("%s\r\n", heart);
	F4G_Params.serverStatuCnt += 1;
	while (*heart != '}')
	{
		USART_SendData(USART2, *heart);
		while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
			;
		heart++;
	}
	USART_SendData(USART2, *heart);
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
		;
}

void checkLinkedStatus(void)
{
	//���������Ƿ�ʧ���
	F4G_ExitUnvarnishSend(); //�˳�͸��
	if (Send_AT_Cmd("AT+CIPSTATUS", "CONNECT OK", NULL, 800))
	{
		while (!Send_AT_Cmd("ATO", "CONNECT", NULL, 500))
			;
		memset(F4G_Fram_Record_Struct.Data_RX_BUF, '\0', RX4G_BUF_MAX_LEN);
		F4G_Fram_Record_Struct.InfBit.FramLength = 0;
	}
	else
	{
		//NVIC_SystemReset();
		char gCmd[100] =
		{ 0 };
		//1.����ģʽΪTCP͸��ģʽ
		while (!Send_AT_Cmd("AT+CIPMODE=1", "OK", NULL, 500))
			;
		sprintf(gCmd, "AT+CIPSTART=\"TCP\",\"%s\",%s", RegisterParams.ip,
				RegisterParams.port);
		while (!Send_AT_Cmd(gCmd, "ALREADY CONNECT", NULL, 1800))
			;
	}
}
/**
 * ״̬����
 */
void statu_heart(char *cmds)
{
//ƴ���ַ���
	char reqBuf[256] =
	{ 0 };
	const char* template = "%s,%s,%s-%s-%s-%s-%s-%s-%s-%s";
//	F4G_Params.serverStatuCnt += 1;
	ReadDeviceID();
	sprintf(reqBuf, template, RDeviceID, cmds, powerbankStatu.powerBankBuf[0],
			powerbankStatu.powerBankBuf[1], powerbankStatu.powerBankBuf[2],
			powerbankStatu.powerBankBuf[3], powerbankStatu.powerBankBuf[4],
			powerbankStatu.powerBankBuf[5],
			Int2Strs(F4G_Fram_Record_Struct.rssi), "aaaaa");
	request2Server(reqBuf);
}
/**
 * ��������
 *
 */
void dd_tc(u8 port, char *cmds)
{
//ƴ���ַ���
	char reqBuf2[256] =
	{ 0 };
	const char* template = "%s,%s,%s-%s-%s";
	F4G_Params.statuCode[port] = checkPowerbankStatus(port,
			powerbankStatu.powerBankBuf[port]);
	ReadDeviceID();
	sprintf(reqBuf2, template, RDeviceID, cmds,
			powerbankStatu.powerBankBuf[port], F4G_Params.dd, "aaaaa");
	request2Server(reqBuf2);
}
/**
 * ϵͳ����
 */
void sys_tc(u8 port, char *cmds)
{
//ƴ���ַ���
	char reqBuf3[256] =
	{ 0 };
	const char* template = "%s,%s,%s-%s";
	F4G_Params.statuCode[port] = checkPowerbankStatus(port,
			powerbankStatu.powerBankBuf[port]);
	ReadDeviceID();
	sprintf(reqBuf3, template, RDeviceID, cmds,
			powerbankStatu.powerBankBuf[port], "aaaaa");
	request2Server(reqBuf3);
}

void currentPortStatuChanged(u8 port)
{
//ƴ���ַ���
	char reqBuf4[256] =
	{ 0 };
	const char* template = "%s,%s,%s-%s";
	ReadDeviceID();
	sprintf(reqBuf4, template, RDeviceID, "37",
			powerbankStatu.powerBankBuf[port], "aaaaa");
	request2Server(reqBuf4);
}

char *Int2Strs(u8 num)
{
	static char tem[3] =
	{ 0 };
	snprintf(tem, 3, "%d", num);
	return tem;
}

/**
 * ���������������
 */
void request2Server(char str[])
{
	printf("%s\r\n", str);
	base64_encode((const unsigned char *) str, F4G_Fram_Record_Struct.EnData);
	F4G_sendStr(F4G_Fram_Record_Struct.EnData);
}

