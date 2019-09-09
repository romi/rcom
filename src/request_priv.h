#ifndef _RCOM_REQUEST_PRIV_H_
#define _RCOM_REQUEST_PRIV_H_

#include "rcom/addr.h"
#include "rcom/request.h"

#include "http_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
        REQUEST_PARSE_HEADERS,
        REQUEST_PARSE_ALL
};
        
typedef struct _service_t service_t;

request_t* new_request();
void delete_request(request_t *request);

int request_parse_html(request_t *request, tcp_socket_t client_socket, int what);

/* void request_parsing_start(request_t* request, int what); */
/* int request_parsing_continue(request_t* request); */

/* int request_on_body(http_parser *parser, const char *data, size_t length); */
/* int request_on_url(http_parser *parser, const char *data, size_t length); */
/* int request_message_begin(http_parser *p); */
/* int request_message_complete(http_parser *p); */
/* int request_on_header_field(http_parser *parser, const char *data, size_t length); */
/* int request_on_header_value(http_parser *parser, const char *data, size_t length); */
/* int request_on_headers_complete(http_parser *p); */

int request_is_websocket(request_t *r);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_REQUEST_PRIV_H_
