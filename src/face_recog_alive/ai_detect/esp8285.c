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

#define UART_NUM    UART_DEVICE_2
#define UART_GPIO_TX_PIN   (7)
#define UART_GPIO_RX_PIN   (6)
#define WIFI_EN_PIN (8)
#define UART_GPIO_TX_FUNC   (FUNC_UART1_TX+UART_NUM*2)
#define UART_GPIO_RX_FUNC   (FUNC_UART1_RX+UART_NUM*2)
#define ESP8285_BUF_SIZE 10240
#define time_ms() (unsigned long)(read_cycle()/(sysctl_clock_get_freq(SYSCTL_CLOCK_CPU)/1000))
static uint8_t uart_buffer[ESP8285_BUF_SIZE];

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

static void rx_empty(void) 
{
    char data = 0;
    
    while(uart_receive_data(UART_NUM,&data,1) > 0) {
        ; 
    }
}
static char* recvString_1(const char* target1,uint32_t timeout)
{
	uint32_t iter = 0;
	memset(uart_buffer,0,ESP8285_BUF_SIZE);
    unsigned long start = time_ms();
    while (time_ms() - start < timeout) {
        while (uart_receive_data(UART_NUM,(char *)&uart_buffer[iter++],1) > 0) {  // read data from uart1 dma buffer.
            if(iter >= ESP8285_BUF_SIZE)
            {
                iter = 0; // buffer full,reset iter.
                break;
            }
        }
        if (data_find(uart_buffer,iter,target1) != -1) {
            return (char*)uart_buffer;
        }
        // msleep(10); 
    }
    return NULL;
}

static char* recvString_2(char* target1, char* target2, uint32_t timeout, int8_t* find_index)
{
	uint32_t iter = 0;
    *find_index = -1;
	memset(uart_buffer,0,ESP8285_BUF_SIZE);
    unsigned int start = time_ms();
    while (time_ms() - start < timeout) {
        while (uart_receive_data(UART_NUM,(char *)&uart_buffer[iter++],1) > 0) {  // read data from uart1 dma buffer.
            if(iter >= ESP8285_BUF_SIZE)
            {
                iter = 0; // buffer full,reset iter.
                break;
            }
        }
        if (data_find(uart_buffer,iter,target1) != -1) {
            *find_index = 0;
            return (char*)uart_buffer;
        } else if (data_find(uart_buffer,iter,target2) != -1) {
            *find_index = 1;
            return (char*)uart_buffer;
        }
        // msleep(15); 
    }
    return NULL;
}

static char* recvString_3(char* target1, char* target2,char* target3,uint32_t timeout, int8_t* find_index)
{
	uint32_t iter = 0;
    *find_index = -1;
	memset(uart_buffer,0,ESP8285_BUF_SIZE);
    unsigned long start = time_ms();
    while (time_ms() - start < timeout) {
        while (uart_receive_data(UART_NUM,(char *)&uart_buffer[iter++],1) > 0) {  // read data from uart1 dma buffer.
            if(iter >= ESP8285_BUF_SIZE)
            {
                iter = 0; // buffer full,reset iter.
                break;
            }
        }
        if (data_find(uart_buffer,iter,target1) != -1) {
            *find_index = 0;
            return (char*)uart_buffer;
        } else if (data_find(uart_buffer,iter,target2) != -1) {
            *find_index = 1;
            return (char*)uart_buffer;
        } else if (data_find(uart_buffer,iter,target3) != -1) {
            *find_index = 2;
            return (char*)uart_buffer;
        }
        // msleep(10); 
    }
    return NULL;
}


static bool recvFind(const char* target, uint32_t timeout)
{
    recvString_1(target, timeout);
    if (data_find(uart_buffer,ESP8285_BUF_SIZE,target) != -1) {
        return true;
    }
    return false;
}

static bool recvFindAndFilter(const char* target, const char* begin, const char* end, char** data, uint32_t timeout)
{
    recvString_1(target, timeout);
    if (data_find(uart_buffer,ESP8285_BUF_SIZE,target) != -1) {
        int32_t index1 = data_find(uart_buffer,ESP8285_BUF_SIZE,begin);
        int32_t index2 = data_find(uart_buffer,ESP8285_BUF_SIZE,end);
        if (index1 != -1 && index2 != -1) {
            index1 += strlen(begin);
			*data = malloc(sizeof(char)*(index2 - index1));
			memcpy(*data,uart_buffer+index1, index2 - index1);
            return true;
        }
    }
    return false;
}

