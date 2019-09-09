#ifndef _RCOM_RESPONSE_PRIV_H_
#define _RCOM_RESPONSE_PRIV_H_

#include "rcom/response.h"
#include "http.h"

enum {
        RESPONSE_PARSE_HEADERS,
        RESPONSE_PARSE_ALL
};

int response_parse_html(response_t *r, tcp_socket_t socket, int what);
list_t *response_headers(response_t *r);
int response_send(response_t *r, tcp_socket_t client_socket);

#endif // _RCOM_RESPONSE_PRIV_H_
