#ifdef PRIVATE_TRACKING
#ifndef __PRIVATE_H__
#define __PRIVATE_H__
#include "hash_table.h"
#include "simulator.h"

#define LINES_PER_PAGE (1 << (PAGEOFF_BITS - N_BLOCKOFF_BITS))

#define PAGE_MASK ((1 << PAGEOFF_BITS) - 1)

#define PAGE_GET_INDEX(__addr) ((__addr & PAGE_MASK) >> N_BLOCKOFF_BITS)

#define PAGE_SET_PROCESSOR_BM(__pt_entry, __core, __addr)	\
  (((__pt_entry)->line_bm[PAGE_GET_INDEX(__addr)]) |= (1 << __core))

#define PAGE_GET_PROCESSOR_BM(__pt_entry, __core, __addr)	\
  (((__pt_entry)->line_bm[PAGE_GET_INDEX(__addr)]) & (1 << __core))

typedef struct {
  /* tag for this page */
  uint64_t tag;

  /* set to true if the page is currently private */
  bool priv;

  /* set to core # of the owner if the page is private and owned */
  int owner;

  // for each cache line on this page, the bitmask of 
  // cores that have accessed it
  // this is for tracking false sharing
  int line_bm[LINES_PER_PAGE];

} pt_entry_t;

typedef struct {
  /* hash table of pt_entry_t */
  ht_t *ht;
} page_table_t;

/* returns True if the page is now privately owned by the accessing core,
returns False if the page is now shared */
bool access_page(int core, uint64_t addr);
void page_table_init(void);
void print_false_sharing_report(void);

#endif
#endif
