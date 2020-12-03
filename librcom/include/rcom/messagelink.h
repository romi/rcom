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
#ifndef _RCOM_MESSAGELINK_H_
#define _RCOM_MESSAGELINK_H_

#include <r.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _messagelink_t messagelink_t;
typedef struct _messagehub_t messagehub_t;

typedef void (*messagelink_onmessage_t)(void *userdata,
                                        messagelink_t *link,
                                        json_object_t message);

typedef void (*messagelink_onclose_t)(void *userdata,
                                      messagelink_t *link);

typedef int (*messagelink_oncommand_t)(void *userdata,
                                       messagelink_t *link,
                                       json_object_t command,
                                       membuf_t *message);

const char *messagelink_uri(messagelink_t *link);

void messagelink_set_userdata(messagelink_t *link, void *userdata);
void *messagelink_get_userdata(messagelink_t *link);
void messagelink_set_onmessage(messagelink_t *link, messagelink_onmessage_t onmessage);
void messagelink_set_onclose(messagelink_t *link, messagelink_onclose_t onclose);

// Send messages.
// The messagelink_send_xxx() functions are multi-thread safe. 
int messagelink_send_num(messagelink_t *link, double value);
int messagelink_send_str(messagelink_t *link, const char* value);
int messagelink_send_f(messagelink_t *link, const char *format, ...);
int messagelink_send_obj(messagelink_t *link, json_object_t value);
int messagelink_send_text(messagelink_t *link, const char *data, int len);
int messagelink_send_v(messagelink_t *link, const char* format, va_list ap);

// Only call messagelink_read() if the link has been initialized
// without an onmessage handler!
json_object_t messagelink_read(messagelink_t *link);


json_object_t messagelink_send_command_f(messagelink_t *link, const char *format, ...);
json_object_t messagelink_send_command(messagelink_t *link, json_object_t command);

        
#ifdef __cplusplus
}
#endif

#endif // _RCOM_MESSAGELINK_H_
