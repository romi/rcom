#include <string.h>
#include <r.h>

#include "rcom/util.h"

#include "sha1.h"
#include "http.h"
#include "net.h"
#include "export.h"
#include "http_parser.h"
#include "service_priv.h"
#include "request_priv.h"
#include "util_priv.h"

/******************************************************************************/

typedef struct _ws_frame_t {
        uint8_t fin : 1;
        uint8_t rsv1 : 1;
        uint8_t rsv2 : 1;
        uint8_t rsv3 : 1;
        uint8_t opcode : 4;
        uint8_t mask : 1;
        uint8_t length : 7;
} __attribute__((packed, aligned(2))) ws_frame_t;

/******************************************************************************/

struct _request_t
{
        service_t *service;
        char *uri;
        char *arg;
        char *mimetype;
        tcp_socket_t socket;
        addr_t *addr;
        int cont;
        int status;
        membuf_t *in;
        membuf_t *out;
        thread_t *thread;
        
        // test
        list_t *headers; 
        http_header_t *cur_header;
};

static const char *request_mimetype(request_t *r, export_t *e);

void delete_request(request_t *r);

request_t *new_request(service_t *service, tcp_socket_t socket)
{
        request_t *r = r_new(request_t);
        if (r == NULL)
                return NULL;

        r->service = service;
        r->socket = socket;
        r->status = 200;
        r->addr = tcp_socket_addr(r->socket);

        r->out = new_membuf();
        if (r->out == NULL) {
                delete_request(r);
                return NULL;
        }
        
        return r;
}

