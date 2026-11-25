#ifndef WTBVERTEX_H
#define WTBVERTEX_H

#include "types.h"

// A vertex of a shape record (WtbHeader).
typedef struct WtbVertex {
	int32_t m_x;
	int32_t m_y;
	int32_t m_z;
	int16_t m_u;
	int16_t m_v;
} WtbVertex;

#endif // WTBVERTEX_H
