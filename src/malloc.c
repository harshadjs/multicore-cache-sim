#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "simulator.h"
#include "pin.H"
#include "malloc.h"

#define KB(__bytes) ((__bytes) * 1024)
#define MB(__bytes) ((KB(__bytes)) * 1024)

#define ALIGN_PAGE_BOUNDARY(__size) (((__size) >> PAGEOFF_BITS) + PAGE_SIZ)
#define MALLOC_NLISTS 4

mem_list_t lists[MALLOC_NLISTS];

#define WITHIN_MEMRANGE(__mem, __real)						\
	(((__real) >= (__mem)->mem_real) &&						\
	 ((__real) <= (__mem)->mem_real + (__mem)->alloc_size))

#define OFFSET_REAL(__mem, __real)				\
	((__real) - ((__mem)->mem_real))

static mem_range_t *
new_mem_range(uint64_t real, uint64_t virt, uint64_t size)
{
	mem_range_t *mem;

	mem = (mem_range_t *)malloc(sizeof(mem_range_t));

	mem->mem_real = real;
	mem->mem_virt = virt;
	mem->mem_size = size;

	return mem;
}

bool memrange_compar(mem_range_t *mem1, mem_range_t *mem2)
{
	if((mem1->mem_real + mem1->mem_size) < (mem2->mem_real))
		return true;
	return false;
}

static void free_memrange(mem_list_t *memlist, uint64_t real)
{
	std::set<mem_range_t *>::iterator iter;
	mem_range_t key;

	key.mem_real = real;

	iter = memlist->alloc_set.find(&key);
	if(iter == memlist->alloc_set.end())
		return;
	memlist->alloc_set.erase(iter);
}

static mem_range_t
*create_memrange(mem_list_t *memlist, uint64_t real, uint64_t size)
{
	mem_range_t *mem, *mem1, *mem_new;
	std::set<mem_range_t *>::iterator iter, iter1;

	iter = memlist->alloc_set.begin();
	if(iter == memlist->alloc_set.end()) {
		/* Heap is empty */
		mem_new = new_mem_range(real, memlist->start_addr, size);
		memlist->alloc_set.insert(mem_new);
		return mem_new;
	}

	mem = *iter;
	if((mem->mem_virt - memlist->start_addr) > size) {
		/* There is enough space before the first node in heap */
		mem_new = new_mem_range(real, memlist->start_addr, size);
		memlist->alloc_set.insert(mem_new);
		return mem_new;
	}

	iter1 = iter;
	iter1++;
	while(iter1 != memlist->alloc_set.end()) {
		mem = *iter;
		mem1 = *iter1;

		if((mem1->mem_virt - mem->mem_virt - mem->mem_size) > size) {
			/* There are at least 4KB inbetween iter and iter1 */
			/* Use this! */
			mem_new = new_mem_range(real, mem->mem_virt + mem->mem_size, size);
			memlist->alloc_set.insert(mem_new);
			return mem_new;
		}

		iter++;
		iter1++;
	}

	/* Allocate at the end */
	mem_new = new_mem_range(real, mem->mem_virt + mem->mem_size, size);
	memlist->alloc_set.insert(mem_new);
	return mem_new;
}


uint64_t virtual_heap_address(void *address)
{
	uint64_t real = (uint64_t)address;
	int listnum = PIN_ThreadId() % MALLOC_NLISTS;
	std::set<mem_range_t *>::iterator iter;
	mem_range_t key, *val;

	key.mem_real = real;
	iter = lists[listnum].alloc_set.find(&key);
	if(iter == lists[listnum].alloc_set.end())
		return real;

	val = *iter;
	return (val->mem_virt + OFFSET_REAL(val, real));

}

void dump_lists(void)
{
#if 0
	int i;
	mem_range_t *mem;
	dlist_node_t *iter;

	for(i = 0; i < MALLOC_NLISTS; i++) {
		printf("List[%d]: ", i);
		iter = lists[i].alloc_list;
		while(iter) {
			mem = (mem_range_t *)dlist_data(iter);
			printf(" [(R)%lx - %lx]:[(V)%lx - %lx] ",
				   mem->mem_real, mem->mem_real + mem->mem_size,
				   mem->mem_virt, mem->mem_virt + mem->mem_size);
			iter = dlist_next(iter);
		}
		printf("\n");
	}
#endif
}

void *pin_malloc(size_t size)
{
	size = ALIGN_PAGE_BOUNDARY(size);
	void *ptr = malloc(size);
	int listnum = PIN_ThreadId() % MALLOC_NLISTS;
	mem_range_t *mem;

	mem = create_memrange(&lists[listnum], (uint64_t)ptr, (uint64_t)size);
	mem->alloc_size = size;

	return ptr;
}

void *pin_realloc(void *orig_ptr, size_t size)
{
	size = ALIGN_PAGE_BOUNDARY(size);
	void *ptr = realloc(orig_ptr, size);
	int listnum = PIN_ThreadId() % MALLOC_NLISTS;
	mem_range_t *mem;

	free_memrange(&lists[listnum], (uint64_t)orig_ptr);
	mem = create_memrange(&lists[listnum], (uint64_t)ptr, (uint64_t)size);
	mem->alloc_size = size;

    return ptr;
}
void *pin_calloc(size_t nmemb, size_t size)
{
	void *ptr = calloc(nmemb, size);
	int listnum = PIN_ThreadId() % MALLOC_NLISTS;
	mem_range_t *mem;
	uint64_t total = ((uint64_t)size) * ((uint64_t)nmemb);
	total = ALIGN_PAGE_BOUNDARY(total);

	mem = create_memrange(&lists[listnum], (uint64_t)ptr, total);
	mem->alloc_size = total;

	return ptr;
}

void pin_free(void *ptr)
{
	int listnum = PIN_ThreadId() % MALLOC_NLISTS;

	free_memrange(&lists[listnum], (uint64_t)ptr);
	free(ptr);
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
