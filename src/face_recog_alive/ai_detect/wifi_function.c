/*
 * @Author: Wangyg wangyeguang521@163.com
 * @Date: 2024-05-30 10:58:49
 * @LastEditors: yeguang wang wangyeguang521@163.com
 * @LastEditTime: 2024-07-10 22:40:44
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
#include "cJSON.h"

#define WIFI_BUFFER_SIZE 5120
static uint8_t wifi_data[WIFI_BUFFER_SIZE];
//协议处理回调函数
typedef int (*uart_callback_t)(uint8_t *data, int len);
#define MAX_CALLBACKS 15
typedef struct {
    uart_callback_t callback;
    char context[20];
} uart_callback_entry_t;

typedef struct {
    uart_callback_entry_t callbacks[MAX_CALLBACKS];
    int count;
} wifi_callback_list_t;
wifi_callback_list_t wifi_callback_list;
// 初始化回调列表
void wifi_callback_list_init(wifi_callback_list_t *list)
{
    memset(list, 0, sizeof(wifi_callback_list_t));
}
// 添加回调函数到列表
static int wifi_callback_list_add(wifi_callback_list_t *list, uart_callback_t callback, void *context)
{
    if (list->count >= MAX_CALLBACKS)
    {
        return -1; // 列表已满
    }

    list->callbacks[list->count].callback = callback;
    if(context)
    {
        strcpy( list->callbacks[list->count].context,context);
    }
   
    list->count++;
    return 0;
}
// 从列表中删除回调函数
int wifi_callback_list_remove(wifi_callback_list_t *list, uart_callback_t callback)
{
    for (int i = 0; i < list->count; i++)
    {
        if (list->callbacks[i].callback == callback)
        {
            // 将最后一个回调函数移到当前位置
            list->callbacks[i] = list->callbacks[list->count - 1];
            list->count--;
            return 0;
        }
    }
    return -1; // 未找到回调函数
}
// 执行回调列表中的所有回调函数
void wifi_callback_list_execute(wifi_callback_list_t *list,char *context,uint8_t *data, int len)
{
    for (int i = 0; i < list->count; i++)
    {
        if(strcmp(list->callbacks[i].context,context)==0)
        {
            list->callbacks[i].callback(data, len);
            break;
        }
        
    }
}
int wifi_init()
{
    int ret =  esp8285_init();
    wifi_callback_list_init(&wifi_callback_list);
        //注册回调函数
    // wifi_callback_list_add(&wifi_callback_list, user_cmd_set_cfg_func, "set_cfg");
    // wifi_callback_list_add(&wifi_callback_list, user_cmd_get_cfg_func, "get_cfg");
    // wifi_callback_list_add(&wifi_callback_list, user_cmd_cal_pic_fea_func, "cal_pic_fea");
    // wifi_callback_list_add(&wifi_callback_list, user_cmd_send_pic_data_func, "send_pic_data");
    // wifi_callback_list_add(&wifi_callback_list, user_cmd_set_wifi_cfg_func, "set_wifi_cfg");
    // wifi_callback_list_add(&wifi_callback_list, user_cmd_get_wifi_cfg_func, "get_wifi_cfg");
    // wifi_callback_list_add(&wifi_callback_list, user_cmd_get_status_func, "get_status");
    // wifi_callback_list_add(&wifi_callback_list, user_cmd_del_user_func, "del_user");
    // wifi_callback_list_add(&wifi_callback_list, user_cmd_query_face_func, "query_face");
    // wifi_callback_list_add(&wifi_callback_list, user_cmd_get_record_func, "get_record");
    // wifi_callback_list_add(&wifi_callback_list, user_cmd_del_record_func, "del_record");
    // wifi_callback_list_add(&wifi_callback_list, user_cmd_set_fota_data_func, "set_fota_data");
    // wifi_callback_list_add(&wifi_callback_list, user_cmd_set_debug_func, "set_debug");
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
    return true;
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

void wifi_parse_data(char *json_string,int len)
{
    // 解析JSON字符串
    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL) {
        printf("Error before: [%s]\n", cJSON_GetErrorPtr());
        cJSON_Delete(root);
        return;
    }
    // 获取"version"数字
    cJSON *version = cJSON_GetObjectItem(root, "version");
    if ((version != NULL) && cJSON_IsNumber(version)) {
        printf("version: %d\n", version->valueint);
    }
    #if 1
    // 获取"type"字符串
    cJSON *type = cJSON_GetObjectItem(root, "type");
    if ((type != NULL) && cJSON_IsString(type)) {
        printf("type: %s\n", type->valuestring);
            // 释放cJSON对象
        char type_value[20] = {0};
        strncpy(type_value,type->valuestring,20);
        if(root) {cJSON_Delete(root); root = NULL;}
        wifi_callback_list_execute(&wifi_callback_list,type_value,(uint8_t *)json_string,strlen(json_string));
    }
#endif
    // 释放cJSON对象
    if(root) {cJSON_Delete(root); root = NULL;}
}
bool wifi_cmd_process()        //tcp连接后接收并处理wifi数据
{
    int read_len = 0;
    int ret = esp8285_recv(wifi_data,WIFI_BUFFER_SIZE,&read_len,1000,1);//test demo
    // read_len = esp8285_recv(wifi_data,1024,NULL,600,1);

    if(ret>=0)// 0 recv end 1 recv other
    {
        wifi_data[read_len] = '\0';
            printf("wifi recv read_len:%d %s\n",read_len,wifi_data);
        wifi_parse_data(wifi_data,read_len);
    }
    return true;
}
int wifi_cmd_send(char *data,int len)
{
    return esp8285_send(data,len,200);
}
int wifi_cmd_recv(uint8_t * buffer,uint32_t buffer_size,uint32_t *read_len,uint32_t timeout, bool first_time_recv)
{
    return esp8285_recv(buffer,buffer_size,read_len,timeout,first_time_recv);
}