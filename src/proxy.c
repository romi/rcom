
#include "rcom/log.h"
#include "rcom/app.h"

#include "mem.h"
#include "util.h"
#include "list.h"
#include "thread.h"
#include "datalink_priv.h"
#include "datahub_priv.h"
#include "messagelink_priv.h"
#include "messagehub_priv.h"
#include "service_priv.h"
#include "streamer_priv.h"
#include "streamerlink_priv.h"
#include "registry_priv.h"
#include "proxy.h"

typedef struct _proxy_t
{
        registry_t *registry;
        messagelink_t *link;
        mutex_t *mutex;
} proxy_t;

static proxy_t* new_proxy(addr_t *addr);
static void delete_proxy(proxy_t* proxy);

// callbacks
static void _proxy_onmessage(messagelink_t *link,
                             void *userdata,
                             json_object_t message);
static void _proxy_onclose(messagelink_t *link, void *userdata);

static void proxy_onmessage(proxy_t* proxy,
                            messagelink_t *link,
                            json_object_t message);
static void proxy_onclose(proxy_t* proxy, messagelink_t *link);


// requests
static int proxy_send_register_request(proxy_t* proxy, registry_entry_t *entry);
static int proxy_send_unregister_request(proxy_t* proxy, registry_entry_t *entry);
static int proxy_send_list_request(proxy_t* proxy);
static int proxy_send_update_address_request(proxy_t* proxy, int id, const char *addr);

// response handlers
static void proxy_handle_register_add(proxy_t* proxy,
                                      messagelink_t *link,
                                      json_object_t message);
static void proxy_handle_register_remove(proxy_t* proxy,
                                         messagelink_t *link,
                                         json_object_t message);
static void proxy_handle_update_list(proxy_t* proxy,
                                     messagelink_t *link,
                                     json_object_t message);
static void proxy_handle_update_address(proxy_t* proxy,
                                        messagelink_t *link,
                                        json_object_t message);

// update the connections
static void proxy_add_connection(proxy_t* proxy, registry_entry_t *entry);
static void proxy_connect_datahub(proxy_t* proxy, registry_entry_t *entry);
static void proxy_connect_datalink(proxy_t* proxy, registry_entry_t *entry);
static void proxy_connect_messagehub(proxy_t* proxy, registry_entry_t *entry);
static void proxy_connect_streamer(proxy_t* proxy, registry_entry_t *entry);
static void proxy_remove_connection(proxy_t* proxy, registry_entry_t *entry);
static void proxy_remove_datahub(proxy_t* proxy, registry_entry_t *entry);
static void proxy_remove_datalink(proxy_t* proxy, registry_entry_t *entry);
static void proxy_remove_messagehub(proxy_t* proxy, registry_entry_t *entry);
        
// utility
static int proxy_is_local(registry_entry_t *entry);


/********************************************************/

static char *_default_name = NULL;
static char *_default_ip = NULL;
static int _default_port = 10101;
static int _standalone = 0;
addr_t* _registry_addr = NULL;
proxy_t* _proxy = NULL;

void set_registry_name(const char* name)
{
        _default_name = name? mem_strdup(name) : NULL;
}

void set_registry_ip(const char* addr)
{
        _default_ip = addr? mem_strdup(addr) : NULL;
}

void set_registry_port(int port)
{
        _default_port = port;
}

void set_registry_standalone(int standalone)
{
        _standalone = standalone;
}

const char *get_registry_name()
{
        return _default_name? _default_name : "registry";
}

const char *get_registry_ip()
{
        return _default_ip? _default_ip : app_ip();
}

int get_registry_port()
{
        return _default_port? _default_port : 10101;
}

void proxy_init()
{
        set_registry_ip("127.0.0.1");
}

void proxy_cleanup()
{
        if (_default_name != NULL)
                mem_free(_default_name);
        if (_default_ip != NULL)
                mem_free(_default_ip);
        if (_proxy)
                delete_proxy(_proxy);
}

static proxy_t *proxy_get()
{
        if (_proxy != NULL)
                return _proxy;

        if (!_standalone) {
                _registry_addr = new_addr(_default_ip, _default_port);
                if (_registry_addr == NULL) {
                        log_err("proxy_create: failed to create the registry address!");
                        return NULL;
                }
        }
        
        _proxy = new_proxy(_registry_addr);
        if (_proxy == NULL) {
                log_err("proxy_create: failed to create the proxy!");
                return NULL;
        }
        
        return _proxy;
}

/********************************************************/

