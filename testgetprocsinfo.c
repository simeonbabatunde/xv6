#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "getprocinfo.h"


void getprocsinfotest(){
  struct procinfo procstr;

  if(getprocsinfo(&procstr) == -1){
    printf (1, "getprocsinfo failed\n");
    exit();
  }else{
    printf(1, "getprocsinfo test ok\n");
  }
}

int
main(int argc, char *argv[])
{
  printf(1, "getprocsinfo test starting...\n");

  getprocsinfotest();

  exit();
}
