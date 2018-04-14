#ifndef _KTHREADS_H_
#define _KTHREADS_H_

struct stat;

struct kthread {
  int pid;
  void *stack;
};

typedef struct kthread kthread_t;

int init_lock(lock_t *lock);
void lock_acquire(lock_t *lock);
void lock_release(lock_t *lock);
struct kthread thread_create(void (*worker_routine)(void*), void *arg);
int thread_join(struct kthread);

#endif // _KTHREADS_H_