static proxy_t* new_proxy(addr_t *addr)
{
        int err;
        proxy_t* proxy;

        proxy = new_obj(proxy_t);
        if (proxy == NULL)
                return NULL;
        
        proxy->registry = new_registry();
        if (proxy->registry == NULL) {
                delete_proxy(proxy);
                return NULL;
        }
        
        proxy->mutex = new_mutex();
        if (proxy->mutex == NULL) {
                delete_proxy(proxy);
                return NULL;
        }
        
        if (addr) {
                proxy->link = new_messagelink(_proxy_onmessage,
                                              _proxy_onclose,
                                              proxy);

                err = client_messagelink_connect(proxy->link, addr);
                if (err != 0) {
                        log_err("new_proxy: failed to connect to the remote proxy");
                        delete_proxy(proxy);
                        return NULL;
                }

                err = proxy_send_list_request(proxy);
                if (err != 0) {
                        log_err("new_proxy: failed to send the list request");
                        delete_proxy(proxy);
                        return NULL;
                }
                
                //messagelink_read_in_background(proxy->link);
        }
        
        return proxy;
}

static void delete_proxy(proxy_t* proxy)
{
        if (proxy) {
                if (proxy->registry)
                        delete_registry(proxy->registry);
                if (proxy->link)
                        delete_messagelink(proxy->link);
                if (proxy->mutex) 
                        delete_mutex(proxy->mutex);
                delete_obj(proxy);
        }
}

static int proxy_is_local(registry_entry_t *entry)
{
        return entry->endpoint != NULL;
}

static void proxy_onmessage(proxy_t *proxy,
                            messagelink_t *link,
                            json_object_t message)
{
        log_debug("proxy_onmessage");
        
        const char *request;
        
        if (json_isnull(message)) {
                log_err("proxy_onmessage: message is null");
                return;
        }
        
        request = json_object_getstr(message, "request");
        
        if (request == NULL)
                return;

        
        log_debug("proxy_onmessage: request=%s", request);
        
        if (streq(request, "register-response")) {
                int success = json_object_getbool(message, "success");
                if (!success) {
                        log_panic("register failed: %s. Quitting.",
                                json_object_getstr(message, "message"));
                        app_set_quit();
                }
                else log_debug("register succeeded");
                
        } else if (streq(request, "unregister-response")) {
                int success = json_object_getbool(message, "success");
                if (!success)
                        log_err("unregister failed: %s",
                                json_object_getstr(message, "message"));
                else log_debug("unregister succeeded");
                
        } else if (streq(request, "update-address-response")) {
                int success = json_object_getbool(message, "success");
                if (!success)
                        log_err("updating of address failed: %s",
                                json_object_getstr(message, "message"));
                else log_debug("updating of address succeeded");
                
        } else if (streq(request, "proxy-add")) {
                proxy_handle_register_add(proxy, link, message);
                
        } else if (streq(request, "proxy-remove")) {
                proxy_handle_register_remove(proxy, link, message);
                
        } else if (streq(request, "proxy-update-address")) {
                proxy_handle_update_address(proxy, link, message);
                
        } else if (streq(request, "proxy-update-list")) {
                proxy_handle_update_list(proxy, link, message);
                
        } else {
                log_warn("proxy_onmessage: unknown request: %s", request);
        }
}

static void proxy_onclose(proxy_t *proxy, messagelink_t *link)
{
}

static void _proxy_onmessage(messagelink_t *link,
                             void *userdata,
                             json_object_t message)
{
        proxy_t *proxy = (proxy_t *) userdata;
        proxy_onmessage(proxy, link, message);
}

static void _proxy_onclose(messagelink_t *link, void *userdata)
{
        proxy_t *proxy = (proxy_t *) userdata;
        proxy_onclose(proxy, link);
}

static void proxy_handle_register_add(proxy_t* proxy,
                                      messagelink_t *link,
                                      json_object_t message)
{
        int count;
        registry_entry_t *entry;
        int err = 0;

        log_debug("proxy_handle_register_add");

        json_object_t obj = json_object_get(message, "entry");
        if (json_isnull(obj))
                return;
        
        entry = registry_entry_parse(obj, &err);
        switch (err) {
        case 0: break; 
        case -1: log_err("proxy_handle_register_add: Invalid name"); return; 
        case -2: log_err("proxy_handle_register_add: Invalid topic"); return; 
        case -3: log_err("proxy_handle_register_add: Invalid type"); return; 
        case -4: log_err("proxy_handle_register_add: Invalid address"); return; 
        case -5: log_err("proxy_handle_register_add: Invalid ID"); return; 
        default: log_err("proxy_handle_register_add: Unknown error"); return; 
        }

        mutex_lock(proxy->mutex);
        
        count = registry_count(proxy->registry, 0, entry->name,
                               entry->topic, entry->type, entry->addr, NULL);
        if (count == 0)
                err = registry_insert_entry(proxy->registry, entry);
        
        else if (count == 1) {
                list_t *list = registry_select(proxy->registry, 0, entry->name,
                                               entry->topic, entry->type,
                                               entry->addr, NULL);
                
                registry_entry_t *e = list_get(list, registry_entry_t);
                if (e->id != entry->id)
                        registry_update_id(proxy->registry, e->id, entry->id);
                
                delete_registry_entry_list(list);
        }
        
        if (err == 0)
                // Update connections
                proxy_add_connection(proxy, entry);
        
        mutex_unlock(proxy->mutex);
        
        delete_registry_entry(entry);        
}

