#ifndef _RCOM_STREAMERLINK_PRIV_H_
#define _RCOM_STREAMERLINK_PRIV_H_

#include "rcom/streamerlink.h"

#ifdef __cplusplus
extern "C" {
#endif

streamerlink_t *new_streamerlink(const char *resource,
                                 streamerlink_ondata_t ondata,
                                 void* data);
        
void delete_streamerlink(streamerlink_t *link);

int streamerlink_connect(streamerlink_t *link, addr_t *addr);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_STREAMERLINK_PRIV_H_
