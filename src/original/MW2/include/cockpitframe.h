#ifndef COCKPITFRAME_H
#define COCKPITFRAME_H

#include "types.h"

// A rectangle of the cockpit layout resource (CPIT) (LoadCptFile): its corner and size.
// SIZE 0x8
typedef struct CockpitFrame {
	MechS16 m_x;      // 0x00
	MechS16 m_y;      // 0x02
	MechS16 m_width;  // 0x04
	MechS16 m_height; // 0x06
} CockpitFrame;

#endif // COCKPITFRAME_H
