/*
 * @Author: Wangyg wangyeguang521@163.com
 * @Date: 2024-05-30 10:58:49
 * @LastEditors: yeguang wang wangyeguang521@163.com
 * @LastEditTime: 2024-07-12 13:16:29
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
#include "cJSON.h" // 包含cJSON库头文件
#include "base64.h"
#include "face_info.h"
#include "rtc_time.h"

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

typedef struct{
    float fea_rgb[FEATURE_LEN]; //计算后的人脸特征值
    int8_t name[NAME_LEN];     //人员姓名
    uint32_t uid;                       //人脸对应的UID
    image_t *recv_image;   //用于处理接收到的图像 320*256*3 传给kpu_image
    int state;  //添加状态              添加成功为1 未成功为0
}add_fea_t;

volatile add_fea_t 

int wifi_cmd_senddata(uint8_t *buf,int len);

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
/**
 * @brief 发送设备初始化登录消息 
 * @note   
 * @retval None
 */
void wifi_cmd_send_login(void)
{
    char *json_string = malloc(256*sizeof(char));
    if(json_string == NULL)
    {
        return ;
    }
    memset(json_string,0,256);
    char unique[32]={0};
    snprintf(unique,32,"%lx",get_chip_number()); //machineID
    snprintf(json_string,256,"{\"version\":1,\"type\":\"init\",\"code\":0,\"msg\":\"init done\",\"sWversion\":\"%s\",\"machineID\":\"%s\"}", \
                                            g_board_cfg.swVersion,unique);
    // 打印 JSON 字符串
    printf("JSON output: %s\n", json_string);
    //发送数据
    wifi_cmd_senddata((uint8_t *)json_string,strlen(json_string));
}
/**
 * @brief 处理设置配置的功能命令
 *
 * @param data 接收到的数据，预期为JSON格式的字符串
 * @param len 数据的长度
 * @return int 返回0表示成功，返回-1表示出现错误
 */
