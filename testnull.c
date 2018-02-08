#include "types.h"
#include "user.h"


void null_test()
{ 
  int *iptr = 0;
 	
  printf(1, "Derefereence a null pointer %d\n", *iptr);

  printf(1, "Test Failed if execution reached here ...\n");

}











int main(int argc, char *argv[])
{
  printf(1, "NULL pointer dereference test starting...\n");

  null_test(); 
 
  exit();
}
