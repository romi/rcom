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
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <signal.h>

#include <r.h>
#include <rcom.h>

#include "proxy.h"
#include "messagelink_priv.h"
#include "registry_priv.h"

static int restart_node(const char *name)
{
        char line[1024];
        char cmd[128];

        rprintf(cmd, sizeof(cmd), "pidof %s", name);
        
        FILE *pidof = popen(cmd, "r");        
        fgets(line, sizeof(line), pidof);
        pid_t pid = strtoul(line, NULL, 10);        
        pclose(pidof);

        kill(pid, SIGHUP);

        return 0;
}

static void open_browser(char *url)
{
        char *argv[4];
        argv[0] = "/usr/bin/firefox";
        argv[1] = "-new-tab";
        argv[2] = url;
        argv[3] = NULL;

        int pid = fork();
	switch (pid) {
        default:
                return;
        case -1:
                fprintf(stderr, "Failed to fork\n");                
                return;
        case 0:  break;
	}
        
        fprintf(stderr, "Starting firefox\n");                
        execv(argv[0], argv);
        perror("monitor: execvp:");
        exit(0);       
}

static int show_topic(const char *topic)
{
        addr_t *addr = registry_get_service(topic);
        if (addr == NULL) {
                fprintf(stderr, "Failed to find the service\n");
                return 1;
        }

        char b[52];
        char buffer[1024];
        rprintf(buffer, sizeof(buffer), "http://%s", addr_string(addr, b, 52));
        open_browser(buffer);
        delete_addr(addr);

        return 0;
}

static int stream_topic(const char *topic)
{
        addr_t *addr = registry_get_streamer(topic);
        if (addr == NULL) {
                fprintf(stderr, "Failed to find the streamer for %s\n", topic);
                return 1;
        }

        char b[52];
        char buffer[1024];
        rprintf(buffer, sizeof(buffer), "http://%s/stream.html", addr_string(addr, b, 52));
        open_browser(buffer);

        return 0;
}

static int list_nodes()
{
        proxy_t *proxy = proxy_get();
        if (proxy == NULL)
        if (proxy == NULL) {
                fprintf(stderr, "Failed to initialize the proxy\n");
                return 1;
        }
                
        int n = proxy_count_nodes(proxy);
        
        if (n > 0) {
                membuf_t *name = new_membuf();
                membuf_t *topic = new_membuf();
                addr_t *addr = new_addr0();
                int type;
                char b[52];
                for (int i = 0; i < n; i++) {
                        proxy_get_node(proxy, i, name, topic, &type, addr);
                        printf("--------------------------------\n");
                        printf("Topic:    %s\n", membuf_data(topic));
                        printf("Type:     %s\n", registry_type_to_str(type));
                        printf("Name:     %s\n", membuf_data(name));
                        if (type == TYPE_DATALINK || type == TYPE_DATAHUB)
                                printf("Address:  udp://%s\n", addr_string(addr, b, 52));
                        else
                                printf("Address:  http://%s\n", addr_string(addr, b, 52));
                }
                printf("--------------------------------\n");
                delete_membuf(name);
                delete_membuf(topic);
                delete_addr(addr);
        } else {
                printf("--------------------------------\n");
                printf("No nodes active\n");
                printf("--------------------------------\n");
        }
        return 0;
}

static void print_data(void *userdata __attribute__((unused)),
                       datalink_t *link __attribute__((unused)),
                       data_t *data,
                       data_t *out __attribute__((unused)))
{
        printf("%.*s\n", data_len(data), data_data(data));
}

static int listen_datahub(const char *topic)
{
        datalink_t *link;

        link = registry_open_datalink("rcquery", topic, print_data, NULL);
        if (link == NULL)
                return 1;

        while (!app_quit())
                clock_sleep(1);

        registry_close_datalink(link);
        return 0;
}

static void print_message(void *userdata __attribute__((unused)),
                          messagelink_t *link __attribute__((unused)),
                          json_object_t message)
{
        json_print(message, 0);
}

static int listen_messagehub(const char *topic)
{
        messagelink_t *link;

        link = registry_open_messagelink("rcquery", topic, print_message, NULL);
        if (link == NULL)
                return 1;

        while (!app_quit())
                clock_sleep(1);
        
        registry_close_messagelink(link);
        return 0;
}

static int print_status(json_object_t reply)
{
        const char *status = json_object_getstr(reply, "status");
        if (status == NULL) {
                fprintf(stderr, "Controller returned an invalid status message");
                return -1;
                
        } else if (rstreq(status, "ok")) {
                return 0;
                
        } else if (rstreq(status, "error")) {
                const char *s = json_object_getstr(reply, "message");
                fprintf(stderr, "Error: %s", s);
                return -1;
        }
        return -1; // ??
}

