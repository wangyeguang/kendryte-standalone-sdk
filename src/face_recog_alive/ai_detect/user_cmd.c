/*
 * @Author: yeguang wang wangyeguang521@163.com
 * @Date: 2024-05-09 14:58:17
 * @LastEditors: Wangyg wangyeguang521@163.com
 * @LastEditTime: 2024-06-27 17:25:42
 * @FilePath: \kendryte-standalone-sdk-new\src\face_recog_alive\ai_detect\user_cmd.c
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#include "user_cmd.h"
#include "fpioa.h"
#include "uarths.h"
#include "uart.h"
#include "cJSON.h"
#include "base64.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "uarths.h"
#include "sysctl.h"
#include "device_config.h"
#include "face_info.h"
#include "rtc_time.h"

#define PROTOCOL_UART_NUM UART_DEVICE_1
#define PROTOCOL_BUF_LEN        (3 * 1024)
#define RECV_DMA_LENTH  6
static unsigned short CRC16_TABLE[] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241, 0xC601, \
    0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440, 0xCC01, 0x0CC0, \
    0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40, 0x0A00, 0xCAC1, 0xCB81, \
    0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841, 0xD801, 0x18C0, 0x1980, 0xD941, \
    0x1B00, 0xDBC1, 0xDA81, 0x1A40, 0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, \
    0x1DC0, 0x1C80, 0xDC41, 0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, \
    0x1680, 0xD641, 0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, \
    0x1040, 0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240, \
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441, 0x3C00, \
    0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41, 0xFA01, 0x3AC0, \
    0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840, 0x2800, 0xE8C1, 0xE981, \
    0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41, 0xEE01, 0x2EC0, 0x2F80, 0xEF41, \
    0x2D00, 0xEDC1, 0xEC81, 0x2C40, 0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, \
    0xE7C1, 0xE681, 0x2640, 0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, \
    0x2080, 0xE041, 0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, \
    0x6240, 0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441, \
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41, 0xAA01, \
    0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840, 0x7800, 0xB8C1, \
    0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41, 0xBE01, 0x7EC0, 0x7F80, \
    0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40, 0xB401, 0x74C0, 0x7580, 0xB541, \
    0x7700, 0xB7C1, 0xB681, 0x7640, 0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, \
    0x71C0, 0x7080, 0xB041, 0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, \
    0x5280, 0x9241, 0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, \
    0x5440, 0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40, \
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841, 0x8801, \
    0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40, 0x4E00, 0x8EC1, \
    0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41, 0x4400, 0x84C1, 0x8581, \
    0x4540, 0x8701, 0x47C0, 0x4680, 0x8641, 0x8201, 0x42C0, 0x4380, 0x8341, \
    0x4100, 0x81C1, 0x8081, 0x4040};
uint16_t calculate_crc(uint8_t *buffer,uint32_t len);

static unsigned char cJSON_recv_buf[PROTOCOL_BUF_LEN];

volatile uint8_t recv_over_flag = 0;
static volatile uint8_t start_recv_flag = 0;
static volatile uint32_t recv_cur_pos = 0;

// 协议开始标志，用于识别数据包的起点
static volatile uint8_t protocol_start_1 = 0xAA;
static volatile uint8_t protocol_start_2 = 0xBB;

// 协议结束标志，用于识别数据包的终点
static volatile uint8_t protocol_end_1 = '\r' ;
static volatile uint8_t protocol_end_2 = '\n' ;
int user_cmd_rx_any();
typedef struct{
    uint8_t *read_data;
    uint16_t data_len;
    uint16_t read_buf_len;
    uint16_t read_buf_head;
    uint16_t read_buf_tail;
}ring_buffer_t ;
static ring_buffer_t  recv_handle;
int is_new_data = 0;
char json_string[1024]={0};

/**
 * @brief  
 * @note   
 * @param  *data: 
 * @param  len: 
 * @retval None
 */
