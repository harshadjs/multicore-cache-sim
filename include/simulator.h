#ifndef __SIMULATOR_H__
#define __SIMULATOR_H__
#include <stdint.h>
#include "hash_table.h"
#include "dlist.h"

// define this if we want to track the code cache 
// as well as the data cache
//#define TRACK_TLB

// define this if we want to consider the first thread
// to be a setup thread, and ignore its cache traffic
#define IGNORE_STARTING_THREAD

// Magny Cours from Cuesta has
// 32k processor cache entries ->
// N_SETS*N_LINES = 32k
// N_SET_BITS = log(16k) = 14
// blocksize = 64 bytes = 2^6
// num processors = 8 (4x2)
#define N_SET_BITS 14
#define N_BLOCKOFF_BITS 6
#define N_TAG_BITS (32-N_SET_BITS-N_BLOCKOFF_BITS)
// page size: 4k
#define PAGEOFF_BITS 12
#define PAGE_SIZ (1 << PAGEOFF_BITS)

#define __MASK_TAG ((1 << (N_TAG_BITS)) - 1)
#define MASK_TAG (__MASK_TAG << (N_BLOCKOFF_BITS + N_SET_BITS))

#define __MASK_SET ((1 << (N_SET_BITS)) - 1)
#define MASK_SET (__MASK_SET << N_BLOCKOFF_BITS)

#define GET_PAGE_TAG(__addr) (__addr >> PAGEOFF_BITS)

#define N_CORES 9
#define N_SETS (1<<(N_SET_BITS))
#define N_LINES 2

enum {
	ST_INVALID,
	ST_SHARED,
	ST_EXCLUSIVE,
	ST_OWNED,
	ST_MODIFIED
};

typedef struct {
	uint64_t tag;
	uint64_t ticks;
	uint8_t state;
} cache_line_t;

#define IS_VALID(__line) (((__line)->state) != ST_INVALID)
#define IS_SHARED(__line) (((__line)->state) == ST_SHARED)
#define IS_EXCL(__line) (((__line)->state) == ST_EXCLUSIVE)
#define IS_OWN(__line) (((__line)->state) == ST_OWNED)
#define IS_MOD(__line) (((__line)->state) == ST_MODIFIED)

#define SET_MOD(__line) (((__line)->state) = ST_MODIFIED)
#define SET_OWN(__line) (((__line)->state) = ST_OWNED)
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

/* Invalidate the cache line for a particular core*/
void cache_invalidate(int , uint64_t);

/* Downgrade the cache line from exclusive to shared for a particular core*/
void cache_downgrade(int , uint64_t);

/* Read from a cache for a particular core */
void cache_read(int, uint64_t);

/* Write from a cache for a particular core */
void cache_write(int, uint64_t);

/* For use with update protocol */
void cache_search_and_update(int, uint64_t);

#define PERCENTAGE(__num, __total) (((__total) == 0) ?	\
									(0) : ((((double)(__num)) * 100.0)/(__total)))

#endif