static void proxy_handle_register_remove(proxy_t* proxy,
                                         messagelink_t *link,
                                         json_object_t message)
{
        registry_entry_t *entry = NULL;
        int err, id;
        double v;
        
        log_debug("proxy_handle_register_remove");
        
        v = json_object_getnum(message, "id");
        if (isnan(v)) {
                log_err("proxy_handle_register_remove: Invalid ID"); 
                return;
        }
        id = (int) v;
        
        mutex_lock(proxy->mutex);
        
        entry = registry_get(proxy->registry, id);
        if (entry) {
                err = registry_delete(proxy->registry, id);
                // Update connections
                proxy_remove_connection(proxy, entry);
        }
        
        mutex_unlock(proxy->mutex);
        
        if (entry) delete_registry_entry(entry);
}

static void proxy_handle_update_address(proxy_t* proxy,
                                        messagelink_t *link,
                                        json_object_t message)
{
        int err;
        int id;
        double v;
        const char *addr;
        
        log_debug("proxy_handle_update_address");
        
        v = json_object_getnum(message, "id");
        if (isnan(v)) {
                log_err("proxy_handle_update_address: Invalid ID"); 
                return;
        }
        id = (int) v;

        addr = json_object_getstr(message, "addr");
        if (addr == NULL) {
                log_err("proxy_handle_update_address: Invalid address"); 
                return;
        }
        
        err = registry_update_addr(proxy->registry, id, addr);
        if (err != 0)
                ; // do something?
}

static void proxy_update_connections(proxy_t* proxy)
{
        mutex_lock(proxy->mutex);

        list_t *entries = registry_select_all(proxy->registry);
        for (list_t *l = entries; l != NULL; l = list_next(l)) {
                registry_entry_t *entry = list_get(l, registry_entry_t);
        }

        mutex_unlock(proxy->mutex);
}

static int _entry_equals(registry_entry_t* entry, registry_entry_t* e) 
{
        return (e->type == entry->type
                && streq(e->name, entry->name)
                && streq(e->topic, entry->topic)
                && addr_eq(e->addr, entry->addr));
}

static registry_entry_t *_list_contains(list_t* list, registry_entry_t *e)
{
        for (list_t *l = list; l != NULL; l = list_next(l)) {
                registry_entry_t *f = list_get(l, registry_entry_t);
                if (_entry_equals(e, f))
                        return f;
        }
        return NULL;
}

static void proxy_handle_update_list(proxy_t* proxy,
                                     messagelink_t *link,
                                     json_object_t message)
{
        int err;

        log_debug("proxy_handle_update_list");
        
        json_object_t obj = json_object_get(message, "list");
        if (json_isnull(obj) || !json_isarray(obj)) {
                log_err("proxy_handle_update_list: 'list' value is not an array");
                return;
        }
        if (json_array_length(obj) == 0) {
                log_err("proxy_handle_update_list: empty 'list' value");
                return;
        }

        list_t *remote_entries = registry_entry_parse_list(obj);
        if (remote_entries == NULL) {
                log_err("proxy_handle_update_list: failed to parse the 'list' value");
                return;
        }
        
        mutex_lock(proxy->mutex);

        // add the missing endpoints
        for (list_t *l = remote_entries; l != NULL; l = list_next(l)) {
                registry_entry_t *e = list_get(l, registry_entry_t);
                int count = registry_count(proxy->registry, 0, e->name, e->topic,
                                           e->type, e->addr, NULL);
                if (count == 0)
                        err = registry_insert_entry(proxy->registry, e);
        }

        // remove the endpoints that have vanished
        list_t *local_entries = registry_select_all(proxy->registry);
        for (list_t *l = local_entries; l != NULL; l = list_next(l)) {

                registry_entry_t *local_entry = list_get(l, registry_entry_t);
                registry_entry_t *remote_entry = _list_contains(remote_entries, local_entry);
                if (remote_entry == NULL) {
                        // a locally registered entry is unknown to the remote registry. 
                        if (local_entry->endpoint != NULL) {
                                log_warn("proxy_handle_update_list: central registry doesn't "
                                         "list active local node '%s:%s'.",
                                         local_entry->name, local_entry->topic);
                        } else {
                                registry_delete(proxy->registry, local_entry->id);
                        }
                }
        }

        delete_registry_entry_list(local_entries);
        delete_registry_entry_list(remote_entries);
        
        list_t *entries = registry_select_all(proxy->registry);
        for (list_t *l = entries; l != NULL; l = list_next(l)) {
                registry_entry_t *entry = list_get(l, registry_entry_t);
                proxy_add_connection(proxy, entry);
        }
        delete_registry_entry_list(entries);
        
        mutex_unlock(proxy->mutex);
}

