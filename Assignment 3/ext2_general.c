#include "ext2.h"

//------------------------ Constants -------------------------//
#define FS_SIZE (128*1024)

//------------------------ GLOBAL VARIABLES -------------------------//
unsigned char * disk;
unsigned int block_size;
struct ext2_inode * inode_table;
struct ext2_super_block * sb;
struct ext2_group_desc * descriptor;
struct ext2_dir_entry_2* root_directory;
unsigned char * inode_bitmap;
unsigned char * block_bitmap;
int i_bitmap_size;
int b_bitmap_size;

//WIN32 was used to develop and test in Windows until Linux was available.

#ifdef WIN32
FILE* fimg;
#endif

//------------------------ Functions -------------------------//


/*
 * Initializes FS global variables
 * Referenced from https://wiki.osdev.org/Ext2
 * and http://www.nongnu.org/ext2-doc/ext2.html#S-LOG-BLOCK-SIZE
*/
void init_datastructures() {
	TRACE("%s\n", __func__);
	sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
	block_size = EXT2_BLOCK_SIZE << (sb -> s_log_block_size);
	descriptor = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE + block_size);
	//block number where the inode table starts
	//Reference from the table 3-1 in nongnu.org
	inode_table = (struct ext2_inode *)(disk + descriptor -> bg_inode_table * block_size);
	inode_bitmap = disk + descriptor -> bg_inode_bitmap * block_size;
	block_bitmap = disk + descriptor -> bg_block_bitmap * block_size;
	root_directory = (struct ext2_dir_entry_2*)(disk + inode_table[EXT2_ROOT_INO-1].i_block[0]* EXT2_BLOCK_SIZE);
	i_bitmap_size = (sb->s_inodes_count)/(sizeof(unsigned char) * 8);
	b_bitmap_size = (sb->s_blocks_count)/(sizeof(unsigned char) * 8);
	LOG("inode_bitmap: ");
	print_bitmap(i_bitmap_size, inode_bitmap);
	LOG("\n");
	LOG("block_bitmap: ");
	print_bitmap(b_bitmap_size, block_bitmap);
	LOG("\n");
}


#ifdef WIN32

//Load Image FS in the memory
void open_image(char * virtual_disk) {
	TRACE("%s\n", __func__);
	FILE* fd = fopen(virtual_disk, "r+b");
	if (fd == 0)
	{
		fprintf(stderr, "Error: Open image file failed\n");
		exit(3);
	}

	disk = malloc(FS_SIZE);
	if (disk == NULL) {
		fprintf(stderr, "Error: Memory allocation failed\n");
		fclose(fd);
		exit(1);
	}
	int ret = fread(disk, FS_SIZE, 1, fd);
	if (ret != 1)
	{
		free(disk);
		fprintf(stderr, "Error: Load image file failed\n");
		fclose(fd);
		exit(2);
	}
	fclose(fd);
}

void save_image(char * virtual_disk) {
	TRACE("%s\n", __func__);
	FILE* fd = fopen(virtual_disk, "w+b");
	if (fd == 0)
	{
		free(disk);
		fprintf(stderr, "Error: Open image file failed\n");
		exit(3);
	}
	int ret = fwrite(disk, FS_SIZE, 1, fd);
	if (ret != 1)
	{
		free(disk);
		fprintf(stderr, "Error: Save image file failed\n");
		fclose(fd);
		exit(2);
	}
	fclose(fd);
}

//Close FS image and Free FS memeory
void close_image(char * virtual_disk) {
	TRACE("%s\n", __func__);
	free(disk);
}


#else
//Load Image FS in the memory
void open_image(char * virtual_disk) {
	TRACE("%s\n", __func__);
	int fd = open(virtual_disk, O_RDWR);

	disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(disk == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}
}

//Close FS image and Free FS memeory
void close_image(char * virtual_disk) {
	TRACE("%s\n", __func__);

}

#endif


