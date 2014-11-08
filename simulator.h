#ifndef __SIMULATOR_H__
#define __SIMULATOR_H__
#include <stdint.h>
#include "hash_table.h"
#include "dlist.h"

// Magny Cours from Cuesta has
// 128k size cache per core ->
// N_SETS*N_LINES = 128*1024
// N_SET_BITS = 16
#define N_SET_BITS 16
#define N_TAG_BITS 13
#define N_BLOCKOFF_BITS 3
// page size: 4k
#define PAGEOFF_BITS 12

#define __MASK_TAG ((1 << (N_TAG_BITS)) - 1)
#define MASK_TAG (__MASK_TAG << (N_BLOCKOFF_BITS + N_SET_BITS))

#define __MASK_SET ((1 << (N_SET_BITS)) - 1)
#define MASK_SET (__MASK_SET << N_BLOCKOFF_BITS)

#define GET_PAGE_TAG(__addr) (__addr >> PAGEOFF_BITS)

#define N_CORES 5
#define N_SETS (1<<(N_SET_BITS))
#define N_LINES 2

enum {
	ST_INVALID,
	ST_SHARED,
	ST_EXCLUSIVE,
};

typedef struct {
	uint64_t tag;
	uint32_t ticks;
	uint8_t state;
} cache_line_t;

#define IS_VALID(__line) (((__line)->state) != ST_INVALID)
#define IS_SHARED(__line) (((__line)->state) == ST_SHARED)
#define IS_EXCL(__line) (((__line)->state) == ST_EXCLUSIVE)

#define SET_EXCLUSIVE(__line) (((__line)->state) = ST_EXCLUSIVE)
#define SET_SHARED(__line) (((__line)->state) = ST_SHARED)
#define SET_INVALID(__line) (((__line)->state) = ST_INVALID)

typedef struct {
	cache_line_t lines[N_LINES];
} cache_set_t;

typedef struct {
	int core;
	cache_set_t sets[N_SETS];
}cache_t;

typedef enum {
	MEM_READ,
	MEM_WRITE,
} workload_access_t;

typedef struct {
	workload_access_t type;
	int core;
	uint64_t address;
} workload_t;

#define PRINT_STAT(__stat) printf("%s = %d\n", #__stat, __stat)

void cache_invalidate(int , uint64_t);
void cache_read(int, uint64_t);
void cache_write(int, uint64_t);


#endif
