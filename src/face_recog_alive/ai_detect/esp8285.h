#ifndef _ESP8285_H
#define _ESP8285_H
#include <stdbool.h>
#include <stdint.h>
typedef struct _ipconfig_obj
{
	char ip[16];
	char gateway[16];
	char netmask[16];
	char ssid[50];
	char MAC[17];
}ipconfig_obj;

bool esp8285_joinAP( const char* ssid, const char* pwd);
bool esp8285_reset(void);
char* esp8285_getVersion(void);
int esp8285_init(void);
bool esp8285_setHostname(const char *hostname);
bool get_ipconfig(ipconfig_obj* ipconfig);
bool eAT(void);
void get_ip();
bool esp8285_JoinState(void);
bool eATCIPSTATUS(char** list);
bool esp8285_sATCIPSTARTSingle(const char* addr, uint32_t port);
bool esp8285_eATCIPCLOSESingle();
void wifi_rx_empty(void);
int esp8285_send(const char *buffer,uint32_t len,uint32_t timeout);
int esp8285_recv(uint8_t *out_buff,uint32_t out_buff_len,uint32_t *read_len,uint32_t timeout, bool first_time_recv);
#endif // !_ESP8285_H