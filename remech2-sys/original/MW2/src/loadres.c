/* In the original, MemCopy and MemSet are C functions whose bodies are an __asm block (rep
   movsd/stosd). This is portable C in their place. */
#include "loadres.h"

#include "decomp.h"
#include "error.h"
#include "files.h"
#include "gamekeys.h"
#include "log.h"
#include "prjfile.h"
#include "simmain.h"
#include "timedoverlays.h"
#include "types.h"

#include <stdio.h>
#include <string.h>

DECOMP_SIZE_ASSERT(ResourceCacheEntry, 0x14)

// The resource cache's hash chains, 0x3f1 of them.
// GLOBAL: MW2 0x100a2c5c
ResourceCacheEntry** g_cacheTable = NULL;

// The number of the next cache log.
// GLOBAL: MW2 0x100a2c60
MechS32 g_cacheDumpNumber = 0;

// GLOBAL: MW2 0x101748d0
MechS32 g_cacheEntryCount;

// The purge list: unlocked items, oldest first.
// GLOBAL: MW2 0x101748d4
ResourceCacheEntry* g_purgeListHead;

// GLOBAL: MW2 0x101748d8
ResourceCacheEntry* g_purgeListTail;
// Unlocks an item and appends it to the purge list.
// FUNCTION: MW2 0x10019af0
void UnlockCacheEntry(ResourceCacheEntry* p_item)
{
	if (p_item->m_lock == 0) {
		return;
	}

	p_item->m_lock = 0;
	if (g_purgeListTail) {
		g_purgeListTail->m_purgeNext = p_item;
	}

	p_item->m_purgePrev = g_purgeListTail;
	g_purgeListTail = p_item;
	p_item->m_purgeNext = NULL;
	if (!g_purgeListHead) {
		g_purgeListHead = p_item;
	}
}

// Locks an item and takes it off the purge list.
// The two list-end comparisons load their operands in the other order (the unit's symbol table).
// FUNCTION: MW2 0x10019b63
void LockCacheEntry(ResourceCacheEntry* p_item)
{
	if (p_item->m_lock == 1) {
		return;
	}

	p_item->m_lock = 1;
	if (p_item->m_purgeNext) {
		p_item->m_purgeNext->m_purgePrev = p_item->m_purgePrev;
	}

	if (p_item->m_purgePrev) {
		p_item->m_purgePrev->m_purgeNext = p_item->m_purgeNext;
	}

	if (g_purgeListHead == p_item) {
		g_purgeListHead = p_item->m_purgeNext;
	}

	if (g_purgeListTail == p_item) {
		g_purgeListTail = p_item->m_purgePrev;
	}

	p_item->m_purgeNext = NULL;
	p_item->m_purgePrev = NULL;
}

// FUNCTION: MW2 0x10019c0c
void AllocateCacheTable(void)
{
	g_cacheTable = MechHeapAllocZeroed(g_primaryHeap, 0x3f1 * sizeof(ResourceCacheEntry*));
}

// Rebuilds the purge list from the unlocked items.
// FUNCTION: MW2 0x10019c2f
void RebuildPurgeList(void)
{
	ResourceCacheEntry* item;
	MechS32 i;

	g_purgeListHead = NULL;
	g_purgeListTail = NULL;
	for (i = 0; i < 0x3f1; i++) {
		for (item = g_cacheTable[i]; item; item = item->m_next) {
			if (item->m_lock == 0) {
				item->m_lock = 1;
				UnlockCacheEntry(item);
			}
		}
	}
}

// Frees every cached item and the hash table.
// FUNCTION: MW2 0x10019cc2
void ShutdownResourceCache(void)
{
	ResourceCacheEntry* item;
	MechS32 i;
	ResourceCacheEntry* next;

	if (!g_cacheTable) {
		return;
	}

	for (i = 0; i < 0x3f1; i++) {
		for (item = g_cacheTable[i]; item; item = next) {
			next = item->m_next;
			FreeCacheEntry(item);
		}
	}

	g_cacheEntryCount = 0;
	g_purgeListHead = NULL;
	g_purgeListTail = NULL;
	MechHeapFree(g_primaryHeap, g_cacheTable);
}

// FUNCTION: MW2 0x10019d73
void InitializeResourceCache(void)
{
	g_cacheEntryCount = 0;
	g_purgeListHead = NULL;
	g_purgeListTail = NULL;
	AllocateCacheTable();
}

// FUNCTION: MW2 0x10019da1
void FUN_10019da1(void)
{
}

