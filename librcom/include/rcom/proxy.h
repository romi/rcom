/*
  rcutil

  Copyright (C) 2019 Sony Computer Science Laboratories
  Author(s) Peter Hanappe

  rcutil is light-weight libary for inter-node communication.

  rcutil is free software: you can redistribute it and/or modify it
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
#ifndef _RCOM_PROXY_H_
#define _RCOM_PROXY_H_

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
