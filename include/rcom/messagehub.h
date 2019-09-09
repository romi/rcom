#ifndef _RCOM_MESSAGEHUB_H_
#define _RCOM_MESSAGEHUB_H_

#include <r.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _messagehub_t messagehub_t;
typedef struct _messagelink_t messagelink_t;
typedef struct _request_t request_t;
typedef struct _response_t response_t;

/*
 * If the returned value is zero, all is well. If the returned value
 * is not zero, the messagelink will be closed.
 */
typedef int (*messagehub_onconnect_t)(void *userdata,
                                      messagehub_t *hub,
                                      request_t *request,
                                      messagelink_t *link);

typedef void (*messagehub_onrequest_t)(void *userdata,
                                       request_t *request,
                                       response_t *response);

int messagehub_set_onrequest(messagehub_t *hub, messagehub_onrequest_t onrequest);

// Broadcast messages to all connected messagelinks
int messagehub_broadcast_num(messagehub_t *hub, messagelink_t *exclude, double value);
int messagehub_broadcast_str(messagehub_t *hub, messagelink_t *exclude, const char* value);
int messagehub_broadcast_f(messagehub_t *hub, messagelink_t *exclude, const char *format, ...);
int messagehub_broadcast_obj(messagehub_t *hub, messagelink_t *exclude, json_object_t value);
int messagehub_broadcast_text(messagehub_t *hub, messagelink_t *exclude, const char *data, int len);
int messagehub_broadcast_v(messagehub_t *hub, messagelink_t *exclude, const char* format, va_list ap);

int messagehub_send_ping(messagehub_t *hub, const char* data, int len);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_MESSAGEHUB_H_
