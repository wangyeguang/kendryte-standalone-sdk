/*
 * @Author: Wangyg wangyeguang521@163.com
 * @Date: 2024-04-23 11:20:21
 * @LastEditors: yeguang wang wangyeguang521@163.com
 * @LastEditTime: 2024-07-04 07:03:32
 * @FilePath: \kendryte-standalone-sdk-new\src\face_recog_alive\ai_detect\esp8285.c
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
/*
 * @Author: Wangyg wangyeguang521@163.com
 * @Date: 2024-04-23 11:20:21
 * @LastEditors: Wangyg wangyeguang521@163.com
 * @LastEditTime: 2024-06-17 13:55:14
 * @FilePath: \kendryte-standalone-sdk-new\src\face_recog_alive\ai_detect\esp8285.c
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#include "esp8285.h"
#include <uart.h>
#include <fpioa.h>
#include <sysctl.h>
#include <string.h>
#include <stdlib.h>
#include <utils.h>
#include <sleep.h>
#include <gpio.h>
#include <gpiohs.h>
#include <spi.h>
#include "buffer.h"
#include <stdio.h>
#include "device_config.h"

#define WIFI_UART_NUM    UART_DEVICE_2
#define UART_GPIO_TX_PIN   (7)
#define UART_GPIO_RX_PIN   (6)
#define WIFI_EN_PIN (8)
#define WIFI_GPIOS_NUM_ENABLE (4)
#define UART_GPIO_TX_FUNC   (FUNC_UART1_TX+WIFI_UART_NUM*2)
#define UART_GPIO_RX_FUNC   (FUNC_UART1_RX+WIFI_UART_NUM*2)
#define WIFI_PIN_SPI_CS (0)
#define WIFI_GPIOS_NUM_CS (5)
#define WIFI_PIN_SPI_MISO   (2)
#define WIFI_PIN_SPI_MOSI   (3)
#define WIFI_PIN_SPI_SCLK   (1)
#define ESP8285_BUF_SIZE 5120 //10240
#define time_ms() (unsigned long)(read_cycle()/(sysctl_clock_get_freq(SYSCTL_CLOCK_CPU)/1000))
#define ESP8285_MAX_ONCE_SEND 2048
static uint8_t uart_buffer[ESP8285_BUF_SIZE];
volatile uint8_t g_net_status = 0;


typedef struct{
    uint8_t *read_data;
    uint16_t data_len;
    uint16_t read_buf_len;
    uint16_t read_buf_head;
    uint16_t read_buf_tail;
}ring_buffer_t ;
static ring_buffer_t  recv_handle;
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
 * @brief  uart RX IRQ
 * @note   
 * @param  *ctx: 
 * @retval 
 */

static int wifi_port_recv_cb(void *ctx)
{
 // static uint8_t last_data = 0;
    uint8_t read_tmp = 0;
    // int len = 0;
    int read_ret = 0;
    if(recv_handle.read_buf_len != 0)
    {
        do
        {   
            // 计算下一个读缓冲区头部位置
            uint16_t next_head = (recv_handle.read_buf_head+1) % recv_handle.read_buf_len;
            // 当缓冲区未满时，继续读取数据
            while(next_head != recv_handle.read_buf_tail)
            {
                read_ret = uart_channel_getchar(WIFI_UART_NUM, &read_tmp);
                if(read_ret == 0)
                    break;
                recv_handle.read_data[recv_handle.read_buf_head] = read_tmp;
                recv_handle.read_buf_head = next_head;
                recv_handle.data_len++;//实际接收大小
                next_head = (recv_handle.read_buf_head+1) % recv_handle.read_buf_len;
            }
            // 如果缓冲区已满，继续读取并丢弃数据，直到没有数据可读
            if(next_head == recv_handle.read_buf_tail)
            {
                do{
                    read_ret = uart_channel_getchar(WIFI_UART_NUM,&read_tmp);
                }while(read_ret != 0);
                break; // 当没有更多数据可读时退出循环
            }

        }while(read_ret!=0);
    }
    
    return 0;
}
static void kmp_get_next(const char* targe, int next[])
{  
    int targe_Len = strlen(targe);  
    next[0] = -1;  
    int k = -1;  
    int j = 0;  
    while (j < targe_Len - 1)  
    {     
        if (k == -1 || targe[j] == targe[k])  
        {  
            ++j;  
            ++k;   
            if (targe[j] != targe[k])  
                next[j] = k;    
            else   
                next[j] = next[k];  
        }  
        else  
        {  
            k = next[k];  
        }  
    }  
}  
static int kmp_match(char* src,int src_len, const char* targe, int* next)  
{  
    int i = 0;  
    int j = 0;  
    int sLen = src_len;  
    int pLen = strlen(targe);  
    while (i < sLen && j < pLen)  
    {     
        if (j == -1 || src[i] == targe[j])  
        {  
            i++;  
            j++;  
        }  
        else  
        {       
            j = next[j];  
        }  
    }  
    if (j == pLen)  
        return i - j;  
    else  
        return -1;  
} 
static int kmp_find(char* src,uint32_t src_len, const char* tagert)
{
	uint32_t index = 0;
	uint32_t tag_len = strlen(tagert);
	int* next = (int*)malloc(sizeof(uint32_t) * tag_len);
	kmp_get_next(tagert,next);
	index = kmp_match(src,src_len,tagert, next);
	free(next);
	return index;
}
static int data_find(uint8_t* src,uint32_t src_len, const char* tagert)
{
	return kmp_find((char*)src,src_len,tagert);
}

static int wifi_uart_rx_char(void)
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

/**
 * @brief  从串口接收buffer获取数据
 * @note   
 * @param  *buf_in: 
 * @param  size: 
 * @retval 
 */
static int wifi_uart_rx_data(uint8_t *buf_in,uint32_t size)
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
/**
 * @brief  计算buffer缓存有效数据大小
 * @note   
 * @retval 返回实际数据大小
 */
