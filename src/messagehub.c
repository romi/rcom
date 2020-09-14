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

#include <r.h>
#include "rcom/app.h"
#include "rcom/messagelink.h"

#include "util_priv.h"
#include "net.h"
#include "http.h"
#include "messagelink_priv.h"
#include "messagehub_priv.h"
#include "request_priv.h"

struct _messagehub_t {
        char *name;
        char *topic;
        thread_t *thread;
        addr_t *addr;
        tcp_socket_t socket;

        list_t *links;
        list_t *closed_links;
        mutex_t *links_mutex;

        membuf_t *mem;
        int quit;
        
        messagehub_onconnect_t onconnect;
        messagehub_onrequest_t onrequest;
        void *userdata;
};

static void messagehub_add_link(messagehub_t *hub, messagelink_t *link);
static void messagehub_lock_links(messagehub_t* hub);
static void messagehub_unlock_links(messagehub_t* hub);
static void messagehub_run(messagehub_t* hub);
static void messagehub_delete_closed_links(messagehub_t *hub);

messagehub_t* new_messagehub(const char *name,
                             const char *topic,
                             int port,
                             messagehub_onconnect_t onconnect,
                             void *userdata)
{
        messagehub_t* hub = r_new(messagehub_t);
        if (hub == NULL)
                return NULL;

        hub->name = r_strdup(name);
        hub->topic = r_strdup(topic);
        hub->onconnect = onconnect;
        hub->onrequest = NULL;
        hub->userdata = userdata;
        hub->mem = NULL;
        hub->links = NULL;        
        hub->closed_links = NULL;        
        hub->links_mutex = new_mutex();
        hub->quit = 0;

        hub->addr = new_addr(app_ip(), port);
        if (hub->addr == NULL) {
                delete_messagehub(hub);
                return NULL;
        }
        
        hub->socket = open_server_socket(hub->addr);
        if (hub->socket == INVALID_TCP_SOCKET) {
                delete_messagehub(hub);
                return NULL;
        }

        hub->thread = new_thread((thread_run_t) messagehub_run, hub);
        if (hub->thread == NULL) {
                delete_messagehub(hub);
                return NULL;
        }

        char b[64];
        r_info("Messagehub listening at http://%s:%d",
                 addr_ip(hub->addr, b, 64), addr_port(hub->addr));
        
        return hub;
}

void delete_messagehub(messagehub_t*hub)
{
        r_debug("delete_messagehub");
        
        if (hub) {
                hub->quit = 1;
                if (hub->thread) {
                        thread_join(hub->thread);
                        delete_thread(hub->thread);
                }
                
                delete_membuf(hub->mem);
                
                if (hub->socket != INVALID_TCP_SOCKET) {
                        r_debug("delete_messagehub: close_tcp_socket");
                        close_tcp_socket(hub->socket);
                        hub->socket = INVALID_TCP_SOCKET;
                }
                
                if (hub->links_mutex) {

                        messagehub_lock_links(hub);
                        list_t *l = hub->links;
                        hub->links = NULL;
                        messagehub_unlock_links(hub);                
                        
                        // stop the active links
                        while (l) {
                                messagelink_t *link = list_get(l, messagelink_t);
                                messagelink_stop_thread(link);
                                l = list_next(l);
                        }
                        delete_list(l);

                        // delete all inactive links
                        messagehub_delete_closed_links(hub);                        
                        delete_mutex(hub->links_mutex);
                }

                delete_addr(hub->addr);
                r_free(hub->name);
                r_free(hub->topic);
                
                r_delete(hub);
        }

        //r_debug("delete_messagehub: done");
}

addr_t *messagehub_addr(messagehub_t *hub)
{
        return hub->addr;
}

const char *messagehub_name(messagehub_t *hub)
{
        return hub->name;
}

const char *messagehub_topic(messagehub_t *hub)
{
        return hub->topic;
}

int messagehub_set_onrequest(messagehub_t *hub, messagehub_onrequest_t onrequest)
{
        hub->onrequest = onrequest;
        return 0;
}