int user_cmd_set_cfg_func(uint8_t *data,int len)
{
    printf("user_cmd_set_cfg_func\n");
    //解析JSON数据
    cJSON *json = NULL;
    json = cJSON_Parse(data);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        char json_string[] = "{\"version\":1,\"type\":\"set_cfg_ret\",\"code\":-1,\"msg\":\" set config msg error\"}";
        user_cmd_senddata((uint8_t *)json_string,strlen(json_string));
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
        user_cmd_senddata((uint8_t *)json_string,strlen(json_string));
    }
    
    // 释放JSON对象
    cJSON_Delete(json);
    return 0;
}
int user_cmd_get_cfg_func(uint8_t *data,int len)
{
    printf("user_cmd_get_cfg_func\n");
    char unique[32]={0};
    snprintf(unique,32,"%lx",get_chip_number());
    int year = 0;
    uint8_t month = 0,day = 0,weekday = 0,hour = 0,mins = 0,sec = 0;
    ex_rtc_get_time(&day,&month,&year,&weekday,&hour,&mins,&sec);
    memset(json_string,0,1024);
    snprintf(json_string,1024,"{\"version\":1,\"type\":\"get_cfg_ret\",\"code\":0,\"msg\":\"get config success\",\"param\":{  \
                                            \"version\":\"%s\",\"device_id\":\"%s\",\"fea_gate\":%d,\"can_type\":%d,\"seat_switch\":\"%s\", \
                                            \"acc_switch\":\"%s\",\"can_switch\":\"%s\",\"relay_switch\":\"%s\",\"seat_level\":%d,\"seat_interval\":%d, \
                                            \"can_interval\":%d,\"acc_interval\":%d,\"RTC\":\"%04d-%02d-%02d_%02d:%02d:%02d_%d\"}}",  \
                                            g_board_cfg.swVersion,unique,(int)g_board_cfg.fea_gate,g_board_cfg.can_type,g_board_cfg.seat_switch==1?"True":"False",  \
                                            g_board_cfg.acc_switch==1?"True":"False",g_board_cfg.can_ctrl_switch==1?"True":"False",g_board_cfg.relay_switch==1?"True":"False",  \
                                            g_board_cfg.seat_level,g_board_cfg.seat_interval,g_board_cfg.can_ctrl_interval,g_board_cfg.acc_interval, \
                                            year,month,day,hour,mins,sec,weekday);
    // 打印 JSON 字符串
    printf("JSON output: %s\n", json_string);
    //发送数据
    user_cmd_senddata((uint8_t *)json_string,strlen(json_string));
    
    return 0;
}
int user_cmd_cal_pic_fea_func(uint8_t *data,int len)
{
    return 0;
}
int user_cmd_send_pic_data_func(uint8_t *data,int len)
{
    return 0;
}
int user_cmd_set_wifi_cfg_func(uint8_t *data,int len)
{
    return 0;
}
int user_cmd_get_wifi_cfg_func(uint8_t *data,int len)
{
    return 0;
}
int user_cmd_get_status_func(uint8_t *data,int len)
{
    return 0;
}
int user_cmd_del_user_func(uint8_t *data,int len)
{
    return 0;
}
int user_cmd_query_face_func(uint8_t *data,int len)
{
    return 0;
}
int user_cmd_get_record_func(uint8_t *data,int len)
{
    printf("user_cmd_get_record_func");
    //解析JSON数据
    cJSON *json = NULL;
    json = cJSON_Parse(data);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        char json_string[] = "{\"version\":1,\"type\":\"get_record_ret\",\"code\":-1,\"msg\":\" get record msg error\"}";
        user_cmd_senddata((uint8_t *)json_string,strlen(json_string));
        return -1;
    }
    
    // 获取"param"对象
    cJSON *param = NULL;
    param = cJSON_GetObjectItem(json, "param");
    
    if (cJSON_IsObject(param)) {
        cJSON *start_index = NULL;
        start_index = cJSON_GetObjectItem(param, "start");
        cJSON *end_index = NULL;
        end_index = cJSON_GetObjectItem(param, "end");
        if (cJSON_IsNumber(start_index)) {printf("start: %d\n", start_index->valueint);
        }
        if (cJSON_IsNumber(end_index)) {printf("end: %d\n", end_index->valueint);
        }
    }
    cJSON_Delete(json);
    return 0;
}
int user_cmd_del_record_func(uint8_t *data,int len)
{
    return 0;
}
int user_cmd_set_fota_data_func(uint8_t *data,int len)
{
    return 0;
}
int user_cmd_set_debug_func(uint8_t *data,int len)
{
    return 0;
}

