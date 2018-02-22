#ifndef _KTHREADS_H_
#define _KTHREADS_H_

struct stat;
typedef struct __lock_t{
	uint flag;
}lock_t;


int thread_create(void (*start_routine)(void*), void *arg);
int thread_join();
int lock_init(lock_t *lk);
void lock_acquire(lock_t *lk);
void lock_release(lock_t *lk);


#endif // _KTHREADS_H_