static int wifi_uart_rx_any()
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
static bool wifi_uart_rx_wait(uint32_t timeout) 
{
    uint32_t start = time_ms();
	// printf("uart_rx_wait | read_buf_head = %d\r\n",recv_handle.read_buf_head);
	// printf("uart_rx_wait | read_buf_tail = %d\r\n",recv_handle.read_buf_tail);
    for (;;) {
        if (recv_handle.read_buf_tail != recv_handle.read_buf_head) {
            return true; // have at least 1 char ready for reading
        }
        if (time_ms() - start >= timeout) {
            return false; // timeout
        }
    }
}
int wifi_uart_recvdata(uint8_t *buf,int len)
{
    int data_num = 0;
    
    if(wifi_uart_rx_wait(300)) //100ms timeout
    {
        int ret_num = 0;   
        while(data_num<len)
        {
            ret_num = wifi_uart_rx_data(buf+data_num,len-data_num);
            if(0!=ret_num)
            {
                data_num+=ret_num;
            }
            else if(0==ret_num || !wifi_uart_rx_any())
            {
                break;
            }
            // msleep(5);
        }
    }
    
    if(data_num != 0)
    {
        return data_num;
    }
    else
    {
        // printf("[ wifi_uart_recvdata ] return error\r\n");
        return -1;
    }
}
int esp8285_uart_senddata(const char *buf,int len)
{
    uint8_t *src = (uint8_t*)buf;
    size_t num_tx = 0;
	size_t cal = 0;
    while (num_tx < len) {
			cal= uart_send_data(WIFI_UART_NUM, (const char*)(src+num_tx), len - num_tx);
 	        num_tx = num_tx + cal;
	    }
    return num_tx;
}
void wifi_rx_empty(void) 
{
    while(1)
    {
        int ret = wifi_uart_rx_char();
        if(ret == -1)
        {
            break;
        }
    }
}
static char* recvString_1(const char* target1,uint32_t timeout,int *recv_len)
{
	int iter = 0;
    int len = 0;
    int max_len = 1024;
	// memset(uart_buffer,0,ESP8285_BUF_SIZE);
    uint8_t *recv_buffer = malloc(max_len*sizeof(uint8_t));
    if(recv_buffer == NULL)
    {   
        max_len = 256;
        recv_buffer = malloc(max_len*sizeof(uint8_t));
        if(recv_buffer == NULL)
        {
                printf("recvString_1 malloc recv_buf error\n");
                wifi_rx_empty();
                return NULL;
        }
    }
    unsigned long start = time_ms();
    while (time_ms() - start < timeout) {
        do{
            len = wifi_uart_recvdata((recv_buffer+iter),(max_len-iter));
            // printf("wifi serial recv data:%s",recv_buffer);
            if(len > 0)
            {
                iter += len;
                if(iter >= max_len)
                {
                    break;
                }
            }
            else
                break;
        }while (1);
        if (data_find(recv_buffer,iter,target1) != -1) {
            // printf("wifi recvString_1 data: %s\n",recv_buffer);
            *recv_len = iter;
            return (char*)recv_buffer;
        }
        // msleep(10); 
    }
    if(recv_buffer)
        free(recv_buffer);
    return NULL;
}

static char* recvString_2(char* target1, char* target2, uint32_t timeout, int8_t* find_index)
{
	int iter = 0;
    int len = 0;
    *find_index = -1;
    int max_len = 1024;
	// memset(uart_buffer,0,ESP8285_BUF_SIZE);
    uint8_t *recv_buffer = malloc(max_len*sizeof(uint8_t));
    if(recv_buffer == NULL)
    {
        max_len=256;
        recv_buffer = malloc(max_len*sizeof(uint8_t));
    }
    unsigned int start = time_ms();
    while (time_ms() - start < timeout) {
        while ((len = wifi_uart_recvdata((recv_buffer+iter),(max_len-iter))) > 0) {  
            iter += len;
            if(iter >= max_len)
            {
                iter = 0; // buffer full,reset iter.
                break;
            }
        }
        if (data_find(recv_buffer,iter,target1) != -1) {
            *find_index = 0;
            printf("wifi recvString_2 data: %s\n",recv_buffer);
            return (char*)recv_buffer;
        } else if (data_find(recv_buffer,iter,target2) != -1) {
            *find_index = 1;
            printf("wifi recvString_2 data: %s\n",recv_buffer);
            return (char*)recv_buffer;
        }
        // msleep(10); 
    }
    free(recv_buffer);
    return NULL;
}

static char* recvString_3(char* target1, char* target2,char* target3,uint32_t timeout, int8_t* find_index)
{
	int iter = 0;
    *find_index = -1;
    int len = 0;
    int max_len = 1024;
	// memset(uart_buffer,0,ESP8285_BUF_SIZE);
    uint8_t *recv_buffer = malloc(max_len*sizeof(uint8_t));
    if(recv_buffer == NULL)
    {
        max_len=256;
        recv_buffer = malloc(max_len*sizeof(uint8_t));
    }
    unsigned long start = time_ms();
    while (time_ms() - start < timeout) {
        while ((len = wifi_uart_recvdata((recv_buffer+iter),(max_len - iter))) > 0) {
            iter += len;
            if(iter >= max_len)
            {
                iter = 0; // buffer full,reset iter.
                break;
            }
        }
        if (data_find(recv_buffer,iter,target1) != -1) {
            *find_index = 0;
            return (char*)recv_buffer;
        } else if (data_find(recv_buffer,iter,target2) != -1) {
            *find_index = 1;
            return (char*)recv_buffer;
        } else if (data_find(recv_buffer,iter,target3) != -1) {
            *find_index = 2;
            return (char*)recv_buffer;
        }
        // msleep(10); 
    }
    printf("wifi recvString_3 data: %s\n",uart_buffer);
    return NULL;
}


