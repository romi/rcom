#ifndef _RCOM_REQUEST_PRIV_H_
#define _RCOM_REQUEST_PRIV_H_

#include "rcom/addr.h"
#include "rcom/request.h"

#include "net.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _service_t service_t;

request_t* new_request(service_t* service, tcp_socket_t socket);
void request_handle(request_t* request);
        
#ifdef __cplusplus
}
#endif

#endif // _RCOM_REQUEST_PRIV_H_
