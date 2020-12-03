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

#include "registry.h"
#include "client.h"
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
