#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include "rcom/log.h"

#include "mem.h"
#include "http.h"
#include "net.h"
#include "util.h"

http_header_t *new_http_header(const char *name, int length)
{
        http_header_t *h = new_obj(http_header_t);
        if (h == NULL) return NULL;

        h->value = NULL;
        h->name = NULL;

        if (http_header_append_name(h, name, length) != 0) {
                delete_http_header(h);
                return NULL;
        }
        
        return h;
} 

void delete_http_header(http_header_t *h)
{
        if (h) {
                if (h->name) mem_free(h->name);
                if (h->value) mem_free(h->value);
                delete_obj(h);
        }
}

static char *_append_(char *s, const char *value, int length)
{
        int len = (s)? strlen(s) : 0;
        int total = len + length;
        s = mem_realloc(s, total + 1);
        if (s == NULL)
                return NULL;
        memcpy(s + len, value, length);
        s[total] = 0;
        return s;
}

static void _trim_(char *s)
{
        int i  = strlen(s) - 1;
        while (i >= 0 && (s[i] == '\n' || s[i] == '\r' || s[i] == ' '))
                s[i--] = 0;
}

int http_header_append_name(http_header_t *h, const char *name, int length)
{
        h->name = _append_(h->name, name, length);
        return (h->name == NULL)? -1 : 0;
}

int http_header_append_value(http_header_t *h, const char *value, int length)
{
        h->value = _append_(h->value, value, length);
        if (h->value == NULL) return -1;
        _trim_(h->value);
        return 0;
}

const char *http_status_string(int status)
{
        switch (status) {
        case 100: return "Continue";
        case 101: return "Switching Protocols";
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 203: return "Non-Authoritative Information";
        case 204: return "No Content";
        case 205: return "Reset Content";
        case 206: return "Partial Content";
        case 300: return "Multiple Choices";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 307: return "Temporary Redirect";
        case 308: return "Permanent Redirect";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 406: return "Not Acceptable";
        case 407: return "Proxy Authentication Required";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 411: return "Length Required";
        case 412: return "Precondition Failed";
        case 413: return "Payload Too Large";
        case 414: return "URI Too Long";
        case 415: return "Unsupported Media Type";
        case 416: return "Range Not Satisfiable";
        case 417: return "Expectation Failed";
        case 418: return "I'm a teapot";
        case 422: return "Unprocessable Entity";
        case 425: return "Too Early";
        case 426: return "Upgrade Required";
        case 428: return "Precondition Required";
        case 429: return "Too Many Requests";
        case 431: return "Request Header Fields Too Large";
        case 451: return "Unavailable For Legal Reasons";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
        case 511: return "Network Authentication Required";
        default: return "--"; break;
        }
}

int http_send_headers(tcp_socket_t socket, int status, const char *mimetype, int content_length)
{
        char header[2048];
        // FIXME: write directly in socket, instead of intermediate buffer
        int len = snprintf(NULL, 0,
                           "HTTP/1.1 %d %s\r\n"
                           "Content-Type: %s\r\n"
                           "Content-Length: %d\r\n"
                           "Connection: close\r\n\r\n",
                           status, http_status_string(status), mimetype, content_length);
        if (len > sizeof(header) - 1) {
                log_err("http_send_headers: header fields too long");
                return -1;
        }
        
        len = snprintf(header, sizeof(header),
                       "HTTP/1.1 %d %s\r\n"
                       "Content-Type: %s\r\n"
                       "Content-Length: %d\r\n"
                       "Connection: close\r\n\r\n",
                       status, http_status_string(status), mimetype, content_length);

        return tcp_socket_send(socket, header, len);
}

int http_send_error_headers(tcp_socket_t socket, int status)
{
        char header[2048];
        log_debug("streamer_client_send_error_headers");
        int len = snprintf(header, 2048,
                           "HTTP/1.1 %d %s\r\n"
                           "Connection: close\r\n\r\n",
                           status, http_status_string(status));
        return tcp_socket_send(socket, header, len);
}

int http_send_streaming_headers(tcp_socket_t socket, const char *mimetype)
{
        char header[2048];
        log_debug("request_send_headers");
        int len = snprintf(header, 2048,
                           "HTTP/1.1 %d %s\r\n"
                           "Content-Type: %s\r\n"
                           "Transfer-Encoding: chunked\r\n"
                           "\r\n",
                           200, http_status_string(200), mimetype);
        return tcp_socket_send(socket, header, len);
}

int http_send_chunk(tcp_socket_t socket, const char *data, int datalen)
{
        char buf[64];
        int len = snprintf(buf, 64, "%x\r\n", datalen);
        if (tcp_socket_send(socket, buf, len) == -1)
                return -1;
        if (tcp_socket_send(socket, data, datalen) == -1)
                return -1;
        if (tcp_socket_send(socket, "\r\n", 2) == -1)
                return -1;
        return 0;
}

typedef struct {
        const char *ext;
        const char *mimetype;
} mime_map_t;

static mime_map_t map[] = { {".html", "text/html"},
                            {".txt", "text/plain"},
                            {".js", "application/javascript; charset=utf-8"},
                            {".json", "application/json; charset=utf-8"},
                            {".css", "text/css"},
                            {".jpg", "image/jpeg"},
                            {".png", "image/png"},
                            {".glb", "application/octet-stream"},
                            {".bin", "application/octet-stream"},
                            {".svg", "image/svg+xml"},
                            {NULL, NULL},
};

const char *filename_to_mimetype(const char *filename)
{
        int i = 0;
        while (map[i].ext) {
                int n = strlen(map[i].ext);
                int m = strlen(filename);
                if (m < n + 1)
                        continue;
                const char *p = filename + m - n;
                if (streq(p, map[i].ext))
                    return map[i].mimetype;
                i++;
        }
        return NULL;
}

const char *mimetype_to_fileextension(const char *mimetype)
{
        int i = 0;
        while (map[i].ext) {
                int n = strlen(map[i].mimetype);
                int m = strlen(mimetype);
                int len = (m < n)? m : n;
                if (strncmp(map[i].mimetype, mimetype, len) == 0)
                    return map[i].ext;
                i++;
        }
        return NULL;
}
