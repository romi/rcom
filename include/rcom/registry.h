#ifndef _RCOM_REGISTRY_H_
#define _RCOM_REGISTRY_H_

#include "rcom/datalink.h"
#include "rcom/datahub.h"
#include "rcom/messagehub.h"
#include "rcom/messagelink.h"
#include "rcom/service.h"
#include "rcom/streamer.h"
#include "rcom/streamerlink.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *get_registry_ip();
int get_registry_port();

// datalink
datalink_t *registry_open_datalink(const char *name,
                                   const char *topic,
                                   datalink_ondata_t callback,
                                   void* userdata);

void registry_close_datalink(datalink_t *datalink);


// datahub
datahub_t *registry_open_datahub(const char *name,
                                 const char *topic,
                                 datahub_onbroadcast_t onbroadcast,
                                 datahub_ondata_t ondata,
                                 void* userdata);

void registry_close_datahub(datahub_t *hub);


// messagehub
messagehub_t *registry_open_messagehub(const char *name,
                                       const char *topic,
                                       int port,
                                       messagehub_onconnect_t onconnect,
                                       void *userdata);

void registry_close_messagehub(messagehub_t *hub);
addr_t *registry_get_messagehub(const char *topic);


// messagelink
messagelink_t *registry_open_messagelink(const char *name,
                                         const char *topic,
                                         messagelink_onmessage_t onmessage,
                                         void *userdata);

void registry_close_messagelink(messagelink_t *hub);


// service
service_t *registry_open_service(const char *name, const char *topic, int port);
void registry_close_service(service_t *service);
addr_t *registry_get_service(const char *topic);


// streamer
streamer_t *registry_open_streamer(const char *name,
                                   const char *topic,
                                   int port,
                                   const char *mimetype,
                                   streamer_onclient_t onclient,
                                   streamer_onbroadcast_t onbroadcast,
                                   void* userdata);

void registry_close_streamer(streamer_t *streamer);
addr_t *registry_get_streamer(const char *topic);


// streamerlink
streamerlink_t *registry_open_streamerlink(const char *name,
                                           const char *topic,
                                           streamerlink_ondata_t ondata,
                                           void* userdata,
                                           int autoconnect);

void registry_close_streamerlink(streamerlink_t *link);

        
#ifdef __cplusplus
}
#endif

#endif // _RCOM_REGISTRY_H_

