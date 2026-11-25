#ifndef VECTOR3_H
#define VECTOR3_H

#include "types.h"

// A point in the world.
typedef struct Vector3 {
	int32_t m_x; // 0x00
	int32_t m_y; // 0x04
	int32_t m_z; // 0x08
} Vector3;

#endif // VECTOR3_H
