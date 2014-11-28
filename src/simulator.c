#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include "malloc.h"
#include "pintool.h"
#include "pin.H"
#include "pin_profile.H"
#include "simulator.h"
#include "directory.h"
#include "private.h"

extern PIN_LOCK lock;

/* Stats */
int hits, misses, directory_transactions, directory_misses,
	directory_hits, directory_invalidations, directory_excl_hits,
	directory_shared_hits, directory_deletions;

uint64_t misses_per_core[N_CORES];
uint64_t inst_per_core[N_CORES];

/* Global virtual clock */
uint64_t ticks;

/* Cache array */
cache_t cores[N_CORES];

char *program_name;

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

void dump_line(cache_line_t *line)
{
	printf("[%lx :%d:]\n", line->tag, line->state);
}

inline int cache_get_set(uint64_t address)
{
	return ((address & MASK_SET) >> N_BLOCKOFF_BITS);
}

inline uint64_t cache_get_tag(uint64_t address)
{
	return (address & MASK_TAG);
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

	//printf("oldest = %d\n", oldest);
	return oldest;
}

/* Finds least recently used node in cache @core and set @set */
int find_lru_node_or_exact_match(int core, int set, uint64_t address)
{
	int i;
	uint64_t tag = cache_get_tag(address);

	for(i = 0; i < N_LINES; i++) {
	  if(IS_VALID(&cores[core].sets[set].lines[i]) && 
		 cores[core].sets[set].lines[i].tag == tag) {
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
			SET_INVALID(line);
			return;
		}
	}
}

void cache_downgrade(int core, uint64_t address)
{
	int set = cache_get_set(address), i;
	cache_line_t *line;
	uint64_t tag;

	tag = cache_get_tag(address);

	for(i = 0; i < N_LINES; i++) {
		line = &cores[core].sets[set].lines[i];

		if((tag == line->tag) && (IS_VALID(line))) {
			SET_SHARED(line);
			return;
		}
	}
}

/* Local cache miss: Shared access requested */
cache_line_t *cache_load_shared(int core, uint64_t address)
{
	int oldest, set, access_level;
#ifdef PRIVATE_TRACKING
	bool priv_page;

	// priv_page is True if we have private access to this page
	// i.e. no directory lookup needed
	// priv_page is False if the page is shared
	// i.e. normal operation: use the directory
	priv_page = access_page(core, address);

	if(!priv_page) {
		/* Directory can give SHARED/EXCL access */
		access_level = dir_get_shared(core, address);
	} else {
		access_level = DIR_ACCESS_EXCL;
	}
#else
	access_level = dir_get_shared(core, address);
#endif

	set = cache_get_set(address);
	oldest = find_lru_node(core, set);
	if(IS_VALID(&cores[core].sets[set].lines[oldest]) &&
	   (cores[core].sets[set].lines[oldest].tag != (MASK_TAG & address))) {
	  directory_delete_node(core, address);
	}
	cores[core].sets[set].lines[oldest].tag = MASK_TAG & address;

	if(access_level == DIR_ACCESS_EXCL)
		SET_EXCLUSIVE(&(cores[core].sets[set].lines[oldest]));
	else
		SET_SHARED(&(cores[core].sets[set].lines[oldest]));

	return &cores[core].sets[set].lines[oldest];
}

/* Local cache miss: exclusive access requested */
cache_line_t *cache_load_excl(int core, uint64_t address)
{
	int oldest, set;
#ifdef PRIVATE_TRACKING
	bool priv_page;

	// priv_page is True if we have private access to this page
	// i.e. no directory lookup needed
	// priv_page is False if the page is shared
	// i.e. normal operation: use the directory
	priv_page = access_page(core, address);

	if(!priv_page) {
		/* Directory Must give us exclusive access */
	  dir_get_excl(core, address);
	} 

#else
	dir_get_excl(core, address);
#endif
	set = cache_get_set(address);
	oldest = find_lru_node_or_exact_match(core, set, address);
	if(IS_VALID(&cores[core].sets[set].lines[oldest]) && 
	   (cores[core].sets[set].lines[oldest].tag != (MASK_TAG & address))) {
	  directory_delete_node(core, address);
	}
	cores[core].sets[set].lines[oldest].tag = MASK_TAG & address;
	SET_EXCLUSIVE(&(cores[core].sets[set].lines[oldest]));
	
	return &cores[core].sets[set].lines[oldest];
}

