#ifndef COCKPITFRAME_H
#define COCKPITFRAME_H

#include "types.h"

// A rectangle of the cockpit layout resource (CPIT) (LoadCptFile): its corner and size.
typedef struct CockpitFrame {
	int16_t m_x;      // 0x00
	int16_t m_y;      // 0x02
	int16_t m_width;  // 0x04
	int16_t m_height; // 0x06
} CockpitFrame;

#endif // COCKPITFRAME_H
