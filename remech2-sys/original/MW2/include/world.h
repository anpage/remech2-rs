#ifndef WORLD_H
#define WORLD_H

#include "types.h"

struct BwdStream;

// The functions and globals of world.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern char g_musicName[12];
	extern int32_t g_musicResource;
	extern int32_t g_loadHudFile;
	extern int32_t g_nextShotRecord;
	extern int32_t g_nextEffectRecord;
	uint32_t WidenEventFlags(uint32_t p_flags);
	int32_t BwdExecuteStream(struct BwdStream* p_stream);
	int32_t LoadWorld(char* p_name);
	void AfterWorldLoader(void);

#ifdef __cplusplus
}
#endif

#endif // WORLD_H