bool eAT(void)
{	
	const char* cmd = "AT\r\n";
    rx_empty();// clear rx
	uart_send_data(UART_NUM,cmd,strlen(cmd));// send cmd to esp8285
    if(recvFind("OK",1000))
        return true;
    rx_empty();
    uart_send_data(UART_NUM,cmd,strlen(cmd));// send cmd to esp8285
    if(recvFind("OK",1000))
        return true;
    rx_empty();
    uart_send_data(UART_NUM,cmd,strlen(cmd));// send cmd to esp8285
    return recvFind("OK",1000);
}

bool eATE(bool enable)
{	
    rx_empty();// clear rx
    if(enable)
    {
    	const char* cmd = "ATE0\r\n";
		uart_send_data(UART_NUM,cmd,strlen(cmd));// send cmd to esp8285
    	return recvFind("OK",1000);
    }
	else
	{
    	const char* cmd = "ATE1\r\n";
		uart_send_data(UART_NUM,cmd,strlen(cmd));// send cmd to esp8285
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
    int send_len = uart_send_data(UART_NUM,cmd,strlen(cmd));// send cmd to esp8285
    printf("[esp8285] send %s",cmd);
    return recvFind("OK",1000);
}

static bool eATGMR(char** version)
{
	const char* cmd = "AT+GMR\r\n";
    rx_empty();// clear rx
    uart_send_data(UART_NUM,cmd,strlen(cmd));// send cmd to esp8285
    return recvFind("OK",1000);
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
	uart_send_data(UART_NUM,(uint8_t*)cmd,strlen(cmd));// send cmd to esp8285
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
	uart_send_data(UART_NUM,(uint8_t*)cmd,strlen(cmd));// send cmd to esp8285
    if(recvString_2("OK", "no change",1000, &find) != NULL)
        return true;
    return false;
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
    uart_send_data(UART_NUM,cmd,strlen(cmd));
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
    snprintf(cmd,20,"AT+CWHOSTNAME=%s\r\n",hostname);// send cmd to esp8285
	uart_send_data(UART_NUM,(uint8_t*)cmd,strlen(cmd));// send cmd to esp8285
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
	uart_send_data(UART_NUM,cmd,strlen(cmd));// send cmd to esp8285
    return recvFind("OK",1000);
}

bool esp8285_joinAP( const char* ssid, const char* pwd)
{
	// const char* cmd = "AT+CWJAP=\"";
    char cmd[50]={0};
    int8_t find;
    rx_empty();	
    snprintf(cmd,50,"AT+CWJAP=\"%s\",\"%s\"\r\n",ssid,pwd);// send cmd to esp8285
	uart_send_data(UART_NUM,(char *)cmd,strlen(cmd));// send cmd to esp8285
    if(recvString_2("OK", "FAIL", 6000, &find) != NULL && find==0 )
        return true;
    return false;
}

bool esp8285_sATCIPSTARTSingle(const char* type, char* addr, uint32_t port)
{
	// const char* cmd = "AT+CIPSTART=\"";
    char cmd[100]={0};
	// mp_obj_t IP = netutils_format_ipv4_addr((uint8_t*)addr,NETUTILS_BIG);
	// const char* host = mp_obj_str_get_str(IP);
	// char port_str[10] = {0};
    int8_t find_index;
	// itoa(port, port_str, 10);
	rx_empty();
    snprintf(cmd,100,"AT+CIPSTART=\"%s\",\"%s\",%d\r\n",type,addr,port);
	uart_send_data(UART_NUM,(char *)cmd,strlen(cmd));// send cmd to esp8285
    if(recvString_3("OK", "ERROR", "ALREADY CONNECT", 10000, &find_index)!=NULL && (find_index==0 || find_index==2) )
        return true;
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
	uart_send_data(UART_NUM,(char *)cmd,strlen(cmd));// send cmd to esp8285
    if(recvString_3("OK", "ERROR", "ALREADY CONNECT", 10000, &find_index) != NULL  && (find_index==0 || find_index==2) )
        return true;
    return false;
}
bool esp8285_sATCIPSENDSingle(const char* buffer, uint32_t len, uint32_t timeout)
{
	// const char* cmd = "AT+CIPSEND=";
	// char len_str[10] = {0};
    char cmd[50]={0};
	rx_empty();
    snprintf(cmd,50,"AT+CIPSEND=%d\r\n",len);
    uart_send_data(UART_NUM,(char *)cmd,strlen(cmd));// send cmd to esp8285
    if (recvFind(">", 5000)) {
        rx_empty();
		uart_send_data(UART_NUM,buffer,len);// send cmd to esp8285
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
    uart_send_data(UART_NUM,(char *)cmd,strlen(cmd));// send cmd to esp8285

    if (recvFind(">", 5000)) {
        rx_empty();
		uart_send_data(UART_NUM,buffer,len);// send cmd to esp8285
        uart_send_data(UART_NUM,"\r\n",strlen("\r\n"));// send cmd to esp8285
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
	uart_send_data(UART_NUM,(char *)cmd,strlen(cmd));// send cmd to esp8285
    if(recvString_2("OK", "link is not", 5000, &find) != NULL)
        return true;
    return false;
}
bool esp8285_eATCIPCLOSESingle()
{

    int8_t find;
	const char* cmd = "AT+CIPCLOSE\r\n";
    rx_empty();
	uart_send_data(UART_NUM,cmd,strlen(cmd));// send cmd to esp8285
    if (recvString_2("OK", "ERROR", 5000, &find) != NULL)
    {
        if( find == 0)
            return true;
    }
    return false;
}
bool qATCIPSTA_CUR(void)
{
	const char* cmd = "AT+CIPSTA?";//_CUR
	rx_empty();
	uart_send_data(UART_NUM,cmd,strlen(cmd));
    uart_send_data(UART_NUM,"\r\n",strlen("\r\n"));

	return recvFind("OK",1000);
}
bool qATCWJAP_CUR()
{
	const char* cmd = "AT+CWJAP_CUR?";
	rx_empty();
	uart_send_data(UART_NUM,cmd,strlen(cmd));
    uart_send_data(UART_NUM,"\r\n",strlen("\r\n"));
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

// char* esp8285_getVersion(void)
// {
//     char* version;
//     eATGMR(&version);
//     return version;
// }

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
    uart_init(UART_NUM);
    uart_configure(UART_NUM, 115200, 8, UART_STOP_1,UART_PARITY_NONE);
    // Send a command to the ESP8285 module
    // eATE(1);
    // printf("[esp8285] change baud\n");
    int len = 0;
    // len = uart_send_data(UART_NUM,"AT+UART_CUR=921600,8,1,0,0\r\n",strlen("AT+UART_CUR=921600,8,1,0,0\r\n"));
    // printf("uart send len :%d\n",len);
    // if(len)
    // {
    //     msleep(150);
    //     uart_receive_data(UART_NUM,uart_buffer,ESP8285_BUF_SIZE);
    //     printf("wifi set baud ok %s\n",uart_buffer);
    //     uart_init(UART_NUM);
    //     uart_configure(UART_NUM, 921600, 8, UART_STOP_1,UART_PARITY_NONE);
    // }
    // msleep(500);
    printf("[esp8285] init function\n");
    esp8285_reset();//重设波特率后不能复位
    bool ret = eAT();
    printf("at status:%d\n",ret);
    eATE(1);
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

    // for(int i=0;i<10;i++)
    // {
        // len = uart_send_data(UART_NUM,(uint8_t*)"AT+CMD?\r\n",strlen("AT+CMD?\r\n"));
        // printf("uart send len :%d\n",len);
        // msleep(500);
        // len = uart_receive_data(UART_NUM,uart_buffer,ESP8285_BUF_SIZE);
        // printf("uart recv len :%d\n",len);
        // if(len)
        // {
        //     printf("uart recv data:%s\n",uart_buffer);
        // }
        len = uart_send_data(UART_NUM,(uint8_t*)"AT+GMR\r\n",strlen("AT+GMR\r\n"));
        printf("uart send len :%d\n",len);
        // msleep(50);
        ret = recvFind("OK",1500);
        printf("[esp8285] AT+GMR recv :%d\n",ret);
        // len = uart_receive_data(UART_NUM,uart_buffer,ESP8285_BUF_SIZE);
        // printf("uart recv len :%d\n",len);
        // if(len)
        {
            int index = 0;
        //     for(int i=0;i<ESP8285_BUF_SIZE;i++)
        //     {
        //         if(uart_buffer[i] == '\n' || uart_buffer[i] == '\r')
        //             index = i+1;//find \n or \r,index+1,start from next char.
        //     }
            printf("uart recv data:%s\n",uart_buffer+index);
        }
    //     len = uart_send_data(UART_NUM,(uint8_t*)"AT+CWJAP=\"beitababa\",\"12345678\"\r\n",strlen("AT+CWJAP=\"beitababa\",\"12345678\"\r\n"));
    //     printf("uart send len :%d\n",len);
    //     msleep(1500);
    //     len = uart_receive_data(UART_NUM,uart_buffer,ESP8285_BUF_SIZE);
    //     printf("uart recv len :%d\n",len);
    //     if(len)
    //     {
    //         printf("uart recv data:%s\n",uart_buffer);
    //     }
    //     sleep(1);
    // }

    // Wait for a response from the ESP8285 module
    // Implement your response handling logic here
    
    return 0;
}