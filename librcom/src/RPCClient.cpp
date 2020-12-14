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

        void RPCClient::assure_ok(JSON &reply)
        {
                r_debug("RPCClient::assure_ok");
                if (json_isnull(reply.ptr())) {
                        r_err("RPCClient::assure_ok: "
                              "messagelink_send_command failed");
                        throw std::runtime_error("RPCClient: "
                                                 "messagelink_send_command failed");
                } else {
                        const char *status = reply.str("status");
                        if (rstreq(status, "error")) {
                                const char *message = reply.str("message");
                                r_err("RPCClient::assure_ok: message: %s", message);
                                throw std::runtime_error(message);
                        }
                }
        }

        void RPCClient::execute(JSON &cmd, JSON &reply)
        {
                r_debug("RPCClient::execute");
                reply = messagelink_send_command(_link, cmd.ptr());
                
                char buffer[256];
                json_tostring(reply.ptr(), buffer, 256);
                r_debug("RPCClient::execute: reply: %s",
                        buffer);
                
                assure_ok(reply);
        }
}
