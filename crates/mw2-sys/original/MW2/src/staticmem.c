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
#include <windows.h>

DECOMP_SIZE_ASSERT(StaticPool, 0x14)
DECOMP_SIZE_ASSERT(StaticPoolGroup, 0x0c)

// GLOBAL: MW2 0x100a6ddc
MechS32 g_staticPoolGroupCount = 0;

// GLOBAL: MW2 0x100a6de0
MechS32 g_staticPoolCount = 0;

// GLOBAL: MW2 0x100be5e0
StaticPoolGroup g_staticPoolGroups[5];

// GLOBAL: MW2 0x100be620
StaticPool g_staticPools[32];

void ResetStaticPools(void);
MechS32 ParseStaticPoolConfig(char* p_mission);
int CompareStaticPools(const void* p_a, const void* p_b);
MechS32 AddStaticPoolType(MechS32 p_size, MechU32 p_tag);

// Reads the mission's pool sizes, places the pools in groups of up to 0xffdc bytes, largest
// first, and allocates the groups. Returns whether it could read the sizes.
// Stack-slot permutation: i, pool, j, group and best. The loop tests and the comparisons of the
// groups' free room follow the unit's symbol table in the other operand order.
// FUNCTION: MW2 0x100498b0
MechS32 InitStaticMem(char* p_mission)
{
	MechS32 i;
	StaticPool* pool;
	MechS32 j;
	StaticPoolGroup* group;
	MechS32 best;

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
// Stack-slot permutation: i, pool, block and group.
// FUNCTION: MW2 0x10049afb
void* StaticPoolAlloc(MechU32 p_size, MechU32 p_tag)
{
	MechS32 i;
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
	if (pool->m_size - pool->m_used >= (MechS32) p_size) {
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
// The loop test compares in the other operand order (the unit's symbol table).
// FUNCTION: MW2 0x10049c55
void FreeStaticMem(void)
{
	MechS32 i;

	for (i = 0; i < g_staticPoolGroupCount; i++) {
		MechHeapFree(g_primaryHeap, g_staticPoolGroups[i].m_block);
		g_staticPoolGroups[i].m_block = NULL;
	}

	g_staticPoolGroupCount = 0;
	g_staticPoolCount = 0;
}

// Orders pools by size, largest first (for qsort).
// Stack-slot permutation: b and difference.
// FUNCTION: MW2 0x10049cc7
int CompareStaticPools(const void* p_a, const void* p_b)
{
	StaticPool* a;
	MechS32 difference;
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

// FUNCTION: MW2 0x10049d1f
void ResetStaticPools(void)
{
}

// Adds the pool sizes of the mission's static memory table (seven tag and size pairs).
// Returns whether the mission has the table.
// FUNCTION: MW2 0x10049d2a
MechS32 ParseStaticPoolConfig(char* p_mission)
{
	StaticPoolSize* entry;
	MechS32 i;
	MechS32 result;

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
// Stack-slot permutation: pool and i. Both tests against g_staticPoolCount compare in the other
// operand order (the unit's symbol table).
// FUNCTION: MW2 0x10049da3
MechS32 AddStaticPoolType(MechS32 p_size, MechU32 p_tag)
{
	StaticPool* pool;
	MechS32 i;

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
