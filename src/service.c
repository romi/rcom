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

#include "rcom.h"

#include "response_priv.h"
#include "request_priv.h"
#include "service_priv.h"

/******************************************************************************/
        
struct _service_t {
        char *name;
        addr_t *addr;
        tcp_socket_t socket;
        list_t* exports;
        mutex_t *mutex;
        thread_t *thread;
        int cont;
};

static void service_run(service_t *service);
static void service_lock(service_t* s);
static void service_unlock(service_t* s);
static void service_index_html(service_t* service, request_t *request, response_t *response);
static void service_index_json(service_t* service, request_t *request, response_t *response);

service_t* new_service(const char *name, int port)
{
        int ret;
        int socklen = sizeof(struct sockaddr_in);
        
        service_t* service = r_new(service_t);
        if (service == NULL)
                return NULL;

        service->name = r_strdup(name);
        if (service->name == NULL) {
                delete_service(service);
                return NULL;
        }

        service->mutex = new_mutex();
        if (service->mutex == NULL) {
                delete_service(service);
                return NULL;
        }

        service->addr = new_addr(app_ip(), port);
        if (service->addr == NULL) {
                delete_service(service);
                return NULL;
        }

        service_export(service, "/", NULL, "text/html",
                       service, (service_onrequest_t) service_index_html);
        service_export(service, "/index.json", NULL, "application/json",
                       service, (service_onrequest_t) service_index_json);

        service->socket = open_server_socket(service->addr);
        if (service->socket == INVALID_TCP_SOCKET) {
                r_err("Failed to create server socket");
                delete_service(service);
                return NULL;
        }

        char b[52];
        r_info("Service listening at http://%s",
                 addr_string(service->addr, b, sizeof(b)));

        service->cont = 1;
        service->thread = new_thread((thread_run_t) service_run, service, 0, 0);
        if (service->thread == NULL) {
                delete_service(service);
                return NULL;
        }
 
        return service;
}

void delete_service(service_t* service)
{
        list_t *l;
        export_t *e;

        if (service) {
                service->cont = 0;
                if (service->thread) {
                        thread_join(service->thread);
                        delete_thread(service->thread);
                }
                
                service_lock(service);
                l = service->exports;
                while (l) {
                        e = list_get(l, export_t);
                        delete_export(e);
                        l = list_next(l);
                }
                delete_list(service->exports);
                service->exports = NULL;
                service_unlock(service);

                if (service->addr)
                        delete_addr(service->addr);
                if (service->name)
                        r_free(service->name);
                if (service->socket != INVALID_TCP_SOCKET)
                        close_tcp_socket(service->socket);

                if (service->mutex)
                        delete_mutex(service->mutex);

                r_delete(service);
        }
}

static void service_lock(service_t* s)
{
        mutex_lock(s->mutex);
}

static void service_unlock(service_t* s)
{
        mutex_unlock(s->mutex);
}

const char *service_name(service_t *service)
{
        return service->name;
}

addr_t *service_addr(service_t* service)
{
        return service->addr;
}

static void service_index_html(service_t* service, request_t *request, response_t *response)
{
        list_t *l;
        export_t *e;
        char b[52];

        response_printf(response, 
                        "<!DOCTYPE html>\n"
                        "<html lang=\"en\">\n"
                        "  <head>\n"
                        "    <meta charset=\"utf-8\">\n"
                        "    <title>%s</title>\n"
                        "  </head>\n"
                        "  <body>\n",
                        service->name);

        addr_string(service->addr, b, 52);
        
        service_lock(service);
        l = service->exports;
        while (l) {
                e = list_get(l, export_t);
                const char* name = export_name(e);

                if (rstreq(name, "*"))
                    ; // skip
                else if (name[0] == '/')
                        response_printf(response, 
                                        "    <a href=\"http://%s%s\">%s</a><br>\n",
                                        b, name, name);
                else
                        response_printf(response, 
                                        "    <a href=\"http://%s/%s\">%s</a><br>\n",
                                        b, name, name);

                l = list_next(l);
        }
        service_unlock(service);
        
        response_printf(response, "  </body>\n</html>\n");
}

static void service_index_json(service_t* service, request_t *request, response_t *response)
{
        list_t *l;
        export_t *e;
        char b[52];
        int count = 0;
        
        response_printf(response, "{\"resources\": [");

        addr_string(service->addr, b, 52);
        
        service_lock(service);
        l = service->exports;
        while (l) {
                e = list_get(l, export_t);
                const char* name = export_name(e);
                const char* s = (name[0] == '/')? name + 1 : name;

                if (rstreq(name, "*"))
                    ; // skip
                else {
                        if (count) response_printf(response, ", ");
                        response_printf(response, 
                                             "{\"name\": \"%s\", \"uri\": \"http://%s/%s\"}",
                                             name, b, s);
                        count++;
                }
                l = list_next(l);
        }
        service_unlock(service);

        response_printf(response, "]}");
}

