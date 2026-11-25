#ifndef XFORM_H
#define XFORM_H

#include "types.h"

// The transform the next block opens with (ApplyBlockXform) and a static object's placement:
// a scale (BeginBlock puts it on the diagonal as 2.29 fixed point), the rotation angles about x,
// y and z (16.16 degrees, BuildMatrix) and a translation.
typedef struct Xform {
	int32_t m_scaleX; // 0x00
	int32_t m_scaleY; // 0x04
	int32_t m_scaleZ; // 0x08
	int32_t m_angleX; // 0x0c
	int32_t m_angleY; // 0x10
	int32_t m_angleZ; // 0x14
	int32_t m_x;      // 0x18
	int32_t m_y;      // 0x1c
	int32_t m_z;      // 0x20
} Xform;

#endif // XFORM_H
