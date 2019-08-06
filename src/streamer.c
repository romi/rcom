#include "rcom/log.h"
#include "rcom/app.h"
#include "rcom/dump.h"
#include "rcom/thread.h"
#include "rcom/util.h"
#include "rcom/membuf.h"
#include "rcom/clock.h"

#include "http_parser.h"
#include "http.h"
#include "list.h"
#include "mem.h"
#include "export.h"
#include "streamer_priv.h"

static void streamer_send_index_html(streamer_t *streamer, tcp_socket_t s);
static void streamer_send_index_json(streamer_t *streamer, tcp_socket_t s);
static int streamer_onclient(streamer_t *streamer, streamer_client_t *client);
static void streamer_add_client(streamer_t *streamer, streamer_client_t *client);
static void streamer_remove_client(streamer_t *streamer, streamer_client_t *client);
static const char *streamer_mimetype(streamer_t *streamer);
/* static export_t *streamer_get_export(streamer_t *s, const char *url); */


/*****************************************************/
/* streamer_client_t
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
        
        // The data transfer passes through a circular buffer.
        circular_buffer_t *buffer;
        // Pointer to the streamer object that created this object.
        streamer_t *streamer;

        void *context;
        streamer_client_delete_context_t del;
} streamer_client_t;

static void delete_streamer_client(streamer_client_t *client);
static void streamer_client_handle_request(streamer_client_t *client);

static streamer_client_t *new_streamer_client(streamer_t *streamer,
                                              tcp_socket_t socket)
{
        streamer_client_t *client;
        circular_buffer_t *c;

        client = new_obj(streamer_client_t);
        if (client == NULL)
                return NULL;
        
        client->streamer = streamer;
        client->socket = socket;
        client->uri = NULL;
        
        client->buffer = new_circular_buffer(1 * 1024 * 1024);
        if (client->buffer == NULL) {
                delete_streamer_client(client);
                return NULL;
        }
        return client;
}

static void streamer_client_set_uri(streamer_client_t *client, char *uri)
{
        log_info("streamer_client: uri=%s", uri);
        client->uri = uri;
}

static int streamer_client_read(streamer_client_t *client, char *buffer, size_t len)
{
        int available;

        if (app_quit())
                return 0;

        available = circular_buffer_available(client->buffer);
        while (available == 0) {
                if (app_quit())
                        return 0;
                clock_sleep(0.010);
                available = circular_buffer_available(client->buffer);
        }

        if (available > len)
                available = len;
        circular_buffer_read(client->buffer, buffer, available);
        
        return available;
}

static void delete_streamer_client(streamer_client_t *client)
{
        if (client) {
                if (client->socket != INVALID_TCP_SOCKET)
                        close_tcp_socket(client->socket);
                if (client->uri)
                        mem_free(client->uri);
                if (client->buffer)
                        delete_circular_buffer(client->buffer);
                if (client->context && client->del)
                        client->del(client);
                delete_obj(client);
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

static int streamer_client_message_complete(http_parser *parser)
{
        streamer_client_t *client = (streamer_client_t *) parser->data;
        client->cont = 0;
        return 0;
}

static int streamer_client_parse_url(http_parser *parser, const char *data, size_t length)
{
        char *s = mem_alloc(length+1);
        memcpy(s, data, length);
        s[length] = 0;

        //log_debug("streamer_client_parse_url: uri=%.*s, len=%d", length, data, length);
        
        streamer_client_t *c = (streamer_client_t *) parser->data;
        streamer_client_set_uri(c, s);

        return 0;
}

static int streamer_client_parse_request(streamer_client_t *client)
{
        http_parser *parser;
        http_parser_settings settings;
        size_t len = 80*1024;
        char buf[len];
        ssize_t received;
        size_t parsed;
        
        http_parser_settings_init(&settings);
        settings.on_url = streamer_client_parse_url;
        settings.on_message_complete = streamer_client_message_complete;
        
        parser = new_obj(http_parser);
        if (parser == NULL) {
                log_err("streamer_client_parse_request: out of memory");
                return -1;
        }
        http_parser_init(parser, HTTP_REQUEST);
        parser->data = client;

        client->cont = 1;
        while (client->cont) {
                received = recv(client->socket, buf, len, 0);
                if (received < 0) {
                        log_err("streamer_client_parse_request: recv failed");
                        delete_obj(parser);
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
                        log_err("streamer_client_parse_request: parsed != received (%d != %d)", parsed, received);
                        //log_err("data: '%s'", buf);
                        delete_obj(parser);
                        return -1;
                }
        }

        delete_obj(parser);

        return 0;
}

static int streamer_client_run(streamer_client_t *client)
{
        //log_debug("streamer_client_run");
        
        int err = http_send_streaming_headers(client->socket,
                                              streamer_mimetype(client->streamer));
        if (err != 0)
                return -1;
        
        while (!app_quit()) {
                char buf[16384];
                int received = streamer_client_read(client, buf, sizeof(buf));
                err = http_send_chunk(client->socket, buf, received);
                if (err == -1) return -1;
        }

        return 0;
}

static void streamer_client_handle_request(streamer_client_t *client)
{
        //log_info("streamer_client: new thread started");
        //log_debug("streamer_client_handle_request 1");
        if (streamer_client_parse_request(client) != 0)
                return;
        
        //log_info("streamer_client: uri=%s", client->uri);

        // Should not happen.
        if (client->uri == NULL) {
                http_send_error_headers(client->socket, 500);
                delete_streamer_client(client);
                return;                
        }
        
        // In case index.html or index.json is requested,
        // short-circuit the normal pathway and send back an index
        // page.
        if (rstreq(client->uri, "/") || rstreq(client->uri, "/index.html")) {
                streamer_send_index_html(client->streamer, client->socket);
                delete_streamer_client(client);
                return;
        } else if (rstreq(client->uri, "/index.json")) {
                streamer_send_index_json(client->streamer, client->socket);
                delete_streamer_client(client);
                return;                
        }

        if (!rstreq(client->uri, "/stream.html")) {
                http_send_error_headers(client->socket, 404);
                return;
        }
        
        //log_debug("streamer_client_handle_request 3");
        if (streamer_onclient(client->streamer, client) != 0) {
                http_send_error_headers(client->socket, 500);
                delete_streamer_client(client);
                return;
        }

        //log_debug("streamer_client_handle_request 4");
        streamer_add_client(client->streamer, client);

        //log_debug("streamer_client_handle_request 5");
        streamer_client_run(client);
        
        //log_debug("streamer_client_handle_request 6");
        streamer_remove_client(client->streamer, client);
        
        //log_debug("streamer_client_handle_request 7");
        delete_streamer_client(client);

        //log_debug("streamer_client_handle_request 8");
        
        log_info("streamer_client: thread finished");
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
        int ret;
        int socklen = sizeof(struct sockaddr_in);
        
        streamer_t *streamer = new_obj(streamer_t);
        if (streamer == NULL)
                return NULL;

        streamer->name = mem_strdup(name);
        streamer->topic = mem_strdup(topic);
        streamer->mimetype = mem_strdup(mimetype);
        if (streamer->name == NULL
            || streamer->mimetype == NULL
            || streamer->topic == NULL) {
                delete_obj(streamer);
                return NULL;
        }

        streamer->onclient = onclient;
        streamer->onbroadcast = onbroadcast;
        streamer->userdata = userdata;
        streamer->clients = NULL;
        
        streamer->clients_mutex = new_mutex();
        if (streamer->clients_mutex == NULL) {
                delete_obj(streamer);
                return NULL;
        }
        
        streamer->addr = new_addr(app_ip(), port);
        if (streamer->addr == NULL) {
                delete_obj(streamer);
                return NULL;
        }
        
        streamer->socket = open_server_socket(streamer->addr);
        if (streamer->socket == INVALID_TCP_SOCKET) {
                log_err("Failed to create server socket");
                delete_streamer(streamer);
                return NULL;
        }

        char b[52];
        log_info("Streamer listening at http://%s",
                 addr_string(streamer->addr, b, sizeof(b)));

        streamer->cont = 1;        

        streamer->server_thread = new_thread((thread_run_t) streamer_run_server,
                                             (void*) streamer, 0, 0);
        if (streamer->server_thread == NULL) {
                log_err("Failed to start the server thread");
                delete_streamer(streamer);
                return NULL;
        }

        if (streamer->onbroadcast) {
                streamer->data_thread = new_thread((thread_run_t) streamer_run_data,
                                                   (void*) streamer, 0, 0);
                if (streamer->data_thread == NULL) {
                        log_err("Failed to start the server thread");
                        delete_streamer(streamer);
                        return NULL;
                }
        }
        
        return streamer;
}

void delete_streamer(streamer_t *streamer)
{
        char b[128];
        list_t *l;
        
        if (streamer) {
                streamer->cont = 0;
                
                if (streamer->server_thread) {
                        //log_debug("delete_streamer: joining server thread");
                        thread_join(streamer->server_thread);
                        delete_thread(streamer->server_thread);
                }
                
                if (streamer->data_thread) {
                        //log_debug("delete_streamer: joining data thread");
                        thread_join(streamer->data_thread);
                        delete_thread(streamer->data_thread);
                }
                
                //log_debug("delete_streamer: deleting clients");
                
                // delete clients
                if (streamer->clients_mutex) {
                        streamer_lock_clients(streamer); // lock
                        l = streamer->clients;
                        while (l) {
                                streamer_client_t *r = list_get(l, streamer_client_t);
                                /* delete_streamer_client(r); */
                                l = list_next(l);
                        }
                        delete_list(streamer->clients);
                        streamer->clients = NULL;
                        streamer_unlock_clients(streamer); // unlock
                        delete_mutex(streamer->clients_mutex);
                }
                if (streamer->topic)
                        mem_free(streamer->topic);
                if (streamer->name)
                        mem_free(streamer->name);
                if (streamer->mimetype)
                        mem_free(streamer->mimetype);
                if (streamer->addr)
                        delete_addr(streamer->addr);
                if (streamer->socket != INVALID_TCP_SOCKET)
                        close_tcp_socket(streamer->socket);

                delete_obj(streamer);                
        }
}

