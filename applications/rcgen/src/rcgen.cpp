/*
  rcutil

  Copyright (C) 2019 Sony Computer Science Laboratories
  Author(s) Peter Hanappe

  rcutil is light-weight libary for inter-node communication.

  rcutil is free software: you can redistribute it and/or modify it
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
#include <stdio.h>
#include <rcom.h>

/*
enum {
        OP_CODE,
        OP_CMAKELISTS
}; 

static int op = OP_CODE;
extern int opterr;

static int parse_args(int *argc, char **argv)
{
        int c, option_index = 0;
        static char *optchars = "ms";
        static struct option long_options[] = {
                {"cmakelists", required_argument, 0, 'm'},
                {"code", required_argument, 0, 's'},
                {0, 0, 0, 0}
        };
        while (1) {
                c = getopt_long(*argc, argv, optchars, long_options, &option_index);
                if (c == -1)
                        break;
                switch (c) {
                case 'm':
                        op = OP_CMAKELISTS;
                        break;
                case 's':
                        op = OP_CMAKELISTS;
                        break;
                }
        }
        return 0;
}
*/

int print_headers(membuf_t *buf, const char *name, const char *header)
{
        membuf_printf(buf, "#include <string.h>\n");
        membuf_printf(buf, "#include <rcom.h>\n");
        if (header)
                membuf_printf(buf, "#include \"%s\"\n", header);
        else
                membuf_printf(buf, "#include \"%s.h\"\n", name);
        membuf_printf(buf, "\n");
        return 0;
}

int print_main(membuf_t *buf)
{
        const char *m =
                "int main(int argc, char** argv)\n"
                "{\n"
                "        int err = 0;\n"
                "\n"
                "        app_init(&argc, argv);\n"
                "\n"
                "        err = init(argc, argv);\n"
                "        if (err != 0) {\n"
                "                r_err(\"init() failed\");\n"
                "                app_set_quit();\n"
                "        }\n"
                "\n"
                "        while (!app_quit())\n"
                "                idle();\n"
                "\n"
                "        cleanup();\n"
                "\n"
                "        return err? 1 : 0;\n"
                "}\n";
        membuf_printf(buf, "%s\n", m);
        return 0;
}

int check_com_i(membuf_t *buf __attribute__((unused)), json_object_t obj)
{
        const char *type = NULL;
        const char *topic = NULL;
        
        if (!json_isobject(obj)) {
                fprintf(stderr, "Com: expected a description\n");
                return -1;
        }
        
        type = json_object_getstr(obj, "type");
        if (type == NULL) {
                fprintf(stderr, "Com: Missing type\n");
                return -1;
        }
        
        topic = json_object_getstr(obj, "topic");
        if (topic == NULL) {
                fprintf(stderr, "Com: Missing topic\n");
                return -1;
        }

        if (!rstreq(type, "datahub")
            && !rstreq(type, "datalink")
            && !rstreq(type, "messagehub")
            && !rstreq(type, "messagelink")
            && !rstreq(type, "messagehub")
            && !rstreq(type, "controller")
            && !rstreq(type, "service")
            && !rstreq(type, "streamer")
            && !rstreq(type, "streamerlink")) {
                fprintf(stderr, "Com: Invalid type: %s\n", type);
                return -1;
        }
        
        return 0;
}

int check_com(membuf_t *buf, json_object_t com)
{
        for (size_t i = 0; i < json_array_length(com); i++) {
                json_object_t obj = json_array_get(com, i);
                int err = check_com_i(buf, obj);
                if (err != 0)
                        return -1;
        }
        return 0;
}

int print_com_decl_i(membuf_t *buf, const char *name __attribute__((unused)), json_object_t obj)
{
        const char *type = json_object_getstr(obj, "type");
        const char *topic = json_object_getstr(obj, "topic");
        
        if (rstreq(type, "controller")) {
                membuf_printf(buf, "static messagehub_t *messagehub_%s = NULL;\n\n", topic);
                membuf_printf(buf,
                              "messagehub_t *get_messagehub_%s()\n"
                              "{\n"
                              "        return messagehub_%s;\n"
                              "}\n\n",
                              topic, topic);
        } else {
                membuf_printf(buf, "static %s_t *%s_%s = NULL;\n\n", type, type, topic);
                membuf_printf(buf,
                              "%s_t *get_%s_%s() {\n"
                              "        return %s_%s;\n"
                              "}\n\n",
                              type,
                              type, topic,
                              type, topic);

        }
        
        if (rstreq(type, "controller")) {
                membuf_printf(buf, "static int messagehub_%s_onconnect(\n"
                              "        void *userdata,\n"
                              "        messagehub_t *hub,\n"
                              "        request_t *request,\n"
                              "        messagelink_t *link);\n", topic);
                membuf_printf(buf, "static void messagehub_%s_onmessage(void *userdata,\n"
                              "        messagelink_t *link,\n"
                              "        json_object_t message);\n\n", topic);
        } else if (rstreq(type, "messagehub")) {
                if (json_object_getstr(obj, "onmessage"))
                        membuf_printf(buf, "static int messagehub_%s_onconnect(\n"
                                      "        void *userdata,\n"
                                      "        messagehub_t *hub,\n"
                                      "        request_t *request,\n"
                                      "        messagelink_t *link);\n", topic);
                
        }
        return 0;
}

