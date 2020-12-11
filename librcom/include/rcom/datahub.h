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
#ifndef _RCOM_DATAHUB_H_
#define _RCOM_DATAHUB_H_

#include <stdarg.h>
#include "addr.h"
#include "data.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _datahub_t datahub_t;

typedef int (*datahub_ondata_t)(void *userdata,
                                datahub_t* hub,
                                addr_t *link,
                                data_t *data);

typedef int (*datahub_onbroadcast_t)(void *userdata,
                                     datahub_t* hub);

// Send to a single remote link
int datahub_send_num(datahub_t* hub, addr_t *link, double value);
int datahub_send_str(datahub_t* hub, addr_t *link, const char* value);
int datahub_send_f(datahub_t* hub, addr_t *link, const char *format, ...);
int datahub_send_obj(datahub_t* hub, addr_t *link, json_object_t value);
int datahub_send_bin(datahub_t* hub, addr_t *link, const char *data, int len);
int datahub_send_v(datahub_t* hub, addr_t *link, const char* format, va_list ap);
int datahub_send(datahub_t* hub, addr_t *link, data_t* data);

// Broadcast to all links.
int datahub_broadcast_num(datahub_t* hub, addr_t *exclude, double value);
int datahub_broadcast_str(datahub_t* hub, addr_t *exclude, const char* value);
int datahub_broadcast_f(datahub_t* hub, addr_t *exclude, const char *format, ...);
int datahub_broadcast_obj(datahub_t* hub, addr_t *exclude, json_object_t value);
int datahub_broadcast_bin(datahub_t* hub, addr_t *exclude, const char *data, int len);
int datahub_broadcast_v(datahub_t* hub, addr_t *exclude, const char* format, va_list ap);
int datahub_broadcast(datahub_t* hub, addr_t *exclude, data_t* data);


#ifdef __cplusplus
}
#endif

#endif // _RCOM_DATASOURCE_H_

