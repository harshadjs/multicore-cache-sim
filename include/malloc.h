#ifndef __MALLOC_H__
#define __MALLOC_H__
#include <stdint.h>
#include <set>
#include <list>
#include <algorithm>
#include "dlist.h"

class mem_range
{
public:
	uint64_t mem_real;
	uint64_t mem_virt;
	uint64_t mem_size;
	uint64_t alloc_size;
};


struct mem_range_compar {
	bool operator() (mem_range *const &lhs, mem_range *const &rhs) const {
		if((lhs->mem_real + lhs->alloc_size - 1) < (rhs->mem_real)) {
			return true;
		}
		return false;
	}
};

//bool memrange_compar(mem_range_t *mem1, mem_range_t *mem2);
bool memrange_compar(const int64_t &a, const int64_t &b);

typedef struct {
	uint64_t start_addr;
	std::list<mem_range *> alloc_list;
	std::set<mem_range *, mem_range_compar> alloc_set;
} mem_list_t;

void *pin_malloc(size_t size);
void pin_free(void *ptr);
void *pin_realloc(void *ptr, size_t size);
void *pin_calloc(size_t nmemb, size_t size);
uint64_t virtual_heap_address(void *address);
void malloc_init(void);

#define ALTER_ADDRESS(__addr) ((virtual_heap_address((void *)__addr)))
#endif
