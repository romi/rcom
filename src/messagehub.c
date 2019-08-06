
#include "rcom/log.h"
#include "rcom/app.h"
#include "rcom/messagelink.h"
#include "rcom/membuf.h"
#include "rcom/thread.h"

#include "util_priv.h"
#include "mem.h"
#include "list.h"
#include "net.h"
#include "http.h"
#include "http_parser.h"
#include "messagelink_priv.h"
#include "messagehub_priv.h"

struct _messagehub_t {
        thread_t *thread;
        addr_t *addr;
        tcp_socket_t socket;

        list_t *links;
        mutex_t *links_mutex;

        membuf_t *mem;
        int quit;
        
        messagehub_onconnect_t onconnect;
        void *userdata;
};

static void messagehub_add_link(messagehub_t *hub, messagelink_t *link);
static void messagehub_lock_links(messagehub_t* hub);
static void messagehub_unlock_links(messagehub_t* hub);
static void messagehub_run(messagehub_t* hub);

messagehub_t* new_messagehub(int port,
                             messagehub_onconnect_t onconnect,
                             void *userdata)
{
        int ret;
        
        messagehub_t* hub = new_obj(messagehub_t);
        if (hub == NULL)
                return NULL;

        hub->onconnect = onconnect;
        hub->userdata = userdata;
        hub->mem = NULL;
        hub->links = NULL;        
        hub->quit = 0;
        hub->links_mutex = new_mutex();

        hub->addr = new_addr(app_ip(), port);
        
        hub->socket = open_server_socket(hub->addr);
        if (hub->socket == INVALID_TCP_SOCKET) {
                delete_messagehub(hub);
                return NULL;
        }

        char b[64];
        log_info("Messagehub listening at http://%s:%d",
                 addr_ip(hub->addr, b, 64), addr_port(hub->addr));

        hub->thread = new_thread((thread_run_t) messagehub_run, hub, 0, 0);
        if (hub->thread == NULL) {
                delete_messagehub(hub);
                return NULL;
        }
        
        return hub;
}

void delete_messagehub(messagehub_t*hub)
{
        list_t *l;

        log_debug("delete_messagehub");
        
        if (hub) {
                hub->quit = 1;
                if (hub->thread) {
                        thread_join(hub->thread);
                        delete_thread(hub->thread);
                }
                
                if (hub->mem)
                        delete_membuf(hub->mem);
                
                if (hub->socket != INVALID_TCP_SOCKET) {
                        close_tcp_socket(hub->socket);
                        hub->socket = INVALID_TCP_SOCKET;
                }
                
                if (hub->links_mutex) {
                        int count = 0;
                        while (hub->links) {
                                // The head of the list is removed one
                                // at the time, protected by the
                                // mutex. Then owner_messagelink_close
                                // is called outside of the mutex to
                                // avoid a deadlock because
                                // owner_messagelink_close calls
                                // messagehub_remove_link, which also
                                // uses the mutex.
                                messagehub_lock_links(hub);
                                l = hub->links;
                                hub->links = list_next(l);
                                messagehub_unlock_links(hub);                

                                log_debug("delete_messagehub: deleting messagelink %d",
                                          count++);
                                messagelink_t *link = list_get(l, messagelink_t);
                                delete_messagelink(link);
                                delete1_list(l);

                        }
                        delete_mutex(hub->links_mutex);
                }

                if (hub->addr)
                        delete_addr(hub->addr);
                
                delete_obj(hub);
        }

        //log_debug("delete_messagehub: done");
}

addr_t *messagehub_addr(messagehub_t *hub)
{
        return hub->addr;
}

static void messagehub_link_onclose()
{
}

static void messagehub_handle_connect(messagehub_t *hub)
{
        messagelink_t *link;
        tcp_socket_t link_socket;

        //log_debug("messagehub_handle_connect");
        
        link_socket = server_socket_accept(hub->socket);
        if (link_socket == INVALID_TCP_SOCKET
            || link_socket == TCP_SOCKET_TIMEOUT)
                return;

        //log_debug("messagehub_handle_connect: new http client");

        link = server_messagelink_connect(hub, link_socket);
        if (link == NULL)
                return;
                                
        //log_debug("messagehub_handle_connect: new link");

        if (hub->onconnect)
                if (hub->onconnect(hub, link, hub->userdata) != 0) {
                        delete_messagelink(link);
                        return;
                }
        
        messagehub_add_link(hub, link);
        messagelink_read_in_background(link);
}

static void messagehub_run(messagehub_t *hub)
{
        while (!hub->quit && !app_quit() && hub->socket != -1) {
                messagehub_handle_connect(hub);
        }
        log_info("messagehub_run: no longer accepting connections");
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
        messagehub_lock_links(hub);
        hub->links = list_remove(hub->links, link);
        messagehub_unlock_links(hub);
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
        if (t == NULL) return -1;
        membuf_lock(hub->mem);
        membuf_clear(hub->mem);
        err = membuf_printf(hub->mem, "\"%s\"", membuf_data(t));
        if (err == 0)
                err = messagehub_broadcast_text(hub, exclude, membuf_data(hub->mem), membuf_len(hub->mem));
        membuf_unlock(hub->mem);
        delete_membuf(t);
        return err;
}

static int32 messagehub_serialise(messagehub_t *hub, const char* s, int32 len)
{
        return membuf_append(hub->mem, s, len);
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
        int len;
        
        if (messagehub_membuf(hub) != 0)
                return -1;
        
        va_start(ap, format);
        len = vsnprintf(NULL, 0, format, ap);
        va_end(ap);

        membuf_lock(hub->mem);
        membuf_clear(hub->mem);

        err = membuf_assure(hub->mem, len+1);

        if (err == 0) {
                va_start(ap, format);
                err = membuf_vprintf(hub->mem, format, ap);
                va_end(ap);
        }

        if (err == 0) 
                err = messagehub_broadcast_text(hub, exclude, membuf_data(hub->mem),
                                                membuf_len(hub->mem));
        
        membuf_unlock(hub->mem);
        
        return err;
}

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


