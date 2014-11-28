#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "directory.h"

/* Stats */
extern int hits, misses, directory_transactions, directory_misses,
	directory_hits, directory_invalidations, directory_excl_hits,
	directory_shared_hits, directory_deletions;

static directory_t dir[N_CORES];

/*
 * Invalidate this entry for all nodes which are sharing this,
 * except @core
 */
void invalidate_all(int core, dir_entry_t *entry, uint64_t address)
{
	int i = 0;
	uint64_t mask = entry->shared_bm;

	for(mask = entry->shared_bm, i = 0; mask;
		mask = mask >> 1, i++) {
		if((i != core) && DIR_GET_PROCESSOR_BM(entry, i))
			cache_invalidate(i, address);
	}
}

/*
 * Downgrade to shared (if exclusive) for the node holding this
 */
void downgrade_all(dir_entry_t *entry, uint64_t address)
{
	int i = 0;
	uint64_t mask = entry->shared_bm;

	for(mask = entry->shared_bm, i = 0; mask;
		mask = mask >> 1, i++) {
		if(DIR_GET_PROCESSOR_BM(entry, i))
		  cache_downgrade(i, address);
	}
}

// evicts a directory entry for a given core
// OR if there is an entry with no cache presence, evict that instead
// the eviction policy is: remove the line being shared by the fewest
// cores (ties broken arbitrarily)
dir_entry_t *dir_evict(directory_t *direct, int core, uint64_t address)
{
	dir_entry_t *val;
	dir_entry_t *evicted = 0;
	int i;

	for(i = 0; i < DIR_NWAYS; i++) {
		val = &direct->entries[DIR_GET_SET(address)][i];

		if(evicted == 0) {
			evicted = val;
		} else if(val->ref_count < evicted->ref_count) {
				evicted = val;
		}

		if(!val->valid) {
			evicted = val;
			break;
		}
	}

	// evicted line has been chosen - delete it
	directory_deletions++;
	if(evicted->valid) {
	  directory_invalidations++;
	  invalidate_all(N_CORES+1, evicted, address);
	}

	return evicted;
}

// search for a specific tag, or evict to make room for it if necessary
static dir_entry_t *dir_search(directory_t *direct, int core, uint64_t address)
{
	int i;
	dir_entry_t *p = NULL;
	uint64_t dirtag = DIR_GET_TAG(address);

	for(i = 0; i < DIR_NWAYS; i++) {
		p = &direct->entries[DIR_GET_SET(address)][i];

		if(p->valid && p->dirtag == dirtag)
			return p;
	}

	return dir_evict(direct, core, address);
}

/**
 * dir_get_shared:
 * Get shared access from directory
 * @args	core: Core
 * @args	tag: tag / address
 */
int dir_get_shared(int core, uint64_t address)
{
	dir_entry_t *val;
	uint64_t index = DIR_GET_INDEX(address);
	uint64_t dirtag = DIR_GET_TAG(address);

	directory_transactions++;

	val = dir_search(&dir[index], core, address);

	if(val->valid && val->dirtag == dirtag) {
	  // entry already exists: update it
	  directory_hits++;
	  directory_shared_hits++;
	  if(val->owner == core && val->ref_count == 1) {
		// we already own it
		return DIR_ACCESS_EXCL;
	  } else {
		if(val->ref_count > 1) {
		  // it is shared
		  // add ourselves
		  val->ref_count++;
		  DIR_SET_PROCESSOR_BM(val, core);
		  return DIR_ACCESS_SHARED;
		} else {
		  // someone else owns it
		  // make it shared
		  val->owner = 0;
		  val->ref_count++;
		  downgrade_all(val, address);
		  DIR_SET_PROCESSOR_BM(val, core);
		  return DIR_ACCESS_SHARED;
		}
	  }
	} else {
	  // new entry or eviction: initialize it
	  directory_misses++;
	  val->dirtag = dirtag;
	  val->owner = core;
	  val->ref_count = 1;
	  val->valid = true;
	  val->shared_bm = 0;
	  DIR_SET_PROCESSOR_BM(val, core);
	  return DIR_ACCESS_EXCL;
	}
}

/**
 * dir_delete_node:
 * Delete node from directory
 * @args	core: Core
 * @args	tag: tag / address
 */
void directory_delete_node(int core, uint64_t address)
{
	dir_entry_t *val;
	int i;
	uint64_t index = DIR_GET_INDEX(address);
	uint64_t dirtag = DIR_GET_TAG(address);

	for(i = 0; i < DIR_NWAYS; i++) {
	  val = &(dir[index].entries[DIR_GET_SET(address)][i]);
		if(val->dirtag == dirtag)
		  break;
	}

	if(val->dirtag != dirtag || !val->valid) {
	  // entry not found
	  return;
	}

	assert(val->ref_count > 0);
	if(DIR_GET_PROCESSOR_BM(val, core)) {
	  DIR_CLR_PROCESSOR_BM(val, core);
	  val->ref_count--;
	}
	if(val->ref_count == 0) {
	  val->valid = false;
	}
}


// determine if this is the only core with any access to address
bool dir_excl_access(int core, uint64_t address) {
	dir_entry_t *val;
	uint64_t index = DIR_GET_INDEX(address);
	uint64_t dirtag = DIR_GET_TAG(address);

	directory_transactions++;

	val = dir_search(&dir[index], core, address);

	assert(val->valid && val->dirtag == dirtag);
	return ((val->ref_count == 1) && (val->owner == core));
}

/**
 * dir_get_excl:
 * Get exclusive access: This function will *surely* give
 * exclusive access to core @core for address with tag @tag
 * @args core:	Requester core
 * @args address:	Requested address
 */
void dir_get_excl(int core, uint64_t address)
{
	dir_entry_t *val;
	uint64_t index = DIR_GET_INDEX(address);
	uint64_t dirtag = DIR_GET_TAG(address);

	directory_transactions++;

	val = dir_search(&dir[index], core, address);

	if(val->valid && val->dirtag == dirtag) {
	  directory_hits++;
	  directory_excl_hits++;
	  // entry already exists: update it
	  if(val->owner == core && val->ref_count == 1) {
		// we already own it
		return;
	  } else {
		// someone else owns it, or it is shared
		// invalidate everyone else, then take ownership
		invalidate_all(core, val, address);
		val->owner = core;
		val->ref_count = 1;
		val->valid = true;
		val->shared_bm = 0;
		DIR_SET_PROCESSOR_BM(val, core);
		return;
	  }
	} else {
	  directory_misses++;
	  // new entry or eviction: initialize it
	  val->dirtag = dirtag;
	  val->owner = core;
	  val->ref_count = 1;
	  val->valid = true;
	  val->shared_bm = 0;
	  DIR_SET_PROCESSOR_BM(val, core);
	  return;
	}
}

void directory_init(void)
{
}
