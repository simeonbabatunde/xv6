#include "types.h"
#include "user.h"
#include "stat.h"
#include "fcntl.h"
#include "memlayout.h"
#include "mmu.h"
void
testPassed(void)
{
  printf(1, "....Passed\n");
}

void
testFailed(void)
{
  printf(1, "....FAILED\n");
}

void expectedVersusActualNumeric(char* name, int expected, int actual)
{
  printf(1, "      %s expected: %d, Actual: %d\n", name, expected, actual);
}

void whenAProcessAccessAPageTwice_shmemReturnsSameAddress()
{
  printf(1, "Test: whenAProcessAccessAPageTwice_shmemReturnsSameAddress...");
  int *shaddr1, *shaddr2;

  shaddr1 = (int *)shmem_access(0);
  shaddr2 = (int *)shmem_access(0);
  
  if(shaddr1 == shaddr2){
    testPassed();
  }else{
    testFailed();
  }

}

void whenRequestingSharedMemory_ValidAddressIsReturned(void)
{
  printf(1, "Test: whenRequestingSharedMemory_ValidAddressIsReturned...");
  char* sharedPage = shmem_access(0);
  char* highestPage =       (char*)(KERNBASE - PGSIZE);
  char* secondHighestPage = (char*)(KERNBASE - 2*PGSIZE);
  char* thirdHighestPage =  (char*)(KERNBASE - 3*PGSIZE);
  char* fourthHighestPage = (char*)(KERNBASE - 4*PGSIZE);
  
  if(sharedPage == highestPage ||
     sharedPage == secondHighestPage ||
     sharedPage == thirdHighestPage ||
     sharedPage == fourthHighestPage) {
    testPassed();
  } else {
    testFailed(); 
  }
}

void whenInvalidPageNumberidPassedAsArgument_countReturnMunus1()
{
  printf(1, "Test: whenInvalidPageNumberisPassedAsArgument_countReturnMunus1..."); 
  int pcount = shmem_count(10);
   if(pcount == -1){
      testPassed();
   }else{
      testFailed();
   }
}


void whenForkIsCalledByParentProcess_childReturn15()
{
  printf(1, "Test: whenForkIsCalledByParentProcess_childReturn15...");
  int *var = shmem_access(1);
  *var = 15;			//Assign 15 to the shared memory 

  int pid = fork();		//Create a child process
  if(pid>0)
  {
    pid = wait();		//wait for child to finish
    printf(1, "process count is now %d after child finishes\n", shmem_count(1));
    if(shmem_count(1) == 1){
       testPassed();
    }else{
       testFailed();
    }
  }else if(pid == 0)
  {
    printf(1, "Shared memory access count after fork is: %d\n", shmem_count(1));
    int *var2 = shmem_access(1);
    if(*var == *var2)
    {
       printf(1, "Child read %d written by parent process\n", *var2);
       testPassed();
    }else
    {
       testFailed();
    }
    exit();
  }else
  {
    printf(1, "Fork failed to due to some reasons\n");
  }
  
}

void whenSharingAPage_ParentSeesChangesMadeByChild()
{
  printf(1, "Test: whenSharingAPage_ParentSeesChangesMadeByChild...");
  char* sharedPage = shmem_access(0);
  sharedPage[0] = 42;

  int pid = fork();
  if(pid == 0){
    // in child
    char* childsSharedPage = shmem_access(0);
    childsSharedPage[0] = childsSharedPage[0] + 1;
    exit();
  } else {
    // in parent
    wait(); // wait for child to terminate
    if(sharedPage[0] == 43){
      testPassed();
    } else {
      testFailed();
      expectedVersusActualNumeric("'sharedPage[0]'", 43, sharedPage[0]);
    }
  }
}

void whenExecIsCalledToRunAProgram_shouldMaintaintState()
{
   printf(1, "Test: whenExecIsCalledToRunAProgram_shouldMaintaintState...");
   char *argv[2];
   argv[0] = "runwithexec";
   argv[1] = 0;

   int *a = shmem_access(0);
   *a = 5;
   int *b = shmem_access(0);
   if(*a == *b){
     testPassed();
   }else{
     testFailed();
   }
  
   int pid = fork();
   if(pid > 0){
     pid = wait();
     exec("runwithexec", argv);
   }else if(pid == 0){
      printf(1, "Tjis is child process");
   }

}

int main(int argc, char *argv[])
{
  printf(1, "Shared memory test starting...\n");
  
  whenAProcessAccessAPageTwice_shmemReturnsSameAddress();

  whenInvalidPageNumberidPassedAsArgument_countReturnMunus1();

  whenForkIsCalledByParentProcess_childReturn15();

  whenRequestingSharedMemory_ValidAddressIsReturned();
 
  whenSharingAPage_ParentSeesChangesMadeByChild();  

  exit();
}
