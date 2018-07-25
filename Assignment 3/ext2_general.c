#include "ext2.h"

/*
 * All extern variables
 */
extern unsigned char * disk;
extern unsigned int block_size;
extern struct ext2_inode * inode_table;
extern struct ext2_super_block * sb;
extern struct ext2_group_desc * descriptor;
extern unsigned char * inode_bitmap;
extern unsigned char * block_bitmap;
extern int i_bitmap_size;
extern int b_bitmap_size;
/*
 * Initializes all global variables
 */
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

	i_bitmap_size = (sb->s_inodes_count)/(sizeof(unsigned char) * 8);
	b_bitmap_size = (sb->s_blocks_count)/(sizeof(unsigned char) * 8);
	printf("inode_bitmap: ");
	for (int i = 0; i < i_bitmap_size; i++) {

			/* Looping through each bit a byte. */
			for (int k = 0; k < 8; k++) {
					printf("%d", (inode_bitmap[i] >> k) & 1);
			}
			printf(" ");
	}
	printf("\n");
	printf("block_bitmap: ");
	for (int i = 0; i < b_bitmap_size; i++) {

			/* Looping through each bit a byte. */
			for (int k = 0; k < 8; k++) {
					printf("%d", (block_bitmap[i] >> k) & 1);
			}
			printf(" ");
	}
	printf("\n");
}


/*
 * Open the image and initailize the disk
 */
void open_image(char * virtual_disk) {
	int fd = open(virtual_disk, O_RDWR);

	disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(disk == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}
}


/*
 * Returns the inode number of file that is at the end of the path.
 * If the file is not found then return 0
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
			return 0;
		}
    token = strtok(NULL, "/");
  }
	return inode_no;
}


/*
 * Checks to see whether or not the file with file name 'name' is in the
 * current working directory
 * Returns the inode_no number if found. Otherwise return 0
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

		if (index < 12) { //DIRECT
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
					if (k == 257) { //searched the entire thing but doesn't exist
						index++;
						block_no = 0;
					}
				}
			}
			if (index == 14) {
				block_no = find_triply_indirect(inode_block[index], i, j, k);
			}
		}
	}
	return 0;
}



/*
 * Looks for all the files in the block block_no. Print all files except for
 * the hidden files if flag is NULL. If flag is not NULL the print all files.
 */
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


/*
 * Returns the inode_no of the pointed inode block from the singly indirect table
 * if it is found. Otherwise return 0
 */
unsigned int find_singly_indirect(int block_no, int i){
	if (block_no != 0) {
		unsigned int * singly_indirect = (unsigned int *)(disk + block_no * block_size);
		return singly_indirect[i];
	}
	return 0;
}


/*
 * Returns the inode_no of the pointed inode block from the doubly indirect table
 * if it is found. Otherwise return 0
 */
unsigned int find_doubly_indirect(int block_no, int i, int j) {
	if (block_no != 0) {
		unsigned int * doubly_indirect = (unsigned int *)(disk + block_no * block_size);
		return find_singly_indirect(doubly_indirect[i], j);
	}
	return 0;
}

/*
 * Returns the inode_no of the pointed inode block from the triply indirect table
 * if it is found. Otherwise return 0
 */
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
			strcat(file, &path[count]);
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

/*
 * Searches the entire bitmap and returns the index of the bitmap with a free
 * spot. If no free spot is found return -ENOMEM
 */
int search_bitmap(unsigned char * bitmap, int max) {
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
	return -ENOMEM;
}


/*
 * Update the bitmap, and set the bit at index 'index' to 1
 */
void take_spot(unsigned char * bitmap, int index) {
	int bit_map_byte = index / 8;
	int bit_order = index % 8;
	if ((bitmap[bit_map_byte] >> bit_order) & 1) {
		fprintf(stderr, "trying to write to a taken spot\n");
		exit(1);
	}
	bitmap[bit_map_byte] = bitmap[bit_map_byte] | (1 << bit_order);
}


/*
 * Searches all i_block in the struct ext2_inode and looks for a free entry
 * and inserts the new inode_no into that entry.
 * Returns 0 if successful and returns -ENOMEM if unsucessful.
 */
