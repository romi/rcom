#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rcom/log.h"
#include "rcom/app.h"
#include "rcom/addr.h"

#include "datalink_priv.h"
#include "datahub_priv.h"
#include "messagehub_priv.h"
#include "messagelink_priv.h"
#include "registry_priv.h"
#include "mem.h"
#include "list.h"
#include "util.h"
#include "thread.h"

int registry_str_to_type(const char* str)
{
        if (streq(str, "datalink"))
                return TYPE_DATALINK;
        if (streq(str, "datahub"))
                return TYPE_DATAHUB;
        if (streq(str, "messagelink"))
                return TYPE_MESSAGELINK;
        if (streq(str, "messagehub"))
                return TYPE_MESSAGEHUB;
        if (streq(str, "service"))
                return TYPE_SERVICE;
        if (streq(str, "streamer"))
                return TYPE_STREAMER;
        if (streq(str, "streamerlink"))
                return TYPE_STREAMERLINK;
        return TYPE_NONE;
}

const char *registry_type_to_str(int type)
{
        switch (type) {
        case TYPE_DATALINK: return "datalink";
        case TYPE_DATAHUB: return "datahub";
        case TYPE_MESSAGELINK: return "messagelink";
        case TYPE_MESSAGEHUB: return "messagehub";
        case TYPE_SERVICE: return "service";
        case TYPE_STREAMER: return "streamer";
        case TYPE_STREAMERLINK: return "streamerlink";
        default: return "unknown";
        }
}

/***********************************************************************/

registry_entry_t *new_registry_entry(const char *id, const char *name, const char *topic,
                                     int type, addr_t *addr, void *endpoint)
{
        if (name == NULL || topic == NULL || addr == NULL || type < 0) {
                char b[64];
                addr_string(addr, b, 64);
                log_err("new_registry_entry: invalid entry data "
                        "(name=%s, topic=%s, addr=%s, type=%d)",
                        name, topic, b, type);
                return NULL;
        }
        
        registry_entry_t* entry = new_obj(registry_entry_t);
        if (entry == NULL)
                return NULL;

        entry->id = (id == NULL)? generate_uuid() : mem_strdup(id);
        entry->name = mem_strdup(name);
        entry->topic = mem_strdup(topic);
        entry->type = type;
        entry->addr = addr_clone(addr);
        entry->endpoint = endpoint;

        if (entry->id == NULL || entry->name == NULL
            || entry->topic == NULL || entry->addr == NULL) {
                delete_registry_entry(entry);
                return NULL;
        }
        
        return entry;
}

void delete_registry_entry(registry_entry_t *entry)
{
        if (entry) {
                if (entry->id)
                        mem_free(entry->id);
                if (entry->name)
                        mem_free(entry->name);
                if (entry->topic)
                        mem_free(entry->topic);
                if (entry->addr)
                        delete_addr(entry->addr);
                if (entry->endpoint)
                        ; // FIXME!
                delete_obj(entry);
        }
}

registry_entry_t *registry_entry_parse(json_object_t obj, int *error)
{
        const char *id;
        const char *name;
        const char *topic;
        const char *type_str;
        const char *addr_str;
        addr_t *addr;
        int type;
        int err;
        
        id = json_object_getstr(obj, "id");
        name = json_object_getstr(obj, "name");
        topic = json_object_getstr(obj, "topic");
        type_str = json_object_getstr(obj, "type");
        addr_str = json_object_getstr(obj, "addr");

        if (!registry_valid_id(id)) {
                *error = -5;
                return NULL;
        }
        if (!registry_valid_name(name)) {
                *error = -1;
                return NULL;
        }

        if (!registry_valid_topic(topic)) {
                *error = -2;
                return NULL;
        }
        
        if (type_str == NULL) {
                *error = -3;
                return NULL;
        }
        type = registry_str_to_type(type_str);
        if (type == TYPE_NONE) {
                *error = -3;
                return NULL;
        }
        
        if (addr_str == NULL) {
                *error = -4;
                return NULL;
        }
        addr = addr_parse(addr_str);
        if (addr == NULL) {
                *error = -4;
                return NULL;
        }

        *error = 0;
        return new_registry_entry(id, name, topic, type, addr, NULL);
}

json_object_t registry_entry_encode(registry_entry_t *e)
{
        char b[64];
        json_object_t obj = json_object_create();
        json_object_setstr(obj, "id", e->id);
        json_object_setstr(obj, "name", e->name);
        json_object_setstr(obj, "topic", e->topic);
        json_object_setstr(obj, "type", registry_type_to_str(e->type));
        json_object_setstr(obj, "addr", addr_string(e->addr, b, 64));
        return obj;
}

/********************************************************/

void delete_registry_entry_list(list_t *list)
{
        for (list_t *l = list; l != NULL; l = list_next(l)) {
                registry_entry_t *e = list_get(l, registry_entry_t);
                delete_registry_entry(e);
        }
        delete_list(list);
}

