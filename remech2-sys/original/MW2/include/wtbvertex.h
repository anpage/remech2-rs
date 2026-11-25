#ifndef WTBVERTEX_H
#define WTBVERTEX_H

#include "types.h"

// A vertex of a shape record (WtbHeader).
typedef struct WtbVertex {
	int32_t m_x; // 0x00
	int32_t m_y; // 0x04
	int32_t m_z; // 0x08
	int16_t m_u; // 0x0c
	int16_t m_v; // 0x0e
} WtbVertex;

#endif // WTBVERTEX_H
