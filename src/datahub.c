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

#include "util_priv.h"
#include "datalink_priv.h"
#include "datahub_priv.h"
#include "net.h"

struct _datahub_t {
        udp_socket_t socket;
        addr_t *addr;
        list_t *links;
        datahub_onbroadcast_t onbroadcast;
        datahub_ondata_t ondata;
        mutex_t *mutex;
        data_t *input;
        data_t *output;
        void *userdata;
        thread_t *data_thread;
        thread_t *broadcast_thread;
        int quit_thread;
};

static void datahub_lock(datahub_t *d);
static void datahub_unlock(datahub_t *d);
static int datahub_send_locked(datahub_t *d, addr_t *exclude, data_t *data, int stamp);
static int datahub_broadcast_locked(datahub_t *d, addr_t *exclude, data_t *data, int stamp);
static void datahub_run_data(datahub_t *hub);
static void datahub_run_broadcast(datahub_t *hub);

datahub_t *new_datahub(datahub_onbroadcast_t onbroadcast,
                       datahub_ondata_t ondata,
                       void *userdata)
{
        datahub_t *hub;

        hub = r_new(datahub_t);
        if (hub == NULL)
                return NULL;

        hub->socket = open_udp_socket();
        if (hub->socket == INVALID_UDP_SOCKET)
                goto error_recovery;
        
        hub->addr = udp_socket_addr(hub->socket);
        if (hub->addr == NULL) 
                goto error_recovery;
        
        hub->onbroadcast = onbroadcast;
        hub->ondata = ondata;
        hub->userdata = userdata;
        hub->quit_thread = 0;
        
        hub->output = new_data();
        hub->input = new_data();
        if (hub->output == NULL || hub->input == NULL)
                goto error_recovery;
        
        hub->mutex = new_mutex();
        if (hub->mutex == NULL)
                goto error_recovery;

        if (hub->ondata)  {
                hub->data_thread = new_thread((thread_run_t) datahub_run_data, hub, 1, 0);
                if (hub->data_thread == NULL)
                        goto error_recovery;
        }

        if (hub->onbroadcast)  {
                hub->broadcast_thread = new_thread((thread_run_t) datahub_run_broadcast,
                                                   hub, 1, 0);
                if (hub->broadcast_thread == NULL)
                        goto error_recovery;
        }
        
        return hub;

error_recovery:
        delete_datahub(hub);
        return NULL;
}

void delete_datahub(datahub_t *hub)
{
        list_t *l;
        
        if (hub) {
                
                hub->quit_thread = 1;
                
                if (hub->data_thread) {
                        thread_join(hub->data_thread);
                        delete_thread(hub->data_thread);
                }
                
                if (hub->broadcast_thread) {
                        thread_join(hub->broadcast_thread);
                        delete_thread(hub->broadcast_thread);
                }
                
                if (hub->mutex) datahub_lock(hub);
                l = hub->links;
                while (l) {
                        addr_t *link = list_get(l, addr_t);
                        delete_addr(link);
                        l = list_next(l);
                }
                delete_list(hub->links);
                hub->links = NULL;
                if (hub->mutex) datahub_unlock(hub);                
                
                if (hub->output)
                        delete_data(hub->output);
                if (hub->input)
                        delete_data(hub->input);
                if (hub->addr)
                        delete_addr(hub->addr);
                if (hub->mutex)
                        delete_mutex(hub->mutex);
                
                r_delete(hub);
        }
}

addr_t *datahub_addr(datahub_t* hub)
{
        return hub->addr;
}

static void datahub_lock(datahub_t *hub)
{
        mutex_lock(hub->mutex);
}

static void datahub_unlock(datahub_t *hub)
{
        mutex_unlock(hub->mutex);
}

static addr_t *datahub_find(datahub_t *hub, addr_t *addr)
{
        for (list_t *l = hub->links; l != NULL; l = list_next(l)) {
                addr_t *a = list_get(l, addr_t);
                if (addr_eq(addr, a))
                        return a;
        }
        return NULL;
}

