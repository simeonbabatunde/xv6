#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include "fscheck.h"
#include <string.h>

// Function prototypes
void addr_confirmation(char *new_bitmap, uint addr);
int bitmap_compare(char *bitmap1, char *bitmap2, int size);
uint get_block_addr(uint off, struct dinode *current_dip, int indirect_flag);
void dfs(int *inode_ref, char* new_bitmap, int inum, int parent_inum);

// Let's initialize the entry-point of each segment in the file system
void *imgfile;
void *datablk;
struct superblock *superblk;
struct dinode *diskino;
char *bitmap;

int main(int argc, char* argv[])
{
  int fdescriptor;
  struct stat statbuff;
  int data_offset, bitmap_size;
  struct dinode *temp_diskino;

  if(argc < 2){
    fprintf(stderr, "Usage: %s file_system.img\n", argv[0]);
    exit(1);
  }


  fdescriptor = open(argv[1], O_RDONLY);       // Lets read the file image
  printf("%d\n", fdescriptor);
  if(fdescriptor < 0){
    fprintf(stderr, "image not found.\n");
    exit(1);
  }

  // call fstat to get the file stat
  if(fstat(fdescriptor, &statbuff) == -1){
    fprintf(stderr, "image not found.\n");
    exit(1);
  }

  imgfile = mmap(NULL, statbuff.st_size, PROT_READ, MAP_PRIVATE, fdescriptor, 0);
  assert(imgfile != MAP_FAILED);

  superblk = (struct superblock *) (imgfile + BSIZE);   //Read the super block[1]
  diskino = (struct dinode *) (imgfile + 2*BSIZE);      //Read the inode block[2]
  bitmap = (char *) (imgfile + (superblk->ninodes/IPB + 3) * BSIZE);  //Read the bitmap block[]
  datablk = (void *) (imgfile + (superblk->ninodes/IPB + superblk->nblocks/BPB + 4) * BSIZE); //Read the data block[]

  data_offset = superblk->ninodes/IPB + superblk->nblocks/BPB + 4;
  bitmap_size = data_offset / 8;
  int inode_ref[superblk->ninodes + 1];
  char new_bitmap[bitmap_size];
  char end = 0x00;

  memset(inode_ref, 0, (superblk->ninodes + 1) * sizeof(int));     //Initialize the inode_ref to 0
  memset(new_bitmap, 0, bitmap_size);
  memset(new_bitmap, 0xFF, data_offset / 8);

  for(int i = 0; i < data_offset % 8; ++i){
    end = (end << 1) | 0x01;
  }
  new_bitmap[data_offset / 8] = end;

  // if(!(ROOTINO + diskino) || (ROOTINO + diskino)->type != T_DIR){ // Check if root dir exist
  //   fprintf(stderr, "ERROR: root directory does not exist.\n");
  //   exit(1);
  // }

  dfs(inode_ref, new_bitmap, ROOTINO, ROOTINO);

  temp_diskino = diskino;
  for(int i = 1; i < superblk->ninodes; i++){
    temp_diskino++;
    if(temp_diskino->type == 0){          //Only work with allocated inodes
      continue;
    }
    // Throw errors for Invalid types
    if(temp_diskino->type != T_FILE && temp_diskino->type != T_DIR
        && temp_diskino->type != T_DEV){
      fprintf(stderr, "ERROR: bad inode.\n");
      exit(1);
    }

    // Throw error if Inode in use but not in the directory
    // if(inode_ref[i] == 0){
    //   fprintf(stderr, "ERROR: inode marked use but not found in a directory.\n");
    //   exit(1);
    // }

    // Throw error for Bad reference count
    if(inode_ref[i] != temp_diskino->nlink){
      fprintf(stderr, "ERROR: bad reference count for file.\n");
      exit(1);
    }

    // Throw error for Extra links on directories
    if(temp_diskino->type == T_DIR && temp_diskino->nlink > 1){
      fprintf(stderr, "ERROR: directory appears more than once in file system.\n");
      exit(1);
    }
  }

  // Throw error when not in use but marked in use
  if(bitmap_compare(bitmap, new_bitmap, bitmap_size)){
    fprintf(stderr, "ERROR: bitmap marks block in use but it is not in use.\n");
    exit(1);
  }

  return 0;
}


