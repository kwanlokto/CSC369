#include "ext2.h"

extern unsigned char * disk;

int main(int argc, char ** argv){
	if (argc < 3 || argc > 4){
		fprintf(stderr, "ls command requires 2 arguments\n");
		exit(1);
	}


	// ------------------------ convert the arguments -----------------------//
	char * virtual_disk = NULL;
	char * path = NULL;
	int check_all = 0;
	//check if flag -a not specified
	if (argc == 3) {
		virtual_disk = argv[1];
		path = argv[2];
	} else if (!strcmp(argv[2], "-a")){
		check_all = 1;
		virtual_disk = argv[1];
		path = argv[3];
	} else {
		fprintf(stderr, "unknown flag specified");
		exit(1);
	}


	//---------------------------- open the image ---------------------------//
	open_image(virtual_disk);


	//--------------------------- setup datastructures -----------------------//
	init_datastructures();


	//---------------------------- go to the paths inode ----------------------//
	unsigned int inode_no;
	if (!(inode_no = path_walk(path))) {
		fprintf(stderr, "not a valid path\n");
		exit(1);
	}



	//-------------- PRINT ALL FILES LISTED IN THAT DIRECTORY ----------------//
	struct ext2_inode * dir = inode_table + (inode_no - 1);
	if (!(dir->i_mode & EXT2_S_IFDIR)) {
		fprintf(stderr, "working on a file\n");
		exit(1);
	}
	check_directory(NULL, inode_no, check_all, &print_file);

}
