/*
 * usart_4G.c
 *
 *  Created on: 2020年6月11日
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
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;	//PA3  RXD
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;	//浮空输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);	//初始化GPIOA3

	//Usart2 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;	//抢占优先级0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		//子优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器

	//USART2 初始化设置
	USART_InitStructure.USART_BaudRate = bound;	//串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;	//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;	//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;	//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl =
	USART_HardwareFlowControl_None;	//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
	USART_Init(USART2, &USART_InitStructure); //初始化串口2

	USART_ITConfig(USART2, USART_IT_RXNE | USART_IT_IDLE, ENABLE); //开启串口接受和总线空闲中断

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
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 	 //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	 //IO口速度为50MHz

	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//开机
	do
	{
		PKEY_4G_Pin_SetH;
		delay_ms(1000);
		delay_ms(1000);
		delay_ms(1000);
		//delay_ms(1000);
		PKEY_4G_Pin_SetL;
		//复位4G模块
		RST_4G_Pin_SetH;
		delay_ms(1100);
		RST_4G_Pin_SetL;
		delay_ms(500);

		F4G_ExitUnvarnishSend();
		Send_AT_Cmd("AT+CIPCLOSE", "OK", NULL, 500);
		Send_AT_Cmd("AT+RSTSET", "OK", NULL, 500);
	} while (!F4G_AT_Test());

	//获取4G模块参数信息
	getModuleMes();
	//连接到服务器
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
	//上电注册
	while (1)
	{
		if (request_cnt >= 3)
		{
			NVIC_SystemReset(); //重启
			//RunApp();
		}
		request(REQ_REGISTER);
		request_cnt++;
		delay_ms(500);
		if (F4G_Fram_Record_Struct.InfBit.FramFinishFlag)
		{
			F4G_Fram_Record_Struct.InfBit.FramFinishFlag = 0;
			split(F4G_Fram_Record_Struct.DeData, ",");
			//注册成功
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
		//收到服务器端发回的数据
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
				//解析数据
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
					//弹出充电宝
					char *tem = strtok(F4G_Fram_Record_Struct.ServerData, "-");

					F4G_Params.port = atoi(tem);
					tem = strtok(NULL, "-");
					F4G_Params.play = (u8) atoi(tem);
					strcpy(F4G_Params.cmd, REQ_TC_POWERBANK);
					F4G_Params.sysTC = 1;
				}
				//收到上报的信息的回复
				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "36",
						2) == 0)
				{
					if (F4G_Params.play)
					{
						play_audio(2);
					}
				}
				//归还成功
				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "38",
						2) == 0)
				{
					if (F4G_Params.play)
					{
						play_audio(3); //归还成功
					}
				}
				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "71",
						2) == 0)
				{
					if (atoi(F4G_Fram_Record_Struct.ServerData) == 1)
					{
						play_audio(2); //租借成功
					}
					else if (atoi(F4G_Fram_Record_Struct.ServerData) == 2)
					{
						play_audio(3); //归还成功
					}
				}
				//强制心跳
				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "92",
						2) == 0)
				{
					strcpy(F4G_Params.htCMD, "93");
					F4G_Params.reqSTheart = 1;
				}
				//重启设备
				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "60",
						2) == 0)
				{
					response2reset("61");
					NVIC_SystemReset();
				}
				//订单弹出充电宝
				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "40",
						2) == 0)
				{
					//弹出充电宝
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
//					//获取经纬度
//					F4G_Params.getLongitude = 1;
//				}
//				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "11",
//						2) == 0)
//				{
//					//给充电宝设置ID
//
//				}
				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "99",
						2) == 0)
				{
					//弹出所有充电宝
					F4G_Params.popAll = 1;
					//popUP_All();
				}
				else if (memcmp(F4G_Fram_Record_Struct.Server_Command[1], "77",
						2) == 0)
				{
					//修改忽略某卡口锁异常状态
					//接口参数：卡口_状态（1表示写死，0表示正常读取
					//弹出充电宝
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
//	if (USART_GetITStatus( USART2, USART_IT_IDLE) == SET)              //数据帧接收完毕
//	{
//		USART_ClearFlag(USART2, USART_IT_IDLE);
//		USART_ReceiveData( USART2); //由软件序列清除中断标志位(先读USART_SR，然后读USART_DR)
//
//		F4G_Params.linkedClosed =
//				strstr(F4G_Fram_Record_Struct.Data_RX_BUF, "CLOSED\r\n") ?
//						1 : 0;
//	}
//	USART_ClearFlag(USART2, USART_FLAG_TC);
}
/**
 * 复位4G模块
 */
