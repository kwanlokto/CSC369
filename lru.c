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
	bottom = bottom -> previous;
	free(bottom -> next);
	coremap[frame].stack_ptr = NULL;
	return frame;
}

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {
	int frame = (p -> frame) >> PAGE_SHIFT;
	printf("%d\n", frame);
	struct linked_list * ptr;
	if (coremap[frame].stack_ptr == NULL) {
		coremap[frame].stack_ptr = malloc(sizeof(struct linked_list));
		ptr = coremap[frame].stack_ptr;
	}
	else {
		ptr = coremap[frame].stack_ptr;
		(ptr -> previous) -> next = ptr -> next;
		(ptr -> next) -> previous = ptr -> previous;
	}
	ptr -> next = top;
	ptr -> previous = NULL;
	ptr -> frame_number = frame;
	top -> previous = ptr;
	top = ptr;

	if (top -> next == NULL) {
		bottom = top;
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