int print_com_decl(membuf_t *buf, const char *name, json_object_t com)
{
        for (size_t i = 0; i < json_array_length(com); i++) {
                json_object_t obj = json_array_get(com, i);
                int err = print_com_decl_i(buf, name, obj);
                if (err != 0)
                        return 1;
        }
        membuf_printf(buf, "\n");
        return 0;
}

int print_new_datahub(membuf_t *buf,
                      const char *name,
                      const char *topic,
                      json_object_t obj)
{
        const char *onbroadcast = json_object_getstr(obj, "onbroadcast");
        const char *ondata = json_object_getstr(obj, "ondata");
        const char *userdata = json_object_getstr(obj, "userdata");

        if (onbroadcast == NULL)
                onbroadcast = "NULL";
        if (ondata == NULL)
                ondata = "NULL";
        if (userdata == NULL)
                userdata = "NULL";
        
        membuf_printf(buf,
                      "        datahub_%s = registry_open_datahub(\"%s\", \"%s\",\n"
                      "                (datahub_onbroadcast_t) %s,\n"
                      "                (datahub_ondata_t) %s,\n"
                      "                %s);\n",
                      topic, name, topic, onbroadcast, ondata, userdata);
        membuf_printf(buf,
                      "        if (datahub_%s == NULL) {\n"
                      "                r_err(\"Failed to create the datahub\");\n"
                      "                return -1;\n"
                      "        };\n",
                      topic);
        return 0;
}

int print_new_datalink(membuf_t *buf,
                       const char *name,
                       const char *topic,
                       json_object_t obj)
{
        const char *ondata = json_object_getstr(obj, "ondata");
        const char *userdata = json_object_getstr(obj, "userdata");

        if (ondata == NULL)
                ondata = "NULL";
        if (userdata == NULL)
                userdata = "NULL";
        
        membuf_printf(buf,
                      "        datalink_%s = registry_open_datalink(\"%s\", \"%s\",\n"
                      "                (datalink_ondata_t) %s, %s);\n",
                      topic, name, topic, ondata, userdata);
        membuf_printf(buf,
                      "        if (datalink_%s == NULL) {\n"
                      "                r_err(\"Failed to create the datalink\");\n"
                      "                return -1;\n"
                      "        };\n",
                      topic);
        return 0;
}

int print_new_messagehub(membuf_t *buf,
                         const char *name,
                         const char *topic,
                         json_object_t obj)
{
        const char *onconnect = json_object_getstr(obj, "onconnect");
        const char *onmessage = json_object_getstr(obj, "onmessage");
        const char *onrequest = json_object_getstr(obj, "onrequest");
        const char *userdata = json_object_getstr(obj, "userdata");
        double port = 0;
        
        if (json_object_has(obj, "port")) {
                port = json_object_getnum(obj, "port");
                if (isnan(port)) {
                        fprintf(stderr, "Invalid port\n");
                        return -1;
                }
        }

        if (onconnect != NULL && onmessage != NULL) {
                fprintf(stderr, "Only one of onconnect and onmessage can be specified\n");
                return -1;
        }
        if (userdata == NULL)
                userdata = "NULL";
        
        if (onconnect != NULL) {
                r_debug("print_new_messagehub: using onconnect provided by input file");
                membuf_printf(buf,
                              "        messagehub_%s = registry_open_messagehub(\n"
                              "                \"%s\",\n"
                              "                \"%s\",\n"
                              "                %d, (messagehub_onconnect_t) %s,\n"
                              "                %s);\n",
                              topic, name, topic, (int) port, onconnect, userdata);
        } else if (onmessage != NULL) {
                r_debug("print_new_messagehub: using generated onconnect and "
                          "onmessage from input file");
                membuf_printf(buf,
                              "        messagehub_%s = registry_open_messagehub(\n"
                              "                \"%s\",\n"
                              "                \"%s\",\n"
                              "                %d, (messagehub_onconnect_t) messagehub_%s_onconnect,\n"
                              "                %s);\n",
                              topic, name, topic, (int) port, topic, userdata);
        } else { 
                r_debug("print_new_messagehub: no onconnect and "
                          "no onmessage were given");
                membuf_printf(buf,
                              "        messagehub_%s = registry_open_messagehub(\n"
                              "                \"%s\",\n"
                              "                \"%s\",\n"
                              "                %d, (messagehub_onconnect_t) NULL,\n"
                              "                %s);\n",
                              topic, name, topic, (int) port, userdata);
        }
        membuf_printf(buf,
                      "        if (messagehub_%s == NULL) {\n"
                      "                r_err(\"Failed to create the messagehub\");\n"
                      "                return -1;\n"
                      "        };\n",
                      topic);
        if (onrequest != NULL) {
                r_debug("print_new_messagehub: using onrequest provided by input file");
                membuf_printf(buf,
                              "        messagehub_set_onrequest(messagehub_%s, "
                              "                                 (messagehub_onrequest_t) %s);\n",
                              topic, onrequest);
        }
        return 0;
}

