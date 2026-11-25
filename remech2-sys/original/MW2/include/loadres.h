#ifndef LOADRES_H
#define LOADRES_H

#include "types.h"

// The header of a cached resource: its ID, type and lock count (the cache log prints
// "ID %5d Type %4s Lock %d Size %7d"), its hash chain in g_cacheTable and its place in the
// purge list of unlocked items. The resource data follows.
typedef struct ResourceCacheEntry {
	int16_t m_id;
	int16_t m_lock;
	int32_t m_type; // four characters
	struct ResourceCacheEntry* m_next;
	struct ResourceCacheEntry* m_purgeNext;
	struct ResourceCacheEntry* m_purgePrev;
} ResourceCacheEntry;

// The functions and globals of loadres.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_cacheEntryCount;
	extern ResourceCacheEntry** g_cacheTable;
	extern int32_t g_cacheDumpNumber;
	extern ResourceCacheEntry* g_purgeListHead;
	extern ResourceCacheEntry* g_purgeListTail;

	void UnlockCacheEntry(ResourceCacheEntry* p_item);
	void LockCacheEntry(ResourceCacheEntry* p_item);
	void AllocateCacheTable(void);
	void RebuildPurgeList(void);
	void ShutdownResourceCache(void);
	void InitializeResourceCache(void);
	ResourceCacheEntry* FindCacheEntry(int32_t p_id, const char* p_type);
	void FreeCacheEntry(ResourceCacheEntry* p_item);
	void DumpResourceCache(void);
	void UnlockCachedResource(int32_t p_id, const char* p_type);
	void* LoadCachedResource(int32_t p_file, int32_t p_id, const char* p_type);
	void FreeCachedResource(int32_t p_id, const char* p_type);
	int32_t PurgeOldestCacheEntry(void);
	void* MemAlloc(uint32_t p_size);
	void* MemCopy(void* p_dst, const void* p_src, uint32_t p_size);
	void* MemSet(void* p_dst, int32_t p_value, uint32_t p_size);
	void MemFree(void* p_block);

#ifdef __cplusplus
}
#endif

#endif // LOADRES_H
