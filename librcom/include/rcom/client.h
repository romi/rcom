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
#ifndef _RCOM_CLIENT_H_
#define _RCOM_CLIENT_H_

#include <r.h>
#include "response.h"

#ifdef __cplusplus
extern "C" {
#endif

json_object_t client_get(const char *topic, const char *resource);
int client_get_data(const char *topic, const char *resource, response_t **response_handle);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_CLIENT_H_
