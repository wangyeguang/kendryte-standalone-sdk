/*
 * @Author: Wangyg wangyeguang521@163.com
 * @Date: 2024-05-30 10:58:41
 * @LastEditors: yeguang wang wangyeguang521@163.com
 * @LastEditTime: 2024-07-11 13:16:27
 * @FilePath: \kendryte-standalone-sdk-new\src\face_recog_alive\ai_detect\wifi_function.h
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
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
int wifi_cmd_send(char *data,int len);
void wifi_cmd_send_login(void);
#endif // !_WIFI_FUNCTION
