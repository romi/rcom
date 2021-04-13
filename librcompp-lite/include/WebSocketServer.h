/*
  rcom

  Copyright (C) 2019 Sony Computer Science Laboratories
  Author(s) Peter Hanappe

  rcom is light-weight libary for inter-node communication.

  rcom is free software: you can redistribute it and/or modify it
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
#ifndef _RCOM_WEBSOCKET_SERVER_H_
#define _RCOM_WEBSOCKET_SERVER_H_

#include <memory>
#include "IWebSocketServer.h"
#include "IAddress.h"
#include "IServerSocket.h"
#include "ISocketFactory.h"

namespace rcom {
        
        class WebSocketServer : public IWebSocketServer
        {
        protected:
                std::unique_ptr<IServerSocket> server_socket_;
                ISocketFactory& factory_;
                std::vector<std::unique_ptr<IWebSocket>> links_;
                rpp::MemBuffer message_;
                std::vector<size_t> to_close_;
                std::vector<size_t> to_remove_;
                
                void handle_new_connections(IWebSocketServerListener& listener);
                void try_new_connection(IWebSocketServerListener& listener, int sockfd);
                void handle_new_connection(IWebSocketServerListener& listener, int sockfd);
                void handle_new_messages(IWebSocketServerListener& listener);
                void handle_new_messages(size_t index, IWebSocketServerListener& listener);
                
                bool send(size_t index, rpp::MemBuffer& message,
                          IWebSocket::MessageType type);
                void close(size_t index, CloseCode code);
                IWebSocket& append(int sockfd);
                void remove_closed_links();
                
        public:
                WebSocketServer(std::unique_ptr<IServerSocket>& server_socket,
                                ISocketFactory& factory);
                virtual ~WebSocketServer();

                void handle_events(IWebSocketServerListener& listener) override;
                void broadcast(rpp::MemBuffer& message,
                               IWebSocket::MessageType type = IWebSocket::kTextMessage) override;
                void get_address(IAddress& address) override;
                size_t count_links() override;
        };
}

#endif // _RCOM_WEBSOCKET_SERVER_H_
