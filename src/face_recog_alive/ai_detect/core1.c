/*
 * @Author: Wangyg wangyeguang521@163.com
 * @Date: 2024-05-14 11:30:32
 * @LastEditors: yeguang wang wangyeguang521@163.com
 * @LastEditTime: 2024-07-11 14:55:41
 * @FilePath: \kendryte-standalone-sdk-new\src\face_recog_alive\ai_detect\core1.c
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
/*
 * @Author: Wangyg wangyeguang521@163.com
 * @Date: 2024-05-14 11:30:32
 * @LastEditors: Wangyg wangyeguang521@163.com
 * @LastEditTime: 2024-05-27 13:01:57
 * @FilePath: \kendryte-standalone-sdk-new\src\face_recog_alive\ai_detect\core1.c
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#include "core1.h"
#include "device_config.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "esp8285.h"
#include "wifi_function.h"
#include "user_cmd.h"
#include "sleep.h"
#include "uarths.h"
#include "face_info.h"
#include "atomic.h"

#define WIFI_SSID "beitababa"
#define WIFI_PASSWORD "12345678"

// uint8_t recv_data[1024];

// volatile Queue_t q_core0; // Queue declaration
volatile Queue_t q_core1; // Queue declaration
// static volatile spinlock_t q_core0_lock;
static volatile spinlock_t q_core1_lock;

static void queue_core1_lock(void)
{
    spinlock_lock(&q_core1_lock);
}

static void queue_core1_unlock(void)
{
    spinlock_unlock(&q_core1_lock);
}
/**
 * @brief  
 * @note   wifi 连接 tcp通信
 * @param  *ctx: 
 * @retval 
 */
int core1_function(void *ctx)
{
	board_cfg_t *g_config = (board_cfg_t *)ctx;
	char wifi_ssid[32];
    char wifi_passwd[32];
	uint64_t core = current_coreid();
    uint8_t is_wifi_connected = 0;
    uint8_t is_tcp_connected = 0;
    printf("Core %ld Start !!!!\n", core);

    memset(wifi_ssid,0,32*sizeof(uint8_t));
    memset(wifi_passwd,0,32*sizeof(uint8_t));
	if((strlen((const char *)g_config->wifi_ssid)>0) && (strlen((const char *)g_config->wifi_passwd)>0))
	{
		memcpy(wifi_ssid,g_config->wifi_ssid,strlen(g_config->wifi_ssid));
		memcpy(wifi_passwd,g_config->wifi_passwd,strlen(g_config->wifi_passwd));
	}
	else{
		memcpy(wifi_ssid,WIFI_SSID,strlen(WIFI_SSID));
		memcpy(wifi_passwd,WIFI_PASSWORD,strlen(WIFI_PASSWORD));
	}
    

    // //init serial
    /**串口控制流程：
     * 1. 初始化串口
     * 2. 循环接收串口数据
     * 3. 解析并处理串口任务
    */
    // user_cmd_init();
    //init wifi
    /**wifi控制流程：
	 * 0. 初始化wifi芯片，设置hostname
     * 1. 判断是否连接ap或有设备接入wifi
     * 2. 连接服务端或判断是否有客户端接入服务器
     * 3. 等待控制端下发指令
     * 4. 解析并处理wifi指令
    */
    bool ret = 0;
    wifi_init();
    ret = wifi_setHostname();
	printf("[esp8285]: Set hostname ret:%d\n",ret);
    int work_mode = 0;// 0:join wifi 1: 连接tcp 2:接收tcp数据
    int recv_len = 0;
    char server_ip[20]={0};
    int get_ip_time = 0;
    int connect_time = 0;

    bool queue_stat = false;
    net_task_t *task = NULL;
    uint8_t have_task = 0;

    while (1)
    {
        queue_core1_lock();
        if (!q_isEmpty(&q_core1))
        {
            queue_stat = q_pop(&q_core1, &task);
            if (queue_stat && task)
            {
                have_task = 1;
            }
        }
        queue_core1_unlock();
        if(have_task)
        {
            have_task = 0;
            if(task)
            {
                switch (task->task_type)
                {
                case TASK_UPLOAD:
                    /* code */
                    break;
                case TASK_CAL_PIC_FEA:
                    break;
                case TASK_CAL_PIC_FEA_RES:
                    break;
                default:
                    break;
                }
            }
        }
        // printf("core %ld is runing\n",core);
        // if((recv_len = user_cmd_recvdata(recv_data,1024))>0)
        // {
        //   // printf("recv data:len:%d %s\n",recv_len,recv_data);
        //   user_cmd_senddata(recv_data,recv_len);
        // }
		//deal serial port
        // user_cmd_process(); //
        
		//deal wifi port
		if(is_wifi_connected==0)//join ap or start ap
		{	
			// if(esp8285_JoinState()) //BUG:
			// {
			// 	get_ip(server_ip);
            //     printf("join ap OK server_ip is:%s\r\n",server_ip);
			// }
			// else 
            if(wifi_joinAp((const char *)wifi_ssid,(const char *)wifi_passwd))
			{
                get_ip_time = 0;
				is_wifi_connected = 1;
				printf("join ap ok\r\n");
				wifi_get_ip(server_ip);
			}
		}
        
		else if(is_tcp_connected == 0) //连接tcp，判断是否连接上ap
		{
            if(strlen(server_ip)<=0)
            {
                printf("server ip is null\r\n");
                wifi_get_ip(server_ip);
                get_ip_time++;
                if(get_ip_time == 5)
                {
                    get_ip_time = 0;
                    is_wifi_connected = 0;//多次未读取到重新连接ap
                }
            }
            else{
                printf("server ip is %s\n",server_ip);
                //connect to server
                ret = wifi_connect(server_ip,80);
                if(ret == false)
                {
                    connect_time++;
                    if(connect_time>=5)
                    {
                        connect_time = 0;
                        is_tcp_connected = 0;
                        is_wifi_connected = 0;//多次未连接
                    }
                }
                else
                {
                    connect_time = 0;
                    printf("connect to server success\n");
                    is_tcp_connected = 1;
                    wifi_cmd_send_login();
                }
            }
		}
        #if 1
		else//接收tcp数据      // 1. 判断是否连接ap或有设备接入wifi
		{
            // wifi_getTcpStatus();
			//  eAT();
            ret = wifi_cmd_process();
            if(ret == false)
            {
                is_tcp_connected = 0;
            }
            // wifi_cmd_send("hello world",13);
             //发送本机数据给服务器
             //循环接收服务器下发指令
		}
        #endif
        // msleep(200);
    }
    
#if 0
    esp8285_init();
    // setHostname();
    
    while(1)
    {
      
     msleep(100);
    }
#endif
}