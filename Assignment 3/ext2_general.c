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
	print_bitmap(i_bitmap_size, inode_bitmap);
	printf("\n");
	printf("block_bitmap: ");
	print_bitmap(b_bitmap_size, block_bitmap);
	printf("\n");
}

void print_bitmap(int bitmap_size, unsigned char * bitmap){
	for (int i = 0; i < bitmap_size; i++) {

			/* Looping through each bit a byte. */
			for (int k = 0; k < 8; k++) {
					printf("%d", (bitmap[i] >> k) & 1);
			}
			printf(" ");
	}
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
		curr = inode_table + (inode_no - 1);
		if (!(curr->i_mode & EXT2_S_IFDIR)) {
			fprintf(stderr, "working on a file\n");
			exit(1);
		}
		inode_no = check_directory(token, inode_no, 0, &check_entry);
		if (!inode_no) {
			return 0;
		}
    token = strtok(NULL, "/");
  }
	return inode_no;
}






//---------------------------------- MODIFIED
/*
 * Checks to see whether or not the file with file name 'name' is in the
 * current working directory
 * Returns the inode_no number if found. Otherwise return 0
 */
int check_directory(char * name, unsigned int inode_no, int flag, int (*fun_ptr)(unsigned int *, int, char *, int)){
	struct ext2_inode * inode = inode_table + (inode_no - 1);
	unsigned int * inode_block = inode->i_block;
	unsigned int * current_block = inode_block;
	int curr_index = 0;
	// Initialize all the variables needed
	int index = 0;
	bool end_next_loop = false;

	int i = 0;
	int j = 0;
	int k = 0;
	if (!inode_block[index]) {
		fprintf(stderr, "something wrong with pathwalk, curr == 0\n");
		exit(1);
	}

	while (index < 15) {
		int return_val;
		if ((return_val = (*fun_ptr)(current_block, curr_index, name, flag)) >= 0) {
			return return_val;
		}

		// Terminating condition
		if (end_next_loop) {
			return 0;
		}

		// Perform checks on the iblocks
		if (index < 12) { //DIRECT
			// Set the next variables
			index++;
			if (index == 12) {
				printf("Now gonna check 12th entry -- singly indirects \n");
				current_block = find_singly_indirect(inode_block, index, &curr_index);
				//curr_index = 0;
			} else {
				curr_index = index;
			}
		}

		else if (index == 12) { //SINGLY INDIRECT
			// Set the next variables
			i++;
			if (i == 257) { // If there are no more
				index++;
				i = 0;
				printf("Now gonna check 13th entry -- doubly indirects \n");
				curr_index = 0;
				current_block = find_doubly_indirect(inode_block, index, i, &curr_index);
			}

			// If we still working on the same inode
			if (index == 12) {
				curr_index = i;
				current_block = find_singly_indirect(inode_block, index, &curr_index);
			}
		}


		else if (index == 13) { //DOUBLY INDIRECT
			i++;
			if (i == 257) {
				i = 0;
				j++;
				if (j == 257) {
					j = 0;
					index++;
					printf("Now gonna check 14th entry -- triply indirects \n");
					curr_index = 0;
					current_block = find_triply_indirect(inode_block, index, i, j, &curr_index);
				}
			}

			// If we are still working on the same inode
			if (index == 13) {
				curr_index = j;
				current_block = find_doubly_indirect(inode_block, index, i, &curr_index);
			}
		}

		else if (index == 14) { //TRIPLY INDIRECT
			i++;
			if (i == 257) {
				i = 0;
				j++;
				if (j == 257) {
					j = 0;
					k++;
					if (k == 257) { //searched the entire thing but doesn't exist
						index++;
					}
				}
			}
			if (index == 14) {
				curr_index = k;
				current_block = find_triply_indirect(inode_block, index, i, j, &curr_index);
			}
		}

		// Checks if current block is valid
		if (!current_block[curr_index]) {
			end_next_loop = true;
		}
	}
	return 0;
}





