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
#include "rcom/dump.h"
#include "rcom/util.h"

#include "http_parser.h"
#include "http.h"
#include "export.h"
#include "request_priv.h"
#include "streamer_priv.h"

static void streamer_send_index_html(streamer_t *streamer, tcp_socket_t s);
static void streamer_send_index_json(streamer_t *streamer, tcp_socket_t s);
static int streamer_onclient(streamer_t *streamer, streamer_client_t *client);
static void streamer_add_client(streamer_t *streamer, streamer_client_t *client);
static void streamer_remove_client(streamer_t *streamer, streamer_client_t *client);
static void streamer_delete_clients(streamer_t *streamer);
static const char *streamer_mimetype(streamer_t *streamer);
static int streamer_quit(streamer_t *streamer);

/*
 * streamer_client_t
 * 
 * The streamer_client_t structure is used by the streamer to track
 * all the clients that registered for a specific stream.
 */
typedef struct _streamer_client_t {
        // The TCP socket and address
        tcp_socket_t socket;
        int cont;
        
        // The URI in the HTTP request
        char *uri;
        request_t *request;
        
        // The data transfer passes through a circular buffer.
        circular_buffer_t *buffer;
        // Pointer to the streamer object that created this object.
        streamer_t *streamer;

        void *context;
        streamer_client_delete_context_t del;

        thread_t *thread;
} streamer_client_t;

static void delete_streamer_client(streamer_client_t *client);
static void streamer_client_run(streamer_client_t *client);

static streamer_client_t *new_streamer_client(streamer_t *streamer,
                                              tcp_socket_t socket)
{
        streamer_client_t *client;

        client = r_new(streamer_client_t);
        client->streamer = streamer;
        client->socket = socket;
        client->buffer = new_circular_buffer(1 * 1024 * 1024);
        return client;
}

static int streamer_client_read(streamer_client_t *client, char *buffer, size_t len)
{
        int available;

        available = circular_buffer_data_available(client->buffer);
        while (available == 0) {
                if (streamer_quit(client->streamer))
                        return 0;
                clock_sleep(0.010);
                available = circular_buffer_data_available(client->buffer);
        }

        if ((unsigned)available > len)
                available = len;
        circular_buffer_read(client->buffer, buffer, available);
        
        return available;
}

static void delete_streamer_client(streamer_client_t *client)
{
        if (client) {
                if (client->socket != INVALID_TCP_SOCKET) {
                    r_debug("delete_streamer_client: close_tcp_socket");
                    close_tcp_socket(client->socket);
                }
                delete_circular_buffer(client->buffer);
                delete_request(client->request);
                delete_thread(client->thread);
                if (client->context && client->del)
                        client->del(client);
                r_delete(client);
        }
}

circular_buffer_t *streamer_client_get_buffer(streamer_client_t *client)
{
        return client->buffer;
}

void streamer_client_set_context(streamer_client_t *c, void *context)
{
        c->context = context;
}

void *streamer_client_get_context(streamer_client_t *c)
{
        return c->context;
}

void streamer_client_set_delete_context(streamer_client_t *c,
                                        streamer_client_delete_context_t del)
{
        c->del = del;
}

static int streamer_client_parse_request(streamer_client_t *client)
{

        client->request = new_request();

        int err = request_parse_html(client->request, client->socket, REQUEST_PARSE_ALL);
        if (err != 0) {
                http_send_error_headers(client->socket, HTTP_Status_Internal_Server_Error);
                delete_request(client->request);
                client->request = NULL;
                return -1;
        }

        if (request_uri(client->request) == NULL) {
                http_send_error_headers(client->socket, HTTP_Status_Bad_Request);
                delete_request(client->request);
                client->request = NULL;
                return -1;
        }

        return 0;
}

static void streamer_client_run(streamer_client_t *client)
{
        char buf[16384];
        int received;
        int err = http_send_streaming_headers(client->socket,
                                              streamer_mimetype(client->streamer));
        if (err != 0) {
            r_debug("streamer_client_run: close_tcp_socket");
                close_tcp_socket(client->socket);
                client->socket = INVALID_TCP_SOCKET;        
                streamer_remove_client(client->streamer, client);
                return;
        }
        
        while (!streamer_quit(client->streamer)) {
                
                received = streamer_client_read(client, buf, sizeof(buf));
                
                err = http_send_chunk(client->socket, buf, received);
                if (err == -1)
                        break;
        }

        r_debug("streamer_client_run 2: close_tcp_socket");
        close_tcp_socket(client->socket);
        client->socket = INVALID_TCP_SOCKET;
        streamer_remove_client(client->streamer, client);
        
        r_info("streamer_client_run: thread finished");
}

