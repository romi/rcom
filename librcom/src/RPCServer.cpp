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

#include <stdexcept>
#include "registry.h"
#include "RPCServer.h"
#include "RPCError.h"

namespace rcom {

        void RPCServer_onmessage(void *userdata,
                                 messagelink_t *link,
                                 json_object_t message)
        {
                RPCServer *self = (RPCServer *) userdata;
                self->onmessage(link, message);
        }

        int RPCServer_onconnect(void *userdata,
                                messagehub_t *hub,
                                request_t *request,
                                messagelink_t *link)
        {
                (void) hub; // Mark as unused for compiler
                (void) request; // Mark as unused for compiler
                messagelink_set_userdata(link, userdata);
                messagelink_set_onmessage(link, RPCServer_onmessage);
                return 0;
        }
        
        RPCServer::RPCServer(IRPCHandler &handler,
                             const char *name,
                             const char *topic)
                : _handler(handler)
        {
                _hub = registry_open_messagehub(name, topic,
                                                0, RPCServer_onconnect, this);
                if (_hub == 0)
                        throw RPCError("Failed to create the hub");
        }
        
        RPCServer::~RPCServer()
        {
                if (_hub)
                        registry_close_messagehub(_hub);
        }

        
        void RPCServer::onmessage(messagelink_t *link, json_object_t message)
        {
                r_debug("RPCServer::onmessage");
                
                JSON cmd = message;
                JSON result;
                
                try {

                        char buffer[256];
                        json_tostring(message, buffer, 256);
                        r_debug("RPCServer::onmessage: message: %s",
                                buffer);

                        
                        _handler.execute(cmd, result);

                        
                        json_tostring(result.ptr(), buffer, 256);
                        r_debug("RPCServer::onmessage: result: %s",
                                buffer);

                        messagelink_send_obj(link, result.ptr());
                        
                } catch (std::exception& e) {

                        r_err("RPCServer::onmessage: caught exception: %s",
                              e.what());
                        
                        result = JSON::construct("{\"status\": \"error\", "
                                                "\"message\": \"%s\"}",
                                                e.what());
                }
                
                messagelink_send_obj(link, result.ptr());
        }
}
