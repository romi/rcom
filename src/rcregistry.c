#include <stdio.h>
#include <stdlib.h>

#include "rcom/app.h"
#include "rcom/log.h"

#include "messagehub_priv.h"
#include "messagelink_priv.h"
#include "registry_priv.h"
#include "util.h"
#include "mem.h"
#include "proxy.h"

typedef struct _rcregistry_t {
        registry_t *registry;
        messagehub_t *hub;
        int id;
} rcregistry_t;

static void _onmessage(messagelink_t *link, void *userdata, json_object_t message);
static void _onclose(messagelink_t *messagelink, void *userdata);
static int _onconnect(messagehub_t *hub, messagelink_t *link, void *userdata);

static void rcregistry_onmessage(rcregistry_t* rcregistry,
                                 messagelink_t *link,
                                 json_object_t message);
        
static void rcregistry_onclose(rcregistry_t* rcregistry, messagelink_t *link);

static int rcregistry_onconnect(rcregistry_t* rcregistry,
                                messagehub_t *hub,
                                messagelink_t *link);


rcregistry_t* new_rcregistry(addr_t *addr)
{
        int err;
        rcregistry_t* rcregistry;

        rcregistry = new_obj(rcregistry_t);
        if (rcregistry == NULL)
                return NULL;

        rcregistry->id = 1;
        
        rcregistry->registry = new_registry();
        if (rcregistry->registry == NULL) {
                log_err("Failed to create the registry. Quiting.");
                return 0;
        }
        
        rcregistry->hub = new_messagehub(get_registry_port(), _onconnect, rcregistry);
        if (rcregistry->hub == NULL) {
                log_err("Failed to create the registry hub. Quiting.");
                delete_registry(rcregistry->registry);
                return 0;
        }
        
        return rcregistry;
}

void delete_rcregistry(rcregistry_t* rcregistry)
{
        if (rcregistry) {
                if (rcregistry->registry)
                        delete_registry(rcregistry->registry);
                if (rcregistry->hub)
                        delete_messagehub(rcregistry->hub);
                delete_obj(rcregistry);
        }
}

addr_t *rcregistry_addr(rcregistry_t *rcregistry)
{
        return messagehub_addr(rcregistry->hub);
}

static int rcregistry_fail(messagelink_t *link, const char *req, const char *message)
{
        log_warn("rcregistry_fail: %s: %s", req, message);
        messagelink_send_f(link,
                           "{\"request\":\"%s\", "
                           "\"success\":false, "
                           "\"message\":\"%s\"}",
                           req, message);
}

static int rcregistry_success(messagelink_t *link, const char *req)
{
        messagelink_send_f(link, "{\"request\":\"%s\", \"success\":true}", req);
}

static void rcregistry_register(rcregistry_t* rcregistry,
                                messagelink_t *link,
                                json_object_t message)
{
        registry_entry_t *entry;
        int err;
        int id;

        json_object_t obj = json_object_get(message, "entry");
        if (json_isnull(obj)) {
                rcregistry_fail(link, "register-response", "Invalid entry value");
                return; 
        }
        
        entry = registry_entry_parse(obj, &err);
        switch (err) {
        case 0: break; 
        case -1: rcregistry_fail(link, "register-response", "Invalid name"); return; 
        case -2: rcregistry_fail(link, "register-response", "Invalid topic"); return; 
        case -3: rcregistry_fail(link, "register-response", "Invalid type"); return; 
        case -4: rcregistry_fail(link, "register-response", "Invalid address"); return; 
        case -5: rcregistry_fail(link, "register-response", "Invalid ID"); return; 
        default: rcregistry_fail(link, "register-response", "Unknown error"); return; 
        }

        id = rcregistry->id++;
        err = registry_insert(rcregistry->registry, id, entry->name,
                              entry->topic, entry->type, entry->addr, NULL);
        
        if (err != 0) {
                rcregistry_fail(link, "register-response", "Internal error");
                delete_registry_entry(entry);
                return;
        }
        
        err = rcregistry_success(link, "register-response");
        if (err != 0) {
                log_err("rcregistry_success returned an error");
        }

        // Broadcast new node
        char b[64];
        messagehub_send_f(rcregistry->hub, NULL, 
                          "{\"request\": \"proxy-add\","
                          "\"entry\": {" 
                          "\"id\": %d,"
                          "\"name\": \"%s\","
                          "\"topic\": \"%s\","
                          "\"type\": \"%s\","
                          "\"addr\": \"%s\"}}",
                          id, entry->name, entry->topic,
                          registry_type_to_str(entry->type),
                          addr_string(entry->addr, b, 64));

        delete_registry_entry(entry);
}

