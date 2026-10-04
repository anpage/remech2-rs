#ifndef XFORM_H
#define XFORM_H

#include "decomp.h"
#include "types.h"

// The transform the next block opens with (ApplyBlockXform) and a static object's placement:
// a scale (BeginBlock puts it on the diagonal as 2.29 fixed point), the rotation angles about x,
// y and z (16.16 degrees, BuildMatrix) and a translation.
// SIZE 0x24
typedef struct Xform {
	MechS32 m_scaleX; // 0x00
	MechS32 m_scaleY; // 0x04
	MechS32 m_scaleZ; // 0x08
	MechS32 m_angleX; // 0x0c
	MechS32 m_angleY; // 0x10
	MechS32 m_angleZ; // 0x14
	MechS32 m_x;      // 0x18
	MechS32 m_y;      // 0x1c
	MechS32 m_z;      // 0x20
} Xform;

#endif // XFORM_H
