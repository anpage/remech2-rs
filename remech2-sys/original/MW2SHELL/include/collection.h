#ifndef COLLECTION_H
#define COLLECTION_H

#include "decomp.h"
#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

	enum {
		c_collectionSorted = 0x02
	};

#pragma pack(1)
	// A growable array of item pointers, optionally kept sorted by m_compare.
	typedef struct Collection {
		int32_t m_capacity;                         // 0x00
		int32_t m_count;                            // 0x04
		int32_t m_growBy;                           // 0x08
		uint8_t m_flags;                            // 0x0c
		int32_t m_unsorted;                         // 0x0d
		void** m_items;                             // 0x11
		void (*m_destroyItem)(void*);               // 0x15
		int (*m_compare)(const void*, const void*); // 0x19
	} Collection;
#pragma pack()

	int32_t CreateCollection(
		Collection** p_collection,
		int32_t p_growBy,
		void (*p_destroyItem)(void*),
		uint8_t p_flags,
		int (*p_compare)(const void*, const void*)
	);
	int32_t DestroyCollection(Collection* p_collection);
	int32_t ExpandCollection(Collection* p_collection, void* p_item);
	void CollectionRemove(Collection* p_collection, void* p_item, uint8_t p_destroy);
	int32_t CollectionFind(Collection* p_collection, void* p_item);
	void SortCollection(Collection* p_collection);
	void EnsureCollectionSorted(Collection* p_collection);
	void* CollectionGet(Collection* p_collection, int32_t p_index);
	void MoveCollectionItems(Collection* p_collection, Collection* p_source);
	void ClearCollection(Collection* p_collection);

#ifdef __cplusplus
}
#endif

#endif // COLLECTION_H
