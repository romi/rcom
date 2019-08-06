#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

#include "run.h"
#include "util_priv.h"
#include "proxy.h"
#include "list.h"

static run_t *_registry = NULL;
//static run_t *_replay = NULL;
//static const char *_dump_dir = NULL;
//static const char *_replay_path = NULL;

/******************************************************/

static int start_registry(pid_t gpid)
{
        int ret;
        int i;

        ret = run_start(_registry, gpid);
        if (ret == -1) {
                log_err("start_registry: run_start_locally returned -1");
                return -1;
        }

        log_debug("child process: registry started");
        
        // Wait a bit before contacting the registry
        clock_sleep(2);

        log_debug("child process: contacting registry");
        
        // Connect to the registry to check it's alive
        int max_tries = 3;
        for (int i = 0; i < max_tries; i++) {
                log_info("Trying to connect to the registry (attempt %d/%d)",
                         i+1, max_tries);
                proxy_t *proxy = proxy_get();
                if (proxy != NULL) {
                        log_info("Connection to the registry succeeded. "
                                 "Starting the nodes.");
                        return 0; // Success
                }
                log_err("Connection to the registry failed");
                clock_sleep(2);
        }

        // If we can't reach the registry, stop it.
        log_err("All attempts to connect to the registry failed. "
                "Stopping it and quitting.");
        if (run_status(_registry) == k_running
            || run_status(_registry) == k_starting) {
                ret = run_stop(_registry);
                if (ret == -1) {
                        log_panic("Failed to stop the registry");
                        return -1;
                }
        }
        
        log_panic("Failed to start the registry");
        return -1;
}

/******************************************************/

/* static int include_dump_file(list_t *nodes, const char *filename) */
/* { */
/*         while (nodes) { */
/*                 run_t *r = list_get(nodes, run_t); */
/*                 int len = strlen(run_name(r)); */
/*                 if ((strncmp(run_name(r), filename, len) == 0) && run_is_replay(r)) { */
/*                         return 1; */
/*                 } */
/*                 nodes = list_next(nodes); */
/*         } */
/*         return 0;         */
/* } */

/* static int start_replay(list_t *list, pid_t gpid) */
/* { */
/*         int ret; */
/*         char path[1024]; */
/*         DIR *d; */
/*         struct dirent *dir; */
/*         int count = 0; */
        
/*         log_info("Dump base dir '%s'", _dump_dir); */
                
/*         if (_dump_dir == NULL) { */
/*                 log_err("You must specify the dump directory in " */
/*                         "the config file when you request a replay"); */
/*                 return -1; */
/*         } */

/*         _replay = new_run("replay", _replay_path, NULL, NULL, 0, 0); */
/*         if (_replay == NULL) */
/*                 return -1; */

/*         run_add_arg(_replay, "--"); */

/*         snprintf(path, 1024, "%s/%s", _dump_dir, get_replay_id()); */
/*         path[1023] = 0; */

/*         log_info("Dump dir '%s'", path); */

/*         d = opendir(path); */
/*         if (d == NULL) { */
/*                 log_err("Couldn't open the dump directory '%s'", path); */
/*                 return -1; */
/*         } */

/*         while ((dir = readdir(d)) != NULL) { */
/*                 if (include_dump_file(list, dir->d_name)) { */
/*                         char filepath[1024]; */
/*                         snprintf(filepath, 1024, "%s/%s", path, dir->d_name); */
/*                         filepath[1023] = 0; */
/*                         log_info("Including dump '%s'", filepath); */
/*                         run_add_arg(_replay, filepath); */
/*                         count++; */
/*                 } else { */
/*                         log_info("Skipping dump file '%s' because it doesn't match a replayable node.", */
/*                                  dir->d_name); */
/*                 } */
/*         } */
/*         closedir(d); */

/*         if (count == 0) { */
/*                 log_warn("Replay: No appropriate dump files found."); */
/*                 return 0; */
/*         } */
        
/*         ret = run_start(_replay, gpid); */
/*         if (ret == -1) { */
/*                 log_err("start_replay: run_start_locally returned -1"); */
/*                 return -1; */
/*         } */

/*         return 0; */
/* } */

/******************************************************/

static list_t *delete_nodes(list_t *n)
{
        list_t *p = n;
        while (n) {
                run_t *r = list_get(n, run_t);
                delete_run(r);
                n = list_next(n);
        }
        delete_list(p);
        return NULL;
}

static run_t *nodes_find(list_t *n, const char *name)
{
        while (n) {
                run_t *r = list_get(n, run_t);
                if (rstreq(run_name(r), name))
                    return r;
                n = list_next(n);
        }
        return NULL;
}

/* static int _start_dependencies(run_t *r, pid_t gpid, list_t *nodes) */
/* { */
/*         for (int i = 0; i < run_count_dependencies(r); i++) { */
/*                 const char *name = run_get_dependency(r, i); */
/*                 run_t *q = nodes_find(nodes, name); */
/*                 if (q == NULL) { */
/*                         log_err("Couldn't find node '%s', dependency of '%s'", */
/*                                 name, run_name(r)); */
/*                         return -1; */
/*                 } */
/*                 log_info("'%s': dependency '%s': status=%d", name, run_name(r), run_status(q)); */
/*                 if (run_status(q) == k_created) { */
/*                         int err = run_start(q, gpid); */
/*                         if (err) return err; */
/*                 } */
/*         } */
/*         return 0; */
/* } */

