#include <stdio.h>
#include <rcom.h>
#include "membuf.h"
#include "util.h"

int print_headers(membuf_t *buf, const char *name)
{
        membuf_printf(buf, "#include <rcom.h>\n");
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
                "                log_err(\"init() failed\");\n"
                "                goto cleanup_and_exit;\n"
                "        }\n"
                "\n"
                "        while (!app_quit())\n"
                "                idle();\n"
                "\n"
                "cleanup_and_exit:\n"
                "\n"
                "        cleanup();\n"
                "\n"
                "        return err? 1 : 0;\n"
                "}\n";
        membuf_printf(buf, "%s\n", m);
        return 0;
}

int check_com_i(membuf_t *buf, json_object_t obj)
{
        const char *type = NULL;
        const char *name = NULL;
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
        
        name = json_object_getstr(obj, "name");
        if (name == NULL) {
                fprintf(stderr, "Com: Missing name\n");
                return -1;
        }
        
        topic = json_object_getstr(obj, "topic");
        if (topic == NULL) {
                fprintf(stderr, "Com: Missing topic\n");
                return -1;
        }

        if (!streq(type, "datahub")
            && !streq(type, "datalink")
            && !streq(type, "messagehub")
            && !streq(type, "messagelink")
            && !streq(type, "messagehub")
            && !streq(type, "service")
            && !streq(type, "streamer")
            && !streq(type, "streamerlink")) {
                fprintf(stderr, "Com: Invalid type: %s\n", type);
                return -1;
        }
        
        return 0;
}

int check_com(membuf_t *buf, json_object_t com)
{
        for (int i = 0; i < json_array_length(com); i++) {
                json_object_t obj = json_array_get(com, i);
                int err = check_com_i(buf, obj);
                if (err != 0)
                        return -1;
        }
        return 0;
}

int print_com_decl_i(membuf_t *buf, json_object_t obj)
{
        const char *type = json_object_getstr(obj, "type");
        const char *name = json_object_getstr(obj, "name");
        membuf_printf(buf, "static %s_t *%s_%s = NULL;\n", type, type, name);
        return 0;
}

int print_com_decl(membuf_t *buf, json_object_t com)
{
        for (int i = 0; i < json_array_length(com); i++) {
                json_object_t obj = json_array_get(com, i);
                int err = print_com_decl_i(buf, obj);
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
                      name, name, topic, onbroadcast, ondata, userdata);
        membuf_printf(buf,
                      "        if (datahub_%s == NULL) {\n"
                      "                log_err(\"Failed to create the datahub\");\n"
                      "                return -1;\n"
                      "        };\n",
                      name);
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
                      name, name, topic, ondata, userdata);
        membuf_printf(buf,
                      "        if (datalink_%s == NULL) {\n"
                      "                log_err(\"Failed to create the datalink\");\n"
                      "                return -1;\n"
                      "        };\n",
                      name);
        return 0;
}

int print_new_messagehub(membuf_t *buf,
                         const char *name,
                         const char *topic,
                         json_object_t obj)
{
}

int print_new_messagelink(membuf_t *buf,
                          const char *name,
                          const char *topic,
                          json_object_t obj)
{
}

int print_new_service(membuf_t *buf,
                      const char *name,
                      const char *topic,
                      json_object_t obj)
{
}

int print_new_streamer(membuf_t *buf,
                       const char *name,
                       const char *topic,
                       json_object_t obj)
{
        const char *onclient = json_object_getstr(obj, "onclient");
        const char *onbroadcast = json_object_getstr(obj, "onbroadcast");
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

        if (onclient == NULL)
                onclient = "NULL";
        if (onbroadcast == NULL) {
                fprintf(stderr, "Missing 'onbroadcast' handler\n");
                return -1;
        }
        if (userdata == NULL)
                userdata = "NULL";
        
        membuf_printf(buf,
                      "        streamer_%s = registry_open_streamer(\"%s\",\n"
                      "                \"%s\",\n"
                      "                %d,\n"
                      "                (streamer_onclient_t) %s,\n"
                      "                (streamer_onbroadcast_t) %s,\n"
                      "                %s);\n",
                      name, name, topic, (int) port, onclient, onbroadcast, userdata);
        membuf_printf(buf,
                      "        if (streamer_%s == NULL) {\n"
                      "                log_err(\"Failed to create the streamer\");\n"
                      "                return -1;\n"
                      "        };\n",
                      name);
        resources = json_object_get(obj, "resources");
        if (json_isnull(resources)) {
                        fprintf(stderr, "Streamer is not exporting any resources\n");
                        return -1;
        }
        if (!json_isarray(resources)) {
                fprintf(stderr, "Expected an array for the resources\n");
                return -1;
        }
        for (int i = 0; i < json_array_length(resources); i++) {
                json_object_t d = json_array_get(resources, i);
                if (!json_isobject(d)) {
                        fprintf(stderr, "Invalid resources description\n");
                        return -1;
                }
                const char *n = json_object_getstr(d, "name");
                const char *m = json_object_getstr(d, "mime");
                if (n == NULL || m == NULL) {
                        fprintf(stderr, "Incomplete resources description\n");
                        return -1;
                }
                membuf_printf(buf,
                              "        err = streamer_export(streamer_%s,\n"
                              "               \"%s\",\n"
                              "               \"%s\");\n"
                              "        if (err) return -1;\n",
                              name, n, m);
        }
        return 0;
}

