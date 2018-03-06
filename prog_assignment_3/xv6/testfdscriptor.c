/* Ensures clone copies file descriptors but does not share created ones with parent*/
#include "types.h"
#include "user.h"
#include "fcntl.h"
#include "x86.h"


int ppid;
#define PGSIZE (4096)
#define NULL 0

volatile uint newfd;

#define assert(x) if (x) {} else { \
   printf(1, "%s: %d ", __FILE__, __LINE__); \
   printf(1, "assert failed (%s)\n", # x); \
   printf(1, "TEST FAILED...............\n"); \
   kill(ppid); \
   exit(); \
}

void worker(void *arg_ptr);

int
main(int argc, char *argv[])
{
   printf(1, "File descriptor test starting ...............\n");
   ppid = getpid();
   void *stack = malloc(PGSIZE*2);
   assert(stack != NULL);
   if((uint)stack % PGSIZE){
     stack = stack + (4096 - (uint)stack % PGSIZE);
   }

   int fd = open("tmp", O_WRONLY|O_CREATE);
   assert(fd == 3);
   int clone_pid = clone(worker, 0, stack);
   assert(clone_pid > 0);
   while(!newfd);
   assert(write(newfd, "Goodbye\n", 8) == -1);
   printf(1, "TEST PASSED...............\n");
   exit();
}

void
worker(void *arg_ptr) {
   assert(write(3, "hello\n", 6) == 6);
   xchg(&newfd, open("tmp2", O_WRONLY|O_CREATE));
   exit();
}
