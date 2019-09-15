#ifndef _RCOM_RESPONSE_PRIV_H_
#define _RCOM_RESPONSE_PRIV_H_

#include "rcom/response.h"
#include "http.h"

enum {
        RESPONSE_PARSE_HEADERS,
        RESPONSE_PARSE_ALL
};


typedef int (*response_onheaders_t)(void *userdata, response_t *response);
typedef int (*response_ondata_t)(void *userdata, response_t *response,
                                 const char *data, int len);

int response_set_onheaders(response_t *r, response_onheaders_t onheaders, void *userdata);
int response_set_ondata(response_t *r, response_ondata_t ondata, void *userdata);

int response_parse_html(response_t *r, tcp_socket_t socket, int what);
list_t *response_headers(response_t *r);
int response_send(response_t *r, tcp_socket_t client_socket);

#endif // _RCOM_RESPONSE_PRIV_H_