int print_new_streamerlink(membuf_t *buf,
                           const char *name,
                           const char *topic,
                           json_object_t obj)
{
        const char *resource = json_object_getstr(obj, "resource");
        const char *ondata = json_object_getstr(obj, "ondata");
        const char *userdata = json_object_getstr(obj, "userdata");

        if (resource == NULL) {
                fprintf(stderr, "Missing 'resource' field\n");
                return -1;
        }
        if (ondata == NULL) {
                fprintf(stderr, "Missing 'ondata' handler\n");
                return -1;
        }
        if (userdata == NULL)
                userdata = "NULL";
        
        membuf_printf(buf,
                      "        streamerlink_%s = registry_open_streamerlink(\"%s\","
                      "                \"%s\", \"%s\",\n"
                      "                (streamerlink_ondata_t) %s, %s);\n",
                      name, name, topic, resource, ondata, userdata);
        membuf_printf(buf,
                      "        if (streamerlink_%s == NULL) {\n"
                      "                log_err(\"Failed to create the streamerlink\");\n"
                      "                return -1;\n"
                      "        };\n",
                      name);
        return 0;
}

int print_com_init_i(membuf_t *buf, json_object_t obj)
{
        const char *type = json_object_getstr(obj, "type");
        const char *name = json_object_getstr(obj, "name");
        const char *topic = json_object_getstr(obj, "topic");
        int err = 0;
        
        if (streq(type, "datahub")) {
                err = print_new_datahub(buf, name, topic, obj);
        } else if (streq(type, "datalink")) {
                err = print_new_datalink(buf, name, topic, obj);
        } else if (streq(type, "messagehub")) {
                err = print_new_messagehub(buf, name, topic, obj);
        } else if (streq(type, "messagelink")) {
                err = print_new_messagelink(buf, name, topic, obj);
        } else if (streq(type, "messagehub")) {
                err = print_new_messagehub(buf, name, topic, obj);
        } else if (streq(type, "service")) {
                err = print_new_service(buf, name, topic, obj);
        } else if (streq(type, "streamer")) {
                err = print_new_streamer(buf, name, topic, obj);
        } else if (streq(type, "streamerlink")) {
                err = print_new_streamerlink(buf, name, topic, obj);
        } else {
                fprintf(stderr, "Com: Invalid type: %s\n", type);
                return -1;
        }

        return err;
}

int print_com_init(membuf_t *buf, json_object_t com)
{
        membuf_printf(buf, "static int init_com()\n{\n");
        membuf_printf(buf, "        int err;\n");
        
        for (int i = 0; i < json_array_length(com); i++) {
                json_object_t obj = json_array_get(com, i);
                int err = print_com_init_i(buf, obj);
                if (err != 0)
                        return -1;
        }

        membuf_printf(buf, "        return 0;\n");
        membuf_printf(buf, "}\n\n");
        return 0;
}

int print_com_cleanup_i(membuf_t *buf, json_object_t obj)
{
        const char *type = json_object_getstr(obj, "type");
        const char *name = json_object_getstr(obj, "name");
        membuf_printf(buf,
                      "        if (%s_%s)\n"
                      "                registry_close_%s(%s_%s);\n",
                      type, name, type, type, name);
        return 0;
}

int print_com_cleanup(membuf_t *buf, json_object_t com)
{
        membuf_printf(buf, "static void cleanup_com()\n{\n");
        
        for (int i = 0; i < json_array_length(com); i++) {
                json_object_t obj = json_array_get(com, i);
                int err = print_com_cleanup_i(buf, obj);
                if (err != 0)
                        return -1;
        }

        membuf_printf(buf, "}\n\n");
        return 0;
}

int print_com(membuf_t *buf, json_object_t com)
{
        int err;
        
        err = print_com_decl(buf, com);
        if (err != 0) return -1;

        err = print_com_init(buf, com);
        if (err != 0) return -1;

        err = print_com_cleanup(buf, com);
        if (err != 0) return -1;

        return 0;
}
       