static uint8_t uart_channel_getchar(uart_device_number_t channel, uint8_t *data);

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
} uart_callback_list_t;
uart_callback_list_t uart_callback_list;
// 初始化回调列表
void uart_callback_list_init(uart_callback_list_t *list)
{
    memset(list, 0, sizeof(uart_callback_list_t));
}

// 添加回调函数到列表
int uart_callback_list_add(uart_callback_list_t *list, uart_callback_t callback, void *context)
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
int uart_callback_list_remove(uart_callback_list_t *list, uart_callback_t callback)
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
void uart_callback_list_execute(uart_callback_list_t *list,char *context,uint8_t *data, int len)
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
 * @brief  串口数据接收中断
 * @note   
 * @param  *ctx: 
 * @retval 
 */
static int protocol_port_recv_cb(void *ctx)
{
    // static uint8_t last_data = 0;
    uint8_t tmp;
    // int len = 0;
    int read_ret = 0;
    do
    {   
        // 计算下一个读缓冲区头部位置
        uint16_t next_head = (recv_handle.read_buf_head+1) % recv_handle.read_buf_len;
         // 当缓冲区未满时，继续读取数据
        while(next_head != recv_handle.read_buf_tail)
        {
            read_ret = uart_channel_getchar(PROTOCOL_UART_NUM, &tmp);
            if(read_ret == 0)
                break;
            is_new_data = 1;
            recv_handle.read_data[recv_handle.read_buf_head] = tmp;
            recv_handle.read_buf_head = next_head;
            recv_handle.data_len++;
            next_head = (recv_handle.read_buf_head+1) % recv_handle.read_buf_len;
        }
        // 如果缓冲区已满，继续读取并丢弃数据，直到没有数据可读
        if(next_head == recv_handle.read_buf_tail)
        {
            do{
                read_ret = uart_channel_getchar(PROTOCOL_UART_NUM,&tmp);
            }while(read_ret != 0);
            break; // 当没有更多数据可读时退出循环
        }

    }while(read_ret!=0);
    return 0;
}

/**
 * @brief  
 * @note   初始化通信串口 使用串口1，波特率115200
 * @retval 
 */
int user_cmd_port_init()
{
    recv_handle.read_data = cJSON_recv_buf;
    recv_handle.read_buf_tail = 0;
    recv_handle.read_buf_head = 0;
    recv_handle.read_buf_len = PROTOCOL_BUF_LEN;
    recv_handle.data_len = 0;
    // fpioa_set_function(30,FUNC_UART1_RX+UART_DEV1*2);
    // fpioa_set_function(31,FUNC_UART1_TX+UART_DEV1*2);
    fpioa_set_function(4,FUNC_UART1_RX+UART_DEV1*2);
    fpioa_set_function(5,FUNC_UART1_TX+UART_DEV1*2);
    uart_init(PROTOCOL_UART_NUM);
    uart_configure(PROTOCOL_UART_NUM,115200,8,UART_STOP_1,UART_PARITY_NONE);
    uart_set_receive_trigger(PROTOCOL_UART_NUM,UART_RECEIVE_FIFO_1);
    uart_irq_register(PROTOCOL_UART_NUM,UART_RECEIVE,protocol_port_recv_cb,NULL,1);//中断函数接收数据到缓存区
    return 0;
}

