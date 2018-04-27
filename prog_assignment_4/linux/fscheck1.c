#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#define stat xv6_stat
#include "types.h"
#include "fscheck.h"
#include "stat.h"
#include "buf.h"

// Function prototypes
int checkRootDirectory();
int validDataBlockCheck(struct dinode temp_inode);
int getInodenumInDirectoryGivenFilename(uint addr, char *name);
int checkbitmap(struct dinode temp_inode);
int getDirdirentryInumGivenCurrentNode(struct dinode temp_inode, char *name);
int getInodenumInDirectoryGivenFilename_by_inum(uint addr, ushort inum);
int checkChildPointsBackToParent(struct dinode parent, uint parent_inum);
int checkIfParentReferencesChild(int inum, int parent_inum);
int bitMapChecker(uint* addresses);
int checkAddressIsUsedOnlyOnce(uint* addresses, struct dinode temp_inode);
int checkIfInodeIsInUSeByDirectories(struct dinode temp_inode, uint current_inum);
int checkInodeIsMarkedInuseInTable(struct dinode temp_inode);
int checkLinksToInode(struct dinode temp_inode, uint current_inum);
void rsect(uint sec, void *buf);

int fdescriptor;
struct superblock superblk;
void *imgfileptr;


int main(int argc, char *argv[]){
	uchar buf[BSIZE];
	int inum, confirmation = 0, t;
	struct dinode temp_inode;

  if(argc < 2) {
    fprintf(stderr, "Usage: %s file_system.img\n", argv[0]);
    exit(1);
  }

	fdescriptor = open(argv[1], O_RDONLY);       // Lets read the file image

  if(fdescriptor < 0){
    fprintf(stderr, "image not found.\n");
    exit(1);
  }

	rsect(SUPERBLOCK,buf);
	memmove(&superblk, buf, sizeof(superblk));

	uint addresses[superblk.size];
	for(t = 0; t < superblk.size; t++){
		addresses[t] = 0;
	}

	for(inum = 0; inum < ((int) superblk.ninodes); inum++){
		if(lseek(fdescriptor, superblk.inodestart * BSIZE + inum * sizeof(struct dinode),
			SEEK_SET) != superblk.inodestart * BSIZE + inum * sizeof(struct dinode)){
			perror("lseek failed to move pointer");
    	exit(1);
		}
		if(read(fdescriptor, buf, sizeof(struct dinode))!=sizeof(struct dinode)){
			perror("read failed");
  		exit(1);
		}
		memmove(&temp_inode, buf, sizeof(temp_inode));

		if(temp_inode.type != 0 && temp_inode.type != T_FILE && temp_inode.type !=
			T_DIR && temp_inode.type != T_DEV){
			close(fdescriptor);
			printf("ERROR: bad inode\n");
			exit(1);
		}else if(temp_inode.type != 0){
			confirmation = validDataBlockCheck(temp_inode);
			if(confirmation == 1){
				close(fdescriptor);
				printf("ERROR: bad address in inode\n");
				exit(1);
			}

			if(checkbitmap(temp_inode) == 1){
				close(fdescriptor);
				printf("ERROR: address used by inode but marked free in bitmap\n");
				exit(1);
			}

			if(checkAddressIsUsedOnlyOnce(addresses, temp_inode) == 1){
				close(fdescriptor);
				printf("ERROR: address used more than once\n");
				exit(1);
			}

			if(checkIfInodeIsInUSeByDirectories(temp_inode, inum) == 1){
				close(fdescriptor);
				printf("ERROR: inode marked use but not in directory\n");
				exit(1);
			}
		}

		if(temp_inode.type == T_DIR){
			int inum_singledot, inum_doubledot;
			inum_singledot = getDirdirentryInumGivenCurrentNode(temp_inode,".");
			inum_doubledot = getDirdirentryInumGivenCurrentNode(temp_inode,"..");
			if(inum_singledot == -1 || inum_doubledot == -1){
				close(fdescriptor);
				printf("ERROR: directory not properly formatted\n");
				exit(1);
			}

			if(checkInodeIsMarkedInuseInTable(temp_inode) == 1){
				close(fdescriptor);
				printf("ERROR: inode referred to in directory but marked free\n");
				exit(1);
			}

			if(inum != 1){
				int parentcheck = checkIfParentReferencesChild(inum, inum_doubledot);
				if(parentcheck == 1){
					close(fdescriptor);
					printf("ERROR: parent directory mismatch\n");
					exit(1);
				}

				int childcheck = checkChildPointsBackToParent(temp_inode,inum);
				if(childcheck == 1){
					close(fdescriptor);
					printf("ERROR: parent directory mismatch\n");
					exit(1);
				}
			}
		}

		if(temp_inode.type == T_FILE){
			if(temp_inode.nlink != checkLinksToInode(temp_inode, inum)){
				close(fdescriptor);
				printf("ERROR: bad reference count for file\n");
				exit(1);
			}
		}
	}

	if(bitMapChecker(addresses) == 1){
		close(fdescriptor);
		printf("ERROR: bitmap marks block in use but it is not in use\n");
		exit(1);
	}

	if(checkRootDirectory() == 1){
		close(fdescriptor);
		printf("ERROR: root directory does not exist\n");
		exit(1);
	}

  close(fdescriptor);

  return 0;
}


