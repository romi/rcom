#ifndef _RCOM_CLIENT_H_
#define _RCOM_CLIENT_H_

#include "rcom/json.h"
#include "rcom/membuf.h"

#ifdef __cplusplus
extern "C" {
#endif

json_object_t client_get(const char *topic, const char *resource);
int client_get_data(const char *topic, const char *resource, membuf_t *out);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_CLIENT_H_
