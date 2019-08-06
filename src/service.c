
#include "rcom/log.h"
#include "rcom/app.h"
#include "rcom/addr.h"
#include "rcom/thread.h"
#include "rcom/util.h"

#include "mem.h"
#include "list.h"
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
static void service_index_html(service_t* service, request_t *request);  
static void service_index_json(service_t* service, request_t *request);  

service_t* new_service(const char *name, int port)
{
        int ret;
        int socklen = sizeof(struct sockaddr_in);
        
        service_t* service = new_obj(service_t);
        if (service == NULL)
                return NULL;

        service->name = mem_strdup(name);
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
                log_err("Failed to create server socket");
                delete_service(service);
                return NULL;
        }

        char b[52];
        log_info("Service listening at http://%s",
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
                        mem_free(service->name);
                if (service->socket != INVALID_TCP_SOCKET)
                        close_tcp_socket(service->socket);

                if (service->mutex)
                        delete_mutex(service->mutex);

                delete_obj(service);
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

static void service_index_html(service_t* service, request_t *request)
{
        list_t *l;
        export_t *e;
        char b[52];
        
        request_reply_printf(request, 
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
                        request_reply_printf(request, 
                                             "    <a href=\"http://%s%s\">%s</a><br>\n",
                                             b, name, name);
                else
                        request_reply_printf(request, 
                                             "    <a href=\"http://%s/%s\">%s</a><br>\n",
                                             b, name, name);

                l = list_next(l);
        }
        service_unlock(service);
        
        request_reply_printf(request, 
                             "  </body>\n"
                             "</html>\n");
}

static void service_index_json(service_t* service, request_t *request)
{
        list_t *l;
        export_t *e;
        char b[52];
        int count = 0;
        request_reply_printf(request, "{\"resources\": [");

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
                        if (count) request_reply_printf(request, ", ");
                        request_reply_printf(request, 
                                             "{\"name\": \"%s\", \"uri\": \"http://%s/%s\"}",
                                             name, b, s);
                        count++;
                }
                l = list_next(l);
        }
        service_unlock(service);

        request_reply_printf(request, "]}");
}

int service_export(service_t* service,
                   const char* name,
                   const char* mimetype_in,
                   const char* mimetype_out,
                   void *data,
                   service_onrequest_t onrequest)
{
        export_t* e = new_export(name, mimetype_in, mimetype_out);
        if (e == NULL) 
                return -1;
        export_set_onrequest(e, data, onrequest);
        
        service_lock(service);
        service->exports = list_prepend(service->exports, e);
        service_unlock(service);

        return 0;
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
                        log_err("accept failed"); 
                        continue;
                }

                request_t* r = new_request(service, client_socket);
                if (r == NULL) {
                        log_err("out of memory");
                        continue;
                }
                
                new_thread((thread_run_t) request_handle, r, 0, 1);
        }
}

void service_run_in_thread(service_t *service)
{
}

export_t *service_get_export(service_t *service, const char *name)
{
        list_t *l;
        export_t *n = NULL;
        
        service_lock(service);
        l = service->exports;
        while (l) {
                export_t* r = list_get(l, export_t);
                //log_debug("export '%s', name '%s'", export_name(r), name);
                if (rstreq(name, export_name(r))
                    || (name[0] == '/' && rstreq(name+1, export_name(r)))) {
                        //log_debug("found export matching '%s'", name);
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
