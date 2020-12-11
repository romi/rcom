/*
  rcutil

  Copyright (C) 2019 Sony Computer Science Laboratories
  Author(s) Peter Hanappe

  rcutil is light-weight libary for inter-node communication.

  rcutil is free software: you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <http://www.gnu.org/licenses/>.

 */
#ifndef _RCOM_REGISTRY_H_
#define _RCOM_REGISTRY_H_

#include "datalink.h"
#include "datahub.h"
#include "messagehub.h"
#include "messagelink.h"
#include "service.h"
#include "streamer.h"
#include "streamerlink.h"

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

void registry_close_messagelink(messagelink_t *link);


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
                                           streamerlink_onresponse_t onresponse,
                                           void* userdata,
                                           int autoconnect);

void registry_close_streamerlink(streamerlink_t *link);

        
#ifdef __cplusplus
}
#endif

#endif // _RCOM_REGISTRY_H_

