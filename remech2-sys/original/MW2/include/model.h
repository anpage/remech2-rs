#ifndef MODEL_H
#define MODEL_H

#include "types.h"

/* One level of detail of a shape's model (the list at Shape::m_models): the header is followed
   by its vertices and faces (AddModel allocates it, TransformModel transforms it). */
typedef struct Model Model;

struct Model {
	int32_t m_key; // the list is sorted by it, ascending
	int16_t m_vertexCount;
	int16_t m_faceCount;
	uint32_t m_faceOffset; // offset of the faces
	Model* m_next;
	uint32_t m_transformCount;
};

#endif // MODEL_H
