#ifndef RECT_H
#define RECT_H

#include "types.h"

// A rectangle without a pixel buffer: PANE's last four fields.
// SIZE 0x10
typedef struct Rect {
	MechS32 m_left;   // 0x00
	MechS32 m_top;    // 0x04
	MechS32 m_right;  // 0x08
	MechS32 m_bottom; // 0x0c
} Rect;

#endif // RECT_H
