#ifndef STATICBLOCK_H
#define STATICBLOCK_H

#include "transform.h"
#include "types.h"
#include "xform.h"

// A block of the static object cache (g_staticBlocks, 32 of them): the box BeginBlock
// opens, its center, the transform it was opened with, the matrix that places its objects
// and the enclosing block.
typedef struct StaticBlock {
	int32_t m_minX;
	int32_t m_minY;
	int32_t m_minZ;
	int32_t m_maxX;
	int32_t m_maxY;
	int32_t m_maxZ;
	int32_t m_centerX;
	int32_t m_centerY;
	int32_t m_centerZ;
	Xform m_xform;
	Matrix m_matrix;
	int32_t m_parent; // the enclosing block, -1: none
} StaticBlock;

#endif // STATICBLOCK_H
