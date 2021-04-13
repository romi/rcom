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
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <r.h>
#include "posix.h"

namespace rcom {

        static WaitStatus do_wait(rpp::ILinux& linux, int fd, double timeout);

        WaitStatus Posix::poll(rpp::ILinux& linux, int fd, double timeout)
        {
                WaitStatus ret = WaitError;
                if (timeout >= 0.0)
                        ret = do_wait(linux, fd, timeout);
                return ret; 
        }

        static WaitStatus do_wait(rpp::ILinux& linux, int fd, double timeout)
        {
                WaitStatus retval = WaitError;
                int timeout_ms = (int) (timeout * 1000.0);
                
                struct pollfd fds[1];
                fds[0].fd = fd;
                fds[0].events = POLLIN;
                
                int pollrc = linux.poll(fds, 1, timeout_ms);
                if (pollrc < 0) {
                        r_err("do_wait: poll error %d", errno);
                        
                } else if (pollrc > 0) {
                        if (fds[0].revents & POLLIN) {
                                retval = WaitOK;
                        }
                } else {
                        r_warn("do_wait: poll timed out");
                        retval = WaitTimeout;
                }
                return retval;
        }

        void Posix::getsockname(rpp::ILinux& linux, int sockfd, IAddress& address)
        {
                struct sockaddr_in local_addr;
                uint32_t socklen = sizeof(local_addr);
                memset((char *) &local_addr, 0, socklen);
                
                linux.getsockname(sockfd, (struct sockaddr*) &local_addr, &socklen);
                
                address.set(inet_ntoa(local_addr.sin_addr),
                            ntohs(local_addr.sin_port));
        }
}
