#ifndef PROJECTEDVERTEX_H
#define PROJECTEDVERTEX_H

#include "decomp.h"
#include "types.h"

// A vertex projected for drawing, one of AllocProjectedVertex's per-frame records: its view-space
// position, screen position, texture coordinates and clip outcodes.
// SIZE 0x20
typedef struct ProjectedVertex {
	MechS32 m_x;                      // 0x00
	MechS32 m_y;                      // 0x04
	MechS32 m_z;                      // 0x08 — the depth
	MechS32 m_screenX;                // 0x0c
	MechS32 m_screenY;                // 0x10
	MechS32 m_u;                      // 0x14 — 16.16
	MechS32 m_v;                      // 0x18
	MechU8 m_outcode;                 // 0x1c — 1 left, 2 right, 4 top, 8 bottom
	MechU8 m_projected;               // 0x1d — set once the screen position is computed
	undefined m_unk0x1e[0x20 - 0x1e]; // 0x1e
} ProjectedVertex;

#endif // PROJECTEDVERTEX_H
