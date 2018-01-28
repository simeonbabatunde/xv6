#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "getprocinfo.h"

//struct procinfo procstr;

void getprocsinfotest(){
  struct procinfo procptr;
  procptr.pname = (char **)malloc(sizeof(char *) * 64);
  for(int i=0; i<64; i++){
    procptr.pname[i] = (char *)malloc(16 * sizeof(char *));
  }

  int count = getprocsinfo(&procptr);
  if(count == -1){
    printf (1, "getprocsinfo failed\n");
    exit();
  }else{
    printf(1, "pid\tpname\n");
    for(int i=0; i<count; i++){
      printf(1, "%d \t%s\n", procptr.pid[i], procptr.pname[i]);
    }
    printf(1, "getprocsinfo test ok\n");
  }

}

int
main(int argc, char *argv[])
{
  //struct procinfo procptr;
  printf(1, "getprocsinfo test starting...\n");

  getprocsinfotest();

  exit();
}