// Function implementations
int checkRootDirectory(){
	if(lseek(fdescriptor, superblk.inodestart * BSIZE + sizeof(struct dinode),
    SEEK_SET)!= superblk.inodestart * BSIZE + sizeof(struct dinode)){
		perror("lseek fails to move pointer");
		exit(1);
	}
	uchar buf[sizeof(struct dinode)];
	struct dinode temp_inode;
	read(fdescriptor, buf, sizeof(struct dinode));
	memmove(&temp_inode, buf, sizeof(struct dinode));
	if (temp_inode.type==1){return 0;}
	return 1;
}


int validDataBlockCheck(struct dinode temp_inode){
	uint blcknumberend = superblk.size-1;
	uint end_bmap=superblk.bmapstart + superblk.size / (8 * BSIZE);
	if(superblk.size%(8*BSIZE) !=0) {
		end_bmap++;
	}
	for(int i=0; i < NDIRECT+1; i++){
		if(temp_inode.addrs[i]!=0 && (temp_inode.addrs[i]<end_bmap || temp_inode.addrs[i]>blcknumberend ) ){
			return 1;
		}
	}
	if(temp_inode.addrs[NDIRECT] != 0){
		if (lseek(fdescriptor, temp_inode.addrs[NDIRECT] * BSIZE, SEEK_SET)!= temp_inode.addrs[NDIRECT] * BSIZE){
			perror("lseek failed");
			exit(1);
		}
		uint buf;
		for(int x=0; x<NINDIRECT; x++){
			if (read(fdescriptor, &buf, sizeof(uint))!= sizeof(uint)){
				perror("read failed");
				exit(1);
			}
			if(buf!=0 && (buf<end_bmap || buf>blcknumberend)){
				return 1;
			}
		}
	}
	return 0;
}

int getInodenumInDirectoryGivenFilename(uint addr, char *name){
	if(lseek(fdescriptor, addr*BSIZE, SEEK_SET) != addr*BSIZE){
		perror("lseek fails to move pointer");
		exit(1);
	}
	struct dirent buf;
	for(int i=0; i<DPB; i++){
		if(read(fdescriptor, &buf, sizeof(struct dirent))!=sizeof(struct dirent)){
			perror("read failed");
			exit(1);
    }
		if(0 == buf.inum){
      continue;
    }
		if(strncmp(name, buf.name, DIRSIZ) == 0){
			return buf.inum;
		}
	}
	return -1;
}

int getDirdirentryInumGivenCurrentNode(struct dinode temp_inode, char *name){
  int response = -1;
	for(int i=0; i < NDIRECT; i++){
    if(temp_inode.addrs[i]==0){
      continue;
		}
    response = getInodenumInDirectoryGivenFilename(temp_inode.addrs[i], name);
    if(response != -1){
      return response;
    }
	}

	if(temp_inode.addrs[NDIRECT] != 0){
    if(lseek(fdescriptor, temp_inode.addrs[NDIRECT] * BSIZE, SEEK_SET)!=
      temp_inode.addrs[NDIRECT] * BSIZE){
			perror("lseek fails to move pointer");
      exit(1);
		}
    //Lets go through indirect imgfileptrs
    uint buf2;
    for(int x=0; x<NINDIRECT; x++){
      if(lseek(fdescriptor, temp_inode.addrs[NDIRECT] * BSIZE + x * sizeof(uint),
        SEEK_SET) != temp_inode.addrs[NDIRECT] * BSIZE + x * sizeof(uint)){
        perror("lseek fails to move pointer");
        exit(1);
      }
      if(read(fdescriptor, &buf2, sizeof(uint))!=sizeof(uint)){
        perror("read fails");
        exit(1);
			}
      response = getInodenumInDirectoryGivenFilename(buf2, name);
      if(response != -1){
        return response;
      }
    }
  }
	return response;
}