static int wifi_cmd_set_cfg_func(uint8_t *data, int len) {
    // 解析JSON数据
    cJSON *json = cJSON_Parse((const char*)data);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "%s: Error before: %s\n", __func__, error_ptr);
        }
        char json_string[] = "{\"version\":1,\"type\":\"set_cfg_ret\",\"code\":-1,\"msg\":\" set config msg error\"}";
        wifi_cmd_senddata((uint8_t *)json_string,strlen(json_string));
        return -1;
    }

     // 获取"param"对象
    cJSON *param = NULL;
    param = cJSON_GetObjectItem(json, "param");
    
    if (cJSON_IsObject(param)) {
        
        cJSON *RTC = NULL;
        RTC = cJSON_GetObjectItem(param, "RTC");
        cJSON *acc_interval = NULL;
        acc_interval = cJSON_GetObjectItem(param, "acc_interval");
        cJSON *acc_switch = NULL;
        acc_switch = cJSON_GetObjectItem(param, "acc_switch");
        // cJSON *canBreak_switch = cJSON_GetObjectItemCaseSensitive(param, "canBreak_switch");
        cJSON *can_interval = NULL;
        can_interval = cJSON_GetObjectItem(param, "can_interval");
        cJSON *can_switch = NULL;
        can_switch = cJSON_GetObjectItem(param, "can_switch");
        cJSON *can_type = NULL;
        can_type = cJSON_GetObjectItem(param, "can_type");
        cJSON *fea_gate = NULL;
        fea_gate = cJSON_GetObjectItem(param, "fea_gate");
        // cJSON *lcd_interval = cJSON_GetObjectItemCaseSensitive(param, "lcd_interval");
        cJSON *relay_switch = NULL;
        relay_switch = cJSON_GetObjectItem(param, "relay_switch");
        cJSON *seat_interval = NULL;
        seat_interval = cJSON_GetObjectItem(param, "seat_interval");
        cJSON *seat_level = NULL;
        seat_level =cJSON_GetObjectItem(param, "seat_level");
        cJSON *seat_switch = NULL;
        seat_switch = cJSON_GetObjectItem(param, "seat_switch");
        // cJSON *uart_baud = cJSON_GetObjectItem(param, "uart_baud");
        // cJSON *work_mode = cJSON_GetObjectItem(param, "work_mode");

        //打印接收数据
        if (cJSON_IsString(RTC)) {
            printf("RTC: %s\n", RTC->valuestring);
            // int year = 0;
            // uint8_t month = 0,day = 0,weekday = 0,hour = 0,mins = 0,sec = 0;
            // 使用 strtok 分割字符串
            char* token = strtok(RTC->valuestring, "-_:");
            int i = 0;
            char temp[6][5] = {"","","","","",""};
            while (token != NULL) {
                strcpy(temp[i++], token);
                token = strtok(NULL, "-_:");
            }

            if (i == 7) {
                int year = atoi(temp[0]);
                int month = atoi(temp[1]);
                int day = atoi(temp[2]);
                int hour = atoi(temp[3]);
                int minute = atoi(temp[4]);
                int second = atoi(temp[5]);
                int extra = atoi(temp[6]);

                printf("Parsed Date and Time:\n");
                printf("Year  : %d\n", year);
                printf("Month : %d\n", month);
                printf("Day   : %d\n", day);
                printf("Hour  : %d\n", hour);
                printf("Minute: %d\n", minute);
                printf("Second: %d\n", second);
                printf("Extra : %d\n", extra);
                if(year>2020 && month>=1 && month <=12 && day>=1 && day <=31 && hour>=0 && hour<=23 && minute >=0 && minute<=59 && second>=0 && second<=59)
                {
                    //set rtc
                    ex_rtc_set_time(day,month,year,extra,hour,minute,second);
                }
            } else {
                printf("Failed to parse the input string\n");
            }
            // if(year>2020 && month>=1 && month <=12 && day>=1 && day <=31 && hour>=0 && hour<=23 && mins >=0 && mins<=59 && sec>=0 && sec<=59)
            // {
            //     //set rtc
            //     ex_rtc_set_time(day,month,year,weekday,hour,mins,sec);
            // }
            }
        
        if (cJSON_IsNumber(acc_interval)) {printf("acc_interval: %d\n", acc_interval->valueint); 
            g_board_cfg.acc_interval = acc_interval->valueint;
        }
        if (cJSON_IsBool(acc_switch)) {printf("acc_switch: %s\n", cJSON_IsTrue(acc_switch) ? "true" : "false");
            g_board_cfg.acc_switch=cJSON_IsTrue(acc_switch) ? 1:0;
        }
        // if (cJSON_IsBool(canBreak_switch)) {printf("canBreak_switch: %s\n", cJSON_IsTrue(canBreak_switch) ? "true" : "false");}
        if (cJSON_IsNumber(can_interval)) {printf("can_interval: %d\n", can_interval->valueint);
            g_board_cfg.can_ctrl_interval = can_interval->valueint;
        }
        if (cJSON_IsBool(can_switch)) {
            printf("can_switch: %s\n", cJSON_IsTrue(can_switch) ? "true" : "false");
            g_board_cfg.can_ctrl_switch =cJSON_IsTrue(can_switch)?1:0;
        }
        if (cJSON_IsNumber(can_type)) {printf("can_type: %d\n", can_type->valueint);
            g_board_cfg.can_type = can_type->valueint;
        }
        if (cJSON_IsNumber(fea_gate)) {printf("fea_gate: %d\n", fea_gate->valueint);
            g_board_cfg.fea_gate = fea_gate->valuedouble;
        }
        // if (cJSON_IsNumber(lcd_interval)) {printf("lcd_interval: %d\n", lcd_interval->valueint);}
        if (cJSON_IsBool(relay_switch)) {printf("relay_switch: %s\n", cJSON_IsTrue(relay_switch) ? "true" : "false");
            g_board_cfg.relay_switch = cJSON_IsTrue(relay_switch)?1:0;
        }
        if (cJSON_IsNumber(seat_interval)) {printf("seat_interval: %d\n", seat_interval->valueint);
            g_board_cfg.seat_interval = seat_interval->valueint;
        }
        if (cJSON_IsNumber(seat_level)) {printf("seat_level: %d\n", seat_level->valueint);
            g_board_cfg.seat_level = seat_level->valueint;
        }
        if (cJSON_IsBool(seat_switch)) {printf("seat_switch: %s\n", cJSON_IsTrue(seat_switch) ? "true" : "false");
            g_board_cfg.seat_switch =cJSON_IsTrue(seat_switch)?1:0;
        }
        // if (cJSON_IsNumber(uart_baud)) {printf("uart_baud: %d\n", uart_baud->valueint);}
        // if (cJSON_IsNumber(work_mode)) {printf("work_mode: %d\n", work_mode->valueint);}

        if (flash_save_cfg((board_cfg_t *)&g_board_cfg) == 0)
        {
            printf("save g_board_cfg failed!\r\n");
        }
        char json_string[] = "{\"version\":1,\"type\":\"set_cfg_ret\",\"code\":0,\"msg\":\" 配置修改成功\"}";
        wifi_cmd_senddata((uint8_t *)json_string,strlen(json_string));
    }

    cJSON_Delete(json);
    return 0;
}
/**
 * @brief 处理获取配置的功能命令
 *
 * @param data 接收到的数据，预期为JSON格式的字符串
 * @param len 数据的长度
 * @return int 返回0表示成功，返回-1表示出现错误
 */
