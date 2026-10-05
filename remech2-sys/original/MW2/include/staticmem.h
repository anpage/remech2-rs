#ifndef STATICMEM_H
#define STATICMEM_H

#include "decomp.h"
#include "types.h"

// A static pool: the room reserved for the blocks of one tag, inside one of the groups.
// SIZE 0x14
typedef struct StaticPool {
	MechU32 m_tag;    // 0x00 — a keyword code
	MechS32 m_size;   // 0x04
	MechS32 m_used;   // 0x08
	MechS32 m_group;  // 0x0c — the index in g_staticPoolGroups
	MechS32 m_offset; // 0x10 — in the group's block
} StaticPool;

// One allocation that static pools share.
// SIZE 0x0c
typedef struct StaticPoolGroup {
	undefined* m_block; // 0x00
	MechS32 m_size;     // 0x04
	MechS32 m_free;     // 0x08
} StaticPoolGroup;

// The functions and globals of staticmem.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_staticPoolGroupCount;
	extern MechS32 g_staticPoolCount;
	extern StaticPoolGroup g_staticPoolGroups[5];
	extern StaticPool g_staticPools[32];

	MechS32 InitStaticMem(char* p_mission);
	void* StaticPoolAlloc(MechU32 p_size, MechU32 p_tag);
	void FreeStaticMem(void);

#ifdef __cplusplus
}
#endif

#endif // STATICMEM_H