// Returns the cached item of an ID and type, or NULL.
// The only diff is a stack-slot permutation of item and slot.
// FUNCTION: MW2 0x10019dac
ResourceCacheEntry* FindCacheEntry(MechS32 p_id, const char* p_type)
{
	ResourceCacheEntry* item;
	MechS32 slot;

	if (p_id < 0) {
		return NULL;
	}

	slot = (p_type[2] + p_type[3] + p_type[0] + p_type[1] + p_id) % 0x3f1;
	for (item = g_cacheTable[slot]; item; item = item->m_next) {
		if (item->m_id == p_id && item->m_type == *(MechS32*) p_type) {
			break;
		}
	}

	return item;
}

// Frees an item. An item missing from its hash chain is reported instead.
// Stack-slot permutation of prev and slot; the original adds type[3] before type[2] (commutative
// operand order).
// FUNCTION: MW2 0x10019e53
void FreeCacheEntry(ResourceCacheEntry* p_item)
{
	ResourceCacheEntry* prev = NULL;
	MechChar type[5];
	MechS32 slot;
	MechChar text[100];

	if (!p_item) {
		return;
	}

	p_item->m_lock = 0;
	LockCacheEntry(p_item);
	type[4] = '\0';
	*(MechS32*) type = p_item->m_type;
	slot = (type[2] + type[3] + type[0] + type[1] + p_item->m_id) % 0x3f1;
	if (g_cacheTable[slot] == p_item) {
		g_cacheTable[slot] = p_item->m_next;
	}
	else {
		for (prev = g_cacheTable[slot]; prev && prev->m_next; prev = prev->m_next) {
			if (prev->m_next == p_item) {
				break;
			}
		}

		if (prev && prev->m_next) {
			prev->m_next = prev->m_next->m_next;
		}
		else {
			Error(0x20, "Freeing bad item (type: %s  id: %i)\n", type, p_item->m_id);
			if (g_purgeListHead == p_item) {
				RebuildPurgeList();
			}

			if (g_missionTimerStopped) {
				sprintf(text, "Freeing bad item (type: %s  id: %i)\n", type, p_item->m_id);
				ShowInGameMessage(text, 1, 0x712, 100);
			}

			return;
		}
	}

	MechHeapFree(g_primaryHeap, p_item);
	g_cacheEntryCount--;
}

// Writes the cache's hash chains and purge list to the next dbugcch<n>.log.
// Stack-slot permutation of i, type, item and name.
// FUNCTION: MW2 0x10019fef
void DumpResourceCache(void)
{
	MechS32 i;
	MechChar type[5];
	ResourceCacheEntry* item;
	MechChar name[100];

	snprintf(name, sizeof(name), "dbugcch%d.log", g_cacheDumpNumber++);
	MechLogDebugf("%s: Cache table", name);
	type[4] = '\0';
	for (i = 0; i < 0x3f1; i++) {
		for (item = g_cacheTable[i]; item; item = item->m_next) {
			*(MechS32*) type = item->m_type;
			MechLogDebugf(
				"%s: bucket=%d ID=%5d Type=%4s Lock=%d Size=%7d",
				name,
				i,
				item->m_id,
				type,
				item->m_lock,
				(MechS32) MechHeapSize(g_primaryHeap, item)
			);
		}
	}

	MechLogDebugf("%s: Purge list", name);
	for (item = g_purgeListHead; item; item = item->m_purgeNext) {
		*(MechS32*) type = item->m_type;
		MechLogDebugf(
			"%s: purge ID=%5d Type=%4s Lock=%d Size=%7d",
			name,
			item->m_id,
			type,
			item->m_lock,
			(MechS32) MechHeapSize(g_primaryHeap, item)
		);
	}
}

// FUNCTION: MW2 0x1001a158
void FUN_1001a158(void)
{
}

// FUNCTION: MW2 0x1001a163
void UnlockCachedResource(MechS32 p_id, const char* p_type)
{
	ResourceCacheEntry* entry;

	entry = FindCacheEntry(p_id, p_type);
	if (entry == NULL) {
		return;
	}

	UnlockCacheEntry(entry);
}

