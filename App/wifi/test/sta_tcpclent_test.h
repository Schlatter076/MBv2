#ifndef _sta_tcpclent_test_H
#define _sta_tcpclent_test_H

#include "bit_band.h"

#define User_WIFI_SSID	  "HHSKJ_1688"	      //Ҫ���ӵ��ȵ������
#define User_WIFI_PWD	  "hhs888888"	  //Ҫ���ӵ��ȵ������

#define User_WIFI_TCPServer_IP	  "192.168.0.108"	  //Ҫ���ӵķ�������IP
#define User_WIFI_TCPServer_PORT	  "8888"	  //Ҫ���ӵķ������Ķ˿�

extern volatile uint8_t TcpClosedFlag;  //����һ��ȫ�ֱ���

void WIFI_STA_TCPClient_Test(void);

#endif
