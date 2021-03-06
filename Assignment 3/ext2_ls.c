/*
ext2_ls: This program takes two command line arguments. The first is the name of an ext2 formatted virtual disk. The second is an absolute path on the ext2 formatted disk. The program should work like ls -1 (that's number one "1", not lowercase letter "L"), printing each directory entry on a separate line. If the flag "-a" is specified (after the disk image argument), your program should also print the . and .. entries. In other words, it will print one line for every directory entry in the directory specified by the absolute path. If the path does not exist, print "No such file or directory", and return an ENOENT. Directories passed as the second argument may end in a "/" - in such cases the contents of the last directory in the path (before the "/") should be printed (as ls would do). Additionally, the path (the last argument) may be a file or link. In this case, your program should simply print the file/link name (if it exists) on a single line, and refrain from printing the . and ...
*/

#include "ext2.h"

extern unsigned char * disk;

int main(int argc, char ** argv){
	TRACE(DEBUG_LEVEL0, "%s\n", __func__);
	if (argc < 3 || argc > 4){
		fprintf(stderr, "Error: Missing parameters. It requires 2 parameters\n");
		fprintf(stderr, "Usage: ext2_ls <disk.img> [-a] <path>\n");
		return EINVAL;
	}

	// ------------------------ convert the arguments -----------------------//
	unsigned char * virtual_disk = NULL;
	char * path = NULL;
	int check_all = 0;
	//check if flag -a not specified
	if (argc == 3) {
		virtual_disk = (unsigned char*)argv[1];
		path = argv[2];
	} else if (!strcmp(argv[2], "-a")){
		check_all = 1;
		virtual_disk = (unsigned char*)argv[1];
		path = argv[3];
	} else {
		fprintf(stderr, "Error: Unknown flag specified\n");
		return EINVAL;
	}

	//---------------------------- open the image ---------------------------//
	open_image(virtual_disk);

	//--------------------------- setup datastructures -----------------------//
	init_datastructures();

	//---------------------------- go to the paths inode ----------------------//
	int inode_no = path_walk(path);
	if (inode_no == -ENOENT) {
		fprintf(stderr, "Error: No such file or directory\n");
		return ENOENT;
	}

	//----------------- PRINT ALL FILES LISTED IN THAT DIRECTORY ---------------//
	struct ext2_inode * dir_inode = inode_table + (inode_no - 1);
	if (!(dir_inode->i_mode & EXT2_S_IFDIR)) {
		char name[EXT2_NAME_LEN];
		char dir[EXT2_PATH_LEN];
		split_path(path, name, dir);
		printf("%s\n",name);
		return 0;
	}
	int return_val = check_directory(NULL, inode_no, check_all, &print_file);
	if (return_val != -1) {
		fprintf(stderr, "Error: Unable to print all files\n");
		return ENOENT;
	}
	close_image(disk);
	return 0;
}
