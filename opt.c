#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"
#include "sim.h"

//Not sure what to do think it over

extern unsigned memsize;

extern int debug;

extern struct frame *coremap;

struct hash_table * ht;

struct hash_table{
		struct opt_page * head;
};

struct opt_page{
	addr_t vaddr;
	struct page_time * start_time;
	struct page_time * end_time;
	struct opt_page * next_page;
};

struct page_time{
	int t;
	struct page_time * next_time;
};

struct linked_list{
	addr_t vaddr;
	struct linked_list * next;
};

struct linked_list * pg_address; //list of pages in the order in which it will be used
struct linked_list * last_vaddr; 

/* Page to evict is chosen using the optimal (aka MIN) algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int opt_evict() {
	//printf("evict\n");
	int longest_time = -1;
	int frame = -1;
	for (int i = 0; i < memsize; i++) {
		addr_t vaddr = coremap[i].vaddr;
		unsigned int dir = PGDIR_INDEX(vaddr);
		struct opt_page * curr = ht[dir].head;

		while (curr != NULL && longest_time != 0) {
			//printf("no same address\n");
			if (curr->vaddr == vaddr ) {
				//printf("start_time is null \n");
				if (curr->start_time != NULL) {
					// Get the difference between the next next time this page will be referenced
					// and the next time it will be referenced
					int diff = (curr->start_time)->t;
					if (diff > longest_time) {
						longest_time = diff;
						frame = i;
					}
				} else {
					longest_time = 0;
					frame = i;
				}
			}
			curr = curr->next_page;
		}

	}
	//printf("finish evict\n");
	if (frame != -1) {
		// NEED TO DO SOMETHING WITH COREMAP[FRAME].VADDR
		coremap[frame].vaddr = pg_address->vaddr;
		return frame;
	} 	
	fprintf(stderr, "Evicting page that doesn't exist????\n");
	exit(1);
}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {
	//printf("ref\n");
	unsigned int frame = (p->frame) >> PAGE_SHIFT;
	addr_t vaddr = coremap[frame].vaddr;
	//printf("Frame: %d addr %lx \n", frame, vaddr);
	unsigned int dir = PGDIR_INDEX(vaddr);
	//printf("Frame: %d addr %lx in dir %d\n", frame, vaddr, dir);
	//printf("these two should be the same: %lx and %lx\n",vaddr, pg_address->vaddr);
	struct opt_page * curr = ht[dir].head;
	//printf("no fail\n");


	while (curr != NULL) {
		//printf("%p\n",curr);
		if (curr->vaddr == vaddr) {
			//printf("%p\n",curr);
			//printf("time should be linear %d\n", curr->start_time ->t);
			
			struct page_time time_temp = *(curr->start_time);
			free(curr->start_time);
			curr->start_time = time_temp.next_time;
			//printf ("llnode \n");
			struct linked_list ll_temp = *(pg_address);
			free(pg_address);
			pg_address = ll_temp.next;
			//printf("successful\n");
			return;

		}
		curr = curr->next_page;
	}
	fprintf(stderr, "Referencing page that doesn't exist????\n");
	exit(1);
}

/* Add the new page_time to the corresponding hash entry
 */
void add_new_time(struct opt_page * pg, struct page_time * t) {
	if (pg->start_time == NULL) {
		pg->start_time = t;
		pg->end_time = t;
	} else {
		(pg->end_time)->next_time = t;
		pg->end_time = t;
	}
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {
	printf("init\n");
	// Initialize the queue
	pg_address = NULL;

	// Initialize the hash_table
	ht = malloc(PTRS_PER_PGDIR * sizeof(struct hash_table));
	for (int i = 0; i < PTRS_PER_PGDIR; i++) {
		ht[i].head = NULL;
	}

	// Create the linked list which represents the order virtual addresses
	// that will be coming in
	FILE * tfp;
	if((tfp = fopen(tracefile, "r")) == NULL) {
		perror("Error opening tracefile:");
		exit(1);
	}
	char buf[MAXLINE];
	addr_t vaddr = 0;
	char type;

	// READ FROM THE TRACE FILE
	// perform the necessary operations to initialize all datastructures
	struct opt_page * pg = NULL;
	struct page_time * pgt = NULL;
	int time_count = 0;
	int index = 0;
	while(fgets(buf, MAXLINE, tfp) != NULL) {
		if(buf[0] != '=') {
			sscanf(buf, "%c %lx", &type, &vaddr);
			//printf("%d \t", time_count);

			// Add the address to the linked list
			struct linked_list * node = malloc(sizeof(struct linked_list));
			node -> vaddr = vaddr;
			node -> next = NULL;
			
			if (pg_address == NULL) { //if the current linked_list is empty
				pg_address = node;
				last_vaddr = pg_address;
			} else {
				last_vaddr->next = node;
				last_vaddr = node;	
			}



			//printf("%d. vaddr %lx\n", time_count, vaddr);
			// Process the address read from trace file
			unsigned int dir = PGDIR_INDEX(vaddr);
			pg = malloc(sizeof(struct opt_page));
			pgt = malloc(sizeof(struct page_time));
			pgt->t = time_count;
			pgt->next_time = NULL;

			// Traverse the list to check if this addr already exists
			// If not then get to the end and add the pg to the end
			//struct opt_page * prev = NULL;
			struct opt_page * curr = ht[dir].head;
			//printf("dir %d \n",dir);
			int exist = 0;  //if this vaddr already exists
			while (curr != NULL && !exist) {
				//printf("too \t");
				if (curr->vaddr == vaddr) {
					//printf("same\n");
					exist = 1;
					free(pg);
					add_new_time(curr, pgt);
				}
				//prev = curr;
				curr = curr->next_page;
			}

			// If there is no match then add it to the end of the list
			// This means it is a new entry in the hash
			// Initialize the new opt_page
			if (!exist) {
				//if (prev == NULL) {
				//	ht[dir].head = pg;
				//} else {
				//	prev ->next_page = pg;
				//}
				pg->next_page = ht[dir].head;
				ht[dir].head = pg;

				//pg->next_page = NULL
				pg->vaddr = vaddr;
				pg->start_time = pgt;
				pg->end_time = pgt;

				// Add the virtual address to the coremap, so we can use the hash table
				// from the coremap
				if (index < memsize) {
					//printf("%d.vaddr: %lx in dir %d in pg %p\n", index, vaddr, dir, pg);
					coremap[index].vaddr = vaddr;
				}
				index++;
			}


			time_count++;
		}
	}
	printf("finished init\n");

	// initialize all nextUse to be 0
	// nextUse indicates that after 'nextUse' references this page will be
	// referenced again
	// for (int i = 0; i < memsize; i++) {
	 	//coremap[i].nextUse = 0;
	// }



}
