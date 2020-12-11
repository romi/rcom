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
#ifndef _RCOM_REQUEST_H_
#define _RCOM_REQUEST_H_

#include <r.h>
#include "addr.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
        HTTP_METHOD_UNSUPPORTED,
        HTTP_METHOD_GET,
        HTTP_METHOD_POST
};

typedef struct _request_t request_t;

const char *request_uri(request_t *r);
const char *request_args(request_t *r);
const char *request_body(request_t *r);
int request_length(request_t *r);
json_object_t request_parse(request_t *r);

const char *request_get_header_value(request_t *request, const char *key);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_REQUEST_H_
