#include "ext2.h"

int main(int argc, char ** argv){
	if (argc != 3) {
		fprintf("Requires 3 args\n");
		exit(1);
	}

	char * virtual_disk = argv[1];
	char * file_path = arg[2];
	char * dir_path = arg[3];

	// Error checking on the 2nd and 3rd argument making sure that they
	// are valid arguments
	if (!strlen(file_path) || file_path[strlen(file_path) - 1] == '/') {
		fprintf("Provide a valid file\n");
		exit(1);
	}

	if (!strlen(dir_path) || dir_path[strlen(dir_path) - 1] != '/') {
		fprintf("Provide a valid path\n")
		exit(1);
	}

	//--------------------------- open the image ------------------------------//
	open_image(virtual_disk);

	init_datastructures();

	//---------------------------- go to the directory inode ----------------------//
	unsigned int dir_inode_no;
	if (!(dir_inode_no = path_walk(dir_path))) {
		fprintf(stderr, "Directory does not exist\n");
		exit(1);
	}

	struct ext2_inode * dir = inode_table + (dir_inode_no - 1);
	if (!(dir->i_mode & EXT2_S_IFDIR)) {
		fprintf(stderr, "Entered directory path does not exist\n");
		exit(1);
	}

	//---------------------------- open and read the file -------------------------//
	FILE * file = fopen(file_path, "r");
	if (file != NULL) {

	} else {
		fprintf("File does not exist\n");
		exit(1);
	}
}
