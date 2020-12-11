/*
  rcutil

  Copyright (C) 2019 Sony Computer Science Laboratories
  Author(s) Peter Hanappe

  rcutil is light-weight libary for inter-node communication.

  rcutil is free software: you can redistribute it and/or modify it
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

#include "app.h"
#include "addr.h"
#include "util.h"

#include "net.h"
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
                
        link = r_new(datalink_t);
        link->in = new_data();
        link->out = new_data();
        link->parser = json_parser_create();
        link->mutex = new_mutex();
        
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

        {
                char b[64];
                r_debug("new_datalink: %s", addr_string(link->addr, b, sizeof(b)));
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
                if (link->socket != INVALID_UDP_SOCKET)
                        close_udp_socket(link->socket);
                delete_data(link->in);
                delete_data(link->out);
                json_parser_destroy(link->parser);
                delete_addr(link->addr);
                delete_addr(link->remote_addr);
                delete_mutex(link->mutex);
                r_delete(link);
        }
}

static void datalink_handle_input(datalink_t* link)
{
        if (link->remote_addr == NULL) {
                //r_debug("datalink_handle_input: remote_addr == NULL");
                // waiting for remote node
                clock_sleep(1);
                return;
        }

        int wait_status;
        data_t *data = datalink_read(link, 1, &wait_status);
        if (wait_status == RCOM_WAIT_OK
            && data != NULL
            && link->ondata != NULL) {
                // Initilize the timestamp and reset the length of the
                // reply data. The client can improve the accuracy of
                // the timestamp if needed.
                data_set_timestamp(link->out);
                data_set_len(link->out, 0);
                
                link->ondata(link->userdata, link, link->in, link->out);
                
                if (data_len(link->out) > 0)
                        datalink_send(link, link->out);
                
        } else if (wait_status == RCOM_WAIT_ERROR) {
                r_debug("datalink_handle_input: datalink_read returned an error");
        } else if (wait_status == RCOM_WAIT_TIMEOUT) {
                r_debug("datalink_handle_input: datalink_read timed out");
        }
}

static void datalink_run(void* d)
{
        datalink_t* link = (datalink_t*) d;

        while (!link->thread_quit && !app_quit())
                datalink_handle_input(link);
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
        mutex_lock(link->mutex);
        if (link->thread == NULL) {
                link->thread_quit = 0;
                link->thread = new_thread(datalink_run, (void*) link);
        }
        mutex_unlock(link->mutex);
        
        return (link->thread == NULL)? -1 : 0;
}

void datalink_stop_thread(datalink_t* link)
{
        mutex_lock(link->mutex);
        link->thread_quit = 1;
        mutex_unlock(link->mutex);
        
        if (link->thread) {
                thread_join(link->thread);
                mutex_lock(link->mutex);
                delete_thread(link->thread);
                link->thread = NULL;
                mutex_unlock(link->mutex);
        }
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
        //r_debug("datalink_set_remote_addr");

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
                        r_err("datalink_set_remote_addr: failed to clone the address!");
                        mutex_unlock(link->mutex);
                        return;
                }
        }
        
        if (link->remote_addr != NULL)
                delete_addr(link->remote_addr);
        link->remote_addr = clone;

        mutex_unlock(link->mutex);
}

// returns: data packet, or NULL if an error or timeout occured.  The
// wait_status will be set to one of RCOM_WAIT_OK, RCOM_WAIT_TIMEOUT, or
// RCOM_WAIT_ERROR.
data_t *datalink_read(datalink_t* link, int timeout, int *wait_status)
{
        data_t *data = NULL;
        
        *wait_status = udp_socket_wait_data(link->socket, timeout);
        if (*wait_status == RCOM_WAIT_OK) {
                addr_t addr; // FIXME
                if (udp_socket_read(link->socket, link->in, &addr) == 0)
                        data = link->in;
        }
        
        return data;
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

        if (err == 0) {
                data_set_timestamp(link->out);
                err = datalink_send(link, link->out);
        }

        return err;
}

int datalink_send_obj(datalink_t *link, json_object_t obj)
{
        int err = data_serialise(link->out, obj);
        if (err == 0) {
                data_set_timestamp(link->out);
                err = datalink_send(link, link->out);
        }
        return err;
}

json_object_t datalink_read_obj(datalink_t *link, int timeout)
{
        json_object_t r = json_null();
        int wait_status;
        data_t *data = datalink_read(link, timeout, &wait_status);
        if (wait_status == RCOM_WAIT_OK
            && data != NULL)
                r = data_parse(data, link->parser);
        return r;
}

json_object_t datalink_rpc_obj(datalink_t* link, json_object_t obj)
{
        json_object_t r = json_null();
        int ret;

        ret = datalink_send_obj(link, obj);
        if (ret == 0)
                r = datalink_read_obj(link, 5);
        return r;
}


json_object_t datalink_rpc_f(datalink_t* link, const char* format, ...)
{
        int ret;
        va_list ap;
        json_object_t r = json_null();

        va_start(ap, format);
        ret = data_vprintf(link->out, format, ap);
        va_end(ap);

        if (ret == 0) {
                data_set_timestamp(link->out);
                ret = datalink_send(link, link->out);
                if (ret == 0) 
                        r = datalink_read_obj(link, 5);
        }
        return r;
}


