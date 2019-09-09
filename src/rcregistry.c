#include <stdio.h>
#include <stdlib.h>
#include <r.h>

#include "rcom/app.h"
#include "rcom/util.h"

#include "messagehub_priv.h"
#include "messagelink_priv.h"
#include "registry_priv.h"
#include "rcregistry_priv.h"
#include "proxy.h"

typedef struct _rcregistry_t {
        registry_t *registry;
        messagehub_t *hub;
} rcregistry_t;

static void _onmessage(void *userdata, messagelink_t *link, json_object_t message);
static void _onclose(void *userdata, messagelink_t *messagelink);
static int _onconnect(void *userdata, messagehub_t *hub,
                      request_t *request, messagelink_t *link);

void delete_rcregistry(rcregistry_t* rcregistry);

static void rcregistry_onmessage(rcregistry_t* rcregistry,
                                 messagelink_t *link,
                                 json_object_t message);
        
static void rcregistry_onclose(rcregistry_t* rcregistry, messagelink_t *link);

static int rcregistry_onconnect(rcregistry_t* rcregistry,
                                messagehub_t *hub,
                                messagelink_t *link);


rcregistry_t* new_rcregistry()
{
        int err;
        rcregistry_t* rcregistry;

        rcregistry = r_new(rcregistry_t);
        if (rcregistry == NULL)
                return NULL;

        rcregistry->registry = new_registry();
        if (rcregistry->registry == NULL) {
                r_err("Failed to create the registry. Quiting.");
                delete_rcregistry(rcregistry);
                return NULL;
        }
        
        rcregistry->hub = new_messagehub(get_registry_port(), _onconnect, rcregistry);
        if (rcregistry->hub == NULL) {
                r_err("Failed to create the registry hub. Quiting.");
                delete_rcregistry(rcregistry);
                return NULL;
        }
        
        addr_t *addr = messagehub_addr(rcregistry->hub);
        char *id = r_uuid();
        err = registry_insert(rcregistry->registry, id, "registry",
                              "registry", TYPE_MESSAGEHUB, addr, rcregistry->hub);
        r_free(id);
        if (err != 0) {
                r_err("Failed to insert the hub into the registry. Quiting.");
                delete_rcregistry(rcregistry);
                return NULL;
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
                r_delete(rcregistry);
        }
}

addr_t *rcregistry_addr(rcregistry_t *rcregistry)
{
        return messagehub_addr(rcregistry->hub);
}

static int rcregistry_fail(messagelink_t *link, const char *req, const char *message)
{
        r_warn("rcregistry_fail: %s: %s", req, message);
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

        err = registry_insert(rcregistry->registry, entry->id, entry->name,
                              entry->topic, entry->type, entry->addr, NULL);
        
        if (err != 0) {
                rcregistry_fail(link, "register-response", "Internal error");
                delete_registry_entry(entry);
                return;
        }
        
        err = rcregistry_success(link, "register-response");
        if (err != 0) {
                r_err("rcregistry_success returned an error");
        }

        r_info("successful registration %s %s:%s id=%s",
                 registry_type_to_str(entry->type), entry->name, entry->topic, entry->id);

        // Broadcast new node
        char b[64];
        messagehub_broadcast_f(rcregistry->hub, NULL, 
                               "{\"request\": \"proxy-add\","
                               "\"entry\": {" 
                               "\"id\": \"%s\","
                               "\"name\": \"%s\","
                               "\"topic\": \"%s\","
                               "\"type\": \"%s\","
                               "\"addr\": \"%s\"}}",
                               entry->id, entry->name, entry->topic,
                               registry_type_to_str(entry->type),
                               addr_string(entry->addr, b, 64));

        delete_registry_entry(entry);
}

