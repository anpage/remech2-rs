#ifndef HEAP_H
#define HEAP_H

// The game's private heap. Originally a Win32 heap (HeapCreate with HEAP_NO_SERIALIZE), now
// implemented on the Rust side. Like the original it isn't synchronised, and destroying it frees
// every block still allocated from it.

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct MechHeap MechHeap;

	MechHeap* MechHeapCreate(void);
	void MechHeapDestroy(MechHeap* p_heap);

	// NULL when out of memory
	void* MechHeapAlloc(MechHeap* p_heap, size_t p_size);
	void* MechHeapAllocZeroed(MechHeap* p_heap, size_t p_size);
	// The block is untouched when this returns NULL
	void* MechHeapReAlloc(MechHeap* p_heap, void* p_block, size_t p_size);
	// Non-zero on success. Freeing a block that isn't allocated from the heap fails harmlessly.
	int MechHeapFree(MechHeap* p_heap, void* p_block);
	// The size the block was allocated with, (size_t) -1 if it isn't allocated from the heap
	size_t MechHeapSize(MechHeap* p_heap, const void* p_block);

#ifdef __cplusplus
}
#endif

#endif // HEAP_H
