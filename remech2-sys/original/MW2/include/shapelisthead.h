#ifndef SHAPELISTHEAD_H
#define SHAPELISTHEAD_H

#include "types.h"

struct Shape;

// The head of a list of shapes: the first 0x18 bytes of a Shape, which the list
// code links and unlinks as if it were a shape (shapelists.c).
typedef struct ShapeListHead {
	uint16_t m_flags;             // 0x00
	uint16_t m_kind;              // 0x02
	struct Shape* m_prev;         // 0x04
	struct Shape* m_next;         // 0x08 — the first shape in the list
	struct Shape* m_prevCollider; // 0x0c
	struct Shape* m_nextCollider; // 0x10 — the first shape in the second list
	uint16_t m_owner;             // 0x14
	uint16_t m_partId;            // 0x16
} ShapeListHead;

#endif // SHAPELISTHEAD_H
