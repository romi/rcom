#ifndef _RCOM_MESSAGELINK_PRIV_H_
#define _RCOM_MESSAGELINK_PRIV_H_

#include "rcom/messagelink.h"
#include "net.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*messagelink_onpong_t)(messagelink_t *link,
                                     void *userdata,
                                     const char *data, int len);

messagelink_t *new_messagelink(messagelink_onmessage_t onmessage,
                               messagelink_onclose_t onclose,
                               void *userdata);
void delete_messagelink(messagelink_t *link);

void messagelink_set_onpong(messagelink_t *link, messagelink_onpong_t onpong);

messagelink_t *server_messagelink_connect(messagehub_t *hub, tcp_socket_t socket);

int client_messagelink_connect(messagelink_t *link, addr_t *addr);
int client_messagelink_disconnect(messagelink_t *link);

void messagelink_read_in_background(messagelink_t *link);

int messagelink_send_ping(messagelink_t *link, const char *data, int len);

addr_t *messagelink_addr(messagelink_t *link);





#ifdef __cplusplus
}
#endif

#endif // _RCOM_MESSAGELINK_PRIV_H_
