#ifndef _RCOM_STREAMER_PRIV_H_
#define _RCOM_STREAMER_PRIV_H_

#include "rcom/streamer.h"

// Pass a port equal to 0 to open the streamer on any available port.
streamer_t *new_streamer(const char *name,
                         const char *topic,
                         int port,
                         const char *mimetype,
                         streamer_onclient_t onclient,
                         streamer_onbroadcast_t onbroadcast,
                         void *userdata);

void delete_streamer(streamer_t *streamer);

#endif // _RCOM_STREAMER_PRIV_H_
