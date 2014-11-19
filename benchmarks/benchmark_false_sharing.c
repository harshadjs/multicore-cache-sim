#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

/* Bytes to Kilobytes */
#define KB(__bytes) ((__bytes) * 1024)

/* Bytes to Megabytes */
#define MB(__bytes) ((KB(__bytes)) * 1024)

/* Page size */
#define PAGE_SIZ KB(4)

/* Number of accesses performed by each thread */
#define N_ACCESSES MB(16)

uint8_t *data;

void *thread_function(void *arg)
{
	volatile int sum = 0;
	register int index;
	int i, my_id = (int)(long)arg;

	for(i = 0; i < N_ACCESSES; i++) {
		index = (my_id * 2) + (i % PAGE_SIZ);
		sum += data[index];
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	int i, n_threads;
	pthread_t *threads;

	if(argc < 2) {
		printf("Usage: %s <number_of_threads>\n", argv[0]);
		return 0;
	}
	n_threads = strtol(argv[1], NULL, 10);
	threads = malloc(sizeof(pthread_t) * n_threads);

	/*
	 * Allocate 2 pages for each thread, so that, even if
	 * the allocation is not aligned to 4KB page boundary,
	 * we are good!
	 */
	data = malloc(2 * PAGE_SIZ * n_threads);

	for(i = 0; i < n_threads; i++) {
		pthread_create(&threads[i],
					   NULL,
					   thread_function,
					   (void *)(long)0);
	}

	for(i = 0; i < n_threads; i++) {
		pthread_join(threads[i], NULL);
	}
	return 0;
}
