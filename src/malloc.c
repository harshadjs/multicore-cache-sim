#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <assert.h>
#include "simulator.h"
#include "pin.H"
#include "malloc.h"

#define KB(__bytes) ((__bytes) * 1024)
#define MB(__bytes) ((KB(__bytes)) * 1024)

#define ALIGN_PAGE_BOUNDARY(__size) (((__size) >> PAGEOFF_BITS) + PAGE_SIZ)
#define MALLOC_NLISTS 4

mem_list_t lists[MALLOC_NLISTS];
#define printf(...)

#define WITHIN_MEMRANGE(__mem, __real)						\
	(((__real) >= (__mem)->mem_real) &&						\
	 ((__real) <= (__mem)->mem_real + (__mem)->alloc_size))

#define OFFSET_REAL(__mem, __real)				\
	((__real) - ((__mem)->mem_real))

static mem_range *
new_mem_range(uint64_t real, uint64_t virt, uint64_t size, uint64_t alloc_size)
{
	mem_range *mem;

	mem = (mem_range *)malloc(sizeof(mem_range));

	mem->mem_real = real;
	mem->mem_virt = virt;
	mem->mem_size = size;
	mem->alloc_size = alloc_size;

	return mem;
}

bool memrange_compar(mem_range *mem1, mem_range *mem2)
{
	if((mem1->mem_real + mem1->mem_size) < (mem2->mem_real))
		return true;
	return false;
}
void dump_lists(void);

static void free_memrange(mem_list_t *memlist, uint64_t real)
{
	std::set<mem_range *>::iterator iter;
	std::list<mem_range *>::iterator iter1;
	mem_range key, *val, *val1;

	key.mem_real = real;
	key.alloc_size = 1;

	iter = memlist->alloc_set.find(&key);
	if(iter == memlist->alloc_set.end()) {
		return;
	}
	val = *iter;
//	dump_lists();

	iter1 = memlist->alloc_list.begin();
	while(iter1 != memlist->alloc_list.end()) {
		val1 = *iter1;

		if(val->mem_virt == val1->mem_virt) {
			memlist->alloc_list.erase(iter1);
			break;
		}
		++iter1;
	}

	memlist->alloc_set.erase(iter);
	free(val);
}

static mem_range
*create_memrange(mem_list_t *memlist, uint64_t real, uint64_t size, uint64_t alloc_size)
{
	mem_range *mem, *mem1, *mem_new;
	std::list<mem_range *>::iterator iter, iter1;

	iter = memlist->alloc_list.begin();
	if(iter == memlist->alloc_list.end()) {
		/* Heap is empty */
		mem_new = new_mem_range(real, memlist->start_addr, size, alloc_size);
		goto done;
	}

	mem = *iter;
	if((mem->mem_virt - memlist->start_addr) > size) {
		/* There is enough space before the first node in heap */
		mem_new = new_mem_range(real, memlist->start_addr, size, alloc_size);
		goto done;
	}

	iter1 = iter;
	++iter1;
	mem = *iter;
	mem1 = *iter1;

	while(iter1 != memlist->alloc_list.end()) {

//		printf("\t[i]%lx %lx\n", mem1->mem_virt, mem1->mem_virt + mem1->mem_size);
		if((mem1->mem_virt - mem->mem_virt - mem->mem_size) > size) {
//			printf("%lx %lx between, size = %lu\n", mem->mem_virt, mem1->mem_virt, memlist->alloc_list.size());
			/* There are at least 4KB inbetween iter and iter1 */
			/* Use this! */
			mem_new = new_mem_range(real, mem->mem_virt + mem->mem_size, size, alloc_size);
			iter = iter1;
			goto done;
		}

		++iter;
		++iter1;
		mem = *iter;
		mem1 = *iter1;
	}

	iter = iter1;
	/* Allocate at the end */
	mem_new = new_mem_range(real, mem->mem_virt + mem->mem_size, size, alloc_size);
//	printf("End\n");
done:
//	printf("Insert ing: %lx - %lx\n", mem_new->mem_virt, mem_new->mem_virt + mem_new->mem_size);
	memlist->alloc_list.insert(iter, mem_new);
	memlist->alloc_set.insert(mem_new);

	if(!(memlist->alloc_set.size() == memlist->alloc_list.size())) {
		dump_lists();
		assert(0);
	}

	return mem_new;
}


