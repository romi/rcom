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
#ifndef _RCOM_NET_H_
#define _RCOM_NET_H_

#include "rcom/data.h"
#include "rcom/addr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int udp_socket_t;

#define INVALID_UDP_SOCKET -1

// Pass port=0 for a random port.
udp_socket_t open_udp_socket();
void close_udp_socket(udp_socket_t socket);

addr_t *udp_socket_addr(udp_socket_t s);

int udp_socket_send(udp_socket_t socket, addr_t *addr, data_t *data);

// Returns:
// -1: error
// 0: timeout
// 1: ok, data available
int udp_socket_wait_data(udp_socket_t socket, int timeout);

// Returns:
// -1: error
// 0: ok
int udp_socket_read(udp_socket_t socket, data_t *data, addr_t *addr);



typedef int tcp_socket_t;

#define INVALID_TCP_SOCKET -1
#define TCP_SOCKET_TIMEOUT -2

tcp_socket_t open_tcp_socket(addr_t *addr);
void close_tcp_socket(tcp_socket_t socket);

int tcp_socket_send(tcp_socket_t socket, const char *data, int len);

// Returns number of bytes read. This number may be smaller than the
// number of bytes requested (len). In case an error occurs then -1 is
// returned. In case the socket was shut down, zero is returned.
// Returns:
// -2: timeout
// -1: error
// 0: shutdown
// >0: amount of data received
int tcp_socket_recv(tcp_socket_t socket, char *data, int len);

// Returns the number of bytes read. This number should be to the
// number of bytes requested unless the end of the stream was
// reached. The function blocks until all data has been read. In case
// the socket was shut down, zero or a number smaller than len i
// returned. In case an error occurs then -1 is returned.
int tcp_socket_read(tcp_socket_t socket, char *data, int len);

// Returns:
// -1: error
// 0: timeout
// 1: ok, data available
int tcp_socket_wait_data(tcp_socket_t socket, int timeout);

tcp_socket_t open_server_socket(addr_t* addr);

tcp_socket_t server_socket_accept(tcp_socket_t s);

addr_t *tcp_socket_addr(tcp_socket_t s);



#ifdef __cplusplus
}
#endif

#endif // _RCOM_NET_H_
