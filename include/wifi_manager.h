#ifndef _WIFI_MANAGER_H
#define _WIFI_MANAGER_H

#include "list_core.h"

typedef struct wifi_info {
	char *name;
	int (*init)(char * name);
	int (*exit)(void);
	int (*send_safe)(char * send_str,char * mach_str);
	int (*send)(char * send_str,int len);
	int (*receive)(char *rcv_buf, int len);
	struct list_head node;
}wifi_opr;


int wifi_init();
int wifi_device_init(char * name, wifi_opr **wifi);
int register_wifi_opr(wifi_opr* opr);


int rm04_register(void);
int rtl8188eus_register(void);



#endif /* _WIFI_MANAGER_H */



