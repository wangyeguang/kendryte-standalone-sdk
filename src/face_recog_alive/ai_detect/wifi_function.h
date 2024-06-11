#ifndef _WIFI_FUNCTION
#define _WIFI_FUNCTION
#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"

int wifi_init(void);
bool wifi_setHostname(void);
bool wifi_connect(const char *addr,int port);
bool wifi_disconnect();
bool wifi_is_connected();
bool wifi_getTcpStatus();
void wifi_get_ip(char *server_ip);
bool wifi_joinAp(const char *ssid,const char *passwd);
bool wifi_cmd_process();        //tcp连接后接收并处理wifi数据
bool wifi_cmd_send(char *data,int len);

#endif // !_WIFI_FUNCTION