int print_init(membuf_t *buf, const char *name)
{
        membuf_printf(buf, 
                      "static int init(int argc, char **argv)\n"
                      "{\n"
                      "        int err = 0;\n"
                      "        \n");

        membuf_printf(buf, 
                      "        err = %s_init(argc, argv);\n"
                      "        if (err != 0)\n"
                      "                return err;\n"
                      "\n", name);
        
        membuf_printf(buf, 
                      "        err = init_com();\n"
                      "        if (err != 0)\n"
                      "                return err;\n"
                      "\n"
                      "        return 0;\n"
                      "}\n");
        
        membuf_printf(buf, "\n");
}
       
int print_cleanup(membuf_t *buf, const char *name)
{
        membuf_printf(buf, "static void cleanup()\n{\n");
        membuf_printf(buf, "        %s_cleanup();\n", name);
        membuf_printf(buf, "        cleanup_com();\n}\n");
        membuf_printf(buf, "\n");
}

int print_idle(membuf_t *buf, const char *idle_func)
{
        membuf_printf(buf, "static void idle()\n{\n");

        if (idle_func)
                membuf_printf(buf, "        %s();\n", idle_func);
        else 
                membuf_printf(buf, "        clock_sleep(1.0);\n");
                
        membuf_printf(buf, "}\n\n");
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
        const char *code_output = NULL;
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
                        fprintf(stderr, "Missing name source code file\n");
                        return 1;
                }
        }
        
        name = json_object_getstr(def, "name");
        if (name == NULL) {
                fprintf(stderr, "Missing name\n");
                return 1;
        }
        
        idle_func = json_object_getstr(def, "idle");

        com = json_object_get(def, "com");
        if (json_isnull(com)) {
                fprintf(stderr, "Missing com section\n");
                return 1;
        }
        if (!json_isarray(com)) {
                fprintf(stderr, "Com section should be an array\n");
                return 1;
        }
        if (json_array_length(com) == 0) {
                fprintf(stderr, "Empty com section\n");
                return 1;
        }
        err = check_com(buf, com);
        if (err != 0)
                return 1;

        err = print_headers(buf, name);
        if (err != 0)
                return 1;
        
        err = print_com(buf, com);
        if (err != 0)
                return 1;

        err = print_init(buf, name);
        if (err != 0)
                return 1;

        err = print_cleanup(buf, name);
        if (err != 0)
                return 1;

        err = print_idle(buf, idle_func);
        if (err != 0)
                return 1;
        
        err = print_main(buf);
        if (err != 0)
                return 1;

        err = wite_code(code_output, buf);
        if (err != 0)
                return 1;

        delete_membuf(buf);
        return 0;
}

int wite_cmakelists(const char *exec, const char *codefile, json_object_t sources)
{
        membuf_t *buf = new_membuf();

        membuf_printf(buf,"cmake_minimum_required(VERSION 2.6)\n");
        membuf_printf(buf, "project(%s)\n", exec);
        
        membuf_printf(buf, "set(SOURCES");
        for (int i = 0; i < json_array_length(sources); i++) {
                const char *s = json_array_getstr(sources, i);
                if (s == NULL) {
                        fprintf(stderr, "Invalid source file\n");
                        return -1;
                }
                membuf_printf(buf, " %s", s);
        }
        membuf_printf(buf, " %s)\n", codefile);
        
        membuf_printf(buf, "add_executable(%s SOURCES)\n", exec);
        membuf_printf(buf, "target_link_libraries(%s rcom)\n", exec);
        membuf_append_zero(buf);
        
        FILE *fp = fopen("CMakeLists.txt", "w");
        if (fp == NULL) {
                fprintf(stderr, "Failed to open CMakeLists.txt\n");
                return -1;
        }
        fprintf(fp, "%s", membuf_data(buf));
        fclose(fp);
        delete_membuf(buf);
        return 0;
}

int generate_cmakelists(const char *inputfile)
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

        err = wite_cmakelists(exec_name, code_output, sources);
        if (err != 0)
                return 1;

        return 0;
}

int main(int argc, char **argv)
{
        int err;
        const char *inputfile = NULL;
        const char *outputfile = NULL;
        
        if (argc < 2) {
                fprintf(stderr, "Usage: rcgen input-file [output-file]\n");
                return 1;
        }

        inputfile = argv[1];
        if (argc >= 3) 
                outputfile = argv[2];
        
        err = generate_code(inputfile, outputfile);

        return err? 1 : 0;
}
