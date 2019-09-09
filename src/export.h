#ifndef _RCOM_EXPORT_H_
#define _RCOM_EXPORT_H_

#include "rcom/service.h"
#include "rcom/response.h"

#ifdef __cplusplus
extern "C" {
#endif
        
typedef struct _export_t export_t;
typedef struct _request_t request_t;

export_t* new_export(const char* name,
                     const char* mimetype_in,
                     const char* mimetype_out);
void delete_export(export_t* e);

const char* export_name(export_t* e);
const char* export_mimetype_in(export_t* e);
const char* export_mimetype_out(export_t* e);
int export_json_in(export_t* e);
int export_json_out(export_t* e);

int export_matches(export_t* e, const char *name);

export_t *export_clone(export_t* e);
int export_set_mimetypes(export_t* e,
                         const char* mimetype_in,
                         const char* mimetype_out);
void export_set_onrequest(export_t* e, void *userdata, service_onrequest_t onrequest);
void export_callback(export_t* e, request_t *request, response_t *response);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_EXPORT_H_
