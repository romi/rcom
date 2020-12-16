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
#include "../include/RPCClient.h"
#include "../include/RPCError.h"

namespace rcom {
        
        RPCClient::RPCClient(const char *name, const char *topic)
        {
                _link = registry_open_messagelink(name, topic,
                                                  (messagelink_onmessage_t) NULL, NULL);
                if (_link == 0)
                        throw RPCError("Failed to create the messagelink");
        }
        
        RPCClient::~RPCClient()
        {
                if (_link)
                        registry_close_messagelink(_link);
        }

        /** Assure that the received reply is conform to what is
         * expected: it must have a status field and if the status is
         * "error" it must have an error message. Anything else will
         * result in an RPCError being thrown. */
        void RPCClient::assure_valid_reply(JSON &reply)
        {
                if (reply.isnull()) {
                        r_warn("RPCClient: messagelink_send_command failed");
                        throw RPCError("RPCClient: failed to send the command");
                        
                } else if (!reply.has("status")) {
                        r_warn("RPCClient: invalid reply: missing status");
                        throw RPCError("RPCClient: invalid reply: missing status");
                        
                } else if (rstreq(reply.str("status"), "error")
                           && !reply.has("message")) {
                        r_warn("RPCClient: invalid reply: missing error message");
                        throw RPCError("RPCClient: invalid reply: missing error message");
                }
        }

        bool RPCClient::is_status_ok(JSON &reply)
        {
                const char *status = reply.str("status");
                return rstreq(status, "ok");
        }

        const char *RPCClient::get_error_message(JSON &reply)
        {
                return reply.str("message");
        }

        void RPCClient::execute(JSON &cmd, JSON &reply)
        {
                r_debug("RPCClient::execute");

                json_object_t r = messagelink_send_command(_link, cmd.ptr());
                reply = r; 
                json_unref(r);

                {
                        char buffer[256];
                        json_tostring(reply.ptr(), buffer, 256);
                        r_debug("RPCClient::execute: reply: %s",
                                buffer);
                }
                
                assure_valid_reply(reply);
        }
}
