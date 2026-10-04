#ifndef VERTEX_H
#define VERTEX_H

#include "decomp.h"
#include "types.h"

struct ProjectedVertex;

// A vertex of a model (Model): its position in the model, the position
// TransformModel transforms it to, and two more values AddShapeVertex sets.
// SIZE 0x2c
typedef struct Vertex {
	MechS32 m_modelX;                     // 0x00 — the position in the model
	MechS32 m_modelY;                     // 0x04
	MechS32 m_modelZ;                     // 0x08
	MechS32 m_worldX;                     // 0x0c — the position TransformModel computes
	MechS32 m_worldY;                     // 0x10
	MechS32 m_worldZ;                     // 0x14
	undefined4 m_u;                       // 0x18 — texture coordinates
	undefined4 m_v;                       // 0x1c
	undefined4 m_depth;                   // 0x20 — the view-space depth
	struct ProjectedVertex* m_projection; // 0x24 — the projected copy (GetViewVertex), once made
	MechU8 m_flags; // 0x28 — bit 0: nearer than the near plane, 1: farther than the far plane, 2: depth computed this
					// frame
	undefined m_unk0x29[0x2c - 0x29]; // 0x29
} Vertex;

#endif // VERTEX_H