// Returns resource p_id of type p_type from project file p_file, locked in the cache: the cached
// copy, or one read in, purging unlocked items while the cache is full or memory runs out. A
// missing resource is logged to symlog.txt and reported. p_unk0x0c goes unused.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x1001a19f
void* LoadCachedResource(MechS32 p_file, MechS32 p_id, const char* p_type, undefined4 p_unk0x0c)
{
	ResourceCacheEntry* entry;
	ResourceCacheEntry* item;
	MechS32 hash;
	MechS32 size;
	MechChar message[100];
	MechChar message2[100];

	if (p_id < 0) {
		return NULL;
	}

	entry = FindCacheEntry(p_id, p_type);
	if (entry) {
		LockCacheEntry(entry);
		return entry + 1;
	}

	if (g_cacheEntryCount >= 1000) {
		if (g_purgeListHead) {
			FreeCacheEntry(g_purgeListHead);
		}
		else {
			if (g_missionTimerStopped) {
				ShowInGameMessage("CACHE FULL!!! Nothing to purge...", 1, 0x712, 100);
			}
			else {
				ShowInGameMessage("Memory running low (2) -- strange things may happen", 1, 0x712, 100);
			}

			return NULL;
		}
	}

	size = GetPrjResourceSize(p_file, p_type, p_id);
	if (size <= 0) {
		MechLogErrorf("symlog.txt: Couldn't load ID=%d Type=%s\n", p_id, p_type);

		Error(0x20, "Non-existant resource (type: %s  id: %i)\n", p_type, p_id);
		if (g_missionTimerStopped) {
			sprintf(message, "Non-existant resource (type: %s  id: %i)\n", p_type, p_id);
			ShowInGameMessage(message, 1, 0x712, 100);
		}

		return NULL;
	}

	while ((item = MechHeapAlloc(g_primaryHeap, size + sizeof(ResourceCacheEntry))) == NULL) {
		if (g_purgeListHead) {
			FreeCacheEntry(g_purgeListHead);
		}
		else {
			ShowInGameMessage("Memory running low (1) -- strange things may happen", 1, 0x712, 100);
			return NULL;
		}
	}

	if (ReadPrjResource(p_file, p_type, p_id, item + 1) == -1) {
		MechLogErrorf("symlog.txt: Couldn't load ID=%d Type=%s\n", p_id, p_type);

		Error(0x20, "Non-existant resource (type: %s  id: %i)\n", p_type, p_id);
		if (g_missionTimerStopped) {
			sprintf(message2, "Non-existant resource (type: %s  id: %i)\n", p_type, p_id);
			ShowInGameMessage(message2, 1, 0x712, 100);
		}

		return NULL;
	}

	hash = (p_type[2] + p_type[3] + p_type[0] + p_type[1] + p_id) % 0x3f1;
	item->m_next = g_cacheTable[hash];
	g_cacheTable[hash] = item;
	item->m_lock = 1;
	item->m_id = p_id;
	item->m_type = *(MechS32*) p_type;
	item->m_purgeNext = NULL;
	item->m_purgePrev = NULL;
	g_cacheEntryCount++;
	return item + 1;
}

// FUNCTION: MW2 0x1001a4e5
void FreeCachedResource(MechS32 p_id, const char* p_type)
{
	ResourceCacheEntry* entry;

	entry = FindCacheEntry(p_id, p_type);
	if (entry == NULL) {
		return;
	}

	FreeCacheEntry(entry);
}

// FUNCTION: MW2 0x1001a521
void FUN_1001a521(undefined4 p_unk0x00)
{
}

// FUNCTION: MW2 0x1001a52c
undefined4 FUN_1001a52c(undefined4 p_unk0x00)
{
	return p_unk0x00;
}

// FUNCTION: MW2 0x1001a53f
void* FUN_1001a53f(MechS32 p_id, const char* p_type)
{
	return LoadCachedResource(0, p_id, p_type, 0);
}

// FUNCTION: MW2 0x1001a563
MechS32 PurgeOldestCacheEntry(void)
{
	if (g_purgeListHead) {
		FreeCacheEntry(g_purgeListHead);
		return 1;
	}

	return 0;
}

// FUNCTION: MW2 0x1001a59a
void* MemAlloc(MechU32 p_size)
{
	return MechHeapAllocZeroed(g_primaryHeap, p_size);
}

// FUNCTION: MW2 0x1001a5bc
void* MemCopy(void* p_dst, const void* p_src, MechU32 p_size)
{
	/* Forwards, a dword at a time and then the remaining bytes, as rep movsd/movsb copy
	   overlapping blocks. */
	MechU8* dst = (MechU8*) p_dst;
	const MechU8* src = (const MechU8*) p_src;
	MechU8 dword[4];
	MechU32 count;

	for (count = p_size >> 2; count; count--) {
		memcpy(dword, src, 4);
		memcpy(dst, dword, 4);
		dst += 4;
		src += 4;
	}

	for (count = p_size & 3; count; count--) {
		*dst++ = *src++;
	}

	return p_dst;
}

// FUNCTION: MW2 0x1001a5e6
void* MemSet(void* p_dst, MechS32 p_value, MechU32 p_size)
{
	memset(p_dst, (MechU8) p_value, p_size);

	return p_dst;
}

// FUNCTION: MW2 0x1001a61e
void MemFree(void* p_block)
{
	MechHeapFree(g_primaryHeap, p_block);
}
