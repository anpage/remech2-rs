#ifndef MODEL_H
#define MODEL_H

#include "decomp.h"
#include "types.h"

/* One level of detail of a shape's model (the list at Shape::m_models): the header is followed
   by the 0x2c-byte vertices and the 0x24-byte faces (AddModel allocates it, TransformModel
   transforms it). */
typedef struct Model Model;

// SIZE 0x18
struct Model {
	MechS32 m_key;                    // 0x00 — the list is sorted by it, ascending
	MechS16 m_vertexCount;            // 0x04
	MechS16 m_faceCount;              // 0x06
	MechU32 m_faceOffset;             // 0x08 — offset of the faces
	Model* m_next;                    // 0x0c
	undefined4 m_transformCount;      // 0x10
	MechU16 m_unk0x14;                // 0x14
	undefined m_unk0x16[0x18 - 0x16]; // 0x16
};

#endif // MODEL_H
