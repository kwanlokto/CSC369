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
	bool flag = false;
	//check if flag -a not specified
	if (argc == 3) {
		virtual_disk = argv[1];
		path = argv[2];
	} else if (strcmp(argv[2], "-a") == 0){
		flag = true;
		virtual_disk = argv[2];
		path = argv[3];
	} else {
		fprintf(stderr, "unknown flag specified");
		exit(1);
	}


	//---------------------------- open the image ---------------------------//
	open_image(virtual_disk);


	//--------------------------- setup datastructures ----------------------//
	init_datastructures();


	//--------------------m-------- go to the paths inode ----------------------//
	unsigned int inode_no;
	if (!(inode_no = path_walk(path))) {
		fprintf(stderr, "not a valid path\n");
		exit(1);
	}



	//-------------- PRINT ALL FILES LISTED IN THAT DIRECTORY ----------------//
	check_directory(NULL, inode_no, &print_file);

}
