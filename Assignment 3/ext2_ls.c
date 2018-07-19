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

int main(int argc, char ** argv){
	if (argc < 3 || argc > 4){
		fprintf(stderr, "ls command requires 2 arguments\n");
		exit(1);
	}


	// ------------------------ convert the arguments -----------------------//
	char * virtual_disk = NULL;
	char * path = NULL;
	bool flag = false;
	//check if flag -a not specified
	if (argc == 3) {
		virtual_disk = argv[1];
		path = argv[2];
	} else if (strcmp(argv[2], "-a") == 0){
		flag = true;
		virtual_disk = argv[2];
		path = argv[3];
	} else {
		fprintf(stderr, "unknown flag specified");
		exit(1);
	}

	//---------------------------- open the image ---------------------------//
	open_image(virtual_disk);

	//--------------------------- setup datastructures ----------------------//
	init_datastructures();

	//Get the root inode
	struct ext2_inode * root = (struct ext2_inode *)block + EXT2_ROOT_INO * block_size;


	//---------------------------- go to the paths inode ----------------------//
	unsigned int block_no;
	if ((block_no = path_walk(root, path)) < 0) {
		fprintf(stderr, "not a valid path");
		exit(1);
	}




	//-------------- PRINT ALL FILES LISTED IN THAT DIRECTORY ----------------//
	check_directory(NULL, block_no, &print_file);
	// struct ext2_inode * inode = (unsigned int *)(block + block_no * block_size);
	// unsigned int * inode_block = inode->i_block;
	// // Initialize all the variables needed
	// int index = 0;
	// unsigned int curr_block_no = inode_block[index];
	// int i = 0;
	// int j = 0;
	// int k = 0;
	// if (curr_block_no == 0) {
	// 	fprintf(stderr, "something wrong with pathwalk, curr == 0\n");
	// 	exit(1);
	// }
	//
	//
	// while (curr_block_no != 0) {
	// 	struct ext2_dir_entry_2 * curr_block = (struct ext2_dir_entry_2 *)block + curr_block_no * block_size;
	// 	printf("%s\n", curr_block->name);
	//
	//
	// 	if (index < 12) { //DIRECT
	// 		// Set the next variables
	// 		index++;
	// 		curr_block_no = 0;
	//
	// 		if (index == 12) {
	// 			printf("Now gonna check 12th entry -- singly indirects \n");
	// 			curr_block_no = find_singly_indirect(inode_block[index], i);
	//
	// 		} else {
	// 			curr_block_no = inode_block[index];
	// 		}
	// 	}
	//
	// 	else if (index == 12) { //SINGLY INDIRECT
	// 		// Set the next variables
	// 		i++;
	// 		curr_block_no = 0;
	// 		if (i == 257) { // If there are no more
	// 			index++;
	// 			i = 0;
	// 			printf("Now gonna check 13th entry -- doubly indirects \n");
	// 			curr_block_no = find_doubly_indirect(inode_block[index], i, j);
	// 		}
	//
	// 		// If we still working on the same inode
	// 		if (index == 12) {
	// 			curr_block_no = find_singly_indirect(inode_block[index], i);
	// 		}
	// 	}
	//
	//
	// 	else if (index == 13) { //DOUBLY INDIRECT
	// 		i++;
	// 		curr_block_no = 0;
	//
	// 		if (i == 257) {
	// 			i = 0;
	// 			j++;
	// 			if (j == 257) {
	// 				j = 0;
	// 				index++;
	// 				printf("Now gonna check 14th entry -- triply indirects \n");
	// 				curr_block_no = find_triply_indirect(inode_block[index], i, j, k);
	// 			}
	// 		}
	// 		// If we are still working on the same inode
	// 		if (index == 13) {
	// 			curr_block_no = find_doubly_indirect(inode_block[index], i, j);
	// 		}
	// 	}
	//
	// 	else if (index == 14) { //TRIPLY INDIRECT
	// 		i++;
	// 		curr_block_no = 0;
	//
	// 		if (i == 257) {
	// 			i = 0;
	// 			j++;
	// 			if (j == 257) {
	// 				j = 0;
	// 				k++;
	// 				if (k == 257) {
	// 					index++;
	// 				}
	// 			}
	// 		}
	// 		if (index == 14) {
	// 			curr_block_no = find_triply_indirect(inode_block[index], i, j, k);
	// 		}
	// 	}
	// 	else {
	// 		printf("what the heck shoulnd't be here\n");
	// 		exit(1);
	// 	}
	// }
	// return 0;
}