addr_t *streamer_addr(streamer_t *s)
{
        return s->addr;
}

static void streamer_run_server(streamer_t *streamer)
{
        tcp_socket_t socket;

        log_info("Thread 'streamer_run_server' starting");
        
        while (!app_quit() && streamer->cont) {
                //log_info("streamer_run_server: waiting for new connection");
                
                socket = server_socket_accept(streamer->socket);
                if (socket == TCP_SOCKET_TIMEOUT) {
                        continue;
                } else if (socket == INVALID_TCP_SOCKET) {
                        // Server socket is probably being closed // FIXME: is this true?
                        log_err("streamer_run_server: accept failed");
                        continue;
                }

                //log_info("streamer_run_server: got connection");
                streamer_client_t *client = new_streamer_client(streamer, socket);
                if (client == NULL) {
                        log_err("out of memory");
                        http_send_error_headers(socket, 500);
                        close_tcp_socket(socket);
                        continue;
                }
                
                //log_info("streamer_run_server: spinning off new thread to handle client");
                new_thread((thread_run_t) streamer_client_handle_request, client, 0, 1);
        }

        log_info("Thread 'streamer_run_server' finished");
}

static void streamer_run_data(streamer_t *streamer)
{
        tcp_socket_t socket;

        log_info("Thread 'streamer_run_data' starting");
        
        while (!app_quit() && streamer->cont) {
                streamer->onbroadcast(streamer->userdata, streamer);
        }

        log_info("Thread 'streamer_run_data' finished");
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
                        log_info("streamer_onclient: callback returned an error");
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
                if (((c = streamer_client_get_buffer(b)) != NULL) 
                    && (circular_buffer_space(c) > total_len)) {
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
        export_t *e;
        list_t *l;
        char b[52];
        membuf_t *membuf = new_membuf();

        //log_debug("streamer_send_index_html");

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
        log_info("streamer_send_index_html: thread finished");
}

static void streamer_send_index_json(streamer_t *streamer, tcp_socket_t s)
{
        export_t *e;
        list_t *l;
        char b[52];
        membuf_t *membuf = new_membuf();

        //log_debug("streamer_send_index_json");

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
        log_info("streamer_send_index: thread finished");
}

static const char *streamer_mimetype(streamer_t *streamer)
{
        return streamer->mimetype;
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
        
/*         log_info("Starting dumper thread"); */
/*         client = new_streamer_client(s, -1, dummy); */
/*         client->exp = e; */
/*         streamer_add_client(s, client); */
/*         pthread_create(&thread, NULL, _dumper_thread_run, client); */
/* } */

