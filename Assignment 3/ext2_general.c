#include "ext2.h"

extern unsigned char * disk;
extern unsigned int block_size;
extern struct ext2_inode * inode_table;
extern struct ext2_super_block * sb;
extern struct ext2_group_desc * descriptor;
extern unsigned char * inode_bitmap;
extern unsigned char * block_bitmap;
extern int i_bitmap_size;
extern int b_bitmap_size;

// ---------------------------- Setting up Functions -------------------------//
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




// --------------------------- Traversing a Path -----------------------------//
/*
 * Returns the inode number of file that is at the end of the path.
 * If the file is not found then return -ENOENT or if is not a directory
 */
int path_walk(char * path) {
	char name[EXT2_NAME_LEN];
	int name_idx = 0;
	int path_idx = 0;
	int inode_no = EXT2_ROOT_INO;
	bool is_dir = true;
	struct ext2_inode * curr;
	while(path_idx < strlen(path)) {

		// Get the name of the next file
		name_idx = 0;
		is_dir = false;

		// if it is absolute path
		if (path_idx == 0 && path[path_idx] == '/') {
			name[0] = '.';
			name_idx++;
		}
		while(path_idx < strlen(path) && path[path_idx] != '/') {
			name[name_idx] = path[path_idx];
			name_idx++;
			path_idx++;
			if (path[path_idx] == '/') {
				is_dir = true;
			}
		}
		name[name_idx] = '\0';
		inode_no = check_directory(name, inode_no, 0, &check_entry);
		if (inode_no == -1) {
			fprintf(stderr, "Path does not exist\n");
			return -ENOENT;
		}
		curr = inode_table + (inode_no - 1);
		if (!(curr->i_mode & EXT2_S_IFDIR) && is_dir) {
			fprintf(stderr, "That is not a directory\n");
			return -ENOTDIR;
		}
		path_idx++;
	}
	return inode_no;

}


// MODIFIED RETURN VALUE OF CHECK_DIR
/*
 * Checks to see whether or not the file with file name 'name' is in the
 * current working directory
 * Returns the inode_no number if found. Otherwise return -1
 */