static int wifi_cmd_get_cfg_func(uint8_t *data, int len) {
    // 解析JSON数据
    cJSON *json = cJSON_Parse((const char*)data);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "%s: Error before: %s\n", __func__, error_ptr);
        }
        return -1;
    }

    char unique[32]={0};
    snprintf(unique,32,"%lx",get_chip_number());
    int year = 0;
    uint8_t month = 0,day = 0,weekday = 0,hour = 0,mins = 0,sec = 0;
    ex_rtc_get_time(&day,&month,&year,&weekday,&hour,&mins,&sec);
     char *json_string = malloc(1024*sizeof(char));
    if(json_string == NULL)
    {
        return ;
    }
    memset(json_string,0,1024);
    snprintf(json_string,1024,"{\"version\":1,\"type\":\"get_cfg_ret\",\"code\":0,\"msg\":\"get config success\",\"param\":{"  \
                                            "\"version\":\"%s\",\"device_id\":\"%s\",\"fea_gate\":%d,\"can_type\":%d,\"seat_switch\":\"%s\","  \
                                            "\"acc_switch\":\"%s\",\"can_switch\":\"%s\",\"relay_switch\":\"%s\",\"seat_level\":%d,\"seat_interval\":%d,"  \
                                            "\"can_interval\":%d,\"acc_interval\":%d,\"RTC\":\"%04d-%02d-%02d_%02d:%02d:%02d_%d\"}}",  \
                                            g_board_cfg.swVersion,unique,(int)g_board_cfg.fea_gate,g_board_cfg.can_type,g_board_cfg.seat_switch==1?"True":"False",  \
                                            g_board_cfg.acc_switch==1?"True":"False",g_board_cfg.can_ctrl_switch==1?"True":"False",g_board_cfg.relay_switch==1?"True":"False",  \
                                            g_board_cfg.seat_level,g_board_cfg.seat_interval,g_board_cfg.can_ctrl_interval,g_board_cfg.acc_interval, \
                                            year,month,day,hour,mins,sec,weekday);
    // 打印 JSON 字符串
    printf("JSON output: %s\n", json_string);
    //发送数据
    wifi_cmd_senddata((uint8_t *)json_string,strlen(json_string));
    
    cJSON_Delete(json);
    return 0;
}
/**
 * @brief 处理图片特征计算的功能命令
 *
 * @param data 接收到的数据，预期为JSON格式的字符串
 * @param len 数据的长度
 * @return int 返回0表示成功，返回-1表示出现错误
 */
