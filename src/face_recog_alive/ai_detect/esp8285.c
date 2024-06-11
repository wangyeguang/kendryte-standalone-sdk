/*
 * @Author: Wangyg wangyeguang521@163.com
 * @Date: 2024-04-23 11:20:21
 * @LastEditors: Wangyg wangyeguang521@163.com
 * @LastEditTime: 2024-06-03 19:21:51
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
#define ESP8285_BUF_SIZE 10240
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
static int wifi_port_recv_cb(void *ctx)
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
            read_ret = uart_channel_getchar(WIFI_UART_NUM, &tmp);
            if(read_ret == 0)
                break;
            recv_handle.read_data[recv_handle.read_buf_head] = tmp;
            recv_handle.read_buf_head = next_head;
            recv_handle.data_len++;
            next_head = (recv_handle.read_buf_head+1) % recv_handle.read_buf_len;
        }
        // 如果缓冲区已满，继续读取并丢弃数据，直到没有数据可读
        if(next_head == recv_handle.read_buf_tail)
        {
            do{
                read_ret = uart_channel_getchar(WIFI_UART_NUM,&tmp);
            }while(read_ret != 0);
            break; // 当没有更多数据可读时退出循环
        }

    }while(read_ret!=0);
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
static uint32_t kmp_find(char* src,uint32_t src_len, const char* tagert)
{
	uint32_t index = 0;
	uint32_t tag_len = strlen(tagert);
	int* next = (int*)malloc(sizeof(uint32_t) * tag_len);
	kmp_get_next(tagert,next);
	index = kmp_match(src,src_len,tagert, next);
	free(next);
	return index;
}
static uint32_t data_find(uint8_t* src,uint32_t src_len, const char* tagert)
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
int wifi_uart_recvdata(uint8_t *buf,int len)
{
    int data_num = 0;
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
int esp8285_uart_senddata(char *buf,int len)
{
    return uart_send_data(WIFI_UART_NUM,buf,len);
}
static void rx_empty(void) 
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
                rx_empty();
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
        while ((len = wifi_uart_recvdata((char *)(recv_buffer+iter),(max_len-iter))) > 0) {  
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
        while ((len = wifi_uart_recvdata((char *)(recv_buffer+iter),(max_len - iter))) > 0) {
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
    if (data_find(recv_data,recv_len,target) != -1) {
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
    if (data_find(recv_data,recv_len,target) != -1) {
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
	rx_empty();
	esp8285_uart_senddata(cmd,strlen(cmd));
    esp8285_uart_senddata("\r\n",strlen("\r\n"));
	return recvFind("OK",1000);
}
bool qATCWJAP()
{
	const char* cmd = "AT+CWJAP?";
	rx_empty();
	esp8285_uart_senddata(cmd,strlen(cmd));
    esp8285_uart_senddata("\r\n",strlen("\r\n"));
	return recvFind("OK",1000);
}

bool eAT(void)
{	
	const char* cmd = "AT\r\n";
    rx_empty();// clear rx
	esp8285_uart_senddata(cmd,strlen(cmd));// send cmd to esp8285
    if(recvFind("OK",1000))
        return true;
    rx_empty();
    esp8285_uart_senddata(cmd,strlen(cmd));// send cmd to esp8285
    if(recvFind("OK",1000))
        return true;
    rx_empty();
    esp8285_uart_senddata(cmd,strlen(cmd));// send cmd to esp8285
    return recvFind("OK",1000);
}

bool eATE(bool enable)
{	
    rx_empty();// clear rx
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
    rx_empty();// clear rx
    int send_len = esp8285_uart_senddata(cmd,strlen(cmd));// send cmd to esp8285
    printf("[esp8285] send %s",cmd);
    return recvFind("OK",1000);
}

static bool eATGMR(char** version)
{
	const char* cmd = "AT+GMR\r\n";
    rx_empty();// clear rx
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
    rx_empty();
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
    rx_empty();
    snprintf(cmd,20,"AT+CWMODE=%d\r\n",mode);// send cmd to esp8285
	esp8285_uart_senddata((uint8_t*)cmd,strlen(cmd));// send cmd to esp8285
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
    rx_empty();
    snprintf(cmd,20,"AT+CWAUTOCONN=%d\r\n",enable);
    esp8285_uart_senddata((uint8_t*)cmd,strlen(cmd));// send cmd to esp8285
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
    rx_empty();
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
    rx_empty();
    snprintf(cmd,50,"AT+CWHOSTNAME=\"%s\"\r\n",hostname);// send cmd to esp8285
	esp8285_uart_senddata((uint8_t*)cmd,strlen(cmd));// send cmd to esp8285
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
    rx_empty();
	esp8285_uart_senddata(cmd,strlen(cmd));// send cmd to esp8285
    return recvFind("OK",1000);
}

bool esp8285_joinAP( const char* ssid, const char* pwd)
{
    char cmd[50]={0};
    int8_t find;
    rx_empty();	
    snprintf(cmd,50,"AT+CWJAP=\"%s\",\"%s\"\r\n",ssid,pwd);// send cmd to esp8285
    printf("[esp8285]: joinAP cmd:%s\n",cmd);
	esp8285_uart_senddata((char *)cmd,strlen(cmd));// send cmd to esp8285
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

bool esp8285_sATCIPSTARTSingle(char* addr, uint32_t port)
{
	// const char* cmd = "AT+CIPSTART=\"";
    char cmd[100]={0};
	// mp_obj_t IP = netutils_format_ipv4_addr((uint8_t*)addr,NETUTILS_BIG);
	// const char* host = mp_obj_str_get_str(IP);
	// char port_str[10] = {0};
    int8_t find_index;
	// itoa(port, port_str, 10);
	rx_empty();
    snprintf(cmd,100,"AT+CIPSTART=\"TCP\",\"%s\",%d\r\n",addr,port);
    printf("[esp8285] send connect tcpserver cmd:%s\n",cmd);
	esp8285_uart_senddata((char *)cmd,strlen(cmd));// send cmd to esp8285
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
	rx_empty();
    snprintf(cmd,100,"AT+CIPSTART=%d,\"%s\",\"%s\",%d\r\n",mux_id,type,addr,port);
	esp8285_uart_senddata((char *)cmd,strlen(cmd));// send cmd to esp8285
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
	rx_empty();
    snprintf(cmd,50,"AT+CIPSEND=%d\r\n",len);
    esp8285_uart_senddata((char *)cmd,strlen(cmd));// send cmd to esp8285
    if (recvFind(">", 5000)) {
        rx_empty();
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
	rx_empty();
    snprintf(cmd,50,"AT+CIPSEND=%d,%d",mux_id,len);
    esp8285_uart_senddata((char *)cmd,strlen(cmd));// send cmd to esp8285

    if (recvFind(">", 5000)) {
        rx_empty();
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
	rx_empty();
    snprintf(cmd,50,"AT+CIPCLOSE=%d\r\n",mux_id);// send cmd to esp8285
	esp8285_uart_senddata((char *)cmd,strlen(cmd));// send cmd to esp8285
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
	char* cmd = "AT+CIPCLOSE\r\n";
    rx_empty();
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
int esp8285_recv(char *buffer,uint32_t buffer_size,uint32_t *read_len,uint32_t timeout)
{
    int err = 0, size = 0;
    static bool socket_eof = false;
    enum fsm_state { IDLE, IPD, DATA, EXIT } State = IDLE;
    return 0;
}
bool qATCIPSTA_CUR(void)
{
	const char* cmd = "AT+CIPSTA?";//_CUR
	rx_empty();
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