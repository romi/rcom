#ifndef _RCOM_SERVICE_H_
#define _RCOM_SERVICE_H_

#include "rcom/request.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _service_t service_t;

typedef int (*service_onrequest_t)(void *data, request_t *request);  

int service_export(service_t *service,
                   const char *name,
                   const char *mimetype_in,
                   const char* mimetype_out,
                   void *userdata,
                   service_onrequest_t onrequest);

const char *service_name(service_t *service);
addr_t *service_addr(service_t *service);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_SERVICE_H_
