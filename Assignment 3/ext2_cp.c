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

	if (!strlen(dir_path)) {
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
		return EINVAL;
	}

	fprintf(stderr, "After path walk\n");
	struct ext2_inode * dir = inode_table + (dir_inode_no - 1);
	if (!(dir->i_mode & EXT2_S_IFDIR)) {
		fprintf(stderr, "Entered directory path does not exist\n");
		return EINVAL;
	}

	fprintf(stderr, "hello\n");

	//---------------------------- open and read the file -------------------------//
	FILE * file = fopen(file_path, "r");
	char buf[EXT2_BLOCK_SIZE];

	//Local machine path manipulation
	char file_name[EXT2_NAME_LEN];
	char f_path[strlen(file_path)];
	split_path(file_path, file_name, f_path);

	char abs_path[strlen(dir_path) + strlen(file_name) + 2];
	if (file != NULL) {
		fprintf(stderr, "read\n");

		//Create the absolute path
		strcpy(abs_path, f_path);
		abs_path[strlen(f_path)] = '\0';
		if (abs_path[strlen(f_path) - 1] != '/') { //if file path doesn't end with /
			abs_path[strlen(f_path)] = '/';
			abs_path[strlen(f_path) + 1] = '\0';
		}
		int len = strlen(abs_path);
		str_cat(abs_path, file_name, &len);

		int return_val = create_file(abs_path, EXT2_FT_REG_FILE, NULL);
		if (return_val) { //If the return value is not zero then an error occurred
			return return_val;
		}

		int inode_no = path_walk(abs_path);

		while (fread(buf, EXT2_BLOCK_SIZE, 1, file) > 0){
			printf("%s", buf);
			//write_file(buf, inode_no);
		}
		printf("done reading\n");

	} else {
		fprintf(stderr, "File does not exist\n");
		exit(1);
	}


	fclose(file);

}


int write_file(char *buf, int inode_no) {
	static int index = 0;

	int block_no;



	printf("block ");
	block_no = search_bitmap(block_bitmap, b_bitmap_size);
	if (block_no == -ENOMEM) {
		fprintf(stderr, "no space in the block bitmap\n");
		return -ENOMEM;
	}
	take_spot(block_bitmap, block_no);
	print_bitmap(b_bitmap_size, block_bitmap);

	printf("size of buf %d\n", strlen(buf));
	inode_table[inode_no].i_size += strlen(buf);
	inode_table[inode_no].i_block[index] = block_no;
	char * modify = (char *)disk + EXT2_BLOCK_SIZE * block_no;
	strncpy(modify, buf, EXT2_BLOCK_SIZE);
	index++;

	return 0;
}
