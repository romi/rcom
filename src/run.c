#define _GNU_SOURCE
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

#include "rcom.h"

#include "run.h"
#include "list.h"
#include "mem.h"
#include "proxy.h"

struct _run_t {
        pid_t pid;
        pid_t gpid;
        char *name;
        char *path;
        char *host;
        char *username;
        list_t *args;
        /* int attach; */
        thread_t *thread;
        run_status_t status;
        //int dump;
        //int replay;
};

run_t *run_parse(json_object_t node)
{
        run_t *r;
        const char *path = json_object_getstr(node, "path");
        const char *host = json_object_getstr(node, "host");
        const char *username = json_object_getstr(node, "username");
        json_object_t args = json_object_get(node, "args");
        /* int attach = json_object_getbool(node, "attach"); */
        /* int replay = json_object_getbool(node, "replay"); */
        /* int dump = json_object_getbool(node, "dump"); */
        
        if (!valid_path(path)) {
                log_err("parse_node: invalid path: %s", path? path : "null");
                return NULL;
        }
        if (!valid_addr(host)) {
                log_err("parse_node: invalid host: %s", host? host : "null");
                return NULL;
        }
        if (!valid_username(username)) {
                log_err("parse_node: invalid username: %s", username? username : "null");
                return NULL;
        }
        if (!json_falsy(args) && !json_isarray(args)) {
                log_err("parse_node: invalid args");
                return NULL;
        }

        if (check_path(path) != 0)
                return NULL;

        /* if (attach == -1) */
        /*         attach = 0; */

        const char *s = strrchr(path, '/');
        if (s == NULL) 
                s = path;
        else if (strlen(s) == 0)
                s = path;
        else s++;

        log_debug("adding new node '%s', binary '%s'", s, path);
        
        r = new_run(s, path, host, username/* , attach */);
        if (r == NULL) return NULL;

        /* r->replay = (replay == 1); */
        /* r->dump = (dump == 1); */
        
        if (json_isarray(args)) {
                int i;
                json_object_t arg;
                for (i = 0; i < json_array_length(args); i++) {
                        arg = json_array_get(args, i);
                        if (!json_isstring(arg)) {
                                log_err("parse_node: invalid arg: not a string (index %d)", i+1);
                                delete_run(r);
                                return NULL;
                        }
                        run_add_arg(r, json_string_value(arg));
                }
        }
        
        return r;
}

run_t *new_run(const char *name,
               const char *path,
               const char *host,
               const char *username/* , */
               /* int attach */)
{
        run_t *r = new_obj(run_t);
        if (r == NULL) return NULL;
        memset(r, 0, sizeof(run_t));
        
        r->name = mem_strdup(name);
        if (r->name == NULL)
                goto error_recovery;
        
        r->path = mem_strdup(path);
        if (r->path == NULL)
                goto error_recovery;
        
        if (host) {
                r->host = mem_strdup(host);
                if (r->host == NULL)
                        goto error_recovery;
        }
        
        if (username) {
                r->username = mem_strdup(username);
                if (r->username == NULL)
                        goto error_recovery;
        }
                
        r->args = NULL;
        r->pid = -1;
        r->status = k_created;
        /* r->attach = attach; */
        
        return r;

error_recovery:
        delete_run(r);
        return NULL;
}

void delete_run(run_t *r)
{
        list_t *l;
        
        if (r) {
                l = r->args;
                while (l) {
                        mem_free(list_get(l, char));
                        l = list_next(l);
                }
                delete_list(r->args);
                if (r->name) mem_free(r->name);
                if (r->host) mem_free(r->host);
                if (r->username) mem_free(r->username);
                if (r->path) mem_free(r->path);
                delete_obj(r);
        }
}

void run_add_arg(run_t *r, const char *s)
{
        char *t = mem_strdup(s);
        r->args = list_append(r->args, t);
}

run_status_t run_status(run_t *r)
{
        return r->status;
}

const char *run_name(run_t *r)
{
        return r->name;
}

/* int run_is_replay(run_t *r) */
/* { */
/*         return r->replay; */
/* } */