// ToDo: Why does this not use messagehub when it's called messagehub_XXX?
static int messagehub_upgrade_connection(messagehub_t *hub __attribute__((unused)),
                                         request_t *request,
                                         tcp_socket_t link_socket)
{
        const char *key = request_get_header_value(request, "Sec-WebSocket-Key");
        unsigned char buffer[100];
        unsigned char digest[21];
        
        snprintf((char*)buffer, 100, "%s%s", key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
        buffer[99] = 0;

        memset(digest, 0, sizeof(digest));
        SHA1(buffer, digest);
        
        char *accept = encode_base64(digest, 20);
        if (accept == NULL) 
                return -1;

        membuf_t *headers = new_membuf();
        if (headers == NULL) {
                r_free(accept);
                return -1;
        }
        
        membuf_printf(headers,
                      "HTTP/1.1 101 Switching Protocols\r\n"
                      "Upgrade: websocket\r\n"
                      "Connection: Upgrade\r\n"
                      "Sec-WebSocket-Accept: %s\r\n"
                      "\r\n", accept);

        int err = tcp_socket_send(link_socket, membuf_data(headers), membuf_len(headers));
        
        r_free(accept);
        delete_membuf(headers);

        return err;
}

static void messagehub_handle_websocket(messagehub_t* hub,
                                        request_t *request,
                                        tcp_socket_t link_socket)
{
        if (messagehub_upgrade_connection(hub, request, link_socket) != 0) {
                http_send_error_headers(link_socket, HTTP_Status_Internal_Server_Error);
                r_debug("messagehub_handle_websocket: close_tcp_socket");
                close_tcp_socket(link_socket);
                delete_request(request);
                return;
        }
        
        messagelink_t *link = server_messagelink_connect(hub, link_socket);
        if (link == NULL) {
                delete_request(request);
                return;
        }
        
        if (hub->onconnect)
                if (hub->onconnect(hub->userdata, hub, request, link) != 0) {
                        r_debug("messagehub_handle_websocket (%s:%s): "
                                "onconnect return non-zero",
                                hub->name, hub->topic);
                        delete_messagelink(link);
                        delete_request(request);
                        return;
                }
        

        delete_request(request);
        messagehub_add_link(hub, link);

        // Do some cleanup.
        messagehub_delete_closed_links(hub);
        
        server_messagelink_read_in_background(link);
}

struct request_and_socket_t {
        messagehub_t* hub;
        request_t* request;
        tcp_socket_t socket;
};

struct request_and_socket_t *new_request_and_socket(messagehub_t* hub,
                                                    request_t* request,
                                                    tcp_socket_t socket)
{
        struct request_and_socket_t *s;
        s = r_new(struct request_and_socket_t);
        if (s == NULL)
                return NULL;
        s->hub = hub;
        s->request = request;
        s->socket = socket;
        return s;
}

static void messagehub_handle_request(messagehub_t* hub,
                                      request_t *request,
                                      tcp_socket_t link_socket)
{
        if (hub->onrequest == NULL) {
                http_send_error_headers(link_socket, HTTP_Status_Not_Found);

        } else {
                response_t *response = new_response(HTTP_Status_OK);
                hub->onrequest(hub->userdata, request, response);
                response_send(response, link_socket);
                delete_response(response);
        }
        
        delete_request(request);
        r_debug("messagehub_handle_request: close_tcp_socket");
        close_tcp_socket(link_socket);
}

static void messagehub_handle(messagehub_t *hub, tcp_socket_t link_socket)
{
        request_t *request = new_request();
        if (request == NULL) {
                http_send_error_headers(link_socket, HTTP_Status_Internal_Server_Error);
                r_err("messagehub_handle: request close_tcp_socket");
                close_tcp_socket(link_socket);
                return;
        }

        int err = request_parse_html(request, link_socket, REQUEST_PARSE_HEADERS);
        if (err != 0) {
                http_send_error_headers(link_socket, HTTP_Status_Internal_Server_Error);
                r_err("messagehub_handle: request parse close_tcp_socket");
                close_tcp_socket(link_socket);
                delete_request(request);
                return;
        }

        if (request_is_websocket(request))
                messagehub_handle_websocket(hub, request, link_socket);
        else
                messagehub_handle_request(hub, request, link_socket);        
}

static void messagehub_wait_connection(messagehub_t *hub)
{
        tcp_socket_t link_socket;

        //r_debug("messagehub_wait_connection");
        
        link_socket = server_socket_accept(hub->socket);
        if (link_socket == INVALID_TCP_SOCKET
            || link_socket == TCP_SOCKET_TIMEOUT)
                return;

        messagehub_handle(hub, link_socket);
}

static void messagehub_run(messagehub_t *hub)
{
        while (!hub->quit && !app_quit() && hub->socket != -1) {
                messagehub_wait_connection(hub);
        }
        r_info("messagehub_run: no longer accepting connections");
}

static void messagehub_lock_links(messagehub_t *hub)
{
        mutex_lock(hub->links_mutex);
}

static void messagehub_unlock_links(messagehub_t *hub)
{
        mutex_unlock(hub->links_mutex);
}

static void messagehub_add_link(messagehub_t *hub, messagelink_t *link)
{
        messagehub_lock_links(hub);
        hub->links = list_append(hub->links, link);
        messagehub_unlock_links(hub);
}

void messagehub_remove_link(messagehub_t *hub, messagelink_t *link)
{
        r_debug("messagehub_remove_link (%s:%s)", hub->name, hub->topic);
        messagehub_lock_links(hub);
        hub->links = list_remove(hub->links, link);
        hub->closed_links = list_prepend(hub->closed_links, link);
        messagehub_unlock_links(hub);
}

// Should be called with the links mutex locked.
static void messagehub_delete_closed_links(messagehub_t *hub)
{
        int count = 0;
        r_debug("messagehub_delete_closed_links (%s:%s)", hub->name, hub->topic);
        messagehub_lock_links(hub);
        for (list_t *l = hub->closed_links; l != NULL; l = list_next(l)) {
                messagelink_t *link = list_get(l, messagelink_t);
                delete_messagelink(link);
                count++;
        }
        delete_list(hub->closed_links);
        hub->closed_links = NULL;
        messagehub_unlock_links(hub);
        r_debug("messagehub_delete_closed_links (%s:%s): deleted %d",
                hub->name, hub->topic, count);
}

static int messagehub_membuf(messagehub_t *hub)
{
        if (hub->mem == NULL) {
                hub->mem = new_membuf();
                if (hub->mem == NULL)
                        return -1;
        }
        return 0;
}

int messagehub_broadcast_num(messagehub_t *hub, messagelink_t *exclude, double value)
{
        int err;
        if (messagehub_membuf(hub) != 0)
                return -1;
        membuf_lock(hub->mem);
        membuf_clear(hub->mem);
        err = membuf_printf(hub->mem, "%f", value);
        if (err == 0)
                err = messagehub_broadcast_text(hub, exclude, membuf_data(hub->mem), membuf_len(hub->mem));
        membuf_unlock(hub->mem);
        return err;
}

int messagehub_broadcast_str(messagehub_t *hub, messagelink_t *exclude, const char* value)
{
        int err;
        if (messagehub_membuf(hub) != 0)
                return -1;        
        membuf_t *t = escape_string(value);

        membuf_lock(hub->mem);
        membuf_clear(hub->mem);
        err = membuf_printf(hub->mem, "\"%s\"", membuf_data(t));
        if (err == 0)
                err = messagehub_broadcast_text(hub, exclude,
                                                membuf_data(hub->mem),
                                                membuf_len(hub->mem));
        membuf_unlock(hub->mem);
        
        delete_membuf(t);
        return err;
}

static int32 messagehub_serialise(messagehub_t *hub, const char* s, int32 len)
{
        membuf_append(hub->mem, s, len);
        return 0;
}

int messagehub_broadcast_obj(messagehub_t *hub, messagelink_t *exclude, json_object_t value)
{
        int err;
        if (messagehub_membuf(hub) != 0)
                return -1;
        membuf_lock(hub->mem);
        membuf_clear(hub->mem);
        err = json_serialise(value, 0, (json_writer_t) messagehub_serialise, hub);
        if (err == 0)
                err = messagehub_broadcast_text(hub, exclude, membuf_data(hub->mem), membuf_len(hub->mem));
        membuf_unlock(hub->mem);
        return err;
}

int messagehub_broadcast_f(messagehub_t *hub, messagelink_t *exclude, const char *format, ...)
{
        int err;
        va_list ap;
        
        if (messagehub_membuf(hub) != 0)
                return -1;

        membuf_lock(hub->mem);
        membuf_clear(hub->mem);

        va_start(ap, format);
        err = membuf_vprintf(hub->mem, format, ap);
        va_end(ap);

        if (err == 0) 
                err = messagehub_broadcast_text(hub, exclude, membuf_data(hub->mem),
                                                membuf_len(hub->mem));
        else if (err < 0) {
            r_err("messagehub_broadcast_f: membuf_vprintf returned an error");
        }
        
        membuf_unlock(hub->mem);
        
        return err;
}

//int messagehub_broadcast_f(messagehub_t *hub, messagelink_t *exclude, const char *format, ...)
//{
//    int err;
//    va_list ap;
//    int len;
//
//    if (messagehub_membuf(hub) != 0)
//        return -1;
//
//    va_start(ap, format);
//    len = vsnprintf(NULL, 0, format, ap);
//    va_end(ap);
//
//    membuf_lock(hub->mem);
//    membuf_clear(hub->mem);
//
//    err = membuf_assure(hub->mem, len+1);
//
//    if (err == 0) {
//        va_start(ap, format);
//        err = membuf_vprintf(hub->mem, format, ap);
//        va_end(ap);
//    }
//
//    if (err == 0)
//        err = messagehub_broadcast_text(hub, exclude, membuf_data(hub->mem),
//                                        membuf_len(hub->mem));
//
//    membuf_unlock(hub->mem);
//
//    return err;
//}

int messagehub_broadcast_text(messagehub_t *hub, messagelink_t *exclude,
                              const char *data, int len)
{
        int err = 0;
        
        if (messagehub_membuf(hub) != 0)
                return -1;
        
        messagehub_lock_links(hub);
        
        list_t *l = hub->links;
        while (l) {
                messagelink_t *link = list_get(l, messagelink_t);
                if (link != exclude) {
                        if (messagelink_send_text(link, data, len) != 0)
                                err = -1;
                }
                l = list_next(l);
        }
        
        messagehub_unlock_links(hub);
        
        return err;
}

int messagehub_send_ping(messagehub_t *hub, const char *data, int len)
{
        int err = 0;
        
        messagehub_lock_links(hub);
        list_t *l = hub->links;
        while (l) {
                messagelink_t *link = list_get(l, messagelink_t);
                if (messagelink_send_ping(link, data, len) != 0)
                        err = -1;
                l = list_next(l);
        }
        messagehub_unlock_links(hub);
        return err;
}


