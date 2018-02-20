typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;
#ifndef NULL
#define NULL (0)
#endif
// Shared memory information
#define SHMEM_PAGE_NUM 4
int shmem_counter[SHMEM_PAGE_NUM];
void* shmem_addr[SHMEM_PAGE_NUM];
