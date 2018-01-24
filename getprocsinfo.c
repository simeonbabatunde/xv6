#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int
main(int argc, char *argv[])
{
  int k = getprocsinfo();
  printf(1, "%d\n", k);

  exit();
}
