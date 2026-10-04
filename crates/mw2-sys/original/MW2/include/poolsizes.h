#ifndef POOLSIZES_H
#define POOLSIZES_H

#include "types.h"

struct BwdStream;

// One entry of a mission's static memory table: a pool tag and its size.
// SIZE 0x08
typedef struct StaticPoolSize {
	MechU32 m_tag;  // 0x00
	MechU32 m_size; // 0x04
} StaticPoolSize;

// The functions and globals of poolsizes.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechU32 g_staticPoolTags[10];

	MechU32 GetStaticPoolSize(MechS32 p_index);
	StaticPoolSize* ReadStaticMemoryTable(char* p_mission);
	MechS32 CountMissionStream(struct BwdStream* p_stream);
	StaticPoolSize* BuildStaticMemoryTable(void);

#ifdef __cplusplus
}
#endif

#endif // POOLSIZES_H
