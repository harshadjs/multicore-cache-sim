#ifndef __PRIVATE_H__
#define __PRIVATE_H__
#include "hash_table.h"
#include "simulator.h"

// Notes
// 1. Directory needs a capacity below cache_size*cores
// 2. Directory needs to be split (part of address decides which core to send it to)

// page size: 4k
#define PAGEOFF_BITS 12

#define GET_PAGE_TAG(__addr) (__addr >> PAGEOFF_BITS)

typedef struct {
  /* tag for this page */
  uint64_t tag;

  /* set to true if the page is currently private */
  bool priv;

  /* set to core # of the owner if the page is private and owned */
  int owner;
} pt_entry_t;

typedef struct {
  /* hash table of pt_entry_t */
  ht_t *ht;
} page_table_t;

/* returns True if the page is now privately owned by the accessing core,
returns False if the page is now shared */
bool access_page(int core, uint64_t addr);
void page_table_init(void);

#endif
