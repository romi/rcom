#include <string.h>

#include <r.h>
#include "rcom/util.h"
#include "rcom/app.h"

#include "http.h"
#include "net.h"
#include "http_parser.h"

static int http_send_request(tcp_socket_t socket, addr_t *addr, const char *resource,
                             const char *content_type, const char *data, int len);
static int http_send_request_headers(tcp_socket_t socket,
                                     const char *resource,
                                     const char *host,
                                     const char *content_type,
                                     int content_length);
static int http_read_response(tcp_socket_t socket, membuf_t *buf);

http_header_t *new_http_header(const char *name, int length)
{
        http_header_t *h = r_new(http_header_t);
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
                if (h->name) r_free(h->name);
                if (h->value) r_free(h->value);
                r_delete(h);
        }
}

static char *_append_(char *s, const char *value, int length)
{
        int len = (s)? strlen(s) : 0;
        int total = len + length;
        s = r_realloc(s, total + 1);
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
        // FIXME: or use a membuf!
        int len = snprintf(NULL, 0,
                           "HTTP/1.1 %d %s\r\n"
                           "Content-Type: %s\r\n"
                           "Content-Length: %d\r\n"
                           "Access-Control-Allow-Origin: *\r\n"
                           "Connection: close\r\n\r\n",
                           status, http_status_string(status), mimetype, content_length);
        if (len > sizeof(header) - 1) {
                r_err("http_send_headers: header fields too long");
                return -1;
        }
        
        len = snprintf(header, sizeof(header),
                       "HTTP/1.1 %d %s\r\n"
                       "Content-Type: %s\r\n"
                       "Content-Length: %d\r\n"
                       "Access-Control-Allow-Origin: *\r\n"
                       "Connection: close\r\n\r\n",
                       status, http_status_string(status), mimetype, content_length);

        return tcp_socket_send(socket, header, len);
}

int http_send_error_headers(tcp_socket_t socket, int status)
{
        char header[2048];
        r_debug("streamer_client_send_error_headers");
        int len = snprintf(header, 2048,
                           "HTTP/1.1 %d %s\r\n"
                           "Connection: close\r\n\r\n",
                           status, http_status_string(status));
        return tcp_socket_send(socket, header, len);
}

int http_send_streaming_headers(tcp_socket_t socket, const char *mimetype)
{
        char header[2048];
        //r_debug("request_send_headers");
        int len = snprintf(header, 2048,
                           "HTTP/1.1 %d %s\r\n"
                           "Content-Type: %s\r\n"
                           "Transfer-Encoding: chunked\r\n"
                           "\r\n",
                           200, http_status_string(200), mimetype);
        return tcp_socket_send(socket, header, len);
}

int http_get(addr_t *addr, const char *resource, membuf_t *out)
{
        return http_post(addr, resource, NULL, NULL, 0, out);
}

int http_post(addr_t *addr,
             const char *resource,
             const char *content_type,
             const char *data, int len,
             membuf_t *out)
{
        int err;
        
        tcp_socket_t socket = open_tcp_socket(addr);
        if (socket == INVALID_TCP_SOCKET) {
                r_err("http_get: failed to open a connection");
                return -1;
        }

        err = http_send_request(socket, addr, resource,
                                content_type, data, len);
        if (err != 0) return err;

        err = http_read_response(socket, out);
        if (err != 0) return err;

        close_tcp_socket(socket);
        
        return 0;
}

int http_send_request(tcp_socket_t socket,
                      addr_t *addr,
                      const char *resource,
                      const char *content_type,
                      const char *data, int len)
{
        int ret;
        char b[52];
        
        addr_string(addr, b, 52);
        
        ret = http_send_request_headers(socket, resource, b, content_type, len);
        if (ret != 0) {
                r_err("client_send_request: failed to send the headers");
                return 0;
        }
        
        if (len) {
                ret = tcp_socket_send(socket, data, len);
                if (ret != 0) 
                        r_err("client_send_request: failed to send the body");
        }

        return ret;
}

static int http_send_request_headers(tcp_socket_t socket,
                                     const char *resource,
                                     const char *host,
                                     const char *content_type,
                                     int content_length)
{
        char header[2048];
        int len;
        
        // FIXME: write directly in socket, instead of intermediate buffer
        if (content_length > 0) {
                len = snprintf(header, 2048,
                               "POST /%s HTTP/1.1\r\n"
                               "Host: %s\r\n"
                               "Content-Type: %s\r\n"
                               "Content-Length: %d\r\n\r\n",
                               resource, host, content_type, content_length);
        } else {
                len = snprintf(header, 2048,
                               "GET /%s HTTP/1.1\r\n"
                               "Host: %s\r\n\r\n",
                               resource, host);
        }
        
        return tcp_socket_send(socket, header, len);
}

static int http_message_complete(http_parser *p)
{
        return 0;
}

static int http_headers_complete(http_parser *p)
{
        return 0;
}

static int http_on_body(http_parser *parser, const char *data, size_t length)
{
        membuf_t *m = (membuf_t *) parser->data;
        membuf_append(m, data, length);
        return 0;
}

static int http_read_response(tcp_socket_t socket, membuf_t *out)
{
        size_t len = 80*1024;
        size_t parsed;
        char buf[len];
        ssize_t received;
        http_parser *parser;
        http_parser_settings settings;
        membuf_t *membuf = NULL;
        int ret;
        char ermes[200];
        
        http_parser_settings_init(&settings);
        settings.on_body = http_on_body;
        settings.on_headers_complete = http_headers_complete;
        settings.on_message_complete = http_message_complete;
        
        parser = r_new(http_parser);
        if (parser == NULL) {
                r_err("http_read_response: out of memory");
                return -1;
        }
        http_parser_init(parser, HTTP_RESPONSE);
        parser->data = out;
        membuf_clear(out);
        
        while (!app_quit()) {
                
                if (tcp_socket_wait_data(socket, 1) == 1) {

                        received = tcp_socket_recv(socket, buf, len);
                        if (received < 0)
                                return -1;

                        //r_debug("http_read_response: %.*s", received, buf);
                        
                        /* Start up / continue the parser.
                         * Note we pass received==0 to signal that EOF has been received.
                         */
                        parsed = http_parser_execute(parser, &settings, buf, received);
                        if (received == 0)
                                break;
                        
                        if (parsed != received)
                                /* Handle error. Usually just close the connection. */
                                r_err("http_read_response: parsed != received");
                }
        }
        
        r_delete(parser);

        return 0;
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
                            {".xml", "application/xml"},
                            {".ico", "image/vnd.microsoft.icon"},
                            {".webmanifest", "application/manifest+json"},
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
                if (rstreq(p, map[i].ext))
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
