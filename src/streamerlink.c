#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "rcom/log.h"
#include "rcom/app.h"
#include "rcom/registry.h"

#include "mem.h"
#include "membuf.h"
#include "util.h"
#include "list.h"
#include "http.h"
#include "thread.h"
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
} streamerlink_t;


static int streamerlink_disconnect(streamerlink_t *link, int join);
static int streamerlink_lock(streamerlink_t *link);
static int streamerlink_unlock(streamerlink_t *link);


streamerlink_t *new_streamerlink(const char *resource,
                                 streamerlink_ondata_t ondata,
                                 void* userdata)
{
        streamerlink_t* link;

        if (resource == NULL || ondata == NULL) {
                log_err("new_streamerlink: invalid arguments");
                return NULL;
        }
        
        link = new_obj(streamerlink_t);
        if (link == NULL)
                return NULL;

        link->resource = mem_strdup(resource);
        if (link->resource == NULL) {
                delete_streamerlink(link);
                return NULL;
        }

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
        
        return link;
}

void delete_streamerlink(streamerlink_t *link)
{
        if (link) {
                
                if (link->mutex) {
                        streamerlink_lock(link);
                        streamerlink_disconnect(link, 1);
                        streamerlink_unlock(link);
                        delete_mutex(link->mutex);
                }
                
                if (link->resource)
                        mem_free(link->resource);
                
                delete_obj(link);
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
                if (streq(name, h->name))
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

static int streamerlink_disconnect(streamerlink_t *link, int join)
{
        log_debug("streamerlink_disconnect");
        
        link->cont = 0;
        
        if (join && link->thread) {
                thread_join(link->thread);
                delete_thread(link->thread);
                link->thread = NULL;
        }
        if (link->socket != INVALID_TCP_SOCKET) {
                close_tcp_socket(link->socket);
                link->socket = INVALID_TCP_SOCKET;
        }
        if (link->addr) {
                delete_addr(link->addr);
                link->addr = new_addr0();
        }
        if (link->remote_addr) {
                delete_addr(link->remote_addr);
                link->remote_addr = NULL;
        }
                
        return 0;
}

static int streamerlink_open(streamerlink_t *link, addr_t *addr)
{
        link->socket = open_tcp_socket(addr);
        if (link->socket == INVALID_TCP_SOCKET) {
                log_err("streamerlink_open: failed to connect the socket");
                return -1;
        }
        if (link->addr != NULL)
                delete_addr(link->addr);
        link->addr = tcp_socket_addr(link->socket);
        link->remote_addr = addr_clone(addr);
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
        if (link->resource[0] == '/') {
                request_length = snprintf(request, 1024,
                                          "GET %s HTTP/1.1\r\n"
                                          "Host: %s\r\n"
                                          "Connection: close\r\n\r\n",
                                          link->resource, b);
        } else {
                request_length = snprintf(request, 1024,
                                          "GET /%s HTTP/1.1\r\n"
                                          "Host: %s\r\n"
                                          "Connection: close\r\n\r\n",
                                          link->resource, b);
        }

        ret = tcp_socket_send(link->socket, request, request_length);
        if (ret == -1) {
                log_err("streamerlink_start: failed to send request");
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

        log_debug("streamerlink_run: sending request");
        
        ret = streamerlink_send_request(link);
        if (ret != 0) {
                streamerlink_lock(link);        
                streamerlink_disconnect(link, 0);
                delete_thread(link->thread);
                link->thread = NULL;
                streamerlink_unlock(link);
                return;
        }

        log_debug("streamerlink_run: parsing response");
        
        http_parser_settings_init(&settings);
        settings.on_body = streamerlink_on_body;
        settings.on_status = streamerlink_on_status;        
        settings.on_header_field = streamerlink_on_header_field;
        settings.on_header_value = streamerlink_on_header_value;
        settings.on_message_begin = streamerlink_message_begin;
        settings.on_message_complete = streamerlink_message_complete;
        settings.on_headers_complete = streamerlink_on_headers_complete;
        
        parser = new_obj(http_parser);
        if (parser == NULL) {
                log_err("streamerlink_start: out of memory");
                streamerlink_lock(link);        
                streamerlink_disconnect(link, 0);
                streamerlink_unlock(link);
                delete_thread(link->thread);
                link->thread = NULL;
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
                if (received == -1 && errno == EAGAIN) {
                        continue;
                }
                
                // an error occured.
                if (received < 0) {
                        log_err("streamerlink: recv failed");
                        break;
                }
        
                /* Start up / continue the parser.
                 * Note we pass received==0 to signal that EOF has been received.
                 */
                parsed = http_parser_execute(parser, &settings, buf, received);
                if (received == 0) {
                        log_err("new_streamerlink: received == 0");
                        break;
                }

                if (parsed != received) {
                        /* Handle error. Usually just close the connection. */
                        log_err("new_streamerlink: parsed != received");
                        break;
                }
        }

        log_debug("streamerlink_run: ending connection");
        
        // CLEANUP
        delete_obj(parser);

        streamerlink_lock(link);        
        streamerlink_disconnect(link, 0);
        delete_thread(link->thread);
        link->thread = NULL;
        streamerlink_unlock(link);
}

int streamerlink_connect(streamerlink_t *link, addr_t *addr)
{
        int ret = 0;

        char b[64];
        log_debug("streamerlink_connect: %s", addr_string(addr, b, 64));
        
        streamerlink_lock(link);
        
        ret = streamerlink_disconnect(link, 1);
        if (ret != 0)
                goto unlock_and_return;

        ret = streamerlink_open(link, addr);
        if (ret != 0) {
                streamerlink_disconnect(link, 0);
                goto unlock_and_return;
        }

        link->thread = new_thread((thread_run_t) streamerlink_run, link, 0);
        if (link->thread == NULL) {
                streamerlink_disconnect(link, 0);
                ret = -1;
                goto unlock_and_return;
        }

        streamerlink_unlock(link);

unlock_and_return:
        
        return ret;
}