/*
 * Returns the inode number of file that is at the end of the path.
 * If the file is not found then return 0
 * Referenced from https://www.geeksforgeeks.org/how-to-split-a-string-in-cc-python-and-java/
*/
unsigned int path_walk(char * path) {
	TRACE("%s\n", __func__);

	char path_c[EXT2_PATH_LEN];
	strcpy(path_c, path);
	char name[EXT2_NAME_LEN];
	int name_idx = 0;
	int path_idx = 0;
	int inode_no = EXT2_ROOT_INO;
	bool is_dir = true;
	struct ext2_inode * curr;
	while(path_idx < strlen(path_c)) {

		// Get the name of the next file
		name_idx = 0;
		is_dir = false;
		while(path_idx < strlen(path_c) && path_c[path_idx] != '/') {
			name[name_idx] = path_c[path_idx];
			name_idx++;
			path_idx++;
			if (path_c[path_idx] == '/') {
				is_dir = true;
			}
		}
		name[name_idx] = '\0';
		inode_no = check_directory(name, inode_no, 0, &check_entry);
		if (!inode_no) {
			LOG(stderr, "Path does not exist\n");
			return -ENOENT;
		}
		curr = inode_table + (inode_no - 1);
		if (!(curr->i_mode & EXT2_S_IFDIR) && is_dir) {
			LOG(stderr, "That is not a directory\n");
			return -ENOTDIR;
		}
		path_idx++;
	}
	return inode_no;
}


/*
 * Checks to see whether or not the file with file name 'name' is in the
 * current working directory
 * Returns the inode_no number if found. Otherwise return 0
 */
int check_directory(char * name, unsigned int inode_no, int flag, int (*fun_ptr)(unsigned int *, int, char *, int)){
	TRACE("%s\n", __func__);
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
				LOG("Now gonna check 12th entry -- singly indirects \n");
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
				LOG("Now gonna check 13th entry -- doubly indirects \n");
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
					LOG("Now gonna check 14th entry -- triply indirects \n");
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
	return 0;
}

/*
* Returns the current_block using the singly indirect table
* if it is found. Otherwise return the block that is empty
 */
unsigned int * find_singly_indirect(unsigned int * block, int block_no , int * index){
	TRACE("%s\n", __func__);
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
	TRACE("%s\n", __func__);
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
	TRACE("%s\n", __func__);
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
int print_file(unsigned int * block, int block_idx, char * null, int include_all) {
	TRACE("%s\n", __func__);
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
			i_entry = (struct ext2_dir_entry_2 *)((unsigned int)i_entry + i_entry->rec_len);
		}
	}
	return -1;

}

/*
 * If checking_free == 0, checks to see if if the block contains information
 * about a file of the name 'name', and returns that block number.
 * Return -1 if not found and no errors
 *
 * If checking_free == 1, checks to see if the block contains a free spot
 * and returns that block number. Return -1 if no free blocks.
 */
