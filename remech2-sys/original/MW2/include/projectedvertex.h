#ifndef PROJECTEDVERTEX_H
#define PROJECTEDVERTEX_H

#include "types.h"

// A vertex projected for drawing, one of AllocProjectedVertex's per-frame records: its view-space
// position, screen position, texture coordinates and clip outcodes.
typedef struct ProjectedVertex {
	int32_t m_x;
	int32_t m_y;
	int32_t m_z; // the depth
	int32_t m_screenX;
	int32_t m_screenY;
	int32_t m_u; // 16.16
	int32_t m_v;
	uint8_t m_outcode;   // 1 left, 2 right, 4 top, 8 bottom
	uint8_t m_projected; // set once the screen position is computed
} ProjectedVertex;

#endif // PROJECTEDVERTEX_H