static int nodes_start(list_t *nodes, pid_t gpid)
{
        list_t *n = nodes;
        int err;

        log_debug("nodes_start n=%p", nodes);
        
        while (n) {
                run_t *r = list_get(n, run_t);
                if (run_status(r) == k_created) {
                        /* err = _start_dependencies(r, gpid, nodes); */
                        /* if (err) return err; */
                        log_debug("starting %s", run_name(r));
                        err = run_start(r, gpid);
                        clock_sleep(0.2);
                        if (err) return err;
                }
                n = list_next(n);
        }
        return 0;
}

static void nodes_stop(list_t *nodes)
{
        list_t *n = nodes;
        // Start a stop signal to all
        log_info("Sending HUP signal to all nodes");
        while (n) {
                run_t *r = list_get(n, run_t);
                run_send_stop(r);
                n = list_next(n);
        }
        // Go over the nodes one by one, and make sure they quit.
        n = nodes;
        log_info("Stopping nodes one by one");
        while (n) {
                run_t *r = list_get(n, run_t);
                run_stop(r);
                n = list_next(n);
        }
}

/******************************************************/

static list_t *parse_nodes(json_object_t config)
{
        int i;
        json_object_t nodes;
        json_object_t node;
        run_t *r;
        list_t *list = NULL;

        log_debug("parsing nodes");

        nodes = json_object_get(config, "nodes");
        if (json_isnull(nodes)) {
                log_err("Didn't find any nodes");
                return NULL;
        }
        
        if (!json_isarray(nodes)) {
                log_err("Invalid nodes field");
                return NULL;
        }

        for (i = 0; i < json_array_length(nodes); i++) {
                node = json_array_get(nodes, i);
                r = run_parse(node);
                if (r == NULL)
                        return delete_nodes(list);
                list = list_append(list, r);
        }

        log_debug("parsing nodes: done");

        return list;
}

static int parse_general(json_object_t config)
{
        json_object_t r;
        
        r = json_object_get(config, "general");
        if (json_falsy(r)) {
                log_debug("did not find a general section. That's ok.");
                return 0;
        }
        
        const char *log = json_object_getstr(r, "log-dir");
        if (log != NULL) {
                // app_set_logdir also update the log file.
                app_set_logdir(log);
        }

        /* _dump_dir = json_object_getstr(r, "dump-dir"); */
        /* if (_dump_dir != NULL && get_dumping()) { */
        /*         char path[1024]; */
        /*         char timestamp[128]; */
        /*         int err; */
                
        /*         clock_datetime(timestamp, 128, '-', '_', '-'); */
        /*         snprintf(path, 1024, "%s/%s", _dump_dir, timestamp); */
        /*         path[1023] = 0; */
                
        /*         err = mkdir(path, 0777); */
        /*         if (err != 0) { */
        /*                 char reason[200]; */
        /*                 strerror_r(errno, reason, 200); */
        /*                 log_err("Failed to create the directory '%s'", path); */
        /*                 return -1; */
        /*         } */
        /*         set_dumping_dir(path); */
        /* } */

        /* _replay_path = json_object_getstr(r, "replay-path"); */
        /* if (_replay_path == NULL) { */
        /*         _replay_path = "/usr/local/bin/ltreplay"; */
        /* } */
        
        log_debug("parse_general done");
        
        return 0;
}

static int parse_registry(json_object_t config)
{
        json_object_t r;
        const char *name;
        const char *addr;
        const char *path;
        int port;
        
        r = json_object_get(config, "registry");
        
        if (!json_falsy(r)) {
        
                addr = json_object_getstr(r, "addr");
                if (addr != NULL) {
                        if (valid_addr(addr))
                                set_registry_ip(addr);
                        else
                                log_err("monitor: parse_registry: bad addr: '%s'", addr);
                }
        
                port = (int) json_object_getnum(r, "port");
                if (port > 0 && port < 65536) {
                        set_registry_port(port);
                }
        
                path = json_object_getstr(r, "path");
        }

        if (path == NULL) {
                path = "/usr/local/bin/rcregistry";
        }

        if (check_path(path) != 0) {
                log_panic("monitor: can't find ltregistry executable. Tried %s.", path);
                return -1;
        }
        
        _registry = new_run("registry", path, NULL, NULL/* , 0 */);
        if (_registry == NULL)
                return -1;
        
        return 0;
}

/******************************************************/