// A datahub registered. Check whether there are any local datalinks
// that wait for data of this hub. If so, connect them.
static void proxy_connect_datahub(proxy_t* proxy, registry_entry_t *entry)
{
        list_t *entries = registry_select(proxy->registry, 0, NULL, entry->topic,
                                          TYPE_DATALINK, NULL, NULL);

        for (list_t *l = entries; l != NULL; l = list_next(l)) {
                registry_entry_t *e = list_get(l, registry_entry_t);
                if (proxy_is_local(e)) {
                        datalink_t *link = (datalink_t *) e->endpoint;
                        datalink_set_remote_addr(link, entry->addr);
                }
        }

        delete_registry_entry_list(entries);
}

// A datalink registered. Check whether there are any local
// datahubs that should send data to this link.
static void proxy_connect_datalink(proxy_t* proxy, registry_entry_t *entry)
{
        list_t *list;
        registry_entry_t *e;
        
        list = registry_select(proxy->registry, 0, NULL, entry->topic,
                               TYPE_DATAHUB, NULL, NULL);
        if (list == NULL) return;
        
        e = list_get(list, registry_entry_t);
        if (proxy_is_local(e)) {
                datahub_t *hub = (datahub_t *) e->endpoint;
                datahub_add_link(hub, entry->addr);
        }

        delete_registry_entry_list(list);
}

// A messagehub registered. Check whether there are any local
// messagelink that are waiting for this hub. If so, connect them.
static void proxy_connect_messagehub(proxy_t* proxy, registry_entry_t *entry)
{
        list_t *list;
        registry_entry_t *e;
        char b[64];
        
        list = registry_select(proxy->registry, 0, NULL, entry->topic,
                               TYPE_MESSAGELINK, NULL, NULL);
        if (list == NULL) return;
        
        for (list_t *l = list; l != NULL; l = list_next(l)) {
                registry_entry_t *e = list_get(l, registry_entry_t);
                if (proxy_is_local(e)) {
                        messagelink_t *link = (messagelink_t *) e->endpoint;
                        int err = client_messagelink_connect(link, entry->addr);
                        if (err == 0) {
                                //messagelink_read_in_background(link);
                                addr_string(messagelink_addr(link), b, 64); // TODO: elegance?
                                registry_update_addr(proxy->registry, e->id, b);
                                proxy_send_update_address_request(proxy, e->id, b);
                        } else if (err == -2)
                                log_err("proxy_connect_messagehub: link %s already connected.", e->name);
                        else
                                log_err("proxy_connect_messagehub: failed to make the connection.");
                }
        }

        delete_registry_entry_list(list);
}


// A streamer registered. Check whether there are any local
// streamerlinks that are pulling data from this streamer. If so, connect them.
static void proxy_connect_streamer(proxy_t* proxy, registry_entry_t *entry)
{
        list_t *list;
        registry_entry_t *e;
        char b[64];
        
        list = registry_select(proxy->registry, 0, NULL, entry->topic,
                               TYPE_STREAMERLINK, NULL, NULL);
        if (list == NULL) return;
        
        for (list_t *l = list; l != NULL; l = list_next(l)) {
                registry_entry_t *e = list_get(l, registry_entry_t);
                if (proxy_is_local(e)) {
                        streamerlink_t *link = (streamerlink_t *) e->endpoint;
                        int err = streamerlink_connect(link, entry->addr);
                        if (err != 0) {
                                log_err("proxy_connect_streamer: failed to make the connection.");
                        }
                }
        }

        delete_registry_entry_list(list);
}

