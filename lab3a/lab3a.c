// Project 3A 
// 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/errno.h>
#include <unistd.h>

#include "ext2_fs.h"

int fd;
unsigned int inode_num;  // Num of Inode
unsigned int group_num;  // Num of Group
unsigned int block_num;  // Num of Block
unsigned int block_size;

struct ext2_super_block superblock;
struct ext2_group_desc groups;

void sum_superblock()
{
    if (pread(fd, &superblock, sizeof(struct ext2_super_block), 1024) < 0)
    {
        fprintf(stderr, "error reading superblock. \n");
        exit(2);
    }
    inode_num = superblock.s_inodes_count;
    group_num = 1;
    block_num = superblock.s_blocks_count;
    block_size = 1024 << superblock.s_log_block_size;
    printf("SUPERBLOCK,");
    printf("%d,%d,%d,%hd,%d,%d,%d\n", superblock.s_blocks_count, superblock.s_inodes_count, 1024 << superblock.s_log_block_size, superblock.s_inode_size, superblock.s_blocks_per_group, superblock.s_inodes_per_group,superblock.s_first_ino);
}

void sum_group()
{
    if (pread(fd, &groups, 32, 2048) < 0)
    {
        fprintf(stderr, "error reading group. \n");
        exit(2);
    }
    printf("GROUP,");
    printf("%d,", 0);
    printf("%d,%d,%hd,%hd,%d,%d,%d\n", superblock.s_blocks_count, superblock.s_inodes_count, groups.bg_free_blocks_count, groups.bg_free_inodes_count, groups.bg_block_bitmap, groups.bg_inode_bitmap, groups.bg_inode_table);
}

void sum_freeblock()
{
    int bBitmap_size = 1024;
    unsigned char* bBitmap_buffer = malloc(bBitmap_size);
    if (bBitmap_buffer == NULL)
    {
        fprintf(stderr, "error allocating bitmap buffer. \n");
        exit(2);
    }
    int bBitmap_offset = (1024 << superblock.s_log_block_size) * groups.bg_block_bitmap;
    if (pread(fd, bBitmap_buffer, bBitmap_size, bBitmap_offset) < 0)
    {
        fprintf(stderr, "error reading block bitmap. \n");
        exit(2);
    }
    int i;
    int j;
    for (i = 0; i < bBitmap_size; i++)
    {
        char c = bBitmap_buffer[i];
        for (j = 0; j < 8; j++)
        {
            int aixinnan = (1 << j);
            if ((aixinnan & c) == 0)
            {
                printf("BFREE,");
                printf("%d\n", i*8+j+1);
            }
        }
    }
}

void sum_freeinode()
{
    int iBitmap_size = 1024;
    unsigned char* iBitmap_buffer = malloc(iBitmap_size);
    if (iBitmap_buffer == NULL)
    {
        fprintf(stderr, "error allocating bitmap buffer. \n");
        exit(2);
    }
    int iBitmap_offset = (1024 << superblock.s_log_block_size) * groups.bg_inode_bitmap;
    if (pread(fd, iBitmap_buffer, iBitmap_size, iBitmap_offset) < 0)
    {
        fprintf(stderr, "error reading inode bitmap. \n");
        exit(2);
    }
    //int allocated[superblock.s_inodes_per_group];
    int i;
    int j;
    for(i = 0; i < iBitmap_size; i++)
    {
        char c = iBitmap_buffer[i];
        for (j = 0; j < 8; j++)
        {
            int num_inodes = i * 8 + j + 1;
            if (num_inodes <= ((int)superblock.s_inodes_per_group))
            {
                int aixinnan = (1 << j);
                if (!(aixinnan & c))
                {
                    printf("IFREE,");
                    printf("%d\n", num_inodes);
                    //allocated[num_inodes-1] = 0;
                }
                else
                {
                    //allocated[num_inodes-1] = 1;
                }
            }
        }
    }
}


// ======= Directory Entries =======
void dirent_summary(struct ext2_inode *inode, int inum) {
	int max_idx = inode->i_blocks / (2 << superblock.s_log_block_size);
	int i;
	int b_offset = 0;
	char dir_buffer[block_size];
	for (i=0; i<max_idx; ++i) {
		if (pread(fd, dir_buffer, block_size, 1024+block_size*(inode->i_block[i]-1)) < 0) {
			fprintf(stderr, "%s\n", "Pread Directory Error");
			exit(2);
		}
		struct ext2_dir_entry *dir_ptr = (struct ext2_dir_entry *) dir_buffer;
        
		while(b_offset < (int) block_size) {
			char fname[dir_ptr->name_len+1];
			strcpy(fname, dir_ptr->name);
			fname[dir_ptr->name_len] = '\0';
            if (dir_ptr != NULL && dir_ptr->inode != 0 && dir_ptr->rec_len != 0) {
			fprintf(stdout, "DIRENT,%d,%u,%u,%u,%u,'%s'\n", 
				    inum, b_offset, dir_ptr->inode, 
				    dir_ptr->rec_len, dir_ptr->name_len, fname);
            
			b_offset += dir_ptr->rec_len;
			dir_ptr = (void *)dir_ptr + dir_ptr->rec_len; 
            }
		}
	}
}



// ======= Indirect Block References =======
int init_count = 0;