int service_export(service_t* service,
                   const char* name,
                   const char* mimetype_in,
                   const char* mimetype_out,
                   void *data,
                   service_onrequest_t onrequest)
{
        export_t *e;
        int found = 0;
        int err = 0;
        
        service_lock(service);
        for (list_t *l = service->exports; l != NULL; l = list_next(l)) {
                e = list_get(l, export_t);
                if (export_matches(e, name)) {
                        if (export_set_mimetypes(e, mimetype_in, mimetype_out) == 0)
                                export_set_onrequest(e, data, onrequest);
                        else err = -1;
                        found = 1;
                        break;
                }
        }
        service_unlock(service);

        if (found)
                return err;
        
        e = new_export(name, mimetype_in, mimetype_out);
        if (e == NULL) 
                return -1;
        export_set_onrequest(e, data, onrequest);
        
        service_lock(service);
        service->exports = list_prepend(service->exports, e);
        service_unlock(service);

        return 0;
}

struct service_and_socket_t {
        service_t* service;
        tcp_socket_t socket;
};

struct service_and_socket_t *new_service_and_socket(service_t* service,
                                                    tcp_socket_t socket)
{
        struct service_and_socket_t *s;
        s = r_new(struct service_and_socket_t);
        if (s == NULL)
                return NULL;
        s->service = service;
        s->socket = socket;
        return s;
}

void service_handle(struct service_and_socket_t *s)
{
        int err = -1;
        request_t* request = NULL;
        export_t *export = NULL;
        response_t *response = NULL;
        service_t* service = s->service;
        tcp_socket_t client_socket = s->socket;

        request = new_request();
        if (request == NULL) {
                http_send_error_headers(client_socket, HTTP_Status_Internal_Server_Error);
                goto cleanup;
        }

        err = request_parse_html(request, client_socket, REQUEST_PARSE_ALL);
        if (err != 0) {
                http_send_error_headers(client_socket, HTTP_Status_Internal_Server_Error);
                goto cleanup;
        }

        //r_debug("request_handle: request for '%s'", request_uri(request));

        if (request_uri(request) == NULL) {
                r_err("request_handle: requested uri == NULL!?");
                http_send_error_headers(client_socket, HTTP_Status_Bad_Request);
                goto cleanup;
        }

        export = service_get_export(service, request_uri(request));
        if (export == NULL) {
                r_err("request_handle: export == NULL: resource '%s'",
                      request_uri(request));
                http_send_error_headers(client_socket, HTTP_Status_Bad_Request);
                goto cleanup;
        }

        response = new_response(HTTP_Status_OK);
        if (response == NULL) {
                http_send_error_headers(client_socket, HTTP_Status_Internal_Server_Error);
                goto cleanup;
        }
        
        response_set_mimetype(response, export_mimetype_out(export));
        export_callback(export, request, response);

        err = response_send(response, client_socket);
        
cleanup:
        if (err != 0)
                r_err("request_handle: failed to handle request");

        close_tcp_socket(client_socket);
        
        if (s)
                r_delete(s);
        if (request)
                delete_request(request);
        if (response)
                delete_response(response);
        if (export)
                delete_export(export);
}

void service_run(service_t* service)
{
        tcp_socket_t client_socket;
        
        while (!app_quit() && service->cont) {
                client_socket = server_socket_accept(service->socket);
                
                if (client_socket == TCP_SOCKET_TIMEOUT) {
                        continue;
                        
                } else if (client_socket == INVALID_TCP_SOCKET) {
                        // Server socket is probably being closed //
                        // FIXME: is this true?
                        r_err("accept failed"); 
                        continue;
                }

                struct service_and_socket_t *s = new_service_and_socket(service,
                                                                        client_socket);

                new_thread((thread_run_t) service_handle, s, 0, 1);
        }
}

export_t *service_get_export(service_t *service, const char *name)
{
        list_t *l;
        export_t *n = NULL;
        
        service_lock(service);
        l = service->exports;
        while (l) {
                export_t* r = list_get(l, export_t);
                //r_debug("export '%s', name '%s'", export_name(r), name);
                if (export_matches(r, name)) {
                        //r_debug("found export matching '%s'", name);
                        n = export_clone(r);
                        break;
                }
                l = list_next(l);
        }
        if (n == NULL) {
                // If no match was found, seek the wildcard service
                l = service->exports;
                while (l) {
                        export_t* r = list_get(l, export_t);
                        if (rstreq("*", export_name(r))) {
                                n = export_clone(r);
                                break;
                        }
                        l = list_next(l);
                }
        }
        service_unlock(service);
        return n;
}
