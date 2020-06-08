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
#include <string.h>
#include <r.h>

#include "rcom/util.h"
#include "rcom/app.h"

#include "sha1.h"
#include "http.h"
#include "net.h"
#include "export.h"
#include "http_parser.h"
#include "request_priv.h"
#include "util_priv.h"

/******************************************************************************/

struct _request_t
{
        int method;
        char *uri;
        char *arg;
        membuf_t *body;        
        list_t *headers;
        
        int continue_parsing;
        int parse_what;
        int parser_header_state;
        membuf_t *uri_buffer;
        membuf_t *header_name;
        membuf_t *header_value;
};

request_t *new_request()
{
        request_t *r = r_new(request_t);
        if (r == NULL)
                return NULL;

        r->parser_header_state = k_header_new;
        r->header_name = new_membuf();
        r->header_value = new_membuf();
        r->uri_buffer = new_membuf();

        if (r->header_name == NULL
            || r->header_value == NULL
            || r->uri_buffer == NULL) {
                delete_request(r);
                return NULL;
        }
        
        return r;
}

void delete_request(request_t *r)
{
        if (r) {
                if (r->body)
                        delete_membuf(r->body);
                if (r->header_name)
                        delete_membuf(r->header_name);
                if (r->header_value)
                        delete_membuf(r->header_value);                
                if (r->uri)
                        r_free(r->uri);
                if (r->arg)
                        r_free(r->arg);
                for (list_t *l = r->headers; l != NULL; l = list_next(l)) {
                        http_header_t *h = list_get(l, http_header_t);
                        delete_http_header(h);
                }
                delete_list(r->headers);
                r_delete(r);
        }
}

const char *request_uri(request_t *r)
{
        return r->uri;
}

const char *request_args(request_t *r)
{
        return r->arg;
}

const char *request_body(request_t *r)
{
        return r->body? membuf_data(r->body) : NULL;
}

int request_length(request_t *r)
{
        return r->body? membuf_len(r->body) : 0;
}

json_object_t request_parse(request_t *r)
{
        json_object_t obj;
        
        if (request_body(r) == NULL || request_length(r) == 0) {
                r_warn("request_parse: empty request");
                return json_null();
        }
        
        membuf_append_zero(r->body);
                
        obj = json_parse(request_body(r));
        if (obj == json_null()) {
                r_warn("request_parse: parsing failed: %s", request_body(r));
                return json_null();
        }

        return obj;
}

static int request_message_begin(http_parser *p)
{
        return 0;
}

static int request_message_complete(http_parser *p)
{
        request_t *r = (request_t *) p->data;
        r->continue_parsing = 0;
        return 0;
}

static int request_on_body(http_parser *parser, const char *data, size_t length)
{
        request_t *r = (request_t *) parser->data;

        if (r->body == NULL) {
                r->body = new_membuf();
        }
        membuf_append(r->body, data, length);
        
        return 0;
}

static int request_on_url(http_parser *parser, const char *data, size_t length)
{
        request_t *r = (request_t *) parser->data;
        membuf_append(r->uri_buffer, data, length);
        return 0;
}

static int request_add_header(request_t *r, const char *key, const char *value)
{
        //r_debug("request_add_header: name %s, value %s", key, value);
        http_header_t *h = new_http_header(key, value);
        if (h == NULL)
                return - 1;
        r->headers = list_prepend(r->headers, h);
        return (r->headers == NULL)? -1 : 0;
}

static http_header_t *request_get_header(request_t *r, const char *key)
{
        for (list_t *l = r->headers; l != NULL; l = list_next(l)) {
                http_header_t *h = list_get(l, http_header_t);
                if (rstreq(key, h->name))
                        return h;
        }
        return NULL;
}

const char *request_get_header_value(request_t *request, const char *key)
{
        http_header_t *h = request_get_header(request, key);
        return h == NULL? NULL : h->value;
}

int request_set_header(request_t *r, const char *key, const char *value)
{
        //r_debug("request_set_header: name %s, value %s", key, value);
        http_header_t *h = request_get_header(r, key);
        if (h == NULL)
                return request_add_header(r, key, value);
        else 
                return http_header_set_value(h, value);
}

int request_on_header_field(http_parser *parser, const char *data, size_t length)
{
        request_t *r = (request_t *) parser->data;

        //r_debug("request_on_header_field: %.*s", length, data);
        
        if (r->parser_header_state == k_header_new
            || r->parser_header_state == k_header_value) {

                // Store the previous header
                if (membuf_len(r->header_name) > 0 
                    && membuf_len(r->header_value) > 0) {
                        membuf_append_zero(r->header_name);
                        membuf_append_zero(r->header_value);
                        request_set_header(r,
                                            membuf_data(r->header_name),
                                            membuf_data(r->header_value));
                }
                
                // Store the next header
                membuf_clear(r->header_name);
                membuf_clear(r->header_value);                
                membuf_append(r->header_name, data, length);
                r->parser_header_state = k_header_field;
        } else {
                // header_state is k_header_field
                membuf_append(r->header_name, data, length);
        }
        return 0;
}

