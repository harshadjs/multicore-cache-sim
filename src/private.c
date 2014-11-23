#ifdef PRIVATE_TRACKING
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "private.h"

extern char *program_name;

static page_table_t pt;
static mem_map_t mm;
static int max_index;

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

void print_mem_map() {
  char fstr[1000];

  FILE *fp = fopen("/proc/self/maps", "r");
  assert(fp);

  while(fgets(fstr, sizeof(fstr), fp) != NULL) {
	printf("%s", fstr);
  }

  fclose(fp);
}

int alloc_new_map(uint64_t addr) {
  char fstr[1000];
  uint64_t start_addr, end_addr;
  char readable, writable;
  bool done = false;

  assert(max_index < MAX_MAPPINGS);

  FILE *fp = fopen("/proc/self/maps", "r");
  assert(fp);

  while(fgets(fstr, sizeof(fstr), fp) != NULL) {
	sscanf(fstr, "%16lx-%16lx %c%c",
		   &start_addr, &end_addr, &readable, &writable);
	if(addr >= start_addr && addr <= end_addr) {
	  done = true;
	  break;
	}
  }
  assert(done);
  fclose(fp);

  mm.maps[max_index].start_addr = start_addr;
  mm.maps[max_index].end_addr = end_addr;
#ifdef DISABLE_RDONLY_COHERENCE
  if(writable == 'w') {
	mm.maps[max_index].writable = true;
  } else {
	mm.maps[max_index].writable = false;
  }
#else
	mm.maps[max_index].writable = true;
#endif
  mm.maps[max_index].num_shared_pages = 0;
  mm.maps[max_index].num_private_pages = 0;
  mm.maps[max_index].num_shared_blocks = 0;
  mm.maps[max_index].num_private_blocks = 0;
  mm.maps[max_index].num_multiprivate_pages = 0;
  mm.maps[max_index].info = (char *)malloc(strlen(fstr));
  strcpy(mm.maps[max_index].info, fstr);

  max_index++;
  return max_index-1;
}

// searches the memory map for a range that addr fits in
// if none found, searches /proc/self/maps for a new mapping and adds it in
int search_mem_map(uint64_t addr) {
  for(int i = 0; i < max_index; i++) {
	if(addr >= mm.maps[i].start_addr && addr <= mm.maps[i].end_addr) {
	  return i;
	} 
  }
  // no matching range found
  return alloc_new_map(addr);
}

