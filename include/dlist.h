#ifndef __D_LIST_H__
#define __D_LIST_H__

#include <stdlib.h>

struct dlist_node;

typedef struct dlist_node {
	void *data;
	struct dlist_node *next, *prev;
} dlist_node_t;

#ifdef __KERNEL__
#define DLIST_ALLOC(__s) (kmalloc((__s), GFP_KERNEL))
#define DLIST_FREE(__p) kfree(__p)
#else
#define DLIST_ALLOC(__s) (malloc(__s))
#define DLIST_FREE(__p) free(__p)
#endif

dlist_node_t *list_new_node(void);
dlist_node_t *list_create(void);
int dlist_insert_tail(dlist_node_t **, void *);
int dlist_insert_head(dlist_node_t **, void *);
dlist_node_t *dlist_next(dlist_node_t *);
void *dlist_data(dlist_node_t *);
void dlist_free_node(dlist_node_t *, void (*rem)(void *));
int dlist_remove_head(dlist_node_t **, void (*rem)(void *));
int dlist_remove_node(dlist_node_t **head, dlist_node_t **node,
					  void (*cleanup)(void *));
int dlist_init(dlist_node_t **);
void dlist_dump(dlist_node_t *, void (*printer)(dlist_node_t *));
void *dlist_search(dlist_node_t **list, void *key, int (*cmp)(void *, void*));
int dlist_find_n_remove_node(dlist_node_t **head, void *key,
							 int (*cmp)(void *, void *),
							 void (*cleanup)(void *));
void dlist_flush(dlist_node_t **list, void (*cleanup)(void *));
#endif /* __LIST_H__ */
