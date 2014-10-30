#include <string.h>
#include "hash_table.h"

static inline int bucket_index(ht_t *table, void *node)
{
	int hash = table->hash(node);

	if(hash < 0)
		hash = -hash;
	return ((hash) % (table->n_buckets));
}

ht_t *ht_create(int n_buckets, int (*hash)(void *),
				int (*cmp)(void *, void *),
				void (*cleanup)(void *))
{
	ht_t *table;

	table = (ht_t *)HT_ALLOC(sizeof(ht_t));
	if(!table)
		return NULL;

	memset(table, 0, sizeof(ht_t));
	table->n_buckets = n_buckets;
	table->hash = hash;
	table->cmp = cmp;
	table->cleanup = cleanup;

	table->buckets = (dlist_node_t **)HT_ALLOC(sizeof(dlist_node_t *) *
											   n_buckets);
	if(!table->buckets) {
		HT_FREE(table);
		return NULL;
	}

	memset(table->buckets, 0, sizeof(dlist_node_t *) * n_buckets);

	return table;
}

int ht_add(ht_t *table, void *node)
{
	return dlist_insert_head(&(table->buckets[bucket_index(table, node)]),
							 node);
}

void *ht_search(ht_t *table, void *node)
{
	return dlist_search(&(table->buckets[bucket_index(table, node)]),
						node, table->cmp);
}

int ht_remove(ht_t *table, void *node)
{
	return dlist_find_n_remove_node(&(table->buckets[bucket_index(table, node)]),
									node, table->cmp, table->cleanup);
}

int ht_iter_init(ht_iter_t *iter, ht_t *table)
{
	if((!table) || (table->n_buckets == 0) || (!iter))
		return -1;

	iter->table = table;
	iter->current_list = 0;
	iter->current_node = table->buckets[0];

	while(!dlist_data(iter->current_node)) {
		iter->current_list++;
		if(iter->current_list >= iter->table->n_buckets)
			return 0;
		iter->current_node = iter->table->buckets[iter->current_list];
	}
	return 0;
}

void *ht_iter_data(ht_iter_t *iter)
{
	if(!iter)
		return NULL;

	return dlist_data(iter->current_node);
}

void ht_iter_next(ht_iter_t *iter)
{
	if(!iter)
		return;

	if(iter->current_node)
		iter->current_node = iter->current_node->next;


	if(iter->current_node)
		return;

	while(!iter->current_node) {
		iter->current_list++;
		if(iter->current_list >= iter->table->n_buckets)
			return;
		iter->current_node = iter->table->buckets[iter->current_list];
	}
}
