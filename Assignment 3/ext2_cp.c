#include "ext2.h"

int write_file(char *buf, int inode_no);


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







	fprintf(stderr, "After path walk\n");
	struct ext2_inode * dir = inode_table + (dir_inode_no - 1);
	if (!(dir->i_mode & EXT2_S_IFDIR)) {
		fprintf(stderr, "Entered directory path does not exist\n");
		exit(1);
	}

	fprintf(stderr, "hello\n");

	//---------------------------- open and read the file -------------------------//
	FILE * file = fopen(file_path, "r");
	char buf[EXT2_BLOCK_SIZE];

	if (file != NULL) {
		fprintf(stderr, "read\n");

		int inode_no;
		printf("inode ");
		inode_no = search_bitmap(inode_bitmap, i_bitmap_size);
		if (inode_no == -ENOMEM) {
			fprintf(stderr, "no space in the inode bitmap\n");
			return -ENOMEM;
		}
		take_spot(inode_bitmap, inode_no);

		create_inode(inode_no, EXT2_FT_REG_FILE);
		while (fread(buf, EXT2_BLOCK_SIZE, 1, file) > 0){
			//printf("%s", buf);
			// for (int i = 0; i < EXT2_BLOCK_SIZE; i++) {
			// 	printf("%c", buf[i]);
			 //}
			write_file(buf, inode_no);
		}

	} else {
		fprintf(stderr, "File does not exist\n");
		exit(1);
	}


	fclose(file);

}


int write_file(char *buf, int inode_no) {
	static int index = 0;

	int block_no;




	// for (int i = 0; i < (sb->s_blocks_count)/(sizeof(unsigned char) * 8); i++) {
	//
	//
  //   /* Looping through each bit a byte. */
  //   for (int k = 0; k < 8; k++) {
  //     if (!((block_bitmap[i] >> k) & 1)) {
	// 			block_no = i*8 + k;
	// 			inode_table[inode_no].i_block[0] = block_no;
	// 		}
	// 	}
	// }



	printf("block ");
	block_no = search_bitmap(block_bitmap, b_bitmap_size);
	if (block_no == -ENOMEM) {
		fprintf(stderr, "no space in the block bitmap\n");
		return -ENOMEM;
	}
	take_spot(block_bitmap, block_no);
	print_bitmap(b_bitmap_size, block_bitmap);

	inode_table[inode_no].i_block[index] = block_no;
	char * modify = (char *)disk + EXT2_BLOCK_SIZE * block_no;
	strncpy(modify, buf, EXT2_BLOCK_SIZE);
	index++;

	return 0;
}
