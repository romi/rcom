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
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

#include <r.h>
#include "rcom.h"

#include "run.h"
#include "proxy.h"

static int get_user_info(const char *user, uid_t *uid, gid_t *gid);

struct _run_t {
        pid_t pid;
        pid_t gpid;
        char *name;
        char *path;
        char *host;
        char *user;
        uid_t uid;
        gid_t gid;
        gid_t* gids;
        int num_gids;
        list_t *args;
        thread_t *thread;
        run_status_t status;
        int disabled;
        //int dump;
        //int replay;
};

run_t *new_run(const char *name,
               const char *path,
               const char *host,
               const char *user)
{
        run_t *r = r_new(run_t);
        if (r == NULL) return NULL;
        
        r->name = r_strdup(name);
        if (r->name == NULL)
                goto error_recovery;
        
        r->path = r_strdup(path);
        if (r->path == NULL)
                goto error_recovery;
        
        if (host) {
                r->host = r_strdup(host);
                if (r->host == NULL)
                        goto error_recovery;
        }
        
        r->uid = -1;
        if (user) {
                r->user = r_strdup(user);
                if (r->user == NULL)
                        goto error_recovery;
                if (get_user_info(r->user, &r->uid, &r->gid) != 0)
                        goto error_recovery;
        }

        const char* groups[] = {"dialout", "video"};
        int num_groups = 2;

        r->num_gids = num_groups;
        r->gids = r_array(gid_t, r->num_gids);

        char buf[1024];
        size_t buflen = sizeof(buf);
        struct group grp;
        struct group *result = NULL;
        int err;

        for (int i = 0; i < r->num_gids; i++) {
                err = getgrnam_r(groups[i], &grp, buf, buflen, &result);
                if (err != 0) {
                        r_err("Failed to obtain information on the 'dialout' group");
                        r->num_gids = 0;
                        r_free(r->gids);
                        r->gids = NULL;
                        break;
                } 
                r_info("gid(%s)=%d", groups[i], grp.gr_gid);
                r->gids[i] = grp.gr_gid;
        }
        
        r->args = NULL;
        r->pid = -1;
        r->status = k_created;
        
        return r;

error_recovery:
        delete_run(r);
        return NULL;
}

void delete_run(run_t *r)
{
        list_t *l;
        
        if (r) {
                if (r->thread) {
                        r_warn("delete_run: thread is not NULL?!");
                        thread_join(r->thread);
                        delete_thread(r->thread);
                        r->thread = NULL;
                }
                l = r->args;
                while (l) {
                        r_free(list_get(l, char));
                        l = list_next(l);
                }
                delete_list(r->args);
                if (r->name)
                        r_free(r->name);
                if (r->host)
                        r_free(r->host);
                if (r->user)
                        r_free(r->user);
                if (r->path)
                        r_free(r->path);
                if (r->gids)
                        r_free(r->gids);
                r_delete(r);
        }
}

