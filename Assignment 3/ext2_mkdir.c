#include "ext2.h"

int main(int argc, char ** argv) {
	if (argc != 3) {
		fprintf(stderr, "Requires 2 args\n");
		exit(1);
	}

	char * virtual_disk = argv[1];
	char * name = argv[2];

	//---------------------------- open the image ---------------------------//
	open_image(virtual_disk);


	//--------------------------- setup datastructures -----------------------//
	init_datastructures();


	//---------------------------- go to the paths inode ----------------------//
	unsigned int dir_inode_no;
	if (!(dir_inode_no = path_walk(path))) {
		fprintf(stderr, "not a valid path\n");
		return -ENOENT;
	}

	int check = create_file(dir_inode_no, );

}

int create_file(unsigned int dir_inode_no, char * path) {
	char file[EXT2_NAME_LEN];
	char dir[strlen(path)];
	split_path(path, file, dir);
	printf("path: %s, file: %s, dir: %s", path, file, dir);

	unsigned int inode_no = check_directory(, inode_no, &check_entry);
	if (inode_no) {
		fprintf(stderr, "file exists\n");
		return -EEXISTS;
	}



	int free_inode;
	if ((free_inode= get_free_spot(inode_bitmap, i_bitmap_size)) == -1) {
		fprintf(stderr, "no free inode space\n");
		return -ENOMEM;
	}
	take_spot(inode_bitmap, free_inode);

	create_inode(free_inode);




}

int create_inode(int free_inode){
	struct ext2_inode * new_inode = inode_table + (free_inode - 1);

	// How to set it to a directory ????
	new_inode->i_mode = new_inode->imode | EXT2_S_IFDIR;

	// https://stackoverflow.com/questions/5141960/get-the-current-time-in-c
	time_t rawtime;
	struct tm * timeinfo;

	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	printf ( "Current local time and date: %s", asctime (timeinfo) );

	new_inode->i_ctime = (unsigned int) timeinfo;
	for (int i = 0; i < 15; i++) {
		(new_inode -> i_block)[i] = 0;
		// How to set create time ???



	}
}