static int wifi_cmd_cal_pic_fea_func(uint8_t *data, int len) {
    cJSON *json = cJSON_Parse((const char*)data);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "%s: Error before: %s\n", __func__, error_ptr);
        }
        return -1;
    }
        // 获取"param"对象
    cJSON *param = NULL;
    param = cJSON_GetObjectItem(json, "param");
    // 处理图片特征计算逻辑
    //设置全局变量，通知core0计算人脸特征值
     if (cJSON_IsObject(param)) {
        cJSON *size_index = NULL;
        size_index = cJSON_GetObjectItem(param, "size");
        cJSON *sha256_index = NULL;
        sha256_index = cJSON_GetObjectItem(param, "sha256");
        cJSON *uid_index = NULL;
        uid_index = cJSON_GetObjectItem(param,"uid");
        cJSON *name_index = NULL;
        name_index = cJSON_GetObjectItem(param,"name");

        if (cJSON_IsNumber(size_index)) {printf("size: %d\n", size_index->valueint);
        }
        if (cJSON_IsString(sha256_index)) {printf("sha256: %s\n", sha256_index->valuestring);
        }
        if(cJSON_IsNumber(uid_index)){
            printf("uid:%d\n",uid_index->valueint);
        }
        if(cJSON_IsString(name_index)){
            printf("name:%s\n",name_index->valuestring);
        }
        //修改接收状态，准备接收数据
    }
    cJSON_Delete(json);
    return 0;
}
/**
 * @brief 处理发送图片数据的功能命令
 *
 * @param data 接收到的数据，预期为JSON格式的字符串
 * @param len 数据的长度
 * @return int 返回0表示成功，返回-1表示出现错误
 */
static int wifi_cmd_send_pic_data_func(uint8_t *data, int len) {
    cJSON *json = cJSON_Parse((const char*)data);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "%s: Error before: %s\n", __func__, error_ptr);
        }
        return -1;
    }
    // 处理发送图片数据逻辑
    cJSON_Delete(json);
    return 0;
}
/**
 * @brief 处理设置WiFi配置的功能命令
 *
 * @param data 接收到的数据，预期为JSON格式的字符串
 * @param len 数据的长度
 * @return int 返回0表示成功，返回-1表示出现错误
 */
static int wifi_cmd_set_wifi_cfg_func(uint8_t *data, int len) {
    cJSON *json = cJSON_Parse((const char*)data);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "%s: Error before: %s\n", __func__, error_ptr);
        }
        return -1;
    }
    // 处理设置WiFi配置的逻辑
    cJSON_Delete(json);
    return 0;
}
/**
 * @brief 处理获取WiFi配置的功能命令
 *
 * @param data 接收到的数据，预期为JSON格式的字符串
 * @param len 数据的长度
 * @return int 返回0表示成功，返回-1表示出现错误
 */
static int wifi_cmd_get_wifi_cfg_func(uint8_t *data, int len) {
    cJSON *json = cJSON_Parse((const char*)data);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "%s: Error before: %s\n", __func__, error_ptr);
        }
        return -1;
    }
    // 处理获取WiFi配置的逻辑
    cJSON_Delete(json);
    return 0;
}

/**
 * @brief 处理获取状态的功能命令
 *
 * @param data 接收到的数据，预期为JSON格式的字符串
 * @param len 数据的长度
 * @return int 返回0表示成功，返回-1表示出现错误
 */
static int wifi_cmd_get_status_func(uint8_t *data, int len) {
    cJSON *json = cJSON_Parse((const char*)data);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "%s: Error before: %s\n", __func__, error_ptr);
        }
        return -1;
    }
    // 处理获取状态的逻辑
    cJSON_Delete(json);
    return 0;
}

/**
 * @brief 处理删除用户的功能命令
 *
 * @param data 接收到的数据，预期为JSON格式的字符串
 * @param len 数据的长度
 * @return int 返回0表示成功，返回-1表示出现错误
 */
static int wifi_cmd_del_user_func(uint8_t *data, int len) {
    cJSON *json = cJSON_Parse((const char*)data);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "%s: Error before: %s\n", __func__, error_ptr);
        }
        return -1;
    }
    // 处理删除用户的逻辑
    cJSON_Delete(json);
    return 0;
}

/**
 * @brief 处理查询面部的功能命令
 *
 * @param data 接收到的数据，预期为JSON格式的字符串
 * @param len 数据的长度
 * @return int 返回0表示成功，返回-1表示出现错误
 */
static int wifi_cmd_query_face_func(uint8_t *data, int len) {
    cJSON *json = cJSON_Parse((const char*)data);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "%s: Error before: %s\n", __func__, error_ptr);
        }
        return -1;
    }
    // 处理查询面部的逻辑
    cJSON_Delete(json);
    return 0;
}

