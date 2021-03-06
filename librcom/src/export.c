/*
  rcutil

  Copyright (C) 2019 Sony Computer Science Laboratories
  Author(s) Peter Hanappe

  rcutil is light-weight libary for inter-node communication.

  rcutil is free software: you can redistribute it and/or modify it
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
#include "util.h"
#include "export.h"

typedef struct _export_t
{
        char* name;
        char* mimetype_in;
        char* mimetype_out;
        
        // Used by service
        void *userdata;
        service_onrequest_t onrequest;
} export_t;

export_t* new_export(const char* name,
                     const char* mimetype_in,
                     const char* mimetype_out)
{
        export_t* e = r_new(export_t);
        if (e == NULL)
                return NULL;
        
        e->name = r_strdup(name);
        if (e->name == NULL) {
                delete_export(e);
                return NULL;
        }

        if (mimetype_in) {
                e->mimetype_in = r_strdup(mimetype_in);
                if (e->mimetype_in == NULL) {
                        delete_export(e);
                        return NULL;
                }
        }
        if (mimetype_out) {
                e->mimetype_out = r_strdup(mimetype_out);
                if (e->mimetype_out == NULL) {
                        delete_export(e);
                        return NULL;
                }
        }
        
        return e;
}

void delete_export(export_t* e)
{
        if (e) {
                if (e->name)
                        r_free(e->name);
                if (e->mimetype_in)
                        r_free(e->mimetype_in);
                if (e->mimetype_out)
                        r_free(e->mimetype_out);
                r_delete(e);
        }
}

// FIXME: needed by service::request_handle only
export_t *export_clone(export_t* e)
{
        export_t *n = new_export(e->name, e->mimetype_in, e->mimetype_out);
        n->userdata = e->userdata;
        n->onrequest = e->onrequest;
        return n;
}

void export_set_onrequest(export_t* e,
                          void *userdata,
                          service_onrequest_t onrequest)
{
        e->userdata = userdata;
        e->onrequest = onrequest;
}

int export_set_mimetypes(export_t* e,
                          const char* mimetype_in,
                          const char* mimetype_out)
{
        if (e->mimetype_in) {
                r_free(e->mimetype_in);
                e->mimetype_in = NULL;
        }
        if (e->mimetype_out) {
                r_free(e->mimetype_out);
                e->mimetype_out = NULL;
        }
        if (mimetype_in) {
                e->mimetype_in = r_strdup(mimetype_in);
                if (e->mimetype_in == NULL)
                        return -1;
        }
        if (mimetype_out) {
                e->mimetype_out = r_strdup(mimetype_out);
                if (e->mimetype_out == NULL)
                        return -1;
        }
        return 0;
}

void export_callback(export_t* e, request_t *request, response_t *response)
{
        if (!e->onrequest)
                response_set_status(response, HTTP_Status_Internal_Server_Error);
        else
                e->onrequest(e->userdata, request, response);
}

const char* export_name(export_t* e)
{
        return e->name;
}

const char* export_mimetype_out(export_t* e)
{
        return e->mimetype_out;
}

const char* export_mimetype_in(export_t* e)
{
        return e->mimetype_in;
}

int export_json_in(export_t* e)
{
        return e->mimetype_in != NULL && rstreq(e->mimetype_in, "application/json");
}

int export_json_out(export_t* e)
{
        return e->mimetype_out != NULL && rstreq(e->mimetype_out, "application/json");
}

int export_matches(export_t* e, const char *name)
{
        return rstreq(name, e->name)
                || (name[0] == '/' && rstreq(name+1, e->name));
}
