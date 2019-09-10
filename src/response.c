#include <r.h>
#include <rcom.h>
#include "net.h"
#include "http_parser.h"
#include "response_priv.h"

struct _response_t {
        int status;
        list_t *headers;
        membuf_t *body;

        int continue_parsing;
        int parse_what;
        int parser_header_state;
        membuf_t *status_buffer;
        membuf_t *header_name;
        membuf_t *header_value;
};

response_t *new_response(int status)
{
        response_t *r = r_new(response_t);
        if (r == NULL)
                return NULL;

        r->status = status;
        r->headers = 0;
        r->body = new_membuf();
        if (r->body == NULL) {
                delete_response(r);
                return NULL;
        }

        //r->cur_header = NULL;
        r->parser_header_state = k_header_new;
        r->header_name = new_membuf();
        r->header_value = new_membuf();
        r->status_buffer = new_membuf();
        
        if (r->header_name == NULL
            || r->header_value == NULL
            || r->status_buffer == NULL) {
                delete_response(r);
                return NULL;
        }
        return r;
}

void delete_response(response_t *r)
{
        if (r) {
                if (r->body)
                        delete_membuf(r->body);                
                for (list_t *l = r->headers; l != NULL; l = list_next(l)) {
                        http_header_t *h = list_get(l, http_header_t);
                        delete_http_header(h);
                }
                if (r->header_name)
                        delete_membuf(r->header_name);
                if (r->header_value)
                        delete_membuf(r->header_value);                
                if (r->status_buffer)
                        delete_membuf(r->status_buffer);                
                delete_list(r->headers);
                r_delete(r);
        }
}

int response_status(response_t *r)
{
        return r->status;
}

void response_set_status(response_t *r, int status)
{
        r->status = status;
}

membuf_t *response_body(response_t *r)
{
        return r->body;
}

list_t *response_headers(response_t *r)
{
        return r->headers;
}

int response_set_mimetype(response_t *r, const char *mimetype)
{
        return response_set_header(r, "Content-Type", mimetype);
}

static int response_add_header(response_t *r, const char *key, const char *value)
{
        //r_debug("response_add_header: name %s, value %s", key, value);
        http_header_t *h = new_http_header(key, value);
        if (h == NULL)
                return - 1;
        r->headers = list_prepend(r->headers, h);
        return (r->headers == NULL)? -1 : 0;
}

static http_header_t *response_get_header(response_t *r, const char *key)
{
        for (list_t *l = r->headers; l != NULL; l = list_next(l)) {
                http_header_t *h = list_get(l, http_header_t);
                if (rstreq(key, h->name))
                        return h;
        }
        return NULL;
}

const char *response_get_header_value(response_t *response, const char *key)
{
        http_header_t *h = response_get_header(response, key);
        return h == NULL? NULL : h->value;
}

int response_set_header(response_t *r, const char *key, const char *value)
{
        //r_debug("response_set_header: name %s, value %s", key, value);
        http_header_t *h = response_get_header(r, key);
        if (h == NULL)
                return response_add_header(r, key, value);
        else 
                return http_header_set_value(h, value);
}


static int response_on_status(http_parser *parser, const char *data, size_t length)
{
        response_t *r = (response_t *) parser->data;
        membuf_append(r->status_buffer, data, length);
        return 0;
}

static int response_on_header_field(http_parser *parser, const char *data, size_t length)
{
        response_t *r = (response_t *) parser->data;

        //r_debug("response_on_header_field: %.*s", length, data);
        
        if (r->parser_header_state == k_header_new
            || r->parser_header_state == k_header_value) {

                // Store the previous header
                if (membuf_len(r->header_name) > 0 
                    && membuf_len(r->header_value) > 0) {
                        membuf_append_zero(r->header_name);
                        membuf_append_zero(r->header_value);
                        response_set_header(r,
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

static int response_on_header_value(http_parser *parser, const char *data, size_t length)
{
        response_t *r = (response_t *) parser->data;

        //r_debug("response_on_header_value: %.*s", length, data);

        if (r->parser_header_state == k_header_new) {
                r_err("response_on_header_value: got header value before header field");
                return 0;
        } else {
                // header_state is k_header_field or k_header_value
                membuf_append(r->header_value, data, length);
                r->parser_header_state = k_header_value;
        }
        return 0;
}

static int response_on_headers_complete(http_parser *p)
{
        response_t *r = (response_t *) p->data;
        
        if (membuf_len(r->header_name) > 0 
            && membuf_len(r->header_value) > 0) {
                membuf_append_zero(r->header_name);
                membuf_append_zero(r->header_value);
                response_set_header(r,
                                    membuf_data(r->header_name),
                                    membuf_data(r->header_value));
        }
        
        membuf_append_zero(r->status_buffer);
        r->status = p->status_code;
        
        delete_membuf(r->header_name);
        r->header_name = NULL;
        delete_membuf(r->header_value);
        r->header_value = NULL;
        
        if (r->parse_what == RESPONSE_PARSE_HEADERS)
                r->continue_parsing = 0;
        
        return 0;
}

static int response_on_body(http_parser *parser, const char *data, size_t length)
{
        response_t *r = (response_t *) parser->data;
        membuf_t *body = r->body;
        membuf_append(body, data, length);
        return 0;
}

static int response_on_message_complete(http_parser *parser)
{
        response_t *r = (response_t *) parser->data;
        r->continue_parsing = 0;
        return 0;
}

int response_parse_html(response_t *response, tcp_socket_t socket, int what)
{
        http_parser *parser;
        http_parser_settings settings;
        //size_t len = 80*1024;
        size_t len = 1;   // ------------------------ // TODO // FIXME
        char buf[len];
        size_t parsed;
        ssize_t received;
                
        http_parser_settings_init(&settings);

        settings.on_status = response_on_status;
        settings.on_header_field = response_on_header_field;
        settings.on_header_value = response_on_header_value;
        settings.on_headers_complete = response_on_headers_complete;
        settings.on_body = response_on_body;
        settings.on_message_complete = response_on_message_complete;
        
        parser = r_new(http_parser);
        if (parser == NULL) {
                r_err("http_read_response: out of memory");
                return -1;
        }
        http_parser_init(parser, HTTP_RESPONSE);
        parser->data = response;
        
        response->parse_what = what;
        response->continue_parsing = 1;
        
        while (!app_quit() && response->continue_parsing) {
                
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

int response_append(response_t *r, const char *data, int len)
{
        int ret;
        ret = membuf_append(r->body, data, len);
        if (ret != 0)
                return -1;

        return 0;
}

static int32 response_serialise(response_t *r, const char *s, int32 len)
{
        return membuf_append(r->body, s, len);
}

int response_json(response_t *r, json_object_t obj)
{
        return json_serialise(obj, 0, (json_writer_t) response_serialise, r); 
}

int response_printf(response_t *r, const char *format, ...)
{
        int len;
        va_list ap;
        int ret;
        
        va_start(ap, format);
        len = vsnprintf(NULL, 0, format, ap);
        va_end(ap);

        if (len < 0)
                return -1;

        membuf_assure(r->body, len+1);

        va_start(ap, format);
        ret = membuf_vprintf(r->body, format, ap);
        va_end(ap);

        return ret;
}

int response_send(response_t *response, tcp_socket_t client_socket)
{
        return http_send_response(client_socket, response);
}
