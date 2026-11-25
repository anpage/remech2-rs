#ifndef SHAPELISTHEAD_H
#define SHAPELISTHEAD_H

#include "types.h"

struct Shape;

// The head of a list of shapes: the first 0x18 bytes of a Shape, which the list
// code links and unlinks as if it were a shape (shapelists.c).
typedef struct ShapeListHead {
	uint16_t m_flags;
	uint16_t m_kind;
	struct Shape* m_prev;
	struct Shape* m_next; // the first shape in the list
	struct Shape* m_prevCollider;
	struct Shape* m_nextCollider; // the first shape in the second list
	uint16_t m_owner;
	uint16_t m_partId;
} ShapeListHead;

#endif // SHAPELISTHEAD_H