static bool recvFind(const char* target, uint32_t timeout)
{
    int recv_len = 0;
    char *recv_data = recvString_1(target, timeout,&recv_len);
    if (data_find((uint8_t *)recv_data,recv_len,target) != -1) {
        if(recv_data)
            free(recv_data);
        return true;
    }
    if(recv_data)
        free(recv_data);
    return false;
}

static bool recvFindAndFilter(const char* target, const char* begin, const char* end, char** data, uint32_t timeout)
{
     int recv_len = 0;
    char *recv_data = recvString_1(target, timeout,&recv_len);
    if (data_find((uint8_t *)recv_data,recv_len,target) != -1) {
        free(recv_data);
        int32_t index1 = data_find(recv_data,recv_len,begin);
        int32_t index2 = data_find(recv_data,recv_len,end);
        if (index1 != -1 && index2 != -1) {
            index1 += strlen(begin);
            printf("recvFindAndFilter index2:%d index1:%d\n",index2,index1);
			*data = malloc(sizeof(char)*(index2 - index1));
            if(*data)
			    memcpy(*data,recv_data+index1, index2 - index1);
            free(recv_data);
            return true;
        }
    }
    free(recv_data);
    return false;
}

void get_ip(char *server_ip)
{
  ipconfig_obj data={0};
  get_ipconfig(&data);
  printf("ipconfig.IP=%s\n",data.ip);
  printf("ipconfig.gateway=%s\n",data.gateway);
    if(server_ip)
  {
    if(strlen(data.gateway))
    {
        strcpy(server_ip,data.gateway);
    }
  }
  printf("ipconfig.netmask=%s\n",data.netmask);
  printf("ipconfig.ssid=%s\n",data.ssid);

}

bool qATCWJAP_CUR()
{
	const char* cmd = "AT+CWJAP_CUR?";
	wifi_rx_empty();
	esp8285_uart_senddata(cmd,strlen(cmd));
    esp8285_uart_senddata("\r\n",strlen("\r\n"));
	return recvFind("OK",1000);
}
bool qATCWJAP()
{
	const char* cmd = "AT+CWJAP?";
	wifi_rx_empty();
	esp8285_uart_senddata(cmd,strlen(cmd));
    esp8285_uart_senddata("\r\n",strlen("\r\n"));
	return recvFind("OK",1000);
}

bool eAT(void)
{	
	const char* cmd = "AT\r\n";
    wifi_rx_empty();// clear rx
	esp8285_uart_senddata(cmd,strlen(cmd));// send cmd to esp8285
    if(recvFind("OK",1000))
        return true;
    wifi_rx_empty();
    esp8285_uart_senddata(cmd,strlen(cmd));// send cmd to esp8285
    if(recvFind("OK",1000))
        return true;
    wifi_rx_empty();
    esp8285_uart_senddata(cmd,strlen(cmd));// send cmd to esp8285
    return recvFind("OK",1000);
}

bool eATE(bool enable)
{	
    wifi_rx_empty();// clear rx
    if(enable)
    {
    	const char* cmd = "ATE0\r\n";
		esp8285_uart_senddata(cmd,strlen(cmd));// send cmd to esp8285
    	return recvFind("OK",1000);
    }
	else
	{
    	const char* cmd = "ATE1\r\n";
		esp8285_uart_senddata(cmd,strlen(cmd));// send cmd to esp8285
    	return recvFind("OK",1000);		
	}
}

/**
 * @brief  
 * @note   
 * @retval 
 */
static bool eATRST(void) 
{
	const char* cmd = "AT+RST\r\n";
    wifi_rx_empty();// clear rx
    int send_len = esp8285_uart_senddata(cmd,strlen(cmd));// send cmd to esp8285
    printf("[esp8285] send %s",cmd);
    return recvFind("OK",1000);
}

static bool eATGMR(char** version)
{
	const char* cmd = "AT+GMR\r\n";
    wifi_rx_empty();// clear rx
    esp8285_uart_senddata(cmd,strlen(cmd));// send cmd to esp8285
    return recvFindAndFilter("OK", "\r\n", "\r\nOK", version, 5000); 
}

bool qATCWMODE(char* mode) 
{
	const char* cmd = "AT+CWMODE?\r\n";
    char* str_mode;
    bool ret;
    if (!mode) {
        return false;
    }
    wifi_rx_empty();
	esp8285_uart_senddata((uint8_t*)cmd,strlen(cmd));// send cmd to esp8285
    ret = recvFindAndFilter("OK", "+CWMODE:", "\r\n\r\nOK", &str_mode,1000); 
    if (ret) {
        *mode = atoi(str_mode);
        return true;
    } else {
        return false;
    }
}
bool sATCWMODE(char mode)
{
	// const char* cmd = "AT+CWMODE=";
	char cmd[20] = {0};
    int8_t find;
    wifi_rx_empty();
    snprintf(cmd,20,"AT+CWMODE=%d\r\n",mode);// send cmd to esp8285
	esp8285_uart_senddata((const char *)cmd,strlen(cmd));// send cmd to esp8285
    // msleep(20);
    char *recv_data = recvString_2("OK", "no change",1000, &find);
    if(recv_data != NULL)
    {
        free(recv_data);
        return true;
    }
        
    return false;
}

bool sATCWAUTOCONN(int enable)
{
    char cmd[20]={0};
    int8_t find;
    wifi_rx_empty();
    snprintf(cmd,20,"AT+CWAUTOCONN=%d\r\n",enable);
    esp8285_uart_senddata(cmd,strlen(cmd));// send cmd to esp8285
    return recvFind("OK",1000);
}
/**
 * @brief  
 * @note   查询 TCP/UDP/SSL 连接状态和信息
 * @param  list: 
 * @retval 
 */
bool eATCIPSTATUS(char** list)
{
	const char* cmd = "AT+CIPSTATUS\r\n";
    msleep(100);
    wifi_rx_empty();
    esp8285_uart_senddata(cmd,strlen(cmd));
    return recvFindAndFilter("OK", "\r\r\n", "\r\n\r\nOK", list,1000);
}

