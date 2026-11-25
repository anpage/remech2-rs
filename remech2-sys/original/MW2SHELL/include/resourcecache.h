#ifndef RESOURCECACHE_H
#define RESOURCECACHE_H

#include "types.h"

// A cached resource, followed by its data. DumpResourceCache prints the fields as "ID", "Type"
// and "Lock". Unlocked entries sit on the purge list, oldest first.
typedef struct ResourceCacheEntry {
	int16_t m_id;                           // 0x00
	int16_t m_lock;                         // 0x02
	uint32_t m_type;                        // 0x04 — the four-character type tag
	struct ResourceCacheEntry* m_next;      // 0x08 — in the g_cacheTable bucket
	struct ResourceCacheEntry* m_purgeNext; // 0x0c — toward g_purgeListTail
	struct ResourceCacheEntry* m_purgePrev; // 0x10
} ResourceCacheEntry;

// The functions and globals of resourcecache.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern ResourceCacheEntry** g_cacheTable;
	extern int32_t g_cacheDumpNumber;
	extern int32_t g_cacheEntryCount;
	extern ResourceCacheEntry* g_purgeListHead;
	extern ResourceCacheEntry* g_purgeListTail;

	void InitializeResourceCache(void);
	void UnlockCachedResource(int32_t p_id, char* p_type);
	void* LoadCachedResource(int32_t p_handle, int32_t p_id, char* p_type);
	void* AllocateMemory(uint32_t p_size);
	void ShutdownResourceCache(void);
	void FreeCachedResource(int32_t p_id, char* p_type);

#ifdef __cplusplus
}
#endif

#endif // RESOURCECACHE_H
