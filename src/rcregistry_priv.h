#ifndef _RCOM_RCREGISTRY_H_
#define _RCOM_RCREGISTRY_H_

#include "rcom/messagelink.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _rcregistry_t rcregistry_t;

rcregistry_t *new_rcregistry();
void delete_rcregistry(rcregistry_t *);
addr_t *rcregistry_addr(rcregistry_t *);
        
#ifdef __cplusplus
}
#endif

#endif // _RCOM_RCREGISTRY_H_
