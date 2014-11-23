#include "nxlist.h"

/**
 * Insert new item right after the current item.
 *
 * @param curItem
 *        current item.
 * @param newItem
 *        inserting item.
 */
void nxSlistInsert( slist_t * curItem, slist_t * newItem )
{
	newItem->next = curItem->next;
	curItem->next = newItem;
}

/**
 * Add new item at the tail.
 *
 * @param newItem
 *        inserting item.
 */
void nxSlistAdd(slist_t * head, slist_t * newItem)
{
  	slist_t * cur = head;

	while (cur->next)
		cur = cur->next;

	cur->next = newItem;
	newItem->next = NULL;
}

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
slist_t* nxSlistRemove( slist_t * head, slist_t * item )
{
	slist_t * pre;

	for (pre = head; pre->next != NULL; pre = pre->next) {
		if (pre->next == item) {
			pre->next = item->next;
			return item;
		}
	}

	return NULL;
}

/**
 * Return current list tail.
 *
 * @param head
 *        head item.
 * @return
 *        list tail.
 */
slist_t* nxSlistTail( slist_t * head )
{
  	slist_t * cur = head;

	while (cur->next)
		cur = cur->next;

	return cur;
}

/**
 * destory current list to tail.
 *
 * @param head
 *        head item.
 */
void
nxSlistFree( slist_t * head, size_t off )
{
  	slist_t * cur = head->next;
	head->next = NULL;

	while (cur != NULL) {
		void *tmp = (char *)cur - off;
		cur = cur->next;
		free(tmp);
	}
}

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
slist_t* nxSlistIterate(slist_t * head, int (*cb)(slist_t * item, void * userdata), void* userdata)
{
	slist_t * cur = head->next;

	while (cur != NULL) {
		slist_t * next = cur->next;
		if (cb(cur, userdata) == 1)
			return cur;
		cur = next;
	}

	return NULL;
}

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
slist_t* nxSlistRemoveEx(slist_t * head, int (*cb)(slist_t * item, void *userdata), void *userdata)
{
	slist_t * res = NULL;
	slist_t * pre;

	for (pre = head; pre->next != NULL; ) {
		/* because cur may be freed in the callback function,
		 * so we prefetch it first
		 */
		slist_t * cur = pre->next;
		slist_t * next = cur->next;

		int ret = cb(cur, userdata);
		if (ret >= 0) {
			res = cur;
			pre->next = next;
			if (ret == 1)
				break;
		} else
			pre = cur;
	}

	return res;
}


/**
 * Insert new item either after the current item.
 *
 * @param curItem
 *        current item.
 * @param newItem
 *        inserting item.
 * @return
 *        pointer to newItem after addition.
 */
void nxDlistInsertAfter(dlist_t * curItem, dlist_t * newItem)
{
	curItem->next->prev = newItem;
	newItem->next = curItem->next;
	newItem->prev = curItem;
	curItem->next = newItem;
}

/**
 * Insert new item either before the current item.
 *
 * @param curItem
 *        current item.
 * @param newItem
 *        inserting item.
 * @return
 *        pointer to newItem after addition.
 */
void nxDlistInsertBefore(dlist_t * curItem, dlist_t * newItem)
{
	curItem->prev->next = newItem;
	newItem->prev = curItem->prev;
	curItem->prev = newItem;
	newItem->next = curItem;
}

/**
 * Remove specified item.
 *
 * @param item
 *        item to be removed.
 */
void nxDlistRemove(dlist_t * curItem)
{
	curItem->prev->next = curItem->next;
	curItem->next->prev = curItem->prev;
}

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
dlist_t* nxDlistIndex(dlist_t * head, dlist_t * start, int index)
{
    dlist_t * item = start;

	if (start == head)
		return NULL;

    if (index > 0)
	{
		do {
			if (item->next == head)
				return NULL;
			item = item->next;
		} while (--index);
	}
	else if (index < 0)
	{
		do {
			if (item->prev == head )
				return NULL;
			item = item->prev;
		} while (++index);
	}

    return item;
}

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
dlist_t* nxDlistIterateNext (dlist_t * head, dlist_t * start, int (*cb)(dlist_t * item, void * userdata), void* userdata)
{
	while (start != head) {
		dlist_t * next = start->next;
		if (cb(start, userdata) == 1)
			return start;
		start = next;
	}

	return NULL;
}

/**
 * Iterate the list following the prev elements util head is found.
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
dlist_t* nxDlistIteratePrev (dlist_t * head, dlist_t * start, int (*cb)(dlist_t * item, void * userdata), void* userdata)
{
	while (start != head) {
		dlist_t * prev = start->prev;
		if (cb(start, userdata) == 1)
			return start;
		start = prev;
	}

	return NULL;
}
