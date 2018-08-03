#include "ext2.h"

extern unsigned char * disk;
int rm_entry_from_block(unsigned int * block, int block_idx, char * null, int rm_inode);

int main(int argc, char ** argv){
	if (argc < 3 || argc > 4){
		fprintf(stderr, "rm command requires at least 2 arguments\n");
		exit(1);
	}


	// ------------------------ convert the arguments -----------------------//
	char * virtual_disk = NULL;
	char * path = NULL;
	int rm_dir = 0;
	//check if flag -a not specified
	if (argc == 3) {
		virtual_disk = argv[1];
		path = argv[2];
	} else if (!strcmp(argv[2], "-r")){
		rm_dir = 1;
		virtual_disk = argv[1];
		path = argv[3];
	} else {
		fprintf(stderr, "unknown flag specified");
		return -ENOENT;
	}


	//----------------------------- open the image -----------------------------//
	open_image(virtual_disk);

	//-------------------------- setup datastructures --------------------------//
	init_datastructures();

	//-------------------------- go to the paths inode -------------------------//
	int inode_no = path_walk(path);
	if (inode_no == -ENOENT || inode_no == -ENOTDIR) {
		return inode_no * -1;
	}

	//------------------------- set the inode to be free -----------------------//
	free_spot(inode_bitmap, inode_no);
	struct ext2_inode * rm_inode = (struct ext2_inode *) inode_table + (inode_no - 1);
	rm_inode->i_links_count = 0;
	rm_inode->i_dtime = 1; // whats the format????
	return 0;
}

int delete_file(char * path, int rm_dir){
	char file[EXT2_NAME_LEN];
	char dir[strlen(path)];
	split_path(path, file, dir);
	printf("path: %s, file: %s, dir: %s\n", path, file, dir);

	unsigned int file_inode_no;
	if (!(file_inode_no = path_walk(path))) {
		fprintf(stderr, "Not a valid path\n");
		return ENOENT;
	}

	unsigned int dir_inode_no;
	if (!(dir_inode_no = path_walk(dir))) {
		fprintf(stderr, "Not a valid path\n");
		return ENOENT;
	}

	check_directory(NULL, dir_inode_no, file_inode_no, &rm_entry_from_block);

	return 0;
}

int rm_entry_from_block(unsigned int * block, int block_idx, char * null, int rm_inode) {
	static int prev_block_no = 0;

	int block_no = block[block_idx];
	if (block_no != 0) {
		struct ext2_dir_entry_2 * i_entry = (struct ext2_dir_entry_2 *)(disk + block_no * block_size);

		int count = 0;
		int prev_count = 0;
		int new_rec_len = -1;
		int idx = -1;

		while (count < EXT2_BLOCK_SIZE) {
			// Check to see if we have found the corresponding entry
			if (i_entry->inode == rm_inode) {
				idx = prev_count;
				new_rec_len = (count - prev_count) + i_entry->rec_len;
			}

			prev_count = count;
			count+= i_entry->rec_len;
			i_entry = (void *)i_entry + i_entry->rec_len;
		}

		// If the entry is found
		// If we are removing then entry at the start of the block
		if (idx == 0) { //What happens when this is the case???
			struct ext2_dir_entry_2 * entry = (struct ext2_dir_entry_2 *)(disk + prev_block_no * block_size);
			count = entry -> rec_len;;
			while (count < EXT2_BLOCK_SIZE) {
				entry = (void *) entry + entry -> rec_len;
				count += entry -> rec_len;
			}
			//entry->rec_len += del_rec_len; //What if missing a block in the middle???

		}
		else if(idx > 0) {
			struct ext2_dir_entry_2 * entry = (struct ext2_dir_entry_2 *)(disk + block_no * block_size);
			entry = (void *)entry + idx;
			entry->rec_len = new_rec_len;
		}
		prev_block_no = block_no;
	}
	return -1;
}
