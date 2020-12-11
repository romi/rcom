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
#ifndef _RCOM_STREAMERLINK_H_
#define _RCOM_STREAMERLINK_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _streamerlink_t streamerlink_t;
typedef struct _response_t response_t;

typedef int (*streamerlink_ondata_t)(void *userdata, response_t *response,
                                     const char *buf, int len);
typedef int (*streamerlink_onresponse_t)(void *userdata, response_t *response);

addr_t *streamerlink_addr(streamerlink_t *link);
int streamerlink_connect(streamerlink_t *link);
int streamerlink_disconnect(streamerlink_t *link);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_STREAMERLINK_H_