static int run_start_locally(run_t *r)
{
        char *argv[32];
	pid_t pid;
        int i, argc, err;
        list_t *a = r->args;
        //char dump_arg[1024];
        
        log_info("Starting '%s' locally", r->name);
        
        pid = fork();
	switch (pid) {
        default:
                r->pid = pid;
                r->status = k_running;
                return 0;;
        case -1:
                r->pid = -1;
                return -1;
        case 0:  break;
	}

        // Set the group id
        err = setpgid(pid, r->gpid);
        if (err != 0) {
                fprintf(stderr, "run_start_locally: Failed to return to the group id\n");
                exit(1);
        }
        
        argc = 0;
        argv[argc++] = r->path;

        argv[argc++] = "-N";
        argv[argc++] = (char*) get_registry_name();

        argv[argc++] = "-A";
        argv[argc++] = (char*) get_registry_ip();

        argv[argc++] = "-C";
        argv[argc++] = (char*) app_get_config();

        if (app_get_logdir() != NULL) {
                argv[argc++] = "-L";
                argv[argc++] = (char*) app_get_logdir();
        }

        /* if (get_dumping() != 0 && r->dump) { */
        /*         snprintf(dump_arg, 1024, "--dump=%s", get_dumping_dir()); */
        /*         argv[argc++] = dump_arg; */
        /* } */
        
        while (a) {
                argv[argc++] = list_get(a, char);
                if (argc >= 31) {
                        fprintf(stderr, "Too many arguments"); // FIXME
                        exit(1);
                }
                a = list_next(a);
        }
        argv[argc] = NULL;

        membuf_t *m = new_membuf();
        for (int i = 0; i < argc; i++) {
                membuf_printf(m, "%s ", argv[i]);
        }
        log_info("Starting %s: %s", r->name, membuf_data(m));
        delete_membuf(m);
        
        // execvp() uses the PATH variable to find the
        // executable. execvp() expects the absolute path. Let's use
        // execv for now.
        //err = execvp(r->path, argv);
        err = execv(r->path, argv);
        perror("monitor: execvp:");
        exit(0);
}

static int run_start_remotely(run_t *r)
{
        char *argv[32];
	pid_t pid;
        int i, argc, err;
        list_t *a = r->args;

        log_info("Starting '%s' remotely", r->name);

        pid = fork();
	switch (pid) {
        default:
                r->pid = pid;
                r->status = k_running;
                return 0;;
        case -1:
                r->pid = -1;
                return -1;
        case 0:  break;
	}

        // Set the group id
        err = setpgid(pid, r->gpid);
        if (err != 0) {
                fprintf(stderr, "run_start_remotely: Failed to return to the group id\n");
                exit(1);
        }

        argc = 0;
        argv[argc++] = "ssh";
        if (r->username) {
                argv[argc++] = "-l";
                argv[argc++] = r->username;
        }
        argv[argc++] = r->host;
        argv[argc++] = r->path;

        argv[argc++] = "-N";
        argv[argc++] = (char*) get_registry_name();

        argv[argc++] = "-A";
        argv[argc++] = (char*) get_registry_ip();

        if (app_get_logdir() != NULL) {
                argv[argc++] = "-L";
                argv[argc++] = (char*) app_get_logdir();
        }

        while (a) {
                argv[argc++] = list_get(a, char);
                if (argc >= 31) {
                        fprintf(stderr, "Too many arguments"); // FIXME
                        exit(1);
                }
                a = list_next(a);
        }
        argv[argc] = NULL;

        membuf_t *m = new_membuf();
        for (int i = 0; i < argc; i++) {
                membuf_printf(m, "%s ", argv[i]);
        }
        log_info("Starting %s: %s", r->name, membuf_data(m));
        delete_membuf(m);
        
        err = execv(r->path, argv);
        perror("monitor: execvp:");
        exit(0);
}

/* static int run_attach(run_t *r) */
/* { */
/*         char cmd[128]; */
/*         char line[128]; */
/*         FILE *pipe; */
/*         pid_t pid; */

/*         snprintf(cmd, 128, "pidof -s %s", r->name); */
/*         cmd[127] = 0; */
        
/*         pipe = popen(cmd, "r"); */
/*         if (pipe == NULL) { */
/*                 char buf[200]; */
/*                 strerror_r(errno, buf, 200); */
/*                 log_err("monitor: attach %s: %s", r->name, buf); */
/*                 return -1; */
/*         } */
        
/*         fgets(line, 128, pipe); */
/*         if (strlen(line) == 0) { */
/*                 log_err("monitor: pidof returned no value for process %s", r->name); */
/*                 return -1; */
/*         } */
        
/*         r->pid = strtoul(line, NULL, 10); */
        
/*         pclose(pipe); */
/*         return 0; */
/* } */

static int _run_start(run_t *r)
{
        log_debug("_run_start %s", run_name(r));
        
        if (r->host)
                return run_start_remotely(r);
        /* else if (r->attach) */
        /*         return run_attach(r); */
        else
                return run_start_locally(r);
}