int print_new_messagelink(membuf_t *buf,
                          const char *name,
                          const char *topic,
                          json_object_t obj)
{
        const char *onmessage = json_object_getstr(obj, "onmessage");
        const char *userdata = json_object_getstr(obj, "userdata");

        if (onmessage == NULL)
                onmessage = "NULL";
        if (userdata == NULL)
                userdata = "NULL";
        
        membuf_printf(buf,
                      "        messagelink_%s = registry_open_messagelink(\"%s\", \"%s\",\n"
                      "                (messagelink_onmessage_t) %s, %s);\n",
                      topic, name, topic, onmessage, userdata);
        membuf_printf(buf,
                      "        if (messagelink_%s == NULL) {\n"
                      "                r_err(\"Failed to create the messagelink\");\n"
                      "                return -1;\n"
                      "        };\n",
                      topic);
        return 0;
}

int print_new_controller(membuf_t *buf,
                         const char *name,
                         const char *topic,
                         json_object_t obj)
{
        const char *userdata = json_object_getstr(obj, "userdata");
        double port = 0;
        
        if (json_object_has(obj, "port")) {
                port = json_object_getnum(obj, "port");
                if (isnan(port)) {
                        fprintf(stderr, "Invalid port\n");
                        return -1;
                }
        }

        if (userdata == NULL)
                userdata = "NULL";
        
        membuf_printf(buf, 
                      "        messagehub_%s = registry_open_messagehub(\n"
                      "                \"%s\",\n"
                      "                \"%s\",\n"
                      "                %d, messagehub_%s_onconnect, %s);\n",
                      topic, name, topic, (int) port, topic, userdata);
        membuf_printf(buf,
                      "        if (messagehub_%s == NULL) {\n"
                      "                r_err(\"Failed to create the messagehub\");\n"
                      "                return -1;\n"
                      "        };\n\n",
                      topic);

        /* membuf_printf(buf, */
        /*               "int messagehub_%s_onconnect(messagehub_t *hub, " */
        /*               "        messagelink_t *link, " */
        /*               "        void *userdata)\n\{" */
        /*               "                %d, (messagehub_onconnect_t) %s, %s);\n", */
        /*               topic, name, topic, (int) port, onconnect, userdata); */
        return 0;
}

