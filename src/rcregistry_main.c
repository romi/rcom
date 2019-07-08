#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include <rcom.h>

#include "rcregistry_priv.h"
#include "registry_priv.h"
#include "service_priv.h"
#include "util.h"
#include "http.h"
#include "mem.h"

static rcregistry_t *rcregistry = NULL;
static service_t *service = NULL;
static char *_dir = "html/rcregistry";

static int rcregistry_send_file(const char *path,
                                const char *mimetype,
                                request_t *request)
{
	log_debug("rcregistry_send_file");
	
        FILE *fp = fopen(path, "r");
        if (fp == NULL) {
                log_err("Failed to open %s", path);
                return -1;
        }

	log_debug("rcregistry_send_file: settings mimetype to '%s'", mimetype);
        request_set_mimetype(request, mimetype);
        
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

static int rcregistry_registry(void *data, request_t *request)
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

static int rcregistry_listen(void *data, request_t *request)
{
        char b[64];
        if (request_args(request) == NULL) {
                return -1;
        }

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
                             "      $(document).ready(function() { listenTo(\"ws://%s\"); });\n"
                             "    </script>\n"
                             "  </body>\n"
                             "</html>\n",
                             request_args(request));        
        return 0;
}

static int set_server_dir(const char *path)
{
        char initial_path[PATH_MAX];
        char resolved_path[PATH_MAX];
        struct stat statbuf;
        
        if (path[0] != '/') {
                char cwd[PATH_MAX];
                char s[PATH_MAX];
                if (getcwd(cwd, PATH_MAX) == NULL) {
                        char reason[200];
                        strerror_r(errno, reason, 200);
                        log_err("getcwd failed: %s", reason);
                        return -1;
                }
                snprintf(initial_path, PATH_MAX, "%s/%s", cwd, path);
                initial_path[PATH_MAX-1] = 0;
        } else {
                snprintf(initial_path, PATH_MAX, "%s", path);
                initial_path[PATH_MAX-1] = 0;
        }

        if (realpath(initial_path, resolved_path) == NULL) {
                char reason[200];
                strerror_r(errno, reason, 200);
                log_err("realpath failed: %s", reason);
                return -1;
        }
        if (stat(resolved_path, &statbuf) != 0) {
                char reason[200];
                strerror_r(errno, reason, 200);
                log_err("stat failed: %s", reason);
                return -1;
        }
        if ((statbuf.st_mode & S_IFMT) != S_IFDIR) {
                log_err("Not a directory: %s", resolved_path);
                return -1;
        }
        
        _dir = mem_strdup(resolved_path);
        log_info("Serving files from directory %s", _dir);
        return 0;
}

static int check_path(const char *filename, char *path, int len)
{
        char requested_path[PATH_MAX];
        char resolved_path[PATH_MAX];
        struct stat statbuf;
        
        snprintf(requested_path, PATH_MAX, "%s/%s", _dir, filename);
        requested_path[PATH_MAX-1] = 0;

        if (realpath(requested_path, resolved_path) == NULL) {
                char reason[200];
                strerror_r(errno, reason, 200);
                log_err("realpath failed: %s", reason);
                return -1;
        }
        if (strncmp(_dir, resolved_path, strlen(_dir)) != 0) {
                log_err("File not in server path: %s", resolved_path);
                return -1;
        }
        if (stat(resolved_path, &statbuf) != 0) {
                char reason[200];
                strerror_r(errno, reason, 200);
                log_err("stat failed: %s", reason);
                return -1;
        }
        if ((statbuf.st_mode & S_IFMT) != S_IFREG) {
                log_err("Not a regular file: %s", resolved_path);
                return -1;
        }
        snprintf(path, len, "%s", resolved_path);
        return 0;
}

static int rcregistry_localfile(void *data, request_t *request)
{
        char path[PATH_MAX];

	log_debug("rcregistry_localfile");
        const char *filename = request_name(request);
	
        if (filename[0] == '/')
                filename++;
        
        const char *mimetype = filename_to_mimetype(filename);
        if (mimetype == NULL)
                return -1;
        
        if (check_path(filename, path, PATH_MAX) != 0)
                return -1;
        
        return rcregistry_send_file(path, mimetype, request);
}


int main(int argc, char **argv)
{
        int err;
        const char *d = "html/rcregistry";
	
        app_init(&argc, argv);

        if (argc >= 2)
	        d = argv[1];

	err = set_server_dir(d);
	if (err != 0) {
	        log_err("Invalid servir dir: '%s'", d);
                return 1;
	}

        rcregistry = new_rcregistry();
        if (rcregistry == NULL)
                return 1;

        service = new_service("rcregistry", 10100);
        if (service == NULL) {
                log_err("Failed to create the HTML service. Quitting.");
                delete_rcregistry(rcregistry);
                return 1;
        }
        
        err = service_export(service, "registry.html", NULL, "text/html",
			     NULL, rcregistry_registry);
        if (err != 0) {
                log_err("Failed to export the HTML page. Quitting.");
                delete_rcregistry(rcregistry);
                delete_service(service);
                return 1;
        }
        
        err = service_export(service, "listen.html", NULL, "text/html",
			     NULL, rcregistry_listen);
        if (err != 0) {
                log_err("Failed to export the HTML page. Quitting.");
                delete_rcregistry(rcregistry);
                delete_service(service);
                return 1;
        }
        
        err = service_export(service, "*", NULL, "text/html",
			     NULL, rcregistry_localfile);
        if (err != 0) {
                log_err("Failed to export the HTML page. Quitting.");
                delete_rcregistry(rcregistry);
                delete_service(service);
                return 1;
        }
        
        while (!app_quit()) {
                clock_sleep(1);
        }

        delete_rcregistry(rcregistry);
        delete_service(service);
        return 0;
}
