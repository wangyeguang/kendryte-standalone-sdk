/*
 * @Author: Wangyg wangyeguang521@163.com
 * @Date: 2024-05-30 10:58:49
 * @LastEditors: Wangyg wangyeguang521@163.com
 * @LastEditTime: 2024-06-03 19:12:08
 * @FilePath: \kendryte-standalone-sdk-new\src\face_recog_alive\ai_detect\wifi_function.c
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#include "wifi_function.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp8285.h"
#include "device_config.h"

int wifi_init()
{
    return esp8285_init();
}
bool wifi_setHostname()
{
    uint64_t machine_id = get_chip_number(); 
    char hostname[30]={0};
    snprintf(hostname,30,"F1-%lx",machine_id);
	printf("[setHostname]: hostname:%s\r\n",hostname);
    return esp8285_setHostname(hostname);
}

bool wifi_connect(const char *addr,int port)
{
    return  esp8285_sATCIPSTARTSingle(addr,port);
}
bool wifi_disconnect()
{
    return esp8285_eATCIPCLOSESingle();
}
bool wifi_is_connected()
{

}
bool wifi_getTcpStatus()
{
    char *list;
    bool ret = eATCIPSTATUS(&list);
    if(ret)
    {
        printf("wifi_getTcpStatus:%s\n",list);
    }
    if(list)
        free(list);
    return ret;
}
void wifi_get_ip(char *server_ip)
{
    get_ip(server_ip);
}
bool wifi_joinAp(const char *ssid,const char *passwd)
{
    return esp8285_joinAP(ssid,passwd);
}
bool wifi_cmd_process()        //tcp连接后接收并处理wifi数据
{

}
bool wifi_cmd_send(char *data,int len)
{
    
}