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
#include <signal.h>
#include <r.h>
#include <MessageHub.h>
#include <IMessageListener.h>

static bool quit = false;
static void set_quit(int sig, siginfo_t *info, void *ucontext);
static void quit_on_control_c();

using namespace rcom;
using namespace rpp;

class HelloWorldListener : public IMessageListener
{
public:
        ~HelloWorldListener() = default; 

        void onmessage(IWebSocket& websocket, rpp::MemBuffer& message) override {
                std::cout << "Client says '" << message.tostring() << "'" << std::endl;
                rpp::MemBuffer reply;
                reply.append_string("world");
                websocket.send(reply);
        }
};


int main()
{
        try {
                HelloWorldListener hello_world;
                MessageHub message_hub("hello-world", hello_world);                

                quit_on_control_c();
        
                while (!quit) {
                        message_hub.handle_events();
                        clock_sleep(0.050);
                }
                
        } catch (std::runtime_error& re) {
                r_err("main: caught runtime_error: %s", re.what());
        } catch (...) {
                r_err("main: caught exception");
        }
}

static void set_quit(int sig, siginfo_t *info, void *ucontext)
{
        (void) sig;
        (void) info;
        (void) ucontext;
        quit = true;
}

static void quit_on_control_c()
{
        struct sigaction act;
        memset(&act, 0, sizeof(struct sigaction));

        act.sa_flags = SA_SIGINFO;
        act.sa_sigaction = set_quit;
        if (sigaction(SIGINT, &act, nullptr) != 0) {
                perror("init_signal_handler");
                exit(1);
        }
}
