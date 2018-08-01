#include "ext2.h"


int main(int argc, char ** argv) {
	if (argc != 3) {
		fprintf(stderr, "mkdir requires 2 args\n");
		return ENOENT;
	}

	char * virtual_disk = argv[1];
	char * path = argv[2];

	//----------------------------- open the image -----------------------------//
	open_image(virtual_disk);

	//-------------------------- setup datastructures --------------------------//
	init_datastructures();

	return create_file(path, EXT2_FT_DIR);
}