int user_cmd_init(void)
{
    //初始化串口
    user_cmd_port_init();
    //初始化函数处理
    uart_callback_list_init(&uart_callback_list);
    //注册回调函数
    uart_callback_list_add(&uart_callback_list, user_cmd_set_cfg_func, "set_cfg");
    uart_callback_list_add(&uart_callback_list, user_cmd_get_cfg_func, "get_cfg");
    uart_callback_list_add(&uart_callback_list, user_cmd_cal_pic_fea_func, "cal_pic_fea");
    uart_callback_list_add(&uart_callback_list, user_cmd_send_pic_data_func, "send_pic_data");
    uart_callback_list_add(&uart_callback_list, user_cmd_set_wifi_cfg_func, "set_wifi_cfg");
    uart_callback_list_add(&uart_callback_list, user_cmd_get_wifi_cfg_func, "get_wifi_cfg");
    uart_callback_list_add(&uart_callback_list, user_cmd_get_status_func, "get_status");
    uart_callback_list_add(&uart_callback_list, user_cmd_del_user_func, "del_user");
    uart_callback_list_add(&uart_callback_list, user_cmd_query_face_func, "query_face");
    uart_callback_list_add(&uart_callback_list, user_cmd_get_record_func, "get_record");
    uart_callback_list_add(&uart_callback_list, user_cmd_del_record_func, "del_record");
    uart_callback_list_add(&uart_callback_list, user_cmd_set_fota_data_func, "set_fota_data");
    uart_callback_list_add(&uart_callback_list, user_cmd_set_debug_func, "set_debug");
    
    return 0;
}

int user_cmd_deinit(void)
{
    return 0;
}

static void parse_json(const char *json_string)
{
// 解析JSON字符串
    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL) {
        printf("Error before: [%s]\n", cJSON_GetErrorPtr());
        cJSON_Delete(root);
        return;
    }
    // 获取"param"对象
    // cJSON *param = cJSON_GetObjectItem(root, "param");
    // if ((param != NULL) && cJSON_IsObject(param)) {
    //     // cJSON *inner_param = cJSON_GetObjectItem(param, "param");
    //     // if (inner_param != NULL) {
    //     //     printf("param: %d\n", inner_param->valueint);
    //     // }
    // }
    
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
        uart_callback_list_execute(&uart_callback_list,type_value,(uint8_t *)json_string,strlen(json_string));
    }
