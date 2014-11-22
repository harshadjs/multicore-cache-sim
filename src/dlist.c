/**
 * A doubly linked list implementation
 * - Harshad Shirwadkar(harshad@cmu.edu)
 */

#include <string.h>
#include "dlist.h"

/** Doubly Linked List functions **/
inline dlist_node_t *dlist_new_node(void)
{
	dlist_node_t *node = (dlist_node_t *)DLIST_ALLOC(sizeof(dlist_node_t));

	memset(node, 0, sizeof(dlist_node_t));
	return node;
}


/** create a linked list **/
inline dlist_node_t *dlist_create(void)
{
	return dlist_new_node();
}

/**
 * dlist_insert_tail:
 * Inserts a node at the tail of list.
 * @args **list:	Double pointer to head of the list
 * @args *data:		Pointer to data
 * @returns	0:		Success
 *			1:		Failure
 */
int dlist_insert_tail(dlist_node_t **list, void *data)
{
	dlist_node_t *p, *q;

	p = (*list);

	if(!p) {
		/* It's an empty list */
		*list = dlist_create();
		if(!(*list))
			return 1; /* dlist_create failed to allocate */

		(*list)->data = data;
		return 0;
	}

	while(p->next) {
		p = p->next;
	}

	q = dlist_new_node();
	if(q == NULL) {
		/* dlist_new_node failed to allocate memory */
		return 1;
	}

	p->next = q;

	q->prev = p;
	q->next = NULL;
	q->data = data;

	return 0;
}

/**
 * dlist_insert_head:
 * Inserts a node at the head of list.
 * @args **list:	Double pointer to head of the list
 * @args *data:		Pointer to data
 * @returns	0:		Success
 *			1:		Failure
 */
int dlist_insert_head(dlist_node_t **list, void *data)
{
	dlist_node_t *p;

	p = dlist_new_node();
	p->data = data;

	p->next = *list;
	p->prev = NULL;
	if(*list) {
		(*list)->prev = p;
	}

	(*list) = p;

	return 0;
}

/* dlist_next returns the next node. Should be used while iterating
 * through the list */
inline dlist_node_t *dlist_next(dlist_node_t *list)
{
	return (list) ? (list->next) : NULL;
}

/* Fetch the data from the node */
void *dlist_data(dlist_node_t *node)
{
	return (node) ? (node->data) : NULL;
}

/**
 * dlist_insert_head:
 * Inserts a node at the head of list.
 * @args **list:	Double pointer to head of the list
 * @args *key:		Pointer to key
 * @returns	found data
 * @returns NULL if data not found
 */
void *dlist_search(dlist_node_t **list, void *key, int (*cmp)(void *, void*))
{
	dlist_node_t *p = *list;

	while(p) {
		if(cmp(key, dlist_data(p)) == 0)
			return dlist_data(p);
		p = dlist_next(p);
	}

	return NULL;
}

dlist_node_t *dlist_search_dlist_node(dlist_node_t **list, void *key, int (*cmp)(void *, void*))
{
	dlist_node_t *p = *list;

	while(p) {
		if(cmp(key, dlist_data(p)) == 0)
			return p;
		p = dlist_next(p);
	}

	return NULL;
}

inline void dlist_free_node(dlist_node_t *node, void (*cleanup)(void *))
{
	cleanup(node->data);
	DLIST_FREE(node);
}

/**
 * dlist_remove_head:
 * Removes head node from the list. Returs 0 on success and 1 on
 * failure.
 */
int dlist_remove_head(dlist_node_t **head, void (*cleanup)(void *))
{
	dlist_node_t *p;

	if(!head || !*head) {
		return 1;
	}

	p = (*head)->next;
	dlist_free_node(*head, cleanup);

	*head = p;
	if(*head) {
		(*head)->prev = NULL;
	}
	return 0;
}

int dlist_remove_node(dlist_node_t **head, dlist_node_t **node,
					  void (*cleanup)(void *))
{
	dlist_node_t *remember;

	if((*node)->next)
		(*node)->next->prev = (*node)->prev;

	if((*node)->prev) {
		(*node)->prev->next = (*node)->next;
		remember = (*node)->prev;
	} else {
		(*head) = (*node)->next;
		remember = *head;
	}
	dlist_free_node(*node, cleanup);

	*node = remember;
	return 0;
}

/**
 * dlist_remove_head:
 * Removes head node from the list. Returs 0 on success and 1 on
 * failure.
 */
int dlist_find_n_remove_node(dlist_node_t **head, void *key,
							 int (*cmp)(void *, void *),
							 void (*cleanup)(void *))
{
	dlist_node_t *p;

	p = dlist_search_dlist_node(head, key, cmp);
	if(!p)
		return 1;

	return dlist_remove_node(head, &p, cleanup);
}

/* Initialize list pointer to null */
int dlist_init(dlist_node_t **list)
{
	if(!list)
		return 1;
	*list = NULL;

	return 0;
}

void dlist_dump(dlist_node_t *list, void (*printer)(dlist_node_t *))
{
	dlist_node_t *p = list;

	while(p) {
		(*printer)(p);
		p = dlist_next(p);
	}
}

void dlist_flush(dlist_node_t **list, void (*cleanup)(void *))
{
	dlist_node_t *next;

	while(*list) {
		next = (*list)->next;
		dlist_remove_head(list, cleanup);
		*list = next;
	}
}
