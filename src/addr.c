/*
  rcom

  Copyright (C) 2019 Sony Computer Science Laboratories
  Author(s) Peter Hanappe

  rcom is light-weight libary for inter-node communication.

  rcom is free software: you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <http://www.gnu.org/licenses/>.

 */
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <r.h>

#include "rcom/addr.h"

addr_t *new_addr0()
{
        return new_addr("0.0.0.0", 0);
}

addr_t *new_addr(const char *ip, int port)
{
        addr_t *addr = r_new(addr_t);

        addr->sin_family = AF_INET;
        if (addr_set(addr, ip, port) != 0) {
                r_delete(addr);
                return NULL;
        }
        return addr;
}

addr_t *addr_clone(addr_t *addr)
{
        addr_t *clone = r_new(addr_t);
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
        if (addr) r_delete(addr);
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
        const char *address;
        if (ip == NULL)
                address = "0.0.0.0";
        else
                address = ip;
        if (inet_aton(address, &addr->sin_addr) == 0)
            return -1;
        return 0;
}

int addr_set_port(addr_t *addr, int port)
{
        if (port < 0 || port >= 65536) {
                r_err("addr_set_port: invalid port: %d", port);
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
        snprintf(buf, len, "%s", inet_ntoa(addr->sin_addr));
        return buf;
}

char *addr_string(addr_t *addr, char* buf, int len)
{
        if (addr == NULL) {
                snprintf(buf, len, "(null)");
        } else {
                snprintf(buf, len, "%s:%d", inet_ntoa(addr->sin_addr), addr_port(addr));
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
                r_err("addr_parse: invalid string: %s", s);
                return NULL;
        }
        
        addr_t *addr = new_addr0();
        
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
        if (q == p+1 || *q != '\0') {
                delete_addr(addr);
                r_err("addr_parse: invalid port: '%s'->%d", p+1, port);
                return NULL;
        }

        err = addr_set_port(addr, port);
        if (err != 0) {
                delete_addr(addr);
                return NULL;
        }

        //r_debug("addr_parse: %s -> %s", s, addr_string(addr, ip, 64));

        return addr;
}
// An example of the above in C++
//addr_t *addr_parse_cpp(const char* s)
//{
//    std::string address(s);
//    std::string ip;
//    std::string port;
//    addr_t *addr = nullptr;
//    try {
//        std::string::iterator split_colon = std::find(address.rbegin(), address.rend(), ':').base();
//        if (split_colon != address.begin())
//        {
//            ip.assign(address.begin(), split_colon-1);
//            port.assign(split_colon, address.end());
//            long portl = std::stol(port);
//            addr = new_addr(ip.c_str(), portl);
//        }
//    }
//    catch (std::exception& e)
//    {
//        r_err("addr_parse: invalid  '%s' exception:%s", address.c_str(), e.what());
//    }
//
//    return addr;
//}
