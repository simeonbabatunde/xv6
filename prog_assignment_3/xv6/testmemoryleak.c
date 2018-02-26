/* set up stack correctly (and without extra items) */
#include "types.h"
#include "user.h"

int ppid;
#define PGSIZE (4096)
// volatile int global = 1;

#define assert(x) if (x) {} else { \
   printf(1, "%s: %d ", __FILE__, __LINE__); \
   printf(1, "assert failed (%s)\n", # x); \
   printf(1, "TEST FAILED...............\n"); \
   kill(ppid); \
   exit(); \
}

void worker(void *arg);

int global = 0;
int num_threads = 2000;

int
main(int argc, char *argv[])
{
  printf(1, "THREAD MEMORY LEAK TEST STARTING .............................\n");
  ppid = getpid();

  int i, thread_pid, join_pid;
  for(i = 0; i < num_threads; i++)
  {
    global = 1;
    thread_pid = thread_create(worker, 0);
    assert(thread_pid > 0);
    join_pid = thread_join();
    assert(thread_pid == join_pid);
    assert(global == 5);
    assert((uint) sbrk(0) < 150 * 4096);
  }
  printf(1, "global value %d\n", global);
  printf(1, "THREAD MEMORY LEAK TEST PASSED .............................\n");

  exit();
}

void worker(void *arg)
{
    assert(global == 1);
    global+=4;
    exit();
}
