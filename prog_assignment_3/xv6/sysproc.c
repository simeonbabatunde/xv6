#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// We will call the clone syscall from here
int sys_clone(void)
{
  void *fcn, *arg, *stack;
  // Confirm first argument is not empty
  if(argptr(0, (void *)&fcn, sizeof(void *)) < 0){
    return -1;
  }
  // Confirm second argument is not empty
  if(argptr(1, (void *)&arg, sizeof(void *)) < 0){
    return -1;
  }
  // Confirm third argument is not empty
  if(argptr(2, (void *)&stack, sizeof(void *)) < 0){
    return -1;
  }
  // Confirm if the stack is page-aligned
  if((uint)stack % PGSIZE != 0){
    return -1;
  }
  // Confirm if the stack is one page in size
  if((uint)myproc()->sz - (uint)stack == PGSIZE/2){
    return -1;
  }
  // Call the clone and return pid
  return clone(fcn, arg, stack);
}

// We will call the join syscall from here
int sys_join(void)
{
  // void **stack;
  int pid;
  // Confirm  kthred pid is not empty
  if(argptr(0, (void *)&pid, sizeof(int)) < 0){
    return -1;
  }
  // Call the join and return pid
  return join(pid);
}
