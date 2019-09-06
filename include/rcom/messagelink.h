#ifndef _RCOM_MESSAGELINK_H_
#define _RCOM_MESSAGELINK_H_

#include <r.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _messagelink_t messagelink_t;
typedef struct _messagehub_t messagehub_t;

typedef void (*messagelink_onmessage_t)(void *userdata,
                                        messagelink_t *link,
                                        json_object_t message);

typedef void (*messagelink_onclose_t)(void *userdata,
                                      messagelink_t *link);

typedef int (*messagelink_oncommand_t)(void *userdata,
                                       messagelink_t *link,
                                       json_object_t command,
                                       membuf_t *message);

void messagelink_set_userdata(messagelink_t *link, void *userdata);
void *messagelink_get_userdata(messagelink_t *link);
void messagelink_set_onmessage(messagelink_t *link, messagelink_onmessage_t onmessage);
void messagelink_set_onclose(messagelink_t *link, messagelink_onclose_t onclose);

// Send messages.
// The messagelink_send_xxx() functions are multi-thread safe. 
int messagelink_send_num(messagelink_t *link, double value);
int messagelink_send_str(messagelink_t *link, const char* value);
int messagelink_send_f(messagelink_t *link, const char *format, ...);
int messagelink_send_obj(messagelink_t *link, json_object_t value);
int messagelink_send_text(messagelink_t *link, const char *data, int len);
int messagelink_send_v(messagelink_t *link, const char* format, va_list ap);

// Only call messagelink_read() if the link has been initialized
// without an onmessage handler!
json_object_t messagelink_read(messagelink_t *link);


json_object_t messagelink_send_command_f(messagelink_t *link, const char *format, ...);

        
#ifdef __cplusplus
}
#endif

#endif // _RCOM_MESSAGELINK_H_
