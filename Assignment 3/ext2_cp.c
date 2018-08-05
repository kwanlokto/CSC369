/*
ext2_cp: This program takes three command line arguments. The first is the name of an ext2 formatted virtual disk. The second is the path to a file on your native operating system, and the third is an absolute path on your ext2 formatted disk. The program should work like cp, copying the file on your native file system onto the specified location on the disk. If the specified file or target location does not exist, then your program should return the appropriate error (ENOENT). Please read the specifications of ext2 carefully, some things you will not need to worry about (like permissions, gid, uid, etc.), while setting other information in the inodes may be important (e.g., i_dtime).
*/

#include "ext2.h"

int write_file(char *buf, int inode_no);
void allocate_indirect_space(unsigned int * indirect, int index);

int main(int argc, char ** argv){
	TRACE(DEBUG_LEVEL0, "%s\n", __func__);
	if (argc != 4) {
		fprintf(stderr, "Error Missing parameters. It requires 3 parameters\n");
		fprintf(stderr, "Usage: ext2_cp <disk.img> <Src_file_path> <dest_path>\n");
		exit(ENOENT);
	}

	unsigned char * virtual_disk = (unsigned char *)argv[1];
	char * file_path = argv[2];
	char * dir_path = argv[3];
	char dest_path[EXT2_PATH_LEN];
	char dest_file[EXT2_NAME_LEN];
	char dest_path_file[EXT2_PATH_LEN];
	char src_path[EXT2_PATH_LEN];
	int ret = 0;

	// Error checking on the 2nd and 3rd argument making sure that they
	// are valid arguments
	if (!strlen(file_path) || file_path[strlen(file_path) - 1] == '/') {
		fprintf(stderr, "Error: Invalid source file path\n");
		exit(ENOENT);
	}

	split_path(file_path, dest_file, src_path);

	if (!strlen(dest_file)) {
		fprintf(stderr, "Error: Invalid source file name\n");
		exit(ENOENT);
	}

	if (!strlen(dir_path)) {
		fprintf(stderr, "Error: Invalid destination path\n");
		exit(ENOENT);
	}

	strcpy(dest_path, dir_path);
	if (dest_path[strlen(dest_path) - 1] != '/') {
		strcat(dest_path,"/");
	}

	strcpy(dest_path_file, dest_path);
	strcat(dest_path_file, dest_file);

	//--------------------------- open the image ------------------------------//
	open_image(virtual_disk);

	init_datastructures();


	//---------------------------- open and read the file -------------------------//
	FILE * file = fopen(file_path, "rb");
	char buf[EXT2_BLOCK_SIZE];

	if (file != NULL) {

		//---------------------------- go to the dest directory inode ----------------------//
		unsigned int dir_inode_no;
		dir_inode_no = path_walk(dest_path);
		if (dir_inode_no == -ENOENT || dir_inode_no == -ENOTDIR) {
				fprintf(stderr, "Error: Destination path not found\n");
			ret = ENOENT;
			goto adr_exit;
		}

		struct ext2_inode * dir_inode = inode_table + (dir_inode_no - 1);
		if (!(dir_inode->i_mode & EXT2_S_IFDIR)) {
			fprintf(stderr, "Error: Destination path not found\n");
			ret = ENOENT;
			goto adr_exit;
		}

		unsigned int file_inode_no;
		file_inode_no = path_walk(dest_path_file);
		if (file_inode_no == -ENOENT) {
			//file doesn't exist
		    int return_val = create_file(dest_path_file, EXT2_FT_REG_FILE, NULL);
		    if (return_val) { //If the return value is not zero then an error occurred
			   return return_val;
		    }
		    file_inode_no = path_walk(dest_path_file);
		}
		else
		{
			//file already exist in file_inode_no
			//deallocate blocks
			struct ext2_inode * file_inode = inode_table + (file_inode_no - 1);
			for (int i = 0; i < 15; i++) {
				free_spot(block_bitmap, file_inode->i_block[i]);
				file_inode->i_block[i] = 0;
			}
			file_inode->i_dtime = 0; // remove deletion time
			file_inode->i_links_count = 1;
			file_inode->i_blocks = 0;
		}
		//allocate blocks and copy data
		memset(buf, 0, sizeof(buf));
		while (fread(buf, 1, EXT2_BLOCK_SIZE, file) > 0) {
			write_file(buf, file_inode_no);
			memset(buf, 0, sizeof(buf));
		}

		fclose(file);
	} else {
		fprintf(stderr, "File does not exist\n");
		ret= EEXIST;
	}

adr_exit:
	save_image(virtual_disk);
	close_image(disk);

	return(ret);
}


int write_file(char *buf, int inode_no) {
	static int index = 0;
	static int i = 0;
	static int j = 0;
	static int k = 0;

	int block_no;

	LOG(DEBUG_LEVEL0, "block ");
	block_no = search_bitmap(block_bitmap, b_bitmap_size);
	if (block_no == -ENOMEM) {
		fprintf(stderr, "no space in the block bitmap\n");
		return -ENOMEM;
	}
	take_spot(block_bitmap, block_no);
	print_bitmap(b_bitmap_size, block_bitmap);

	LOG(DEBUG_LEVEL0, "size of buf %d\n", strlen(buf));

	struct ext2_inode * inode = inode_table + (inode_no - 1);
	inode->i_size += strlen(buf);

	if (index < 12) { //Direct blocks
		inode->i_blocks += 2;
		(inode->i_block)[index] = block_no;
	}
	else if (index < 15){
		if (!(inode->i_block)[index]) {
			allocate_indirect_space(inode->i_block, index);
			inode->i_blocks += 2;
		}

		unsigned int * indirect_1 = (unsigned int *)(disk + (inode->i_block)[index] * block_size);

		if (index == 12){ //SINGLY
			indirect_1[i] = block_no;
			i++;
			if (i == 256) {
				i = 0;
				index++;
			}
		}

		else { //GREATER THAN SINGLY
			if (indirect_1[i]) {
				allocate_indirect_space(indirect_1, i);
				inode->i_blocks += 2;
			}
			unsigned int * indirect_2 = (unsigned int *)(disk + indirect_1[i] * block_size);

			if (index == 13) { //DOUBLY
				indirect_2[j] = block_no;
				j++;
				if (j == 256) {
					j = 0;
					i++;
					if (i == 256) {
						i = 0;
						index++;
					}
				}
			}

			else {
				if (indirect_2[j]) {
					allocate_indirect_space(indirect_2, j);
					inode->i_blocks += 2;
				}
				unsigned int * indirect_3 = (unsigned int *)(disk + indirect_2[j] * block_size);

				indirect_3[k] = block_no;
				k++;
				if (k == 256) {
					k = 0;
					j++;
					if (j == 256) {
						j = 0;
						i++;
						if (i == 256) {
							i = 0;
							index++;
						}
					}
				}
			}
		}
	}

	else {
		fprintf(stderr, "Not enough space\n");
		// NEED TO CLEAR THE IBLOCKS
		return exit(ENOMEM);
	}
	char * modify = (char *)disk + EXT2_BLOCK_SIZE * block_no;
	strncpy(modify, buf, EXT2_BLOCK_SIZE);
	index++;

	return 0;
}

void allocate_indirect_space(unsigned int * indirect, int index){
	int indirect_no = search_bitmap(block_bitmap, b_bitmap_size);
	if (indirect_no == -ENOMEM) {
		fprintf(stderr, "no space in the block bitmap\n");
		exit(ENOMEM);
	}
	take_spot(block_bitmap, indirect_no);

	indirect[index] = indirect_no;
}
