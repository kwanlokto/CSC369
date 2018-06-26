#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

struct queue {
	int frameNumber;
	struct queue *next;
};

struct queue * Q;
struct queue * last;
/* Page to evict is chosen using the fifo algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int fifo_evict() {
	if (Q == last) {
		last = NULL;
	}
	int evictFrame = Q -> frameNumber;
	Q = Q -> next;
	return evictFrame;
}

/* This function is called on each access to a page to update any information
 * needed by the fifo algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void fifo_ref(pgtbl_entry_t *p) {
	if (Q == NULL) {
		Q -> frameNumber = (p -> frame) << PAGE_SHIFT;
		Q -> next = NULL;
		last = Q;
	} else {
		last = last -> next;
		last -> frameNumber = (p -> frame) << PAGE_SHIFT;
		last -> next = NULL;
	}
	return;
}

/* Initialize any data structures needed for this
 * replacement algorithm
 */
void fifo_init() {
	Q = NULL;
	last = NULL;
}
