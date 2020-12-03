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
#ifndef _RCOM_RUN_H_
#define _RCOM_RUN_H_

#include "rcom.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _run_status_t {
        k_created,
        k_starting,
        k_running,
        k_finished
} run_status_t;

typedef struct _run_t run_t;

// Should go elsewhere. util.h?
int valid_name(const char *name);
int valid_path(const char *path);
int valid_arg(const char *arg);
int valid_addr(const char *host);
int valid_user(const char *user);
int check_path(const char *path);

run_t *new_run(const char *name,
               const char *path,
               const char *host,
               const char *user);
void delete_run(run_t *r);

run_status_t run_status(run_t *r);
const char *run_name(run_t *r);
//int run_is_replay(run_t *r);

void run_add_arg(run_t *r, const char *s);
/* void run_add_dependency(run_t *r, const char *s); */
/* int run_count_dependencies(run_t *r); */
/* const char *run_get_dependency(run_t *r, int n); */

void run_set_disabled(run_t *r, int value);
int run_disabled(run_t *r);

int run_start(run_t *r, pid_t gpid);
int run_send_stop(run_t *r);
int run_stop(run_t *r);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_RUN_H_
