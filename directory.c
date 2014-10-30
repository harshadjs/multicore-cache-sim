#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "directory.h"

/* Stats */
extern int hits, misses, directory_transactions, directory_misses,
	directory_hits, directory_invalidations, directory_excl_hits,
	directory_shared_hits, directory_deletions;

static directory_t dir;

/* Hash table functions */

/* Compare function */
static int dir_cmp(void *val1, void *val2)
{
	cache_line_t *line1 = (cache_line_t *)val1;
	cache_line_t *line2 = (cache_line_t *)val2;

	if(line1->tag == line2->tag)
		return 0;

	return 1;
}

/* Hash function */
static int dir_hash(void *data)
{
	cache_line_t *line = (cache_line_t *)data;

	return (int)(line->tag);
}

/*
 * Invalidate this entry for all nodes which are sharing this,
 * except @core
 */
void invalidate_all(int core, dir_entry_t *entry)
{
	int i = 0;
	uint64_t mask = entry->shared_bm;

	for(mask = entry->shared_bm, i = 0; mask;
		mask = mask >> 1, i++) {
		if((i != core) && DIR_GET_PROCESSOR_BM(entry, i))
			cache_invalidate(i, entry->line.tag);
	}
}

dir_entry_t *dir_get_shared(int core, uint64_t tag)
{
	dir_entry_t key, *val;

	tag = tag & DIR_TAG_MASK;

	key.line.tag = tag;

	directory_transactions++;

	val = (dir_entry_t *)ht_search(dir.ht, &key);
	if((!val) ||
	   (!DIR_IS_VALID(&val->line))) {
		val = (dir_entry_t *)malloc(sizeof(dir_entry_t));
		val->line.tag = tag;
		val->line.state = ST_SHARED;
		val->ref_count = 1;
		DIR_SET_PROCESSOR_BM(val, core);

		directory_misses++;
		ht_add(dir.ht, val);
		return val;
	}

	directory_hits++;
	val->ref_count++;

	if(DIR_IS_SHARED(&val->line)) {
		directory_shared_hits++;
		/* line is shared */
		assert(!DIR_GET_PROCESSOR_BM(val, core));

		DIR_SET_PROCESSOR_BM(val, core);
		return val;
	}

	if(DIR_IS_EXCL(&val->line)) {
		/* Someone owns this line */

		assert(val->owner != core);
		/*
		 * Invalidate the line in owner's cache
		 * XXX: We can do better here: instead of invalidating,
		 * 		change its state to shared
		 */
		cache_invalidate(val->owner, tag);
		DIR_SET_SHARED(&val->line);
		DIR_SET_PROCESSOR_BM(val, core);

		return val;
	}

	/* Should not reach here */
	return NULL;
}

void directory_delete_node(int core, uint64_t tag)
{
	dir_entry_t key, *val;

	key.line.tag = tag;
	val = (dir_entry_t *)ht_search(dir.ht, &key);
	if(!val)
		return;
	val->ref_count--;
	if(val->ref_count == 0) {
		directory_deletions++;
		ht_remove(dir.ht, &key);
		return;
	}

	if(DIR_IS_SHARED(&val->line)) {
		DIR_CLR_PROCESSOR_BM(val, core);
	}

	if(DIR_IS_EXCL(&val->line)) {
		DIR_SET_INVALID(&val->line);
	}
}

/**
 * dir_get_excl:
 * Get exclusive access: This function will *surely* give
 * exclusive access to core @core for address with tag @tag
 * If the directory miss occurs i.e. no cache has that node,
 * a new empty node will be created.
 * @args core:	Requester core
 * @args tag:	Tag for requested address
 * @returns:	directory entry
 */
dir_entry_t *dir_get_excl(int core, uint64_t tag)
{
	dir_entry_t key, *val;

	directory_transactions++;
	tag = tag & DIR_TAG_MASK;

	key.line.tag = tag;
	val = (dir_entry_t *)ht_search(dir.ht, &key);

	if((!val) ||
	   (!DIR_IS_VALID(&val->line))) {
		/* Invalid cache line */
		val = (dir_entry_t *)malloc(sizeof(dir_entry_t));
		val->line.tag = tag;
		val->owner = core;
		val->ref_count = 1;
		DIR_SET_EXCLUSIVE(&val->line);
		ht_add(dir.ht, val);

		directory_misses++;
		return val;
	}

	val->ref_count++;
	directory_hits++;

	if(DIR_IS_SHARED(&val->line)) {
		/* Invalidate all the lines in caches who are sharing this line */
		invalidate_all(core, val);
		DIR_SET_EXCLUSIVE(&val->line);
		val->owner = core;

		return val;
	}

	if(DIR_IS_EXCL(&val->line)) {
		/* Someone owns this line */

//		if(val->owner == core) {
//		}
		assert(val->owner != core);
		/*
		 * Invalidate the line in owner's cache
		 */
		cache_invalidate(val->owner, tag);
		DIR_SET_EXCLUSIVE(&val->line);
		val->owner = core;

		return val;
	}

	/* Should not reach here */
	return NULL;
}

void directory_init(void)
{
	dir.ht = ht_create(23, dir_hash, dir_cmp, free);
	dir.count = 0;
}
