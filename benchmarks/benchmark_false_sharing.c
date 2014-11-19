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
#define N_PAGES_PER_THREAD 16
#define N_THREADS 16

struct my_page {
	uint8_t data[PAGE_SIZ];
} __attribute__((aligned(PAGE_SIZ)));

struct my_page pages[N_PAGES_PER_THREAD * N_THREADS];

void *thread_function(void *arg)
{
	int i, j, my_id = (int)(long)arg;

	for(i = 0; i < N_PAGES_PER_THREAD; i++)
		for(j = 0; j < PAGE_SIZ; j++)
			pages[(my_id * N_PAGES_PER_THREAD) + i].data[j]++;

	return NULL;
}

int main(int argc, char *argv[])
{
	int i;
	pthread_t threads[N_THREADS];

	for(i = 0; i < N_THREADS; i++) {
		pthread_create(&threads[i],
					   NULL,
					   thread_function,
					   (void *)(long)0);
	}

	for(i = 0; i < N_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}
	return 0;
}
