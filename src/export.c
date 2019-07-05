#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "mem.h"
#include "util.h"
#include "export.h"

typedef struct _export_t
{
        char* name;
        char* mimetype_in;
        char* mimetype_out;
        //int registered;
        
        // Used by service
        void *userdata;
        service_onrequest_t onrequest;
} export_t;

export_t* new_export(const char* name,
                     const char* mimetype_in,
                     const char* mimetype_out)
{
        export_t* e = new_obj(export_t);
        if (e == NULL)
                return NULL;
        
        e->name = mem_strdup(name);
        if (e->name == NULL) {
                delete_export(e);
                return NULL;
        }

        if (mimetype_in) {
                e->mimetype_in = mem_strdup(mimetype_in);
                if (e->mimetype_in == NULL) {
                        delete_export(e);
                        return NULL;
                }
        }
        if (mimetype_out) {
                e->mimetype_out = mem_strdup(mimetype_out);
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
                        mem_free(e->name);
                if (e->mimetype_in)
                        mem_free(e->mimetype_in);
                if (e->mimetype_out)
                        mem_free(e->mimetype_out);
                delete_obj(e);
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

int export_callback(export_t* e, request_t *request)
{
        if (!e->onrequest)
                return -1;
        return e->onrequest(e->userdata, request);
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
        return e->mimetype_in != NULL && streq(e->mimetype_in, "application/json");
}

int export_json_out(export_t* e)
{
        return e->mimetype_out != NULL && streq(e->mimetype_out, "application/json");
}

/* int export_registered(export_t* e) */
/* { */
/*         return e->registered; */
/* } */

/* void export_set_registered(export_t* e, int val) */
/* { */
/*         e->registered = val; */
/* } */
