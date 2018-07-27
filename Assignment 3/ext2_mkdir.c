#include "ext2.h"


int create_file(char * path, int file_type);
void init_dir(int dir_inode_no, int parent_inode_no);
void create_inode(int new_inode_no, int file_type, void * info);

int main(int argc, char ** argv) {
	if (argc != 3) {
		fprintf(stderr, "Requires 2 args\n");
		exit(1);
	}

	char * virtual_disk = argv[1];
	char * path = argv[2];

	//---------------------------- open the image ---------------------------//
	open_image(virtual_disk);


	//--------------------------- setup datastructures -----------------------//
	init_datastructures();


	//---------------------------- go to the paths inode ----------------------//


	return create_file(path, EXT2_FT_DIR);

}

int create_file(char * path, int file_type) {
	char file[EXT2_NAME_LEN];
	char dir[strlen(path)];
	printf("split\n");
	split_path(path, file, dir);
	printf("path: %s, file: %s, dir: %s", path, file, dir);

	unsigned int dir_inode_no;
	if (!(dir_inode_no = path_walk(dir))) {
		fprintf(stderr, "not a valid path\n");
		return -ENOENT;
	}

	// Checks to see if the file already exists
	unsigned int inode_no = check_directory(file, dir_inode_no, 1, &check_entry);
	if (inode_no) {
		struct ext2_inode * dir_inode = inode_table + (inode_no - 1);
		if (dir_inode->i_mode & EXT2_S_IFDIR) {
			fprintf(stderr, "directory already exists\n");
			return -EEXIST;
		}
	}

	// add the inode to the dir_inode i_block
	unsigned int new_inode_no = check_directory(file, dir_inode_no, file_type, &add_entry);
	if (!new_inode_no) {
		fprintf(stderr, "unable to add the file\n");
		return -ENOENT;
	}

	create_inode(new_inode_no, EXT2_FT_DIR, &dir_inode_no);
	// initialize the directory inode
	return 0;
}



void init_reg(int reg_inode_no){
	struct ext2_inode * reg_inode = inode_table + (reg_inode_no - 1);
	reg_inode->i_mode = reg_inode->i_mode | EXT2_S_IFREG;
}

void init_link(int link_inode_no) {
	struct ext2_inode * link_inode = inode_table + (link_inode_no - 1);
	link_inode->i_mode = link_inode->i_mode | EXT2_S_IFLNK;
}

void init_dir(int dir_inode_no, int parent_inode_no) {
	struct ext2_inode * dir_inode = inode_table + (dir_inode_no - 1);
	dir_inode->i_mode = dir_inode->i_mode | EXT2_S_IFDIR;
	int block_no = search_bitmap(block_bitmap, b_bitmap_size);
	if (block_no == -ENOMEM) {
		fprintf(stderr, "no space in block bitmap\n");
		exit(1);
	}
	take_spot(block_bitmap, block_no);
	(dir_inode->i_block)[0] = block_no;

	// Initializign the file '.'
	create_new_entry(block_no, dir_inode_no, 0, ".", EXT2_FT_DIR);

	// Initializing the file '..'
	create_new_entry(block_no, parent_inode_no, sizeof(struct ext2_dir_entry_2), "..", EXT2_FT_DIR);

}