int print_new_service(membuf_t *buf,
                      const char *name,
                      const char *topic,
                      json_object_t obj)
{
        const char *userdata = json_object_getstr(obj, "userdata");
        json_object_t resources;
        double port = 0;

        if (json_object_has(obj, "port")) {
                port = json_object_getnum(obj, "port");
                if (isnan(port)) {
                        fprintf(stderr, "Invalid port\n");
                        return -1;
                }
        }

        if (userdata == NULL)
                userdata = "NULL";
        
        membuf_printf(buf,
                "        int err;\n"
                      "        service_%s = registry_open_service(\"%s\",\n"
                      "                \"%s\",\n"
                      "                %d);\n",
                      topic, name, topic, (int) port);
        membuf_printf(buf,
                      "        if (service_%s == NULL) {\n"
                      "                r_err(\"Failed to create the service\");\n"
                      "                return -1;\n"
                      "        };\n",
                      topic);
        resources = json_object_get(obj, "resources");
        if (json_isnull(resources)) {
                        fprintf(stderr, "Service is not exporting any resources\n");
                        return -1;
        }
        if (!json_isarray(resources)) {
                fprintf(stderr, "Expected an array for the resources\n");
                return -1;
        }
        for (size_t i = 0; i < json_array_length(resources); i++) {
                json_object_t d = json_array_get(resources, i);
                if (!json_isobject(d)) {
                        fprintf(stderr, "Invalid resources description\n");
                        return -1;
                }
                const char *n = json_object_getstr(d, "name");
                const char *mout = json_object_getstr(d, "mimetype_out");
                const char *onrequest = json_object_getstr(d, "onrequest");
                if (n == NULL || mout == NULL) {
                        fprintf(stderr, "Incomplete resources description\n");
                        return -1;
                }
                const char *min = json_object_getstr(d, "mimetype_in");
                
                membuf_printf(buf,
                              "        err = service_export(service_%s,\n"
                              "               \"%s\",\n",
                              topic, n);
                
                if (min == NULL)
                        membuf_printf(buf, "               NULL,\n");
                else
                        membuf_printf(buf, "               \"%s\",\n", min);
                
                membuf_printf(buf, "               \"%s\",\n", mout);
                
                membuf_printf(buf,
                              "               %s,\n"
                              "               %s);\n"
                              "        if (err) return -1;\n",
                              userdata, onrequest);
        }
        return 0;
}

int print_new_streamer(membuf_t *buf,
                       const char *name,
                       const char *topic,
                       json_object_t obj)
{
        const char *onclient = json_object_getstr(obj, "onclient");
        const char *onbroadcast = json_object_getstr(obj, "onbroadcast");
        const char *userdata = json_object_getstr(obj, "userdata");
        const char *mimetype = json_object_getstr(obj, "mimetype");
        double port = 0;

        if (json_object_has(obj, "port")) {
                port = json_object_getnum(obj, "port");
                if (isnan(port)) {
                        fprintf(stderr, "Invalid port\n");
                        return -1;
                }
        }

        if (mimetype == NULL) {
                fprintf(stderr, "Missing mimetype for streamer %s:%s\n", name, topic);
                return -1;
        }
        if (onclient == NULL)
                onclient = "NULL";
        if (onbroadcast == NULL)
                onbroadcast = "NULL";
        if (userdata == NULL)
                userdata = "NULL";
        
        membuf_printf(buf,
                      "        streamer_%s = registry_open_streamer(\"%s\",\n"
                      "                \"%s\",\n"
                      "                %d,\n"
                      "                \"%s\",\n"
                      "                (streamer_onclient_t) %s,\n"
                      "                (streamer_onbroadcast_t) %s,\n"
                      "                %s);\n",
                      topic, name, topic, (int) port, mimetype,
                      onclient, onbroadcast, userdata);
        membuf_printf(buf,
                      "        if (streamer_%s == NULL) {\n"
                      "                r_err(\"Failed to create the streamer\");\n"
                      "                return -1;\n"
                      "        };\n",
                      topic);
        return 0;
}

int print_new_streamerlink(membuf_t *buf,
                           const char *name,
                           const char *topic,
                           json_object_t obj)
{
        const char *ondata = json_object_getstr(obj, "ondata");
        const char *onresponse = json_object_getstr(obj, "onresponse");
        const char *userdata = json_object_getstr(obj, "userdata");
        int autoconnect = json_object_getbool(obj, "autoconnect");

        
        if (ondata == NULL) {
                fprintf(stderr, "Missing 'ondata' handler\n");
                return -1;
        }
        if (onresponse == NULL)
                onresponse = "NULL";
        if (userdata == NULL)
                userdata = "NULL";
        if (autoconnect == -1)
                autoconnect = 1;
        
        membuf_printf(buf,
                      "        streamerlink_%s = registry_open_streamerlink(\n"
                      "                \"%s\",\n"
                      "                \"%s\",\n"
                      "                (streamerlink_ondata_t) %s,\n"
                      "                (streamerlink_onresponse_t) %s,\n"
                      "                %s, %d);\n",
                      topic, name, topic, ondata, onresponse, userdata, autoconnect);
        membuf_printf(buf,
                      "        if (streamerlink_%s == NULL) {\n"
                      "                r_err(\"Failed to create the streamerlink\");\n"
                      "                return -1;\n"
                      "        };\n",
                      topic);
        return 0;
}

