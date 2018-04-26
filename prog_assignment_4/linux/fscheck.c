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
int checkRootInode();

// Let's initialize the entry-point of each segment in the file system
void *imgfile;
void *datablk;
struct superblock *superblk;
struct dinode *diskino;
void *bitmap;

int fdescriptor;
struct stat statbuff;
int data_offset, bitmap_size;
struct dinode *temp_diskino;

int main(int argc, char* argv[])
{
  int i, j, blockAddr, type;

  if(argc < 2){
    fprintf(stderr, "Usage: %s file_system.img\n", argv[0]);
    exit(1);
  }


  fdescriptor = open(argv[1], O_RDONLY);       // Lets read the file image
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
  diskino = (struct dinode *) (imgfile + 2*BSIZE);      // First inode block in list of inodes
  bitmap = (void *) (imgfile + (superblk->ninodes/IPB + 3) * BSIZE);  // Pointer to bitmap location
  datablk = (void *) (imgfile + (superblk->ninodes/IPB + superblk->nblocks/BPB + 4) * BSIZE); //Read the data block[]

  int usedBlocks[superblk->size];
  int numInodeLinks[superblk->ninodes];
  int inodeUsed[superblk->ninodes];           // List of used inodes
  int usedInodeDirectory[superblk->ninodes];  // Directory list for used inodes

  // Lets initialize block list to 0
  for(j = 0; j < superblk->size; j++){
    usedBlocks[j] = 0;
  }
  // Mark used blocks as used in list
  for(j = 0; j < (superblk->ninodes/IPB + 3 + (superblk->size/(BSIZE*8) + 1)); j++){
    usedBlocks[j] = 1;
  }

  type  = checkRootInode();

  // Loop through inodes
  for(i = 0; i < superblk->ninodes; i++){
    int k;
    struct dirent *currDir;
    // type = diskino->type;

    // Lets check for valid file ty
    // Check for valid file type
		if(type >= 0 && type <= 3){
      if(type != 0){
        // If directory, check to make sure it is a valid directory
        if(type == T_DIR){
          void *blkAddr = imgfile + diskino->addrs[0] * BSIZE;
		      currDir = (struct dirent*)(blkAddr);
					// Lets check for valid directory format
		      if(strcmp(currDir->name, ".") != 0){
            fprintf(stderr, "ERROR: directory not properly formatted.\n");
		        exit(1);
          }
          currDir++;
		      if(strcmp(currDir->name, "..") != 0){
            fprintf(stderr, "ERROR: directory not properly formatted.\n");
            exit(1);
          }
          // Check 5: valid parent directory
		      if(i != 1 && currDir->inum == i){
            fprintf(stderr, "ERROR: parent directory mismatch.\n");
            return 1;
          }
          struct dinode *parentDir = (struct dinode *)(imgfile + 2*BSIZE + currDir->inum*sizeof(struct dinode));
          if(parentDir->type != T_DIR){
            fprintf(stderr, "ERROR: parent directory mismatch.\n");
            return 1;
          }
          int validPrntDir = 0;
					int x, y, z;
          for(x = 0; x < NDIRECT; x++){
            struct dirent *currDir;
            if(parentDir->addrs[x] != 0){
							currDir = (struct dirent *)(imgfile + parentDir->addrs[x]*BSIZE);
							// Check for valid parent directory
              for(y = 0; y < BSIZE/sizeof(struct dirent); y++){
                if(currDir->inum != i){
                  validPrntDir = 1;
								}
								currDir++;
              }
            }
		        if(diskino->addrs[x] != 0){
              currDir = (struct dirent *)(imgfile + diskino->addrs[x]*BSIZE);
							// Find used inodes and mark them in list
              for(z = 0; z < BSIZE/sizeof(struct dirent *); x++){
                if(currDir->inum == 0){
                  break;
								}
                usedInodeDirectory[currDir->inum] = 1;
								// For each used inode found also increment reference count
                if(strcmp(currDir->name,".") != 0 && strcmp(currDir->name,"..") != 0){
                  numInodeLinks[currDir->inum]++;
                }
                currDir++;
              }
            }
          }
          // Check 5: valid parent directory
          if(validPrntDir == 0){
            fprintf(stderr, "ERROR: parent directory mismatch.\n");
            exit(1);
          }
        }

        for(k = 0; k < NDIRECT+1; k++){
          int x;
          blockAddr = diskino->addrs[k];
          if(blockAddr != 0){
            if (blockAddr != 0){
              // Lets check for bad address in inode
              if((blockAddr) < ((int)BBLOCK(superblk->nblocks, superblk->ninodes))+1
                || blockAddr > (superblk->size * BSIZE)){
                fprintf(stderr, "ERROR: bad address in inode.\n");
                exit(1);
              }
              // Check 8: check used blocks are only used once
              if(usedBlocks[blockAddr] == 1){
                fprintf(stderr, "ERROR: address used more than once.\n");
                exit(1);
              }
            }
            if(type == 1 && blockAddr > 1 && k == 0){
              int checkDirFormat = 0, isRoot = 0, checkRootDir = 0;
						  if(i == 1){
                isRoot++;
              }
              for(x = 0; x < NDIRECT+1; x++){
                currDir = (struct dirent *)(imgfile + (blockAddr*BSIZE) + x*(sizeof(struct dirent)));
                if(currDir->inum != 0){
                  if(strcmp(currDir->name, ".") == 0 || strcmp(currDir->name, "..") == 0){
                    checkDirFormat++;
                    if(isRoot == 1 && currDir->inum == 1)
                      checkRootDir++;
								}
							}
						}
            // Check 4: valid directory format
						if(checkDirFormat != 1){
              fprintf(stderr, "ERROR: directory not properly formatted.\n");
							exit(1);
						}
            // Check 3: valid root inode
						if(isRoot == 1){
              if(checkRootDir != 2){
                fprintf(stderr, "ERROR: root directory does not exist.\n");
								exit(1);
							}
						}
					}
        }
        if(diskino->size > BSIZE * NDIRECT){
          int *indirect = (int *)(imgfile + (blockAddr*BSIZE));
          for(k = 0; k < BSIZE/4; k++){
            int block = *(indirect + k);
            // Check if address is valid
            if(block != 0){
              // Check 2: bad address in inode
              if(block < ((int)BBLOCK(superblk->nblocks, superblk->ninodes))+1){
                fprintf(stderr, "ERROR: bad address in inode.\n");
                exit(1);
              }
              // Check 8: check used blocks are only used once
              if(usedBlocks[block] == 1){
                fprintf(stderr, "ERROR: address used more than once.\n");
                exit(1);
              }
              usedBlocks[block] = 1;
              // Check 6: check used blocks in bitmap
              int bitmapLocation = (*((char*)bitmap + (block >> 3)) >> (block & 7)) & 1;
              if(bitmapLocation == 0){
                fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
                exit(1);
              }
            }
          }
        }
      }
      diskino++;
    }else{
      // Check 1: valid type
			fprintf(stderr,"ERROR: bad inode.\n");
			exit(1);
    }
  }
  numInodeLinks[1]++;
  // Check 7: check used blocks are marked bitmap and are used
  int block;
  for(block = 0; block < superblk->size; block++){
    int bitmapLocation = (*((char*)bitmap + (block >> 3)) >> (block & 7)) & 1;
    if(bitmapLocation == 1){
      if(usedBlocks[block] == 0){
        fprintf(stderr, "ERROR: bitmap marks block in use but it is not in use.\n");
        exit(1);
      }
    }
  }

  diskino = (struct dinode *)(imgfile + 2*BSIZE);
  for(i = 0; i < superblk->ninodes; i++){
    type = diskino->type;
    if(type != 0){
      inodeUsed[i] = 1;
    }
    // Check 9: check used inodes reference a directory
    if(inodeUsed[i] == 1 && usedInodeDirectory[i] == 0){
      fprintf(stderr, "ERROR: inode marked use but not found in a directory.\n");
      exit(1);
    }
    // Check 10: check used inodes in directory are in inode table
    if(usedInodeDirectory[i] == 1 && inodeUsed[i] == 0){
      fprintf(stderr, "ERROR: inode referred to in directory but marked free.\n");
      exit(1);
    }
		// Check 12: No extra links allowed for directories
    if(type == 1 && numInodeLinks[i] != 1){
      fprintf(stderr, "ERROR: directory appears more than once in file system.\n");
      exit(1);
    }
		// Check 11: hard links to file match files reference count
    if(numInodeLinks[i] != diskino->nlink){
      fprintf(stderr, "ERROR: bad reference count for file.\n");
      exit(1);
    }
		diskino++;
  }
}


  return 0;
}


// function implementations
int checkRootInode(){
	if (lseek(fdescriptor, superblk->inodestart * BSIZE + sizeof(struct dinode),
    SEEK_SET) != superblk->inodestart * BSIZE + sizeof(struct dinode)){
		perror("lseek failed");
		exit(1);
	}
	u_char buf[sizeof(struct dinode)];
	struct dinode temp_inode;
	read(fdescriptor, buf, sizeof(struct dinode));
	memmove(&temp_inode, buf, sizeof(struct dinode));
	if(temp_inode.type != ROOTINO){
    fprintf(stderr, "ERROR: root directory does not exist.\n");
    exit(1);
  }
  return temp_inode.type;
}