static void proxy_add_connection(proxy_t* proxy, registry_entry_t *entry)
{
        switch (entry->type) {
        case TYPE_DATALINK: 
                proxy_connect_datalink(proxy, entry);
                break;
        case TYPE_DATAHUB:
                proxy_connect_datahub(proxy, entry);
                break;
        case TYPE_MESSAGELINK:
                break;
        case TYPE_MESSAGEHUB: 
                proxy_connect_messagehub(proxy, entry);
                break;
        case TYPE_STREAMER: 
                proxy_connect_streamer(proxy, entry);
                break;
        }
}

// A datahub disappeared. Tell all the local datalinks that the hub is gone.
static void proxy_remove_datahub(proxy_t* proxy, registry_entry_t *entry)
{
        list_t *entries = registry_select(proxy->registry, 0, NULL, entry->topic,
                                          TYPE_DATALINK, NULL, NULL);

        for (list_t *l = entries; l != NULL; l = list_next(l)) {
                registry_entry_t *e = list_get(l, registry_entry_t);
                if (proxy_is_local(e)) {
                        datalink_t *link = (datalink_t *) e->endpoint;
                        datalink_set_remote_addr(link, NULL);
                }
        }

        delete_registry_entry_list(entries);
}

// A datalink disappeared. Tell the local datahub to romve the link
// from its list.
static void proxy_remove_datalink(proxy_t* proxy, registry_entry_t *entry)
{
        list_t *list;
        registry_entry_t *e;
        
        list = registry_select(proxy->registry, 0, NULL, entry->topic,
                               TYPE_DATAHUB, NULL, NULL);
        if (list == NULL) return;
        
        e = list_get(list, registry_entry_t);
        if (proxy_is_local(e)) {
                datahub_t *hub = (datahub_t *) e->endpoint;
                datahub_remove_link(hub, entry->addr);
        }

        delete_registry_entry_list(list);
}

// A messagehub has disappeared. Tell the local links to disconnect from the hub.
static void proxy_remove_messagehub(proxy_t* proxy, registry_entry_t *entry)
{
        list_t *list;
        registry_entry_t *e;
        
        list = registry_select(proxy->registry, 0, NULL, entry->topic,
                               TYPE_MESSAGELINK, NULL, NULL);
        if (list == NULL) return;
        
        for (list_t *l = list; l != NULL; l = list_next(l)) {
                registry_entry_t *e = list_get(l, registry_entry_t);
                messagelink_t *link = (messagelink_t *) e->endpoint;
                int err = client_messagelink_disconnect(link);
                if (err != 0)
                        log_err("proxy_remove_messagehub: failed to disconnect.");
        }

        delete_registry_entry_list(list);
}

static void proxy_remove_connection(proxy_t* proxy, registry_entry_t *entry)
{
        switch (entry->type) {
        case TYPE_DATALINK: break;
                proxy_remove_datalink(proxy, entry);
                break;
        case TYPE_DATAHUB:
                proxy_remove_datahub(proxy, entry);
                break;
        case TYPE_MESSAGELINK: break;
        case TYPE_MESSAGEHUB: break;
                proxy_remove_messagehub(proxy, entry);
                break;
        }
}

static int proxy_send_register_request(proxy_t* proxy, registry_entry_t *entry)
{
        // TODO: register to remote registry
        int r;
        char b[64];
        r = messagelink_send_f(proxy->link,
                               "{\"request\": \"register\","
                               "\"entry\": {"
                               "\"id\": %d, "
                               "\"name\": \"%s\", "
                               "\"topic\": \"%s\", "
                               "\"type\": \"%s\", "
                               "\"addr\": \"%s\"}}",
                               entry->id, entry->name, entry->topic, 
                               registry_type_to_str(entry->type),
                               addr_string(entry->addr, b, 64));
        return r;
}

static int proxy_send_unregister_request(proxy_t* proxy, registry_entry_t *entry)
{
        return messagelink_send_f(proxy->link,
                                  "{\"request\": \"unregister\", \"id\": %d}",
                                  entry->id);
}

static int proxy_send_update_address_request(proxy_t* proxy, int id, const char *addr)
{
        return messagelink_send_f(proxy->link,
                                  "{\"request\": \"update-address\", "
                                  "\"id\": %d, \"addr\": \"%s\"}", id, addr);
}

static int proxy_send_list_request(proxy_t* proxy)
{
        return messagelink_send_f(proxy->link, "{\"request\": \"list\"}");
}