void run_add_arg(run_t *r, const char *s)
{
        char *t = r_strdup(s);
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

void run_set_disabled(run_t *r, int value)
{
        r->disabled = value;
}

int run_disabled(run_t *r)
{
        return r->disabled;
}

/* int run_is_replay(run_t *r) */
/* { */
/*         return r->replay; */
/* } */


static int get_user_info(const char *user, uid_t *uid, gid_t *gid) 
{
        struct passwd pwbuf;
        struct passwd *pwbufp = NULL;
        char *buf;
        long bufsize;

        // From man getpwnam_r
        bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
        if (bufsize == -1)
                bufsize = 16384;

        buf = malloc(bufsize);
        if (buf == NULL) {
                r_panic("out of memory");
                return -1;
        }

        int err = getpwnam_r(user, &pwbuf, buf, bufsize, &pwbufp);
        if (pwbufp == NULL) {
                if (err == 0) {
                        r_err("Failed to obtain the UID associated with '%s'", user);
                        free(buf);
                        return -1;
                } else {
                        char msg[200];
                        strerror_r(err, msg, 200);
                        r_err("getpwnam_r failed: %s", msg);
                        free(buf);
                        return -1;
                }
        }
        free(buf);

        *uid = pwbuf.pw_uid;
        *gid = pwbuf.pw_gid;
        return 0;
}

int run_drop_privileges(run_t *r) 
{
	if (getuid() != 0)
                return 0;
	if (r->user == NULL)
                return 0;
	if (rstreq(r->user, "root"))
                return 0;

        if (r->num_gids > 0)
                if (setgroups(r->num_gids, r->gids) != 0) {
                        char msg[200];
                        strerror_r(errno, msg, sizeof(msg));
                        r_warn("Failed to set the additional groups: %s", msg);
                }

        if (setgid(r->gid) != 0) {
                r_err("Failed to set the group ID. user '%s', gid %d",
                      r->user, (int) r->gid);
                return -1;
        }
        if (setuid(r->uid) != 0) {
                r_err("Failed to set the user ID. user '%s', uid %d",
                      r->user, (int) r->uid);
                return -1;
        }
	return 0;
}

static int run_start_locally(run_t *r)
{
        char *argv[32];
	pid_t pid;
        int argc, err;
        list_t *a = r->args;
        //char dump_arg[1024];
        
        r_info("Starting '%s' locally", r->name);
        
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
                r_err("run_start_locally: Failed to return to the group id\n");
                exit(1);
        }

        if (run_drop_privileges(r) != 0) {
                r_err("run_start_locally: Failed to set the user\n");
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

        if (app_get_session() != NULL) {
                argv[argc++] = "-s";
                argv[argc++] = (char*) app_get_session();
        } else {
                if (app_get_logdir() != NULL) {
                        argv[argc++] = "-L";
                        argv[argc++] = (char*) app_get_logdir();
                }

                /* if (get_dumping() != 0 && r->dump) { */
                /*         snprintf(dump_arg, 1024, "--dump=%s", get_dumping_dir()); */
                /*         argv[argc++] = dump_arg; */
                /* } */
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
        r_info("Starting %s: %s", r->name, membuf_data(m));
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
        int argc, err;
        list_t *a = r->args;

        r_info("Starting '%s' remotely", r->name);

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
        if (r->user) {
                argv[argc++] = "-l";
                argv[argc++] = r->user;
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
        r_info("Starting %s: %s", r->name, membuf_data(m));
        delete_membuf(m);
        
        err = execv(r->path, argv);
        perror("monitor: execvp:");
        exit(0);
}

static int _run_start(run_t *r)
{
        r_debug("_run_start %s", run_name(r));
        
        if (r->host)
                return run_start_remotely(r);
        else
                return run_start_locally(r);
}

static void _run(void* userdata)
{
        run_t* r = (run_t*) userdata;
        int err, status;
        pid_t p;

        r_debug("_run %s", run_name(r));
        
        while (!app_quit()) {
                
                err = _run_start(r);

                if (err == 0) {
                        p = waitpid(r->pid, &status, 0);
                        r_info("'%s': waitpid returned", r->name);
                        if (p == -1) {
                                // Should not happen
                                char buf[200];
                                strerror_r(errno, buf, 200);
                                r_panic("panic: '%s': %s", r->name, buf);
                                app_set_quit();
                        }
                } 
                clock_sleep(1);
        }
        
        r_info("'%s' is finished", r->name);
        r->status = k_finished;
}

int run_start(run_t *r, pid_t gpid)
{
        /* if (do_replay() && r->replay) { */
        /*         r_info("Not starting '%s' because it is being replayed.", r->name); */
        /*         return 0; */
        /* } */
        r_debug("run_start %s", run_name(r));
        if (r->disabled) {
                r->pid = -1;
                r_debug("run_start %s: disabled", run_name(r));
                return 0;
        }
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

        r_info("*** run_send_stop: Sending signal to '%s', pid %d", r->name, r->pid);
        err = kill(r->pid, SIGHUP);
        r_info("*** run_send_stop: Sending signal to '%s' done", r->name, r->pid);
        if (err != 0) {
                char buf[200];
                strerror_r(errno, buf, 200);
                r_err("run_send_stop: '%s': %s", r->name, buf);
                return -1;
        }
        return 0;
}

int run_stop(run_t *r)
{
        int err;
        int sig = SIGHUP;
        int count = 0;

        if (r->pid == -1)
                return 0;

        while (1) {
                r_info("*** run_stop: Sending signal %d to '%s', pid %d (count=%d)",
                         sig, r->name, r->pid, count);
                err = kill(r->pid, sig);
                if (err != 0) {
                        char buf[200];
                        strerror_r(errno, buf, 200);
                        r_err("run_stop: '%s': %s", r->name, buf);
                        return -1;
                }
                
                r_info("Waiting for '%s' to quit", r->name);
                sleep(3);
                
                if (r->status == k_finished) {
                        r_err("'%s' has quit", r->name);
                        thread_join(r->thread);
                        delete_thread(r->thread);
                        r->thread = NULL;
                        return 0;
                }

                count++;
                if (count > 3) {
                        r_info("'%s' still alive after three signals. "
                                 "Starting to send SIGKILL.", r->name);
                        sig = SIGKILL;
                }
                if (count > 6) {
                        r_err("monitor: run_stop: '%s': "
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
        size_t len, i;
        if (s == NULL)
                return (minlen == 0);
        if (strlen(s) < (unsigned)minlen || strlen(s) > (unsigned)maxlen)
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

int valid_user(const char *user)
{
        static char *valid_chars = ("abcdefghijklmnopqrstuvwxyz"
                                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                    "1234567890"
                                    "-_");
        return valid_info(user, 0, 32, valid_chars);
}

int check_path(const char *path)
{
        struct stat statbuf;
        int err;

        err = stat(path, &statbuf);
        if (err != 0) {
                r_panic("Couldn't find executable: %s", path);
                return -1;
        }
        
        if ((statbuf.st_mode & S_IFMT) == S_IFLNK) {
                char newpath[1024];
                readlink(path, newpath, 1024);
                if (rstreq(newpath, path)) {
                        r_panic("Symbolic link points to itself?!:  %s", path);
                        return -1;
                }
                return check_path(newpath);
        }

        if ((statbuf.st_mode & S_IFMT) != S_IFREG) {
                r_panic("Not a file: %s", path);
                return -1;
        }

        if ((statbuf.st_mode & S_IXUSR) == 0) {
                r_panic("File is not executable: %s", path);
                return -1;
        }

        return 0;
}
