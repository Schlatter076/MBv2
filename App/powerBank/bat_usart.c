/*
 * bat_usart.c
 *
 *  Created on: 2020��6��17��
 *      Author: loyer
 */
#include "bat_usart.h"

Uart3MagType UART3_RX_BUF =
{ 0 };   //�жϽ��ջ���

/**����״̬*/
const unsigned char bat_statuBuf[16] =
{ 1, 0, 9, 8, 3, 2, 11, 10, 0xFF, 0xFF, 5, 4, 0xFF, 0xFF, 7, 6 };

/**
 * ��ʼ������3
 * @bound ������-��9600
 */
void USART3_Init(u32 bound)
{
	//GPIO�˿�����
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	USART_DeInit(USART3);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	/*  ����GPIO��ģʽ��IO�� */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; //TX			   //�������PB10
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	    //�����������
	GPIO_Init(GPIOB, &GPIO_InitStructure); /* ��ʼ����������IO */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;	    //RX			 //��������PB11
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;		  //ģ������
	GPIO_Init(GPIOB, &GPIO_InitStructure); /* ��ʼ��GPIO */

	//USART1 ��ʼ������
	USART_InitStructure.USART_BaudRate = bound;		  //����������
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;		 //�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;		  //һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;		  //����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl =
	USART_HardwareFlowControl_None;		  //��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
	USART_Init(USART3, &USART_InitStructure); //��ʼ������1

	USART_Cmd(USART3, ENABLE);  //ʹ�ܴ���1

	USART_ClearFlag(USART3, USART_FLAG_TC);

	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);  //��������ж�

	//Usart1 NVIC ����
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;  //����1�ж�ͨ��
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //��ռ���ȼ�1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//�����ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ�����
}

/**
 * ��ʼ����籦������ͨ�Ŵ���
 * @bound ���ڲ�����
 * ͬʱ��ʼ����ͬ�ڳ�籦ͨ��ѡͨ
 */
void Init_Bat_Usart(u32 bound)
{
	USART3_Init(bound);

	GPIO_InitTypeDef GPIO_InitStructure; //����ṹ�����

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15;  //ѡ����Ҫ���õ�IO��
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	 //�����������ģʽ
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	  //���ô�������
	GPIO_Init(GPIOB, &GPIO_InitStructure); /* ��ʼ��GPIO */

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8; //ѡ����Ҫ���õ�IO��
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	 //�����������ģʽ
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	  //���ô�������
	GPIO_Init(GPIOC, &GPIO_InitStructure); /* ��ʼ��GPIO */

}
/**
 * ѡ�����ĸ��ڵĳ�籦����ͨ��
 * @port ����1-6���������豸��������
 */
void communicateWithPort(u8 port)
{
	switch (port)
	{
	case 1:
		_A1_L;
		_B1_L;
		break;
	case 2:
		_A1_H;
		_B1_L;
		break;
	case 3:
		_A1_L;
		_B1_H;
		break;
	case 4:
		_A1_H;
		_B1_H;
		_A2_L;
		_B2_L;
		break;
	case 5:
		_A1_H;
		_B1_H;
		_A2_H;
		_B2_L;
		break;
	case 6:
		_A1_H;
		_B1_H;
		_A2_L;
		_B2_H;
		break;
	default:
		break;
	}
//	printf("now communicate with powerBank_%d\r\n", port);
}
/**
 * ����ĳ����籦�ڵ�ͨ��״��
 * @port 0-5 �����1-6
 */
u8 communicationTest(u8 p)
{
	u8 cnt = 0;
	if (!powerbankStatu.Charging[p]) //��ǰδ���
	{
		controlPowerBankCharge(p + 1, 1); //����籦����
	}
	communicateWithPort(p + 1); //������������
	do
	{
		if (cnt > 4)
		{
			return 0;
		}
		TX3_RX3_Exchange;
		delay_ms(2);
		controlPowerbank(0x0F);
		delay_ms(50);
		cnt++;
	} while (!powerbankStatu.TEST);
	powerbankStatu.TEST = 0;
	return 1;
}
/**
 * ����״̬
 * @i 0-5
 * @buf �ֶ�
 * @return ״̬��
 */