int print_com_init_i(membuf_t *buf, const char *name, json_object_t obj)
{
        const char *type = json_object_getstr(obj, "type");
        const char *topic = json_object_getstr(obj, "topic");
        int err = 0;
        
        if (rstreq(type, "datahub")) {
                err = print_new_datahub(buf, name, topic, obj);
        } else if (rstreq(type, "datalink")) {
                err = print_new_datalink(buf, name, topic, obj);
        } else if (rstreq(type, "messagehub")) {
                err = print_new_messagehub(buf, name, topic, obj);
        } else if (rstreq(type, "messagelink")) {
                err = print_new_messagelink(buf, name, topic, obj);
        } else if (rstreq(type, "controller")) {
                err = print_new_controller(buf, name, topic, obj);
        } else if (rstreq(type, "service")) {
                err = print_new_service(buf, name, topic, obj);
        } else if (rstreq(type, "streamer")) {
                err = print_new_streamer(buf, name, topic, obj);
        } else if (rstreq(type, "streamerlink")) {
                err = print_new_streamerlink(buf, name, topic, obj);
        } else {
                fprintf(stderr, "Com: Invalid type: %s\n", type);
                return -1;
        }

        return err;
}

int print_com_init(membuf_t *buf, const char *name, json_object_t com)
{
        membuf_printf(buf, "static int init_com()\n{\n");

        for (size_t i = 0; i < json_array_length(com); i++) {
                json_object_t obj = json_array_get(com, i);
                int err = print_com_init_i(buf, name, obj);
                if (err != 0)
                        return -1;
        }

        membuf_printf(buf, "        return 0;\n");
        membuf_printf(buf, "}\n\n");
        return 0;
}

int print_com_controller_handlers(membuf_t *buf, const char *name __attribute__((unused)), json_object_t obj)
{
        const char *topic = json_object_getstr(obj, "topic");
        json_object_t commands = json_object_get(obj, "commands");
        
        if (json_isnull(commands)) {
                        fprintf(stderr, "Controller is not exporting any commands\n");
                        return 0;
        }
        if (!json_isarray(commands)) {
                fprintf(stderr, "Expected an array for the commands\n");
                return -1;
        }
        
        membuf_printf(buf,
                      "static void messagehub_%s_onmessage(void *userdata,\n"
                      "        messagelink_t *link,\n"
                      "        json_object_t message)\n{\n", topic);
        membuf_printf(buf,
                      "        static membuf_t *buffer = NULL;\n");
        membuf_printf(buf,
                      "        const char *cmd = json_object_getstr(message, \"command\");\n");
        membuf_printf(buf,
                      "        int status = 0;\n");
        membuf_printf(buf,
                      "        if (buffer == NULL) buffer = new_membuf();\n"
                      "        else membuf_clear(buffer);\n");
        membuf_printf(buf,
                      "        if (cmd == NULL) {\n"
                      "                r_warn(\"Message doesn't contain a command\");\n"
                      "                messagelink_send_f(link, \"{\\\"status\\\":\\\"error\\\", \\\"message\\\": \\\"no command given\\\"}\");\n"
                      "                return;\n"
                      "        }\n");

        for (size_t i = 0; i < json_array_length(commands); i++) {
                json_object_t d = json_array_get(commands, i);
                if (!json_isobject(d)) {
                        fprintf(stderr, "Invalid commands description\n");
                        return -1;
                }
                const char *n = json_object_getstr(d, "name");
                const char *oncommand = json_object_getstr(d, "oncommand");
                if (n == NULL) {
                        fprintf(stderr, "Incomplete commands description\n");
                        return -1;
                }

                if (i == 0) 
                        membuf_printf(buf,
                                      "        if (strcmp(cmd, \"%s\") == 0)\n"
                                      "               status = %s(userdata, link, message, buffer);\n",
                                      n, oncommand);
                else
                        membuf_printf(buf,
                                      "        else if (strcmp(cmd, \"%s\") == 0)\n"
                                      "               status = %s(userdata, link, message, buffer);\n",
                                      n, oncommand);
        }
        
        membuf_printf(buf,
                      "        else {\n"
                      "                r_warn(\"Unknown command: %%s\", cmd);\n"
                      "                messagelink_send_f(link, \"{\\\"status\\\":\\\"error\\\", \\\"message\\\": \\\"unknown command: %%s\\\"}\", cmd);\n"
                      "                return;\n"
                      "        }\n");
        membuf_printf(buf,
                      "        if (status == 0) \n"
                      "                messagelink_send_f(link, \"{\\\"status\\\":\\\"ok\\\"}\");\n"
                      "        else {\n"
                      "                membuf_append_zero(buffer);\n"
                      "                messagelink_send_f(link, \"{\\\"status\\\":\\\"error\\\", \\\"message\\\": \\\"%%s\\\"}\", membuf_data(buffer));\n"
                      "        }\n");
        membuf_printf(buf,
                      "}\n\n");


        membuf_printf(buf,
                      "static int messagehub_%s_onconnect(\n"
                      "        void *userdata,\n"
                      "        messagehub_t *hub,\n"
                      "        request_t *request,\n"
                      "        messagelink_t *link)\n{\n", topic);
        membuf_printf(buf, "        messagelink_set_userdata(link, userdata);\n");
        membuf_printf(buf, "        messagelink_set_onmessage(link, messagehub_%s_onmessage);\n", topic);
        membuf_printf(buf, "        return 0;\n");
        membuf_printf(buf, "}\n\n");
        return 0;
}

