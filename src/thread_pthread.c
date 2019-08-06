#include <pthread.h>
#include "rcom/log.h"
#include "rcom/thread.h"
#include "mem.h"

struct _thread_t {
        pthread_t thread;
        thread_run_t run;
        void *data;
        int autodelete;
};

static void* _run(void* data);

thread_t* new_thread(thread_run_t run, void *data, int realtime, int autodelete)
{
        thread_t* thread = new_obj(thread_t);
        if (thread == NULL)
                return NULL;

        thread->run = run;
        thread->data = data;
        thread->autodelete = autodelete;

        int ret = pthread_create(&thread->thread, NULL, _run, (void*) thread);
        if (ret != 0) {
                log_err("new_thread: failed to create the thread");
                delete_thread(thread);
                return NULL;
        }
        
        return thread;
}

static void* _run(void* data)
{
        thread_t* thread = (thread_t*) data;
        thread->run(thread->data);
        if (thread->autodelete)
                delete_thread(thread);
        pthread_exit(NULL);
        return NULL;
}

void delete_thread(thread_t* thread)
{
        if (thread) delete_obj(thread);
}

int thread_join(thread_t* thread)
{
        pthread_join(thread->thread, NULL);
}

/************************************************************/

struct _mutex_t
{
        pthread_mutex_t mutex;
};

mutex_t *new_mutex()
{
        mutex_t *mutex = new_obj(mutex_t);
        if (mutex == NULL)
                return NULL;
        pthread_mutex_init(&mutex->mutex, NULL);
        return mutex;
}

void delete_mutex(mutex_t *mutex)
{
        if (mutex) {
                pthread_mutex_destroy(&mutex->mutex);
                delete_obj(mutex);
        }
}

void mutex_lock(mutex_t *mutex)
{
        pthread_mutex_lock(&mutex->mutex);
}

void mutex_unlock(mutex_t *mutex)
{
        pthread_mutex_unlock(&mutex->mutex);
}
