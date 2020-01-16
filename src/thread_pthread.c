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
        thread_t* thread = r_new(thread_t);
        if (thread == NULL)
                return NULL;

        thread->run = run;
        thread->data = data;
        thread->autodelete = autodelete;

        int ret = pthread_create(&thread->thread, NULL, _run, (void*) thread);
        if (ret != 0) {
                r_err("new_thread: failed to create the thread");
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
        if (thread) r_delete(thread);
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
        mutex_t *mutex = r_new(mutex_t);
        if (mutex == NULL)
                return NULL;
        pthread_mutex_init(&mutex->mutex, NULL);
        return mutex;
}

void delete_mutex(mutex_t *mutex)
{
        if (mutex) {
                pthread_mutex_destroy(&mutex->mutex);
                r_delete(mutex);
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
