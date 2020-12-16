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
#include "RPCClient.h"

namespace rcom {
        
        RPCClient::RPCClient(const char *name, const char *topic)
        {
                _link = registry_open_messagelink(name, topic,
                                                  (messagelink_onmessage_t) NULL, NULL);
                if (_link == 0)
                        throw std::runtime_error("Failed to create the messagelink");
        }
        
        RPCClient::~RPCClient()
        {
                if (_link)
                        registry_close_messagelink(_link);
        }

        void RPCClient::check_error(json_object_t retval, RPCError &error)
        {
                if (json_object_has(retval, "error")) {
                        
                        json_object_t e = json_object_get(retval, "error");
                        
                        // json_object_get returns json_null() on
                        // failure.
                        json_object_t code = json_object_get(e, "code");
                        json_object_t message = json_object_get(e, "message");
                        
                        if (json_isnumber(code) && json_isstring(message)) {

                                error.code = (int) json_number_value(code);
                                error.message = json_string_value(message);
                                
                        } else {
                                
                                error.code = RPCError::InvalidResponse;
                                error.message = "RPCError: Response has invalid error";
                        }
                         
                } else {
                        error.code = 0;
                }
        }

        void RPCClient::execute(const char *method, JSON &params,
                                JSON &result, RPCError &error) 
        {
                r_debug("RPCClient::execute");

                if (method != 0) {
                        
                        JSON request = JSON::construct("{\"method\": \"%s\"}", method);

                        // FIXME: use C++ API?
                        json_object_set(request.ptr(), "params", params.ptr());
                        
                        json_object_t retval = messagelink_send_command(_link,
                                                                        request.ptr());

                        result = json_object_get(retval, "result");
                        check_error(retval, error);

                        {
                                char buffer[256];
                                json_tostring(retval, buffer, 256);
                                r_debug("RPCClient::execute: reply: %s",
                                        buffer);
                        }
                
                        json_unref(retval);
                        
                } else {
                        error.code = RPCError::NullMethod;
                        error.message = "RPCClient: Null method";
                }
        }
}