static void rcregistry_unregister(rcregistry_t* rcregistry,
                                  messagelink_t *link,
                                  json_object_t message)
{
        int err;
        const char *id;
        
        id = json_object_getstr(message, "id");
        if (id == NULL) {
                r_err("rcregistry_unregister: Invalid ID"); 
                return;
        }
        
        err = registry_delete(rcregistry->registry, id);
        if (err != 0) {
                rcregistry_fail(link, "unregister-response", "Internal error");
                return;
        }
        
        err = rcregistry_success(link, "unregister-response");
        if (err != 0)
                r_err("rcregistry_success returned an error");

        r_info("successful unregistration: id=%s", id); 

        // Broadcast 
        messagehub_broadcast_f(rcregistry->hub, NULL, 
                               "{\"request\": \"proxy-remove\", \"id\": \"%s\"}", id);
}

static void rcregistry_send_list(rcregistry_t* rcregistry, messagelink_t *link)
{
        list_t *entries = registry_select_all(rcregistry->registry);
        json_object_t list = registry_entry_encode_list(entries);
        json_object_t message = json_object_create();
        json_object_setstr(message, "request", "proxy-update-list");
        json_object_set(message, "list", list);
        json_unref(list);

        messagelink_send_obj(link, message);
        
        json_unref(message);
        delete_registry_entry_list(entries);
}

static void rcregistry_update_address(rcregistry_t* rcregistry,
                                      messagelink_t *link,
                                      json_object_t message)
{
        int err;
        const char *id;
        const char *addr;
        
        id = json_object_getstr(message, "id");
        if (id == NULL) {
                r_err("rcregistry_update_address: Invalid ID"); 
                return;
        }

        addr = json_object_getstr(message, "addr");
        if (addr == NULL) {
                r_err("rcregistry_update_address: Invalid address"); 
                return;
        }
        
        err = registry_update_addr(rcregistry->registry, id, addr);
        if (err != 0) {
                rcregistry_fail(link, "update-address-response", "Internal error");
                return;
        }
        
        err = rcregistry_success(link, "update-address-response");
        if (err != 0)
                r_err("rcregistry_success returned an error");
        
        // Broadcast update
        messagehub_broadcast_f(rcregistry->hub, link, 
                               "{\"request\": \"proxy-update-address\","
                               "\"id\": \"%s\", \"addr\": \"%s\"}", id, addr);
}

static void rcregistry_onmessage(rcregistry_t* rcregistry,
                                 messagelink_t *link,
                                 json_object_t message)
{
        //r_debug("rcregistry: onmessage");
        
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
        
        //r_debug("rcregistry_onmessage: request=%s", request);

        if (rstreq(request, "register")) {
                rcregistry_register(rcregistry, link, message);
                
        } else if (rstreq(request, "unregister")) {
                rcregistry_unregister(rcregistry, link, message);
                
        } else if (rstreq(request, "list")) {
                rcregistry_send_list(rcregistry, link);                
                
        } else if (rstreq(request, "update-address")) {
                rcregistry_update_address(rcregistry, link, message);                
                
        } else {
                r_warn("Unknown request: %s", request);
                rcregistry_fail(link, request, "Unknown request");
        }
}

static void rcregistry_onclose(rcregistry_t* rcregistry, messagelink_t *link)
{
        //r_debug("rcregistry: onclose");

        // TODO: remove all links and hubs related to this link?!
}

static int rcregistry_onconnect(rcregistry_t* rcregistry,
                                messagehub_t *hub,
                                messagelink_t *link)
{
        //r_debug("rcregistry: onconnect");
        messagelink_set_userdata(link, rcregistry);
        messagelink_set_onmessage(link, _onmessage);
        messagelink_set_onclose(link, _onclose);
        return 0;
}

/******************************************************/

static void _onmessage(void *userdata, messagelink_t *link, json_object_t message)
{
        rcregistry_t *rcregistry = (rcregistry_t *) userdata;
        rcregistry_onmessage(rcregistry, link, message);        
}

static void _onclose(void *userdata, messagelink_t *link)
{
        rcregistry_t *rcregistry = (rcregistry_t *) userdata;
        rcregistry_onclose(rcregistry, link);
}

static int _onconnect(void *userdata, messagehub_t *hub,
                      request_t *request, messagelink_t *link)
{
        rcregistry_t *rcregistry = (rcregistry_t *) userdata;
        rcregistry_onconnect(rcregistry, hub, link);
        return 0;
}

