/*
  romi-rover

  Copyright (C) 2019 Sony Computer Science Laboratories
  Author(s) Peter Hanappe

  romi-rover is collection of applications for the Romi Rover.

  romi-rover is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <http://www.gnu.org/licenses/>.

 */

#ifndef __RCOM_RPC_SERVER_H
#define __RCOM_RPC_SERVER_H

#include "IRPCHandler.h"
#include "messagehub.h"

namespace rcom {
        
        class RPCServer {
        protected:
                messagehub_t *_hub;
                IRPCHandler &_handler;

                friend void RPCServer_onmessage(void *userdata,
                                                messagelink_t *link,
                                                json_object_t message);
                
                void onmessage(messagelink_t *link, json_object_t message);

                json_object_t construct_response(int code, const char *message);
                json_object_t construct_response(RPCError &error, JsonCpp &result);
                
        public:
                RPCServer(IRPCHandler &handler, const char *name, const char *topic);
                virtual ~RPCServer();
        };
}

#endif // __RCOM_RPC_SERVER_H
