#ifndef STATICMEM_H
#define STATICMEM_H

#include "types.h"

// A static pool: the room reserved for the blocks of one tag, inside one of the groups.
typedef struct StaticPool {
	uint32_t m_tag; // a keyword code
	int32_t m_size;
	int32_t m_used;
	int32_t m_group;  // the index in g_staticPoolGroups
	int32_t m_offset; // in the group's block
} StaticPool;

// One allocation that static pools share.
typedef struct StaticPoolGroup {
	uint8_t* m_block;
	int32_t m_size;
	int32_t m_free;
} StaticPoolGroup;

// The functions and globals of staticmem.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_staticPoolGroupCount;
	extern int32_t g_staticPoolCount;
	extern StaticPoolGroup g_staticPoolGroups[5];
	extern StaticPool g_staticPools[32];

	int32_t InitStaticMem(char* p_mission);
	void* StaticPoolAlloc(uint32_t p_size, uint32_t p_tag);
	void FreeStaticMem(void);

#ifdef __cplusplus
}
#endif

#endif // STATICMEM_H
