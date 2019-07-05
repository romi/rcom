
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

#if MEM_DIAGNOSTICS
#include <sgc.h>
#endif

#include "rcom/log.h"
#include "rcom/app.h"
#include "rcom/dump.h"

#include "util.h"
#include "mem.h"
#include "registry_priv.h"
#include "proxy.h"

static jmp_buf env;
static int interrupt_count = 0;

static void app_update_logfile();
static int diagnostics_set_signal_handlers(); 
static int diagnostics_set_signal_handler(int signum); 
static void diagnostics_scan_memory();
static void diagnostics_cleanup();

static int _quit = 0;
static char *_ip = NULL;
static char *_logdir = NULL;
static char *_name = NULL;
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

        static char *optchars = "A:N:P:L:I:R:D::pa";
        static struct option long_options[] = {
                {"registry-name", required_argument, 0, 'N'},
                {"registry-addr", required_argument, 0, 'A'},
                {"registry-port", required_argument, 0, 'P'},
                {"ip", required_argument, 0, 'I'},
                {"log-dir", required_argument, 0, 'L'},
                {"dump", optional_argument, 0, 'D'},
                {"replay", required_argument, 0, 'R'},
                {"print", no_argument, 0, 'p'},
                {"stand-alone", no_argument, 0, 'a'},
                {0, 0, 0, 0}
        };

        while (1) {
                c = getopt_long(*argc, argv, optchars, long_options, &option_index);
                if (c == -1)
                        break;

                switch (c) {
                case 'N':
                        log_info("app_parse_args: setting registry name to '%s'",
                                 optarg);
                        set_registry_name(optarg);
                        break;
                case 'A':
                        log_info("app_parse_args: setting registry address to '%s'",
                                 optarg);
                        set_registry_ip(optarg);
                        break;
                case 'P':
                        log_info("app_parse_args: setting registry port to %d",
                                 atoi(optarg));
                        set_registry_port(atoi(optarg));
                        break;
                case 'I':
                        log_info("app_parse_args: setting default IP address to %s",
                                 optarg);
                        _ip = mem_strdup(optarg);
                        break;
                case 'L':
                        app_set_logdir(optarg);
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
                _ip = mem_strdup("0.0.0.0");
        return 0;
}

static int app_check_ip_OLD()
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
                        if (!streq(host, "127.0.0.1")) {
                                log_warn("app_check_ip: Using '%s' by default",
                                         host);
                                log_warn("app_check_ip: Consider using "
                                          "the --ip=... option");
                                _ip = mem_strdup(host);
                                break;
                        }
                }
        }

        if (_ip == NULL) {
                _ip = mem_strdup("127.0.0.1");
        }
        
        freeifaddrs(ifaddr);
        return 0;
}

const char *app_ip()
{
        return _ip;
}

void app_init(int *argc, char **argv)
{
        int ret;

        //_logdir = mem_strdup(".");
        app_set_name(argv[0]);

        proxy_init();
        
        ret = diagnostics_set_signal_handlers();
        atexit(diagnostics_cleanup);
        
        if (setjmp(env) != 0) {
		log_panic("Abort (longjmp)");
                app_set_quit();
                exit(1);
        }

        app_parse_args(argc, argv);

        if (app_check_ip() != 0) {
		log_panic("Failed to get the IP address");
                app_set_quit();
                exit(1);
        }

#if MEM_DIAGNOSTICS
	if (sgc_init(argc, 0) != 0) {
                log_err("Failed the initialise the SGC memory heap");
                exit(1);
        }
#endif
}

static void app_update_logfile()
{
        char buf[512];

        if (_logdir == NULL || _name == NULL)
                return;
        
        // Update the path of the log file
        snprintf(buf, 511, "%s/%s.log", _logdir, _name);
        buf[511] = 0;
        log_set_file(buf);
}

void app_set_logdir(const char *logdir)
{
        if (_logdir != NULL) {
                mem_free(_logdir);
                _logdir = NULL;
        }
        _logdir = mem_strdup(logdir);
        app_update_logfile();
}

const char *app_get_logdir()
{
        return _logdir;
}

void app_set_name(const char *path)
{
        // The call to basename() may change the contents of the
        // buffer. Copy it first.
        char *s = mem_strdup(path);
        char *name = basename(s);
        if (_name != NULL)
                mem_free(_name);
        _name = mem_strdup(name);
        mem_free(s);
        log_set_app(_name);
        app_update_logfile();
}

const char *app_get_name()
{
        return _name;
}

static void diagnostics_print_backtrace()
{
	void *array[10];
	size_t size;
	char **strings;
	size_t i;
	
	size = backtrace(array, 10);
	strings = backtrace_symbols(array, size);
	
	log_panic("***************************");
	log_panic("*** diagnostics ***********");
	log_panic("*** Application crashed *** ");
	log_panic("Backtrace: ");
	for (i = 0; i < size; i++) {
		log_panic(strings[i]);
	}
	
	free(strings);
}

static void diagnostics_signal_handler(int signum, siginfo_t *info, void *unused)
{
        log_info("Received %s signal", strsignal(signum));
        
	if ((signum == SIGINT) || (signum == SIGHUP) || (signum == SIGTERM)) {
		log_info("quitting");
                diagnostics_set_signal_handler(SIGINT);
                diagnostics_set_signal_handler(SIGHUP);
                diagnostics_set_signal_handler(SIGTERM);
                app_set_quit();
                interrupt_count++;
                if (interrupt_count == 4)
                        exit(1);
	}

	if (signum == SIGSEGV) {
		log_panic("Segment violation");
		diagnostics_print_backtrace();
                app_set_quit();
		exit(1);
	}

	if (signum == SIGFPE) {
		log_panic("Floating-point exception");
		diagnostics_print_backtrace();
	}

	if (signum == SIGPIPE) {
		log_panic("Broken pipe exception");
		diagnostics_print_backtrace();
	}

	if (signum == SIGILL) {
		log_panic("Illegal instruction");
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

#if MEM_DIAGNOSTICS
static int diagnostics_print_memory_leak(int op, void* ptr, unsigned char type,
                                         int counter, int size)
{
        if (op == 3) {
                printf("Memory leak: counter=%d, size=%d\n", counter, size);
        } 
        return 1;
}
#endif

static void diagnostics_scan_memory()
{
#if MEM_DIAGNOSTICS
        log_info("Scanning for memory leaks");
        sgc_search_memory_leaks(diagnostics_print_memory_leak);
        sgc_cleanup();
#endif
}

static void diagnostics_cleanup()
{
        proxy_cleanup();
        json_cleanup();
        diagnostics_scan_memory();
}
