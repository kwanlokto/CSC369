#include "ext2.h"

extern unsigned char * disk;
extern unsigned int block_size;
extern struct ext2_inode * inode_table;
extern struct ext2_super_block * sb;
extern struct ext2_group_desc * descriptor;
extern unsigned char * inode_bitmap;
extern unsigned char * block_bitmap;

void init_datastructures() {
	// Referenced from https://wiki.osdev.org/Ext2
	// and http://www.nongnu.org/ext2-doc/ext2.html#S-LOG-BLOCK-SIZE
	sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
	block_size = EXT2_BLOCK_SIZE << (sb -> s_log_block_size);
	descriptor = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE + block_size);
	//block number where the inode table starts
	//Reference from the table 3-1 in nongnu.org
	inode_table = (struct ext2_inode *)(disk + descriptor -> bg_inode_table * block_size);
	inode_bitmap = disk + descriptor -> bg_inode_bitmap * block_size;
	block_bitmap = disk + descriptor -> bg_block_bitmap * block_size;

	printf("inode_bitmap: ");
	for (int i = 0; i < (sb->s_inodes_count)/(sizeof(unsigned char) * 8); i++) {

			/* Looping through each bit a byte. */
			for (int k = 0; k < 8; k++) {
					printf("%d", (inode_bitmap[i] >> k) & 1);
			}
			printf(" ");
	}
	printf("\n");
	printf("block_bitmap: ");
	for (int i = 0; i < (sb->s_blocks_count)/(sizeof(unsigned char) * 8); i++) {

			/* Looping through each bit a byte. */
			for (int k = 0; k < 8; k++) {
					printf("%d", (block_bitmap[i] >> k) & 1);
			}
			printf(" ");
	}
	printf("\n");
}



void open_image(char * virtual_disk) {
	int fd = open(virtual_disk, O_RDWR);

	disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(disk == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}
}


/*
 * Returns the inode number of the path to file
 */
unsigned int path_walk(char * path) {

	// Referenced from https://www.geeksforgeeks.org/how-to-split-a-string-in-cc-python-and-java/
  char *token = strtok(path, "/");
	int inode_no = EXT2_ROOT_INO;

	struct ext2_inode * curr;

  while (token != NULL) {
		printf("enetere");
		curr = inode_table + (inode_no - 1);
		if (!(curr->i_mode & EXT2_S_IFDIR)) {
			fprintf(stderr, "working on a file\n");
			exit(1);
		}
		inode_no = check_directory(token, inode_no, &check_entry);
		printf("check %d\n", inode_no);
		if (!inode_no) {
			return inode_no;
		}
    token = strtok(NULL, "/");
  }
	return inode_no;
}


/*
 * Checks to see whether or not the file with file name 'name' is in the
 * current working directory
 * Returns the inode_no number if found
 */
unsigned int check_directory(char * name, unsigned int inode_no, unsigned int (*fun_ptr)(unsigned int, char *)){
	struct ext2_inode * inode = inode_table + (inode_no - 1);
	unsigned int * inode_block = inode->i_block;
	// Initialize all the variables needed
	int index = 0;
	unsigned int block_no = inode_block[index];
	int i = 0;
	int j = 0;
	int k = 0;
	if (block_no == 0) {
		fprintf(stderr, "something wrong with pathwalk, curr == 0\n");
		exit(1);
	}

	while (block_no != 0) {
		printf("Checking block no %d in inode no %d at index %d\n",block_no, inode_no, index);
		unsigned int return_val;
		if ((return_val = (*fun_ptr)(block_no, name)) > 0) {
			return return_val;
		}
		// printf("%s\n", curr_block->name);

		if (block_no != 0) { //DIRECT
			// Set the next variables
			index++;
			block_no = 0;

			if (index == 12) {
				printf("Now gonna check 12th entry -- singly indirects \n");
				block_no = find_singly_indirect(inode_block[index], i);

			} else {
				block_no = inode_block[index];
			}
		}

		else if (index == 12) { //SINGLY INDIRECT
			// Set the next variables
			i++;
			block_no = 0;
			if (i == 257) { // If there are no more
				index++;
				i = 0;
				printf("Now gonna check 13th entry -- doubly indirects \n");
				block_no = find_doubly_indirect(inode_block[index], i, j);
			}

			// If we still working on the same inode
			if (index == 12) {
				block_no = find_singly_indirect(inode_block[index], i);
			}
		}


		else if (index == 13) { //DOUBLY INDIRECT
			i++;
			block_no = 0;

			if (i == 257) {
				i = 0;
				j++;
				if (j == 257) {
					j = 0;
					index++;
					printf("Now gonna check 14th entry -- triply indirects \n");
					block_no = find_triply_indirect(inode_block[index], i, j, k);
				}
			}
			// If we are still working on the same inode
			if (index == 13) {
				block_no = find_doubly_indirect(inode_block[index], i, j);
			}
		}

		else if (index == 14) { //TRIPLY INDIRECT
			i++;
			block_no = 0;

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
				block_no = find_triply_indirect(inode_block[index], i, j, k);
			}
		}
		else {
			printf("what the heck shoulnd't be here\n");
			exit(1);
		}
	}
	return 0;
}




