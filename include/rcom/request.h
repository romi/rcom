#ifndef _RCOM_REQUEST_H_
#define _RCOM_REQUEST_H_

#include <r.h>
#include "rcom/addr.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
        HTTP_METHOD_UNSUPPORTED,
        HTTP_METHOD_GET,
        HTTP_METHOD_POST
};

typedef struct _request_t request_t;

const char *request_uri(request_t *r);
const char *request_args(request_t *r);
const char *request_body(request_t *r);
int request_length(request_t *r);
json_object_t request_parse(request_t *r);

const char *request_get_header_value(request_t *request, const char *key);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_REQUEST_H_
