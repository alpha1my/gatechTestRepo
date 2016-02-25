/**********************************************************************
gtthread_sched.c.
This file contains the implementation of the scheduling subset of the
gtthreads library.  A simple round-robin queue should be used.
 **********************************************************************/
/*
  Include as needed
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include "gtthread.h"

static sigset_t vtalrm;

/*
   Students should define global variables and helper functions as
   they see fit.
 */

gtthread_t* threads[100];
gtthread_t myThread;\
gtthread_t* currThread;
int idCounter = 0;
steque_t schedule;

void schedule_next(int sig) {
  sigprocmask(SIG_BLOCK, &vtalrm, NULL);

  if (steque_isempty(&schedule)) {
    if (currThread->finished || currThread->cancelled) {
      ucontext_t * curContext = &currThread->ucp;
      currThread = &myThread;
      sigprocmask(SIG_UNBLOCK, &vtalrm, NULL);
      swapcontext(curContext, &myThread.ucp);
      return;
    }
    // Keep running the current thread since nothing else is queued.
    sigprocmask(SIG_UNBLOCK, &vtalrm, NULL);
    return;
  }

  gtthread_t* nextThread = steque_pop(&schedule);

  if (!currThread->finished &&
      !currThread->cancelled) {
    steque_enqueue(&schedule, currThread);
  }
  ucontext_t * currentContext = NULL;
  if (currThread != NULL) {
    currentContext = &currThread->ucp;
  } else {
    currentContext = &myThread.ucp;
  }
  currThread = nextThread;
  sigprocmask(SIG_UNBLOCK, &vtalrm, NULL);
  swapcontext(currentContext, &nextThread->ucp);
}


/*
  The gtthread_init() function does not have a corresponding pthread equivalent.
  It must be called from the main thread before any other GTThreads
  functions are called. It allows the caller to specify the scheduling
  period (quantum in micro second), and may also perform any other
  necessary initialization.  If period is zero, then thread switching should
  occur only on calls to gtthread_yield().
  Recall that the initial thread of the program (i.e. the one running
  main() ) is a thread like any other. It should have a
  gtthread_t that clients can retrieve by calling gtthread_self()
  from the initial thread, and they should be able to specify it as an
  argument to other GTThreads functions. The only difference in the
  initial thread is how it behaves when it executes a return
  instruction. You can find details on this difference in the man page
  for pthread_create.
 */
void gtthread_init(long period){
  currThread = &myThread;
  // initialize main thread.
  myThread.joined_thread_id = -1;
  myThread.id = -100;
  if (getcontext(&myThread.ucp) < 0) {
    perror("Unable to getcontext for main");
    exit(-1);
  }
  myThread.ucp.uc_stack.ss_sp = (char*) malloc(SIGSTKSZ);
  myThread.ucp.uc_stack.ss_size = SIGSTKSZ;

  sigemptyset(&vtalrm);
  sigaddset(&vtalrm, SIGVTALRM);
  struct sigaction act;
  memset (&act, '\0', sizeof(act));
  act.sa_handler = &schedule_next;
  if (sigaction(SIGVTALRM, &act, NULL) < 0) {
    perror("sigaction");
    exit(1);
  }
  steque_init(&schedule);
  if (period > 0) {
    struct itimerval* T = (struct itimerval*) malloc(sizeof(struct itimerval));
    T->it_value.tv_sec = T->it_interval.tv_sec = 0;
    T->it_value.tv_usec = T->it_interval.tv_usec = period;
    setitimer(ITIMER_VIRTUAL, T, NULL);
  }
}


void run_thread(void *(*start_routine)(void *), void* args)
{
  void * value = start_routine(args);
  if (gtthread_equal(*currThread, myThread)) {
    int * t = (int *)value;
    exit(*t);
  } else {
    gtthread_exit(value);
  }
}


/*
  The gtthread_create() function mirrors the pthread_create() function,
  only default attributes are always assumed.
 */
int gtthread_create(gtthread_t *thread,
		    void *(*start_routine)(void *),
		    void *arg){
  thread->finished = false;
  thread->cancelled = false;
  thread->id = idCounter++;
  thread->joined_thread_id = -1;
  if (thread->id >= 100) {
    perror("Max thread count reached!");
    exit(-1);
  }

  if (getcontext(&thread->ucp) < 0) {
    perror("Unable to getcontext");
    exit(-1);
  }

  thread->ucp.uc_stack.ss_sp = (char*) malloc(SIGSTKSZ);
  thread->ucp.uc_stack.ss_size = SIGSTKSZ;
  thread->ucp.uc_link = &myThread.ucp;

  makecontext(&thread->ucp, run_thread, 2, start_routine, arg);

  threads[thread->id] = thread;
  sigprocmask(SIG_BLOCK, &vtalrm, NULL);
  steque_enqueue(&schedule, thread);
  sigprocmask(SIG_UNBLOCK, &vtalrm, NULL);
  return 0;
}

int gtthread_join(gtthread_t thread, void **status) {
  if (gtthread_equal(thread, *currThread)) {
    return -1;
  }

  gtthread_t* joinedThread = threads[thread.id];

  if (currThread->joined_thread_id == joinedThread->id) {
    return -2;
  }

  joinedThread->joined_thread_id = currThread->id;

  while (!joinedThread->finished && !joinedThread->cancelled) {
    gtthread_yield();
  }

  if (status != NULL) {
    *status = joinedThread->value;
  }

  return 0;
}

/*
  The gtthread_exit() function is analogous to pthread_exit.
 */
void gtthread_exit(void* value) {
  currThread->value = value;
  currThread->finished = true;
  raise(SIGVTALRM);
}


/*
  The gtthread_yield() function is analogous to pthread_yield, causing
  the calling thread to relinquish the cpu and place itself at the
  back of the schedule queue.
 */
void gtthread_yield(void){
  raise(SIGVTALRM);
}

/*
  The gtthread_yield() function is analogous to pthread_equal,
  returning non-zero if the threads are the same and zero otherwise.
 */
int gtthread_equal(gtthread_t t1, gtthread_t t2){
  return t1.id == t2.id ? 1 : 0;
}

/*
  The gtthread_cancel() function is analogous to pthread_cancel,
  allowing one thread to terminate another asynchronously.
 */
int gtthread_cancel(gtthread_t thread){
  threads[thread.id]->cancelled = true;
  return 0;
}

/*
  Returns calling thread.
 */
gtthread_t gtthread_self(void) {
  return *currThread;
}