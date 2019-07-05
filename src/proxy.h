#ifndef _RCOM_PROXY_H_
#define _RCOM_PROXY_H_

#include "rcom/messagelink.h"

#ifdef __cplusplus
extern "C" {
#endif

void set_registry_name(const char* name);
void set_registry_ip(const char* addr);
void set_registry_port(int port);
void set_registry_standalone(int standalone);
const char *get_registry_name();
const char *get_registry_ip();
int get_registry_port();

void proxy_init();
void proxy_cleanup();

#ifdef __cplusplus
}
#endif

#endif // _RCOM_PROXY_H_
