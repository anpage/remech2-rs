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
	// SIZE 0x1d
	typedef struct Collection {
		MechS32 m_capacity;                         // 0x00
		MechS32 m_count;                            // 0x04
		MechS32 m_growBy;                           // 0x08
		MechU8 m_flags;                             // 0x0c
		MechS32 m_unsorted;                         // 0x0d
		void** m_items;                             // 0x11
		void (*m_destroyItem)(void*);               // 0x15
		int (*m_compare)(const void*, const void*); // 0x19
	} Collection;
#pragma pack()

	MechS32 CreateCollection(
		Collection** p_collection,
		MechS32 p_growBy,
		void (*p_destroyItem)(void*),
		MechU8 p_flags,
		int (*p_compare)(const void*, const void*)
	);
	MechS32 DestroyCollection(Collection* p_collection);
	MechS32 ExpandCollection(Collection* p_collection, void* p_item);
	void CollectionRemove(Collection* p_collection, void* p_item, MechU8 p_destroy);
	MechS32 CollectionFind(Collection* p_collection, void* p_item);
	void SortCollection(Collection* p_collection);
	void EnsureCollectionSorted(Collection* p_collection);
	void* CollectionGet(Collection* p_collection, MechS32 p_index);
	void MoveCollectionItems(Collection* p_collection, Collection* p_source);
	void ClearCollection(Collection* p_collection);

#ifdef __cplusplus
}
#endif

#endif // COLLECTION_H