int datahub_add_link(datahub_t* hub, addr_t *addr)
{
        int ret = 0;

        char b[64];
        //r_debug("datahub_add_link: %s", addr_string(addr, b, sizeof(b)));

        datahub_lock(hub);
        addr_t *a = datahub_find(hub, addr);
        if (a != NULL) {
                r_debug("datahub_add_link: %s is already listed", addr_string(addr, b, sizeof(b)));
                datahub_unlock(hub);
                return 0;
        }
        datahub_unlock(hub);

        addr_t *clone = addr_clone(addr);
        if (clone == NULL)
                return -1;
        
        datahub_lock(hub);
        hub->links = list_append(hub->links, clone);
        if (hub->links == NULL) ret = -1;
        datahub_unlock(hub);
        
        return ret;
}

int datahub_remove_link(datahub_t* hub, addr_t *addr)
{
        int ret = 0;
        //r_debug("datahub_remove_link: %s", addr_string(addr, b, sizeof(b)));

        datahub_lock(hub);
        addr_t *a = datahub_find(hub, addr);
        if (a != NULL)
                hub->links = list_remove(hub->links, a);
        datahub_unlock(hub);
        return ret;
}

/**************************************************************/

int datahub_send_num(datahub_t *hub, addr_t *link, double value)
{
        int r;
        datahub_lock(hub);
        r = data_printf(hub->output, "%f", value);
        if (r == 0) r = datahub_send_locked(hub, link, hub->output, 1);
        datahub_unlock(hub);
        return r;
}

int datahub_send_str(datahub_t *hub, addr_t *link, const char *value)
{
        int r;
        membuf_t *t = escape_string(value);
        if (t == NULL) return -1;
        datahub_lock(hub);
        r = data_printf(hub->output, "\"%s\"", membuf_data(t)); 
        if (r == 0) r = datahub_send_locked(hub, link, hub->output, 1);
        datahub_unlock(hub);
        delete_membuf(t);
        return r;
}

int datahub_send_obj(datahub_t *hub, addr_t *link, json_object_t value)
{
        int r;
        datahub_lock(hub);
        r = data_serialise(hub->output, value);
        if (r != 0) {
                r_err("datahub_send: json_tostring failed");
        } else {
                r = datahub_send_locked(hub, link, hub->output, 1);
        }
        datahub_unlock(hub);
        return r;
}

int datahub_send_f(datahub_t *hub, addr_t *link, const char *format, ...)
{
        int r;
        va_list ap;

        datahub_lock(hub);
        
        va_start(ap, format);
        data_vprintf(hub->output, format, ap);
        va_end(ap);

        r = datahub_send_locked(hub, link, hub->output, 1);

        datahub_unlock(hub);
        return r;
}

int datahub_send_v(datahub_t *hub, addr_t *link, const char *format, va_list ap)
{
        int r;
        datahub_lock(hub);
        data_vprintf(hub->output, format, ap);
        r = datahub_send_locked(hub, link, hub->output, 1);
        datahub_unlock(hub);
        return r;
}

int datahub_send_bin(datahub_t *hub, addr_t *link, const char *data, int len)
{
        int r;
        datahub_lock(hub);
        data_set_data(hub->output, data, len);
        r = datahub_send_locked(hub, link, hub->output, 1);
        datahub_unlock(hub);
        return r;
}

int datahub_send(datahub_t *hub, addr_t *link, data_t *m)
{
        int r;
        datahub_lock(hub);
        r = datahub_send_locked(hub, link, m, 0);
        datahub_unlock(hub);
        return r;
}

static int datahub_send_locked(datahub_t *hub, addr_t *link, data_t *data, int stamp)
{
        if (link == NULL || data == NULL) {
                r_err("datahub_send_locked: invalid arguments");
                return -1;
        }
        
        if (stamp)
                data_set_timestamp(data);

        int err = udp_socket_send(hub->socket, link, data);
        if (err) {
                char b[64];
                r_err("datahub_send_locked: failed to send to %s",
                        addr_string(link, b, 64));
                // FIXME: is this the proper way to handle errors?
                hub->links = list_remove(hub->links, link);
                return -1;
        }
        
        return 0;
}

/**************************************************************/

int datahub_broadcast_num(datahub_t *hub, addr_t *exclude, double value)
{
        int r;
        datahub_lock(hub);
        r = data_printf(hub->output, "%f", value);
        if (r == 0) r = datahub_broadcast_locked(hub, exclude, hub->output, 1);
        datahub_unlock(hub);
        return r;
}