/**
 * @brief  
 * @note   设置 ESP Station 的主机名称
 * @param  *hostname: 
 * @retval 
 */
bool esp8285_setHostname(const char *hostname)
{
	char cmd[50] = {0};
    int8_t find;
    wifi_rx_empty();
    snprintf(cmd,50,"AT+CWHOSTNAME=\"%s\"\r\n",hostname);// send cmd to esp8285
	esp8285_uart_senddata(cmd,strlen(cmd));// send cmd to esp8285
    msleep(20);
    return recvFind("OK",1000);
}
/**
 * @brief  断开与AP的连接
 * @note   
 * @retval 
 */
bool eATCWQAP(void)
{
	const char* cmd = "AT+CWQAP\r\n";
    wifi_rx_empty();
	esp8285_uart_senddata(cmd,strlen(cmd));// send cmd to esp8285
    return recvFind("OK",1000);
}

bool esp8285_joinAP( const char* ssid, const char* pwd)
{
    char cmd[50]={0};
    int8_t find;
    wifi_rx_empty();	
    snprintf(cmd,50,"AT+CWJAP=\"%s\",\"%s\"\r\n",ssid,pwd);// send cmd to esp8285
    printf("[esp8285]: joinAP cmd:%s\n",cmd);
	esp8285_uart_senddata((const char *)cmd,strlen(cmd));// send cmd to esp8285
    char *recv_data = recvString_2("OK", "FAIL", 6000, &find);
    if(recv_data != NULL)
    {
        free(recv_data);
        if( find==0)
            return true;
    }
    else if(recv_data != NULL)
    {
        free(recv_data);
    }
       
    return false;
}
bool esp8285_JoinState(void)
{
    bool ret = qATCWJAP();
    if(ret)
    {
        printf("get join state:%s\r\n",(char *)uart_buffer);
    }
    return ret;
}

bool esp8285_sATCIPSTARTSingle(const char* addr, uint32_t port)
{
	// const char* cmd = "AT+CIPSTART=\"";
    char cmd[100]={0};
	// mp_obj_t IP = netutils_format_ipv4_addr((uint8_t*)addr,NETUTILS_BIG);
	// const char* host = mp_obj_str_get_str(IP);
	// char port_str[10] = {0};
    int8_t find_index;
	// itoa(port, port_str, 10);
	wifi_rx_empty();
    snprintf(cmd,100,"AT+CIPSTART=\"TCP\",\"%s\",%d\r\n",addr,port);
    printf("[esp8285] send connect tcpserver cmd:%s\n",cmd);
	esp8285_uart_senddata((const char *)cmd,strlen(cmd));// send cmd to esp8285
    char *recv_data=recvString_3("OK", "ERROR", "ALREADY CONNECT", 10000, &find_index);
    if(recv_data!=NULL)
    {
        free(recv_data);
        if(find_index==0 || find_index==2)
            return true;
    }
    return false;
}
bool esp8285_sATCIPSTARTMultiple(char mux_id, char* type, char* addr, uint32_t port)
{
	// const char* cmd = "AT+CIPSTART=";
	// char port_str[10] = {0};
    int8_t find_index;
    char cmd[100]={0};
	// itoa(port,port_str ,10);
	wifi_rx_empty();
    snprintf(cmd,100,"AT+CIPSTART=%d,\"%s\",\"%s\",%d\r\n",mux_id,type,addr,port);
	esp8285_uart_senddata((const char *)cmd,strlen(cmd));// send cmd to esp8285
    char *recv_data=recvString_3("OK", "ERROR", "ALREADY CONNECT", 10000, &find_index);
    if( recv_data!= NULL)
    {
        free(recv_data);
        if(find_index==0 || find_index==2)
            return true;
    }
    return false;
}
bool esp8285_sATCIPSENDSingle(const char* buffer, uint32_t len, uint32_t timeout)
{
	// const char* cmd = "AT+CIPSEND=";
	// char len_str[10] = {0};
    char cmd[50]={0};
	wifi_rx_empty();
    snprintf(cmd,50,"AT+CIPSEND=%d\r\n",len);
    esp8285_uart_senddata((const char *)cmd,strlen(cmd));// send cmd to esp8285
    if (recvFind(">", 5000)) {
        wifi_rx_empty();
		esp8285_uart_senddata(buffer,len);// send cmd to esp8285
        return recvFind("SEND OK", timeout);
    }
    return false;
}
bool esp8285_sATCIPSENDMultiple(char mux_id, const char* buffer, uint32_t len)
{
	// const char* cmd = "AT+CIPSEND=";
	// char len_str[10] = {0};
	char cmd[50]={0};
	wifi_rx_empty();
    snprintf(cmd,50,"AT+CIPSEND=%d,%d",mux_id,len);
    esp8285_uart_senddata((const char *)cmd,strlen(cmd));// send cmd to esp8285

    if (recvFind(">", 5000)) {
        wifi_rx_empty();
		esp8285_uart_senddata(buffer,len);// send cmd to esp8285
        esp8285_uart_senddata("\r\n",strlen("\r\n"));// send cmd to esp8285
        return recvFind("SEND OK", 10000);
    }
    return false;
}
bool esp8285_sATCIPCLOSEMulitple(char mux_id)
{
	// const char* cmd = "AT+CIPCLOSE=";
    int8_t find;
    char cmd[50]={0};
	wifi_rx_empty();
    snprintf(cmd,50,"AT+CIPCLOSE=%d\r\n",mux_id);// send cmd to esp8285
	esp8285_uart_senddata((const char *)cmd,strlen(cmd));// send cmd to esp8285
    char *recv_data = recvString_2("OK", "link is not", 5000, &find);
    if( recv_data!= NULL)
    {
        free(recv_data);
        return true;
    }
        
    return false;
}
bool esp8285_eATCIPCLOSESingle()
{

    int8_t find;
	const char* cmd = "AT+CIPCLOSE\r\n";
    wifi_rx_empty();
	esp8285_uart_senddata(cmd,strlen(cmd));// send cmd to esp8285
    char *recv_data = recvString_2("OK", "ERROR", 5000, &find);
    if (recv_data != NULL)
    {
        free(recv_data);
        if( find == 0)
            return true;
    }
    return false;
}

