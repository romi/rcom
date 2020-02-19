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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <execinfo.h>
#include <setjmp.h>
#include <getopt.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <libgen.h>
#include <unistd.h>
#include <r.h>

#include "rcom/app.h"
#include "rcom/dump.h"
#include "rcom/util.h"

#include "registry_priv.h"
#include "proxy.h"

static jmp_buf env;
static int interrupt_count = 0;

static void app_update_logfile();
static int diagnostics_set_signal_handlers(); 
static int diagnostics_set_signal_handler(int signum); 
static void diagnostics_scan_memory();
static void rcom_cleanup();
static void app_set_config(const char *path);

static int _quit = 0;
static char *_ip = NULL;
static char *_logdir = NULL;
static char *_session = NULL;
static char *_name = NULL;
static char *_config = NULL;
static int _print = 0;

int app_quit()
{
        return _quit;
}

void app_set_quit()
{
        _quit++;
}

int app_print()
{
        return _print;
}

void app_set_print()
{
        _print++;
}

extern int opterr;

int app_parse_args(int *argc, char **argv)
{
        int option_index = 0;
        int c, i, j;

        //opterr = 0;

        static char *optchars = "A:N:P:L:I:R:D::C:s:pa";
        static struct option long_options[] = {
                {"registry-name", required_argument, 0, 'N'},
                {"registry-addr", required_argument, 0, 'A'},
                {"registry-port", required_argument, 0, 'P'},
                {"ip", required_argument, 0, 'I'},
                {"session", required_argument, 0, 's'},
                {"log-dir", required_argument, 0, 'L'},
                {"dump", optional_argument, 0, 'D'},
                {"replay", required_argument, 0, 'R'},
                {"print", no_argument, 0, 'p'},
                {"stand-alone", no_argument, 0, 'a'},
                {"config", required_argument, 0, 'C'},
                {0, 0, 0, 0}
        };

        while (1) {
                c = getopt_long(*argc, argv, optchars, long_options, &option_index);
                if (c == -1)
                        break;

                switch (c) {
                case 'N':
                        r_info("app_parse_args: setting registry name to '%s'",
                                 optarg);
                        set_registry_name(optarg);
                        break;
                case 'A':
                        r_info("app_parse_args: setting registry address to '%s'",
                                 optarg);
                        set_registry_ip(optarg);
                        break;
                case 'P':
                        r_info("app_parse_args: setting registry port to %d",
                                 atoi(optarg));
                        set_registry_port(atoi(optarg));
                        break;
                case 'I':
                        r_info("app_parse_args: setting default IP address to %s",
                                 optarg);
                        _ip = r_strdup(optarg);
                        break;
                case 'L':
                        app_set_logdir(optarg);
                        break;
                case 's':
                        app_set_session(optarg);
                        break;
                case 'C':
                        app_set_config(optarg);
                        break;
                case 'D':
                        set_dumping(1);
                        if (optarg) 
                                set_dumping_dir(optarg);
                        else 
                                set_dumping_dir(".");
                        break;
                case 'p':
                        app_set_print(1);
                        break;
                case 'a':
                        set_registry_standalone(1);
                        break;
                case 'R':
                        set_replay_id(optarg);
                        break;
                }
        }

        for (i = optind, j = 1; i < *argc; i++, j++) {
                argv[j] = argv[i];
        }
        argv[j] = NULL;
        *argc = j;

        return 0;
}

static int app_check_ip()
{
        if (_ip == NULL)
                _ip = r_strdup("0.0.0.0");
        return 0;
}

static int app_check_ip_HIDE()
{
        struct ifaddrs *ifaddr, *ifa;
        int family, s;
        char host[NI_MAXHOST];

        if (_ip != NULL)
                return 0;
        
        if (getifaddrs(&ifaddr) == -1) {
                perror("getifaddrs");
                return -1;
        }

        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr == NULL)
                        continue;  
                
                s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                                host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

                if (s != 0)
                        continue;

                if (ifa->ifa_addr->sa_family == AF_INET) {
                        if (!rstreq(host, "127.0.0.1")) {
                                r_warn("app_check_ip: Using '%s' by default",
                                       host);
                                r_warn("app_check_ip: Consider using "
                                       "the --ip=... option");
                                _ip = r_strdup(host);
                                break;
                        }
                }
        }

        if (_ip == NULL)
                _ip = r_strdup("0.0.0.0");
        
        freeifaddrs(ifaddr);
        return 0;
}

const char *app_ip()
{
        return _ip;
}

int app_standalone()
{
        return get_registry_standalone();
}

void app_init(int *argc, char **argv)
{
        int ret;

        if (r_init(argc) != 0)
                exit(1);

        //_logdir = r_strdup(".");
        app_set_name(argv[0]);

        if (r_log_init() != 0) {
		r_panic("rcom_log_init failed");
                exit(1);
        }
                
        proxy_init();
        
        ret = diagnostics_set_signal_handlers();
        atexit(rcom_cleanup);
        
        if (setjmp(env) != 0) {
		r_panic("Abort (longjmp)");
                app_set_quit();
                exit(1);
        }

        app_parse_args(argc, argv);

        if (app_check_ip() != 0) {
		r_panic("Failed to get the IP address");
                app_set_quit();
                exit(1);
        }
}