uint64_t virtual_heap_address(void *address)
{
	uint64_t real = (uint64_t)address;
	int listnum = PIN_ThreadId() % MALLOC_NLISTS;
	std::set<mem_range *>::iterator iter;
	mem_range key, *val;

	key.mem_real = real;
	key.alloc_size = 1;
	iter = lists[listnum].alloc_set.find(&key);
	if(iter == lists[listnum].alloc_set.end()) {
//		printf("[NOT] %p\n", address);
		return real;
	}

	val = *iter;
//	printf("[FOUND] %p\n", address);
	return (val->mem_virt + OFFSET_REAL(val, real));
}

void dump_lists(void)
{
	int i;
	mem_range *mem;
	std::list<mem_range *>::iterator iter;
	std::set<mem_range *>::iterator iter1;

	for(i = 0; i < 1; i++) {
		printf("List[%d]: ", i);
		iter = lists[i].alloc_list.begin();
		while(iter != lists[i].alloc_list.end()) {
			mem = *iter;
			printf("\n[(R)%lx - %lx]:[(V)%lx - %lx]",
				   mem->mem_real, mem->mem_real + mem->alloc_size,
				   mem->mem_virt, mem->mem_virt + mem->mem_size);
			iter++;
		}
		printf("\n------------SET---------------");
		iter1 = lists[i].alloc_set.begin();
		while(iter1 != lists[i].alloc_set.end()) {
			mem = *iter1;

			printf("\n[(R)%lx - %lx]:[(V)%lx - %lx]",
				   mem->mem_real, mem->mem_real + mem->alloc_size,
				   mem->mem_virt, mem->mem_virt + mem->mem_size);
			iter1++;
		}
		printf("\n==============================\n");
	}

}

void *pin_malloc(size_t size)
{
	void *ptr = malloc(size);
	uint64_t size_aligned = ALIGN_PAGE_BOUNDARY(size);
	int listnum = PIN_ThreadId() % MALLOC_NLISTS;
	int i;
	mem_range *mem;

	printf("Malloced %lu bytes at %p\n", size, ptr);

	mem = create_memrange(&lists[listnum], (uint64_t)ptr, size_aligned, size);

	dump_lists();

	return ptr;
}

void *pin_realloc(void *orig_ptr, size_t size)
{
	void *ptr = realloc(orig_ptr, size);
	uint64_t size_aligned = ALIGN_PAGE_BOUNDARY(size);
	int listnum = PIN_ThreadId() % MALLOC_NLISTS;
	mem_range *mem;

	free_memrange(&lists[listnum], (uint64_t)orig_ptr);
	printf("Realloced %lx bytes at %p\n", size, ptr);
	mem = create_memrange(&lists[listnum], (uint64_t)ptr, size_aligned, size);
	mem->alloc_size = size;

	dump_lists();

    return ptr;
}

void *pin_calloc(size_t nmemb, size_t size)
{
	void *ptr = calloc(nmemb, size);
	int listnum = PIN_ThreadId() % MALLOC_NLISTS;
	mem_range *mem;
	uint64_t total = ((uint64_t)size) * ((uint64_t)nmemb);
	total = ALIGN_PAGE_BOUNDARY(total);

	printf("Calloced %lx bytes at %p\n", size*nmemb, ptr);
	mem = create_memrange(&lists[listnum], (uint64_t)ptr, total, nmemb*size);
	mem->alloc_size = nmemb * size;


	dump_lists();

	return ptr;
}

void pin_free(void *ptr)
{
	printf("Pin free\n");
	int listnum = PIN_ThreadId() % MALLOC_NLISTS;

	free_memrange(&lists[listnum], (uint64_t)ptr);
	free(ptr);
	printf("Finished\n");
}

void malloc_init(void)
{
	int i;
	uint64_t start_offsets[MALLOC_NLISTS] = {
		0xc000000000000000,
		0xc400000000000000,
		0xc800000000000000,
		0xcc00000000000000,
	};

	for(i = 0; i < MALLOC_NLISTS; i++) {
		lists[i].start_addr = start_offsets[i];
	}
}