/*
 * Looks for all the files in the block block_no. Print all files except for
 * the hidden files if flag is NULL. If flag is not NULL the print all files.
 * Return -1 to indicate to the check_directory to keep printing all files
 */
int print_file(unsigned int * block, int block_idx, char * null, int include_all) {
	int block_no = block[block_idx];
	if (block_no != 0) {
		struct ext2_dir_entry_2 * i_entry = (struct ext2_dir_entry_2 *)(disk + block_no * block_size);
		int inode_no = i_entry->inode;

		int count = 0;
		while (inode_no != 0 && count < EXT2_BLOCK_SIZE) {
			char * name = i_entry -> name;

			// Check to see if hidden files are allowed
			if (include_all || (strlen(name) > 0 && !(name[0] == '.'))) {
				printf("%s\n", i_entry->name);
			}
			inode_no = i_entry->inode;
			count+= i_entry->rec_len;
			i_entry = (void *)i_entry + i_entry->rec_len;
		}
	}
	return -1;

}



/*
 * If name is not NULL, checks to see if if the block contains information
 * about a file of the name 'name', and returns that block number.
 * Return -1 if not found and no errors
 *
 * If name is NULL, checks to see if the block contains a free spot
 * and returns that block number. Return -1 if no free blocks.
 */
int check_entry(unsigned int * block, int block_idx, char * name, int checking_free){
	int block_no = block[block_idx];
	if (block_no != 0) {

		struct ext2_dir_entry_2 * i_entry = (struct ext2_dir_entry_2 *)(disk + block_no * block_size);
		int inode_no = i_entry->inode;

		int count = i_entry->rec_len - 1;
		while (inode_no != 0 && count < EXT2_BLOCK_SIZE) {

			if (!checking_free) { //If we are not looking for a free spot
				if (!strcmp(i_entry->name, name)) {
					return i_entry->inode;
				}
			}
			inode_no = i_entry->inode;
			if (inode_no){
				count+= i_entry->rec_len;
				i_entry = (void *)i_entry + i_entry->rec_len;
			}

		}

		if (checking_free && count < EXT2_BLOCK_SIZE) {
			return count;
		}
	}
	return -1;
}

/*
 * Add a new file to the current block if there is space and returns the
 * new file's inode
 * Returns -1 if no space is found
 */
int add_entry(unsigned int * block, int block_idx, char * name, int file_type) {
	static unsigned int * prev_block = 0;
	static unsigned int prev_idx = 0;

	// Checks to see if the current block exists
	if (!block[block_idx]) {

		// Create the new inode for directory
		int new_inode_no = search_bitmap(inode_bitmap, i_bitmap_size);
		if (new_inode_no == -ENOMEM) {
			fprintf(stderr, "no space in inode bitmap\n");
			exit(1);
		}
		take_spot(inode_bitmap, new_inode_no);
		create_inode(new_inode_no);
		// Check the entries and see if previous is full
		int idx = check_entry(prev_block, prev_idx, NULL, true);

		int block_no = prev_block[prev_idx];
		if (idx < 0) { //If there is no space in the previous block
			int singly_block_no = search_bitmap(block_bitmap, b_bitmap_size);
			if (singly_block_no == -ENOMEM) {
				fprintf(stderr, "no space in block bitmap\n");
				exit(1);
			}
			take_spot(block_bitmap, singly_block_no);
			block[block_idx] = singly_block_no;
			block_no = singly_block_no;
			idx = 0;
		}

		// Create the new entry
		create_new_entry(block_no, new_inode_no, idx, name, file_type);
		return new_inode_no;
	}
	prev_block = block;
	prev_idx = block_idx;
	return -1;
}

/*
 * Create a new file at the block number 'block_no' with displacement bytes
 * where this entry corresponds to the inode with inode number 'inode_no'.
 * The name of the file is 'name' of file type 'file_type'
 */
