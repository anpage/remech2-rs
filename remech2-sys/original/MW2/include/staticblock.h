#ifndef STATICBLOCK_H
#define STATICBLOCK_H

#include "decomp.h"
#include "transform.h"
#include "types.h"
#include "xform.h"

// A block of the static object cache (g_staticBlocks, 32 of them): the box BeginBlock
// opens, its center, the transform it was opened with, the matrix that places its objects
// and the enclosing block.
typedef struct StaticBlock {
	int32_t m_minX;    // 0x00
	int32_t m_minY;    // 0x04
	int32_t m_minZ;    // 0x08
	int32_t m_maxX;    // 0x0c
	int32_t m_maxY;    // 0x10
	int32_t m_maxZ;    // 0x14
	int32_t m_centerX; // 0x18
	int32_t m_centerY; // 0x1c
	int32_t m_centerZ; // 0x20
	Xform m_xform;     // 0x24
	Matrix m_matrix;   // 0x48
	int32_t m_parent;  // 0x78 — the enclosing block, -1: none
} StaticBlock;

#endif // STATICBLOCK_H
