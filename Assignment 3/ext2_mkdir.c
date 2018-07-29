#include "ext2.h"


int main(int argc, char ** argv) {
	if (argc != 3) {
		fprintf(stderr, "Requires 2 args\n");
		exit(1);
	}

	char * virtual_disk = argv[1];
	char * path = argv[2];

	//----------------------------- open the image -----------------------------//
	open_image(virtual_disk);

	//-------------------------- setup datastructures --------------------------//
	init_datastructures();

	return create_file(path, EXT2_FT_DIR);
}
