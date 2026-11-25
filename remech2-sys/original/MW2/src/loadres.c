/* In the original, MemCopy and MemSet are C functions whose bodies are an __asm block (rep
   movsd/stosd). This is portable C in their place. */
#include "loadres.h"

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

// The resource cache's hash chains, 0x3f1 of them.
ResourceCacheEntry** g_cacheTable = NULL;

// The number of the next cache log.
int32_t g_cacheDumpNumber = 0;

int32_t g_cacheEntryCount;

// The purge list: unlocked items, oldest first.
ResourceCacheEntry* g_purgeListHead;

ResourceCacheEntry* g_purgeListTail;
// Unlocks an item and appends it to the purge list.
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

void AllocateCacheTable(void)
{
	g_cacheTable = MechHeapAllocZeroed(g_primaryHeap, 0x3f1 * sizeof(ResourceCacheEntry*));
}

// Rebuilds the purge list from the unlocked items.
void RebuildPurgeList(void)
{
	ResourceCacheEntry* item;
	int32_t i;

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
void ShutdownResourceCache(void)
{
	ResourceCacheEntry* item;
	int32_t i;
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

void InitializeResourceCache(void)
{
	g_cacheEntryCount = 0;
	g_purgeListHead = NULL;
	g_purgeListTail = NULL;
	AllocateCacheTable();
}

// Returns the cached item of an ID and type, or NULL.
ResourceCacheEntry* FindCacheEntry(int32_t p_id, const char* p_type)
{
	ResourceCacheEntry* item;
	int32_t slot;

	if (p_id < 0) {
		return NULL;
	}

	slot = (p_type[2] + p_type[3] + p_type[0] + p_type[1] + p_id) % 0x3f1;
	for (item = g_cacheTable[slot]; item; item = item->m_next) {
		if (item->m_id == p_id && item->m_type == *(int32_t*) p_type) {
			break;
		}
	}

	return item;
}

// Frees an item. An item missing from its hash chain is reported instead.
void FreeCacheEntry(ResourceCacheEntry* p_item)
{
	ResourceCacheEntry* prev = NULL;
	char type[5];
	int32_t slot;
	char text[100];

	if (!p_item) {
		return;
	}

	p_item->m_lock = 0;
	LockCacheEntry(p_item);
	type[4] = '\0';
	*(int32_t*) type = p_item->m_type;
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
void DumpResourceCache(void)
{
	int32_t i;
	char type[5];
	ResourceCacheEntry* item;
	char name[100];

	snprintf(name, sizeof(name), "dbugcch%d.log", g_cacheDumpNumber++);
	MechLogDebugf("%s: Cache table", name);
	type[4] = '\0';
	for (i = 0; i < 0x3f1; i++) {
		for (item = g_cacheTable[i]; item; item = item->m_next) {
			*(int32_t*) type = item->m_type;
			MechLogDebugf(
				"%s: bucket=%d ID=%5d Type=%4s Lock=%d Size=%7d",
				name,
				i,
				item->m_id,
				type,
				item->m_lock,
				(int32_t) MechHeapSize(g_primaryHeap, item)
			);
		}
	}

	MechLogDebugf("%s: Purge list", name);
	for (item = g_purgeListHead; item; item = item->m_purgeNext) {
		*(int32_t*) type = item->m_type;
		MechLogDebugf(
			"%s: purge ID=%5d Type=%4s Lock=%d Size=%7d",
			name,
			item->m_id,
			type,
			item->m_lock,
			(int32_t) MechHeapSize(g_primaryHeap, item)
		);
	}
}

void UnlockCachedResource(int32_t p_id, const char* p_type)
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
// missing resource is logged and reported.
void* LoadCachedResource(int32_t p_file, int32_t p_id, const char* p_type)
{
	ResourceCacheEntry* entry;
	ResourceCacheEntry* item;
	int32_t hash;
	int32_t size;
	char message[100];
	char message2[100];

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
	item->m_type = *(int32_t*) p_type;
	item->m_purgeNext = NULL;
	item->m_purgePrev = NULL;
	g_cacheEntryCount++;
	return item + 1;
}

void FreeCachedResource(int32_t p_id, const char* p_type)
{
	ResourceCacheEntry* entry;

	entry = FindCacheEntry(p_id, p_type);
	if (entry == NULL) {
		return;
	}

	FreeCacheEntry(entry);
}

int32_t PurgeOldestCacheEntry(void)
{
	if (g_purgeListHead) {
		FreeCacheEntry(g_purgeListHead);
		return 1;
	}

	return 0;
}

void* MemAlloc(uint32_t p_size)
{
	return MechHeapAllocZeroed(g_primaryHeap, p_size);
}

void* MemCopy(void* p_dst, const void* p_src, uint32_t p_size)
{
	/* Forwards, a dword at a time and then the remaining bytes, as rep movsd/movsb copy
	   overlapping blocks. */
	uint8_t* dst = (uint8_t*) p_dst;
	const uint8_t* src = (const uint8_t*) p_src;
	uint8_t dword[4];
	uint32_t count;

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

void* MemSet(void* p_dst, int32_t p_value, uint32_t p_size)
{
	memset(p_dst, (uint8_t) p_value, p_size);

	return p_dst;
}

void MemFree(void* p_block)
{
	MechHeapFree(g_primaryHeap, p_block);
}