int esp8285_send(const char *buffer,uint32_t len,uint32_t timeout)
{
    uint32_t send_total_len = 0;
    bool ret = false;
    uint16_t send_len = 0;
    while(send_total_len < len)
    {
        send_len = ((len-send_total_len) > ESP8285_MAX_ONCE_SEND ? ESP8285_MAX_ONCE_SEND :(len-send_total_len));
        ret = esp8285_sATCIPSENDSingle(buffer+send_total_len,send_len,timeout);
        if(!ret)
        {
            return send_total_len;
        }
        send_total_len  += send_len;
    }
    return send_total_len;
}
/**
 * @brief  
 * @note   
 * @param  *buffer: 
 * @param  buffer_size: 
 * @param  *read_len: 
 * @param  timeout: 
 * @retval -1: error -2:EOF -3:timeout -4:close
 */
int esp8285_recv(uint8_t *out_buff,uint32_t out_buff_len,uint32_t *read_len,uint32_t timeout, bool first_time_recv)
{
    return wifi_uart_recvdata(out_buff,out_buff_len);
    if (out_buff == NULL)
    {
        return -1;
    }
    int ret = 0;
    uint8_t recv_buffer[1024] = {0};
    uint32_t recv_size = 0;
    static int32_t frame_sum = 0, frame_len = 0, frame_bak = 0;
    recv_size = out_buff_len > 1024? 1024:out_buff_len;
    int recv_len = wifi_uart_recvdata(recv_buffer,recv_size);
   
    if(recv_len > 0)
    { printf("recv_len:%d recv_size:%d\r\n",recv_len,recv_size);
        if(first_time_recv)
        {
            const char *ipd_str = "+IPD,";
            char *ipd_start = strstr(recv_buffer, ipd_str);

            if (ipd_start != NULL) {
                // ipd_start += strlen(ipd_str); // 移动到"+IPD,"之后的数据
                printf("Found +IPD, at position: %ld %c\n", ipd_start - (char *)&recv_buffer[0],*ipd_start);
                if (sscanf(ipd_start, "+IPD,%d:", &frame_len) == 1) {
                    char *data_start = strchr(ipd_start, ':') + 1;
                    int size = recv_len-(data_start-(char *)&recv_buffer[0]);
                    size = frame_len>size?size:frame_len;
                    printf("Data length: %d size:%d recv_len:%d\n", frame_len,size,recv_len);
                    printf("Data: %.*s\n", frame_len, data_start);
                    // int pos = data_start-recv_buffer;
                    
                    memcpy(out_buff,recv_buffer,size);
                    if (read_len) {
                        *read_len = size;
                    }
                    frame_len -=size;
                    if(frame_len>0) ret = 1;
                    else
                    ret = 0;
                } else {
                    printf("Failed to parse data length.\n");
                    ret = -1;
                }
            } else {
                printf("No +IPD, found in the data.\n");
                ipd_start = strstr(recv_buffer,"CLOSED\r\n");
                if(ipd_start != NULL)
                {
                    ret = -4;
                }
                else
                    ret = -1;
            }
        }
        else
        {

        }
        
    }
    return ret;
    #if 0
    int err = 0, size = 0;
    

    
    static bool socket_eof = false;
    static int8_t mux_id = -1;
    static int32_t frame_sum = 0, frame_len = 0, frame_bak = 0;
    enum fsm_state { IDLE, IPD, DATA, EXIT } State = IDLE;//数据接收状态
    uint32_t  interrupt = get_time_ms(),uart_recv_len = 0, res = 0;
    uint32_t tmp_bak = 0, tmp_state = 0, tmp_len = 0, tmp_pos = 0, tmp_eof = 0;
    // printf("start recv interrupt:%d\n",interrupt);
    /*判断是否为第一次接收，如果为接收第一帧则解析帧头*/
    if (first_time_recv) {
        frame_sum = frame_len = 0;
        socket_eof = false;
    }
    

    if(frame_len>0)
    {
        int length = out_buff_len > frame_len ? frame_len: out_buff_len;
        tmp_len = 0;
        length = length > 1024 ? 1024: length;
        printf("data have data: tmp_len:%d length:%d frame_len:%d\n",tmp_len,length,frame_len);
        while((get_time_ms()-interrupt) < timeout)
        {
            
            res = wifi_uart_recvdata(recv_buffer+tmp_len,length-tmp_len);
            // printf("res=%d\n",res);
            if(res>0)
            {
                interrupt = get_time_ms();
                tmp_len += res;
            }
            else
            {
                continue;
            }
            if(length==tmp_len)
            {
                break;
            }
        }
        frame_len -= tmp_len;
        // if(tmp_len==0) 
        frame_sum = frame_len = 0;
        socket_eof = false;
        printf("tmp_len:%d frame_len:%d size:%d uart_recv_len:%d\n",tmp_len,frame_len,size,uart_recv_len);
        memcpy(out_buff,recv_buffer,size);
        if (read_len) {
            *read_len = size;
        }
        return tmp_len;
    }

    while(socket_eof == false && State != EXIT)
    {
        // uart_recv_len = wifi_uart_rx_any();//判断数据缓存区长度
        // if(uart_recv_len > 0)
       {
            if(tmp_len >= 1024)
            {
                break;
            }
            res = wifi_uart_recvdata(recv_buffer+tmp_len,1);//读取一个字节
            if(res == 1)
            {
                interrupt = get_time_ms();
                // printf("interrupt:%d\n",interrupt);
                tmp_pos = tmp_len, tmp_len += 1; // buffer push
            
                if (State == IDLE) {
                    if (recv_buffer[tmp_pos] == '+') {
                        tmp_state = 1, tmp_bak = tmp_pos, State = IPD;//tmp_bak记录帧头起始字符位置
                        continue;
                    } else {
                        printf("other (%02X) %c\n", recv_buffer[tmp_pos],recv_buffer[tmp_pos]);
                        tmp_len -= 1; // clear don't need data, such as (0D)(0A)
                        continue;
                    }
                    continue;
                }
                if (State == IPD) {

                    if (tmp_pos - tmp_bak > 12) { // Over the length of the '+IPD,3,1452:' or '+IPD,1452:'
                        tmp_state = 0, State = IDLE;
                        continue;
                    }

                    if (0 < tmp_state && tmp_state < 5) {
                        printf("(%d, %02X) [%d, %02X]\n", tmp_pos, recv_buffer[tmp_pos], tmp_pos - tmp_bak, ("+IPD,")[tmp_pos - tmp_bak]);
                        if (recv_buffer[tmp_pos] == ("+IPD,")[tmp_pos - tmp_bak]) {
                            tmp_state += 1; // tmp_state 1 + "IPD," to tmp_state 5
                        } else {
                            tmp_state = 0, State = IDLE;
                        }
                        continue;
                    }

                    if (tmp_state == 5 && recv_buffer[tmp_pos] == ':')
                    {
                        tmp_state = 6, State = IDLE;
                        recv_buffer[tmp_pos + 1] = '\0'; // lock recv_buffer
                        printf("%s | is `IPD` tmp_bak %d tmp_len %d command %s\n", __func__, tmp_bak, tmp_len, recv_buffer + tmp_bak);
                        char *index = strstr((char *)recv_buffer + tmp_bak + 5 /* 5 > '+IPD,' and `+IPD,325:` in recv_buffer */, ",");
                        int ret = 0, len = 0;
                        if (index) { // '+IPD,3,1452:'
                            ret = sscanf((char *)recv_buffer + tmp_bak, "+IPD,%hhd,%d:", &mux_id, &len);
                            if (ret != 2 || mux_id < 0 || mux_id > 4 || len <= 0) {
                                ; // Misjudge or fail, return, or clean up later
                            } else {
                                tmp_len = tmp_bak, tmp_bak = 0; // Clean up the commands in the buffer and roll back the data
                                frame_len = len, State = DATA;
                            }
                        } else { // '+IPD,1452:'
                            ret = sscanf((char *)recv_buffer + tmp_bak, "+IPD,%d:", &len);
                            if (ret != 1 || len <= 0) {
                                ; // Misjudge or fail, return, or clean up later
                            } else {
                                tmp_len = tmp_bak, tmp_bak = 0; // Clean up the commands in the buffer and roll back the data
                                frame_len = len, State = DATA;//记录接收数据帧长度
                                printf("%s | IPD frame_len %d\n", __func__, frame_len);
                            }
                        }
                        continue;
                    }
                    continue;
                }
                if (State == DATA) {
                    // printf("%s | frame_len %d tmp_len %d tmp_buf[tmp_pos] %02X\n", __func__, frame_len, tmp_len, recv_buffer[tmp_pos]);
                    
                    // frame_len -= tmp_len, tmp_len = 0; // get data
                    frame_len -=res;

                    if (frame_len < 0) { // already frame_len - tmp_len before (not frame_len <= 0)
                        tmp_eof = 0;
                        if (frame_len == -1 && recv_buffer[tmp_pos] == 'C') { // wait "CLOSED\r\n"
                            frame_bak = frame_len;
                            tmp_state = 7;
                            continue;
                        }
                        if (tmp_state == 6 && recv_buffer[tmp_pos] == '\r') {
                            tmp_state = 7;
                            continue;
                        }
                        if (tmp_state == 7 && recv_buffer[tmp_pos] == '\n') {
                            if (frame_len == -2) { // match +IPD EOF （\r\n）
                                tmp_state = 0, State = IDLE;
                                // After receive complete, confirm the data is enough
                                size = wifi_uart_rx_any();
                                // printk("%s | size %d out_buff_len %d\n", __func__, size, out_buff_len);
                                if (size >= out_buff_len) { // data enough
                                    // printk("%s | recv out_buff_len overflow\n", __func__);
                                    State = EXIT;
                                }
                            } else if (frame_len == -8 && frame_bak == -1)  {
                                // printk("%s | Get 'CLOSED'\n", __func__);
                                socket_eof = true;
                                frame_bak = 0, tmp_state = 0, State = EXIT;
                            } else {
                                tmp_state = 6;
                            }
                            continue;
                        }
                        
                        // 存在异常，没有得到 \r\n 的匹配，并排除 CLOSED\r\n 的指令触发的可能性，意味着传输可能越界出错了 \r\n ，则立即回到空闲状态。
                        if (frame_len <= -1 && frame_bak != -1) {
                            // printk("%s | tmp_state %d frame_len %d tmp %02X\n", __func__, tmp_state, frame_len, tmp_buf[tmp_pos]);
                            State = IDLE;
                            continue;
                        }
                    } else {
                        // for(int i = 0; i < tmp_len; i++) {
                        //     int tmp = tmp_buf[i];
                        //     printk("[%02X]", tmp);
                        // }
                        // printk("%s | frame_len %d tmp_len %d\n", __func__, frame_len, tmp_pos + 1);
                        // printk("%.*s", tmp_len, tmp_buf);
                        // if (!Buffer_Puts(&nic->buffer, tmp_buf, (tmp_pos + 1))) {
                        //     printk("%s | network->buffer overflow Buffer Max %d Size %d\n", __func__, ESP8285_BUF_SIZE, Buffer_Size(&nic->buffer));
                        //     State = EXIT;
                        // } else {
                            // reduce data len
                            // printf("[%c]", recv_buffer[tmp_pos]);
                            // frame_sum += (tmp_pos + 1);
                        // }
                        // printf("frame_sum %d frame_len %d tmp_len %d\n", frame_sum, frame_len, tmp_pos + 1);
                        if ((frame_len == 0) || (tmp_len>=1024)) {
                            // After receive complete, confirm the data is enough
                            // size = Buffer_Size(&nic->buffer);
                            // size = wifi_uart_rx_any();
                            // printk("%s | size %d out_buff_len %d\n", __func__, size, out_buff_len);
                            // if (size >= out_buff_len) { // data enough
                                // printk("%s | recv out_buff_len overflow\n", __func__);
                                tmp_eof = 1;
                                break;
                                // State = EXIT;
                            // }
                        }
                    }
                    continue;
                }

                continue;

            } else {
                printf("recv len:%d\n",res);
                State = EXIT;
            }

            continue;
        }
    
        if (get_time_ms() - interrupt > timeout) {
            if(tmp_len>0)
            printf("uart timeout break %ld %d %d\r\n", get_time_ms(), interrupt, timeout);
            break; // uart no return data to timeout break
        }
    
        msleep(10); 
        //         if (tmp_eof > 0 && tmp_eof++ > 100) { // 806400 tmp_eof 10 > one byte so 115200 > tmp_eof * 8 
        //     // printk("size %d\r\n", size);
        //     break; // sometimes communication no eof(\r\n)
        // }
    } 
    
    size = tmp_len > out_buff_len ? out_buff_len : tmp_len;
    printf("tmp_len:%d frame_len:%d size:%d uart_recv_len:%d\n",tmp_len,frame_len,size,uart_recv_len);
    // Buffer_Gets(&nic->buffer, (uint8_t *)out_buff, size);
    memcpy(out_buff,recv_buffer,size);
    if (read_len) {
        *read_len = size;
    }

    frame_sum -= size;

    // printk(" %s | tail obl %d *dl %d se %d\n", __func__, out_buff_ en, *data_len, size);

    return size;
    #endif
}
bool qATCIPSTA_CUR(void)
{
	const char* cmd = "AT+CIPSTA?";//_CUR
	wifi_rx_empty();
	esp8285_uart_senddata(cmd,strlen(cmd));
    esp8285_uart_senddata("\r\n",strlen("\r\n"));

	return recvFind("OK",1000);
}

