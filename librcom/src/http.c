/*
  rcutil

  Copyright (C) 2019 Sony Computer Science Laboratories
  Author(s) Peter Hanappe

  rcutil is light-weight libary for inter-node communication.

  rcutil is free software: you can redistribute it and/or modify it
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
#include <string.h>

#include <r.h>
#include "util.h"

#include "http.h"
#include "net.h"

static int http_send_request(tcp_socket_t socket, addr_t *addr, const char *resource,
                             const char *content_type, const char *data, int len);
static int http_send_request_headers(tcp_socket_t socket,
                                     const char *resource,
                                     const char *host,
                                     const char *content_type,
                                     int content_length);
/* static int http_read_response(tcp_socket_t socket, membuf_t *buf); */

http_header_t *new_http_header(const char *name, const char *value)
{
        http_header_t *h = r_new(http_header_t);
        if (h == NULL) return NULL;

        h->name = NULL;
        h->value = NULL;

        //r_debug("new_http_header");
        if (name) {
                //r_debug("   name %s", name);
                h->name = r_strdup(name);
                if (h->name == NULL) {
                        delete_http_header(h);
                        return NULL;
                }
        }

        if (value) {
                //r_debug("   value %s", value);
                h->value = r_strdup(value);
                if (h->value == NULL) {
                        delete_http_header(h);
                        return NULL;
                }
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

int http_header_name_append(http_header_t *h, const char *name, int length)
{
        h->name = _append_(h->name, name, length);
        return (h->name == NULL)? -1 : 0;
}

int http_header_value_append(http_header_t *h, const char *value, int length)
{
        h->value = _append_(h->value, value, length);
        if (h->value == NULL) return -1;
        _trim_(h->value);
        return 0;
}

int http_header_set_value(http_header_t *h, const char *value)
{
        //r_debug("http_header_set_value: name %s, value %s", h->name, value);
        if (h->value == NULL)
                r_free(h->value);
        h->value = r_strdup(value);
        return h->value == NULL? -1 : 0;
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

int http_send_response(tcp_socket_t socket, response_t* r)
{
        int status = response_status(r);
        membuf_t *headers = new_membuf();
        membuf_t *body = response_body(r);
        
        membuf_printf(headers, "HTTP/1.1 %d %s\r\n",
                      status, http_status_string(status));
        
        membuf_printf(headers, "Content-Length: %d\r\n", membuf_len(body));
        membuf_printf(headers, "Connection: close\r\n");
        
        for (list_t *l = response_headers(r); l != NULL; l = list_next(l)) {
                http_header_t *h = list_get(l, http_header_t);
                membuf_printf(headers, "%s: %s\r\n", h->name, h->value);
        }

        membuf_printf(headers, "\r\n");

        if (tcp_socket_send(socket, membuf_data(headers), membuf_len(headers)) != 0)
                return -1;

        delete_membuf(headers);
        return tcp_socket_send(socket, membuf_data(body), membuf_len(body));
}

int http_send_headers(tcp_socket_t socket, int status,
                      const char *mimetype, int content_length)
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
        if ((unsigned)len > (sizeof(header) - 1)) {
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
        //r_debug("streamer_client_send_error_headers");
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

int http_get(addr_t *addr, const char *resource, response_t **response_handle)
{
        return http_post(addr, resource, NULL, NULL, 0, response_handle);
}

int http_post(addr_t *addr,
              const char *resource,
              const char *content_type,
              const char *data, int len,
              response_t **response_handle)
{
        int err;
        
        tcp_socket_t socket = open_tcp_socket(addr);
        if (socket == INVALID_TCP_SOCKET)
                return -1;
        
        err = http_send_request(socket, addr, resource, content_type, data, len);
        if (err != 0) {
                r_err("http_post: send request close_tcp_socket");
                close_tcp_socket(socket);
                return -1;
        }

        int allocated_response = 0;
        response_t *response = *response_handle;
        if (response == NULL) {
                response = new_response(HTTP_Status_OK);
                if (response == NULL) {
                        r_err("http_post: response close_tcp_socket");
                        close_tcp_socket(socket);
                        return -1;
                }
                *response_handle = response;
                allocated_response = 1;
        }
        
        err = response_parse_html(response, socket, RESPONSE_PARSE_ALL);
        if (err != 0 && allocated_response) {
                delete_response(response);
                *response_handle = NULL;
        }
        r_debug("http_post: close_tcp_socket");
        close_tcp_socket(socket);
        
        return err;
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

        const char *slash = "/";
        if (resource[0] == '/')
                slash = "";
        
        // FIXME: write directly in socket, instead of intermediate buffer
        if (content_length > 0) {
                len = snprintf(header, 2048,
                               "POST %s%s HTTP/1.1\r\n"
                               "Host: %s\r\n"
                               "Content-Type: %s\r\n"
                               "Content-Length: %d\r\n\r\n",
                               slash, resource, host, content_type, content_length);
        } else {
                len = snprintf(header, 2048,
                               "GET %s%s HTTP/1.1\r\n"
                               "Host: %s\r\n\r\n",
                               slash, resource, host);
        }
        
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
                            {".xml", "application/xml"},
                            {".ico", "image/vnd.microsoft.icon"},
                            {".webmanifest", "application/manifest+json"},
                            {NULL, NULL},
};

const char *filename_to_mimetype(const char *filename)
{
        const char *s = "application/octet-stream";
        
        for (int i = 0; map[i].ext != NULL; i++) {
                int n = strlen(map[i].ext);
                int m = strlen(filename);
                if (m < n + 1)
                        continue;
                const char *p = filename + m - n;
                if (rstreq(p, map[i].ext)) {
                        s = map[i].mimetype;
                        break;
                }
        }

        return s;
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