u8 checkPowerbankStatus(u8 i, char buf[])
{
	u8 pbStatu = 0;
	u8 motorStatu = 0;
	u8 communicationStatu = 0;
	u8 portStatu = 0;
	u8 index = 0;
	u8 pbERROR = 0;
	char i2s[3] =
	{ 0 };

	key_statu = read_74HC165();
	communicationStatu = communicationTest(i);
	pbStatu = ((key_statu & 0xFF) & (1 << i)) == 0 ? 0 : 1;
	motorStatu = (((key_statu >> 8) & 0xFF) & (1 << i)) == 0 ? 0 : 1;
	index = (pbERROR << 3) | (pbStatu << 2) | (communicationStatu << 1)
			| (motorStatu << 0);

	//��״̬��
	portStatu = bat_statuBuf[index];
	//�����ж�============Begin
	if (portStatu == 11 || portStatu == 1) //�е�أ��������쳣��ͨ������
	{
		if (ReadIgnoreLock(i) == 1) //�����˺�����״̬
		{
			portStatu = (portStatu == 11) ? 10 : 0;
		}
	}
	//�����ж�============End
	//=============================================
	setBATInstruction(i, portStatu);
	//=============================================

	if (communicationStatu)
	{
		if (pbERROR)
		{
			sprintf(buf, "%d_%d_%02X_%s", i, portStatu, powerbankStatu.ERROR,
					(char *) powerbankStatu.ChargeBankID);
		}
		else
		{
//			buf[0] = i + 0x30;
			strcpy(buf, itoa(i, i2s, 10));
			strcat(buf, "_");
			strcat(buf, itoa(portStatu, i2s, 10));
			strcat(buf, "_");
			strcat(buf, itoa(powerbankStatu.VOL, i2s, 10));
			strcat(buf, "_");
			strncat(buf, (char *) powerbankStatu.ChargeBankID, 8);

//			sprintf(buf, "%d_%d_%d_%s", i, portStatu, powerbankStatu.VOL,
//					(char *) powerbankStatu.ChargeBankID);
		}
	}
	//��3��4�ֶ�
	else
	{
		snprintf(buf, 4, "%d_%d", i, portStatu);
	}
//	printf("%s\r\n", buf);
	return portStatu;
}

void scanPowerBank(void)
{
	u8 inx = 0;
	for (inx = 0; inx < 6; inx++)
	{
		memset(powerbankStatu.powerBankBuf[inx], '\0', 18);
		F4G_Params.statuCode[inx] = checkPowerbankStatus(inx,
				powerbankStatu.powerBankBuf[inx]);
	}
}

/**
 * ���ݼ���״̬����
 */
