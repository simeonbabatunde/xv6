#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "getprocinfo.h"


void getprocsinfotest(){
  struct procinfo *procptr;

  //Dynamically allocate memory
  procptr = (struct procinfo*)malloc(sizeof(struct procinfo) * 64);

  //Allocate for the name variable
  for(int i=0; i<64; i++){
    procptr[i].pname = (char *)malloc(21 * sizeof(char *));
  }

  //Inititate the system call here
  int count = getprocsinfo(procptr);
  if(count == -1){
    printf (1, "getprocsinfo failed\n");
    exit();
  }else{
    printf(1, "pid\tpname\n");
    for(int i=0; i<count; i++){
      printf(1, "%d\t%s\n", procptr[i].pid, procptr[i].pname);
    }
    printf(1, "getprocsinfo test ok\n");
  }

  //Free the memory blocks
  for(int i=0; i<64; i++){
    free(procptr[i].pname);
  }
  free(procptr);
}

int
main(int argc, char *argv[])
{
  printf(1, "getprocsinfo test starting...\n");

  getprocsinfotest();

  exit();
}
