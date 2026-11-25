#ifndef MODEL_H
#define MODEL_H

#include "types.h"

/* One level of detail of a shape's model (the list at Shape::m_models): the header is followed
   by its vertices and faces (AddModel allocates it, TransformModel transforms it). */
typedef struct Model Model;

struct Model {
	int32_t m_key;             // 0x00 — the list is sorted by it, ascending
	int16_t m_vertexCount;     // 0x04
	int16_t m_faceCount;       // 0x06
	uint32_t m_faceOffset;     // 0x08 — offset of the faces
	Model* m_next;             // 0x0c
	uint32_t m_transformCount; // 0x10
};

#endif // MODEL_H
