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
#ifndef _RCOM_EXPORT_H_
#define _RCOM_EXPORT_H_

#include "rcom/service.h"
#include "rcom/response.h"

#ifdef __cplusplus
extern "C" {
#endif
        
typedef struct _export_t export_t;
typedef struct _request_t request_t;

export_t* new_export(const char* name,
                     const char* mimetype_in,
                     const char* mimetype_out);
void delete_export(export_t* e);

const char* export_name(export_t* e);
const char* export_mimetype_in(export_t* e);
const char* export_mimetype_out(export_t* e);
int export_json_in(export_t* e);
int export_json_out(export_t* e);

int export_matches(export_t* e, const char *name);

export_t *export_clone(export_t* e);
int export_set_mimetypes(export_t* e,
                         const char* mimetype_in,
                         const char* mimetype_out);
void export_set_onrequest(export_t* e, void *userdata, service_onrequest_t onrequest);
void export_callback(export_t* e, request_t *request, response_t *response);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_EXPORT_H_