/* Search local cache for shared access to address */
cache_line_t *cache_search_shared(int core, uint64_t address)
{
	int i;

	//printf("Searching shared: in set %d, tag exp: %lx\n",
	//     cache_get_set(address),
	//	   cache_get_tag(address));
	for(i = 0; i < N_LINES; i++) {
		cache_line_t *line;

		line = &cores[core].sets[cache_get_set(address)].lines[i];
		//printf("==>%lx, %d\n", line->tag, line->state);
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
	misses_per_core[core]++;
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
	misses_per_core[core]++;
	return NULL;
}

void cache_read(int core, uint64_t address)
{
	cache_line_t *line;

	// for now, just tick on every read/write, lamport clock style
	ticks++;
	//printf("[%d]\tRD: ", core);
	/* Do I have shared / exclusive access for this address? */
	line = cache_search_shared(core, address);
	if(!line) {
		/* Get shared access */
		line = cache_load_shared(core, address);
	}
	//dump_line(line);
	line->ticks = ticks;
}

void cache_write(int core, uint64_t address)
{
	cache_line_t *line;

	// for now, just tick on every read/write, lamport clock style
	ticks++;
	//printf("[%d]\tWR: ", core);
	/* Do I have exclusive access for this address? */
	line = cache_search_excl(core, address);
	if(!line) {
		/* Get exclusive access */
		line = cache_load_excl(core, address);
	} else {
	}
	//dump_line(line);
	line->ticks = ticks;
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

void pin_finish(int code, void *v)
{
	int i;
	FILE *fp = fopen(PLOT_DIR"/cache_stats", "a");
	assert(fp);

	PRINT_STAT(hits);
	PRINT_STAT(misses);
	PRINT_STAT(directory_transactions);
	PRINT_STAT(directory_misses);
	PRINT_STAT(directory_hits);
	PRINT_STAT(directory_invalidations);
	PRINT_STAT(directory_excl_hits);
	PRINT_STAT(directory_shared_hits);
	PRINT_STAT(directory_deletions);
	for(i = 0; i < N_CORES; i++) {
		printf("Misses on core %d = %lu\n", i, misses_per_core[i]);
	}
	fprintf(fp, "%s %lf %lf\n",
			program_name,
			PERCENTAGE(misses, hits + misses),
			PERCENTAGE(directory_misses, directory_misses + directory_hits));
	fclose(fp);
#ifdef PRIVATE_TRACKING
	print_false_sharing_report();
#endif
}

int main(int argc, char *argv[])
{
	unsigned int i;

	directory_init();
	if(argc > 12) {
	  program_name = strrchr(argv[12], '/');
	  printf("program: %s\n", program_name);
	  if(!program_name) {
		program_name = argv[argc - 1];
	  } else {
		program_name = program_name + 1;
	  }
	} else {
	  program_name = argv[argc-1];
	}

#ifdef PRIVATE_TRACKING
	page_table_init();
#endif

#ifdef OPTIMIZ
#ifdef OPTIMIZ_HEAP
	malloc_init();
#endif
#endif

	PIN_InitSymbols();
	if(PIN_Init(argc,argv)) {
		return -1;
	}

	IMG_AddInstrumentFunction(pin_image_handler, 0);

	INS_AddInstrumentFunction(pin_instruction_handler, 0);
	PIN_AddFiniFunction(pin_finish, 0);
	PIN_InitLock(&lock);
	PIN_StartProgram();
#if 0
	/* Not used anymore */
	for(i = 0; i < N_WORKLOAD; i++) {
	  //printf("**** [%d] %s:	0x%lx ****\n",
	  //	   workload[i].core, workload[i].type == MEM_READ ?
	  //	   "MEM_READ" : "MEM_WRITE", workload[i].address);
		if(workload[i].type == MEM_READ) {
			cache_read(workload[i].core, workload[i].address);
		} else {
			cache_write(workload[i].core, workload[i].address);
		}
		//print_changed_stats();
	}
	PRINT_STAT(hits);
	PRINT_STAT(misses);
	PRINT_STAT(directory_transactions);
	PRINT_STAT(directory_misses);
	PRINT_STAT(directory_hits);
	PRINT_STAT(directory_invalidations);
	PRINT_STAT(directory_excl_hits);
	PRINT_STAT(directory_shared_hits);
	PRINT_STAT(directory_deletions);
#endif

	return 0;
}
