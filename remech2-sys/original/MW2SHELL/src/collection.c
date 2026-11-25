#include "collection.h"

#include "types.h"
#include "windowstate.h"

#include <search.h>
#include <stdio.h>
#include <stdlib.h>

int32_t CreateCollection(
	Collection** p_collection,
	int32_t p_growBy,
	void (*p_destroyItem)(void*),
	uint8_t p_flags,
	int (*p_compare)(const void*, const void*)
)
{
	Collection* collection;
	void** items;
	int32_t i;

	collection = (Collection*) MechHeapAlloc(g_primaryHeap, sizeof(Collection));
	if (collection == NULL) {
		fprintf(stderr, "Could not allocate collection\n");
		fflush(stderr);
		exit(1);
	}

	*p_collection = collection;
	collection->m_growBy = p_growBy;
	collection->m_count = 0;
	collection->m_destroyItem = p_destroyItem;
	collection->m_capacity = collection->m_growBy;
	collection->m_flags = p_flags;
	collection->m_unsorted = 0;
	collection->m_compare = p_compare;

	items = (void**) MechHeapAlloc(g_primaryHeap, collection->m_growBy * sizeof(void*));
	if (items == NULL) {
		fprintf(stderr, "Out of memory in create collection\n");
		fflush(stderr);
		exit(1);
	}

	for (i = 0; i < collection->m_capacity; i++) {
		items[i] = NULL;
	}

	collection->m_items = items;
	return 0;
}

int32_t DestroyCollection(Collection* p_collection)
{
	int32_t i;
	void* item;

	for (i = 0; i < p_collection->m_capacity; i++) {
		item = CollectionGet(p_collection, i);
		if (item != NULL) {
			p_collection->m_destroyItem(item);
		}
	}

	MechHeapFree(g_primaryHeap, p_collection->m_items);

	MechHeapFree(g_primaryHeap, p_collection);

	return 0;
}

int32_t ExpandCollection(Collection* p_collection, void* p_item)
{
	void** items;
	int32_t i;

	if (p_collection->m_count > p_collection->m_capacity - 1) {
		items = p_collection->m_items;
		p_collection->m_capacity += p_collection->m_growBy;

		if (items != NULL) {
			items = (void**) MechHeapReAlloc(g_primaryHeap, items, p_collection->m_capacity * sizeof(void*));
		}
		else {
			items = (void**) MechHeapAlloc(g_primaryHeap, p_collection->m_capacity * sizeof(void*));
		}

		if (items == NULL) {
			fprintf(stderr, "Out of memory in expand collection\n");
			fflush(stderr);
			exit(1);
		}

		for (i = p_collection->m_capacity - p_collection->m_growBy; i < p_collection->m_capacity; i++) {
			items[i] = NULL;
		}

		p_collection->m_items = items;
	}

	items = p_collection->m_items;
	items[p_collection->m_count] = p_item;
	p_collection->m_count++;
	p_collection->m_unsorted = 1;
	return 0;
}

void CollectionRemove(Collection* p_collection, void* p_item, uint8_t p_destroy)
{
	int32_t index;
	int32_t i;
	void* item;

	index = CollectionFind(p_collection, p_item);
	if (index != -1) {
		p_collection->m_count--;

		if (p_destroy == 1) {
			item = p_collection->m_items[index];
			p_collection->m_destroyItem(item);
		}

		if (index < p_collection->m_count) {
			for (i = index; i < p_collection->m_count; i++) {
				p_collection->m_items[i] = p_collection->m_items[i + 1];
			}
		}
	}
}

int32_t CollectionFind(Collection* p_collection, void* p_item)
{
	void** found;
	void* item;
	void* key;
	int32_t i;

	if (p_collection->m_flags & c_collectionSorted) {
		EnsureCollectionSorted(p_collection);
		found = (void**)
			bsearch(&p_item, p_collection->m_items, p_collection->m_count, sizeof(void*), p_collection->m_compare);
		if (found != NULL) {
			return ((char*) found - (char*) p_collection->m_items) / sizeof(void*);
		}
	}
	else {
		for (i = 0; i < p_collection->m_count; i++) {
			item = CollectionGet(p_collection, i);
			key = p_item;

			if (p_collection->m_compare == NULL) {
				if (key == item) {
					return i;
				}
			}
			else if (p_collection->m_compare(&item, &key) == 0) {
				return i;
			}
		}
	}

	return -1;
}

void SortCollection(Collection* p_collection)
{
	if (p_collection->m_unsorted == 1) {
		qsort(p_collection->m_items, p_collection->m_count, sizeof(void*), p_collection->m_compare);
		p_collection->m_unsorted = 0;
	}
}

void EnsureCollectionSorted(Collection* p_collection)
{
	if ((p_collection->m_flags & c_collectionSorted) && p_collection->m_unsorted == 1) {
		qsort(p_collection->m_items, p_collection->m_count, sizeof(void*), p_collection->m_compare);
		p_collection->m_unsorted = 0;
	}
}

void* CollectionGet(Collection* p_collection, int32_t p_index)
{
	EnsureCollectionSorted(p_collection);

	if (p_collection->m_count < p_index || p_index < 0) {
		return NULL;
	}

	return p_collection->m_items[p_index];
}

void MoveCollectionItems(Collection* p_collection, Collection* p_source)
{
	int32_t i;

	for (i = 0; i < p_source->m_count; i++) {
		ExpandCollection(p_collection, p_source->m_items[i]);
		p_source->m_items[i] = NULL;
	}

	ClearCollection(p_source);
}

void ClearCollection(Collection* p_collection)
{
	int32_t i;
	void** items;
	void* item;

	for (i = 0; i < p_collection->m_count; i++) {
		item = p_collection->m_items[i];
		if (item != NULL) {
			p_collection->m_destroyItem(item);
		}
	}

	MechHeapFree(g_primaryHeap, p_collection->m_items);

	p_collection->m_count = 0;
	p_collection->m_unsorted = 0;

	items = (void**) MechHeapAlloc(g_primaryHeap, p_collection->m_growBy * sizeof(void*));
	if (items == NULL) {
		fprintf(stderr, "Out of memory in clear collection\n");
		fflush(stderr);
		exit(1);
	}

	p_collection->m_capacity = p_collection->m_growBy;
	for (i = 0; i < p_collection->m_capacity; i++) {
		items[i] = NULL;
	}

	p_collection->m_items = items;
}