static registry_entry_t *proxy_new_entry(proxy_t *proxy,
                                         const char *name,
                                         const char *topic,
                                         int type,
                                         addr_t *addr,
                                         void *endpoint)
{
        registry_entry_t *entry;
        int r;
        
        if (!registry_valid_name(name)) {
                log_warn("proxy_new_entry: Invalid name");
                return NULL;
        }
        if (!registry_valid_topic(topic)) {
                log_warn("proxy_new_entry: Invalid topic name");
                return NULL;
        }
        
        entry = new_registry_entry(0, name, topic, type, addr, endpoint);
        if (entry == NULL)
                return NULL;
        
        mutex_lock(proxy->mutex);
        
        r = registry_insert_entry(proxy->registry, entry);

        mutex_unlock(proxy->mutex);

        if (r != 0) {
                delete_registry_entry(entry);
                return NULL;
        }

        r = proxy_send_register_request(proxy, entry);
        if (r != 0) {
                registry_delete(proxy->registry, entry->id);
                delete_registry_entry(entry);
                return NULL;
        }

        return entry;
}

static int proxy_delete_entry(proxy_t *proxy, int type, void *endpoint)
{
        registry_entry_t *entry = NULL;
        list_t *entries;
        
        mutex_lock(proxy->mutex);
        entries = registry_select(proxy->registry, 0, NULL, NULL, type, NULL, endpoint);
        if (entries) {
                entry = list_get(entries, registry_entry_t);
                registry_delete(proxy->registry, entry->id);
        }
        mutex_unlock(proxy->mutex);
        
        if (entry)
                proxy_send_unregister_request(proxy, entry);
        
        delete_registry_entry_list(entries);
        return 0;
}

static datalink_t *proxy_open_datalink(proxy_t *proxy,
                                       const char *name,
                                       const char *topic,
                                       datalink_ondata_t callback,
                                       void* userdata)
{
        datalink_t *link;
        registry_entry_t *entry;

        log_debug("proxy_open_datalink: %s:%s", name, topic);

        link = new_datalink(callback, userdata);
        if (link == NULL)
                return NULL;

        entry = proxy_new_entry(proxy, name, topic, TYPE_DATALINK,
                                datalink_addr(link), link);
        if (entry == NULL) {
                delete_datalink(link);
                return NULL;
        }

        mutex_lock(proxy->mutex);
        
        // Let's check whether there already is an open datahub
        list_t *entries = registry_select(proxy->registry, 0, NULL, entry->topic,
                                          TYPE_DATAHUB, NULL, NULL);
        if (entries != NULL) {
                registry_entry_t *e = list_get(entries, registry_entry_t);
                datalink_set_remote_addr(link, e->addr);
        } else
                log_debug("proxy_open_datalink: didn't find hub for topic %s", topic);

        mutex_unlock(proxy->mutex);

        delete_registry_entry_list(entries);
        
        return link;
}

static void proxy_close_datalink(proxy_t *proxy, datalink_t *link)
{
        proxy_delete_entry(proxy, TYPE_DATALINK, link);
        delete_datalink(link);
}

static datahub_t *proxy_open_datahub(proxy_t *proxy,
                                     const char *name,
                                     const char *topic,
                                     datahub_onbroadcast_t onbroadcast,
                                     datahub_ondata_t ondata,
                                     void* userdata)
{
        datahub_t *hub;
        registry_entry_t *entry;

        hub = new_datahub(onbroadcast, ondata, userdata);
        if (hub == NULL)
                return NULL;

        entry = proxy_new_entry(proxy, name, topic, TYPE_DATAHUB,
                                datahub_addr(hub), hub);
        if (entry == NULL) {
                delete_datahub(hub);
                return NULL;
        }

        mutex_lock(proxy->mutex);

        // Connect to the available datalinks
        list_t *entries = registry_select(proxy->registry, 0, NULL, entry->topic,
                                          TYPE_DATALINK, NULL, NULL);
        for (list_t *l = entries; l != NULL; l = list_next(l)) {
                registry_entry_t *e = list_get(l, registry_entry_t);
                datahub_add_link(hub, e->addr);
        }

        mutex_unlock(proxy->mutex);

        delete_registry_entry_list(entries);
        
        return hub;
}

static void proxy_close_datahub(proxy_t *proxy, datahub_t *hub)
{
        proxy_delete_entry(proxy, TYPE_DATAHUB, hub);
        delete_datahub(hub);
}

