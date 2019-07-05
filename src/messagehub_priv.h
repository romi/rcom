#ifndef _RCOM_MESSAGEHUB_PRIV_H_
#define _RCOM_MESSAGEHUB_PRIV_H_

#include "rcom/messagehub.h"
#include "rcom/addr.h"

#ifdef __cplusplus
extern "C" {
#endif

// Set the port equal to 0 to let the OS pick one for you.
messagehub_t *new_messagehub(int port,
                             messagehub_onconnect_t onconnect,
                             void *userdata);
void delete_messagehub(messagehub_t *hub);
addr_t *messagehub_addr(messagehub_t *hub);

void messagehub_remove_link(messagehub_t *hub, messagelink_t *link);
                
#ifdef __cplusplus
}
#endif

#endif // _RCOM_ENDPOINT_PRIV_H_
