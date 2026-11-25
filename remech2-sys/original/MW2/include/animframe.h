#ifndef ANIMFRAME_H
#define ANIMFRAME_H

#include "types.h"

// One frame of an animation set: a CEL resource, loaded on first use.
typedef struct AnimFrame {
	int16_t m_resourceId; // -1 for a free slot
	int16_t m_useCount;
	uint16_t* m_data; // width, height, then the pixels
} AnimFrame;

#endif // ANIMFRAME_H