int getInodenumInDirectoryGivenFilename_by_inum(uint addr, ushort inum){
	if(lseek(fdescriptor, addr*BSIZE, SEEK_SET) != addr*BSIZE){
		perror("lseek");
		exit(1);
  }
	struct dirent buf;
	for(int i=0; i<DPB; i++){
		read(fdescriptor, &buf, sizeof(struct dirent));
		if(buf.inum==inum){
			return 0;
		}
	}
	return 1;
}

int checkbitmap(struct dinode temp_inode){
	int i;
	uint buf, buf2;
	for (i=0; i < NDIRECT+1; i++){
		if(temp_inode.addrs[i]==0){
			continue;
		}
		if(lseek(fdescriptor, superblk.bmapstart*BSIZE + temp_inode.addrs[i]/8,
			SEEK_SET) != superblk.bmapstart*BSIZE + temp_inode.addrs[i]/8 ){
			perror("lseek failed");
			exit(1);
		}
		if(read(fdescriptor, &buf2, 1) != 1){
			perror("read failed");
			exit(1);
		}
		int amount_shifted = temp_inode.addrs[i]%8;
		buf2=buf2 >> amount_shifted;
		buf2=buf2%2;
		if(buf2==0){
			return 1;
		}
	}
	if(temp_inode.addrs[NDIRECT] != 0){
		for(int x=0; x<NINDIRECT; x++){
			if(lseek(fdescriptor, temp_inode.addrs[NDIRECT] * BSIZE + x*sizeof(uint),
				SEEK_SET) != temp_inode.addrs[NDIRECT] * BSIZE + x*sizeof(uint)){
				perror("lseek failed");
				exit(1);
			}
			if(read(fdescriptor, &buf, sizeof(uint)) != sizeof(uint)){
				perror("read failed");
				exit(1);
			}
			if(buf!=0){
				if(lseek(fdescriptor, superblk.bmapstart*BSIZE + buf/8,
					SEEK_SET) != superblk.bmapstart*BSIZE + buf/8){
					perror("lseek failed");
					exit(1);
				}
				if(read(fdescriptor, &buf2, 1) != 1){
					perror("read failed");
					exit(1);
				}
				int amount_shifted = temp_inode.addrs[i]%8;
				buf2 = buf2 >> amount_shifted;
				buf2 = buf2%2;
				if(buf2==0){
					return 1;
				}
			}
		}
	}
	return 0;
}

