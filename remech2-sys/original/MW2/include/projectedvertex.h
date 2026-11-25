#ifndef PROJECTEDVERTEX_H
#define PROJECTEDVERTEX_H

#include "decomp.h"
#include "types.h"

// A vertex projected for drawing, one of AllocProjectedVertex's per-frame records: its view-space
// position, screen position, texture coordinates and clip outcodes.
typedef struct ProjectedVertex {
	int32_t m_x;         // 0x00
	int32_t m_y;         // 0x04
	int32_t m_z;         // 0x08 — the depth
	int32_t m_screenX;   // 0x0c
	int32_t m_screenY;   // 0x10
	int32_t m_u;         // 0x14 — 16.16
	int32_t m_v;         // 0x18
	uint8_t m_outcode;   // 0x1c — 1 left, 2 right, 4 top, 8 bottom
	uint8_t m_projected; // 0x1d — set once the screen position is computed
} ProjectedVertex;

#endif // PROJECTEDVERTEX_H
