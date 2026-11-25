/* A buffer split into two stacks that grow towards each other: AllocProjectedVertex takes 0x20-byte
   records from the top, AllocQueuedPolygon QueuedPolygon records (0xc bytes in the original) from
   the bottom, and both clear g_queueHasRoom when the gap between them drops to 0xc8 bytes.

   In the original, AllocProjectedVertex and AllocQueuedPolygon are C functions whose bodies are
   mostly an __asm block. This is portable C in their place. */
#include "recordstacks.h"

#include "clock.h"
#include "decomp.h"
#include "depthsort.h"
#include "error.h"
#include "loadres.h"
#include "queuedpolygon.h"
#include "types.h"

MechS32 g_drawBufferSize = 0x80;

MechU8* g_drawBufferMemory = NULL;

MechS32 g_depthListCapacity;

MechU8* g_drawBuffer;

MechU8* g_drawBufferTop;

DepthEntry* g_drawList;

MechU8* g_drawBufferBottom;

DepthEntry* g_depthQueue;

MechS32 g_queueHasRoom;

void ShutdownDrawBuffer(void)
{
	if (g_drawBufferMemory != NULL) {
		MemFree(g_drawBufferMemory);
		g_drawBufferMemory = NULL;
	}
}

// The << 4 was two lists of 8-byte DepthEntry.
void InitializeDrawBuffer(MechS32 p_kilobytes, MechS32 p_entries)
{
	MechU32 size;

	g_depthListCapacity = p_entries;
	g_drawBufferSize = p_kilobytes << 10;
	size = (g_depthListCapacity * 2 * sizeof(DepthEntry)) + (p_kilobytes << 10);
	g_drawBufferMemory = MemAlloc(size);
	if (g_drawBufferMemory == NULL) {
		Error(0x18, NULL);
	}

	MemSet(g_drawBufferMemory, 0, size);
	g_drawBuffer = g_drawBufferMemory;
	g_drawList = (DepthEntry*) (g_drawBufferMemory + g_drawBufferSize);
	g_depthQueue = g_drawList + g_depthListCapacity;
	g_drawBufferBottom = g_drawBuffer;
	g_drawBufferTop = g_drawBuffer + (g_drawBufferSize << 5) - 0x600;
	InitSqrtTable();
	InitSinAtanTables();
	InitSlopeTables();
}

void ResetDrawBuffer(void)
{
	g_drawBufferBottom = g_drawBuffer;
	g_drawBufferTop = g_drawBuffer + g_drawBufferSize - 0x30;
}

ProjectedVertex* AllocProjectedVertex(void)
{
	MechS32 recordSize;
	ProjectedVertex* record;

	recordSize = sizeof(ProjectedVertex);
	/* The gap compares as the unsigned addresses do: the two stacks share one buffer. */
	g_drawBufferTop -= recordSize;
	record = (ProjectedVertex*) g_drawBufferTop;
	record->m_projected = 0;
	if (g_drawBufferTop - g_drawBufferBottom <= 0xc8) {
		g_queueHasRoom = 0;
	}

	return record;
}

MechU8* AllocQueuedPolygon(void)
{
	MechS32 recordSize;
	MechU8* record;

	recordSize = sizeof(QueuedPolygon);
	record = g_drawBufferBottom;
	g_drawBufferBottom += recordSize;
	if (g_drawBufferTop - g_drawBufferBottom <= 0xc8) {
		g_queueHasRoom = 0;
	}

	return record;
}
