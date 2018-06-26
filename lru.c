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
	printf("evict %d\n", frame);
	bottom = bottom -> previous;
	free(bottom -> next);
	coremap[frame].stack_ptr = NULL;
	bottom -> next = NULL;
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
		if (ptr -> previous != NULL) {
			(ptr -> previous) -> next = ptr -> next;
		}
		if (ptr -> next != NULL) {
			(ptr -> next) -> previous = ptr -> previous;
		}
	}
	printf("1.passed \n");
	ptr -> next = top;
	printf("2.passed \n");
	ptr -> previous = NULL;
	printf("3.passed \n");
	ptr -> frame_number = frame;
	printf("4.passed \n");
	if (top != NULL) {
		top -> previous = ptr;
	}
	printf("5.passed \n");
	top = ptr;
	printf("6.passed \n");
	if (top -> next == NULL) {
		bottom = top;
	}
	printf("7.passed \n");
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
