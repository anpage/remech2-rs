#ifndef RECORDSTACKS_H
#define RECORDSTACKS_H

#include "depthsort.h"
#include "projectedvertex.h"
#include "types.h"

// The functions and globals of recordstacks.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_depthListCapacity;
	extern DepthEntry* g_drawList;
	extern MechU8* g_drawBufferBottom;
	extern DepthEntry* g_depthQueue;
	extern MechS32 g_queueHasRoom;
	extern MechS32 g_drawBufferSize;
	extern MechU8* g_drawBufferMemory;
	extern MechU8* g_drawBuffer;
	extern MechU8* g_drawBufferTop;

	void ShutdownDrawBuffer(void);
	void InitializeDrawBuffer(MechS32 p_kilobytes, MechS32 p_entries);
	void ResetDrawBuffer(void);
	ProjectedVertex* AllocProjectedVertex(void);
	MechU8* AllocQueuedPolygon(void);

#ifdef __cplusplus
}
#endif

#endif // RECORDSTACKS_H