bool get_ipconfig(ipconfig_obj* ipconfig)
{

	if(0 == qATCIPSTA_CUR())
    {
        printf("cipsta_cur error\n");
        return false;
    }
		
	char* cur = NULL;
     int index = 0;
     for(int i=0;i<ESP8285_BUF_SIZE;i++)
    {
        if(uart_buffer[i] == '\n' || uart_buffer[i] == '\r')
            index = i+1;//find \n or \r,index+1,start from next char.
    }
    printf("get ipconfig:%s\n",uart_buffer+index);
	cur = strstr((char*)uart_buffer, "ip");
	if(cur == NULL)
	{
		printf("[CanMV] %s | esp8285_ipconfig could'n get ip\r\n",__func__);
		return false;
	}
	// char ip_buf[16] = {0};
	sscanf(cur,"ip:\"%[^\"]\"",ipconfig->ip);
	// ipconfig->ip = mp_obj_new_str(ip_buf,strlen(ip_buf));
	cur = strstr((char*)uart_buffer, "gateway");
	if(cur == NULL)
	{
	// 	mp_printf(&mp_plat_print, "[CanMV] %s | esp8285_ipconfig could'n get gateway\r\n",__func__);
		return false;
	}
	// char gateway_buf[16] = {0};
	sscanf(cur,"gateway:\"%[^\"]\"",ipconfig->gateway);
	// ipconfig->gateway = mp_obj_new_str(gateway_buf,strlen(gateway_buf));
	cur = strstr((char*)uart_buffer, "netmask");
	if(cur == NULL)
	{
	// 	mp_printf(&mp_plat_print, "[CanMV] %s | esp8285_ipconfig could'n get netmask\r\n",__func__);
		return false;
	}
	// char netmask_buf[16] = {0};
	sscanf(cur,"netmask:\"%[^\"]\"",ipconfig->netmask );
	// ipconfig->netmask = mp_obj_new_str(netmask_buf,strlen(netmask_buf));
	//ssid & mac
	if(false == qATCWJAP_CUR())
	{
		return false;
	}
	// char ssid[50] = {0};
	// char MAC[17] = {0};
	cur = strstr((char*)uart_buffer, "+CWJAP_CUR:");
	sscanf(cur, "+CWJAP_CUR:\"%[^\"]\",\"%[^\"]\"", ipconfig->ssid,  ipconfig->MAC);
	// ipconfig->ssid = mp_obj_new_str(ssid,strlen(ssid));
	// ipconfig->MAC = mp_obj_new_str(MAC,strlen(MAC));
	return true;
}


