#include <r.h>

#include "rcom/registry.h"
#include "rcom/client.h"
#include "http.h"

json_object_t client_get(const char *topic, const char *resource)
{
        response_t *r = NULL;
        int err = client_get_data(topic, resource, &r);
        if (err != 0) return json_null();
        
        membuf_t *out = response_body(r);
        membuf_append_zero(out);
        
        json_object_t obj = json_parse(membuf_data(out));

        delete_response(r);
        return obj;
}

int client_get_data(const char *topic, const char *resource, response_t **response_handle)
{
        addr_t *addr = registry_get_service(topic);
        if (addr == NULL) {
                r_err("client_get: couldn't find no service for topic '%s'", topic);
                return -1;
        }
        int err = http_get(addr, resource, response_handle);
        delete_addr(addr);
        return err;
}