// function implementations
// Confirm if the address is valid
void addr_confirmation(char *new_bitmap, uint addr){
  if(addr == 0){
    return;
  }

  // When given address is out of bound
  if(addr < (superblk->ninodes/IPB + superblk->nblocks/BPB + 4)
      || addr >= (superblk->ninodes/IPB + superblk->nblocks/BPB + 4 + superblk->nblocks)){
    fprintf(stderr, "ERROR: bad address in inode.\n");
    exit(1);
  }

  // In use but marked free
  char byte = *(bitmap + addr / 8);
  if(!((byte >> (addr % 8)) & 0x01)){
    fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
    exit(1);
  }
}


// Compare two bitmaps. Return 1 when they are different, 0 when the same
int bitmap_compare(char *bitmap1, char *bitmap2, int size){
  for(int i = 0; i < size; ++i){
    if(*(bitmap1++) != *(bitmap2++)){
      return 1;
    }
  }
  return 0;
}

// Get the block number
uint get_block_addr(uint off, struct dinode *current_dip, int indirect_flag){
  if(off / BSIZE <= NDIRECT && !indirect_flag){
    return current_dip->addrs[off / BSIZE];
  }else{
    return *((uint*) (imgfile + current_dip->addrs[NDIRECT] * BSIZE) + off / BSIZE - NDIRECT);
  }
}


void dfs(int *inode_ref, char* new_bitmap, int inum, int parent_inum){
  struct dinode *temp_diskino = diskino + inum;

  // if(temp_diskino->type == 0){
  //   fprintf(stderr, "ERROR: inode referred to in directory but marked free.\n");
  //   exit(1);
  // }

  // Lets check for empty dir without . and ..
  if(temp_diskino->type == T_DIR && temp_diskino->size == 0){
    fprintf(stderr, "ERROR: directory not properly formatted.\n");
  }

  // Lets update current node's reference count
  inode_ref[inum]++;
  if(inode_ref[inum] > 1 && temp_diskino->type == T_DIR){
    fprintf(stderr, "ERROR: directory appears more than once in file system.\n");
    exit(1);
  }

  int off;
  // Offset at NDIRECT will need two address check:
  // One for addrs[NDIRECT], and one for the first addr in indirect block
  int indirect_flag = 0;

  for(off = 0; off < temp_diskino->size; off += BSIZE){
    uint addr = get_block_addr(off, temp_diskino, indirect_flag);
    addr_confirmation(new_bitmap, addr);

    if(off / BSIZE == NDIRECT && !indirect_flag){
      off -= BSIZE;
      indirect_flag = 1;
    }

    // Lets check for duplicate and mark on new bitmap when the inode first occur
    if(inode_ref[inum] == 1){
      char byte = *(new_bitmap + addr / 8);
      if((byte >> (addr % 8)) & 0x01){
        fprintf(stderr, "ERROR: address used more than once.\n");
        exit(1);
      }else{
        byte = byte | (0x01 << (addr % 8));
        *(new_bitmap + addr / 8) = byte;
      }
    }

    if(temp_diskino->type == T_DIR){
      struct dirent *de = (struct dirent *) (imgfile + addr * BSIZE);

      // Lets confirm the resence of . and .. in DIR
      if(off == 0){
        if(strcmp(de->name, ".")){
          fprintf(stderr, "ERROR: directory not properly formatted.\n");
          exit(1);
        }
        if (strcmp((de + 1)->name, "..")) {
          fprintf(stderr, "ERROR: directory not properly formatted.\n");
          exit(1);
        }

        // Lets confirm the presence of parent
        if((de + 1)->inum != parent_inum){
          // if(inum == ROOTINO){
          //   fprintf(stderr, "ERROR: root directory does not exist.\n");
          // }else{
          //   fprintf(stderr, "ERROR: parent directory mismatch.\n");
          // }
          // exit(1);
        }
        de += 2;
      }

      for(; de < (struct dirent *)(ulong)(imgfile + (addr + 1) * BSIZE); de++){
        if(de->inum == 0){
          continue;
        }
        dfs(inode_ref, new_bitmap, de->inum, inum);
      }
    }
  }
}
