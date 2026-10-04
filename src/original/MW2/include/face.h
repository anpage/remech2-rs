#ifndef FACE_H
#define FACE_H

#include "decomp.h"
#include "types.h"

struct Shape;

// A face of a model (Model): its vertex indices are bytes at m_indexOffset from
// the face itself, m_indexCount of them, and its plane's normal.
// SIZE 0x24
typedef struct Face {
	MechU16 m_color;       // 0x00 — bits 0-11: color or shade, 12-14: the draw mode (m_drawFace)
	MechU16 m_indexCount;  // 0x02
	MechU32 m_indexOffset; // 0x04 — from the face to its MechU8 vertex indices
	MechS32
		m_modelNormalX; // 0x08 — the normal in the model (ComputeFaceNormal); TransformModel rotates it into m_normal
	MechS32 m_modelNormalY; // 0x0c
	MechS32 m_modelNormalZ; // 0x10
	MechS32 m_normal[3];    // 0x14 — 2.29 fixed point
	struct Shape* m_shape;  // 0x20
} Face;

#endif // FACE_H