static messagelink_t *proxy_open_messagelink(proxy_t *proxy,
                                             const char *name,
                                             const char *topic,
                                             messagelink_onmessage_t onmessage,
                                             void *userdata)
{
        messagelink_t *link;
        registry_entry_t *entry;

        link = new_messagelink(onmessage, NULL, userdata);
        if (link == NULL)
                return NULL;

        entry = proxy_new_entry(proxy, name, topic, TYPE_MESSAGELINK,
                                messagelink_addr(link), link);
        if (entry == NULL) {
                delete_messagelink(link);
                return NULL;
        }

        mutex_lock(proxy->mutex);

        // Let's check whether there already is a message hub
        list_t *entries = registry_select(proxy->registry, 0, NULL, entry->topic,
                                          TYPE_MESSAGEHUB, NULL, NULL);
        if (entries != NULL) {
                registry_entry_t *e = list_get(entries, registry_entry_t);
                int err = client_messagelink_connect(link, e->addr);
                /* if (err == 0) */
                /*         messagelink_read_in_background(link); */
                /* else */
                /*         log_err("proxy_open_messagelink: failed to make the connection."); */
                if (err)
                        log_err("proxy_open_messagelink: failed to make the connection.");
        }

        mutex_unlock(proxy->mutex);

        delete_registry_entry_list(entries);

        return link;
}

static void proxy_close_messagelink(proxy_t *proxy, messagelink_t *link)
{
        proxy_delete_entry(proxy, TYPE_MESSAGELINK, link);
        delete_messagelink(link);
}

static messagehub_t *proxy_open_messagehub(proxy_t *proxy,
                                           const char *name,
                                           const char *topic,
                                           int port,
                                           messagehub_onconnect_t onconnect,
                                           void *userdata)
{
        messagehub_t *hub;
        registry_entry_t *entry;

        hub = new_messagehub(port, onconnect, userdata);
        if (hub == NULL)
                return NULL;

        entry = proxy_new_entry(proxy, name, topic, TYPE_MESSAGEHUB,
                                messagehub_addr(hub), hub); 
        if (entry == NULL) {
                delete_messagehub(hub);
                return NULL;
        }

        return hub;
}

static void proxy_close_messagehub(proxy_t *proxy, messagehub_t *hub)
{
        proxy_delete_entry(proxy, TYPE_MESSAGEHUB, hub);
        delete_messagehub(hub);
}

// service

static service_t *proxy_open_service(proxy_t *proxy, const char *name,
                                     const char *topic, int port)
{
        registry_entry_t *entry;
        
        service_t *service = new_service(name, port);
        if (service == NULL)
                return NULL;

        entry = proxy_new_entry(proxy, name, topic, TYPE_SERVICE,
                                service_addr(service), service); 
        if (entry == NULL) {
                delete_service(service);
                return NULL;
        }
        
        if (entry == NULL) {
                delete_service(service);
                return NULL;
        }

        return service;
}

static void proxy_close_service(proxy_t *proxy, service_t *service)
{
        proxy_delete_entry(proxy, TYPE_SERVICE, service);
        delete_service(service);
}

static addr_t *proxy_get_service(proxy_t *proxy, const char *topic)
{
        list_t *list;
        registry_entry_t *e;
        service_t *service;
        addr_t *addr = NULL;
        
        mutex_lock(proxy->mutex);
        
        list = registry_select(proxy->registry, 0, NULL, topic, TYPE_SERVICE, NULL, NULL);

        if (list) {
                e = list_get(list, registry_entry_t);
                service = (service_t *) e->endpoint;
                addr = service_addr(service);
        }
        
        mutex_unlock(proxy->mutex);
        
        return addr;
}

static streamer_t *proxy_open_streamer(proxy_t *proxy,
                                        const char *name,
                                        const char *topic,
                                        int port,
                                        streamer_onclient_t onclient,
                                        streamer_onbroadcast_t onbroadcast,
                                        void* userdata)
{
        streamer_t *streamer;
        registry_entry_t *entry;

        streamer = new_streamer(port, onclient, onbroadcast, userdata);
        if (streamer == NULL)
                return NULL;

        entry = proxy_new_entry(proxy, name, topic, TYPE_STREAMER,
                                streamer_addr(streamer), streamer); 
        if (entry == NULL) {
                delete_streamer(streamer);
                return NULL;
        }

        return streamer;
}

static void proxy_close_streamer(proxy_t *proxy, streamer_t *streamer)
{
        proxy_delete_entry(proxy, TYPE_STREAMER, streamer);
        delete_streamer(streamer);
}

static streamerlink_t *proxy_open_streamerlink(proxy_t *proxy,
                                                const char *name,
                                                const char *topic,
                                                const char *resource,
                                                streamerlink_ondata_t ondata,
                                                void* userdata)
{
        streamerlink_t *link;
        registry_entry_t *entry;

        link = new_streamerlink(resource, ondata, userdata);
        if (link == NULL)
                return NULL;

        entry = proxy_new_entry(proxy, name, topic, TYPE_STREAMERLINK,
                                streamerlink_addr(link), link); 
        if (entry == NULL) {
                delete_streamerlink(link);
                return NULL;
        }

        return link;
}

