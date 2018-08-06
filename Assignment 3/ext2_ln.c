#include "ext2.h"

int main(int argc, char ** argv){
	if (argc < 4 || argc > 5) {
		fprintf(stderr, "Error: Missing parameters. It requires 3 parameters\n");
		fprintf(stderr, "Usage: ext2_ln <disk.img> [-s] <src_file> <target_file>\n");
		return EINVAL;
	}

	int file_type = EXT2_FT_REG_FILE;
	unsigned char * virtual_disk = (unsigned char*)argv[1];
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
		fprintf(stderr, "Error: Unknown flag specified\n");
		return EINVAL;
	}


	//----------------------------- open the image -----------------------------//
	open_image(virtual_disk);

	//-------------------------- setup datastructures --------------------------//
	init_datastructures();

	//---------------------- create the symbolic link file ---------------------//

	if (link_path[strlen(link_path) - 1] == '/') {
		fprintf(stderr, "Error: Cannot create directory link\n");
		return EISDIR;
	}

	// Check to see if the link to refers to a directory
	int inode_no = path_walk(file_path);
	if (inode_no == -ENOENT) {
		fprintf(stderr, "Error: No such file or directory\n");
		return ENOENT;
	}

	if (inode_table[inode_no - 1].i_mode & EXT2_S_IFDIR) {
		fprintf(stderr, "Error: Cannot link to a directory\n");
		return EISDIR;
	}

	return create_file(link_path, file_type, file_path);
}
