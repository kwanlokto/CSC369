#include "ext2.h"


void create_inode(int free_inode);
int create_file(unsigned int dir_inode_no, char * path);

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
	unsigned int dir_inode_no;
	if (!(dir_inode_no = path_walk(path))) {
		fprintf(stderr, "not a valid path\n");
		return -ENOENT;
	}

	return create_file(dir_inode_no, path);

}

int create_file(unsigned int dir_inode_no, char * path) {
	char file[EXT2_NAME_LEN];
	char dir[strlen(path)];
	split_path(path, file, dir);
	printf("path: %s, file: %s, dir: %s", path, file, dir);

	// Checks to see if the file already exists
	unsigned int inode_no = check_directory(file, dir_inode_no, 1, &check_entry);
	if (inode_no) {
		struct ext2_inode * check_inode = inode_table + (inode_no - 1);
		if (check_inode->i_mode & EXT2_S_IFDIR) {
			fprintf(stderr, "directory already exists\n");
			return -EEXIST;
		}
	}

	int free_inode;
	if ((free_inode= search_bitmap(inode_bitmap, i_bitmap_size)) == -ENOMEM) {
		fprintf(stderr, "no free inode space\n");
		return -ENOMEM;
	}
	take_spot(inode_bitmap, free_inode);

	create_inode(free_inode);



	return 0;
}
