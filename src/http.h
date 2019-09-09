
#ifndef _RCOM_HTTP_H_
#define _RCOM_HTTP_H_

#include <stdlib.h>
#include <string.h>
#include <r.h>
#include "net.h"
#include "response_priv.h"

#ifdef __cplusplus
extern "C" {
#endif
        
enum {
        k_header_new = 0,
        k_header_field,
        k_header_value
};

typedef struct _http_header_t {
        char *name;
        char *value;
} http_header_t;


http_header_t *new_http_header(const char *name, const char *value);
void delete_http_header(http_header_t *h);
int http_header_name_append(http_header_t *h, const char *name, int length);
int http_header_value_append(http_header_t *h, const char *value, int length);
int http_header_set_value(http_header_t *h, const char *value);


const char *http_status_string(int status);

int http_send_headers(tcp_socket_t socket, int status,
                      const char *mimetype, int content_length);
int http_send_response(tcp_socket_t socket, response_t* r);
int http_send_error_headers(tcp_socket_t socket, int status);
int http_send_streaming_headers(tcp_socket_t socket, const char *mimetype);
int http_send_chunk(tcp_socket_t socket, const char *data, int datalen);

int http_get(addr_t *addr, const char *resource, response_t **response_handle);
int http_post(addr_t *addr,
              const char *resource,
              const char *content_type,
              const char *data, int len,
              response_t **response_handle);

const char *mimetype_to_fileextension(const char *mimetype);


#ifdef __cplusplus
}
#endif

#endif // _RCOM_HTTP_H_