list_t *registry_entry_parse_list(json_object_t a)
{
        list_t *list = NULL;
        
        if (!json_isarray(a)) {
                log_err("registry_entry_parse_list: not an array");
                return NULL;
        }
        for (int i = 0; i < json_array_length(a); i++) {
                int err;
                json_object_t obj = json_array_get(a, i);
                registry_entry_t *entry = registry_entry_parse(obj, &err);
                if (err == 0) {
                        list = list_prepend(list, entry);
                } else {
                        switch (err) {
                        case 0: break; 
                        case -1: log_err("registry_entry_parse_list: Invalid name"); break; 
                        case -2: log_err("registry_entry_parse_list: Invalid topic"); break; 
                        case -3: log_err("registry_entry_parse_list: Invalid type"); break; 
                        case -4: log_err("registry_entry_parse_list: Invalid address"); break; 
                        case -5: log_err("registry_entry_parse_list: Invalid ID"); break; 
                        default: log_err("registry_entry_parse_list: Unknown error"); break; 
                        }
                        delete_registry_entry_list(list);
                        return NULL;
                }
        }

        return list;
}

json_object_t registry_entry_encode_list(list_t *list)
{
        json_object_t a = json_array_create();
        for (list_t *l = list;
             l != NULL;
             l = list_next(l)) {
                registry_entry_t *e = list_get(l, registry_entry_t);
                json_object_t obj = registry_entry_encode(e);
                json_array_push(a, obj);
        }
        return a;
}

/********************************************************/

int registry_valid_id(const char *id)
{
        const char *p = id;
        int i, j;
        
        if (id == NULL)
                return 0;
        int len = strlen(id);
        if (len != 36)
                return 0;
        
        // format: 1b4e28ba-2fa1-11d2-883f-0016d3cca427
        
        for (i = 0; i < 8; i++) 
                if (strchr("0123456789abcdef", *p++) == NULL)
                        return 0;
        if (*p++ != '-') return 0;
        
        for (j = 0; j < 3; j++) {
                for (i = 0; i < 4; i++) 
                        if (strchr("0123456789abcdef", *p++) == NULL)
                                return 0;
                if (*p++ != '-') return 0;
        }
        
        for (i = 0; i < 12; i++) 
                if (strchr("0123456789abcdef", *p++) == NULL)
                        return 0;
        
        return 1;
}

// TODO: improve
int registry_valid_topic(const char *topic)
{
        if (topic == NULL)
                return 0;
        int len = strlen(topic);
        if (len < 4 || len > 256)
                return 0;
        if (strchr("abcdefghijklmnopqrstuvwxyz", topic[0]) == NULL)
                return 0;
        for (int i = 1; i < len; i++) {
                if (strchr("abcdefghijklmnopqrstuvwxyz.-", topic[i]) == NULL)
                        return 0;
        }
        return 1;
}

// TODO: improve
int registry_valid_name(const char *name)
{
        if (name == NULL)
                return 0;
        int len = strlen(name);
        if (len < 4 || len > 256)
                return 0;
        if (strchr("abcdefghijklmnopqrstuvwxyz", name[0]) == NULL)
                return 0;
        for (int i = 1; i < len; i++) {
                if (strchr("abcdefghijklmnopqrstuvwxyz-_", name[i]) == NULL)
                        return 0;
        }
        return 1;
}

/********************************************************/

typedef struct _registry_t
{
        list_t *entries;
        messagelink_t *link;
        mutex_t *mutex;
} registry_t;

static void registry_lock(registry_t* registry);
static void registry_unlock(registry_t* registry);

registry_t* new_registry()
{
        registry_t* registry;

        registry = new_obj(registry_t);
        if (registry == NULL)
                return NULL;

        registry->mutex = new_mutex();        
        return registry;
}

void delete_registry(registry_t* registry)
{
        if (registry) {
                if (registry->mutex) {
                        mutex_lock(registry->mutex);
                        delete_registry_entry_list(registry->entries);
                        registry->entries = NULL;
                        mutex_unlock(registry->mutex);
                        delete_mutex(registry->mutex);
                }
                delete_obj(registry);
        }
}

static void registry_lock(registry_t* registry)
{
        mutex_lock(registry->mutex);
}

static void registry_unlock(registry_t* registry)
{
        mutex_unlock(registry->mutex);
}

registry_entry_t *registry_get(registry_t* registry, const char *id)
{
        registry_entry_t *clone = NULL;
        registry_entry_t *e;

        registry_lock(registry);
        for (list_t *l = registry->entries; l != NULL; l = list_next(l)) {
                e = list_get(l, registry_entry_t);
                if (streq(e->id, id)) {
                        clone = new_registry_entry(e->id, e->name, e->topic, e->type,
                                                   e->addr, e->endpoint);
                        break;
                }
        }
        registry_unlock(registry);
        return clone;
}