void indir_block_ref_summary(struct ext2_inode *inode, int inum, int block_1, int block_2, int block_3, int offset) {
	__u8 *block_buf;
	int count;
	int i;
	block_buf = malloc(block_size);
	if (!block_buf) {
		fprintf(stderr, "%s\n", "Indir Block Ref Malloc Error");
		exit(2);
	}
	if (block_1) {
		if (pread(fd, block_buf, block_size, block_size * block_1) < 0) {
			fprintf(stderr, "%s\n", "1st Indir Block Pread Error");
			exit(2);
		}
		count = 12 + init_count;
		if (offset) {
			count = offset;
			count += init_count - 1;
		}
		for (i=0; i< (int) block_size; ++i) {
			if (block_buf[i] != 0) {
				fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", 
					    inum, 1, count, block_1, block_buf[i]);
				count++;
				if (!offset) init_count++;
			}
		}
	}

	if (block_2) {
		if (pread(fd, block_buf, block_size, block_size * block_2) < 0) {
			fprintf(stderr, "%s\n", "2nd Indir Block Pread Error");
			exit(2);
		}
		count = 268;
		if (offset) {
			count = offset;
		}
		for (i=0; i<(int) block_size; ++i) {
			if (block_buf[i] != 0) {
				fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", 
					    inum, 2, count, block_2, block_buf[i]);
				indir_block_ref_summary(inode, inum, block_buf[i], 0, 0, count);
				count += 256;
			}
		}
	}

	if (block_3) {
		if (pread(fd, block_buf, block_size, block_size * block_3) < 0) {
			fprintf(stderr, "%s\n", "3rd Indir Blcok Pread Error");
			exit(2);
		}
		count = 65804;
		if (offset) {
			count = offset;
		}
		for (i=0; i<(int) block_size; ++i) {
			if (block_buf[i] != 0) {
				fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", 
					    inum, 3, count, block_3, block_buf[i]);
				indir_block_ref_summary(inode, inum, 0, block_buf[i], 0, count);
				count += 65536;
			}
		}
	}
	free(block_buf);
}


// ======= I-node Summary =======
void inode_summary() {
    int j;
    char ftype='?';

    // int inode_per_group = superblock.s_inodes_per_group;
    //int inode_size = superblock.s_inode_size;
    // int inode_size = (int) sizeof(struct ext2_inode);
    struct ext2_inode inode;
    // if (!inodes) {
    // 	fprintf(stderr, "%s\n", "Inode Malloc Error");
    // 	exit(2);
    // }
    // for (i=0; i<(int) group_num; ++i) { 
    //     if (pread(fd, inodes, inode_num * inode_size, block_size * groups.bg_inode_table < 0) {
    //     	fprintf(stderr, "%s\n", "Pread Error");     
    //     }
    for (j=0; j<(int) inode_num; ++j) {
    	//struct ext2_inode *inode = &inodes[j];
        pread(fd, &inode, sizeof(struct ext2_inode), block_size*groups.bg_inode_table+j*sizeof(struct ext2_inode));
    	if (inode.i_mode != 0 && inode.i_links_count != 0) {
    		if (inode.i_mode & 0x8000) {
    			if (inode.i_mode & 0x2000) {
    				ftype = 's';
    			}
    			else {ftype = 'f';}
    		}
    		else if (inode.i_mode & 0x4000) {
    			ftype = 'd';
    		}
        	char ctime[32]; //create
        	char mtime[32]; //modify
        	char atime[32];	//access
        	struct tm ts;
        	time_t c_t = inode.i_ctime;
        	time_t m_t = inode.i_mtime;
        	time_t a_t = inode.i_atime;

        	ts = *gmtime(&c_t);
        	strftime(ctime, sizeof(ctime), "%D %T", &ts);
        	ts = *gmtime(&m_t);
        	strftime(mtime, sizeof(mtime), "%D %T", &ts);
        	ts = *gmtime(&a_t);
        	strftime(atime, sizeof(atime), "%D %T", &ts);

        	fprintf(stdout, "INODE,%d,%c,%o,%d,%u,%u,%s,%s,%s,%u,%u", j+1, 
        		    ftype, (inode.i_mode) & 0x0FFF, inode.i_uid, inode.i_gid, 
        		    inode.i_links_count, ctime, mtime, atime, 
        		    inode.i_size, inode.i_blocks);
        	if (!(ftype == 's' && inode.i_size <= 60)) {
        		fprintf(stdout, ",%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", 
        			    inode.i_block[0], inode.i_block[1], inode.i_block[2], 
        			    inode.i_block[3], inode.i_block[4], inode.i_block[5], 
        			    inode.i_block[6], inode.i_block[7], inode.i_block[8], 
        			    inode.i_block[9], inode.i_block[10], inode.i_block[11], 
        			    inode.i_block[12], inode.i_block[13], inode.i_block[14]);
        	}
        	else fprintf(stdout, "\n");
        	if (ftype == 'd') {
        		dirent_summary(&inode, j+1);
        		// indir_block_ref_summary(inode, j+1, 0);
        	}
            indir_block_ref_summary(&inode, j+1, inode.i_block[12], inode.i_block[13], inode.i_block[14], 0);
        }
    	// indir_block_ref_summary(&inode, j+1, inode.i_block[12], inode.i_block[13], inode.i_block[14], 0);
    }
    //}
}



// ======= main =======
int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "%s\n", "Bad Arguments");
		exit(1);
	}
	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "%s\n", "Bad Arguments");
		exit(1);
	}
    sum_superblock();
    sum_group();
    sum_freeblock();
    sum_freeinode();
    inode_summary();
    exit(0);
}




