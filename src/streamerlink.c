
#include <r.h>

#include "rcom/app.h"
#include "rcom/registry.h"
#include "rcom/util.h"

#include "http.h"
#include "http_parser.h"
#include "streamerlink_priv.h"

typedef struct _streamerlink_t {
        char *resource;
        void* userdata;
        streamerlink_ondata_t ondata;
        tcp_socket_t socket;
        addr_t *addr;
        addr_t *remote_addr;
        list_t *headers; 
        http_header_t *cur_header;
        mutex_t *mutex;
        thread_t *thread;
        int cont;
        int autoconnect;
} streamerlink_t;


static int streamerlink_stop_thread(streamerlink_t *link);
static int streamerlink_close_connection(streamerlink_t *link);
static int streamerlink_lock(streamerlink_t *link);
static int streamerlink_unlock(streamerlink_t *link);


streamerlink_t *new_streamerlink(streamerlink_ondata_t ondata,
                                 void* userdata, int autoconnect)
{
        streamerlink_t* link;

        if (ondata == NULL) {
                r_err("new_streamerlink: invalid arguments");
                return NULL;
        }
        
        link = r_new(streamerlink_t);
        if (link == NULL)
                return NULL;

        link->mutex = new_mutex();
        if (link->mutex == NULL) {
                delete_streamerlink(link);
                return NULL;
        }
        
        link->socket = INVALID_TCP_SOCKET;
        link->addr = new_addr0();
        link->remote_addr = NULL;
        link->userdata = userdata;
        link->ondata = ondata;
        link->autoconnect = autoconnect;
        
        return link;
}

void delete_streamerlink(streamerlink_t *link)
{
        if (link) {
                
                if (link->mutex) {
                        streamerlink_stop_thread(link);
                        
                        streamerlink_lock(link);
                        streamerlink_close_connection(link);
                        if (link->remote_addr) {
                                delete_addr(link->remote_addr);
                                link->remote_addr = NULL;
                        }                        
                        streamerlink_unlock(link);
                        delete_mutex(link->mutex);
                }
                if (link->addr) {
                        delete_addr(link->addr);
                }
                for (list_t *l = link->headers; l != NULL; l = list_next(l)) {
                        http_header_t *h = list_get(l, http_header_t);
                        delete_http_header(h);
                }
                delete_list(link->headers);
                
                r_delete(link);
        }
}

addr_t *streamerlink_addr(streamerlink_t *link)
{
        return link->addr;
}

static int streamerlink_lock(streamerlink_t *link)
{
        mutex_lock(link->mutex);
}

static int streamerlink_unlock(streamerlink_t *link)
{
        mutex_unlock(link->mutex);
}

static int streamerlink_message_begin(http_parser *p)
{
        return 0;
}

static int streamerlink_message_complete(http_parser *p)
{
        return 0;
}

static int streamerlink_on_status(http_parser *p, const char *data, size_t length)
{
        return 0;
}

static int streamerlink_on_body(http_parser *parser, const char *data, size_t length)
{
        streamerlink_t *link = (streamerlink_t *) parser->data;
        return link->ondata(link->userdata, data, length);
}

static int streamerlink_on_header_field(http_parser *parser, const char *data, size_t length)
{
        streamerlink_t *r = (streamerlink_t *) parser->data;
        r->cur_header = new_http_header(data, length);
        if (r->cur_header == NULL) return -1;
        return 0;
}

static int streamerlink_on_header_value(http_parser *parser, const char *data, size_t length)
{
        streamerlink_t *r = (streamerlink_t *) parser->data;
        if (r->cur_header != NULL) {
                http_header_append_value(r->cur_header, data, length);
                r->headers = list_prepend(r->headers, r->cur_header);
                r->cur_header = NULL;
        }
        return 0;
}

http_header_t *streamerlink_get_header(streamerlink_t *listener, const char *name)
{
        list_t * l = listener->headers;
        while (l) {
                http_header_t *h = list_get(l, http_header_t);
                if (rstreq(name, h->name))
                        return h;
                l = list_next(l);
        }
        return NULL;
}

static int streamerlink_on_headers_complete(http_parser *parser)
{
        streamerlink_t *listener = (streamerlink_t *) parser->data;        
        return 0;
}

int streamerlink_disconnect(streamerlink_t *link)
{
        //r_debug("streamerlink_disconnect");
        int err = streamerlink_stop_thread(link);
        if (err != 0)
                return -1;
        
        streamerlink_lock(link);        
        err = streamerlink_close_connection(link);
        streamerlink_unlock(link);

        return err;
}

static int streamerlink_close_connection(streamerlink_t *link)
{
        //r_debug("streamerlink_close_connection");
        if (link->socket != INVALID_TCP_SOCKET) {
                close_tcp_socket(link->socket);
                link->socket = INVALID_TCP_SOCKET;
        }
        if (link->addr) {
                delete_addr(link->addr);
                link->addr = new_addr0();
        }
        return 0;
}

static int streamerlink_stop_thread(streamerlink_t *link)
{
        //r_debug("streamerlink_stop_thread");
        link->cont = 0;
        
        streamerlink_lock(link);
        if (link->thread) {
                streamerlink_unlock(link);
                thread_join(link->thread);
                streamerlink_lock(link);
        }
        streamerlink_unlock(link);
        
        return 0;
}

