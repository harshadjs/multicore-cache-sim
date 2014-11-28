#ifndef __MALLOC_H__
#define __MALLOC_H__
#include <stdint.h>
#include <set>
#include <algorithm>
#include "dlist.h"

struct mem_range_t {
	uint64_t mem_real;
	uint64_t mem_virt;
	uint64_t mem_size;
	uint64_t alloc_size;
	bool operator <(mem_range_t other) {
		if((mem_real + mem_size) < (other.mem_real))
			return true;
		return false;
	}
};

typedef struct mem_range_t mem_range_t;

//bool memrange_compar(mem_range_t *mem1, mem_range_t *mem2);
bool memrange_compar(const int64_t &a, const int64_t &b);

typedef struct {
	uint64_t start_addr;
	std::set<mem_range_t *> alloc_set;
} mem_list_t;

void *pin_malloc(size_t size);
void pin_free(void *ptr);
void *pin_realloc(void *ptr, size_t size);
void *pin_calloc(size_t nmemb, size_t size);
uint64_t virtual_heap_address(void *address);
void malloc_init(void);

#define ALTER_ADDRESS(__addr) ((virtual_heap_address((void *)__addr)))
#endif
