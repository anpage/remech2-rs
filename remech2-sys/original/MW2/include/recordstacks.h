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

	extern int32_t g_depthListCapacity;
	extern DepthEntry* g_drawList;
	extern uint8_t* g_drawBufferBottom;
	extern DepthEntry* g_depthQueue;
	extern int32_t g_queueHasRoom;
	extern int32_t g_drawBufferSize;
	extern uint8_t* g_drawBufferMemory;
	extern uint8_t* g_drawBuffer;
	extern uint8_t* g_drawBufferTop;

	void ShutdownDrawBuffer(void);
	void InitializeDrawBuffer(int32_t p_kilobytes, int32_t p_entries);
	void ResetDrawBuffer(void);
	ProjectedVertex* AllocProjectedVertex(void);
	uint8_t* AllocQueuedPolygon(void);

#ifdef __cplusplus
}
#endif

#endif // RECORDSTACKS_H
