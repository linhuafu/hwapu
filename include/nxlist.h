#ifndef __LIST_H
#define __LIST_H

#include <stdio.h>
#include <stdlib.h>
#include "nxutils.h"

/* single linked list data structure*/

typedef struct _slist {
    struct _slist *next;	/* next item */
} slist_t;

/* double linked list data structure*/

typedef struct _dlist {
    struct _dlist *next;	/* next item */
    struct _dlist *prev;	/* previous item */
} dlist_t;

#define INIT_SLIST_NODE(name)	name = { NULL }
#define INIT_DLIST_NODE(name)	name = { &(name), &(name) }

#define DEFINE_SLIST_HEAD(name)	slist_t INIT_SLIST_NODE(name)
#define DEFINE_DLIST_HEAD(name)	dlist_t INIT_DLIST_NODE(name)

#define nxSlistEmpty(head)		((head)->next == NULL)
#define nxDlistEmpty(head)		((head)->next == (head))

/**
 * Insert new item right after the current item.
 *
 * @param curItem
 *        current item.
 * @param newItem
 *        inserting item.
 */
void nxSlistInsert(slist_t * curItem, slist_t * newItem);

/**
 * Add new item at the tail.
 *
 * @param newItem
 *        inserting item.
 */
void nxSlistAdd(slist_t * head, slist_t * newItem);

/**
 * Remove specified item.
 *
 * @param head
 *        head item.
 * @param item
 *        removing item.
 * @return
 *        the removed item, NULL if item is not in list
 */
slist_t* nxSlistRemove(slist_t * head, slist_t * item);

/**
 * Return current list tail.
 *
 * @param head
 *        head item.
 * @return
 *        list tail.
 */
slist_t* nxSlistTail(slist_t * head);

/**
 * destory current list to tail.
 *
 * @param head
 *        head item.
 */
void nxSlistFree(slist_t * head, size_t off);

/**
 * Iterate the list following the next elements util tail is found.
 * The callback function will be called once for each element.
 * You should return 1 from the callback if you want to stop the iteration.
 *
 * @param start
 *        Where to start iterating. the callback will be called for this item too.
 * @param cb
 *        The callback function that will be called.
 * @param data
 *        A pointer to arbitrary user data. It will be passed to the callback at each iteration.
 * @return
 *        The item that callback function return 1.
 */
slist_t* nxSlistIterate(slist_t * head, int (*cb)(slist_t * item, void * userdata), void* userdata);

/**
 * Remove the item that callback function return >= 0.
 * The callback function will be called once for each element.
 * You should return 1 from the callback if you want to stop the iteration.
 *
 * @param head
 *        head item.
 * @param compr
 *        matching function.
 * @param data
 *        matching target.
 * @return
 *        the last removed item.
 */
slist_t* nxSlistRemoveEx(slist_t * head, int (*cb)(slist_t * item, void *userdata), void *userdata);


/**
 * Insert new item either after the current item.
 *
 * @param curItem
 *        current item.
 * @param newItem
 *        inserting item.
 */
void nxDlistInsertAfter(dlist_t * curItem, dlist_t * newItem);

/**
 * Insert new item either before the current item.
 *
 * @param curItem
 *        current item.
 * @param newItem
 *        inserting item.
 */
void nxDlistInsertBefore(dlist_t * curItem, dlist_t * newItem);

/**
 * Remove specified item.
 *
 * @param curItem
 *        item to be removed.
 */
void nxDlistRemove(dlist_t * curItem);

/**
 * Find a item in the list by index.
 *
 * @param start
 *        the start item begin to search
 * @param index
 *        the index of the item be searched
 * @retrun
 *        pointer of the item, if no found return NULL.
 */
dlist_t* nxDlistIndex(dlist_t * head, dlist_t * start, int index);

/**
 * Iterate the list following the next elements util tail is found.
 * The callback function will be called once for each element.
 * You should return 1 from the callback if you want to stop the iteration.
 *
 * @param start
 *        Where to start iterating. the callback will be called for this item too.
 * @param cb
 *        The callback function that will be called.
 * @param data
 *        A pointer to arbitrary user data. It will be passed to the callback at each iteration.
 * @return
 *        The node that callback function return 1.
 */
dlist_t* nxDlistIteratePrev(dlist_t * head, dlist_t * start, int (*cb)(dlist_t * item, void * userdata), void* userdata);

/**
 * Iterate the list following the next elements util tail is found.
 * The callback function will be called once for each element.
 * You should return 1 from the callback if you want to stop the iteration.
 *
 * @param start
 *        Where to start iterating. the callback will be called for this item too.
 * @param cb
 *        The callback function that will be called.
 * @param data
 *        A pointer to arbitrary user data. It will be passed to the callback at each iteration.
 * @return
 *        The node that callback function return 1.
 */
dlist_t* nxDlistIterateNext(dlist_t * head, dlist_t * start, int (*cb)(dlist_t * item, void * userdata), void* userdata);

#endif /* __LIST_H */
