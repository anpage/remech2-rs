#ifndef RESOURCECACHE_H
#define RESOURCECACHE_H

#include "decomp.h"
#include "types.h"

// A cached resource, followed by its data. DumpResourceCache prints the fields as "ID", "Type"
// and "Lock". Unlocked entries sit on the purge list, oldest first.
// SIZE 0x14
typedef struct ResourceCacheEntry {
	MechS16 m_id;                           // 0x00
	MechS16 m_lock;                         // 0x02
	undefined4 m_type;                      // 0x04 — the four-character type tag
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
	extern MechS32 g_cacheDumpNumber;
	extern MechS32 g_cacheEntryCount;
	extern ResourceCacheEntry* g_purgeListHead;
	extern ResourceCacheEntry* g_purgeListTail;

	void InitializeResourceCache(void);
	void UnlockCachedResource(MechS32 p_id, char* p_type);
	void* LoadCachedResource(MechS32 p_handle, MechS32 p_id, char* p_type, MechS32 p_unk0x0c);
	void* AllocateMemory(undefined4 p_size);
	void ShutdownResourceCache(void);
	void FreeCachedResource(MechS32 p_id, char* p_type);

#ifdef __cplusplus
}
#endif

#endif // RESOURCECACHE_H