/**
 * @brief 处理获取记录的功能命令
 *
 * @param data 接收到的数据，预期为JSON格式的字符串
 * @param len 数据的长度
 * @return int 返回0表示成功，返回-1表示出现错误
 */
static int wifi_cmd_get_record_func(uint8_t *data, int len) {
    cJSON *json = cJSON_Parse((const char*)data);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "%s: Error before: %s\n", __func__, error_ptr);
        }
        return -1;
    }
    // 处理获取记录的逻辑
    cJSON_Delete(json);
    return 0;
}

/**
 * @brief 处理删除记录的功能命令
 *
 * @param data 接收到的数据，预期为JSON格式的字符串
 * @param len 数据的长度
 * @return int 返回0表示成功，返回-1表示出现错误
 */
static int wifi_cmd_del_record_func(uint8_t *data, int len) {
    cJSON *json = cJSON_Parse((const char*)data);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "%s: Error before: %s\n", __func__, error_ptr);
        }
        return -1;
    }
    // 处理删除记录的逻辑
    cJSON_Delete(json);
    return 0;
}

/**
 * @brief 处理设置FOTA数据的功能命令
 *
 * @param data 接收到的数据，预期为JSON格式的字符串
 * @param len 数据的长度
 * @return int 返回0表示成功，返回-1表示出现错误
 */
static int wifi_cmd_set_fota_data_func(uint8_t *data, int len) {
    cJSON *json = cJSON_Parse((const char*)data);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "%s: Error before: %s\n", __func__, error_ptr);
        }
        return -1;
    }
    // 处理设置FOTA数据的逻辑
    cJSON_Delete(json);
    return 0;
}

/**
 * @brief 处理调试设置的功能命令
 *
 * @param data 接收到的数据，预期为JSON格式的字符串
 * @param len 数据的长度
 * @return int 返回0表示成功，返回-1表示出现错误
 */
static int wifi_cmd_set_debug_func(uint8_t *data, int len) {
    cJSON *json = cJSON_Parse((const char*)data);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "%s: Error before: %s\n", __func__, error_ptr);
        }
        return -1;
    }
    // 处理调试设置的逻辑
    cJSON_Delete(json);
    return 0;
}

/**
 * @brief  
 * @note   
 * @param  *buf: 
 * @param  len: 
 * @retval 
 */
int wifi_cmd_senddata(uint8_t *buf,int len)
{
    return wifi_cmd_send(buf,len);
}
/**
 * @brief  初始化wifi功能
 * @note   
 * @retval 
 */
int wifi_init()
{
    int ret =  esp8285_init();
    wifi_callback_list_init(&wifi_callback_list);
        //注册回调函数
    // Initialize an array of callback entries
    static const uart_callback_entry_t wifi_callbacks[] = {
        {wifi_cmd_set_cfg_func, "set_cfg"},
        {wifi_cmd_get_cfg_func, "get_cfg"},
        {wifi_cmd_cal_pic_fea_func, "cal_pic_fea"},//下发人脸信息
        {wifi_cmd_send_pic_data_func, "send_pic_data"},
        {wifi_cmd_set_wifi_cfg_func, "set_wifi_cfg"},
        {wifi_cmd_get_wifi_cfg_func, "get_wifi_cfg"},
        {wifi_cmd_get_status_func, "get_status"},
        {wifi_cmd_del_user_func, "del_user"},
        {wifi_cmd_query_face_func, "query_face"},
        // {wifi_cmd_add_user_func,"add_user_by_fea"},
        {wifi_cmd_get_record_func, "get_record"},
        {wifi_cmd_del_record_func, "del_record"},
        {wifi_cmd_set_fota_data_func, "set_fota_data"},
        {wifi_cmd_set_debug_func, "set_debug"}
    };

    // Register all callbacks from the array
    for (size_t i = 0; i < sizeof(wifi_callbacks) / sizeof(wifi_callbacks[0]); ++i) {
        wifi_callback_list_add(&wifi_callback_list, wifi_callbacks[i].callback, wifi_callbacks[i].context);
    }
    return ret;
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
    if(ret == -4) //CLOSE
    {
        return false;
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