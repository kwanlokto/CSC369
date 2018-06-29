#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

struct linked_list * top;
struct linked_list * bottom;

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int lru_evict() {
	int frame = bottom -> frame_number;
	struct linked_list temp = *bottom;
	free(bottom);

	bottom = temp.previous;
	if (bottom != NULL){ //Checks if there is no previous
		bottom -> next = NULL;
	} else {
		top = NULL;
	}
	coremap[frame].stack_ptr = NULL;

	return frame;
}


void add_to_top(struct linked_list * ptr) {
	ptr -> next = top;
	ptr -> previous = NULL;
	if (top != NULL) {
		top -> previous = ptr;
	}
	top = ptr;
	if (top -> next == NULL) {
		bottom = top;
	}
	//printf("add to top\n");
}


/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {
	int frame = (p -> frame) >> PAGE_SHIFT;
	struct linked_list * ptr;
	if (coremap[frame].stack_ptr == NULL) { //Checks if the hash is not pointing to something

		//printf("empty\n");

		coremap[frame].stack_ptr = malloc(sizeof(struct linked_list));
		ptr = coremap[frame].stack_ptr;
		ptr -> frame_number = frame;
		add_to_top(ptr);
		//printf("empty_done!\n");
	}
	else {
		ptr = coremap[frame].stack_ptr;
		if (ptr -> previous != NULL && ptr -> next != NULL) { // case in the middle
			//printf("middle\n");
			
			(ptr -> previous) -> next = ptr -> next;
			(ptr -> next) -> previous = ptr -> previous;
			add_to_top(ptr);
		}

		else if (ptr -> previous != NULL && ptr -> next == NULL){ //case is in the bottom and > 1 page
			//printf("bottom\n");

			bottom -> next = top;
			top = bottom;

			bottom = bottom -> previous;
			bottom->next = NULL;
			top -> previous = NULL;
			//printf("bottom_done!\n");
		}
	}

	return;
}




/* Initialize any data structures needed for this
 * replacement algorithm
 */
void lru_init() {
	for (int i = 0; i < memsize; i++) {
		coremap[i].stack_ptr = NULL;
	}
	top = NULL;
	bottom = NULL;
}
