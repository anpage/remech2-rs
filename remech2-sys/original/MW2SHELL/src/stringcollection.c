#include "collection.h"
#include "stringutil.h"
#include "types.h"

#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// String collections: Collections of heap-copied strings, ordered by strcmp when sorted.

uint8_t FindString(Collection* p_collection, char* p_string, char** p_found);
int CompareStringEntries(const void* p_a, const void* p_b);

Collection* CreateStringCollection(int32_t p_flags)
{
	Collection* collection;
	int32_t result;

	result = CreateCollection(&collection, 10, free, (uint8_t) p_flags, CompareStringEntries);
	if (result != 0) {
		return NULL;
	}

	return collection;
}

uint8_t AddUniqueString(Collection* p_collection, char* p_string)
{
	char* copy;

	if (p_string == NULL) {
		return TRUE;
	}

	if (FindString(p_collection, p_string, NULL) == TRUE) {
		return FALSE;
	}

	copy = AllocateString(p_string);
	ExpandCollection(p_collection, copy);
	return TRUE;
}

uint8_t FindString(Collection* p_collection, char* p_string, char** p_found)
{
	int32_t i;
	char** entry;
	char* item;

	if (p_collection->m_flags & c_collectionSorted) {
		EnsureCollectionSorted(p_collection);
		entry =
			bsearch(&p_string, p_collection->m_items, p_collection->m_count, sizeof(void*), p_collection->m_compare);
		if (entry == NULL) {
			return FALSE;
		}

		item = *entry;
		if (item != NULL) {
			if (p_found != NULL) {
				*p_found = item;
			}

			return TRUE;
		}
	}
	else {
		for (i = 0; i < p_collection->m_count; i++) {
			item = CollectionGet(p_collection, i);
			if (!strcmp(item, p_string)) {
				if (p_found != NULL) {
					*p_found = item;
				}

				return TRUE;
			}
		}
	}

	return FALSE;
}

// Moves every string of p_source into p_collection, freeing the ones it already holds.
void MoveStrings(Collection* p_collection, Collection* p_source)
{
	int32_t i;
	char* item;

	for (i = 0; i < p_source->m_count; i++) {
		item = CollectionGet(p_source, i);
		p_source->m_items[i] = NULL;
		if (!AddUniqueString(p_collection, item)) {
			p_source->m_destroyItem(item);
			p_source->m_items[i] = NULL;
		}
	}

	ClearCollection(p_source);
}

void RemoveString(Collection* p_collection, char* p_string)
{
	CollectionRemove(p_collection, p_string, TRUE);
}

void AddString(Collection* p_collection, char* p_string)
{
	char* copy;

	copy = AllocateString(p_string);
	ExpandCollection(p_collection, copy);
}

uint8_t StringsEqualAt(Collection* p_a, int32_t p_indexA, Collection* p_b, int32_t p_indexB)
{
	char* a;
	char* b;

	a = CollectionGet(p_a, p_indexA);
	b = CollectionGet(p_b, p_indexB);
	if (!strcmp(a, b)) {
		return TRUE;
	}

	return FALSE;
}

void DumpStrings(char* p_title, Collection* p_collection)
{
	int32_t i;

	fprintf(stdout, "%s\n", p_title);
	fflush(stdout);
	for (i = 0; i < p_collection->m_count; i++) {
		fprintf(stdout, "%d, %s\n", i, (char*) CollectionGet(p_collection, i));
		fflush(stdout);
	}

	fprintf(stdout, "\n");
	fflush(stdout);
}

Collection* CopyStringCollection(Collection* p_source)
{
	char* copy;
	int32_t i;
	Collection* collection;
	char* item;

	collection = CreateStringCollection(p_source->m_flags);
	for (i = 0; i < p_source->m_count; i++) {
		item = CollectionGet(p_source, i);
		copy = AllocateString(item);
		ExpandCollection(collection, copy);
	}

	return collection;
}

int CompareStringEntries(const void* p_a, const void* p_b)
{
	char* a;
	char* b;
	char** entryA;
	char** entryB;

	entryA = (char**) p_a;
	entryB = (char**) p_b;
	a = *entryA;
	b = *entryB;
	return strcmp(a, b);
}