void setBATInstruction(u8 port, u8 sta)
{
	if (sta >= 0 && sta <= 11)
	{
		if (sta == 10) //��������״̬
		{
//			powerbankStatu.ChargingCnt = 0; //��0��ʼ����
//			if (powerbankStatu.VOL < 95) //��������95%���
//			{
//				for (int j = 0; j < 6; j++)
//				{
//					if (powerbankStatu.Charging[j] != 0) //�����
//					{
//						powerbankStatu.ChargingCnt++;
//					}
//				}
//				if (powerbankStatu.ChargingCnt < 3)
//				{
//					powerbankStatu.Charging[port] = 1;
//					controlPowerBankCharge(port + 1, 1); //���
//					HC595_STATUS.slowBLINK[port] = 0; //����
//					ledON(port + 1);
//					HC595_STATUS.fastBLINK[port] = 0;
//				}
//				else
//				{
//					powerbankStatu.Charging[port] = 0;
//					controlPowerBankCharge(port + 1, 0); //�����
//					ledON(port + 1);
//					HC595_STATUS.slowBLINK[port] = 0; //����
//					HC595_STATUS.fastBLINK[port] = 0;
//				}
//			}
//			else
//			{
//				powerbankStatu.Charging[port] = 0;
//				controlPowerBankCharge(port + 1, 0); //�����
//				ledON(port + 1);
//				HC595_STATUS.slowBLINK[port] = 0; //����
//				HC595_STATUS.fastBLINK[port] = 0;
//			}
			//11.10���ĳ���߼�Start=========================================
			ledON(port + 1);
			HC595_STATUS.slowBLINK[port] = 0; //����
			HC595_STATUS.fastBLINK[port] = 0;
			if (powerbankStatu.VOL < 95) //��������95%���
			{
				controlPowerBankCharge(port + 1, 1); //���
			}
			else
			{
				controlPowerBankCharge(port + 1, 0); //�����
			}
			//11.10���ĳ���߼�End===========================================
		}
		else if (sta == 0) //�����޳�籦״̬
		{
			powerbankStatu.Charging[port] = 0;
			controlPowerBankCharge(port + 1, 0); //�����
			ledOFF(port + 1);
			HC595_STATUS.slowBLINK[port] = 0; //����
			HC595_STATUS.fastBLINK[port] = 0;
		}
		else //���쳣
		{
			powerbankStatu.Charging[port] = 0;
			controlPowerBankCharge(port + 1, 0); //�����
			HC595_STATUS.slowBLINK[port] = 0; //����
			HC595_STATUS.fastBLINK[port] = 1; //����
			//ledON(port + 1);
		}
	}
}
//============================================================================

uint8_t BSP_UART_SendOneByte(uint8_t data)
{
	USART_SendData(USART3, data);
	while (USART_GetFlagStatus(USART3, USART_FLAG_TC) != SET)
		;
	return (data);
}

/**
 * CRCУ�麯��
 * @pbuf ��У�������
 * @num ��У��ĸ���
 * @return У����crcֵ
 */
unsigned int get_crc(unsigned char pbuf[], unsigned char num)
{
	unsigned char i, j;
	unsigned int wcrc = 0x0000;
	for (i = 0; i < num; i++)
	{
		wcrc ^= (pbuf[i] << 8);
		for (j = 0; j < 8; j++)
		{
			if (wcrc & 0x8000)
			{
				wcrc <<= 1;
				wcrc ^= 0x1021;
			}
			else
			{
				wcrc <<= 1;
			}
		}
	}
	return wcrc;
}
//unsigned int get_crc(unsigned char pbuf[], unsigned char num)
//{
//	unsigned char i, j;
//	unsigned int wcrc = 0xffff;
//	for (i = 0; i < num; i++)
//	{
//		wcrc ^= (unsigned int) (pbuf[i]);
//		for (j = 0; j < 8; j++)
//		{
//			if (wcrc & 1)
//			{
//				wcrc >>= 1;
//				wcrc ^= 0xa001;
//			}
//			else
//			{
//				wcrc >>= 1;
//			}
//		}
//	}
//	return wcrc;
//}
/**
 * CRC16У�麯��
 * �ɱ�Σ�@numΪ����У�������
 */
