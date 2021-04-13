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
#include <iostream>
#include <memory>
#include <signal.h>
#include <r.h>
#include <MessageLink.h>

using namespace rcom;
using namespace rpp;

int main()
{
        try {
                MessageLink link("hello-world");                
                MemBuffer message;                
                message.append_string("hello");

                if (link.send(message)
                    && link.recv(message, 1.0)) {

                        std::cout << "Server replies '" << message.tostring() << "'"
                                  << std::endl;
                        
                } else {
                        r_err("main: failed to send & receive the message");
                }
                
                
        } catch (std::runtime_error& re) {
                r_err("main: caught runtime_error: %s", re.what());
        } catch (...) {
                r_err("main: caught exception");
        }
}
