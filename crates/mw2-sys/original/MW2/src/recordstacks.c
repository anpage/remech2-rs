/* A buffer split into two stacks that grow towards each other: AllocProjectedVertex takes 0x20-byte
   records from the top, AllocQueuedPolygon 0xc-byte records from the bottom, and both clear
   g_queueHasRoom when the gap between them drops to 0xc8 bytes.

   Hand-written assembly: AllocProjectedVertex and AllocQueuedPolygon are C functions whose bodies are
   mostly an __asm block (eax carries the new top across statements, which /Od never does). Their
   portable C (PORTABLE_C) is tested against the assembly by tests/asmequiv. */
#include "recordstacks.h"

#include "clock.h"
#include "compat.h"
#include "decomp.h"
#include "depthsort.h"
#include "error.h"
#include "loadres.h"
#include "types.h"

// GLOBAL: MW2 0x100ba5cc
MechS32 g_drawBufferSize = 0x80;

// GLOBAL: MW2 0x100ba5d0
MechU8* g_drawBufferMemory = NULL;

// GLOBAL: MW2 0x100c1a68
MechS32 g_depthListCapacity;

// GLOBAL: MW2 0x100c1a6c
MechU8* g_drawBuffer;

// GLOBAL: MW2 0x100c1a70
MechU8* g_drawBufferTop;

// GLOBAL: MW2 0x100c2280
DepthEntry* g_drawList;

// GLOBAL: MW2 0x100c2698
MechU8* g_drawBufferBottom;

// GLOBAL: MW2 0x100c269c
DepthEntry* g_depthQueue;

// GLOBAL: MW2 0x1010b5ac
MechS32 g_queueHasRoom;

// FUNCTION: MW2 0x1007d120
void ShutdownDrawBuffer(void)
{
	if (g_drawBufferMemory != NULL) {
		MemFree(g_drawBufferMemory);
		g_drawBufferMemory = NULL;
	}
}

// Operand order: the original computes g_depthListCapacity << 4 before p_kilobytes << 10 in size
// (both front ends reorder commutative operands).
// FUNCTION: MW2 0x1007d150
void InitializeDrawBuffer(MechS32 p_kilobytes, MechS32 p_entries)
{
	MechU32 size;

	g_depthListCapacity = p_entries;
	g_drawBufferSize = p_kilobytes << 10;
	size = (g_depthListCapacity << 4) + (p_kilobytes << 10);
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

// FUNCTION: MW2 0x1007d220
void ResetDrawBuffer(void)
{
	g_drawBufferBottom = g_drawBuffer;
	g_drawBufferTop = g_drawBuffer + g_drawBufferSize - 0x30;
}

// FUNCTION: MW2 0x1007d248
ProjectedVertex* AllocProjectedVertex(void)
{
	MechS32 recordSize;
	ProjectedVertex* record;

	recordSize = 0x20;
#ifdef PORTABLE_C
	/* The gap compares as the unsigned addresses do: the two stacks share one buffer. */
	g_drawBufferTop -= recordSize;
	record = (ProjectedVertex*) g_drawBufferTop;
	record->m_projected = 0;
	if (g_drawBufferTop - g_drawBufferBottom <= 0xc8) {
		g_queueHasRoom = 0;
	}
#else
	__asm {
		mov ecx, recordSize
		mov eax, g_drawBufferTop
		sub eax, ecx
		mov record, eax
		mov g_drawBufferTop, eax
		mov ebx, record
		mov byte ptr [ebx+0x1d], 0
		sub eax, 0xc8
		cmp eax, g_drawBufferBottom
		ja done
		xor eax, eax
		mov g_queueHasRoom, eax
done:
	}
#endif

	return record;
}

// FUNCTION: MW2 0x1007d296
MechU8* AllocQueuedPolygon(void)
{
	MechS32 recordSize;
	MechU8* record;

	recordSize = 0xc;
#ifdef PORTABLE_C
	record = g_drawBufferBottom;
	g_drawBufferBottom += recordSize;
	if (g_drawBufferTop - g_drawBufferBottom <= 0xc8) {
		g_queueHasRoom = 0;
	}
#else
	__asm {
		mov ecx, recordSize
		mov eax, g_drawBufferBottom
		mov record, eax
		add eax, ecx
		mov g_drawBufferBottom, eax
		add eax, 0xc8
		cmp eax, g_drawBufferTop
		jb done
		xor eax, eax
		mov g_queueHasRoom, eax
done:
	}
#endif

	return record;
}
