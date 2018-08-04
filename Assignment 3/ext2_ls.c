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
		fprintf(stderr, "unknown flag specified\n");
		return ENOENT;
	}

	//----------------------------- open the image -----------------------------//
	open_image(virtual_disk);

	//-------------------------- setup datastructures --------------------------//
	init_datastructures();

	//-------------------------- go to the paths inode -------------------------//
	int inode_no = path_walk(path);
	if (inode_no == -ENOENT) {
		return inode_no * -1;
	}

	//----------------- PRINT ALL FILES LISTED IN THAT DIRECTORY ---------------//
	struct ext2_inode * dir_inode = inode_table + (inode_no - 1);
	if (!(dir_inode->i_mode & EXT2_S_IFDIR)) {
		char name[EXT2_NAME_LEN];
		char dir[strlen(path)];
		split_path(path, name, dir);
		printf("%s\n",name);
		return 0;
	}
	int return_val = check_directory(NULL, inode_no, check_all, &print_file);
	if (return_val != -1) {
		fprintf(stderr, "Didn't print all files\n");
		return ENOENT;
	}


	return 0;
}
