#ifndef _KTHREADS_H_
#define _KTHREADS_H_

struct stat;

typedef int kthread_t;

int init_lock(lock_t *lock);
void lock_acquire(lock_t *lock);
void lock_release(lock_t *lock);
int thread_create(void (*worker_routine)(void*), void *arg);
int thread_join();


#endif // _KTHREADS_H_