int datahub_broadcast_str(datahub_t *hub, addr_t *exclude, const char *value)
{
        int r;
        membuf_t *t = escape_string(value);
        if (t == NULL) return -1;
        datahub_lock(hub);
        r = data_printf(hub->output, "\"%s\"", membuf_data(t)); 
        if (r == 0) r = datahub_broadcast_locked(hub, exclude, hub->output, 1);
        datahub_unlock(hub);
        delete_membuf(t);
        return r;
}

int datahub_broadcast_obj(datahub_t *hub, addr_t *exclude, json_object_t value)
{
        int r;
        datahub_lock(hub);
        r = data_serialise(hub->output, value);
        if (r != 0) {
                r_err("datahub_broadcast: json_tostring failed");
        } else {
                r = datahub_broadcast_locked(hub, exclude, hub->output, 1);
        }
        datahub_unlock(hub);
        return r;
}

int datahub_broadcast_f(datahub_t *hub, addr_t *exclude, const char *format, ...)
{
        int r;
        va_list ap;

        datahub_lock(hub);
        
        va_start(ap, format);
        data_vprintf(hub->output, format, ap);
        va_end(ap);

        r = datahub_broadcast_locked(hub, exclude, hub->output, 1);

        datahub_unlock(hub);
        return r;
}

int datahub_broadcast_v(datahub_t *hub, addr_t *exclude, const char *format, va_list ap)
{
        int r;
        datahub_lock(hub);
        data_vprintf(hub->output, format, ap);
        r = datahub_broadcast_locked(hub, exclude, hub->output, 1);
        datahub_unlock(hub);
        return r;
}

int datahub_broadcast_bin(datahub_t *hub, addr_t *exclude, const char *data, int len)
{
        int r;
        datahub_lock(hub);
        data_set_data(hub->output, data, len);
        r = datahub_broadcast_locked(hub, exclude, hub->output, 1);
        datahub_unlock(hub);
        return r;
}

int datahub_broadcast(datahub_t *hub, addr_t *exclude, data_t *m)
{
        int r;
        datahub_lock(hub);
        r = datahub_broadcast_locked(hub, exclude, m, 0);
        datahub_unlock(hub);
        return r;
}

static int datahub_broadcast_locked(datahub_t *hub, addr_t *exclude, data_t *data, int stamp)
{
        list_t *l = hub->links;
        list_t *rm = NULL;
        
        if (stamp)
                data_set_timestamp(data);

        while (l) {
                int err = 0;
                addr_t *addr = list_get(l, addr_t);
                if (addr != exclude)
                        err = udp_socket_send(hub->socket, addr, data);
                if (err) {
                        char b[64];
                        r_err("datahub_broadcast_locked: failed to send to %s",
                                addr_string(addr, b, 64));
                        rm = list_prepend(rm, addr);
                }
                l = list_next(l);
        }

        // FIXME: is this the proper way to handle errors?
        l = rm;
        while (l) {
                addr_t *addr = list_get(l, addr_t);
                hub->links = list_remove(hub->links, addr);
                l = list_next(l);
        }
        if (rm) delete_list(rm);
        
        return 0;
}

/**************************************************************/

static void datahub_read(datahub_t *hub)
{
        list_t *list;
        addr_t *link;
        
        datahub_lock(hub);
        
        list = hub->links;
        
        while (list != NULL) {
                link = list_get(list, addr_t);
                int err = udp_socket_read(hub->socket, hub->input, link);
                if (err == 0) {
                        datahub_unlock(hub);
                        if (hub->ondata)
                                hub->ondata(hub->userdata, hub, link, hub->input);
                        datahub_lock(hub);
                        list = list_next(list);
                } else {
                        // TODO: are you sure this is the right way to
                        // handle connection errors?
                        list = list_next(list);
                        hub->links = list_remove(hub->links, link);
                        delete_addr(link);
                }
        }
        
        datahub_unlock(hub);
}

static void datahub_run_data(datahub_t *hub)
{
        while (!app_quit() && !hub->quit_thread) {
                if (udp_socket_wait_data(hub->socket, 1) == 1)
                        datahub_read(hub);
        }
}

/**************************************************************/

static void datahub_run_broadcast(datahub_t *hub)
{
        while (!app_quit() && !hub->quit_thread) {
                hub->onbroadcast(hub->userdata, hub);
        }
}
