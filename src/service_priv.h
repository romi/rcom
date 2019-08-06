#ifndef _RCOM_SERVICE_PRIV_H_
#define _RCOM_SERVICE_PRIV_H_

#include "rcom/service.h"
#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

// Set the port equal to 0 to let the OS pick one for you.
service_t *new_service(const char *name, int port);

void delete_service(service_t *service);

export_t *service_get_export(service_t *service, const char *name);
                
#ifdef __cplusplus
}
#endif

#endif // _RCOM_SERVICE_PRIV_H_
