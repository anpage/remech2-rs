#ifndef QUEUEDPOLYGON_H
#define QUEUEDPOLYGON_H

#include "types.h"

struct ProjectedVertex;
struct Face;

// A polygon queued for drawing, taken from the bottom of the draw buffer (AllocQueuedPolygon):
// m_count pointers to its projected vertices (struct ProjectedVertex*) follow the header.
// SIZE 0xc
typedef struct QueuedPolygon {
	MechS16 m_count;     // 0x00 — bit 15 marks a shape queued in its place (DrawShapeList)
	MechU16 m_flags;     // 0x02 — the drawing flags
	struct Face* m_face; // 0x04
	MechS32 m_depth;     // 0x08
} QueuedPolygon;

#endif // QUEUEDPOLYGON_H