/*****************************************************/

struct _streamer_t
{
        char *name;
        char *topic;
        addr_t *addr;
        tcp_socket_t socket;
        char *mimetype;
        list_t *exports;

        mutex_t *mutex;

        list_t *clients;
        list_t *finished_clients;
        mutex_t *clients_mutex;
        
        thread_t* server_thread;
        thread_t* data_thread;

        int cont;
        streamer_onclient_t onclient;
        streamer_onbroadcast_t onbroadcast;
        void *userdata;
};

/* static void start_dumper_thread(streamer_t *s, export_t *e); */
/* static void *_streamer_run(void *p); */
static void streamer_run_server(streamer_t *streamer);
static void streamer_run_data(streamer_t *streamer);

streamer_t *new_streamer(const char *name,
                         const char *topic,
                         int port,
                         const char *mimetype,
                         streamer_onclient_t onclient,
                         streamer_onbroadcast_t onbroadcast,
                         void *userdata)
{
        streamer_t *streamer = r_new(streamer_t);

        streamer->name = r_strdup(name);
        streamer->topic = r_strdup(topic);
        streamer->mimetype = r_strdup(mimetype);

        streamer->onclient = onclient;
        streamer->onbroadcast = onbroadcast;
        streamer->userdata = userdata;
        streamer->clients_mutex = new_mutex();
        
        streamer->addr = new_addr(app_ip(), port);
        if (streamer->addr == NULL) {
                r_delete(streamer);
                return NULL;
        }
        
        streamer->socket = open_server_socket(streamer->addr);
        if (streamer->socket == INVALID_TCP_SOCKET) {
                r_err("Failed to create server socket");
                delete_streamer(streamer);
                return NULL;
        }

        char b[52];
        r_info("Streamer listening at http://%s",
                 addr_string(streamer->addr, b, sizeof(b)));

        streamer->cont = 1;        

        streamer->server_thread = new_thread((thread_run_t) streamer_run_server,
                                             (void*) streamer);
        if (streamer->server_thread == NULL) {
                r_err("Failed to start the server thread");
                delete_streamer(streamer);
                return NULL;
        }

        if (streamer->onbroadcast) {
                streamer->data_thread = new_thread((thread_run_t) streamer_run_data,
                                                   (void*) streamer);
                if (streamer->data_thread == NULL) {
                        r_err("Failed to start the server thread");
                        delete_streamer(streamer);
                        return NULL;
                }
        }
        
        return streamer;
}

void delete_streamer(streamer_t *streamer)
{
        //ToDo: commented out to remove warning until code below fixed. (Issue #11 Github)
        // list_t *l;

        if (streamer) {
                streamer->cont = 0;
                
                if (streamer->server_thread) {
                        //r_debug("delete_streamer: joining server thread");
                        thread_join(streamer->server_thread);
                        delete_thread(streamer->server_thread);
                }
                
                if (streamer->data_thread) {
                        //r_debug("delete_streamer: joining data thread");
                        thread_join(streamer->data_thread);
                        delete_thread(streamer->data_thread);
                }
                
                //r_debug("delete_streamer: deleting clients");
                
                // Delete clients
                if (streamer->clients_mutex && streamer->clients) {
                        
                        streamer_lock_clients(streamer);
                        list_t *l = streamer->clients;
                        streamer->clients = NULL;
                        streamer_unlock_clients(streamer);

                        while (l) {
                                streamer_client_t *c = list_get(l, streamer_client_t); 
                                thread_join(c->thread);
                                l = list_next(l);
                        }
                }
                streamer_delete_clients(streamer);
                delete_mutex(streamer->clients_mutex);
                
                r_free(streamer->topic);
                r_free(streamer->name);
                r_free(streamer->mimetype);
                delete_addr(streamer->addr);
                if (streamer->socket != INVALID_TCP_SOCKET) {
                    r_debug("delete_streamer: close_tcp_socket");
                    close_tcp_socket(streamer->socket);
                }

                r_delete(streamer);                
        }
}

addr_t *streamer_addr(streamer_t *s)
{
        return s->addr;
}

static void streamer_start_stream(streamer_t *streamer, streamer_client_t *client)
{
        if (streamer_onclient(streamer, client) != 0) {
                http_send_error_headers(client->socket,
                                        HTTP_Status_Internal_Server_Error);
                return;
        }
        
        streamer_add_client(streamer, client);
                
        r_info("streamer_start_stream: spinning off new thread to handle client");
        client->thread = new_thread((thread_run_t) streamer_client_run, client);
}