int checkChildPointsBackToParent(struct dinode parent, uint parent_inum){
	struct dirent child_direntry;
	struct dinode child_inode;
	for(int i = 0; i < NDIRECT; i++){
    if(parent.addrs[i]==0){
      continue;
    }
		for(int j=0; j<DPB; j++){
      if(lseek(fdescriptor, parent.addrs[i] * BSIZE + j*sizeof(struct dirent),
      SEEK_SET) !=  parent.addrs[i] * BSIZE + j*sizeof(struct dirent)){
        perror("lseek failed");
        exit(1);
			}
			if(read(fdescriptor, &child_direntry, sizeof(struct dirent)) != sizeof(struct dirent)){
        perror("read failed");
        exit(1);
      }
			if(0 == child_direntry.inum){
				continue;
			}
			if(strncmp(child_direntry.name,".",DIRSIZ) == 0 && child_direntry.inum != parent_inum){
				printf("the '.' direntry doesnt match parent  \n");
        exit(1);
			}
			if(strncmp(child_direntry.name, ".", DIRSIZ)==0){
				continue;
			}

			if(strncmp(child_direntry.name,"..",DIRSIZ)==0){
				if((child_direntry.inum !=1 && child_direntry.inum == parent_inum)){
					return 1;
				}
				continue;
			}

			if(lseek(fdescriptor, superblk.inodestart*BSIZE + child_direntry.inum * sizeof(struct dinode),
				SEEK_SET) !=  superblk.inodestart*BSIZE + child_direntry.inum * sizeof(struct dinode)){
					perror("lseek pointer error");
					exit(1);
			}
			if(read(fdescriptor, &child_inode, sizeof(struct dinode)) != 	sizeof(struct dinode)){
				perror("read failed");
				exit(1);
			}
			if(child_inode.type != T_DIR){
				continue;
			}

			for(int x=0; x < NDIRECT; x++){
				struct dirent direntry;
				if(child_inode.addrs[x] != 0){
					int k;
					for(k=0; k<DPB; k++){
						if(lseek(fdescriptor, child_inode.addrs[x] * BSIZE + k*sizeof(struct dirent),
						 SEEK_SET) != child_inode.addrs[x] * BSIZE + k*sizeof(struct dirent)){
							perror("lseek failed to move pointer");
							exit(1);
						}
						if(read(fdescriptor, &direntry, sizeof(struct dirent)) != sizeof(struct dirent)){
							perror("directory read failed");
							exit(1);
						}
						if(strncmp(direntry.name,"..",DIRSIZ) == 0 && direntry.inum != parent_inum){
							printf("the parent inum DOESNT match the '..' direntry in the inum\n");
							return 1;
						}
					}
				}
			}

			if(child_inode.addrs[NDIRECT] !=0 ){
				if(lseek(fdescriptor, child_inode.addrs[NDIRECT] * BSIZE, SEEK_SET) !=
					child_inode.addrs[NDIRECT] * BSIZE){
						perror("lseek failed to move pointer");
						exit(1);
				}
				uint imgfileptr;
				for(int y=0; y<NINDIRECT; y++){
					if(lseek(fdescriptor, child_inode.addrs[NDIRECT] * BSIZE + y * sizeof(uint),
						SEEK_SET) != child_inode.addrs[NDIRECT] * BSIZE + y * sizeof(uint)){
						perror("lseek failed to move pointer");
						exit(1);
					}
					if (read(fdescriptor, &imgfileptr, sizeof(uint)) != sizeof(uint)){
						perror("read failed");
						exit(1);
					}
					struct dirent i_direntry;
					for(int m=0; m<DPB; m++){
						if(lseek(fdescriptor, imgfileptr*BSIZE + sizeof(struct dirent)*m,
							SEEK_SET) !=  imgfileptr*BSIZE + sizeof(struct dirent)*m){
							perror("lseek failed");
							exit(1);
						}
						if (read(fdescriptor, &i_direntry, sizeof(struct dirent)) != sizeof(struct dirent)){
							perror("read failed");
							exit(1);
						}
						if(strncmp(i_direntry.name,"..",DIRSIZ)==0 && i_direntry.inum != parent_inum){
							return 1;
						}
					}
				}
			}
		}
		if(parent.addrs[NDIRECT] !=0){
			uint directory_address;
			for(int z=0; z<NINDIRECT; z++){
				if(lseek(fdescriptor, parent.addrs[NDIRECT]*BSIZE + z*sizeof(uint),
					SEEK_SET) != parent.addrs[NDIRECT]*BSIZE + z*sizeof(uint)){
					perror("lseek failed");
					exit(1);
				}
				if(read(fdescriptor, &directory_address, sizeof(uint)) != sizeof(uint)){
					perror("read failed");
					exit(1);
				}
				if(directory_address==0){
					continue;
				}

				for(int j=0; j<DPB; j++){
					if(lseek(fdescriptor, directory_address * BSIZE + j*sizeof(struct dirent),
						SEEK_SET)  != directory_address * BSIZE + j*sizeof(struct dirent)){
						perror("lseek failed");
						exit(1);
					}
					if (read(fdescriptor, &child_direntry, sizeof(struct dirent)) != sizeof(struct dirent)){																		 //read dir direntry
						perror("read failed");
						exit(1);
					}

					if(0 == child_direntry.inum){
						continue;
					}

					if(strncmp(child_direntry.name,".",DIRSIZ)==0 && child_direntry.inum!=parent_inum){
						return 1;
					}
					if(strncmp(child_direntry.name, ".", DIRSIZ)==0){
						continue;
					}
					if(strncmp(child_direntry.name,"..",DIRSIZ)==0){
						if((child_direntry.inum !=1 && child_direntry.inum == parent_inum)){
							return 1;
						}
						continue;
					}

					if(lseek(fdescriptor, superblk.inodestart*BSIZE + child_direntry.inum * sizeof(struct dinode),
						SEEK_SET) != superblk.inodestart*BSIZE + child_direntry.inum * sizeof(struct dinode)){
						perror("lseek failed");
						exit(1);
					}
					if(read(fdescriptor, &child_inode, sizeof(struct dinode)) != sizeof(struct dinode)){															//read child inode
						perror("read failed");
						exit(1);
					}
					if(child_inode.type!=T_DIR){
						continue;
					}

					for(int x=0; x < NDIRECT; x++){
						struct dirent direntry;
						if (child_inode.addrs[x] != 0){
							for(int k=0; k<DPB; k++){
								if(lseek(fdescriptor, child_inode.addrs[x] * BSIZE + k*sizeof(struct dirent),
									SEEK_SET) != child_inode.addrs[x] * BSIZE + k*sizeof(struct dirent)){
									perror("lseek failed");
									exit(1);
								}
								if(read(fdescriptor, &direntry, sizeof(struct dirent)) != sizeof(struct dirent)){																		 //read dir direntry
									perror("read failed");
									exit(1);
								}

								if(strncmp(direntry.name,"..",DIRSIZ)==0 && direntry.inum != parent_inum){
									return 1;
								}
							}
						}
					}

					if(child_inode.addrs[NDIRECT] !=0 ){
						if(lseek(fdescriptor, child_inode.addrs[NDIRECT] * BSIZE, SEEK_SET) !=
							child_inode.addrs[NDIRECT] * BSIZE){
							perror("lseek failed");
							exit(1);
						}
						uint imgfileptr;
						for(int y=0; y<NINDIRECT; y++){
							if(lseek(fdescriptor, child_inode.addrs[NDIRECT] * BSIZE + y * sizeof(uint),
								SEEK_SET) != child_inode.addrs[NDIRECT] * BSIZE + y * sizeof(uint)){
								perror("lseek failed");
								exit(1);
							}
							if(read(fdescriptor, &imgfileptr, sizeof(uint)) != sizeof(uint)){
								exit(1);
							}
							struct dirent i_direntry;
							for(int m=0; m<DPB; m++){
								if(lseek(fdescriptor, imgfileptr*BSIZE + sizeof(struct dirent)*m,
									SEEK_SET) != imgfileptr*BSIZE + sizeof(struct dirent)*m){
									perror("lseek failed");
									exit(1);
								}
								if(read(fdescriptor, &i_direntry, sizeof(struct dirent)) != sizeof(struct dirent)){
									perror("read failed");
									exit(1);
								}
								if(strncmp(i_direntry.name,"..",DIRSIZ)==0 && i_direntry.inum != parent_inum){
									return 1;
								}
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

int checkIfParentReferencesChild(int inum, int parent_inum){
	struct dinode parent;
	int parent_inode_location=superblk.inodestart*BSIZE + parent_inum * sizeof(struct dinode);
	if(lseek(fdescriptor, parent_inode_location, SEEK_SET) != parent_inode_location){
    perror("lseek failed to move pointer");
		exit(1);
  }
	if (read(fdescriptor, &parent, sizeof(struct dinode)) != sizeof(struct dinode)){
		perror("read failed");
		exit(1);
  }
	int response=1;
	for (int i=0; i < NDIRECT; i++){
    if(parent.addrs[i]==0){continue;}
    response = getInodenumInDirectoryGivenFilename_by_inum(parent.addrs[i], (ushort) inum);
		  if(response == 0) {return 0;}
	}
	if(parent.addrs[NDIRECT] != 0){
    if(lseek(fdescriptor, parent.addrs[NDIRECT] * BSIZE, SEEK_SET)!= parent.addrs[NDIRECT]*BSIZE){
      perror("lseek");
			exit(1);
		}
		uint buf2;
    for(int x=0; x<NINDIRECT; x++){
			if (lseek(fdescriptor, parent.addrs[NDIRECT] * BSIZE + x * sizeof(uint),
				SEEK_SET) != parent.addrs[NDIRECT] * BSIZE + x * sizeof(uint)){
				perror("lseek failed");
				exit(1);
			}
			if(read(fdescriptor, &buf2, sizeof(uint)) != sizeof(uint)){
				perror("read failed");
				exit(1);
			}
			if(0 == buf2){continue;}
			response = getInodenumInDirectoryGivenFilename_by_inum(buf2,(ushort) inum);
			if(response == 0){
        return 0;
      }
		}
	}
	return response;
}

int checkAddressIsUsedOnlyOnce(uint* addresses, struct dinode temp_inode){
	for(int i=0;i<NDIRECT+1;i++){
		if(temp_inode.addrs[i] == 0){
			continue;
		}
		if(addresses[temp_inode.addrs[i]] == 1){
			return 1;
		}
		addresses[temp_inode.addrs[i]]=1;
	}

	uint address;
	if(temp_inode.addrs[NDIRECT] != 0){
		for(int j=0; j<NINDIRECT; j++){
			if(lseek(fdescriptor, temp_inode.addrs[NDIRECT] * BSIZE + j*sizeof(uint),
				SEEK_SET) != temp_inode.addrs[NDIRECT] * BSIZE + j*sizeof(uint)){
				perror("lseek failed");
				exit(1);
			}
			if(read(fdescriptor, &address, sizeof(uint)) != sizeof(uint)){
				perror("read failed");
				exit(1);
			}
			if(address==0){
				continue;
			}
			if(addresses[address] == 1){return 1;}
			addresses[address] = 1;
		}
	}
	return 0;
}

int bitMapChecker(uint* addresses){
	int start=superblk.bmapstart*BSIZE + superblk.size/8 - superblk.nblocks/8;
	uint bit;
	int count=(superblk.bmapstart + 1);

	if (lseek(fdescriptor, start, SEEK_SET) != start){
		perror("lseek failed");
		exit(1);
	}

	int byte;
	for(int i=count; i<superblk.size; i+=8){
		read(fdescriptor, &byte, 1);
		for (int x=0; x<8; x++, count++){
			bit= (byte >> x)%2;
			if (bit!=0){
				if(addresses[count]==0){
					return 1;
				}
			}
		}
	}
	return 0;
}

int checkIfInodeIsInUSeByDirectories(struct dinode temp_inode, uint current_inum){
	struct dinode in;
	for(int inum=0; inum<superblk.ninodes; inum++){
		if(inum==current_inum  && inum!=1){
			continue;
		}
		if(lseek(fdescriptor, superblk.inodestart*BSIZE + inum * sizeof(struct dinode),
			SEEK_SET) != superblk.inodestart*BSIZE + inum * sizeof(struct dinode)){
			perror("lseek failed");
			exit(1);
		}
		if(read(fdescriptor, &in, sizeof(struct dinode)) != sizeof(struct dinode)){
			perror("read failed");
			exit(1);
		}
		if(in.type!=T_DIR){
			continue;
		}
		for(int x=0; x<NDIRECT; x++){
			if(in.addrs[x]==0){
				continue;
			}
			if(0==getInodenumInDirectoryGivenFilename_by_inum(in.addrs[x], current_inum)){
				return 0;
			}
		}
		uint directory_address;
		if (in.addrs[NDIRECT]!=0){
			for(int y=0; y<NINDIRECT; y++){
				if(lseek(fdescriptor, in.addrs[NDIRECT] * BSIZE + y*sizeof(uint),
					SEEK_SET) != in.addrs[NDIRECT] * BSIZE + y*sizeof(uint)){
					perror("lseek failed");
					exit(1);
				}
				if(read(fdescriptor, &directory_address, sizeof(uint)) != sizeof(uint)){
					perror("read failed");
					exit(1);
				}
				if(0 == directory_address){
					continue;
				}
				if(0 == getInodenumInDirectoryGivenFilename_by_inum(directory_address, current_inum)){
					return 0;
				}
			}
		}
	}
	return 1;
}

int checkLinksToInode(struct dinode temp_inode, uint current_inum){
	int inum, count=0;
	struct dinode in;
	for(inum=0; inum<superblk.ninodes; inum++){
		if(inum==current_inum  && inum!=1){
			continue;
		}
		if(lseek(fdescriptor, superblk.inodestart*BSIZE + inum * sizeof(struct dinode),
			SEEK_SET) != superblk.inodestart*BSIZE + inum * sizeof(struct dinode)){
			perror("lseek failed");
			exit(1);
		}
		if(read(fdescriptor, &in, sizeof(struct dinode)) != sizeof(struct dinode)){
			perror("read failed");
			exit(1);
		}
		if(in.type != T_DIR){
			continue;
		}
		for(int x=0; x<NDIRECT; x++){
			if(in.addrs[x]==0){
				continue;
			}
			if(0 == getInodenumInDirectoryGivenFilename_by_inum(in.addrs[x], current_inum)){
				count++;
			}
		}
		uint directory_address;
		if (in.addrs[NDIRECT]!=0){
			for(int y=0; y<NINDIRECT; y++){
				if (lseek(fdescriptor, in.addrs[NDIRECT] * BSIZE + y*sizeof(uint),
					SEEK_SET) != in.addrs[NDIRECT] * BSIZE + y*sizeof(uint)){
					perror("lseek failed");
					exit(1);
				}
				if (read(fdescriptor, &directory_address, sizeof(uint)) != sizeof(uint)){
					perror("read failed");
					exit(1);
				}
				if(0 == directory_address){
					continue;
				}
				if(0 == getInodenumInDirectoryGivenFilename_by_inum(directory_address, current_inum)){
					count++;
				}
			}
		}
	}
	return count;
}

int checkInodeIsMarkedInuseInTable(struct dinode temp_inode){
	struct dirent direntry;
	struct dinode temp;
	uint addr;
	for(int i=0; i< NDIRECT; i++){
		if(temp_inode.addrs[i]==0){continue;}
		for (int x=0; x<DPB; x++){
			if(lseek(fdescriptor, temp_inode.addrs[i] * BSIZE + x*sizeof(struct dirent),
				SEEK_SET) != temp_inode.addrs[i] * BSIZE + x*sizeof(struct dirent)){
				perror("lseek failed");
				exit(1);
			}
			if(read(fdescriptor, &direntry, sizeof(struct dirent)) != sizeof(struct dirent)){
				perror("read failed");
				exit(1);
			}
			if(direntry.inum==0){
				continue;
			}
			if(lseek(fdescriptor, superblk.inodestart*BSIZE + direntry.inum * sizeof(struct dinode),
				SEEK_SET) != superblk.inodestart*BSIZE + direntry.inum * sizeof(struct dinode)){
				perror("lseek failed");
				exit(1);
			}
			if(read(fdescriptor, &temp, sizeof(struct dinode)) != sizeof(struct dinode)){
				perror("read failed");
				exit(1);
			}
			if(temp.type==0){
				return 1;
			}
		}
	}

	if(temp_inode.addrs[NDIRECT]!=0){
		for(int y=0; y<NINDIRECT; y++){
			if (lseek(fdescriptor, temp_inode.addrs[NDIRECT] * BSIZE + y*sizeof(uint),
				SEEK_SET) != temp_inode.addrs[NDIRECT] * BSIZE + y*sizeof(uint)){
				perror("lseek failed");
				exit(1);
			}
			if(read(fdescriptor, &addr, sizeof(uint)) != sizeof(uint)){
				perror("read failed");
				exit(1);
			}
			for(int z=0; z<DPB; z++){
				if(lseek(fdescriptor, addr*BSIZE + z*sizeof(struct dirent),
					SEEK_SET) !=addr*BSIZE + z*sizeof(struct dirent)){
					perror("lseek failed");
					exit(1);
				}
				if(read(fdescriptor, &direntry, sizeof(struct dirent)) != sizeof(struct dirent)){
					perror("read failed");
					exit(1);
				}
				if(direntry.inum==0){
					continue;
				}
				if (lseek(fdescriptor, superblk.inodestart*BSIZE + direntry.inum * sizeof(struct dinode),
					SEEK_SET) !=  superblk.inodestart*BSIZE + direntry.inum * sizeof(struct dinode)){
					perror("lseek failed to move pointer");
					exit(1);
				}
				if(read(fdescriptor, &temp, sizeof(struct dinode)) != sizeof(struct dinode)){
					perror("read failed");
					exit(1);
				}
				if(temp.type==0){
					return 1;
				}
			}
		}
	}
	return 0;
}

void rsect(uint sec, void *buf){
  if(lseek(fdescriptor, sec * BSIZE, 0) != sec * BSIZE){
    perror("lseek failed");
    exit(1);
  }
  if(read(fdescriptor, buf, BSIZE) != BSIZE){
    perror("read failed");
    exit(1);
  }
}
