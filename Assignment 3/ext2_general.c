#include "ext2.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

void init_datastructures() {
	// Referenced from https://wiki.osdev.org/Ext2
	// and http://www.nongnu.org/ext2-doc/ext2.html#S-LOG-BLOCK-SIZE
	block = disk + EXT2_BLOCK_SIZE;
	sb = (struct ext2_super_block *)(block);
	block_size = EXT2_BLOCK_SIZE << (sb -> s_log_block_size);
	descriptor = (struct ext2_group_desc *)(block + block_size);

	//block number where the inode table starts
	unsigned int block_number = descriptor -> bg_inode_table;
	//Reference from the table 3-1 in nongnu.org
	inode_table = (struct ext2_inode *)(block + block_number * block_size);
}



void open_image(char * virtual_disk) {
	int fd = open(virtual_disk, O_RDWR);

	disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(disk == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}
	//return disk;
}


/*
 * Returns the inode number of the path to file
 */
int path_walk(struct ext2_inode * root, char * path) {
	if (!(root -> i_mode & EXT2_S_IFDIR)){
		fprintf(stderr, "root is not a directory\n");
		exit(1);
	}

	// Referenced from https://www.geeksforgeeks.org/how-to-split-a-string-in-cc-python-and-java/
  char *token = strtok(path, "/");
	int block_no = EXT2_ROOT_INO;
	//struct ext2_inode * curr_inode = root;

  while (token != NULL) {

		block_no = check_directory(token, block_no, &check_entry);
		if (!(block_no > 0)) {
			return block_no;
		}
		printf("%s\n", token);
    token = strtok(NULL, "/");
  }

	return block_no;
}

/*
 * Checks to see whether or not the file with file name 'name' is in the
 * current working directory
 * Returns the block_no number if found
 */
int check_directory(char * name, unsigned int block_no, int (*fun_ptr)(unsigned int, char *)){
	struct ext2_inode * inode = (struct ext2_inode *)(block + block_no * block_size);
	unsigned int * inode_block = inode->i_block;
	// Initialize all the variables needed
	int index = 0;
	unsigned int curr_block_no = inode_block[index];
	int i = 0;
	int j = 0;
	int k = 0;
	if (curr_block_no == 0) {
		fprintf(stderr, "something wrong with pathwalk, curr == 0\n");
		exit(1);
	}

	while (curr_block_no != 0) {
		(*fun_ptr)(curr_block_no, name);
		// struct ext2_dir_entry_2 * curr_block = (struct ext2_dir_entry_2 *)block + curr_block_no * block_size;
		// printf("%s\n", curr_block->name);

		if (index < 12) { //DIRECT
			// Set the next variables
			index++;
			curr_block_no = 0;

			if (index == 12) {
				printf("Now gonna check 12th entry -- singly indirects \n");
				curr_block_no = find_singly_indirect(inode_block[index], i);

			} else {
				curr_block_no = inode_block[index];
			}
		}

		else if (index == 12) { //SINGLY INDIRECT
			// Set the next variables
			i++;
			curr_block_no = 0;
			if (i == 257) { // If there are no more
				index++;
				i = 0;
				printf("Now gonna check 13th entry -- doubly indirects \n");
				curr_block_no = find_doubly_indirect(inode_block[index], i, j);
			}

			// If we still working on the same inode
			if (index == 12) {
				curr_block_no = find_singly_indirect(inode_block[index], i);
			}
		}


		else if (index == 13) { //DOUBLY INDIRECT
			i++;
			curr_block_no = 0;

			if (i == 257) {
				i = 0;
				j++;
				if (j == 257) {
					j = 0;
					index++;
					printf("Now gonna check 14th entry -- triply indirects \n");
					curr_block_no = find_triply_indirect(inode_block[index], i, j, k);
				}
			}
			// If we are still working on the same inode
			if (index == 13) {
				curr_block_no = find_doubly_indirect(inode_block[index], i, j);
			}
		}

		else if (index == 14) { //TRIPLY INDIRECT
			i++;
			curr_block_no = 0;

			if (i == 257) {
				i = 0;
				j++;
				if (j == 257) {
					j = 0;
					k++;
					if (k == 257) {
						index++;
					}
				}
			}
			if (index == 14) {
				curr_block_no = find_triply_indirect(inode_block[index], i, j, k);
			}
		}
		else {
			printf("what the heck shoulnd't be here\n");
			exit(1);
		}
	}
	return 0;
}




