#ifndef ANIMFRAME_H
#define ANIMFRAME_H

#include "types.h"

// One frame of an animation set: a CEL resource, loaded on first use.
// SIZE 0x08
typedef struct AnimFrame {
	MechS16 m_resourceId; // 0x00 — -1 for a free slot
	MechS16 m_useCount;   // 0x02
	MechU16* m_data;      // 0x04 — width, height, then the pixels
} AnimFrame;

#endif // ANIMFRAME_H