int check_directory(char * name, unsigned int inode_no, int flag, int (*fun_ptr)(unsigned int *, int, char *, int)){
	struct ext2_inode * inode = inode_table + (inode_no - 1);
	unsigned int * inode_block = inode->i_block;
	unsigned int * current_block = inode_block;
	int curr_index = 0;
	// Initialize all the variables needed
	int index = 0;

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
			if (i == 256) { // If there are no more
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
			if (i == 256) {
				i = 0;
				j++;
				if (j == 256) {
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
			if (i == 256) {
				i = 0;
				j++;
				if (j == 256) {
					j = 0;
					k++;
					if (k == 256) { //searched the entire thing but doesn't exist
						index++;
					}
				}
			}
			if (index == 14) {
				curr_index = k;
				current_block = find_triply_indirect(inode_block, index, i, j, &curr_index);
			}
		}

	}
	return -1;
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




// ---------------------- Operations on EXT2_DIR_ENTRIES ---------------------//
/*
 * Looks for all the files in the block block_no. Print all files except for
 * the hidden files if flag is NULL. If flag is not NULL the print all files.
 * Return -1 to indicate to the check_directory to keep printing all files
 */
int print_file(unsigned int * block, int block_idx, char * cmp_name, int include_all) {
	int block_no = block[block_idx];
	if (block_no != 0) {
		struct ext2_dir_entry_2 * i_entry = (struct ext2_dir_entry_2 *)(disk + block_no * block_size);
		int inode_no = i_entry->inode;

		int count = 0;
		while (inode_no != 0 && count < EXT2_BLOCK_SIZE) {
			char * name = i_entry -> name;
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


// MODIFIED
/*
 * If checking_free == 0, checks to see if if the block contains information
 * about a file of the name 'name', and returns that block number.
 * Return -1 if not found and no errors
 *
 * If checking_free == 1, checks to see if the block contains a free spot
 * and returns that block number. Return -1 if no free blocks.
 */
int check_entry(unsigned int * block, int block_idx, char * name, int checking_free){
	int block_no = block[block_idx];
	if (block_no != 0) {

		struct ext2_dir_entry_2 * i_entry = (struct ext2_dir_entry_2 *)(disk + block_no * block_size);
		//int inode_no = i_entry->inode;
		int count = 0;
		while (count < EXT2_BLOCK_SIZE) {
			if (!checking_free) { //If we are not looking for a free spot
				if (!strcmp(i_entry->name, name)) {
					return i_entry->inode;
				}
			}
			//inode_no = i_entry->inode;
			count+= i_entry->rec_len;
			if (count < EXT2_BLOCK_SIZE) {
				i_entry = (void *)i_entry + i_entry->rec_len;
			}
		}

		if (checking_free) {
			int current_idx = count - i_entry->rec_len;
			int current_size = sizeof(struct ext2_dir_entry_2) + i_entry->name_len;
			int adding_size = sizeof(struct ext2_dir_entry_2) + strlen(name);
			int new_rec_len = current_idx + current_size + adding_size;

			printf("curr_idx %d, curr_size %d, new_rec %d on block %d\n", current_idx, current_size, new_rec_len, block_no);
			if (new_rec_len < EXT2_BLOCK_SIZE) {
				i_entry->rec_len = current_size + 4 - (current_size % 4);
				//i_entry = (void *)i_entry + current_size;
				//i_entry->rec_len = EXT2_BLOCK_SIZE - (current_idx + current_size);
				return current_idx + i_entry->rec_len;
			}

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
	int idx = -1;
	if (prev_block != 0) {
		// Check to see if there is space in the previous entry
		idx = check_entry(prev_block, prev_idx, name, 1);

	}
	// Checks to see if the current block exists
	if (idx >= 0 || !block[block_idx]) {
		printf("entered\n");
		// Create the new inode for directory

		// Check the entries and see if we can add name to previous

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
		init_entry(block_no, idx, name, file_type);
		return block_no;
	}
	prev_block = block;
	prev_idx = block_idx;
	return -1;
}



//---------------------- Operations to create a new file ---------------------//
/*
 * Create a new file at the path 'path' with a file type 'file_type'
 */
int create_file(char * path, int file_type, char * link_to) {
	char file[EXT2_NAME_LEN];
	char dir[strlen(path)];
	split_path(path, file, dir);
	printf("path: %s, file: %s, dir: %s\n", path, file, dir);

	int dir_inode_no = path_walk(dir);
	if (dir_inode_no == -ENOENT || dir_inode_no == -ENOTDIR) {
		return dir_inode_no * -1;
	}

	// Checks to see if the file already exists
	unsigned int check_exist = check_directory(file, dir_inode_no, 0, &check_entry);
	if (check_exist != -1) {
			fprintf(stderr, "File with the same name exists\n");
			return EEXIST;
	}

	// add the inode to the dir_inode i_block and get the block_no
	unsigned int block_no = check_directory(file, dir_inode_no, file_type, &add_entry);
	if (block_no == -1) {
		fprintf(stderr, "Unable to create file\n");
		return ENOENT;
	}


	struct ext2_dir_entry_2 * i_entry = (struct ext2_dir_entry_2 *)(disk + block_no * block_size);
	int count = 0;
	while(count < EXT2_BLOCK_SIZE) {
		printf("create entry count %d\n", count);
		count += i_entry -> rec_len;
		i_entry = (void *) i_entry + i_entry -> rec_len;
	}
	int return_val = 0;
	if (link_to == NULL) {
		if (file_type == EXT2_FT_DIR){
			printf("directory\n");
			return_val = init_dir(block_no, dir_inode_no, file);
		}

		else if(file_type == EXT2_FT_REG_FILE) {
			printf("reg file\n");
			return_val = init_reg(block_no, file);
		}
	} else {
		printf("init link\n");
		return_val = init_link(block_no, file, file_type, link_to);
	}
	// initialize the directory inode
	return return_val;
}

/*
 * Create a new file at the block number 'block_no' with displacement bytes
 * where this entry corresponds to the inode with inode number 'inode_no'.
 * The name of the file is 'name' of file type 'file_type'
 */
void init_entry(int block_no, int displacement, char * name, int file_type){

	struct ext2_dir_entry_2 * i_entry = (struct ext2_dir_entry_2 *)(disk + block_no * block_size);
	i_entry = (void *)i_entry + displacement;
	i_entry->name_len = strlen(name);
	i_entry->file_type = file_type;
	i_entry->rec_len = block_size - displacement; //is this correct????
	strncpy(i_entry->name, name, EXT2_NAME_LEN);
	printf("file with name %s is on block %d at index %d\n", name, block_no, displacement);

	i_entry = (struct ext2_dir_entry_2 *)(disk + block_no * block_size);
}

/*
 * Creates a new inode at the inodenumber 'new_inode'
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




//------------------ Operations to manipulate the path string ----------------//
/*
 * Extracts the filename and dir from the path
 */
void split_path(char * path, char * name, char * dir) {
	int count = 0;
	int name_idx = 0;
	int dir_idx = 0;
	while (count < strlen(path)) {
		char file[EXT2_NAME_LEN];
		int file_idx = 0;
		while (count < strlen(path) && path[count] != '/' ) {
			file[file_idx] = path[count];
			file_idx++;
			count++;
		}
		file[file_idx] = '\0';
		// indicating the last file in the path

		if (path[count] != '/') {
			str_cat(name, file, &name_idx);
		} else {
			str_cat(dir, file, &dir_idx);
			(dir)[dir_idx] = '/';
			(dir)[dir_idx + 1] = '\0';
			dir_idx ++;
			count++;
		}

	}

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
}




//---------------------------- Bitmap Operations -----------------------------//

// MODIFIED
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
					return index + 1;
				}
			}
	}
	return -ENOMEM;
}

// MODIFIED
/*
 * Update the bitmap, and set the bit at index 'index' to 1
 */
void take_spot(unsigned char * bitmap, int index) {
	index -= 1;
	int bit_map_byte = index / 8;
	int bit_order = index % 8;
	if ((bitmap[bit_map_byte] >> bit_order) & 1) {
		fprintf(stderr, "trying to write to a taken spot\n");
		exit(1);
	}
	bitmap[bit_map_byte] = bitmap[bit_map_byte] | (1 << bit_order);
	if (bitmap == inode_bitmap) {
		descriptor->bg_free_inodes_count--;
		sb->s_free_inodes_count--;
	} else {
		descriptor->bg_free_blocks_count--;
		sb->s_free_blocks_count--;
	}
}

// MODIFIED
/*
 * Updates the bitmap and sets the bit ad index 'index' to 0
 */
void free_spot(unsigned char * bitmap, int index) {
	index -= 1;
	int bit_map_byte = index / 8;
	int bit_order = index % 8;
	bitmap[bit_map_byte] = bitmap[bit_map_byte] | ~(1 << bit_order);
	if (bitmap == inode_bitmap) {
		descriptor->bg_free_inodes_count++;
		sb->s_free_inodes_count++;
	} else {
		descriptor->bg_free_blocks_count++;
		sb->s_free_blocks_count++;
	}
}

/*
 * Prints out the bitmap
 */
void print_bitmap(int bitmap_size, unsigned char * bitmap){
	for (int i = 0; i < bitmap_size; i++) {

			/* Looping through each bit a byte. */
			for (int k = 0; k < 8; k++) {
					printf("%d", (bitmap[i] >> k) & 1);
			}
			printf(" ");
	}
}




//--------------------------- Initializing Files -----------------------------//
int init_dir(int block_no, int parent_inode_no, char * name) {
	// Creates the inode for this directory
	int new_inode_no = search_bitmap(inode_bitmap, i_bitmap_size);
	if (new_inode_no == -ENOMEM) {
		fprintf(stderr, "no space in inode bitmap\n");
		exit(1);
	}
	take_spot(inode_bitmap, new_inode_no);
	create_inode(new_inode_no);

	// Add the new_inode to the entry
	struct ext2_dir_entry_2 * i_entry = (struct ext2_dir_entry_2 *)(disk + block_no * block_size);
	int idx = find_entry(block_no, name);
	if (idx == -1) {
		fprintf(stderr, "could not find entry\n Should not be here!! \n");
		exit(1);
	}
	i_entry = (void *) i_entry + idx;
	i_entry->inode = new_inode_no;
	// int count = 0;
	// printf("checking block no %d for file %s\n", block_no, name);
	// int check_inode_no = i_entry ->inode;

	// while (count < EXT2_BLOCK_SIZE) {
	// 	if(i_entry->rec_len != 0) {
	// 		printf("rec_len %d\n", i_entry->rec_len);
	// 	}
	// 	if (!strcmp(i_entry->name, name)) {
	// 		printf("found at displacement %d\n", count);
	// 		i_entry->inode = new_inode_no;
	// 		printf("rec_len %d\n", i_entry->rec_len);
	// 		count = EXT2_BLOCK_SIZE;
	// 	}
	// 	count+= i_entry->rec_len;
	// 	if (count < EXT2_BLOCK_SIZE) {
	// 		i_entry = (void *)i_entry + i_entry->rec_len;
	// 	}
	// }

	struct ext2_inode * dir_inode = inode_table + (new_inode_no - 1);
	dir_inode->i_mode = 0;
	dir_inode->i_mode = dir_inode->i_mode | EXT2_S_IFDIR;
	dir_inode->i_dtime = 0;
	dir_inode->i_size = block_size; // HOW TO SET THE SIZE????
	(dir_inode->i_links_count)++;
	int new_block_no = search_bitmap(block_bitmap, b_bitmap_size);
	if (new_block_no == -ENOMEM) {
		fprintf(stderr, "no space in block bitmap\n");
		exit(1);
	}
	take_spot(block_bitmap, new_block_no);
	(dir_inode->i_block)[0] = new_block_no;
	printf("create new block %d\n", new_block_no);
	// Initializing the file '.'
	init_entry(new_block_no, 0, ".", EXT2_FT_DIR);
	struct ext2_dir_entry_2 * f_entry = (struct ext2_dir_entry_2 *)(disk + new_block_no * block_size);
	f_entry->inode = new_inode_no;
	f_entry->rec_len = 12;
	// Initializing the file '..'
	init_entry(new_block_no, sizeof(struct ext2_dir_entry_2) + 4, "..", EXT2_FT_DIR);
	f_entry = (void *)f_entry + 12;
	f_entry ->inode = parent_inode_no;


	struct ext2_inode * parent_inode = inode_table + (parent_inode_no - 1);
	parent_inode->i_links_count++;


	descriptor->bg_used_dirs_count++;
	return 0;
}


// MODIFIED
int init_link(int block_no, char * name, int file_type, char * link_to) {
	struct ext2_dir_entry_2 * i_entry = (struct ext2_dir_entry_2 *)(disk + block_no * block_size);

	int idx = find_entry(block_no, name);
	if (idx == -1) {
		fprintf(stderr, "could not find entry\n Should not be here!! \n");
		exit(1);
	}
	i_entry = (void *) i_entry + idx;

	// If we are initializing a symbolic link
	if (file_type == EXT2_FT_SYMLINK) {
		// Creates the inode for this directory
		int new_inode_no = search_bitmap(inode_bitmap, i_bitmap_size);
		if (new_inode_no == -ENOMEM) {
			fprintf(stderr, "no space in inode bitmap\n");
			exit(1);
		}
		take_spot(inode_bitmap, new_inode_no);
		create_inode(new_inode_no);
		i_entry->inode = new_inode_no;

		struct ext2_inode * new_inode = inode_table + (new_inode_no - 1);
		char * link_path = (char *) (new_inode -> i_block);

		//set name to absolute_path
		char name[60];
		int i = 0;
		int pos = 0;
		if (link_to[0] == '.') {
			name[0] = '/';
			name[1] = '\0';
			pos = 1;
			i = 2;
			if (link_to[1] != '/') {
				i = 1;
			}
		}
		if (strlen(link_to) + pos - i > 60) {
			fprintf(stderr, "Link path is too long. Cannot fit in i_blocks");
			return ENAMETOOLONG;
		}
		str_cat(name, link_to + i, &pos);
		i_entry->file_type = EXT2_FT_SYMLINK;
		//copy the name to the iblocks
		new_inode->i_size += sizeof(name);
		strncpy(link_path, name, 60);

	} else {
		int inode_no = path_walk(link_to);
		if (inode_no == -ENOENT || inode_no == -ENOTDIR) {
			return inode_no * -1;
		}
		struct ext2_inode * link_inode = inode_table + (inode_no - 1);
		(link_inode->i_links_count)++;
		i_entry->inode = inode_no;
		i_entry->file_type = EXT2_FT_REG_FILE;
	}
	//i_entry->inode = new_inode_no;
	//reg_inode->i_mode = reg_inode->i_mode | EXT2_S_IFREG;
	return 0;
}





int init_reg(int block_no, char * name){
	// Creates the inode for this directory
	int new_inode_no = search_bitmap(inode_bitmap, i_bitmap_size);
	if (new_inode_no == -ENOMEM) {
		fprintf(stderr, "no space in inode bitmap\n");
		exit(1);
	}
	take_spot(inode_bitmap, new_inode_no);
	create_inode(new_inode_no);

	struct ext2_dir_entry_2 * i_entry = (struct ext2_dir_entry_2 *)(disk + block_no * block_size);

	int idx = find_entry(block_no, name);
	if (idx == -1) {
		fprintf(stderr, "could not find entry\n Should not be here!! \n");
		exit(1);
	}
	i_entry = (void *) i_entry + idx;
	i_entry->inode = new_inode_no;
	//reg_inode->i_mode = reg_inode->i_mode | EXT2_S_IFREG;
	return 0;
}

/*
 * Return the index at which the file is found in the entry block
 */
int find_entry(int block_no, char * name){
	struct ext2_dir_entry_2 * i_entry = (struct ext2_dir_entry_2 *)(disk + block_no * block_size);
	int count = 0;
	printf("checking block no %d for file %s\n", block_no, name);

	while (count < EXT2_BLOCK_SIZE) {
		if(i_entry->rec_len != 0) {
			printf("rec_len %d\n", i_entry->rec_len);
		}
		if (!strcmp(i_entry->name, name)) {
			printf("found at displacement %d\n", count);
			return count;
		}
		count+= i_entry->rec_len;
		if (count < EXT2_BLOCK_SIZE) {
			i_entry = (void *)i_entry + i_entry->rec_len;
		}
	}
	return -1;
}
