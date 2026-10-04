#ifndef STATICBLOCK_H
#define STATICBLOCK_H

#include "decomp.h"
#include "transform.h"
#include "types.h"
#include "xform.h"

// A block of the static object cache (g_staticBlocks, 32 of them): the box BeginBlock
// opens, its center, the transform it was opened with, the matrix that places its objects
// and the enclosing block.
// SIZE 0x7c
typedef struct StaticBlock {
	MechS32 m_minX;    // 0x00
	MechS32 m_minY;    // 0x04
	MechS32 m_minZ;    // 0x08
	MechS32 m_maxX;    // 0x0c
	MechS32 m_maxY;    // 0x10
	MechS32 m_maxZ;    // 0x14
	MechS32 m_centerX; // 0x18
	MechS32 m_centerY; // 0x1c
	MechS32 m_centerZ; // 0x20
	Xform m_xform;     // 0x24
	Matrix m_matrix;   // 0x48
	MechS32 m_parent;  // 0x78 — the enclosing block, -1: none
} StaticBlock;

#endif // STATICBLOCK_H