void create_new_entry(int block_no, int inode_no, int displacement, char * name, int file_type){
	struct ext2_dir_entry_2 * i_entry = (struct ext2_dir_entry_2 *)(disk + block_no * block_size + displacement);
	i_entry = (void *)i_entry + displacement;
	i_entry->inode = inode_no;
	i_entry->name_len = strlen(name);
	i_entry->file_type = file_type;
	i_entry->rec_len = sizeof(struct ext2_dir_entry_2) + 1; //is this correct????
	strncpy(i_entry->name, name, EXT2_NAME_LEN);
}

/*
 * Creates a new inode at the inodenumber 'new_inode'
 * info can be:
 *						parent_inode
 *						data in a reg file
 */
void create_inode(int new_inode_no){

	// Reset all values to be the default
	struct ext2_inode * new_inode = inode_table + (new_inode_no - 1);
	for (int i = 0; i < 15; i++) {
		free_spot(block_bitmap, (new_inode -> i_block)[i]);
		(new_inode -> i_block)[i] = 0;
	}
	new_inode->i_dtime = 0; // remove deletion time
	new_inode->i_links_count = 1;
}

/*
* Returns the current_block using the singly indirect table
* if it is found. Otherwise return the block that is empty
 */
unsigned int * find_singly_indirect(unsigned int * block, int block_no , int * index){
	if (block[block_no]) {
		unsigned int * singly_indirect = (unsigned int *)(disk + block[block_no] * block_size);
		return singly_indirect;
	}
	*index = block_no;
	return block;
}


/*
 * Returns the current_block using the doubly indirect table
 * if it is found. Otherwise return the block that is empty
 */
unsigned int * find_doubly_indirect(unsigned int * block, int block_no , int i, int * index) {
	if (block[block_no]) {
		unsigned int * doubly_indirect = (unsigned int *)(disk + block[block_no] * block_size);
		return find_singly_indirect(doubly_indirect, i, index);
	}
	*index = block_no;
	return block;
}

/*
* Returns the current_block using the triply indirect table
* if it is found. Otherwise return the block that is empty
 */
unsigned int * find_triply_indirect(unsigned int * block, int block_no , int i, int j, int * index) {
	if (block[block_no]) {
		unsigned int * triply_indirect = (unsigned int *)(disk + block[block_no] * block_size);
		return find_doubly_indirect(triply_indirect, i, j, index);
	}
	*index = block_no;
	return block;
}


/*
 * Extracts the filename and dir from the path
 */
void split_path(char * path, char * name, char * dir) {
	int count = 0;
	int name_idx = 0;
	int dir_idx = 0;
	while (count < strlen(path)) {
		//printf("count %d\n", count);
		char file[EXT2_NAME_LEN];
		int file_idx = 0;
		while (count < strlen(path) && path[count] != '/' ) {
			file[file_idx] = path[count];
			file_idx++;
			count++;
		}
		file[file_idx] = '\0';
		printf("count: %d file: %s\n",count, file);
		// indicating the last file in the path

		if (path[count] != '/') {
			str_cat(name, file, &name_idx);
		} else {
			str_cat(dir, file, &dir_idx);
			(dir)[dir_idx] = '/';
			(dir)[dir_idx + 1] = '\0';
			dir_idx ++;
			printf("dir: %s\n", dir);
			count++;
		}

		//printf("count %d\n", count);
		//count++;
	}
	//fprintf(stderr,"%s, %s", *dir, *name);
}

/*
 * Append the substring 'substring' to str
 */
void str_cat(char * str, char * substring, int * index) {
	int count = 0;
	while (count < strlen(substring)) {
		(str)[*index] = substring[count];
		(*index)++;
		count++;
	}
	(str)[*index] = '\0';
	printf("file: %s\n",str);
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

void free_spot(unsigned char * bitmap, int index) {
	int bit_map_byte = index / 8;
	int bit_order = index % 8;
	bitmap[bit_map_byte] = bitmap[bit_map_byte] | ~(1 << bit_order);
}