static int send_request(const char *topic, const char *command)
{
        messagelink_t *link;
        json_object_t reply;
        
        link = registry_open_messagelink("rcquery", topic, NULL, NULL);
        if (link == NULL)
                return 1;

        reply = messagelink_send_command_f(link, "%s", command);
        int err = print_status(reply);
        
        registry_close_messagelink(link);

        return (err)? 1 : 0;
}

static int send_data(const char *topic, const char *text)
{
        messagelink_t *link;
        json_object_t reply;
        int exit_code = 0;
        
        link = registry_open_messagelink("rcquery", topic, NULL, NULL);
        if (link == NULL)
                return 1;

        reply = messagelink_send_command_f(link, "%s", text);
        json_print(reply, 0);

        if (json_isnull(reply))
                exit_code = 1;
        
        registry_close_messagelink(link);

        return exit_code;
}

static void print_usage_listen()
{
        printf("Prints the data sent by a datahub or messagehub\n");
        printf("Usage: rcom listen [datahub|messagehub] <topic-name>\n");
}

static void print_usage_show()
{
        printf("Opens a browser tab for the service with the given topic name\n");
        printf("Usage: rcom show <topic-name>\n");
}

static void print_usage_stream()
{
        printf("Opens a browser tab for the streamer with the given topic name\n");
        printf("Usage: rcom stream <topic-name>\n");
}

static void print_usage_request()
{
        printf("Sends a request to a controller and prints the status.\n");
        printf("Usage: rcom request <topic-name> <json-message>\n");
        printf("The message most likely has the following format: \n");
        printf("    \"{'command': '<command>', ...}\"\n");
}

static void print_usage_send()
{
        printf("Sends a (json) string to a messagehub and prints the response.\n");
        printf("Usage: rcom send <topic-name> <data>\n");
}

static void print_usage_restart()
{
        printf("Restarts the node with the given name\n");
        printf("Usage: rcom restart <node-name>\n");
}

static void print_usage()
{
        printf("Usage: rcom command [options]\n");
        printf("  Available commands: list, listen, request, show, stream, help, restart\n");
}

int main(int argc, char **argv)
{
        const char *command;
        
        app_init(&argc, argv);
        
        if (argc < 2) {
                print_usage();
                return 1;
        }

        command = argv[1];

        if (rstreq(command, "help")) {
                print_usage();                
        }

        int exit_code = 0;
        
        if (rstreq(command, "list")) {
                exit_code = list_nodes();
                
        } else if (rstreq(command, "listen")) {
                if ((argc >= 3 && rstreq(argv[2], "help"))  || argc < 4) {
                        print_usage_listen();
                } else {
                        const char *what = argv[2];
                        const char *topic = argv[3];
                        if (rstreq(what, "datahub")) {
                                exit_code = listen_datahub(topic);
                        } else if (rstreq(what, "messagehub")) {
                                exit_code = listen_messagehub(topic);
                        } else {
                                printf("Unknown type %s\n", what);
                                print_usage_listen();
                        }
                }
                
        } else if (rstreq(command, "request")) {
                if ((argc >= 3 && rstreq(argv[2], "help")) || argc < 4) {
                        print_usage_request();
                } else {
                        const char *topic = argv[2];
                        const char *request = argv[3];
                        exit_code = send_request(topic, request);
                }
                
        } else if (rstreq(command, "send")) {
                if ((argc >= 3 && rstreq(argv[2], "help")) || argc < 4) {
                        print_usage_send();
                } else {
                        const char *topic = argv[2];
                        const char *text = argv[3];
                        exit_code = send_data(topic, text);
                }
                
        } else if (rstreq(command, "show")) {
                if ((argc >= 3 && rstreq(argv[2], "help"))  || argc < 3) {
                        print_usage_show();
                } else {
                        const char *topic = argv[2];
                        exit_code = show_topic(topic);
                }
                
        } else if (rstreq(command, "stream")) {
                if ((argc >= 3 && rstreq(argv[2], "help"))  || argc < 3) {
                        print_usage_stream();
                } else {
                        const char *topic = argv[2];
                        exit_code = stream_topic(topic);
                }
                
        } else if (rstreq(command, "restart")) {
                if ((argc >= 3 && rstreq(argv[2], "help"))  || argc < 3) {
                        print_usage_restart();
                } else {
                        const char *name = argv[2];
                        exit_code = restart_node(name);
                }
                
        } else {
                printf("Unknown command %s\n", command);
                exit_code = 1;
        }
        
        exit(exit_code);
}