bool esp8285_reset(void)
{
    unsigned long start;
    if (eATRST()) {
        msleep(2000);
        start = time_ms();
        while (time_ms() - start < 3000) {
            if (eAT()) {
                msleep(1500); /* Waiting for stable */
                printf("esp8285 reset success\r\n");
                return true;
            }
            msleep(100);
        }
    }
    printf("esp8285 reset failed\r\n");
    return false;
}

char* esp8285_getVersion(void)
{
    char* version = NULL;
    eATGMR(&version);
    return version;
}

/**
 * @brief  
 * @note   串口使用DMA方式收发
 * @retval 
 */
int esp8285_init(void)
{
    printf("[esp8285] init port\n");
    // Initialize the UART
    fpioa_set_function(UART_GPIO_TX_PIN, FUNC_UART2_TX);
    fpioa_set_function(UART_GPIO_RX_PIN, FUNC_UART2_RX);
    // fpioa_set_function(28, FUNC_UART2_TX);
    // fpioa_set_function(29, FUNC_UART2_RX);
    fpioa_set_function(WIFI_EN_PIN,FUNC_GPIO4);
    fpioa_set_function(0, FUNC_GPIO5);
    gpio_set_drive_mode(5,GPIO_DM_OUTPUT);
    gpio_set_pin(5,GPIO_PV_LOW);

    gpio_set_drive_mode(4, GPIO_DM_OUTPUT);
    // gpio_pin_value_t value = GPIO_PV_HIGH;
    gpio_set_pin(4, GPIO_PV_LOW);
    msleep(50);
    gpio_set_pin(4,GPIO_PV_HIGH);
    msleep(500);
    recv_handle.read_data = uart_buffer;
    recv_handle.read_buf_tail = 0;
    recv_handle.read_buf_head = 0;
    recv_handle.read_buf_len = ESP8285_BUF_SIZE;
    recv_handle.data_len = 0;
    uart_init(WIFI_UART_NUM);
    msleep(1);
    uart_configure(WIFI_UART_NUM, 115200, 8, UART_STOP_1,UART_PARITY_NONE);
    uart_set_receive_trigger(WIFI_UART_NUM,UART_RECEIVE_FIFO_1);
    uart_irq_register(WIFI_UART_NUM,UART_RECEIVE,wifi_port_recv_cb,NULL,1);//中断函数接收数据到缓存区

    int len = 0;

    // Send a command to the ESP8285 module
    printf("set ATE0 %d\n",eATE(1));

    printf("[esp8285] change baud\n");
    
    len = esp8285_uart_senddata("AT+UART_CUR=921600,8,1,0,0\r\n",strlen("AT+UART_CUR=921600,8,1,0,0\r\n"));
    printf("uart send len :%d\n",len);
    if(len)
    {
        msleep(150);
        char recv_data[100]={0};
        uart_receive_data(WIFI_UART_NUM,recv_data,100);
        printf("wifi set baud ok %s\n",recv_data);
        uart_init(WIFI_UART_NUM);
        msleep(1);
        uart_configure(WIFI_UART_NUM, 921600, 8, UART_STOP_1,UART_PARITY_NONE);
        uart_set_receive_trigger(WIFI_UART_NUM,UART_RECEIVE_FIFO_1);
        uart_irq_register(WIFI_UART_NUM,UART_RECEIVE,wifi_port_recv_cb,NULL,1);//中断函数接收数据到缓存区
    }
    msleep(500);
    printf("[esp8285] init function\n");
    // esp8285_reset();//重设波特率后不能复位

    bool ret = false;
    for(int i=0;i<10;i++)
    {
         if((ret = eAT()))
         {
            printf("at is OK!\r\n");
            break;
         }
         
    }
    printf("at status:%d\n",ret);
    printf("set ATE0 %d\n",eATE(1));
    // char *version = esp8285_getVersion();
    // printf("ESP8285 version:%s\r\n",uart_buffer);
    //设置工作模式
    printf("[esp8285] set work mode\n");
    ret = sATCWMODE(1);
    if(ret)
    {
        printf("esp8285 set mode success\r\n");
    }
    else
    {
        printf("esp8285 set mode failed\r\n");
        ret = sATCWMODE(1);
    }
    sATCWAUTOCONN(0);//设置上电不自动连接
    /*test code*/
    // while (1)
    // {
    //     len =uart_send_data(WIFI_UART_NUM,"AT\r\n",4);
    //     if(len)
    //     {
    //         msleep(150);
    //         len = uart_receive_data(WIFI_UART_NUM,uart_buffer,ESP8285_BUF_SIZE);
    //         if(len>0)
    //         {
    //             printf("[esp8285] recv:%s\n",uart_buffer);
    //         }
    //     }
    //     msleep(500);
    // }
    // char *version = esp8285_getVersion();
    // if(version)
    // {
    //     printf("[esp8285]: version:%s\r\n",version);
    //     free(version);
    // }
    // Wait for a response from the ESP8285 module
    // Implement your response handling logic here
    
    return 0;
}

