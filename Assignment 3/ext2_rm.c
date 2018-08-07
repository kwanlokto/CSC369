#include "ext2.h"

extern unsigned char * disk;
int rm_entry_from_block(unsigned int * block, int block_idx, char * null, int rm_inode);
int delete_file(char * path, int rm_dir);
int rm_inode(int dir_inode_no);

int main(int argc, char ** argv){
	if (argc != 3){
		fprintf(stderr, "Error: Missing parameters. It requires 2 parameters\n");
		fprintf(stderr, "Usage: ext2_rm <disk.img> <file_path>\n");
		return EINVAL;
	}


	// ------------------------ convert the arguments -----------------------//
	unsigned char* virtual_disk = NULL;
	char * path = NULL;
	virtual_disk = (unsigned char*) argv[1];
	path = argv[2];



	//----------------------------- open the image -----------------------------//
	open_image(virtual_disk);

	//-------------------------- setup datastructures --------------------------//
	init_datastructures();

	//-------------------------- go to the paths inode -------------------------//
	int return_val = delete_file(path, 0);

	//------------------------- set the inode to be free -----------------------//


	return return_val;
}

int delete_file(char * path, int rm_dir){
	char file[EXT2_NAME_LEN];
	char dir[EXT2_PATH_LEN] = "/";
	split_path(path, file, dir);
	LOG(DEBUG_LEVEL0, "path: %s, file: %s, dir: %s\n", path, file, dir);

	unsigned int file_inode_no = path_walk(path);
	if (file_inode_no == -ENOENT) {
		fprintf(stderr, "Error: No such file or directory\n");
		return ENOENT;
	}

	struct ext2_inode * rm_inode = (struct ext2_inode *) inode_table + (file_inode_no - 1);
	if ((rm_dir == 0) && (rm_inode->i_mode & EXT2_S_IFDIR))
	{
		fprintf(stderr, "Error: File is a Directory\n");
		return EISDIR;
	}

	unsigned int dir_inode_no = path_walk(dir);
	if (dir_inode_no == -ENOENT) {
		fprintf(stderr, "Error: No such file or directory\n");
		return ENOENT;
	}


	LOG(DEBUG_LEVEL0, "before removing\n");
	int return_val = check_directory(file, dir_inode_no, rm_dir, &rm_entry_from_block);
	if (return_val == -1) {
		fprintf(stderr, "Error: No such file or directory\n");
		return ENOENT;
	}
	LOG(DEBUG_LEVEL0, "finish removing\n");

	return 0;
}

/*
 * Recursively remove all files in the subdirectories
 * Returns 0 if sucessful and other values if unsucessful
 */
int rm_inode(int dir_inode_no){
	struct ext2_inode * dir_inode = inode_table + (dir_inode_no - 1);
	unsigned int * dir_iblocks = dir_inode->i_block;

	for (int i = 0; i < 12; i++) { //Direct blocks
		free_spot(block_bitmap, dir_iblocks[i]);
	}

	unsigned int * singly_indirect_block = (unsigned int *)(disk + dir_iblocks[12] * block_size);
	if (dir_iblocks[12]) {
		for (int i = 0; i < 256; i++) {
			free_spot(block_bitmap, singly_indirect_block[i]);
		}
		free_spot(block_bitmap, dir_iblocks[12]);
	}

	unsigned int * doubly_indirect_block = (unsigned int *)(disk + dir_iblocks[13] * block_size);
	if (dir_iblocks[13]) {
		for (int i = 0; i < 257; i++) {

			singly_indirect_block = (unsigned int *)(disk + doubly_indirect_block[i] * block_size);
			if (doubly_indirect_block[i]) {
				for (int j = 0; j < 257; j++) {
					free_spot(block_bitmap, singly_indirect_block[j]);
				}
				free_spot(block_bitmap, doubly_indirect_block[i]);
			}
		}
		free_spot(block_bitmap, dir_iblocks[13]);
	}

	unsigned int * triply_indirect_block = (unsigned int *)(disk + dir_iblocks[14] * block_size);
	if (dir_iblocks[14]) {
		for (int i = 0; i < 257; i++) {

			doubly_indirect_block = (unsigned int *)(disk + triply_indirect_block[i] * block_size);
			if (triply_indirect_block[i]) {
				for (int j = 0; j < 257; j++) {

					singly_indirect_block = (unsigned int *)(disk + doubly_indirect_block[j] * block_size);
					if (doubly_indirect_block[j]) {
						for (int k = 0; k < 257; k++) {
							free_spot(block_bitmap, singly_indirect_block[k]);
						}
						free_spot(block_bitmap, doubly_indirect_block[j]);
					}
				}
				free_spot(block_bitmap, triply_indirect_block[i]);
			}
		}
		free_spot(block_bitmap, dir_iblocks[14]);
	}

	dir_inode->i_blocks = 0;
	free_spot(inode_bitmap, dir_inode_no);
	return 0;
}


/*
 * Removes the entry with the same name as name
 */
int rm_entry_from_block(unsigned int * block, int block_idx, char * name, int rm_dir) {

	int block_no = block[block_idx];
	if (block_no != 0) {
		struct ext2_dir_entry_2 * i_entry = (struct ext2_dir_entry_2 *)(disk + block_no * block_size);

		int count = 0;
		int prev_count = 0;
		int new_rec_len = -1;
		int idx = -1;

		//traverse to the entry with the same inode
		while (count < EXT2_BLOCK_SIZE) {
			// Check to see if we have found the corresponding entry
			char * curr_name = get_name(i_entry->name, i_entry->name_len);
			LOG(DEBUG_LEVEL0, "curr_name %s vs %s\n", curr_name, name);
			if (!strcmp(curr_name, name)) {
				idx = prev_count;
				LOG(DEBUG_LEVEL0, "found entry wtih name %s at idx %d \n", name, idx);
				new_rec_len = (count - prev_count) + i_entry->rec_len;
				count = EXT2_BLOCK_SIZE;
			}
			else {
				prev_count = count;
				count+= i_entry->rec_len;
				i_entry = (struct ext2_dir_entry_2 *)((char *)i_entry + i_entry->rec_len);
			}
			free(curr_name);
		}

		// If the entry is found
		// If we are removing then entry at the start of the block
		if (idx >= 0) {
			struct ext2_dir_entry_2 * prev_entry = (struct ext2_dir_entry_2 *)(disk + block_no * block_size);
			prev_entry = (struct ext2_dir_entry_2 *)((char *)prev_entry + idx);

			struct ext2_dir_entry_2 * curr_entry = (struct ext2_dir_entry_2 *)((char *)prev_entry + prev_entry->rec_len);

			struct ext2_inode * curr_inode = inode_table + (curr_entry->inode - 1);

			(curr_inode->i_links_count)--;
			//If no more links to this inode remove it
			//Or if this inode is for a directory remove it
			if (!curr_inode->i_links_count) {
				curr_inode->i_dtime = get_curr_time();
				curr_inode->i_size = 0;
				curr_inode->i_blocks = 0;
				rm_inode(curr_entry->inode);
			}
			curr_entry->inode = 0;
			curr_entry->name_len = 0;
			prev_entry->rec_len = new_rec_len;
			return 0;
		}
		//prev_block_no = block_no;
	}
	return -1;
}
