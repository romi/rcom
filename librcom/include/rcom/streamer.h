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
#ifndef _RCOM_STREAMER_H_
#define _RCOM_STREAMER_H_

#include "addr.h"
#include "circular.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _streamer_client_t streamer_client_t;
typedef void (*streamer_client_delete_context_t)(streamer_client_t *c);

/* circular_buffer_t *streamer_client_get_buffer(streamer_client_t *b, const char *name); */
circular_buffer_t *streamer_client_get_buffer(streamer_client_t *b);
void streamer_client_set_context(streamer_client_t *c, void *context);
void *streamer_client_get_context(streamer_client_t *c);
void streamer_client_set_delete_context(streamer_client_t *c, streamer_client_delete_context_t del);


typedef struct _streamer_t streamer_t;

typedef int (*streamer_onclient_t)(void *userdata,
                                   streamer_client_t *client,
                                   /* const char *exp, */
                                   streamer_t *streamer);

typedef int (*streamer_onbroadcast_t)(void *userdata, streamer_t *streamer);


/* int streamer_export(streamer_t *streamer, */
/*                     const char *name, */
/*                     const char *mimetype); */

addr_t *streamer_addr(streamer_t *s);

        
void streamer_lock_clients(streamer_t* s);
void streamer_unlock_clients(streamer_t* s);
streamer_client_t* streamer_get_client(streamer_t* s, int n);
int streamer_has_clients(streamer_t* s);
int streamer_count_clients(streamer_t *s);

int streamer_send_multipart(streamer_t *s,
                            /* const char *exp, */
                            const char *data, int length,
                            const char *mimetype,
                            double time);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_STREAMER_H_

