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
#ifndef _RCOM_RESPONSE_H_
#define _RCOM_RESPONSE_H_

#include <r.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HTTP_Status_Continue 100
#define HTTP_Status_Switching_Protocols 101
#define HTTP_Status_OK 200
#define HTTP_Status_Created 201
#define HTTP_Status_Accepted 202
#define HTTP_Status_Non_Authoritative_Information 203
#define HTTP_Status_No_Content 204
#define HTTP_Status_Reset_Content 205
#define HTTP_Status_Partial_Content 206
#define HTTP_Status_Multiple_Choices 300
#define HTTP_Status_Moved_Permanently 301
#define HTTP_Status_Found 302
#define HTTP_Status_See_Other 303
#define HTTP_Status_Not_Modified 304
#define HTTP_Status_Temporary_Redirect 307
#define HTTP_Status_Permanent_Redirect 308
#define HTTP_Status_Bad_Request 400
#define HTTP_Status_Unauthorized 401
#define HTTP_Status_Forbidden 403
#define HTTP_Status_Not_Found 404
#define HTTP_Status_Method_Not_Allowed 405
#define HTTP_Status_Not_Acceptable 406
#define HTTP_Status_Proxy_Authentication_Required 407
#define HTTP_Status_Request_Timeout 408
#define HTTP_Status_Conflict 409
#define HTTP_Status_Gone 410
#define HTTP_Status_Length_Required 411
#define HTTP_Status_Precondition_Failed 412
#define HTTP_Status_Payload_Too_Large 413
#define HTTP_Status_URI_Too_Long 414
#define HTTP_Status_Unsupported_Media_Type 415
#define HTTP_Status_Range_Not_Satisfiable 416
#define HTTP_Status_Expectation_Failed 417
#define HTTP_Status_Im_A_Teapot 418
#define HTTP_Status_Unprocessable_Entity 422
#define HTTP_Status_Too_Early 425
#define HTTP_Status_Upgrade_Required 426
#define HTTP_Status_Precondition_Required 428
#define HTTP_Status_Too_Many_Requests 429
#define HTTP_Status_Request_Header_Fields_Too_Large 431
#define HTTP_Status_Unavailable_For_Legal_Reasons 451
#define HTTP_Status_Internal_Server_Error 500
#define HTTP_Status_Not_Implemented 501
#define HTTP_Status_Bad_Gateway 502
#define HTTP_Status_Service_Unavailable 503
#define HTTP_Status_Gateway_Timeout 504
#define HTTP_Status_HTTP_Version_Not_Supported 505
#define HTTP_Status_Network_Authentication_Required 511
        
typedef struct _response_t response_t;

response_t *new_response(int status);
void delete_response(response_t *r);

int response_status(response_t *r);
const char *response_get_header_value(response_t *r, const char *key);
membuf_t *response_body(response_t *r);

void response_set_status(response_t *r, int status);
int response_set_mimetype(response_t *r, const char *mimetype);
int response_set_header(response_t *r, const char *key, const char *value);

int response_append(response_t *r, const char *data, int len);
int response_json(response_t *r, json_object_t obj);
int response_printf(response_t *r, const char *format, ...);

// For debugging
int response_dumpto(response_t *r, const char *file);
FILE *response_dumpfile(response_t *r);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_RESPONSE_H_