int check_entry(unsigned int * block, int block_idx, char * name, int checking_free){
	TRACE("%s\n", __func__);
	int block_no = block[block_idx];
	if (block_no != 0) {

		struct ext2_dir_entry_2 * i_entry = (struct ext2_dir_entry_2 *)(disk + block_no * block_size);
		int inode_no = i_entry->inode;
		int count = 0;
		while (inode_no != 0 && count < EXT2_BLOCK_SIZE) {
			if (!checking_free) { //If we are not looking for a free spot
				if (!strcmp(i_entry->name, name)) {
					return i_entry->inode;
				}
			}
			inode_no = i_entry->inode;
			if (inode_no){
				count+= i_entry->rec_len;
				i_entry = (struct ext2_dir_entry_2 *)((unsigned int)i_entry + i_entry->rec_len);
			}

		}

		if (checking_free && count < EXT2_BLOCK_SIZE) {
			int current_idx = count - i_entry->inode;
			int current_size = sizeof(struct ext2_dir_entry_2) + i_entry->name_len;
			int adding_size = sizeof(struct ext2_dir_entry_2) + strlen(name);
			if (current_idx + current_size + adding_size < EXT2_BLOCK_SIZE) {
				i_entry->rec_len = current_size;
				return current_idx + current_size;
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
	TRACE("%s\n", __func__);
	static unsigned int * prev_block = 0;
	static unsigned int prev_idx = 0;
	int idx = -1;
	if (prev_block != 0) {
		// Check to see if there is space in the previous entry
		idx = check_entry(prev_block, prev_idx, name, 1);
	}
 	// Checks to see if the current block exists
 	if (idx >= 0 || !block[block_idx]) {

		// Create the new inode for directory
		int new_inode_no = search_bitmap(inode_bitmap, i_bitmap_size);
		if (new_inode_no == -ENOMEM) {
			fprintf(stderr, "no space in inode bitmap\n");
			exit(1);
		}
		take_spot(inode_bitmap, new_inode_no);
		create_inode(new_inode_no);
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
		create_new_entry(block_no, new_inode_no, idx, name, file_type);
		return new_inode_no;
	}
	prev_block = block;
	prev_idx = block_idx;
	return -1;
}
// int add_entry(unsigned int * block, int block_idx, char * name, int file_type) {
// 	static unsigned int * prev_block = 0;
// 	static unsigned int prev_idx = 0;
//
// 	// Checks to see if the current block exists
// 	if (!block[block_idx]) {
//
// 		// Create the new inode for directory
// 		int new_inode_no = search_bitmap(inode_bitmap, i_bitmap_size);
// 		if (new_inode_no == -ENOMEM) {
// 			fprintf(stderr, "no space in inode bitmap\n");
// 			exit(1);
// 		}
// 		take_spot(inode_bitmap, new_inode_no);
// 		create_inode(new_inode_no);
// 		// Check the entries and see if we can add name to previous
// 		int idx = check_entry(prev_block, prev_idx, name, 1);
//
// 		int block_no = prev_block[prev_idx];
// 		if (idx < 0) { //If there is no space in the previous block
// 			int singly_block_no = search_bitmap(block_bitmap, b_bitmap_size);
// 			if (singly_block_no == -ENOMEM) {
// 				fprintf(stderr, "no space in block bitmap\n");
// 				exit(1);
// 			}
// 			take_spot(block_bitmap, singly_block_no);
// 			block[block_idx] = singly_block_no;
// 			block_no = singly_block_no;
// 			idx = 0;
// 		}
//
// 		// Create the new entry
// 		create_new_entry(block_no, new_inode_no, idx, name, file_type);
// 		return new_inode_no;
// 	}
// 	prev_block = block;
// 	prev_idx = block_idx;
// 	return -1;
// }

//---------------------- Operations to create a new file ---------------------//
/*
 * Create a new file at the path 'path' with a file type 'file_type'
 */
int create_file(char * path, int file_type) {
	TRACE("%s\n", __func__);
	char file[EXT2_NAME_LEN];
	char dir[EXT2_PATH_LEN];
	split_path(path, file, dir);
	LOG("path: %s, file: %s, dir: %s\n", path, file, dir);

	int dir_inode_no = path_walk(dir);
	if (dir_inode_no == -ENOENT || dir_inode_no == -ENOTDIR) {
		return dir_inode_no * -1;
	}

	// Checks to see if the file already exists
	unsigned int dir_inode = check_directory(file, dir_inode_no, 0, &check_entry);
	if (dir_inode) {

			fprintf(stderr, "File with the same name exists\n");
			return EEXIST;
	}

	// add the inode to the dir_inode i_block
	unsigned int file_inode_no = check_directory(file, dir_inode_no, file_type, &add_entry);
	if (!file_inode_no) {
		fprintf(stderr, "Unable to create file\n");
		return ENOENT;
	}

	if (file_type == EXT2_FT_DIR){
		LOG("directory\n");
		init_dir(file_inode_no, dir_inode_no);
	}
	// initialize the directory inode
	return 0;
}

/*
 * Create a new file at the block number 'block_no' with displacement bytes
 * where this entry corresponds to the inode with inode number 'inode_no'.
 * The name of the file is 'name' of file type 'file_type'
 */
void create_new_entry(int block_no, int inode_no, int displacement, char * name, int file_type){
	TRACE("%s\n", __func__);
	struct ext2_dir_entry_2 * i_entry = (struct ext2_dir_entry_2 *)(disk + block_no * block_size);
	i_entry = (struct ext2_dir_entry_2 *)((unsigned int)i_entry + displacement);
	i_entry->inode = inode_no;
	i_entry->name_len = strlen(name);
	i_entry->file_type = file_type;
	// if (size < 12) {
	// 	size = 12;
	// }
	i_entry->rec_len = block_size - displacement; //is this correct????
	strncpy(i_entry->name, name, EXT2_NAME_LEN);
}

/*
 * Creates a new inode at the inodenumber 'new_inode'
 */
void create_inode(int new_inode_no){

	TRACE("%s\n", __func__);
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
	TRACE("%s\n", __func__);
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
	TRACE("%s\n", __func__);
	int count = 0;
	while (count < strlen(substring)) {
		(str)[*index] = substring[count];
		(*index)++;
		count++;
	}
	(str)[*index] = '\0';
}




//---------------------------- Bitmap Operations -----------------------------//
/*
 * Searches the entire bitmap and returns the index of the bitmap with a free
 * spot. If no free spot is found return -ENOMEM
 */
int search_bitmap(unsigned char * bitmap, int max) {
	TRACE("%s\n", __func__);
	for (int i = 0; i < max; i++) {

			/* Looping through each bit a byte. */
			for (int k = 0; k < 8; k++) {
				int bit = (bitmap[i] >> k) & 1;
				if (!bit) {
					int index = i * 8 + k;
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
	TRACE("%s\n", __func__);
	int bit_map_byte = index / 8;
	int bit_order = index % 8;
	if ((bitmap[bit_map_byte] >> bit_order) & 1) {
		fprintf(stderr, "trying to write to a taken spot\n");
		exit(1);
	}
	bitmap[bit_map_byte] = bitmap[bit_map_byte] | (1 << bit_order);
}

/*
 * Updates the bitmap and sets the bit ad index 'index' to 0
 */
void free_spot(unsigned char * bitmap, int index) {
	TRACE("%s\n", __func__);
	int bit_map_byte = index / 8;
	int bit_order = index % 8;
	bitmap[bit_map_byte] = bitmap[bit_map_byte] | ~(1 << bit_order);
}

/*
 * Prints out the bitmap
 */
void print_bitmap(int bitmap_size, unsigned char * bitmap){
	TRACE("%s\n", __func__);
	for (int i = 0; i < bitmap_size; i++) {

			/* Looping through each bit a byte. */
			for (int k = 0; k < 8; k++) {
					LOG("%d", (bitmap[i] >> k) & 1);
			}
			LOG(" ");
	}
}




//--------------------------- Initializing Files -----------------------------//
void init_dir(int dir_inode_no, int parent_inode_no) {
	TRACE("%s\n", __func__);
	struct ext2_inode * dir_inode = inode_table + (dir_inode_no - 1);
	dir_inode->i_mode = dir_inode->i_mode | EXT2_S_IFDIR;
	int block_no = search_bitmap(block_bitmap, b_bitmap_size);
	if (block_no == -ENOMEM) {
		fprintf(stderr, "no space in block bitmap\n");
		exit(1);
	}
	take_spot(block_bitmap, block_no);
	(dir_inode->i_block)[0] = block_no;

	// Initializing the file '.'
	create_new_entry(block_no, dir_inode_no, 0, ".", EXT2_FT_DIR);
	struct ext2_dir_entry_2 * i_entry = (struct ext2_dir_entry_2 *)(disk + block_no * block_size);
	i_entry->rec_len = 12;
	// Initializing the file '..'
	create_new_entry(block_no, parent_inode_no, sizeof(struct ext2_dir_entry_2) + 4, "..", EXT2_FT_DIR);

}


void init_link(int link_inode_no, int file_inode_no) {
	TRACE("%s\n", __func__);
	struct ext2_inode * link_inode = inode_table + (link_inode_no - 1);
	link_inode->i_mode = link_inode->i_mode | EXT2_S_IFLNK;
}


void init_reg(int reg_inode_no){
	TRACE("%s\n", __func__);
	struct ext2_inode * reg_inode = inode_table + (reg_inode_no - 1);
	reg_inode->i_mode = reg_inode->i_mode | EXT2_S_IFREG;
}
