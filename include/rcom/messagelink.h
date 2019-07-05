#ifndef _RCOM_MESSAGELINK_H_
#define _RCOM_MESSAGELINK_H_

#include "rcom/json.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _messagelink_t messagelink_t;
typedef struct _messagehub_t messagehub_t;

typedef void (*messagelink_onmessage_t)(messagelink_t *link,
                                        void *userdata,
                                        json_object_t message);
typedef void (*messagelink_callback_t)(messagelink_t *link,
                                       void *userdata);

// Send messages.
// The messagelink_send_xxx() functions are multi-thread safe. 
int messagelink_send_num(messagelink_t *link, double value);
int messagelink_send_str(messagelink_t *link, const char* value);
int messagelink_send_f(messagelink_t *link, const char *format, ...);
int messagelink_send_obj(messagelink_t *link, json_object_t value);
int messagelink_send_text(messagelink_t *link, const char *data, int len);
int messagelink_send_v(messagelink_t *link, const char* format, va_list ap);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_MESSAGELINK_H_