int map_cmp(const void* a, const void* b) {
  map_t *m1 = (map_t*)a;
  map_t *m2 = (map_t*)b;
  return m1->num_shared_pages - m2->num_shared_pages;
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
	val->map_index = search_mem_map(addr);
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
		if(mm.maps[val->map_index].writable) {
		  // This is how pages become permanently shared,
		  // and enter into the directory protocol
		  val->priv = false;
		  cache_invalidate(val->owner, addr);
		  PAGE_SET_PROCESSOR_BM(val, core, addr);
		  return false;
		} else {
		  PAGE_SET_PROCESSOR_BM(val, core, addr);
		  return true;
		}
	  }
	} else {
	  // pt entry shared
	  // nothing special to do
	  PAGE_SET_PROCESSOR_BM(val, core, addr);
	  if(mm.maps[val->map_index].writable) {	  
		return false;
	  } else {
		return true;
	  }
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
	int priv_pages=0, shared_pages=0, multiprivate_pages=0;
	int priv_blocks=0, shared_blocks=0, total_pages=0;;
	pt_entry_t *entry;
	bool any_shared_blocks;

	FILE *fp_page_tracking = fopen(PLOT_DIR"/page_track", "a");
	FILE *fp_block_tracking = fopen(PLOT_DIR"/block_track", "a");

	for(ht_iter_init(&iter, pt.ht); ht_iter_data(&iter); ht_iter_next(&iter)) {
		entry = (pt_entry_t *)ht_iter_data(&iter);

		any_shared_blocks = false; 
		for(int i=0; i < LINES_PER_PAGE; i++) {
			if(shared(entry->line_bm[i])) {
				any_shared_blocks = true;
				shared_blocks++;
				mm.maps[entry->map_index].num_shared_blocks++;
			} else {
				priv_blocks++;
				mm.maps[entry->map_index].num_private_blocks++;
			}
		}

		if(entry->priv) {
			priv_pages++;
			mm.maps[entry->map_index].num_private_pages++;
		} else if(any_shared_blocks) {
			shared_pages++;
			mm.maps[entry->map_index].num_shared_pages++;
		} else {
			// if there are only private blocks from different cores, 
			// then this is DEFINITELY a false share page, because
			// we could just separate the data
			//   (there are other kinds of false sharing not covered
			//    by this check)
			multiprivate_pages++;
			mm.maps[entry->map_index].num_multiprivate_pages++;
		}

	}

	// sort regions by # of shared pages
	qsort((void*)mm.maps, (size_t)max_index, sizeof(map_t), map_cmp);

	printf("********************\n");
	printf("Data Privacy Report\n");
	printf("********************\n");

	for(int i = 0; i < max_index; i++) {
		printf("%s", mm.maps[i].info);
		printf("Private Pages: %Lu (%.2f%%)\n", mm.maps[i].num_private_pages,
			   (100.0*mm.maps[i].num_private_pages)/
			   (mm.maps[i].num_private_pages+
				mm.maps[i].num_shared_pages+
				mm.maps[i].num_multiprivate_pages));
		printf("Shared Pages: %Lu (%.2f%%)\n", mm.maps[i].num_shared_pages,
			   (100.0*mm.maps[i].num_shared_pages)/
			   (mm.maps[i].num_private_pages+
				mm.maps[i].num_shared_pages+
				mm.maps[i].num_multiprivate_pages));
		printf("Multiprivate Pages: %Lu (%.2f%%)\n", 
			   mm.maps[i].num_multiprivate_pages,
			   (100.0*mm.maps[i].num_multiprivate_pages)/
			   (mm.maps[i].num_private_pages+
				mm.maps[i].num_shared_pages+
				mm.maps[i].num_multiprivate_pages));

		printf("Private Blocks: %Lu (%.2f%%)\n", mm.maps[i].num_private_blocks,
			   (100.0*mm.maps[i].num_private_blocks)/
			   (mm.maps[i].num_private_blocks+
				mm.maps[i].num_shared_blocks));
		printf("Shared Blocks: %Lu (%.2f%%)\n", mm.maps[i].num_shared_blocks,
			   (100.0*mm.maps[i].num_shared_blocks)/
			   (mm.maps[i].num_private_blocks+
				mm.maps[i].num_shared_blocks));

		printf("Potential Page Gain: %.1f\n",
			   (mm.maps[i].num_private_pages+
				mm.maps[i].num_shared_pages+
				mm.maps[i].num_multiprivate_pages)*
			   ((1.0*mm.maps[i].num_private_blocks)/
				(mm.maps[i].num_private_blocks+
				 mm.maps[i].num_shared_blocks) -
				(1.0*mm.maps[i].num_private_pages)/
				(mm.maps[i].num_private_pages+
				 mm.maps[i].num_shared_pages+
				 mm.maps[i].num_multiprivate_pages)));

	}

	printf("********************\n");
	printf("      Summary       \n");
	printf("********************\n");

	printf("Private Pages: %d (%.2f%%)\n", priv_pages,
		   (100.0*priv_pages)/(priv_pages+shared_pages+multiprivate_pages));
	printf("Shared Pages: %d (%.2f%%)\n", shared_pages,
		   (100.0*shared_pages)/(priv_pages+shared_pages+multiprivate_pages));
	printf("Multiprivate Pages: %d (%.2f%%)\n", multiprivate_pages,
		   (100.0*multiprivate_pages)/
		   (priv_pages+shared_pages+multiprivate_pages));
	printf("Private Blocks: %d (%.2f%%)\n", priv_blocks,
		   (100.0*priv_blocks)/(priv_blocks+shared_blocks));
	printf("Shared Blocks: %d (%.2f%%)\n", shared_blocks,
		   (100.0*shared_blocks)/(priv_blocks+shared_blocks));
	printf("Potential Gap: %.2f%%\n",
		   (100.0*priv_blocks)/(priv_blocks+shared_blocks) -
		   (100.0*priv_pages)/(priv_pages+shared_pages+multiprivate_pages));
	total_pages = priv_pages + shared_pages + multiprivate_pages;
	fprintf(fp_page_tracking, "%s %lf %lf %lf\n",
			program_name,
			PERCENTAGE(priv_pages, total_pages),
			PERCENTAGE(shared_pages, total_pages),
			PERCENTAGE(multiprivate_pages, total_pages));
	fclose(fp_page_tracking);
	fprintf(fp_block_tracking, "%s %lf %lf\n",
			program_name,
			PERCENTAGE(priv_blocks, priv_blocks + shared_blocks),
			PERCENTAGE(shared_blocks, priv_blocks + shared_blocks));
	fclose(fp_block_tracking);
}

void page_table_init(void)
{
  pt.ht = ht_create(101, page_hash, page_cmp, free);
  max_index = 0;
}
#endif
