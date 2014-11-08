#include <stdio.h>
#include <stdint.h>
#ifdef PINTOOL
#include "pintool.h"
#include "pin.H"
#include "pin_profile.H"
#endif
#include "simulator.h"
#include "directory.h"
#include "private.h"

#ifdef PINTOOL
extern PIN_LOCK lock;
#endif

/* Stats */
int hits, misses, directory_transactions, directory_misses,
	directory_hits, directory_invalidations, directory_excl_hits,
	directory_shared_hits, directory_deletions;


/* Global virtual clock */
uint32_t ticks;

/* Cache array */
cache_t cores[N_CORES];

#ifdef PINTOOL
#define printf(...)
#endif

void dump_core_cache(int core)
{
	int i, j;

	printf("----- Core %d -----\n", core);
	for(i = 0; i < N_SETS; i++) {
		printf("[Set %d]: ", i);
		for(j = 0; j < N_LINES; j++) {
			printf("[%lx :%d:]\t", cores[core].sets[i].lines[j].tag,
				   cores[core].sets[i].lines[j].state);
		}
		printf("\n");
	}
	printf("-------------------\n");
}

inline int cache_get_set(uint64_t address)
{
	return ((address & MASK_SET) >> N_BLOCKOFF_BITS);
}

inline uint64_t cache_get_tag(uint64_t address)
{
	return (address & MASK_TAG);
}

inline uint64_t cache_get_tag_dir(int set, uint64_t tag)
{
	return (tag) | (set << N_BLOCKOFF_BITS);
}

/* Finds least recently used node in cache @core and set @set */
int find_lru_node(int core, int set)
{
	int i, oldest;

	oldest = 0;
	for(i = 0; i < N_LINES; i++) {
		if((cores[core].sets[set].lines[oldest].ticks
			> cores[core].sets[set].lines[i].ticks))
			oldest = i;

		if(!IS_VALID(&cores[core].sets[set].lines[i])) {
			oldest = i;
			break;
		}
	}

	printf("oldest = %d\n", oldest);
	return oldest;
}

/* Finds least recently used node in cache @core and set @set */
int find_lru_node_or_exact_match(int core, int set, uint64_t address)
{
	int i;
	uint64_t tag = cache_get_tag(address);

	for(i = 0; i < N_LINES; i++) {
	  if(cores[core].sets[set].lines[i].tag == tag) {
		return i;
	  }
	}

	return find_lru_node(core, set);
}

void cache_invalidate(int core, uint64_t address)
{
	int set = cache_get_set(address), i;
	cache_line_t *line;
	uint64_t tag;

	tag = cache_get_tag(address);

	for(i = 0; i < N_LINES; i++) {
		line = &cores[core].sets[set].lines[i];

		if((tag == line->tag) && (IS_VALID(line))) {
			/* throw it away */
			directory_invalidations++;
			directory_delete_node(core, address);
			SET_INVALID(line);
			return;
		}
	}
}

/* Local cache miss: Shared access requested */
cache_line_t *cache_load_shared(int core, uint64_t address)
{
	int oldest, set;
	dir_entry_t *dir_entry;

	// TODO: access_page should be added here
	// if it returns False, we go on as normal
	// if it returns True, we skip the directory phase
	// and only add it to our local cache

	/*
	 * Directory will search for the tag
	 * if the entry is found, it will be returned
	 * Otherwise new entry will be created and that will be returned
	 * In any case, directory will never return NULL.
	 */
	dir_entry = dir_get_shared(core, address);

	/* Mark that this processor has t1he entry */
	DIR_SET_PROCESSOR_BM(dir_entry, core);

	/* Replace in core's own cache */
	set = cache_get_set(address);
	oldest = find_lru_node(core, set);
	directory_delete_node(core, cache_get_tag_dir(set, cores[core].sets[set].lines[oldest].tag));

	cores[core].sets[set].lines[oldest] = dir_entry->line;
	cores[core].sets[set].lines[oldest].tag &= MASK_TAG;

	return &cores[core].sets[set].lines[oldest];
}

/* Local cache miss: exclusive access requested */
cache_line_t *cache_load_excl(int core, uint64_t address)
{
	int oldest, set;
	dir_entry_t *dir_entry;

	// TODO: access_page should be added here
	// if it returns False, we go on as normal
	// if it returns True, we skip the directory phase
	// and only add it to our local cache

	/*
	 * Directory will search for the tag
	 * if the entry is found, it will be returned
	 * Otherwise new entry will be created and that will be returned
	 * In any case, directory will never return NULL.
	 */
	dir_entry = dir_get_excl(core, address);

	/* Replace in core's own cache */
	set = cache_get_set(address);
	oldest = find_lru_node_or_exact_match(core, set, address);
	directory_delete_node(core, cache_get_tag_dir(set, cores[core].sets[set].lines[oldest].tag));

	cores[core].sets[set].lines[oldest] = dir_entry->line;
	cores[core].sets[set].lines[oldest].tag &= MASK_TAG;

	return &cores[core].sets[set].lines[oldest];
}

