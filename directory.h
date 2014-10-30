#ifndef __DIRECTORY_H__
#define __DIRECTORY_H__
#include "hash_table.h"
#include "simulator.h"

#define DIR_TAG_MASK (MASK_TAG | MASK_SET)

#define DIR_SET_PROCESSOR_BM(__dir_entry, __core)	\
	(((__dir_entry)->shared_bm) |= (1 << __core))

#define DIR_CLR_PROCESSOR_BM(__dir_entry, __core)	\
	(((__dir_entry)->shared_bm) &= (~(1 << __core)))

#define DIR_GET_PROCESSOR_BM(__dir_entry, __core)	\
	(((__dir_entry)->shared_bm) & (1 << __core))

#define DIR_SET_SHARED SET_SHARED
#define DIR_SET_EXCLUSIVE SET_EXCLUSIVE
#define DIR_SET_INVALID SET_INVALID

#define DIR_IS_SHARED IS_SHARED
#define DIR_IS_EXCL IS_EXCL
#define DIR_IS_VALID IS_VALID


typedef struct {
	/* cache line for that dir entry */
	cache_line_t line;

	/* if shared, bitmask of processors who have the line */
	uint64_t shared_bm;

	/* if exclusive, this represents the owner of this cache line */
	int owner;
	int ref_count;
} dir_entry_t;

typedef struct {
	/* Hash table of directory entries - contains cache lines */
	ht_t *ht;

	/* Total number of lines in the directory */
	/* Should not exceed N_LINES * N_CORES */
	int count;
}directory_t;

void directory_init(void);
dir_entry_t *dir_get_shared(int core, uint64_t tag);
dir_entry_t *dir_get_excl(int core, uint64_t tag);
void directory_delete_node(int core, uint64_t tag);

#endif
