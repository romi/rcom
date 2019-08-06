#ifndef _RCOM_DATALINK_H_
#define _RCOM_DATALINK_H_

#include <stdio.h>
#include <netinet/in.h>
#include "rcom/data.h"
#include "rcom/json.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _datalink_t datalink_t;

typedef void (*datalink_ondata_t)(void *userdata,
                                  datalink_t *datalink,
                                  data_t *input,
                                  data_t *output);


int datalink_send(datalink_t *datalink, data_t *data);

// returns: 0 is all ok, -1 if error, 1 if timeout
data_t *datalink_read(datalink_t *datalink, int timeout);
json_object_t datalink_read_obj(datalink_t *datalink, int timeout);

// Utility function
int datalink_send_f(datalink_t *datalink, const char *format, ...);
int datalink_send_obj(datalink_t *datalink, json_object_t obj);


data_t *datalink_get_output(datalink_t* datalink);
json_object_t datalink_parse(datalink_t *datalink, data_t *data);


// Synchronous functions
data_t *datalink_rpc(datalink_t* datalink, data_t *data);
json_object_t datalink_rpc_f(datalink_t* datalink, const char *format, ...);




// Private
/* datalink_t *new_datalink(datalink_ondata_t callback, void* userdata); */
/* void delete_datalink(datalink_t *datalink); */
/* int datalink_start_thread(datalink_t *datalink); */
/* void datalink_stop_thread(datalink_t *datalink); */
/* addr_t *datalink_addr(datalink_t *datalink); */
/* addr_t *datalink_remote_addr(datalink_t *datalink); */
/* void datalink_set_remote_addr(datalink_t *datalink, addr_t *addr); */


#ifdef __cplusplus
}
#endif

#endif // _RCOM_DATALINK_H_

