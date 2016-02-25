#ifndef GTTHREAD_H
#define GTTHREAD_H

#include "steque.h"

#include <ucontext.h>
#include <stdbool.h>

/* Define gtthread_t and gtthread_mutex_t types here */

typedef struct gtthread_t {
  int id;
  ucontext_t ucp;
  bool cancelled;
  bool finished;
  void* value;
  int joined_thread_id;
} gtthread_t;

typedef struct gtthread_mutex_t {
  steque_t schedule;
  int lockThreadId;
  bool locked;
} gtthread_mutex_t;

extern gtthread_t * currThread;

void gtthread_init(long period);
int  gtthread_create(gtthread_t *thread,
                     void *(*start_routine)(void *),
                     void *arg);
int  gtthread_join(gtthread_t thread, void **status);
void gtthread_exit(void *retval);
void gtthread_yield(void);
int  gtthread_equal(gtthread_t t1, gtthread_t t2);
int  gtthread_cancel(gtthread_t thread);
gtthread_t gtthread_self(void);


int  gtthread_mutex_init(gtthread_mutex_t *mutex);
int  gtthread_mutex_lock(gtthread_mutex_t *mutex);
int  gtthread_mutex_unlock(gtthread_mutex_t *mutex);
int  gtthread_mutex_destroy(gtthread_mutex_t *mutex);
void run_thread(void *(*start_routine)(void *), void* args);
#endif