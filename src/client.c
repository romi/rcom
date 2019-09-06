#include <r.h>

#include "rcom/registry.h"
#include "rcom/client.h"
#include "http.h"

json_object_t client_get(const char *topic, const char *resource)
{
        membuf_t *out = new_membuf();

        int err = client_get_data(topic, resource, out);
        if (err != 0) return json_null();
        membuf_append_zero(out);
        
        json_object_t obj = json_parse(membuf_data(out));

        delete_membuf(out);
        return obj;
}

int client_get_data(const char *topic, const char *resource, membuf_t *out)
{
        addr_t *addr = registry_get_service(topic);
        if (addr == NULL) {
                r_err("client_get: couldn't find no service for topic '%s'", topic);
                return -1;
        }
        int r = http_get(addr, resource, out);
        delete_addr(addr);
        return r;
}