#endif
    // 释放cJSON对象
    if(root) {cJSON_Delete(root); root = NULL;}
}
/**
 * @brief 处理传输数据
*/
int user_cmd_process()
{
    int recv_len = 0;
    uint64_t tim = 0;
    uint8_t step = 0; //0 head1 1 head2 3 len 4 data+crc
    uint8_t *frame_data = NULL;
    //循环接收处理串口数据 //设置超时时间10ms，超出10ms返回
    if(is_new_data || (user_cmd_rx_any())){is_new_data = 0;}else {return 0;}
    tim = sysctl_get_time_us();
    do{
        if((sysctl_get_time_us()-tim) > 20000)//200ms
        {
            // printf("recv timeout\n");
            break;
        }
        switch(step)
        {
            case 0://head1
            {
                uint8_t head_frame=0;
                if((recv_len = user_cmd_recvdata(&head_frame,1)) == 1)
                {
                    // printf("recv head1:%x\n",head_frame);
                    if(head_frame == protocol_start_1)
                    {
                        step = 1;
                    }
                }
            }
            break;
            case 1:
            {
                uint8_t head_frame = 0;
                if((recv_len == user_cmd_recvdata(&head_frame,1))==1)
                {
                    // printf("recv head2:%x\n",head_frame);
                    if(head_frame == protocol_start_2)
                    {
                        step = 2;
                    }
                    else if(head_frame == protocol_start_1)
                        step = 1;
                    else
                        step = 0;
                }
                else
                    step = 0;
            }
            break;
            case 2: //len data crc
            {
                uint32_t data_len = 0;
                if((recv_len = user_cmd_recvdata((uint8_t *)&data_len,4))==4)
                {
                    // printf("recv len:%x\n",data_len);
                    frame_data = (uint8_t *)malloc(data_len+1);
                    if(frame_data==NULL)
                    {
                        step = 0;
                        break;
                    }
                    int rx_len = user_cmd_rx_any();
                    if(rx_len<data_len) break;
                    recv_len = user_cmd_recvdata(frame_data,data_len);
                    if(recv_len != data_len)
                    {
                        printf("recv frame error: recv len:%d data_len:%d\n",recv_len,data_len);
                        step = 0;
                        break;
                    }
                    uint16_t crc16 = 0;
                    recv_len = user_cmd_recvdata((uint8_t *)&crc16,2);
                    if(recv_len != 2)
                    {
                        step = 0;
                        break;
                    }
                    uint16_t crc_result = calculate_crc(frame_data,data_len);
                    if(crc_result!=crc16)
                    {
                        printf("CRC ERROR calculate crc:%d recv crc:%d\n",crc_result,crc16);
                        step = 0;
                        break;
                    }
                    //parse json
                    frame_data[data_len] = '\0';
                    printf("recv_data:%s\n",frame_data);
                    parse_json((const char *)frame_data);
                    if(frame_data)
                    {
                        free(frame_data);
                        frame_data = NULL;
                    }
                        
                    printf("parse_json end\n");
                    step = 0;
                }
                else
                    step = 0;
            }
            break;
            default:
                break;
        }
    }while(1);
    if(frame_data)
        free(frame_data);
    return 0;
}
void print_recv(void)
{
    printf("recvdata len:%d %s\n",recv_cur_pos,cJSON_recv_buf);
    recv_cur_pos = 0;
}
int user_cmd_rx_char(void)
{
    if(recv_handle.read_buf_tail != recv_handle.read_buf_head)
    {
        uint8_t data;
        data = recv_handle.read_data[recv_handle.read_buf_tail];
        recv_handle.read_buf_tail = (recv_handle.read_buf_tail +1) % recv_handle.read_buf_len;
        recv_handle.data_len--;
        return data;
    }
    return -1;
}
int user_cmd_rx_data(uint8_t *buf_in,uint32_t size)
{
    uint16_t data_num = 0;
    uint8_t* buf = buf_in;
    while (recv_handle.read_buf_tail != recv_handle.read_buf_head && size>0)
    {
        *buf = recv_handle.read_data[recv_handle.read_buf_tail];
        recv_handle.read_buf_tail = (recv_handle.read_buf_tail+1) % recv_handle.read_buf_len;
        recv_handle.data_len--;
        buf++;
        data_num++;
        size--;
    }
    return data_num;
}
int user_cmd_rx_any()
{
    int buffer_bytes = recv_handle.read_buf_head-recv_handle.read_buf_tail;
    if(buffer_bytes<0)
    {
        return buffer_bytes+ recv_handle.read_buf_len;
    }
    else if(buffer_bytes >0)
    {
        return buffer_bytes;
    }
    else
        return 0;
}
int user_cmd_recvdata(uint8_t *buf,int len)
{
    int data_num = 0;
    int ret_num = 0;
    while(data_num<len)
    {
        ret_num = user_cmd_rx_data(buf+data_num,len-data_num);
        if(0!=ret_num)
        {
            data_num+=ret_num;
        }
        else if(0==ret_num || !user_cmd_rx_any())
        {
            break;
        }
    }
    if(data_num != 0)
    {
        return data_num;
    }
    else
    {
        return -1;
    }
}
/**
 * @brief  根据协议帧打包发送数据并调用发送函数发送
 * @note   发送帧格式 帧头+LEN+value+crc16
 * @param  *buf: 
 * @param  len: 
 * @retval 
 */
