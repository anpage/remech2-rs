#ifndef COCKPITFRAME_H
#define COCKPITFRAME_H

#include "types.h"

// A rectangle of the cockpit layout resource (CPIT) (LoadCptFile): its corner and size.
typedef struct CockpitFrame {
	int16_t m_x;
	int16_t m_y;
	int16_t m_width;
	int16_t m_height;
} CockpitFrame;

#endif // COCKPITFRAME_H
