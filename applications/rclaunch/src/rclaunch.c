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
/*

  1) The main rclaunch process starts a child process and then waits
  for the child process to quit.

  2) The child process creates a new process group.

  3) The child process creates a new thread for each requested
  application in the new process group.

  4) Each thread subsequently forks to execute the application.

  5) Each thread waits for its application to quit. It restarts
  its application again if it exits prematurely.

  6) After starting the threads, the child process returns to the
  original process group and waits for signals.

  7) When the child process receives a signal it forwards the signal
  to the applications.

  8) The application does a clean exit upon receiving a HUP, INT, or
  TERM signal. If the application doesn't exit after having received
  four termination signals, it does a hard Johnson brexit.

 */


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <r.h>

#include "run.h"
#include "proxy.h"

static run_t *_registry = NULL;
static const char *_user = NULL;
//static run_t *_replay = NULL;
//static const char *_dump_dir = NULL;
//static const char *_replay_path = NULL;

uid_t get_uid(const char *user);

/******************************************************/

static int start_registry(pid_t gpid)
{
        int ret;
        ret = run_start(_registry, gpid);
        if (ret == -1) {
                r_err("start_registry: run_start_locally returned -1");
                return -1;
        }

        r_debug("child process: registry started");

        // Wait a bit before contacting the registry
        clock_sleep(2);

        r_debug("child process: contacting registry");

        // Connect to the registry to check it's alive
        int max_tries = 3;
        for (int i = 0; i < max_tries; i++) {
                r_info("Trying to connect to the registry (attempt %d/%d)",
                         i+1, max_tries);
                proxy_t *proxy = proxy_get();
                if (proxy != NULL) {
                        r_info("Connection to the registry succeeded. "
                                 "Starting the nodes.");
                        return 0; // Success
                }
                r_err("Connection to the registry failed");
                clock_sleep(2);
        }

        // If we can't reach the registry, stop it.
        r_err("All attempts to connect to the registry failed. "
                "Stopping it and quitting.");
        if (run_status(_registry) == k_running
            || run_status(_registry) == k_starting) {
                ret = run_stop(_registry);
                if (ret == -1) {
                        r_panic("Failed to stop the registry");
                        return -1;
                }
        }

        r_panic("Failed to start the registry");
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

/*         r_info("Dump base dir '%s'", _dump_dir); */

/*         if (_dump_dir == NULL) { */
/*                 r_err("You must specify the dump directory in " */
/*                         "the config file when you request a replay"); */
/*                 return -1; */
/*         } */

/*         _replay = new_run("replay", _replay_path, NULL, NULL, 0, 0); */
/*         if (_replay == NULL) */
/*                 return -1; */

/*         run_add_arg(_replay, "--"); */

/*         snprintf(path, 1024, "%s/%s", _dump_dir, get_replay_id()); */
/*         path[1023] = 0; */

/*         r_info("Dump dir '%s'", path); */

/*         d = opendir(path); */
/*         if (d == NULL) { */
/*                 r_err("Couldn't open the dump directory '%s'", path); */
/*                 return -1; */
/*         } */

/*         while ((dir = readdir(d)) != NULL) { */
/*                 if (include_dump_file(list, dir->d_name)) { */
/*                         char filepath[1024]; */
/*                         snprintf(filepath, 1024, "%s/%s", path, dir->d_name); */
/*                         filepath[1023] = 0; */
/*                         r_info("Including dump '%s'", filepath); */
/*                         run_add_arg(_replay, filepath); */
/*                         count++; */
/*                 } else { */
/*                         r_info("Skipping dump file '%s' because it doesn't match a replayable node.", */
/*                                  dir->d_name); */
/*                 } */
/*         } */
/*         closedir(d); */

/*         if (count == 0) { */
/*                 r_warn("Replay: No appropriate dump files found."); */
/*                 return 0; */
/*         } */

/*         ret = run_start(_replay, gpid); */
/*         if (ret == -1) { */
/*                 r_err("start_replay: run_start_locally returned -1"); */
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

__attribute__((unused))
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

static int nodes_start(list_t *nodes, pid_t gpid)
{
        list_t *n = nodes;
        int err;

        while (n) {
                run_t *r = list_get(n, run_t);
                if (run_status(r) == k_created) {
                        r_debug("starting %s", run_name(r));
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
        r_info("Sending HUP signal to all nodes");
        while (n) {
                run_t *r = list_get(n, run_t);
                run_send_stop(r);
                n = list_next(n);
        }
        // Go over the nodes one by one, and make sure they quit.
        n = nodes;
        r_info("Stopping nodes one by one");
        while (n) {
                run_t *r = list_get(n, run_t);
                run_stop(r);
                n = list_next(n);
        }
}

/******************************************************/

run_t *run_parse(json_object_t node, const char *default_user)
{
        run_t *r;
        const char *path = json_object_getstr(node, "path");
        const char *host = json_object_getstr(node, "host");
        const char *user = json_object_getstr(node, "user");
        json_object_t args = json_object_get(node, "args");
        int disabled = json_object_getbool(node, "disabled");
        /* int replay = json_object_getbool(node, "replay"); */
        /* int dump = json_object_getbool(node, "dump"); */

        if (!valid_path(path)) {
                r_err("parse_node: invalid path: %s", path? path : "null");
                return NULL;
        }
        if (!valid_addr(host)) {
                r_err("parse_node: invalid host: %s", host? host : "null");
                return NULL;
        }
        if (!valid_user(user)) {
                r_err("parse_node: invalid user: %s", user? user : "null");
                return NULL;
        }
        if (!json_falsy(args) && !json_isarray(args)) {
                r_err("parse_node: invalid args");
                return NULL;
        }

        const char *s = strrchr(path, '/');
        if (s == NULL)
                s = path;
        else if (strlen(s) == 0)
                s = path;
        else
                s++;

        r_debug("adding new node '%s', executable '%s'", s, path);

        if (user == NULL)
                user = default_user;

        r = new_run(s, path, host, user);
        if (r == NULL)
                return NULL;

        if (disabled == 1)
                run_set_disabled(r, 1);

        /* r->replay = (replay == 1); */
        /* r->dump = (dump == 1); */

        if (json_isarray(args)) {
                int i;
                json_object_t arg;
                for (i = 0; i < json_array_length(args); i++) {
                        arg = json_array_get(args, i);
                        if (!json_isstring(arg)) {
                                r_err("parse_node: invalid arg: not a string (index %d)", i+1);
                                delete_run(r);
                                return NULL;
                        }
                        run_add_arg(r, json_string_value(arg));
                }
        }

        return r;
}

static list_t *parse_nodes(json_object_t config, const char *default_user)
{
        int i;
        json_object_t nodes;
        json_object_t node;
        run_t *r;
        list_t *list = NULL;

        nodes = json_object_get(config, "nodes");
        if (json_isnull(nodes)) {
                r_err("Didn't find any nodes");
                return NULL;
        }

        if (!json_isarray(nodes)) {
                r_err("Invalid nodes field");
                return NULL;
        }

        for (i = 0; i < json_array_length(nodes); i++) {
                node = json_array_get(nodes, i);
                r = run_parse(node, default_user);
                if (r == NULL) {
                        delete_nodes(list);
                        return NULL;
                }
                list = list_append(list, r);
        }

        return list;
}

static int parse_general(json_object_t config)
{
        json_object_t r;

        r = json_object_get(config, "general");
        if (json_falsy(r)) {
                r_debug("did not find a general section. That's ok.");
                return 0;
        }

        const char *user = json_object_getstr(r, "user");
        if (user != NULL) {
                _user = r_strdup(user);
                r_info("Starting nodes under user '%s'", _user);
        }

        const char *sessions_dir = json_object_getstr(r, "sessions-dir");
        if (sessions_dir != NULL) {
                //r_debug("parse_general: sessions_dir %s", sessions_dir);

                char datetime[128];
                clock_datetime(datetime, sizeof(datetime), '-', '_', '-');

                char path[1024];

                //r_debug("parse_general: create session directory");
                rprintf(path, sizeof(path), "%s/%s", sessions_dir, datetime);
                directory_create(path);
                if (user)
                        path_chown(path, user);

                //r_debug("parse_general: create session log directory");
                rprintf(path, sizeof(path), "%s/%s/log", sessions_dir, datetime);
                directory_create(path);
                if (user)
                        path_chown(path, user);

                //r_debug("parse_general: create session dump directory");
                rprintf(path, sizeof(path), "%s/%s/dump", sessions_dir, datetime);
                directory_create(path);
                if (user)
                        path_chown(path, user);

                //r_debug("parse_general: create session db directory");
                rprintf(path, sizeof(path), "%s/%s/db", sessions_dir, datetime);
                directory_create(path);
                if (user)
                        path_chown(path, user);

                rprintf(path, sizeof(path), "%s/%s", sessions_dir, datetime);
                app_set_session(path);
        }

        /* _replay_path = json_object_getstr(r, "replay-path"); */
        /* if (_replay_path == NULL) { */
        /*         _replay_path = "/usr/local/bin/ltreplay"; */
        /* } */

        return 0;
}

static int parse_registry(json_object_t config)
{
        json_object_t r;
        const char *addr;
        const char *path = NULL;
        int port = 10101;

        r = json_object_get(config, "registry");

        if (!json_falsy(r)) {

                addr = json_object_getstr(r, "addr");
                if (addr != NULL) {
                        if (valid_addr(addr))
                                set_registry_ip(addr);
                        else
                                r_err("monitor: parse_registry: bad addr: '%s'", addr);
                }

                if (json_object_has(r, "port"))
                {
                    port = (int) json_object_getnum(r, "port");
                }
                if (port > 0 && port < 65536) {
                        set_registry_port(port);
                }

                path = json_object_getstr(r, "path");
        }

        if (path == NULL) 
                path = "rcregistry";

        _registry = new_run("registry", path, NULL, _user);
        if (_registry == NULL)
                return -1;

        return 0;
}

/******************************************************/

list_t *load_config(const char *filename)
{
        json_object_t config;
        json_object_t launch;
        list_t *list = NULL;
        char errmsg[200];
        int ret;

        r_debug("loading config");

        config = json_load(filename, &ret, errmsg, 200);
        if (ret != 0) {
                r_panic("Failed to load the config file '%s': %s", filename, errmsg);
                return NULL;
        }

        r_debug("getting 'launch' section");

        launch = json_object_get(config, "launch");
        if (json_falsy(launch)) {
                r_panic("No launch section: exiting");
                json_unref(config);
                return NULL;
        }

        if (parse_general(launch) != 0) {
                json_unref(config);
                return NULL;
        }

        if (parse_registry(launch) != 0) {
                json_unref(config);
                return NULL;
        }

        // FIXME: don't pass 'user' from parse_general to here through
        // global variable!
        list = parse_nodes(launch, _user);
        if (list == NULL) {
                json_unref(config);
                return NULL;
        }

        r_debug("parsing done");

        json_unref(config);
        return list;
}

static void child_main(pid_t monitor_gpid)
{
        int ret;
        pid_t gpid;
        list_t *list = NULL;

        app_set_name("rclaunch-child");

        r_debug("child process: loading configuration");

        list = load_config(app_get_config());
        if (list == NULL)
                exit(1);

        gpid = getpgrp();

        r_debug("child process: starting registry");

        ret = start_registry(gpid);
        if (ret != 0) {
                r_info("Exiting!");
                exit(1);
        }

        /* if (do_replay()) { */

        /*         r_info("Do replay? yes"); */

        /*         ret = start_replay(list, gpid); */
        /*         if (ret != 0) { */
        /*                 r_info("Exiting!"); */
        /*                 exit(1); */
        /*         } */
        /* } */

        r_debug("child process: starting nodes");

        ret = nodes_start(list, gpid);
        if (ret != 0) {
                nodes_stop(list);
                r_info("Stopping registry node");
                run_stop(_registry);
                r_info("Exiting!");
                exit(1);
        }

        // Return to the original group id
        ret = setpgid(0, monitor_gpid);
        if (ret == -1) {
                r_err("Failed to return to the group id");
                nodes_stop(list);
                r_info("Stopping registry node");
                run_stop(_registry);
                r_info("Exiting!");
                exit(1);
        }

        while (!app_quit()) {
                clock_sleep(1);
        }

        nodes_stop(list);
        r_info("Stopping replay node");
        //if (_replay) run_stop(_replay);
        r_info("Stopping registry node");
        run_stop(_registry);
        delete_nodes(list);
        delete_run(_registry);
        r_info("Exiting!");

        exit(0);
}

// This forks a child process. The child process than forks for each
// of the nodes. The intermediate child process is needed in order to
// put all of the nodes in the same process group. By putting them in
// the same group, different from the monitor process, only the
// monitor catches the signals (ctrl-c, HUP, ...). It can then shut
// down the individual nodes correctly.
//
// Returns the PID of the child process, or -1 in case of error.
static pid_t start_child()
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

        // Store a copy of the parents group id
        monitor_gpid = getpgrp();

        // Create a new group id
        ret = setpgid(pid, 0);
        if (ret == -1) {
                r_panic("Failed to set the group id");
                perror("monitor: start_children");
                return -1;
        }

        nodes_gpid = getpgrp();
        r_info("nodes group id: %d, parent group id: %d", nodes_gpid, monitor_gpid);

        child_main(monitor_gpid);

        exit(0);
}

/******************************************************/

int main(int argc, char **argv)
{
        pid_t child_pid =  -1;
        pid_t monitor_pid;
        int status;
        siginfo_t info;
        int ret;

        app_init(&argc, argv);

        if (app_get_config() == NULL) {
                fprintf(stderr, "Usage: rclaunch -C config-file\n");
                exit(1);
        }

        monitor_pid = getpid();
        r_info("PID %d", monitor_pid);

        // Start the nodes.
        child_pid = start_child();
        if (child_pid == -1) {
                r_panic("Failed to start the nodes");
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

        r_info("Waiting for ltmonitor-child to exit");
        ret = waitpid(child_pid, &status, 0);
        r_info("Exiting!");

        exit(0);
}