static void rcregistry_unregister(rcregistry_t* rcregistry,
                                  messagelink_t *link,
                                  json_object_t message)
{
        int err, id;
        double v;
        
        v = json_object_getnum(message, "id");
        if (isnan(v)) {
                log_err("rcregistry_unregister: Invalid ID"); 
                return;
        }
        id = (int) v;
        
        err = registry_delete(rcregistry->registry, id);
        if (err != 0) {
                rcregistry_fail(link, "unregister-response", "Internal error");
                return;
        }
        
        err = rcregistry_success(link, "unregister-response");
        if (err != 0)
                log_err("rcregistry_success returned an error");

        // Broadcast 
        messagehub_send_f(rcregistry->hub, NULL, 
                          "{\"request\": \"proxy-remove\", \"id\": %d}", id);
}

static void rcregistry_send_list(rcregistry_t* rcregistry, messagelink_t *link)
{
        list_t *entries = registry_select_all(rcregistry->registry);
        json_object_t list = registry_entry_encode_list(entries);
        json_object_t message = json_object_create();
        json_object_setstr(message, "request", "proxy-update-list");
        json_object_set(message, "list", list);
        
        messagelink_send_obj(link, message);
        
        json_unref(message);
        json_unref(list);
        delete_registry_entry_list(entries);
}

static void rcregistry_update_address(rcregistry_t* rcregistry,
                                      messagelink_t *link,
                                      json_object_t message)
{
        int err, id;
        double v;
        const char *addr;
        
        v = json_object_getnum(message, "id");
        if (isnan(v)) {
                log_err("rcregistry_update_address: Invalid ID"); 
                return;
        }
        id = (int) v;

        addr = json_object_getstr(message, "addr");
        if (addr == NULL) {
                log_err("rcregistry_update_address: Invalid address"); 
                return;
        }
        
        err = registry_update_addr(rcregistry->registry, id, addr);
        if (err != 0) {
                rcregistry_fail(link, "update-address-response", "Internal error");
                return;
        }
        
        err = rcregistry_success(link, "update-address-response");
        if (err != 0)
                log_err("rcregistry_success returned an error");
        
        // Broadcast update
        messagehub_send_f(rcregistry->hub, link, 
                          "{\"request\": \"proxy-update-address\","
                          "\"id\": %d, \"addr\": \"%s\"}", id, addr);
}

static void rcregistry_onmessage(rcregistry_t* rcregistry,
                                 messagelink_t *link,
                                 json_object_t message)
{
        log_debug("rcregistry: onmessage");
        
        const char *request;
        
        if (json_isnull(message)) {
                rcregistry_fail(link, "?", "Failed to parse the request");
                return;
        }
        
        request = json_object_getstr(message, "request");
        
        if (request == NULL) {
                rcregistry_fail(link, "?", "Invalid request");
                return;
        }
        
        log_debug("rcregistry_onmessage: request=%s", request);

        if (streq(request, "register")) {
                rcregistry_register(rcregistry, link, message);
                
        } else if (streq(request, "unregister")) {
                rcregistry_unregister(rcregistry, link, message);
                
        } else if (streq(request, "list")) {
                rcregistry_send_list(rcregistry, link);                
                
        } else if (streq(request, "update-address")) {
                rcregistry_update_address(rcregistry, link, message);                
                
        } else {
                log_warn("Unknown request: %s", request);
                rcregistry_fail(link, request, "Unknown request");
        }
}

static void rcregistry_onclose(rcregistry_t* rcregistry, messagelink_t *link)
{
        log_debug("rcregistry: onclose");

        // TODO: remove all links and hubs related to this link?!
}

static int rcregistry_onconnect(rcregistry_t* rcregistry,
                                messagehub_t *hub,
                                messagelink_t *link)
{
        log_debug("rcregistry: onconnect");
        messagelink_set_userdata(link, rcregistry);
        messagelink_set_onmessage(link, _onmessage);
        messagelink_set_onclose(link, _onclose);
        return 0;
}

/******************************************************/

static void _onmessage(messagelink_t *link, void *userdata, json_object_t message)
{
        rcregistry_t *rcregistry = (rcregistry_t *) userdata;
        rcregistry_onmessage(rcregistry, link, message);        
}

static void _onclose(messagelink_t *link, void *userdata)
{
        rcregistry_t *rcregistry = (rcregistry_t *) userdata;
        rcregistry_onclose(rcregistry, link);
}

static int _onconnect(messagehub_t *hub, messagelink_t *link, void *userdata)
{
        rcregistry_t *rcregistry = (rcregistry_t *) userdata;
        rcregistry_onconnect(rcregistry, hub, link);
        return 0;
}

