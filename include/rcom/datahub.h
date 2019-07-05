#ifndef _RCOM_DATAHUB_H_
#define _RCOM_DATAHUB_H_

#include <stdarg.h>
#include "rcom/addr.h"
#include "rcom/data.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _datahub_t datahub_t;

typedef int (*datahub_ondata_t)(void *userdata,
                                datahub_t* hub,
                                addr_t *link,
                                data_t *data);

typedef int (*datahub_onbroadcast_t)(void *userdata,
                                     datahub_t* hub);

// Send to a single remote link
int datahub_send_num(datahub_t* hub, addr_t *link, double value);
int datahub_send_str(datahub_t* hub, addr_t *link, const char* value);
int datahub_send_f(datahub_t* hub, addr_t *link, const char *format, ...);
int datahub_send_obj(datahub_t* hub, addr_t *link, json_object_t value);
int datahub_send_bin(datahub_t* hub, addr_t *link, const char *data, int len);
int datahub_send_v(datahub_t* hub, addr_t *link, const char* format, va_list ap);
int datahub_send(datahub_t* hub, addr_t *link, data_t* data);

// Broadcast to all links.
int datahub_broadcast_num(datahub_t* hub, addr_t *exclude, double value);
int datahub_broadcast_str(datahub_t* hub, addr_t *exclude, const char* value);
int datahub_broadcast_f(datahub_t* hub, addr_t *exclude, const char *format, ...);
int datahub_broadcast_obj(datahub_t* hub, addr_t *exclude, json_object_t value);
int datahub_broadcast_bin(datahub_t* hub, addr_t *exclude, const char *data, int len);
int datahub_broadcast_v(datahub_t* hub, addr_t *exclude, const char* format, va_list ap);
int datahub_broadcast(datahub_t* hub, addr_t *exclude, data_t* data);


#ifdef __cplusplus
}
#endif

#endif // _RCOM_DATASOURCE_H_