/******************************************************************************/
/*****************spi 8266 interface*****************/
#if 0

void WiFiSpi_init(int8_t rst_pin)
{
    // Initialize the connection arrays
    for(uint8_t i = 0; i < MAX_SOCK_NUM; ++i)
    {
        WiFiSpi_state[i] = NA_STATE;
        WiFiSpi_server_port[i] = 0;
    }

    WiFiSpi_hwResetPin = rst_pin;

    WiFiSpiDrv_wifiDriverInit();
    WiFiSpi_hardReset();
}

uint8_t spi_8266_init_device(void)
{
    g_net_status = 0;
    fpioa_set_function(WIFI_EN_PIN,FUNC_GPIOHS0+WIFI_GPIOS_NUM_ENABLE);
    gpiohs_set_drive_mode(WIFI_GPIOS_NUM_ENABLE, GPIO_DM_OUTPUT);
    gpiohs_set_pin(WIFI_GPIOS_NUM_ENABLE,0);
    //hard reset
    // gpiohs_set_pin(WIFI_GPIOS_NUM_ENABLE, 0); //disable WIFI
    msleep(50);
    gpiohs_set_pin(WIFI_GPIOS_NUM_ENABLE, 1); //enable WIFI
    msleep(500);

    fpioa_set_function(WIFI_PIN_SPI_CS,FUNC_GPIOHS0+WIFI_GPIOS_NUM_CS);
    gpiohs_set_drive_mode(WIFI_GPIOS_NUM_CS,GPIO_DM_OUTPUT);
    gpiohs_set_pin(WIFI_GPIOS_NUM_CS,1);
    fpioa_set_function(WIFI_PIN_SPI_MISO,FUNC_SPI1_D1);
    fpioa_set_function(WIFI_PIN_SPI_MOSI,FUNC_SPI1_S0);
    fpioa_set_function(WIFI_PIN_SPI_SCLK,FUNC_SPI1_SCLK);
    spi_init(SPI_DEVICE_1, SPI_WORK_MODE_1, SPI_FF_STANDARD, 8, 0);
    printk("set spi clk:%d\r\n", spi_set_clk_rate(SPI_DEVICE_1, 1000000 * 8)); /*set clk rate*/

    WiFiSpi_init(-1);
}
#endif