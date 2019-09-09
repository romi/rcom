
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
        streamerlink_onresponse_t onresponse;
        tcp_socket_t socket;
        addr_t *addr;
        addr_t *remote_addr;
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
                                 streamerlink_onresponse_t onresponse,
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
        link->onresponse = onresponse;
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
                if (link->addr)
                        delete_addr(link->addr);
                
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
        response_t *response = NULL;
        size_t len = 80*1024;
        char buf[len];
        int err;

        //r_debug("streamerlink_run: sending request");
        
        err = streamerlink_send_request(link);
        if (err != 0)
                goto cleanup;
        
        response = new_response(HTTP_Status_OK);
        if (response == NULL)
                goto cleanup;
        
        err = response_parse_html(response, link->socket, RESPONSE_PARSE_HEADERS);
        if (err != 0)
                goto cleanup;


        // call onresponse
        if (link->onresponse)
                link->onresponse(link->userdata, response);
        
        delete_response(response);
        
        // read the http response. The stream is read below. We rely
        // on the parser to handle the response data. 
        while (!app_quit() && link->cont) {
                
                int received = tcp_socket_recv(link->socket, buf, len);

                // we got a timeout. This allows us to check whether
                // app_quit has been set, and then try again.
                if (received == -2)
                        continue;
                
                // an error occured.
                if (received < 0) {
                        r_err("streamerlink: recv failed");
                        break;
                }

                err = link->ondata(link->userdata, buf, received);
                if (err != 0)
                        break;
        }

cleanup:

        if (response)
                delete_response(response);
        
        streamerlink_lock(link);        
        streamerlink_close_connection(link);
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