void reset_4G_module(void)
{
//复位4G模块
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
 * 将str通过delims进行分割,所得的字符串填充在res中
 * @str 待转换的数据字符串
 * @delims 分隔符
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
 * 对F4G模块发送AT指令
 * @cmd：待发送的指令
 * @ack1 @ack2：期待的响应，为NULL表不需响应，两者为或逻辑关系
 * @time：等待响应的时间
 * @return 1：发送成功 0：失败
 */
bool Send_AT_Cmd(char *cmd, char *ack1, char *ack2, u32 time)
{
	F4G_Fram_Record_Struct.InfAll = 0; //重新开始接收数据包
	F4G_USART("%s\r\n", cmd);
	if (ack1 == 0 && ack2 == 0)	 //不需要接收数据
	{
		return true;
	}
	delay_ms(time);	  //延时time时间

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
 * 测试4G模块是否正常(支持AT指令)
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
 * 连接到服务器
 * @IP 服务器ip地址
 * @PORT 服务器开放的端口
 * @return 1--连接成功  0--连接失败
 */
bool connectToTCPServer(char *IP, char * PORT)
{
	char gCmd[100] =
	{ 0 };
//1.设置模式为TCP透传模式
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
 * 透传模式下4G模块发送字符串
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
 * 4G模块退出透传模式
 */
void F4G_ExitUnvarnishSend(void)
{
	delay_ms(1000);
	F4G_USART("+++");
	delay_ms(500);
}
/***********************以下开始为与服务器通信业务代码部分*************************************/
void getModuleMes(void)
{
	char *result = NULL;
	u8 inx = 0;

//	Send_AT_Cmd("AT+SAPBR=0,1", "OK", NULL, 500); //去激活
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
	//获取经纬度和时间
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
//	Send_AT_Cmd("AT+SAPBR=0,1", "OK", NULL, 500); //去激活
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
//获取物联网卡号
	while (!Send_AT_Cmd("AT+CCID", "OK", NULL, 500))
		;
	result = F4G_Fram_Record_Struct.Data_RX_BUF;
	inx = 0;
	while (!(*result <= '9' && *result >= '0'))
	{
		result++;
	}
//当值为字母和数字时
	while ((*result <= '9' && *result >= '0')
			|| (*result <= 'Z' && *result >= 'A')
			|| (*result <= 'z' && *result >= 'a'))
	{
		F4G_Fram_Record_Struct.ccid[inx++] = *result;
		result++;
	}
	printf("CCID=%s\r\n", F4G_Fram_Record_Struct.ccid);

//获取模块网络信息
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
//获取信号
	while (!Send_AT_Cmd("AT+CSQ", "OK", NULL, 500))
		;
	result = F4G_Fram_Record_Struct.Data_RX_BUF;
	while (*result++ != ':')
		;
	result++;
	F4G_Fram_Record_Struct.rssi = atoi(strtok(result, ","));
	printf("CSQ is %d\n", F4G_Fram_Record_Struct.rssi);
	//暂时将联通APN配死
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
	delay_ms(1000); //强制等待1S
}
/**
 * 组装与上位机通讯时的字符串
 * @buf 填充的字符串
 * @cmd 控制字
 * @ccid 物联网卡号
 * @net 当前使用网络
 * @module 当前使用的模块
 * @ver 当前固件版本号
 * @attr 当前经纬度
 * @num  几口机型
 * @return 无
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
 * 向服务器发起请求
 * @cmd 请求的控制字
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
 * 普通心跳
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
	//增加连接是否丢失检查
	F4G_ExitUnvarnishSend(); //退出透传
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
		//1.设置模式为TCP透传模式
		while (!Send_AT_Cmd("AT+CIPMODE=1", "OK", NULL, 500))
			;
		sprintf(gCmd, "AT+CIPSTART=\"TCP\",\"%s\",%s", RegisterParams.ip,
				RegisterParams.port);
		while (!Send_AT_Cmd(gCmd, "ALREADY CONNECT", NULL, 1800))
			;
	}
}
/**
 * 状态心跳
 */
void statu_heart(char *cmds)
{
//拼接字符串
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
 * 订单弹出
 *
 */
void dd_tc(u8 port, char *cmds)
{
//拼接字符串
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
 * 系统弹出
 */
void sys_tc(u8 port, char *cmds)
{
//拼接字符串
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
//拼接字符串
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
 * 向服务器发起请求
 */
void request2Server(char str[])
{
	printf("%s\r\n", str);
	base64_encode((const unsigned char *) str, F4G_Fram_Record_Struct.EnData);
	F4G_sendStr(F4G_Fram_Record_Struct.EnData);
}

