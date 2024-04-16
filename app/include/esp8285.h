#ifndef _ESP8285_H
#define _ESP8285_H
#include <stdbool.h>

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
#endif // !_ESP8285_H