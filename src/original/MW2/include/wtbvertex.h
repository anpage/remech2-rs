#ifndef WTBVERTEX_H
#define WTBVERTEX_H

#include "decomp.h"
#include "types.h"

// A vertex of a shape record (WtbHeader).
// SIZE 0x10
typedef struct WtbVertex {
	MechS32 m_x; // 0x00
	MechS32 m_y; // 0x04
	MechS32 m_z; // 0x08
	MechS16 m_u; // 0x0c
	MechS16 m_v; // 0x0e
} WtbVertex;

#endif // WTBVERTEX_H