static void streamer_run_server(streamer_t *streamer)
{
        tcp_socket_t socket;

        r_info("Thread 'streamer_run_server' starting");
        
        while (!streamer_quit(streamer)) {
                //r_info("streamer_run_server: waiting for new connection");
                
                socket = server_socket_accept(streamer->socket);
                if (socket == TCP_SOCKET_TIMEOUT) {
                        continue;
                } else if (socket == INVALID_TCP_SOCKET) {
                        // Server socket is probably being closed // FIXME: is this true?
                        r_err("streamer_run_server: accept failed");
                        continue;
                }

                // Check whether finished client need cleaning up.
                streamer_delete_clients(streamer);

                //r_info("streamer_run_server: got connection");
                streamer_client_t *client = new_streamer_client(streamer, socket);

                if (streamer_client_parse_request(client) != 0) {
                        r_debug("streamer_run_server: close_tcp_socket");
                        close_tcp_socket(client->socket);
                        client->socket = INVALID_TCP_SOCKET;
                        continue;
                }
        
                const char *uri = request_uri(client->request);

                if (rstreq(uri, "/") || rstreq(uri, "/index.html")) {
                        streamer_send_index_html(streamer, socket);
                        delete_streamer_client(client);
                        
                } else if (rstreq(uri, "/index.json")) {
                        streamer_send_index_json(streamer, socket);
                        delete_streamer_client(client);
                        
                } else if (rstreq(uri, "/stream.html")) {
                        streamer_start_stream(streamer, client);
                
                } else {
                        http_send_error_headers(socket, HTTP_Status_Bad_Request);
                        delete_streamer_client(client);
                }
        }

        r_info("Thread 'streamer_run_server' finished");
}

static void streamer_run_data(streamer_t *streamer)
{
        r_info("Thread 'streamer_run_data' starting");
        
        while (!streamer_quit(streamer))
                streamer->onbroadcast(streamer->userdata, streamer);

        r_info("Thread 'streamer_run_data' finished");
}

static list_t *streamer_get_clients(streamer_t *s)
{
        return s->clients;
}

void streamer_lock_clients(streamer_t *streamer)
{
        mutex_lock(streamer->clients_mutex);
}

void streamer_unlock_clients(streamer_t *streamer)
{
        mutex_unlock(streamer->clients_mutex);
}

int streamer_count_clients(streamer_t *s)
{
        return list_size(s->clients);
}

streamer_client_t *streamer_get_client(streamer_t *s, int n)
{
        list_t *l = list_nth(s->clients, n);
        return (l)? list_get(l, streamer_client_t) : NULL;
}

static int streamer_onclient(streamer_t *streamer, streamer_client_t *client)
{
        if (streamer->onclient) {
                int ret = streamer->onclient(streamer->userdata,
                                             client,
                                             /* export_name(client->exp), */
                                             streamer);
                if (ret != 0) {
                        r_info("streamer_onclient: callback returned an error");
                        return -1;
                }
        }
        return 0;
}

static void streamer_add_client(streamer_t *streamer, streamer_client_t *client)
{
        streamer_lock_clients(streamer);
        streamer->clients = list_append(streamer->clients, client);
        streamer_unlock_clients(streamer);
}

static void streamer_remove_client(streamer_t *streamer, streamer_client_t *client)
{
        streamer_lock_clients(streamer);
        streamer->clients = list_remove(streamer->clients, client);
        streamer->finished_clients = list_prepend(streamer->finished_clients, client);
        streamer_unlock_clients(streamer);
}

static void streamer_delete_clients(streamer_t *streamer)
{
        streamer_lock_clients(streamer);
        list_t *l = streamer->finished_clients;
        while (l) {
                streamer_client_t *r = list_get(l, streamer_client_t);
                delete_streamer_client(r);
                l = list_next(l);
        }
        delete_list(streamer->finished_clients);
        streamer->finished_clients = NULL;
        streamer_unlock_clients(streamer);
}

int streamer_has_clients(streamer_t* streamer)
{
        return streamer->clients != NULL;
}

