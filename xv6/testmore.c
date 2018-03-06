#include "types.h"
#include "user.h"

int ppid;
#define PGSIZE (4096)
#define NULL 0

volatile int global = 1;

#define assert(x) if (x) {} else { \
   printf(1, "%s: %d ", __FILE__, __LINE__); \
   printf(1, "assert failed (%s)\n", # x); \
   printf(1, "TEST FAILED...............\n"); \
   kill(ppid); \
   exit(); \
}

// Prototypes
void routine(void *arg_ptr);

void testWaitShouldHandleForkAndNotJoin()
{
  printf(1, "Test_wait_should_handle_fork_and_not_join starting ...\n");

  int fork_pid = fork();
  if(fork_pid == 0){
    exit();
  }
  assert(fork_pid > 0);

  int join_response = join(fork_pid);

  assert(join_response == -1);
  printf(1, "TEST PASSED ....................\n");

  // Wait to clean up the mess created
  wait();
  // exit();
}

void testJoinShouldHandleThreadsAndNotWait()
{
  printf(1, "Test_join_should_handle_threads_and_not_wait starting ...\n");

  void *stack = malloc(PGSIZE * 2);
  assert(stack != NULL);
  // Confirm if it's page aligned
  if((uint)stack % PGSIZE){
    stack = stack + (PGSIZE - (uint)stack % PGSIZE);
  }

  int arg = 20;
  int clone_pid = clone(routine, &arg, stack);
  assert(clone_pid > 0);

  sleep(300);
  assert(wait() == -1);

  int join_response = join(clone_pid);
  assert(join_response == 0);
  assert(global == 5);

  printf(1, "TEST PASSED ....................\n");

  exit();
}

void routine(void *arg_ptr)
{
  int arg = *(int *)arg_ptr;
  assert(arg == 20);
  assert(global == 1);
  global += 4;
  exit();
}


int main(int argc, char *argv[])
{
  ppid = getpid();

  testWaitShouldHandleForkAndNotJoin();

  testJoinShouldHandleThreadsAndNotWait();

  exit();
}
