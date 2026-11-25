#ifndef POOLSIZES_H
#define POOLSIZES_H

#include "types.h"

struct BwdStream;

// One entry of a mission's static memory table: a pool tag and its size.
typedef struct StaticPoolSize {
	uint32_t m_tag;  // 0x00
	uint32_t m_size; // 0x04
} StaticPoolSize;

// The functions and globals of poolsizes.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern uint32_t g_staticPoolTags[10];
	extern int16_t g_missionPlayers;
	extern int16_t g_missionObjects;
	extern int16_t g_missionClassEntries;
	extern int16_t g_missionAnims;
	extern int16_t g_missionAnimTracks;
	extern int32_t g_missionAnimFrameBytes;
	extern StaticPoolSize g_staticPoolSizes[10];

	uint32_t GetStaticPoolSize(int32_t p_index);
	StaticPoolSize* ReadStaticMemoryTable(char* p_mission);
	int32_t CountMissionStream(struct BwdStream* p_stream);
	StaticPoolSize* BuildStaticMemoryTable(void);

#ifdef __cplusplus
}
#endif

#endif // POOLSIZES_H
