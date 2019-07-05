#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include "rcom/json.h"
#include "rcom/log.h"
#include "rcom/app.h"
#include "rcom/addr.h"

#include "mem.h"
#include "util.h"
#include "net.h"
#include "thread.h"
#include "datalink_priv.h"

struct _datalink_t {
        udp_socket_t socket;
        addr_t *addr;
        addr_t *remote_addr;
        data_t* in;
        data_t* out;
        void* userdata;
        datalink_ondata_t ondata;
        json_parser_t* parser;
        int thread_quit;
        thread_t *thread;
        mutex_t *mutex;
};

int datalink_start_thread(datalink_t *datalink);
void datalink_stop_thread(datalink_t *datalink);

datalink_t* new_datalink(datalink_ondata_t callback, void* userdata)
{
        int ret;
        datalink_t* link;
                
        link = new_obj(datalink_t);
        if (link == NULL)
                return NULL;
        
        link->in = new_data();
        link->out = new_data();
        link->parser = json_parser_create();
        if (link->in == NULL || link->out == NULL || link->parser == NULL) {
                delete_datalink(link);
                return NULL;
        }

        link->mutex = new_mutex();
        if (link->mutex == NULL) {
                delete_datalink(link);
                return NULL;
        }
        
        // create a UDP socket
        link->socket = open_udp_socket(0);
        if (link->socket == INVALID_UDP_SOCKET) {
                delete_datalink(link);
                return NULL;
        }

        link->addr = udp_socket_addr(link->socket);
        if (link->addr == NULL) {
                delete_datalink(link);
                return NULL;
        }

        link->userdata = userdata;
        link->ondata = callback;

        if (link->ondata) {
                ret = datalink_start_thread(link);
                if (ret != 0) {
                        delete_datalink(link);
                        return NULL;
                }
        }
        
        return link;
}

void delete_datalink(datalink_t* link)
{
        if (link) {
                datalink_stop_thread(link);
                if (link->socket > 0)
                        close_udp_socket(link->socket);
                if (link->in != NULL)
                        delete_data(link->in);
                if (link->out != NULL)
                        delete_data(link->out);
                if (link->parser != NULL)
                        json_parser_destroy(link->parser);
                if (link->addr != NULL)
                        delete_addr(link->addr);
                if (link->remote_addr != NULL)
                        delete_addr(link->remote_addr);
                if (link->mutex != NULL)
                        delete_mutex(link->mutex);
                delete_obj(link);
        }
}

static int datalink_handle_input(datalink_t* link)
{
        int i, ret;
        
        if (link->remote_addr == NULL) {
                // waiting for remote node
                log_debug("datalink_handle_input: remote_addr == NULL");
                sleep(1);
                return 0;
        }
        
        data_t *data = datalink_read(link, 1);
        if (data == NULL) // FIXME: error? timeout?
                return 0;
        
        if (link->ondata) {
                // Initilize the timestamp and reset the length of the
                // reply data. The client can improve the accuracy of
                // the timestamp is needed.
                data_set_timestamp(link->out);
                data_set_len(link->out, 0);
                
                ret = link->ondata(link->userdata, link, link->in, link->out);
                
                if (ret == 0 && data_len(link->out) > 0) {
                        datalink_send(link, link->out);
                }
        }
        
        return ret;
}

static void datalink_run(void* d)
{
        datalink_t* link = (datalink_t*) d;
        int r;

        while (!link->thread_quit && !app_quit()) {
                r = datalink_handle_input(link);
                if (r != 0) {
                        log_info("datalink_run: datalink_handle_input "
                                 "returned non-zero. Quitting.");
                        break;
                }
        }
}

data_t *datalink_get_output(datalink_t* link)
{
        return link->out;
}

json_object_t datalink_parse(datalink_t* link, data_t* data)
{
        return data_parse(data, link->parser);
}

int datalink_start_thread(datalink_t *link)
{
        int ret;
        link->thread_quit = 0;
        link->thread = new_thread(datalink_run, (void*) link, 0);
        return (link->thread == NULL)? -1 : 0;
}

void datalink_stop_thread(datalink_t* link)
{
        link->thread_quit = 1;
        if (link->thread)
                thread_join(link->thread);
}

addr_t *datalink_addr(datalink_t* link)
{
        return link->addr;
}

addr_t *datalink_remote_addr(datalink_t* link)
{
        return link->remote_addr;
}

void datalink_set_remote_addr(datalink_t *link, addr_t *addr)
{
        log_debug("datalink_set_remote_addr");

        mutex_lock(link->mutex);
        if (link->remote_addr != NULL && addr != NULL && addr_eq(link->remote_addr, addr)) {
                mutex_unlock(link->mutex);
                return;
        }
        if (link->remote_addr == NULL && addr == NULL) {
                mutex_unlock(link->mutex);
                return;
        }
        
        addr_t *clone = NULL;

        if (addr != NULL) {
                clone =  addr_clone(addr);
                if (clone == NULL) {
                        log_err("datalink_set_remote_addr: failed to clone the address!");
                        mutex_unlock(link->mutex);
                        return;
                }
        }
        
        if (link->remote_addr != NULL)
                delete_addr(link->remote_addr);
        link->remote_addr = clone;

        mutex_unlock(link->mutex);
}

// returns: 0 is all ok, -1 if error, 1 if timeout
data_t *datalink_read(datalink_t* link, int timeout)
{
        if (timeout >= 0) {
                int r = udp_socket_wait_data(link->socket, timeout);
                if (r <= 0) return NULL;
        }
        addr_t addr; // FIXME
        int ret = udp_socket_read(link->socket, link->in, &addr);
        return (ret == 0)? link->in : NULL;
}

int datalink_sendto(datalink_t *link, addr_t *addr, data_t *data)
{
        return udp_socket_send(link->socket, addr, data);
}

int datalink_send(datalink_t* link, data_t *data)
{
        int err = 0;
        mutex_lock(link->mutex);
        if (link->remote_addr != NULL)
                err = udp_socket_send(link->socket, link->remote_addr, data);
        mutex_unlock(link->mutex);
        return err;
}

int datalink_send_f(datalink_t* link, const char *format, ...)
{
        int err;
        va_list ap;
        
        va_start(ap, format);
        err = data_vprintf(link->out, format, ap);
        va_end(ap);

        if (err != 0) return err;
        
        data_set_timestamp(link->out);
        
        return datalink_send(link, link->out);
}

int datalink_send_obj(datalink_t *link, json_object_t obj)
{
        int err = data_serialise(link->out, obj);
        if (err != 0) return err;
        data_set_timestamp(link->out);
        return datalink_send(link, link->out);
}

json_object_t datalink_read_obj(datalink_t *link, int timeout)
{
        data_t *data = datalink_read(link, timeout);
        if (data == NULL)
                return json_null();
        return data_parse(data, link->parser);
}

json_object_t datalink_rpc_obj(datalink_t* link, json_object_t obj)
{
        int ret;

        ret = datalink_send_obj(link, obj);
        if (ret != 0)
                return json_null();

        return datalink_read_obj(link, 5);
}


json_object_t datalink_rpc_f(datalink_t* link, const char* format, ...)
{
        int ret;
        va_list ap;

        va_start(ap, format);
        ret = data_vprintf(link->out, format, ap);
        va_end(ap);

        if (ret != 0) 
                return json_null();
        
        data_set_timestamp(link->out);
        
        ret = datalink_send(link, link->out);
        if (ret != 0) 
                return json_null();
        
        return datalink_read_obj(link, 5);
}


