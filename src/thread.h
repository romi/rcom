#ifndef _RCOM_THREAD_H_
#define _RCOM_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _thread_t thread_t;

typedef void (*thread_run_t)(void* data);

thread_t* new_thread(thread_run_t run, void *data, int realtime);
void delete_thread(thread_t* thread);
int thread_join(thread_t* thread);


typedef struct _mutex_t mutex_t;

mutex_t *new_mutex();
void delete_mutex(mutex_t *mutex);
void mutex_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_THREAD_H_
