#ifndef QUEUEDPOLYGON_H
#define QUEUEDPOLYGON_H

#include "types.h"

struct ProjectedVertex;
struct Face;

// A polygon queued for drawing, taken from the bottom of the draw buffer (AllocQueuedPolygon):
// m_count pointers to its projected vertices (struct ProjectedVertex*) follow the header.
typedef struct QueuedPolygon {
	int16_t m_count;  // bit 15 marks a shape queued in its place (DrawShapeList)
	uint16_t m_flags; // the drawing flags
	struct Face* m_face;
	int32_t m_depth;
} QueuedPolygon;

#endif // QUEUEDPOLYGON_H