list_t *registry_select(registry_t* registry, const char *id, const char *name,
                        const char *topic, int type, addr_t *addr, void *endpoint)
{
        list_t *results = NULL;
        registry_entry_t *e;

        registry_lock(registry);
        for (list_t *l = registry->entries; l != NULL; l = list_next(l)) {
                e = list_get(l, registry_entry_t);
                if ((id == NULL || streq(e->id, id))
                    && (type == TYPE_ANY || e->type == type)
                    && (name == NULL || streq(e->name, name))
                    && (topic == NULL || streq(e->topic, topic))
                    && (addr == NULL || addr_eq(e->addr, addr))
                    && (endpoint == NULL || endpoint == e->endpoint)) {
                        registry_entry_t *clone;
                        clone = new_registry_entry(e->id, e->name, e->topic, e->type,
                                                   e->addr, e->endpoint);
                        results = list_prepend(results, clone);
                }
        }
        registry_unlock(registry);
        return results;
}

list_t *registry_select_all(registry_t* registry)
{
        return registry_select(registry, 0, NULL, NULL, TYPE_ANY, NULL, NULL);
}

int registry_count(registry_t* registry, const char *id, const char *name,
                   const char *topic, int type, addr_t *addr, void *endpoint)
{
        int count = 0;
        registry_entry_t *e;

        registry_lock(registry);
        for (list_t *l = registry->entries; l != NULL; l = list_next(l)) {
                e = list_get(l, registry_entry_t);
                if ((id == NULL || streq(e->id, id))
                    && (type == TYPE_ANY || e->type == type)
                    && (name == NULL || streq(e->name, name))
                    && (topic == NULL || streq(e->topic, topic))
                    && (addr == NULL || addr_eq(e->addr, addr))
                    && (endpoint == NULL || endpoint == e->endpoint)) {
                        count++;
                }
        }
        registry_unlock(registry);
        return count;
}

static int registry_add_entry_locked(registry_t* registry, registry_entry_t *entry)
{
        registry->entries = list_append(registry->entries, entry);
        return (registry->entries == NULL)? -1 : 0;
}

static int registry_add_entry(registry_t* registry, registry_entry_t *entry)
{
        int r;
        mutex_lock(registry->mutex);
        r = registry_add_entry_locked(registry, entry);
        mutex_unlock(registry->mutex);
        return r;
}

int registry_insert(registry_t* registry, const char *id, const char *name,
                    const char *topic, int type, addr_t *addr, void *endpoint)
{
        registry_entry_t *entry;

        log_debug("registry_add %s:%s", name, topic);
        
        entry = new_registry_entry(id, name, topic, type, addr, endpoint);
        if (entry == NULL)
                return -1;
        
        int r = registry_add_entry(registry, entry);
        if (r != 0) {
                delete_registry_entry(entry);
                return -1;
        }

        return 0;
}

int registry_insert_entry(registry_t* registry, registry_entry_t *entry)
{
        return registry_insert(registry, entry->id, entry->name, entry->topic,
                               entry->type, entry->addr, entry->endpoint);
}

static int registry_remove_entry_locked(registry_t* registry, registry_entry_t *entry)
{
        registry->entries = list_remove(registry->entries, entry);
        return 0;
}
        
static int registry_remove_entry(registry_t* registry, registry_entry_t *entry)
{
        int r;
        mutex_lock(registry->mutex);
        r = registry_remove_entry_locked(registry, entry);
        mutex_unlock(registry->mutex);
        return r;
}

int registry_delete(registry_t* registry, const char *id)
{
        int ret = -1;
        
        log_debug("registry_delete id=%s", id);

        mutex_lock(registry->mutex);
        list_t *l = registry->entries;
        while (l) {
                registry_entry_t *e = list_get(l, registry_entry_t);
                if (streq(e->id, id)) {
                        ret = registry_remove_entry_locked(registry, e);
                        delete_registry_entry(e);
                        break;
                }
                l = list_next(l);
        }
        mutex_unlock(registry->mutex);
        return ret;
}

int registry_update_addr(registry_t* registry, const char *id, const char *addr)
{
        int err = 0;
        registry_entry_t *e;

        registry_lock(registry);
        for (list_t *l = registry->entries; l != NULL; l = list_next(l)) {
                e = list_get(l, registry_entry_t);
                if (streq(id, e->id)) {
                        if (e->addr != NULL)
                                delete_addr(e->addr);
                        e->addr = addr_parse(addr);
                        if (e->addr == NULL) {
                                err = -1;
                                log_err("fixme_registry_update_addr: invalid addr");
                        }
                        break;
                }
        }
        registry_unlock(registry);
        return err;        
}

/* int registry_update_id(registry_t* registry, int old_id, int new_id) */
/* { */
/*         int err = 0; */
/*         registry_entry_t *e; */

/*         log_debug("registry_update_id: %d -> %d", old_id, new_id); */
        
/*         registry_lock(registry); */
/*         for (list_t *l = registry->entries; l != NULL; l = list_next(l)) { */
/*                 e = list_get(l, registry_entry_t); */
/*                 if (e->id == old_id) { */
/*                         e->id = new_id; */
/*                         break; */
/*                 } */
/*         } */
/*         registry_unlock(registry); */
/*         return err;         */
/* } */


