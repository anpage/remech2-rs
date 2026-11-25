#include "staticmem.h"

#include "bwd.h"
#include "decomp.h"
#include "error.h"
#include "loadres.h"
#include "mw2log.h"
#include "poolsizes.h"
#include "simmain.h"
#include "types.h"

#include <stdlib.h>

int32_t g_staticPoolGroupCount = 0;

int32_t g_staticPoolCount = 0;

StaticPoolGroup g_staticPoolGroups[5];

StaticPool g_staticPools[32];

void ResetStaticPools(void);
int32_t ParseStaticPoolConfig(char* p_mission);
int CompareStaticPools(const void* p_a, const void* p_b);
int32_t AddStaticPoolType(int32_t p_size, uint32_t p_tag);

// Reads the mission's pool sizes, places the pools in groups of up to 0xffdc bytes, largest
// first, and allocates the groups. Returns whether it could read the sizes.
int32_t InitStaticMem(char* p_mission)
{
	int32_t i;
	StaticPool* pool;
	int32_t j;
	StaticPoolGroup* group;
	int32_t best;

	g_staticPoolGroupCount = 0;
	g_staticPoolCount = 0;
	ResetStaticPools();
	if (!ParseStaticPoolConfig(p_mission)) {
		Error(0x44, NULL);
		return 0;
	}

	qsort(g_staticPools, g_staticPoolCount, sizeof(StaticPool), CompareStaticPools);
	for (i = 0; i < g_staticPoolCount; i++) {
		pool = &g_staticPools[i];
		best = -1;
		for (j = 0; j < g_staticPoolGroupCount; j++) {
			if (g_staticPoolGroups[j].m_free >= pool->m_size) {
				if (best < 0) {
					best = j;
				}
				else if (g_staticPoolGroups[j].m_free < g_staticPoolGroups[best].m_free) {
					best = j;
				}
			}
		}

		if (best == -1) {
			if (g_staticPoolGroupCount >= 5) {
				Error(0x44, NULL);
			}

			group = &g_staticPoolGroups[g_staticPoolGroupCount];
			best = g_staticPoolGroupCount++;
			group->m_block = NULL;
			group->m_free = 0xffdc;
			group->m_size = group->m_free;
		}
		else {
			group = &g_staticPoolGroups[best];
		}

		pool->m_offset = group->m_size - group->m_free;
		pool->m_group = best;
		pool->m_used = 0;
		group->m_free -= pool->m_size;
	}

	for (i = 0; i < g_staticPoolGroupCount; i++) {
		group = &g_staticPoolGroups[i];
		group->m_size -= group->m_free;
		group->m_free = group->m_size;
		group->m_block = MemAlloc(group->m_size);
		if (group->m_block == NULL) {
			Error(0x45, NULL);
		}
	}

	return 1;
}

// Takes a block from the pool of a tag, logging the call to mw2.log.
void* StaticPoolAlloc(uint32_t p_size, uint32_t p_tag)
{
	int32_t i;
	StaticPool* pool;
	void* block;
	StaticPoolGroup* group;

	block = NULL;
	for (i = 0; i < g_staticPoolCount && g_staticPools[i].m_tag != p_tag; i++) {
	}

	if (g_staticPoolCount == i) {
		Error(0x46, NULL);
		WriteToMw2Log(" ");
		WriteToMw2Log(GetKeywordName(p_tag));
		return NULL;
	}

	pool = &g_staticPools[i];
	WriteToMw2Log("\nstatic_malloc called ");
	WriteToMw2Log(GetKeywordName(p_tag));
	if (pool->m_size - pool->m_used >= (int32_t) p_size) {
		group = &g_staticPoolGroups[pool->m_group];
		block = group->m_size - group->m_free + group->m_block;
		group->m_free = group->m_free - p_size;
		pool->m_used = pool->m_used + p_size;
	}
	else {
		Error(0x47, NULL);
		WriteToMw2Log(" ");
		WriteToMw2Log(GetKeywordName(p_tag));
	}

	return block;
}

// Frees the groups' blocks.
void FreeStaticMem(void)
{
	int32_t i;

	for (i = 0; i < g_staticPoolGroupCount; i++) {
		MechHeapFree(g_primaryHeap, g_staticPoolGroups[i].m_block);
		g_staticPoolGroups[i].m_block = NULL;
	}

	g_staticPoolGroupCount = 0;
	g_staticPoolCount = 0;
}

// Orders pools by size, largest first (for qsort).
int CompareStaticPools(const void* p_a, const void* p_b)
{
	StaticPool* a;
	int32_t difference;
	StaticPool* b;

	a = (StaticPool*) p_a;
	b = (StaticPool*) p_b;
	difference = b->m_size - a->m_size;
	if (difference < 0) {
		return -1;
	}

	if (difference > 0) {
		return 1;
	}

	return 0;
}

void ResetStaticPools(void)
{
}

// Adds the pool sizes of the mission's static memory table (seven tag and size pairs).
// Returns whether the mission has the table.
int32_t ParseStaticPoolConfig(char* p_mission)
{
	StaticPoolSize* entry;
	int32_t i;
	int32_t result;

	result = 1;
	entry = ReadStaticMemoryTable(p_mission);
	if (entry != NULL) {
		for (i = 0; i < 7; i++) {
			AddStaticPoolType(entry->m_size, entry->m_tag);
			entry++;
		}
	}
	else {
		result = 0;
	}

	return result;
}

// Adds room for a tag, making its pool on first use. Returns whether the pool still fits in a
// group.
int32_t AddStaticPoolType(int32_t p_size, uint32_t p_tag)
{
	StaticPool* pool;
	int32_t i;

	for (i = 0; i < g_staticPoolCount && g_staticPools[i].m_tag != p_tag; i++) {
	}

	pool = &g_staticPools[i];
	if (i == g_staticPoolCount) {
		pool->m_tag = p_tag;
		pool->m_size = 0;
		g_staticPoolCount++;
		if (g_staticPoolCount > 32) {
			Error(0x44, NULL);
		}
	}

	pool->m_size += p_size;
	if (pool->m_size > 0xffdc) {
		return FALSE;
	}

	return TRUE;
}