int print_com_messagehub_handlers(membuf_t *buf, const char *name __attribute__((unused)), json_object_t obj)
{
        const char *topic = json_object_getstr(obj, "topic");
        const char *onmessage = json_object_getstr(obj, "onmessage");

        if (onmessage == NULL)
                return 0;
        
        membuf_printf(buf,
                      "static int messagehub_%s_onconnect(\n"
                      "        void *userdata,\n"
                      "        messagehub_t *hub,\n"
                      "        request_t *request,\n"
                      "        messagelink_t *link)\n{\n", topic);
        membuf_printf(buf, "        messagelink_set_userdata(link, userdata);\n");
        membuf_printf(buf, "        messagelink_set_onmessage(link, (messagelink_onmessage_t) %s);\n", onmessage);
        membuf_printf(buf, "        return 0;\n");
        membuf_printf(buf, "}\n\n");
        return 0;
}

int print_com_handlers(membuf_t *buf, const char *name, json_object_t com)
{
        int err = 0;
        for (size_t i = 0; i < json_array_length(com); i++) {
                json_object_t obj = json_array_get(com, i);
                const char *type = json_object_getstr(obj, "type");
                if (rstreq(type, "controller")) {
                        err = print_com_controller_handlers(buf, name, obj);
                        if (err != 0)
                                return err;
                } else if (rstreq(type, "messagehub")) {
                        err = print_com_messagehub_handlers(buf, name, obj);
                        if (err != 0)
                                return err;
                }
        }
        return 0;
}

int print_com_cleanup_i(membuf_t *buf, json_object_t obj)
{
        const char *type = json_object_getstr(obj, "type");
        const char *topic = json_object_getstr(obj, "topic");
        if (rstreq(type, "controller")) {
                membuf_printf(buf,
                              "        if (messagehub_%s) {\n"
                              "                registry_close_messagehub(messagehub_%s);\n"
                              "                messagehub_%s = NULL;\n"
                              "        }\n",
                              topic, topic, topic);
        } else {
                membuf_printf(buf,
                              "        if (%s_%s) {\n"
                              "                registry_close_%s(%s_%s);\n"
                              "                %s_%s = NULL;\n"
                              "        }\n",
                              type, topic, type, type, topic, type, topic);
        }
        return 0;
}

int print_com_cleanup(membuf_t *buf, json_object_t com)
{
        membuf_printf(buf, "static void cleanup_com()\n{\n");
        
        for (size_t i = 0; i < json_array_length(com); i++) {
                json_object_t obj = json_array_get(com, i);
                int err = print_com_cleanup_i(buf, obj);
                if (err != 0)
                        return -1;
        }

        membuf_printf(buf, "}\n\n");
        return 0;
}

int print_com(membuf_t *buf, const char *name, json_object_t com)
{
        int err;
        
        err = print_com_decl(buf, name, com);
        if (err != 0) return -1;

        err = print_com_init(buf, name, com);
        if (err != 0) return -1;

        err = print_com_handlers(buf, name, com);
        if (err != 0) return -1;

        err = print_com_cleanup(buf, com);
        if (err != 0) return -1;

        return 0;
}
       
int print_init(membuf_t *buf, const char *name, const char *init_func)
{
        membuf_printf(buf, 
                      "static int init(int argc, char **argv)\n"
                      "{\n"
                      "        int err = 0;\n"
                      "        \n");

        if (init_func == NULL) {
                membuf_printf(buf, 
                              "        err = %s_init(argc, argv);\n"
                              "        if (err != 0)\n"
                              "                return err;\n"
                              "\n", name);
                
        } else if (rstreq(init_func, "null")) {
                ; // no init function
                
        } else {
                membuf_printf(buf, 
                              "        err = %s(argc, argv);\n"
                              "        if (err != 0)\n"
                              "                return err;\n"
                              "\n", init_func);
        }
        
        membuf_printf(buf, 
                      "        err = init_com();\n"
                      "        if (err != 0)\n"
                      "                return err;\n"
                      "\n"
                      "        return 0;\n"
                      "}\n");
        
        membuf_printf(buf, "\n");
        return 0;
}
       
