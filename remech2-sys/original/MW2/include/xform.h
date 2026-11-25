#ifndef XFORM_H
#define XFORM_H

#include "types.h"

// The transform the next block opens with (ApplyBlockXform) and a static object's placement:
// a scale (BeginBlock puts it on the diagonal as 2.29 fixed point), the rotation angles about x,
// y and z (16.16 degrees, BuildMatrix) and a translation.
typedef struct Xform {
	int32_t m_scaleX;
	int32_t m_scaleY;
	int32_t m_scaleZ;
	int32_t m_angleX;
	int32_t m_angleY;
	int32_t m_angleZ;
	int32_t m_x;
	int32_t m_y;
	int32_t m_z;
} Xform;

#endif // XFORM_H
