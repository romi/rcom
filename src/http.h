
#ifndef _RCOM_HTTP_H_
#define _RCOM_HTTP_H_

#include <stdlib.h>
#include <string.h>
#include "net.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _http_header_t {
        char *name;
        char *value;
} http_header_t;


http_header_t *new_http_header(const char *name, int length);
void delete_http_header(http_header_t *h);
int http_header_append_name(http_header_t *h, const char *name, int length);
int http_header_append_value(http_header_t *h, const char *value, int length);


const char *http_status_string(int status);

int http_send_headers(tcp_socket_t socket, int status, const char *mimetype, int content_length);
int http_send_error_headers(tcp_socket_t socket, int status);
int http_send_streaming_headers(tcp_socket_t socket, const char *mimetype);
int http_send_chunk(tcp_socket_t socket, const char *data, int datalen);


const char *filename_to_mimetype(const char *filename);
const char *mimetype_to_fileextension(const char *mimetype);


#ifdef __cplusplus
}
#endif

#endif // _RCOM_HTTP_H_