int request_on_header_value(http_parser *parser, const char *data, size_t length)
{
        request_t *r = (request_t *) parser->data;

        //r_debug("request_on_header_value: %.*s", length, data);

        if (r->parser_header_state == k_header_new) {
                r_err("request_on_header_value: got header value before header field");
                return 0;
        } else {
                // header_state is k_header_field or k_header_value
                membuf_append(r->header_value, data, length);
                r->parser_header_state = k_header_value;
        }
        return 0;
}

int request_on_headers_complete(http_parser *p)
{
        request_t *r = (request_t *) p->data;

        // check for remaining header data
        if (membuf_len(r->header_name) > 0 
            && membuf_len(r->header_value) > 0) {
                membuf_append_zero(r->header_name);
                membuf_append_zero(r->header_value);
                request_set_header(r,
                                    membuf_data(r->header_name),
                                    membuf_data(r->header_value));
        }

        // process the uri
        membuf_append_zero(r->uri_buffer);

        // make a copy
        char uri[2048];
        int len = membuf_len(r->uri_buffer);
        if (len > sizeof(uri)) {
                r_err("URI too long");
                return -1;
        }
        memcpy(uri, membuf_data(r->uri_buffer), len);
        membuf_clear(r->uri_buffer);
                        
        // check for arguments 
        char *t = strchr(uri, '?');
        if (t != NULL) {
                *t = '\0';
                t = t+1;
        }

        r->uri = r_strdup(uri);
        if (t)
                r->arg = r_strdup(t);
        
        delete_membuf(r->uri_buffer);
        r->uri_buffer = NULL;
        delete_membuf(r->header_name);
        r->header_name = NULL;
        delete_membuf(r->header_value);
        r->header_value = NULL;

        switch (p->method) {
        case HTTP_GET:
                r->method = HTTP_METHOD_GET;
                break;
        case HTTP_POST:
                r->method = HTTP_METHOD_POST;
                break;
        default:
                r->method = HTTP_METHOD_UNSUPPORTED;
                break;
        }
        
        if (r->parse_what == REQUEST_PARSE_HEADERS)
                r->continue_parsing = 0;
        return 0;
}

int request_parse_html(request_t *request, tcp_socket_t client_socket, int what)
{
        http_parser *parser;
        http_parser_settings settings;
        //size_t len = 80*1024;
        size_t len = 1;   // ------------------------ // TODO // FIXME
        char buf[len];
        ssize_t received;
        size_t parsed;
        
        http_parser_settings_init(&settings);

        settings.on_url = request_on_url;
        settings.on_header_field = request_on_header_field;
        settings.on_header_value = request_on_header_value;
        settings.on_headers_complete = request_on_headers_complete;
        settings.on_message_begin = request_message_begin;
        settings.on_body = request_on_body;
        settings.on_message_complete = request_message_complete;
        
        parser = r_new(http_parser);
        if (parser == NULL) {
                r_err("request_parse: out of memory");
                return -1;
        }
        http_parser_init(parser, HTTP_REQUEST);
        parser->data = request;

        request->parse_what = what;
        request->continue_parsing = 1;
        
        while (!app_quit() && request->continue_parsing) {
                
                received = tcp_socket_recv(client_socket, buf, len);
                if (received < 0) {
                        r_err("request_parse: recv failed");
                        http_send_error_headers(client_socket,
                                                HTTP_Status_Internal_Server_Error);
                        r_delete(parser);
                        return -1;
                }

                /* Start up / continue the parser.
                 * Note we pass received==0 to signal that EOF has been received.
                 */
                parsed = http_parser_execute(parser, &settings, buf, received);
                
                if (received == 0 && request->continue_parsing == 0)
                        break;
                
                if (received == 0 && request->continue_parsing != 0) {
                        r_err("request_parse: bad request");
                        r_delete(parser);
                        return -1;
                }

                if (parsed != received
                    && parsed != received - 1
                    && request->continue_parsing) {
                        /* Handle error. Usually just close the connection. */
                        r_err("request_parse: parsed != received (%d != %d)",
                              parsed, received);
                        /* http_send_error_headers(client_socket, */
                        /*                         HTTP_Status_Internal_Server_Error); */
                        r_delete(parser);
                        return -1;
                }
        }
        
        r_delete(parser);
        return 0;
}

int request_is_websocket(request_t *r)
{
        const char *key = request_get_header_value(r, "Sec-WebSocket-Key");
        if (key == NULL)
                return 0;
        
        const char *version = request_get_header_value(r, "Sec-WebSocket-Version");
        if (version == NULL)
                return 0;
        if (!rstreq(version, "13"))
                return 0;
        
        const char *upgrade = request_get_header_value(r, "Upgrade");
        if (upgrade == NULL)
                return 0;
        if (!rstreq(upgrade, "websocket"))
                return 0;

        const char *connection = request_get_header_value(r, "Connection");
        if (connection == NULL)
                return 0;
        if (strstr(connection, "Upgrade") == NULL)
                return 0;
        
        return 1;
}
