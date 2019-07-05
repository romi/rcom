#ifndef _RCOM_MESSAGEHUB_H_
#define _RCOM_MESSAGEHUB_H_

#include "rcom/json.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _messagehub_t messagehub_t;
typedef struct _messagelink_t messagelink_t;

// if returned value is zero, all is well.
// if returned value is not zero, the messagelink will be closed.
typedef int (*messagehub_onconnect_t)(messagehub_t *hub,
                                      messagelink_t *link,
                                      void *userdata);

// Broadcast messages to all connected messagelinks
int messagehub_send_num(messagehub_t *hub, messagelink_t *exclude, double value);
int messagehub_send_str(messagehub_t *hub, messagelink_t *exclude, const char* value);
int messagehub_send_f(messagehub_t *hub, messagelink_t *exclude, const char *format, ...);
int messagehub_send_obj(messagehub_t *hub, messagelink_t *exclude, json_object_t value);
int messagehub_send_text(messagehub_t *hub, messagelink_t *exclude, const char *data, int len);
int messagehub_send_v(messagehub_t *hub, messagelink_t *exclude, const char* format, va_list ap);

int messagehub_send_ping(messagehub_t *hub, const char* data, int len);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_MESSAGEHUB_H_
