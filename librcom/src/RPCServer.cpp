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
                        throw std::runtime_error("Failed to create the hub");
        }
        
        RPCServer::~RPCServer()
        {
                if (_hub)
                        registry_close_messagehub(_hub);
        }

        /* Construct the envelope for an error reponse to be sent back
         * to the client. This is still done "the old way", for now,
         * using the C JSON API.  */
        json_object_t RPCServer::construct_response(int code, const char *message)
        {
                json_object_t response = json_object_create();
                
                if (code != 0) {
                        json_object_t error = json_object_create();
                        
                        json_object_set(response, "error", error);
                        
                        json_object_setnum(error, "code", code);
                        
                        if (message != 0 && strlen(message) > 0) {
                                json_object_setstr(error, "message", message);
                        } else {
                                json_object_setstr(error, "message", "No message was given");
                        }
                        
                        json_unref(error); // refcount held by response object
                }

                return response;
        }
        
        /* Construct the envelope for a reponse with results, to be
         * sent back to the client. This is still done "the old way",
         * for now, using the C JSON API.  */
        json_object_t RPCServer::construct_response(RPCError &error, JSON &result)
        {
                json_object_t response = construct_response(error.code, error.message.c_str());
                
                if (!result.isnull()) {
                        json_object_set(response, "result", result.ptr());
                }
                
                return response;
        }
        
        void RPCServer::onmessage(messagelink_t *link, json_object_t message)
        {
                r_debug("RPCServer::onmessage");
                

                {
                        char buffer[256];
                        json_tostring(message, buffer, 256);
                        r_debug("RPCServer::onmessage: message: %s",
                                buffer);
                }
                
                json_object_t response;
                
                const char *method = json_object_getstr(message, "method");

                if (method != 0) {

                        JSON params = json_object_get(message, "params");

                        try {

                                JSON result;
                                RPCError error;
                        
                                _handler.execute(method, params, result, error);

                                {
                                        char buffer[256];
                                        json_tostring(result.ptr(), buffer, 256);
                                        r_debug("RPCServer::onmessage: result: %s",
                                                buffer);
                                }
                                
                                response = construct_response(error, result);

                        } catch (std::exception &e) {
                                response = construct_response(RPCError::InternalError, e.what());
                        }
                        
                } else {
                        response = construct_response(RPCError::MethodNotFound, "The method is missing");
                }
                
                messagelink_send_obj(link, response);

                json_unref(response);
        }
}