void delete_request(request_t *r)
{
        if (r) {
                if (r->socket >= 0)
                        close_tcp_socket(r->socket);
                if (r->in)
                        delete_membuf(r->in);
                if (r->out)
                        delete_membuf(r->out);
                if (r->uri)
                        r_free(r->uri);
                if (r->mimetype)
                        r_free(r->mimetype);
                if (r->thread)
                        delete_thread(r->thread);
                if (r->addr)
                        delete_addr(r->addr);
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

addr_t *request_addr(request_t *r)
{
        return r->addr;        
}

const char *request_data(request_t *r)
{
        return r->in? membuf_data(r->in) : NULL;
}

int request_len(request_t *r)
{
        return r->in? membuf_len(r->in) : 0;
}

int request_set_status(request_t *r, int status)
{
        r->status = status;
}

json_object_t request_parse(request_t *r)
{
        json_object_t obj;
        
        if (request_data(r) == NULL || request_len(r) == 0) {
                r_warn("request_parse: empty request");
                return json_null();
        }
        
        membuf_append_zero(r->in);
                
        obj = json_parse(request_data(r));
        if (obj == json_null()) {
                r_warn("request_parse: parsing failed: %s", request_data(r));
                return json_null();
        }

        return obj;
}

int request_reply_append(request_t *r, const char *data, int len)
{
        int ret;
        ret = membuf_append(r->out, data, len);
        if (ret != 0)
                return -1;

        return 0;
}

static int32 request_serialise(request_t *r, const char *s, int32 len)
{
        return membuf_append(r->out, s, len);
}

int request_reply_json(request_t *r, json_object_t obj)
{
        return json_serialise(obj, 0, (json_writer_t) request_serialise, r); 
}

int request_reply_printf(request_t *r, const char *format, ...)
{
        int len;
        va_list ap;
        int ret;
        
        va_start(ap, format);
        len = vsnprintf(NULL, 0, format, ap);
        va_end(ap);

        if (len < 0)
                return -1;

        membuf_assure(r->out, len+1);

        va_start(ap, format);
        ret = membuf_vprintf(r->out, format, ap);
        va_end(ap);

        return ret;
}

membuf_t *request_reply_buffer(request_t *r)
{
        return r->out;
}

static int request_message_begin(http_parser *p)
{
        return 0;
}

static int request_message_complete(http_parser *p)
{
        request_t *r = (request_t *) p->data;
        r->cont = 0;
        return 0;
}

static int request_on_status(http_parser *p, const char *data, size_t length)
{
        int i;
        for (i = 0; i < length; i++) {
                printf("%c", *(data+i));
        }
        printf("\n");
        return 0;
}

static int request_on_body(http_parser *parser, const char *data, size_t length)
{
        request_t *r = (request_t *) parser->data;

        if (r->in == NULL) {
                r->in = new_membuf();
                if (r->in == NULL)
                        return 1;
        }
        if (membuf_append(r->in, data, length) != 0)
                return 1;
        
        return 0;
}

static int request_on_url(http_parser *parser, const char *data, size_t length)
{
        request_t *r = (request_t *) parser->data;

        /* membuf_t *buf = new_membuf(); */
        /* int err = urldecode(data, length, buf); */
        /* if (err == -1) */
        /*         return -1; */

        /* int len = membuf_len(buf); */
        /* r->uri = r_alloc(length + 1); */
        /* memcpy(r->uri, membuf_data(buf), len); */
        /* r->uri[len] = '\0'; */
        /* delete_membuf(buf); */

        r->uri = r_alloc(length + 1);
        memcpy(r->uri, data, length);
        r->uri[length] = '\0';

        // FIXME: shouldn't we seek the question mark before the URI
        // is decoded?
        char *s = strchr(r->uri, '?');
        if (s != NULL) {
                *s = '\0';
                r->arg = s+1;
        }
        
        return 0;
}

static int request_on_header_field(http_parser *parser, const char *data, size_t length)
{
        request_t *r = (request_t *) parser->data;
        r->cur_header = new_http_header(data, length);
        if (r->cur_header == NULL) return -1;
        return 0;
}

static int request_on_header_value(http_parser *parser, const char *data, size_t length)
{
        request_t *r = (request_t *) parser->data;
        if (r->cur_header != NULL) {
                http_header_append_value(r->cur_header, data, length);
                r->headers = list_prepend(r->headers, r->cur_header);
                r->cur_header = NULL;
        }
        return 0;
}

static int request_on_headers_complete(http_parser *parser)
{
        return 0;
}

static int request_parse_html(request_t *r)
{
        size_t len = 80*1024;
        size_t parsed;
        char buf[len];
        ssize_t received;
        http_parser *parser;
        http_parser_settings settings;

        http_parser_settings_init(&settings);
        settings.on_body = request_on_body;
        settings.on_url = request_on_url;
        settings.on_header_field = request_on_header_field;
        settings.on_header_value = request_on_header_value;
        settings.on_headers_complete = request_on_headers_complete;

        settings.on_status = request_on_status;        
        settings.on_message_begin = request_message_begin;
        settings.on_message_complete = request_message_complete;
        
        parser = r_new(http_parser);
        if (parser == NULL) {
                r_err("request_parse: out of memory");
                return -1;
        }
        http_parser_init(parser, HTTP_REQUEST);
        parser->data = r;

        r->cont = 1;
        while (r->cont) {
                received = recv(r->socket, buf, len, 0);
                if (received < 0) {
                        r_err("request_parse: recv failed");
                        r_delete(parser);
                        return -1;
                }
        
                /* Start up / continue the parser.
                 * Note we pass received==0 to signal that EOF has been received.
                 */
                parsed = http_parser_execute(parser, &settings, buf, received);
                if (received == 0) {
                        break;
                }

                if (parsed != received && parsed != received - 1) {
                        /* Handle error. Usually just close the connection. */
                        r_err("request_parse: parsed != received (%d != %d)", parsed, received);
                        r_err("data: '%s'", buf);
                        r_delete(parser);
                        return -1;
                }
        }

        // CLEANUP
        r_delete(parser);
        return 0;
}

static int request_send_headers(request_t *r, int status, const char *mimetype, int len)
{
        return http_send_headers(r->socket, status, mimetype, len);
}

void request_handle(request_t* r)
{
        int err;
        export_t *export = NULL;

        membuf_clear(r->out);
        
        err = request_parse_html(r);
        if (err != 0) {
                r_err("request_handle: parsing failed");
                goto cleanup;
        }

        r_debug("request_handle: request for %s", r->uri);

        if (r->uri == NULL) {
                r_err("request_handle: requested uri == NULL!?");
                err = request_send_headers(r, 400, "text/plain", 0);
                goto cleanup;
        }

        export = service_get_export(r->service, r->uri);
        if (export == NULL) {
                r_err("request_handle: export == NULL: resource '%s'", r->uri);
                err = request_send_headers(r, 404, "text/plain", 0);
                goto cleanup;
        }

        err = export_callback(export, r);
        if (err != 0) {
                r_err("Request handler returned non-zero (%d) for request '%s'", err, r->uri);
                err = request_send_headers(r, 500, "text/plain", 0);
                goto cleanup;
        }

        if (r->status != 200) {
                r_err("request_handle: status != 200");
                err = request_send_headers(r, r->status, "text/plain", 0);
                goto cleanup;
                
        } else {
                //r_debug("request_handle: sending response, mimetype '%s'", request_mimetype(r, export));
                err = request_send_headers(r, 200, request_mimetype(r, export),
                                           membuf_len(r->out));
                if (err != 0) {
                        r_err("request_handle: request_send_headers failed");
                        goto cleanup;
                }
                //r_debug("request_handle: sending body");
                err = tcp_socket_send(r->socket,
                                      membuf_data(r->out),
                                      membuf_len(r->out));
        }
        
cleanup:
        if (err != 0) {
                r_err("request_handle: failed to handle request");
        }
        if (export)
                delete_export(export);
        delete_request(r);
}

void request_set_mimetype(request_t *r, const char *mimetype)
{
        //r_debug("request_set_mimetype: settings mimetype to '%s'", mimetype);
        if (r->mimetype)
                r_free(r->mimetype);
        r->mimetype = NULL;
        if (mimetype != NULL)
                r->mimetype = r_strdup(mimetype);
}

static const char *request_mimetype(request_t *r, export_t *e)
{
        return (r->mimetype)? r->mimetype : export_mimetype_out(e);
}

static http_header_t *request_get_header(request_t *r, const char *name)
{
        list_t * l = r->headers;
        while (l) {
                http_header_t *h = list_get(l, http_header_t);
                if (rstreq(name, h->name))
                        return h;
                l = list_next(l);
        }
        return NULL;
}