int print_cleanup(membuf_t *buf, const char *name, const char *cleanup_func)
{
        membuf_printf(buf, "static void cleanup()\n{\n");
        if (cleanup_func == NULL) {
                membuf_printf(buf, "        %s_cleanup();\n", name);
        } else if (rstreq(cleanup_func, "null")) {
                ; // no cleanup function                
        } else {
                membuf_printf(buf, "        %s();\n", cleanup_func);
        }
        membuf_printf(buf, "        cleanup_com();\n}\n");
        membuf_printf(buf, "\n");
        return 0;
}

int print_idle(membuf_t *buf, const char *idle_func)
{
        membuf_printf(buf, "static void idle()\n{\n");

        if (idle_func)
                membuf_printf(buf, "        %s();\n", idle_func);
        else 
                membuf_printf(buf, "        clock_sleep(1.0);\n");
                
        membuf_printf(buf, "}\n\n");
        return 0;
}

int wite_code(const char *filename, membuf_t *buf)
{
        FILE *fp = fopen(filename, "w");
        if (fp == NULL) {
                fprintf(stderr, "Failed to open the output file '%s'\n", filename);
                return -1;
        }

        membuf_append_zero(buf);
        fprintf(fp, "%s", membuf_data(buf));
        fclose(fp);
        return 0;
}

int generate_code(const char *inputfile, const char *outputfile)
{
        const char *name = NULL;
        const char *idle_func = NULL;
        const char *init_func = NULL;
        const char *cleanup_func = NULL;
        const char *code_output = NULL;
        const char *header = NULL;
        json_object_t def;
        json_object_t com;
        int err;
        char errmsg[256];        
        membuf_t *buf = new_membuf();
        
        def = json_load(inputfile, &err, errmsg, sizeof(errmsg));
        if (err != 0) {
                fprintf(stderr, "Failed to load the definition file: %s\n", errmsg);
                return 1;
        }

        if (outputfile != NULL)
                code_output = outputfile;
        else {
                code_output = json_object_getstr(def, "output");
                if (code_output == NULL) {
                        fprintf(stderr, "Missing output file\n");
                        err = 1;
                        goto cleanup_and_return;
                }
        }
        
        name = json_object_getstr(def, "name");
        if (name == NULL) {
                fprintf(stderr, "Missing name\n");
                err = 1;
                goto cleanup_and_return;
        }
        
        idle_func = json_object_getstr(def, "idle");
        
        if (!json_object_has(def, "init")) {
                init_func = NULL;
                r_debug("No init function specified. Using default name.");
        } else if (json_isnull(json_object_get(def, "init"))) {
                init_func = "null";
                r_debug("Init function is set to null.");
        } else {
                init_func = json_object_getstr(def, "init"); 
                r_debug("Init function is set to '%s'.", init_func);
        }
        
        if (!json_object_has(def, "cleanup")) 
                cleanup_func = NULL;
        else if (json_isnull(json_object_get(def, "cleanup"))) 
                cleanup_func = "null";
        else cleanup_func = json_object_getstr(def, "cleanup");

        
        com = json_object_get(def, "com");
        if (json_isnull(com)) {
                fprintf(stderr, "Missing com section\n");
                err = 1;
                goto cleanup_and_return;
        }
        if (!json_isarray(com)) {
                fprintf(stderr, "Com section should be an array\n");
                err = 1;
                goto cleanup_and_return;
        }
        if (json_array_length(com) == 0) {
                fprintf(stderr, "Empty com section\n");
                err = 1;
                goto cleanup_and_return;
        }
        err = check_com(buf, com);
        if (err != 0) goto cleanup_and_return;
        
        header = json_object_getstr(def, "header");
        err = print_headers(buf, name, header);
        if (err != 0) goto cleanup_and_return;
        
        err = print_com(buf, name, com);
        if (err != 0) goto cleanup_and_return;
        
        err = print_init(buf, name, init_func);
        if (err != 0) goto cleanup_and_return;
        
        err = print_cleanup(buf, name, cleanup_func);
        if (err != 0) goto cleanup_and_return;
        
        err = print_idle(buf, idle_func);
        if (err != 0) goto cleanup_and_return;
        
        err = print_main(buf);
        if (err != 0) goto cleanup_and_return;

        err = wite_code(code_output, buf);
        if (err != 0) goto cleanup_and_return;

cleanup_and_return:
        json_unref(def);
        delete_membuf(buf);
        return err;
}

