#include "resourcecache.h"

#include "log.h"
#include "prjfile.h"
#include "types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void FreeCacheEntry(ResourceCacheEntry* p_entry);

ResourceCacheEntry** g_cacheTable = NULL;

// The number of the next dump DumpResourceCache writes.
int32_t g_cacheDumpNumber = 0;

// The number of entries.
int32_t g_cacheEntryCount;

ResourceCacheEntry* g_purgeListHead;

ResourceCacheEntry* g_purgeListTail;

void UnlockCacheEntry(ResourceCacheEntry* p_entry)
{
	if (p_entry->m_lock == 0) {
		return;
	}

	p_entry->m_lock = 0;
	if (g_purgeListTail != NULL) {
		g_purgeListTail->m_purgeNext = p_entry;
	}
	p_entry->m_purgePrev = g_purgeListTail;
	g_purgeListTail = p_entry;
	p_entry->m_purgeNext = NULL;
	if (g_purgeListHead == NULL) {
		g_purgeListHead = p_entry;
	}
}

void LockCacheEntry(ResourceCacheEntry* p_entry)
{
	if (p_entry->m_lock == 1) {
		return;
	}

	p_entry->m_lock = 1;
	if (p_entry->m_purgeNext != NULL) {
		p_entry->m_purgeNext->m_purgePrev = p_entry->m_purgePrev;
	}
	if (p_entry->m_purgePrev != NULL) {
		p_entry->m_purgePrev->m_purgeNext = p_entry->m_purgeNext;
	}
	if (p_entry == g_purgeListHead) {
		g_purgeListHead = p_entry->m_purgeNext;
	}
	if (p_entry == g_purgeListTail) {
		g_purgeListTail = p_entry->m_purgePrev;
	}
	p_entry->m_purgeNext = NULL;
	p_entry->m_purgePrev = NULL;
}

void AllocateCacheTable(void)
{
	g_cacheTable = (ResourceCacheEntry**) calloc(0x3f1, sizeof(ResourceCacheEntry*));
}

void RebuildPurgeList(void)
{
	int32_t i;
	ResourceCacheEntry* entry;

	g_purgeListHead = NULL;
	g_purgeListTail = NULL;
	for (i = 0; i < 0x3f1; i++) {
		for (entry = g_cacheTable[i]; entry != NULL; entry = entry->m_next) {
			if (entry->m_lock == 0) {
				entry->m_lock = 1;
				UnlockCacheEntry(entry);
			}
		}
	}
}

void ShutdownResourceCache(void)
{
	int32_t i;
	ResourceCacheEntry* entry;
	ResourceCacheEntry* next;

	if (g_cacheTable == NULL) {
		return;
	}

	for (i = 0; i < 0x3f1; i++) {
		for (entry = g_cacheTable[i]; entry != NULL; entry = next) {
			next = entry->m_next;
			FreeCacheEntry(entry);
		}
	}
	g_cacheEntryCount = 0;
	g_purgeListHead = NULL;
	g_purgeListTail = NULL;
	free(g_cacheTable);
}

void InitializeResourceCache(void)
{
	g_cacheEntryCount = 0;
	g_purgeListHead = 0;
	g_purgeListTail = 0;
	AllocateCacheTable();
}

ResourceCacheEntry* FindCacheEntry(int32_t p_id, char* p_type)
{
	int32_t bucket;
	ResourceCacheEntry* entry;

	if (p_id < 0) {
		return NULL;
	}

	bucket = ((int8_t) p_type[3] + (int8_t) p_type[2] + (int8_t) p_type[0] + (int8_t) p_type[1] + p_id) % 0x3f1;
	for (entry = g_cacheTable[bucket]; entry != NULL; entry = entry->m_next) {
		if (entry->m_id == p_id && entry->m_type == *(uint32_t*) p_type) {
			break;
		}
	}

	return entry;
}

void FreeCacheEntry(ResourceCacheEntry* p_entry)
{
	int32_t bucket;
	ResourceCacheEntry* entry;
	char type[5];

	entry = NULL;
	if (p_entry == NULL) {
		return;
	}

	p_entry->m_lock = 0;
	LockCacheEntry(p_entry);
	type[4] = '\0';
	*(uint32_t*) type = p_entry->m_type;
	bucket = ((int8_t) type[2] + (int8_t) type[3] + (int8_t) type[0] + (int8_t) type[1] + p_entry->m_id) % 0x3f1;
	if (g_cacheTable[bucket] == p_entry) {
		g_cacheTable[bucket] = p_entry->m_next;
	}
	else {
		for (entry = g_cacheTable[bucket]; entry != NULL && entry->m_next != NULL; entry = entry->m_next) {
			if (entry->m_next == p_entry) {
				break;
			}
		}
		if (entry != NULL && entry->m_next != NULL) {
			entry->m_next = entry->m_next->m_next;
		}
		else {
			return;
		}
	}

	free(p_entry);
	g_cacheEntryCount--;
}

