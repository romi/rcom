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
int valid_username(const char *username);
int check_path(const char *path);


run_t *run_parse(json_object_t node);
run_t *new_run(const char *name,
               const char *path,
               const char *host,
               const char *username/* , */
               /* int attach */);
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
