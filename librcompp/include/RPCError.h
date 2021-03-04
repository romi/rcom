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
#ifndef __RCOM_I_RPC_ERROR_H
#define __RCOM_I_RPC_ERROR_H

#include <string>

namespace rcom {
                                                      
        class RPCError
        {
        public:
                enum {
                        ParseError = -32700,     // Invalid JSON was received by the server.
                        InvalidRequest = -32600, // The JSON request is not a valid.
                        MethodNotFound = -32601, // The method does not exist.
                        InvalidParams = -32602,  // Invalid method parameter(s).
                        InternalError = -32603,  // Internal error.
                        
                        NullMethod = -32000,     // The method was null.
                        InvalidResponse = -32001, // The JSON response is not a valid.
                };
                        
                int code;
                std::string message;
        public:
            RPCError() : code(0), message(){};
        };
}

#endif // __RCOM_I_RPC_ERROR_H