// Writes the cache to dbugcch<n>.log: every entry by bucket, then the purge list. The original also
// printed each entry's block size (_msize).
void DumpResourceCache(void)
{
	char name[100];
	ResourceCacheEntry* entry;
	char type[5];
	int32_t i;

	snprintf(name, sizeof(name), "dbugcch%d.log", g_cacheDumpNumber++);
	MechLogDebugf("%s: Cache table", name);
	type[4] = '\0';
	for (i = 0; i < 0x3f1; i++) {
		for (entry = g_cacheTable[i]; entry; entry = entry->m_next) {
			*(int32_t*) type = entry->m_type;
			MechLogDebugf("%s: bucket=%d ID=%5d Type=%4s Lock=%d", name, i, entry->m_id, type, entry->m_lock);
		}
	}

	MechLogDebugf("%s: Purge list", name);
	for (entry = g_purgeListHead; entry; entry = entry->m_purgeNext) {
		*(int32_t*) type = entry->m_type;
		MechLogDebugf("%s: purge ID=%5d Type=%4s Lock=%d", name, entry->m_id, type, entry->m_lock);
	}
}

void UnlockCachedResource(int32_t p_id, char* p_type)
{
	ResourceCacheEntry* entry = FindCacheEntry(p_id, p_type);
	if (entry == NULL) {
		return;
	}

	UnlockCacheEntry(entry);
}

// Returns the data of p_type item p_id of p_handle, loading it into the cache when needed and
// purging the least recently used entries to make room. p_unk0x0c is ignored (the name tables
// pass 1), so nothing gives it a name.
void* LoadCachedResource(int32_t p_handle, int32_t p_id, char* p_type, int32_t p_unk0x0c)
{
	ResourceCacheEntry* entry;
	ResourceCacheEntry* block;
	int32_t bucket;
	int32_t size;

	if (p_id < 0) {
		return NULL;
	}

	entry = FindCacheEntry(p_id, p_type);
	if (entry != NULL) {
		LockCacheEntry(entry);
		return entry + 1;
	}

	if (g_cacheEntryCount >= 1000) {
		if (g_purgeListHead != NULL) {
			FreeCacheEntry(g_purgeListHead);
		}
		else {
			return NULL;
		}
	}

	size = GetArchiveItemSize(p_handle, p_type, p_id);
	if (size <= 0) {
		MechLogErrorf("symlog.txt: Couldn't load ID=%d Type=%s\n", p_id, p_type);
		return NULL;
	}

	while ((block = (ResourceCacheEntry*) malloc(size + sizeof(ResourceCacheEntry))) == NULL) {
		if (g_purgeListHead != NULL) {
			FreeCacheEntry(g_purgeListHead);
		}
		else {
			return NULL;
		}
	}

	if (ReadArchiveItem(p_handle, p_type, p_id, block + 1) == -1) {
		MechLogErrorf("symlog.txt: Couldn't load ID=%d Type=%s\n", p_id, p_type);
		return NULL;
	}

	bucket = ((int8_t) p_type[2] + (int8_t) p_type[3] + (int8_t) p_type[0] + (int8_t) p_type[1] + p_id) % 0x3f1;
	block->m_next = g_cacheTable[bucket];
	g_cacheTable[bucket] = block;
	block->m_lock = 1;
	block->m_id = p_id;
	block->m_type = *(uint32_t*) p_type;
	block->m_purgeNext = NULL;
	block->m_purgePrev = NULL;
	g_cacheEntryCount++;
	return block + 1;
}

void FreeCachedResource(int32_t p_id, char* p_type)
{
	ResourceCacheEntry* entry = FindCacheEntry(p_id, p_type);
	if (entry == NULL) {
		return;
	}

	FreeCacheEntry(entry);
}

int32_t PurgeOldestCacheEntry(void)
{
	if (g_purgeListHead != NULL) {
		FreeCacheEntry(g_purgeListHead);
		return 1;
	}

	return 0;
}

// Zeroed, with calloc.
void* AllocateMemory(uint32_t p_size)
{
	return calloc(p_size, 1);
}

// Unused.
void FreeMemory(void* p_buffer)
{
	free(p_buffer);
}