static void child_main(list_t *list, pid_t monitor_gpid)
{
        int ret;
        pid_t gpid;
        
        gpid = getpgrp();

        log_debug("child process: starting registry");

        ret = start_registry(gpid);
        if (ret != 0) {
                log_info("Exiting!");
                exit(1);
        }

        /* if (do_replay()) { */
                
        /*         log_info("Do replay? yes"); */

        /*         ret = start_replay(list, gpid); */
        /*         if (ret != 0) { */
        /*                 log_info("Exiting!"); */
        /*                 exit(1); */
        /*         } */
        /* } */

        log_debug("child process: starting nodes");

        ret = nodes_start(list, gpid);
        if (ret != 0) {
                nodes_stop(list);
                log_info("Stopping registry node");
                run_stop(_registry);
                log_info("Exiting!");
                exit(1);
        }

        // Return to the original group id
        ret = setpgid(0, monitor_gpid);
        if (ret == -1) {
                log_err("Failed to return to the group id");
                nodes_stop(list);
                log_info("Stopping registry node");
                run_stop(_registry);
                log_info("Exiting!");
                exit(1);
        }

        while (!app_quit()) {
                clock_sleep(1);
        }
        
        nodes_stop(list);
        log_info("Stopping replay node");
        //if (_replay) run_stop(_replay);
        log_info("Stopping registry node");
        run_stop(_registry);
        delete_nodes(list);
        delete_run(_registry);
        log_info("Exiting!");
        
        exit(0);
}

// Start the nodes. This forks a child process. The child process than
// forks for each of the nodes. The intermediate child process is
// needed in order to put all of the nodes in the same process
// group. By putting them in the same group, different from the
// monitor process, only the monitor catches the signals (ctrl-c, HUP,
// ...). It can then shut down the individual nodes orderly.
//
// Returns the PID of the child process, or -1 in case of error.
static pid_t start_child(list_t *list)
{
        int ret;
	pid_t pid;
        pid_t nodes_gpid;
        pid_t monitor_gpid;

        // Fork so that the current process is no longer the group
        // leader.
        pid = fork();
	switch (pid) {
        default: return pid;
        case -1: return -1;
        case 0:  break;
	}

        app_set_name("rclaunch-child");
        log_info("PID %d", getpid());

        // Store a copy of the parents group id
        monitor_gpid = getpgrp();
        
        // Create a new group id
        ret = setpgid(pid, 0);
        if (ret == -1) {
                log_panic("Failed to set the group id");
                perror("monitor: start_children");
                return -1;
        }
        
        nodes_gpid = getpgrp();
        log_info("nodes group id: %d, parent group id: %d", nodes_gpid, monitor_gpid);

        child_main(list, monitor_gpid);
        
        exit(0);
}

/******************************************************/

list_t *load_config(const char *filename)
{
        json_object_t config;
        json_object_t launch;
        list_t *list = NULL;
        char errmsg[200];
        int ret;

        log_debug("loading config");

        config = json_load(filename, &ret, errmsg, 200);
        if (ret != 0) {
                log_panic("Failed to load the config file '%s': %s", filename, errmsg);
                return NULL;
        }

        log_debug("getting 'launch' section");

        launch = json_object_get(config, "launch");
        if (json_falsy(launch)) {
                log_panic("No launch section: exiting");
                json_unref(config);
                json_unref(launch);
                return NULL;
        }

        log_debug("parsing 'launch.general' section");

        if (parse_general(launch) != 0) {
                json_unref(config);
                json_unref(launch);
                return NULL;
        }

        log_debug("parsing 'launch.registry' section");
        
        if (parse_registry(launch) != 0) {
                json_unref(config);
                json_unref(launch);
                return NULL;
        }
        
        log_debug("parsing 'launch.nodes' section");
        
        list = parse_nodes(launch);
        if (list == NULL) {
                json_unref(config);
                json_unref(launch);
                return NULL;
        }

        log_debug("parsing done");
        
        json_unref(launch);
        json_unref(config);
        return list;
}

int main(int argc, char **argv)
{
        list_t *list = NULL;
        pid_t child_pid =  -1;
        pid_t monitor_pid;
        pid_t pid;
        int status;
        siginfo_t info;
        int ret;
               
        app_init(&argc, argv);

        if (app_get_config() == NULL) {
                fprintf(stderr, "Usage: rclaunch -C config-file\n");
                exit(1);
        }

        list = load_config(app_get_config());
        if (list == NULL)
                exit(1);
        
        monitor_pid = getpid();
        log_info("PID %d", monitor_pid);
        
        // Start the nodes.  
        child_pid = start_child(list);
        if (child_pid == -1) {
                log_panic("Failed to start the nodes");
                exit(1);
        }

        while (!app_quit()) {
                
                clock_sleep(1);

                // Check if the child process quit. If so, quit also.
                info.si_pid = 0;
                ret = waitid(P_PID, child_pid, &info, WEXITED | WNOHANG | WNOWAIT);
                if (ret == 0 && info.si_pid == child_pid)
                        app_set_quit();
        }
        
        log_info("Sending HUP signal to child process, pid %d", child_pid);
        kill(child_pid, SIGHUP);
        log_info("Waiting for ltmonitor-child to exit");
        ret = waitpid(child_pid, &status, 0);
        log_info("Exiting!");
        exit(0);
}
