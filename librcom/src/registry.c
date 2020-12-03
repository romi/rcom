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

#include "addr.h"
#include "registry_priv.h"

int registry_str_to_type(const char* str)
{
        if (rstreq(str, "datalink"))
                return TYPE_DATALINK;
        if (rstreq(str, "datahub"))
                return TYPE_DATAHUB;
        if (rstreq(str, "messagelink"))
                return TYPE_MESSAGELINK;
        if (rstreq(str, "messagehub"))
                return TYPE_MESSAGEHUB;
        if (rstreq(str, "service"))
                return TYPE_SERVICE;
        if (rstreq(str, "streamer"))
                return TYPE_STREAMER;
        if (rstreq(str, "streamerlink"))
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
                r_err("new_registry_entry: invalid entry data "
                        "(name=%s, topic=%s, addr=%s, type=%d)",
                        name, topic, b, type);
                return NULL;
        }
        
        registry_entry_t* entry = r_new(registry_entry_t);
        if (entry == NULL)
                return NULL;

        entry->id = (id == NULL)? r_uuid() : r_strdup(id);
        entry->name = r_strdup(name);
        entry->topic = r_strdup(topic);
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
                        r_free(entry->id);
                if (entry->name)
                        r_free(entry->name);
                if (entry->topic)
                        r_free(entry->topic);
                if (entry->addr)
                        delete_addr(entry->addr);
                // ToDo: This isn't created by the new fubction so we can't delete it can we?
//                if (entry->endpoint)
//                        ; // FIXME!
                r_delete(entry);
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
        
        registry_entry_t *entry = new_registry_entry(id, name, topic, type, addr, NULL);
        if (entry == NULL) {
                delete_addr(addr);
                *error = -6;
                return NULL;
        }
        
        delete_addr(addr);
        
        *error = 0;
        return entry;
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
                r_err("registry_entry_parse_list: not an array");
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
                        case -1: r_err("registry_entry_parse_list: Invalid name"); break; 
                        case -2: r_err("registry_entry_parse_list: Invalid topic"); break; 
                        case -3: r_err("registry_entry_parse_list: Invalid type"); break; 
                        case -4: r_err("registry_entry_parse_list: Invalid address"); break; 
                        case -5: r_err("registry_entry_parse_list: Invalid ID"); break; 
                        default: r_err("registry_entry_parse_list: Unknown error"); break; 
                        }
                        if (entry) delete_registry_entry(entry);
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
                json_unref(obj);
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
        if (len < 2 || len > 256)
                return 0;
        if (strchr("abcdefghijklmnopqrstuvwxyz", topic[0]) == NULL)
                return 0;
        for (int i = 1; i < len; i++) {
                if (strchr("abcdefghijklmnopqrstuvwxyz.-_", topic[i]) == NULL)
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
                if (strchr("abcdefghijklmnopqrstuvwxyz0123456789-_", name[i]) == NULL)
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

        registry = r_new(registry_t);
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
                r_delete(registry);
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
                if (rstreq(e->id, id)) {
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
                if ((id == NULL || rstreq(e->id, id))
                    && (type == TYPE_ANY || e->type == type)
                    && (name == NULL || rstreq(e->name, name))
                    && (topic == NULL || rstreq(e->topic, topic))
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
                if ((id == NULL || rstreq(e->id, id))
                    && (type == TYPE_ANY || e->type == type)
                    && (name == NULL || rstreq(e->name, name))
                    && (topic == NULL || rstreq(e->topic, topic))
                    && (addr == NULL || addr_eq(e->addr, addr))
                    && (endpoint == NULL || endpoint == e->endpoint)) {
                        count++;
                }
        }
        registry_unlock(registry);
        return count;
}


int registry_geti(registry_t* registry, int n,
                  membuf_t *name, membuf_t *topic,
                  int *type, addr_t *addr)
{
        list_t* l;
        registry_entry_t *e;
        
        registry_lock(registry);
        l = list_nth(registry->entries, n);
        if (l) {
                e = list_get(l, registry_entry_t);
                
                membuf_clear(name);
                membuf_append(name, e->name, strlen(e->name));
                membuf_append_zero(name);
                
                membuf_clear(topic);
                membuf_append(topic, e->topic, strlen(e->topic));
                membuf_append_zero(topic);
                
                addr_set(addr, "0.0.0.0", 0);
                if (e->addr) addr_copy(e->addr, addr);
                
                *type = e->type;
        }
        registry_unlock(registry);
        
        return 0;
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

        //r_debug("registry_add %s %s:%s", registry_type_to_str(type), name, topic);
        
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

// ToDo: This seems wrong having two functions. We could use a recursive mutex.
// ToDo: Just one lock function needed as anything calling it already has access to the lock.
static int registry_remove_entry_locked(registry_t* registry, registry_entry_t *entry)
{
        registry->entries = list_remove(registry->entries, entry);
        return 0;
}

// ToDo: See above. Remove this function and rename the above.
__attribute__((unused))
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
        
        //r_debug("registry_delete id=%s", id);

        mutex_lock(registry->mutex);
        list_t *l = registry->entries;
        while (l) {
                registry_entry_t *e = list_get(l, registry_entry_t);
                if (rstreq(e->id, id)) {
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
                if (rstreq(id, e->id)) {
                        if (e->addr != NULL)
                                delete_addr(e->addr);
                        e->addr = addr_parse(addr);
                        if (e->addr == NULL) {
                                err = -1;
                                r_err("fixme_registry_update_addr: invalid addr");
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

/*         r_debug("registry_update_id: %d -> %d", old_id, new_id); */
        
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


