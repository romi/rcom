#ifndef _RCOM_STREAMER_H_
#define _RCOM_STREAMER_H_

#include "addr.h"
#include "circular.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _streamer_client_t streamer_client_t;
typedef void (*streamer_client_delete_context_t)(streamer_client_t *c);

/* circular_buffer_t *streamer_client_get_buffer(streamer_client_t *b, const char *name); */
circular_buffer_t *streamer_client_get_buffer(streamer_client_t *b);
void streamer_client_set_context(streamer_client_t *c, void *context);
void *streamer_client_get_context(streamer_client_t *c);
void streamer_client_set_delete_context(streamer_client_t *c, streamer_client_delete_context_t del);


typedef struct _streamer_t streamer_t;

typedef int (*streamer_onclient_t)(void *userdata,
                                   streamer_client_t *client,
                                   /* const char *exp, */
                                   streamer_t *streamer);

typedef int (*streamer_onbroadcast_t)(void *userdata, streamer_t *streamer);


/* int streamer_export(streamer_t *streamer, */
/*                     const char *name, */
/*                     const char *mimetype); */

addr_t *streamer_addr(streamer_t *s);

        
void streamer_lock_clients(streamer_t* s);
void streamer_unlock_clients(streamer_t* s);
streamer_client_t* streamer_get_client(streamer_t* s, int n);
int streamer_has_clients(streamer_t* s);
int streamer_count_clients(streamer_t *s);

int streamer_send_multipart(streamer_t *s,
                            /* const char *exp, */
                            const char *data, int length,
                            const char *mimetype,
                            double time);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_STREAMER_H_