static void _run(void* userdata)
{
        run_t* r = (run_t*) userdata;
        int err, status;
        pid_t p;

        log_debug("_run %s", run_name(r));
        
        while (!app_quit()) {
                
                err = _run_start(r);

                if (err == 0) {
                        p = waitpid(r->pid, &status, 0);
                        log_info("'%s': waitpid returned", r->name);
                        if (p == -1) {
                                // Should not happen
                                char buf[200];
                                strerror_r(errno, buf, 200);
                                log_panic("panic: '%s': %s", r->name, buf);
                                app_set_quit();
                        }
                } 
                clock_sleep(1);
        }
        
        log_info("'%s' is finished", r->name);
        r->status = k_finished;
}

int run_start(run_t *r, pid_t gpid)
{
        /* if (do_replay() && r->replay) { */
        /*         log_info("Not starting '%s' because it is being replayed.", r->name); */
        /*         return 0; */
        /* } */
        log_debug("run_start %s", run_name(r));
        r->status = k_starting;
        r->gpid = gpid;
        r->thread = new_thread(_run, (void*) r, 0, 0);
        return 0;
}

int run_send_stop(run_t *r)
{
        int err;

        if (r->pid == -1)
                return 0;

        log_info("Sending signal to '%s', pid %d", r->name, r->pid);
        err = kill(r->pid, SIGHUP);
        if (err != 0) {
                char buf[200];
                strerror_r(errno, buf, 200);
                log_err("run_send_stop: '%s': %s", r->name, buf);
                return -1;
        }
        return 0;
}

int run_stop(run_t *r)
{
        int err;
        int sig = SIGHUP;
        int count = 0;
        pid_t p;
        int status;
        siginfo_t info;

        if (r->pid == -1)
                return 0;

        while (1) {
                log_info("Sending signal %d to '%s', pid %d (count=%d)",
                         sig, r->name, r->pid, count);
                err = kill(r->pid, sig);
                if (err != 0) {
                        char buf[200];
                        strerror_r(errno, buf, 200);
                        log_err("run_stop: '%s': %s", r->name, buf);
                        return -1;
                }
                
                log_info("Waiting for '%s' to quit", r->name);
                sleep(2);
                
                if (r->status == k_finished) {
                        log_err("'%s' has quit", r->name);
                        thread_join(r->thread);
                        return 0;
                }

                count++;
                if (count > 3) {
                        log_info("'%s' still alive after three signals. "
                                 "Starting to send SIGKILL.", r->name);
                        sig = SIGKILL;
                }
                if (count > 6) {
                        log_err("monitor: run_stop: '%s': "
                                "too many kill attempts. Giving up.",
                                r->name);
                        return -1;
                }
        }
        return -1;
}



static int valid_info(const char *s,
                      int minlen,
                      int maxlen,
                      const char *valid_chars)
{
        int len, i;
        if (s == NULL)
                return (minlen == 0);
        if (strlen(s) < minlen || strlen(s) > maxlen)
                return 0;
        len = strlen(s);
        for (i = 0; i < len; i++) {
                if (strchr(valid_chars, *(s+i)) == NULL)
                        return 0;
        }
        return 1;
}

int valid_name(const char *name)
{
        static char *valid_chars = ("abcdefghijklmnopqrstuvwxyz"
                                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                    "-_");        
        return valid_info(name, 1, 128, valid_chars);
}

int valid_path(const char *path)
{
        static char *valid_chars = ("abcdefghijklmnopqrstuvwxyz"
                                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                    "1234567890"
                                    "-_/.");
        return valid_info(path, 2, 256, valid_chars);
}

int valid_arg(const char *arg)
{
        static char *valid_chars = ("abcdefghijklmnopqrstuvwxyz"
                                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                    "1234567890"
                                    "-_/.=");
        return valid_info(arg, 1, 256, valid_chars);
}

int valid_addr(const char *host)
{
        static char *valid_chars = ("1234567890.");
        return valid_info(host, 0, 15, valid_chars);
}

int valid_username(const char *username)
{
        static char *valid_chars = ("abcdefghijklmnopqrstuvwxyz"
                                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                    "1234567890"
                                    "-_");
        return valid_info(username, 0, 32, valid_chars);
}

int check_path(const char *path)
{
        struct stat statbuf;
        int err;

        err = stat(path, &statbuf);
        if (err != 0) {
                log_panic("Couldn't find executable: %s", path);
                return -1;
        }
        
        if ((statbuf.st_mode & S_IFMT) == S_IFLNK) {
                char newpath[1024];
                readlink(path, newpath, 1024);
                if (rstreq(newpath, path)) {
                        log_panic("Symbolic link points to itself?!:  %s", path);
                        return -1;
                }
                return check_path(newpath);
        }

        if ((statbuf.st_mode & S_IFMT) != S_IFREG) {
                log_panic("Not a file: %s", path);
                return -1;
        }

        if ((statbuf.st_mode & S_IXUSR) == 0) {
                log_panic("File is not executable: %s", path);
                return -1;
        }

        return 0;
}