static void app_update_logfile()
{
        char buf[512];

        if (_logdir == NULL || _name == NULL)
                return;
        
        // Update the path of the log file
        snprintf(buf, 511, "%s/%s.log", _logdir, _name);
        buf[511] = 0;
        r_log_set_file(buf);
}

void app_set_logdir(const char *logdir)
{
        if (_logdir != NULL) {
                r_free(_logdir);
                _logdir = NULL;
        }
        _logdir = r_strdup(logdir);
        app_update_logfile();
}

const char *app_get_logdir()
{
        return _logdir;
}

void app_set_session(const char *session)
{
        if (_session != NULL) {
                r_free(_session);
                _session = NULL;
        }
        _session = r_strdup(session);

        char path[1024];
        rprintf(path, sizeof(path), "%s/log", session);
        app_set_logdir(path);

        rprintf(path, sizeof(path), "%s/dump", session);
        set_dumping_dir(path);
}

const char *app_get_session()
{
        return _session;
}

static void app_set_config(const char *path)
{
        if (_config != NULL) {
                r_free(_config);
                _config = NULL;
        }

        char buffer[1024];
        int r = path_make_absolute(path, buffer, sizeof(buffer));
        if (r != 0) {
                r_err("app_set_config failed: make_absolute_path != 0");
                app_set_quit();
                return;
        }
        
        _config = r_strdup(buffer);
}

const char *app_get_config()
{
        return _config;
}

void app_set_name(const char *path)
{
        // The call to basename() may change the contents of the
        // buffer. Copy it first.
        char *s = r_strdup(path);
        char *name = basename(s);
        if (_name != NULL)
                r_free(_name);
        _name = r_strdup(name);
        r_free(s);
        r_log_set_app(_name);
        app_update_logfile();
}

const char *app_get_name()
{
        return _name;
}

void app_cleanup()
{
        if (_name != NULL)
                r_free(_name);
        if (_ip != NULL)
                r_free(_ip);
        if (_logdir != NULL)
                r_free(_logdir);
        if (_session != NULL)
                r_free(_session);
        if (_config != NULL)
                r_free(_config);
}

static void diagnostics_print_backtrace()
{
	void *array[10];
	size_t size;
	char **strings;
	size_t i;
	
	size = backtrace(array, 10);
	strings = backtrace_symbols(array, size);
	
	r_panic("***************************");
	r_panic("*** diagnostics ***********");
	r_panic("*** Application crashed *** ");
	r_panic("Backtrace: ");
	for (i = 0; i < size; i++) {
		r_panic(strings[i]);
	}
	
	free(strings);
}

static void diagnostics_signal_handler(int signum, siginfo_t *info, void *unused)
{
        fprintf(stderr, "%s: Received %s signal (signal #%d)\n",
                _name, strsignal(signum), interrupt_count+1);
        r_info("Received %s signal (signal #%d)", strsignal(signum), interrupt_count+1);

	if ((signum == SIGINT) || (signum == SIGHUP) || (signum == SIGTERM)) {
		r_info("quitting");
                diagnostics_set_signal_handler(SIGINT);
                diagnostics_set_signal_handler(SIGHUP);
                diagnostics_set_signal_handler(SIGTERM);
                app_set_quit();
                interrupt_count++;
                if (interrupt_count == 4)
                        exit(1);
	}

	if (signum == SIGSEGV) {
		r_panic("Segment violation");
		diagnostics_print_backtrace();
                app_set_quit();
		exit(1);
	}

	if (signum == SIGFPE) {
		r_panic("Floating-point exception");
		diagnostics_print_backtrace();
	}

	if (signum == SIGPIPE) {
		r_panic("Broken pipe exception");
		diagnostics_print_backtrace();
	}

	if (signum == SIGILL) {
		r_panic("Illegal instruction");
		diagnostics_print_backtrace();
                app_set_quit();
		exit(1);
	}

	if (signum == SIGABRT) {
                longjmp(env, 1);
	}
}

static int diagnostics_set_signal_handler(int signum)
{
        int ret;
        struct sigaction act;
        memset(&act, 0, sizeof(struct sigaction));

        act.sa_flags = SA_SIGINFO;
        act.sa_sigaction = diagnostics_signal_handler;
        ret = sigaction(signum, &act, NULL);
        if (ret != 0) {
                perror("diagnostics_set_signal_handler");
        }
        return ret;
}

static int diagnostics_set_signal_handlers()
{
        int ret = 0;
	ret += diagnostics_set_signal_handler(SIGINT);
	ret += diagnostics_set_signal_handler(SIGHUP);
	ret += diagnostics_set_signal_handler(SIGTERM);
	ret += diagnostics_set_signal_handler(SIGSEGV);
	ret += diagnostics_set_signal_handler(SIGFPE);
	ret += diagnostics_set_signal_handler(SIGILL);
	ret += diagnostics_set_signal_handler(SIGABRT);
	return ret;
}

static void rcom_cleanup()
{
        proxy_cleanup();
        json_cleanup();
        app_cleanup();
        r_log_cleanup();
        r_cleanup();
}
