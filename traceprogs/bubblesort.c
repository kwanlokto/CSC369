#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void bubble_sort(int iters) {
	int i;
	time_t t;
	int nums[iters];
	/* Intializes random number generator */
   	srand((unsigned) time(&t));

   	/* Sets all values in nums array to be random numbers from 0 to iters */
  	for( i = 0 ; i < iters ; i++ ) {
		nums[i] = rand() % iters;
   	}

	//Bubble sort
	for (int i = 1; i < iters; i++) {
		for (int i = 1; i < iters; i++) {
			if (nums[i-1] > nums[i]) {
				int temp = nums[i-1];
				nums[i-1] = nums[i];
				nums[i] = temp;	
			}
		}
	}

	for (int i = 0; i < iters; i++) {
		printf("%d, ", nums[i]);
	}
}

int main(int argc, char ** argv) {
	/* Markers used to bound trace regions of interest */
	volatile char MARKER_START, MARKER_END;
	/* Record marker addresses */
	FILE* marker_fp = fopen("simpleloop.marker","w");
	if(marker_fp == NULL ) {
		perror("Couldn't open marker file:");
		exit(1);
	}
	fprintf(marker_fp, "%p %p", &MARKER_START, &MARKER_END );
	fclose(marker_fp);

	MARKER_START = 33;
	bubble_sort(10000);
	MARKER_END = 34;

	return 0;
}
