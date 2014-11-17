#ifdef PRIVATE_TRACKING
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "private.h"

static page_table_t pt;

static int page_cmp(void *val1, void *val2)
{
  uint64_t *tag1 = (uint64_t*)val1;
  uint64_t *tag2 = (uint64_t*)val2;

  if(*tag1 == *tag2)
	return 0;

  return 1;
}

static int page_hash(void *data)
{
  uint64_t *tag = (uint64_t*)data;
  return (int)(*tag);
}

bool access_page(int core, uint64_t addr)
{
  uint64_t tag = GET_PAGE_TAG(addr);
  pt_entry_t *val;

  val = (pt_entry_t*)ht_search(pt.ht, &tag);
  if(!val) {
	// pt entry not found: unowned page
	// make it owned by the accessing core

	val = (pt_entry_t *)malloc(sizeof(pt_entry_t));
	val->owner = core;
	val->tag = tag;
	val->priv = true;
	for(int i=0; i < LINES_PER_PAGE; i++) {
	  val->line_bm[i] = 0;
	}
	PAGE_SET_PROCESSOR_BM(val, core, addr);
	ht_add(pt.ht, val);

	return true;
  } else {
	if(val->priv) {
	  if(val->owner == core) {
		// pt entry owned by this core
		PAGE_SET_PROCESSOR_BM(val, core, addr);
		return true;
	  } else {
		// pt entry owned by other code
		// This is how pages become permanently shared,
		// and enter into the directory protocol
		val->priv = false;
		cache_invalidate(val->owner, addr);
		PAGE_SET_PROCESSOR_BM(val, core, addr);
		return false;
	  }
	} else {
	  // pt entry shared
	  // nothing special to do
	  PAGE_SET_PROCESSOR_BM(val, core, addr);
	  return false;
	}
  }
}

// old trick to find if there are at least 2 1's in a bitmask
bool shared(int bitmask) {
  return ((bitmask-1) & bitmask) != 0;
}

void print_false_sharing_report(void)
{
  ht_iter_t iter;
  int priv_pages=0, shared_pages=0;
  int priv_blocks=0, shared_blocks=0;
  pt_entry_t *entry;

  for(ht_iter_init(&iter, pt.ht); ht_iter_data(&iter); ht_iter_next(&iter)) {
	entry = (pt_entry_t *)ht_iter_data(&iter);
	if(entry->priv) {
	  priv_pages++;
	} else {
	  shared_pages++;
	}
	for(int i=0; i < LINES_PER_PAGE; i++) {
	  if(shared(entry->line_bm[i])) {
		shared_blocks++;
	  } else {
		priv_blocks++;
	  }
	}
  }

  printf("Private Pages: %d\n", priv_pages);
  printf("Shared Pages: %d\n", shared_pages);
  printf("Page Privacy: %.2f%%\n", 
		 (100.0*priv_pages)/(priv_pages+shared_pages));
  printf("Private Blocks: %d\n", priv_blocks);
  printf("Shared Blocks: %d\n", shared_blocks);
  printf("Block Privacy: %.2f%%\n", 
		 (100.0*priv_blocks)/(priv_blocks+shared_blocks));
}

void page_table_init(void)
{
  pt.ht = ht_create(101, page_hash, page_cmp, free);
}
#endif
