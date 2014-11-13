#ifndef __DIRECTORY_H__
#define __DIRECTORY_H__
#include "hash_table.h"
#include "simulator.h"

// 256k directory = 
// 4*2^16
// coverage ratio of 2
#define DIR_SET_BITS 16
#define DIR_NWAYS 4
#define DIR_NSETS (1 << (DIR_SET_BITS))

#define DIR_TAG_MASK (-1 << (DIR_SET_BITS + N_BLOCKOFF_BITS))
#define DIR_GET_TAG(addr) ((addr) & DIR_TAG_MASK)

#define DIR_SET_MASK ((DIR_NSETS) - 1)
#define DIR_GET_SET(__addr) (((__addr) >> N_BLOCKOFF_BITS) & (DIR_SET_MASK))

// DIR_GET_INDEX returns the index of the core directory
// responsible for a given address
// we could use:
//   (DIR_GET_TAG(addr) % N_CORES)
// for interleaving adjacent lines to different cores,
// but the system described in Cuesta doesn't use that,
// so I am intentionally not interleaving by cache line
// and instead interleaving by page size
#define DIR_GET_INDEX(addr) ((addr >> PAGEOFF_BITS) % N_CORES)

#define DIR_SET_PROCESSOR_BM(__dir_entry, __core)	\
	(((__dir_entry)->shared_bm) |= (1 << __core))

#define DIR_CLR_PROCESSOR_BM(__dir_entry, __core)	\
	(((__dir_entry)->shared_bm) &= (~(1 << __core)))

#define DIR_GET_PROCESSOR_BM(__dir_entry, __core)	\
	(((__dir_entry)->shared_bm) & (1 << __core))

typedef struct {
    /* directory tag for this line (different from cache tag) */
    uint64_t dirtag;

    /* validity of this directory line */
    bool valid;

	/* if shared, bitmask of processors who have the line */
	uint64_t shared_bm;

	/* if exclusive, this represents the owner of this cache line */
	int owner;
	int ref_count;
} dir_entry_t;

typedef struct {
	/* Hash table of directory entries - contains cache lines */
	dir_entry_t entries[DIR_NSETS][DIR_NWAYS];
}directory_t;

void directory_init(void);
void dir_get_shared(int core, uint64_t tag);
void dir_get_excl(int core, uint64_t tag);
void directory_delete_node(int core, uint64_t tag);

#endif
