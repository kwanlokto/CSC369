#include "ext2.h"

int main(int argc, char ** argv){
	if (argc != 4) {
		fprintf(stderr, "Requires 3 args\n");
		exit(1);
	}

	char * virtual_disk = argv[1];
	char * file_path = argv[2];
	char * dir_path = argv[3];

	// Error checking on the 2nd and 3rd argument making sure that they
	// are valid arguments
	if (!strlen(file_path) || file_path[strlen(file_path) - 1] == '/') {
		fprintf(stderr, "Provide a valid file\n");
		exit(1);
	}

	if (!strlen(dir_path) || dir_path[strlen(dir_path) - 1] != '/') {
		fprintf(stderr, "Provide a valid path\n");
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

	printf("After path walk");
	struct ext2_inode * dir = inode_table + (dir_inode_no - 1);
	if (!(dir->i_mode & EXT2_S_IFDIR)) {
		fprintf(stderr, "Entered directory path does not exist\n");
		exit(1);
	}

	//---------------------------- open and read the file -------------------------//
	FILE * file = fopen(file_path, "r");
	char * buf;

	if (file != NULL) {
		while (fread(buf, EXT2_BLOCK_SIZE, 1, file) > 0){
			for (int i = 0; i < EXT2_BLOCK_SIZE; i++) {
				printf("%c", buf[i]);
			}
			//write_file(buf);
		}

	} else {
		fprintf(stderr, "File does not exist\n");
		exit(1);
	}


	fclose(file);

}
