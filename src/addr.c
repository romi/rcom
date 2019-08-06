#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "rcom/log.h"
#include "rcom/addr.h"
#include "rcom/util.h"

#include "mem.h"

addr_t *new_addr0()
{
        return new_addr("0.0.0.0", 0);
}

addr_t *new_addr(const char *ip, int port)
{
        addr_t *addr = new_obj(addr_t);
        if (addr == NULL) return addr;
        addr->sin_family = AF_INET;
        if (addr_set(addr, ip, port) != 0) {
                delete_obj(addr);
                return NULL;
        }
        return addr;
}

addr_t *addr_clone(addr_t *addr)
{
        addr_t *clone = new_obj(addr_t);
        if (clone == NULL) return NULL;
        memcpy(clone, addr, sizeof(addr_t));
        return clone;
}

void addr_copy(addr_t *src, addr_t *dest)
{
        memcpy(dest, src, sizeof(addr_t));
}

void delete_addr(addr_t *addr)
{
        if (addr) delete_obj(addr);
}

int addr_set(addr_t *addr, const char *ip, int port)
{
        if (addr_set_ip(addr, ip) != 0)
                return -1;
        if (addr_set_port(addr, port) != 0)
                return -1;
        return 0;
}

int addr_set_ip(addr_t *addr, const char *ip)
{
        if (ip == NULL) {
                inet_aton("0.0.0.0", &addr->sin_addr);
                return 0;
        }
        // just a basic check. this doesn't validate the string.
        if (strlen(ip) < 7 || strlen(ip) > 15) {
                log_err("addr_set_ip: invalid ip: %s", ip);
                return -1;
        }
        inet_aton(ip, &addr->sin_addr);
        return 0;
}

int addr_set_port(addr_t *addr, int port)
{
        if (port < 0 || port >= 65536) {
                log_err("addr_set_port: invalid port: %d", port);
                return -1;
        }
        addr->sin_port = htons(port);
        return 0;
}

int addr_port(addr_t *addr)
{
        return ntohs(addr->sin_port);
}

char *addr_ip(addr_t *addr, char* buf, int len)
{
        snprintf(buf, len-1, "%s", inet_ntoa(addr->sin_addr));
        buf[len-1] = 0;
        return buf;
}

char *addr_string(addr_t *addr, char* buf, int len)
{
        if (addr == NULL) {
                snprintf(buf, len, "(null)");
                buf[len-1] = 0;
        } else {
                snprintf(buf, len, "%s:%d", inet_ntoa(addr->sin_addr), addr_port(addr));
                buf[len-1] = 0;
        }
        return buf;
}       

int addr_eq(addr_t *addr1, addr_t *addr2)
{
        char b1[64], b2[64];;
        if (addr_port(addr1) != addr_port(addr2))
                return 0;
        if (rstreq(addr_ip(addr1, b1, 64),
                  addr_ip(addr2, b2, 64)))
                return 1;
        return 0;            
}

addr_t *addr_parse(const char* s)
{
        char *p, *q;
        const char *r;
                
        p = strchr(s, ':');
        if (p == NULL) {
                log_err("addr_parse: invalid string: %s", s);
                return NULL;
        }
        
        addr_t *addr = new_addr0();
        if (addr == NULL)
                return NULL;
        
        char ip[64];
        for (q = ip, r = s; r < p; q++, r++)
                *q = *r;
        *q = '\0';
        
        int err = addr_set_ip(addr, ip);
        if (err != 0) {
                delete_addr(addr);
                return NULL;
        }

        int port = strtol(p+1, &q, 10);
        if (port < 0 || port > 65535 || q == p+1 || *q != '\0') {
                log_err("addr_parse: invalid port: '%s'->%d", p+1, port);
                return NULL;
        }

        err = addr_set_port(addr, port);
        if (err != 0) {
                delete_addr(addr);
                return NULL;
        }

        //log_debug("addr_parse: %s -> %s", s, addr_string(addr, ip, 64));

        return addr;
}       
