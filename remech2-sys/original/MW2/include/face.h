#ifndef FACE_H
#define FACE_H

#include "types.h"

struct Shape;

// A face of a model (Model): its vertex indices are bytes at m_indexOffset from
// the face itself, m_indexCount of them, and its plane's normal.
typedef struct Face {
	uint16_t m_color;       // 0x00 — bits 0-11: color or shade, 12-14: the draw mode (m_drawFace)
	uint16_t m_indexCount;  // 0x02
	uint32_t m_indexOffset; // 0x04 — from the face to its uint8_t vertex indices
	int32_t
		m_modelNormalX; // 0x08 — the normal in the model (ComputeFaceNormal); TransformModel rotates it into m_normal
	int32_t m_modelNormalY; // 0x0c
	int32_t m_modelNormalZ; // 0x10
	int32_t m_normal[3];    // 0x14 — 2.29 fixed point
	struct Shape* m_shape;  // 0x20
} Face;

#endif // FACE_H
