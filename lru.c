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


void add_to_top(struct linked_list * ptr, int frame) {
	ptr -> next = top;
	ptr -> previous = NULL;
	ptr -> frame_number = frame;
	if (top != NULL) {
		top -> previous = ptr;
	} else {
		bottom = NULL;
	}
	top = ptr;
	if (top -> next == NULL) {
		bottom = top;
	}
}


/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {
	int frame = (p -> frame) >> PAGE_SHIFT;
	struct linked_list * ptr;
	if (coremap[frame].stack_ptr == NULL) { //Checks if the hash is pointing to something
		coremap[frame].stack_ptr = malloc(sizeof(struct linked_list));
		ptr = coremap[frame].stack_ptr;
		add_to_top(ptr, frame);
	}
	else {
		ptr = coremap[frame].stack_ptr;
		if (ptr -> previous != NULL && ptr -> next != NULL) { // case in the middle
			(ptr -> previous) -> next = ptr -> next;
			(ptr -> next) -> previous = ptr -> previous;
			add_to_top(ptr, frame);
		}

		else if (ptr == bottom) { //case is in the bottom and > 1 page
			bottom -> next = top;
			top = bottom;

			bottom = bottom -> previous;
			bottom->next = NULL;
			top -> previous = NULL;
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
