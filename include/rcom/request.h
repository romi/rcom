#ifndef _RCOM_REQUEST_H_
#define _RCOM_REQUEST_H_

#include "rcom/addr.h"
#include "rcom/json.h"

#ifdef __cplusplus
extern "C" {
#endif
        
typedef struct _request_t request_t;

const char *request_uri(request_t *r);
const char *request_args(request_t *r);
addr_t *request_addr(request_t *r);
const char *request_data(request_t *r);
int request_len(request_t *r);
json_object_t request_parse(request_t *r);

int request_set_status(request_t *r, int status);
void request_set_mimetype(request_t *r, const char *mimetype);
int request_reply_append(request_t *r, const char *data, int len);
int request_reply_json(request_t *r, json_object_t obj);
int request_reply_printf(request_t *r, const char *format, ...);
        
#ifdef __cplusplus
}
#endif

#endif // _RCOM_REQUEST_H_
