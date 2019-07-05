#ifndef _RCOM_DATALINK_PRIV_H_
#define _RCOM_DATALINK_PRIV_H_

#include "rcom/datalink.h"
#include "rcom/addr.h"

#ifdef __cplusplus
extern "C" {
#endif

datalink_t *new_datalink(datalink_ondata_t callback, void* userdata);
void delete_datalink(datalink_t *datalink);
addr_t *datalink_addr(datalink_t *datalink);
addr_t *datalink_remote_addr(datalink_t *datalink);
void datalink_set_remote_addr(datalink_t *datalink, addr_t *addr);

int datalink_sendto(datalink_t *datalink, addr_t *addr, data_t *data);


#ifdef __cplusplus
}
#endif

#endif // _RCOM_DATALINK_PRIV_H_

