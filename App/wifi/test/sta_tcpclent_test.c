#include "sta_tcpclent_test.h"
#include "systick.h"
#include "usart.h"
#include "wifi_drive.h"

volatile u8 TcpClosedFlag = 0;

void WIFI_STA_TCPClient_Test(void)
{
	u8 res;

	char str[100] =
	{ 0 };

	printf("\r\n正在配置wifi请耐心等待...\r\n");

	WIFI_CH_PD_Pin_SetH;

	WIFI_AT_Test();
	WIFI_Net_Mode_Choose(STA);
	while (!WIFI_JoinAP(User_WIFI_SSID, User_WIFI_PWD))
		;
	WIFI_Enable_MultipleId(DISABLE);
	while (!WIFI_Link_Server(enumTCP, User_WIFI_TCPServer_IP,
			User_WIFI_TCPServer_PORT, Single_ID_0))
		;

	while (!WIFI_UnvarnishSend())
		;
	printf("\r\n配置WIFI OK！\r\n");

	while (1)
	{
		sprintf(str, "www.prechin.cn\r\n");
		WIFI_SendString(ENABLE, str, 0, Single_ID_0);
		delay_ms(1000);
		if (TcpClosedFlag) //检测是否失去连接
		{
			WIFI_ExitUnvarnishSend(); //退出透传模式
			do
			{
				res = WIFI_Get_LinkStatus();     //获取连接状态
			} while (!res);

			if (res == 4)                     //确认失去连接后重连
			{
				printf("\r\n请稍等，正在重连热点和服务器...\r\n");

				while (!WIFI_JoinAP(User_WIFI_SSID, User_WIFI_PWD))
					;

				while (!WIFI_Link_Server(enumTCP, User_WIFI_TCPServer_IP,
						User_WIFI_TCPServer_PORT, Single_ID_0))
					;

				printf("\r\n重连热点和服务器成功\r\n");
			}
			while (!WIFI_UnvarnishSend())
				;
		}
	}

}