int assign_iblock(unsigned int dir_inode_no, unsigned int inode_no) {
	struct ext2_inode * dir_inode = inode_table + (dir_inode_no - 1);

	int index = 0;
	int i = 0;
	int j = 0;
	int k = 0;
	unsigned int * inode_block = dir_inode->i_block;

	while (index < 15) {
		if (index < 12) { // DIRECT
			if (!inode_block[index]) {
				inode_block[index] = inode_no;
				return 0;
			}
			index++;
		}

		else if (index == 12) { //SINGLY INDIRECT
			if (!inode_block[index]) { //no singly indirect table
				int singly_block_no = search_bitmap(block_bitmap, b_bitmap_size);
				if (singly_block_no == -ENOMEM) {
					return -ENOMEM;
				}
				take_spot(block_bitmap, singly_block_no);
				inode_block[index] = singly_block_no;
				// unsigned int * singly_indirect = (unsigned int *)(disk + inode_block[index] * block_size);
				// singly_indirect[i] = inode_no;
				// return 0;
			}
			if (!find_singly_indirect(inode_block[index], i)) { //entry i is free
				unsigned int * singly_indirect = (unsigned int *)(disk + inode_block[index] * block_size);
				singly_indirect[i] = inode_no;
				return 0;
			}
			i++;
			if (i == 257) {
				index++;
				i = 0;
			}
		}


		else if (index == 13) { //DOUBLY INDIRECT
			if (!inode_block[index]) { //no doubly indirect table
				int doubly_block_no = search_bitmap(block_bitmap, b_bitmap_size);
				if (doubly_block_no == -ENOMEM) {
					return -ENOMEM;
				}
				take_spot(block_bitmap, doubly_block_no);
				inode_block[index] = doubly_block_no;
				// unsigned int * singly_indirect = (unsigned int *)(disk + inode_block[index] * block_size);
				// singly_indirect[i] = inode_no;
				// return 0;
			}
			unsigned int * doubly_indirect = (unsigned int *)(disk + inode_block[index] * block_size);
			if (!doubly_indirect[i]) {
				int singly_block_no = search_bitmap(block_bitmap, b_bitmap_size);
				if (singly_block_no == -ENOMEM) {
					return -ENOMEM;
				}
				take_spot(block_bitmap, singly_block_no);
				doubly_indirect[i] = singly_block_no;
			}
			if (!find_singly_indirect(doubly_indirect[i], j)){
				unsigned int * singly_indirect = (unsigned int *)(disk + doubly_indirect[i] * block_size);
				singly_indirect[j] = inode_no;
				return 0;
			}

			i++;
			if (i == 257) {
				j++;
				i = 0;
				if (j == 257) {
					index++;
					j = 0;
				}
			}
		}


		else if (index == 14) { //TRIPLY INDIRECT
			if (!inode_block[index]) { //no triply indirect table
				unsigned int triply_block_no = search_bitmap(block_bitmap, b_bitmap_size);
				if (triply_block_no == -ENOMEM) {
					return -ENOMEM;
				}
				take_spot(block_bitmap, triply_block_no);
				inode_block[index] = triply_block_no;
				// unsigned int * singly_indirect = (unsigned int *)(disk + inode_block[index] * block_size);
				// singly_indirect[i] = inode_no;
				// return 0;
			}
			unsigned int * triply_indirect = (unsigned int *)(disk + inode_block[index] * block_size);
			if (!triply_indirect[i]) {
				unsigned int doubly_block_no = search_bitmap(block_bitmap, b_bitmap_size);
				if (doubly_block_no == -ENOMEM) {
					return -ENOMEM;
				}
				take_spot(block_bitmap, doubly_block_no);
				triply_indirect[i] = doubly_block_no;
			}
			unsigned int * doubly_indirect = (unsigned int *)(disk + triply_indirect[i] * block_size);
			if (!doubly_indirect[j]){
				unsigned int singly_block_no = search_bitmap(block_bitmap, b_bitmap_size);
				if (singly_block_no == -ENOMEM) {
					return -ENOMEM;
				}
				take_spot(block_bitmap, singly_block_no);
				doubly_indirect[i] = singly_block_no;
			}
			if (!find_singly_indirect(doubly_indirect[j], k)){
				unsigned int * singly_indirect = (unsigned int *)(disk + doubly_indirect[j] * block_size);
				singly_indirect[j] = inode_no;
				return 0;
			}

			i++;
			if (i == 257){
				j++;
				i = 0;
				if (j == 257) {
					k++;
					j = 0;
					if (k == 257) {
						k = 0;
						index++;
					}
				}
			}
		}
	}
	return -ENOMEM;
}