/* Search local cache for shared access to address */
cache_line_t *cache_search_shared(int core, uint64_t address)
{
	int i;

	printf("Searching shared: in set %d, tag exp: %lx\n",
		   cache_get_set(address),
		   cache_get_tag(address));
	for(i = 0; i < N_LINES; i++) {
		cache_line_t *line;

		line = &cores[core].sets[cache_get_set(address)].lines[i];
		printf("==>%lx, %d\n", line->tag, line->state);
		if((IS_VALID(line)) &&
		   ((line->tag) & MASK_TAG) == cache_get_tag(address)) {

			if(IS_SHARED(line) || IS_EXCL(line)) {
				hits++;
				return line;
			} else {
				break;
			}
		}
	}

	misses++;
	return NULL;
}

/* Search local cache for exclusive access to address */
cache_line_t *cache_search_excl(int core, uint64_t address)
{
	int i;

	for(i = 0; i < N_LINES; i++) {
		cache_line_t *line;

		line = &cores[core].sets[cache_get_set(address)].lines[i];
		if((IS_VALID(line)) &&
		   (((line->tag) & MASK_TAG) == cache_get_tag(address))) {
			if(IS_EXCL(line)) {
				hits++;
				return line;
			} else {
				break;
			}
		}
	}

	misses++;
	return NULL;
}

void cache_read(int core, uint64_t address)
{
	cache_line_t *line;

	dump_core_cache(0);
	dump_core_cache(1);
	dump_core_cache(2);
	dump_core_cache(3);
	/* Do I have shared / exclusive access for this address? */
	line = cache_search_shared(core, address);
	if(!line) {
		/* Get shared access */
		line = cache_load_shared(core, address);
	}
}

void cache_write(int core, uint64_t address)
{
	cache_line_t *line;

	dump_core_cache(0);
	/* Do I have exclusive access for this address? */
	line = cache_search_excl(core, address);
	if(!line) {
		/* Get exclusive access */
		line = cache_load_excl(core, address);
	} else {
	}
}

workload_t workload[] = {
	/*
	 * Following file "workload.h" contains a sample workload
	 * for offline testing Pintool generates output in following format:
	 *
	 * { MEM_WRITE, 0, 0x7fff6eadac28 },
	 *
	 * This ouput can be directly copied and used here
	 * Just redirect the output of pintool to file workload.h
	 */
#include "workload.h"
};

#define N_WORKLOAD ((sizeof(workload))/sizeof(workload[0]))

void print_changed_stats(void)
{
	static int old_hits, old_misses, old_directory_transactions,
		old_directory_misses, old_directory_hits, old_directory_invalidations,
		old_directory_excl_hits, old_directory_shared_hits, old_directory_deletions;

	if(hits != old_hits)
		PRINT_STAT(hits);
	if(misses != old_misses)
		PRINT_STAT(misses);
	if(directory_transactions != old_directory_transactions)
		PRINT_STAT(directory_transactions);
	if(directory_misses != old_directory_misses)
		PRINT_STAT(directory_misses);
	if(directory_hits != old_directory_hits)
		PRINT_STAT(directory_hits);
	if(directory_invalidations != old_directory_invalidations)
		PRINT_STAT(directory_invalidations);
	if(directory_excl_hits != old_directory_excl_hits)
		PRINT_STAT(directory_excl_hits);
	if(directory_shared_hits != old_directory_shared_hits)
		PRINT_STAT(directory_shared_hits);
	if(directory_deletions != old_directory_deletions)
		PRINT_STAT(directory_deletions);

	old_misses = misses;
	old_directory_transactions = directory_transactions;
	old_directory_misses = directory_misses;
	old_directory_hits = directory_hits;
	old_directory_invalidations = directory_invalidations;
	old_directory_excl_hits = directory_excl_hits;
	old_directory_shared_hits = directory_shared_hits;
	old_directory_deletions = directory_deletions;
	old_hits = hits;
}

int main(int argc, char *argv[])
{
	unsigned int i;

	directory_init();
	page_table_init();

#ifdef PINTOOL
	PIN_InitSymbols();
	if(PIN_Init(argc,argv)) {
		return -1;
	}

	INS_AddInstrumentFunction(pin_instruction_handler, 0);
//	PIN_AddFiniFunction(Fini, 0);
	PIN_StartProgram();
	PIN_InitLock(&lock);
#else
	for(i = 0; i < N_WORKLOAD; i++) {
		printf("**** [%d] %s:	0x%lx ****\n",
			   workload[i].core, workload[i].type == MEM_READ ?
			   "MEM_READ" : "MEM_WRITE", workload[i].address);
		if(workload[i].type == MEM_READ) {
			cache_read(workload[i].core, workload[i].address);
		} else {
			cache_write(workload[i].core, workload[i].address);
		}
		print_changed_stats();
	}
#endif

	PRINT_STAT(hits);
	PRINT_STAT(misses);
	PRINT_STAT(directory_transactions);
	PRINT_STAT(directory_misses);
	PRINT_STAT(directory_hits);
	PRINT_STAT(directory_invalidations);
	PRINT_STAT(directory_excl_hits);
	PRINT_STAT(directory_shared_hits);
	PRINT_STAT(directory_deletions);

	return 0;
}
