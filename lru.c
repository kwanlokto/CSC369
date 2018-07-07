#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

int top_frame;
int bottom_frame;
int empty;
/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int lru_evict() {
	//printf("victim frame: %d\n", bottom_frame);
	int victim_page = bottom_frame;
	bottom_frame = coremap[bottom_frame].previous_frame;
	if (bottom_frame != -1){ //Checks if there is no previous
		coremap[bottom_frame].next_frame = -1;
	} else {
		top_frame = -1;
		empty = 1;
	}
	coremap[victim_page].next_frame = -1;
	coremap[victim_page].previous_frame = -1;
	//printf("finish evict\n");
	return victim_page;
}

/* Add frame to the top of the stack
 * Fixes any pointers
 */
void add_to_top(int frame) {
	if(frame != top_frame){ //if it is already at the top of the stack don't do anything
		coremap[frame].next_frame = top_frame;
		coremap[frame].previous_frame = -1;
		if (top_frame != -1) {
			coremap[top_frame].previous_frame = frame;
		}
		top_frame = frame;
		if (coremap[top_frame].next_frame == -1) {
			bottom_frame = top_frame;
		}
	}
	//printf("finish add to top\n");
}


/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {
	int frame = (p -> frame) >> PAGE_SHIFT;
	//printf("Frame ref: %d\n",frame);
	int previous = coremap[frame].previous_frame;
	int next = coremap[frame].next_frame;
	
	// Case where frame is in the middle of the stack
	if (previous != -1 && next != -1) {
		coremap[previous].next_frame = next;
		coremap[next].previous_frame = previous;
	}

	// Case where frame is at the bottom of the stack
	else if (previous != -1 && next == -1){
		printf("ref: %d and bot: %d\n", frame, bottom_frame);
		//printf("Bottom frame: %d\n",bottom_frame);
		bottom_frame = coremap[bottom_frame].previous_frame;
		coremap[bottom_frame].next_frame = -1;
	}
	add_to_top(frame);	
	//printf("Finish Reference\n");
	return;
}




/* Initialize any data structures needed for this
 * replacement algorithm
 */
void lru_init() {
	for (int i = 0; i < memsize; i++) {
		coremap[i].next_frame = -1;
		coremap[i].previous_frame = -1;
	}
	empty = 1;
	top_frame = -1;
	bottom_frame = -1;
}
