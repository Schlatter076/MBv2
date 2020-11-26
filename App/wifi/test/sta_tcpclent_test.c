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

	printf("\r\n��������wifi�����ĵȴ�...\r\n");

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
	printf("\r\n����WIFI OK��\r\n");

	while (1)
	{
		sprintf(str, "www.prechin.cn\r\n");
		WIFI_SendString(ENABLE, str, 0, Single_ID_0);
		delay_ms(1000);
		if (TcpClosedFlag) //����Ƿ�ʧȥ����
		{
			WIFI_ExitUnvarnishSend(); //�˳�͸��ģʽ
			do
			{
				res = WIFI_Get_LinkStatus();     //��ȡ����״̬
			} while (!res);

			if (res == 4)                     //ȷ��ʧȥ���Ӻ�����
			{
				printf("\r\n���Եȣ����������ȵ�ͷ�����...\r\n");

				while (!WIFI_JoinAP(User_WIFI_SSID, User_WIFI_PWD))
					;

				while (!WIFI_Link_Server(enumTCP, User_WIFI_TCPServer_IP,
						User_WIFI_TCPServer_PORT, Single_ID_0))
					;

				printf("\r\n�����ȵ�ͷ������ɹ�\r\n");
			}
			while (!WIFI_UnvarnishSend())
				;
		}
	}

}