int streamer_send_multipart(streamer_t* s,
                            /* const char *exp, */
                            const char *data, int length,
                            const char *mimetype,
                            double time)
{
        char header[512];
        int header_len;
        int total_len;
        list_t *l;
        streamer_client_t *b;
        circular_buffer_t *c;

        header_len = snprintf(header, sizeof(header),
                              "--nextimage\r\n"
                              "Content-Type: %s\r\n"
                              "Content-Length: %d\r\n"
                              "X-LT-Timestamp: %f\r\n"
                              "\r\n",
                              mimetype,
                              length,
                              time);

        total_len = header_len + length;
        
        streamer_lock_clients(s);
        l = streamer_get_clients(s);
        while (l) {
                b = list_get(l, streamer_client_t);
                if (0) {
                        c = streamer_client_get_buffer(b);
                        r_debug("datalen %d, buffer space %d, buffer size %d",
                                total_len, circular_buffer_space_available(c),
                                circular_buffer_size(c));
                }
                if (((c = streamer_client_get_buffer(b)) != NULL) 
                    && (circular_buffer_space_available(c) > total_len)) {
                        circular_buffer_write(c, header, header_len);
                        circular_buffer_write(c, data, length);
                }
                l = list_next(l);
        }
        streamer_unlock_clients(s);
        
        return 0;        
}

static void streamer_send_index_html(streamer_t *streamer, tcp_socket_t s)
{
        char b[52];
        membuf_t *membuf = new_membuf();

        //r_debug("streamer_send_index_html");

        if (membuf == NULL) {
                http_send_error_headers(s, 500);
                return;
        }
        
        membuf_printf(membuf, 
                      "<!DOCTYPE html>\n"
                      "<html lang=\"en\">\n"
                      "  <head>\n"
                      "    <meta charset=\"utf-8\">\n"
                      "    <title>%s</title>\n"
                      "  </head>\n"
                      "  <body>\n",
                      streamer->name);

        addr_string(streamer->addr, b, 52);

        membuf_printf(membuf, 
                      "    <a href=\"http://%s/\">%s:%s</a><br>\n",
                      b, streamer->name, streamer->topic);
        
        membuf_printf(membuf, "  </body>\n</html>\n");

        int err = http_send_headers(s, 200, "text/html", membuf_len(membuf));
        if (err != 0) goto cleanup;
        
        err = tcp_socket_send(s, membuf_data(membuf), membuf_len(membuf));
        if (err == -1) goto cleanup;

cleanup:
        delete_membuf(membuf);
        r_info("streamer_send_index_html: thread finished");
}

static void streamer_send_index_json(streamer_t *streamer, tcp_socket_t s)
{
        char b[52];
        membuf_t *membuf = new_membuf();

        //r_debug("streamer_send_index_json");

        if (membuf == NULL) {
                http_send_error_headers(s, 500);
                return;
        }
        
        addr_string(streamer->addr, b, 52);
        
        membuf_printf(membuf, "{\"exports\": [");

        membuf_printf(membuf, 
                      "{\"name\": \"%s\", \"topic\": \"%s\", \"uri\": \"http://%s/\"}",
                      streamer->name, streamer->topic, b);

        membuf_printf(membuf, "]}");

        int err = http_send_headers(s, 200, "application/json", membuf_len(membuf));
        if (err != 0) goto cleanup;
        
        err = tcp_socket_send(s, membuf_data(membuf), membuf_len(membuf));
        if (err == -1) goto cleanup;

cleanup:
        delete_membuf(membuf);
        r_info("streamer_send_index: thread finished");
}

static const char *streamer_mimetype(streamer_t *streamer)
{
        return streamer->mimetype;
}

static int streamer_quit(streamer_t *streamer)
{
        return app_quit() || streamer->cont == 0;
}

/* static void *_dumper_thread_run(void *data) */
/* { */
/*         streamer_client_t *client = (streamer_client_t*) data;         */
        
/*         FILE *fp = create_dump(streamer_name(client->streamer), */
/*                                export_name(client->exp), */
/*                                export_mimetype_out(client->exp), */
/*                                EXPORT_STREAMER); */
/*         if (fp == NULL)  */
/*                 pthread_exit(NULL); */
        
/*         while (!app_quit()) { */
/*                 char buffer[2048]; */
/*                 int n = streamer_client_read(client, buffer, 2048); */
/*                 if (n == 0) break; */
/*                 int err = dump_buffer(fp, buffer, n); */
/*                 if (err != 0) break; */
/*         } */
        
/*         close_dump(fp); */
/*         streamer_remove_client(client->streamer, client); */
/*         delete_streamer_client(client); */

/*         pthread_exit(NULL); */
/* } */

/* static void start_dumper_thread(streamer_t *s, export_t *e) */
/* { */
/*         pthread_t thread; */
/*         streamer_client_t *client;         */
/*         struct sockaddr_in dummy; */
        
/*         r_info("Starting dumper thread"); */
/*         client = new_streamer_client(s, -1, dummy); */
/*         client->exp = e; */
/*         streamer_add_client(s, client); */
/*         pthread_create(&thread, NULL, _dumper_thread_run, client); */
/* } */