// uint8_t frame_header[6] = {0xAA,0xBB,0,0,0,0};
// uint8_t send_base64[1024];
int user_cmd_senddata(uint8_t *buf,int len)
{
    printf("user_cmd_senddata len:%d buf:%s\n",len,buf);
    uint8_t *frame_data = NULL;
    uint32_t frame_len = 0;
    frame_len = len+8;
    frame_data = (uint8_t *)malloc(frame_len*sizeof(uint8_t));
    if(frame_data == NULL)
    {
        printf("Memory allocation frame_data failed\n");
        return -1;
    }
    memset(frame_data,0,frame_len);
    frame_data[0] = 0xAA;
    frame_data[1] = 0xBB;//head
    frame_data[2] = (uint8_t)(len&0xFF);
    frame_data[3] = (uint8_t)(len>>8);
    frame_data[4] = (uint8_t)(len>>16);
    frame_data[5] = (uint8_t)(len>>24);
    // uart_send_data(PROTOCOL_UART_NUM,frame_header,6);
    memcpy(frame_data+6,buf,len);
    uint16_t crc_result = calculate_crc(buf,len);
    printf("crc_result:0x%x\n",crc_result);
    frame_data[frame_len-2] = (uint8_t)(crc_result&0xFF);
    frame_data[frame_len-1] = (uint8_t)(crc_result>>8);
    
    #if 1
    size_t output_length = 0;
    unsigned char *encoded_output = base64_encode(frame_data,frame_len,&output_length);
    printf("base64 encoded output length:%ld\n",output_length);
    // frame_data[output_length] = '\r';
    // frame_data[output_length+1] = '\n';
    printf("send encoded data:%s\n",frame_data);
    #endif
    int ret =  uart_send_data(PROTOCOL_UART_NUM,encoded_output,output_length);
    // int ret = uart_send_data(PROTOCOL_UART_NUM,frame_data,frame_len);
    ret = uart_send_data(PROTOCOL_UART_NUM,"\r\n",2);
    if(encoded_output!=NULL) {free(encoded_output);}

    if(frame_data != NULL)
    {
        free(frame_data);
    }

    // return ret;
    return 0;
}
extern volatile uart_t *const uart[3];
static uint8_t uart_channel_getchar(uart_device_number_t channel, uint8_t *data)
{
    /* If received empty */
    if (!(uart[channel]->LSR & 1))
    {
        return 0;
    }
    *data = (uint8_t)(uart[channel]->RBR & 0xff);
    return 1;
}

/**
 * @brief  
 * @note   输出数据不包含帧头及长度字节
 * @param  *buffer: 
 * @param  len: 
 * @retval 
 */
uint16_t calculate_crc(uint8_t *buffer,uint32_t len)
{
    unsigned short crc = 0xFFFF;
    crc = (crc >> 8) ^ CRC16_TABLE[((crc) ^ 0xAA) & 0xFF];
    crc = (crc >> 8) ^ CRC16_TABLE[((crc) ^ 0xBB) & 0xFF];
    crc = (crc >> 8) ^ CRC16_TABLE[((crc) ^ (uint8_t)(len&0xFF)) & 0xFF];
    crc = (crc >> 8) ^ CRC16_TABLE[((crc) ^ (uint8_t)((len>>8)&0xFF)) & 0xFF];
    crc = (crc >> 8) ^ CRC16_TABLE[((crc) ^ (uint8_t)((len>>16)&0xFF)) & 0xFF];
    crc = (crc >> 8) ^ CRC16_TABLE[((crc) ^ (uint8_t)((len>>24)&0xFF)) & 0xFF];
    for(int i=0;i<len;i++)
    {
        crc = (crc >> 8) ^ CRC16_TABLE[((crc) ^ (unsigned char)*(buffer+i)) & 0xFF];
    }
    uint8_t crc_L = (crc&0xFF00)>>8;
    uint8_t crc_H = crc&0xFF;
    crc = crc_L + (((uint16_t)crc_H)<<8);

    return crc;
}