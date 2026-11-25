#ifndef ANIMFRAME_H
#define ANIMFRAME_H

#include "types.h"

// One frame of an animation set: a CEL resource, loaded on first use.
typedef struct AnimFrame {
	int16_t m_resourceId; // 0x00 — -1 for a free slot
	int16_t m_useCount;   // 0x02
	uint16_t* m_data;     // 0x04 — width, height, then the pixels
} AnimFrame;

#endif // ANIMFRAME_H
