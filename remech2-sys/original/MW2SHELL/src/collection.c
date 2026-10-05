#include "collection.h"

#include "decomp.h"
#include "types.h"
#include "windowstate.h"

#include <search.h>
#include <stdio.h>
#include <stdlib.h>

// The original frees through a macro: its trailing empty `else` emits a `jmp` to the next statement.
#define HEAP_FREE(p_mem)                                                                                               \
	if ((p_mem) != NULL) {                                                                                             \
		MechHeapFree(g_primaryHeap, (p_mem));                                                           \
		(p_mem) = NULL;                                                                                                \
	}                                                                                                                  \
	else

// Stack-slot permutation: collection, items and i take different [ebp-N] slots than the original.
// FUNCTION: MW2SHELL 0x1003c0a0
MechS32 CreateCollection(
	Collection** p_collection,
	MechS32 p_growBy,
	void (*p_destroyItem)(void*),
	MechU8 p_flags,
	int (*p_compare)(const void*, const void*)
)
{
	Collection* collection;
	void** items;
	MechS32 i;

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

// FUNCTION: MW2SHELL 0x1003c1dc
MechS32 DestroyCollection(Collection* p_collection)
{
	MechS32 i;
	void* item;

	for (i = 0; i < p_collection->m_capacity; i++) {
		item = CollectionGet(p_collection, i);
		if (item != NULL) {
			p_collection->m_destroyItem(item);
		}
	}

	HEAP_FREE(p_collection->m_items);

	HEAP_FREE(p_collection);

	return 0;
}

// FUNCTION: MW2SHELL 0x1003c296
MechS32 ExpandCollection(Collection* p_collection, void* p_item)
{
	void** items;
	MechS32 i;

	if (p_collection->m_count > p_collection->m_capacity - 1) {
		items = p_collection->m_items;
		p_collection->m_capacity += p_collection->m_growBy;

		if (items != NULL) {
			items =
				(void**) MechHeapReAlloc(g_primaryHeap, items, p_collection->m_capacity * sizeof(void*));
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

// Stack-slot permutation: index and item swap [ebp-N] slots with the original.
// FUNCTION: MW2SHELL 0x1003c3ba
void CollectionRemove(Collection* p_collection, void* p_item, MechU8 p_destroy)
{
	MechS32 index;
	MechS32 i;
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

// Stack-slot permutation: found, item, key and i take different [ebp-N] slots than the original.
// FUNCTION: MW2SHELL 0x1003c45f
MechS32 CollectionFind(Collection* p_collection, void* p_item)
{
	void** found;
	void* item;
	void* key;
	MechS32 i;

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

// FUNCTION: MW2SHELL 0x1003c55d
void SortCollection(Collection* p_collection)
{
	if (p_collection->m_unsorted == 1) {
		qsort(p_collection->m_items, p_collection->m_count, sizeof(void*), p_collection->m_compare);
		p_collection->m_unsorted = 0;
	}
}

// FUNCTION: MW2SHELL 0x1003c59e
void EnsureCollectionSorted(Collection* p_collection)
{
	if ((p_collection->m_flags & c_collectionSorted) && p_collection->m_unsorted == 1) {
		qsort(p_collection->m_items, p_collection->m_count, sizeof(void*), p_collection->m_compare);
		p_collection->m_unsorted = 0;
	}
}

// FUNCTION: MW2SHELL 0x1003c5f0
void* CollectionGet(Collection* p_collection, MechS32 p_index)
{
	EnsureCollectionSorted(p_collection);

	if (p_collection->m_count < p_index || p_index < 0) {
		return NULL;
	}

	return p_collection->m_items[p_index];
}

// FUNCTION: MW2SHELL 0x1003c638
void MoveCollectionItems(Collection* p_collection, Collection* p_source)
{
	MechS32 i;

	for (i = 0; i < p_source->m_count; i++) {
		ExpandCollection(p_collection, p_source->m_items[i]);
		p_source->m_items[i] = NULL;
	}

	ClearCollection(p_source);
}

// Operand order: the original loads i before m_items for m_items[i]; i and item also take
// different [ebp-N] slots than the original.
// FUNCTION: MW2SHELL 0x1003c69e
void ClearCollection(Collection* p_collection)
{
	MechS32 i;
	void** items;
	void* item;

	for (i = 0; i < p_collection->m_count; i++) {
		item = p_collection->m_items[i];
		if (item != NULL) {
			p_collection->m_destroyItem(item);
		}
	}

	HEAP_FREE(p_collection->m_items);

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
