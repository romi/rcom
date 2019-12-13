/** 
 * \file 
 * The Rcom addresses are used specify where to reach hubs, links, services, streamers...
 */

#ifndef _RCOM_ADDR_H_
#define _RCOM_ADDR_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sockaddr_in addr_t;

/**
 *  \brief Create a new Rcom address from an IP address and a port
 *  number.
 *
 */        
addr_t *new_addr(const char *ip, int port);

/**
 *  \brief Create an empty Rcom address.
 *
 */        
addr_t *new_addr0();

/**
 *  \brief Clone an address.
 *
 */        
addr_t *addr_clone(addr_t *addr);

/**
 *  \brief Copy an address.
 *
 */        
void addr_copy(addr_t *src, addr_t *dest);

/**
 *  \brief Delete the address.
 *
 */        
void delete_addr(addr_t *addr);

/**
 *  \brief Set the values of the address.
 *
 */        
int addr_set(addr_t *addr, const char *ip, int port);

/**
 *  \brief Set the IP number of the address.
 *
 */        
int addr_set_ip(addr_t *addr, const char *ip);

/**
 *  \brief Set the port number of the address.
 *
 */        
int addr_set_port(addr_t *addr, int port);

/**
 *  \brief Get the IP number of the address.
 *
 */        
char *addr_ip(addr_t *addr, char* buf, int len);

/**
 *  \brief Get the port number of the address.
 *
 */        
int addr_port(addr_t *addr);

/**
 *  \brief Convert the address to a string representation.
 *
 */        
char *addr_string(addr_t *addr, char* buf, int len);

/**
 *  \brief Convert a string representation into an address.
 *
 */        
addr_t *addr_parse(const char* s);

/**
 *  \brief Test whether two addresses have the same value.
 *
 */        
int addr_eq(addr_t *addr1, addr_t *addr2);


#ifdef __cplusplus
}
#endif

#endif // _RCOM_ADDR_H_
