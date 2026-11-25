#ifndef VERTEX_H
#define VERTEX_H

#include "types.h"

struct ProjectedVertex;

// A vertex of a model (Model): its position in the model, the position
// TransformModel transforms it to, and two more values AddShapeVertex sets.
typedef struct Vertex {
	int32_t m_modelX; // the position in the model
	int32_t m_modelY;
	int32_t m_modelZ;
	int32_t m_worldX; // the position TransformModel computes
	int32_t m_worldY;
	int32_t m_worldZ;
	uint32_t m_u; // texture coordinates
	uint32_t m_v;
	uint32_t m_depth;                     // the view-space depth
	struct ProjectedVertex* m_projection; // the projected copy (GetViewVertex), once made
	uint8_t m_flags; // bit 0: nearer than the near plane, 1: farther than the far plane, 2: depth computed this
					 // frame
} Vertex;

#endif // VERTEX_H
