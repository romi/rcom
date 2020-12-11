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
#ifndef _RCOM_MESSAGELINK_PRIV_H_
#define _RCOM_MESSAGELINK_PRIV_H_

#include "messagelink.h"
#include "net.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*messagelink_onpong_t)(messagelink_t *link,
                                     void *userdata,
                                     const char *data, int len);

messagelink_t *new_messagelink(const char *name,
                               const char *topic,
                               messagelink_onmessage_t onmessage,
                               messagelink_onclose_t onclose,
                               void *userdata);
void delete_messagelink(messagelink_t *link);

void messagelink_set_onpong(messagelink_t *link, messagelink_onpong_t onpong);

messagelink_t *server_messagelink_connect(messagehub_t *hub, tcp_socket_t socket);

int client_messagelink_connect(messagelink_t *link, addr_t *addr);
int client_messagelink_disconnect(messagelink_t *link);

void server_messagelink_read_in_background(messagelink_t *link);
void messagelink_read_in_background(messagelink_t *link);

int messagelink_send_ping(messagelink_t *link, const char *data, int len);

addr_t *messagelink_addr(messagelink_t *link);
const char *messagelink_name(messagelink_t *link);
const char *messagelink_topic(messagelink_t *link);

void messagelink_stop_thread(messagelink_t *link);


#ifdef __cplusplus
}
#endif

#endif // _RCOM_MESSAGELINK_PRIV_H_
