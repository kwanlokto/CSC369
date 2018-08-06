#include "ext2.h"


int main(int argc, char ** argv) {
	if (argc != 3) {
		fprintf(stderr, "Error: Missing parameters. It requires 2 parameters\n");
		fprintf(stderr, "Usage: ext2_mkdir <disk.img> <dir_path>\n");
		return EINVAL;
	}

	unsigned char * virtual_disk = (unsigned char*)argv[1];
	char * path = argv[2];

	//----------------------------- open the image -----------------------------//
	open_image(virtual_disk);

	//-------------------------- setup datastructures --------------------------//
	init_datastructures();

	return create_file(path, EXT2_FT_DIR, NULL);
}
