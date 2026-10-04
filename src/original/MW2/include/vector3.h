#ifndef VECTOR3_H
#define VECTOR3_H

#include "types.h"

// A point in the world.
// SIZE 0xc
typedef struct Vector3 {
	MechS32 m_x; // 0x00
	MechS32 m_y; // 0x04
	MechS32 m_z; // 0x08
} Vector3;

#endif // VECTOR3_H
