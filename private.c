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
	ht_add(pt.ht, val);

	return true;
  } else {
	if(val->priv) {
	  if(val->owner == core) {
		// pt entry owned by this core
		return true;
	  } else {
		// pt entry owned by other code
		// This is how pages become permanently shared,
		// and enter into the directory protocol
		val->priv = false;
		// Not entirely sure what to do here: invalidate all existing cache
		// lines for this page? That should be safe, if not efficient
		// TODO: figure out what to do here		
		return false;
	  }
	} else {
	  // pt entry shared
	  // nothing special to do
	  return false;
	}
  }
}


void page_table_init(void)
{
  pt.ht = ht_create(101, page_hash, page_cmp, free);
}
