#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;



struct linked_list * head;
struct linked_list * last;
/* Page to evict is chosen using the fifo algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int fifo_evict() {
	struct linked_list temp = *head;
	free(head);
	int evictFrame = temp.frame_number;
	head = temp.next;

	if (head == NULL) {
		last = NULL;
	}
	return evictFrame;
}

/* This function is called on each access to a page to update any information
 * needed by the fifo algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void fifo_ref(pgtbl_entry_t *p) {
	if (head == NULL) {
		head = malloc(sizeof(struct linked_list));
		head -> frame_number = (p -> frame) >> PAGE_SHIFT;
		head -> next = NULL;
		last = head;
	} else {
		last = last -> next;
		last = malloc(sizeof(struct linked_list));
		last -> frame_number = (p -> frame) >> PAGE_SHIFT;
		last -> next = NULL;
	}
	return;
}

/* Initialize any data structures needed for this
 * replacement algorithm
 */
void fifo_init() {
	head = NULL;
	last = NULL;
}
