/**
 * Hash table implementation using doubly linked list
 * Author: Harshad Shirwadkar
 * Email: harshad@cmu.edu
 */

#ifndef __HASH_TABLE_H__
#define __HASH_TABLE_H__

#include "dlist.h"
#include "stdlib.h"

typedef struct {
	int n_buckets, n_items;
	dlist_node_t **buckets;
	int (*hash)(void *);
	int (*cmp)(void *, void *);
	void (*cleanup)(void *);
} ht_t;

#ifdef __KERNEL__
#define HT_ALLOC(__s) (kmalloc((__s), GFP_KERNEL))
#define HT_FREE(__p) kfree(__p)
#else
#define HT_ALLOC(__s) (malloc(__s))
#define HT_FREE(__p) free(__p)
#endif

typedef struct {
	ht_t *table;
	int current_list;
	dlist_node_t *current_node;
} ht_iter_t;


/**
 * ht_create:
 * Create a hash table
 * @args n_buckets:	Number of buckets, use a prime number to minimize hash
 *					collisions
 * @args hash:		hash function
 * @args cmp:		Key matching function to compare 2 entries in your
 *					hash table should return 0, if the passed values are equal
 *					Should return != 0, if they are not equal
 * @args cleanup:	Cleanup routine for every hash table value
 * @returns:		Constructed hash table
 */
ht_t *ht_create(int n_buckets, int (*hash)(void *),
				int (*cmp)(void *, void *),
				void (*cleanup)(void *));

/**
 * ht_add:
 * Add value @val to hash table @ht
 * @returns	0:	Success
 * @returns 1:	Failure
 */
int ht_add(ht_t *ht, void *val);

/**
 * ht_search:
 * Search value @val in the hash table @ht
 * @returns the required node if search was successful
 * @returns NULL if entry not found in hash table
 */
void *ht_search(ht_t *ht, void *val);

/**
 * ht_remove:
 * Remove an entry from hash table
 * @returns the required node if search was successful
 * @returns NULL if entry not found in hash table
 */
int ht_remove(ht_t *ht, void *val);

/**
 * ht_cleanup:
 * Clear hash table
 * @returns 0 if successful
 */
int ht_cleanup(ht_t *);

/** Hash table Iterator functions **/
/**
 * ht_iter_init:
 * Intialize Hash table iterator
 * @args iter:	Iterator to be initialized
 * @args ht:	Hash table on which this iterator should run
 * @returns 0 if successful
 */
int ht_iter_init(ht_iter_t *iter, ht_t *ht);

/**
 * ht_iter_data:
 * Return data pointed to by iterator
 * @args iter:	Iterator
 * @returns pointer to data
 */
void *ht_iter_data(ht_iter_t *iter);

/**
 * ht_iter_next:
 * iterator++
 * @args iter:	Iterator
 */
void ht_iter_next(ht_iter_t *iter);
#endif
