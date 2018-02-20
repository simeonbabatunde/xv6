#include "types.h"
#include "stat.h"
#include "user.h"


int main(int argc, char *argv[])
{
   int page_no = (int) argv[1];
   int *addr = shmem_access(page_no);
   
   printf(1, "Virtual Address after exec %x\n", *addr);
   printf(1, "Shared mem count  after exec %x\n", shmem_count(page_no));

   if(shmem_count(0) == 2 && *addr == 5){
     printf(1, "....Exect Test Passed\n");
   }else{
     printf(1, "....Exect Test Failed\n");
   }
}
