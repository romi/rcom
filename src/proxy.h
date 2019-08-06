#ifndef _RCOM_PROXY_H_
#define _RCOM_PROXY_H_

#include "rcom/messagelink.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _proxy_t proxy_t;

void set_registry_name(const char* name);
const char *get_registry_name();

void set_registry_ip(const char* addr);
void set_registry_port(int port);

void set_registry_standalone(int standalone);
int get_registry_standalone();

void proxy_init();
void proxy_cleanup();
proxy_t *proxy_get();

int proxy_count_nodes(proxy_t *proxy);
int proxy_get_node(proxy_t *proxy, int i,
                   membuf_t *name, membuf_t *topic,
                   int *type, addr_t *addr);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_PROXY_H_