int write_cmakelists(const char *exec, const char *codefile,
                     json_object_t sources, const char *outputfile)
{
        membuf_t *buf = new_membuf();

        membuf_printf(buf,"cmake_minimum_required(VERSION 2.6)\n");
        membuf_printf(buf, "project(%s)\n", exec);
        
        membuf_printf(buf, "set(SOURCES");
        for (size_t i = 0; i < json_array_length(sources); i++) {
                const char *s = json_array_getstr(sources, i);
                if (s == NULL) {
                        fprintf(stderr, "Invalid source file\n");
                        return -1;
                }
                membuf_printf(buf, " %s", s);
        }
        membuf_printf(buf, " %s)\n", codefile);
        
        membuf_printf(buf, "add_executable(%s SOURCES)\n", exec);
        membuf_printf(buf, "target_link_libraries(%s rcutil)\n", exec);
        membuf_append_zero(buf);
        
        FILE *fp = fopen(outputfile, "w");
        if (fp == NULL) {
                fprintf(stderr, "Failed to open CMakeLists.txt\n");
                return -1;
        }
        fprintf(fp, "%s", membuf_data(buf));
        fclose(fp);
        delete_membuf(buf);
        return 0;
}

int generate_cmakelists(const char *inputfile, const char *outputfile)
{
        int err;
        json_object_t def;
        char errmsg[256];        
        const char *exec_name = NULL;
        json_object_t sources;
        const char *code_output = NULL;
        
        def = json_load(inputfile, &err, errmsg, sizeof(errmsg));
        if (err != 0) {
                fprintf(stderr, "Failed to load the definition file: %s\n", errmsg);
                return 1;
        }

        code_output = json_object_getstr(def, "output");
        if (code_output == NULL) {
                fprintf(stderr, "Missing name source code file\n");
                return 1;
        }
        
        sources = json_object_get(def, "sources");
        if (sources == NULL) {
                fprintf(stderr, "Missing source code files\n");
                return 1;
        }
        if (!json_isarray(sources)) {
                fprintf(stderr, "The source definition should be an array\n");
                return 1;
        }

        exec_name = json_object_getstr(def, "exec");
        if (exec_name == NULL) {
                fprintf(stderr, "Missing name of executable\n");
                return 1;
        }

        err = write_cmakelists(exec_name, code_output, sources, outputfile);
        if (err != 0)
                return 1;

        return 0;
}

void print_usage()
{
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "    rcgen code output-file [input-file]\n");
        fprintf(stderr, "    rcgen cmakelists [output-file] [input-file]\n");
        fprintf(stderr, "    rcgen help\n");
}

int main(int argc, char **argv)
{
        int err;
        const char *op = NULL;
        const char *inputfile = NULL;
        const char *outputfile = NULL;

        app_init(&argc, argv);
        
        if (argc < 2) {
                print_usage();
                return 1;
        }

        op = argv[1];
        if (!rstreq(op, "code") && !rstreq(op, "cmakelists")
            && !rstreq(op, "help") && !rstreq(op, "-h")) {
                fprintf(stderr, "Unknown operation: %s\n", op);
                return 1;
        }
        
        if (rstreq(op, "help") || rstreq(op, "-h")) {
                print_usage();
                return 0;
                
        } else if (rstreq(op, "code")) {
                if (argc == 3) {
                        outputfile = argv[2];
                        inputfile = "rcgen.json";
                } else if (argc == 4) {
                        outputfile = argv[2];
                        inputfile = argv[3];
                } else {
                        print_usage();
                        return 1;
                }
                err = generate_code(inputfile, outputfile);
                
        } else if (rstreq(op, "cmakelists")) {
                if (argc == 2) {
                        outputfile = "CMakeLists.txt";
                        inputfile = "rcgen.json";
                } else if (argc == 3) {
                        outputfile = argv[2];
                        inputfile = "rcgen.json";
                } else if (argc == 4) {
                        outputfile = argv[2];
                        inputfile = argv[3];
                } else {
                        print_usage();
                        return 1;
                }
                err = generate_cmakelists(inputfile, outputfile);
        }
        
        return err? 1 : 0;
}
