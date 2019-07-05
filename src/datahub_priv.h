#ifndef _RCOM_DATAHUB_PRIV_H_
#define _RCOM_DATAHUB_PRIV_H_

#include "rcom/addr.h"
#include "rcom/datahub.h"

#ifdef __cplusplus
extern "C" {
#endif

datahub_t* new_datahub(datahub_onbroadcast_t onbroadcast,
                       datahub_ondata_t ondata,
                       void *userdata);
        
void delete_datahub(datahub_t* hub);

int datahub_add_link(datahub_t* hub, addr_t *addr);
int datahub_remove_link(datahub_t* hub, addr_t *addr);

addr_t *datahub_addr(datahub_t* hub);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_DATAHUB_PRIV_H__

