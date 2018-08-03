#include "ext2.h"

/*
 * This program takes three command line arguments. The first is the name of an
 * ext2 formatted virtual disk. The other two are absolute paths on your ext2
 * formatted disk. The program should work like ln, creating a link from the
 * first specified file to the second specified path. If the source file does
 * not exist (ENOENT), if the link name already exists (EEXIST), or if either
 * location refers to a directory (EISDIR), then your program should return the
 * appropriate error. Note that this version of ln only works with files.
 * Additionally, this command may take a "-s" flag, after the disk image argument.
 * When this flag is used, your program must create a symlink instead (other
 * arguments remain the same).
 */

int main(int argc, char ** argv){
	if (argc < 4 || argc > 5) {
		fprintf(stderr, "Requires 3 args\n");
		exit(1);
	}

	int file_type = EXT2_FT_REG_FILE;
	char * virtual_disk = argv[1];
	char * link_path;
	char * file_path;
	if (argc == 4){
		link_path = argv[2];
		file_path = argv[3];
	} else if (!strcmp(argv[2], "-s")) {
		file_type = EXT2_FT_SYMLINK;
		link_path = argv[3];
		file_path = argv[4];
	} else {
		fprintf(stderr, "unknown flag specified\n");
		return ENOENT;
	}




	//----------------------------- open the image -----------------------------//
	open_image(virtual_disk);

	//-------------------------- setup datastructures --------------------------//
	init_datastructures();

	//---------------------- create the symbolic link file ---------------------//

	int create_link = create_file(link_path, file_type, file_path);
	if(create_link != 0) {
		return create_link;
	}

	//--------------------- go to the link file inode -----------------//
	int link_inode_no;
	if (!(link_inode_no = path_walk(file_path))) {
		//fprintf(stderr, "Link path does not exist\n");
		return link_inode_no;
	}

	//----------------------- go to the regular file inode ---------------------//
	int file_inode_no;
	if (!(file_inode_no = path_walk(file_path))) {
		//fprintf(stderr, "File path does not exist\n");
		return file_inode_no;
	}

	//---------------------------- initialize the link -------------------------//

}
