/**********************************************************************
gtthread_mutex.c.
This file contains the implementation of the mutex subset of the
gtthreads library.  The locks can be implemented with a simple queue.
 **********************************************************************/

/*
  Include as needed
*/

#include <stdio.h>
#include <stdlib.h>

#include "gtthread.h"

/*
  The gtthread_mutex_init() function is analogous to
  pthread_mutex_init with the default parameters enforced.
  There is no need to create a static initializer analogous to
  PTHREAD_MUTEX_INITIALIZER.
 */
int gtthread_mutex_init(gtthread_mutex_t* mutex){
  steque_init(&mutex->schedule);
  mutex->locked = false;
  mutex->lockThreadId = -1;
  return 0;
}

/*
  The gtthread_mutex_lock() is analogous to pthread_mutex_lock.
  Returns zero on success.
 */
int gtthread_mutex_lock(gtthread_mutex_t* mutex){
  if (mutex->locked == false) {
    mutex->locked = true;
    mutex->lockThreadId = currThread->id;
    return 0;
  }

  steque_enqueue(&mutex->schedule, &currThread->id);
  while(true) {
    while (mutex->locked == true) {
      gtthread_yield();
    }

    int * nextId = (int *)steque_front(&mutex->schedule);
    if (*nextId == currThread->id) {
      steque_pop(&mutex->schedule);
      mutex->locked = true;
      mutex->lockThreadId = currThread->id;
      return 0;
    } else {
      gtthread_yield();
    }
  }

  return -1;
}

/*
  The gtthread_mutex_unlock() is analogous to pthread_mutex_unlock.
  Returns zero on success.
 */
int gtthread_mutex_unlock(gtthread_mutex_t *mutex){
  mutex->lockThreadId = -1;
  mutex->locked = false;
  return 0;
}

/*
  The gtthread_mutex_destroy() function is analogous to
  pthread_mutex_destroy and frees any resourcs associated with the mutex.
*/
int gtthread_mutex_destroy(gtthread_mutex_t *mutex){
  steque_destroy(&mutex->schedule);
  return 0;
}