unsigned int print_file(unsigned int block_no, char * flag) {
	if (block_no != 0) {


		struct ext2_dir_entry_2 * i_entry = (struct ext2_dir_entry_2 *)(disk + block_no * block_size);
		int inode_no = i_entry->inode;

		int count = 0;
		while (inode_no != 0 && count < EXT2_BLOCK_SIZE) {
			char * name = i_entry -> name;

			// Check to see if hidden files are allowed
			if (flag != NULL || (strlen(name) > 0 && !(name[0] == '.'))) {
				printf("%s\n", i_entry->name);
			}
			inode_no = i_entry->inode;
			count+= i_entry->rec_len;
			i_entry = (void *)i_entry + i_entry->rec_len;
		}
	}
	return 0;

}



/*
 * Checks to see if if the block contains information about a file of the name
 * name, and returns that block number
 * Return 0 if not found and no errors
 */
unsigned int check_entry(unsigned int block_no, char * name){
	if (block_no != 0) {


		struct ext2_dir_entry_2 * i_entry = (struct ext2_dir_entry_2 *)(disk + block_no * block_size);
		int inode_no = i_entry->inode;

		int count = 0;
		while (inode_no != 0 && count < EXT2_BLOCK_SIZE) {

			printf("comparing two names: %s vs %s \n \n", i_entry->name, name);
			if (strcmp(i_entry->name, name) == 0) {
				return i_entry->inode;
			}
			inode_no = i_entry->inode;
			count+= i_entry->rec_len;
			i_entry = (void *)i_entry + i_entry->rec_len;

		}
	}
	return 0;
}

unsigned int find_singly_indirect(int block_no, int i){
	if (block_no != 0) {
		unsigned int * singly_indirect = (unsigned int *)(disk + block_no * block_size);
		return singly_indirect[i];
	}
	return 0;
}

unsigned int find_doubly_indirect(int block_no, int i, int j) {
	if (block_no != 0) {
		unsigned int * doubly_indirect = (unsigned int *)(disk + block_no * block_size);
		return find_singly_indirect(doubly_indirect[i], j);
	}
	return 0;
}

unsigned int find_triply_indirect(int block_no, int i, int j, int k) {
	if (block_no != 0) {
		unsigned int * triply_indirect = (unsigned int *)(disk + block_no * block_size);
		return find_doubly_indirect(triply_indirect[i], j, k);
	}
	return 0;
}


/*
 * Extracts the filename and dir from the path
 */
void split_path(char * path, char * name, char * dir) {
	int count = 0;
	while (path[count] != '\0') {
		char file[EXT2_NAME_LEN];
		while (path[count] != '\0' && path[count] != '/' ) {
			strcat(file, path[count]);
			count++;
		}

		// indicating the last file in the path
		if (path[count] != '/') {
			strcat(name, file);
		} else {
			strcat(dir, file);
			strcat(dir, "/");
		}

		count++;
	}
}

int get_free_spot(unsigned char * bitmap, int max) {
	for (int i = 0; i < max; i++) {

			/* Looping through each bit a byte. */
			for (int k = 0; k < 8; k++) {
				int bit = (bitmap[i] >> k) & 1;
				if (!bit) {
					int index = i * 8 + k;
					printf("index: %d\n", index);
					return index;
				}
			}
	}
	return -1;
}

int take_spot(unsigned char * bitmap, int index) {
	int bit_map_byte = index / 8;
	int bit_order = index % 8;
	if ((bitmap[bit_map_byte] >> bit_order) & 1) {
		fprintf(stderr, "trying to write to a taken spot\n");
		exit(1);
	}
	bitmap[bit_map_byte] = bitmap[bit_map_byte] | (1 << bit_order);
}