unsigned int get_crc_2(int num, ...)
{
	va_list ap;  //��������ָ��
	unsigned int wcrc = 0x0000;
	unsigned char j = 0;
	va_start(ap, num); //���ò���ָ��
	while (num)
	{
		wcrc ^= (va_arg(ap, int) << 8);
		for (j = 0; j < 8; j++)
		{
			if (wcrc & 0x8000)
			{
				wcrc <<= 1;
				wcrc ^= 0x1021;
			}
			else
			{
				wcrc <<= 1;
			}
		}
		num--;
	}
	va_end(ap); //�ͷ�ָ����Դ
	return wcrc;
}
//unsigned int get_crc_2(int num, ...)
//{
//	va_list ap;  //��������ָ��
//	unsigned int wcrc = 0xffff;
//	unsigned char j = 0;
//	va_start(ap, num); //���ò���ָ��
//
//	while (num)
//	{
//		wcrc ^= (unsigned int) (va_arg(ap, int));
//		for (j = 0; j < 8; j++)
//		{
//			if (wcrc & 1)
//			{
//				wcrc >>= 1;
//				wcrc ^= 0xa001;
//			}
//			else
//			{
//				wcrc >>= 1;
//			}
//		}
//		num--;
//	}
//	va_end(ap); //�ͷ�ָ����Դ
//	return wcrc;
//}
/*
 * �������ƣ�BSP_UART_SendMultipleBytes
 * ����˵�������ڷ��Ͷ��ֽ�����
 * IN��      data ����������
 * OUT��     ��
 * Return��  ��
 */
void BSP_UART_SendMultipleBytes(unsigned char buf[], unsigned char len)
{
	uint8_t i;
	UART3_RX_BUF.counter = 0;
	for (i = 0; i < len; i++)
	{
		BSP_UART_SendOneByte(buf[i]);
	}
}

FramType framMag =
{ 0 };
PowerBankType powerbankStatu =
{ 0 };
/** function: app_frame_anasys
 *   breaf: ֡��Э�����
 *   input:  none
 *   output: none
 *   return: none
 */
void app_frame_anasys(void)
{
	uint8_t i = 0;
	uint16_t crc;
//ȡ��ָ���
	framMag.len = UART3_RX_BUF.data[1];
//ȡ��������
	framMag.cmd = UART3_RX_BUF.data[2];
//����CRCУ����ֽ�
	for (i = 0; i < framMag.len - 4; i++)
	{
		framMag.CRC_BUF[i] = UART3_RX_BUF.data[i + 1];
	}
	crc = get_crc(framMag.CRC_BUF, framMag.len - 4);
//�ж�CRCУ��ֵ�Ƿ���ȷ
	if (crc
			== (UART3_RX_BUF.data[framMag.len - 3] << 8
					| UART3_RX_BUF.data[framMag.len - 2]))
	{
		//��ʼ��������
		app_cmd_anasys();
	}
}
/** function: app_cmd_anasys
 *   breaf: �����Э�����
 *   input:  none
 *   output: none
 *   return: none
 */
void app_cmd_anasys(void)
{
	uint8_t inx = 0;
	switch (framMag.cmd)
	{
	case 0x8F:
		powerbankStatu.TEST = 1;
		powerbankStatu.VOL = UART3_RX_BUF.data[11];
		for (inx = 0; inx < ChargeBankIDSize; inx++)
		{
			powerbankStatu.ChargeBankID[inx] = UART3_RX_BUF.data[inx + 3];
		}
		//		printf("%s.vol=%02X\r\n", powerbankStatu.ChargeBankID,
		//				powerbankStatu.VOL);
		break;
	case ComTest:
		powerbankStatu.TEST = 1;
//		printf("test powerbank UART success.\r\n");
		break;
	case UP_ReadID:
		powerbankStatu.TEST = 1;
		for (inx = 0; inx < ChargeBankIDSize; inx++)
		{
			powerbankStatu.ChargeBankID[inx] = UART3_RX_BUF.data[inx + 3];
		}
//		printf("current powerbankID=%s\r\n", powerbankStatu.ChargeBankID);
		break;
	case UP_ReadVOL:
		powerbankStatu.TEST = 1;
		powerbankStatu.VOL = UART3_RX_BUF.data[11];
		for (inx = 0; inx < ChargeBankIDSize; inx++)
		{
			powerbankStatu.ChargeBankID[inx] = UART3_RX_BUF.data[inx + 3];
		}
//		printf("%s.vol=%02X\r\n", powerbankStatu.ChargeBankID,
//				powerbankStatu.VOL);
		break;
	case UP_ReadCUR:
		powerbankStatu.TEST = 1;
		powerbankStatu.CUR = (UART3_RX_BUF.data[11] << 8)
				| UART3_RX_BUF.data[12];
		for (inx = 0; inx < ChargeBankIDSize; inx++)
		{
			powerbankStatu.ChargeBankID[inx] = UART3_RX_BUF.data[inx + 3];
		}
//		printf("%s.cur=%04X\r\n", powerbankStatu.ChargeBankID,
//				powerbankStatu.CUR);
		break;
	case UP_ReadERROR:
		powerbankStatu.TEST = 1;
		powerbankStatu.ERROR = UART3_RX_BUF.data[11];
		for (inx = 0; inx < ChargeBankIDSize; inx++)
		{
			powerbankStatu.ChargeBankID[inx] = UART3_RX_BUF.data[inx + 3];
		}
//		printf("%s.ERROR=%02X\r\n", powerbankStatu.ChargeBankID,
//				powerbankStatu.ERROR);
		break;
	default:
		break;
	}
}
/**
 * ���Ƴ�籦
 * @cmd ������
 */
