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
#ifndef _RCOM_DATALINK_H_
#define _RCOM_DATALINK_H_

#include <stdio.h>
#include <r.h>
#include "rcom/data.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _datalink_t datalink_t;

typedef void (*datalink_ondata_t)(void *userdata,
                                  datalink_t *datalink,
                                  data_t *input,
                                  data_t *output);


int datalink_send(datalink_t *datalink, data_t *data);

// returns: 0 is all ok, -1 if error, 1 if timeout
data_t *datalink_read(datalink_t *datalink, int timeout);
json_object_t datalink_read_obj(datalink_t *datalink, int timeout);

// Utility function
int datalink_send_f(datalink_t *datalink, const char *format, ...);
int datalink_send_obj(datalink_t *datalink, json_object_t obj);


data_t *datalink_get_output(datalink_t* datalink);
json_object_t datalink_parse(datalink_t *datalink, data_t *data);


// Synchronous functions
data_t *datalink_rpc(datalink_t* datalink, data_t *data);
json_object_t datalink_rpc_f(datalink_t* datalink, const char *format, ...);


#ifdef __cplusplus
}
#endif

#endif // _RCOM_DATALINK_H_