static void proxy_close_streamerlink(proxy_t *proxy, streamerlink_t *link)
{
        proxy_delete_entry(proxy, TYPE_STREAMERLINK, link);
        delete_streamerlink(link);
}

/*****************************************************************/


datalink_t *registry_open_datalink(const char *name,
                                   const char *topic,
                                   datalink_ondata_t ondata,
                                   void* userdata)
{
        proxy_t *proxy = proxy_get();
        if (proxy == NULL) return NULL;
        return proxy_open_datalink(proxy, name, topic, ondata, userdata);
}

void registry_close_datalink(datalink_t *link)
{
        if (link) {
                proxy_t *proxy = proxy_get();
                if (proxy == NULL) return;
                proxy_close_datalink(proxy, link);
        }
}

datahub_t *registry_open_datahub(const char *name,
                                 const char *topic,
                                 datahub_onbroadcast_t onbroadcast,
                                 datahub_ondata_t ondata,
                                 void* userdata)
{
        proxy_t *proxy = proxy_get();
        if (proxy == NULL) return NULL;
        return proxy_open_datahub(proxy, name, topic, onbroadcast, ondata, userdata);
}

void registry_close_datahub(datahub_t *hub)
{
        if (hub) {
                proxy_t *proxy = proxy_get();
                if (proxy == NULL) return;
                proxy_close_datahub(proxy, hub);
        }
}

messagelink_t *registry_open_messagelink(const char *name,
                                         const char *topic,
                                         messagelink_onmessage_t onmessage,
                                         void *userdata)
{
        proxy_t *proxy = proxy_get();
        if (proxy == NULL) return NULL;
        return proxy_open_messagelink(proxy, name, topic, onmessage, userdata);
}

void registry_close_messagelink(messagelink_t *link)
{
        if (link) {
                proxy_t *proxy = proxy_get();
                if (proxy == NULL) return;
                proxy_close_messagelink(proxy, link);
        }
}

messagehub_t *registry_open_messagehub(const char *name,
                                       const char *topic,
                                       int port,
                                       messagehub_onconnect_t onconnect,
                                       void *userdata)
{
        proxy_t *proxy = proxy_get();
        if (proxy == NULL) return NULL;
        return proxy_open_messagehub(proxy, name, topic, port, onconnect, userdata);
}

void registry_close_messagehub(messagehub_t *hub)
{
        if (hub) {
                proxy_t *proxy = proxy_get();
                if (proxy == NULL) return;
                return proxy_close_messagehub(proxy, hub);
        }
}

service_t *registry_open_service(const char *name, const char *topic, int port)
{
        proxy_t *proxy = proxy_get();
        if (proxy == NULL) return NULL;
        return proxy_open_service(proxy, name, topic, port);
}

void registry_close_service(service_t *service)
{
        if (service) {
                proxy_t *proxy = proxy_get();
                if (proxy == NULL) return;
                proxy_close_service(proxy, service);
        }
}

addr_t *registry_get_service(const char *topic)
{
        proxy_t *proxy = proxy_get();
        if (proxy == NULL) return NULL;
        return proxy_get_service(proxy, topic);
}

streamer_t *registry_open_streamer(const char *name,
                                   const char *topic,
                                   int port,
                                   streamer_onclient_t onclient,
                                   streamer_onbroadcast_t onbroadcast,
                                   void* userdata)
{
        proxy_t *proxy = proxy_get();
        if (proxy == NULL) return NULL;
        return proxy_open_streamer(proxy, name, topic, port, onclient, onbroadcast, userdata);
}

void registry_close_streamer(streamer_t *streamer)
{
        if (streamer) {
                proxy_t *proxy = proxy_get();
                if (proxy == NULL) return;
                proxy_close_streamer(proxy, streamer);
        }
}

streamerlink_t *registry_open_streamerlink(const char *name,
                                           const char *topic,
                                           const char *resource,
                                           streamerlink_ondata_t ondata,
                                           void* userdata)
{
        proxy_t *proxy = proxy_get();
        if (proxy == NULL) return NULL;
        return proxy_open_streamerlink(proxy, name, topic, resource, ondata, userdata);
}

void registry_close_streamerlink(streamerlink_t *link)
{
        if (link) {
                proxy_t *proxy = proxy_get();
                if (proxy == NULL) return;
                proxy_close_streamerlink(proxy, link);
        }
}

