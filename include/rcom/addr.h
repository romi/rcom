#ifndef _RCOM_ADDR_H_
#define _RCOM_ADDR_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sockaddr_in addr_t;

addr_t *new_addr(const char *ip, int port);
addr_t *new_addr0();
addr_t *addr_clone(addr_t *addr);
void delete_addr(addr_t *addr);

int addr_set(addr_t *addr, const char *ip, int port);
int addr_set_ip(addr_t *addr, const char *ip);
int addr_set_port(addr_t *addr, int port);

int addr_port(addr_t *addr);
char *addr_ip(addr_t *addr, char* buf, int len);

char *addr_string(addr_t *addr, char* buf, int len);
addr_t *addr_parse(const char* s);

int addr_eq(addr_t *addr1, addr_t *addr2);


#ifdef __cplusplus
}
#endif

#endif // _RCOM_ADDR_H_
