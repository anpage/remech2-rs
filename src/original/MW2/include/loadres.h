#ifndef LOADRES_H
#define LOADRES_H

#include "decomp.h"
#include "types.h"

// The header of a cached resource: its ID, type and lock count (the cache log prints
// "ID %5d Type %4s Lock %d Size %7d"), its hash chain in g_cacheTable and its place in the
// purge list of unlocked items. The resource data follows.
// SIZE 0x14
typedef struct ResourceCacheEntry {
	MechS16 m_id;                           // 0x00
	MechS16 m_lock;                         // 0x02
	MechS32 m_type;                         // 0x04 — four characters
	struct ResourceCacheEntry* m_next;      // 0x08
	struct ResourceCacheEntry* m_purgeNext; // 0x0c
	struct ResourceCacheEntry* m_purgePrev; // 0x10
} ResourceCacheEntry;

// The functions and globals of loadres.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_cacheEntryCount;

	void UnlockCacheEntry(ResourceCacheEntry* p_item);
	void LockCacheEntry(ResourceCacheEntry* p_item);
	void AllocateCacheTable(void);
	void RebuildPurgeList(void);
	void ShutdownResourceCache(void);
	void InitializeResourceCache(void);
	void FUN_10019da1(void);
	ResourceCacheEntry* FindCacheEntry(MechS32 p_id, const char* p_type);
	void FreeCacheEntry(ResourceCacheEntry* p_item);
	void DumpResourceCache(void);
	void FUN_1001a158(void);
	void UnlockCachedResource(MechS32 p_id, const char* p_type);
	void* LoadCachedResource(MechS32 p_file, MechS32 p_id, const char* p_type, undefined4 p_unk0x0c);
	void FreeCachedResource(MechS32 p_id, const char* p_type);
	void FUN_1001a521(undefined4 p_unk0x00);
	undefined4 FUN_1001a52c(undefined4 p_unk0x00);
	void* FUN_1001a53f(MechS32 p_id, const char* p_type);
	MechS32 PurgeOldestCacheEntry(void);
	void* MemAlloc(MechU32 p_size);
	void* MemCopy(void* p_dst, const void* p_src, MechU32 p_size);
	void* MemSet(void* p_dst, MechS32 p_value, MechU32 p_size);
	void MemFree(void* p_block);

#ifdef __cplusplus
}
#endif

#endif // LOADRES_H