void controlPowerbank(u8 cmd)
{
	unsigned char DOWN_Data_BUF[6];
	u16 crc;
	DOWN_Data_BUF[0] = CMD_Header;
	DOWN_Data_BUF[1] = 0x06;
	DOWN_Data_BUF[2] = cmd;
	crc = get_crc_2(2, DOWN_Data_BUF[1], DOWN_Data_BUF[2]);
	DOWN_Data_BUF[3] = crc >> 8 & 0xFF;
	DOWN_Data_BUF[4] = crc & 0xFF;
	DOWN_Data_BUF[5] = CMD_End;

	BSP_UART_SendMultipleBytes(DOWN_Data_BUF, DOWN_Data_BUF[1]);
}

/*******************************************************************************
 * �� �� ��         : USART3_IRQHandler
 * ��������		   : USART3�жϺ���
 * ��    ��         : ��
 * ��    ��         : ��
 *******************************************************************************/
void USART3_IRQHandler(void)
{
	char data;
	if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)  //�����ж�
	{
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);
		data = USART_ReceiveData(USART3);  //(USART1->DR);	//��ȡ���յ�������
		if (UART3_RX_BUF.flag == 0)  //�Ƿ���������µ����ݰ�
		{
			UART3_RX_BUF.data[UART3_RX_BUF.counter] = data; //�������
			UART3_RX_BUF.counter++;
			switch (UART3_RX_BUF.counter)
			{
			case 1:  //У����ʼ�ַ�
				if (data != UP_Header)
					UART3_RX_BUF.counter = 0;
				break;
			case 2:  //��ȡ���ݰ�����
				UART3_RX_BUF.len = data;
				break;
			case UART3_RX_BUF_SIZE: //����������������δ���յ���ȷ�Ľ����ַ�
				UART3_RX_BUF.counter = 0;
				//BSP_UART_SendOneByte(0xF3); //����0xF3�������ݷ�������
				printf("UART3 data ERROR.\r\n");
				break;
			default:
				//��ȡ��ȫ�����ݣ������һ�ֽ�����У����ȷ
				if (UART3_RX_BUF.counter == UART3_RX_BUF.len && data == UP_End)
				{
					UART3_RX_BUF.flag = 1;
					UART3_RX_BUF.counter = 0;
					app_frame_anasys();
					UART3_RX_BUF.flag = 0;
					//BSP_UART_SendOneByte(0xFF);
				}
				break;
			}
		}
	}
	if (USART_GetFlagStatus(USART3, USART_FLAG_ORE) != RESET)
	{
		USART_ClearFlag(USART3, USART_FLAG_ORE);
		USART_ReceiveData(USART3);
	}
//	USART_ClearFlag(USART3, USART_FLAG_TC);
}