static int streamerlink_open(streamerlink_t *link)
{
        link->socket = open_tcp_socket(link->remote_addr);
        if (link->socket == INVALID_TCP_SOCKET) {
                r_err("streamerlink_open: failed to connect the socket");
                return -1;
        }
        if (link->addr != NULL)
                delete_addr(link->addr);
        link->addr = tcp_socket_addr(link->socket);
        //link->remote_addr = addr_clone(addr);
        return 0;
}

static int streamerlink_send_request(streamerlink_t *link)
{
        char request[1024];
        int request_length;
        int ret;
        char b[64];
        
        addr_ip(link->remote_addr, b, 64);

        // send the http request
        request_length = snprintf(request, 1024,
                                  "GET /stream.html HTTP/1.1\r\n"
                                  "Host: %s\r\n"
                                  "Connection: close\r\n\r\n",
                                  b);

        ret = tcp_socket_send(link->socket, request, request_length);
        if (ret == -1) {
                r_err("streamerlink_start: failed to send request");
                return - 1;
        }

        return 0;
}

static void streamerlink_run(streamerlink_t *link)
{
        size_t len = 80*1024;
        size_t parsed;
        char buf[len];
        ssize_t received;
        http_parser *parser;
        http_parser_settings settings;
        int ret;

        //r_debug("streamerlink_run: sending request");
        
        ret = streamerlink_send_request(link);
        if (ret != 0) {
                streamerlink_lock(link);        
                streamerlink_close_connection(link);
                delete_thread(link->thread);
                link->thread = NULL;
                streamerlink_unlock(link);
                return;
        }

        //r_debug("streamerlink_run: parsing response");
        
        http_parser_settings_init(&settings);
        settings.on_body = streamerlink_on_body;
        settings.on_status = streamerlink_on_status;        
        settings.on_header_field = streamerlink_on_header_field;
        settings.on_header_value = streamerlink_on_header_value;
        settings.on_message_begin = streamerlink_message_begin;
        settings.on_message_complete = streamerlink_message_complete;
        settings.on_headers_complete = streamerlink_on_headers_complete;
        
        parser = r_new(http_parser);
        if (parser == NULL) {
                r_err("streamerlink_start: out of memory");
                streamerlink_lock(link);        
                streamerlink_close_connection(link);
                delete_thread(link->thread);
                link->thread = NULL;
                streamerlink_unlock(link);
                return;
        }
        http_parser_init(parser, HTTP_RESPONSE);
        parser->data = link;

        link->cont = 1;
        
        // read the http response. The stream is read below. We rely
        // on the parser to handle the response data. 
        while (!app_quit() && link->cont) {
                
                received = tcp_socket_recv(link->socket, buf, len);

                // we got a timeout. This allows us to check whether
                // app_quit has been set, and then try again.
                if (received == -2) {
                        continue;
                }
                
                // an error occured.
                if (received < 0) {
                        r_err("streamerlink: recv failed");
                        break;
                }
        
                /* Start up / continue the parser.
                 * Note we pass received==0 to signal that EOF has been received.
                 */
                parsed = http_parser_execute(parser, &settings, buf, received);
                if (received == 0) {
                        r_err("new_streamerlink: received == 0");
                        break;
                }

                if (parsed != received) {
                        /* Handle error. Usually just close the connection. */
                        r_err("new_streamerlink: parsed != received");
                        break;
                }
        }

        //r_debug("streamerlink_run: ending connection");
        
        // CLEANUP
        r_delete(parser);

        streamerlink_lock(link);        
        delete_thread(link->thread);
        link->thread = NULL;
        streamerlink_unlock(link);
}

int streamerlink_connect(streamerlink_t *link)
{
        int ret = 0;
        char b[64];
        
        //r_debug("streamerlink_connect");

        if (streamerlink_disconnect(link) != 0)
                return -1;
        
        //r_debug("streamerlink_connect @0");
        
        streamerlink_lock(link);

        //r_debug("streamerlink_connect @1");

        if (link->remote_addr == NULL) {
                r_debug("streamerlink_connect: no remote address");
                goto unlock_and_return;
        }

        //r_debug("streamerlink_connect @2: %s", addr_string(link->remote_addr, b, 64));

        ret = streamerlink_open(link);
        if (ret != 0) {
                streamerlink_close_connection(link);
                goto unlock_and_return;
        }

        //r_debug("streamerlink_connect @3");
                
        link->thread = new_thread((thread_run_t) streamerlink_run, link, 0, 0);
        if (link->thread == NULL) {
                streamerlink_close_connection(link);
                ret = -1;
                goto unlock_and_return;
        }

unlock_and_return:
        
        //r_debug("streamerlink_connect @4");
                
        streamerlink_unlock(link);
        return ret;
}

int streamerlink_set_remote(streamerlink_t *link, addr_t *addr)
{
        int ret = -1;

        char b[52];
        r_debug("streamerlink_set_remote: %s", addr_string(addr, b, 52));

        ret = streamerlink_stop_thread(link);
        if (ret != 0)
                return -1;

        streamerlink_lock(link);
        
        ret = streamerlink_close_connection(link);
        if (ret != 0)
                goto unlock_and_return;

        if (link->remote_addr) {
                delete_addr(link->remote_addr);
                link->remote_addr = NULL;
        }
        if (addr != NULL) {
                link->remote_addr = addr_clone(addr);
                if (link->remote_addr == NULL) {
                        ret = -1;
                        goto unlock_and_return;
                }
        }
        
        streamerlink_unlock(link);

        if (link->remote_addr != NULL && link->autoconnect)
                return streamerlink_connect(link);
        else return 0;

        
unlock_and_return:        
        streamerlink_unlock(link);
        return ret;
}
