#ifndef _RCOM_REGISTRY_PRIV_H_
#define _RCOM_REGISTRY_PRIV_H_

#include "rcom/registry.h"
#include "rcom/addr.h"
#include "rcom/membuf.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
        TYPE_NONE = -2,
        TYPE_ANY = -1,
        TYPE_DATALINK = 0,
        TYPE_DATAHUB,
        TYPE_MESSAGELINK,
        TYPE_MESSAGEHUB,
        TYPE_SERVICE,
        TYPE_STREAMER,
        TYPE_STREAMERLINK
};

/**************************************************/
// registry entry
typedef struct _registry_entry_t
{
        char *id;
        //int id;
        char *name;
        char *topic;
        int type;
        addr_t *addr;
        void *endpoint;
} registry_entry_t;

registry_entry_t *new_registry_entry(const char *id, const char *name, const char *topic,
                                     int type, addr_t *addr, void *endpoint);
void delete_registry_entry(registry_entry_t *entry);

// Returns: 0: all ok, -1: invalid name, -2: invalid topic, -3: invalid type, -4: invalid addr, -5: invalid ID
registry_entry_t *registry_entry_parse(json_object_t obj, int *error);
json_object_t registry_entry_encode(registry_entry_t *e);
int registry_entry_equals(registry_entry_t* entry, registry_entry_t* e);

/**************************************************/
// list of entries
list_t *registry_entry_parse_list(json_object_t a);
json_object_t registry_entry_encode_list(list_t *list);
void delete_registry_entry_list(list_t *list);

/**************************************************/
// registry api
typedef struct _registry_t registry_t;

registry_t* new_registry();
void delete_registry(registry_t* registry);

int registry_insert(registry_t* registry, const char *id, const char *name,
                    const char *topic, int type, addr_t *addr, void *endpoint);
int registry_insert_entry(registry_t* registry, registry_entry_t *entry);

int registry_delete(registry_t* registry, const char *id);

registry_entry_t *registry_get(registry_t* registry, const char *id);

list_t *registry_select(registry_t* registry, const char *id, const char *name,
                        const char *topic, int type, addr_t *addr, void *endpoint);
list_t *registry_select_all(registry_t* registry);

int registry_count(registry_t* registry, const char *id, const char *name,
                   const char *topic, int type, addr_t *addr, void *endpoint);

int registry_geti(registry_t* registry, int i,
                  membuf_t *name, membuf_t *topic,
                  int *type, addr_t *addr);

int registry_update_addr(registry_t* registry, const char *id, const char *addr);
//int registry_update_id(registry_t* registry, int old_id, int new_id);

/**************************************************/
// global utility function
int registry_valid_id(const char *id);
int registry_valid_name(const char *name);
int registry_valid_topic(const char *topic);
int registry_str_to_type(const char* str);
const char *registry_type_to_str(int type);
addr_t *registry_valid_addr(const char *s);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_REGISTRY_PRIV_H_
