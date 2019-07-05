#include <rcom.h>

#include "rcregistry_priv.h"
#include "registry_priv.h"
#include "service_priv.h"
#include "util.h"
#include "http.h"

static rcregistry_t *rcregistry = NULL;
static service_t *service = NULL;
static char *_dir = "../html/rcregistry";

static int rcregistry_send_file(const char *filename, request_t *request)
{
        char path[256];
        snprintf(path, 256, "%s/%s", _dir, filename);
        
        FILE *fp = fopen(path, "r");
        if (fp == NULL) {
                log_err("Failed to open %s", path);
                return -1;
        }
        
        while (1) {
                size_t num;
                char buffer[256];
                num = fread(buffer, 1, 256, fp);
                request_reply_append(request, buffer, num);
                if (feof(fp)) break;
                if (ferror(fp)) {
                        log_err("Failed to read %s", path);
                        fclose(fp);
                        return -1;
                }
        }
        fclose(fp);
        return 0;
}

static int rcregistry_localfile(void *data, request_t *request)
{
        const char *filename = request_name(request);
        if (filename[0] == '/')
                filename++;
        if (!streq(filename, "index.html")
            && !streq(filename, "index.js")
            && !streq(filename, "index.css")
            && !streq(filename, "jquery.min.js"))
                return -1;
        request_set_mimetype(request, filename_to_mimetype(filename));
        return rcregistry_send_file(filename, request);
}

static int rcregistry_index(void *data, request_t *request)
{
        char b[64];
        request_reply_printf(request,
                             "<!DOCTYPE html>\n"
                             "<html lang=\"en\">\n"
                             "  <head>\n"
                             "    <meta charset=\"utf-8\">\n"
                             "    <title>Registry</title>\n"
                             "    <link rel=\"stylesheet\" href=\"index.css\">\n"
                             "    <script src=\"index.js\"></script>\n"
                             "    <script src=\"jquery.min.js\"></script>\n"
                             "  </head>\n"
                             "  <body>\n"
                             "    <div id=\"registry\"></div>\n"
                             "    <script>\n"
                             "      $(document).ready(function() { showRegistry(\"ws://%s\"); });\n"
                             "    </script>\n"
                             "  </body>\n"
                             "</html>\n",
                             addr_string(rcregistry_addr(rcregistry), b, sizeof(b)));        
        return 0;
}

int main(int argc, char **argv)
{
        int err;
        
        app_init(&argc, argv);

        rcregistry = new_rcregistry();
        if (rcregistry == NULL)
                return 0;

        service = new_service("rcregistry", 10100);
        if (service == NULL) {
                log_err("Failed to create the HTML service. Quitting.");
                delete_rcregistry(rcregistry);
                return 0;
        }
        
        err = service_export(service, "index.html", NULL, "text/html", NULL, rcregistry_index);
        if (err != 0) {
                log_err("Failed to export the HTML page. Quitting.");
                delete_rcregistry(rcregistry);
                delete_service(service);
                return 0;
        }
        
        err = service_export(service, "/", NULL, "text/html", NULL, rcregistry_index);
        if (err != 0) {
                log_err("Failed to export the HTML page. Quitting.");
                delete_rcregistry(rcregistry);
                delete_service(service);
                return 0;
        }
        
        err = service_export(service, "*", NULL, "text/html", NULL, rcregistry_localfile);
        if (err != 0) {
                log_err("Failed to export the HTML page. Quitting.");
                delete_rcregistry(rcregistry);
                delete_service(service);
                return 0;
        }
        
        service_run_in_thread(service);

        while (!app_quit()) {
                clock_sleep(1);
        }

        delete_rcregistry(rcregistry);
        delete_service(service);
        return 0;
}