int print_file(unsigned int block_no, char * name) {
	struct ext2_dir_entry_2 * curr_block = (struct ext2_dir_entry_2 *)(block + block_no * block_size);
	printf("%s\n", curr_block->name);
	return 0;
}




// int check_directory(char * name, unsigned int block_no){
// 	int entry_number = 0;
// 	unsigned int * doubly_indirect;
// 	unsigned int * triply_indirect;
// 	unsigned int * singly_indirect;
// 	//Referenced from 3.5.10. i_blocks
// 	//int max_block = inode_block/(2 << block_size) - 1;
//
// 	//Check the first 12. Just direct check very easy
// 	for (int entry_number = 0; entry_number < 12; entry_number ++) {
// 		int check = check_entry(inode_block[entry_number], name);
// 		if (check != 0) {
// 			return check;
// 		}
// 	}
//
// 	// Indirect BLOCK -- entry number == 12
// 	// Is inode_block[12] a pointer to a list of block_no?
//
// 	// On the website it says that it contains the block id of the first file contained
// 	// in the indirect block
// 	// The block number of the first file contained in the indirect block
// 	unsigned int singly_indirect = block + inode_block[12] * block_size;
// 	for (int i = 0; i < 256; i++) {
//
// 		int check = check_entry(singly_indirect[i], name);
// 		if (check != 0) {
// 			return check;
// 		}
// 	}
//
// 	//Double Indirect BLOCK -- entry number == 13
// 	unsigned int * doubly_indirect = block + inode_block[13] * block_size;
// 	for (int i = 0; i < 256; i++) {
//
// 		// ????????????? need to do i * block_size ??? or no??
// 		unsigned int singly_indirect = block + doubly_indirect[i] * block_size;
// 		for (int j = 0; j < 256 < j++) {
//
// 			//SAME AS ABOVE
// 			int check = check_entry(singly_indirect[j], name);
// 			if (check != 0) {
// 				return check;
// 			}
// 		}
// 	}
//
// 	//Triple Indirect BLOCK
// 	triply_indirect = block + inode_block[14] * block_size;
// 	for (int i = 0; i < 256; i++) {
// 		doubly_indirect = block + triple_indirect[i] * block_size;
// 		for (int j = 0; j < 256; j++) {
// 			singly_indirect = block + doubly_indirect[j] * block_size;
// 			for (int k = 0; k < 256 ; k++) {
//
// 				//SAME AS ABOVE
// 				int check = check_entry(singly_indirect[k], name);
// 				if (check != 0) {
// 					return check;
// 				}
// 			}
// 		}
// 	}
// 	return -1;
// }














/*
 * Checks to see if if the block contains information about a file of the name
 * name, and returns that block number
 * Return 0 if not found and no errors
 */
int check_entry(unsigned int block_no, char * name){
	if (block_no == 0) {
		fprintf(stderr, "No such file or directory \n");
		//return ENOENT;
		return -1;
	}
	struct ext2_dir_entry_2 * i_entry = (struct ext2_dir_entry_2 *)(block + block_size * block_no);

	//???????????? EXT2_FT_DIR VS EXT2_S_IFDIR ????
	if (strcmp(i_entry -> name, name) == 0 && i_entry -> file_type == EXT2_FT_DIR) {
		//curr_inode = (inode_table + (i_entry->inode - 1) * block_size);
		return block_no;
	}
	return 0;
}

int find_singly_indirect(int block_no, int i){
	if (block_no != 0) {
		unsigned int * singly_indirect = (unsigned int *)block + block_no * block_size;
		return singly_indirect[i];
	}
	return 0;
}

int find_doubly_indirect(int block_no, int i, int j) {
	if (block_no != 0) {
		unsigned int * doubly_indirect = (unsigned int *)block + block_no * block_size;
		return find_singly_indirect(doubly_indirect[i], j);
	}
	return 0;
}

int find_triply_indirect(int block_no, int i, int j, int k) {
	if (block_no != 0) {
		unsigned int * triply_indirect = (unsigned int *)block + block_no * block_size;
		return find_doubly_indirect(triply_indirect[i], j, k);
	}
	return 0;
}


// int check = check_entry(inode_block[entry_number], name);
// 	if (check != 0) {
// 		return check;
// 	}
