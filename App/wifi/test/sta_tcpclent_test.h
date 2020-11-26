#ifndef _sta_tcpclent_test_H
#define _sta_tcpclent_test_H

#include "bit_band.h"

#define User_WIFI_SSID	  "HHSKJ_1688"	      //要连接的热点的名称
#define User_WIFI_PWD	  "hhs888888"	  //要连接的热点的密码

#define User_WIFI_TCPServer_IP	  "192.168.0.108"	  //要连接的服务器的IP
#define User_WIFI_TCPServer_PORT	  "8888"	  //要连接的服务器的端口

extern volatile uint8_t TcpClosedFlag;  //定义一个全局变量

void WIFI_STA_TCPClient_Test(void);

#endif
